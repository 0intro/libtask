LIB=libtask.a
TCPLIBS=

ASM=asm.o
OFILES=\
	$(ASM)\
	channel.o\
	context.o\
	fd.o\
	net.o\
	print.o\
	qlock.o\
	rendez.o\
	task.o\
	ip.o\

all: $(LIB) echo httpload primes tcpload tcpproxy testdelay

$(OFILES): taskimpl.h task.h 386-ucontext.h power-ucontext.h ip.h

AS=gcc -c
CC=gcc
CFLAGS=-Wall -Wextra -c -I. -ggdb
#CFLAGS+=-DUSE_VALGRIND -I/usr/include/valgrind

%.o: %.S
	$(AS) $*.S

%.o: %.c
	$(CC) $(CFLAGS) $*.c

$(LIB): $(OFILES)
	ar rvc $(LIB) $(OFILES)

echo: echo.o $(LIB)
	$(CC) -o echo echo.o $(LIB)

primes: primes.o $(LIB)
	$(CC) -o primes primes.o $(LIB)

tcpload: tcpload.o $(LIB)
	$(CC) -o tcpload tcpload.o $(LIB)

tcpproxy: tcpproxy.o $(LIB)
	$(CC) -o tcpproxy tcpproxy.o $(LIB) $(TCPLIBS)

httpload: httpload.o $(LIB)
	$(CC) -o httpload httpload.o $(LIB)

testdelay: testdelay.o $(LIB)
	$(CC) -o testdelay testdelay.o $(LIB)

testdelay1: testdelay1.o $(LIB)
	$(CC) -o testdelay1 testdelay1.o $(LIB)

clean:
	rm -f *.o echo primes tcpload tcpproxy httpload testdelay testdelay1 $(LIB)

install: $(LIB)
	cp $(LIB) /usr/local/lib
	cp task.h /usr/local/include

