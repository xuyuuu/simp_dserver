CFLAGS=
LDFLAGS=-L../net_lib/ -lpthread -levent -lxml2 -lrt
BIN=simp_dserver

OBJS=$(shell ls | grep "\.o$ ")

all:${BIN}

${BIN}:${OBJS}
	@gcc $^ ${CFLAGS} ${LDFLAGS} -o $@

clean:
	rm *.o ${BIN} -rf
