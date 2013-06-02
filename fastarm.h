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

/*
 * Fast for memcpy for ARM, any alignment. An optimal method depending on
 * alignment constraints is automatically chosen.
 */

void *fastarm_memcpy(void *dest, const void *src, int n);

/*
 * Version for when it is known that both source and destination are 32-byte aligned.
 * May also be reasonable when the source is 32-byte aligned and the destination
 * is 4-byte (word) aligned.
 */

void *fastarm_memcpy_aligned32(void *dest, const void *src, int n);

/*
 * Blit function using the optimized memcpy functions. Some of the overhead of repeatedly
 * calling the memcpy function is avoided.
 *
 * src_stride, dst_stride, and width_in_bytes are in byte units.
 *
 * This function only works correctly for non-overlapping blits or overlapping blits
 * where copying from top to bottom and left to right is acceptable
 */

void fast_arm_blit(void *src, void *dest, int src_stride, int dest_stride,
int width_in_bytes, int height);
