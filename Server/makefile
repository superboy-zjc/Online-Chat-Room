main:main.o login.o chat.o search_root.o search_ban.o search_speak.o
	g++ $^ -o main -I/usr/include/mysql -lmysqlclient -Wall -lpthread
main.o:
	g++ main.c -c -I/usr/include/mysql -lmysqlclient
search_root.o:
	g++ search_root.c -c -I/usr/include/mysql -lmysqlclient
search_ban.o:
	g++ search_ban.c -c -I/usr/include/mysql -lmysqlclient
search_speak.o:
	g++ search_speak.c -c -I/usr/include/mysql -lmysqlclient
login.o:search_root.o
	g++ login.c -c -I/usr/include/mysql -lmysqlclient
chat.o:search_ban.o search_speak.o
	g++ chat.c -c -I/usr/include/mysql -lmysqlclient
clean:
	rm *.o 
