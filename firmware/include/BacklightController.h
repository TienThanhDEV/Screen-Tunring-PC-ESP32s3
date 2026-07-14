#pragma once

#include <Arduino.h>
#include <Preferences.h>

class BacklightController final {
 public:
  struct State {
    bool enabled;
    uint8_t brightness;
  };

  BacklightController(uint8_t pin, bool activeHigh, uint32_t frequency,
                      uint8_t resolutionBits, uint8_t legacyChannel);
  bool begin(uint8_t defaultBrightness);
  void setEnabled(bool enabled);
  void setBrightness(uint8_t percent);
  State state() const;
  void loop();
  void saveNow();

 private:
  uint8_t pin_;
  bool activeHigh_;
  uint32_t frequency_;
  uint8_t resolutionBits_;
  uint8_t legacyChannel_;
  bool enabled_ = true;
  uint8_t brightness_ = 70;
  bool dirty_ = false;
  uint32_t changedAtMs_ = 0;
  Preferences preferences_;

  uint32_t maxDuty() const;
  void apply();
  void markDirty();
};

