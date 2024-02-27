//
// Created by why on 2024/2/23.
//
#include <stdlib.h>
#include "menu.h"

void Init_Menu(Menu **menu, u32 size, bool flag){
    *menu = (Menu *) malloc(sizeof(Menu) + size * sizeof(Detail));
    (*menu)->selection_number = size;
    (*menu)->print_flag = flag;
}
