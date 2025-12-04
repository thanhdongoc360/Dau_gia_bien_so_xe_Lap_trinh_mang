#ifndef USERS_H
#define USERS_H

void load_users();
void save_users();
int  register_user(char *username, char *password);
int  login_user(char *username, char *password, char *role_out);
void logout_user(char *username);

int  change_password(const char *username, const char *oldp, const char *newp);
int  change_display_name(const char *username, const char *newname);

#endif
