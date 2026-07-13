#pragma once

#include <Arduino.h>

struct OtaStatus {
  enum class Phase : uint8_t { Idle, Receiving, Verifying, Success, Failed };
  Phase phase = Phase::Idle;
  uint8_t progress = 0;
  uint32_t changedAtMs = 0;

  void set(Phase next, uint8_t percent = 0) {
    phase = next;
    progress = constrain(percent, 0, 100);
    changedAtMs = millis();
  }

  bool visible(uint32_t now) const {
    if (phase == Phase::Idle) return false;
    if (phase == Phase::Failed) return now - changedAtMs < 4000;
    return true;
  }
};
