#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <task.h>
#include <stdlib.h>

enum
{
	STACK = 32768
};

char *server;
int port;

void fetchtask(void*);

void
taskmain(int argc, char **argv)
{
	int i, n;

	if(argc != 4){
		fprintf(stderr, "usage: httpload n server port\n");
		taskexitall(1);
	}
	n = atoi(argv[1]);
	server = argv[2];
	port = atoi(argv[3]);

	for(i=0; i<n; i++)
		taskcreate(fetchtask, 0, STACK);
}

void
fetchtask(void *v)
{
	int fd, i;
	char buf[512];

	(void)v;

	for(i=0; i<1000; i++){
		if((fd = netdial(TCP, server, port)) < 0){
			fprintf(stderr, "dial %s: %s (%s)\n", server, strerror(errno), taskgetstate());
			continue;
		}
		snprintf(buf, sizeof buf, "xxxxxxxxxx");
		fdwrite(fd, buf, strlen(buf));
		close(fd);
	}
}
