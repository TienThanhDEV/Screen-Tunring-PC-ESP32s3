#include "NetworkTimeManager.h"

#include <WiFi.h>
#include <time.h>

#include "AppConfig.h"

namespace {
constexpr uint32_t kReconnectIntervalMs = 30000;
constexpr time_t kMinimumValidTime = 1704067200;  // 2024-01-01 UTC
}

bool NetworkTimeManager::begin() {
  if (!preferences_.begin("network", false)) return false;
  ssid_ = preferences_.getString("ssid", "");
  password_ = preferences_.getString("password", "");
  utcOffsetMinutes_ = constrain(preferences_.getShort("utc_min", 420),
                                static_cast<int16_t>(-720),
                                static_cast<int16_t>(840));
  // A clean flash has no SSID and enters the QR provisioning flow. Devices
  // upgraded from an older release already have an SSID; treating them as
  // provisioned keeps OTA startup behavior unchanged.
  provisioned_ = preferences_.getBool("provisioned", !ssid_.isEmpty());

  WiFi.mode(WIFI_AP_STA);
  WiFi.setHostname(AppConfig::HOSTNAME);
  if (!WiFi.softAP(AppConfig::AP_SSID, AppConfig::AP_PASSWORD)) return false;
  configureTime();
  connectStation();
  return true;
}

void NetworkTimeManager::loop() {
  if (!provisioned_ && WiFi.status() == WL_CONNECTED) {
    provisioned_ = true;
    preferences_.putBool("provisioned", true);
  }
  if (ssid_.isEmpty() || WiFi.status() == WL_CONNECTED) return;
  const uint32_t now = millis();
  if (now - lastConnectAttemptMs_ >= kReconnectIntervalMs) connectStation();
}

NetworkTimeManager::State NetworkTimeManager::state() const {
  const bool connected = WiFi.status() == WL_CONNECTED;
  return {ssid_, !password_.isEmpty(), connected,
          time(nullptr) >= kMinimumValidTime, utcOffsetMinutes_,
          connected ? WiFi.RSSI() : 0,
          connected ? WiFi.localIP().toString() : String("--"),
          WiFi.softAPIP().toString()};
}

NetworkTimeManager::ClockSnapshot NetworkTimeManager::clock() const {
  ClockSnapshot result{false, "--:--", "--/--/----"};
  const time_t now = time(nullptr);
  if (now < kMinimumValidTime) return result;
  struct tm localTime {};
  localtime_r(&now, &localTime);
  snprintf(result.time, sizeof(result.time), "%02d:%02d",
           localTime.tm_hour, localTime.tm_min);
  snprintf(result.date, sizeof(result.date), "%02d/%02d/%04d",
           localTime.tm_mday, localTime.tm_mon + 1, localTime.tm_year + 1900);
  result.synced = true;
  return result;
}

void NetworkTimeManager::updateConfig(const String& ssid,
                                      const String& password,
                                      bool updatePassword,
                                      int16_t utcOffsetMinutes) {
  ssid_ = ssid.substring(0, 32);
  if (updatePassword) password_ = password.substring(0, 63);
  utcOffsetMinutes_ = constrain(utcOffsetMinutes, static_cast<int16_t>(-720),
                                static_cast<int16_t>(840));
  preferences_.putString("ssid", ssid_);
  if (updatePassword) preferences_.putString("password", password_);
  preferences_.putShort("utc_min", utcOffsetMinutes_);
  configureTime();
  connectStation();
}

String NetworkTimeManager::displayAddress() const {
  return WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString()
                                       : WiFi.softAPIP().toString();
}

void NetworkTimeManager::connectStation() {
  lastConnectAttemptMs_ = millis();
  WiFi.disconnect(false, false);
  if (!ssid_.isEmpty()) WiFi.begin(ssid_.c_str(), password_.c_str());
}

void NetworkTimeManager::configureTime() const {
  configTime(static_cast<long>(utcOffsetMinutes_) * 60L, 0,
             "pool.ntp.org", "time.google.com", "time.cloudflare.com");
}
