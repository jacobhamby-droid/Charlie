#include <Arduino.h>
#include <Wire.h>
#define XPOWERS_CHIP_AXP2101
#include "XPowersLib.h"
#include <Arduino_GFX_Library.h>

// === Pins from Waveshare ESP32-S3-Touch-AMOLED-1.75 ===
#define LCD_CS     12
#define LCD_SCLK   38
#define LCD_SDIO0  4
#define LCD_SDIO1  5
#define LCD_SDIO2  6
#define LCD_SDIO3  7
#define LCD_RESET  39
#define LCD_WIDTH  466
#define LCD_HEIGHT 466

XPowersPMU pmu;

Arduino_DataBus *bus = new Arduino_ESP32QSPI(
  LCD_CS, LCD_SCLK, LCD_SDIO0, LCD_SDIO1, LCD_SDIO2, LCD_SDIO3
);

Arduino_CO5300 *gfx = new Arduino_CO5300(
  bus, LCD_RESET, 0 /* rotation */,
  LCD_WIDTH, LCD_HEIGHT,
  6, 0, 0, 0   // official col_offset
);

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("Starting...");

  // Power up the display rails (needed on this board)
  Wire.begin(15, 14);
  if (pmu.begin(Wire, AXP2101_SLAVE_ADDRESS, 15, 14)) {
    Serial.println("PMU OK");
    pmu.setALDO1Voltage(3300); pmu.enableALDO1();
    pmu.setALDO2Voltage(3300); pmu.enableALDO2();
    pmu.setALDO3Voltage(3300); pmu.enableALDO3();
    pmu.setALDO4Voltage(3300); pmu.enableALDO4();
    pmu.setBLDO1Voltage(3300); pmu.enableBLDO1();
    pmu.setBLDO2Voltage(3300); pmu.enableBLDO2();
    pmu.setDLDO1Voltage(3300); pmu.enableDLDO1();
    delay(100);
  } else {
    Serial.println("PMU failed");
  }

  // Extra hardware reset (some boards need this)
  pinMode(39, OUTPUT);
  pinMode(40, OUTPUT);
  digitalWrite(39, LOW);
  digitalWrite(40, LOW);
  delay(50);
  digitalWrite(39, HIGH);
  digitalWrite(40, HIGH);
  delay(100);

  // Init display - exactly like Waveshare example
  if (!gfx->begin()) {
    Serial.println("gfx->begin() FAILED");
  } else {
    Serial.println("gfx->begin() OK");
  }

  gfx->fillScreen(BLACK);
  gfx->setBrightness(200);

  // Draw two big white eyes
  gfx->fillCircle(160, 233, 70, WHITE);
  gfx->fillCircle(160, 233, 28, BLACK);

  gfx->fillCircle(306, 233, 70, WHITE);
  gfx->fillCircle(306, 233, 28, BLACK);

  Serial.println("Eyes drawn - done");
}

void loop() {
  delay(2000);
}
