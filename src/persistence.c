#include "../include/common.h"
#include "../include/persistence.h"
#include <stdio.h>
#include <string.h>

static void items_filename(int room_id, char *out, size_t n){
    snprintf(out, n, "items_room%d.txt", room_id);
}

void save_rooms() {
    FILE *f = fopen("rooms.txt", "w");
    if (!f) return;

    for (int i = 0; i < room_count; i++) {
        Room *r = &rooms[i];
        fprintf(f, "%d %s %d %s\n", r->id, r->name, r->active, r->admin);
    }
    fclose(f);
}

void load_rooms() {
    FILE *f = fopen("rooms.txt", "r");
    if (!f) {
        room_count = 0;
        next_room_id = 1;
        return;
    }

    room_count = 0;
    int max_id = 0;

    while (room_count < MAX_ROOMS) {
        Room r;
        if (fscanf(f, "%d %49s %d %49s",
                   &r.id, r.name, &r.active, r.admin) != 4) break;

        r.member_count = 0;
        r.item_count   = 0;

        rooms[room_count++] = r;
        if (r.id > max_id) max_id = r.id;
    }
    fclose(f);

    next_room_id = max_id + 1;
}

void save_items(int room_id) {
    char fn[64];
    items_filename(room_id, fn, sizeof(fn));
    FILE *f = fopen(fn, "w");
    if (!f) return;

    for (int i = 0; i < room_count; i++) {
        if (rooms[i].id != room_id) continue;
        Room *r = &rooms[i];

        for (int j = 0; j < r->item_count; j++) {
            Item *it = &r->items[j];
            fprintf(f, "%d %s %d %d %d %s %d %ld %d %d\n",
                    it->id, it->license_plate,
                    it->current_price, it->step_price, it->buy_now_price,
                    it->leader, it->status,
                    (long)it->end_time, it->auction_active, it->warned30);
        }
        break;
    }
    fclose(f);
}

void load_items(int room_id) {
    char fn[64];
    items_filename(room_id, fn, sizeof(fn));
    FILE *f = fopen(fn, "r");
    if (!f) return;

    for (int i = 0; i < room_count; i++) {
        if (rooms[i].id != room_id) continue;
        Room *r = &rooms[i];

        r->item_count = 0;
        while (r->item_count < MAX_ITEMS) {
            Item it;
            long endt;
            if (fscanf(f, "%d %19s %d %d %d %49s %d %ld %d %d",
                       &it.id, it.license_plate,
                       &it.current_price, &it.step_price, &it.buy_now_price,
                       it.leader, &it.status,
                       &endt, &it.auction_active, &it.warned30) != 10) break;

            it.end_time = (time_t)endt;
            r->items[r->item_count++] = it;
        }
        break;
    }
    fclose(f);
}
