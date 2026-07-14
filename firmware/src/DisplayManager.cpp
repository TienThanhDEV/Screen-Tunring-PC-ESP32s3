#include "DisplayManager.h"

#include <cmath>

#include "AppConfig.h"
#include "BoardConfig.h"
#include "VietnameseBitmaps.h"

namespace {
constexpr uint16_t kAmdRed = 0xF8C3;
constexpr uint16_t kAmdOrange = 0xFC60;
constexpr uint16_t kCyan = 0x2DFF;
constexpr uint16_t kLime = 0xAFE5;
constexpr uint16_t kPanel = 0x0861;
constexpr uint16_t kPanelBorder = 0x2124;
uint32_t background(bool light) { return light ? TFT_WHITE : TFT_BLACK; }
uint32_t foreground(bool light) { return light ? 0x1082 : TFT_WHITE; }
uint32_t muted(bool light) { return light ? 0x5ACB : 0x9CF3; }
uint32_t track(bool light) { return light ? 0xCE79 : 0x2104; }

void drawBitmapCentered(lgfx::LGFX_Sprite& canvas,
                        const ViText::Bitmap& bitmap, int y,
                        uint16_t color) {
  canvas.drawXBitmap((240 - bitmap.width) / 2, y, bitmap.data, bitmap.width,
                     bitmap.height, color);
}

void drawVietnameseHeader(lgfx::LGFX_Sprite& canvas, uint8_t page,
                          bool systemPage, uint16_t color) {
  const ViText::Bitmap* bitmap = &ViText::kOverview;
  if (page == 1) bitmap = &ViText::kTemperature;
  else if (page == 2) bitmap = &ViText::kCooling;
  else if (page == 3) bitmap = systemPage ? &ViText::kSystem
                                           : &ViText::kPerformance;
  canvas.drawXBitmap(18, 9, bitmap->data, bitmap->width, bitmap->height,
                     color);
}

float percent(float value) {
  return isnan(value) ? 0.0f : constrain(value, 0.0f, 100.0f);
}

uint16_t temperatureColor(float value) {
  if (isnan(value) || value < 65.0f) return kCyan;
  if (value < 82.0f) return kAmdOrange;
  return kAmdRed;
}
}  // namespace

DisplayManager::DisplayManager(DisplayDevice& display,
                               BootAssetManager& bootAsset,
                               const OtaStatus& otaStatus)
    : display_(display), bootAsset_(bootAsset), otaStatus_(otaStatus),
      canvas_(&display) {}

bool DisplayManager::begin() {
  if (!display_.init()) return false;
  display_.setRotation(0);
  display_.setColorDepth(16);
  canvas_.setColorDepth(16);
  return canvas_.createSprite(BoardConfig::TFT_WIDTH, BoardConfig::TFT_HEIGHT) != nullptr;
}

void DisplayManager::setRotation(uint8_t rotation) {
  rotation &= 0x03U;
  if (rotation_ == rotation) return;
  rotation_ = rotation;
  display_.setRotation(rotation_);
}

void DisplayManager::showBoot() {
  transitionActive_ = false;
  canvas_.setTextSize(1.0f);
  canvas_.fillScreen(TFT_BLACK);
  canvas_.fillCircle(120, 91, 44, 0x055D);
  canvas_.drawCircle(120, 91, 48, 0x2DFF);
  canvas_.setTextDatum(middle_center);
  canvas_.setTextColor(TFT_WHITE);
  canvas_.setFont(&fonts::Font4);
  canvas_.drawString("PC", 120, 91);
  canvas_.setFont(&fonts::Font2);
  canvas_.setTextColor(0xBDF7);
  canvas_.drawString("PC SCREEN S3", 120, 157);
  canvas_.setTextColor(0x5E7F);
  canvas_.drawString(String("Firmware ") + AppConfig::FIRMWARE_VERSION, 120,
                     184);
  canvas_.pushSprite(0, 0);
}

void DisplayManager::showProvisioning(UiSettings::Language language,
                                      const String& apSsid,
                                      const String& apPassword,
                                      const String& address) {
  transitionActive_ = false;
  language_ = language;
  canvas_.setTextSize(1.0f);
  canvas_.fillScreen(TFT_BLACK);
  canvas_.fillRoundRect(5, 5, 230, 230, 18, 0x0861);
  canvas_.drawRoundRect(5, 5, 230, 230, 18, 0x2DFF);

  if (language == UiSettings::Language::Vietnamese) {
    drawBitmapCentered(canvas_, ViText::kConnectWifi, 14, TFT_WHITE);
  } else {
    canvas_.setTextDatum(top_center);
    canvas_.setFont(&fonts::Font2);
    canvas_.setTextColor(TFT_WHITE);
    canvas_.drawString("CONNECT TO WI-FI", 120, 12);
  }

  const String qrPayload = String("WIFI:T:WPA;S:") + apSsid + ";P:" +
                           apPassword + ";;";
  canvas_.qrcode(qrPayload.c_str(), 8, 43, 124, 1, true);

  canvas_.setTextDatum(top_left);
  canvas_.setFont(&fonts::Font0);
  canvas_.setTextColor(0x9CF3);
  canvas_.drawString(language == UiSettings::Language::English
                         ? "NETWORK"
                         : "TEN MANG",
                     140, 49);
  canvas_.setTextColor(TFT_WHITE);
  canvas_.drawString(apSsid, 140, 65);
  canvas_.setTextColor(0x9CF3);
  canvas_.drawString(language == UiSettings::Language::English
                         ? "PASSWORD"
                         : "MAT KHAU",
                     140, 91);
  canvas_.setTextColor(TFT_WHITE);
  canvas_.drawString(apPassword, 140, 107);
  canvas_.setTextColor(0x9CF3);
  canvas_.drawString("WEB", 140, 133);
  canvas_.setTextColor(kCyan);
  canvas_.drawString(address, 140, 149);

  if (language == UiSettings::Language::Vietnamese) {
    drawBitmapCentered(canvas_, ViText::kScanQr, 188, kCyan);
  } else {
    canvas_.setTextDatum(middle_center);
    canvas_.setFont(&fonts::Font2);
    canvas_.setTextColor(kCyan);
    canvas_.drawString("SCAN QR CODE", 120, 198);
  }
  canvas_.setTextDatum(middle_center);
  canvas_.setFont(&fonts::Font0);
  canvas_.setTextColor(0x9CF3);
  canvas_.drawString("PCScreen-Setup", 120, 220);
  canvas_.pushSprite(0, 0);
}

void DisplayManager::showWelcome(UiSettings::Language language,
                                 uint32_t now) {
  transitionActive_ = false;
  language_ = language;
  const uint8_t pulse = 6 + static_cast<uint8_t>((now / 90U) % 8U);
  canvas_.setTextSize(1.0f);
  canvas_.fillScreen(TFT_BLACK);
  canvas_.fillCircle(120, 91, 51 + pulse / 2, 0x0861);
  canvas_.drawCircle(120, 91, 51 + pulse / 2, kCyan);
  canvas_.fillCircle(120, 91, 42, 0x02A5);
  canvas_.setTextDatum(middle_center);
  canvas_.setFont(&fonts::Font4);
  canvas_.setTextColor(TFT_WHITE);
  canvas_.drawString("OK", 120, 91);
  if (language == UiSettings::Language::Vietnamese) {
    drawBitmapCentered(canvas_, ViText::kWelcome, 154, TFT_WHITE);
  } else {
    canvas_.setFont(&fonts::Font4);
    canvas_.drawString("WELCOME", 120, 166);
  }
  canvas_.setFont(&fonts::Font0);
  canvas_.setTextColor(0x9CF3);
  canvas_.drawString(language == UiSettings::Language::English
                         ? "Wi-Fi connected"
                         : "Wi-Fi da ket noi",
                     120, 201);
  canvas_.pushSprite(0, 0);
}

void DisplayManager::showOtaAnimation(uint32_t now) {
  transitionActive_ = false;
  renderOta(now);
  canvas_.pushSprite(0, 0);
}

void DisplayManager::render(const TelemetryData& data, bool connected,
                            const String& address,
                            const UiSettings::State& settings,
                            const NetworkTimeManager::ClockSnapshot& clock,
                            uint32_t now) {
  setRotation(settings.rotation);
  language_ = settings.language;
  fontScale_ = settings.fontScalePercent / 100.0f;
  headerBold_ = settings.headerBold;
  labelBold_ = settings.labelBold;
  valueBold_ = settings.valueBold;
  customTextColors_ = settings.customTextColors;
  primaryTextColor_ = settings.primaryTextColor;
  secondaryTextColor_ = settings.secondaryTextColor;
  transitionStyle_ = settings.transitionStyle;
  transitionDurationMs_ = settings.transitionDurationMs;
  const TelemetryData& viewData =
      smoothTelemetry(data, settings.smoothValues, now);
  if (otaStatus_.visible(now)) {
    bootAsset_.endPlayback();
    activePage_ = 0xFF;
    transitionActive_ = false;
    renderOta(now);
    canvas_.pushSprite(0, 0);
    return;
  }
  if (activePage_ == 0xFF) {
    lastPageChangeMs_ = now;
    for (const uint8_t page : settings.pageOrder) {
      if (settings.pageMask & (1U << page)) {
        currentPage_ = page;
        break;
      }
    }
  }
  if ((settings.pageMask & (1U << currentPage_)) == 0) {
    advancePage(settings.pageMask, settings.pageOrder);
    lastPageChangeMs_ = now;
  }
  const uint16_t pageDuration = currentPage_ == 4
                                    ? settings.logoPageDurationSeconds
                                    : settings.pageIntervalSeconds;
  if (settings.autoRotate && now - lastPageChangeMs_ >= pageDuration * 1000UL) {
    advancePage(settings.pageMask, settings.pageOrder);
    lastPageChangeMs_ = now;
  }
  if (activePage_ != currentPage_) {
    const bool hadActivePage = activePage_ != 0xFF;
    bootAsset_.endPlayback();
    activePage_ = currentPage_;
    transitionActive_ = hadActivePage && currentPage_ != 4 &&
                        transitionStyle_ != UiSettings::TransitionStyle::None;
    transitionStartedMs_ = now;
    logoAvailable_ = currentPage_ == 4 && bootAsset_.beginPlayback();
  }
  if (currentPage_ == 4) {
    if (logoAvailable_) {
      bootAsset_.updatePlayback(now);
      if (!bootAsset_.isPlaying()) logoAvailable_ = false;
    }
    if (!logoAvailable_) logoMissing();
    return;
  }
  if (currentPage_ == 0) {
    bentoDashboard(viewData, connected, settings, clock);
    present(now);
    return;
  }
  // Detail pages intentionally stay black for the MI50 enclosure theme.
  const bool detailLight = false;
  canvas_.fillScreen(TFT_BLACK);
  const char* titles[] = {"OVERVIEW", "TEMPERATURE", "COOLING", "PERFORMANCE"};
  const char* title = titles[currentPage_];
  header(language_ == UiSettings::Language::English ? title : "",
         connected, detailLight);
  if (language_ == UiSettings::Language::Vietnamese) {
    drawVietnameseHeader(canvas_, currentPage_, false,
                         primaryText(detailLight));
  }
  if (currentPage_ == 1) temperatures(viewData, detailLight);
  else if (currentPage_ == 2) cooling(viewData, detailLight);
  else performance(viewData, detailLight);
  footer(address, settings.pageMask, detailLight);
  present(now);
}

void DisplayManager::bentoDashboard(
    const TelemetryData& data, bool connected,
    const UiSettings::State& settings,
    const NetworkTimeManager::ClockSnapshot& clock) {
  const bool light = settings.theme == UiSettings::Theme::Light;
  canvas_.fillScreen(background(light));
  canvas_.setTextSize(fontScale_);
  canvas_.setTextDatum(top_left);
  canvas_.setFont(&fonts::Font2);
  canvas_.setTextColor(primaryText(light));
  canvas_.drawString(settings.dashboardTitle, 8, 4);
  if (headerBold_) canvas_.drawString(settings.dashboardTitle, 9, 4);

  canvas_.setTextDatum(top_right);
  canvas_.setFont(&fonts::Font4);
  canvas_.drawString(clock.time, 232, 0);
  if (valueBold_) canvas_.drawString(clock.time, 231, 0);
  canvas_.setFont(&fonts::Font0);
  canvas_.setTextColor(secondaryText(light));
  if (settings.showDate) {
    canvas_.drawString(clock.date, 232, 25);
    if (labelBold_) canvas_.drawString(clock.date, 231, 25);
  }
  canvas_.fillCircle(8, 29, 3, connected ? 0x07E0 : 0xF800);

  const bool english = language_ == UiSettings::Language::English;
  const uint8_t radius = settings.cardRadius;
  bentoCard(7, 39, english ? "CPU TEMP" : "NHIET CPU",
            number(data.cpuTemperature) + " C",
            settings.cardColors[0], radius);
  bentoCard(123, 39, english ? "GPU TEMP" : "NHIET GPU",
            number(data.gpuTemperature) + " C",
            settings.cardColors[1], radius);
  bentoCard(7, 95, english ? "MEMORY" : "BO NHO",
            number(data.memoryLoad) + "%",
            settings.cardColors[2], radius);
  bentoCard(123, 95,
            isnan(data.fps) ? (english ? "CPU LOAD" : "TAI CPU") : "FPS",
            isnan(data.fps) ? number(data.cpuLoad) + "%" : number(data.fps),
            settings.cardColors[3], radius);
  bentoCard(7, 151, english ? "CPU FAN" : "QUAT CPU",
            number(data.cpuFanRpm),
            settings.cardColors[4], radius);
  bentoCard(123, 151,
            isnan(data.gpuFanRpm) ? (english ? "DISK" : "O DIA")
                                  : (english ? "GPU FAN" : "QUAT GPU"),
            isnan(data.gpuFanRpm) ? number(data.diskLoad) + "%"
                                  : number(data.gpuFanRpm),
            settings.cardColors[5], radius);

  for (uint8_t page = 0; page < 5; ++page) {
    if ((settings.pageMask & (1U << page)) == 0) continue;
    canvas_.fillCircle(92 + page * 14, 222, page == currentPage_ ? 4 : 2,
                       page == currentPage_ ? 0x2DFF : secondaryText(light));
  }
}

void DisplayManager::bentoCard(int x, int y, const char* label,
                               const String& value, uint16_t color,
                               uint8_t radius) {
  const uint16_t automaticText = cardTextColor(color);
  const uint16_t labelText = customTextColors_ ? secondaryTextColor_
                                                : automaticText;
  const uint16_t valueText = customTextColors_ ? primaryTextColor_
                                                : automaticText;
  canvas_.fillRoundRect(x, y, 110, 50, radius, color);
  canvas_.setTextDatum(top_left);
  canvas_.setTextColor(labelText);
  canvas_.setFont(&fonts::Font0);
  canvas_.drawString(label, x + 8, y + 5);
  if (labelBold_) canvas_.drawString(label, x + 9, y + 5);
  canvas_.setFont(&fonts::Font4);
  canvas_.setTextColor(valueText);
  canvas_.drawString(value, x + 8, y + 19);
  if (valueBold_) canvas_.drawString(value, x + 9, y + 19);
}

void DisplayManager::renderOta(uint32_t now) {
  canvas_.setTextSize(1.0f);
  constexpr uint16_t kBackground = 0x0021;
  constexpr uint16_t kPanel = 0x0041;
  constexpr uint16_t kFrame = 0x10E5;
  constexpr uint16_t kTrack = 0x11C7;
  constexpr uint16_t kAccent = 0x37B8;
  constexpr uint16_t kMuted = 0x9D13;
  constexpr uint16_t kError = 0xF9CB;
  constexpr int kCenterX = 120;
  constexpr int kCenterY = 82;
  constexpr int kOuterRadius = 42;
  constexpr int kInnerRadius = 34;
  constexpr int kMidRadius = (kOuterRadius + kInnerRadius) / 2;
  constexpr int kCapRadius = (kOuterRadius - kInnerRadius) / 2;

  const bool failed = otaStatus_.phase == OtaStatus::Phase::Failed;
  const uint16_t progressColor = failed ? kError : kAccent;
  const uint8_t progress = constrain(otaStatus_.progress, 0, 100);

  canvas_.fillScreen(kBackground);
  canvas_.fillRoundRect(7, 7, 226, 226, 32, kFrame);
  canvas_.fillRoundRect(14, 14, 212, 212, 27, kPanel);

  canvas_.fillArc(kCenterX, kCenterY, kOuterRadius, kInnerRadius, 0.0f,
                  360.0f, kTrack);
  if (progress > 0) {
    const float endAngle = static_cast<float>(progress) * 3.6f;
    canvas_.fillArc(kCenterX, kCenterY, kOuterRadius, kInnerRadius, 0.0f,
                    endAngle, progressColor);

    // Rounded caps reproduce the smooth circular progress ring in the design.
    canvas_.fillCircle(kCenterX, kCenterY - kMidRadius, kCapRadius,
                       progressColor);
    const float radians = endAngle * 0.0174532925f;
    const int endX = kCenterX + static_cast<int>(sinf(radians) * kMidRadius);
    const int endY = kCenterY - static_cast<int>(cosf(radians) * kMidRadius);
    const int pulse = otaStatus_.phase == OtaStatus::Phase::Receiving &&
                              ((now / 240U) & 1U)
                          ? 1
                          : 0;
    canvas_.fillCircle(endX, endY, kCapRadius + pulse, progressColor);
  }

  canvas_.setTextDatum(middle_center);
  canvas_.setTextColor(TFT_WHITE);
  canvas_.setFont(&fonts::Font4);
  canvas_.drawString(String(progress) + "%", kCenterX, kCenterY);

  const char* title = "Updating";
  const char* subtitle = "Do not disconnect power";
  const ViText::Bitmap* viTitle = &ViText::kUpdating;
  const ViText::Bitmap* viSubtitle = &ViText::kDoNotPowerOff;
  if (otaStatus_.phase == OtaStatus::Phase::Verifying) {
    title = "Verifying";
    subtitle = "Checking firmware";
    viTitle = &ViText::kVerifying;
    viSubtitle = &ViText::kVerifyFirmware;
  } else if (otaStatus_.phase == OtaStatus::Phase::Success) {
    title = "Complete";
    subtitle = "Restarting device";
    viTitle = &ViText::kComplete;
    viSubtitle = &ViText::kRestarting;
  } else if (failed) {
    title = "Update failed";
    subtitle = "Retry from website";
    viTitle = &ViText::kUpdateFailed;
    viSubtitle = &ViText::kRetryWebsite;
  }

  if (language_ == UiSettings::Language::Vietnamese) {
    drawBitmapCentered(canvas_, *viTitle, 142,
                       failed ? kError : TFT_WHITE);
    drawBitmapCentered(canvas_, *viSubtitle, 174, kMuted);
  } else {
    canvas_.setFont(&fonts::Font2);
    canvas_.setTextColor(failed ? kError : TFT_WHITE);
    canvas_.drawString(title, 120, 151);
    canvas_.drawString(title, 121, 151);
    canvas_.setFont(&fonts::Font0);
    canvas_.setTextColor(kMuted);
    canvas_.drawString(subtitle, 120, 176);
  }

  canvas_.fillRoundRect(55, 202, 130, 5, 3, kTrack);
  const int barWidth = static_cast<int>(progress) * 130 / 100;
  if (barWidth > 0) {
    canvas_.fillRoundRect(55, 202, barWidth, 5, 3, progressColor);
  }
}

void DisplayManager::header(const char* title, bool connected, bool light) {
  canvas_.setTextSize(fontScale_);
  canvas_.fillRoundRect(8, 7, 4, 25, 2, kAmdRed);
  canvas_.setTextDatum(top_left);
  canvas_.setFont(&fonts::Font2);
  canvas_.setTextColor(primaryText(light));
  canvas_.drawString(title, 18, 9);
  if (headerBold_) canvas_.drawString(title, 19, 9);
  canvas_.setTextDatum(top_right);
  canvas_.setFont(&fonts::Font0);
  canvas_.setTextColor(secondaryText(light));
  canvas_.drawString("MI50", 215, 10);
  canvas_.fillCircle(224, 27, 3, connected ? 0x07E0 : kAmdRed);
  canvas_.drawFastHLine(8, 36, 224, kPanelBorder);
}

void DisplayManager::footer(const String& address, uint8_t mask, bool light) {
  canvas_.drawFastHLine(8, 211, 224, kPanelBorder);
  canvas_.setFont(&fonts::Font0);
  canvas_.setTextDatum(top_left);
  canvas_.setTextColor(secondaryText(light));
  canvas_.drawString(address.substring(0, 20), 8, 220);
  if (labelBold_) canvas_.drawString(address.substring(0, 20), 9, 220);
  for (uint8_t page = 0; page < 5; ++page) {
    if ((mask & (1U << page)) == 0) continue;
    canvas_.fillCircle(181 + page * 11, 225, page == currentPage_ ? 4 : 2,
                       page == currentPage_ ? kAmdRed : secondaryText(light));
  }
}

void DisplayManager::present(uint32_t now) {
  if (!transitionActive_) {
    canvas_.pushSprite(0, 0);
    return;
  }
  const uint32_t elapsed = now - transitionStartedMs_;
  if (elapsed >= transitionDurationMs_) {
    transitionActive_ = false;
    canvas_.pushSprite(0, 0);
    return;
  }
  const float phase = static_cast<float>(elapsed) / transitionDurationMs_;
  // Smoothstep avoids the visible jump caused by a linear start/stop while
  // remaining cheap enough for a 240 x 240 ESP32-S3 render loop.
  const float eased = phase * phase * (3.0f - 2.0f * phase);
  const int offset = static_cast<int>(240.0f * (1.0f - eased));
  if (transitionStyle_ == UiSettings::TransitionStyle::Rise) {
    canvas_.pushSprite(0, offset);
  } else {
    canvas_.pushSprite(offset, 0);
  }
}

const TelemetryData& DisplayManager::smoothTelemetry(
    const TelemetryData& source, bool enabled, uint32_t now) {
  if (!enabled || lastSmoothingMs_ == 0 || !displayData_.hasData()) {
    displayData_ = source;
    lastSmoothingMs_ = now;
    return displayData_;
  }

  const uint32_t deltaMs = min<uint32_t>(now - lastSmoothingMs_, 250U);
  lastSmoothingMs_ = now;
  const float alpha = 1.0f - expf(-static_cast<float>(deltaMs) / 260.0f);
  auto approach = [alpha](float current, float target) {
    if (isnan(target)) return target;
    if (isnan(current)) return target;
    return current + (target - current) * alpha;
  };

  displayData_.cpuTemperature = approach(displayData_.cpuTemperature, source.cpuTemperature);
  displayData_.cpuLoad = approach(displayData_.cpuLoad, source.cpuLoad);
  displayData_.cpuClockMHz = approach(displayData_.cpuClockMHz, source.cpuClockMHz);
  displayData_.cpuPowerWatts = approach(displayData_.cpuPowerWatts, source.cpuPowerWatts);
  displayData_.cpuFanRpm = approach(displayData_.cpuFanRpm, source.cpuFanRpm);
  displayData_.gpuTemperature = approach(displayData_.gpuTemperature, source.gpuTemperature);
  displayData_.gpuLoad = approach(displayData_.gpuLoad, source.gpuLoad);
  displayData_.gpuClockMHz = approach(displayData_.gpuClockMHz, source.gpuClockMHz);
  displayData_.gpuPowerWatts = approach(displayData_.gpuPowerWatts, source.gpuPowerWatts);
  displayData_.gpuFanRpm = approach(displayData_.gpuFanRpm, source.gpuFanRpm);
  displayData_.gpuMemoryLoad = approach(displayData_.gpuMemoryLoad, source.gpuMemoryLoad);
  displayData_.memoryLoad = approach(displayData_.memoryLoad, source.memoryLoad);
  displayData_.memoryUsedGb = approach(displayData_.memoryUsedGb, source.memoryUsedGb);
  displayData_.memoryTotalGb = approach(displayData_.memoryTotalGb, source.memoryTotalGb);
  displayData_.fps = approach(displayData_.fps, source.fps);
  displayData_.diskLoad = approach(displayData_.diskLoad, source.diskLoad);
  displayData_.networkDownMbps = approach(displayData_.networkDownMbps, source.networkDownMbps);
  displayData_.networkUpMbps = approach(displayData_.networkUpMbps, source.networkUpMbps);
  displayData_.loadAverage = approach(displayData_.loadAverage, source.loadAverage);
  displayData_.systemUptimeSeconds = source.systemUptimeSeconds;
  displayData_.processCount = source.processCount;
  displayData_.cpuCoreCount = source.cpuCoreCount;
  displayData_.receivedAtMs = source.receivedAtMs;
  displayData_.packetCount = source.packetCount;
  return displayData_;
}

void DisplayManager::logoMissing() {
  canvas_.setTextSize(1.0f);
  canvas_.fillScreen(TFT_BLACK);
  canvas_.setTextDatum(middle_center);
  canvas_.setFont(&fonts::Font4);
  canvas_.setTextColor(TFT_WHITE);
  canvas_.drawString("MI50", 120, 96);
  canvas_.setFont(&fonts::Font2);
  canvas_.setTextColor(kAmdRed);
  canvas_.drawString("PCSCREEN", 120, 126);
  canvas_.setFont(&fonts::Font0);
  canvas_.setTextColor(0x7BEF);
  canvas_.drawString(language_ == UiSettings::Language::English
                         ? "Add a logo from the web console"
                         : "Them logo tren trang web",
                     120, 156);
  canvas_.pushSprite(0, 0);
}

void DisplayManager::temperatures(const TelemetryData& data, bool light) {
  (void)light;
  const uint16_t cpuColor = temperatureColor(data.cpuTemperature);
  const uint16_t gpuColor = temperatureColor(data.gpuTemperature);
  const bool english = language_ == UiSettings::Language::English;
  dataCard(8, 43, 108, 126, english ? "CPU CORE" : "CPU",
           number(data.cpuTemperature) + " C",
           String(english ? "LOAD  " : "TAI   ") + number(data.cpuLoad) + "%",
           data.cpuTemperature, cpuColor);
  dataCard(124, 43, 108, 126, "GPU MI50",
           number(data.gpuTemperature) + " C",
           String(english ? "LOAD  " : "TAI   ") + number(data.gpuLoad) + "%",
           data.gpuTemperature, gpuColor);
  miniStat(8, 177, english ? "CPU POWER" : "CS CPU",
           number(data.cpuPowerWatts) + " W", cpuColor);
  miniStat(124, 177, english ? "GPU POWER" : "CS GPU",
           number(data.gpuPowerWatts) + " W", gpuColor);
}

void DisplayManager::cooling(const TelemetryData& data, bool light) {
  (void)light;
  const bool english = language_ == UiSettings::Language::English;
  ringGauge(62, english ? "CPU FAN" : "QUAT CPU", data.cpuFanRpm,
            data.cpuTemperature, kCyan);
  ringGauge(178, english ? "GPU FAN" : "QUAT GPU", data.gpuFanRpm,
            data.gpuTemperature, kAmdRed);
  miniStat(8, 177, english ? "CPU TEMP" : "NHIET CPU",
           number(data.cpuTemperature) + " C",
           temperatureColor(data.cpuTemperature));
  miniStat(124, 177, english ? "GPU TEMP" : "NHIET GPU",
           number(data.gpuTemperature) + " C",
           temperatureColor(data.gpuTemperature));
}

void DisplayManager::performance(const TelemetryData& data, bool light) {
  (void)light;
  const bool english = language_ == UiSettings::Language::English;
  const String cpuClock = isnan(data.cpuClockMHz)
                              ? String("--")
                              : number(data.cpuClockMHz / 1000.0f, 1) + " GHz";
  const String gpuClock = isnan(data.gpuClockMHz)
                              ? String("--")
                              : number(data.gpuClockMHz / 1000.0f, 1) + " GHz";
  dataCard(8, 43, 108, 76, english ? "CPU LOAD" : "TAI CPU",
           number(data.cpuLoad) + "%",
           cpuClock, percent(data.cpuLoad), kCyan);
  dataCard(124, 43, 108, 76, english ? "GPU LOAD" : "TAI GPU",
           number(data.gpuLoad) + "%",
           gpuClock, percent(data.gpuLoad), kAmdRed);
  dataCard(8, 127, 108, 76, english ? "MEMORY" : "BO NHO",
           number(data.memoryLoad) + "%",
           number(data.memoryUsedGb, 1) + " / " +
               number(data.memoryTotalGb, 1) + " GB",
           percent(data.memoryLoad), kLime);
  const float fpsProgress = isnan(data.fps)
                                ? 0.0f
                                : constrain(data.fps * 100.0f / 240.0f,
                                            0.0f, 100.0f);
  dataCard(124, 127, 108, 76, english ? "FRAME RATE" : "KHUNG HINH",
           number(data.fps), "FPS",
           fpsProgress, kAmdOrange);
}

void DisplayManager::dataCard(int x, int y, int width, int height,
                              const char* label, const String& value,
                              const String& detail, float progress,
                              uint16_t accent) {
  canvas_.fillRoundRect(x, y, width, height, 9, kPanelBorder);
  canvas_.fillRoundRect(x + 1, y + 1, width - 2, height - 2, 8, kPanel);
  canvas_.fillRoundRect(x + 8, y + 7, 22, 3, 2, accent);
  const bool compact = height < 90;
  canvas_.setTextDatum(top_left);
  canvas_.setFont(&fonts::Font0);
  canvas_.setTextColor(secondaryText(false));
  canvas_.drawString(label, x + 8, y + (compact ? 11 : 15));
  if (labelBold_) canvas_.drawString(label, x + 9, y + (compact ? 11 : 15));
  canvas_.setFont(&fonts::Font4);
  canvas_.setTextColor(TFT_WHITE);
  canvas_.drawString(value, x + 8, y + (compact ? 25 : 33));
  if (valueBold_) canvas_.drawString(value, x + 9,
                                      y + (compact ? 25 : 33));
  canvas_.setFont(&fonts::Font0);
  canvas_.setTextColor(secondaryText(false));
  canvas_.drawString(detail, x + 8, y + height - (compact ? 23 : 29));
  const int barWidth = width - 16;
  canvas_.fillRoundRect(x + 8, y + height - 8, barWidth, 3, 2, 0x18E3);
  const int fill = static_cast<int>(percent(progress) * barWidth / 100.0f);
  if (fill > 0) canvas_.fillRoundRect(x + 8, y + height - 8, fill, 3, 2,
                                      accent);
}

void DisplayManager::ringGauge(int centerX, const char* label, float rpm,
                               float temperature, uint16_t accent) {
  constexpr int centerY = 105;
  canvas_.fillArc(centerX, centerY, 48, 40, 0.0f, 360.0f, 0x18E3);
  if (!isnan(rpm)) {
    const float angle = constrain(rpm / 5000.0f, 0.0f, 1.0f) * 360.0f;
    canvas_.fillArc(centerX, centerY, 48, 40, 0.0f, angle, accent);
  }
  canvas_.setTextDatum(middle_center);
  canvas_.setFont(&fonts::Font0);
  canvas_.setTextColor(secondaryText(false));
  canvas_.drawString(label, centerX, 62);
  if (labelBold_) canvas_.drawString(label, centerX + 1, 62);
  canvas_.setFont(&fonts::Font4);
  canvas_.setTextColor(TFT_WHITE);
  canvas_.drawString(number(rpm), centerX, 101);
  if (valueBold_) canvas_.drawString(number(rpm), centerX + 1, 101);
  canvas_.setFont(&fonts::Font0);
  canvas_.setTextColor(accent);
  canvas_.drawString("RPM", centerX, 123);
  canvas_.setTextColor(secondaryText(false));
  canvas_.drawString(number(temperature) + " C", centerX, 144);
}

void DisplayManager::miniStat(int x, int y, const char* label,
                              const String& value, uint16_t accent) {
  canvas_.fillRoundRect(x, y, 108, 26, 7, kPanelBorder);
  canvas_.fillRoundRect(x + 1, y + 1, 106, 24, 6, kPanel);
  canvas_.fillRoundRect(x + 6, y + 6, 3, 14, 2, accent);
  canvas_.setTextDatum(top_left);
  canvas_.setFont(&fonts::Font0);
  canvas_.setTextColor(secondaryText(false));
  canvas_.drawString(label, x + 14, y + 8);
  canvas_.setTextDatum(top_right);
  canvas_.setTextColor(TFT_WHITE);
  canvas_.drawString(value, x + 102, y + 8);
}

void DisplayManager::advancePage(uint8_t mask,
                                 const std::array<uint8_t, 5>& order) {
  uint8_t currentIndex = 0;
  for (uint8_t index = 0; index < order.size(); ++index) {
    if (order[index] == currentPage_) {
      currentIndex = index;
      break;
    }
  }
  for (uint8_t count = 1; count <= order.size(); ++count) {
    const uint8_t page = order[(currentIndex + count) % order.size()];
    if (mask & (1U << page)) {
      currentPage_ = page;
      return;
    }
  }
  currentPage_ = 0;
}

String DisplayManager::number(float value, uint8_t decimals) {
  return isnan(value) ? String("--") : String(value, static_cast<unsigned int>(decimals));
}

uint16_t DisplayManager::cardTextColor(uint16_t color) {
  const uint16_t red = ((color >> 11U) & 0x1FU) * 255U / 31U;
  const uint16_t green = ((color >> 5U) & 0x3FU) * 255U / 63U;
  const uint16_t blue = (color & 0x1FU) * 255U / 31U;
  const uint16_t luminance = (red * 299U + green * 587U + blue * 114U) / 1000U;
  return luminance > 160U ? TFT_BLACK : TFT_WHITE;
}

uint16_t DisplayManager::primaryText(bool light) const {
  return customTextColors_ ? primaryTextColor_ : foreground(light);
}

uint16_t DisplayManager::secondaryText(bool light) const {
  return customTextColors_ ? secondaryTextColor_ : muted(light);
}
