#include "../include/common.h"
#include "../include/users.h"
#include "../include/log.h"
#include <stdio.h>
#include <string.h>

void load_users() {
    FILE *f = fopen("users.txt", "r");
    if (!f) return;

    user_count = 0;
    char line[256];

    while (fgets(line, sizeof(line), f) && user_count < MAX_USERS) {
        User u;
        u.online = 0;
        int c = sscanf(line, "%49s %49s %9s %49s",
                       u.username, u.password, u.role, u.display_name);
        if (c < 3) continue;
        if (c == 3) strcpy(u.display_name, u.username);
        users[user_count++] = u;
    }
    fclose(f);
}

void save_users() {
    FILE *f = fopen("users.txt", "w");
    if (!f) return;

    for (int i = 0; i < user_count; i++) {
        fprintf(f, "%s %s %s %s\n",
                users[i].username,
                users[i].password,
                users[i].role,
                users[i].display_name);
    }
    fclose(f);
}

int register_user(char *username, char *password) {
    pthread_mutex_lock(&users_mutex);

    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0) {
            pthread_mutex_unlock(&users_mutex);
            return 0;
        }
    }
    if (user_count >= MAX_USERS) {
        pthread_mutex_unlock(&users_mutex);
        return 0;
    }

    strcpy(users[user_count].username, username);
    strcpy(users[user_count].password, password);
    strcpy(users[user_count].role, "user");
    strcpy(users[user_count].display_name, username);
    users[user_count].online = 0;
    user_count++;

    save_users();
    pthread_mutex_unlock(&users_mutex);

    log_event("REGISTER user=%s", username);
    return 1;
}

int login_user(char *username, char *password, char *role_out) {
    pthread_mutex_lock(&users_mutex);

    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0 &&
            strcmp(users[i].password, password) == 0)
        {
            if (users[i].online) {
                pthread_mutex_unlock(&users_mutex);
                return -1;
            }
            users[i].online = 1;
            strcpy(role_out, users[i].role);
            pthread_mutex_unlock(&users_mutex);

            log_event("LOGIN user=%s role=%s", username, role_out);
            return 1;
        }
    }
    pthread_mutex_unlock(&users_mutex);
    return 0;
}

void logout_user(char *username) {
    pthread_mutex_lock(&users_mutex);
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0) {
            users[i].online = 0;
            break;
        }
    }
    pthread_mutex_unlock(&users_mutex);
    log_event("LOGOUT user=%s", username);
}

int change_password(const char *username, const char *oldp, const char *newp) {
    pthread_mutex_lock(&users_mutex);
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0) {
            if (strcmp(users[i].password, oldp) != 0) {
                pthread_mutex_unlock(&users_mutex);
                return 0;
            }
            strcpy(users[i].password, newp);
            save_users();
            pthread_mutex_unlock(&users_mutex);
            log_event("CHANGE_PASS user=%s", username);
            return 1;
        }
    }
    pthread_mutex_unlock(&users_mutex);
    return 0;
}

int change_display_name(const char *username, const char *newname) {
    pthread_mutex_lock(&users_mutex);
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0) {
            strcpy(users[i].display_name, newname);
            save_users();
            pthread_mutex_unlock(&users_mutex);
            log_event("CHANGE_NAME user=%s new=%s", username, newname);
            return 1;
        }
    }
    pthread_mutex_unlock(&users_mutex);
    return 0;
}

int promote_user_to_admin(const char *username, const char *admin) {
    pthread_mutex_lock(&users_mutex);
    
    // Find user to promote
    int target_idx = -1;
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0) {
            target_idx = i;
            break;
        }
    }
    
    if (target_idx == -1) {
        pthread_mutex_unlock(&users_mutex);
        return 0;  // User not found
    }
    
    // Check if already admin
    if (strcmp(users[target_idx].role, "admin") == 0) {
        pthread_mutex_unlock(&users_mutex);
        return -1;  // Already admin
    }
    
    // Promote
    strcpy(users[target_idx].role, "admin");
    save_users();
    pthread_mutex_unlock(&users_mutex);
    
    log_event("PROMOTE_TO_ADMIN user=%s promoted_by=%s", username, admin);
    return 1;  // Success
}
