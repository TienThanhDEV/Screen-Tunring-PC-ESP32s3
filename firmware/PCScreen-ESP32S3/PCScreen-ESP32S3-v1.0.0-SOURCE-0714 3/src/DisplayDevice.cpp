#include "DisplayDevice.h"

#include "BoardConfig.h"

DisplayDevice::DisplayDevice() {
  {
    auto cfg = bus_.config();
    cfg.spi_host = SPI2_HOST;
    cfg.spi_mode = 0;
    cfg.freq_write = BoardConfig::TFT_SPI_HZ;
    cfg.freq_read = 16000000;
    cfg.spi_3wire = true;
    cfg.use_lock = true;
    cfg.dma_channel = SPI_DMA_CH_AUTO;
    cfg.pin_sclk = BoardConfig::TFT_SCLK;
    cfg.pin_mosi = BoardConfig::TFT_MOSI;
    cfg.pin_miso = -1;
    cfg.pin_dc = BoardConfig::TFT_DC;
    bus_.config(cfg);
    panel_.setBus(&bus_);
  }
  {
    auto cfg = panel_.config();
    cfg.pin_cs = BoardConfig::TFT_CS;
    cfg.pin_rst = BoardConfig::TFT_RST;
    cfg.pin_busy = -1;
    cfg.panel_width = BoardConfig::TFT_WIDTH;
    cfg.panel_height = BoardConfig::TFT_HEIGHT;
    cfg.memory_width = 240;
    cfg.memory_height = 240;
    cfg.offset_x = 0;
    cfg.offset_y = 0;
    cfg.offset_rotation = 0;
    cfg.readable = false;
    cfg.invert = true;
    cfg.rgb_order = false;
    cfg.dlen_16bit = false;
    cfg.bus_shared = false;
    panel_.config(cfg);
  }
  setPanel(&panel_);
}

