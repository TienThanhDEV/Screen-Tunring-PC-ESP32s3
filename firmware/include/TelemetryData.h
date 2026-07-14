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
  float diskLoad = NAN;
  float networkDownMbps = NAN;
  float networkUpMbps = NAN;
  float loadAverage = NAN;
  uint32_t systemUptimeSeconds = 0;
  uint16_t processCount = 0;
  uint8_t cpuCoreCount = 0;
  uint32_t receivedAtMs = 0;
  uint32_t packetCount = 0;

  bool hasData() const { return receivedAtMs != 0; }
  bool isFresh(uint32_t now, uint32_t timeoutMs) const {
    return hasData() && (now - receivedAtMs) <= timeoutMs;
  }
};
