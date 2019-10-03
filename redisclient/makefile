all:process_exec.c
	gcc -c run.c -g
	gcc -c compile.c -g
	gcc -c process_manager.c -g
	gcc -o process_manager compile.o run.o process_manager.o cJSON.o -lhiredis -g
	gcc -c process_exec.c -g
	gcc -o process_exec compile.o run.o process_exec.o cJSON.o -lhiredis -g