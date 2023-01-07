#include"protocol.h"
#ifndef LOGIN_H
#define LOGIN_H
ACK client_register(USER user, MYSQL* mysql);
ACK client_login(USER user, MYSQL* mysql, int shmid, int socket);//处理用户的登录请求
ACK client_change(USER user, MYSQL* mysql, int socket);//修改密码模块比较特殊，模块内需要
#endif
