#include "WebConfigServer.h"

#include <ArduinoJson.h>
#include <LittleFS.h>
#include <Update.h>
#include <WiFi.h>
#include <esp_system.h>

#include "AppConfig.h"
#include "BootAssetManager.h"

namespace {
constexpr uint8_t kOtaMagic[8] = {'P', 'C', 'S', 'O', 'T', 'A', '1', 0};
constexpr size_t kOtaHeaderSize = 16;
constexpr uint32_t kOtaFormatVersion = 1;
extern const uint8_t kIndexHtmlStart[] asm("_binary_data_index_html_start");
extern const uint8_t kIndexHtmlEnd[] asm("_binary_data_index_html_end");
extern const uint8_t kAppJsStart[] asm("_binary_data_app_js_start");
extern const uint8_t kAppJsEnd[] asm("_binary_data_app_js_end");
extern const uint8_t kI18nJsStart[] asm("_binary_data_i18n_js_start");
extern const uint8_t kI18nJsEnd[] asm("_binary_data_i18n_js_end");
extern const uint8_t kV4CssStart[] asm("_binary_data_v4_css_start");
extern const uint8_t kV4CssEnd[] asm("_binary_data_v4_css_end");
extern const uint8_t kV5CssStart[] asm("_binary_data_v5_css_start");
extern const uint8_t kV5CssEnd[] asm("_binary_data_v5_css_end");
extern const uint8_t kV6CssStart[] asm("_binary_data_v6_css_start");
extern const uint8_t kV6CssEnd[] asm("_binary_data_v6_css_end");
extern const uint8_t kOtaAnimationCssStart[]
    asm("_binary_data_ota_animation_css_start");
extern const uint8_t kOtaAnimationCssEnd[]
    asm("_binary_data_ota_animation_css_end");
extern const uint8_t kCropThemeCssStart[]
    asm("_binary_data_crop_theme_css_start");
extern const uint8_t kCropThemeCssEnd[]
    asm("_binary_data_crop_theme_css_end");

size_t embeddedSize(const uint8_t* start, const uint8_t* end) {
  size_t size = static_cast<size_t>(end - start);
  if (size > 0 && start[size - 1] == 0) --size;
  return size;
}

void purgeLegacyWebCopies() {
  // Since v1.6.0 the console is served from the application image. Removing
  // the older duplicated files frees LittleFS exclusively for the boot asset.
  static constexpr const char* kLegacyFiles[] = {
      "/index.html", "/app.js", "/i18n.js", "/style.css", "/ota.css", "/v4.css",
      "/v5.css", "/v6.css", "/ota-animation.css", "/crop-theme.css",
      "/web.version"};
  for (const char* path : kLegacyFiles) LittleFS.remove(path);
}

uint32_t readUint32Le(const uint8_t* data) {
  return static_cast<uint32_t>(data[0]) |
         (static_cast<uint32_t>(data[1]) << 8U) |
         (static_cast<uint32_t>(data[2]) << 16U) |
         (static_cast<uint32_t>(data[3]) << 24U);
}

String rgb565ToHex(uint16_t color) {
  const uint8_t red = ((color >> 11U) & 0x1FU) * 255U / 31U;
  const uint8_t green = ((color >> 5U) & 0x3FU) * 255U / 63U;
  const uint8_t blue = (color & 0x1FU) * 255U / 31U;
  char value[8];
  snprintf(value, sizeof(value), "#%02X%02X%02X", red, green, blue);
  return String(value);
}

bool hexToRgb565(const String& text, uint16_t& result) {
  if (text.length() != 7 || text[0] != '#') return false;
  char* end = nullptr;
  const uint32_t rgb = strtoul(text.c_str() + 1, &end, 16);
  if (!end || *end != '\0') return false;
  const uint8_t red = (rgb >> 16U) & 0xFFU;
  const uint8_t green = (rgb >> 8U) & 0xFFU;
  const uint8_t blue = rgb & 0xFFU;
  result = ((red & 0xF8U) << 8U) | ((green & 0xFCU) << 3U) | (blue >> 3U);
  return true;
}

bool hexToRgb888(const String& text, uint8_t& red, uint8_t& green,
                 uint8_t& blue) {
  if (text.length() != 7 || text[0] != '#') return false;
  char* end = nullptr;
  const uint32_t rgb = strtoul(text.c_str() + 1, &end, 16);
  if (!end || *end != '\0') return false;
  red = (rgb >> 16U) & 0xFFU;
  green = (rgb >> 8U) & 0xFFU;
  blue = rgb & 0xFFU;
  return true;
}
}  // namespace

WebConfigServer::WebConfigServer(BacklightController& backlight,
                                 RgbLedController& rgb,
                                 TelemetryData& telemetry,
                                 UiSettings& settings,
                                 OtaStatus& otaStatus,
                                 NetworkTimeManager& network,
                                 CloudFleetClient& cloud)
    : backlight_(backlight), rgb_(rgb), telemetry_(telemetry),
      settings_(settings), otaStatus_(otaStatus), network_(network),
      cloud_(cloud) {}

bool WebConfigServer::begin() {
  if (!LittleFS.begin(true)) return false;
  purgeLegacyWebCopies();
  registerRoutes();
  server_.begin();
  socket_.begin();
  socket_.onEvent([this](uint8_t client, WStype_t type, uint8_t* payload,
                         size_t length) {
    handleWebSocketEvent(client, type, payload, length);
  });
  return true;
}

void WebConfigServer::loop() {
  server_.handleClient();
  socket_.loop();
  const auto settings = settings_.state();
  const uint32_t pushInterval = settings.telemetryPaused
                                    ? 1000UL
                                    : settings.telemetryIntervalMs;
  if (millis() - lastWebSocketPushMs_ >= pushInterval) {
    lastWebSocketPushMs_ = millis();
    pushRealtime();
  }
  if (restartAtMs_ != 0 &&
      static_cast<int32_t>(millis() - restartAtMs_) >= 0) {
    ESP.restart();
  }
}

String WebConfigServer::address() const {
  return network_.displayAddress();
}

void WebConfigServer::setOtaFrameCallback(std::function<void()> callback) {
  otaFrameCallback_ = callback;
}

void WebConfigServer::registerRoutes() {
  server_.on("/api/v1/display/backlight", HTTP_GET,
             [this]() { sendBacklightState(); });
  server_.on("/api/v1/display/backlight", HTTP_PUT,
             [this]() { updateBacklight(); });
  server_.on("/api/v1/board/rgb", HTTP_GET, [this]() { sendRgbState(); });
  server_.on("/api/v1/board/rgb", HTTP_PUT, [this]() { updateRgb(); });
  server_.on("/api/v1/assets/boot", HTTP_GET,
             [this]() { sendBootAssetState(); });
  server_.on("/api/v1/assets/boot", HTTP_DELETE,
             [this]() { deleteBootAsset(); });
  server_.on("/api/v1/assets/boot", HTTP_POST,
             [this]() { finishBootAssetUpload(); },
             [this]() { handleBootAssetUpload(); });
  server_.on("/api/v1/system/ota", HTTP_POST,
             [this]() { finishOtaUpload(); },
             [this]() { handleOtaUpload(); });
  server_.on("/api/v1/telemetry", HTTP_GET, [this]() { sendTelemetry(); });
  server_.on("/api/v1/telemetry/settings", HTTP_GET,
             [this]() { sendTelemetrySettings(); });
  server_.on("/api/v1/telemetry/settings", HTTP_PUT,
             [this]() { updateTelemetrySettings(); });
  server_.on("/api/v1/ui", HTTP_GET, [this]() { sendUiSettings(); });
  server_.on("/api/v1/ui", HTTP_PUT, [this]() { updateUiSettings(); });
  server_.on("/api/v1/network", HTTP_GET, [this]() { sendNetworkState(); });
  server_.on("/api/v1/network", HTTP_PUT, [this]() { updateNetwork(); });
  server_.on("/api/v1/network/scan", HTTP_GET, [this]() { scanWifi(); });
  server_.on("/api/v1/system", HTTP_GET, [this]() { sendSystemState(); });
  server_.on("/api/v1/cloud", HTTP_GET, [this]() { sendCloudState(); });
  server_.on("/api/v1/cloud/effects", HTTP_GET,
             [this]() { sendCloudEffects(); });
  server_.on("/api/v1/cloud/effects/apply", HTTP_POST,
             [this]() { applyCloudEffectRequest(); });
  server_.on("/api/v1/cloud/pages", HTTP_GET,
             [this]() { sendCloudPages(); });
  server_.on("/api/v1/cloud/pages/apply", HTTP_POST,
             [this]() { applyCloudPages(); });
  server_.on("/api/v1/cloud/check", HTTP_POST, [this]() { checkCloud(); });
  server_.on("/api/v1/cloud/update", HTTP_POST,
             [this]() { installCloudUpdate(); });
  server_.on("/api/v1/system/restart", HTTP_POST,
             [this]() { restartSystem(); });
  server_.on("/", HTTP_GET, [this]() {
    server_.send_P(200, "text/html; charset=utf-8",
                   reinterpret_cast<const char*>(kIndexHtmlStart),
                   embeddedSize(kIndexHtmlStart, kIndexHtmlEnd));
  });
  const auto embeddedRoute = [this](const char* path, const char* contentType,
                                    const uint8_t* start,
                                    const uint8_t* end) {
    server_.on(path, HTTP_GET, [this, contentType, start, end]() {
      server_.send_P(200, contentType, reinterpret_cast<const char*>(start),
                     embeddedSize(start, end));
    });
  };
  embeddedRoute("/v4.css", "text/css; charset=utf-8", kV4CssStart, kV4CssEnd);
  embeddedRoute("/v5.css", "text/css; charset=utf-8", kV5CssStart, kV5CssEnd);
  embeddedRoute("/v6.css", "text/css; charset=utf-8", kV6CssStart, kV6CssEnd);
  embeddedRoute("/ota-animation.css", "text/css; charset=utf-8",
                kOtaAnimationCssStart, kOtaAnimationCssEnd);
  embeddedRoute("/crop-theme.css", "text/css; charset=utf-8",
                kCropThemeCssStart, kCropThemeCssEnd);
  embeddedRoute("/app.js", "application/javascript; charset=utf-8",
                kAppJsStart, kAppJsEnd);
  embeddedRoute("/i18n.js", "application/javascript; charset=utf-8",
                kI18nJsStart, kI18nJsEnd);
  server_.onNotFound([this]() {
    server_.send(404, "application/json", "{\"error\":\"not_found\"}");
  });
}

void WebConfigServer::sendBacklightState() {
  const auto state = backlight_.state();
  JsonDocument doc;
  doc["enabled"] = state.enabled;
  doc["brightness"] = state.brightness;
  String body;
  serializeJson(doc, body);
  server_.send(200, "application/json", body);
}

void WebConfigServer::updateBacklight() {
  JsonDocument doc;
  if (deserializeJson(doc, server_.arg("plain"))) {
    server_.send(400, "application/json", "{\"error\":\"invalid_json\"}");
    return;
  }
  if (doc["enabled"].is<bool>()) backlight_.setEnabled(doc["enabled"].as<bool>());
  if (doc["brightness"].is<int>()) {
    const int value = doc["brightness"].as<int>();
    if (value < 0 || value > 100) {
      server_.send(422, "application/json", "{\"error\":\"brightness_range\"}");
      return;
    }
    backlight_.setBrightness(static_cast<uint8_t>(value));
  }
  sendBacklightState();
}

void WebConfigServer::sendRgbState() {
  const auto state = rgb_.state();
  JsonDocument doc;
  doc["enabled"] = state.enabled;
  doc["red"] = state.red;
  doc["green"] = state.green;
  doc["blue"] = state.blue;
  doc["brightness"] = state.brightness;
  doc["maxBrightness"] = AppConfig::RGB_MAX_SAFE_BRIGHTNESS;
  const char* effects[] = {"solid", "pulse", "rainbow", "temperature", "load"};
  doc["effect"] = effects[static_cast<uint8_t>(state.effect)];
  doc["speed"] = state.speed;
  char color[8];
  snprintf(color, sizeof(color), "#%02X%02X%02X", state.red, state.green,
           state.blue);
  doc["color"] = color;
  String body;
  serializeJson(doc, body);
  server_.send(200, "application/json", body);
}

void WebConfigServer::updateRgb() {
  JsonDocument doc;
  if (deserializeJson(doc, server_.arg("plain"))) {
    server_.send(400, "application/json", "{\"error\":\"invalid_json\"}");
    return;
  }
  if (doc["enabled"].is<bool>()) rgb_.setEnabled(doc["enabled"].as<bool>());
  if (doc["brightness"].is<int>()) {
    const int value = doc["brightness"].as<int>();
    if (value < 0 || value > AppConfig::RGB_MAX_SAFE_BRIGHTNESS) {
      server_.send(422, "application/json", "{\"error\":\"brightness_range\"}");
      return;
    }
    rgb_.setBrightness(static_cast<uint8_t>(value));
  }
  if (doc["red"].is<int>() && doc["green"].is<int>() && doc["blue"].is<int>()) {
    const int red = doc["red"].as<int>();
    const int green = doc["green"].as<int>();
    const int blue = doc["blue"].as<int>();
    if (red < 0 || red > 255 || green < 0 || green > 255 || blue < 0 || blue > 255) {
      server_.send(422, "application/json", "{\"error\":\"color_range\"}");
      return;
    }
    rgb_.setColor(red, green, blue);
  }
  if (doc["speed"].is<int>()) {
    const int value = doc["speed"].as<int>();
    if (value < 1 || value > 100) {
      server_.send(422, "application/json", "{\"error\":\"speed_range\"}");
      return;
    }
    rgb_.setSpeed(value);
  }
  if (doc["effect"].is<const char*>()) {
    const String effect = doc["effect"].as<String>();
    if (effect == "solid") rgb_.setEffect(RgbLedController::Effect::Solid);
    else if (effect == "pulse") rgb_.setEffect(RgbLedController::Effect::Pulse);
    else if (effect == "rainbow") rgb_.setEffect(RgbLedController::Effect::Rainbow);
    else if (effect == "temperature") rgb_.setEffect(RgbLedController::Effect::Temperature);
    else if (effect == "load") rgb_.setEffect(RgbLedController::Effect::Load);
    else {
      server_.send(422, "application/json", "{\"error\":\"unknown_effect\"}");
      return;
    }
  }
  sendRgbState();
}

void WebConfigServer::sendBootAssetState() {
  JsonDocument doc;
  const String path = BootAssetManager::currentPath();
  doc["exists"] = !path.isEmpty();
  doc["type"] = BootAssetManager::currentType();
  doc["size"] = BootAssetManager::currentSize();
  doc["path"] = path;
  doc["maxBytes"] = AppConfig::BOOT_ASSET_MAX_BYTES;
  doc["filesystemTotal"] = LittleFS.totalBytes();
  doc["filesystemUsed"] = LittleFS.usedBytes();
  String body;
  serializeJson(doc, body);
  server_.send(200, "application/json", body);
}

void WebConfigServer::finishBootAssetUpload() {
  if (!uploadOk_) {
    JsonDocument doc;
    doc["error"] = uploadError_.isEmpty() ? "upload_failed" : uploadError_;
    String body;
    serializeJson(doc, body);
    server_.send(422, "application/json", body);
    return;
  }
  sendBootAssetState();
}

void WebConfigServer::handleBootAssetUpload() {
  HTTPUpload& upload = server_.upload();
  if (upload.status == UPLOAD_FILE_START) {
    uploadOk_ = false;
    uploadSize_ = 0;
    uploadError_ = "";
    uploadExtension_ = upload.filename;
    uploadExtension_.toLowerCase();
    if (uploadExtension_.endsWith(".jpeg")) uploadExtension_ = ".jpg";
    else if (uploadExtension_.endsWith(".jpg")) uploadExtension_ = ".jpg";
    else if (uploadExtension_.endsWith(".png")) uploadExtension_ = ".png";
    else if (uploadExtension_.endsWith(".gif")) uploadExtension_ = ".gif";
    else {
      uploadError_ = "only_png_jpg_gif";
      return;
    }
    LittleFS.remove("/boot.tmp");
    uploadFile_ = LittleFS.open("/boot.tmp", "w");
    if (!uploadFile_) uploadError_ = "filesystem_open_failed";
    return;
  }

  if (upload.status == UPLOAD_FILE_WRITE) {
    if (!uploadFile_ || !uploadError_.isEmpty()) return;
    uploadSize_ += upload.currentSize;
    if (uploadSize_ > AppConfig::BOOT_ASSET_MAX_BYTES) {
      uploadError_ = "file_too_large_1200kb";
      uploadFile_.close();
      LittleFS.remove("/boot.tmp");
      return;
    }
    if (uploadFile_.write(upload.buf, upload.currentSize) != upload.currentSize) {
      uploadError_ = "filesystem_write_failed";
    }
    return;
  }

  if (upload.status == UPLOAD_FILE_END) {
    if (uploadFile_) uploadFile_.close();
    if (!uploadError_.isEmpty() || uploadSize_ == 0) {
      if (uploadError_.isEmpty()) uploadError_ = "empty_file";
      LittleFS.remove("/boot.tmp");
      return;
    }

    File check = LittleFS.open("/boot.tmp", "r");
    uint8_t magic[8] = {};
    const size_t count = check ? check.read(magic, sizeof(magic)) : 0;
    check.close();
    const bool validGif = uploadExtension_ == ".gif" && count >= 3 &&
                          magic[0] == 'G' && magic[1] == 'I' && magic[2] == 'F';
    const bool validPng = uploadExtension_ == ".png" && count >= 8 &&
                          magic[0] == 0x89 && magic[1] == 'P' && magic[2] == 'N' && magic[3] == 'G';
    const bool validJpg = uploadExtension_ == ".jpg" && count >= 2 &&
                          magic[0] == 0xFF && magic[1] == 0xD8;
    if (!validGif && !validPng && !validJpg) {
      uploadError_ = "file_signature_mismatch";
      LittleFS.remove("/boot.tmp");
      return;
    }

    LittleFS.remove("/boot.gif");
    LittleFS.remove("/boot.png");
    LittleFS.remove("/boot.jpg");
    const String target = "/boot" + uploadExtension_;
    uploadOk_ = LittleFS.rename("/boot.tmp", target);
    if (!uploadOk_) uploadError_ = "filesystem_rename_failed";
    return;
  }

  if (upload.status == UPLOAD_FILE_ABORTED) {
    if (uploadFile_) uploadFile_.close();
    LittleFS.remove("/boot.tmp");
    uploadError_ = "upload_aborted";
  }
}

void WebConfigServer::deleteBootAsset() {
  BootAssetManager::removeAll();
  sendBootAssetState();
}

void WebConfigServer::finishOtaUpload() {
  JsonDocument doc;
  if (!otaOk_) {
    doc["ok"] = false;
    doc["error"] = otaError_.isEmpty() ? "ota_failed" : otaError_;
    String body;
    serializeJson(doc, body);
    server_.send(422, "application/json", body);
    return;
  }

  doc["ok"] = true;
  doc["rebooting"] = true;
  doc["written"] = otaWritten_;
  String body;
  serializeJson(doc, body);
  server_.sendHeader("Connection", "close");
  server_.send(200, "application/json", body);
  restartAtMs_ = millis() + 1500;
}

void WebConfigServer::handleOtaUpload() {
  HTTPUpload& upload = server_.upload();

  if (upload.status == UPLOAD_FILE_START) {
    if (Update.isRunning()) Update.abort();
    otaOk_ = false;
    otaStarted_ = false;
    otaFirstPayloadChecked_ = false;
    otaHeaderBytes_ = 0;
    otaExpectedSize_ = 0;
    otaWritten_ = 0;
    otaError_ = "";
    memset(otaHeader_, 0, sizeof(otaHeader_));
    otaStatus_.set(OtaStatus::Phase::Receiving, 0);
    updateOtaDisplay(true);

    String filename = upload.filename;
    filename.toLowerCase();
    if (!filename.endsWith(".pcota")) {
      otaError_ = "only_pcota_file";
      otaStatus_.set(OtaStatus::Phase::Failed, 0);
    }
    return;
  }

  if (upload.status == UPLOAD_FILE_WRITE) {
    if (!otaError_.isEmpty()) return;

    size_t offset = 0;
    while (otaHeaderBytes_ < kOtaHeaderSize && offset < upload.currentSize) {
      otaHeader_[otaHeaderBytes_++] = upload.buf[offset++];
    }

    if (otaHeaderBytes_ == kOtaHeaderSize && !otaStarted_) {
      if (memcmp(otaHeader_, kOtaMagic, sizeof(kOtaMagic)) != 0) {
        otaError_ = "invalid_pcota_signature";
        otaStatus_.set(OtaStatus::Phase::Failed, 0);
        return;
      }
      if (readUint32Le(otaHeader_ + 8) != kOtaFormatVersion) {
        otaError_ = "unsupported_pcota_version";
        otaStatus_.set(OtaStatus::Phase::Failed, 0);
        return;
      }
      otaExpectedSize_ = readUint32Le(otaHeader_ + 12);
      if (otaExpectedSize_ == 0) {
        otaError_ = "empty_firmware";
        otaStatus_.set(OtaStatus::Phase::Failed, 0);
        return;
      }
      if (!Update.begin(otaExpectedSize_, U_FLASH)) {
        otaError_ = String("ota_begin_failed:") + Update.errorString();
        otaStatus_.set(OtaStatus::Phase::Failed, 0);
        return;
      }
      otaStarted_ = true;
    }

    if (!otaStarted_ || offset >= upload.currentSize) return;
    const size_t chunkSize = upload.currentSize - offset;
    if (otaWritten_ + chunkSize > otaExpectedSize_) {
      otaError_ = "pcota_has_extra_data";
      otaStatus_.set(OtaStatus::Phase::Failed, otaStatus_.progress);
      Update.abort();
      otaStarted_ = false;
      return;
    }
    if (!otaFirstPayloadChecked_) {
      if (upload.buf[offset] != 0xE9) {
        otaError_ = "invalid_esp32_image";
        otaStatus_.set(OtaStatus::Phase::Failed, otaStatus_.progress);
        Update.abort();
        otaStarted_ = false;
        return;
      }
      otaFirstPayloadChecked_ = true;
    }
    const size_t written = Update.write(upload.buf + offset, chunkSize);
    if (written != chunkSize) {
      otaError_ = String("ota_write_failed:") + Update.errorString();
      otaStatus_.set(OtaStatus::Phase::Failed, otaStatus_.progress);
      Update.abort();
      otaStarted_ = false;
      return;
    }
    otaWritten_ += written;
    if (otaExpectedSize_ > 0) {
      otaStatus_.set(OtaStatus::Phase::Receiving,
                     static_cast<uint8_t>((otaWritten_ * 100ULL) / otaExpectedSize_));
      updateOtaDisplay();
    }
    return;
  }

  if (upload.status == UPLOAD_FILE_END) {
    if (!otaError_.isEmpty()) {
      if (Update.isRunning()) Update.abort();
      otaStarted_ = false;
      return;
    }
    if (!otaStarted_ || otaHeaderBytes_ != kOtaHeaderSize ||
        otaWritten_ != otaExpectedSize_) {
      otaError_ = "pcota_size_mismatch";
      otaStatus_.set(OtaStatus::Phase::Failed, otaStatus_.progress);
      if (Update.isRunning()) Update.abort();
      otaStarted_ = false;
      return;
    }
    otaStatus_.set(OtaStatus::Phase::Verifying, 100);
    updateOtaDisplay(true);
    if (!Update.end() || !Update.isFinished()) {
      otaError_ = String("ota_finalize_failed:") + Update.errorString();
      otaStatus_.set(OtaStatus::Phase::Failed, 100);
      otaStarted_ = false;
      return;
    }
    otaStarted_ = false;
    otaOk_ = true;
    otaStatus_.set(OtaStatus::Phase::Success, 100);
    updateOtaDisplay(true);
    return;
  }

  if (upload.status == UPLOAD_FILE_ABORTED) {
    if (Update.isRunning()) Update.abort();
    otaStarted_ = false;
    otaError_ = "ota_upload_aborted";
    otaStatus_.set(OtaStatus::Phase::Failed, otaStatus_.progress);
    updateOtaDisplay(true);
  }
}

void WebConfigServer::updateOtaDisplay(bool force) {
  const uint32_t now = millis();
  if (!otaFrameCallback_) return;
  if (!force && now - lastOtaFrameMs_ < 100) return;
  lastOtaFrameMs_ = now;
  otaFrameCallback_();
}

void WebConfigServer::sendTelemetry() {
  JsonDocument doc;
  const auto settings = settings_.state();
  const uint32_t timeoutMs =
      max<uint32_t>(AppConfig::TELEMETRY_STALE_MS,
          static_cast<uint32_t>(settings.telemetryIntervalMs) * 3UL);
  doc["connected"] = settings.telemetryPaused
                         ? telemetry_.hasData()
                         : telemetry_.isFresh(millis(), timeoutMs);
  doc["telemetryIntervalMs"] = settings.telemetryIntervalMs;
  doc["telemetryPaused"] = settings.telemetryPaused;
  doc["cpuTemp"] = telemetry_.cpuTemperature;
  doc["cpuLoad"] = telemetry_.cpuLoad;
  doc["cpuClockMHz"] = telemetry_.cpuClockMHz;
  doc["cpuPowerWatts"] = telemetry_.cpuPowerWatts;
  doc["cpuFanRpm"] = telemetry_.cpuFanRpm;
  doc["gpuTemp"] = telemetry_.gpuTemperature;
  doc["gpuLoad"] = telemetry_.gpuLoad;
  doc["gpuClockMHz"] = telemetry_.gpuClockMHz;
  doc["gpuPowerWatts"] = telemetry_.gpuPowerWatts;
  doc["gpuFanRpm"] = telemetry_.gpuFanRpm;
  doc["gpuMemoryLoad"] = telemetry_.gpuMemoryLoad;
  doc["ramLoad"] = telemetry_.memoryLoad;
  doc["memoryUsedGb"] = telemetry_.memoryUsedGb;
  doc["memoryTotalGb"] = telemetry_.memoryTotalGb;
  doc["fps"] = telemetry_.fps;
  doc["diskLoad"] = telemetry_.diskLoad;
  doc["networkDownMbps"] = telemetry_.networkDownMbps;
  doc["networkUpMbps"] = telemetry_.networkUpMbps;
  doc["loadAverage"] = telemetry_.loadAverage;
  doc["systemUptimeSeconds"] = telemetry_.systemUptimeSeconds;
  doc["processCount"] = telemetry_.processCount;
  doc["cpuCoreCount"] = telemetry_.cpuCoreCount;
  doc["packetCount"] = telemetry_.packetCount;
  doc["telemetryAgeMs"] = telemetry_.hasData()
                              ? millis() - telemetry_.receivedAtMs
                              : 0;
  doc["freeHeap"] = ESP.getFreeHeap();
  doc["uptime"] = millis() / 1000UL;
  String body;
  serializeJson(doc, body);
  server_.send(200, "application/json", body);
}

void WebConfigServer::sendTelemetrySettings() {
  const auto settings = settings_.state();
  JsonDocument doc;
  doc["intervalMs"] = settings.telemetryIntervalMs;
  doc["paused"] = settings.telemetryPaused;
  doc["minimumMs"] = AppConfig::TELEMETRY_INTERVAL_MIN_MS;
  doc["maximumMs"] = AppConfig::TELEMETRY_INTERVAL_MAX_MS;
  const char* mode = "custom";
  if (settings.telemetryIntervalMs == 250) mode = "realtime";
  else if (settings.telemetryIntervalMs == 500) mode = "fast";
  else if (settings.telemetryIntervalMs == 1000) mode = "balanced";
  else if (settings.telemetryIntervalMs == 2000) mode = "slow";
  doc["mode"] = mode;
  String body;
  serializeJson(doc, body);
  server_.send(200, "application/json", body);
}

void WebConfigServer::updateTelemetrySettings() {
  JsonDocument doc;
  if (deserializeJson(doc, server_.arg("plain"))) {
    server_.send(400, "application/json", "{\"error\":\"invalid_json\"}");
    return;
  }
  if (doc["intervalMs"].is<int>()) {
    const int value = doc["intervalMs"].as<int>();
    if (value < AppConfig::TELEMETRY_INTERVAL_MIN_MS ||
        value > AppConfig::TELEMETRY_INTERVAL_MAX_MS) {
      server_.send(422, "application/json",
                   "{\"error\":\"telemetry_interval_range\"}");
      return;
    }
    settings_.setTelemetryInterval(static_cast<uint16_t>(value));
  }
  if (doc["paused"].is<bool>()) {
    settings_.setTelemetryPaused(doc["paused"].as<bool>());
  }
  lastWebSocketPushMs_ = 0;
  sendTelemetrySettings();
}

void WebConfigServer::sendUiSettings() {
  const auto state = settings_.state();
  JsonDocument doc;
  doc["theme"] = state.theme == UiSettings::Theme::Light ? "light" : "dark";
  doc["language"] = state.language == UiSettings::Language::English ? "en" : "vi";
  doc["autoRotate"] = state.autoRotate;
  doc["pageInterval"] = state.pageIntervalSeconds;
  doc["pageMask"] = state.pageMask;
  doc["bootDuration"] = state.bootDurationSeconds;
  doc["logoDuration"] = state.logoPageDurationSeconds;
  doc["dashboardTitle"] = state.dashboardTitle;
  doc["showDate"] = state.showDate;
  doc["cardRadius"] = state.cardRadius;
  doc["rotation"] = state.rotation;
  doc["fontScale"] = state.fontScalePercent;
  doc["headerBold"] = state.headerBold;
  doc["labelBold"] = state.labelBold;
  doc["valueBold"] = state.valueBold;
  doc["customTextColors"] = state.customTextColors;
  doc["primaryTextColor"] = rgb565ToHex(state.primaryTextColor);
  doc["secondaryTextColor"] = rgb565ToHex(state.secondaryTextColor);
  const char* transitionStyle = "slide";
  if (state.transitionStyle == UiSettings::TransitionStyle::None) {
    transitionStyle = "none";
  } else if (state.transitionStyle == UiSettings::TransitionStyle::Rise) {
    transitionStyle = "rise";
  }
  doc["transitionStyle"] = transitionStyle;
  doc["transitionDurationMs"] = state.transitionDurationMs;
  doc["smoothValues"] = state.smoothValues;
  JsonArray colors = doc["cardColors"].to<JsonArray>();
  for (const uint16_t color : state.cardColors) colors.add(rgb565ToHex(color));
  JsonArray order = doc["pageOrder"].to<JsonArray>();
  for (const uint8_t page : state.pageOrder) order.add(page);
  String body;
  serializeJson(doc, body);
  server_.send(200, "application/json", body);
}

void WebConfigServer::updateUiSettings() {
  JsonDocument doc;
  if (deserializeJson(doc, server_.arg("plain"))) {
    server_.send(400, "application/json", "{\"error\":\"invalid_json\"}");
    return;
  }
  if (doc["theme"].is<const char*>()) {
    const String theme = doc["theme"].as<String>();
    if (theme != "dark" && theme != "light") {
      server_.send(422, "application/json", "{\"error\":\"unknown_theme\"}");
      return;
    }
    settings_.setTheme(theme == "light" ? UiSettings::Theme::Light : UiSettings::Theme::Dark);
  }
  if (doc["language"].is<const char*>()) {
    const String language = doc["language"].as<String>();
    if (language != "vi" && language != "en") {
      server_.send(422, "application/json", "{\"error\":\"unknown_language\"}");
      return;
    }
    settings_.setLanguage(language == "en" ? UiSettings::Language::English
                                             : UiSettings::Language::Vietnamese);
  }
  if (doc["rotation"].is<int>()) {
    const int value = doc["rotation"].as<int>();
    if (value < 0 || value > 3) {
      server_.send(422, "application/json", "{\"error\":\"rotation_range\"}");
      return;
    }
    settings_.setRotation(static_cast<uint8_t>(value));
  }
  if (doc["autoRotate"].is<bool>()) settings_.setAutoRotate(doc["autoRotate"].as<bool>());
  if (doc["pageInterval"].is<int>()) {
    const int value = doc["pageInterval"].as<int>();
    if (value < 2 || value > 30) {
      server_.send(422, "application/json", "{\"error\":\"page_interval_range\"}");
      return;
    }
    settings_.setPageInterval(value);
  }
  if (doc["pageMask"].is<int>()) settings_.setPageMask(doc["pageMask"].as<int>());
  if (doc["bootDuration"].is<int>()) {
    const int value = doc["bootDuration"].as<int>();
    if (value < 1 || value > 10) {
      server_.send(422, "application/json", "{\"error\":\"boot_duration_range\"}");
      return;
    }
    settings_.setBootDuration(value);
  }
  if (doc["logoDuration"].is<int>()) {
    const int value = doc["logoDuration"].as<int>();
    if (value < 2 || value > 30) {
      server_.send(422, "application/json", "{\"error\":\"logo_duration_range\"}");
      return;
    }
    settings_.setLogoPageDuration(value);
  }
  if (doc["dashboardTitle"].is<const char*>()) {
    const String title = doc["dashboardTitle"].as<String>();
    if (title.isEmpty() || title.length() > 12) {
      server_.send(422, "application/json", "{\"error\":\"dashboard_title_range\"}");
      return;
    }
    settings_.setDashboardTitle(title);
  }
  if (doc["showDate"].is<bool>()) settings_.setShowDate(doc["showDate"].as<bool>());
  if (doc["cardRadius"].is<int>()) {
    const int value = doc["cardRadius"].as<int>();
    if (value < 2 || value > 18) {
      server_.send(422, "application/json", "{\"error\":\"card_radius_range\"}");
      return;
    }
    settings_.setCardRadius(value);
  }
  if (doc["fontScale"].is<int>()) {
    const int value = doc["fontScale"].as<int>();
    if (value < 85 || value > 115) {
      server_.send(422, "application/json", "{\"error\":\"font_scale_range\"}");
      return;
    }
    settings_.setFontScale(static_cast<uint8_t>(value));
  }
  if (doc["headerBold"].is<bool>()) settings_.setHeaderBold(doc["headerBold"].as<bool>());
  if (doc["labelBold"].is<bool>()) settings_.setLabelBold(doc["labelBold"].as<bool>());
  if (doc["valueBold"].is<bool>()) settings_.setValueBold(doc["valueBold"].as<bool>());
  if (doc["customTextColors"].is<bool>()) {
    settings_.setCustomTextColors(doc["customTextColors"].as<bool>());
  }
  if (doc["primaryTextColor"].is<const char*>()) {
    uint16_t color;
    if (!hexToRgb565(doc["primaryTextColor"].as<String>(), color)) {
      server_.send(422, "application/json", "{\"error\":\"primary_text_color\"}");
      return;
    }
    settings_.setPrimaryTextColor(color);
  }
  if (doc["secondaryTextColor"].is<const char*>()) {
    uint16_t color;
    if (!hexToRgb565(doc["secondaryTextColor"].as<String>(), color)) {
      server_.send(422, "application/json", "{\"error\":\"secondary_text_color\"}");
      return;
    }
    settings_.setSecondaryTextColor(color);
  }
  if (doc["transitionStyle"].is<const char*>()) {
    const String style = doc["transitionStyle"].as<String>();
    if (style == "none") {
      settings_.setTransitionStyle(UiSettings::TransitionStyle::None);
    } else if (style == "slide") {
      settings_.setTransitionStyle(UiSettings::TransitionStyle::Slide);
    } else if (style == "rise") {
      settings_.setTransitionStyle(UiSettings::TransitionStyle::Rise);
    } else {
      server_.send(422, "application/json",
                   "{\"error\":\"unknown_transition\"}");
      return;
    }
  }
  if (doc["transitionDurationMs"].is<int>()) {
    const int value = doc["transitionDurationMs"].as<int>();
    if (value < 180 || value > 900) {
      server_.send(422, "application/json",
                   "{\"error\":\"transition_duration_range\"}");
      return;
    }
    settings_.setTransitionDuration(static_cast<uint16_t>(value));
  }
  if (doc["smoothValues"].is<bool>()) {
    settings_.setSmoothValues(doc["smoothValues"].as<bool>());
  }
  if (doc["cardColors"].is<JsonArrayConst>()) {
    const JsonArrayConst colors = doc["cardColors"].as<JsonArrayConst>();
    if (colors.size() != 6) {
      server_.send(422, "application/json", "{\"error\":\"six_card_colors_required\"}");
      return;
    }
    uint8_t index = 0;
    for (const JsonVariantConst item : colors) {
      uint16_t color = 0;
      if (!item.is<const char*>() || !hexToRgb565(item.as<String>(), color)) {
        server_.send(422, "application/json", "{\"error\":\"invalid_card_color\"}");
        return;
      }
      settings_.setCardColor(index++, color);
    }
  }
  if (doc["pageOrder"].is<JsonArrayConst>()) {
    const JsonArrayConst values = doc["pageOrder"].as<JsonArrayConst>();
    if (values.size() != 5) {
      server_.send(422, "application/json", "{\"error\":\"five_pages_required\"}");
      return;
    }
    std::array<uint8_t, 5> order{};
    uint8_t index = 0;
    for (const JsonVariantConst item : values) {
      if (!item.is<int>()) {
        server_.send(422, "application/json", "{\"error\":\"invalid_page_order\"}");
        return;
      }
      order[index++] = item.as<int>();
    }
    if (!settings_.setPageOrder(order)) {
      server_.send(422, "application/json", "{\"error\":\"invalid_page_order\"}");
      return;
    }
  }
  sendUiSettings();
}

void WebConfigServer::sendNetworkState() {
  const auto state = network_.state();
  const auto clock = network_.clock();
  JsonDocument doc;
  doc["ssid"] = state.ssid;
  doc["passwordSet"] = state.passwordSet;
  doc["connected"] = state.connected;
  doc["timeSynced"] = state.timeSynced;
  doc["utcOffsetMinutes"] = state.utcOffsetMinutes;
  doc["rssi"] = state.rssi;
  doc["stationIp"] = state.stationIp;
  doc["accessPointIp"] = state.accessPointIp;
  doc["time"] = clock.time;
  doc["date"] = clock.date;
  String body;
  serializeJson(doc, body);
  server_.send(200, "application/json", body);
}

void WebConfigServer::updateNetwork() {
  JsonDocument doc;
  if (deserializeJson(doc, server_.arg("plain"))) {
    server_.send(400, "application/json", "{\"error\":\"invalid_json\"}");
    return;
  }
  const auto current = network_.state();
  const String ssid = doc["ssid"].is<const char*>()
                          ? doc["ssid"].as<String>()
                          : current.ssid;
  if (ssid.length() > 32) {
    server_.send(422, "application/json", "{\"error\":\"ssid_too_long\"}");
    return;
  }
  const bool updatePassword = doc["password"].is<const char*>();
  const String password = updatePassword ? doc["password"].as<String>() : String();
  if (password.length() > 63) {
    server_.send(422, "application/json", "{\"error\":\"password_too_long\"}");
    return;
  }
  const int offset = doc["utcOffsetMinutes"].is<int>()
                         ? doc["utcOffsetMinutes"].as<int>()
                         : current.utcOffsetMinutes;
  if (offset < -720 || offset > 840) {
    server_.send(422, "application/json", "{\"error\":\"utc_offset_range\"}");
    return;
  }
  network_.updateConfig(ssid, password, updatePassword,
                        static_cast<int16_t>(offset));
  sendNetworkState();
}

void WebConfigServer::scanWifi() {
  const int count = WiFi.scanNetworks(false, true);
  JsonDocument doc;
  doc["type"] = "wifiScan";
  JsonArray networks = doc["networks"].to<JsonArray>();
  if (count >= 0) {
    for (int index = 0; index < count && networks.size() < 20; ++index) {
      const String ssid = WiFi.SSID(index);
      if (ssid.isEmpty()) continue;
      bool duplicate = false;
      for (JsonObject existing : networks) {
        if (String(existing["ssid"] | "") == ssid) {
          duplicate = true;
          break;
        }
      }
      if (duplicate) continue;
      JsonObject item = networks.add<JsonObject>();
      item["ssid"] = ssid;
      item["rssi"] = WiFi.RSSI(index);
      item["channel"] = WiFi.channel(index);
      item["secure"] = WiFi.encryptionType(index) != WIFI_AUTH_OPEN;
    }
  }
  doc["count"] = networks.size();
  WiFi.scanDelete();
  String body;
  serializeJson(doc, body);
  socket_.broadcastTXT(body);
  server_.send(count < 0 ? 503 : 200, "application/json", body);
}

void WebConfigServer::sendSystemState() {
  const auto backlight = backlight_.state();
  const auto network = network_.state();
  JsonDocument doc;
  doc["firmware"] = AppConfig::FIRMWARE_VERSION;
  doc["uptimeSeconds"] = millis() / 1000UL;
  doc["freeHeap"] = ESP.getFreeHeap();
  doc["flashBytes"] = ESP.getFlashChipSize();
  doc["sketchBytes"] = ESP.getSketchSize();
  doc["freeSketchBytes"] = ESP.getFreeSketchSpace();
  doc["filesystemTotal"] = LittleFS.totalBytes();
  doc["filesystemUsed"] = LittleFS.usedBytes();
  doc["displayEnabled"] = backlight.enabled;
  doc["brightness"] = backlight.brightness;
  doc["wifiConnected"] = network.connected;
  doc["resetReason"] = static_cast<int>(esp_reset_reason());
  String body;
  serializeJson(doc, body);
  server_.send(200, "application/json", body);
}

void WebConfigServer::sendCloudState() {
  const auto state = cloud_.status();
  JsonDocument doc;
  doc["deviceId"] = state.deviceId;
  doc["mac"] = state.mac;
  doc["serverUrl"] = state.serverUrl;
  doc["currentVersion"] = state.currentVersion;
  doc["latestVersion"] = state.latestVersion;
  doc["lastError"] = state.lastError;
  doc["channel"] = state.channel;
  doc["internetConnected"] = state.internetConnected;
  doc["serverReachable"] = state.serverReachable;
  doc["registered"] = state.registered;
  doc["deviceEnabled"] = state.deviceEnabled;
  doc["autoProvision"] = state.autoProvision;
  doc["updateAvailable"] = state.updateAvailable;
  doc["mandatory"] = state.mandatory;
  doc["lastCheckMs"] = state.lastCheckMs;
  doc["nextCheckMs"] = state.nextCheckMs;
  doc["effectCount"] = state.effectCount;
  doc["selectedEffectId"] = state.selectedEffectId;
  doc["effectsUpdatedAt"] = state.effectsUpdatedAt;
  doc["pagesUpdatedAt"] = state.pagesUpdatedAt;
  doc["pagesAvailable"] = state.pagesAvailable;
  doc["reportingEnabled"] = state.reportingEnabled;
  doc["lastReportOk"] = state.lastReportOk;
  doc["lastReportMs"] = state.lastReportMs;
  String body;
  serializeJson(doc, body);
  server_.send(200, "application/json", body);
}

void WebConfigServer::sendCloudEffects() {
  server_.send(200, "application/json", cloudEffectsJson());
}

void WebConfigServer::sendCloudPages() {
  const auto pages = cloud_.pageSetup();
  JsonDocument doc;
  doc["available"] = pages.available;
  doc["language"] = pages.language;
  doc["pageMask"] = pages.mask;
  doc["updatedAt"] = pages.updatedAt;
  JsonArray order = doc["pageOrder"].to<JsonArray>();
  for (const uint8_t page : pages.order) order.add(page);
  String body;
  serializeJson(doc, body);
  server_.send(200, "application/json", body);
}

void WebConfigServer::applyCloudPages() {
  const auto pages = cloud_.pageSetup();
  if (!pages.available) {
    server_.send(409, "application/json", "{\"error\":\"pages_not_synced\"}");
    return;
  }
  settings_.setPageOrder(pages.order);
  settings_.setPageMask(pages.mask);
  settings_.setLanguage(pages.language == "en"
                            ? UiSettings::Language::English
                            : UiSettings::Language::Vietnamese);
  server_.send(200, "application/json", "{\"ok\":true}");
}

String WebConfigServer::cloudEffectsJson() const {
  JsonDocument doc;
  doc["type"] = "cloudEffects";
  doc["selectedId"] = cloud_.status().selectedEffectId;
  JsonArray effects = doc["effects"].to<JsonArray>();
  for (size_t index = 0; index < cloud_.effectCount(); ++index) {
    const auto* source = cloud_.effectAt(index);
    if (!source) continue;
    JsonObject effect = effects.add<JsonObject>();
    effect["id"] = source->id;
    effect["name"] = source->name;
    effect["type"] = source->type;
    effect["enabled"] = source->enabled;
    effect["speed"] = source->speed;
    effect["brightness"] = source->brightness;
    JsonArray palette = effect["palette"].to<JsonArray>();
    for (uint8_t color = 0; color < source->paletteSize; ++color)
      palette.add(source->palette[color]);
    JsonObject display = effect["display"].to<JsonObject>();
    if (!source->theme.isEmpty()) display["theme"] = source->theme;
    if (!source->language.isEmpty()) display["language"] = source->language;
    if (source->rotation >= 0) display["rotation"] = source->rotation;
    if (source->autoRotate >= 0)
      display["autoRotate"] = source->autoRotate == 1;
    if (source->pageInterval >= 0)
      display["pageInterval"] = source->pageInterval;
    if (source->pageMask >= 0) display["pageMask"] = source->pageMask;
    if (source->hasPageOrder) {
      JsonArray order = display["pageOrder"].to<JsonArray>();
      for (const uint8_t page : source->pageOrder) order.add(page);
    }
  }
  String body;
  serializeJson(doc, body);
  return body;
}

void WebConfigServer::broadcastCloudEffects(int client) {
  String body = cloudEffectsJson();
  if (client >= 0) socket_.sendTXT(static_cast<uint8_t>(client), body);
  else socket_.broadcastTXT(body);
}

bool WebConfigServer::applyCloudEffect(const String& id, String& error) {
  const auto* effect = cloud_.findEffect(id);
  if (!effect) {
    error = "effect_not_found";
    return false;
  }
  if (!effect->enabled) {
    error = "effect_disabled";
    return false;
  }
  if (effect->type == "gradient" || effect->type == "rainbow")
    rgb_.setEffect(RgbLedController::Effect::Rainbow);
  else if (effect->type == "pulse")
    rgb_.setEffect(RgbLedController::Effect::Pulse);
  else if (effect->type == "temperature")
    rgb_.setEffect(RgbLedController::Effect::Temperature);
  else if (effect->type == "load")
    rgb_.setEffect(RgbLedController::Effect::Load);
  else if (effect->type == "solid")
    rgb_.setEffect(RgbLedController::Effect::Solid);
  else {
    error = "effect_type_unsupported";
    return false;
  }
  if (effect->paletteSize > 0) {
    uint8_t red = 0, green = 0, blue = 0;
    if (hexToRgb888(effect->palette[0], red, green, blue))
      rgb_.setColor(red, green, blue);
  }
  rgb_.setBrightness(min<uint8_t>(effect->brightness,
                                  AppConfig::RGB_MAX_SAFE_BRIGHTNESS));
  rgb_.setSpeed(effect->speed);
  rgb_.setEnabled(true);
  if (effect->theme == "light") settings_.setTheme(UiSettings::Theme::Light);
  else if (effect->theme == "dark") settings_.setTheme(UiSettings::Theme::Dark);
  if (effect->language == "en") settings_.setLanguage(UiSettings::Language::English);
  else if (effect->language == "vi") settings_.setLanguage(UiSettings::Language::Vietnamese);
  if (effect->rotation >= 0) settings_.setRotation(effect->rotation);
  if (effect->autoRotate >= 0) settings_.setAutoRotate(effect->autoRotate == 1);
  if (effect->pageInterval >= 0)
    settings_.setPageInterval(effect->pageInterval);
  if (effect->pageMask >= 0) settings_.setPageMask(effect->pageMask);
  if (effect->hasPageOrder) settings_.setPageOrder(effect->pageOrder);
  cloud_.selectEffect(effect->id);
  broadcastCloudEffects();
  return true;
}

void WebConfigServer::applyCloudEffectRequest() {
  JsonDocument request;
  if (deserializeJson(request, server_.arg("plain"))) {
    server_.send(400, "application/json", "{\"error\":\"invalid_json\"}");
    return;
  }
  String error;
  if (!applyCloudEffect(String(request["id"] | ""), error)) {
    JsonDocument response;
    response["error"] = error;
    String body;
    serializeJson(response, body);
    server_.send(422, "application/json", body);
    return;
  }
  JsonDocument response;
  response["ok"] = true;
  response["selectedId"] = cloud_.status().selectedEffectId;
  String body;
  serializeJson(response, body);
  server_.send(200, "application/json", body);
}

void WebConfigServer::checkCloud() {
  if (otaStarted_) {
    server_.send(409, "application/json", "{\"error\":\"ota_in_progress\"}");
    return;
  }
  const bool ok = cloud_.checkNow();
  if (!ok) {
    const auto state = cloud_.status();
    JsonDocument doc;
    doc["error"] = state.lastError;
    doc["serverReachable"] = false;
    String body;
    serializeJson(doc, body);
    server_.send(502, "application/json", body);
    return;
  }
  broadcastCloudEffects();
  sendCloudState();
}

void WebConfigServer::installCloudUpdate() {
  if (otaStarted_) {
    server_.send(409, "application/json", "{\"error\":\"ota_in_progress\"}");
    return;
  }
  otaStarted_ = true;
  const bool ok = cloud_.installAvailableUpdate([this]() {
    updateOtaDisplay(true);
  });
  if (!ok) {
    otaStarted_ = false;
    const auto state = cloud_.status();
    JsonDocument doc;
    doc["error"] = state.lastError;
    String body;
    serializeJson(doc, body);
    server_.send(422, "application/json", body);
    return;
  }
  server_.sendHeader("Connection", "close");
  server_.send(200, "application/json",
               "{\"ok\":true,\"restarting\":true}");
  restartAtMs_ = millis() + 1500;
}

void WebConfigServer::restartSystem() {
  if (otaStarted_) {
    server_.send(409, "application/json", "{\"error\":\"ota_in_progress\"}");
    return;
  }
  server_.sendHeader("Connection", "close");
  server_.send(202, "application/json",
               "{\"ok\":true,\"restarting\":true}");
  restartAtMs_ = millis() + 1200;
}

void WebConfigServer::handleWebSocketEvent(uint8_t client, WStype_t type,
                                            uint8_t* payload, size_t length) {
  if (type == WStype_CONNECTED) {
    socket_.sendTXT(client, String("{\"type\":\"ready\",\"firmware\":\"") +
                                AppConfig::FIRMWARE_VERSION + "\"}");
    broadcastCloudEffects(client);
    return;
  }
  if (type != WStype_TEXT || !payload || length == 0) return;

  JsonDocument request;
  JsonDocument response;
  response["type"] = "commandAck";
  if (deserializeJson(request, payload, length)) {
    response["ok"] = false;
    response["error"] = "invalid_json";
  } else {
    const String command = request["command"] | "";
    response["command"] = command;
    if (command == "display" && request["enabled"].is<bool>()) {
      backlight_.setEnabled(request["enabled"].as<bool>());
      response["ok"] = true;
      response["enabled"] = backlight_.state().enabled;
    } else if (command == "brightness" && request["value"].is<int>()) {
      const int value = request["value"].as<int>();
      if (value >= 0 && value <= 100) {
        backlight_.setBrightness(static_cast<uint8_t>(value));
        response["ok"] = true;
        response["brightness"] = backlight_.state().brightness;
      } else {
        response["ok"] = false;
        response["error"] = "brightness_range";
      }
    } else if (command == "syncCloud" && !otaStarted_) {
      const bool ok = cloud_.checkNow();
      response["ok"] = ok;
      if (ok) broadcastCloudEffects();
      else response["error"] = cloud_.status().lastError;
    } else if (command == "applyCloudEffect" && request["id"].is<const char*>()) {
      String error;
      const bool ok = applyCloudEffect(request["id"].as<String>(), error);
      response["ok"] = ok;
      response["selectedId"] = cloud_.status().selectedEffectId;
      if (!ok) response["error"] = error;
    } else if (command == "restart" && !otaStarted_) {
      response["ok"] = true;
      response["restarting"] = true;
      restartAtMs_ = millis() + 1200;
    } else {
      response["ok"] = false;
      response["error"] = otaStarted_ ? "ota_in_progress" : "invalid_command";
    }
  }
  String body;
  serializeJson(response, body);
  socket_.sendTXT(client, body);
}

void WebConfigServer::pushRealtime() {
  const auto cloudState = cloud_.status();
  if (cloudState.effectCount != lastBroadcastEffectCount_ ||
      cloudState.effectsUpdatedAt != lastBroadcastEffectsUpdatedAt_) {
    lastBroadcastEffectCount_ = cloudState.effectCount;
    lastBroadcastEffectsUpdatedAt_ = cloudState.effectsUpdatedAt;
    broadcastCloudEffects();
  }
  JsonDocument doc;
  const auto settings = settings_.state();
  doc["type"] = "telemetry";
  const uint32_t timeoutMs =
      max<uint32_t>(AppConfig::TELEMETRY_STALE_MS,
          static_cast<uint32_t>(settings.telemetryIntervalMs) * 3UL);
  doc["connected"] = settings.telemetryPaused
                         ? telemetry_.hasData()
                         : telemetry_.isFresh(millis(), timeoutMs);
  doc["telemetryIntervalMs"] = settings.telemetryIntervalMs;
  doc["telemetryPaused"] = settings.telemetryPaused;
  doc["cpuTemp"] = telemetry_.cpuTemperature;
  doc["cpuLoad"] = telemetry_.cpuLoad;
  doc["cpuFanRpm"] = telemetry_.cpuFanRpm;
  doc["gpuTemp"] = telemetry_.gpuTemperature;
  doc["gpuLoad"] = telemetry_.gpuLoad;
  doc["gpuFanRpm"] = telemetry_.gpuFanRpm;
  doc["ramLoad"] = telemetry_.memoryLoad;
  doc["fps"] = telemetry_.fps;
  doc["diskLoad"] = telemetry_.diskLoad;
  doc["networkDownMbps"] = telemetry_.networkDownMbps;
  doc["networkUpMbps"] = telemetry_.networkUpMbps;
  doc["loadAverage"] = telemetry_.loadAverage;
  doc["systemUptimeSeconds"] = telemetry_.systemUptimeSeconds;
  doc["processCount"] = telemetry_.processCount;
  doc["packetCount"] = telemetry_.packetCount;
  doc["telemetryAgeMs"] = telemetry_.hasData()
                              ? millis() - telemetry_.receivedAtMs
                              : 0;
  doc["freeHeap"] = ESP.getFreeHeap();
  doc["filesystemTotal"] = LittleFS.totalBytes();
  doc["filesystemUsed"] = LittleFS.usedBytes();
  const auto backlight = backlight_.state();
  doc["displayEnabled"] = backlight.enabled;
  doc["brightness"] = backlight.brightness;
  const auto network = network_.state();
  doc["wifiConnected"] = network.connected;
  doc["wifiRssi"] = network.rssi;
  const auto clock = network_.clock();
  doc["time"] = clock.time;
  doc["date"] = clock.date;
  String body;
  serializeJson(doc, body);
  socket_.broadcastTXT(body);
}
