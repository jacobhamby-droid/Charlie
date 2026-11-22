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

// Proper power-on + reset for the Waveshare 1.75" AMOLED (original board)
static void wakeWaveshare175Display() {
  // Start I2C bus (used by power chip, touch, sensors)
  Wire.begin(15, 14);

  // Turn on the power rails that feed the display
  if (pmu.begin(Wire, AXP2101_SLAVE_ADDRESS, 15, 14)) {
    pmu.setALDO1Voltage(3300); pmu.enableALDO1();
    pmu.setALDO2Voltage(3300); pmu.enableALDO2();
    pmu.setALDO3Voltage(3300); pmu.enableALDO3();
    pmu.setALDO4Voltage(3300); pmu.enableALDO4();
    pmu.setBLDO1Voltage(3300); pmu.enableBLDO1();
    pmu.setBLDO2Voltage(3300); pmu.enableBLDO2();
    pmu.setDLDO1Voltage(3300); pmu.enableDLDO1();
    delay(30);
  }

  // Hard-reset the display and touch chip using the real GPIO pins
  // (LCD_RST = GPIO 39, TP_RST = GPIO 40 on the original board)
  pinMode(39, OUTPUT);
  pinMode(40, OUTPUT);

  digitalWrite(39, LOW);   // hold reset
  digitalWrite(40, LOW);
  delay(20);

  digitalWrite(39, HIGH);  // release reset
  digitalWrite(40, HIGH);
  delay(50);
}

void setup() {
  Serial.begin(115200);
  delay(300);

  // This is the critical part that was broken before
  wakeWaveshare175Display();

  lcd.init();
  lcd.setRotation(0);
  lcd.setBrightness(180);   // start a bit lower than max

  personalityInit();
  eyesInit(lcd);
  sensorsInit();
  audioInit();
  socialInit();
  screensInit(lcd);

  eyesSetFromMood();
  lastMs = millis();

  Serial.println("Charlie started - display should be on");
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

  lcd.setBrightness(gSleeping ? 50 : 180);

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
