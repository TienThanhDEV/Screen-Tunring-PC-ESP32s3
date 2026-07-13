#pragma once

#include <Arduino.h>
#include <Preferences.h>

class NetworkTimeManager final {
 public:
  struct State {
    String ssid;
    bool passwordSet;
    bool connected;
    bool timeSynced;
    int16_t utcOffsetMinutes;
    int32_t rssi;
    String stationIp;
    String accessPointIp;
  };

  struct ClockSnapshot {
    bool synced;
    char time[6];
    char date[11];
  };

  bool begin();
  void loop();
  State state() const;
  ClockSnapshot clock() const;
  void updateConfig(const String& ssid, const String& password,
                    bool updatePassword, int16_t utcOffsetMinutes);
  String displayAddress() const;

 private:
  Preferences preferences_;
  String ssid_;
  String password_;
  int16_t utcOffsetMinutes_ = 420;
  uint32_t lastConnectAttemptMs_ = 0;

  void connectStation();
  void configureTime() const;
};
