#ifndef COMMON_H
#define COMMON_H
#include <sys/socket.h>
#include <unistd.h>

#include <pthread.h>
#include <time.h>

#define PORT 12345
#define BUF_SIZE 1024
#define MAX_CLIENTS 100
#define MAX_USERS 100
#define MAX_ROOMS 50
#define MAX_ROOM_USERS 50
#define MAX_ITEMS 100

#define AUCTION_DURATION 60   // đổi 180 nếu muốn test lâu hơn

typedef struct {
    char username[50];
    char password[50];
    char role[10];        // "admin" / "user"
    char display_name[50];
    int  online;
} User;

typedef struct {
    int  id;
    char license_plate[20];
    int  current_price;
    int  step_price;
    int  buy_now_price;
    char leader[50];
    int  status;          // 0 ACTIVE, 1 SOLD

    time_t end_time;
    int auction_active;
    int warned30;
} Item;

typedef struct {
    int  id;
    char name[50];
    int  active;                  // 1 OPEN, 0 CLOSED
    char admin[50];
    char members[MAX_ROOM_USERS][50];
    int  member_count;
    Item items[MAX_ITEMS];
    int  item_count;
} Room;

typedef struct {
    int sock;
    char username[50];
    char role[10];
    int active;
} ClientSession;

// globals
extern User users[MAX_USERS];
extern int user_count;

extern Room rooms[MAX_ROOMS];
extern int room_count;
extern int next_room_id;

extern ClientSession sessions[MAX_CLIENTS];

extern pthread_mutex_t users_mutex;
extern pthread_mutex_t rooms_mutex;
extern pthread_mutex_t sessions_mutex;
extern pthread_mutex_t log_mutex;
extern pthread_mutex_t history_mutex;

#endif
