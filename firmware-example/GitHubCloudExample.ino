#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>

#include "CloudDataClient.h"

// Không commit mật khẩu thật. Với firmware chính, hãy lấy Wi-Fi từ NVS/web setup.
constexpr char WIFI_SSID[] = "YOUR_WIFI_NAME";
constexpr char WIFI_PASSWORD[] = "YOUR_WIFI_PASSWORD";
constexpr char CURRENT_FIRMWARE[] = "0.7.0";
constexpr char CLOUD_DATA_URL[] =
    "https://tienthanhdev.github.io/Screen-Tunring-PC-ESP32s3/data";

CloudDataClient cloud(CLOUD_DATA_URL);

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  const uint32_t startedAt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startedAt < 15000) {
    delay(200);
  }
}

void syncCloudData() {
  String error;
  JsonDocument effects;
  if (cloud.fetchEffects(effects, error)) {
    for (JsonObject effect : effects["effects"].as<JsonArray>()) {
      if (!(effect["enabled"] | false)) continue;
      Serial.printf("Effect: %s, speed=%d, brightness=%d%%\n",
                    effect["name"] | "unknown", effect["speed"] | 0,
                    effect["brightness"] | 0);
      // TODO: chuyển palette/type sang AnimationManager của firmware chính.
    }
  } else {
    Serial.println("Effects error: " + error);
  }

  CloudFirmwareInfo firmware;
  error = "";
  if (cloud.fetchFirmware(firmware, error) && firmware.updateAvailable) {
    Serial.printf("OTA available: %s, %u bytes\n", firmware.version.c_str(),
                  static_cast<unsigned>(firmware.size));
    Serial.println("URL: " + firmware.url);
    Serial.println("SHA-256: " + firmware.sha256);
    // Không tự flash ngay tại đây. Firmware chính phải tải .pcota, kiểm tra
    // header/board/size/SHA-256, hỏi người dùng rồi mới gọi Update API.
  } else if (!error.isEmpty()) {
    Serial.println("Firmware error: " + error);
  }
}

void setup() {
  Serial.begin(115200);
  cloud.setCurrentVersion(CURRENT_FIRMWARE);
  cloud.setPollInterval(15UL * 60UL * 1000UL);
  connectWiFi();
  if (WiFi.status() == WL_CONNECTED) syncCloudData();
}

void loop() {
  if (WiFi.status() == WL_CONNECTED && cloud.shouldPoll(millis())) syncCloudData();
  delay(50);
}
