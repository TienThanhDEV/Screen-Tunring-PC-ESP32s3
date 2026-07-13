#pragma once

#include <Arduino.h>

struct TelemetryData {
  float cpuTemperature = NAN;
  float cpuLoad = NAN;
  float cpuClockMHz = NAN;
  float cpuPowerWatts = NAN;
  float cpuFanRpm = NAN;
  float gpuTemperature = NAN;
  float gpuLoad = NAN;
  float gpuClockMHz = NAN;
  float gpuPowerWatts = NAN;
  float gpuFanRpm = NAN;
  float gpuMemoryLoad = NAN;
  float memoryLoad = NAN;
  float memoryUsedGb = NAN;
  float memoryTotalGb = NAN;
  float fps = NAN;
  uint32_t receivedAtMs = 0;

  bool hasData() const { return receivedAtMs != 0; }
  bool isFresh(uint32_t now, uint32_t timeoutMs) const {
    return hasData() && (now - receivedAtMs) <= timeoutMs;
  }
};
