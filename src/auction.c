#include "../include/common.h"
#include "../include/auction.h"
#include "../include/persistence.h"
#include "../include/history.h"
#include "../include/session.h"
#include "../include/log.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void *auction_manager(void *arg) {
    (void)arg;
    while (1) {
        sleep(1);
        time_t now = time(NULL);

        pthread_mutex_lock(&rooms_mutex);
        for (int i = 0; i < room_count; i++) {
            Room *r = &rooms[i];
            for (int j = 0; j < r->item_count; j++) {
                Item *it = &r->items[j];
                if (it->status != 0 || !it->auction_active) continue;

                long rem = (long)(it->end_time - now);

                if (rem <= 0) {
                    it->status = 1;
                    it->auction_active = 0;
                    save_items(r->id);

                    log_event("SOLD_TIMEOUT room_id=%d item_id=%d plate=%s winner=%s price=%d",
                              r->id, it->id, it->license_plate, it->leader, it->current_price);

                    if (strcmp(it->leader, "None") != 0)
                        append_history(it->leader, r->id, it, "TIMEOUT");

                    char soldmsg[256];
                    snprintf(soldmsg, sizeof(soldmsg),
                             "SOLD item %d plate=%s winner=%s price=%d\n",
                             it->id, it->license_plate, it->leader, it->current_price);
                    broadcast_to_room_nolock(r->id, soldmsg);
                }
                else if (rem == 30 && !it->warned30) {
                    it->warned30 = 1;
                    save_items(r->id);

                    char warnmsg[128];
                    snprintf(warnmsg, sizeof(warnmsg),
                             "WARNING_30S item %d plate=%s remain=30s\n",
                             it->id, it->license_plate);
                    broadcast_to_room_nolock(r->id, warnmsg);
                }
            }
        }
        pthread_mutex_unlock(&rooms_mutex);
    }
    return NULL;
}
