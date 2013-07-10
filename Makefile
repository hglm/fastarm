# PLATFORM must be SUNXI or RPI and is used to select the memcpy
# variant used in the memcpy replacement library (libfastarm.so)
# Uncomment the THUMBFLAGS definition to compile in ARM mode as opposed to Thumb2

PLATFORM = SUNXI
THUMBFLAGS = -march=armv7-a -Wa,-march=armv7-a -mthumb -Wa,-mthumb \
 -Wa,-mimplicit-it=always -mthumb-interwork -DCONFIG_THUMB
BENCHMARK_CONFIG_FLAGS = # -DINCLUDE_LIBARMMEM_MEMCPY
#LIBARMMEM = -larmmem
CFLAGS = -std=gnu99 -Ofast -Wall $(BENCHMARK_CONFIG_FLAGS)
PCFLAGS = -std=gnu99 -O -Wall $(BENCHMARK_CONFIG_FLAGS) -pg -ggdb

all : benchmark libfastarm.so

benchmark : benchmark.o arm_asm.o new_arm.o
	$(CC) $(CFLAGS) benchmark.o arm_asm.o new_arm.o -o benchmark -lm -lrt $(LIBARMMEM)

benchmarkp : benchmark.c arm_asm.S
	$(CC) $(PCFLAGS) benchmark.c arm_asm.S new_arm.S -o benchmarkp -lc -lm -lrt $(LIBARMMEM)

install_memcpy_replacement : libfastarm.so
	install -m 0755 libfastarm.so /usr/lib/arm-linux-gnueabihf/libfastarm.so
	@echo 'To enable the use of the enhanced memcpy by applications, edit or'
	@echo 'create the file /etc/ld.so.preload so that it contains the line:'
	@echo '/usr/lib/arm-linux-gnueabihf/libfastarm.so'
	@echo 'On the RPi platform, references to libcofi_rpi.so should be commented'
	@echo 'out or deleted.'

libfastarm.so : memcpy_replacement.o
	$(CC) -o libfastarm.so -shared memcpy_replacement.o

memcpy_replacement.o : new_arm.S
	$(CC) -c -s -x assembler-with-cpp $(THUMBFLAGS) \
-DMEMCPY_REPLACEMENT_$(PLATFORM) -DMEMSET_REPLACEMENT_$(PLATFORM) \
-o memcpy_replacement.o new_arm.S

clean :
	rm -f benchmark
	rm -f benchmark.o
	rm -f benchmarkp
	rm -f arm_asm.s
	rm -f arm_asm.o
	rm -f new_arm.o
	rm -f memcpy_replacement.o
	rm -f libfastarm.so

benchmark.o : benchmark.c arm_asm.h

arm_asm.o : arm_asm.S arm_asm.h

new_arm.o : new_arm.S new_arm.h

.c.o : 
	$(CC) -c $(CFLAGS) $< -o $@

.S.o :
	$(CC) -c -s $(CFLAGS) $(THUMBFLAGS) $< -o $@

.c.s :
	$(CC) -S $(CFLAGS) $< -o $@
