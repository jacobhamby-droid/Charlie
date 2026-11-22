#include "Screens.hpp"
#include "Eyes.hpp"
#include "Personality.hpp"

ScreenID gScreen = SCREEN_FACE;
bool gWifiOn = false;

static uint32_t countdownEndMs = 0;
static uint32_t timerPausedRemaining = 0;
static bool timerRunning = false;
static int timerMinutes = 5;

void screensInit(LGFX_Charlie& lcd) {}

void onSwipe(int dx, int dy) {
  if (abs(dx) > abs(dy)) {
    if (dx > 40 && gScreen == SCREEN_FACE) gScreen = SCREEN_TIMER;
    if (dx < -40 && gScreen == SCREEN_TIMER) gScreen = SCREEN_FACE;
  } else {
    if (dy < -40 && gScreen == SCREEN_FACE) gScreen = SCREEN_CONNECT;
    if (dy > 40 && gScreen == SCREEN_CONNECT) {
      gScreen = SCREEN_FACE;
      gWifiOn = false;
    }
  }
}

void onScreenTap(int16_t x, int16_t y) {
  if (gScreen == SCREEN_TIMER) {
    int dy = y - CENTER_Y;
    if (!timerRunning && abs(dy) > 75) {
      timerMinutes = constrain(timerMinutes + (dy < 0 ? 5 : -5), 1, 60);
      timerPausedRemaining = 0;
    } else {
      if (!timerRunning) {
        uint32_t dur = (timerPausedRemaining > 0) ? timerPausedRemaining : (timerMinutes * 60000UL);
        countdownEndMs = millis() + dur;
        timerRunning = true;
      } else {
        timerPausedRemaining = (countdownEndMs > millis()) ? (countdownEndMs - millis()) : 0;
        timerRunning = false;
      }
    }
  } else if (gScreen == SCREEN_CONNECT) {
    gWifiOn = !gWifiOn;
  }
}

void screensUpdate(float dt) {
  if (gScreen == SCREEN_TIMER && timerRunning) {
    if ((int32_t)(millis() - countdownEndMs) >= 0) {
      timerRunning = false;
      timerPausedRemaining = 0;
      gScreen = SCREEN_FACE;
      gMood.happiness = min(1.0f, gMood.happiness + 0.25f);
      eyesBlink();
    }
  }
}

void screensDraw(LGFX_Charlie& lcd) {
  if (gScreen == SCREEN_FACE) {
    eyesDraw(lcd);
    return;
  }

  lcd.fillScreen(TFT_BLACK);
  uint16_t accent = lcd.color565((gEyeColor >> 16) & 0xFF, (gEyeColor >> 8) & 0xFF, gEyeColor & 0xFF);

  if (gScreen == SCREEN_TIMER) {
    float totalMs = timerMinutes * 60000.0f;
    float remMs = timerRunning ? max(0.0f, (float)(countdownEndMs - millis()))
                               : (timerPausedRemaining > 0 ? (float)timerPausedRemaining : totalMs);
    float progress = constrain(remMs / totalMs, 0.0f, 1.0f);

    lcd.drawArc(CENTER_X, CENTER_Y, 128, 114, 0, 360, lcd.color565(30, 30, 35));
    if (progress > 0.005f) {
      lcd.fillArc(CENTER_X, CENTER_Y, 128, 114, 270, 270 + (int)(360.0f * progress), accent);
    }

    if (!timerRunning) {
      lcd.fillTriangle(CENTER_X - 18, CENTER_Y - 26, CENTER_X - 18, CENTER_Y + 26, CENTER_X + 28, CENTER_Y, accent);
      lcd.fillTriangle(CENTER_X, CENTER_Y - 92, CENTER_X - 14, CENTER_Y - 76, CENTER_X + 14, CENTER_Y - 76, accent);
      lcd.fillTriangle(CENTER_X, CENTER_Y + 92, CENTER_X - 14, CENTER_Y + 76, CENTER_X + 14, CENTER_Y + 76, accent);
    } else {
      lcd.fillRoundRect(CENTER_X - 22, CENTER_Y - 24, 14, 48, 4, accent);
      lcd.fillRoundRect(CENTER_X + 8,  CENTER_Y - 24, 14, 48, 4, accent);
    }
  }
  else if (gScreen == SCREEN_CONNECT) {
    uint16_t iconCol = gWifiOn ? lcd.color565(0, 255, 140) : accent;
    lcd.drawCircle(CENTER_X, CENTER_Y, 95, iconCol);
    lcd.fillCircle(CENTER_X, CENTER_Y + 34, 10, iconCol);
    lcd.drawArc(CENTER_X, CENTER_Y + 34, 36, 28, 225, 315, iconCol);
    lcd.drawArc(CENTER_X, CENTER_Y + 34, 62, 54, 225, 315, iconCol);
    lcd.drawArc(CENTER_X, CENTER_Y + 34, 88, 80, 225, 315, iconCol);
  }
}
