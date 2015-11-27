CFLAGS = -I/usr/include/libxml2
LDFLAGS =

_OBJS = $(shell ls *.c)
OBJS = $(_OBJS:.c=.o)

%.o:%.c
	@gcc -c ${CFLAGS} $^ -o $@ 

all:${OBJS}
	@echo "Is making net_xml folder ......"


clean:
	rm *.o -f


