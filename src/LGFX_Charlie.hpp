#pragma once
#include <LovyanGFX.hpp>

class LGFX_Charlie : public lgfx::LGFX_Device {
  lgfx::Panel_CO5300 _panel_instance;
  lgfx::Bus_SPI      _bus_instance;
  lgfx::Touch_CST816S _touch_instance;

public:
  LGFX_Charlie(void) {
    {
      auto cfg = _bus_instance.config();
      cfg.freq_write = 40000000;
      cfg.spi_host   = SPI2_HOST;
      cfg.pin_sclk   = 38;
      cfg.pin_mosi   = -1;
      cfg.pin_miso   = -1;
      cfg.pin_dc     = -1;
      cfg.spi_mode   = 0;
      cfg.spi_3wire  = false;
      cfg.use_lock   = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
      cfg.pin_d0 = 4;
      cfg.pin_d1 = 5;
      cfg.pin_d2 = 6;
      cfg.pin_d3 = 7;
      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }
    {
      auto cfg = _panel_instance.config();
      cfg.pin_cs           = 12;
      cfg.pin_rst          = 39;
      cfg.pin_busy         = -1;
      cfg.memory_width     = 466;
      cfg.memory_height    = 466;
      cfg.panel_width      = 466;
      cfg.panel_height     = 466;
      cfg.offset_x         = 0;
      cfg.offset_y         = 0;
      cfg.offset_rotation  = 0;
      cfg.readable         = false;
      cfg.invert           = false;
      cfg.rgb_order        = false;
      cfg.dlen_16bit       = false;
      cfg.bus_shared       = false;
      _panel_instance.config(cfg);
    }
    {
      auto cfg = _touch_instance.config();
      cfg.x_min           = 0;
      cfg.x_max           = 465;
      cfg.y_min           = 0;
      cfg.y_max           = 465;
      cfg.pin_int         = 11;
      cfg.pin_rst         = 40;
      cfg.bus_shared      = true;
      cfg.offset_rotation = 0;
      cfg.i2c_port        = 0;
      cfg.i2c_addr        = 0x5A;
      cfg.pin_sda         = 15;
      cfg.pin_scl         = 14;
      cfg.freq            = 400000;
      _touch_instance.config(cfg);
      _panel_instance.setTouch(&_touch_instance);
    }
    setPanel(&_panel_instance);
  }
};
