#include"protocol.h"
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
#include<sys/shm.h>
#include<sys/ipc.h>
#include"chat.h"
ACK client_register(USER user, MYSQL* mysql)//处理用户的注册请求
{
	char tmp_str[100];
	ACK ack;	ack.flag = 0;
	sprintf(tmp_str,"insert into user (username,passwd,question,answer) values('%s','%s','%s','%s')",user.username, user.passwd, user.question, user.answer );
	if(0 != mysql_query(mysql, tmp_str))
	{
		printf("register:query(insert) error: %s\n", mysql_error(mysql));
		ack.flag = -1;
		sprintf(ack.reason, "%s", mysql_error(mysql));
		return ack;
	}
	return ack;
}

ACK client_login(USER user, MYSQL* mysql, int shmid, int socket)//处理用户的登录请求,查询共享内存中的在线人数表
{
	char tmp_str[100];
	ACK ack;	ack.flag = 0;
	sprintf(tmp_str,"select passwd from user where username='%s'", user.username);	
	if(0 != mysql_query(mysql, tmp_str))
	{
		printf("login:query(select) error: %s\n", mysql_error(mysql));
		ack.flag = -1;
		sprintf(ack.reason, "%s", mysql_error(mysql));
		return ack;
	}
	MYSQL_RES* res = mysql_store_result(mysql);
	if(res == NULL)
	{
		printf("%s", mysql_error(mysql));	
		ack.flag = -1;
		sprintf(ack.reason, "%s", mysql_error(mysql));
		return ack;
	}
	if(mysql_num_rows(res) == 0)
	{
		printf("用户名错误\n");
		ack.flag = -1;
		sprintf(ack.reason, "用户名错误\n");
		mysql_free_result(res);
		return ack;
	}
	else
	{
		MYSQL_ROW row = mysql_fetch_row(res);
		if(strcmp(row[0], user.passwd) == 0)
		{
			void* sh_mem = shmat(shmid, NULL, 0);
			if((void*) -1 == sh_mem)
			{
				perror("shmat error");
				ack.flag = -1;
				sprintf(ack.reason, "shmat error\n");
				mysql_free_result(res);
				return ack;
			}
			for(int i = 0;i < 50; i ++)
			{
				if(strcmp(((ONLINE_USER*) sh_mem)[i].username, user.username) == 0)
				{
					printf("user: %s 登录冲突\n", user.username);
					ack.flag = -1;
					sprintf(ack.reason, "用户已在线！\n");
					mysql_free_result(res);
					return ack;
				}
			}
			printf("user: %s 登录\n", user.username);
			for(int i = 0;i  < 50; i ++)
			{
				if(((ONLINE_USER*) sh_mem)[i].socket_fd == 0)
				{
					((ONLINE_USER*) sh_mem)[i].socket_fd = socket;
					strcpy(((ONLINE_USER*) sh_mem)[i].username, user.username);
					break;
				}
			}
			shmdt(sh_mem);//分离共享内存
			if(search_root(mysql, user.username) == 1)//如果是管理员
			{
				ack.flag = ROOT_SUCCESS;
				return ack;
			}
			else
			{
				ack.flag = SUCCESS;
				return ack;
			}
			/*
			if(-1 == send(socket, &ack, sizeof(ACK), 0))//发送用户的权限信息
			{
				perror("login:send question: send error");
				mysql_free_result(res);
				ack.flag = FLASE;
				sprintf(ack.reason, "权限发送失败！\n");
				return ack;
			}*/
		}
		else
		{
			printf("user: %s 密码错误\n", user.username);
			ack.flag = -1;
			sprintf(ack.reason, "密码错误！\n");
			mysql_free_result(res);
			return ack;
		}
	}
	mysql_free_result(res);
	return ack;
}

ACK client_change(USER user, MYSQL* mysql, int socket)//修改密码模块比较特殊，模块内需要发送密保问题，多一个socket参数
{
	char tmp_str[100];
	ACK ack;    ack.flag = 0;
	sprintf(tmp_str,"select question,answer from user where username='%s'", user.username);
	if(0 != mysql_query(mysql, tmp_str))//在数据库中查询用户的密保问题和答案
	{
		printf("change:query(select) error: %s\n", mysql_error(mysql));
		ack.flag = -1;
		sprintf(ack.reason, "%s", mysql_error(mysql));
		return ack;
	}
	MYSQL_RES* res = mysql_store_result(mysql);//提取查询的内容
	if(res == NULL)//如果mysql_store_result执行失败
	{
		printf("%s", mysql_error(mysql));
		ack.flag = -1;
		sprintf(ack.reason, "%s", mysql_error(mysql));
		return ack;
	}
	if(mysql_num_rows(res) == 0)//如果内容为空，可能是无此用户名
	{
		printf("change: 无此用户名\n");
		ack.flag = -1;
		sprintf(ack.reason, "无此用户名\n");
		mysql_free_result(res);
		return ack;
	}
	else//存在此用户名
	{
		MYSQL_ROW row = mysql_fetch_row(res);//提取查询的行内容（问题，答案）
		sprintf(ack.reason, "%s", row[0]);//把问题封装到数据包里，返回给客户端
		if(-1 == send(socket, &ack, sizeof(ACK), 0))//发送带有密保问题的数据包给客户端进行进一步验证
			{
				perror("change:send question: send error");
				mysql_free_result(res);
				ack.flag = -1;
				sprintf(ack.reason, "系统服务器连接出现问题\n");
				return ack;
			}
		if(-1 == recv(socket, &user, sizeof(USER), 0))//等待客户端发送密保答案和修改的最新密码
			{
				perror("change: recv error");
				mysql_free_result(res);
				ack.flag = -1;
				sprintf(ack.reason, "系统服务器连接出现问题\n");
				return ack;
			}
		//接受到客户端发送的答案与新密码
		if(strcmp(row[1], user.answer) == 0)//如果答案正确，修改密码        
		{
			printf("user: %s 密保答案正确，修改密码\n", user.username);
			sprintf(tmp_str, "update user set passwd='%s' where username = '%s'", user.passwd, user.username);
			if(0 != mysql_query(mysql, tmp_str))//在数据库中更新用户的新密码
			{
				printf("change:query(update error: %s\n", mysql_error(mysql));
				ack.flag = -1;
				sprintf(ack.reason, "%s", mysql_error(mysql));
				mysql_free_result(res);
				return ack;
			}
		}
		else    //如果答案错误
		{
			printf("user: %s 密保答案错误\n", user.username);
			ack.flag = -1;
			sprintf(ack.reason, "密保答案错误！\n");
			mysql_free_result(res);
			return ack;
		}
	}   
	mysql_free_result(res);
	return ack;

}

