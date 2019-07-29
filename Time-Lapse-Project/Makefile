INCLUDE_DIRS = 
LIB_DIRS = 
CC=gcc

CDEFS=
CFLAGS= -O0 -g $(INCLUDE_DIRS) $(CDEFS)
LIBS= -lrt -lpthread -pthread

HFILES=
CFILES= time_lapse_main.c

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.c=.o}

all:	time_lapse_main
	@echo "***********************************STEPS TO RUN CODE******************************************"
	@echo "1. Run myscript.sh as root\n2. Run the below command to execute the code"
	@echo "USAGE                : [./<exe name> | /dev/video0 | <frame_count> | HRES | VRES]"
	@echo "EXAMPLE              : ./time_lapse_1hz /dev/video0 100 640 480"
	@echo "SUPPORTED RESOLUTIONS: 640x480"
	@echo "**********************************************************************************************"


clean:
	@echo "Deleting all .ppm and .pgm files......"
	-rm -f *.o *.d *.ppm *.pgm *.csv
	-rm -f time_lapse_main
	@echo "Deleted!"
	

distclean:
	-rm -f *.o *.d

time_lapse_main:	${OBJS}
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $@.o $(LIBS)

depend:

.c.o:
	$(CC) $(CFLAGS) -c $<
