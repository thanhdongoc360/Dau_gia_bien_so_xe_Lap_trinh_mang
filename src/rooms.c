#include "../include/common.h"
#include "../include/rooms.h"
#include "../include/persistence.h"
#include "../include/log.h"
#include <stdio.h>
#include <string.h>

int create_room(char *name, char *admin) {
    if (room_count >= MAX_ROOMS) return -1;

    Room *r = &rooms[room_count];
    r->id = next_room_id++;
    strcpy(r->name, name);
    r->active = 1;
    strcpy(r->admin, admin);
    r->member_count = 0;
    r->item_count   = 0;

    room_count++;

    save_rooms();
    log_event("CREATE_ROOM admin=%s room_id=%d name=%s", admin, r->id, name);
    return r->id;
}

int close_room(int id, char *admin) {
    for (int i = 0; i < room_count; i++) {
        if (rooms[i].id == id && rooms[i].active) {
            if (strcmp(rooms[i].admin, admin) != 0)
                return -1;
            rooms[i].active = 0;
            rooms[i].member_count = 0;

            save_rooms();
            log_event("CLOSE_ROOM admin=%s room_id=%d", admin, id);
            return 1;
        }
    }
    return 0;
}

int open_room(int id, char *admin) {
    for (int i = 0; i < room_count; i++) {
        if (rooms[i].id == id) {
            if (strcmp(rooms[i].admin, admin) != 0)
                return -1;
            rooms[i].active = 1;

            save_rooms();
            log_event("OPEN_ROOM admin=%s room_id=%d", admin, id);
            return 1;
        }
    }
    return 0;
}

void list_rooms(int sock) {
    char msg[1024] = "ROOM_LIST\n";
    char line[128];

    for (int i = 0; i < room_count; i++) {
        sprintf(line, "%d. %s [%s]\n",
                rooms[i].id,
                rooms[i].name,
                rooms[i].active ? "OPEN" : "CLOSED");
        strcat(msg, line);
    }
    send(sock, msg, strlen(msg), 0);
}

int user_current_room(const char *username) {
    for (int i = 0; i < room_count; i++) {
        for (int j = 0; j < rooms[i].member_count; j++) {
            if (strcmp(rooms[i].members[j], username) == 0)
                return rooms[i].id;
        }
    }
    return 0;
}

int join_room(char *username, int id) {
    int cur = user_current_room(username);
    if (cur != 0 && cur != id) return -3;

    for (int i = 0; i < room_count; i++) {
        if (rooms[i].id == id && rooms[i].active) {

            for (int j = 0; j < rooms[i].member_count; j++) {
                if (strcmp(rooms[i].members[j], username) == 0)
                    return -2;
            }
            if (rooms[i].member_count >= MAX_ROOM_USERS)
                return -4;

            strcpy(rooms[i].members[rooms[i].member_count++], username);
            return 1;
        }
    }
    return 0;
}

int leave_room_and_getid(char *username) {
    for (int i = 0; i < room_count; i++) {
        for (int j = 0; j < rooms[i].member_count; j++) {
            if (strcmp(rooms[i].members[j], username) == 0) {
                int rid = rooms[i].id;
                for (int k = j; k < rooms[i].member_count - 1; k++)
                    strcpy(rooms[i].members[k], rooms[i].members[k + 1]);
                rooms[i].member_count--;
                return rid;
            }
        }
    }
    return 0;
}
