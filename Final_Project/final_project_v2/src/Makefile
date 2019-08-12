#_*_ Makefile _*_

CC=gcc

CFLAGS = -I../inc/ -pthread -lrt -Werror

vpath %.h ../inc/

OBJ = main.o image_capture.o print_scheduler.o v4l2_driver.o ppm_dump.o image_send.o sequencer.o client.o

time_lapse: $(OBJ)
	$(CC) -o time_lapse $(OBJ) -lpthread -lrt
	@echo "***********************************STEPS TO RUN CODE******************************************"
	@echo "1. Run myscript.sh as root\n2. Run the below command to execute the code"
	@echo "USAGE                : [./<exe name> | /dev/video0 | <frame_count> | HRES | VRES | Frequency(1hz or 10 hz) | Socket Enable (1)/Disable (0)]"
	@echo "EXAMPLE              : ./time_lapse /dev/video0 1800 640 480 1 1"	#1 Hertz Socket Enabled	
	@echo "EXAMPLE              : ./time_lapse /dev/video0 1800 640 480 1 0"	#1 Hertz Socket Disabled
	@echo "EXAMPLE              : ./time_lapse /dev/video0 6000 640 480 10 0"	#10 Hertz Socket Disabled
	@echo "SUPPORTED RESOLUTIONS: 640x480"
	@echo "**********************************************************************************************"


main.o: main.c main.h
	$(CC) -c -g main.c $(CFLAGS)

image_capture.o: image_capture.c image_capture.h
	$(CC) -c -g image_capture.c $(CFLAGS)

print_scheduler.o: print_scheduler.c print_scheduler.h
	$(CC) -c -g print_scheduler.c $(CFLAGS)

v4l2_driver.o: v4l2_driver.c v4l2_driver.h
	$(CC) -c -g v4l2_driver.c $(CFLAGS)

ppm_dump.o: ppm_dump.c ppm_dump.h
	$(CC) -c -g ppm_dump.c $(CFLAGS)

image_send.o: image_send.c image_send.h
	$(CC) -c -g image_send.c $(CFLAGS)

sequencer.o: sequencer.c sequencer.h
	$(CC) -c -g sequencer.c $(CFLAGS)

client.o: client.c client.h
	$(CC) -c -g client.c $(CFLAGS)


clean:
	@echo "Deleting all .ppm and .pgm files......"
	-rm -f *.o *.d *.ppm *.pgm *.csv *.mp4
	-rm -f time_lapse
	@echo "Deleted!"
