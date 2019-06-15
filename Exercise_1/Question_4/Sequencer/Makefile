INCLUDE_DIRS = 
LIB_DIRS = 
CC=gcc

CDEFS=
CFLAGS= -O0 $(INCLUDE_DIRS) $(CDEFS)
LIBS= 

HFILES=
CFILES= question4.c
#CFILES= q4.c

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.c=.o}

all:	question4
#all:	q4

clean:
	-rm -f *.o *.d
	-rm -f perfmon question4
#	-rm -f perfmon q4

distclean:
	-rm -f *.o *.d
	-rm -f question4
#	-rm -f q4

#q4: q4.o
#	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $@.o -lpthread -pthread

question4: question4.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $@.o -lpthread -pthread

depend:

.c.o:
	$(CC) $(CFLAGS) -c $<
