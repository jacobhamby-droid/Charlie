#pragma once
#include "Config.h"

void personalityInit();
void personalityUpdate(float dt);
void personalityLoadFromNVS();
void personalitySaveToNVS();
void personalityApplyFromApp(float values[PERSONALITY_COUNT]);
