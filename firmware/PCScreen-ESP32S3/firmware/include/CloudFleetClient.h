#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <array>
#include <functional>

#include "OtaStatus.h"

class CloudFleetClient final {
 public:
  static constexpr size_t MAX_EFFECTS = 8;

  struct EffectPreset {
    String id;
    String name;
    String type;
    std::array<String, 3> palette;
    uint8_t paletteSize = 0;
    uint8_t speed = 50;
    uint8_t brightness = 50;
    bool enabled = true;
    String theme;
    int8_t rotation = -1;
    int8_t autoRotate = -1;
    int16_t pageInterval = -1;
    int16_t pageMask = -1;
  };

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
    size_t effectCount = 0;
    String selectedEffectId;
    String effectsUpdatedAt;
    uint32_t lastCheckMs = 0;
    uint32_t nextCheckMs = 0;
  };

  explicit CloudFleetClient(OtaStatus& otaStatus);

  void begin();
  void loop();
  bool checkNow();
  bool installAvailableUpdate(std::function<void()> frameCallback = nullptr);
  Status status() const;
  size_t effectCount() const;
  const EffectPreset* effectAt(size_t index) const;
  const EffectPreset* findEffect(const String& id) const;
  void selectEffect(const String& id);

 private:
  OtaStatus& otaStatus_;
  Preferences preferences_;
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
  std::array<EffectPreset, MAX_EFFECTS> effects_;
  size_t effectCount_ = 0;
  String effectsUpdatedAt_;
  String selectedEffectId_;

  bool fetchControlManifest();
  bool fetchDeviceRegistry();
  bool fetchFirmwareManifest();
  bool fetchEffects();
  bool getJson(const String& name, JsonDocument& output, String& error);
  void buildIdentity();
  static int compareVersions(const String& left, const String& right);
};
