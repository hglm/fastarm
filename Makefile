CFLAGS = -std=gnu99 -Ofast -Wall
PCFLAGS = -std=gnu99 -O -Wall -pg -ggdb

all : libfastarm.a benchmark

libfastarm.a : fastarm.o
	rm -f libfastarm.a
	ar r libfastarm.a fastarm.o

benchmark : benchmark.o /usr/include/fastarm.h
	gcc $(CFLAGS) benchmark.o -o benchmark -lrt -lfastarm

benchmarkp : benchmark.c fastarm.h fastarm.c
	gcc $(PCFLAGS) benchmark.c fastarm.c -o benchmarkp -lc -lrt

install : libfastarm.a fastarm.h
	install -m 0644 fastarm.h /usr/include
	install -m 0644 libfastarm.a /usr/lib

clean :
	rm -f libfastarm.a
	rm -f fastarm.o
	rm -f benchmark
	rm -f benchmark.o
	rm -f benchmarkp

fastarm.o : fastarm.c fastarm.h

benchmark.o : benchmark.c

.c.o : 
	$(CC) -c $(CFLAGS) $< -o $@

.c.s :
	$(CC) -S $(CFLAGS) $< -o $@
