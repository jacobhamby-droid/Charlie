#include "Eyes.hpp"
#include <math.h>

EyeState gLeft;
EyeState gRight;

static LGFX_Charlie* lcd = nullptr;
static LGFX_Sprite canvas;
static float breath = 0.0f;
static float microX = 0, microY = 0;
static float targetX = 0, targetY = 0;
static float smoothRoll = 0.0f;
static float blinkT = 3.0f;
static bool  blinking = false;
static float blinkP = 0;

void eyesInit(LGFX_Charlie& l) {
  lcd = &l;
  gLeft.color = gRight.color = gEyeColor;
  canvas.setPsram(true);
  canvas.setColorDepth(16);
  canvas.createSprite(SCREEN_W, SCREEN_H);
}

void eyesSetFromMood() {
  float open = 0.18f + gMood.energy * 0.22f + gPersonality[CURIOSITY] * 0.12f;
  float upper = constrain(1.0f - open - gMood.happiness * 0.18f, 0.0f, 0.65f);
  gLeft.lidUpper = gRight.lidUpper = upper;
  gLeft.lidLower = gRight.lidLower = (gMood.happiness > 0.75f) ? 0.28f : (0.04f + (1.0f - gMood.energy) * 0.12f);
  gLeft.lidAngle = (gMood.annoyance > 0.4f) ? (gMood.annoyance * 22.0f) : 0.0f;
  gRight.lidAngle = -gLeft.lidAngle;
  gLeft.glow = gRight.glow = 0.78f + gMood.happiness * 0.22f;
}

void eyesLook(float x, float y, float speed) {
  targetX = constrain(x, -1.0f, 1.0f);
  targetY = constrain(y, -1.0f, 1.0f);
}

void eyesBlink() {
  if (!blinking) { blinking = true; blinkP = 0; }
}

void eyesUpdate(float dt, float gravityDeg) {
  breath += dt * 1.7f;
  float bScale = 1.0f + sinf(breath) * 0.012f;

  // Imperceptible anti-burn-in drift + micro-saccades
  microX = sinf(millis() * 0.00055f) * 2.2f;
  microY = cosf(millis() * 0.00075f) * 1.8f;

  // Smooth gravity-leveling rotation
  smoothRoll += (gravityDeg - smoothRoll) * min(1.0f, dt * 8.0f);

  gLeft.lookX  += (targetX - gLeft.lookX) * min(1.0f, dt * 6.0f);
  gLeft.lookY  += (targetY - gLeft.lookY) * min(1.0f, dt * 6.0f);
  gRight.lookX = gLeft.lookX;
  gRight.lookY = gLeft.lookY;

  if (blinking) {
    blinkP += dt * 11.0f;
    if (blinkP >= 1.0f) { blinking = false; blinkP = 0; }
    float b = (blinkP < 0.5f) ? blinkP * 2.0f : (1.0f - blinkP) * 2.0f;
    gLeft.lidUpper = gRight.lidUpper = max(gLeft.lidUpper, b);
  }

  blinkT -= dt;
  if (blinkT <= 0) {
    eyesBlink();
    blinkT = 2.2f + (random(0, 4800) / 1000.0f);
  }

  gLeft.scale = gRight.scale = bScale;
}

static void drawSoftEye(LGFX_Sprite& s, int cx, int cy, int w, int h, uint32_t col,
                        float glow, float lidU, float lidL, float angleDeg, bool happyArch) {
  int r = w / 3;

  // Soft outer perimeter fade for 3D OLED depth
  for (int i = 10; i >= 1; --i) {
    float a = powf((11 - i) / 11.0f, 1.8f) * 0.45f * glow;
    uint16_t c = s.color565(
      ((col >> 16) & 0xFF) * a,
      ((col >>  8) & 0xFF) * a,
      ( col        & 0xFF) * a);
    s.fillRoundRect(cx - w/2 - i, cy - h/2 - i, w + i*2, h + i*2, r + i, c);
  }

  // Constant squircle core (never warps shape)
  uint16_t mainC = s.color565((col>>16)&0xFF, (col>>8)&0xFF, col&0xFF);
  s.fillRoundRect(cx - w/2, cy - h/2, w, h, r, mainC);

  // Upper eyelid black mask (supports angled \ / and / \ expressions)
  if (lidU > 0.01f || fabsf(angleDeg) > 0.5f) {
    int lidH = (int)(h * lidU);
    float rad = angleDeg * DEG_TO_RAD;
    int tiltDy = (int)(tanf(rad) * (w * 0.65f));
    int x0 = cx - w/2 - 14, x1 = cx + w/2 + 14;
    int yTop = cy - h/2 - 25;
    int yL = cy - h/2 + lidH + tiltDy;
    int yR = cy - h/2 + lidH - tiltDy;
    s.fillTriangle(x0, yTop, x1, yTop, x0, yL, TFT_BLACK);
    s.fillTriangle(x1, yTop, x1, yR, x0, yL, TFT_BLACK);
  }

  // Lower eyelid black mask (supports happy ^ ^ crescent arch or flat squint)
  if (lidL > 0.01f) {
    int lidH = (int)(h * lidL);
    if (happyArch) {
      s.fillCircle(cx, cy + h/2 + (w/2) - lidH, (int)(w * 0.62f), TFT_BLACK);
    } else {
      s.fillRect(cx - w/2 - 14, cy + h/2 - lidH, w + 28, lidH + 20, TFT_BLACK);
    }
  }
}

void eyesDraw(LGFX_Charlie& l) {
  canvas.fillScreen(TFT_BLACK);

  int w   = 98;
  int h   = 126;
  int gap = 74;
  float rad = -smoothRoll * DEG_TO_RAD;
  float cosR = cosf(rad), sinR = sinf(rad);

  // Rotate eye centers around screen center so eyes stay level when tilted
  float offLx = -gap + gLeft.lookX * 22.0f + microX;
  float offLy = -4.0f + gLeft.lookY * 18.0f + microY;
  float offRx =  gap + gRight.lookX * 22.0f + microX;
  float offRy = -4.0f + gRight.lookY * 18.0f + microY;

  int lx = CENTER_X + (int)(offLx * cosR - offLy * sinR);
  int ly = CENTER_Y + (int)(offLx * sinR + offLy * cosR);
  int rx = CENTER_X + (int)(offRx * cosR - offRy * sinR);
  int ry = CENTER_Y + (int)(offRx * sinR + offRy * cosR);

  bool isHappy = (gMood.happiness > 0.72f);

  drawSoftEye(canvas, lx, ly, (int)(w * gLeft.scale), (int)(h * gLeft.scale),
              gLeft.color, gLeft.glow, gLeft.lidUpper, gLeft.lidLower, gLeft.lidAngle, isHappy);
  drawSoftEye(canvas, rx, ry, (int)(w * gRight.scale), (int)(h * gRight.scale),
              gRight.color, gRight.glow, gRight.lidUpper, gRight.lidLower, gRight.lidAngle, isHappy);

  canvas.pushSprite(0, 0);
  eyesCosmeticLayer(l);
}

void eyesCosmeticLayer(LGFX_Charlie& l) {
  // empty – future hats / glasses / mini pets go here
}
