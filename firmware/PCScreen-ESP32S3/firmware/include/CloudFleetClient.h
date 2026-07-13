#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <functional>

#include "OtaStatus.h"

class CloudFleetClient final {
 public:
  struct FirmwareInfo {
    String version;
    String minimumVersion;
    String url;
    String sha256;
    size_t size = 0;
    bool mandatory = false;
    bool updateAvailable = false;
  };

  struct Status {
    String deviceId;
    String mac;
    String serverUrl;
    String currentVersion;
    String latestVersion;
    String lastError;
    String channel;
    bool internetConnected = false;
    bool serverReachable = false;
    bool registered = false;
    bool deviceEnabled = true;
    bool autoProvision = true;
    bool updateAvailable = false;
    bool mandatory = false;
    uint32_t lastCheckMs = 0;
    uint32_t nextCheckMs = 0;
  };

  explicit CloudFleetClient(OtaStatus& otaStatus);

  void begin();
  void loop();
  bool checkNow();
  bool installAvailableUpdate(std::function<void()> frameCallback = nullptr);
  Status status() const;

 private:
  OtaStatus& otaStatus_;
  FirmwareInfo firmware_;
  String deviceId_;
  String mac_;
  String lastError_;
  String channel_ = "stable";
  bool serverReachable_ = false;
  bool registered_ = false;
  bool deviceEnabled_ = true;
  bool autoProvision_ = true;
  bool checking_ = false;
  bool updating_ = false;
  uint32_t startedAtMs_ = 0;
  uint32_t lastCheckMs_ = 0;
  uint32_t pollIntervalMs_ = 15UL * 60UL * 1000UL;

  bool fetchControlManifest();
  bool fetchDeviceRegistry();
  bool fetchFirmwareManifest();
  bool getJson(const String& name, JsonDocument& output, String& error);
  void buildIdentity();
  static int compareVersions(const String& left, const String& right);
};
