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
  void showOtaAnimation(uint32_t now);
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
  bool logoAvailable_ = false;
  uint32_t lastPageChangeMs_ = 0;

  void header(const char* title, bool connected, bool light);
  void footer(const String& address, uint8_t mask, bool light);
  void overview(const TelemetryData& data, bool light);
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
  void metric(const char* name, float primary, const char* unit, float load,
              int y, uint32_t color, bool light);
  void advancePage(uint8_t mask);
  static String number(float value, uint8_t decimals = 0);
  static uint16_t cardTextColor(uint16_t background);
};
