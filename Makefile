# PLATFORM must one of the following list and is used to select the memcpy/
# memset variants used in the replacement library (libfastarm.so)
# - RPI selects optimizations for the armv6-based Raspberry Pi with a
#   preload offset of 96 bytes.
# - ARMV7_32 selects a cache line size of 32, suitable for most Cortex
#   platforms. The used preload offset is 192 bytes.
# - ARMV7_64 selects a cache line size of 64 bytes, suitable for potential
#   Cortex platforms in which all cache line fills (including from DRAM) are
#   64 bytes. The used preload offset is 192 bytes.
# - NEON_32 selects NEON optimizations with a cache line of 32 bytes.
#   The used preload offset is 192 bytes.
# - NEON_64 selects NEON optimizations with a cahce line size of 64 bytes.
#   The used preload offset is 192 bytes.
# - NEON_AUTO selects NEON optimizations for Cortex cores with a suitably
#   advanced automatic prefetcher that most preload instructions are unnecessary.
#   Only early preloads are generated.
# Uncomment the THUMBFLAGS definition to compile in ARM mode as opposed to Thumb2

PLATFORM = NEON_32
THUMBFLAGS = -march=armv7-a -Wa,-march=armv7-a -mthumb -Wa,-mthumb \
 -Wa,-mimplicit-it=always -mthumb-interwork -DCONFIG_THUMB
BENCHMARK_CONFIG_FLAGS = -DINCLUDE_MEMCPY_HYBRID # -DINCLUDE_LIBARMMEM_MEMCPY
#LIBARMMEM = -larmmem
CORTEX_STRINGS_MEMCPY_HYBRID = memcpy-hybrid.o
CFLAGS = -std=gnu99 -Ofast -Wall $(BENCHMARK_CONFIG_FLAGS)
PCFLAGS = -std=gnu99 -O -Wall $(BENCHMARK_CONFIG_FLAGS) -pg -ggdb

all : benchmark libfastarm.so

benchmark : benchmark.o arm_asm.o new_arm.o $(CORTEX_STRINGS_MEMCPY_HYBRID)
	$(CC) $(CFLAGS) benchmark.o arm_asm.o new_arm.o \
	$(CORTEX_STRINGS_MEMCPY_HYBRID) -o benchmark -lm -lrt $(LIBARMMEM)

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

memcpy-hybrid.o : memcpy-hybrid.S

.c.o : 
	$(CC) -c $(CFLAGS) $< -o $@

.S.o :
	$(CC) -c -s $(CFLAGS) $(THUMBFLAGS) $< -o $@

.c.s :
	$(CC) -S $(CFLAGS) $< -o $@
