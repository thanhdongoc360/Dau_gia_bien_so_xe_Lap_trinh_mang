#include "../include/common.h"
#include "../include/history.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

void append_history(const char *winner, int room_id, Item *it, const char *method) {
    pthread_mutex_lock(&history_mutex);
    FILE *f = fopen("history.txt", "a");
    if (!f) { pthread_mutex_unlock(&history_mutex); return; }

    time_t now = time(NULL);
    fprintf(f, "%ld %s %d %d %s %d %s\n",
            (long)now, winner, room_id, it->id,
            it->license_plate, it->current_price, method);

    fclose(f);
    pthread_mutex_unlock(&history_mutex);
}

void send_history(int sock, const char *username) {
    pthread_mutex_lock(&history_mutex);
    FILE *f = fopen("history.txt", "r");
    if (!f) {
        pthread_mutex_unlock(&history_mutex);
        send(sock, "HISTORY_EMPTY\n", 14, 0);
        return;
    }

    char msg[4096] = "HISTORY_LIST\n";
    int found = 0;

    long ts;
    char u[50], plate[20], method[20];
    int room_id, item_id, price;

    while (fscanf(f, "%ld %49s %d %d %19s %d %19s",
                  &ts, u, &room_id, &item_id, plate, &price, method) == 7)
    {
        if (strcmp(u, username) == 0) {
            found = 1;
            time_t t = (time_t)ts;
            struct tm *tm_info = localtime(&t);
            char tss[32];
            strftime(tss, sizeof(tss), "%Y-%m-%d %H:%M:%S", tm_info);

            char line[256];
            snprintf(line, sizeof(line),
                     "%s | Room %d | Item %d | %s | Price %d | %s\n",
                     tss, room_id, item_id, plate, price, method);
            if (strlen(msg) + strlen(line) < sizeof(msg) - 1)
                strcat(msg, line);
        }
    }

    fclose(f);
    pthread_mutex_unlock(&history_mutex);

    if (!found) strcpy(msg, "HISTORY_EMPTY\n");
    send(sock, msg, strlen(msg), 0);
}
