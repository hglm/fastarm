CONFIG_FLAGS =
CFLAGS = -std=gnu99 -Ofast -Wall $(CONFIG_FLAGS)
PCFLAGS = -std=gnu99 -O -Wall $(CONFIG_FLAGS) -pg -ggdb

all : libfastarm.a benchmark

libfastarm.a : fastarm.o
	rm -f libfastarm.a
	ar r libfastarm.a fastarm.o

benchmark : benchmark.o arm_asm.o libfastarm.a fastarm.h arm_asm.h
	gcc $(CFLAGS) benchmark.o arm_asm.o -o benchmark libfastarm.a -lm -lrt

benchmarkp : benchmark.c arm_asm.S fastarm.h fastarm.c
	gcc $(PCFLAGS) benchmark.c arm_asm.S fastarm.c -o benchmarkp -lc -lm -lrt

install : libfastarm.a fastarm.h
	install -m 0644 fastarm.h /usr/include
	install -m 0644 libfastarm.a /usr/lib

clean :
	rm -f libfastarm.a
	rm -f fastarm.o
	rm -f benchmark
	rm -f benchmark.o
	rm -f benchmarkp
	rm -f arm_asm.o

fastarm.o : fastarm.c fastarm.h

benchmark.o : benchmark.c

arm_asm.o : arm_asm.S

.c.o : 
	$(CC) -c $(CFLAGS) $< -o $@

.S.o :
	$(CC) -c -s $(CFLAGS) $< -o $@

.c.s :
	$(CC) -S $(CFLAGS) $< -o $@
