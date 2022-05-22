/**
 * Copyright (c) 2022 KingK
 * 
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "bubble.h"

bool OnUserCreate(void)
{
    return true;
};

bool OnUserUpdate(double dt)
{
    if(keys[VK_ESCAPE].bPressed) return false;
    bubble_console_draw_circle( 32,25,10,PIXEL_SOLID,FG_WHITE);
    return true;
};

bool OnUserDestroy(void)
{
    return true;
};

int wmain(int argc, wchar_t const *argv[])
{
    bubble_console_int(60,50,12,12);
    return 0;
}
