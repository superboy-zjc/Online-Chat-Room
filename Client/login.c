#include"protocol.h"
#include<sys/socket.h>
#include<string.h>
#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/wait.h>
int ui_login(void)//登录时的字符界面
{
	printf("1、登录	2、注册	3、修改密码	4、退出\n");
	int num;
	scanf("%d", &num);
	return num;
}

int Register(int socket)//客户端注册请求
{
	USER me;
	me.flag = REGISTER;
	printf("请输入用户名:\n");//输入用户名
	scanf("%s", me.username);
	printf("请输入密码:\n");//输入密码
	scanf("%s", me.passwd);
	getchar();
	printf("请输入密保问题\n");//输入密保问题
	if(fgets(me.question, QTSIZE, stdin) == NULL)
	{
		printf("register: fgets error\n");
		exit(1);
	}
	me.question[strlen(me.question) - 1] = '\0';//去除fgets吸收的键盘回车
	printf("请输入密保答案\n");//输入密保答案
	if(fgets(me.answer, QTSIZE, stdin) == NULL)
	{
		printf("register: fgets error\n");
		exit(1);
	}
	me.answer[strlen(me.answer) - 1] = '\0';//去除fgets吸收的键盘回车
	if(-1 == send(socket, &me, sizeof(USER), 0))//发送注册数据包
	{
		perror("send error");
		return -1;
	}
	ACK ack;
	if(-1 == recv(socket, &ack, sizeof(ACK), 0))//注册请求发送后，等待服务器回应
	{
		perror("recv error");
		return -1;
	}
	if(ack.flag == -1)//判断服务器回复，注册是否成功
	{
		printf("注册失败！\n原因：%s", ack.reason);
		return -1;
	}
	if(ack.flag == 0)
	{
		printf("注册成功！\n");
	}
	return 0;
}
int login(int socket)//客户端登录请求
{
	USER me;
	me.flag = LOGIN;
	printf("请输入用户名:\n");
	scanf("%s", me.username);
	printf("请输入密码:\n");
	scanf("%s", me.passwd);
	if(-1 == send(socket, &me, sizeof(USER), 0))
	{
		perror("send error");
		return -1;
	}
	ACK ack;
	if(-1 == recv(socket, &ack, sizeof(ACK), 0))//登录请求发送后，等待服务器回应
	{
		perror("recv error");
		return -1;
	}
	if(ack.flag == FALSE)//判断服务器回复，登录是否成功
	{
		printf("登录失败！\n原因：%s", ack.reason);
		return -1;
	}
	if(ack.flag == SUCCESS)
	{
		printf("登录成功！\n");
	}
	if(ack.flag == ROOT_SUCCESS)
	{
		printf("登陆成功 您是root用户！\n");
	}
/////////////////////////////////
	return ack.flag;
}

int change(int socket)
{
	USER me;
	me.flag = CHANGE;
	printf("请输入用户名:\n");
	scanf("%s", me.username);

	if(-1 == send(socket, &me, sizeof(USER), 0)) //请求服务器数据库中的密保问题
	{
		perror("send error");
		return -1;
	}
	ACK ack;
	if(-1 == recv(socket, &ack, sizeof(ACK), 0))//等待服务器回复密保问题
	{
		perror("recv error");
		return -1;
	}
	if(ack.flag == -1)//如果不存在用户名
	{
		printf("无此用户名！\n");
		return -1;
	}
	printf("请输入新密码:\n");
	scanf("%s", me.passwd);
	printf("你的密保问题是：\n%s\n", ack.reason);
	getchar();//吸收上一个scanf的回车，否则fgets会接受
	printf("请输入你的密保答案:\n");
	if(fgets(me.answer, QTSIZE, stdin) == NULL)
	{
		printf("change: fgets error\n");
		return -1;
	}
	me.answer[strlen(me.answer) - 1] = '\0';//去除fgets吸收的键盘回车
	if(-1 == send(socket, &me, sizeof(USER), 0))
	{
		perror("send error");
		return -1;
	}
	if(-1 == recv(socket, &ack, sizeof(ACK), 0))//修改密码请求发送后，等待服务器回应
	{
		perror("recv error");
		return -1;
	}
	if(ack.flag == -1)//判断服务器回复，修改密码是否成功
	{
		printf("修改失败！\n原因：%s", ack.reason);
		return -1;
	}
	if(ack.flag == 0)
	{
		printf("修改成功！\n");
	}
/////////////////////////////////
	return 0;
}
int Exit(int socket)
{
	USER me;
	me.flag = UNCONNECT;
	if(-1 == send(socket, &me, sizeof(USER), 0))
	{
		perror("send error");
		return -1;
	}
	return 0;
}
