/*
 * PC Screen Easy Start
 * Arduino IDE sketch for validating ST7789 + GPIO8 backlight PWM.
 * Install LovyanGFX, select "ESP32S3 Dev Module", enable USB CDC on boot.
 */
#include <Arduino.h>
#include <LovyanGFX.hpp>
#include <esp_arduino_version.h>

constexpr int PIN_SCLK = 13, PIN_MOSI = 12, PIN_RST = 11;
constexpr int PIN_DC = 10, PIN_CS = 9, PIN_BL = 8;
constexpr int LEGACY_LEDC_CHANNEL = 0;

class Screen final : public lgfx::LGFX_Device {
 public:
  Screen() {
    auto busConfig = bus_.config();
    busConfig.spi_host = SPI2_HOST;
    busConfig.spi_mode = 0;
    busConfig.freq_write = 40000000;
    busConfig.pin_sclk = PIN_SCLK;
    busConfig.pin_mosi = PIN_MOSI;
    busConfig.pin_miso = -1;
    busConfig.pin_dc = PIN_DC;
    bus_.config(busConfig);
    panel_.setBus(&bus_);

    auto panelConfig = panel_.config();
    panelConfig.pin_cs = PIN_CS;
    panelConfig.pin_rst = PIN_RST;
    panelConfig.panel_width = 240;
    panelConfig.panel_height = 240;
    panelConfig.memory_width = 240;
    panelConfig.memory_height = 240;
    panelConfig.invert = true;
    panel_.config(panelConfig);
    setPanel(&panel_);
  }
 private:
  lgfx::Bus_SPI bus_;
  lgfx::Panel_ST7789 panel_;
};

Screen screen;

void setBacklight(uint8_t percent) {
  const uint32_t duty = 255U * constrain(percent, 0, 100) / 100U;
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(PIN_BL, duty);
#else
  ledcWrite(LEGACY_LEDC_CHANNEL, duty);
#endif
}

void setup() {
  Serial.begin(115200);
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(PIN_BL, 20000, 8);
#else
  ledcSetup(LEGACY_LEDC_CHANNEL, 20000, 8);
  ledcAttachPin(PIN_BL, LEGACY_LEDC_CHANNEL);
#endif
  setBacklight(70);

  screen.init();
  screen.setRotation(0);
  screen.fillScreen(TFT_BLACK);
  screen.fillRoundRect(12, 12, 216, 216, 18, 0x0861);
  screen.setTextDatum(middle_center);
  screen.setTextColor(0x4DFF);
  screen.setFont(&fonts::Font4);
  screen.drawString("PC SCREEN", 120, 66);
  screen.setTextColor(TFT_WHITE);
  screen.setFont(&fonts::Font2);
  screen.drawString("ST7789 240 x 240", 120, 108);
  screen.drawString("BL: GPIO8 PWM 70%", 120, 137);
  screen.setTextColor(TFT_GREEN);
  screen.drawString("HARDWARE READY", 120, 178);
  Serial.println("Easy Start ready. Send 0..100 followed by Enter.");
}

void loop() {
  if (Serial.available()) {
    const int value = Serial.parseInt();
    if (value >= 0 && value <= 100) {
      setBacklight(value);
      Serial.printf("Brightness: %d%%\n", value);
    }
    while (Serial.available()) Serial.read();
  }
}

