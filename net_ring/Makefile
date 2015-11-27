CFLAGS =  -g
LDFLAGS =

_OBJS = $(shell ls *.c)
OBJS = $(_OBJS:.c=.o)

%.o:%.c
	@gcc -c ${CFLAGS} $^ -o $@ 

all:${OBJS}
	@echo "Is making net_ring_common folder ......"


clean:
	rm *.o -f


