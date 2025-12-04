#include "../include/common.h"
#include "../include/users.h"
#include "../include/rooms.h"
#include "../include/persistence.h"
#include "../include/auction.h"
#include "../include/handler.h"
#include "../include/log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

User users[MAX_USERS];
int user_count = 0;

Room rooms[MAX_ROOMS];
int room_count = 0;
int next_room_id = 1;

ClientSession sessions[MAX_CLIENTS];

pthread_mutex_t users_mutex    = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t rooms_mutex    = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t sessions_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t log_mutex      = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t history_mutex  = PTHREAD_MUTEX_INITIALIZER;

int main() {
    int server_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    pthread_t tid, auction_tid;

    memset(sessions, 0, sizeof(sessions));
    for (int i=0;i<MAX_CLIENTS;i++) sessions[i].sock = -1;

    load_users();

    pthread_mutex_lock(&rooms_mutex);
    load_rooms();
    for (int i=0;i<room_count;i++) load_items(rooms[i].id);
    pthread_mutex_unlock(&rooms_mutex);

    pthread_create(&auction_tid, NULL, auction_manager, NULL);
    pthread_detach(auction_tid);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); exit(1); }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    if (bind(server_fd,(struct sockaddr*)&server_addr,sizeof(server_addr))<0){
        perror("bind"); exit(1);
    }
    if (listen(server_fd, 5)<0){ perror("listen"); exit(1); }

    printf("Server listening on port %d...\n", PORT);
    log_event("SERVER_START port=%d", PORT);

    while (1) {
        int *pclient = malloc(sizeof(int));
        if (!pclient) continue;

        *pclient = accept(server_fd,(struct sockaddr*)&client_addr,&client_len);
        if (*pclient < 0){ free(pclient); continue; }

        pthread_create(&tid, NULL, handle_client, pclient);
        pthread_detach(tid);
    }

    close(server_fd);
    return 0;
}
