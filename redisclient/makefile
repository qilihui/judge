all:process_exec.c
	gcc -c write_log.c -g
	gcc -c cJSON.c
	gcc -c run.c -g
	gcc -c compile.c -g
	gcc -c process_manager.c -g
	gcc -o process_manager compile.o run.o process_manager.o cJSON.o write_log.o -lhiredis -g
	gcc -c process_exec.c -g
	gcc -o process_exec compile.o run.o process_exec.o cJSON.o write_log.o -lhiredis -g
	gcc -c z_test.c
	gcc -o z_test z_test.o run.o write_log.o
	rm *.o