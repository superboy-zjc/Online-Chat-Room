#include<sys/socket.h>
#include<unistd.h>
#include<signal.h>
#include<stdio.h>
#include<stdlib.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/wait.h>
#include<my_global.h>
#include<mysql.h>
#include<string.h>
#include<vector> //C++中vector容器
#include<sys/shm.h>
#include<sys/ipc.h>
#include<unistd.h>
#include<pthread.h>
#include"protocol.h"
#include"login.h"
#include"chat.h"

#define WAITLOGIN 1//等待登录标识符
#define FINLOGIN 2 //已登录状态
#define EXIT 100 //退出状态 
#define ROOT_FINLOGIN 3//超级用户已登录状态
using namespace std;//C++声明

MYSQL* mysql = NULL;
int shmid;//共享内存描述符

void* pthread_fun(void* arg)
{
		int acc_socket = *((int*) arg);
		void* sh_mem;
		int flag = WAITLOGIN;//状态标志,对于服务端来说，状态是局部变量，因为服务端处理多用户
		printf("PID  is coming!\n");
		//***************************等待登录状态**********************************//
		USER who;
		while(flag == WAITLOGIN)
		{
			if(-1 == recv(acc_socket, &who, sizeof(USER), 0))
			{
				perror("recv error(用户退出)");
				flag = EXIT;
			}
			ACK ack;
			switch(who.flag)
			{
				case REGISTER:
					ack = client_register(who, mysql);
					if(-1 == send(acc_socket, &ack, sizeof(ACK), 0))
					{
						perror("work_register: send error");
						flag = EXIT;
					}
					break;
				case LOGIN:
					ack = client_login(who, mysql, shmid, acc_socket);
					if(-1 == send(acc_socket, &ack, sizeof(ACK), 0))
					{
						perror("work_login: send error");
						flag = EXIT;
					}
					if(ack.flag == SUCCESS || ack.flag == ROOT_SUCCESS)//如果登录成功，更新在线人数
					{
						flag = FINLOGIN;//状态转换为已登录状态
					}
					break;
				case CHANGE:
					ack = client_change(who, mysql, acc_socket);
					if(-1 == send(acc_socket, &ack, sizeof(ACK), 0))
					{
						perror("work_change: send error");
						flag = EXIT;
					}
					break;
				case UNCONNECT:
					flag = EXIT;
					break;
			}
		}
		//***************************登陆成功状态**************************//
		while(flag == FINLOGIN)
		{
			CHAT msg;
			if(0 >= recv(acc_socket,(CHAT*) &msg, sizeof(CHAT), 0))
			{
				printf("客户端%s与服务器断开连接\n", who.username);
				flag = EXIT;
				break;
			}
			strcpy(msg.srcname, who.username);
			switch(msg.type)
			{
				case PUBTYPE:
					sh_mem = shmat(shmid, NULL, 0);//映射共享内存
					if((void*) -1 == sh_mem)
					{
						perror("shmat error");
						exit(1);
					}
					pubchat(msg, (ONLINE_USER*) sh_mem, mysql);
					shmdt(sh_mem);//分离共享内存
					break;
				case PRITYPE:case REFUSE:
					sh_mem = shmat(shmid, NULL, 0);//映射共享内存
					if((void*) -1 == sh_mem)
					{
						perror("shmat error");
						exit(1);
					}
					prichat(msg, (ONLINE_USER*) sh_mem, mysql);
					shmdt(sh_mem);//分离共享内存
					break;
				//*************服务器收到一个文件传输请求包**************8//
				case FILEQUETYPE:	
					printf("收到文件传输请求包\n");
					sh_mem = shmat(shmid, NULL, 0);//映射共享内存
					if((void*) -1 == sh_mem)
					{
						perror("shmat error");
						exit(1);
					}
					requesend(msg, (ONLINE_USER*) sh_mem, mysql, acc_socket);
					shmdt(sh_mem);//分离共享内存
					break;
				//**************************************************//
				case AGREE:
					sh_mem = shmat(shmid, NULL, 0);//映射共享内存
					if((void*) -1 == sh_mem)
					{
						perror("shmat error");
						exit(1);
					}
					printf("agree: acc_socket = %d\n", acc_socket);
					agreefile(msg, (ONLINE_USER*) sh_mem, mysql, acc_socket);
					shmdt(sh_mem);//分离共享内存
					break;
				case SPEAKTYPE:
					speak(msg, mysql, acc_socket);
					break;
				case BANTYPE:
					ban(msg, mysql, acc_socket);
					break;
				case SEARCHTYPE:
					sh_mem = shmat(shmid, NULL, 0);//映射共享内存
					if((void*) -1 == sh_mem)
					{
						perror("shmat error");
						exit(1);
					}
					search((ONLINE_USER*) sh_mem, acc_socket);
					shmdt(sh_mem);//分离共享内存	
					break;
				case LOGOUTTYPE:
					flag = EXIT;
					break;
			}
		}
		//××××××××××××××××××用户退出 ××××××××××××××××××××××××××××//
		if(flag == EXIT)
		{
			sh_mem = shmat(shmid, NULL, 0);
			if((void*) -1 == sh_mem)
			{
				perror("");
				exit(1);
			}
			for(int i = 0;i < 50; i ++)
			{
				if(((ONLINE_USER*) sh_mem)[i].socket_fd == acc_socket)
				{
					printf("user: %s 退出\n", ((ONLINE_USER*) sh_mem)[i].username);
					memset(&(((ONLINE_USER*) sh_mem)[i]), 0, sizeof(ONLINE_USER));
					//break;
				}
				else
				{
					if(((ONLINE_USER*) sh_mem)[i].socket_fd != 0)
					{
						printf("username: %s fd = %d\n", ((ONLINE_USER*) sh_mem)[i].username, ((ONLINE_USER*) sh_mem)[i].socket_fd);
					}
				}
			}
			shmdt(sh_mem);//分离共享内存
		}
		pthread_exit(NULL);
}
int main()
{
	ONLINE_USER users[50];
	memset(&users[0], 0, 50*sizeof(ONLINE_USER));
	//建立共享内存
	shmid = shmget(IPC_PRIVATE, sizeof(ONLINE_USER) * 50, 0666);
	if(shmid == -1)
	{
		perror("shmget error");
		exit(1);
	}
	void* sh_mem = shmat(shmid, NULL, 0);
	if((void*) -1 == sh_mem)
	{
		perror("shmat error");
		exit(1);
	}
	memcpy(sh_mem, &users[0], sizeof(ONLINE_USER) * 50);
	shmdt(sh_mem);//分离共享内存
	//	printf("~~~~~~~~~~~~%d\n", ((ONLINE_USER*) sh_mem)[50].socket_fd);
/*	if(signal(SIGCHLD, sig_fun) == SIG_ERR)//安装子进程回收信号
	{
		perror("signal");
		exit(1);
	}*/
	mysql = mysql_init(NULL);
	if(NULL == mysql)
	{
		printf("mysql_init error: %s\n", mysql_error(mysql));
		exit(1);
	}
	if(NULL == mysql_real_connect(mysql, "localhost", "root", "asd4466911", "ChatRoomProject", 0, NULL, 0))
	{
		printf("mysql_real_connect error: %s\n", mysql_error(mysql));
		exit(1);
	}
	int tcp_socket = socket(AF_INET, SOCK_STREAM,0);//创建一个TCP/IPV4套接字
	if(tcp_socket == -1)
	{
		perror("socket error");
		exit(1);
	}
	struct sockaddr_in local_addr;
	local_addr.sin_family = AF_INET;
	local_addr.sin_port = htons(LOCALHOST);
	local_addr.sin_addr.s_addr = inet_addr(LOCAL_IP);
	if(-1 == bind(tcp_socket, (struct sockaddr*) &local_addr, sizeof(struct sockaddr_in)))//绑定服务器IP和端口
	{
		perror("bind error");
		exit(1);
	}
	if(-1 == listen(tcp_socket, 50))//设置服务器监听数量
	{
		perror("listen error");
		exit(1);
	}
	while(1)
	{
		int acc_socket;
		struct sockaddr_in remote_addr;
		socklen_t len = sizeof(struct sockaddr_in);
		if(-1 == (acc_socket = accept(tcp_socket, (struct sockaddr*) &remote_addr, &len)))//等待用户连接
		{
			perror("accept error");
			exit(1);
		}
		/*{
			CHAT msg;
			struct sockaddr_in remote_addr;
			socklen_t len;
			if(-1 == getpeername(acc_socket, (struct sockaddr*) &remote_addr, &len))
			{
				perror("getpeername error");
				return -1;
			}
			inet_ntop(AF_INET, &remote_addr.sin_addr, msg.content, sizeof(msg.content));
			msg.length = strlen(msg.content);
			printf("%s: %d \n", msg.content, msg.length);
		}*/
		pthread_t pth;
		if(!pthread_create(&pth, NULL, pthread_fun, (void*) &acc_socket))
		{
			perror("pthread_create error");
		}
		printf("acc_socket = %d\n", acc_socket);
	}
	close(tcp_socket);
}
