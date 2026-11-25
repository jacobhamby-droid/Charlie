#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <Arduino_GFX_Library.h>

// Simple wrapper that uses a Canvas (required for reliable drawing on CO5300)
class LGFX_Charlie {
public:
  Arduino_DataBus *bus = nullptr;
  Arduino_CO5300  *panel = nullptr;
  Arduino_Canvas  *gfx = nullptr;   // we draw on this

  LGFX_Charlie() {
    bus = new Arduino_ESP32QSPI(
      12 /* CS */, 38 /* SCK */, 4 /* D0 */, 5 /* D1 */, 6 /* D2 */, 7 /* D3 */
    );

    // Official Waveshare settings: 466x466, col_offset = 6
    panel = new Arduino_CO5300(
      bus, 39 /* RST */, 0 /* rotation */,
      466 /* width */, 466 /* height */,
      6 /* col_offset1 */, 0, 0, 0
    );

    // Canvas is what we actually draw on
    gfx = new Arduino_Canvas(466, 466, panel);
  }

  void init() {
    // Start the real panel first
    if (!panel->begin()) {
      Serial.println("panel->begin() failed");
    }
    panel->setBrightness(180);

    // Then start the canvas
    gfx->begin();
    gfx->fillScreen(RGB565_BLACK);
    gfx->flush();
  }

  void setBrightness(uint8_t b) {
    panel->setBrightness(b);
  }

  void fillScreen(uint16_t c) {
    gfx->fillScreen(c);
  }

  void fillCircle(int16_t x, int16_t y, int16_t r, uint16_t c) {
    gfx->fillCircle(x, y, r, c);
  }

  void flush() {
    gfx->flush();   // push the canvas to the real screen
  }
};
