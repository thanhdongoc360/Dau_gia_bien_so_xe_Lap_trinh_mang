#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <ctype.h>

#define PORT 12345
#define BUF_SIZE 4096

static int sockfd;

// ====== UI/STATE ======
typedef struct {
    int logged_in;
    char username[50];
    char role[10];          // "admin" / "user"
    int current_room;       // 0 nếu chưa vào room
    int pending_join_room;  // lưu room_id vừa JOIN để nhận JOIN_OK cập nhật
} ClientState;

static ClientState st;
static pthread_mutex_t state_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;

// ---- helper safe print ----
static void safe_printf(const char *fmt, ...) {
    pthread_mutex_lock(&print_mutex);
    va_list ap; va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    fflush(stdout);
    pthread_mutex_unlock(&print_mutex);
}

// ---- trim newline ----
static void trim_nl(char *s) {
    s[strcspn(s, "\r\n")] = 0;
}

// ---- check if line only digits (menu choice) ----
static int is_digits_only(const char *s) {
    if (!s || !*s) return 0;
    for (const char *p=s; *p; p++) if (!isdigit((unsigned char)*p)) return 0;
    return 1;
}

// ---- read line ----
static void read_line(char *buf, size_t n) {
    if (!fgets(buf, n, stdin)) {
        buf[0]='\0';
        return;
    }
    trim_nl(buf);
}

// ---- state setters/getters ----
static void set_logged_in(const char *user, const char *role) {
    pthread_mutex_lock(&state_mutex);
    st.logged_in = 1;
    strncpy(st.username, user, sizeof(st.username)-1);
    strncpy(st.role, role, sizeof(st.role)-1);
    st.current_room = 0;
    st.pending_join_room = 0;
    pthread_mutex_unlock(&state_mutex);
}

static void set_logged_out() {
    pthread_mutex_lock(&state_mutex);
    st.logged_in = 0;
    st.username[0] = '\0';
    st.role[0] = '\0';
    st.current_room = 0;
    st.pending_join_room = 0;
    pthread_mutex_unlock(&state_mutex);
}

static void set_join_room_pending(int rid) {
    pthread_mutex_lock(&state_mutex);
    st.pending_join_room = rid;
    pthread_mutex_unlock(&state_mutex);
}

static void confirm_join_room() {
    pthread_mutex_lock(&state_mutex);
    if (st.pending_join_room > 0) {
        st.current_room = st.pending_join_room;
        st.pending_join_room = 0;
    }
    pthread_mutex_unlock(&state_mutex);
}

static void set_left_room() {
    pthread_mutex_lock(&state_mutex);
    st.current_room = 0;
    st.pending_join_room = 0;
    pthread_mutex_unlock(&state_mutex);
}

static void get_state_snapshot(ClientState *out) {
    pthread_mutex_lock(&state_mutex);
    *out = st;
    pthread_mutex_unlock(&state_mutex);
}

// ====== RECV THREAD ======
void *recv_loop(void *arg) {
    (void)arg;
    char buf[BUF_SIZE];

    while (1) {
        int n = recv(sockfd, buf, BUF_SIZE - 1, 0);
        if (n <= 0) break;
        buf[n] = '\0';

        // ---- update local state based on server replies ----
        // LOGIN_OK role
        if (strncmp(buf, "LOGIN_OK", 8) == 0) {
            char role[10];
            if (sscanf(buf, "LOGIN_OK %9s", role) == 1) {
                // username đã lưu ở main trước khi gửi LOGIN
                pthread_mutex_lock(&state_mutex);
                st.logged_in = 1;
                strncpy(st.role, role, sizeof(st.role)-1);
                st.current_room = 0;
                st.pending_join_room = 0;
                pthread_mutex_unlock(&state_mutex);
            }
        }
        else if (strncmp(buf, "LOGOUT_OK", 9) == 0) {
            set_logged_out();
        }
        else if (strncmp(buf, "JOIN_OK", 7) == 0) {
            confirm_join_room();
        }
        else if (strncmp(buf, "LEAVE_OK", 8) == 0) {
            set_left_room();
        }

        // ---- print message ----
        pthread_mutex_lock(&print_mutex);
        printf("\nServer: %s", buf);
        if (buf[n-1] != '\n') printf("\n");
        fflush(stdout);
        pthread_mutex_unlock(&print_mutex);
    }

    safe_printf("\n[Disconnected]\n");
    exit(0);
    return NULL;
}

// ====== SEND COMMAND ======
static void send_cmd(const char *cmd) {
    if (!cmd || !*cmd) return;
    send(sockfd, cmd, strlen(cmd), 0);
}

// ====== MENUS ======
static void show_menu(const ClientState *s) {
    safe_printf("\n========== AUCTION CLIENT ==========\n");

    if (!s->logged_in) {
        safe_printf("Chưa login.\n");
        safe_printf("1. REGISTER\n");
        safe_printf("2. LOGIN\n");
        safe_printf("9. Gõ lệnh thô (raw command)\n");
        safe_printf("0. EXIT\n");
        return;
    }

    safe_printf("User: %s | Role: %s | Room: %d\n",
                s->username, s->role, s->current_room);

    if (s->current_room == 0) {
        safe_printf("1. LIST_ROOMS\n");
        safe_printf("2. JOIN_ROOM\n");
        safe_printf("3. HISTORY\n");
        safe_printf("4. CHANGE_PASS\n");
        safe_printf("5. CHANGE_NAME\n");
        if (strcmp(s->role, "admin") == 0) {
            safe_printf("6. CREATE_ROOM (admin)\n");
            safe_printf("7. OPEN_ROOM (admin)\n");
            safe_printf("8. CLOSE_ROOM (admin)\n");
        }
        safe_printf("9. Gõ lệnh thô (raw command)\n");
        safe_printf("0. LOGOUT\n");
        return;
    }

    // đang ở trong room
    safe_printf("1. LIST_ITEMS (room hiện tại)\n");
    safe_printf("2. BID\n");
    safe_printf("3. BUY_NOW\n");
    if (strcmp(s->role, "admin") == 0) {
        safe_printf("4. ADD_ITEM (admin)\n");
        safe_printf("5. REMOVE_ITEM (admin)\n");
        safe_printf("6. OPEN_ROOM (admin)\n");
        safe_printf("7. CLOSE_ROOM (admin)\n");
        safe_printf("8. LEAVE_ROOM\n");
    } else {
        safe_printf("4. LEAVE_ROOM\n");
    }
    safe_printf("9. Gõ lệnh thô (raw command)\n");
    safe_printf("0. LOGOUT\n");
}

// ====== MAIN ======
int main() {
    struct sockaddr_in serv_addr;
    char input[BUF_SIZE];

    memset(&st, 0, sizeof(st));

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) { perror("socket"); return 1; }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect"); return 1;
    }

    safe_printf("Connected to server!\n");

    pthread_t tid;
    pthread_create(&tid, NULL, recv_loop, NULL);
    pthread_detach(tid);

    while (1) {
        ClientState snap;
        get_state_snapshot(&snap);
        show_menu(&snap);

        safe_printf("You (nhập số/menu hoặc lệnh): ");
        read_line(input, sizeof(input));
        if (strlen(input) == 0) continue;

        // ===== raw command mode =====
        if (!is_digits_only(input) || strcmp(input, "9")==0) {
            if (strcmp(input, "9")==0) {
                safe_printf("Raw> ");
                read_line(input, sizeof(input));
                if (strlen(input)==0) continue;
            }

            // nếu là LOGIN u p thì lưu username local để show menu
            if (strncmp(input, "LOGIN ", 6)==0) {
                char u[50], p[50];
                if (sscanf(input, "LOGIN %49s %49s", u, p) == 2) {
                    pthread_mutex_lock(&state_mutex);
                    strncpy(st.username, u, sizeof(st.username)-1);
                    pthread_mutex_unlock(&state_mutex);
                }
            }
            // JOIN_ROOM id => set pending
            if (strncmp(input, "JOIN_ROOM ", 10)==0) {
                int rid;
                if (sscanf(input, "JOIN_ROOM %d", &rid)==1)
                    set_join_room_pending(rid);
            }

            send_cmd(input);
            continue;
        }

        int choice = atoi(input);

        // ===== not logged in =====
        if (!snap.logged_in) {
            if (choice == 0) { safe_printf("Bye!\n"); break; }

            if (choice == 1) {
                char u[50], p[50];
                safe_printf("Username: "); read_line(u, sizeof(u));
                safe_printf("Password: "); read_line(p, sizeof(p));

                char cmd[128];
                snprintf(cmd, sizeof(cmd), "REGISTER %s %s", u, p);
                send_cmd(cmd);
            }
            else if (choice == 2) {
                char u[50], p[50];
                safe_printf("Username: "); read_line(u, sizeof(u));
                safe_printf("Password: "); read_line(p, sizeof(p));

                pthread_mutex_lock(&state_mutex);
                strncpy(st.username, u, sizeof(st.username)-1);
                pthread_mutex_unlock(&state_mutex);

                char cmd[128];
                snprintf(cmd, sizeof(cmd), "LOGIN %s %s", u, p);
                send_cmd(cmd);
            }
            continue;
        }

        // ===== logged in but not in room =====
        if (snap.current_room == 0) {
            if (choice == 0) { send_cmd("LOGOUT"); continue; }

            if (choice == 1) send_cmd("LIST_ROOMS");
            else if (choice == 2) {
                char rid_s[20];
                safe_printf("Room ID: "); read_line(rid_s, sizeof(rid_s));
                int rid = atoi(rid_s);
                set_join_room_pending(rid);

                char cmd[64];
                snprintf(cmd, sizeof(cmd), "JOIN_ROOM %d", rid);
                send_cmd(cmd);
            }
            else if (choice == 3) send_cmd("HISTORY");
            else if (choice == 4) {
                char oldp[50], newp[50];
                safe_printf("Old pass: "); read_line(oldp, sizeof(oldp));
                safe_printf("New pass: "); read_line(newp, sizeof(newp));

                char cmd[128];
                snprintf(cmd, sizeof(cmd), "CHANGE_PASS %s %s", oldp, newp);
                send_cmd(cmd);
            }
            else if (choice == 5) {
                char nn[50];
                safe_printf("New display name: "); read_line(nn, sizeof(nn));

                char cmd[128];
                snprintf(cmd, sizeof(cmd), "CHANGE_NAME %s", nn);
                send_cmd(cmd);
            }
            else if (strcmp(snap.role, "admin")==0) {
                if (choice == 6) {
                    char rn[50];
                    safe_printf("Room name: "); read_line(rn, sizeof(rn));
                    char cmd[128];
                    snprintf(cmd, sizeof(cmd), "CREATE_ROOM %s", rn);
                    send_cmd(cmd);
                }
                else if (choice == 7) {
                    char rid_s[20];
                    safe_printf("Room ID: "); read_line(rid_s, sizeof(rid_s));
                    char cmd[64];
                    snprintf(cmd, sizeof(cmd), "OPEN_ROOM %d", atoi(rid_s));
                    send_cmd(cmd);
                }
                else if (choice == 8) {
                    char rid_s[20];
                    safe_printf("Room ID: "); read_line(rid_s, sizeof(rid_s));
                    char cmd[64];
                    snprintf(cmd, sizeof(cmd), "CLOSE_ROOM %d", atoi(rid_s));
                    send_cmd(cmd);
                }
            }
            continue;
        }

        // ===== in room =====
        if (choice == 0) { send_cmd("LOGOUT"); continue; }

        if (choice == 1) {
            char cmd[64];
            snprintf(cmd, sizeof(cmd), "LIST_ITEMS %d", snap.current_room);
            send_cmd(cmd);
        }
        else if (choice == 2) {
            char item_s[20], price_s[20];
            safe_printf("Item ID: "); read_line(item_s, sizeof(item_s));
            safe_printf("Bid price: "); read_line(price_s, sizeof(price_s));

            char cmd[128];
            snprintf(cmd, sizeof(cmd), "BID %d %d %d",
                     snap.current_room, atoi(item_s), atoi(price_s));
            send_cmd(cmd);
        }
        else if (choice == 3) {
            char item_s[20];
            safe_printf("Item ID: "); read_line(item_s, sizeof(item_s));

            char cmd[128];
            snprintf(cmd, sizeof(cmd), "BUY_NOW %d %d",
                     snap.current_room, atoi(item_s));
            send_cmd(cmd);
        }

        if (strcmp(snap.role, "admin")==0) {
            if (choice == 4) {
                char plate[20], start_s[20], step_s[20], buy_s[20];
                safe_printf("Plate: "); read_line(plate, sizeof(plate));
                safe_printf("Start price: "); read_line(start_s, sizeof(start_s));
                safe_printf("Step price: "); read_line(step_s, sizeof(step_s));
                safe_printf("BuyNow price: "); read_line(buy_s, sizeof(buy_s));

                char cmd[256];
                snprintf(cmd, sizeof(cmd), "ADD_ITEM %d %s %d %d %d",
                         snap.current_room, plate,
                         atoi(start_s), atoi(step_s), atoi(buy_s));
                send_cmd(cmd);
            }
            else if (choice == 5) {
                char item_s[20];
                safe_printf("Item ID: "); read_line(item_s, sizeof(item_s));

                char cmd[128];
                snprintf(cmd, sizeof(cmd), "REMOVE_ITEM %d %d",
                         snap.current_room, atoi(item_s));
                send_cmd(cmd);
            }
            else if (choice == 6) {
                char cmd[64];
                snprintf(cmd, sizeof(cmd), "OPEN_ROOM %d", snap.current_room);
                send_cmd(cmd);
            }
            else if (choice == 7) {
                char cmd[64];
                snprintf(cmd, sizeof(cmd), "CLOSE_ROOM %d", snap.current_room);
                send_cmd(cmd);
            }
            else if (choice == 8) {
                send_cmd("LEAVE_ROOM");
            }
        } else {
            if (choice == 4) send_cmd("LEAVE_ROOM");
        }
    }

    close(sockfd);
    return 0;
}
