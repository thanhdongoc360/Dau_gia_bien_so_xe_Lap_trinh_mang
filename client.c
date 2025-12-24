#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 12345
#define BUF_SIZE 2048

static int sockfd;
static pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;

// Thread nhận data từ server (broadcast / reply)
void *recv_loop(void *arg) {
    (void)arg;
    char buf[BUF_SIZE];

    while (1) {
        int n = recv(sockfd, buf, BUF_SIZE - 1, 0);
        if (n <= 0) break;

        buf[n] = '\0';

        pthread_mutex_lock(&print_mutex);
        printf("Server: %s", buf);
        if (buf[n-1] != '\n') printf("\n");
        fflush(stdout);
        pthread_mutex_unlock(&print_mutex);
    }

    pthread_mutex_lock(&print_mutex);
    printf("\n[Disconnected]\n");
    pthread_mutex_unlock(&print_mutex);
    exit(0);
    return NULL;
}

int main() {
    struct sockaddr_in serv_addr;
    char input[BUF_SIZE];
    char server_ip[64];

int main(int argc, char *argv[]) {
    struct sockaddr_in serv_addr;
    char input[BUF_SIZE];
    char *server_ip;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <server_ip>\n", argv[0]);
        fprintf(stderr, "Ví dụ: %s 192.168.1.10\n", argv[0]);
        return 1;
    }
    server_ip = argv[1];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) { perror("socket"); return 1; }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = inet_addr(server_ip);

    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect");
        return 1;
    }

    printf("Connected to server!\n");
    printf("Type commands...\n\n");

    pthread_t tid;
    pthread_create(&tid, NULL, recv_loop, NULL);
    pthread_detach(tid);

    while (1) {
        pthread_mutex_lock(&print_mutex);
        printf("You: ");
        fflush(stdout);
        pthread_mutex_unlock(&print_mutex);

        if (!fgets(input, sizeof(input), stdin)) break;
        input[strcspn(input, "\n")] = 0;
        if (strlen(input) == 0) continue;

        send(sockfd, input, strlen(input), 0);
    }

    close(sockfd);
    return 0;
}
