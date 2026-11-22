#pragma once
#include "Config.h"

void sensorsInit();
void sensorsUpdate(float dt);
float getGravityAngle();
bool  isPickedUp();
bool  isShaken();
void  processTouch(int16_t x, int16_t y, bool pressed, uint32_t pressTime);
