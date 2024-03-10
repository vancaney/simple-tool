//
// Created by why on 2024/2/23.
//

#ifndef UNTITLED1_MENU_H
#define UNTITLED1_MENU_H

#include <switch.h>

typedef struct {
    char name[255];
    u32 x;
    u32 y;
    u32 index; // 位于主菜单的第几个行
} Detail;

typedef struct {
    u32 max_selection_number;
    u32 cur_selection_number;
    bool print_flag;
    Detail selection[];
} Menu;

void Init_Menu(Menu **menu, u32 size, bool flag);
u32 Init_selection_detail(Menu *menu, const char *name, u32 x, u32 y);
#endif //UNTITLED1_MENU_H
