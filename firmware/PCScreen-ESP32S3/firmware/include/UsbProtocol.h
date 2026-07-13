#pragma once

#include <Arduino.h>

#include "BacklightController.h"
#include "TelemetryData.h"

class UsbProtocol final {
 public:
  UsbProtocol(Stream& stream, TelemetryData& telemetry,
              BacklightController& backlight);
  void loop();

 private:
  Stream& stream_;
  TelemetryData& telemetry_;
  BacklightController& backlight_;
  String line_;
  void processLine(const String& line);
  void replyStatus(const char* type, bool ok, const char* error = nullptr);
};

