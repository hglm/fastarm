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
#include <time.h>
#include <sys/time.h>

#include "fastarm.h"

#define TEST_DURATION 2.0
#define RANDOM_BUFFER_SIZE 256

typedef void *(*memcpy_func_type)(void *dest, const void *src, size_t n);

memcpy_func_type memcpy_func;
uint8_t *buffer_chunk, *buffer_page;
int *random_buffer_1024;

static void *fastarm_memcpy_wrapper(void *dest, const void *src, size_t n) {
    return fastarm_memcpy(dest, src, n);
}

static double get_time() {
   struct timespec ts;
   clock_gettime(CLOCK_REALTIME, &ts);
   return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
}

static void test_unaligned_random_64(int i) {
    memcpy_func(buffer_page + random_buffer_1024[(i * 2) & (RANDOM_BUFFER_SIZE - 1)],
        buffer_page + 4096 + random_buffer_1024[(i * 2 + 1) & (RANDOM_BUFFER_SIZE - 1)],
        64);
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

static void test_chunk_aligned_1024(int i) {
    memcpy_func(buffer_chunk, buffer_chunk + 1024, 1024);
}

static void test_chunk_aligned_4096(int i) {
    memcpy_func(buffer_chunk, buffer_chunk + 4096, 4096);
}

static void test_chunk_aligned_32768(int i) {
    memcpy_func(buffer_chunk, buffer_chunk + 32768, 32768);
}

static void test_page_aligned_1024(int i) {
    memcpy_func(buffer_page, buffer_page + 4096, 1024);
}

static void test_page_aligned_4096(int i) {
    memcpy_func(buffer_page, buffer_page + 4096, 4096);
}

static void test_page_aligned_32768(int i) {
    memcpy_func(buffer_page, buffer_page + 32768, 32768);
}

static void test_page_aligned_256K(int i) {
    memcpy_func(buffer_page, buffer_page + 256 * 1024, 256 * 1024);
}

static void test_page_aligned_1M(int i) {
    memcpy_func(buffer_page, buffer_page + 1024 * 1024, 1024 * 1024);
}

static void test_page_aligned_8M(int i) {
    memcpy_func(buffer_page, buffer_page + 8 * 1024 * 1024, 8 * 1024 * 1024);
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

static void do_test(const char *name, void (*test_func)(int), int bytes) {
    /* Warm-up. */
    int nu_iterations;
    if (bytes >= 64) 
        nu_iterations = (256 * 1024 * 1024) / bytes;
    else
        nu_iterations = 1024 * 1024 * 4;
    for (int i = 0; i < nu_iterations; i++)
       test_func(i);
    double start_time = get_time();
    double end_time;
    int count = 0;
    for (;;) {
        for (int i = 0; i < nu_iterations; i++)
            test_func(i);
        count++;
        end_time = get_time();
        if (end_time - start_time >= TEST_DURATION)
            break;
    }
    double bandwidth = (double)bytes * nu_iterations * count / (1024 * 1024)
        / (end_time - start_time);
    printf("%s: %.2lf MB/s\n", name, bandwidth);
}

static void do_test_both(const char *name, void (*test_func)(), int bytes) {
    memcpy_func = memcpy;
    printf("standard memcpy: ");
    do_test(name, test_func, bytes);
    memcpy_func = fastarm_memcpy_wrapper;
    printf("fastarm memcpy: ");
    do_test(name, test_func, bytes);
}

int main(int argc, char *argv[]) {
    void *buffer_alloc = malloc(1024 * 1024 * 32);
    buffer_page = (uint8_t *)buffer_alloc + ((4096 - ((uintptr_t)buffer_alloc & 4095))
        & 4095);
    buffer_chunk = buffer_page + 17 * 32;
    srand(0);
    random_buffer_1024 = malloc(sizeof(int) * RANDOM_BUFFER_SIZE);
    for (int i = 0; i < RANDOM_BUFFER_SIZE; i++)
        random_buffer_1024[i] = rand() % 1023;

    do_test_both("64 bytes randomly aligned", test_unaligned_random_64, 64);
    do_test_both("1024 bytes randomly aligned", test_unaligned_random_1024, 1024);
    do_test_both("32768 bytes randomly aligned", test_unaligned_random_32768, 32768);
    do_test_both("1M bytes randomly aligned", test_unaligned_random_1M, 1024 * 1024);
    do_test_both("1024 bytes randomly aligned, source aligned with dest",
        test_source_dest_aligned_random_1024, 1024);
    do_test_both("32768 bytes randomly aligned, source aligned with dest",
        test_source_dest_aligned_random_32768, 32768);
    do_test_both("1M bytes randomly aligned, source aligned with dest",
        test_source_dest_aligned_random_1M, 1024 *1024);
    do_test_both("Up to 1024 bytes randomly aligned", test_random_mixed_sizes_1024, 512);
    do_test_both("Up to 64 bytes randomly aligned", test_random_mixed_sizes_64, 32);
    do_test_both("Up to 1024 bytes randomly aligned (DRAM)", test_random_mixed_sizes_DRAM_1024,
       512);
    do_test_both("Up to 64 bytes randomly aligned (DRAM)", test_random_mixed_sizes_DRAM_64,
       32);
    do_test_both("1024 bytes aligned", test_chunk_aligned_1024, 1024);
    do_test_both("4096 bytes aligned", test_chunk_aligned_4096, 4096);
    do_test_both("32768 bytes aligned", test_chunk_aligned_32768, 32768);
    do_test_both("1024 bytes page aligned", test_page_aligned_1024, 1024);
    do_test_both("4096 bytes page aligned", test_page_aligned_4096, 4096);
    do_test_both("32768 bytes page aligned", test_page_aligned_32768, 32768);
    do_test_both("256K bytes page aligned", test_page_aligned_256K, 256 * 1024);
    do_test_both("1M bytes page aligned", test_page_aligned_1M, 1024 * 1024);
    do_test_both("8M bytes page aligned", test_page_aligned_8M, 8 * 1024 * 1024);
}
