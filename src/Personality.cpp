#include "Personality.hpp"
#include <Preferences.h>

Preferences prefs;
Mood gMood;
float gPersonality[PERSONALITY_COUNT] = {0.6f, 0.55f, 0.7f, 0.4f, 0.65f, 0.5f};
uint32_t gEyeColor = DEFAULT_EYE_COLOR;
bool gSleeping = false;
uint32_t gSteps = 0;

void personalityInit() {
  personalityLoadFromNVS();
}

void personalityLoadFromNVS() {
  prefs.begin("charlie", true);
  for (int i = 0; i < PERSONALITY_COUNT; i++) {
    gPersonality[i] = prefs.getFloat(("p" + String(i)).c_str(), gPersonality[i]);
  }
  gEyeColor = prefs.getUInt("eyeColor", DEFAULT_EYE_COLOR);
  gSteps = prefs.getUInt("steps", 0);
  prefs.end();
}

void personalitySaveToNVS() {
  prefs.begin("charlie", false);
  for (int i = 0; i < PERSONALITY_COUNT; i++) {
    prefs.putFloat(("p" + String(i)).c_str(), gPersonality[i]);
  }
  prefs.putUInt("eyeColor", gEyeColor);
  prefs.putUInt("steps", gSteps);
  prefs.end();
}

void personalityApplyFromApp(float values[PERSONALITY_COUNT]) {
  for (int i = 0; i < PERSONALITY_COUNT; i++) {
    gPersonality[i] = constrain(values[i], 0.0f, 1.0f);
  }
  personalitySaveToNVS();
}

void personalityUpdate(float dt) {
  // natural mood drift
  gMood.happiness += (0.55f - gMood.happiness) * dt * 0.08f;
  gMood.energy    += (gPersonality[ENERGY_BASELINE] - gMood.energy) * dt * 0.05f;
  gMood.annoyance *= (1.0f - dt * 0.15f);

  // night sleep (9 PM to 7 AM)
  time_t now = time(nullptr);
  struct tm* t = localtime(&now);
  if (t && t->tm_year > 100) {
    if (t->tm_hour >= 21 || t->tm_hour < 7) {
      gSleeping = true;
      gMood.energy *= 0.92f;
    } else {
      gSleeping = false;
    }
  }
}
