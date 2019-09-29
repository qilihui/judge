all:webserver.c
	gcc -c webserver.c -I/usr/include/evhtp
	gcc -o webserver webserver.o -L/usr/lib/x86_64-linux-gnu -levhtp -levent