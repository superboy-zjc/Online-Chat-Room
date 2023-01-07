#ifndef CHAT_H
#define CHAT_H
int ui_chat(int flag);
int pubchat(int socket);
int prichat(int socket);
int ban(int socket);
int speak(int socket);
int search(int socket);
int recvfile(void);
int sendfile(int socket);
#endif
