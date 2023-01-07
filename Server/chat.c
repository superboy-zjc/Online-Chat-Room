#include"chat.h"
#include<sys/socket.h>
#include<mysql.h>
#include<my_global.h>
#include<string.h>
#include<stdio.h>
#include<arpa/inet.h>
#include<netinet/in.h>
int pubchat(CHAT msg, ONLINE_USER on_users[50], MYSQL* mysql)
{
	if((search_ban(mysql, msg.srcname) != 1) && (search_speak(mysql, msg.srcname) != 0))
	{
		for(int i = 0;i < 50; i ++)
		{
			if(on_users[i].socket_fd != 0 && (search_ban(mysql, on_users[i].username) != 1) && strcmp(on_users[i].username, msg.srcname) !=0)
			{
				if(-1 == send(on_users[i].socket_fd,(CHAT*) &msg, sizeof(CHAT), 0))
				{
					perror("send error:");
					printf("转发来自 %s 的群聊发送到 %s 失败!\n", msg.srcname, on_users[i].username);
					for(int i = 0;i < 50; i ++)
					{
							if(((ONLINE_USER*) on_users)[i].socket_fd != 0)
							{
								printf("username: %s fd = %d\n", ((ONLINE_USER*) on_users)[i].username, ((ONLINE_USER*) on_users)[i].socket_fd);
							}
					}
					return -1;
				}
			}
		}
	}
	return 0;
}


int prichat(CHAT msg, ONLINE_USER on_users[50], MYSQL* mysql)
{
	if((search_ban(mysql, msg.srcname) != 1) && (search_speak(mysql, msg.srcname) != 0))
	{
		for(int i = 0;i < 50; i ++)
		{
			if((search_ban(mysql, on_users[i].username) != 1) && strcmp(on_users[i].username, msg.dstname) ==0)
			{
				if(-1 == send(on_users[i].socket_fd,(CHAT*) &msg, sizeof(CHAT), 0))
				{
					perror("send error:");
					printf("转发来自 %s 的传输包发送到 %s 失败!\n", msg.srcname, on_users[i].username);
					for(int i = 0;i < 50; i ++)
					{
							if(((ONLINE_USER*) on_users)[i].socket_fd != 0)
							{
								printf("username: %s fd = %d\n", ((ONLINE_USER*) on_users)[i].username, ((ONLINE_USER*) on_users)[i].socket_fd);
							}
					}
					return -1;
				}
			}
		}
	}
	return 0;
}

int ban(CHAT msg, MYSQL* mysql, int socket)
{
	char tmp_str[100];
	ACK ack;
	ack.flag = 0;
	sprintf(tmp_str, "UPDATE user SET ban='%d' WHERE username = '%s'",msg.flag, msg.dstname);
	if(mysql_query(mysql, tmp_str))
	{
		printf("error: %s", mysql_error(mysql));
		ack.flag = -1;
		strcpy(ack.reason, "设置失败");
	}
	if(-1 == send(socket, &ack, sizeof(ACK), 0))
	{
		perror("send error");
		return -1;
	}
	return 0;
}

int speak(CHAT msg, MYSQL* mysql, int socket)
{
	char tmp_str[100];
	ACK ack;
	ack.flag = 0;
	sprintf(tmp_str, "UPDATE user SET speak='%d' WHERE username = '%s'",msg.flag, msg.dstname);
	if(mysql_query(mysql, tmp_str))
	{
		printf("error: %s", mysql_error(mysql));
		ack.flag = -1;
		strcpy(ack.reason, "设置失败");
	}
	if(-1 == send(socket, &ack, sizeof(ACK), 0))
	{
		perror("send error");
		return -1;
	}
	return 0;
}
int search(ONLINE_USER on_users[50], int socket)
{
	SEARCH_USER users;
	memset(&users, 0, sizeof(SEARCH_USER));
	users.type = SEARCHTYPE;
	for(int i = 0; i < 50; i ++)
	{
		if(on_users[i].socket_fd != 0)
		{
			strcpy(users.username[users.count], on_users[i].username);
			printf("%s\n", users.username[users.count]);
			users.count ++;
		}
	}
	for(int i = 0; i < users.count; i ++)
	{
		printf("%s\n", users.username[i]);
	}
	printf("%d\n", users.type);
	if(-1 == send(socket, &users, sizeof(SEARCH_USER), 0))
	{
		perror("send error");
		return -1;
	}
	return 0;
}
int agreefile(CHAT msg, ONLINE_USER on_users[50], MYSQL* mysql, int t_socket)
{
	//将同意传输文件数据包转发给请求人
	if((search_ban(mysql, msg.srcname) != 1))
	{
		for(int i = 0;i < 50; i ++)
		{
			if(on_users[i].socket_fd != 0)
			{
				if((search_ban(mysql, on_users[i].username) != 1) && (search_speak(mysql, on_users[i].username)) != 0 && strcmp(on_users[i].username, msg.dstname) == 0)
				{/*
					struct sockaddr_in remote_addr;
					socklen_t len;
					printf("agree fun socket: %d\n", socket);
				
					if(-1 == getpeername(socket, (struct sockaddr*) &remote_addr, &len))
					{
						perror("getpeername error");
						return -1;
					}
					perror("getpeername ");
					inet_ntop(AF_INET, &remote_addr.sin_addr, msg.content, sizeof(msg.content));
					msg.length = strlen(msg.content);
					printf("接收人同意文件传输请求，将接收人%s的IP地址：%s 发送给 请求人%s \n", msg.srcname, msg.content, msg.dstname);*/

					
					
						struct sockaddr_in remote_addr;
						socklen_t len = sizeof(struct sockaddr_in);
						if(-1 == getpeername(t_socket, (struct sockaddr*) &remote_addr, &len))
						{
							perror("getpeername error");
							return -1;
						}
						inet_ntop(AF_INET, &remote_addr.sin_addr, msg.content, sizeof(msg.content));
						msg.length = strlen(msg.content);
						printf("%s: %d \n", msg.content, msg.length);
					

					if(-1 == send(on_users[i].socket_fd,(CHAT*) &msg, sizeof(CHAT), 0))
					{
						perror("send error:");
						printf("转发来自 %s 的同意传输信息发送到 %s 失败!\n", msg.srcname, on_users[i].username);
						for(int i = 0;i < 50; i ++)
						{
							if(((ONLINE_USER*) on_users)[i].socket_fd != 0)
							{
								printf("username: %s fd = %d\n", ((ONLINE_USER*) on_users)[i].username, ((ONLINE_USER*) on_users)[i].socket_fd);
							}
						}
						return -1;
					}
					return 0;
				}

			}
		}
	}
	return -1;
}
int requesend(CHAT msg, ONLINE_USER on_users[50], MYSQL* mysql, int socket)
{
	printf("进入请求函数\n");
	int i;
	//被ban或者被禁言都不可以请求传输文件
	if((search_ban(mysql, msg.srcname) != 1) && (search_speak(mysql, msg.srcname) != 0))
	{
		printf("查询发送人权限完毕\n");
		//遍历在线用户表，如果用户存在并且没有被ban则可以接受请求
		for(i = 0;i < 50; i ++)
		{
			int banflag;
			if(on_users[i].socket_fd != 0 )
			{
				if(((banflag = search_ban(mysql, on_users[i].username)) != 1) && strcmp(on_users[i].username, msg.dstname) == 0)
				{
					printf("查询到符合接受的人\n");
					if(-1 == send(on_users[i].socket_fd,(CHAT*) &msg, sizeof(CHAT), 0))
					{
						perror("send error:");
						printf("转发来自 %s 的传输包发送到 %s 失败!\n", msg.srcname, on_users[i].username);
						for(int i = 0;i < 50; i ++)
						{
							if(((ONLINE_USER*) on_users)[i].socket_fd != 0)
							{
								printf("username: %s fd = %d\n", ((ONLINE_USER*) on_users)[i].username, ((ONLINE_USER*) on_users)[i].socket_fd);
							}
						}
						return -1;
					}
					printf("发送完毕,请求人%s, 接收人%s\n", msg.srcname, msg.dstname);
					return 0;
				}
				else
				{
					printf("username %s :ban:%d 不符合\n", on_users[i].username, banflag);
				}
			}
		}
	}
	if(i >= 50)
	{
		CHAT req;
		req.type = REFUSE;
		printf("没找到此人！\n");
		if(-1 == send(socket, &req, sizeof(CHAT), 0))
		{
			perror("send error");
			return -1;
		}
	}
	return 0;
}
