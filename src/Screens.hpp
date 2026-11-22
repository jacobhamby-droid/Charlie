#pragma once
#include "LGFX_Charlie.hpp"
#include "Config.h"

void screensInit(LGFX_Charlie& lcd);
void screensUpdate(float dt);
void screensDraw(LGFX_Charlie& lcd);
void onSwipe(int dx, int dy);
