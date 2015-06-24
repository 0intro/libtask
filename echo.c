#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <task.h>
#include <stdlib.h>
#include <sys/socket.h>

enum
{
	STACK = 32768
};

static int verbose;

char *server;
void echotask(void*);

int*
mkfd2(int fd1, int fd2)
{
	int *a;

	a = malloc(2*sizeof a[0]);
	if(a == 0){
		fprintf(stderr, "out of memory\n");
		abort();
	}
	a[0] = fd1;
	a[1] = fd2;
	return a;
}

void
taskmain(int argc, char **argv)
{
	int cfd, fd;
	int rport;
	char remote[46];

	if(argc != 3){
		fprintf(stderr, "usage: tcpproxy server port\n");
		taskexitall(1);
	}
	server = argv[1];

	if((fd = netannounce(TCP, 0, atoi(argv[2]))) < 0){
		fprintf(stderr, "cannot announce on tcp port %d: %s\n", atoi(argv[1]), strerror(errno));
		taskexitall(1);
	}
	if(fdnoblock(fd) < 0){
		fprintf(stderr, "fdnoblock\n");
		taskexitall(1);
	}
	while((cfd = netaccept(fd, remote, &rport)) >= 0){
		if(verbose)
			fprintf(stderr, "connection from %s:%d\n", remote, rport);
		taskcreate(echotask, (void*)(uintptr_t)cfd, STACK);
	}
}

void
echotask(void *v)
{
	char buf[512];
	int fd, n;

	fd = (int)(uintptr_t)v;

	while((n = fdread(fd, buf, sizeof buf)) > 0)
		fdwrite(fd, buf, n);
	close(fd);
}
