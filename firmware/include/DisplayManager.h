#pragma once

#include "BootAssetManager.h"
#include "DisplayDevice.h"
#include "NetworkTimeManager.h"
#include "OtaStatus.h"
#include "TelemetryData.h"
#include "UiSettings.h"

class DisplayManager final {
 public:
  DisplayManager(DisplayDevice& display, BootAssetManager& bootAsset,
                 const OtaStatus& otaStatus);
  bool begin();
  void setRotation(uint8_t rotation);
  void showBoot();
  void showProvisioning(UiSettings::Language language, const String& apSsid,
                        const String& apPassword, const String& address);
  void showWelcome(UiSettings::Language language, uint32_t now);
  void showOtaAnimation(uint32_t now);
  bool isTransitioning() const { return transitionActive_; }
  void render(const TelemetryData& data, bool connected, const String& address,
              const UiSettings::State& settings,
              const NetworkTimeManager::ClockSnapshot& clock, uint32_t now);

 private:
  DisplayDevice& display_;
  BootAssetManager& bootAsset_;
  const OtaStatus& otaStatus_;
  lgfx::LGFX_Sprite canvas_;
  uint8_t currentPage_ = 0;
  uint8_t activePage_ = 0xFF;
  uint8_t rotation_ = 0;
  UiSettings::Language language_ = UiSettings::Language::Vietnamese;
  bool logoAvailable_ = false;
  uint32_t lastPageChangeMs_ = 0;
  uint32_t transitionStartedMs_ = 0;
  bool transitionActive_ = false;
  float fontScale_ = 1.0f;
  bool headerBold_ = true;
  bool labelBold_ = true;
  bool valueBold_ = true;
  bool customTextColors_ = false;
  uint16_t primaryTextColor_ = TFT_WHITE;
  uint16_t secondaryTextColor_ = 0x9CF3;
  UiSettings::TransitionStyle transitionStyle_ =
      UiSettings::TransitionStyle::Slide;
  uint16_t transitionDurationMs_ = 420;
  TelemetryData displayData_;
  uint32_t lastSmoothingMs_ = 0;

  void header(const char* title, bool connected, bool light);
  void footer(const String& address, uint8_t mask, bool light);
  void bentoDashboard(const TelemetryData& data, bool connected,
                      const UiSettings::State& settings,
                      const NetworkTimeManager::ClockSnapshot& clock);
  void bentoCard(int x, int y, const char* label, const String& value,
                 uint16_t color, uint8_t radius);
  void temperatures(const TelemetryData& data, bool light);
  void cooling(const TelemetryData& data, bool light);
  void performance(const TelemetryData& data, bool light);
  void logoMissing();
  void renderOta(uint32_t now);
  void dataCard(int x, int y, int width, int height, const char* label,
                const String& value, const String& detail, float progress,
                uint16_t accent);
  void ringGauge(int centerX, const char* label, float rpm, float temperature,
                 uint16_t accent);
  void miniStat(int x, int y, const char* label, const String& value,
                uint16_t accent);
  void present(uint32_t now);
  const TelemetryData& smoothTelemetry(const TelemetryData& source,
                                       bool enabled, uint32_t now);
  void advancePage(uint8_t mask, const std::array<uint8_t, 5>& order);
  static String number(float value, uint8_t decimals = 0);
  static uint16_t cardTextColor(uint16_t background);
  uint16_t primaryText(bool light) const;
  uint16_t secondaryText(bool light) const;
};
