#pragma once

#include <Arduino.h>

namespace BoardConfig {
constexpr int TFT_SCLK = 13;
constexpr int TFT_MOSI = 12;
constexpr int TFT_RST = 11;
constexpr int TFT_DC = 10;
constexpr int TFT_CS = 9;
// Direct PWM signal to the BL input of 1.54TFT-SPI-ST7789 Ver:1.1.
// This module revision has an onboard transistor/resistor backlight stage.
constexpr int TFT_BL = 8;
// Most ESP32-S3 Super Mini boards use one WS2812/SK6812 on GPIO48.
// Change this value if your exact clone uses another pin.
constexpr int RGB_LED_PIN = 48;
constexpr uint8_t RGB_LED_COUNT = 1;

constexpr uint16_t TFT_WIDTH = 240;
constexpr uint16_t TFT_HEIGHT = 240;
constexpr uint32_t TFT_SPI_HZ = 40000000;

constexpr uint32_t BL_PWM_HZ = 20000;
constexpr uint8_t BL_PWM_BITS = 8;
constexpr uint8_t BL_PWM_CHANNEL = 0;  // Arduino-ESP32 2.x only.
// GPIO8 HIGH enables the module's onboard backlight transistor.
constexpr bool BL_ACTIVE_HIGH = true;
}  // namespace BoardConfig
