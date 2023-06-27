thread-tcp-para-get: output_file.c thread-tcp-para-get-pipe.c tcp-para-get-pipe.h tcp-para-get-servers.h
	gcc -o thread-tcp-para-get output_file.c  -lpthread thread-tcp-para-get-pipe.c
