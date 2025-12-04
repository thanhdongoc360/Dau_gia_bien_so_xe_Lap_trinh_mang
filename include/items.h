#ifndef ITEMS_H
#define ITEMS_H

int add_item(int room_id, char *license_plate,
             int start_price, int step_price,
             int buy_now_price, char *admin);

int remove_item(int room_id, int item_id, char *admin);

void list_items(int sock, int room_id);

#endif
