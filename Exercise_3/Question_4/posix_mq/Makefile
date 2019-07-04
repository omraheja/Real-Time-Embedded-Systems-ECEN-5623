# _*_ Makefile _*_



OBJ = posix_msgq.o

posix_msgq: $(OBJ)
	gcc -o posix_msgq $(OBJ) -pthread -lrt


posix_msgq.o: posix_msgq.c
	gcc -c posix_msgq.c


clean:
	rm *.o posix_msgq

