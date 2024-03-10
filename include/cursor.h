//
// Created by why on 2024/2/22.
//

#ifndef UNTITLED1_CURSOR_H
#define UNTITLED1_CURSOR_H

#include <switch.h>
#include "menu.h"

#define FT_WIDTH 1280
#define FT_HEIGHT 720
typedef struct {
    u32 x;
    u32 y;
    u32 width;
    u32 height;
    u32 offset;
} Cursor;

void Init_Cursor(Cursor *cursor, u32 x, u32 y, u32 width, u32 height, u32 offset);

void Draw_Cursor(Cursor *cursor, u32 stride, bool (*text_draw)[FT_WIDTH][FT_HEIGHT], u32 *framebuf, u32 R, u32 G, u32 B);
void decide_menu_up(Cursor *cursor, const Menu *menu, u32 selection);
void decide_menu_down(Cursor *cursor, const Menu *menu, u32 selection);

#endif //UNTITLED1_CURSOR_H
