#ifndef ROOMS_H
#define ROOMS_H

int  create_room(char *name, char *admin);
int  close_room(int id, char *admin);
int  open_room(int id, char *admin);
void list_rooms(int sock);

int  user_current_room(const char *username);
int  join_room(char *username, int id);
int  leave_room_and_getid(char *username);

#endif
