#pragma once

#include <Arduino.h>

namespace AppConfig {
constexpr char FIRMWARE_VERSION[] = "1.7.0";
constexpr char PROJECT_AUTHOR[] = "Nguyen Tien Thanh";
constexpr char PROJECT_GITHUB[] = "TienThanhDEV";
constexpr char CLOUD_DATA_BASE_URL[] =
    "https://tienthanhdev.github.io/Screen-Tunring-PC-ESP32s3/data";
constexpr uint32_t CLOUD_FIRST_CHECK_DELAY_MS = 15000;
constexpr uint32_t CLOUD_DEFAULT_POLL_MS = 15UL * 60UL * 1000UL;
constexpr uint32_t CLOUD_MINIMUM_POLL_MS = 60UL * 1000UL;
// Leave empty to start the setup access point immediately.
constexpr char WIFI_SSID[] = "";
constexpr char WIFI_PASSWORD[] = "";
constexpr char AP_SSID[] = "PCScreen-Setup";
constexpr char AP_PASSWORD[] = "pcscreen123";
constexpr char HOSTNAME[] = "pcscreen";

constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 12000;
constexpr uint32_t TELEMETRY_STALE_MS = 5000;
constexpr uint32_t DISPLAY_REFRESH_MS = 100;
constexpr uint32_t DISPLAY_TRANSITION_FRAME_MS = 33;
constexpr uint16_t TELEMETRY_INTERVAL_MIN_MS = 250;
constexpr uint16_t TELEMETRY_INTERVAL_MAX_MS = 5000;
constexpr uint16_t TELEMETRY_INTERVAL_DEFAULT_MS = 1000;
constexpr uint32_t SETTINGS_COMMIT_DELAY_MS = 800;
constexpr uint8_t DEFAULT_BRIGHTNESS = 70;
constexpr uint8_t RGB_MAX_SAFE_BRIGHTNESS = 100;
constexpr uint32_t RGB_EFFECT_FRAME_MS = 40;
// Exact public upload ceiling. LittleFS keeps the remaining bytes for its
// metadata and the embedded web console files.
constexpr size_t BOOT_ASSET_MAX_BYTES = 1200000;
constexpr uint32_t BOOT_ASSET_SHOW_MS = 3500;
constexpr uint32_t FIRST_BOOT_WELCOME_MS = 2600;
}  // namespace AppConfig
