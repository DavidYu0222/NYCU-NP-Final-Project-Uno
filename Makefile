include ../Make.defines

PROGS =	serv cli standalone_uno\

all:	${PROGS}

standalone_uno: standalone_uno.o
		${CC} ${CFLAGS} -o $@ standalone_uno.o ${LIBS}

serv:	serv1.o uno.o
		${CC} ${CFLAGS} -o $@ serv1.o uno.o ${LIBS}

cli:	cli.o
		${CC} ${CFLAGS} -o $@ cli.o ${LIBS}


clean:
		rm -f ${PROGS} ${CLEANFILES}
