#include "taskimpl.h"
#if USE_EPOLL
#include <sys/epoll.h>
#else
#include <sys/poll.h>
#endif
#include <fcntl.h>

#if USE_EPOLL
static int epfd;
#else
enum
{
	MAXFD = 1024
};

static struct pollfd pollfd[MAXFD];
static Task *polltask[MAXFD];
static int npollfd;
#endif

static int startedfdtask;
static Tasklist sleeping;
static int sleepingcounted;
static uvlong nsec(void);

#if USE_EPOLL

void
fdtask(void *v)
{
	struct epoll_event events[1000];
	int i, ms, nevents;
	Task *t;
	uvlong now;

	USED(v);
	tasksystem();
	taskname("fdtask");
	for(;;){
		/* let everyone else run */
		while(taskyield() > 0)
			;
		/* we're the only one runnable - poll for i/o */
		errno = 0;
		taskstate("epoll");
		if((t=sleeping.head) == nil)
			ms = -1;
		else{
			/* sleep at most 5s */
			now = nsec();
			if(now >= t->alarmtime)
				ms = 0;
			else if(now+5*1000*1000*1000LL >= t->alarmtime)
				ms = (t->alarmtime - now)/1000000;
			else
				ms = 5000;
		}
		if((nevents = epoll_wait(epfd, events, 1000, ms)) < 0){
			if(errno == EINTR)
				continue;
			fprint(2, "epoll: %s\n", strerror(errno));
			taskexitall(0);
		}

		/* wake up the guys who deserve it */
		for(i=0; i<nevents; i++){
			taskready((Task *)events[i].data.ptr);
		}

		now = nsec();
		while((t=sleeping.head) && now >= t->alarmtime){
			deltask(&sleeping, t);
			if(!t->system && --sleepingcounted == 0)
				taskcount--;
			taskready(t);
		}
	}
}

#else

void
fdtask(void *v)
{
	int i, ms;
	Task *t;
	uvlong now;

	USED(v);
	tasksystem();
	taskname("fdtask");
	for(;;){
		/* let everyone else run */
		while(taskyield() > 0)
			;
		/* we're the only one runnable - poll for i/o */
		errno = 0;
		taskstate("poll");
		if((t=sleeping.head) == nil)
			ms = -1;
		else{
			/* sleep at most 5s */
			now = nsec();
			if(now >= t->alarmtime)
				ms = 0;
			else if(now+5*1000*1000*1000LL >= t->alarmtime)
				ms = (t->alarmtime - now)/1000000;
			else
				ms = 5000;
		}
		if(poll(pollfd, npollfd, ms) < 0){
			if(errno == EINTR)
				continue;
			fprint(2, "poll: %s\n", strerror(errno));
			taskexitall(0);
		}

		/* wake up the guys who deserve it */
		for(i=0; i<npollfd; i++){
			while(i < npollfd && pollfd[i].revents){
				taskready(polltask[i]);
				--npollfd;
				pollfd[i] = pollfd[npollfd];
				polltask[i] = polltask[npollfd];
			}
		}

		now = nsec();
		while((t=sleeping.head) && now >= t->alarmtime){
			deltask(&sleeping, t);
			if(!t->system && --sleepingcounted == 0)
				taskcount--;
			taskready(t);
		}
	}
}

#endif

uint
taskdelay(uint ms)
{
	uvlong when, now;
	Task *t;

	if(!startedfdtask){
		startedfdtask = 1;
#if USE_EPOLL
		epfd = epoll_create(1);
		if(epfd < 0)
			abort();
#endif
		taskcreate(fdtask, 0, 32768);
	}

	now = nsec();
	when = now+(uvlong)ms*1000000;
	for(t=sleeping.head; t!=nil && t->alarmtime < when; t=t->next)
		;

	if(t){
		taskrunning->prev = t->prev;
		taskrunning->next = t;
	}else{
		taskrunning->prev = sleeping.tail;
		taskrunning->next = nil;
	}

	t = taskrunning;
	t->alarmtime = when;
	if(t->prev)
		t->prev->next = t;
	else
		sleeping.head = t;
	if(t->next)
		t->next->prev = t;
	else
		sleeping.tail = t;

	if(!t->system && sleepingcounted++ == 0)
		taskcount++;
	taskswitch();

	return (nsec() - now)/1000000;
}

#if USE_EPOLL

void
fdwait(int fd, int rw)
{
	struct epoll_event ev;
	int bits, duped, r;

	if(!startedfdtask){
		startedfdtask = 1;
		epfd = epoll_create(1);
		if(epfd < 0)
			abort();
		taskcreate(fdtask, 0, 32768);
	}

	taskstate("fdwait for %s", rw=='r' ? "read" : rw=='w' ? "write" : "error");
	bits = 0;
	switch(rw){
	case 'r':
		bits |= EPOLLIN | EPOLLPRI;
		break;
	case 'w':
		bits |= EPOLLOUT;
		break;
	}

	memset(&ev, 0, sizeof(ev));
	ev.events = bits;
	ev.data.ptr = taskrunning;
	r = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
	duped = 0;
	if(r < 0 || errno == EEXIST){
		duped = 1;
		fd = dup(fd);
		if(fd < 0)
			abort();
		r = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
		if(r != 0)
			abort();
	}
	taskswitch();
	epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &ev);
	if(duped)
		close(fd);
}

#else

void
fdwait(int fd, int rw)
{
	int bits;

	if(!startedfdtask){
		startedfdtask = 1;
		taskcreate(fdtask, 0, 32768);
	}

	if(npollfd >= MAXFD){
		fprint(2, "too many poll file descriptors\n");
		abort();
	}

	taskstate("fdwait for %s", rw=='r' ? "read" : rw=='w' ? "write" : "error");
	bits = 0;
	switch(rw){
	case 'r':
		bits |= POLLIN;
		break;
	case 'w':
		bits |= POLLOUT;
		break;
	}

	polltask[npollfd] = taskrunning;
	pollfd[npollfd].fd = fd;
	pollfd[npollfd].events = bits;
	pollfd[npollfd].revents = 0;
	npollfd++;
	taskswitch();
}

#endif

/* Like fdread but always calls fdwait before reading. */
int
fdread1(int fd, void *buf, int n)
{
	int m;

	do
		fdwait(fd, 'r');
	while((m = read(fd, buf, n)) < 0 && errno == EAGAIN);
	return m;
}

int
fdread(int fd, void *buf, int n)
{
	int m;

	while((m=read(fd, buf, n)) < 0 && errno == EAGAIN)
		fdwait(fd, 'r');
	return m;
}

int
fdreadn(int fd, void *buf, int n)
{
	int m, tot;

	for(tot=0; tot<n; tot+=m){
		m = fdread(fd, (char*)buf+tot, n-tot);
		if(m < 0)
			return m;
		if(m == 0)
			break;
	}

	return tot;
}

int
fdwrite(int fd, void *buf, int n)
{
	int m, tot;

	for(tot=0; tot<n; tot+=m){
		while((m=write(fd, (char*)buf+tot, n-tot)) < 0 && errno == EAGAIN)
			fdwait(fd, 'w');
		if(m < 0)
			return m;
		if(m == 0)
			break;
	}
	return tot;
}

int
fdnoblock(int fd)
{
	return fcntl(fd, F_SETFL, fcntl(fd, F_GETFL)|O_NONBLOCK);
}

static uvlong
nsec(void)
{
	struct timeval tv;

	if(gettimeofday(&tv, 0) < 0)
		return -1;
	return (uvlong)tv.tv_sec*1000*1000*1000 + tv.tv_usec*1000;
}

