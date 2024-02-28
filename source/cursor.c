#include <switch.h>
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
    for (u32 i = cursor->y - cursor->height; i < cursor->y; i++) {
        for (u32 j = cursor->x; j < cursor->width; j++) {
            u32 pos = i * stride / sizeof(u32) + j;
            if (!(*text_draw)[j][i])
                framebuf[pos] = RGBA8_MAXALPHA(R, G, B);
        }
    }
}

void decide_menu_up(Cursor *cursor, const Menu *menu, int selection){
    for(int i = selection; i > 0; --i){
        if(cursor->y == menu->selection[i].y){
            if(i == 1){
                cursor->y = menu->selection[selection].y;
            }else{
                cursor->y = menu->selection[--i].y;
            }
            break;
        }
    }
}

void decide_menu_down(Cursor *cursor, const Menu *menu, int selection){
    for(int i = 0; i <= selection; ++i){
        if(cursor->y == menu->selection[i].y){
            if(i < selection){
                cursor->y = menu->selection[++i].y;
            }else{
                cursor->y = menu->selection[1].y;
            }
            break;
        }
    }
}



