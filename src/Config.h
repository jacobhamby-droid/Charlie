#pragma once
#include <Arduino.h>

#define SCREEN_W          466
#define SCREEN_H          466
#define CENTER_X          (SCREEN_W / 2)
#define CENTER_Y          (SCREEN_H / 2)

#define DEFAULT_EYE_COLOR 0x00D4FF

enum PersonalityIndex {
    CURIOSITY = 0,
    PLAYFULNESS,
    AFFECTION,
    SENSITIVITY,
    ENERGY_BASELINE,
    STUBBORNNESS,
    PERSONALITY_COUNT
};

enum ScreenID {
    SCREEN_FACE = 0,
    SCREEN_TIMER,
    SCREEN_CONNECT
};

struct Mood {
    float happiness = 0.65f;
    float energy    = 0.75f;
    float annoyance = 0.0f;
};

struct EyeState {
    float lookX     = 0.0f;
    float lookY     = 0.0f;
    float lidUpper  = 0.12f;
    float lidLower  = 0.06f;
    float lidAngle  = 0.0f;
    float scale     = 1.0f;
    float glow      = 1.0f;
    uint32_t color  = DEFAULT_EYE_COLOR;
};

extern Mood gMood;
extern float gPersonality[PERSONALITY_COUNT];
extern EyeState gLeft, gRight;
extern ScreenID gScreen;
extern bool gWifiOn;
extern uint32_t gSteps;
extern bool gSleeping;
extern uint32_t gEyeColor;
