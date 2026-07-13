#pragma once

#include <FS.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <functional>

#include "BacklightController.h"
#include "CloudFleetClient.h"
#include "NetworkTimeManager.h"
#include "OtaStatus.h"
#include "RgbLedController.h"
#include "TelemetryData.h"
#include "UiSettings.h"

class WebConfigServer final {
 public:
  WebConfigServer(BacklightController& backlight, RgbLedController& rgb,
                  TelemetryData& telemetry, UiSettings& settings,
                  OtaStatus& otaStatus, NetworkTimeManager& network,
                  CloudFleetClient& cloud);
  bool begin();
  void loop();
  void setOtaFrameCallback(std::function<void()> callback);
  String address() const;

 private:
  WebServer server_{80};
  WebSocketsServer socket_{81};
  BacklightController& backlight_;
  RgbLedController& rgb_;
  TelemetryData& telemetry_;
  UiSettings& settings_;
  OtaStatus& otaStatus_;
  NetworkTimeManager& network_;
  CloudFleetClient& cloud_;
  bool accessPoint_ = false;
  File uploadFile_;
  bool uploadOk_ = false;
  size_t uploadSize_ = 0;
  String uploadExtension_;
  String uploadError_;
  bool otaOk_ = false;
  bool otaStarted_ = false;
  bool otaFirstPayloadChecked_ = false;
  size_t otaHeaderBytes_ = 0;
  uint8_t otaHeader_[16] = {};
  uint32_t otaExpectedSize_ = 0;
  size_t otaWritten_ = 0;
  String otaError_;
  uint32_t restartAtMs_ = 0;
  uint32_t lastWebSocketPushMs_ = 0;
  uint32_t lastOtaFrameMs_ = 0;
  size_t lastBroadcastEffectCount_ = 0;
  String lastBroadcastEffectsUpdatedAt_;
  std::function<void()> otaFrameCallback_;
  void registerRoutes();
  void sendBacklightState();
  void updateBacklight();
  void sendRgbState();
  void updateRgb();
  void sendTelemetry();
  void sendUiSettings();
  void updateUiSettings();
  void sendNetworkState();
  void updateNetwork();
  void scanWifi();
  void sendSystemState();
  void sendCloudState();
  void sendCloudEffects();
  void broadcastCloudEffects(int client = -1);
  String cloudEffectsJson() const;
  bool applyCloudEffect(const String& id, String& error);
  void applyCloudEffectRequest();
  void checkCloud();
  void installCloudUpdate();
  void restartSystem();
  void handleWebSocketEvent(uint8_t client, WStype_t type,
                            uint8_t* payload, size_t length);
  void pushRealtime();
  void sendBootAssetState();
  void finishBootAssetUpload();
  void handleBootAssetUpload();
  void deleteBootAsset();
  void finishOtaUpload();
  void handleOtaUpload();
  void updateOtaDisplay(bool force = false);
};
