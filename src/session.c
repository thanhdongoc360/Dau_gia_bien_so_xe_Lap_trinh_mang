#include "../include/common.h"
#include "../include/session.h"
#include "../include/rooms.h"
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

void update_session_login(int sock, const char *username, const char *role) {
    pthread_mutex_lock(&sessions_mutex);
    int idx = -1;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (sessions[i].active && sessions[i].sock == sock) { idx = i; break; }
        if (!sessions[i].active && idx == -1) idx = i;
    }
    if (idx != -1) {
        sessions[idx].sock = sock;
        sessions[idx].active = 1;
        strncpy(sessions[idx].username, username, sizeof(sessions[idx].username)-1);
        strncpy(sessions[idx].role, role, sizeof(sessions[idx].role)-1);
        sessions[idx].username[sizeof(sessions[idx].username)-1] = '\0';
        sessions[idx].role[sizeof(sessions[idx].role)-1] = '\0';
    }
    pthread_mutex_unlock(&sessions_mutex);
}

void remove_session(int sock) {
    pthread_mutex_lock(&sessions_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (sessions[i].active && sessions[i].sock == sock) {
            sessions[i].active = 0;
            sessions[i].username[0] = '\0';
            sessions[i].role[0] = '\0';
            sessions[i].sock = -1;
            break;
        }
    }
    pthread_mutex_unlock(&sessions_mutex);
}

void broadcast_to_room_nolock(int room_id, const char *msg) {
    pthread_mutex_lock(&sessions_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!sessions[i].active) continue;
        if (sessions[i].username[0] == '\0') continue;

        int cur = user_current_room(sessions[i].username);
        if (cur == room_id) {
            send(sessions[i].sock, msg, strlen(msg), 0);
        }
    }
    pthread_mutex_unlock(&sessions_mutex);
}
