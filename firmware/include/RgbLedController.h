#pragma once

#include <Adafruit_NeoPixel.h>
#include <Preferences.h>

#include "TelemetryData.h"

class RgbLedController final {
 public:
  enum class Effect : uint8_t { Solid = 0, Pulse = 1, Rainbow = 2, Temperature = 3, Load = 4 };

  struct State {
    bool enabled;
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t brightness;
    Effect effect;
    uint8_t speed;
  };

  RgbLedController(uint16_t count, int16_t pin);
  bool begin();
  void setEnabled(bool enabled);
  void setColor(uint8_t red, uint8_t green, uint8_t blue);
  void setBrightness(uint8_t percent);
  void setEffect(Effect effect);
  void setSpeed(uint8_t speed);
  void loop(const TelemetryData& telemetry);
  State state() const;

 private:
  Adafruit_NeoPixel pixel_;
  Preferences preferences_;
  bool enabled_ = false;
  uint8_t red_ = 0;
  uint8_t green_ = 160;
  uint8_t blue_ = 255;
  uint8_t brightness_ = 25;
  Effect effect_ = Effect::Solid;
  uint8_t speed_ = 50;
  bool dirty_ = false;
  uint32_t changedAtMs_ = 0;
  uint32_t lastFrameMs_ = 0;
  uint8_t lastOutputRed_ = 255;
  uint8_t lastOutputGreen_ = 255;
  uint8_t lastOutputBlue_ = 255;
  uint8_t lastOutputBrightness_ = 255;
  void apply(uint8_t red, uint8_t green, uint8_t blue, uint8_t brightness);
  void markDirty();
  void save();
  static uint32_t wheel(uint8_t position);
};
