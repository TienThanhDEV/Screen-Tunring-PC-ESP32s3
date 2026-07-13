#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

struct CloudFirmwareInfo {
  String version;
  String minimumVersion;
  String url;
  String sha256;
  size_t size = 0;
  bool mandatory = false;
  bool updateAvailable = false;
};

class CloudDataClient final {
 public:
  explicit CloudDataClient(String baseUrl);

  void setCurrentVersion(const String& version);
  void setPollInterval(uint32_t intervalMs);
  bool shouldPoll(uint32_t nowMs) const;

  bool fetchEffects(JsonDocument& output, String& error);
  bool fetchFirmware(CloudFirmwareInfo& output, String& error);

 private:
  String baseUrl_;
  String currentVersion_ = "0.0.0";
  uint32_t pollIntervalMs_ = 15UL * 60UL * 1000UL;
  uint32_t lastPollMs_ = 0;

  bool getJson(const String& path, JsonDocument& output, String& error);
  static int compareVersions(const String& left, const String& right);
};
