#include <Arduino.h>
#include <Wire.h>
#define XPOWERS_CHIP_AXP2101
#include "XPowersLib.h"
#include "LGFX_Charlie.hpp"

LGFX_Charlie lcd;
XPowersPMU pmu;

void wakeDisplay() {
  Wire.begin(15, 14);

  // Power the display rails
  if (pmu.begin(Wire, AXP2101_SLAVE_ADDRESS, 15, 14)) {
    pmu.setALDO1Voltage(3300); pmu.enableALDO1();
    pmu.setALDO2Voltage(3300); pmu.enableALDO2();
    pmu.setALDO3Voltage(3300); pmu.enableALDO3();
    pmu.setALDO4Voltage(3300); pmu.enableALDO4();
    pmu.setBLDO1Voltage(3300); pmu.enableBLDO1();
    pmu.setBLDO2Voltage(3300); pmu.enableBLDO2();
    pmu.setDLDO1Voltage(3300); pmu.enableDLDO1();
    delay(50);
  }

  // Hardware reset of the panel
  pinMode(39, OUTPUT);
  pinMode(40, OUTPUT);
  digitalWrite(39, LOW);
  digitalWrite(40, LOW);
  delay(20);
  digitalWrite(39, HIGH);
  digitalWrite(40, HIGH);
  delay(50);
}

void setup() {
  Serial.begin(115200);
  delay(500);

  wakeDisplay();
  lcd.init();

  // Black background
  lcd.fillScreen(RGB565_BLACK);

  // Two simple white eyes with black pupils
  // Left eye
  lcd.fillCircle(155, 233, 55, RGB565_WHITE);
  lcd.fillCircle(155, 233, 22, RGB565_BLACK);

  // Right eye
  lcd.fillCircle(311, 233, 55, RGB565_WHITE);
  lcd.fillCircle(311, 233, 22, RGB565_BLACK);

  // IMPORTANT: push the drawing to the real screen
  lcd.flush();

  Serial.println("Two eyes should be visible now");
}

void loop() {
  delay(1000);
}
