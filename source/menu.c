//
// Created by why on 2024/2/23.
//
#include <stdlib.h>
#include <string.h>
#include "menu.h"

void Init_Menu(Menu **menu, u32 size, bool flag){
    *menu = (Menu *) malloc(sizeof(Menu) + size * sizeof(Detail));
    (*menu)->max_selection_number = size;
    (*menu)->print_flag = flag;
    (*menu)->cur_selection_number = 0;
}

u32 Init_selection_detail(Menu *menu, const char *name, u32 x, u32 y){
    strcpy(menu->selection[menu->cur_selection_number].name, name);
    u32 i = menu->cur_selection_number;
    menu->selection[menu->cur_selection_number].index = i;
    menu->selection[menu->cur_selection_number].x = x;
    menu->selection[menu->cur_selection_number].y = y;
    menu->cur_selection_number++;
    return i;
}
