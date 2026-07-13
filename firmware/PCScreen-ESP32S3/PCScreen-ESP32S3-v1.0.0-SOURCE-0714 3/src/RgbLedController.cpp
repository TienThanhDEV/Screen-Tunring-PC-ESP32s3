#include "RgbLedController.h"

#include <cmath>

#include "AppConfig.h"

RgbLedController::RgbLedController(uint16_t count, int16_t pin)
    : pixel_(count, pin, NEO_GRB + NEO_KHZ800) {}

bool RgbLedController::begin() {
  pixel_.begin();
  if (!preferences_.begin("board_rgb", false)) return false;
  enabled_ = preferences_.getBool("enabled", false);
  red_ = preferences_.getUChar("red", 0);
  green_ = preferences_.getUChar("green", 160);
  blue_ = preferences_.getUChar("blue", 255);
  brightness_ = constrain(preferences_.getUChar("bright", 25), 0,
                          AppConfig::RGB_MAX_SAFE_BRIGHTNESS);
  effect_ = static_cast<Effect>(constrain(preferences_.getUChar("effect", 0), 0, 4));
  speed_ = constrain(preferences_.getUChar("speed", 50), 1, 100);
  apply(red_, green_, blue_, brightness_);
  return true;
}

void RgbLedController::setEnabled(bool enabled) {
  enabled_ = enabled;
  markDirty();
}

void RgbLedController::setColor(uint8_t red, uint8_t green, uint8_t blue) {
  red_ = red;
  green_ = green;
  blue_ = blue;
  markDirty();
}

void RgbLedController::setBrightness(uint8_t percent) {
  brightness_ = constrain(percent, 0, AppConfig::RGB_MAX_SAFE_BRIGHTNESS);
  markDirty();
}

void RgbLedController::setEffect(Effect effect) { effect_ = effect; markDirty(); }
void RgbLedController::setSpeed(uint8_t speed) { speed_ = constrain(speed, 1, 100); markDirty(); }

RgbLedController::State RgbLedController::state() const {
  return {enabled_, red_, green_, blue_, brightness_, effect_, speed_};
}

void RgbLedController::loop(const TelemetryData& telemetry) {
  const uint32_t now = millis();
  if (dirty_ && now - changedAtMs_ >= AppConfig::SETTINGS_COMMIT_DELAY_MS) save();
  if (now - lastFrameMs_ < AppConfig::RGB_EFFECT_FRAME_MS) return;
  lastFrameMs_ = now;

  uint8_t red = red_, green = green_, blue = blue_, brightness = brightness_;
  const float phase = static_cast<float>((now * (speed_ + 10U) / 20U) % 2000U) / 2000.0f;
  if (effect_ == Effect::Pulse) {
    brightness = static_cast<uint8_t>(brightness_ * (0.2f + 0.8f * (0.5f - 0.5f * cosf(phase * 6.283185f))));
  } else if (effect_ == Effect::Rainbow) {
    const uint32_t color = wheel(static_cast<uint8_t>(phase * 255.0f));
    red = color >> 16; green = color >> 8; blue = color;
  } else if (effect_ == Effect::Temperature) {
    const float temp = !isnan(telemetry.gpuTemperature) ? telemetry.gpuTemperature : telemetry.cpuTemperature;
    const float value = isnan(temp) ? 0.0f : constrain((temp - 35.0f) / 55.0f, 0.0f, 1.0f);
    red = static_cast<uint8_t>(255.0f * value);
    green = static_cast<uint8_t>(255.0f * (1.0f - fabsf(value * 2.0f - 1.0f)));
    blue = static_cast<uint8_t>(255.0f * (1.0f - value));
  } else if (effect_ == Effect::Load) {
    const float load = !isnan(telemetry.gpuLoad) ? telemetry.gpuLoad : telemetry.cpuLoad;
    const float value = isnan(load) ? 0.0f : constrain(load / 100.0f, 0.0f, 1.0f);
    red = static_cast<uint8_t>(255.0f * value);
    green = static_cast<uint8_t>(220.0f * (1.0f - value));
    blue = 32;
  }
  apply(red, green, blue, brightness);
}

void RgbLedController::apply(uint8_t red, uint8_t green, uint8_t blue,
                             uint8_t brightness) {
  if (!enabled_) brightness = 0;
  if (red == lastOutputRed_ && green == lastOutputGreen_ &&
      blue == lastOutputBlue_ && brightness == lastOutputBrightness_) return;
  lastOutputRed_ = red; lastOutputGreen_ = green; lastOutputBlue_ = blue;
  lastOutputBrightness_ = brightness;
  pixel_.setBrightness(static_cast<uint8_t>(brightness * 255U / 100U));
  pixel_.setPixelColor(0, brightness > 0 ? pixel_.Color(red, green, blue) : 0);
  pixel_.show();
}

void RgbLedController::markDirty() { dirty_ = true; changedAtMs_ = millis(); }

void RgbLedController::save() {
  preferences_.putBool("enabled", enabled_);
  preferences_.putUChar("red", red_);
  preferences_.putUChar("green", green_);
  preferences_.putUChar("blue", blue_);
  preferences_.putUChar("bright", brightness_);
  preferences_.putUChar("effect", static_cast<uint8_t>(effect_));
  preferences_.putUChar("speed", speed_);
  dirty_ = false;
}

uint32_t RgbLedController::wheel(uint8_t position) {
  position = 255 - position;
  if (position < 85) return (static_cast<uint32_t>(255 - position * 3) << 16) | (position * 3);
  if (position < 170) { position -= 85; return (static_cast<uint32_t>(position * 3) << 8) | (255 - position * 3); }
  position -= 170;
  return (static_cast<uint32_t>(position * 3) << 16) | (static_cast<uint32_t>(255 - position * 3) << 8);
}
