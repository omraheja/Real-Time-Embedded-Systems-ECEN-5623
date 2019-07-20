INCLUDE_DIRS = 
LIB_DIRS = 
CC=gcc

CDEFS=
CFLAGS= -O0 -g $(INCLUDE_DIRS) $(CDEFS)
LIBS= -lrt -lpthread -pthread

HFILES= 
CFILES= Question5.c

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.c=.o}

all:	Question5
	@echo "***********************************STEPS TO RUN CODE******************************************"
	@echo "1. Run myscript.sh as root\n2. Run the below command to execute the code"
	@echo "USAGE: [./<exe name> | /dev/video0 | <frame_count> | <sharpen/ bnw / grayscale> | HRES | VRES]"
	@echo "EXAMPLE: ./Question5 /dev/video0 100 sharpen 640 480"
	@echo "Supported Resolutions:\n1. 960x720\n2. 800x600\n3. 640x480\n4. 320x240\n5. 160x120"
	@echo "**********************************************************************************************"


clean:
	@echo "Deleting all .ppm and .pgm files......"
	-rm -f *.o *.d *.ppm *.pgm *.csv
	-rm -f Question5
	@echo "Deleted!"
	

distclean:
	-rm -f *.o *.d

Question5:	${OBJS}
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $@.o $(LIBS)

depend:

.c.o:
	$(CC) $(CFLAGS) -c $<
