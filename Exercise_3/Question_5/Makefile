# _*_ Makefile _*_



OBJ = watchdog.o

watchdog: $(OBJ)
	gcc -o watchdog $(OBJ) -pthread -lrt


watchdog.o: watchdog.c
	gcc -c watchdog.c


clean:
	rm *.o watchdog

