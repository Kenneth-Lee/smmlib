CFLAGS = -Wall
smm.o: smm.c

clean:
	rm -f *.o
	make -C test clean

.PHONY: clean
