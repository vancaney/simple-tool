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
} Detail;

typedef struct {
    u32 selection_number;
    bool print_flag;
    Detail selection[];
} Menu;

void Init_Menu(Menu **menu, u32 size, bool flag);
#endif //UNTITLED1_MENU_H
