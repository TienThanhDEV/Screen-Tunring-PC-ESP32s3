#include "CloudDataClient.h"

#include <HTTPClient.h>
#include <WiFiClientSecure.h>

CloudDataClient::CloudDataClient(String baseUrl) : baseUrl_(baseUrl) {
  while (baseUrl_.endsWith("/")) baseUrl_.remove(baseUrl_.length() - 1);
}

void CloudDataClient::setCurrentVersion(const String& version) {
  currentVersion_ = version;
}

void CloudDataClient::setPollInterval(const uint32_t intervalMs) {
  pollIntervalMs_ = max(intervalMs, 60000UL);
}

bool CloudDataClient::shouldPoll(const uint32_t nowMs) const {
  return lastPollMs_ == 0 || (nowMs - lastPollMs_) >= pollIntervalMs_;
}

bool CloudDataClient::getJson(const String& path, JsonDocument& output, String& error) {
  WiFiClientSecure tls;

  // Dễ thử nghiệm: bỏ kiểm tra CA. Khi phát hành sản phẩm, thay bằng
  // tls.setCACert(rootCa) và cập nhật chứng thư gốc theo định kỳ.
  tls.setInsecure();

  HTTPClient http;
  http.setConnectTimeout(8000);
  http.setTimeout(12000);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.useHTTP10(true);

  const String url = baseUrl_ + "/" + path;
  if (!http.begin(tls, url)) {
    error = "Khong khoi tao duoc HTTPS";
    return false;
  }

  http.addHeader("Accept", "application/json");
  http.addHeader("User-Agent", "PCScreen-S3/0.6.0");
  const int status = http.GET();
  if (status != HTTP_CODE_OK) {
    error = "HTTP " + String(status) + " khi tai " + url;
    http.end();
    return false;
  }

  const DeserializationError jsonError = deserializeJson(output, http.getStream());
  http.end();
  if (jsonError) {
    error = "JSON khong hop le: " + String(jsonError.c_str());
    return false;
  }

  lastPollMs_ = millis();
  return true;
}

bool CloudDataClient::fetchEffects(JsonDocument& output, String& error) {
  if (!getJson("effects.json", output, error)) return false;
  if (output["schemaVersion"] != 1 || !output["effects"].is<JsonArray>()) {
    error = "Schema effects.json khong duoc ho tro";
    return false;
  }
  return true;
}

bool CloudDataClient::fetchFirmware(CloudFirmwareInfo& output, String& error) {
  JsonDocument document;
  if (!getJson("firmware-manifest.json", document, error)) return false;

  if (document["schemaVersion"] != 1 || document["flashMB"] != 4 ||
      String(document["board"] | "") != "esp32-s3-super-mini") {
    error = "Firmware khong dung board hoac flash 4 MB";
    return false;
  }

  output.version = String(document["latestVersion"] | "");
  output.minimumVersion = String(document["minimumVersion"] | "");
  output.url = String(document["downloadUrl"] | "");
  output.sha256 = String(document["sha256"] | "");
  output.size = document["size"] | 0;
  output.mandatory = document["mandatory"] | false;
  output.updateAvailable = compareVersions(output.version, currentVersion_) > 0;

  if (output.version.isEmpty() || !output.url.startsWith("https://") ||
      output.sha256.length() != 64 || output.size < 1000) {
    error = "Firmware manifest thieu truong an toan";
    return false;
  }
  return true;
}

int CloudDataClient::compareVersions(const String& left, const String& right) {
  int leftParts[3] = {0, 0, 0};
  int rightParts[3] = {0, 0, 0};
  sscanf(left.c_str(), "%d.%d.%d", &leftParts[0], &leftParts[1], &leftParts[2]);
  sscanf(right.c_str(), "%d.%d.%d", &rightParts[0], &rightParts[1], &rightParts[2]);
  for (int index = 0; index < 3; ++index) {
    if (leftParts[index] != rightParts[index]) return leftParts[index] > rightParts[index] ? 1 : -1;
  }
  return 0;
}
