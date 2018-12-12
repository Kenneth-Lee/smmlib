CFLAGS=-Wall

test: test.o ../smm.o

test.o: test.c ../smm.h
smm.o: ../smm.c ../smm.h

clean:
	rm -f *.o test

.PHONY: clean
