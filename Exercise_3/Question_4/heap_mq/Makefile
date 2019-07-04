# _*_ Makefile _*_



OBJ = heap_mq.o

heap_mq: $(OBJ)
	gcc -o heap_mq $(OBJ) -pthread -lrt


heap_mq.o: heap_mq.c
	gcc -c heap_mq.c


clean:
	rm *.o heap_mq

