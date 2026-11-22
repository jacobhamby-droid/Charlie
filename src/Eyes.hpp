#pragma once
#include "Config.h"
#include "LGFX_Charlie.hpp"

void eyesInit(LGFX_Charlie& lcd);
void eyesUpdate(float dt, float gravityDeg);
void eyesDraw(LGFX_Charlie& lcd);
void eyesSetFromMood();
void eyesLook(float x, float y, float speed = 5.0f);
void eyesBlink();
void eyesCosmeticLayer(LGFX_Charlie& lcd);
