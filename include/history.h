#include "common.h"
#ifndef HISTORY_H
#define HISTORY_H

void append_history(const char *winner, int room_id, Item *it, const char *method);
void send_history(int sock, const char *username);

#endif
