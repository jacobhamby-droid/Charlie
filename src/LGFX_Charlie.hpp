#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <Arduino_GFX_Library.h>

class LGFX_Charlie {
public:
  Arduino_DataBus *bus = nullptr;
  Arduino_CO5300  *gfx = nullptr;

  LGFX_Charlie() {
    bus = new Arduino_ESP32QSPI(
      12 /* CS */, 38 /* SCK */, 4 /* D0 */, 5 /* D1 */, 6 /* D2 */, 7 /* D3 */
    );
    gfx = new Arduino_CO5300(
      bus, 39 /* RST */, 0 /* rotation */, 466 /* width */, 466 /* height */,
      6 /* col_offset1 */, 0 /* row_offset1 */, 0 /* col_offset2 */, 0 /* row_offset2 */
    );
  }

  void init() {
    gfx->begin();
    gfx->fillScreen(BLACK);
  }

  void setRotation(uint8_t r) { gfx->setRotation(r); }

  void setBrightness(uint8_t b) {
    gfx->setBrightness(b);
  }

  void fillScreen(uint16_t c) { gfx->fillScreen(c); }

  uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
    return gfx->color565(r, g, b);
  }

  void drawCircle(int16_t x, int16_t y, int16_t r, uint16_t c) {
    gfx->drawCircle(x, y, r, c);
  }

  void fillCircle(int16_t x, int16_t y, int16_t r, uint16_t c) {
    gfx->fillCircle(x, y, r, c);
  }

  void fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t c) {
    gfx->fillRoundRect(x, y, w, h, r, c);
  }

  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t c) {
    gfx->fillRect(x, y, w, h, c);
  }

  void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t c) {
    gfx->fillTriangle(x0, y0, x1, y1, x2, y2, c);
  }

  void drawArc(int16_t x, int16_t y, int16_t r1, int16_t r2, int16_t start, int16_t end, uint16_t c) {
    gfx->drawArc(x, y, r1, r2, start, end, c);
  }

  void fillArc(int16_t x, int16_t y, int16_t r1, int16_t r2, int16_t start, int16_t end, uint16_t c) {
    gfx->fillArc(x, y, r1, r2, start, end, c);
  }

  bool getTouch(int16_t *x, int16_t *y) {
    // CST9217 I2C read on address 0x5A (SDA=15, SCL=14)
    Wire.beginTransmission(0x5A);
    Wire.write(0xD0);
    Wire.write(0x00);
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom(0x5A, 7) != 7) return false;
    uint8_t buf[7];
    for (int i = 0; i < 7; i++) buf[i] = Wire.read();
    if ((buf[0] & 0x0F) == 0) return false;
    *x = ((uint16_t)(buf[1] & 0x0F) << 8) | buf[2];
    *y = ((uint16_t)(buf[3] & 0x0F) << 8) | buf[4];
    return (*x < 466 && *y < 466);
  }
};

#ifndef TFT_BLACK
#define TFT_BLACK 0x0000
#endif
