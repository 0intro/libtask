#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <task.h>

enum { STACK = 32768 };

Channel *c;

void
delaytask(void *v)
{
	taskdelay((int)(uintptr_t)v);
	printf("awake after %d ms\n", (int)(uintptr_t)v);
	chansendul(c, 0);
}

void
taskmain(int argc, char **argv)
{
	int i, n;

	c = chancreate(sizeof(unsigned long), 0);

	n = 0;
	for(i=1; i<argc; i++){
		n++;
		printf("x");
		taskcreate(delaytask, (void*)(uintptr_t)atoi(argv[i]), STACK);
	}

	/* wait for n tasks to finish */
	for(i=0; i<n; i++){
		printf("y");
		chanrecvul(c);
	}
	taskexitall(0);
}
