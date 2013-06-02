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
#include <string.h>
#include <stdint.h>

#include "fastarm.h"

/*
 * When ALIGN_DESTINATION_WRITES_MAIN_PART is defined, unaligned destination write access will
 * be avoided for the main part of the destination buffer. When is not defined, a faster copying
 * function is used but it leaves it the processor to handle unaligned writes.
 *
 * Note that even with this option, a small part of the destination buffer at the beginning
 * or end may still be written to with unaligned writes.
 */

#define ALIGN_DESTINATION_WRITES_MAIN_PART

typedef void (*fastarm_tail_func_type)(const uint8_t *src, uint8_t *dest, int n);

typedef void (*fastarm_copy_chunks_unaligned_func_type)(const uint8_t *src, uint8_t *dest, int chunks);

typedef void (*fastarm_word_align_func_type)(const uint8_t *src, uint8_t *dest);

static const fastarm_tail_func_type fastarm_tail_func[32];
static const fastarm_copy_chunks_unaligned_func_type fastarm_copy_chunks_unaligned_func[4];
static const fastarm_word_align_func_type fastarm_word_align_func[4];

/* Macro for the ARM cache line preload instruction. */
#define ARM_PRELOAD(_var, _offset)\
    asm volatile ("pld [%[address], %[offset]]" : : [address] "r" (_var), [offset] "I" (_offset));

/* Macros for ARM multi-register load/store instructions. */
#define ARM_LDMIA_R4_R7(_var) \
    asm volatile("ldmia %[address]!, {r4-r7}" : [address] "+r" (_var) : : "r4", "r5", "r6", "r7");

#define ARM_LDMIA_R8_R11(_var) \
    asm volatile("ldmia %[address]!, {r8-r11}" : [address] "+r" (_var) : : "r8", "r9", "r10", "r11");

#define ARM_STMIA_R4_R7(_var) \
    asm volatile("stmia %[address]!, {r4-r7}" : [address] "+r" (_var) : : );

#define ARM_STMIA_R8_R11(_var) \
    asm volatile("stmia %[address]!, {r8-r11}" : [address] "+r" (_var) : : );

/*
 * Optimized ARM assembler copying function. Copies source-aligned chunks of 32 bytes.
 * It is strongly advisable that srclinep is 32-byte aligned and dstlinep is at least
 * word (32-bit) aligned, preferably 32-byte aligned.
 */

static void copy_chunks_aligned(const uint8_t *src, uint8_t *dest, int chunks) {
    do {
        ARM_LDMIA_R4_R7(src);
        ARM_LDMIA_R8_R11(src);
        ARM_STMIA_R4_R7(dest);
        ARM_PRELOAD(src, 128);
        ARM_STMIA_R8_R11(dest);
        chunks--;
    }  while (chunks != 0);
}

/* The destination is offset by 1 byte from word alignment. */

static void copy_chunks_unaligned_offset_1(const uint8_t *src, uint8_t *dest, int chunks) {
    ARM_PRELOAD(src, 32);
    do {
       uint32_t v0 = *(uint32_t *)src;
       uint32_t v1 = *(uint32_t *)(src + 4);
       *(uint8_t *)dest = (uint8_t)v0;
       *(uint16_t *)(dest + 1) = (uint16_t)(v0 >> 8);
       *(uint32_t *)(dest + 3) = (v0 >> 24) + (v1 << 8);
       uint32_t v2 = *(uint32_t *)(src + 8);
       uint32_t v3 = *(uint32_t *)(src + 12);
       *(uint32_t *)(dest + 7) = (v1 >> 24) + (v2 << 8);
       *(uint32_t *)(dest + 11) = (v2 >> 24) + (v3 << 8);
       uint32_t v4 = *(uint32_t *)(src + 16);
       uint32_t v5 = *(uint32_t *)(src + 20);
       *(uint32_t *)(dest + 15) = (v3 >> 24) + (v4 << 8);
       ARM_PRELOAD(src, 64);
       *(uint32_t *)(dest + 19) = (v4 >> 24) + (v5 << 8);
       uint32_t v6 = *(uint32_t *)(src + 24);
       uint32_t v7 = *(uint32_t *)(src + 28);
       *(uint32_t *)(dest + 23) = (v5 >> 24) + (v6 << 8);
       *(uint32_t *)(dest + 27) = (v6 >> 24) + (v7 << 8);
       *(uint8_t *)(dest + 31) = v7 >> 24;
       src += 32;
       dest += 32;
       chunks--;
    }  while (chunks != 0);
}

/* The destination is offset by 2 bytes from word alignment. */

static void copy_chunks_unaligned_offset_2(const uint8_t *src, uint8_t *dest, int chunks) {
    ARM_PRELOAD(src, 32);
    do {
       uint32_t v0 = *(uint32_t *)src;
       uint32_t v1 = *(uint32_t *)(src + 4);
       *(uint16_t *)dest = (uint16_t)v0;
       *(uint32_t *)(dest + 2) = (v0 >> 16) + (v1 << 16);
       uint32_t v2 = *(uint32_t *)(src + 8);
       uint32_t v3 = *(uint32_t *)(src + 12);
       *(uint32_t *)(dest + 6) = (v1 >> 16) + (v2 << 16);
       *(uint32_t *)(dest + 10) = (v2 >> 16) + (v3 << 16);
       uint32_t v4 = *(uint32_t *)(src + 16);
       uint32_t v5 = *(uint32_t *)(src + 20);
       *(uint32_t *)(dest + 14) = (v3 >> 16) + (v4 << 16);
       ARM_PRELOAD(src, 64);
       *(uint32_t *)(dest + 18) = (v4 >> 16) + (v5 << 16);
       uint32_t v6 = *(uint32_t *)(src + 24);
       uint32_t v7 = *(uint32_t *)(src + 28);
       *(uint32_t *)(dest + 22) = (v5 >> 16) + (v6 << 16);
       *(uint32_t *)(dest + 26) = (v6 >> 16) + (v7 << 16);
       *(uint16_t *)(dest + 30) = v7 >> 16;
       src += 32;
       dest += 32;
       chunks--;
    }  while (chunks != 0);
}

/* The destination is offset by 3 bytes from word alignment. */

static void copy_chunks_unaligned_offset_3(const uint8_t *src, uint8_t *dest, int chunks) {
    ARM_PRELOAD(src, 32);
    do {
       uint32_t v0 = *(uint32_t *)src;
       uint32_t v1 = *(uint32_t *)(src + 4);
       uint32_t v2 = *(uint32_t *)(src + 8);
       *(uint8_t *)dest = (uint8_t)v0;
       *(uint32_t *)(dest + 1) = (v0 >> 8) + (v1 << 24);
       *(uint32_t *)(dest + 5) = (v1 >> 8) + (v2 << 24);
       uint32_t v3 = *(uint32_t *)(src + 12);
       uint32_t v4 = *(uint32_t *)(src + 16);
       *(uint32_t *)(dest + 9) = (v2 >> 8) + (v3 << 24);
       *(uint32_t *)(dest + 13) = (v3 >> 8) + (v4 << 24);
       uint32_t v5 = *(uint32_t *)(src + 20);
       uint32_t v6 = *(uint32_t *)(src + 24);
       *(uint32_t *)(dest + 17) = (v4 >> 8) + (v5 << 24);
       ARM_PRELOAD(src, 64);
       *(uint32_t *)(dest + 21) = (v5 >> 8) + (v6 << 24);
       uint32_t v7 = *(uint32_t *)(src + 28);
       *(uint32_t *)(dest + 25) = (v6 >> 8) + (v7 << 24);
       *(uint16_t *)(dest + 29) = (uint16_t)(v7 >> 8);
       *(uint8_t *)(dest + 31) = v7 >> 24;
       src += 32;
       dest += 32;
       chunks--;
    }  while (chunks != 0);
}

static const fastarm_copy_chunks_unaligned_func_type fastarm_copy_chunks_unaligned_func[4] = {
    copy_chunks_aligned,
    copy_chunks_unaligned_offset_1,
    copy_chunks_unaligned_offset_2,
    copy_chunks_unaligned_offset_3
};

static void fastarm_memcpy_aligned32_tail(void *dest, const void *src, int n) {
    const uint8_t *srcp = src;
    uint8_t *destp = dest;
    int i = 0;
    while (i + 8 <= n) {
         uint32_t v0 = *(uint32_t *)(srcp + i);
         uint32_t v1 = *(uint32_t *)(srcp + i + 4);
         *(uint32_t *)(destp + i) = v0;
         *(uint32_t *)(destp + i + 4) = v1;
         i += 8;
    }
    if (i == n)
        return;
    if (n & 4) {
        *(uint32_t *)(destp + i) = *(uint32_t *)(srcp + i);
        if (n & 2) {
            *(uint16_t *)(destp + i + 4) = *(uint16_t *)(srcp + i + 4);
            if (n & 1)
                *(uint8_t *)(destp + i + 6) = *(uint8_t *)(srcp + i + 6);
            return;
        }
        if (n & 1)
            *(uint8_t *)(destp + i + 4) = *(uint8_t *)(srcp + i + 4);
        return;
    }
    if (n & 2) {
        *(uint16_t *)(destp + i) = *(uint16_t *)(srcp + i);
        if (n & 1)
            *(uint8_t *)(destp + i + 4) = *(uint8_t *)(srcp + i + 2);
        return;
    }
    if (n & 1)
        *(uint8_t *)(destp + i + 4) = *(uint8_t *)(srcp + i);
    return;
}

static void fastarm_memcpy_source_aligned32(void *dest, const void *src, int n) {
    uintptr_t alignshift_dest = (uintptr_t)dest & 31;
    if ((alignshift_dest & 3) == 0) {
        /* The destination is at least word aligned. Just copy. */
        int chunks = n >> 5;
        if (chunks > 0)
            copy_chunks_aligned((uint8_t *)src, (uint8_t *)dest, chunks);
        int tail_size = n & 31;
        if (tail_size != 0)
            fastarm_tail_func[tail_size]((uint8_t *)src + chunks * 32,
                (uint8_t *)dest + chunks * 32, tail_size);
        return;
    }
    int chunks = n >> 5;
    if (chunks > 0)
        fastarm_copy_chunks_unaligned_func[alignshift_dest & 3](src, dest, chunks);
    // Accept unaligned destination access for the tail.
    int tail_size = n & 31;
    if (tail_size !=  0)
        fastarm_tail_func[tail_size]((uint8_t *)src + chunks * 32, (uint8_t *)dest +
            chunks * 32, tail_size);
}

/* The source and destination have the same alignment within a 32-byte chunk. */

static void fastarm_memcpy_source_and_dest_aligned(void *dest, const void *src, int n) {
    uintptr_t alignshift_src = (uintptr_t)src & 31;
    if ((alignshift_src & 3) == 0) {
        /* The source and destination are at least word aligned. */
        int head_size = (32 - alignshift_src) & 31;
        if (head_size != 0) {
            ARM_PRELOAD(src, 32);
            if (n <= head_size) {
                fastarm_tail_func[n]((uint8_t *)src, (uint8_t *)dest, n);
                return;
            }
            fastarm_tail_func[head_size]((uint8_t *)src, (uint8_t *)dest, head_size);
            src = (uint8_t *)src + head_size;
            dest = (uint8_t *)dest + head_size;
            n -= head_size;
        }
        int chunks = n >> 5;
        if (chunks > 0)
            copy_chunks_aligned((uint8_t *)src, (uint8_t *)dest, chunks);
        int tail_size = n & 31;
        if (tail_size != 0)
            fastarm_tail_func[tail_size]((uint8_t *)src + chunks * 32,
                (uint8_t *)dest + chunks * 32, tail_size);
        return;
    }
    /* The source and destination are not word aligned. */
    int head_size = (32 - alignshift_src) & 31;
    if (head_size != 0) {
        /* Align copies of the head by fist copying up to 3 bytes to gain alignment. */
        int word_align_size = (4 - (alignshift_src & 3)) & 3;
        if (word_align_size != 0) {
           fastarm_word_align_func[word_align_size]((uint8_t *)src, (uint8_t *)dest);
           src = (uint8_t *)src + word_align_size;
           dest = (uint8_t *)dest + word_align_size;
           n -= word_align_size;
           head_size -= word_align_size;
           if (head_size == 0)
               goto copy_chunks;
        }
        if (n <= head_size) {
           fastarm_tail_func[n]((uint8_t *)src, (uint8_t *)dest, n);
           return;
        }
        ARM_PRELOAD(src, 32);
        fastarm_tail_func[head_size]((uint8_t *)src, (uint8_t *)dest, head_size);
        src = (uint8_t *)src + head_size;
        dest = (uint8_t *)dest + head_size;
        n -= head_size;
    }
    /* Source and destination are now 32-byte aligned. */
copy_chunks: ;
    int chunks = n >> 5;
    if (chunks > 0)
        copy_chunks_aligned(src, dest, chunks);
    // Accept unaligned access for the tail.
    int tail_size = n & 31;
    if (tail_size !=  0)
        fastarm_tail_func[tail_size]((uint8_t *)src + chunks * 32, (uint8_t *)dest +
            chunks * 32, tail_size);
}

/* The source and destination are word aligned but not chunk aligned. */

static void fastarm_memcpy_source_and_dest_word_aligned(void *dest, const void *src, int n) {
    uintptr_t alignshift_src = (uintptr_t)src & 31;
    int head_size = (32 - alignshift_src) & 31;
    if (head_size != 0) {
        if (n <= head_size) {
            fastarm_tail_func[n]((uint8_t *)src, (uint8_t *)dest, n);
            return;
        }
        ARM_PRELOAD(src, 32);
        fastarm_tail_func[head_size]((uint8_t *)src, (uint8_t *)dest, head_size);
        src = (uint8_t *)src + head_size;
        dest = (uint8_t *)dest + head_size;
        n -= head_size;
    }
    /* Accept non-chunk aligned access to destination. */
    int chunks = n >> 5;
    if (chunks > 0)
        copy_chunks_aligned((uint8_t *)src, (uint8_t *)dest, chunks);
    int tail_size = n & 31;
    if (tail_size != 0)
        fastarm_tail_func[tail_size]((uint8_t *)src + chunks * 32,
            (uint8_t *)dest + chunks * 32, tail_size);
    return;
}

/* The source is word aligned but the destination is not. */

static void fastarm_memcpy_source_word_aligned(void *dest, const void *src, int n) {
    uintptr_t alignshift_src = (uintptr_t)src & 31;
    int head_size = (32 - alignshift_src) & 31;
    if (head_size != 0) {
        /* For now accept unaligned destination access to the head. */
        if (n <= head_size) {
            fastarm_tail_func[n]((uint8_t *)src, (uint8_t *)dest, n);
            return;
        }
        ARM_PRELOAD(src, 32);
        fastarm_tail_func[head_size]((uint8_t *)src, (uint8_t *)dest, head_size);
        src = (uint8_t *)src + head_size;
        dest = (uint8_t *)dest + head_size;
        n -= head_size;
    }
    /* The source is now chunk aligned. */
    uintptr_t alignshift_dest = (uintptr_t)dest & 31;
    int chunks = n >> 5;
    if (chunks > 0)
#ifdef ALIGN_DESTINATION_WRITES_MAIN_PART
        fastarm_copy_chunks_unaligned_func[alignshift_dest & 3](src, dest, chunks);
#else
        copy_chunks_aligned(src, dest, chunks);
#endif
    // Accept unaligned destination access for the tail.
    int tail_size = n & 31;
    if (tail_size !=  0)
        fastarm_tail_func[tail_size]((uint8_t *)src + chunks * 32, (uint8_t *)dest +
            chunks * 32, tail_size);
}

/* The source and destination have the same alignment within a 32-bit word. */

static void fastarm_memcpy_source_and_dest_aligned_within_word(void *dest, const void *src, int n) {
    /* The source and destination are not word aligned. */
    uintptr_t alignshift_src = (uintptr_t)src & 31;
    int head_size = (32 - alignshift_src) & 31;
    if (head_size != 0) {
        /* Align copies of the head by fist copying up to 3 bytes to gain alignment. */
        int word_align_size = (4 - (alignshift_src & 3)) & 3;
        if (word_align_size != 0) {
           fastarm_word_align_func[word_align_size]((uint8_t *)src, (uint8_t *)dest);
           src = (uint8_t *)src + word_align_size;
           dest = (uint8_t *)dest + word_align_size;
           n -= word_align_size;
           head_size -= word_align_size;
           if (head_size == 0)
               goto copy_chunks;
        }
        if (n <= head_size) {
           fastarm_tail_func[n]((uint8_t *)src, (uint8_t *)dest, n);
           return;
        }
        ARM_PRELOAD(src, 32);
        fastarm_tail_func[head_size]((uint8_t *)src, (uint8_t *)dest, head_size);
        src = (uint8_t *)src + head_size;
        dest = (uint8_t *)dest + head_size;
        n -= head_size;
    }
    /* Source is now 32-byte aligned, destination is word aligned. */
copy_chunks: ;
    int chunks = n >> 5;
    if (chunks > 0)
        copy_chunks_aligned(src, dest, chunks);
    int tail_size = n & 31;
    if (tail_size !=  0)
        fastarm_tail_func[tail_size]((uint8_t *)src + chunks * 32, (uint8_t *)dest +
            chunks * 32, tail_size);
}

/* The source is not word-aligned and the destination does not have the same alignment. */

static void fastarm_memcpy_source_not_word_aligned(void *dest, const void *src, int n) {
    uintptr_t alignshift_src = (uintptr_t)src & 31;
    int head_size = (32 - alignshift_src) & 31;
    if (head_size != 0) {
        /* At least align reading of the head by fist copying up to 3 bytes to gain alignment. */
        int word_align_size = (4 - (alignshift_src & 3)) & 3;
        if (word_align_size != 0) {
           fastarm_word_align_func[word_align_size]((uint8_t *)src, (uint8_t *)dest);
           src = (uint8_t *)src + word_align_size;
           dest = (uint8_t *)dest + word_align_size;
           n -= word_align_size;
           head_size -= word_align_size;
           if (head_size == 0)
               goto copy_chunks;
        }
        /* Accept unaligned access to destination. */
        if (n <= head_size) {
           fastarm_tail_func[n]((uint8_t *)src, (uint8_t *)dest, n);
           return;
        }
        ARM_PRELOAD(src, 32);
        fastarm_tail_func[head_size]((uint8_t *)src, (uint8_t *)dest, head_size);
        src = (uint8_t *)src + head_size;
        dest = (uint8_t *)dest + head_size;
        n -= head_size;
    }
    /* Source is now 32-byte aligned. */
copy_chunks: ;
    uintptr_t alignshift_dest = (uintptr_t)dest & 31;
    int chunks = n >> 5;
    if (chunks > 0)
#ifdef ALIGN_DESTINATION_WRITES_MAIN_PART
        fastarm_copy_chunks_unaligned_func[alignshift_dest & 3](src, dest, chunks);
#else
        copy_chunks_aligned(src, dest, chunks);
#endif
    // Accept unaligned destination access for the tail.
    int tail_size = n & 31;
    if (tail_size != 0)
        fastarm_tail_func[tail_size]((uint8_t *)src + chunks * 32, (uint8_t *)dest +
            chunks * 32, tail_size);
}

/* Main memcpy function. */

void *fastarm_memcpy(void *dest, const void *src, int n) {
    if (n <= 12) {
        if (n < 0)
            return dest;
        // Don't care about alignment. */
        fastarm_tail_func[n](src, dest, n);
        return dest;
    }
    ARM_PRELOAD(src, 0);
    uintptr_t alignshift_src = (uintptr_t)src & 31;
    uintptr_t alignshift_dest = (uintptr_t)dest & 31;
    if (alignshift_src == 0) {
        if (alignshift_dest == 0)
            if (n >= 32)
                return fastarm_memcpy_aligned32(dest, src, n);
            else {
                fastarm_tail_func[n](src, dest, n);
                return dest;
            }
        else
            fastarm_memcpy_source_aligned32(dest, src, n);
            return dest;
    }
    if (alignshift_src == alignshift_dest) {
        fastarm_memcpy_source_and_dest_aligned(dest, src, n);
        return dest;
    }
    if ((alignshift_src & 3) == 0) {
        if ((alignshift_dest & 3) == 0) {
            fastarm_memcpy_source_and_dest_word_aligned(dest, src, n);
            return dest;
        }
        fastarm_memcpy_source_word_aligned(dest, src, n);
        return dest;
    }
    /* Source is not word aligned. */
    if ((alignshift_src & 3) == (alignshift_dest & 3)) {
        fastarm_memcpy_source_and_dest_aligned_within_word(dest, src, n);
        return dest;
    }
    fastarm_memcpy_source_not_word_aligned(dest, src, n);
    return dest;
}

/* Version for when source an destinations are guaranteed to be 32-byte aligned. */

void *fastarm_memcpy_aligned32(void *dest, const void *src, int n) {
    if (n <= 0)
        return dest;
    int chunks = n >> 5;
    copy_chunks_aligned((uint8_t *)src, (uint8_t *)dest, chunks);
    int tail_size = n & 31;
    if (tail_size != 0)
        fastarm_tail_func[tail_size]((uint8_t *)src + chunks * 32,
            (uint8_t *)dest + chunks * 32, tail_size);
    return dest;
}

static void fastarm_tail_func_0(const uint8_t *src, uint8_t *dest, int n) {
    return;
}

static void fastarm_tail_func_1_2_3(const uint8_t *src, uint8_t *dest, int n) {
    if (n & 2) {
        *(uint16_t *)dest = *(uint16_t *)src;
        if (n & 1)
           *(dest + 2) = *(src + 2);
        return;
    }
    *dest = *src;
    return;
}

static void fastarm_tail_func_4(const uint8_t *src, uint8_t *dest, int n) {
   *(uint32_t *)dest = *(uint32_t *)src;
   return;
}

static void fastarm_tail_func_5_6_7(const uint8_t *src, uint8_t *dest, int n) {
    *(uint32_t *)dest = *(uint32_t *)src;
    if (n & 2) {
        *(uint16_t *)(dest + 4) = *(uint16_t *)(src + 4);
        if (n & 1)
           *(dest + 6) = *(src + 6);
        return;
    }
    *(dest + 4) = *(src + 4);
    return;
}

static void fastarm_tail_func_8(const uint8_t *src, uint8_t *dest, int n) {
    *(uint32_t *)dest = *(uint32_t *)src;
    *(uint32_t *)(dest + 4) = *(uint32_t *)(src + 4);
    return;
}

static void fastarm_tail_func_9_10_11(const uint8_t *src, uint8_t *dest, int n) {
    *(uint32_t *)dest = *(uint32_t *)src;
    *(uint32_t *)(dest + 4) = *(uint32_t *)(src + 4);
    if (n & 2) {
        *(uint16_t *)(dest + 8) = *(uint16_t *)(src + 8);
        if (n & 1)
           *(dest + 10) = *(src + 10);
        return;
    }
    *(dest + 8) = *(src + 8);
    return;
}

static void fastarm_tail_func_12_13_14_15(const uint8_t *src, uint8_t *dest, int n) {
    *(uint32_t *)dest = *(uint32_t *)src;
    *(uint32_t *)(dest + 4) = *(uint32_t *)(src + 4);
    *(uint32_t *)(dest + 8) = *(uint32_t *)(src + 8);
    if (n == 12)
        return;
    if (n & 2) {
        *(uint16_t *)(dest + 12) = *(uint16_t *)(src + 12);
        if (n & 1)
           *(dest + 14) = *(src + 14);
        return;
    }
    *(dest + 12) = *(src + 12);
    return;
}

static void fastarm_tail_func_16(const uint8_t *src, uint8_t *dest, int n) {
    *(uint32_t *)dest = *(uint32_t *)src;
    *(uint32_t *)(dest + 4) = *(uint32_t *)(src + 4);
    *(uint32_t *)(dest + 8) = *(uint32_t *)(src + 8);
    *(uint32_t *)(dest + 12) = *(uint32_t *)(src + 12);
}

static void fastarm_tail_func_17_18_19(const uint8_t *src, uint8_t *dest, int n) {
    *(uint32_t *)dest = *(uint32_t *)src;
    *(uint32_t *)(dest + 4) = *(uint32_t *)(src + 4);
    *(uint32_t *)(dest + 8) = *(uint32_t *)(src + 8);
    *(uint32_t *)(dest + 12) = *(uint32_t *)(src + 12);
    if (n & 2) {
        *(uint16_t *)(dest + 16) = *(uint16_t *)(src + 16);
        if (n & 1)
           *(dest + 18) = *(src + 18);
        return;
    }
    *(dest + 16) = *(src + 16);
    return;
}

static void fastarm_tail_func_20_21_22_23(const uint8_t *src, uint8_t *dest, int n) {
    *(uint32_t *)dest = *(uint32_t *)src;
    *(uint32_t *)(dest + 4) = *(uint32_t *)(src + 4);
    *(uint32_t *)(dest + 8) = *(uint32_t *)(src + 8);
    *(uint32_t *)(dest + 12) = *(uint32_t *)(src + 12);
    *(uint32_t *)(dest + 16) = *(uint32_t *)(src + 16);
    if (n == 20)
        return;
    if (n & 2) {
        *(uint16_t *)(dest + 20) = *(uint16_t *)(src + 20);
        if (n & 1)
           *(dest + 22) = *(src + 22);
        return;
    }
    *(dest + 20) = *(src + 20);
    return;
}

static void fastarm_tail_func_24_25_26_27(const uint8_t *src, uint8_t *dest, int n) {
    *(uint32_t *)dest = *(uint32_t *)src;
    *(uint32_t *)(dest + 4) = *(uint32_t *)(src + 4);
    *(uint32_t *)(dest + 8) = *(uint32_t *)(src + 8);
    *(uint32_t *)(dest + 12) = *(uint32_t *)(src + 12);
    *(uint32_t *)(dest + 16) = *(uint32_t *)(src + 16);
    *(uint32_t *)(dest + 20) = *(uint32_t *)(src + 20);
    if (n == 24)
        return;
    if (n & 2) {
        *(uint16_t *)(dest + 24) = *(uint16_t *)(src + 24);
        if (n & 1)
           *(dest + 26) = *(src + 26);
        return;
    }
    *(dest + 24) = *(src + 24);
    return;
}

static void fastarm_tail_func_28_29_30_31(const uint8_t *src, uint8_t *dest, int n) {
    *(uint32_t *)dest = *(uint32_t *)src;
    *(uint32_t *)(dest + 4) = *(uint32_t *)(src + 4);
    *(uint32_t *)(dest + 8) = *(uint32_t *)(src + 8);
    *(uint32_t *)(dest + 12) = *(uint32_t *)(src + 12);
    *(uint32_t *)(dest + 16) = *(uint32_t *)(src + 16);
    *(uint32_t *)(dest + 20) = *(uint32_t *)(src + 20);
    *(uint32_t *)(dest + 24) = *(uint32_t *)(src + 24);
    if (n == 28)
        return;
    if (n & 2) {
        *(uint16_t *)(dest + 28) = *(uint16_t *)(src + 28);
        if (n & 1)
           *(dest + 30) = *(src + 30);
        return;
    }
    *(dest + 28) = *(src + 28);
    return;
}

static const fastarm_tail_func_type fastarm_tail_func[32] = {
    fastarm_tail_func_0,
    fastarm_tail_func_1_2_3,
    fastarm_tail_func_1_2_3,
    fastarm_tail_func_1_2_3,
    fastarm_tail_func_4,
    fastarm_tail_func_5_6_7,
    fastarm_tail_func_5_6_7,
    fastarm_tail_func_5_6_7,
    fastarm_tail_func_8,
    fastarm_tail_func_9_10_11,
    fastarm_tail_func_9_10_11,
    fastarm_tail_func_9_10_11,
    fastarm_tail_func_12_13_14_15,
    fastarm_tail_func_12_13_14_15,
    fastarm_tail_func_12_13_14_15,
    fastarm_tail_func_12_13_14_15,
    fastarm_tail_func_16,
    fastarm_tail_func_17_18_19,
    fastarm_tail_func_17_18_19,
    fastarm_tail_func_17_18_19,
    fastarm_tail_func_20_21_22_23,
    fastarm_tail_func_20_21_22_23,
    fastarm_tail_func_20_21_22_23,
    fastarm_tail_func_20_21_22_23,
    fastarm_tail_func_24_25_26_27,
    fastarm_tail_func_24_25_26_27,
    fastarm_tail_func_24_25_26_27,
    fastarm_tail_func_24_25_26_27,
    fastarm_tail_func_28_29_30_31,
    fastarm_tail_func_28_29_30_31,
    fastarm_tail_func_28_29_30_31,
    fastarm_tail_func_28_29_30_31
};

/* Copy one byte so that word alignment is gained. */

static void fastarm_word_align_1(const uint8_t *src, uint8_t *dest) {
    *dest = *src;
}

/* Copy two bytes so that word alignment is gained for the source. */

static void fastarm_word_align_2(const uint8_t *src, uint8_t *dest) {
    if (((uintptr_t)dest & 1)) {
        uint32_t v = *(uint16_t *)src;
        *dest = (uint8_t)v;
        *(dest + 1) = (uint8_t)(v >> 8);
        return;
    }
    *(uint16_t *)dest = *(uint16_t *)src;
}

/* Copy three bytes so that word alignment is gained for the source. */

static void fastarm_word_align_3(const uint8_t *src, uint8_t *dest) {
    if (((uintptr_t)dest & 1)) {
        /* The destination is, like the source, not 16-bit aligned. */
        *dest = *src;
        *(uint16_t *)(dest + 1)  = *(uint16_t *)(src + 1);
        return;
    }
    /* The destination is 16-bit aligned, but the source is not. */
    uint32_t v0 = *src;
    uint32_t v1 = *(uint16_t *)(src + 1);
    *(uint16_t *)dest = (uint16_t)(v0 + (v1 << 8));
    *(dest + 2) = (uint8_t)(v1 >> 8);
}

static const fastarm_word_align_func_type fastarm_word_align_func[4] = {
    NULL,
    fastarm_word_align_1,
    fastarm_word_align_2,
    fastarm_word_align_3,
};


/*
 * Blit interface.
 */

/*
 * The source and destination are at least word aligned, and have the same alignment
 * within a 32-byte chunk.
 */

static void fastarm_blit_aligned_word_aligned(void *src, void *dest, int src_stride, int dest_stride,
int width_in_bytes, int height) {
    uintptr_t alignshift_src = (uintptr_t)src & 31;
    int head_size = (32 - alignshift_src) & 31;
    int chunks;
    int tail_size;
    if (width_in_bytes <= head_size) {
        chunks = 0;
        tail_size = 0;
    }
    else {
        chunks = (width_in_bytes - head_size) >> 5;
        tail_size = (width_in_bytes - head_size) & 31;
    }
    while (height > 1) {
        uint8_t *srclinep = src;
        uint8_t *destlinep = dest;
        if (head_size != 0) {
            fastarm_tail_func[head_size](srclinep, destlinep, head_size);
            srclinep += head_size;
            destlinep += head_size;
        }
        if (chunks != 0)
            copy_chunks_aligned((uint8_t *)srclinep, (uint8_t *)destlinep, chunks);
        if (tail_size != 0)
            fastarm_tail_func[tail_size](srclinep + chunks * 32,
                destlinep + chunks * 32, tail_size);
        src = (uint8_t *)src + src_stride;
        ARM_PRELOAD(src, 0);
        dest = (uint8_t *)dest + dest_stride;
        height--;
    }
    uint8_t *srclinep = src;
    uint8_t *destlinep = dest;
    if (head_size != 0) {
        fastarm_tail_func[head_size]((uint8_t *)srclinep, (uint8_t *)destlinep, head_size);
            srclinep += head_size;
            destlinep += head_size;
    }
    if (chunks != 0)
        copy_chunks_aligned((uint8_t *)srclinep, (uint8_t *)destlinep, chunks);
    if (tail_size != 0)
        fastarm_tail_func[tail_size]((uint8_t *)srclinep + chunks * 32,
            (uint8_t *)destlinep + chunks * 32, tail_size);
}

/*
 * The source and destination are not word aligned, but have the same alignment
 * within a 32-byte chunk.
 */

static void fastarm_blit_aligned(void *src, void *dest, int src_stride, int dest_stride,
int width_in_bytes, int height) {
    uintptr_t alignshift_src = (uintptr_t)src & 31;
    int head_size = (32 - alignshift_src) & 31;
    int chunks;
    int tail_size;
    int word_align_size;
    if (head_size != 0) {
        word_align_size = (4 - (alignshift_src & 3)) & 3;
        if (word_align_size != 0) {
           width_in_bytes-= word_align_size;
           head_size -= word_align_size;
           if (width_in_bytes < 0) {
               word_align_size += width_in_bytes;
               width_in_bytes = 0;
           }
        }
    }
    else
        word_align_size = 0;
    if (width_in_bytes <= head_size) {
        head_size = width_in_bytes;
        chunks = 0;
        tail_size = 0;
    }
    else {
        chunks = (width_in_bytes - head_size) >> 5;
        tail_size = (width_in_bytes - head_size) & 31;
    }
    while (height > 1) {
        uint8_t *srclinep = src;
        uint8_t *destlinep = dest;
        if (word_align_size != 0) {
            fastarm_word_align_func[word_align_size]((uint8_t *)srclinep, (uint8_t *)destlinep);
            srclinep += word_align_size;
            destlinep += word_align_size;
        }
        if (head_size != 0) {
            fastarm_tail_func[head_size]((uint8_t *)srclinep, (uint8_t *)destlinep, head_size);
            srclinep += head_size;
            destlinep += head_size;
        }
        if (chunks != 0)
            copy_chunks_aligned((uint8_t *)srclinep, (uint8_t *)destlinep, chunks);
        if (tail_size != 0)
            fastarm_tail_func[tail_size]((uint8_t *)src + chunks * 32,
                (uint8_t *)dest + chunks * 32, tail_size);
        src = (uint8_t *)src + src_stride;
        ARM_PRELOAD(src, 0);
        dest = (uint8_t *)dest + dest_stride;
        height--;
    }
    uint8_t *srclinep = src;
    uint8_t *destlinep = dest;
    if (word_align_size != 0) {
        fastarm_word_align_func[word_align_size]((uint8_t *)srclinep, (uint8_t *)destlinep);
        srclinep += word_align_size;
        destlinep += word_align_size;
    }
    if (head_size != 0) {
        fastarm_tail_func[head_size]((uint8_t *)srclinep, (uint8_t *)destlinep, head_size);
        srclinep += head_size;
        destlinep += head_size;
    }
    if (chunks != 0)
        copy_chunks_aligned((uint8_t *)srclinep, (uint8_t *)destlinep, chunks);
    if (tail_size != 0)
        fastarm_tail_func[tail_size]((uint8_t *)src + chunks * 32,
            (uint8_t *)dest + chunks * 32, tail_size);
}

void fast_arm_blit(void *src, void *dest, int src_stride, int dest_stride, int width_in_bytes,
int height) {
    if (width_in_bytes <= 0 || height <= 0)
        return;
    ARM_PRELOAD(src, 0);
    uintptr_t alignshift_src = (uintptr_t)src & 31;
    uintptr_t alignshift_dest = (uintptr_t)dest & 31;
    if (alignshift_src == alignshift_dest) {
        if ((alignshift_src & 3) == 0) {
            fastarm_blit_aligned_word_aligned(src, dest, src_stride, dest_stride, width_in_bytes,
                height);
            return;
        }
        fastarm_blit_aligned(src, dest, src_stride, dest_stride, width_in_bytes,
            height);
        return;
    }
    /* Fall back to general case. */
    while (height > 1) {
        fastarm_memcpy(dest, src, width_in_bytes);
        src = (uint8_t *)src + src_stride;
        ARM_PRELOAD(src, 0);
        dest = (uint8_t *)dest + dest_stride;
        height--;
    }
    fastarm_memcpy(dest, src, width_in_bytes);
}
