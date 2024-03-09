#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "logList.h"

//
// Created by why on 2024/3/7.
//
logList *createLogList(int log_list_length, int log_length) {
    logList *log_list = malloc(sizeof(logList));
    log_list->logs = malloc(log_list_length * sizeof(Log));
    for (int i = 0; i < log_list_length; ++i) {
        log_list->logs[i].log = malloc(log_length * sizeof(char));
        log_list->logs[i].log_length = log_length;
    }
    log_list->log_list_length = log_list_length;
    log_list->cur_index = -1;
    return log_list;
}

bool isEmpty(logList *ll) {
    assert(ll != NULL);
    return ll->cur_index == -1;
}

bool isFull(logList *ll) {
    assert(ll != NULL);
    return ll->cur_index == ll->log_list_length - 1;
}

void add(logList *ll, const char *str) {
    if (isFull(ll)) {
        for (int i = 0; i < ll->log_list_length - 1; ++i) {
            strcpy(ll->logs[i].log, ll->logs[i + 1].log);
        }
        strcpy(ll->logs[ll->log_list_length - 1].log, str);
    } else {
        ll->cur_index++;
        strcpy(ll->logs[ll->cur_index].log, str);
    }
}

void clearLogList(logList *ll){
    assert(ll != NULL);
    for (int i = 0; i < ll->log_list_length; ++i) {
        free(ll->logs[i].log);
    }
    free(ll->logs);
    free(ll);
}