all:webserver.c
	gcc -c webserver.c -I/usr/include/evhtp -g
	gcc -o webserver cJSON.o webserver.o -L/usr/lib/x86_64-linux-gnu -levhtp -levent -g