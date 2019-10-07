all:process_exec.c
	gcc -c cJSON.c
	gcc -c run.c
	gcc -c compile.c
	gcc -c process_manager.c
	gcc -o process_manager compile.o run.o process_manager.o cJSON.o -lhiredis
	gcc -c process_exec.c
	gcc -o process_exec compile.o run.o process_exec.o cJSON.o -lhiredis