
extern void *memcpy_armv5te(void *dest, const void *src, int n);

extern void *memcpy_armv5te_no_overfetch_align_16_block_write_8(void *dest, const void *src, int n);

extern void *memcpy_armv5te_no_overfetch_align_16_block_write_16(void *dest,
    const void *src, int n);

extern void *memcpy_armv5te_no_overfetch_align_32_block_write_8(void *dest, const void *src, int n);

extern void *memcpy_armv5te_no_overfetch_align_32_block_write_16(void *dest,
    const void *src, int n);

extern void *memcpy_armv5te_no_overfetch_align_32_block_write_32(void *dest,
    const void *src, int n);
