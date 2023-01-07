#ifndef PROTOCOL_H
#define PROTOCOL_H

#define LOCALHOST 6666
#define LOCAL_IP "127.0.0.1"
#define REMOTE_HOST 6666
#define REMOTE_IP "127.0.0.1"

//客户端请求登录状态标志位
#define LOGIN 1	//登录标志
#define REGISTER 2//注册标志
#define CHANGE 3    //修改标志
#define UNCONNECT 4//断开连接标志
//
//客户端回复ACK包标志位
#define SUCCESS 0
#define FLASE -1
#define ROOT_SUCCESS 3	//回应用户权限
//
#define UNSIZE 20	//用户名长度
#define PWSIZE 20	//密码长度
#define QTSIZE 50	//密保问题和答案长度
typedef struct user	//用户账户操作的数据包
{
	int flag;
	char username[UNSIZE];
	char passwd[PWSIZE];
	char question[QTSIZE];
	char answer[QTSIZE];
}USER;//登录状态时用户的数据包结构体

typedef struct users
{
	int socket_fd;//套接字	
	char username[UNSIZE];//用户名
}ONLINE_USER;//在线人数维护

typedef struct search_users
{
	int type;
	char username[50][UNSIZE];//用户名
	int count;
}SEARCH_USER;//人数查询包

typedef struct ack
{
	int flag;
	char reason[100];
}ACK;//服务器应答数据包结构体

#define PUBTYPE 1
#define PRITYPE 2
#define FILETYPE 3
#define SPEAKTYPE 4
#define BANTYPE 5
#define SEARCHTYPE 6
#define LOGOUTTYPE 7
#define FILEQUETYPE 8
#define AGREE 9
#define REFUSE 10
#define FILENAME 11

#define DATASIZE 1024
typedef struct chat
{
	int type;//数据包类型
	int length;//数据包长度
	int flag;//root发送权限管理标志位
	char srcname[20];//来自谁
	char dstname[20];//发给谁
	char content[DATASIZE];//512KB的数据包
}CHAT;

#endif
