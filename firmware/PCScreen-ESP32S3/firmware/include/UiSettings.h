#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include <array>

class UiSettings final {
 public:
  enum class Theme : uint8_t { Dark = 0, Light = 1 };

  struct State {
    Theme theme;
    uint8_t rotation;
    bool autoRotate;
    uint16_t pageIntervalSeconds;
    uint8_t pageMask;
    uint8_t bootDurationSeconds;
    uint8_t logoPageDurationSeconds;
    String dashboardTitle;
    bool showDate;
    uint8_t cardRadius;
    std::array<uint16_t, 6> cardColors;
  };

  bool begin();
  void loop();
  State state() const;
  void setTheme(Theme theme);
  void setRotation(uint8_t rotation);
  void setAutoRotate(bool enabled);
  void setPageInterval(uint16_t seconds);
  void setPageMask(uint8_t mask);
  void setBootDuration(uint8_t seconds);
  void setLogoPageDuration(uint8_t seconds);
  void setDashboardTitle(const String& title);
  void setShowDate(bool enabled);
  void setCardRadius(uint8_t radius);
  void setCardColor(uint8_t index, uint16_t color);

 private:
  Preferences preferences_;
  Theme theme_ = Theme::Dark;
  uint8_t rotation_ = 0;
  bool autoRotate_ = true;
  uint16_t pageIntervalSeconds_ = 5;
  uint8_t pageMask_ = 0x0F;
  uint8_t bootDurationSeconds_ = 4;
  uint8_t logoPageDurationSeconds_ = 5;
  String dashboardTitle_ = "PC SCREEN";
  bool showDate_ = true;
  uint8_t cardRadius_ = 10;
  std::array<uint16_t, 6> cardColors_ = {
      0x04DF, 0x05EC, 0xF31F, 0xFB04, 0xBFC4, 0x7A7F};
  bool dirty_ = false;
  uint32_t changedAtMs_ = 0;
  void markDirty();
  void save();
};
