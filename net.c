#include "taskimpl.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/poll.h>
#include "ip.h"

static int
family(unsigned char *addr)
{
	if(isv4(addr))
		return AF_INET;
	return AF_INET6;
}

int
netannounce(int istcp, char *server, int port)
{
	int fd, n, proto;
	struct sockaddr_storage ss;
	socklen_t sn;
	unsigned char ip[IPaddrlen];

	taskstate("netannounce");
	proto = istcp ? SOCK_STREAM : SOCK_DGRAM;
	memset(&ss, 0, sizeof ss);
	if(server != nil && strcmp(server, "*") != 0){
		if(netlookup(server, ip) < 0){
			taskstate("netlookup failed");
			return -1;
		}
		ss.ss_family = family(ip);
		switch(ss.ss_family){
		case AF_INET:
			v6tov4((unsigned char*)&((struct sockaddr_in*)&ss)->sin_addr.s_addr, ip);
			break;
		case AF_INET6:
			memcpy(&((struct sockaddr_in6*)&ss)->sin6_addr.s6_addr, ip, sizeof(struct in6_addr));
			break;
		}
	}else{
		ss.ss_family = AF_INET6;
		((struct sockaddr_in6*)&ss)->sin6_addr = in6addr_any;
	}
	switch(ss.ss_family){
	case AF_INET:
		hnputs(&((struct sockaddr_in*)&ss)->sin_port, port);
		break;
	case AF_INET6:
		hnputs(&((struct sockaddr_in6*)&ss)->sin6_port, port);
		break;
	}
	if((fd = socket(ss.ss_family, proto, 0)) < 0){
		taskstate("socket failed");
		return -1;
	}

	/* set reuse flag for tcp */
	sn = sizeof n;
	if(istcp && getsockopt(fd, SOL_SOCKET, SO_TYPE, (void*)&n, &sn) >= 0){
		n = 1;
		setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)&n, sizeof n);
	}

	if(bind(fd, (struct sockaddr*)&ss, sizeof ss) < 0){
		taskstate("bind failed");
		close(fd);
		return -1;
	}

	if(proto == SOCK_STREAM)
		listen(fd, 16);

	fdnoblock(fd);
	taskstate("netannounce succeeded");
	return fd;
}

int
netaccept(int fd, char *server, int *port)
{
	int cfd, one;
	struct sockaddr_storage ss;
	socklen_t len;

	fdwait(fd, 'r');

	taskstate("netaccept");
	len = sizeof ss;
	if((cfd = accept(fd, (void*)&ss, &len)) < 0){
		taskstate("accept failed");
		return -1;
	}

	switch(ss.ss_family){
	case AF_INET:
		if(server)
			inet_ntop(AF_INET, &((struct sockaddr_in*)&ss)->sin_addr.s_addr, server, INET_ADDRSTRLEN);
		if(port)
			*port = nhgets(&((struct sockaddr_in*)&ss)->sin_port);
		break;
	case AF_INET6:
		if(server)
			inet_ntop(AF_INET6, &((struct sockaddr_in6*)&ss)->sin6_addr.s6_addr, server, INET6_ADDRSTRLEN);
		if(port)
			*port = nhgets(&((struct sockaddr_in6*)&ss)->sin6_port);
		break;
	}
	fdnoblock(cfd);
	one = 1;
	setsockopt(cfd, IPPROTO_TCP, TCP_NODELAY, (char*)&one, sizeof one);
	taskstate("netaccept succeeded");
	return cfd;
}

int
netlookup(char *name, unsigned char *ip)
{
	struct hostent *he;
	struct addrinfo *result;

	if(parseip(ip, name) == 0)
		return 0;

	/* BUG - Name resolution blocks.  Need a non-blocking DNS. */
	taskstate("netlookup");
	if((he = gethostbyname(name)) != 0){
		switch(he->h_addrtype) {
		case AF_INET:
			v4tov6(ip, (unsigned char*)he->h_addr);
			break;
		case AF_INET6:
			memcpy(ip, (unsigned char*)he->h_addr, he->h_length);
			break;
		}
		taskstate("netlookup succeeded");
		return 0;
	}else if(getaddrinfo(name, NULL, NULL, &result) == 0) {
		switch (result->ai_family) {
		case AF_INET:
			v4tov6(ip, (unsigned char*)&((struct sockaddr_in*)result->ai_addr)->sin_addr.s_addr);
			break;
		case AF_INET6:
			memcpy(ip, (unsigned char*)&((struct sockaddr_in6*)result->ai_addr)->sin6_addr.s6_addr, sizeof(struct in6_addr));
			break;
		}
		taskstate("netlookup succeeded");
		return 0;
	}

	taskstate("netlookup failed");
	return -1;
}

int
netdial(int istcp, char *server, int port)
{
	int proto, fd, n;
	unsigned char ip[IPaddrlen];
	struct sockaddr_storage ss;
	socklen_t sn;

	if(netlookup(server, ip) < 0)
		return -1;

	taskstate("netdial");
	proto = istcp ? SOCK_STREAM : SOCK_DGRAM;
	if((fd = socket(family(ip), proto, 0)) < 0){
		taskstate("socket failed");
		return -1;
	}
	fdnoblock(fd);

	/* for udp */
	if(!istcp){
		n = 1;
		setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &n, sizeof n);
	}

	/* start connecting */
	memset(&ss, 0, sizeof ss);
	ss.ss_family = family(ip);
	switch(ss.ss_family){
	case AF_INET:
		v6tov4((unsigned char*)&((struct sockaddr_in*)&ss)->sin_addr.s_addr, ip);
		hnputs(&((struct sockaddr_in*)&ss)->sin_port, port);
		break;
	case AF_INET6:
		memcpy(&((struct sockaddr_in6*)&ss)->sin6_addr.s6_addr, ip, sizeof(struct in6_addr));
		hnputs(&((struct sockaddr_in6*)&ss)->sin6_port, port);
		break;
	}

	if(connect(fd, (struct sockaddr*)&ss, sizeof ss) < 0 && errno != EINPROGRESS){
		taskstate("connect failed");
		close(fd);
		return -1;
	}

	/* wait for finish */
	fdwait(fd, 'w');
	sn = sizeof ss;
	if(getpeername(fd, (struct sockaddr*)&ss, &sn) >= 0){
		taskstate("connect succeeded");
		return fd;
	}

	/* report error */
	sn = sizeof n;
	getsockopt(fd, SOL_SOCKET, SO_ERROR, (void*)&n, &sn);
	if(n == 0)
		n = ECONNREFUSED;
	close(fd);
	taskstate("connect failed");
	errno = n;
	return -1;
}

