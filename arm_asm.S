/*
 * Copyright © 2006-2008, 2013 Siarhei Siamashka <siarhei.siamashka@gmail.com>
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
 * Modified by Harm Hanemaaijer <fgenfb@yahoo.com>:
 * 1. Add ".type <function_name>, function" to function definition macro, which
 *    was required for correct linkage on my platform.
 * 2. Add non-overfetching memcpy version. Various optimizations for this version
 *    are documented in the git log. The function is split into several variants
 *    using macros.
 *    To do: -- Implement write-align == 32 for unaligned case.
 *           -- Implement line_size == 64 and write_align == 64 for unaligned case.
 */

/* Prevent the stack from becoming executable */
#if defined(__linux__) && defined(__ELF__)
.section .note.GNU-stack,"",%progbits
#endif

#ifdef __arm__

.text
.syntax unified
.fpu neon
.arch armv7a
.object_arch armv4
.arm
.altmacro
.p2align 2

/******************************************************************************/

.macro asm_function function_name
    .global \function_name
.func \function_name
.type \function_name, function
.p2align 5
\function_name:
.endm

/******************************************************************************/

/*
 * Helper macro for memcpy function, it can copy data from source (r1) to 
 * destination (r0) buffers fixing alignment in the process. Destination
 * buffer should be aligned already (4 bytes alignment is required.
 * Size of the block to copy is in r2 register
 */
.macro  UNALIGNED_MEMCPY shift
    sub     r1, #(\shift)
    ldr     ip, [r1], #4

    tst     r0, #4
    movne   r3, ip, lsr #(\shift * 8)
    ldrne   ip, [r1], #4
    subne   r2, r2, #4
    orrne   r3, r3, ip, asl #(32 - \shift * 8)
    strne   r3, [r0], #4

    tst     r0, #8
    movne   r3, ip, lsr #(\shift * 8)
    ldmiane r1!, {r4, ip}
    subne   r2, r2, #8
    orrne   r3, r3, r4, asl #(32 - \shift * 8)
    movne   r4, r4, lsr #(\shift * 8)
    orrne   r4, r4, ip, asl #(32 - \shift * 8)
    stmiane r0!, {r3-r4}
    cmp     r2, #32
    blt     3f
    pld     [r1, #48]
    stmfd   sp!, {r7, r8, r9, r10, r11}
    add     r3, r1, #128
    bic     r3, r3, #31
    sub     r9, r3, r1
1:
    pld     [r1, r9]
    subs    r2, r2, #32
    movge   r3, ip, lsr #(\shift * 8)
    ldmiage r1!, {r4-r6, r7, r8, r10, r11, ip}
    orrge   r3, r3, r4, asl #(32 - \shift * 8)
    movge   r4, r4, lsr #(\shift * 8)
    orrge   r4, r4, r5, asl #(32 - \shift * 8)
    movge   r5, r5, lsr #(\shift * 8)
    orrge   r5, r5, r6, asl #(32 - \shift * 8)
    movge   r6, r6, lsr #(\shift * 8)
    orrge   r6, r6, r7, asl #(32 - \shift * 8)
    stmiage r0!, {r3-r6}
    movge   r7, r7, lsr #(\shift * 8)
    orrge   r7, r7, r8, asl #(32 - \shift * 8)
    movge   r8, r8, lsr #(\shift * 8)
    orrge   r8, r8, r10, asl #(32 - \shift * 8)
    movge   r10, r10, lsr #(\shift * 8)
    orrge   r10, r10, r11, asl #(32 - \shift * 8)
    movge   r11, r11, lsr #(\shift * 8)
    orrge   r11, r11, ip, asl #(32 - \shift * 8)
    stmiage r0!, {r7, r8, r10, r11}
    bgt     1b
2:
    ldmfd   sp!, {r7, r8, r9, r10, r11}
3:  /* copy remaining data */
    tst     r2, #16
    movne   r3, ip, lsr #(\shift * 8)
    ldmiane r1!, {r4-r6, ip}
    orrne   r3, r3, r4, asl #(32 - \shift * 8)
    movne   r4, r4, lsr #(\shift * 8)
    orrne   r4, r4, r5, asl #(32 - \shift * 8)
    movge   r5, r5, lsr #(\shift * 8)
    orrge   r5, r5, r6, asl #(32 - \shift * 8)
    movge   r6, r6, lsr #(\shift * 8)
    orrge   r6, r6, ip, asl #(32 - \shift * 8)
    stmiane r0!, {r3-r6}

    tst     r2, #8
    movne   r3, ip, lsr #(\shift * 8)
    ldmiane r1!, {r4, ip}
    orrne   r3, r3, r4, asl #(32 - \shift * 8)
    movne   r4, r4, lsr #(\shift * 8)
    orrne   r4, r4, ip, asl #(32 - \shift * 8)
    stmiane r0!, {r3-r4}

    tst     r2, #4
    movne   r3, ip, lsr #(\shift * 8)
    ldrne   ip, [r1], #4
    sub     r1, r1, #(4 - \shift)
    orrne   r3, r3, ip, asl #(32 - \shift * 8)
    strne   r3, [r0], #4

    tst     r2, #2
    ldrbne  r3, [r1], #1
    ldrbne  r4, [r1], #1
    ldr     r5, [sp], #4
    strbne  r3, [r0], #1
    strbne  r4, [r0], #1

    tst     r2, #1
    ldrbne  r3, [r1], #1
    ldr     r6, [sp], #4
    strbne  r3, [r0], #1

    pop     {r0, r4}

    bx      lr
.endm

/*
 * Memcpy function with Raspberry Pi specific aligned prefetch, based on
 * https://garage.maemo.org/plugins/scmsvn/viewcvs.php/mplayer/trunk/fastmem-arm9/fastmem-arm9.S
 */
asm_function memcpy_armv5te
    cmp     r2, #20
    blt     9f
    /* copy data until destination address is 4 bytes aligned */
    tst     r0, #1
    ldrbne  r3, [r1], #1
    stmfd   sp!, {r0, r4}
    subne   r2, r2, #1
    strbne  r3, [r0], #1
    tst     r0, #2
    ldrbne  r3, [r1], #1
    ldrbne  r4, [r1], #1
    stmfd   sp!, {r5, r6}
    subne   r2, r2, #2
    orrne   r3, r3, r4, asl #8
    strhne  r3, [r0], #2
    /* destination address is 4 bytes aligned */
    /* now we should handle 4 cases of source address alignment */
    tst     r1, #1
    bne     6f
    tst     r1, #2
    bne     7f

    /* both source and destination are 4 bytes aligned */
    stmfd   sp!, {r7, r8, r9, r10, r11}
    tst     r0, #4
    ldrne   r4, [r1], #4
    subne   r2, r2, #4
    strne   r4, [r0], #4
    tst     r0, #8
    ldmiane r1!, {r3-r4}
    add     r9, r1, #96
    subne   r2, r2, #8
    bic     r9, r9, #31
    stmiane r0!, {r3-r4}
    sub     r9, r9, r1
1:
    subs    r2, r2, #32
    ldmiage r1!, {r3-r6, r7, r8, r10, r11}
    pld     [r1, r9]
    stmiage r0!, {r3-r6}
    stmiage r0!, {r7, r8, r10, r11}
    bgt     1b
2:
    ldmfd   sp!, {r7, r8, r9, r10, r11}
    tst     r2, #16
    ldmiane r1!, {r3-r6}
    stmiane r0!, {r3-r6}
    tst     r2, #8
    ldmiane r1!, {r3-r4}
    stmiane r0!, {r3-r4}
    tst     r2, #4
    ldrne   r3, [r1], #4
    mov     ip, r0
    strne   r3, [ip], #4
    tst     r2, #2
    ldrhne  r3, [r1], #2
    ldmfd   sp!, {r5, r6}
    strhne  r3, [ip], #2
    tst     r2, #1
    ldrbne  r3, [r1], #1
    ldmfd   sp!, {r0, r4}
    strbne  r3, [ip], #1

    bx      lr

6:
    tst    r1, #2
    bne    8f
    UNALIGNED_MEMCPY 1
7:
    UNALIGNED_MEMCPY 2
8:
    UNALIGNED_MEMCPY 3
9:
    stmfd  sp!, {r0, r4}
1:  subs   r2, r2, #3
    ldrbge ip, [r0]
    ldrbge r3, [r1], #1
    ldrbge r4, [r1], #1
    ldrbge ip, [r1], #1
    strbge r3, [r0], #1
    strbge r4, [r0], #1
    strbge ip, [r0], #1
    bge    1b
    adds   r2, r2, #2
    ldrbge r3, [r1], #1
    mov    ip, r0
    ldr    r0, [sp], #4
    strbge r3, [ip], #1
    ldrbgt r3, [r1], #1
    ldr    r4, [sp], #4
    strbgt r3, [ip], #1
    bx     lr
.endfunc

/*
 * Helper macro for non-overfetching version.
 */

.macro  UNALIGNED_MEMCPY_NO_OVERFETCH shift, write_align, block_write_size, preload_offset
    sub     r1, #(\shift)
    ldr     ip, [r1], #4

    tst     r0, #4
    movne   r3, ip, lsr #(\shift * 8)
    ldrne   ip, [r1], #4
    subne   r2, r2, #4
    orrne   r3, r3, ip, asl #(32 - \shift * 8)
    strne   r3, [r0], #4

    tst     r0, #8
    movne   r3, ip, lsr #(\shift * 8)
    ldmiane r1!, {r4, ip}
    subne   r2, r2, #8
    orrne   r3, r3, r4, asl #(32 - \shift * 8)
    movne   r4, r4, lsr #(\shift * 8)
    orrne   r4, r4, ip, asl #(32 - \shift * 8)
    stmiane r0!, {r3-r4}
    cmp     r2, #32
    blt     3f
.if \preload_offset != 0
    cmp     r2, #\preload_offset
.endif
    stmfd   sp!, {r7, r8, r9, r10, r11}
.if \preload_offset != 0
    add     r3, r1, #\preload_offset
    bic     r3, r3, #31
    sub     r9, r3, r1
    /* If there are less than preload_offset bytes to go, skip the main loop. */
    blt     4f
1:
    pld     [r1, r9]
    sub     r2, r2, #32
    mov     r3, ip, lsr #(\shift * 8)
    ldmia   r1!, {r4-r6, r7, r8, r10, r11, ip}
    orr     r3, r3, r4, asl #(32 - \shift * 8)
    mov     r4, r4, lsr #(\shift * 8)
    cmp     r2, #\preload_offset
    orr     r4, r4, r5, asl #(32 - \shift * 8)
    mov     r5, r5, lsr #(\shift * 8)
.if \block_write_size == 8
    stmia   r0!, {r3-r4}
.endif
    orr     r5, r5, r6, asl #(32 - \shift * 8)
    mov     r6, r6, lsr #(\shift * 8)
    orr     r6, r6, r7, asl #(32 - \shift * 8)
    mov     r7, r7, lsr #(\shift * 8)
.if \block_write_size == 16
    stmia   r0!, {r3-r6}
.endif
.if \block_write_size == 8
    stmia   r0!, {r5-r6}
.endif
    orr     r7, r7, r8, asl #(32 - \shift * 8)
    mov     r8, r8, lsr #(\shift * 8)
    orr     r8, r8, r10, asl #(32 - \shift * 8)
    mov     r10, r10, lsr #(\shift * 8)
.if \block_write_size == 8
    stmia   r0!, {r7-r8}
.endif
    orr     r10, r10, r11, asl #(32 - \shift * 8)
    mov     r11, r11, lsr #(\shift * 8)
    orr     r11, r11, ip, asl #(32 - \shift * 8)
.if \block_write_size == 32
    stmia   r0!, {r3-r6, r7, r8, r10, r11}
.endif
.if \block_write_size == 16
    stmia   r0!, {r7, r8, r10, r11}
.endif
.if \block_write_size == 8
    stmia   r0!, {r10-r11}
.endif
    bge     1b
.endif /* preload_offset != 0 */
4:
    sub     r2, r2, #32
    mov     r3, ip, lsr #(\shift * 8)
    ldmia   r1!, {r4-r6, r7, r8, r10, r11, ip}
    orr     r3, r3, r4, asl #(32 - \shift * 8)
    mov     r4, r4, lsr #(\shift * 8)
    cmp     r2, #32
    orr     r4, r4, r5, asl #(32 - \shift * 8)
    mov     r5, r5, lsr #(\shift * 8)
.if \block_write_size == 8
    stmia   r0!, {r3-r4}
.endif
    orr     r5, r5, r6, asl #(32 - \shift * 8)
    mov     r6, r6, lsr #(\shift * 8)
    orr     r6, r6, r7, asl #(32 - \shift * 8)
    mov     r7, r7, lsr #(\shift * 8)
.if \block_write_size == 16
    stmia   r0!, {r3-r6}
.endif
.if \block_write_size == 8
    stmia   r0!, {r5-r6}
.endif
    orr     r7, r7, r8, asl #(32 - \shift * 8)
    mov     r8, r8, lsr #(\shift * 8)
    orr     r8, r8, r10, asl #(32 - \shift * 8)
    mov     r10, r10, lsr #(\shift * 8)
.if \block_write_size == 8
    stmia   r0!, {r7-r8}
.endif
    orr     r10, r10, r11, asl #(32 - \shift * 8)
    mov     r11, r11, lsr #(\shift * 8)
    orr     r11, r11, ip, asl #(32 - \shift * 8)
.if \block_write_size == 32
    stmia   r0!, {r3-r6, r7, r8, r10, r11}
.endif
.if \block_write_size == 16
    stmia   r0!, {r7, r8, r10, r11}
.endif
.if \block_write_size == 8
    stmia   r0!, {r10-r11}
.endif
    bge     4b
2:
    ldmfd   sp!, {r7, r8, r9, r10, r11}
3:  /* copy remaining data */
    tst     r2, #16
    movne   r3, ip, lsr #(\shift * 8)
    ldmiane r1!, {r4-r6, ip}
    orrne   r3, r3, r4, asl #(32 - \shift * 8)
    movne   r4, r4, lsr #(\shift * 8)
    orrne   r4, r4, r5, asl #(32 - \shift * 8)
    movge   r5, r5, lsr #(\shift * 8)
    orrge   r5, r5, r6, asl #(32 - \shift * 8)
    movge   r6, r6, lsr #(\shift * 8)
    orrge   r6, r6, ip, asl #(32 - \shift * 8)
    stmiane r0!, {r3-r6}

    tst     r2, #8
    movne   r3, ip, lsr #(\shift * 8)
    ldmiane r1!, {r4, ip}
    orrne   r3, r3, r4, asl #(32 - \shift * 8)
    movne   r4, r4, lsr #(\shift * 8)
    orrne   r4, r4, ip, asl #(32 - \shift * 8)
    stmiane r0!, {r3-r4}

    tst     r2, #4
    movne   r3, ip, lsr #(\shift * 8)
    ldrne   ip, [r1], #4
    sub     r1, r1, #(4 - \shift)
    orrne   r3, r3, ip, asl #(32 - \shift * 8)
    strne   r3, [r0], #4

    tst     r2, #2
    ldrbne  r3, [r1], #1
    ldrbne  r4, [r1], #1
    ldr     r5, [sp], #4
    strbne  r3, [r0], #1
    strbne  r4, [r0], #1

    tst     r2, #1
    ldrbne  r3, [r1], #1
    ldr     r6, [sp], #4
    strbne  r3, [r0], #1

    pop     {r0, r4}

    bx      lr
.endm

/*
 * Macro that defines the main body of a memcpy version with no over-fetching
 * beyond the source memory region.
 *
 * line_size must be 32 or 64.
 * write_align must be 32 or 16, or 64.
 * block_write_size must be 32, 16 or 8.
 * preload_offset must be a multiple of 32, 96 was the default setting. When preload_offset is 0,
 * no preload instructions will be generated at all.
 * preload_early must be 0 or 1. 1 is only allowed if write_align is 32.
 *
 * If line_size is 64, write_align must be 64, block_write_size must be 32, and preload_offset
 * must be a multiple of 64.
 */

.macro MEMCPY_NO_OVERFETCH line_size, write_align, block_write_size, preload_offset, preload_early
.if \preload_early == 1
    bic     r3, r1, #(\line_size - 1)
.endif
.if \write_align == 32
    cmp     r2, #36
.else
    cmp     r2, #20
.endif
.if \preload_early == 1
    pld     [r3]
.endif
    blt     9f
    /* copy data until destination address is 4 bytes aligned */
    stmfd   sp!, {r0, r4}
.if \preload_early == 1
    mov     r4, r3
.endif
    tst     r0, #1
    ldrbne  r3, [r1], #1
    subne   r2, r2, #1
    strbne  r3, [r0], #1
.if \preload_early == 1
    pld     [r4, #\line_size]
.endif
    stmfd   sp!, {r5, r6}
    tst     r0, #2
    ldrbne  r3, [r1], #1
    ldrbne  r5, [r1], #1
    subne   r2, r2, #2
    orrne   r3, r3, r5, asl #8
    strhne  r3, [r0], #2
    /* destination address is 4 bytes aligned */
    /* now we should handle 4 cases of source address alignment */
    tst     r1, #1
    bne     6f
    tst     r1, #2
    bne     7f

    /* both source and destination are 4 bytes aligned */
    stmfd   sp!, {r7, r8, r9, r10, r11}
    tst     r0, #4
    ldrne   r5, [r1], #4
    subne   r2, r2, #4
    strne   r5, [r0], #4
    tst     r0, #8
    ldmiane r1!, {r3, r5}
    subne   r2, r2, #8
    stmiane r0!, {r3, r5}
.if \write_align == 32 || \write_align == 64
    tst     r0, #16
    ldmiane r1!, {r3, r5-r7}
    subne   r2, r2, #16
    stmiane r0!, {r3, r5-r7}
.endif
.if \write_align == 64
    cmp     r2, #32
    blt     14f
    tst     r0, #32
    ldmiane r1!, {r3, r5-r11}
    subne   r2, r2, #32
    stmiane r0!, {r3, r5-r11}
.endif
    /* Source is now write_align bytes aligned. */
.if \preload_offset == 0
    cmp     r2, #32
    blt     2f
.elseif \write_align == 64
    cmp     r2, #64
    mov     r9, #\preload_offset
    blt     2f
    cmp     r2, #(\preload_offset + 64)
    ble     10f
.elseif \write_align == 32
    /* In the case of write_align == 32 r9 will be equal to preload_offset. */
    cmp     r2, #32
    mov     r9, #\preload_offset
    blt     2f
    cmp     r2, #(\preload_offset + 32)
    ble     10f
.else
    add     r9, r1, #\preload_offset
    cmp     r2, #32
    bic     r9, r9, #31
    /* If there are less than 32 bytes to go, skip all loops. */
    blt     2f
    cmp     r2, #(\preload_offset + 32)
    sub     r9, r9, r1
    /* If there are <= (preload_offset + 32) bytes to go, skip the main loop. */
    ble     10f
.endif
.if \preload_offset != 0
.if \preload_early == 1
    pld     [r4, #(\line_size * 2)]
.if \write_align == 32 && \block_write_size == 16 && \preload_offset >= 128
    cmp     r2, #(\preload_offset + 128)
    blt     1f
    /* When early preload is enabled, gradually catch up to the preload offset */
    /* in the main loop while processing 128 bytes at a time. */
    add     ip, r4, #96
11:
    sub     r2, r2, #32
    ldmia   r1!, {r3-r6, r7, r8, r10, r11}
    pld     [ip]
    stmia   r0!, {r3-r6}
    sub     r4, ip, r1
    add     r4, r4, #32
    cmp     r2, r4
    stmia   r0!, {r7, r8, r10, r11}
    /* Jump if the next preload would overfetch, */
    blt     13f
    sub     r2, r2, #32
    pld     [ip, #32]
    cmp     r2, r4
    ldmia   r1!, {r3-r6, r7, r8, r10, r11}
    stmia   r0!, {r3-r6}
    /* Jump if the next preload would overfetch, */
    stmialt r0!, {r7, r8, r10, r11}
    blt     13f
    sub     r4, ip, r1
    add     r4, r4, #96
    cmp     r2, r4
    pld     [ip, #64]
    stmia   r0!, {r7, r8, r10, r11}
    /* Jump if the next preload would overfetch, */
    blt     13f
    sub     r2, r2, #32
    ldmia   r1!, {r3-r6, r7, r8, r10, r11}
    pld     [ip, #96]
    stmia   r0!, {r3-r6}
    sub     r4, ip, r1
    add     r4, r4, #128
    stmia   r0!, {r7, r8, r10, r11}
    cmp     r2, r4
    /* Jump if the next preload would overfetch, */
    blt     13f
    sub     r2, r2, #32
    ldmia   r1!, {r3-r6, r7, r8, r10, r11}
    pld     [ip, #128]
    add     ip, #160
    stmia   r0!, {r3-r6}
    sub     r4, ip, r1
    /* Has the offset catched up yet? */
    cmp     r4, r9
    stmia   r0!, {r7, r8, r10, r11}
    bgt     11b
13:
    cmp     r2, #(\preload_offset + 32)
    blt     12f
.endif
.endif
1:
.if \line_size == 64
    sub     r2, r2, #64
    ldmia   r1!, {r3-r6, r7, r8, r10, r11}
    cmp     r2, #(\preload_offset + 64)
    stmia   r0!, {r3-r6, r7, r8, r10, r11}
    ldmia   r1!, {r3-r6, r7, r8, r10, r11}
    pld     [r1, r9]
    stmia   r0!, {r3-r6, r7, r8, r10, r11}
.else
    sub     r2, r2, #32
.if \block_write_size == 32
    ldmia   r1!, {r3-r6, r7, r8, r10, r11}
    cmp     r2, #(\preload_offset + 32)
    pld     [r1, r9]
    stmia   r0!, {r3-r6, r7, r8, r10, r11}
.endif
.if \block_write_size == 16
    ldmia   r1!, {r3-r6, r7, r8, r10, r11}
    cmp     r2, #(\preload_offset + 32)
    pld     [r1, r9]
    stmia   r0!, {r3-r6}
    stmia   r0!, {r7, r8, r10, r11}
.endif
.if \block_write_size == 8
    ldmia   r1!, {r3-r6, r7, r8, r10, r11}
    cmp     r2, #(\preload_offset + 32)
    stmia   r0!, {r3-r4}
    stmia   r0!, {r5-r6}
    pld     [r1, r9]
    stmia   r0, {r7-r8}
    stmia   r0, {r10-r11}
.endif
    bge     1b
.endif /* line_size ==  64 */
.endif /* preload != 0 */
10:
.if \line_size == 64
    sub     r2, r2, #64
    ldmia   r1!, {r3-r6, r7, r8, r10, r11}
    cmp     r2, #64
    stmia   r0!, {r3-r6, r7, r8, r10, r11}
    ldmia   r1!, {r3-r6, r7, r8, r10, r11}
    stmia   r0!, {r3-r6, r7, r8, r10, r11}
.else
    sub     r2, r2, #32
.if \block_write_size == 32
    ldmia   r1!, {r3-r6, r7, r8, r10, r11}
    cmp     r2, #32
    stmia   r0!, {r3-r6, r7, r8, r10, r11}
.endif
.if \block_write_size == 16
    ldmia   r1!, {r3-r6, r7, r8, r10, r11}
    cmp     r2, #32
    stmia   r0!, {r3-r6}
    stmia   r0!, {r7, r8, r10, r11}
.endif
.if \block_write_size == 8
    ldmia   r1!, {r3-r6, r7, r8, r10, r11}
    cmp     r2, #32
    stmia   r0!, {r3-r4}
    stmia   r0!, {r5-r6}
    stmia   r0!, {r7-r8}
    stmia   r0!, {r10-r11}
.endif
.endif /* line_size == 64 */
    bge     10b
2:
.if \line_size == 64
    tst     r2, #32
    ldmiane r1!, {r3-r6, r7, r8, r10, r11}
    stmiane r0!, {r3-r6, r7, r8, r10, r11}
.endif
14:
    ldmfd   sp!, {r7, r8, r9, r10, r11}
    tst     r2, #16
    ldmiane r1!, {r3-r6}
    stmiane r0!, {r3-r6}
    tst     r2, #8
    ldmiane r1!, {r3-r4}
    stmiane r0!, {r3-r4}
    tst     r2, #4
    ldrne   r3, [r1], #4
    mov     ip, r0
    strne   r3, [ip], #4
    tst     r2, #2
    ldrhne  r3, [r1], #2
    ldmfd   sp!, {r5, r6}
    strhne  r3, [ip], #2
    tst     r2, #1
    ldrbne  r3, [r1], #1
    ldmfd   sp!, {r0, r4}
    strbne  r3, [ip], #1

    bx      lr

12:
    cmp     r2, #32
    bge     10b
    b       2b
6:
    tst    r1, #2
    bne    8f
    UNALIGNED_MEMCPY_NO_OVERFETCH 1, \write_align, \block_write_size, \preload_offset
7:
    UNALIGNED_MEMCPY_NO_OVERFETCH 2, \write_align, \block_write_size, \preload_offset
8:
    UNALIGNED_MEMCPY_NO_OVERFETCH 3, \write_align, \block_write_size, \preload_offset
9:
    /*
     * Size < 36.
     * Test whether both source and destination
     * are word-aligned, otherwise use a byte-copier.
     */
    tst    r0, #3
    andseq r3, r1, #3
    stmfd  sp!, {r0, r4}
    beq    2f
    tst    r0, #1
    andseq r3, r1, #1
    beq    3f
    /* Byte copier. */
1:
    subs   r2, r2, #3
    ldrbge ip, [r0]
    ldrbge r3, [r1], #1
    ldrbge r4, [r1], #1
    ldrbge ip, [r1], #1
    strbge r3, [r0], #1
    strbge r4, [r0], #1
    strbge ip, [r0], #1
    bge    1b
    adds   r2, r2, #2
    ldrbge r3, [r1], #1
    mov    ip, r0
    ldr    r0, [sp], #4
    strbge r3, [ip], #1
    ldrbgt r3, [r1], #1
    ldr    r4, [sp], #4
    strbgt r3, [ip], #1
    bx     lr
2:
    /* Copy words. */
    subs   r2, r2, #4
    ldrge  r3, [r1], #4
    strge  r3, [r0], #4
    bge    2b
    tst    r2, #2
    mov    ip, r0
    ldrhne r3, [r1], #2
    strhne r3, [ip], #2
    tst    r2, #1
    ldrbne r3, [r1], #1
    ldmfd  sp!, {r0, r4}
    strbne r3, [ip], #1
    bx     lr
3:
    /* Copy half-words. */
    subs   r2, r2, #2
    ldrhge r3, [r1], #2
    strhge r3, [r0], #2
    bge    3b
    tst    r2, #1
    mov    ip, r0
    ldrbne r3, [r1], #1
    ldmfd  sp!, {r0, r4}
    strbne r3, [ip], #1
    bx     lr
.endm

asm_function memcpy_armv5te_no_overfetch_align_16_block_write_8
    MEMCPY_NO_OVERFETCH 32, 16, 8, 96, 0
.endfunc

asm_function memcpy_armv5te_no_overfetch_align_16_block_write_16
    MEMCPY_NO_OVERFETCH 32, 16, 16, 96, 0
.endfunc

asm_function memcpy_armv5te_no_overfetch_align_32_block_write_8
    MEMCPY_NO_OVERFETCH 32, 32, 8, 96, 0
.endfunc

asm_function memcpy_armv5te_no_overfetch_align_32_block_write_16_preload_64
    MEMCPY_NO_OVERFETCH 32, 32, 16, 64 0
.endfunc

asm_function memcpy_armv5te_no_overfetch_align_32_block_write_16_preload_96
    MEMCPY_NO_OVERFETCH 32, 32, 16, 96, 0
.endfunc

asm_function memcpy_armv5te_no_overfetch_align_32_block_write_16_preload_128
    MEMCPY_NO_OVERFETCH 32, 32, 16, 128, 0
.endfunc

asm_function memcpy_armv5te_no_overfetch_align_32_block_write_16_preload_160
    MEMCPY_NO_OVERFETCH 32, 32, 16, 160, 0
.endfunc

asm_function memcpy_armv5te_no_overfetch_align_32_block_write_16_preload_192
    MEMCPY_NO_OVERFETCH 32, 32, 16, 192, 0
.endfunc

asm_function memcpy_armv5te_no_overfetch_align_32_block_write_16_preload_256
    MEMCPY_NO_OVERFETCH 32, 32, 16, 256, 0
.endfunc

asm_function memcpy_armv5te_no_overfetch_align_32_block_write_32_preload_64
    MEMCPY_NO_OVERFETCH 32, 32, 32, 64, 0
.endfunc

asm_function memcpy_armv5te_no_overfetch_align_32_block_write_32_preload_96
    MEMCPY_NO_OVERFETCH 32, 32, 32, 96, 0
.endfunc

asm_function memcpy_armv5te_no_overfetch_align_32_block_write_32_preload_128
    MEMCPY_NO_OVERFETCH 32, 32, 32, 128, 0
.endfunc

asm_function memcpy_armv5te_no_overfetch_align_32_block_write_32_preload_160
    MEMCPY_NO_OVERFETCH 32, 32, 32, 160, 0
.endfunc

asm_function memcpy_armv5te_no_overfetch_align_32_block_write_32_preload_192
    MEMCPY_NO_OVERFETCH 32, 32, 32, 192, 0
.endfunc

asm_function memcpy_armv5te_no_overfetch_align_32_block_write_32_preload_256
    MEMCPY_NO_OVERFETCH 32, 32, 32, 256, 0
.endfunc

asm_function memcpy_armv5te_no_overfetch_align_32_block_write_16_preload_early_96
    MEMCPY_NO_OVERFETCH 32, 32, 16, 96, 1
.endfunc

asm_function memcpy_armv5te_no_overfetch_align_32_block_write_16_preload_early_128
    MEMCPY_NO_OVERFETCH 32, 32, 16, 128, 1
.endfunc

asm_function memcpy_armv5te_no_overfetch_align_32_block_write_16_preload_early_192
    MEMCPY_NO_OVERFETCH 32, 32, 16, 192, 1
.endfunc

asm_function memcpy_armv5te_no_overfetch_align_32_block_write_16_preload_early_256
    MEMCPY_NO_OVERFETCH 32, 32, 16, 256, 1
.endfunc

asm_function memcpy_armv5te_no_overfetch_align_32_block_write_32_preload_early_128
    MEMCPY_NO_OVERFETCH 32, 32, 32, 128, 1
.endfunc

asm_function memcpy_armv5te_no_overfetch_align_32_block_write_32_preload_early_256
    MEMCPY_NO_OVERFETCH 32, 32, 32, 256, 1
.endfunc

asm_function memcpy_armv5te_no_overfetch_align_32_block_write_16_no_preload
    MEMCPY_NO_OVERFETCH 32, 32, 16, 0, 0
.endfunc

asm_function memcpy_armv5te_no_overfetch_align_32_block_write_32_no_preload
    MEMCPY_NO_OVERFETCH 32, 32, 32, 0, 0
.endfunc

asm_function memcpy_armv5te_no_overfetch_align_64_block_write_32_preload_128
    MEMCPY_NO_OVERFETCH 64, 64, 32, 128, 0
.endfunc

asm_function memcpy_armv5te_no_overfetch_align_64_block_write_32_preload_192
    MEMCPY_NO_OVERFETCH 64, 64, 32, 192, 0
.endfunc

asm_function memcpy_armv5te_no_overfetch_align_64_block_write_32_preload_256
    MEMCPY_NO_OVERFETCH 64, 64, 32, 256, 0
.endfunc

#endif
