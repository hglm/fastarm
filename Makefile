CFLAGS = -std=gnu99 -O2 -Wall

all : libfastarm.a benchmark

libfastarm.a : fastarm.o
	rm -f libfastarm.a
	ar r libfastarm.a fastarm.o

benchmark : benchmark.o
	gcc $(CFLAGS) benchmark.o -o benchmark -lrt -lfastarm

install : libfastarm.a fastarm.h
	install -m 0644 fastarm.h /usr/include
	install -m 0644 libfastarm.a /usr/lib

clean :
	rm -f libfastarm.a
	rm -f fastarm.o
	rm -f benchmark
	rm -f benchmark.o

fastarm.o : fastarm.c fastarm.h

benchmark.o : benchmark.c

.c.o : 
	$(CC) -c $(CFLAGS) $< -o $@
