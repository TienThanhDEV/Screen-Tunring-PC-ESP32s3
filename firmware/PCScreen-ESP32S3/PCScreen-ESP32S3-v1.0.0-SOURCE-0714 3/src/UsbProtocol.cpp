#include "UsbProtocol.h"

#include <ArduinoJson.h>

#include "AppConfig.h"

UsbProtocol::UsbProtocol(Stream& stream, TelemetryData& telemetry,
                         BacklightController& backlight)
    : stream_(stream), telemetry_(telemetry), backlight_(backlight) {
  line_.reserve(384);
}

void UsbProtocol::loop() {
  while (stream_.available()) {
    const char c = static_cast<char>(stream_.read());
    if (c == '\n') {
      if (!line_.isEmpty()) processLine(line_);
      line_ = "";
    } else if (c != '\r' && line_.length() < 512) {
      line_ += c;
    } else if (line_.length() >= 512) {
      line_ = "";
      replyStatus("error", false, "line_too_long");
    }
  }
}

void UsbProtocol::processLine(const String& line) {
  JsonDocument doc;
  if (deserializeJson(doc, line)) {
    replyStatus("error", false, "invalid_json");
    return;
  }
  const char* type = doc["type"] | "";
  if (!strcmp(type, "hello")) {
    JsonDocument response;
    response["type"] = "hello";
    response["device"] = "PCSCREEN-S3";
    response["firmware"] = AppConfig::FIRMWARE_VERSION;
    response["protocol"] = 2;
    serializeJson(response, stream_);
    stream_.println();
    return;
  }
  if (!strcmp(type, "telemetry")) {
    telemetry_.cpuTemperature = doc["cpuTemp"] | NAN;
    telemetry_.cpuLoad = doc["cpuLoad"] | NAN;
    telemetry_.cpuClockMHz = doc["cpuClockMHz"] | NAN;
    telemetry_.cpuPowerWatts = doc["cpuPowerWatts"] | NAN;
    telemetry_.cpuFanRpm = doc["cpuFanRpm"] | NAN;
    telemetry_.gpuTemperature = doc["gpuTemp"] | NAN;
    telemetry_.gpuLoad = doc["gpuLoad"] | NAN;
    telemetry_.gpuClockMHz = doc["gpuClockMHz"] | NAN;
    telemetry_.gpuPowerWatts = doc["gpuPowerWatts"] | NAN;
    telemetry_.gpuFanRpm = doc["gpuFanRpm"] | NAN;
    telemetry_.gpuMemoryLoad = doc["gpuMemoryLoad"] | NAN;
    telemetry_.memoryLoad = doc["ramLoad"] | NAN;
    telemetry_.memoryUsedGb = doc["memoryUsedGb"] | NAN;
    telemetry_.memoryTotalGb = doc["memoryTotalGb"] | NAN;
    telemetry_.fps = doc["fps"] | NAN;
    telemetry_.receivedAtMs = millis();
    replyStatus("telemetry", true);
    return;
  }
  if (!strcmp(type, "display")) {
    const char* command = doc["command"] | "";
    if (!strcmp(command, "backlight") && doc["enabled"].is<bool>()) {
      backlight_.setEnabled(doc["enabled"].as<bool>());
      replyStatus("display", true);
      return;
    }
    if (!strcmp(command, "brightness") && doc["value"].is<int>()) {
      const int value = doc["value"].as<int>();
      if (value >= 0 && value <= 100) {
        backlight_.setBrightness(static_cast<uint8_t>(value));
        replyStatus("display", true);
        return;
      }
    }
    replyStatus("display", false, "invalid_command");
    return;
  }
  replyStatus("error", false, "unknown_type");
}

void UsbProtocol::replyStatus(const char* type, bool ok, const char* error) {
  JsonDocument response;
  response["type"] = type;
  response["ok"] = ok;
  if (error) response["error"] = error;
  serializeJson(response, stream_);
  stream_.println();
}
