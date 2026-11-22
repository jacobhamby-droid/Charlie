#include <Arduino.h>
#include <Wire.h>
#define XPOWERS_CHIP_AXP2101
#include "XPowersLib.h"

#include "LGFX_Charlie.hpp"
#include "Config.h"
#include "Eyes.hpp"
#include "Personality.hpp"
#include "Sensors.hpp"
#include "Screens.hpp"
#include "AudioBeat.hpp"
#include "Social.hpp"
#include "SyncServer.hpp"

LGFX_Charlie lcd;
XPowersPMU pmu;
static uint32_t lastMs = 0;

void setup() {
  Serial.begin(115200);
  delay(200);

  // Power up the AMOLED display & touch rails via onboard AXP2101 PMU
  Wire.begin(15, 14);
  if (pmu.begin(Wire, AXP2101_SLAVE_ADDRESS, 15, 14)) {
    pmu.setALDO1Voltage(3300);
    pmu.enableALDO1();
    pmu.setALDO2Voltage(3300);
    pmu.enableALDO2();
    pmu.setALDO3Voltage(3300);
    pmu.enableALDO3();
    pmu.setBLDO1Voltage(3300);
    pmu.enableBLDO1();
    delay(50);
  }

  lcd.init();
  lcd.setBrightness(210);
  lcd.setRotation(0);

  personalityInit();
  eyesInit(lcd);
  sensorsInit();
  audioInit();
  socialInit();
  screensInit(lcd);

  eyesSetFromMood();
  lastMs = millis();
}

void loop() {
  uint32_t now = millis();
  float dt = (now - lastMs) / 1000.0f;
  lastMs = now;
  if (dt > 0.05f) dt = 0.05f;

  personalityUpdate(dt);
  sensorsUpdate(dt);
  audioUpdate(dt);
  socialUpdate();
  screensUpdate(dt);
  eyesUpdate(dt, getGravityAngle());

  // Auto-dim AMOLED when sleeping to save battery & prevent burn-in
  lcd.setBrightness(gSleeping ? 45 : 210);

  // Touch press & release handling
  static bool wasPressed = false;
  static uint32_t pressStart = 0;
  static int16_t lastX = 0, lastY = 0;

  int16_t x, y;
  if (lcd.getTouch(&x, &y)) {
    if (!wasPressed) {
      wasPressed = true;
      pressStart = now;
    }
    lastX = x;
    lastY = y;
    processTouch(x, y, true, pressStart);
  } else if (wasPressed) {
    wasPressed = false;
    processTouch(lastX, lastY, false, pressStart);
    pressStart = 0;
  }

  screensDraw(lcd);
  syncServerLoop();

  delay(12);
}
