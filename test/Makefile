CFLAGS=-Wall

test: test.o ../smm.o

test.o: test.c ../smm.h ut.c
smm.o: ../smm.c ../smm.h

clean:
	rm -f *.o test ../*.o

.PHONY: clean
