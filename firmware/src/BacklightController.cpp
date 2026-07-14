#include "BacklightController.h"

#include <esp_arduino_version.h>

#include "AppConfig.h"

BacklightController::BacklightController(uint8_t pin, bool activeHigh,
                                         uint32_t frequency,
                                         uint8_t resolutionBits,
                                         uint8_t legacyChannel)
    : pin_(pin),
      activeHigh_(activeHigh),
      frequency_(frequency),
      resolutionBits_(resolutionBits),
      legacyChannel_(legacyChannel) {}

bool BacklightController::begin(uint8_t defaultBrightness) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  if (!ledcAttach(pin_, frequency_, resolutionBits_)) return false;
#else
  ledcSetup(legacyChannel_, frequency_, resolutionBits_);
  ledcAttachPin(pin_, legacyChannel_);
#endif

  if (!preferences_.begin("display", false)) return false;
  enabled_ = preferences_.getBool("bl_on", true);
  brightness_ = constrain(preferences_.getUChar("bl_pct", defaultBrightness), 0, 100);
  apply();
  return true;
}

void BacklightController::setEnabled(bool enabled) {
  if (enabled_ == enabled) return;
  enabled_ = enabled;
  apply();
  markDirty();
}

void BacklightController::setBrightness(uint8_t percent) {
  percent = constrain(percent, 0, 100);
  if (brightness_ == percent && (percent == 0 || enabled_)) return;
  brightness_ = percent;
  enabled_ = percent > 0;
  apply();
  markDirty();
}

BacklightController::State BacklightController::state() const {
  return {enabled_, brightness_};
}

void BacklightController::loop() {
  if (dirty_ && millis() - changedAtMs_ >= AppConfig::SETTINGS_COMMIT_DELAY_MS) {
    saveNow();
  }
}

void BacklightController::saveNow() {
  preferences_.putBool("bl_on", enabled_);
  preferences_.putUChar("bl_pct", brightness_);
  dirty_ = false;
}

uint32_t BacklightController::maxDuty() const {
  return (1UL << resolutionBits_) - 1UL;
}

void BacklightController::apply() {
  uint32_t duty = enabled_ ? (maxDuty() * brightness_ / 100U) : 0;
  if (!activeHigh_) duty = maxDuty() - duty;
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(pin_, duty);
#else
  ledcWrite(legacyChannel_, duty);
#endif
}

void BacklightController::markDirty() {
  dirty_ = true;
  changedAtMs_ = millis();
}

