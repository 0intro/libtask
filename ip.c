#include "taskimpl.h"
#include <ctype.h>
#include <string.h>
#include "ip.h"

void
hnputl(void *p, uint v)
{
	uchar *a;

	a = p;
	a[0] = v>>24;
	a[1] = v>>16;
	a[2] = v>>8;
	a[3] = v;
}

void
hnputs(void *p, ushort v)
{
	uchar *a;

	a = p;
	a[0] = v>>8;
	a[1] = v;
}

uint32_t
nhgetl(void *p)
{
	unsigned char *a;

	a = p;
	return (a[0]<<24)|(a[1]<<16)|(a[2]<<8)|(a[3]<<0);
}

uint16_t
nhgets(void *p)
{
	unsigned char *a;

	a = p;
	return (a[0]<<8)|(a[1]<<0);
}

char*
v4parseip(unsigned char *to, char *from)
{
	int i;
	char *p;

	p = from;
	for(i = 0; i < 4 && *p; i++){
		to[i] = strtoul(p, &p, 0);
		if(*p == '.')
			p++;
	}
	switch(CLASS(to)){
	case 0:	/* class A - 1 uchar net */
	case 1:
		if(i == 3){
			to[3] = to[2];
			to[2] = to[1];
			to[1] = 0;
		} else if (i == 2){
			to[3] = to[1];
			to[1] = 0;
		}
		break;
	case 2:	/* class B - 2 uchar net */
		if(i == 3){
			to[3] = to[2];
			to[2] = 0;
		}
		break;
	}
	return p;
}

static int
ipcharok(int c)
{
	return c == '.' || c == ':' || (isascii(c) && isxdigit(c));
}

static int
delimchar(int c)
{
	if(c == '\0')
		return 1;
	if(c == '.' || c == ':' || (isascii(c) && isalnum(c)))
		return 0;
	return 1;
}

uint32_t
parseip(uchar *to, char *from)
{
	int i, elipsis = 0, v4 = 1;
	uint32_t x;
	char *p, *op;

	memset(to, 0, IPaddrlen);
	p = from;
	for(i = 0; i < IPaddrlen && ipcharok(*p); i+=2){
		op = p;
		x = strtoul(p, &p, 16);
		if(*p == '.' || (*p == 0 && i == 0)){	/* ends with v4? */
			p = v4parseip(to+i, op);
			i += 4;
			break;
		}
		/* v6: at most 4 hex digits, followed by colon or delim */
		if(x != (ushort)x || (*p != ':' && !delimchar(*p))) {
			memset(to, 0, IPaddrlen);
			return -1;			/* parse error */
		}
		to[i] = x>>8;
		to[i+1] = x;
		if(*p == ':'){
			v4 = 0;
			if(*++p == ':'){	/* :: is elided zero short(s) */
				if (elipsis) {
					memset(to, 0, IPaddrlen);
					return -1;	/* second :: */
				}
				elipsis = i+2;
				p++;
			}
		} else if (p == op)		/* strtoul made no progress? */
			break;
	}
	if (p == from || !delimchar(*p)) {
		memset(to, 0, IPaddrlen);
		return -1;				/* parse error */
	}
	if(i < IPaddrlen){
		memmove(&to[elipsis+IPaddrlen-i], &to[elipsis], i-elipsis);
		memset(&to[elipsis], 0, IPaddrlen-i);
	}
	if(v4){
		to[10] = to[11] = 0xff;
		return nhgetl(to + IPv4off);
	} else
		return 6;
}

uchar IPnoaddr[IPaddrlen];

uchar v4prefix[IPaddrlen] = {
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0xff, 0xff,
	0, 0, 0, 0
};

int
isv4(uchar *ip)
{
	return memcmp(ip, v4prefix, IPv4off) == 0;
}

void
v4tov6(unsigned char *v6, unsigned char *v4)
{
	v6[0] = 0;
	v6[1] = 0;
	v6[2] = 0;
	v6[3] = 0;
	v6[4] = 0;
	v6[5] = 0;
	v6[6] = 0;
	v6[7] = 0;
	v6[8] = 0;
	v6[9] = 0;
	v6[10] = 0xff;
	v6[11] = 0xff;
	v6[12] = v4[0];
	v6[13] = v4[1];
	v6[14] = v4[2];
	v6[15] = v4[3];
}

int
v6tov4(unsigned char *v4, unsigned char *v6)
{
	if(v6[0] == 0
	&& v6[1] == 0
	&& v6[2] == 0
	&& v6[3] == 0
	&& v6[4] == 0
	&& v6[5] == 0
	&& v6[6] == 0
	&& v6[7] == 0
	&& v6[8] == 0
	&& v6[9] == 0
	&& v6[10] == 0xff
	&& v6[11] == 0xff)
	{
		v4[0] = v6[12];
		v4[1] = v6[13];
		v4[2] = v6[14];
		v4[3] = v6[15];
		return 0;
	} else {
		memset(v4, 0, 4);
		if(memcmp(v6, IPnoaddr, IPaddrlen) == 0)
			return 0;
		return -1;
	}
}
