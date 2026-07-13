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
UsbProtocol usb(Serial, telemetry, backlight);
WebConfigServer web(backlight, rgb, telemetry, uiSettings, otaStatus, network,
                    cloud);
bool displayReady = false;

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
  } else if (displayReady) {
    bootAsset.play(uiSettings.state().bootDurationSeconds * 1000UL);
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
  if (now - lastRender >= AppConfig::DISPLAY_REFRESH_MS) {
    lastRender = now;
    if (displayReady) {
      display.render(telemetry,
                     telemetry.isFresh(now, AppConfig::TELEMETRY_STALE_MS),
                     web.address(), uiSettings.state(), network.clock(), now);
    }
  }
}
