#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <array>
#include <functional>

#include "OtaStatus.h"
#include "TelemetryData.h"

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
    String language;
    int8_t rotation = -1;
    int8_t autoRotate = -1;
    int16_t pageInterval = -1;
    int16_t pageMask = -1;
    std::array<uint8_t, 5> pageOrder = {0, 1, 2, 3, 4};
    bool hasPageOrder = false;
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

  struct PageSetup {
    std::array<uint8_t, 5> order = {0, 1, 2, 3, 4};
    uint8_t mask = 0x0F;
    String language = "vi";
    String updatedAt;
    bool available = false;
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
    String pagesUpdatedAt;
    bool pagesAvailable = false;
    bool reportingEnabled = false;
    bool lastReportOk = false;
    uint32_t lastReportMs = 0;
    uint32_t lastCheckMs = 0;
    uint32_t nextCheckMs = 0;
  };

  explicit CloudFleetClient(OtaStatus& otaStatus);

  void begin();
  void loop();
  void reportTelemetry(const TelemetryData& telemetry, bool pcConnected,
                       uint32_t freeHeap, uint8_t rotation,
                       const char* language);
  bool checkNow();
  bool installAvailableUpdate(std::function<void()> frameCallback = nullptr);
  Status status() const;
  size_t effectCount() const;
  const EffectPreset* effectAt(size_t index) const;
  const EffectPreset* findEffect(const String& id) const;
  void selectEffect(const String& id);
  PageSetup pageSetup() const;

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
  String telemetryUrl_;
  uint32_t lastReportMs_ = 0;
  bool lastReportOk_ = false;
  std::array<uint8_t, 5> cloudPageOrder_ = {0, 1, 2, 3, 4};
  uint8_t cloudPageMask_ = 0x0F;
  String cloudPageLanguage_ = "vi";
  String pagesUpdatedAt_;
  bool pagesAvailable_ = false;

  bool fetchControlManifest();
  bool fetchDeviceRegistry();
  bool fetchFirmwareManifest();
  bool fetchEffects();
  bool fetchPages();
  bool getJson(const String& name, JsonDocument& output, String& error);
  void buildIdentity();
  static int compareVersions(const String& left, const String& right);
};
