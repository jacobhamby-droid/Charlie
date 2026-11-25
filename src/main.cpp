#include <Arduino.h>
#include <Wire.h>
#define XPOWERS_CHIP_AXP2101
#include "XPowersLib.h"
#include "LGFX_Charlie.hpp"

LGFX_Charlie lcd;
XPowersPMU pmu;

// Turn on the display power and reset it
void wakeDisplay() {
  Wire.begin(15, 14);

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

  // Reset pins for the screen
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
  delay(300);

  wakeDisplay();

  lcd.init();
  lcd.setBrightness(180);

  // Black background
  lcd.fillScreen(0x0000);  // black

  // Two simple white eyes
  // Left eye
  lcd.fillCircle(160, 233, 60, 0xFFFF);  // white circle
  lcd.fillCircle(160, 233, 25, 0x0000);  // black pupil

  // Right eye
  lcd.fillCircle(306, 233, 60, 0xFFFF);  // white circle
  lcd.fillCircle(306, 233, 25, 0x0000);  // black pupil

  Serial.println("Two eyes drawn!");
}

void loop() {
  // Do nothing - just keep the eyes on screen
  delay(1000);
}
