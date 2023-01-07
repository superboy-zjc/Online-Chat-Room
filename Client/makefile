main:main.o login.o chat.o
	g++ $^ -o main  -lpthread -Wall
main.o:main.c
	g++ main.c -c
login.o:login.c
	g++ -c login.c
chat.o:chat.c
	g++ -c chat.c -lpthread
.PHONY:
clean:
	rm *.o
