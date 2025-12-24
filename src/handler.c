#include "../include/common.h"
#include "../include/users.h"
#include "../include/rooms.h"
#include "../include/items.h"
#include "../include/history.h"
#include "../include/session.h"
#include "../include/persistence.h"
#include "../include/log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <errno.h>

void *handle_client(void *arg) {
    int client_sock = *((int *)arg);
    free(arg);

    char buffer[BUF_SIZE];
    char username[50] = "";
    char role[10]     = "";

    struct timeval tv;
    tv.tv_sec  = 300;
    tv.tv_usec = 0;
    setsockopt(client_sock, SOL_SOCKET, SO_RCVTIMEO,
               (const char *)&tv, sizeof(tv));

    while (1) {
        int n = recv(client_sock, buffer, BUF_SIZE, 0);
        if (n <= 0) {
            if (n < 0 && (errno == EWOULDBLOCK || errno == EAGAIN))
                printf("Client %d timeout.\n", client_sock);
            break;
        }
        buffer[n] = '\0';

        // REGISTER
        if (strncmp(buffer, "REGISTER", 8) == 0) {
            char u[50], p[50];
            sscanf(buffer, "REGISTER %49s %49s", u, p);
            if (register_user(u, p))
                send(client_sock, "REGISTER_OK\n", 12, 0);
            else
                send(client_sock, "REGISTER_FAIL\n", 14, 0);
        }

        // LOGIN
        else if (strncmp(buffer, "LOGIN", 5) == 0) {
            char u[50], p[50];
            sscanf(buffer, "LOGIN %49s %49s", u, p);
            int res = login_user(u, p, role);
            if (res == 1) {
                strcpy(username, u);
                update_session_login(client_sock, username, role);
                char msg[100];
                sprintf(msg, "LOGIN_OK %s\n", role);
                send(client_sock, msg, strlen(msg), 0);
            } else if (res == -1) {
                send(client_sock, "LOGIN_FAIL_ALREADY_ONLINE\n", 26, 0);
            } else {
                send(client_sock, "LOGIN_FAIL\n", 11, 0);
            }
        }

        // LOGOUT
        else if (strncmp(buffer, "LOGOUT", 6) == 0) {
            if (strlen(username) == 0) {
                send(client_sock, "NEED_LOGIN\n", 11, 0);
            } else {
                pthread_mutex_lock(&rooms_mutex);
                int left = leave_room_and_getid(username);
                if (left) {
                    char bmsg[128];
                    snprintf(bmsg, sizeof(bmsg),
                             "USER_LEAVE %s room %d\n", username, left);
                    broadcast_to_room_nolock(left, bmsg);
                    log_event("LEAVE_ROOM user=%s room_id=%d (logout)", username, left);
                }
                pthread_mutex_unlock(&rooms_mutex);

                logout_user(username);
                remove_session(client_sock);

                send(client_sock, "LOGOUT_OK\n", 10, 0);
                username[0] = '\0';
                role[0] = '\0';
            }
        }

        // CHANGE_PASS
        else if (strncmp(buffer, "CHANGE_PASS", 11) == 0) {
            if (strlen(username) == 0) send(client_sock, "NEED_LOGIN\n", 11, 0);
            else {
                char oldp[50], newp[50];
                if (sscanf(buffer, "CHANGE_PASS %49s %49s", oldp, newp) != 2)
                    send(client_sock, "BAD_FORMAT\n", 11, 0);
                else if (change_password(username, oldp, newp))
                    send(client_sock, "CHANGE_PASS_OK\n", 15, 0);
                else
                    send(client_sock, "CHANGE_PASS_FAIL\n", 17, 0);
            }
        }

        // CHANGE_NAME
        else if (strncmp(buffer, "CHANGE_NAME", 11) == 0) {
            if (strlen(username) == 0) send(client_sock, "NEED_LOGIN\n", 11, 0);
            else {
                char newname[50];
                if (sscanf(buffer, "CHANGE_NAME %49s", newname) != 1)
                    send(client_sock, "BAD_FORMAT\n", 11, 0);
                else if (change_display_name(username, newname))
                    send(client_sock, "CHANGE_NAME_OK\n", 15, 0);
                else
                    send(client_sock, "CHANGE_NAME_FAIL\n", 17, 0);
            }
        }

        // HISTORY
        else if (strncmp(buffer, "HISTORY", 7) == 0) {
            if (strlen(username) == 0) send(client_sock, "NEED_LOGIN\n", 11, 0);
            else send_history(client_sock, username);
        }

        // CREATE_ROOM (admin)
        else if (strncmp(buffer, "CREATE_ROOM", 11) == 0) {
            if (strlen(username) == 0) send(client_sock, "NEED_LOGIN\n", 11, 0);
            else if (strcmp(role, "admin") != 0) send(client_sock, "PERMISSION_DENIED\n", 18, 0);
            else {
                char room_name[50];
                sscanf(buffer, "CREATE_ROOM %49s", room_name);

                pthread_mutex_lock(&rooms_mutex);
                int id = create_room(room_name, username);
                pthread_mutex_unlock(&rooms_mutex);

                if (id > 0) {
                    char msg[100];
                    sprintf(msg, "ROOM_CREATED %d\n", id);
                    send(client_sock, msg, strlen(msg), 0);
                } else send(client_sock, "ROOM_CREATE_FAIL\n", 17, 0);
            }
        }

        // OPEN_ROOM (admin)
        else if (strncmp(buffer, "OPEN_ROOM", 9) == 0) {
            if (strlen(username) == 0) send(client_sock, "NEED_LOGIN\n", 11, 0);
            else if (strcmp(role, "admin") != 0) send(client_sock, "PERMISSION_DENIED\n", 18, 0);
            else {
                int id; sscanf(buffer, "OPEN_ROOM %d", &id);

                pthread_mutex_lock(&rooms_mutex);
                int res = open_room(id, username);
                pthread_mutex_unlock(&rooms_mutex);

                if (res == 1) send(client_sock, "ROOM_OPENED\n", 12, 0);
                else send(client_sock, "ROOM_OPEN_FAIL\n", 15, 0);
            }
        }

        // CLOSE_ROOM (admin)
        else if (strncmp(buffer, "CLOSE_ROOM", 10) == 0) {
            if (strlen(username) == 0) send(client_sock, "NEED_LOGIN\n", 11, 0);
            else if (strcmp(role, "admin") != 0) send(client_sock, "PERMISSION_DENIED\n", 18, 0);
            else {
                int id; sscanf(buffer, "CLOSE_ROOM %d", &id);

                pthread_mutex_lock(&rooms_mutex);
                int res = close_room(id, username);
                pthread_mutex_unlock(&rooms_mutex);

                if (res == 1) send(client_sock, "ROOM_CLOSED\n", 12, 0);
                else send(client_sock, "ROOM_CLOSE_FAIL\n", 16, 0);
            }
        }

        // LIST_ROOMS
        else if (strncmp(buffer, "LIST_ROOMS", 10) == 0) {
            if (strlen(username) == 0) send(client_sock, "NEED_LOGIN\n", 11, 0);
            else list_rooms(client_sock);
        }

        // JOIN_ROOM
        else if (strncmp(buffer, "JOIN_ROOM", 9) == 0) {
            if (strlen(username) == 0) send(client_sock, "NEED_LOGIN\n", 11, 0);
            else {
                int id; sscanf(buffer, "JOIN_ROOM %d", &id);

                pthread_mutex_lock(&rooms_mutex);
                int res = join_room(username, id);
                if (res == 1) {
                    char bmsg[128];
                    snprintf(bmsg, sizeof(bmsg), "USER_JOIN %s room %d\n", username, id);
                    broadcast_to_room_nolock(id, bmsg);
                    log_event("JOIN_ROOM user=%s room_id=%d", username, id);
                }
                pthread_mutex_unlock(&rooms_mutex);

                if (res == 1) send(client_sock, "JOIN_OK\n", 8, 0);
                else if (res == -2) send(client_sock, "JOIN_ALREADY_IN_ROOM\n", 22, 0);
                else if (res == -3) send(client_sock, "JOIN_FAIL_OTHER_ROOM\n", 21, 0);
                else send(client_sock, "JOIN_FAIL\n", 10, 0);
            }
        }

        // PROMOTE_TO_ADMIN (admin only)
        else if (strncmp(buffer, "PROMOTE_TO_ADMIN", 16) == 0) {
            if (strlen(username) == 0) send(client_sock, "NEED_LOGIN\n", 11, 0);
            else if (strcmp(role, "admin") != 0) send(client_sock, "PERMISSION_DENIED\n", 18, 0);
            else {
                char target[50];
                sscanf(buffer, "PROMOTE_TO_ADMIN %49s", target);
                
                int res = promote_user_to_admin(target, username);
                if (res == 1) send(client_sock, "PROMOTE_OK\n", 11, 0);
                else if (res == -1) send(client_sock, "PROMOTE_ALREADY_ADMIN\n", 22, 0);
                else send(client_sock, "PROMOTE_FAIL\n", 13, 0);
                
                if (res == 1) {
                    log_event("PROMOTE_TO_ADMIN user=%s promoted_by=%s", target, username);
                }
            }
        }

        // LEAVE_ROOM
        else if (strncmp(buffer, "LEAVE_ROOM", 10) == 0) {
            if (strlen(username) == 0) send(client_sock, "NEED_LOGIN\n", 11, 0);
            else {
                pthread_mutex_lock(&rooms_mutex);
                int left = leave_room_and_getid(username);
                if (left) {
                    char bmsg[128];
                    snprintf(bmsg, sizeof(bmsg), "USER_LEAVE %s room %d\n", username, left);
                    broadcast_to_room_nolock(left, bmsg);
                    log_event("LEAVE_ROOM user=%s room_id=%d", username, left);
                }
                pthread_mutex_unlock(&rooms_mutex);
                send(client_sock, "LEAVE_OK\n", 9, 0);
            }
        }

        // ADD_ITEM (admin)
        else if (strncmp(buffer, "ADD_ITEM", 8) == 0) {
            if (strlen(username) == 0) send(client_sock, "NEED_LOGIN\n", 11, 0);
            else if (strcmp(role, "admin") != 0) send(client_sock, "PERMISSION_DENIED\n", 18, 0);
            else {
                int room_id, start_price, step_price, buy_now_price;
                char plate[20];
                sscanf(buffer, "ADD_ITEM %d %19s %d %d %d",
                       &room_id, plate, &start_price, &step_price, &buy_now_price);

                pthread_mutex_lock(&rooms_mutex);
                int res = add_item(room_id, plate, start_price, step_price, buy_now_price, username);
                pthread_mutex_unlock(&rooms_mutex);

                if (res > 0) send(client_sock, "ITEM_ADDED_OK\n", 14, 0);
                else send(client_sock, "ADD_ITEM_FAIL\n", 14, 0);
            }
        }

        // REMOVE_ITEM (admin)
        else if (strncmp(buffer, "REMOVE_ITEM", 11) == 0) {
            if (strlen(username) == 0) send(client_sock, "NEED_LOGIN\n", 11, 0);
            else if (strcmp(role, "admin") != 0) send(client_sock, "PERMISSION_DENIED\n", 18, 0);
            else {
                int room_id, item_id;
                sscanf(buffer, "REMOVE_ITEM %d %d", &room_id, &item_id);

                pthread_mutex_lock(&rooms_mutex);
                int res = remove_item(room_id, item_id, username);
                pthread_mutex_unlock(&rooms_mutex);

                if (res == 1) send(client_sock, "ITEM_REMOVED_OK\n", 16, 0);
                else send(client_sock, "REMOVE_ITEM_FAIL\n", 17, 0);
            }
        }

        // LIST_ITEMS (user phải đang ở room đó)
        else if (strncmp(buffer, "LIST_ITEMS", 10) == 0) {
            if (strlen(username) == 0) send(client_sock, "NEED_LOGIN\n", 11, 0);
            else {
                int room_id; sscanf(buffer, "LIST_ITEMS %d", &room_id);

                pthread_mutex_lock(&rooms_mutex);
                int cur = user_current_room(username);
                pthread_mutex_unlock(&rooms_mutex);

                if (cur == 0) send(client_sock, "NEED_JOIN_ROOM\n", 15, 0);
                else if (cur != room_id) send(client_sock, "NOT_IN_THIS_ROOM\n", 17, 0);
                else list_items(client_sock, room_id);
            }
        }

        // BID
        else if (strncmp(buffer, "BID", 3) == 0) {
            if (strlen(username) == 0) send(client_sock, "NEED_LOGIN\n", 11, 0);
            else {
                int room_id, item_id, amount;
                if (sscanf(buffer, "BID %d %d %d", &room_id, &item_id, &amount) != 3) {
                    send(client_sock, "BAD_FORMAT\n", 11, 0);
                    continue;
                }

                pthread_mutex_lock(&rooms_mutex);

                int cur = user_current_room(username);
                if (cur == 0) { pthread_mutex_unlock(&rooms_mutex); send(client_sock, "NEED_JOIN_ROOM\n", 15, 0); continue; }
                if (cur != room_id) { pthread_mutex_unlock(&rooms_mutex); send(client_sock, "NOT_IN_THIS_ROOM\n", 17, 0); continue; }

                Room *r = NULL; Item *it = NULL;
                for (int i = 0; i < room_count; i++)
                    if (rooms[i].id == room_id && rooms[i].active) { r = &rooms[i]; break; }

                if (!r) { pthread_mutex_unlock(&rooms_mutex); send(client_sock, "ROOM_NOT_FOUND\n", 15, 0); continue; }

                for (int j = 0; j < r->item_count; j++)
                    if (r->items[j].id == item_id) { it = &r->items[j]; break; }

                if (!it || it->status != 0 || !it->auction_active) {
                    pthread_mutex_unlock(&rooms_mutex);
                    send(client_sock, "ITEM_NOT_FOUND_OR_SOLD\n", 23, 0);
                    continue;
                }

                time_t now = time(NULL);
                if (now >= it->end_time) {
                    it->auction_active = 0; it->status = 1;
                    save_items(room_id);
                    pthread_mutex_unlock(&rooms_mutex);
                    send(client_sock, "AUCTION_ENDED\n", 14, 0);
                    continue;
                }

                int min_ok = it->current_price + it->step_price;
                if (amount < min_ok) {
                    pthread_mutex_unlock(&rooms_mutex);
                    send(client_sock, "BID_TOO_LOW\n", 12, 0);
                    continue;
                }

                it->current_price = amount;
                strcpy(it->leader, username);

                long rem = (long)(it->end_time - now);
                if (rem <= 5) {
                    it->end_time += 5;
                    rem += 5;
                    save_items(room_id);

                    char extmsg[128];
                    snprintf(extmsg, sizeof(extmsg),
                             "TIME_EXTENDED item %d +5s new_rem=%lds\n", it->id, rem);
                    broadcast_to_room_nolock(room_id, extmsg);
                    log_event("TIME_EXTENDED room_id=%d item_id=%d +5s", room_id, it->id);
                }

                save_items(room_id);

                char bmsg[256];
                snprintf(bmsg, sizeof(bmsg),
                         "NEW_BID item %d plate=%s price=%d leader=%s rem=%lds\n",
                         it->id, it->license_plate, it->current_price, it->leader,
                         (long)(it->end_time - time(NULL)));
                broadcast_to_room_nolock(room_id, bmsg);

                log_event("BID user=%s room_id=%d item_id=%d amount=%d",
                          username, room_id, item_id, amount);

                pthread_mutex_unlock(&rooms_mutex);
                send(client_sock, "BID_OK\n", 7, 0);
            }
        }

        // BUY_NOW
        else if (strncmp(buffer, "BUY_NOW", 7) == 0) {
            if (strlen(username) == 0) send(client_sock, "NEED_LOGIN\n", 11, 0);
            else {
                int room_id, item_id;
                if (sscanf(buffer, "BUY_NOW %d %d", &room_id, &item_id) != 2) {
                    send(client_sock, "BAD_FORMAT\n", 11, 0);
                    continue;
                }

                pthread_mutex_lock(&rooms_mutex);

                int cur = user_current_room(username);
                if (cur == 0) { pthread_mutex_unlock(&rooms_mutex); send(client_sock, "NEED_JOIN_ROOM\n", 15, 0); continue; }
                if (cur != room_id) { pthread_mutex_unlock(&rooms_mutex); send(client_sock, "NOT_IN_THIS_ROOM\n", 17, 0); continue; }

                Room *r=NULL; Item *it=NULL;
                for (int i=0;i<room_count;i++)
                    if (rooms[i].id==room_id && rooms[i].active){ r=&rooms[i]; break; }

                if(!r){ pthread_mutex_unlock(&rooms_mutex); send(client_sock,"ROOM_NOT_FOUND\n",15,0); continue; }

                for (int j=0;j<r->item_count;j++)
                    if (r->items[j].id==item_id){ it=&r->items[j]; break; }

                if(!it || it->status!=0 || !it->auction_active){
                    pthread_mutex_unlock(&rooms_mutex);
                    send(client_sock,"ITEM_NOT_FOUND_OR_SOLD\n",23,0); continue;
                }

                time_t now=time(NULL);
                if(now>=it->end_time){
                    it->auction_active=0; it->status=1;
                    save_items(room_id);
                    pthread_mutex_unlock(&rooms_mutex);
                    send(client_sock,"AUCTION_ENDED\n",14,0); continue;
                }

                it->current_price=it->buy_now_price;
                strcpy(it->leader,username);
                it->status=1; it->auction_active=0; it->end_time=now;

                save_items(room_id);
                append_history(username, room_id, it, "BUY_NOW");

                log_event("BUY_NOW user=%s room_id=%d item_id=%d plate=%s price=%d",
                          username, room_id, item_id, it->license_plate, it->current_price);

                char soldmsg[256];
                snprintf(soldmsg,sizeof(soldmsg),
                         "SOLD_BUY_NOW item %d plate=%s winner=%s price=%d\n",
                         it->id,it->license_plate,it->leader,it->current_price);
                broadcast_to_room_nolock(room_id,soldmsg);

                pthread_mutex_unlock(&rooms_mutex);
                send(client_sock,"BUY_NOW_OK\n",11,0);
            }
        }

        else {
            send(client_sock, "UNKNOWN_COMMAND\n", 16, 0);
        }
    }

    if (strlen(username) > 0) {
        pthread_mutex_lock(&rooms_mutex);
        int left = leave_room_and_getid(username);
        if (left) {
            char bmsg[128];
            snprintf(bmsg, sizeof(bmsg), "USER_LEAVE %s room %d\n", username, left);
            broadcast_to_room_nolock(left, bmsg);
            log_event("LEAVE_ROOM user=%s room_id=%d (disconnect)", username, left);
        }
        pthread_mutex_unlock(&rooms_mutex);
        logout_user(username);
    }

    remove_session(client_sock);
    close(client_sock);
    pthread_exit(NULL);
}
