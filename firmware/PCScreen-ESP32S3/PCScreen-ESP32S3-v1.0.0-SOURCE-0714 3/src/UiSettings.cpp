#include "UiSettings.h"

#include "AppConfig.h"

bool UiSettings::begin() {
  if (!preferences_.begin("ui", false)) return false;
  theme_ = preferences_.getUChar("theme", 0) == 1 ? Theme::Light : Theme::Dark;
  language_ = preferences_.getUChar("language", 0) == 1
                  ? Language::English
                  : Language::Vietnamese;
  rotation_ = preferences_.getUChar("rotation", 0) & 0x03U;
  autoRotate_ = preferences_.getBool("rotate", true);
  pageIntervalSeconds_ = constrain(preferences_.getUShort("page_sec", 5), 2, 30);
  pageMask_ = preferences_.getUChar("pages", 0x0F) & 0x1F;
  if (pageMask_ == 0) pageMask_ = 0x01;
  bootDurationSeconds_ = constrain(preferences_.getUChar("boot_sec", 4), 1, 10);
  logoPageDurationSeconds_ = constrain(preferences_.getUChar("logo_sec", 5), 2, 30);
  dashboardTitle_ = preferences_.getString("dash_title", "PC SCREEN").substring(0, 12);
  if (dashboardTitle_.isEmpty()) dashboardTitle_ = "PC SCREEN";
  showDate_ = preferences_.getBool("show_date", true);
  cardRadius_ = constrain(preferences_.getUChar("card_radius", 10), 2, 18);
  const uint16_t defaults[6] = {0x04DF, 0x05EC, 0xF31F, 0xFB04, 0xBFC4, 0x7A7F};
  for (uint8_t i = 0; i < cardColors_.size(); ++i) {
    const String key = "color" + String(i);
    cardColors_[i] = preferences_.getUShort(key.c_str(), defaults[i]);
  }
  std::array<bool, 5> seen = {false, false, false, false, false};
  bool validOrder = true;
  for (uint8_t i = 0; i < pageOrder_.size(); ++i) {
    const String key = "order" + String(i);
    pageOrder_[i] = preferences_.getUChar(key.c_str(), i);
    if (pageOrder_[i] >= 5 || seen[pageOrder_[i]]) validOrder = false;
    else seen[pageOrder_[i]] = true;
  }
  if (!validOrder) pageOrder_ = {0, 1, 2, 3, 4};
  return true;
}

void UiSettings::loop() {
  if (dirty_ && millis() - changedAtMs_ >= AppConfig::SETTINGS_COMMIT_DELAY_MS) save();
}

UiSettings::State UiSettings::state() const {
  return {theme_, language_, rotation_, autoRotate_, pageIntervalSeconds_, pageMask_,
          bootDurationSeconds_, logoPageDurationSeconds_, dashboardTitle_,
          showDate_, cardRadius_, cardColors_, pageOrder_};
}

void UiSettings::setTheme(Theme theme) { theme_ = theme; markDirty(); }
void UiSettings::setLanguage(Language language) { language_ = language; markDirty(); }
void UiSettings::setRotation(uint8_t rotation) { rotation_ = rotation & 0x03U; markDirty(); }
void UiSettings::setAutoRotate(bool enabled) { autoRotate_ = enabled; markDirty(); }
void UiSettings::setPageInterval(uint16_t seconds) { pageIntervalSeconds_ = constrain(seconds, 2, 30); markDirty(); }
void UiSettings::setPageMask(uint8_t mask) { pageMask_ = mask & 0x1F; if (pageMask_ == 0) pageMask_ = 0x01; markDirty(); }
void UiSettings::setBootDuration(uint8_t seconds) { bootDurationSeconds_ = constrain(seconds, 1, 10); markDirty(); }
void UiSettings::setLogoPageDuration(uint8_t seconds) { logoPageDurationSeconds_ = constrain(seconds, 2, 30); markDirty(); }
void UiSettings::setDashboardTitle(const String& title) {
  dashboardTitle_ = title.substring(0, 12);
  if (dashboardTitle_.isEmpty()) dashboardTitle_ = "PC SCREEN";
  markDirty();
}
void UiSettings::setShowDate(bool enabled) { showDate_ = enabled; markDirty(); }
void UiSettings::setCardRadius(uint8_t radius) { cardRadius_ = constrain(radius, 2, 18); markDirty(); }
void UiSettings::setCardColor(uint8_t index, uint16_t color) {
  if (index >= cardColors_.size()) return;
  cardColors_[index] = color;
  markDirty();
}

bool UiSettings::setPageOrder(const std::array<uint8_t, 5>& order) {
  std::array<bool, 5> seen = {false, false, false, false, false};
  for (const uint8_t page : order) {
    if (page >= 5 || seen[page]) return false;
    seen[page] = true;
  }
  pageOrder_ = order;
  markDirty();
  return true;
}

void UiSettings::markDirty() { dirty_ = true; changedAtMs_ = millis(); }

void UiSettings::save() {
  preferences_.putUChar("theme", static_cast<uint8_t>(theme_));
  preferences_.putUChar("language", static_cast<uint8_t>(language_));
  preferences_.putUChar("rotation", rotation_);
  preferences_.putBool("rotate", autoRotate_);
  preferences_.putUShort("page_sec", pageIntervalSeconds_);
  preferences_.putUChar("pages", pageMask_);
  preferences_.putUChar("boot_sec", bootDurationSeconds_);
  preferences_.putUChar("logo_sec", logoPageDurationSeconds_);
  preferences_.putString("dash_title", dashboardTitle_);
  preferences_.putBool("show_date", showDate_);
  preferences_.putUChar("card_radius", cardRadius_);
  for (uint8_t i = 0; i < cardColors_.size(); ++i) {
    const String key = "color" + String(i);
    preferences_.putUShort(key.c_str(), cardColors_[i]);
  }
  for (uint8_t i = 0; i < pageOrder_.size(); ++i) {
    const String key = "order" + String(i);
    preferences_.putUChar(key.c_str(), pageOrder_[i]);
  }
  dirty_ = false;
}
