all:process_exec.c
	gcc -c killer.c
	gcc -c write_log.c
	gcc -c cJSON.c
	gcc -c run.c
	gcc -c compile.c
	gcc -c process_manager.c
	gcc -o process_manager process_manager.o cJSON.o write_log.o -lhiredis -lmysqlclient
	gcc -c process_exec.c
	gcc -o process_exec compile.o run.o process_exec.o cJSON.o write_log.o killer.o -lhiredis -lmysqlclient -lpthread
	gcc -c z_test.c
	gcc -o z_test z_test.o run.o write_log.o killer.o -lmysqlclient -lpthread
	rm *.o