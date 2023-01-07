#include<mysql.h>
#include<my_global.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
int search_ban(MYSQL* mysql, char username[20])
{
	char tmp_str[100];
	sprintf(tmp_str, "select ban from user where username='%s'", username);
	if(0 != mysql_query(mysql, tmp_str))
	{
		printf("查询是否被踢失败，原因:%s", mysql_error(mysql));
		return -1;
	}
	MYSQL_RES* res = mysql_store_result(mysql);
	if(res == NULL)
	{
		printf("mysql_store_result error:%s", mysql_error(mysql));
		mysql_free_result(res);
		return -1;
	}
	MYSQL_ROW row = mysql_fetch_row(res);
	if(strcmp(row[0], "0") == 0)
	{
		mysql_free_result(res);
		return 0;	
	}
	else if(strcmp(row[0], "1") == 0)
	{

		mysql_free_result(res);
		return 1;
	}
	else
	{

		mysql_free_result(res);
		return -1;
	}
	return -1;
}
