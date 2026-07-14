#pragma once

#include <Arduino.h>

#include "BacklightController.h"
#include "TelemetryData.h"
#include "UiSettings.h"

class UsbProtocol final {
 public:
  UsbProtocol(Stream& stream, TelemetryData& telemetry,
              BacklightController& backlight, UiSettings& settings);
  void loop();

 private:
  Stream& stream_;
  TelemetryData& telemetry_;
  BacklightController& backlight_;
  UiSettings& settings_;
  String line_;
  void processLine(const String& line);
  void replyStatus(const char* type, bool ok, const char* error = nullptr);
};
