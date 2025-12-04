#include "../include/common.h"
#include "../include/items.h"
#include "../include/persistence.h"
#include "../include/log.h"
#include "../include/session.h"
#include <stdio.h>
#include <string.h>

static int next_item_id(Room *r) {
    int maxid = 0;
    for (int j = 0; j < r->item_count; j++)
        if (r->items[j].id > maxid) maxid = r->items[j].id;
    return maxid + 1;
}

int add_item(int room_id, char *license_plate,
             int start_price, int step_price,
             int buy_now_price, char *admin)
{
    for (int i = 0; i < room_count; i++) {
        if (rooms[i].id == room_id && rooms[i].active) {
            if (strcmp(rooms[i].admin, admin) != 0)
                return -1;

            if (rooms[i].item_count >= MAX_ITEMS)
                return -2;

            Room *r = &rooms[i];
            Item *item = &r->items[r->item_count++];
            item->id = next_item_id(r);

            strcpy(item->license_plate, license_plate);
            item->current_price = start_price;
            item->step_price    = step_price;
            item->buy_now_price = buy_now_price;
            strcpy(item->leader, "None");
            item->status = 0;

            item->auction_active = 1;
            item->warned30 = 0;
            item->end_time = time(NULL) + AUCTION_DURATION;

            save_items(room_id);

            log_event("ADD_ITEM admin=%s room_id=%d item_id=%d plate=%s start=%d step=%d buy=%d",
                      admin, room_id, item->id, license_plate, start_price, step_price, buy_now_price);

            char bmsg[256];
            snprintf(bmsg, sizeof(bmsg),
                     "ITEM_ADDED room %d item %d plate=%s start=%d step=%d buy=%d\n",
                     room_id, item->id, license_plate, start_price, step_price, buy_now_price);

            broadcast_to_room_nolock(room_id, bmsg);
            return item->id;
        }
    }
    return 0;
}

int remove_item(int room_id, int item_id, char *admin) {
    for (int i = 0; i < room_count; i++) {
        if (rooms[i].id == room_id && rooms[i].active) {
            if (strcmp(rooms[i].admin, admin) != 0)
                return -1;

            Room *r = &rooms[i];
            for (int j = 0; j < r->item_count; j++) {
                if (r->items[j].id == item_id) {
                    char plate[20];
                    strcpy(plate, r->items[j].license_plate);

                    for (int k = j; k < r->item_count - 1; k++)
                        r->items[k] = r->items[k + 1];
                    r->item_count--;

                    save_items(room_id);

                    log_event("REMOVE_ITEM admin=%s room_id=%d item_id=%d plate=%s",
                              admin, room_id, item_id, plate);

                    char bmsg[128];
                    snprintf(bmsg, sizeof(bmsg),
                             "ITEM_REMOVED room %d item %d plate=%s\n",
                             room_id, item_id, plate);
                    broadcast_to_room_nolock(room_id, bmsg);

                    return 1;
                }
            }
            return 0;
        }
    }
    return -2;
}

void list_items(int sock, int room_id) {
    char msg[2048] = "ITEM_LIST\n";
    char line[256];

    time_t now = time(NULL);

    for (int i = 0; i < room_count; i++) {
        if (rooms[i].id == room_id && rooms[i].active) {
            Room *r = &rooms[i];
            for (int j = 0; j < r->item_count; j++) {
                Item *item = &r->items[j];
                long rem = (item->auction_active && item->status==0)
                           ? (long)(item->end_time - now) : 0;
                if (rem < 0) rem = 0;

                sprintf(line,
                        "%d. %s | Giá: %d | Step: %d | BuyNow: %d | Leader: %s | Status: %s | Remain: %lds\n",
                        item->id,
                        item->license_plate,
                        item->current_price,
                        item->step_price,
                        item->buy_now_price,
                        item->leader,
                        item->status == 0 ? "ACTIVE" : "SOLD",
                        rem);
                strcat(msg, line);
            }
        }
    }

    send(sock, msg, strlen(msg), 0);
}
