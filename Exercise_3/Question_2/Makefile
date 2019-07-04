# _*_ Makefile _*_



OBJ = thread.o

thread: $(OBJ)
	gcc -o thread $(OBJ) -pthread -lrt


thread.o: thread.c
	gcc -c thread.c


clean:
	rm *.o thread

