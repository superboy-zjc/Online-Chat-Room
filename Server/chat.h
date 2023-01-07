#ifndef CHAT_H
#define CHAT_H
#include"protocol.h"
#include<mysql.h>
#include<my_global.h>
int search_speak(MYSQL* mysql, char username[20]);
int search_ban(MYSQL* mysql, char username[20]);
int search_root(MYSQL* mysql, char username[20]);
int pubchat(CHAT msg, ONLINE_USER on_users[50], MYSQL* mysql);
int prichat(CHAT msg, ONLINE_USER on_users[50], MYSQL* mysql);
int ban(CHAT msg, MYSQL* mysql, int socket);
int speak(CHAT msg, MYSQL* mysql, int socket);
int search(ONLINE_USER on_users[50], int socket);
int agreefile(CHAT msg, ONLINE_USER on_users[50], MYSQL* mysql, int socket);
int requesend(CHAT msg, ONLINE_USER on_users[50], MYSQL* mysql, int socket);
#endif
