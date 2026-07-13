#include "CloudFleetClient.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <Update.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <esp_mac.h>
#include <mbedtls/sha256.h>

#include "AppConfig.h"

namespace {
constexpr uint8_t kOtaMagic[8] = {'P', 'C', 'S', 'O', 'T', 'A', '1', 0};
constexpr size_t kOtaHeaderSize = 16;
constexpr uint32_t kOtaFormatVersion = 1;
constexpr size_t kReadBufferSize = 4096;
constexpr uint32_t kReadTimeoutMs = 15000;

uint32_t readUint32Le(const uint8_t* data) {
  return static_cast<uint32_t>(data[0]) |
         (static_cast<uint32_t>(data[1]) << 8U) |
         (static_cast<uint32_t>(data[2]) << 16U) |
         (static_cast<uint32_t>(data[3]) << 24U);
}

String digestToHex(const uint8_t digest[32]) {
  static constexpr char kHex[] = "0123456789abcdef";
  char output[65];
  for (size_t index = 0; index < 32; ++index) {
    output[index * 2] = kHex[digest[index] >> 4U];
    output[index * 2 + 1] = kHex[digest[index] & 0x0FU];
  }
  output[64] = '\0';
  return String(output);
}

bool readExactly(Stream& stream, uint8_t* output, size_t length) {
  size_t received = 0;
  const uint32_t started = millis();
  while (received < length && millis() - started < kReadTimeoutMs) {
    const int available = stream.available();
    if (available <= 0) {
      delay(1);
      continue;
    }
    const size_t chunk = min(length - received, static_cast<size_t>(available));
    const size_t count = stream.readBytes(output + received, chunk);
    if (count == 0) continue;
    received += count;
  }
  return received == length;
}
}  // namespace

CloudFleetClient::CloudFleetClient(OtaStatus& otaStatus)
    : otaStatus_(otaStatus) {}

void CloudFleetClient::begin() {
  buildIdentity();
  if (preferences_.begin("cloud", false)) {
    selectedEffectId_ = preferences_.getString("effect", "");
  }
  startedAtMs_ = millis();
  pollIntervalMs_ = AppConfig::CLOUD_DEFAULT_POLL_MS;
}

void CloudFleetClient::loop() {
  if (checking_ || updating_ || WiFi.status() != WL_CONNECTED) return;
  const uint32_t now = millis();
  if (lastCheckMs_ == 0) {
    if (now - startedAtMs_ < AppConfig::CLOUD_FIRST_CHECK_DELAY_MS) return;
  } else if (now - lastCheckMs_ < pollIntervalMs_) {
    return;
  }
  checkNow();
}

bool CloudFleetClient::checkNow() {
  if (checking_ || updating_) {
    lastError_ = "busy";
    return false;
  }
  if (WiFi.status() != WL_CONNECTED) {
    serverReachable_ = false;
    lastError_ = "wifi_not_connected";
    return false;
  }

  checking_ = true;
  lastError_.clear();
  serverReachable_ = false;
  registered_ = false;
  deviceEnabled_ = true;

  const bool controlOk = fetchControlManifest();
  const bool registryOk = controlOk && fetchDeviceRegistry();
  const bool firmwareOk = controlOk && fetchFirmwareManifest();
  const bool effectsOk = controlOk && fetchEffects();
  const bool pagesOk = controlOk && fetchPages();
  serverReachable_ = controlOk && registryOk && firmwareOk && effectsOk && pagesOk;
  lastCheckMs_ = millis();
  checking_ = false;
  return serverReachable_;
}

bool CloudFleetClient::fetchPages() {
  JsonDocument document;
  String error;
  if (!getJson("pages.json", document, error)) {
    lastError_ = error;
    return false;
  }
  if (document["schemaVersion"].as<int>() != 1 ||
      !document["pages"].is<JsonArray>()) {
    lastError_ = "pages_manifest_invalid";
    return false;
  }
  std::array<bool, 5> seen = {false, false, false, false, false};
  std::array<uint8_t, 5> order = {0, 1, 2, 3, 4};
  uint8_t mask = 0;
  uint8_t index = 0;
  for (JsonObject page : document["pages"].as<JsonArray>()) {
    const int type = page["type"] | -1;
    if (type < 0 || type >= 5 || seen[type] || index >= order.size()) continue;
    seen[type] = true;
    order[index++] = static_cast<uint8_t>(type);
    if (page["enabled"] | true) mask |= 1U << type;
  }
  for (uint8_t page = 0; page < 5; ++page) {
    if (!seen[page]) order[index++] = page;
  }
  cloudPageOrder_ = order;
  cloudPageMask_ = mask == 0 ? 1 : mask;
  cloudPageLanguage_ = String(document["language"] | "vi") == "en" ? "en" : "vi";
  pagesUpdatedAt_ = String(document["updatedAt"] | "");
  pagesAvailable_ = true;
  return true;
}

bool CloudFleetClient::fetchEffects() {
  JsonDocument document;
  String error;
  if (!getJson("effects.json", document, error)) {
    lastError_ = error;
    return false;
  }
  if (document["schemaVersion"].as<int>() != 1 ||
      !document["effects"].is<JsonArray>()) {
    lastError_ = "effects_manifest_invalid";
    return false;
  }

  effectCount_ = 0;
  effectsUpdatedAt_ = String(document["updatedAt"] | "");
  for (JsonObject item : document["effects"].as<JsonArray>()) {
    if (effectCount_ >= effects_.size()) break;
    const String id = String(item["id"] | "").substring(0, 40);
    if (id.isEmpty()) continue;
    EffectPreset& effect = effects_[effectCount_++];
    effect = EffectPreset{};
    effect.id = id;
    effect.name = String(item["name"] | id).substring(0, 40);
    effect.type = String(item["type"] | "solid").substring(0, 16);
    effect.enabled = item["enabled"] | true;
    effect.speed = constrain(item["speed"] | 50, 1, 100);
    effect.brightness = constrain(item["brightness"] | 50, 0, 100);
    if (item["palette"].is<JsonArray>()) {
      for (JsonVariant color : item["palette"].as<JsonArray>()) {
        if (effect.paletteSize >= effect.palette.size()) break;
        String value = color.as<String>();
        if (value.length() == 7 && value[0] == '#') {
          effect.palette[effect.paletteSize++] = value;
        }
      }
    }
    if (item["display"].is<JsonObject>()) {
      JsonObject display = item["display"].as<JsonObject>();
      effect.theme = String(display["theme"] | "");
      effect.language = String(display["language"] | "");
      if (display["rotation"].is<int>())
        effect.rotation = constrain(display["rotation"].as<int>(), 0, 3);
      if (display["autoRotate"].is<bool>())
        effect.autoRotate = display["autoRotate"].as<bool>() ? 1 : 0;
      if (display["pageInterval"].is<int>())
        effect.pageInterval = constrain(display["pageInterval"].as<int>(), 2, 30);
      if (display["pageMask"].is<int>())
        effect.pageMask = constrain(display["pageMask"].as<int>(), 1, 31);
      if (display["pageOrder"].is<JsonArray>() &&
          display["pageOrder"].as<JsonArray>().size() == 5) {
        std::array<bool, 5> seen = {false, false, false, false, false};
        bool valid = true;
        uint8_t index = 0;
        for (JsonVariant value : display["pageOrder"].as<JsonArray>()) {
          const int page = value.as<int>();
          if (page < 0 || page >= 5 || seen[page]) valid = false;
          else seen[page] = true;
          effect.pageOrder[index++] = static_cast<uint8_t>(page);
        }
        effect.hasPageOrder = valid;
      }
    }
  }
  if (effectCount_ == 0) selectedEffectId_.clear();
  return true;
}

bool CloudFleetClient::fetchControlManifest() {
  JsonDocument document;
  String error;
  if (!getJson("device-manifest.json", document, error)) {
    lastError_ = error;
    return false;
  }
  if (document["schemaVersion"].as<int>() != 1 ||
      String(document["compatibility"]["board"] | "") !=
          "esp32-s3-super-mini" ||
      (document["compatibility"]["flashMB"] | 0) != 4) {
    lastError_ = "control_manifest_incompatible";
    return false;
  }
  autoProvision_ = document["autoProvision"] | true;
  telemetryUrl_ = String(document["telemetryEndpoint"] | "");
  if (!telemetryUrl_.startsWith("https://")) telemetryUrl_.clear();
  const uint32_t seconds = document["minimumPollSeconds"] | 900;
  const uint32_t requestedIntervalMs = seconds * static_cast<uint32_t>(1000);
  pollIntervalMs_ =
      max<uint32_t>(AppConfig::CLOUD_MINIMUM_POLL_MS, requestedIntervalMs);
  return true;
}

void CloudFleetClient::reportTelemetry(const TelemetryData& telemetry,
                                       bool pcConnected, uint32_t freeHeap,
                                       uint8_t rotation,
                                       const char* language) {
  if (telemetryUrl_.isEmpty() || WiFi.status() != WL_CONNECTED || updating_ ||
      checking_) return;
  const uint32_t now = millis();
  if (lastReportMs_ != 0 && now - lastReportMs_ < 300000UL) return;
  if (lastReportMs_ == 0 && now - startedAtMs_ < 30000UL) return;
  lastReportMs_ = now;

  JsonDocument document;
  document["deviceId"] = deviceId_;
  document["mac"] = mac_;
  document["firmware"] = AppConfig::FIRMWARE_VERSION;
  document["board"] = "esp32-s3-super-mini";
  document["flashMB"] = 4;
  document["uptimeSeconds"] = now / 1000UL;
  document["freeHeap"] = freeHeap;
  document["pcConnected"] = pcConnected;
  document["rotation"] = rotation;
  document["language"] = language;
  JsonObject values = document["telemetry"].to<JsonObject>();
  if (!isnan(telemetry.cpuTemperature)) values["cpuTemp"] = telemetry.cpuTemperature;
  if (!isnan(telemetry.cpuLoad)) values["cpuLoad"] = telemetry.cpuLoad;
  if (!isnan(telemetry.gpuTemperature)) values["gpuTemp"] = telemetry.gpuTemperature;
  if (!isnan(telemetry.gpuLoad)) values["gpuLoad"] = telemetry.gpuLoad;
  if (!isnan(telemetry.memoryLoad)) values["ramLoad"] = telemetry.memoryLoad;
  if (!isnan(telemetry.fps)) values["fps"] = telemetry.fps;
  String body;
  serializeJson(document, body);

  WiFiClientSecure tls;
  tls.setInsecure();
  HTTPClient http;
  http.setConnectTimeout(8000);
  http.setTimeout(12000);
  if (!http.begin(tls, telemetryUrl_)) {
    lastReportOk_ = false;
    return;
  }
  http.addHeader("Content-Type", "application/json");
  http.addHeader("User-Agent", String("PCScreen-S3/") + AppConfig::FIRMWARE_VERSION);
  const int status = http.POST(body);
  lastReportOk_ = status >= 200 && status < 300;
  http.end();
}

bool CloudFleetClient::fetchDeviceRegistry() {
  JsonDocument document;
  String error;
  if (!getJson("devices.json", document, error)) {
    lastError_ = error;
    return false;
  }
  if (document["schemaVersion"].as<int>() != 1 ||
      !document["devices"].is<JsonArray>()) {
    lastError_ = "device_registry_invalid";
    return false;
  }

  for (JsonObject device : document["devices"].as<JsonArray>()) {
    String configuredMac = device["mac"] | "";
    String configuredId = device["id"] | "";
    configuredMac.toUpperCase();
    configuredId.toUpperCase();
    if (configuredMac == mac_ || configuredId == deviceId_) {
      registered_ = true;
      deviceEnabled_ = device["enabled"] | true;
      channel_ = String(device["channel"] | "stable");
      return true;
    }
  }
  deviceEnabled_ = autoProvision_;
  return true;
}

bool CloudFleetClient::fetchFirmwareManifest() {
  JsonDocument document;
  String error;
  if (!getJson("firmware-manifest.json", document, error)) {
    lastError_ = error;
    return false;
  }
  if (document["schemaVersion"].as<int>() != 1 ||
      String(document["board"] | "") != "esp32-s3-super-mini" ||
      (document["flashMB"] | 0) != 4 ||
      String(document["format"] | "") != "pcota") {
    lastError_ = "firmware_manifest_incompatible";
    return false;
  }

  firmware_.version = String(document["latestVersion"] | "");
  firmware_.minimumVersion = String(document["minimumVersion"] | "");
  firmware_.url = String(document["downloadUrl"] | "");
  firmware_.sha256 = String(document["sha256"] | "");
  firmware_.sha256.toLowerCase();
  firmware_.size = document["size"] | 0;
  firmware_.mandatory = document["mandatory"] | false;
  firmware_.updateAvailable =
      compareVersions(firmware_.version, AppConfig::FIRMWARE_VERSION) > 0;

  if (firmware_.version.isEmpty() ||
      !firmware_.url.startsWith("https://") ||
      firmware_.sha256.length() != 64 || firmware_.size < kOtaHeaderSize) {
    lastError_ = "firmware_manifest_missing_fields";
    return false;
  }
  return true;
}

bool CloudFleetClient::getJson(const String& name, JsonDocument& output,
                               String& error) {
  WiFiClientSecure tls;
  tls.setInsecure();

  HTTPClient http;
  http.setConnectTimeout(8000);
  http.setTimeout(12000);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.useHTTP10(true);
  const String url = String(AppConfig::CLOUD_DATA_BASE_URL) + "/" + name;
  if (!http.begin(tls, url)) {
    error = "https_begin_failed";
    return false;
  }
  http.addHeader("Accept", "application/json");
  http.addHeader("User-Agent", String("PCScreen-S3/") +
                                   AppConfig::FIRMWARE_VERSION + " " +
                                   deviceId_);
  const int status = http.GET();
  if (status != HTTP_CODE_OK) {
    error = String("http_") + status + "_" + name;
    http.end();
    return false;
  }
  const DeserializationError jsonError = deserializeJson(output, http.getStream());
  http.end();
  if (jsonError) {
    error = String("invalid_json_") + name;
    return false;
  }
  return true;
}

bool CloudFleetClient::installAvailableUpdate(
    std::function<void()> frameCallback) {
  if (checking_ || updating_) {
    lastError_ = "busy";
    return false;
  }
  if (!firmware_.updateAvailable || firmware_.url.isEmpty()) {
    lastError_ = "no_update_available";
    return false;
  }
  if (!deviceEnabled_) {
    lastError_ = "device_disabled";
    return false;
  }
  if (WiFi.status() != WL_CONNECTED) {
    lastError_ = "wifi_not_connected";
    return false;
  }

  updating_ = true;
  lastError_.clear();
  otaStatus_.set(OtaStatus::Phase::Receiving, 0);
  if (frameCallback) frameCallback();

  WiFiClientSecure tls;
  tls.setInsecure();
  HTTPClient http;
  http.setConnectTimeout(10000);
  http.setTimeout(kReadTimeoutMs);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  if (!http.begin(tls, firmware_.url)) {
    lastError_ = "ota_https_begin_failed";
    otaStatus_.set(OtaStatus::Phase::Failed, 0);
    updating_ = false;
    return false;
  }
  http.addHeader("Accept", "application/octet-stream");
  http.addHeader("User-Agent", String("PCScreen-S3/") +
                                   AppConfig::FIRMWARE_VERSION + " " +
                                   deviceId_);
  const int status = http.GET();
  if (status != HTTP_CODE_OK) {
    lastError_ = String("ota_http_") + status;
    http.end();
    otaStatus_.set(OtaStatus::Phase::Failed, 0);
    updating_ = false;
    return false;
  }

  const int contentLength = http.getSize();
  if ((contentLength > 0 && static_cast<size_t>(contentLength) != firmware_.size) ||
      firmware_.size <= kOtaHeaderSize) {
    lastError_ = "ota_size_mismatch";
    http.end();
    otaStatus_.set(OtaStatus::Phase::Failed, 0);
    updating_ = false;
    return false;
  }

  WiFiClient* stream = http.getStreamPtr();
  stream->setTimeout(kReadTimeoutMs);
  uint8_t header[kOtaHeaderSize];
  mbedtls_sha256_context sha;
  mbedtls_sha256_init(&sha);
  mbedtls_sha256_starts_ret(&sha, 0);

  if (!readExactly(*stream, header, sizeof(header))) {
    lastError_ = "ota_header_timeout";
    mbedtls_sha256_free(&sha);
    http.end();
    otaStatus_.set(OtaStatus::Phase::Failed, 0);
    updating_ = false;
    return false;
  }
  mbedtls_sha256_update_ret(&sha, header, sizeof(header));

  const uint32_t formatVersion = readUint32Le(header + 8);
  const uint32_t payloadSize = readUint32Le(header + 12);
  if (memcmp(header, kOtaMagic, sizeof(kOtaMagic)) != 0 ||
      formatVersion != kOtaFormatVersion || payloadSize == 0 ||
      static_cast<size_t>(payloadSize) + kOtaHeaderSize != firmware_.size ||
      !Update.begin(payloadSize, U_FLASH)) {
    lastError_ = "ota_header_or_partition_invalid";
    mbedtls_sha256_free(&sha);
    http.end();
    otaStatus_.set(OtaStatus::Phase::Failed, 0);
    updating_ = false;
    return false;
  }

  uint8_t buffer[kReadBufferSize];
  size_t received = 0;
  uint32_t lastDataAt = millis();
  bool writeOk = true;
  while (received < payloadSize) {
    const int available = stream->available();
    if (available <= 0) {
      if (millis() - lastDataAt >= kReadTimeoutMs) {
        lastError_ = "ota_download_timeout";
        writeOk = false;
        break;
      }
      delay(1);
      continue;
    }
    const size_t chunk = min(
        min(static_cast<size_t>(available), sizeof(buffer)),
        static_cast<size_t>(payloadSize) - received);
    const size_t count = stream->readBytes(buffer, chunk);
    if (count == 0) continue;
    lastDataAt = millis();
    mbedtls_sha256_update_ret(&sha, buffer, count);
    if (Update.write(buffer, count) != count) {
      lastError_ = String("ota_write_failed_") + Update.errorString();
      writeOk = false;
      break;
    }
    received += count;
    const uint8_t progress = static_cast<uint8_t>(received * 100ULL / payloadSize);
    otaStatus_.set(OtaStatus::Phase::Receiving, progress);
    if (frameCallback) frameCallback();
    delay(0);
  }

  uint8_t digest[32];
  mbedtls_sha256_finish_ret(&sha, digest);
  mbedtls_sha256_free(&sha);
  http.end();

  if (!writeOk || received != payloadSize ||
      digestToHex(digest) != firmware_.sha256) {
    if (lastError_.isEmpty()) lastError_ = "ota_sha256_mismatch";
    if (Update.isRunning()) Update.abort();
    otaStatus_.set(OtaStatus::Phase::Failed, 0);
    updating_ = false;
    return false;
  }

  otaStatus_.set(OtaStatus::Phase::Verifying, 100);
  if (frameCallback) frameCallback();
  if (!Update.end() || !Update.isFinished()) {
    lastError_ = String("ota_finalize_failed_") + Update.errorString();
    otaStatus_.set(OtaStatus::Phase::Failed, 0);
    updating_ = false;
    return false;
  }

  firmware_.updateAvailable = false;
  otaStatus_.set(OtaStatus::Phase::Success, 100);
  if (frameCallback) frameCallback();
  updating_ = false;
  return true;
}

CloudFleetClient::Status CloudFleetClient::status() const {
  const uint32_t now = millis();
  const uint32_t next = lastCheckMs_ == 0
                            ? startedAtMs_ + AppConfig::CLOUD_FIRST_CHECK_DELAY_MS
                            : lastCheckMs_ + pollIntervalMs_;
  Status result;
  result.deviceId = deviceId_;
  result.mac = mac_;
  result.serverUrl = AppConfig::CLOUD_DATA_BASE_URL;
  result.currentVersion = AppConfig::FIRMWARE_VERSION;
  result.latestVersion = firmware_.version;
  result.lastError = lastError_;
  result.channel = channel_;
  result.internetConnected = WiFi.status() == WL_CONNECTED;
  result.serverReachable = serverReachable_;
  result.registered = registered_;
  result.deviceEnabled = deviceEnabled_;
  result.autoProvision = autoProvision_;
  result.updateAvailable = firmware_.updateAvailable;
  result.mandatory = firmware_.mandatory;
  result.effectCount = effectCount_;
  result.selectedEffectId = selectedEffectId_;
  result.effectsUpdatedAt = effectsUpdatedAt_;
  result.pagesUpdatedAt = pagesUpdatedAt_;
  result.pagesAvailable = pagesAvailable_;
  result.reportingEnabled = !telemetryUrl_.isEmpty();
  result.lastReportOk = lastReportOk_;
  result.lastReportMs = lastReportMs_;
  result.lastCheckMs = lastCheckMs_;
  result.nextCheckMs = next > now ? next - now : 0;
  return result;
}

size_t CloudFleetClient::effectCount() const { return effectCount_; }

const CloudFleetClient::EffectPreset* CloudFleetClient::effectAt(
    size_t index) const {
  return index < effectCount_ ? &effects_[index] : nullptr;
}

const CloudFleetClient::EffectPreset* CloudFleetClient::findEffect(
    const String& id) const {
  for (size_t index = 0; index < effectCount_; ++index) {
    if (effects_[index].id == id) return &effects_[index];
  }
  return nullptr;
}

void CloudFleetClient::selectEffect(const String& id) {
  selectedEffectId_ = id.substring(0, 40);
  preferences_.putString("effect", selectedEffectId_);
}

CloudFleetClient::PageSetup CloudFleetClient::pageSetup() const {
  PageSetup result;
  result.order = cloudPageOrder_;
  result.mask = cloudPageMask_;
  result.language = cloudPageLanguage_;
  result.updatedAt = pagesUpdatedAt_;
  result.available = pagesAvailable_;
  return result;
}

void CloudFleetClient::buildIdentity() {
  uint8_t mac[6] = {};
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  char macText[18];
  snprintf(macText, sizeof(macText), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  mac_ = macText;
  char idText[22];
  snprintf(idText, sizeof(idText), "PCSCREEN-%02X%02X%02X%02X%02X%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  deviceId_ = idText;
}

int CloudFleetClient::compareVersions(const String& left,
                                      const String& right) {
  int leftParts[3] = {0, 0, 0};
  int rightParts[3] = {0, 0, 0};
  sscanf(left.c_str(), "%d.%d.%d", &leftParts[0], &leftParts[1],
         &leftParts[2]);
  sscanf(right.c_str(), "%d.%d.%d", &rightParts[0], &rightParts[1],
         &rightParts[2]);
  for (size_t index = 0; index < 3; ++index) {
    if (leftParts[index] == rightParts[index]) continue;
    return leftParts[index] > rightParts[index] ? 1 : -1;
  }
  return 0;
}
