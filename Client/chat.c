#include "protocol.h"
#include<stdio.h>
#include<string.h>
#include<pthread.h>
#include<sys/socket.h>
#include<stdlib.h>
#include<unistd.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/stat.h>
#include<fcntl.h>
extern int interrupt;
extern pthread_spinlock_t lock;
CHAT requefilemsg;
int ui_chat(int flag)
{
	int result = 0;
	if(flag == ROOT_FINLOGIN)
	{
		printf("请输入功能选项:1、群聊　2、私聊 3、文件传输 4、踢人 5、禁言 8、显示在线人数 9、注销 0、退出\n");
		scanf("%d", &result);
	}
	else
	{
		printf("请输入功能选项:1、群聊　2、私聊 3、文件传输 8、显示在线人数 9、注销 0、退出\n");
		scanf("%d", &result);
		getchar();
	}
	return result;
}
int recvfile(void)
{
	int fd;
	int tcp_socket = socket(AF_INET, SOCK_STREAM,0);//创建一个TCP/IPV4套接字
	if(tcp_socket == -1)	
	{
		perror("socket error");
		return -1;
	}
	struct sockaddr_in local_addr;
	local_addr.sin_family = AF_INET;
	local_addr.sin_port = htons(7777);
	local_addr.sin_addr.s_addr = inet_addr(LOCAL_IP);
	if(-1 == bind(tcp_socket, (struct sockaddr*) &local_addr, sizeof(struct sockaddr_in)))//绑定服务器IP和端口
	{
		perror("bind error");
		close(tcp_socket);
		return -1;
	}
	if(-1 == listen(tcp_socket, 10))//设置服务器监听数量
	{
		perror("listen error");
		close(tcp_socket);
		return -1;
	}
	int acc_socket;
	struct sockaddr_in remote_addr;
	socklen_t len = sizeof(struct sockaddr_in);
	printf("等待对方的连接...\n");
	if(-1 == (acc_socket = accept(tcp_socket, (struct sockaddr*) &remote_addr, &len)))//等待用户连接
	{
		perror("accept error");
		close(tcp_socket);
		return -1;
	}
	printf("连接成功！\n");
	while(1)
	{
		CHAT msg;
		ssize_t length;
		length = recv(acc_socket, &msg, sizeof(CHAT), 0);
		if(length == -1)
		{
			perror("recv error");
			close(acc_socket);
			close(tcp_socket);
			return -1;
		}
		else if(length == 0)
		{
			printf("传输完毕，对方已关闭连接\n");
			close(tcp_socket);
			close(acc_socket);
			return 0;
		}
		if(msg.type == FILENAME)
		{
			if((fd = open(msg.srcname, O_CREAT | O_WRONLY | O_APPEND , 0666)) == -1)
			{
				perror("open error");
				close(acc_socket);
				close(tcp_socket);
				return -1;
			}
		}
		else if(msg.type == FILETYPE)
		{
			if(msg.length == 0)
			{
				printf("传输完毕\n");
				close(acc_socket);
				close(tcp_socket);
				close(fd);
				return 0;
			}
			if(write(fd, msg.content, msg.length) < 0)
			{
				perror("write error");
				close(acc_socket);
				close(tcp_socket);
				close(fd);
				return -1;
			}
		}
	}
}
int sendfile(int t_socket)
{
	//××××××××××××××××封装请求包××××××××××××××××//
	CHAT msg;
	int fd;
	msg.type = FILEQUETYPE;
	printf("请输入想要发送的用户:");
	scanf("%s", msg.dstname);
	printf("您要请求发送文件的用户是%s\n", msg.dstname);
	//***************************************//
	//****************发送请求传输文件包**************//
	if(-1 == send(t_socket, &msg, sizeof(CHAT), 0))
	{
		perror("send error");
		return -1;
	}
	memset(&msg, 0, sizeof(CHAT));
	//while(msg.type == )
	if(-1 == recv(t_socket, &msg, sizeof(CHAT), 0))
	{
		perror("recv error");
		return -1;
	}
//	}
	if(msg.type == REFUSE)
	{
		printf("对方拒绝了你的请求\n");
	}
	else if(msg.type == AGREE)
	{
		printf("对方同意了, IP 是 %s\n", msg.content);
		int tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
		if(tcp_socket == -1)
		{
			perror("socket error");
		}
		struct sockaddr_in remote_addr;
		remote_addr.sin_family = AF_INET;
		remote_addr.sin_port = htons(7777);
		remote_addr.sin_addr.s_addr = inet_addr(msg.content);
		//remote_addr.sin_addr.s_addr = inet_addr(LOCAL_IP);
	//	sleep(2);
		if(-1 == connect(tcp_socket, (struct sockaddr*) &remote_addr, sizeof(struct sockaddr_in)))
		{
			perror("connect error");
			return -1;
		}
		char filename[100];
		CHAT send_msg;send_msg.type = FILENAME;
		printf("请输入要传输的文件名(不超过20字符)");
		scanf("%s", filename);
		if((fd = open(filename, O_RDONLY | 0666)) == -1)
		{
			perror("open error");
			close(tcp_socket);
			return -1;
		}
		strcpy(send_msg.srcname, strrchr(filename, '/') + 1);
		printf("%s\n", send_msg.srcname);
		if(-1 == send(tcp_socket, &send_msg, sizeof(CHAT), 0))
		{
			perror("send error");
			close(tcp_socket);
			return -1;
		}
		while(1)
		{
			CHAT send_msg;
			send_msg.type = FILETYPE;
			send_msg.length = read(fd, send_msg.content, DATASIZE - 1);
			if(send_msg.length == -1)
			{
				perror("read error");
				close(tcp_socket);
				return -1;
			}
			else if(send_msg.length > 0 )
			{
				//printf("%s\n", send_msg.content);
				if(-1 == send(tcp_socket, &send_msg, sizeof(CHAT), 0))
				{
					perror("send error");
					close(tcp_socket);
					return -1;
				}
			}
			else if(send_msg.length == 0)
			{
				printf("发送完毕\n");
				close(tcp_socket);
				return 0;
			}
		}
		close(tcp_socket);
	}
	close(fd);
	return 0;
}
void* pthread_fun(void* arg)
{
	CHAT msg;
	int result;
	while(!interrupt)
	{
		//printf("%d\n", interrupt);
	//	sleep(1);
		if(0 >= recv(*((int*) arg), (CHAT*) &msg, sizeof(CHAT), 0))
		{
			perror("recv error");
			result = -1;
			pthread_exit((void*) &result);
		}
		if(msg.type == PRITYPE)
		{
			printf("%s悄悄对你说: ", msg.srcname);
			for(int i = 0;i < msg.length; i ++)
			{
				printf("%c", msg.content[i]);
			}
			printf("\n");
		}
		else if(msg.type == PUBTYPE)
		{
			printf("%s大声的说: ", msg.srcname);
			for(int i = 0;i < msg.length; i ++)
			{
				printf("%c", msg.content[i]);
			}
			printf("\n");
		}
		else if(msg.type == FILEQUETYPE)//如果有人传数据
		{
			interrupt = 2;
			requefilemsg = msg;
			/*if(pthread_spin_unlock(&lock))
			{
				perror("pthread_spin_unlock error");
			}*/
		}
		//printf("我在这\n");
	}
	pthread_exit(NULL);
	printf("bYe!\n");
}
int pubchat(int socket)
{
	pthread_t pth;
	void* result;
	CHAT msg;
	pthread_create(&pth, NULL, pthread_fun, &socket);
	interrupt = 0;
	while(interrupt == 0  || interrupt == 2)
	{
		/*if(pthread_spin_lock(&lock))
		{
			perror("pthread_spin_lock error");
		}*/
		if(interrupt == 0)
		{
			msg.type = PUBTYPE;

			fgets(msg.content, DATASIZE, stdin);
			msg.content[strlen(msg.content) - 1] = '\0';
			msg.length = strlen(msg.content);
			if(msg.length == 0)	continue;
			if(-1 == send(socket,(CHAT*) &msg, sizeof(CHAT), 0))
			{
				perror("send error");
				return -1;
			}
		}
		else if(interrupt == 2)
		{
			CHAT reply;
			strcpy(reply.dstname, requefilemsg.srcname);
			char sc;
			printf("用户%s请求传输文件，是否同意接受(y/n)", requefilemsg.srcname);
			scanf("%c", &sc);
			getchar();
			if(sc == 'y')//发送同意包
			{
				reply.type = AGREE;
				/*
				int pid = fork();
				if(pid == -1)
					printf(" fork error");
				if(pid == 0)
				{
					if(recvfile() == -1)
					{
						printf("传输失败\n");
					}
					exit(1);
				}*/
				printf("您同意了请求\n");
				if(-1 == send(socket, &reply, sizeof(CHAT), 0))//发送
				{
					perror("send error");
					//pthread_exit(NULL);
				}
				if(recvfile() == -1)
				{
					printf("传输失败\n");
				}
			}
			else if(sc == 'n')//发送拒绝包
			{
				reply.type = REFUSE;
				if(-1 == send(socket, &reply, sizeof(CHAT), 0))//发送
				{
					perror("send error");
					//pthread_exit(NULL);
				}
				printf("您拒绝了请求\n");
			}
			interrupt = 0;

		}
		printf("解锁\n");
	}
	if(pthread_cancel(pth))
	{
		perror("pthread_cancel");
	}
	printf("cancel le\n");
	return 0;
}
//待完成
int prichat(int socket)
{
	interrupt = 0;
	pthread_t pth;
	void* result;
	CHAT msg;
	printf("输入对方用户名:");
	scanf("%s", msg.dstname);
	getchar();
	pthread_create(&pth, NULL, pthread_fun, &socket);
	while(!interrupt)
	{
		msg.type = PRITYPE;
		fgets(msg.content, DATASIZE, stdin);
		msg.content[strlen(msg.content) - 1] = '\0';
		msg.length = strlen(msg.content);
		if(-1 == send(socket,(CHAT*) &msg, sizeof(CHAT), 0))
		{
			perror("send error");
			return -1;
		}
	}
	if(pthread_cancel(pth))
	{
		perror("pthread_cancel");
	}
	return 0;
}
int ban(int socket)
{
	CHAT msg;
	ACK ack;
	msg.type = BANTYPE;
	printf("输入要处理选项的用户名:");
	scanf("%s", msg.dstname);
	printf("输入选择：0、拉回 1、踢人");
	scanf("%d", &msg.flag);
	if(-1 == send(socket, &msg, sizeof(CHAT), 0))
	{
		perror("send error");
		return -1;
	}
	if(-1 == recv(socket, &ack, sizeof(ACK), 0))
	{
		perror("recv error");
		return -1;
	}
	if(ack.flag == 0)
	{
		printf("成功\n");
		return 0;
	}
	else if(ack.flag == -1)
	{
		printf("失败:%s", ack.reason);
		return -1;
	}
	return -1;
}

int speak(int socket)
{
	CHAT msg;
	ACK ack;
	msg.type = SPEACKTYPE;
	printf("输入要处理选项的用户名:");
	scanf("%s", msg.dstname);
	printf("输入选择：0、禁言 1、解禁");
	scanf("%d", &msg.flag);
	if(-1 == send(socket, &msg, sizeof(CHAT), 0))
	{
		perror("send error");
		return -1;
	}
	if(-1 == recv(socket, &ack, sizeof(ACK), 0))
	{
		perror("recv error");
		return -1;
	}
	if(ack.flag == 0)
	{
		printf("成功\n");
		return 0;
	}
	else if(ack.flag == -1)
	{
		printf("失败:%s\n", ack.reason);
		return -1;
	}
	return -1;
}
int search(int socket)
{
	SEARCH_USER users;
	CHAT msg;
	msg.type = SEARCHTYPE;
	if(-1 == send(socket, &msg, sizeof(CHAT), 0))
	{
		perror("send error");
		return -1;
	}
	if(-1 == recv(socket, &users, sizeof(SEARCH_USER), 0))
	{
		perror("recv");
		return -1;	
	}
	printf("%d\n", users.type);
	if(users.type == SEARCHTYPE)
	{
		printf("%d\n", users.count);
		for(int i = 0; i < users.count; i ++)
		{
			printf("用户名: %s\n", users.username[i]);
		}
	}
	return 0;
}
