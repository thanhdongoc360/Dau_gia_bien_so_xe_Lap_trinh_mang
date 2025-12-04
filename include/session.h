#ifndef SESSION_H
#define SESSION_H
void update_session_login(int sock, const char *username, const char *role);
void remove_session(int sock);
void broadcast_to_room_nolock(int room_id, const char *msg);
#endif
