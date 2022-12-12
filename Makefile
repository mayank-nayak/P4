CC     := gcc
CFLAGS := -Wall -Werror

SRCS   := server.c mkfs.c client.c \

OBJS   := ${SRCS:c=o}
PROGS  := ${SRCS:.c=}

.PHONY: all
all: ${PROGS}

${PROGS} : % : %.o Makefile libmfs.so
	${CC} $< -o $@ udp.c -lmfs -L.

libmfs.so: libmfs.c
	${CC} ${CFLAGS} -o libmfs.so -fPIC -shared libmfs.c

clean:
	rm -f ${PROGS} ${OBJS} libmfs.so

%.o: %.c Makefile
	${CC} ${CFLAGS} -c $<

