#ifndef _IP_H_
#define _IP_H_ 1

#ifdef __cplusplus
extern "C" {
#endif

enum
{
	IPaddrlen = 16,
	IPv4addrlen = 4,
	IPv4off = 12
};

int		isv4(unsigned char*);
uint32_t	parseip(unsigned char*, char*);
char*		v4parseip(unsigned char*, char*);

void		hnputl(void*, uint32_t);
void		hnputs(void*, uint16_t);
uint32_t	nhgetl(void*);
uint16_t	nhgets(void*);

int		v6tov4(unsigned char*, unsigned char*);
void		v4tov6(unsigned char*, unsigned char*);

#define CLASS(p) ((*(uchar*)(p))>>6)

#ifdef __cplusplus
}
#endif
#endif
