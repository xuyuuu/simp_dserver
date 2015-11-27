CFLAGS =  -g
LDFLAGS =

_OBJS = $(shell ls *.c)
OBJS = $(_OBJS:.c=.o)

%.o:%.c
	@gcc -c ${CFLAGS} $^ -o $@ 

all:${OBJS}
	@echo "Is making net_main folder ......"


clean:
	rm *.o -f


