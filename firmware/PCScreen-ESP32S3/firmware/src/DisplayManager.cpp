#include "DisplayManager.h"

#include <cmath>

#include "AppConfig.h"
#include "BoardConfig.h"

namespace {
uint32_t background(bool light) { return light ? TFT_WHITE : TFT_BLACK; }
uint32_t foreground(bool light) { return light ? 0x1082 : TFT_WHITE; }
uint32_t muted(bool light) { return light ? 0x5ACB : 0x9CF3; }
uint32_t track(bool light) { return light ? 0xCE79 : 0x2104; }
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

void DisplayManager::showOtaAnimation(uint32_t now) {
  renderOta(now);
  canvas_.pushSprite(0, 0);
}

void DisplayManager::render(const TelemetryData& data, bool connected,
                            const String& address,
                            const UiSettings::State& settings,
                            const NetworkTimeManager::ClockSnapshot& clock,
                            uint32_t now) {
  setRotation(settings.rotation);
  if (otaStatus_.visible(now)) {
    bootAsset_.endPlayback();
    activePage_ = 0xFF;
    renderOta(now);
    canvas_.pushSprite(0, 0);
    return;
  }
  if (activePage_ == 0xFF) lastPageChangeMs_ = now;
  if ((settings.pageMask & (1U << currentPage_)) == 0) {
    advancePage(settings.pageMask);
    lastPageChangeMs_ = now;
  }
  const uint16_t pageDuration = currentPage_ == 4
                                    ? settings.logoPageDurationSeconds
                                    : settings.pageIntervalSeconds;
  if (settings.autoRotate && now - lastPageChangeMs_ >= pageDuration * 1000UL) {
    advancePage(settings.pageMask);
    lastPageChangeMs_ = now;
  }
  if (activePage_ != currentPage_) {
    bootAsset_.endPlayback();
    activePage_ = currentPage_;
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
  const bool light = settings.theme == UiSettings::Theme::Light;
  if (currentPage_ == 0) {
    bentoDashboard(data, connected, settings, clock);
    canvas_.pushSprite(0, 0);
    return;
  }
  canvas_.fillScreen(background(light));
  const char* titles[] = {"OVERVIEW", "TEMPERATURE", "COOLING", "PERFORMANCE"};
  header(titles[currentPage_], connected, light);
  if (currentPage_ == 1) temperatures(data, light);
  else if (currentPage_ == 2) cooling(data, light);
  else performance(data, light);
  footer(address, settings.pageMask, light);
  canvas_.pushSprite(0, 0);
}

void DisplayManager::bentoDashboard(
    const TelemetryData& data, bool connected,
    const UiSettings::State& settings,
    const NetworkTimeManager::ClockSnapshot& clock) {
  const bool light = settings.theme == UiSettings::Theme::Light;
  canvas_.fillScreen(background(light));
  canvas_.setTextDatum(top_left);
  canvas_.setFont(&fonts::Font2);
  canvas_.setTextColor(foreground(light));
  canvas_.drawString(settings.dashboardTitle, 8, 4);

  canvas_.setTextDatum(top_right);
  canvas_.setFont(&fonts::Font4);
  canvas_.drawString(clock.time, 232, 0);
  canvas_.setFont(&fonts::Font0);
  canvas_.setTextColor(muted(light));
  if (settings.showDate) {
    canvas_.drawString(clock.date, 232, 25);
    canvas_.drawString(clock.date, 231, 25);
  }
  canvas_.fillCircle(8, 29, 3, connected ? 0x07E0 : 0xF800);

  const uint8_t radius = settings.cardRadius;
  bentoCard(7, 39, "CPU TEMP", number(data.cpuTemperature) + " C",
            settings.cardColors[0], radius);
  bentoCard(123, 39, "GPU TEMP", number(data.gpuTemperature) + " C",
            settings.cardColors[1], radius);
  bentoCard(7, 95, "RAM", number(data.memoryLoad) + "%",
            settings.cardColors[2], radius);
  bentoCard(123, 95, "FPS", number(data.fps),
            settings.cardColors[3], radius);
  bentoCard(7, 151, "CPU FAN", number(data.cpuFanRpm),
            settings.cardColors[4], radius);
  bentoCard(123, 151, "GPU FAN", number(data.gpuFanRpm),
            settings.cardColors[5], radius);

  for (uint8_t page = 0; page < 5; ++page) {
    if ((settings.pageMask & (1U << page)) == 0) continue;
    canvas_.fillCircle(92 + page * 14, 222, page == currentPage_ ? 4 : 2,
                       page == currentPage_ ? 0x2DFF : muted(light));
  }
}

void DisplayManager::bentoCard(int x, int y, const char* label,
                               const String& value, uint16_t color,
                               uint8_t radius) {
  const uint16_t text = cardTextColor(color);
  canvas_.fillRoundRect(x, y, 110, 50, radius, color);
  canvas_.setTextDatum(top_left);
  canvas_.setTextColor(text);
  canvas_.setFont(&fonts::Font0);
  canvas_.drawString(label, x + 8, y + 5);
  canvas_.drawString(label, x + 9, y + 5);
  canvas_.setFont(&fonts::Font4);
  canvas_.drawString(value, x + 8, y + 19);
}

void DisplayManager::renderOta(uint32_t now) {
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

  const char* title = "Dang cap nhat";
  const char* subtitle = "Khong ngat nguon thiet bi";
  if (otaStatus_.phase == OtaStatus::Phase::Verifying) {
    title = "Dang kiem tra";
    subtitle = "Xac minh firmware";
  } else if (otaStatus_.phase == OtaStatus::Phase::Success) {
    title = "Hoan tat";
    subtitle = "Dang khoi dong lai";
  } else if (failed) {
    title = "Cap nhat loi";
    subtitle = "Thu lai trong website";
  }

  canvas_.setFont(&fonts::Font2);
  canvas_.setTextColor(failed ? kError : TFT_WHITE);
  canvas_.drawString(title, 120, 151);
  canvas_.drawString(title, 121, 151);
  canvas_.setFont(&fonts::Font0);
  canvas_.setTextColor(kMuted);
  canvas_.drawString(subtitle, 120, 176);

  canvas_.fillRoundRect(55, 202, 130, 5, 3, kTrack);
  const int barWidth = static_cast<int>(progress) * 130 / 100;
  if (barWidth > 0) {
    canvas_.fillRoundRect(55, 202, barWidth, 5, 3, progressColor);
  }
}

void DisplayManager::header(const char* title, bool connected, bool light) {
  canvas_.setTextDatum(top_left);
  canvas_.setFont(&fonts::Font2);
  canvas_.setTextColor(foreground(light));
  canvas_.drawString(title, 12, 9);
  canvas_.fillCircle(220, 17, 4, connected ? 0x07E0 : 0xF800);
  canvas_.drawFastHLine(12, 34, 216, track(light));
}

void DisplayManager::footer(const String& address, uint8_t mask, bool light) {
  canvas_.drawFastHLine(12, 211, 216, track(light));
  canvas_.setFont(&fonts::Font0);
  canvas_.setTextDatum(top_left);
  canvas_.setTextColor(muted(light));
  canvas_.drawString(address, 12, 220);
  for (uint8_t page = 0; page < 5; ++page) {
    if ((mask & (1U << page)) == 0) continue;
    canvas_.fillCircle(178 + page * 11, 225, page == currentPage_ ? 4 : 2,
                       page == currentPage_ ? 0x2DFF : muted(light));
  }
}

void DisplayManager::logoMissing() {
  canvas_.fillScreen(TFT_BLACK);
  canvas_.setTextDatum(middle_center);
  canvas_.setFont(&fonts::Font4);
  canvas_.setTextColor(TFT_WHITE);
  canvas_.drawString("LOGO", 120, 103);
  canvas_.setFont(&fonts::Font2);
  canvas_.setTextColor(0x7BEF);
  canvas_.drawString("Upload in web settings", 120, 140);
  canvas_.pushSprite(0, 0);
}

void DisplayManager::overview(const TelemetryData& data, bool light) {
  metric("CPU", data.cpuTemperature, "C", data.cpuLoad, 44, 0x4DFF, light);
  metric("GPU", data.gpuTemperature, "C", data.gpuLoad, 111, 0xFD25, light);
  canvas_.setTextDatum(top_left); canvas_.setFont(&fonts::Font2);
  canvas_.setTextColor(muted(light)); canvas_.drawString("RAM", 12, 181);
  canvas_.setTextColor(foreground(light)); canvas_.drawString(number(data.memoryLoad) + "%", 54, 181);
  canvas_.setTextColor(muted(light)); canvas_.drawString("FPS", 126, 181);
  canvas_.setTextColor(0xEDE0); canvas_.drawString(number(data.fps), 171, 181);
}

void DisplayManager::temperatures(const TelemetryData& data, bool light) {
  metric("CPU TEMP", data.cpuTemperature, "C", data.cpuTemperature, 48, 0x4DFF, light);
  metric("GPU TEMP", data.gpuTemperature, "C", data.gpuTemperature, 120, 0xFD25, light);
}

void DisplayManager::cooling(const TelemetryData& data, bool light) {
  metric("CPU FAN", data.cpuFanRpm, "RPM", data.cpuTemperature, 48, 0x4DFF, light);
  metric("GPU FAN", data.gpuFanRpm, "RPM", data.gpuTemperature, 120, 0xFD25, light);
}

void DisplayManager::performance(const TelemetryData& data, bool light) {
  canvas_.setTextDatum(top_left); canvas_.setFont(&fonts::Font2);
  const char* labels[] = {"CPU CLOCK", "CPU POWER", "GPU CLOCK", "GPU POWER", "VRAM", "MEMORY"};
  const float values[] = {data.cpuClockMHz, data.cpuPowerWatts, data.gpuClockMHz,
                          data.gpuPowerWatts, data.gpuMemoryLoad, data.memoryLoad};
  const char* units[] = {" MHz", " W", " MHz", " W", "%", "%"};
  for (uint8_t i = 0; i < 6; ++i) {
    const int y = 45 + i * 26;
    canvas_.setTextColor(muted(light)); canvas_.drawString(labels[i], 12, y);
    canvas_.setTextDatum(top_right); canvas_.setTextColor(foreground(light));
    canvas_.drawString(number(values[i]) + units[i], 228, y);
    canvas_.setTextDatum(top_left);
  }
}

void DisplayManager::metric(const char* name, float primary, const char* unit,
                            float load, int y, uint32_t color, bool light) {
  canvas_.setTextDatum(top_left); canvas_.setFont(&fonts::Font2);
  canvas_.setTextColor(color); canvas_.drawString(name, 12, y);
  canvas_.setTextDatum(top_right); canvas_.setTextColor(foreground(light));
  canvas_.setFont(&fonts::Font4); canvas_.drawString(number(primary) + " " + unit, 228, y - 5);
  canvas_.fillRoundRect(12, y + 42, 216, 9, 4, track(light));
  int width = isnan(load) ? 0 : constrain(static_cast<int>(load * 2.16f), 0, 216);
  if (width > 0) canvas_.fillRoundRect(12, y + 42, width, 9, 4, color);
}

void DisplayManager::advancePage(uint8_t mask) {
  for (uint8_t count = 0; count < 5; ++count) {
    currentPage_ = (currentPage_ + 1) % 5;
    if (mask & (1U << currentPage_)) return;
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
