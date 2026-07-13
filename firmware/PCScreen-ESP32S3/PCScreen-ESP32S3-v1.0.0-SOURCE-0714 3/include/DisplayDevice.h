#pragma once

#include <FS.h>
#include <LittleFS.h>
#include <LovyanGFX.hpp>

class DisplayDevice final : public lgfx::LGFX_Device {
 public:
  DisplayDevice();

 private:
  lgfx::Bus_SPI bus_;
  lgfx::Panel_ST7789 panel_;
};
