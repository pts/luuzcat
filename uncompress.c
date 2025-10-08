/* uncompress.c -- decompress files in the (n)compress 4.2.4 format.
 * porting, safety fixes and optimizations by pts@fazekas.hu at Thu Oct  2 13:46:01 CEST 2025
 *
 * The code in this file is based on and derived from gzip-1.2.4/unlzw.c
 * with a single bugfix from gzip-1.14/unlzw.c (see just below `resetbuf:').
 * The code in gzip-1.2.4/lzw.c is directly derived from the public domain
 * 'compress' written by Spencer Thomas, Joe Orost, James Woods, Jim McKie,
 * Steve Davies, Ken Turkowski, Dave Mack and Peter Jannesen.
 *
 * Contributions by pts:
 *
 * * porting to CPU-independent and platform-independnet  C89 (ANSI C) and
 *   C++98
 * * memory safety fixes
 * * memory optimizaton for DOS: the stackless decompressor saves ~64 KiB
 * * integer variable optimizations: now it doesn't use the long type, and
 *   works with any sizeof(int) (2 or more).
 * * read optimization: reduced the amount of byte copies before reading
 *   to a buffer to 2 (previously it could be thousands of bytes)
 * * lots of small optimizations such a for bit reading and bit counting
 * * speed optimization for long code strings (e.g. for input files with
 *   long runs of the same byte): using memcpy(...)
 *   when copying from lzw_stack to global_write_buffer has make it ~1.632
 *   times faster
 *
 * pts has added a memory optimization for reading and writing. pts has
 * added 16-bit support (i.e. no operations on `long').
 *
 * https://vapier.github.io/ncompress/ contains a detailed history of
 * (n)compress. An important fact is that earlier versions can't decompress
 * the output. compress 3.0+ can still decompress the output of earlier
 * versions.
 *
 * The code in this file is very similar to the decompress(...) function in
 * ncompress-4.2.4/compress42.c and gzip-.../unlzw.c and
 * busybox-1.21.1/archival/libarchive/decompress_uncompress.c . Probably all
 * of them were copied from (n)compress.
 *
 * File format history:
 *
 * * File format 1 doesn't have any headers (i.e. the compressed data
 *   started with the first uncompressed byte), it is dependent on byte
 *   order and bit order, and it doesn't store the maxbits value. It doesn't
 *   use block mode, and it increases n_bits too early (suboptimally).
 * * File format 2 has a 2-byte signature (0x1f 0x9d), it stores
 *   the maxbits value (9..16) used at compression time in the following
 *   byte, and it is independent of byte order and bit order. It doesn't use
 *   block mode.
 * * File format 3 has a 2-byte signature (0x1f 0x9d), it stores the maxbits
 *   value (9..16) used at compression time in the following byte (and it
 *   also stores the flag bitmask 0x80 in that byte to distinguish the file
 *   format from file format 2), and it is independent of byte order and bit
 *   order. It uses block mode. Block mode makes it possible for the
 *   compressor to send a CLEAR code, which makes the decompressor clear the
 *   dictionary. This can improve the compression ratio if the data
 *   distribution changes in the input file after the distionary has been
 *   filled.
 *
 * File format version support:
 *
 * * File format 1 cannot be autodetected based on signatures, because its
 *   first byte is arbitrary, and the next byte is also almost arbitrary
 *   (for little endian order, its lowest bit must be 0). Thus we don't
 *   support decompressing file format 1.
 * * compress versions 1.x and earlier compress and decompress file format 1
 *   exclusively.
 * * Little-endian file format 1 can be compressed and decompressed by
 *   compress 2.0 and 3.0 (maybe also later versions) if compress.c is
 *   recompiled with `cc -DCOMPATIBLE` (not enabled by default).
 * * compress 2.0 can compress and decompress file format 2.
 * * compress >=3.0, gzip and we can decompress file formats 2 and 3,
 *   autodetecting the file format based on the byte value after the
 *   signature bytes.
 * * compress >=3.0 compresses file format 3 if the commnd-line flag `-C` is
 *   not specified (this is the default).
 * * compress 3.0 compresses file format 2 if the commnd-line flag `-C` is
 *   specified (not enabled by the default). This can be decompressed by
 *   compress >=2.0, gzip and also us.
 * * At some time after compress 3.0 (but not later than ncompress 4.2.4.4,
 *   which has the bug) a compression bug has been introduced, and if the
 *   command-line flag `-C` is specified for compression, the output file
 *   will be invalid and non-decompressible by any compress version. If
 *   you want to create file format 2, use compress 2.0 or compress 3.0
 *   (the latter with the `-C` flag).
 */

#include "luuzcat.h"

/* Below p is a const uc8* (const unsigned char*). */
#ifdef IS_UNALIGNED_LE
#  define LOAD_UINT_16BP(p) (((const um16*)(p))[0])  /* This does READ_BUFFER_OVERSHOOT. */
#  define LOAD_UINT_24BP(p) (((const unsigned int*)(p))[0])  /* This works only if sizeof(unsigned int) > 2. */  /* This does READ_BUFFER_OVERSHOOT. */
#else
#  define LOAD_UINT_16BP(p) ((p)[0] | (p)[1] << 8)
#  define LOAD_UINT_24BP(p) ((p)[0] | (p)[1] << 8 | (p)[2] << 16)  /* This works only if sizeof(unsigned int) > 2. */
#endif

#define BITS 16
#define INIT_BITS 9              /* Initial number of bits per code */

#if READ_BUFFER_SIZE + READ_BUFFER_EXTRA > 0xffffU - READ_BUFFER_EXTRA - BITS  /* BITS is included here for posbyte_after_read calculations. */
#  error Input buffer too large for 16-bit compress (LZW) calculations.
#endif
#if READ_BUFFER_SIZE < 15  /* Needed by `global_inptr += discard_byte_count'. */
#  error Input buffer too small for compress (LZW) calculations.
#endif

#define BITMASK    0x1f /* Mask for 'number of compression bits' */
/* Mask 0x20 is reserved to mean a fourth header byte, and 0x40 is free.
 * It's a pity that old uncompress does not check bit 0x20. That makes
 * extension of the format actually undesirable because old compress
 * would just crash on the new format instead of giving a meaningful
 * error message. It does check the number of bits, but it's more
 * helpful to say "unsupported format, get a new version" than
 * "can only handle 16 bits".
 */

#define BLOCK_MODE  0x80
/* Block compression: if table is full and compression rate is dropping,
 * clear the dictionary.
 */

#define LZW_RESERVED 0x60 /* reserved bits */

#define	CLEAR  256       /* flush the dictionary */
#define FIRST  (CLEAR+1) /* first free entry */

/* !! Submit patch: gzip 1.14 has INBUF_EXTRA == 64 here, which is a bit wasteful: BITS + (sizeof(long) - 2) is enough there. */
/* !! Submit patch: gzip 1.14 still has `#define OUTBUF_EXTRA 2048', completely unnecessary by unlzw.c. */

/* The code works equivalently with 2-byte, 4-byte and 8-byte uint (unsigned
 * short, unsigned int, unsigned long, unsigned long long (e.g. with GCC
 * __extension__ typedef)). We use `unsigned int' because provides the
 * fastest native speeds.
 */
typedef unsigned int uint;

/* !! patch: gzip 1.2.4 allocates 0x2000*2 bytes with SMALL_MEM for its
 * stack in d_buf, which is too little. Report bug. The maximum which can be
 * used is is 0xffffU - 255U, and it happens with an uncompressed input of
 * consisting of 4938271605 repetition of the same byte.
 */

/* Decompression uses 2 large arrays: tab_prefix is ~128 KiB and tab_suffix
 * is ~64 KiB. In gzip-1.2.4/unlzw.c, there was a 3rd array: ~64 KiB (if
 * implemented correctly) for stack, but this implementation does the
 * decompression without the stack array, with a slowdown factor ~1.78 for
 * streams with very long runs of the same byte, and more modest slowdown or
 * regular files. In addition to these large arrays, there is the output
 * buffer, which should be at least 32 KiB for decent speeds (for smaller
 * buffers the workaround which avoids the stack array gets much slower) and
 * at least 64 KiB for full speed; and there is also the input buffer (the
 * usual 4 KiB is enough for good speed). The input and output buffers are
 * not considered large arrays.
 *
 * Most modern targets (since about 1993) are 32-bit or 64-bit, and they can
 * accommodate the two large arrays (~128 KiB and ~64 KiB). However, DOS
 * 8086 is an exception, and we provide multiple, compiler-specific
 * implementations below using far pointers for the two large arrays.
 */
#if !IS_DOS_16
  /* This is the large array implementation for targets supporting arrays
   * larger than 64 KiB. Most modern 32-bit and 64-bit targets do so.
   */
  /* For faster code string writes. Resize it to 1 to save ~64 KiB of
   * memory. Must be large enough to fit a code string of this size: 1
   * finchar, 0xff00 codes pointing back, 1 literal code.
   */
  static uc8 lzw_stack[(1UL << BITS) - 254U];
  static uc8  tab_suffix_ary[(1UL << BITS) - 256U];  /* ~64 KiB. For each code, it contains the last byte. */
  static um16 tab_prefix_ary[(1UL << BITS) - 256U];  /* ~128 KiB. Prefix for each code. */
#  define tab_prefixof(code) (tab_prefix_ary - 256)[(code)]  /* Indexes 0 <= code < 256 are invalid and unused. */
#  define tab_suffixof(code) (tab_suffix_ary - 256)[(code)]  /* Indexes 0 <= code < 256 are invalid and unused. */
#  define tab_prefix_get(code) tab_prefixof(code)  /* Indexes 0 <= code < 256 are invalid and unused. */
#  define tab_prefix_set(code, value) (tab_prefixof(code) = (value))  /* Indexes 0 <= code < 256 are invalid and unused. */
#  define tab_suffix_get(code) tab_suffixof(code)  /* Indexes 0 <= code < 256 are invalid and unused. */
#  define tab_suffix_set(code, value) (tab_suffixof(code) = (value))  /* Indexes 0 <= code < 256 are invalid and unused. */
#  define tab_init() do {} while (0)
#endif
#if IS_DOS_16 && defined(__WATCOMC__) && !defined(__TURBOC__)
  /* This is the optimized far pointer implementation of the large arrays
   * for the OpenWatcom C compiler targeting DOS 8086.
   */
#  define lzw_stack big.compress.dummy  /* static uc8 lzw_stack[1]; */  /* Used only to check its size. */
  static __segment tab_prefix0_seg, tab_prefix1_seg, tab_suffix_seg;
#  if 0  /* This works. */
    static um16 tab_prefix_get(um16 code) { return *((const um16 __far*)(((code & 1 ? tab_prefix1_seg : tab_prefix0_seg) :> (code & ~1U)))); }  /* Indexes 0 <= code < 256 are invalid and unused. */
    static void tab_prefix_set(um16 code, um16 value) {  *((um16 __far*)(((code & 1 ? tab_prefix1_seg : tab_prefix0_seg) :> (code & ~1U)))) = value; }  /* Indexes 0 <= code < 256 are invalid and unused. */
    static uc8  tab_suffix_get(um16 code) { return *((const uc8 __far*)((tab_suffix_seg :> code))); }  /* Indexes 0 <= code < 256 are invalid and unused. */
    static void tab_suffix_set(um16 code, uc8 value) {  *((uc8 __far*)((tab_suffix_seg :> code))) = value; }  /* Indexes 0 <= code < 256 are invalid and unused. */
#  else  /* This is the optimized alternative in __WATCOMC__ 8086 assembly. */
    static um16 tab_prefix_get(um16 code);
    /* tab_prefix_get(...) works with __modify [__es] and __modify [__es __bx], but it doesn't work with __modify __exact [es __bx] */
    /* The manual register allocation of __value [__bx] __parm [__bx] is useful for the code = tab_prefix_get(code) below. */
#    pragma aux tab_prefix_get = "shr bx, 1"  "jc odd"  "mov es, tab_prefix0_seg"  "jmp have"  "odd: mov es, tab_prefix1_seg"  "have: add bx, bx"  "mov bx, es:[bx]" __value [__bx] __parm [__bx] __modify [__es __bx]
    static void tab_prefix_set(um16 code, um16 value);
#    pragma aux tab_prefix_set = "shr bx, 1"  "jc odd"  "mov es, tab_prefix0_seg"  "jmp have"  "odd: mov es, tab_prefix1_seg"  "have: add bx, bx"  "mov es:[bx], ax" __parm [__bx] [__ax] __modify __exact [__es __bx]
    static uc8 tab_suffix_get(um16 code);
#    pragma aux tab_suffix_get = "mov es, tab_suffix_seg"  "mov al, es:[bx]" __value [__al] __parm [__bx] __modify __exact [__es]
    static void tab_suffix_set(um16 code, uc8 value);
#    pragma aux tab_suffix_set = "mov es, tab_suffix_seg"  "mov es:[bx], al" __parm [__bx] [__al] __modify __exact [__es]
#  endif
#  ifndef _DOSCOMSTART
#    include <malloc.h>  /* For OpenWatcom __DOS__ halloc(...). */
#  endif
  static void tab_init(void) {
    /* We use OpenWatcom-specific halloc(...), because all our attempts to
     * create __far arrays (such as uc8 `__far tab_suffix_ary[(1UL << BITS)
     * - 256U];') have added lots of NUL bytes to the .exe.
     *
     * halloc(...) also zero-initializes. We don't need that, but it doesn't
     * hurt either.
     */
#  define TAB_PARA_COUNT (unsigned short)((((1UL << BITS) - 256U) * 3 + 15) >> 4)  /* Number of 16-byte paragraphs needed by tables above. */
#  ifndef _DOSCOMSTART
    void __far *a;
#  endif
    unsigned short segment;
    if (tab_suffix_seg) return;  /* Prevent subsequent initialization. */
#  ifdef _DOSCOMSTART
#    define DOS_COM_PROG_PARA_COUNT 0x1000U  /* 64 KiB == 0x1000 16-byte paragraphs. */
    /* *((unsigned short)2) fetches top program segment from the PSP (https://fd.lod.bz/rbil/interrup/dos_kernel/2126.html) of the DOS .com program. */
    if (TAB_PARA_COUNT + DOS_COM_PROG_PARA_COUNT > get_prog_mem_end_seg() - get_psp_seg()) fatal_out_of_memory();
    segment = get_psp_seg() + (TAB_PARA_COUNT + DOS_COM_PROG_PARA_COUNT);
#  else
    a = halloc(((unsigned long)TAB_PARA_COUNT) << 4, 1);  /* Always returns offset = 0. */  /* !! Reuse this between different decompressors (hfree(...)?). */
    segment = (unsigned long)a >> 16;
    if (segment == 0U || (unsigned short)a != 0U) fatal_out_of_memory();
#  endif
    tab_prefix0_seg = segment - (256U >> 4);  /* For each code, it contains the last byte. */
    tab_prefix1_seg = segment - (256U >> 4) + (unsigned short)(((1UL << BITS) - 256U) >> 4);  /* Prefix for even codes. */
    tab_suffix_seg  = segment - (256U >> 4) + (unsigned short)(((1UL << BITS) - 256U) >> 4) * 2;  /* Prefix for odd codes. */
  }
#endif
#if IS_DOS_16 && 0 && defined(__WATCOMC__) && !defined(__TURBOC__)
  /* This is the basic (unoptimized but working) far pointer implementation
   * of the larger arrays with the DOS 8086 target. Currently it works with
   * the OpenWatcom C compiler, but with some work it could be made work for
   * other compilers as well.
   *
   * Unfortunately, OpenWatcom, for each byte in the arrays below, adds a
   * NUL byte to the .exe. There is no way to add global far variables to
   * _BSS. The optimized implementation above avoids this problem by using
   * dynamic memory allocation with halloc(...) instead.
   */
#  define lzw_stack big.compress.dummy  /* static uc8 lzw_stack[1]; */  /* Used only to check its size. */
  static uc8  __far tab_suffix_ary[(1UL << BITS) - 256U];  /* For each code, it contains the last byte. */
  static um16 __far tab_prefix0_ary[((1UL << BITS) - 256U) >> 1];  /* Prefix for even codes. */
  static um16 __far tab_prefix1_ary[((1UL << BITS) - 256U) >> 1];  /* Prefix for odd  codes. */
  um16 __far *tab_prefix_fptr_ary[2];
#  define tab_prefixof(code) tab_prefix_fptr_ary[(code) & 1][((code) - 256U) >> 1]  /* Indexes 0 <= code < 256 are invalid and unused. */
#  define tab_prefix_get(code) tab_prefixof(code)  /* Indexes 0 <= code < 256 are invalid and unused. */
#  define tab_prefix_set(code, value) (tab_prefixof(code) = (value))  /* Indexes 0 <= code < 256 are invalid and unused. */
#  define tab_suffixof(code) tab_suffix_ary[(code) - 256U]  /* Indexes 0 <= code < 256 are invalid and unused. */
#  define tab_suffix_get(code) tab_suffixof(code)  /* Indexes 0 <= code < 256 are invalid and unused. */
#  define tab_suffix_set(code, value) (tab_suffixof(code) = (value))  /* Indexes 0 <= code < 256 are invalid and unused. */
#  define tab_init() do { tab_prefix_fptr_ary[0] = tab_prefix0_ary; tab_prefix_fptr_ary[1] = tab_prefix1_ary; } while (0)
#endif
#if IS_DOS_16 && !defined(__WATCOMC__) && defined(__TURBOC__)
  /* This is the memory-optimized (but not speed-optimized) far pointer
   * implementation of the large arrays for the Turbo C++ 1.00 or 1.01
   * compilers targeting DOS 8086. Don't specify the `-A' flag at compile
   * time, it will hide the `far' keyword needed here.
   *
   * To avoid adding NUL bytes to the .exe, we use allocmem(...) to allocate
   * these arrays. We don't need them initialized.
   */
#  include <dos.h>  /* For Turbo C++ allocmem(...), MK_FP(...) and FP_SEG(...). */
#  define lzw_stack big.compress.dummy  /* static uc8 lzw_stack[1]; */  /* Used only to check its size. */
  static uc8 far *tab_suffix_fptr;  /* [(1UL << BITS) - 256U]. For each code, it contains the last byte. */
  um16 far *tab_prefix_fptr_ary[2];  /* [((1UL << BITS) - 256U) >> 1]. [0] is prefix for even codes, [1] is prefix for odd codes. */
#  define tab_prefixof(code) tab_prefix_fptr_ary[(code) & 1][(code) >> 1]  /* Indexes 0 <= code < 256 are invalid and unused. */
#  define tab_prefix_get(code) tab_prefixof(code)  /* Indexes 0 <= code < 256 are invalid and unused. */
#  define tab_prefix_set(code, value) (tab_prefixof(code) = (value))  /* Indexes 0 <= code < 256 are invalid and unused. */
#  define tab_suffixof(code) tab_suffix_fptr[(code)]  /* Indexes 0 <= code < 256 are invalid and unused. */
#  define tab_suffix_get(code) tab_suffixof(code)  /* Indexes 0 <= code < 256 are invalid and unused. */
#  define tab_suffix_set(code, value) (tab_suffixof(code) = (value))  /* Indexes 0 <= code < 256 are invalid and unused. */
  static void tab_init(void) {
#  define TAB_PARA_COUNT (unsigned short)((((1UL << BITS) - 256U) * 3 + 15) >> 4)  /* Number of 16-byte paragraphs needed by tables above. */
    unsigned segment;
    if (FP_SEG(tab_suffix_fptr) != 0) return;  /* Prevent subsequent initialization. */
    segment = 0;  /* In case allocmem(...) doesn't set it on failure. */
    (void)allocmem(TAB_PARA_COUNT, &segment);  /* !! Reuse this between different decompressors (hfree(...)?). */
    if (segment == 0U) fatal_out_of_memory();
    tab_prefix_fptr_ary[0] = MK_FP(segment - (256U >> 4), 0);  /* For each code, it contains the last byte. */
    tab_prefix_fptr_ary[1] = MK_FP(segment - (256U >> 4) + (unsigned short)(((1UL << BITS) - 256U) >> 4), 0);  /* Prefix for even codes. */
    tab_suffix_fptr        = MK_FP(segment - (256U >> 4) + (unsigned short)(((1UL << BITS) - 256U) >> 4) * 2, 0);  /* Prefix for odd codes. */
  }
#endif
#if IS_DOS_16 && 0 && !defined(__WATCOMC__) && defined(__TURBOC__)
  /* This is the basic (unoptimized but working) far pointer implementation
   * of the large arrays for the Turbo C++ 1.00 or 1.01 compilers targeting
   * DOS 8086. Don't specify the `-A' flag at compile time, it will hide
   * the `far' keyword needed here.
   *
   * Unfortunately, Turbo C++, for each byte in the arrays below, adds a
   * NUL byte to the .exe. There is no way to add global far variables to
   * _BSS. The optimized implementation above avoids this problem by using
   * dynamic memory allocation with allocmem(...) instead.
   */
#  define lzw_stack big.compress.dummy  /* static uc8 lzw_stack[1]; */  /* Used only to check its size. */
  static uc8  far tab_suffix_ary[(1UL << BITS) - 256U];  /* For each code, it contains the last byte. */
  static um16 far tab_prefix0_ary[((1UL << BITS) - 256U) >> 1];  /* Prefix for even codes. */
  static um16 far tab_prefix1_ary[((1UL << BITS) - 256U) >> 1];  /* Prefix for odd  codes. */
  um16 far *tab_prefix_fptr_ary[2];
#  define tab_prefixof(code) tab_prefix_fptr_ary[(code) & 1][((code) - 256U) >> 1]  /* Indexes 0 <= code < 256 are invalid and unused. */
#  define tab_prefix_get(code) tab_prefixof(code)  /* Indexes 0 <= code < 256 are invalid and unused. */
#  define tab_prefix_set(code, value) (tab_prefixof(code) = (value))  /* Indexes 0 <= code < 256 are invalid and unused. */
#  define tab_suffixof(code) tab_suffix_ary[(code) - 256U]  /* Indexes 0 <= code < 256 are invalid and unused. */
#  define tab_suffix_get(code) tab_suffixof(code)  /* Indexes 0 <= code < 256 are invalid and unused. */
#  define tab_suffix_set(code, value) (tab_suffixof(code) = (value))  /* Indexes 0 <= code < 256 are invalid and unused. */
#  define tab_init() do { tab_prefix_fptr_ary[0] = tab_prefix0_ary; tab_prefix_fptr_ary[1] = tab_prefix1_ary; } while (0)
#endif

/* A helper function for calculating code_count. */
#if defined(__WATCOMC__) && defined(_M_I86)
#  define DIVMODCALC 4
#else
#  define DIVMODCALC 2
#endif

/* Declaration if it is a function: static unsigned divmodcalc(unsigned diff, unsigned n_bits, unsigned char posbiti);
 * It must work only if 0 <= diff <= 0xffffU and 9 <= n_bits <= 16 and posbiti <= 7.
 */
#if DIVMODCALC == 1  /* This is correct, but it uses unsigned long division, which is slow. */
#  define divmodcalc(diff, n_bits, posbiti) ((unsigned int)((((unsigned long)(diff) << 3) - (posbiti)) / (n_bits)))
#endif
#if DIVMODCALC == 2  /* This doesn't use unsigned long division, but it may use 2 or 3 divisions, depending on the compiler. (__WATCOMC__ generates 3 divs) */
#  define divmodcalc(diff, n_bits, posbiti) ((((diff) / (n_bits)) << 3) + ((((diff) % (n_bits)) << 3) + (n_bits) - (posbiti)) / (n_bits) - 1U)
#endif
#if DIVMODCALC == 3  /* Optimized 8086 assembly implementation which uses only 2 divs by reusing both the quotient (in AX) and the remainder (in DX). */
  static unsigned divmodcalc(unsigned diff, unsigned n_bits, unsigned char posbiti);
#  pragma aux divmodcalc = "xor dx, dx"  "div bx"  "mov cl, 3" \
      "shl ax, cl"  "shl dl, cl"  "dec ax"  "add dl, bl"  "sub dl, ch" \
      "xchg cx, ax"  "xchg ax, dx"  "xor dx, dx"  "div bx"  "add ax, cx" \
      __parm [__ax] [__bx] [__ch] __value [__ax] __modify [__cx __dx]
#endif
#if DIVMODCALC == 4  /* Optimized 8086 assembly implementation which uses only 1 div. */
  static unsigned divmodcalc(unsigned diff, unsigned n_bits, unsigned char posbiti);
  /* Unfortunately we can't use the shorter `cbw' instead of `xor dx ,dx'
   * here, because `cbw' works if AX (diff) < 0x7fffU, and it can be a bit
   * larger: it's maximum value, READ_BUFFER_SIZE + READ_BUFFER_EXTRA is
   * 0x8002U.
   */
#  pragma aux divmodcalc = "xor dx, dx"  "shl ax, 1"  "rcl dx, 1"  \
      "shl ax, 1"  "rcl dx, 1"  "shl ax, 1"  "rcl dx, 1"  "sub ax, cx" \
      "sbb dx, 0"  "div bx" \
      __parm [__ax] [__bx] [__cx] __value [__ax] __modify [__dx]
#endif

#ifdef USE_DEBUG
static void abort_line(unsigned int line) {
  fprintf(stderr, "abort at line %u\n", line);
  abort();
}
#define abort() abort_line(__LINE__)
#endif

void decompress_compress_nohdr(void) {
  uint code, incode, oldcode;  /* 0 <= ... <= 0ffffU. */
  ub8 is_very_first_code;
  uc8 finchar;
  uint code_count;
  uint code_count_remainder;  /* Only the low 3 bits matter, so it can overflow. */
  um8 posbiti;  /* 0 <= posbiti <= 7. */
  uint discard_byte_count;  /* 0 <= discard_byte_count <= 15. */
  uint write_idx;  /* 0 <= write_idx <= WRITE_BUFFER_SIZE. */
  uint bitmask;  /* 0 <= bitmask <= 0xffffU. */
  uint free_ent1;  /* 255 <= free_ent1 <= 0xffffU. */
  uint maxcode;  /* 0 <= maxcode <= 0x10000U. If sizeof(uint) == 2, then the maximum value overflows to 0U. */
  uint maxmaxcode1;  /* 511 <= maxmaxcode1 <= 0xffffU. It's always 1 less than a power of 2. */
  uint n_bits;  /* 9 <= n_bits <= 16. */
  uint rsize;
  uint maxbits;  /* max bits per code for LZW */
  ub8 block_mode;  /* Nonzero for block mode (compress 3.0 without `-C'), zero for non-block mode (compress 2.0). */
  uint w_finchar;  /* 0 <= w_finchar <= 256. */
  uint w_idx;  /* 0 <= write_idx <= WRITE_BUFFER_SIZE. */
  uint w_code;  /* 0 <= w_code <= 0xffffU. */
  uint w_size;  /* 0 <= w_size <= 0xffffU. */
  uint w_skip;  /* 0 <= w_skip <= 0xffffU. */
  uc8 *stack_top;

  tab_init();
  maxbits = get_byte();
  block_mode = (maxbits & BLOCK_MODE) ? 1 : 0;
#ifdef USE_DEBUG
  fprintf(stderr, "COMPRESSED maxbits=%u block_mode=is_3plus=%u reserved=%u\n", maxbits & BITMASK, block_mode, (maxbits & LZW_RESERVED) != 0 ? 1 : 0);
#endif
  if ((maxbits & LZW_RESERVED) != 0) fatal_corrupted_input();  /* A reserved bit is 1. No known (current or earlier) version of (n)compress in 2025 sets that. */
  maxbits &= BITMASK;
#if defined(__i386__) && defined(__amd64__) || defined(__386__) || defined(_M_I86) || defined(_M_I386)
  maxmaxcode1 = (1U << maxbits) - 1U;  /* On x86, it's OK to shift a 16-bit value by 16 and get 0. */
#else
  maxmaxcode1 = (sizeof(maxmaxcode1) > 2) ? (1U << maxbits) - 1U : (maxbits == 16 ? 0xffffU : (1U << maxbits) - 1U);
#endif
#ifdef USE_DEBUG
  fprintf(stderr, "maxmaxcode1=%lu\n", (unsigned long)maxmaxcode1);
#endif
  /* !! report gzip-1.14 bug: Check maxbits < INIT_BITS as well! */
  if (maxbits < INIT_BITS || maxbits > BITS) fatal_corrupted_input();
  rsize = global_insize;
  n_bits = INIT_BITS;
  maxcode = (1U << INIT_BITS) - 1U;  /* Doesn't overflow 0xffffU. */
  bitmask = ((1U << (INIT_BITS - (sizeof(unsigned int) > 2 ? 0 : 8))) - 1U);
  oldcode = 0;  /* Not needed. */
  is_very_first_code = 1;
  finchar = 0;
  write_idx = 0;
  posbiti = 0;
  free_ent1 = FIRST - 1;
  if (!block_mode) --free_ent1;
  discard_byte_count = 0;
  code_count_remainder = 0;
  for (;;) {
    if (global_insize - global_inptr < discard_byte_count) {
      discard_byte_count -= global_insize - global_inptr;
      global_inptr = global_insize = 0;
    } else {
      global_inptr += discard_byte_count;
      discard_byte_count = 0;
      if (sizeof(unsigned int) > 2) {
        code_count = (((global_insize - global_inptr) << 3) - posbiti) / n_bits;  /* The `<< 3' won't overflow. Good. */
      } else {
        code_count = divmodcalc(global_insize - global_inptr, n_bits, posbiti);
      }
      if (code_count != 0) goto after_read;
      /* Now we have code_count == 0, which implies that there aren't enough
       * bits remaining for a single code. Thus global_insize - global_inptr
       * >= 3 is impossible, because that would imply that there are at
       * least 24 - posbiti bits available, and minimum_bits_needed ==
       * n_bits <= 16 < 17 == 24 - 7 <= 24 - posbiti <= bits_avaialble.
       */
#ifdef USE_CHECK
      if (global_insize - global_inptr > 2) abort();
#endif
      if (global_insize > READ_BUFFER_EXTRA) {
        global_insize -= global_inptr;
#ifdef USE_CHECK
        if (global_insize > READ_BUFFER_EXTRA) abort();  /* This can't happen, because now global_insize <= 2 == READ_BUFFER_EXTRA. */
#endif
#ifdef USE_DEBUG
        fprintf(stderr, "RESET_COPY insize=%lu inptr=%lu n_bits=%lu\n", (unsigned long)global_insize, (unsigned long)global_inptr, (unsigned long)n_bits);
#endif
        for (w_idx = 0 ; w_idx < global_insize; ++w_idx) {  /* No need for memmove(...), global_insize is small. */
          global_read_buffer[w_idx] = global_read_buffer[w_idx + global_inptr];
        }
        global_inptr = 0;
      }
    }
    /* Doing == (uint)-1U, because with `(int)... < 0), 0x8000U would fail. */
    if ((rsize = read(STDIN_FILENO, global_read_buffer + global_insize, READ_BUFFER_SIZE)) == (uint)-1U) fatal_read_error();
#ifdef USE_DEBUG
    if (1) fprintf(stderr, "READ(%u)=%lu\n", READ_BUFFER_SIZE, (unsigned long)rsize);
#endif
    if (rsize == 0) break;
    global_insize += rsize;
    continue;
   after_read:
#ifdef USE_CHECK
    if (code_count == 0) abort();  /* Can't happen here. */
#endif
    code_count_remainder += code_count;
    if (is_very_first_code) {  /* Happens at the very beginning of the input only. */
      code = global_read_buffer[global_inptr++];
      if ((global_read_buffer[global_inptr] & 1) != 0) fatal_corrupted_input();  /* First code must be 0 <= code <= 255. */
      ++posbiti;  /* Advance 9 bits by increasing this from 0 to 1. */
#ifdef USE_DEBUG
      if (code >= 256U) abort();  /* Implied by the assignment above. */
#endif
      --is_very_first_code;  /* = 0. */
      flush_full_write_buffer_in_the_beginning();  /* Usually this is a no-op, because the write buffer is not full in the beginning. */
      global_write_buffer[write_idx++] = (finchar = (uc8)(oldcode = code));
      if (--code_count == 0) continue;
    }
   maybe_next_code:
#ifdef USE_DEBUG
    if (0) fprintf(stderr, "READ code_count=%lu inptr=%lu %lu/%lu\n", (unsigned long)code_count, (unsigned long)global_inptr, (unsigned long)free_ent1, (unsigned long)(unsigned short)maxcode);
#endif
    if ((sizeof(maxcode) > 2) ? (free_ent1 >= maxcode) : (free_ent1 > (uint)(maxcode - 1U))) {
#ifdef USE_DEBUG
      fprintf(stderr, "INCBITS inptr=%lu\n", (unsigned long)global_inptr);
#endif
      ++n_bits;
      bitmask <<= 1; ++bitmask;  /* Faster than bitmask = (1U << n_bits) - 1U; */
      maxcode <<= 1; ++maxcode;
      if (n_bits == maxbits) ++maxcode;  /* This may overflow from 0xffffU to 0U. That's OK. */
      if (n_bits == 10 && !block_mode) {
#ifdef USE_CHECK
        if (posbiti != 1) abort();
        if (((code_count_remainder - code_count) & 7) != 1) abort();
#endif
        posbiti = 0;
        discard_byte_count = 8;
      } else {
        /* Except for the first increment of bits in non-block mode, all the
         * remainders are 0 now, because the modulus is old_n_bits << 3
         * bits, and actually old_n_bits << (old_nbits - 1) bits have been
         * read since the last discard, and old_nbits - 1 >= 3. Thus we
         * don't have to do anything here.
         */
#ifdef USE_CHECK
        if (posbiti != 0) abort();
        if (((code_count_remainder - code_count) & 7) != 0) abort();
        if (discard_byte_count != 0) abort();
#endif
      }
     do_discard_bytes:
      code_count_remainder = 0;
      /* Discarding of additional bytes here (if discard_byte_count > 0) is
       * a backward-compatible quirk of the compress format. The compressed
       * file would have been shorter if the compressor had omitted these
       * bytes in the first place.
       */
      continue;
    }
    /* Now: 9 == INIT_BITS <= n_bits <= maxbits <= BITS == 16. */
    /* Now: bitmask == (1UL << n_bits) - 1UL. */
#ifdef USE_DEBUG
    if (n_bits > (maxbits == 9 ? 10 : maxbits) || n_bits < INIT_BITS) abort();
#endif
    /* Read n_bits (9 <= n_bits <= 16) from global_read_buffer at posbits, advance posbits. */
    if (n_bits == 16)  {  /* Shortcut. Now posbiti == 0. */
      code = LOAD_UINT_16BP(&global_read_buffer[global_inptr]) & 0xffffU;
      global_inptr += 2;
    } else {
      if (sizeof(unsigned int) > 2) {  /* Works for 1 <= n_bits <= 17, but we only use it for 9 <= n_bits <= 16. */
        code = (LOAD_UINT_24BP(&global_read_buffer[global_inptr]) >> posbiti) & bitmask;
      } else {  /* Works for 9 <= n_bits <= 16. */
        code = ((LOAD_UINT_16BP(&global_read_buffer[global_inptr]) >> posbiti) & 0xffU) | (((LOAD_UINT_16BP(&global_read_buffer[global_inptr + 1]) >> posbiti) & bitmask) << 8);
      }
      ++global_inptr;
      if ((posbiti += n_bits - 8) > 7) {
        ++global_inptr;
        posbiti -= 8;
      }
    }
    --code_count;
#ifdef USE_DEBUG
    if (0) fprintf(stderr, "READ code=%lu n_bits=%lu icl=%lu\n", (unsigned long)code, (unsigned long)n_bits, (unsigned long)code_count);
#endif
    if (code == CLEAR && block_mode) {
#ifdef USE_DEBUG
      fprintf(stderr, "CLEAR\n");
#endif
      /* !! Report unnecessary code in gzip-1.14: clear_tab_prefixof(). The size 256 is also buggy. */
      if (posbiti != 0) { ++global_inptr; posbiti = 0; }
      discard_byte_count = (((code_count_remainder - code_count) & 7) * n_bits + 7) >> 3;
      if ((discard_byte_count %= n_bits) != 0) discard_byte_count = n_bits - discard_byte_count;
      free_ent1 = FIRST - 1 - 1;
      n_bits = INIT_BITS;
      maxcode = (1U << INIT_BITS) - 1U;  /* Doesn't overflow 0xffffU. */
      bitmask = ((1U << (INIT_BITS - (sizeof(unsigned int) > 2 ? 0 : 8))) - 1U);
      goto do_discard_bytes;
    }
    incode = code;
    /* Good C compilers keep only one of the `if' branches below, eliminating the other one from the executable. */
    if (sizeof(lzw_stack) >= (1UL << BITS) - 254U) {  /* We use the a faster code string writer implementation. */
      stack_top = lzw_stack + sizeof(lzw_stack);
      if (code > free_ent1) { /* Special case for KwKwK string. */
        if (code - 1U > free_ent1) fatal_corrupted_input();  /* The `code - 1U' doesn't underflow, because code != 0U is guaranteed by the `if' right above. */
        *--stack_top = finchar;
        code = oldcode;
      }
      for (; code >= 256U; code = tab_prefix_get(code)) {
        *--stack_top = tab_suffix_get(code);  /* Generate characters of the code string in reverse order. */
      }
      *--stack_top = finchar = (uc8)code;
#if 0  /* This works, but it is slow. */
      while (stack_top != lzw_stack + sizeof(lzw_stack)) {
        global_write_buffer[write_idx++] = *stack_top++;
        if (write_idx == WRITE_BUFFER_SIZE) write_idx = flush_write_buffer(WRITE_BUFFER_SIZE);
      }
#else
      w_size = lzw_stack + sizeof(lzw_stack) - stack_top;
      if (w_size <= 4) {  /* Write a few bytes quickly, without memcpy(...) etc. */
        while (stack_top != lzw_stack + sizeof(lzw_stack)) {
          global_write_buffer[write_idx++] = *stack_top++;
          if (write_idx == WRITE_BUFFER_SIZE) write_idx = flush_write_buffer(WRITE_BUFFER_SIZE);
        }
      } else {
        while (w_size >= (w_skip = WRITE_BUFFER_SIZE - write_idx)) {
          memcpy(global_write_buffer + write_idx, stack_top, w_skip);
          write_idx = flush_write_buffer(WRITE_BUFFER_SIZE);
          stack_top += w_skip;
          w_size -= w_skip;
        }
        memcpy(global_write_buffer + write_idx, stack_top, w_size);
        write_idx += w_size;  /* It doesn't fill it fully. */
#ifdef USE_CHECK
        if (write_idx == WRITE_BUFFER_SIZE) abort();  /* Must not be full. */
#endif
      }
#endif
    } else {  /* Use the slower, but less memory-hungry code string writer implementation. It's slow because it does multiple passes on the linked list. */
      /* It became 2 times slower for long matches (4938271605 runs of the same byte): time ./uncompress <xes0.Z >xes0.bin && cmp xes0.good xes0.bin && echo OK
       * Please not that gcc -fsanitize=address is enabled.
       * lzw_stack (uncompress.c.7t): real 0m12.969s user    0m9.131s  sys     0m3.669s
       * old       (uncompress.c.5):  real 0m21.165s user    0m17.151s sys     0m3.891s
       * new       (uncompress.c.6c): real 0m39.502s user    0m35.492s sys     0m3.901s
       * new       (uncompress.c.6d): real 0m43.572s user    0m39.420s sys     0m4.042s  (Why did it become slower than .6c?)
       * new       (uncompress.c.6e): real 0m40.412s user    0m36.501s sys     0m3.782s
       * new       (uncompress.c.6f): real 0m37.575s user    0m33.750s sys     0m3.698s
       */
      if (code > free_ent1) { /* Special case for KwKwK string. */
        if (code - 1U > free_ent1) fatal_corrupted_input();  /* The `code - 1U' doesn't underflow, because code != 0U is guaranteed by the `if' right above. */
        w_finchar = finchar + 1;  /* After this, 1 <= w_finchar <= 256. */
        w_code = oldcode;
      } else {
        w_finchar = 0;
        w_code = code;
      }
      w_size = 1;  /* Count the last code first. */
      if (w_finchar != 0) ++w_size;
      for (code = w_code; code >= 256U; ++w_size, code = tab_prefix_get(code)) {}
      /* Now w_size is number of characters to write in reverse. */
      finchar = (uc8)code;
      /* Now generate output characters in reverse order. */
      for (;;) {
        if (w_size > WRITE_BUFFER_SIZE - write_idx) {  /* This is the rare case with long string or string near the end of global_write_buffer. */
#ifdef USE_DEBUG
          if (1) fprintf(stderr, "WRITE w_size=%lu remaining=%lu code=%lu\n", (unsigned long)w_size, (unsigned long)(WRITE_BUFFER_SIZE - write_idx), (unsigned long)code);
#endif
          w_idx = WRITE_BUFFER_SIZE;
          w_skip = w_size -= (WRITE_BUFFER_SIZE - write_idx);  /* After this, w_skip > 0 && w_size > 0. */
#ifdef USE_CHECK
          if (w_size == 0) abort();
          if (w_skip == 0) abort();
          if (w_idx == write_idx) abort();
#endif
          if (w_finchar != 0) --w_skip;
          for (code = w_code; w_skip != 0; --w_skip, code = tab_prefix_get(code)) {}
        } else {
          w_idx = write_idx + w_size;
          if (w_finchar != 0) {
            global_write_buffer[--w_idx] = (uc8)w_finchar - 1;
            if (w_idx == write_idx) break;
          }
          code = w_code;
          w_skip = 1;
        }
        for (; --w_idx != write_idx; code = tab_prefix_get(code)) {
          global_write_buffer[w_idx] = tab_suffix_get(code);
        }
        if (code >= 256U) code = tab_suffix_get(code);
        global_write_buffer[w_idx] = code;   /* Last code is always a literal. */
        if (w_skip != 0) break;  /* This was the last iteration, the entire string has been printed. */
        write_idx = flush_write_buffer(WRITE_BUFFER_SIZE);
      }
      if ((write_idx += w_size) == WRITE_BUFFER_SIZE) write_idx = flush_write_buffer(WRITE_BUFFER_SIZE);
    }
    if (free_ent1 < maxmaxcode1) {  /* Generate the new entry. */
      ++free_ent1;  /* Never overflows 0xffffU. */
      tab_prefix_set(free_ent1, oldcode);
      tab_suffix_set(free_ent1, finchar);
#ifdef USE_DEBUG
      if (0) fprintf(stderr, "free_ent1=%lu ", (unsigned long)free_ent1);
#endif
    }
    oldcode = incode;   /* Remember previous code. */
    if (code_count != 0) goto maybe_next_code;
  }
  if (global_insize - global_inptr != (posbiti > 0)) fatal_corrupted_input();  /* Unexpected EOF. */
  flush_write_buffer(write_idx);
  read_force_eof();
}
