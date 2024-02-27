#include <switch.h>
#include <stdlib.h>
#include "cursor.h"

//
// Created by why on 2024/2/22.
//
void Init_Cursor(Cursor *cursor, u32 x, u32 y, u32 width, u32 height, u32 offset) {
    cursor->x = x;
    cursor->y = y;
    cursor->width = width;
    cursor->height = height;
    cursor->offset = offset;
}

void Draw_Cursor(Cursor *cursor, u32 stride, bool (*text_draw)[FT_WIDTH][FT_HEIGHT], u32 *framebuf, u32 R, u32 G, u32 B){
    for (u32 i = cursor->y; i < cursor->height; i++) {
        for (u32 j = cursor->x; j < cursor->width; j++) {
            u32 pos = i * stride / sizeof(u32) + j;
            if (!(*text_draw)[j][i])
                framebuf[pos] = RGBA8_MAXALPHA(R, G, B);
        }
    }
}

void Destroy_Cursor(Cursor *cursor){
    free(cursor);
}

