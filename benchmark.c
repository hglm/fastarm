/*
 * Copyright (C) 2013 Harm Hanemaaijer <fgenfb@yahoo.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>

#include "fastarm.h"
#include "arm_asm.h"

#define DEFAULT_TEST_DURATION 2.0
#define RANDOM_BUFFER_SIZE 256
#define NU_MEMCPY_VARIANTS 37

typedef void *(*memcpy_func_type)(void *dest, const void *src, size_t n);

memcpy_func_type memcpy_func;
uint8_t *buffer_alloc, *buffer_chunk, *buffer_page, *buffer_compare;
int *random_buffer_1024;
double test_duration = DEFAULT_TEST_DURATION;
int memcpy_mask[NU_MEMCPY_VARIANTS];

static const char *memcpy_variant_name[NU_MEMCPY_VARIANTS] = {
    "standard memcpy",
    "libfastarm memcpy",
    "armv5te memcpy",
    "armv5te non-overfetching memcpy with write alignment of 16 and block write size of 8, preload offset 96",
    "armv5te non-overfetching memcpy with write alignment of 16 and block write size of 16, preload offset 96",
    "armv5te non-overfetching memcpy with write alignment of 16 and block write size of 16, preload offset 96 with early preload",
    "armv5te non-overfetching memcpy with write alignment of 16 and block write size of 16, preload offset 128 with early preload",
    "armv5te non-overfetching memcpy with write alignment of 32 and block write size of 8, preload offset 96",
    "armv5te non-overfetching memcpy with write alignment of 32 and block write size of 16, preload offset 64",
    "armv5te non-overfetching memcpy with write alignment of 32 and block write size of 16, preload offset 96",
    "armv5te non-overfetching memcpy with write alignment of 32 and block write size of 16, preload offset 128",
    "armv5te non-overfetching memcpy with write alignment of 32 and block write size of 16, preload offset 160",
    "armv5te non-overfetching memcpy with write alignment of 32 and block write size of 16, preload offset 192",
    "armv5te non-overfetching memcpy with write alignment of 32 and block write size of 16, preload offset 256",
    "armv5te non-overfetching memcpy with write alignment of 32 and block write size of 32, preload offset 64",
    "armv5te non-overfetching memcpy with write alignment of 32 and block write size of 32, preload offset 96",
    "armv5te non-overfetching memcpy with write alignment of 32 and block write size of 32, preload offset 128",
    "armv5te non-overfetching memcpy with write alignment of 32 and block write size of 32, preload offset 160",
    "armv5te non-overfetching memcpy with write alignment of 32 and block write size of 32, preload offset 192",
    "armv5te non-overfetching memcpy with write alignment of 32 and block write size of 32, preload offset 256",
    "armv5te non-overfetching memcpy with write alignment of 32 and block write size of 16, preload offset 96 with early preload",
    "armv5te non-overfetching memcpy with write alignment of 32 and block write size of 16, preload offset 128 with early preload",
    "armv5te non-overfetching memcpy with write alignment of 32 and block write size of 16, preload offset 192 with early preload",
    "armv5te non-overfetching memcpy with write alignment of 32 and block write size of 16, preload offset 256 with early preload",
    "armv5te non-overfetching memcpy with write alignment of 32 and block write size of 32, preload offset 128 with early preload",
    "armv5te non-overfetching memcpy with write alignment of 32 and block write size of 32, preload offset 192 with early preload",
    "armv5te non-overfetching memcpy with write alignment of 32 and block write size of 32, preload offset 256 with early preload",
    "armv5te non-overfetching memcpy with write alignment of 32 and block write size of 16, no preload",
    "armv5te non-overfetching memcpy with write alignment of 32 and block write size of 32, no preload",
    "armv5te non-overfetching memcpy with line size of 64, write alignment of 32 and block write size of 32, preload offset 128 with early preload",
    "armv5te non-overfetching memcpy with line size of 64, write alignment of 32 and block write size of 32, preload offset 192 with early preload",
    "armv5te non-overfetching memcpy with line size of 64, write alignment of 32 and block write size of 32, preload offset 256 with early preload",
    "armv5te non-overfetching memcpy with line_size of 64, write alignment of 32 and block write size of 32, preload offset 320 with early preload",
    "armv5te non-overfetching memcpy with line size of 64, write alignment of 64 and block write size of 32, preload offset 256 with early preload",
    "armv5te non-overfetching memcpy with line_size of 64, write alignment of 64 and block write size of 32, preload offset 320 with early preload",
    "armv5te overfetching memcpy with write alignment of 16 and block write size of 16, preload offset 128 with early preload",
    "armv5te overfetching memcpy with write alignment of 32 and block write size of 32, preload offset 192 with early preload"
};

static const memcpy_func_type memcpy_variant[NU_MEMCPY_VARIANTS] = {
    memcpy,
    fastarm_memcpy,
    memcpy_armv5te,
    memcpy_armv5te_no_overfetch_align_16_block_write_8_preload_96,
    memcpy_armv5te_no_overfetch_align_16_block_write_16_preload_96,
    memcpy_armv5te_no_overfetch_align_16_block_write_16_preload_early_96,
    memcpy_armv5te_no_overfetch_align_16_block_write_16_preload_early_128,
    memcpy_armv5te_no_overfetch_align_32_block_write_8_preload_96,
    memcpy_armv5te_no_overfetch_align_32_block_write_16_preload_64,
    memcpy_armv5te_no_overfetch_align_32_block_write_16_preload_96,
    memcpy_armv5te_no_overfetch_align_32_block_write_16_preload_128,
    memcpy_armv5te_no_overfetch_align_32_block_write_16_preload_160,
    memcpy_armv5te_no_overfetch_align_32_block_write_16_preload_192,
    memcpy_armv5te_no_overfetch_align_32_block_write_16_preload_256,
    memcpy_armv5te_no_overfetch_align_32_block_write_32_preload_64,
    memcpy_armv5te_no_overfetch_align_32_block_write_32_preload_96,
    memcpy_armv5te_no_overfetch_align_32_block_write_32_preload_128,
    memcpy_armv5te_no_overfetch_align_32_block_write_32_preload_160,
    memcpy_armv5te_no_overfetch_align_32_block_write_32_preload_192,
    memcpy_armv5te_no_overfetch_align_32_block_write_32_preload_256,
    memcpy_armv5te_no_overfetch_align_32_block_write_16_preload_early_96,
    memcpy_armv5te_no_overfetch_align_32_block_write_16_preload_early_128,
    memcpy_armv5te_no_overfetch_align_32_block_write_16_preload_early_192,
    memcpy_armv5te_no_overfetch_align_32_block_write_16_preload_early_256,
    memcpy_armv5te_no_overfetch_align_32_block_write_32_preload_early_128,
    memcpy_armv5te_no_overfetch_align_32_block_write_32_preload_early_192,
    memcpy_armv5te_no_overfetch_align_32_block_write_32_preload_early_256,
    memcpy_armv5te_no_overfetch_align_32_block_write_16_no_preload,
    memcpy_armv5te_no_overfetch_align_32_block_write_32_no_preload,
    memcpy_armv5te_no_overfetch_line_64_align_32_block_write_32_preload_early_128,
    memcpy_armv5te_no_overfetch_line_64_align_32_block_write_32_preload_early_192,
    memcpy_armv5te_no_overfetch_line_64_align_32_block_write_32_preload_early_256,
    memcpy_armv5te_no_overfetch_line_64_align_32_block_write_32_preload_early_320,
    memcpy_armv5te_no_overfetch_line_64_align_64_block_write_32_preload_early_256,
    memcpy_armv5te_no_overfetch_line_64_align_64_block_write_32_preload_early_320,
    memcpy_armv5te_overfetch_align_16_block_write_16_preload_early_128,
    memcpy_armv5te_overfetch_align_32_block_write_32_preload_early_192
};

static double get_time() {
   struct timespec ts;
   clock_gettime(CLOCK_REALTIME, &ts);
   return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
}

static void test_unaligned_random_3(int i) {
    memcpy_func(buffer_page + random_buffer_1024[(i * 2) & (RANDOM_BUFFER_SIZE - 1)],
        buffer_page + 4096 + random_buffer_1024[(i * 2 + 1) & (RANDOM_BUFFER_SIZE - 1)],
        3);
}

static void test_unaligned_random_8(int i) {
    memcpy_func(buffer_page + random_buffer_1024[(i * 2) & (RANDOM_BUFFER_SIZE - 1)],
        buffer_page + 4096 + random_buffer_1024[(i * 2 + 1) & (RANDOM_BUFFER_SIZE - 1)],
        8);
}

static void test_unaligned_random_17(int i) {
    memcpy_func(buffer_page + random_buffer_1024[(i * 2) & (RANDOM_BUFFER_SIZE - 1)],
        buffer_page + 4096 + random_buffer_1024[(i * 2 + 1) & (RANDOM_BUFFER_SIZE - 1)],
        17);
}

static void test_unaligned_random_30(int i) {
    memcpy_func(buffer_page + random_buffer_1024[(i * 2) & (RANDOM_BUFFER_SIZE - 1)],
        buffer_page + 4096 + random_buffer_1024[(i * 2 + 1) & (RANDOM_BUFFER_SIZE - 1)],
        30);
}

static void test_unaligned_random_64(int i) {
    memcpy_func(buffer_page + random_buffer_1024[(i * 2) & (RANDOM_BUFFER_SIZE - 1)],
        buffer_page + 4096 + random_buffer_1024[(i * 2 + 1) & (RANDOM_BUFFER_SIZE - 1)],
        64);
}

static void test_unaligned_random_137(int i) {
    memcpy_func(buffer_page + random_buffer_1024[(i * 2) & (RANDOM_BUFFER_SIZE - 1)],
        buffer_page + 4096 + random_buffer_1024[(i * 2 + 1) & (RANDOM_BUFFER_SIZE - 1)],
        137);
}

static void test_unaligned_random_1024(int i) {
    memcpy_func(buffer_page + random_buffer_1024[(i * 2) & (RANDOM_BUFFER_SIZE - 1)],
        buffer_page + 4096 + random_buffer_1024[(i * 2 + 1) & (RANDOM_BUFFER_SIZE - 1)],
        1024);
}

static void test_unaligned_random_32768(int i) {
    memcpy_func(buffer_page + random_buffer_1024[(i * 2) & (RANDOM_BUFFER_SIZE - 1)],
        buffer_page + 65536 + random_buffer_1024[(i * 2 + 1) & (RANDOM_BUFFER_SIZE - 1)],
        32768);
}

static void test_unaligned_random_1M(int i) {
    memcpy_func(buffer_page + random_buffer_1024[(i * 2) & (RANDOM_BUFFER_SIZE - 1)],
        buffer_page + 2 * 1024 * 1024 +
        random_buffer_1024[(i * 2 + 1) & (RANDOM_BUFFER_SIZE - 1)],
        1024 * 1024);
}

static void test_source_dest_aligned_random_64(int i) {
    memcpy_func(buffer_page + random_buffer_1024[i & (RANDOM_BUFFER_SIZE - 1)],
        buffer_page + 4096 + random_buffer_1024[i & (RANDOM_BUFFER_SIZE - 1)],
        64);
}

static void test_source_dest_aligned_random_1024(int i) {
    memcpy_func(buffer_page + random_buffer_1024[i & (RANDOM_BUFFER_SIZE - 1)],
        buffer_page + 4096 + random_buffer_1024[i & (RANDOM_BUFFER_SIZE - 1)],
        1024);
}

static void test_source_dest_aligned_random_32768(int i) {
    memcpy_func(buffer_page + random_buffer_1024[i & (RANDOM_BUFFER_SIZE - 1)],
        buffer_page + 65536 + random_buffer_1024[i & (RANDOM_BUFFER_SIZE - 1)],
        32768);
}

static void test_source_dest_aligned_random_1M(int i) {
    memcpy_func(buffer_page + random_buffer_1024[i & (RANDOM_BUFFER_SIZE - 1)],
        buffer_page + 2 * 1024 * 1024 +
        random_buffer_1024[i & (RANDOM_BUFFER_SIZE - 1)],
        1024 * 1024);
}

static void test_word_aligned_30(int i) {
    memcpy_func(buffer_page + random_buffer_1024[(i * 2) & (RANDOM_BUFFER_SIZE - 1)] * 4,
        buffer_page + 64 * 1024 + random_buffer_1024[(i * 2 + 1) & (RANDOM_BUFFER_SIZE - 1)] * 4,
        30);
}

static void test_word_aligned_64(int i) {
    memcpy_func(buffer_page + random_buffer_1024[(i * 2) & (RANDOM_BUFFER_SIZE - 1)] * 4,
        buffer_page + 64 * 1024 + random_buffer_1024[(i * 2 + 1) & (RANDOM_BUFFER_SIZE - 1)] * 4,
        64);
}

static void test_word_aligned_296(int i) {
    memcpy_func(buffer_page + random_buffer_1024[(i * 2) & (RANDOM_BUFFER_SIZE - 1)] * 4,
        buffer_page + 64 * 1024 + random_buffer_1024[(i * 2 + 1) & (RANDOM_BUFFER_SIZE - 1)] * 4,
        296);
}

static void test_word_aligned_1024(int i) {
    memcpy_func(buffer_page + random_buffer_1024[(i * 2) & (RANDOM_BUFFER_SIZE - 1)] * 4,
        buffer_page + 64 * 1024 + random_buffer_1024[(i * 2 + 1) & (RANDOM_BUFFER_SIZE - 1)] * 4,
        1024);
}

static void test_word_aligned_4096(int i) {
    memcpy_func(buffer_page + random_buffer_1024[(i * 2) & (RANDOM_BUFFER_SIZE - 1)] * 4,
        buffer_page + 64 * 1024 + random_buffer_1024[(i * 2 + 1) & (RANDOM_BUFFER_SIZE - 1)] * 4,
        4096);
}

static void test_word_aligned_32768(int i) {
    memcpy_func(buffer_page + random_buffer_1024[(i * 2) & (RANDOM_BUFFER_SIZE - 1)] * 4,
        buffer_page + 128 * 1024 + random_buffer_1024[(i * 2 + 1) & (RANDOM_BUFFER_SIZE - 1)] * 4,
        32768);
}

static void test_chunk_aligned_64(int i) {
    memcpy_func(buffer_chunk + random_buffer_1024[(i * 2) & (RANDOM_BUFFER_SIZE - 1)] * 32,
        buffer_chunk + 64 * 1024 + random_buffer_1024[(i * 2 + 1) & (RANDOM_BUFFER_SIZE - 1)] * 32,
        64);
}

static void test_chunk_aligned_296(int i) {
    memcpy_func(buffer_chunk + random_buffer_1024[(i * 2) & (RANDOM_BUFFER_SIZE - 1)] * 32,
        buffer_chunk + 64 * 1024 + random_buffer_1024[(i * 2 + 1) & (RANDOM_BUFFER_SIZE - 1)] * 32,
        296);
}

static void test_chunk_aligned_1024(int i) {
    memcpy_func(buffer_chunk + random_buffer_1024[(i * 2) & (RANDOM_BUFFER_SIZE - 1)] * 32,
        buffer_chunk + 64 * 1024 + random_buffer_1024[(i * 2 + 1) & (RANDOM_BUFFER_SIZE - 1)] * 32,
        1024);
}

static void test_chunk_aligned_4096(int i) {
    memcpy_func(buffer_chunk + random_buffer_1024[(i * 2) & (RANDOM_BUFFER_SIZE - 1)] * 32,
        buffer_chunk + 64 * 1024 + random_buffer_1024[(i * 2 + 1) & (RANDOM_BUFFER_SIZE - 1)] * 32,
        4096);
}

static void test_chunk_aligned_32768(int i) {
    memcpy_func(buffer_chunk + random_buffer_1024[(i * 2) & (RANDOM_BUFFER_SIZE - 1)] * 32,
        buffer_chunk + 128 * 1024 + random_buffer_1024[(i * 2 + 1) & (RANDOM_BUFFER_SIZE - 1)] * 32,
        32768);
}

static void test_page_aligned_1024(int i) {
    memcpy_func(buffer_page + random_buffer_1024[(i * 2) & (RANDOM_BUFFER_SIZE - 1)] * 4096,
        buffer_page + 8192 * 1024 + random_buffer_1024[(i * 2 + 1) & (RANDOM_BUFFER_SIZE - 1)] * 4096,
        1024);
}

static void test_page_aligned_4096(int i) {
    memcpy_func(buffer_page + random_buffer_1024[(i * 2) & (RANDOM_BUFFER_SIZE - 1)] * 4096,
        buffer_page + 8192 * 1024 + random_buffer_1024[(i * 2 + 1) & (RANDOM_BUFFER_SIZE - 1)] * 4096,
        4096);
}

static void test_page_aligned_32768(int i) {
    memcpy_func(buffer_page + random_buffer_1024[(i * 2) & (RANDOM_BUFFER_SIZE - 1)] * 4096,
        buffer_page + 8192 * 1024 + random_buffer_1024[(i * 2 + 1) & (RANDOM_BUFFER_SIZE - 1)] * 4096,
        32768);
}

static void test_page_aligned_256K(int i) {
    memcpy_func(buffer_page + random_buffer_1024[(i * 2) & (RANDOM_BUFFER_SIZE - 1)] * 4096,
        buffer_page + 8192 * 1024 + random_buffer_1024[(i * 2 + 1) & (RANDOM_BUFFER_SIZE - 1)] * 4096,
        256 * 1024);
}

static void test_page_aligned_1M(int i) {
    memcpy_func(buffer_page + random_buffer_1024[(i * 2) & (RANDOM_BUFFER_SIZE - 1)] * 4096,
        buffer_page + 8192 * 1024 + random_buffer_1024[(i * 2) & (RANDOM_BUFFER_SIZE - 1)] * 4096,
        1024 * 1024);
}

static void test_page_aligned_8M(int i) {
    memcpy_func(buffer_page + random_buffer_1024[(i * 2) & (RANDOM_BUFFER_SIZE - 1)] * 4096,
        buffer_page + 16384 * 1024 + random_buffer_1024[(i * 2 + 1) & (RANDOM_BUFFER_SIZE - 1)] * 4096,
        8 * 1024 * 1024);
}

static void test_random_mixed_sizes_1024(int i) {
    memcpy_func(buffer_page + random_buffer_1024[(i * 4) & (RANDOM_BUFFER_SIZE - 1)],
        buffer_page + 4096 + random_buffer_1024[(i * 4 + 1) & (RANDOM_BUFFER_SIZE - 1)],
        1 + random_buffer_1024[((i * 4 + 2) & (RANDOM_BUFFER_SIZE - 1))]);
}

static void test_random_mixed_sizes_64(int i) {
    memcpy_func(buffer_page + random_buffer_1024[(i * 4) & (RANDOM_BUFFER_SIZE - 1)],
        buffer_page + 4096 + random_buffer_1024[(i * 4 + 1) & (RANDOM_BUFFER_SIZE - 1)],
        1 + (random_buffer_1024[((i * 4 + 2) & (RANDOM_BUFFER_SIZE - 1))] & 63));
}

static void test_random_mixed_sizes_DRAM_1024(int i) {
    /* Source and destination address selected randomly from range of 8MB. */
    memcpy_func(buffer_page +
        // Select a random 8192 bytes aligned addres.
        8192 * random_buffer_1024[(i * 2) & (RANDOM_BUFFER_SIZE - 1)] +
        // Add a random offset up to (4096 - 256) in steps of 256 based on higher bits
        // of the iteration number.
        ((i / (RANDOM_BUFFER_SIZE / 4)) & 15) * 256 +
        // Add a random offset up to 1023 in steps of 1 based on the lower end bits
        // of the iteration number.
        random_buffer_1024[(i * 4) & (RANDOM_BUFFER_SIZE - 1)],
        buffer_page +
        8192 * random_buffer_1024[(i * 2 + 1) & (RANDOM_BUFFER_SIZE - 1)] +
        ((i / (RANDOM_BUFFER_SIZE / 4)) & 15) * 256 +
        random_buffer_1024[(i * 4 + 1) & (RANDOM_BUFFER_SIZE - 1)],
        1 + random_buffer_1024[((i * 4 + 2) & (RANDOM_BUFFER_SIZE - 1))]);
}

static void test_random_mixed_sizes_DRAM_64(int i) {
    /* Source and destination address selected randomly from range of 8MB. */
    memcpy_func(buffer_page +
        8192 * random_buffer_1024[(i * 2) & (RANDOM_BUFFER_SIZE - 1)] +
        ((i / (RANDOM_BUFFER_SIZE / 4)) & 15) * 256 +
        random_buffer_1024[(i * 4) & (RANDOM_BUFFER_SIZE - 1)],
        buffer_page +
        8192 * random_buffer_1024[(i * 2 + 1) & (RANDOM_BUFFER_SIZE - 1)] +
        ((i / (RANDOM_BUFFER_SIZE / 4)) & 15) * 256 +
        random_buffer_1024[(i * 4 + 1) & (RANDOM_BUFFER_SIZE - 1)],
        1 + (random_buffer_1024[((i * 4 + 2) & (RANDOM_BUFFER_SIZE - 1))] & 63));
}

static void test_random_mixed_sizes_DRAM_word_aligned_1024(int i) {
    /* Source and destination address selected randomly from range of 8MB. */
    memcpy_func(buffer_page +
        // Select a random 8192 bytes aligned addres.
        8192 * random_buffer_1024[(i * 2) & (RANDOM_BUFFER_SIZE - 1)] +
        // Add a random offset up to (4096 - 256) in steps of 256 based on higher bits
        // of the iteration number.
        ((i / (RANDOM_BUFFER_SIZE / 4)) & 15) * 256 +
        // Add a random offset up to 1020 in steps of 4 based on the lower end bits
        // of the iteration number.
        (random_buffer_1024[(i * 4) & (RANDOM_BUFFER_SIZE - 1)] & (~3)),
        buffer_page +
        8192 * random_buffer_1024[(i * 2 + 1) & (RANDOM_BUFFER_SIZE - 1)] +
        ((i / (RANDOM_BUFFER_SIZE / 4)) & 15) * 256 +
        (random_buffer_1024[(i * 4 + 1) & (RANDOM_BUFFER_SIZE - 1)] & (~3)),
        4 + (random_buffer_1024[((i * 4 + 2) & (RANDOM_BUFFER_SIZE - 1))] & (~3)));
}

static void test_random_mixed_sizes_DRAM_word_aligned_256(int i) {
    /* Source and destination address selected randomly from range of 8MB. */
    memcpy_func(buffer_page +
        8192 * random_buffer_1024[(i * 2) & (RANDOM_BUFFER_SIZE - 1)] +
        ((i / (RANDOM_BUFFER_SIZE / 4)) & 15) * 256 +
        (random_buffer_1024[(i * 4) & (RANDOM_BUFFER_SIZE - 1)] & (~3)),
        buffer_page +
        8192 * random_buffer_1024[(i * 2 + 1) & (RANDOM_BUFFER_SIZE - 1)] +
        ((i / (RANDOM_BUFFER_SIZE / 4)) & 15) * 256 +
        (random_buffer_1024[(i * 4 + 1) & (RANDOM_BUFFER_SIZE - 1)] & (~3)),
        4 + (random_buffer_1024[((i * 4 + 2) & (RANDOM_BUFFER_SIZE - 1))] & 252));
}

static void test_random_mixed_sizes_DRAM_word_aligned_64(int i) {
    /* Source and destination address selected randomly from range of 8MB. */
    memcpy_func(buffer_page +
        8192 * random_buffer_1024[(i * 2) & (RANDOM_BUFFER_SIZE - 1)] +
        ((i / (RANDOM_BUFFER_SIZE / 4)) & 15) * 256 +
        (random_buffer_1024[(i * 4) & (RANDOM_BUFFER_SIZE - 1)] & (~3)),
        buffer_page +
        8192 * random_buffer_1024[(i * 2 + 1) & (RANDOM_BUFFER_SIZE - 1)] +
        ((i / (RANDOM_BUFFER_SIZE / 4)) & 15) * 256 +
        (random_buffer_1024[(i * 4 + 1) & (RANDOM_BUFFER_SIZE - 1)] & (~3)),
        4 + (random_buffer_1024[((i * 4 + 2) & (RANDOM_BUFFER_SIZE - 1))] & 60));
}

static void clear_data_cache() {
    int val = 0;
    for (int i = 0; i < 1024 * 1024 * 32; i += 4) {
        val += buffer_alloc[i];
    }
    for (int i = 0; i < 1024 * 1024 * 32; i += 4) {
        buffer_alloc[i] = val;
    }
}

static void do_test(const char *name, void (*test_func)(int), int bytes) {
    int nu_iterations;
    if (bytes >= 1024) 
        nu_iterations = (256 * 1024 * 1024) / bytes;
    else if (bytes >= 64)
        nu_iterations = (32 * 1024 * 1024) / bytes;
    else
        nu_iterations = 1024 * 1024 / 2;
    /* Warm-up. */
    clear_data_cache();
    double temp_time = get_time();
    for (int i = 0; i < nu_iterations; i++)
       test_func(i);
    usleep(100000);
    double start_time = get_time();
    double end_time;
    int count = 0;
    for (;;) {
        for (int i = 0; i < nu_iterations; i++)
            test_func(i);
        count++;
        end_time = get_time();
        if (end_time - start_time >= test_duration)
            break;
    }
    double bandwidth = (double)bytes * nu_iterations * count / (1024 * 1024)
        / (end_time - start_time);
    printf("%s: %.2lf MB/s\n", name, bandwidth);
}

static void do_test_all(const char *name, void (*test_func)(), int bytes) {
    for (int j = 0; j < NU_MEMCPY_VARIANTS; j++)
        if (memcpy_mask[j]) {
            printf("%s:\n", memcpy_variant_name[j]);
            memcpy_func = memcpy_variant[j];
            do_test(name, test_func, bytes);
        }
}

static void fill_buffer(uint8_t *buffer) {
    for (int i = 0; i < 1024 * 1024 * 16; i++) {
        buffer[i] = i & 0xFF;
    }
}

static int compare_buffers(uint8_t *buffer0, uint8_t *buffer1) {
    for (int i = 0; i < 1024 * 1024 * 16; i++) {
        if (buffer0[i] != buffer1[i])
            return 0;
    }
    return 1;
}

static void memcpy_emulate(uint8_t *dest, uint8_t *src, int size) {
    for (int i = 0; i < size; i++)
        dest[i] = src[i];
}

static void do_validation(int repeat) {
    int passed = 1;
    for (int i = 0; i < 10 * repeat; i++)  {
        int size = floor(pow(2.0, (double)rand() * 20.0 / RAND_MAX));
        int source = rand() % (1024 * 1024 * 16 + 1 - size);
        int dest;
        do {
            dest = rand() % (1024 * 1024 * 16 + 1 - size);
        }
        while (dest + size > source && dest < source + size);
        fill_buffer(buffer_compare);
        memcpy_emulate(buffer_compare + dest, buffer_compare + source, size);
        fill_buffer(buffer_alloc);
        memcpy_func(buffer_alloc + dest, buffer_alloc + source, size);
        if (!compare_buffers(buffer_alloc, buffer_compare)) {
            printf("Validation failed (source offset = %d, destination offset = %d, size = %d.\n",
                source, dest, size);
            passed = 0;
        }
    }
    if (passed) {
        printf("Passed.\n");
    }
}

#define NU_TESTS 37

typedef struct {
    const char *name;
    void (*test_func)();
    int bytes;
} test_t;

test_t test[NU_TESTS] = {
    { "3 bytes randomly aligned", test_unaligned_random_3, 3 },
    { "8 bytes randomly aligned", test_unaligned_random_8, 8 },
    { "17 bytes randomly aligned", test_unaligned_random_17, 17 },
    { "30 bytes randomly aligned", test_unaligned_random_30, 30 },
    { "64 bytes randomly aligned", test_unaligned_random_64, 64 },
    { "137 bytes randomly aligned", test_unaligned_random_137, 137 },
    { "1024 bytes randomly aligned", test_unaligned_random_1024, 1024 },
    { "32768 bytes randomly aligned", test_unaligned_random_32768, 32768 },
    { "1M bytes randomly aligned", test_unaligned_random_1M, 1024 * 1024 },
    { "64 bytes randomly aligned, source aligned with dest",
        test_source_dest_aligned_random_64, 64 },
    { "1024 bytes randomly aligned, source aligned with dest",
        test_source_dest_aligned_random_1024, 1024 },
    { "32768 bytes randomly aligned, source aligned with dest",
        test_source_dest_aligned_random_32768, 32768 },
    { "1M bytes randomly aligned, source aligned with dest",
        test_source_dest_aligned_random_1M, 1024 *1024 },
    { "Up to 1024 bytes randomly aligned", test_random_mixed_sizes_1024, 512 },
    { "Up to 64 bytes randomly aligned", test_random_mixed_sizes_64, 32 },
    { "Up to 1024 bytes randomly aligned (DRAM)", test_random_mixed_sizes_DRAM_1024,
       512 },
    { "Up to 64 bytes randomly aligned (DRAM)", test_random_mixed_sizes_DRAM_64,
       32 },
    { "Up to 1024 bytes word aligned (DRAM)", test_random_mixed_sizes_DRAM_word_aligned_1024,
       514 },
    { "Up to 256 bytes word aligned (DRAM)", test_random_mixed_sizes_DRAM_word_aligned_256,
       130 },
    { "Up to 64 bytes word aligned (DRAM)", test_random_mixed_sizes_DRAM_word_aligned_64,
       34 },
    { "30 bytes 4-byte aligned", test_word_aligned_30, 30 },
    { "64 bytes 4-byte aligned", test_word_aligned_64, 64 },
    { "296 bytes 4-byte aligned", test_word_aligned_296, 296 },
    { "1024 bytes 4-byte aligned", test_word_aligned_1024, 1024 },
    { "4096 bytes 4-byte aligned", test_word_aligned_4096, 4096 },
    { "32768 bytes 4-byte aligned", test_word_aligned_32768, 32768 },
    { "64 bytes 32-byte aligned", test_chunk_aligned_64, 64 },
    { "296 bytes 32-byte aligned", test_chunk_aligned_296, 296 },
    { "1024 bytes 32-byte aligned", test_chunk_aligned_1024, 1024 },
    { "4096 bytes 32-byte aligned", test_chunk_aligned_4096, 4096 },
    { "32768 bytes 32-byte aligned", test_chunk_aligned_32768, 32768 },
    { "1024 bytes page aligned", test_page_aligned_1024, 1024 },
    { "4096 bytes page aligned", test_page_aligned_4096, 4096 },
    { "32768 bytes page aligned", test_page_aligned_32768, 32768 },
    { "256K bytes page aligned", test_page_aligned_256K, 256 * 1024 },
    { "1M bytes page aligned", test_page_aligned_1M, 1024 * 1024 },
    { "8M bytes page aligned", test_page_aligned_8M, 8 * 1024 * 1024 },
};

static void usage() {
            printf("Commands:\n"
                "--list          List test numbers and memcpy variants.\n"
                "--test <number> Perform test <number> only, 5 times for each memcpy variant.\n"
                "--all           Perform each test 5 times for each memcpy variant.\n"
                "--help          Show this message.\n"
                "Options:\n"
                "--duration <n>  Sets the duration of each individual test. Default is 2 seconds.\n"
                "--repeat <n>    Repeat each test n times. Default is 5.\n"
                "--quick         Shorthand for --duration 1 -repeat 2.\n"
                "--memcpy <list> Instead of testing all memcpy variants, test only the memcpy variants\n"
                "                in <list>. <list> is a string of characters from a to h or higher, corresponding\n"
                "                to each memcpy variant (for example, abcdef selects the first six variants).\n"
                "--validate      Validate for correctness instead of measuring performance. The --repeat option\n"
                "                can be used to influence the number of validation tests performed (default 5).\n"
                );
}

static int char_to_memcpy_variant(char c) {
    if (c >= 'a' && c <= 'z')
        return c - 'a';
    if (c >= 'A' && c <= 'Z')
        return c - 'A' + 26;
    return - 1;
}

static char memcpy_variant_to_char(int i) {
    if (i < 26)
        return 'a' + i;
    return 'A' + i - 26;
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
        usage();
        return 0;
    }
    int argi = 1;
    int command_test = - 1;
    int command_all = 0;
    int repeat = 5;
    int validate = 0;
    for (int i = 0; i < NU_MEMCPY_VARIANTS; i++)
        memcpy_mask[i] = 1;
    // Disable fastarm memcpy because it is buggy.
    memcpy_mask[1] = 0;
    for (;;) {
        if (argi >= argc)
            break;
        if (argi + 1 < argc && strcasecmp(argv[argi], "--test") == 0) {
            int t = atoi(argv[argi + 1]);
            if (t < 0 || t >= NU_TESTS) {
                printf("Test out of range.\n");
                return 1;
            }
            command_test = t;
            argi += 2;
            continue;
        }
        if (strcasecmp(argv[argi], "--quick") == 0) {
            test_duration = 1.0;
            repeat = 2;
            argi++;
            continue;
        }
        if (strcasecmp(argv[argi], "--all") == 0) {
            command_all = 1;
            argi++;
            continue;
        }
        if (strcasecmp(argv[argi], "--list") == 0) {
            printf("Tests:\n");
            for (int i = 0; i < NU_TESTS; i++)
                printf("%3d    %s\n", i, test[i].name);
            printf("memcpy variants:\n");
            for (int i = 0; i < NU_MEMCPY_VARIANTS; i++)
                printf("  %c    %s\n", memcpy_variant_to_char(i), memcpy_variant_name[i]);
            return 0;
        }
        if (strcasecmp(argv[argi], "--help") == 0) {
            usage();
            return 0;
        }
        if (argi + 1 < argc && strcasecmp(argv[argi], "--duration") == 0) {
            double d = strtod(argv[argi + 1], NULL);
            if (d < 0.1 || d >= 100.0) {
                printf("Duration out of range.\n");
                return 1;
            }
            test_duration = d;
            argi += 2;
            continue;
        }
        if (argi + 1 < argc && strcasecmp(argv[argi], "--memcpy") == 0) {
            for (int i = 0; i < NU_MEMCPY_VARIANTS; i++)
                memcpy_mask[i] = 0;
            for (int i = 0; i < strlen(argv[argi + 1]); i++)
                if (char_to_memcpy_variant(argv[argi + 1][i]) >= 0 && char_to_memcpy_variant(argv[argi + 1][i]) < NU_MEMCPY_VARIANTS)
                    memcpy_mask[char_to_memcpy_variant(argv[argi + 1][i])] = 1;
            argi += 2;
            continue;
        }
        if (argi + 1 < argc && strcasecmp(argv[argi], "--repeat") == 0) {
            repeat = atoi(argv[argi + 1]);
            if (repeat < 1 || repeat >= 1000) {
                printf("Number of repeats out of range.\n");
                return 1;
            }
            argi += 2;
            continue;
        }
        if (strcasecmp(argv[argi], "--validate") == 0) {
            validate = 1;
            argi++;
            continue;
        }
        printf("Unkown option. Try --help.\n");
        return 1;
    }

    if ((command_test != -1) + command_all != 1 && !validate) {
        printf("Specify only one of --test and --all.\n");
        return 1;
    }

    buffer_alloc = malloc(1024 * 1024 * 32);
    buffer_page = (uint8_t *)buffer_alloc + ((4096 - ((uintptr_t)buffer_alloc & 4095))
        & 4095);
    buffer_chunk = buffer_page + 17 * 32;
    if (validate)
        buffer_compare = malloc(1024 * 1024 * 16);
    srand(0);
    random_buffer_1024 = malloc(sizeof(int) * RANDOM_BUFFER_SIZE);
    for (int i = 0; i < RANDOM_BUFFER_SIZE; i++)
        random_buffer_1024[i] = rand() % 1023;

    if (sizeof(size_t) != sizeof(int)) {
        printf("sizeof(size_t) != sizeof(int), unable to directly replace memcpy.\n");
        return 1;
    }

    int start_test, end_test;
    start_test = 0;
    end_test = NU_TESTS - 1;
    if (command_test != - 1) {
        start_test = command_test;
        end_test = command_test;
    }
    if (validate) {
        for (int j = 0; j < NU_MEMCPY_VARIANTS; j++)
            if (memcpy_mask[j]) {
                printf("%s:\n", memcpy_variant_name[j]);
                memcpy_func = memcpy_variant[j];
                do_validation(repeat);
            }
        return 0;
    }
    for (int t = start_test; t <= end_test; t++) {
        for (int j = 0; j < NU_MEMCPY_VARIANTS; j++)
            if (memcpy_mask[j]) {
                printf("%s:\n", memcpy_variant_name[j]);
                memcpy_func = memcpy_variant[j];
                for (int i = 0; i < repeat; i++)
                    do_test(test[t].name, test[t].test_func, test[t].bytes);
            }
    }
 }
