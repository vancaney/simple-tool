//
// Created by why on 2024/3/7.
//

#ifndef UNTITLED1_LOGLIST_H
#define UNTITLED1_LOGLIST_H

#include <stdbool.h>

typedef struct {
    char *log;
    int log_length;
} Log;

typedef struct {
    Log *logs;
    int cur_index;
    int log_list_length;
} logList;

logList *createLogList(int log_list_length, int log_length);
bool isEmpty(logList *ll);
bool isFull(logList *ll);
bool haveEnoughCapacity(logList* ll);
void add(logList *ll, const char* str);
void clearLogList(logList *ll);
#endif //UNTITLED1_LOGLIST_H

