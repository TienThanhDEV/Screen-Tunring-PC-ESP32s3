#include <Arduino.h>

#include "AppConfig.h"
#include "BacklightController.h"
#include "BoardConfig.h"
#include "BootAssetManager.h"
#include "CloudFleetClient.h"
#include "DisplayDevice.h"
#include "DisplayManager.h"
#include "NetworkTimeManager.h"
#include "OtaStatus.h"
#include "RgbLedController.h"
#include "TelemetryData.h"
#include "UsbProtocol.h"
#include "UiSettings.h"
#include "WebConfigServer.h"

DisplayDevice displayDevice;
OtaStatus otaStatus;
BootAssetManager bootAsset(displayDevice);
DisplayManager display(displayDevice, bootAsset, otaStatus);
TelemetryData telemetry;
UiSettings uiSettings;
NetworkTimeManager network;
CloudFleetClient cloud(otaStatus);
BacklightController backlight(
    BoardConfig::TFT_BL, BoardConfig::BL_ACTIVE_HIGH, BoardConfig::BL_PWM_HZ,
    BoardConfig::BL_PWM_BITS, BoardConfig::BL_PWM_CHANNEL);
RgbLedController rgb(BoardConfig::RGB_LED_COUNT, BoardConfig::RGB_LED_PIN);
UsbProtocol usb(Serial, telemetry, backlight, uiSettings);
WebConfigServer web(backlight, rgb, telemetry, uiSettings, otaStatus, network,
                    cloud);
bool displayReady = false;
bool bootPlaybackActive = false;
uint32_t bootPlaybackEndsAtMs = 0;
enum class StartupState : uint8_t { NormalBoot, Provisioning, Welcome, Running };
StartupState startupState = StartupState::NormalBoot;
uint32_t welcomeEndsAtMs = 0;

void setup() {
  Serial.begin(115200);
  delay(250);

  if (!backlight.begin(AppConfig::DEFAULT_BRIGHTNESS)) {
    Serial.println("{\"type\":\"boot\",\"error\":\"backlight\"}");
  }
  if (!rgb.begin()) {
    Serial.println("{\"type\":\"boot\",\"error\":\"rgb\"}");
  }
  if (!uiSettings.begin()) {
    Serial.println("{\"type\":\"boot\",\"error\":\"ui_settings\"}");
  }
  if (!network.begin()) {
    Serial.println("{\"type\":\"boot\",\"error\":\"network\"}");
  }
  cloud.begin();
  displayReady = display.begin();
  if (!displayReady) {
    Serial.println("{\"type\":\"boot\",\"error\":\"display\"}");
  } else {
    display.setRotation(uiSettings.state().rotation);
    display.showBoot();
  }
  web.setOtaFrameCallback([]() {
    if (displayReady) display.showOtaAnimation(millis());
  });
  if (!web.begin()) {
    Serial.println("{\"type\":\"boot\",\"error\":\"web\"}");
  }
  if (displayReady) {
    if (network.provisioningRequired()) {
      startupState = StartupState::Provisioning;
    } else {
      bootPlaybackActive = bootAsset.beginPlayback();
      bootPlaybackEndsAtMs =
          millis() + uiSettings.state().bootDurationSeconds * 1000UL;
      startupState = bootPlaybackActive ? StartupState::NormalBoot
                                        : StartupState::Running;
    }
  }
}

void loop() {
  usb.loop();
  web.loop();
  network.loop();
  cloud.loop();
  backlight.loop();
  uiSettings.loop();
  rgb.loop(telemetry);

  static uint32_t lastRender = 0;
  const uint32_t now = millis();
  const auto uiState = uiSettings.state();
  if (startupState == StartupState::Provisioning &&
      !network.provisioningRequired()) {
    startupState = StartupState::Welcome;
    welcomeEndsAtMs = now + AppConfig::FIRST_BOOT_WELCOME_MS;
  }
  if (startupState == StartupState::Welcome &&
      static_cast<int32_t>(now - welcomeEndsAtMs) >= 0) {
    startupState = StartupState::Running;
  }
  if (bootPlaybackActive) {
    bootAsset.updatePlayback(now);
    if (!bootAsset.isPlaying() ||
        static_cast<int32_t>(now - bootPlaybackEndsAtMs) >= 0) {
      bootAsset.endPlayback();
      bootPlaybackActive = false;
      startupState = StartupState::Running;
    }
  }
  const uint32_t telemetryTimeoutMs =
      max<uint32_t>(AppConfig::TELEMETRY_STALE_MS,
          static_cast<uint32_t>(uiState.telemetryIntervalMs) * 3UL);
  const bool pcConnected = uiState.telemetryPaused
                               ? telemetry.hasData()
                               : telemetry.isFresh(now, telemetryTimeoutMs);
  cloud.reportTelemetry(
      telemetry, pcConnected, ESP.getFreeHeap(), uiState.rotation,
      uiState.language == UiSettings::Language::English ? "en" : "vi");
  const uint32_t renderInterval = display.isTransitioning()
                                      ? AppConfig::DISPLAY_TRANSITION_FRAME_MS
                                      : AppConfig::DISPLAY_REFRESH_MS;
  if (now - lastRender >= renderInterval) {
    lastRender = now;
    if (displayReady && otaStatus.visible(now)) {
      display.showOtaAnimation(now);
    } else if (displayReady && startupState == StartupState::Provisioning) {
      display.showProvisioning(uiState.language, AppConfig::AP_SSID,
                               AppConfig::AP_PASSWORD,
                               network.displayAddress());
    } else if (displayReady && startupState == StartupState::Welcome) {
      display.showWelcome(uiState.language, now);
    } else if (displayReady && !bootPlaybackActive) {
      display.render(telemetry, pcConnected,
                     web.address(), uiState, network.clock(), now);
    }
  }
}
