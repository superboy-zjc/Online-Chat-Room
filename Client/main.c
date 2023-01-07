#include<sys/socket.h>
#include<string.h>
#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/wait.h>
#include"protocol.h"
#include"login.h"
#include"chat.h"
#include<pthread.h>

int interrupt = 0;//打断操作 
pthread_spinlock_t lock;
int tcp_socket;

int flag = WAITLOGIN;//对于客户端来说 状态是全局变量。
//int tcp_socket;//设置全局变量供信号使用
void sig_fun(int sig)
{
	if(sig == SIGINT)
	{
		/*if(flag == FINLOGIN || ROOT_FINLOGIN)
		{
			if(Exit(tcp_socket) == 0)
			{
				flag = UNCONNECT;
				printf("Bye!\n");
			}
			else
			{
				printf("与服务器断开失败\n");
			}
		}*/
		close(tcp_socket);
		exit(1);
	}
	if(sig == SIGTSTP)
	{
		printf("等待\n");
		printf("进入\n");
		interrupt = 1;
		printf("回车继续\n");
	}
	int pid;
	if(sig == SIGCHLD)
	{
		while((pid = waitpid(-1, NULL, WNOHANG)) > 0)
		{
			printf("进程%d回收\n", pid);
		}
	}
}
int main(int argc, char* argv[])
{
	if(signal(SIGINT, sig_fun) == SIG_ERR)//客户异常退出
	{
		perror("signal");
		exit(1);
	}
	if(signal(SIGTSTP, sig_fun) == SIG_ERR)//安装中断信号
	{
		perror("signal");
		exit(1);
	}
	if(signal(SIGCHLD, sig_fun) == SIG_ERR)//安装子进程回收信号
	{
		perror("signal");
		exit(1);
	}
	if(pthread_spin_init(&lock, PTHREAD_PROCESS_PRIVATE))
	{
		perror("pthread_spin_init error");
		return -1;
	}

login:
	tcp_socket = socket(AF_INET, SOCK_STREAM,0);//创建一个TCP/IPV4套接字
	if(tcp_socket == -1)
	{
		perror("socket error");
		exit(1);
	}
	struct sockaddr_in remote_addr;
	remote_addr.sin_family = AF_INET;
	remote_addr.sin_port = htons(REMOTE_HOST);
	remote_addr.sin_addr.s_addr = inet_addr(REMOTE_IP);
	printf("连接\n");
	if(-1 == connect(tcp_socket, (struct sockaddr*) &remote_addr, sizeof(struct sockaddr_in)))
	{
		perror("connect error");
		exit(1);
	}
	printf("成功\n");
	while(flag == WAITLOGIN)
	{
		switch(ui_login())
		{
			case 1://登录
				{
					int stat = login(tcp_socket);
					if(stat == SUCCESS)//用户是普通用户
					{
						flag = FINLOGIN;
					}
					if(stat == ROOT_SUCCESS)//用户是超级用户
					{
						flag = ROOT_FINLOGIN;
					}
				}
				break;
			case 2://注册
				Register(tcp_socket);
				break;
			case 3://修改密码
				change(tcp_socket);
				break;
			case 4://退出
				if(Exit(tcp_socket) == 0)
				{
					flag = UNCONNECT;
					printf("Bye!\n");
				}
				else
				{
					printf("与服务器断开失败\n");
				}
				break;
		}
	}
	while(flag == FINLOGIN || flag == ROOT_FINLOGIN)//用户与服务器成功连接
	{
	//	printf("出来了\n");
		switch(ui_chat(flag))
		{
			case 1://群聊
				pubchat(tcp_socket);	
				break;
			case 2://私聊
				prichat(tcp_socket);	
				break;
			case 3://文件传输
				if(sendfile(tcp_socket) == 0)
				{
					printf("传输成功\n");
				}
				else
				{
					printf("传输失败\n");
				}
				break;
			case 4://踢人
				ban(tcp_socket);
				break;
			case 5://禁言
				speak(tcp_socket);
				break;
			case 8://显示在线人数
				search(tcp_socket);
				break;
			case 9://注销
				flag = WAITLOGIN;
				{
					CHAT msg;
					msg.type = LOGOUTTYPE;
					if(-1 == send(tcp_socket, &msg, sizeof(CHAT), 0))
					{
						perror("logout send error");
					}
					close(tcp_socket);
				}
				goto login;
				break;
			case 0://退出
				flag = EXIT;
				break;
		}	
	}
	if(flag == EXIT)
		close(tcp_socket);
	pthread_spin_destroy(&lock);
}
