all:main.c
	gcc -c run.c -g
	gcc -c compile.c -g
	gcc -c main.c -g
	gcc -o main compile.o run.o main.o cJSON.o -lhiredis -g