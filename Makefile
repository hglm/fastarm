CONFIG_FLAGS = # -DINCLUDE_LIBARMMEM_MEMCPY
#LIBARMMEM = -larmmem
CFLAGS = -std=gnu99 -Ofast -Wall $(CONFIG_FLAGS)
PCFLAGS = -std=gnu99 -O -Wall $(CONFIG_FLAGS) -pg -ggdb

all : benchmark

benchmark : benchmark.o arm_asm.o
	gcc $(CFLAGS) benchmark.o arm_asm.o -o benchmark -lm -lrt $(LIBARMMEM)

benchmarkp : benchmark.c arm_asm.S
	gcc $(PCFLAGS) benchmark.c arm_asm.S  -o benchmarkp -lc -lm -lrt $(LIBARMMEM)

clean :
	rm -f benchmark
	rm -f benchmark.o
	rm -f benchmarkp
	rm -f arm_asm.o

benchmark.o : benchmark.c arm_asm.h

arm_asm.o : arm_asm.S arm_asm.h

.c.o : 
	$(CC) -c $(CFLAGS) $< -o $@

.S.o :
	$(CC) -c -s $(CFLAGS) $< -o $@

.c.s :
	$(CC) -S $(CFLAGS) $< -o $@
