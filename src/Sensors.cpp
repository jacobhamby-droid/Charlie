#include "Sensors.hpp"
#include "Eyes.hpp"
#include "Personality.hpp"
#include "Screens.hpp"
#include <Wire.h>
#include <math.h>

// Waveshare 1.75" AMOLED QMI8658 6-Axis IMU (I2C address 0x6B on SDA=15, SCL=14)
#define QMI8658_ADDR 0x6B

static float gravityAngle = 0.0f;
static bool pickedUp = false;
static bool shaken = false;
static uint32_t dizzyUntil = 0;

// Cadence-filtered step counter variables
static uint32_t lastStepMs = 0;
static int rhythmicStreak = 0;
static float filteredMag = 1.0f;

// Touch tracking variables
static int16_t startX = 0, startY = 0;
static bool touchActive = false;
static float totalDrag = 0.0f;

// External tap helper from Screens.cpp
extern void onScreenTap(int16_t x, int16_t y);

static void qmiWrite(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(QMI8658_ADDR);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

void sensorsInit() {
  Wire.begin(15, 14);
  // Enable QMI8658 accelerometer & gyroscope
  qmiWrite(0x02, 0x60);
  qmiWrite(0x03, 0x25);
  qmiWrite(0x08, 0x03);
}

void sensorsUpdate(float dt) {
  Wire.beginTransmission(QMI8658_ADDR);
  Wire.write(0x35);
  if (Wire.endTransmission(false) != 0) return;

  if (Wire.requestFrom(QMI8658_ADDR, 6) == 6) {
    int16_t rawX = (int16_t)(Wire.read() | (Wire.read() << 8));
    int16_t rawY = (int16_t)(Wire.read() | (Wire.read() << 8));
    int16_t rawZ = (int16_t)(Wire.read() | (Wire.read() << 8));

    float ax = rawX / 4096.0f;
    float ay = rawY / 4096.0f;
    float az = rawZ / 4096.0f;

    // 1. Gravity leveling angle (keeps eyes upright when tilted)
    float targetAngle = atan2f(ax, ay) * RAD_TO_DEG;
    gravityAngle += (targetAngle - gravityAngle) * min(1.0f, dt * 8.0f);

    float mag = sqrtf(ax * ax + ay * ay + az * az);
    float delta = fabsf(mag - filteredMag);
    filteredMag += (mag - filteredMag) * 0.15f;

    // 2. Shaking detection -> Dizzy eyes
    if (delta > 1.35f) {
      shaken = true;
      dizzyUntil = millis() + 2500;
      gMood.annoyance = min(1.0f, gMood.annoyance + 0.15f * gPersonality[SENSITIVITY]);
    } else if (millis() > dizzyUntil) {
      shaken = false;
    }

    if (shaken) {
      float swirl = millis() * 0.018f;
      eyesLook(cosf(swirl) * 0.85f, sinf(swirl) * 0.85f);
    }

    // 3. Pickup / Desk tap detection
    pickedUp = (fabsf(az) < 0.45f && delta > 0.25f);
    if (pickedUp) {
      eyesLook(0.0f, -0.35f);
    } else if (delta > 0.45f && delta <= 1.35f && !shaken) {
      eyesLook((random(-100, 100)) / 100.0f, -0.25f);
    }

    // 4. Cadence-filtered walking step counter ("Pixels")
    uint32_t now = millis();
    if (mag > 1.28f && mag < 2.2f && !shaken) {
      uint32_t gap = now - lastStepMs;
      if (gap >= 350 && gap <= 800) {
        rhythmicStreak++;
        lastStepMs = now;
        if (rhythmicStreak == 4) {
          gSteps += 4;
        } else if (rhythmicStreak > 4) {
          gSteps++;
          if (gSteps % 25 == 0) personalitySaveToNVS();
        }
      } else if (gap > 800) {
        rhythmicStreak = 1;
        lastStepMs = now;
      }
    }
  }
}

float getGravityAngle() { return gravityAngle; }
bool isPickedUp() { return pickedUp; }
bool isShaken() { return shaken; }

void processTouch(int16_t x, int16_t y, bool pressed, uint32_t pressTime) {
  if (pressed) {
    if (!touchActive) {
      touchActive = true;
      startX = x;
      startY = y;
      totalDrag = 0.0f;
    } else {
      static int16_t prevX = x, prevY = y;
      totalDrag += hypotf(x - prevX, y - prevY);
      prevX = x;
      prevY = y;

      if (gScreen == SCREEN_FACE) {
        float nx = (x - CENTER_X) / (float)(SCREEN_W / 2);
        float ny = (y - CENTER_Y) / (float)(SCREEN_H / 2);
        eyesLook(nx, ny);

        // Rubbing / Petting -> Happiness boost!
        if (totalDrag > 60.0f) {
          gMood.happiness = min(1.0f, gMood.happiness + 0.006f * (0.5f + gPersonality[AFFECTION]));
          gMood.annoyance = max(0.0f, gMood.annoyance - 0.01f);
        }
      }
    }
  } else if (touchActive) {
    touchActive = false;
    int dx = x - startX;
    int dy = y - startY;
    uint32_t heldMs = millis() - pressTime;

    // 1. Check for screen swipes first (Right = Timer, Up = Connect)
    if (hypot(dx, dy) > 55) {
      onSwipe(dx, dy);
      return;
    }

    // 2. Utility screen taps (Timer or Connect button)
    if (gScreen != SCREEN_FACE) {
      onScreenTap(x, y);
      return;
    }

    // 3. Face taps & long presses
    if (heldMs > 650 && totalDrag < 30.0f) {
      gMood.happiness = min(1.0f, gMood.happiness + 0.12f);
      eyesBlink();
    } else if (heldMs <= 300 && totalDrag < 25.0f) {
      gMood.annoyance = min(1.0f, gMood.annoyance + 0.15f * gPersonality[SENSITIVITY]);
      eyesBlink();
    }
  }
}
