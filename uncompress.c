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
 *   when copying from lzw_stack to compress_write_buffer has make it ~1.632
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

#ifdef _DOSCOMSTART_UNCOMPRC
#  define LUUZCAT_MAIN 1  /* Make luuzcat.h generate some functions early for the DOS .com target uncomprc.com. */
#  define READ_BUFFER_SIZE  0x1000  /* The larger, the more it reduces syscall overhead. 0x800 would be enough already. */
#  define WRITE_BUFFER_SIZE 0x2000  /* The larger, the faster output byte generation is. Unfortunately, 0x3000 would make the memry usage of the DOS .com program more than 64 KiB. */
#endif
#include "luuzcat.h"
#ifdef _DOSCOMSTART_UNCOMPRC
#  define read_force_eof() do {} while (0)
#endif

#ifdef LUUZCAT_DUCML  /* This functionality is duplicated in luuzcat.c for !LUUZCAT_DUCML. */
  int luuzcat_ducml_read(int fd, void *buf, unsigned int count);
#  pragma aux luuzcat_ducml_read = "mov ah, 3fh"  "int 21h"  "jnc ok"  "sbb ax, ax"  "ok:" \
      __value [__ax] __parm [__bx] [__dx] [__cx] __modify __exact [__ax]

  /* Like write(...), but count must be nonzero. */
  int luuzcat_ducml_write_nonzero(int fd, const void *buf, unsigned int count);
#  pragma aux luuzcat_ducml_write_nonzero = "mov ah, 40h"  "int 21h"  "jnc ok"  \
      "sbb ax, ax"  "ok:"  \
      __value [__ax] __parm [__bx] [__dx] [__cx] __modify __exact [__ax]

  /* MS-DOS 3.30 returns CF == 0 (success) and AX == 0 (no bytes written)
   * when writing \x1a (^Z) to the console. By `mov ax, cx', we simulate
   * returning maximum value in this case. !! Modify other write(...) calls.
   */
  int luuzcat_ducml_write_nonzero_or_fatal(int fd, const void *buf, unsigned int count);
#  pragma aux luuzcat_ducml_write_nonzero_or_fatal = "mov ah, 40h"  "int 21h"  "jnc noerr"  "jmp fatal_write_error"  "noerr: test ax, ax"  "jnz ok"  "mov ax, cx"  "ok:" \
      __value [__ax] __parm [__bx] [__dx] [__cx] __modify __exact [__ax]

  __noreturn void fatal_msg(const char *msg) {
    luuzcat_ducml_write_nonzero(STDERR_FILENO, WRITE_PTR_ARG_CAST(msg), strlen(msg));
    exit(EXIT_FAILURE);
  }

  __noreturn void fatal_read_error(void) { fatal_msg("read error" LUUZCAT_NL); }
  __noreturn void fatal_write_error(void) { fatal_msg("write error" LUUZCAT_NL); }
  __noreturn void fatal_unexpected_eof(void) { fatal_msg("unexpected EOF" LUUZCAT_NL); }
  __noreturn void fatal_corrupted_input(void) { fatal_msg("corrupted input" LUUZCAT_NL); }
#  ifdef LUUZCAT_MALLOC_OK  /* !! Also omit from luuzcatn.com. */
    __noreturn void fatal_out_of_memory(void) { fatal_msg("out of memory" LUUZCAT_NL); }
#  endif

  unsigned int read_byte(ub8 is_eof_ok) {
    int got;
    if (global_inptr < global_insize) return global_read_buffer[global_inptr++];
    if (global_read_had_eof) goto already_eof;
    if ((got = luuzcat_ducml_read(STDIN_FILENO, (char*)global_read_buffer, (int)READ_BUFFER_SIZE)) < 0) fatal_read_error();
    if (got == 0) {
      ++global_read_had_eof;  /* = 1. */
     already_eof:
      if (is_eof_ok) return BEOF;
      fatal_unexpected_eof();
    }
    global_total_read_size += global_insize = got;
    global_inptr = 1;
    return global_read_buffer[0];
  }

  /* A shorter, assembly implementation of flush_write_buffer(...) below. */
  static unsigned int flush_write_buffer_inline(unsigned int size);
#  pragma aux flush_write_buffer_inline = \
      "push dx"  "mov dx, offset global_write_buffer"  "push cx"  "push bx"  "mov bx, 1"  "xchg cx, ax" \
      "again: mov ah, 40h"  "int 21h"  "jnc noerr"  "jmp fatal_write_error"  "noerr: test ax, ax"  "jnz ok"  "mov ax, cx" \
      "ok: add dx, ax"  "sub cx, ax" "jnz again"  "xor ax, ax"  "pop bx"  "pop cx"  "pop dx" \
      __value [__ax] __parm [__ax] __modify __exact []
  /* Always returns 0, which is the new buffer write index.
   * flush_write_buffer(...) uses a buffer different from flush_write_buffer(...) uses.
   */
  unsigned int flush_write_buffer(unsigned int size) {
#  if 1
    return flush_write_buffer_inline(size);
#  else
    unsigned int size_i;
    for (size_i = 0; size_i < size; ) {
      /* MS-DOS 3.00 works with size as large as 0xffff. */
      size_i += luuzcat_ducml_write_nonzero_or_fatal(STDOUT_FILENO, WRITE_PTR_ARG_CAST((is_uncompress_now ? uncompress_ducml_write_buffer : global_write_buffer) + size_i), size - size_i);
    }
    return 0;
#  endif
  }

  struct uncompress_ducml_big_14 {
    uc8 write_buffer[0x2000];  /* The struct must start with write_buffer. Make it as lare as possible to avoid slow reversal in `if (w_size > COMPRESS_WRITE_BUFFER_SIZE - write_idx)' below. */
    uc8 tab_suffix_14_ary[(1U << 14) - 256U];
    uc8 padding1[256U];  /* Removing this wouldn't decrease the memory usage, because the size of struct uncompress_ducml_big_16 is unchanged. */
    um16 tab_prefix0_14_ary[((1UL << 14) - 256U) >> 1];
    uc8 padding2[256U];
    um16 tab_prefix1_14_ary[((1UL << 14) - 256U) >> 1];
    uc8 padding3[256U];
  };
  typedef char assert_sizeof_struct_uncompress_ducml_big_14[sizeof(struct uncompress_ducml_big_14) == 0xe000U ? 1 : -1];
  struct uncompress_ducml_big_15 {
    uc8 write_buffer[0x6000];  /* The struct must start with write_buffer. Make it as lare as possible to avoid slow reversal in `if (w_size > COMPRESS_WRITE_BUFFER_SIZE - write_idx)' below. */
    uc8 tab_suffix_15_ary[(1U << 15) - 256U];
    uc8 padding[256U];
    /* um16 tab_prefix0_15_ary[((1UL << 15) - 256U) >> 1]; */  /* On the heap. */
    /* um16 tab_prefix1_15_ary[((1UL << 15) - 256U) >> 1]; */  /* On the heap. */
  };
  typedef char assert_sizeof_struct_uncompress_ducml_big_15[sizeof(struct uncompress_ducml_big_15) == 0xe000U ? 1 : -1];
  struct uncompress_ducml_big_16 {
    uc8 write_buffer[0xe000];  /* The struct must start with write_buffer. Make it as lare as possible to avoid slow reversal in `if (w_size > COMPRESS_WRITE_BUFFER_SIZE - write_idx)' below. */
    /* uc8 tab_suffix_16_ary[(1U << 16) - 256U]; */  /* On the heap. */
    /* um16 tab_prefix0_16_ary[((1UL << 16) - 256U) >> 1]; */  /* On the heap. */
    /* um16 tab_prefix1_16_ary[((1UL << 16) - 256U) >> 1]; */  /* On the heap. */
  };
  typedef char assert_sizeof_struct_uncompress_ducml_big_16[sizeof(struct uncompress_ducml_big_16) == 0xe000U ? 1 : -1];
  union uncompress_ducml_big_u {
    uc8 write_buffer_and_more[0xe000];
    struct uncompress_ducml_big_14 b14;
    struct uncompress_ducml_big_15 b15;
    struct uncompress_ducml_big_16 b16;
  };
  typedef char assert_sizeof_struct_uncompress_ducml_big_u[sizeof(union uncompress_ducml_big_u) == 0xe000U ? 1 : -1];
  /* We must have CONST, CONST2, _DATA and _BSS empty for LUUZCAT_DUCML uncompress.c. Thus we use absolute addresses for these variables. */
#  define uncompress_ducmp_write_buffer_asm "mov word ptr ds:[flush_write_buffer+2], 0xc00"
  extern uc8 uncompress_ducml_write_buffer[];
#  pragma aux uncompress_ducml_write_buffer "uncompress_ducml_write_buffer__FIXOFS_0xc00"
#  if 1  /* This one happens to produce a smaller program file.. */
#    define uncompress_ducml_big (*(union uncompress_ducml_big_u*)0xc00)
#  else
    extern union uncompress_ducml_big_u uncompress_ducml_big;  /* Also contains uncompress_ducml_write_buffer. */
#    pragma aux uncompress_ducml_big "uncompress_ducml_big__FIXOFS_0xc00"
#endif
  extern unsigned int uncompress_ducml_write_buffer_size;
#  pragma aux uncompress_ducml_write_buffer_size "uncompress_ducml_write_buffer_size__FIXOFS_0xfc12"  /* Unfortunately there is no way to generate this with `wcc -za'. Without `-xa', there is _Pragma("..."). */

#  define COMPRESS_READ_BUFFER_SIZE READ_BUFFER_SIZE
#  define read_force_eof() exit(EXIT_SUCCESS)
#  define compress_read(fd, buf, count) luuzcat_ducml_read(fd, buf, count)
#  define compress_write_buffer uncompress_ducml_write_buffer
#else
#  ifdef LUUZCAT_SMALLBUF
#    define COMPRESS_READ_BUFFER_SIZE COMPRESS_SMALLBUF_READ_BUFFER_SIZE
#    define compress_read(fd, buf, count) read(fd, buf, count)
#    define compress_write_buffer big.compress.u.sn.sn_write_buffer
#  else
#    define COMPRESS_READ_BUFFER_SIZE READ_BUFFER_SIZE
#    define compress_read(fd, buf, count) read(fd, buf, count)
#    define compress_write_buffer global_write_buffer
#  endif
#endif  /* #else LUUZCAT_DUCML. */

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
#undef BITS
#undef COMPRESS_WRITE_BUFFER_SIZE
#if !IS_X86_16 && !defined(LUUZCAT_SMALLBUF)
  /* This is the large array implementation for targets supporting arrays
   * larger than 64 KiB. Most modern 32-bit and 64-bit targets do so.
   */
#  define BITS 16
#  define COMPRESS_WRITE_BUFFER_SIZE WRITE_BUFFER_SIZE
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
#  define tab_suffix_get(code) tab_suffixof(code)  /* Indexes 0 <= code < 256 are invalid and unused. */
#  define tab_prefix_suffix_set(code, prefix, suffix) (tab_prefixof(code) = (prefix), tab_suffixof(code) = (suffix))  /* Indexes 0 <= code < 256 are invalid and unused. */
#  define tab_init(maxbits) do {} while (0)
#endif
#if defined(LUUZCAT_SMALLBUF)
#  define BITS COMPRESS_SMALLBUF_BITS  /* 14. */
#  define COMPRESS_WRITE_BUFFER_SIZE COMPRESS_SMALLBUF_WRITE_BUFFER_SIZE
#  define lzw_stack big.compress.u.dummy_stack  /* Small size, so no stack, so slow decompression. */
#  define tab_prefixof(code) (big.compress.u.sn.tab_prefix_ary - 256)[(code)]  /* Indexes 0 <= code < 256 are invalid and unused. */
#  define tab_suffixof(code) (big.compress.u.sn.tab_suffix_ary - 256)[(code)]  /* Indexes 0 <= code < 256 are invalid and unused. */
#  define tab_prefix_get(code) tab_prefixof(code)  /* Indexes 0 <= code < 256 are invalid and unused. */
#  define tab_suffix_get(code) tab_suffixof(code)  /* Indexes 0 <= code < 256 are invalid and unused. */
#  define tab_prefix_suffix_set(code, prefix, suffix) (tab_prefixof(code) = (prefix), tab_suffixof(code) = (suffix))  /* Indexes 0 <= code < 256 are invalid and unused. */
#  define tab_init(maxbits) do {} while (0)
  /* !!! speed optimization: add back fast BITS <= 13 with lzw_stack. */
#endif
#if IS_X86_16 && !defined(LUUZCAT_SMALLBUF) && (!IS_DOS_16 || defined(_PROGX86_NOALLOC))
  /* This implementation supports maxbits <= 13 (so it doesn't support 14, 15 or 16), but it keeps the data segment under 64 KiB. For DOS .com programs, it keeps code+data under 64 KiB. */
#  define BITS 13
#  define COMPRESS_WRITE_BUFFER_SIZE (1U << 13)
#  define lzw_stack big.compress.u.noa.stack_supplement
#  define tab_suffixof(code) ((uc8*)(compress_write_buffer + COMPRESS_WRITE_BUFFER_SIZE - 256))[code]  /* In compress_write_buffer, after the COMPRESS_WRITE_BUFFER_SIZE bytes. */
#  define tab_prefixof(code) ((um16*)(compress_write_buffer + COMPRESS_WRITE_BUFFER_SIZE - 256 + (1 << BITS)) - 256)[code]  /* In compress_write_buffer, after tab_suffixof. */
#  if BITS > 14
#    error BITS too large for 16-bit noalloc uncompress.
#  endif
  typedef char assert_write_buffer_size_for_uncompress[sizeof(compress_write_buffer) >= COMPRESS_WRITE_BUFFER_SIZE + (3U << BITS) - 3U * 256 ? 1 : -1];
  typedef char assert_stack_size_for_uncompress[sizeof(lzw_stack) >= (1U << BITS) - 254U ? 1 : -1];
#  define tab_prefix_get(code) tab_prefixof(code)  /* Indexes 0 <= code < 256 are invalid and unused. */
#  define tab_suffix_get(code) tab_suffixof(code)  /* Indexes 0 <= code < 256 are invalid and unused. */
#  define tab_prefix_suffix_set(code, prefix, suffix) (tab_prefixof(code) = (prefix), tab_suffixof(code) = (suffix))  /* Indexes 0 <= code < 256 are invalid and unused. */
#  define tab_init(maxbits) do {} while (0)
#endif
#if IS_DOS_16 && !defined(LUUZCAT_SMALLBUF) && defined(LUUZCAT_DUCML) && defined(_PROGX86) && defined(_PROGX86_DOSMEM) && defined(_PROGX86_HAVE_PARA_REUSE_START) && defined(__SMALL__) && !defined(_DOSCOMSTART_UNCOMPRC) && !defined(_DOSCOMSTART) && !defined(_PROGX86_NOALLOC) && defined(__WATCOMC__) && !defined(__TURBOC__)
  /* This is the optimized far pointer implementation of the large arrays
   * for LUUZCAT_DUCML (using the OpenWatcom C compiler targeting DOS 8086).
   * This allocates heap memory only for bits == 15 and 16, and it allocates
   * less for bits == 15.
   */
#  define BITS 16
#  define COMPRESS_WRITE_BUFFER_SIZE uncompress_ducml_write_buffer_size
#  define lzw_stack big.compress.u.noa.crc32_table_dummy  /* static uc8 lzw_stack[1]; */  /* Used only to check its size. */
#  if 0
#    define tab_suffix_seg  (*(__segment*)0xfc0c)  /* We must have CONST, CONST2, _DATA and _BSS empty for LUUZCAT_DUCML uncompress.c. Thus we use absolute addresses for these variables. */
#    define tab_prefix0_seg (*(__segment*)0xfc0e)
#    define tab_prefix1_seg (*(__segment*)0xfc10)
#  else
    extern __segment tab_suffix_seg, tab_prefix0_seg, tab_prefix1_seg;
#    pragma aux tab_suffix_seg   "tab_suffix_seg__FIXOFS_0xfc0c"
#    pragma aux tab_prefix0_seg  "tab_prefix0_seg__FIXOFS_0xfc0e"
#    pragma aux tab_prefix1_seg  "tab_prefix1_seg__FIXOFS_0xfc10"
#  endif
#  define tab_suffix_seg_asm  "mov es, ds:[0xfc0c]"  /* The OpenWatcom C compiler will omit the `ds:' prefix byte. Good. */
#  define tab_prefix0_seg_asm "mov es, ds:[0xfc0e]"
#  define tab_prefix1_seg_asm "mov es, ds:[0xfc10]"
  static um16 tab_prefix_get(um16 code);
  /* tab_prefix_get(...) works with __modify [__es] and __modify [__es __bx], but it doesn't work with __modify __exact [es __bx] */
  /* The manual register allocation of __value [__bx] __parm [__bx] is useful for the code = tab_prefix_get(code) below. */
#  pragma aux tab_prefix_get = "shr bx, 1"  "jc odd"  tab_prefix0_seg_asm  "jmp have"  "odd:"  tab_prefix1_seg_asm  "have: add bx, bx"  "mov bx, es:[bx]" __value [__bx] __parm [__bx] __modify [__es __bx]
  static uc8 tab_suffix_get(um16 code);
#  pragma aux tab_suffix_get = tab_suffix_seg_asm  "mov al, es:[bx]" __value [__al] __parm [__bx] __modify __exact [__es]
  static void tab_prefix_suffix_set(um16 code, um16 prefix, uc8 suffix);
#  pragma aux tab_prefix_suffix_set = tab_suffix_seg_asm  "mov es:[bx], al"  "shr bx, 1"  "jc odd"  tab_prefix0_seg_asm  "jmp have"  "odd:"  tab_prefix1_seg_asm  "have: add bx, bx"  "mov es:[bx], dx" __parm [__bx] [__dx] [__al] __modify __exact [__es __bx]
  static unsigned short get_ds(void);
#  pragma aux get_ds = "mov ax, ds" __value [__ax] __modify __exact []
  static void set_uncompress_ducml_write_buffer(void);
#  pragma aux set_uncompress_ducml_write_buffer = uncompress_ducmp_write_buffer_asm __modify __exact []  /* Sets the argument of the `mov dx' in flush_write_buffer(...) to uncompress_ducml_write_buffer. */
  static void tab_init(maxbits) {
    unsigned short segment = get_ds();
#  if 0  /* Not needed. */
    if (tab_suffix_seg) return;  /* Prevent subsequent initialization. */
#  endif
#  if !LUUZCAT_WRITE_BUFFER_IS_EMPTY_AT_START_OF_DECOMPRESS
#    error LUUZCAT_WRITE_BUFFER_IS_EMPTY_AT_START_OF_DECOMPRESS must be true for LUUZCAT_DUCML.
#  endif
    set_uncompress_ducml_write_buffer();  /* Self-modifying code: change the buffer address in flush_write_buffer from global_write_buffer to uncompress_ducml_write_buffer. */
    if (maxbits < 15) {
      uncompress_ducml_write_buffer_size = 0x2000;
      /* !! size optimization: Make this function shorter in assembly. */
      tab_prefix0_seg = segment + (((unsigned)&uncompress_ducml_big.b14.tab_prefix0_14_ary - 256U) >> 4);  /* It's aligned to a multiple of 0x10. */
      tab_prefix1_seg = segment + (((unsigned)&uncompress_ducml_big.b14.tab_prefix1_14_ary - 256U) >> 4);
      tab_suffix_seg  = segment + (((unsigned)&uncompress_ducml_big.b14.tab_suffix_14_ary  - 256U) >> 4);
    } else if (maxbits == 15) {
      uncompress_ducml_write_buffer_size = 0x6000;
      tab_suffix_seg  = segment + (((unsigned)&uncompress_ducml_big.b15.tab_suffix_15_ary  - 256U) >> 4);
#  define TAB_ALLOC_PARA_COUNT_15 (unsigned short)((((1UL << 15) - 256U) * 2 + 15) >> 4)  /* Number of 16-byte paragraphs needed by tab_prefix0_seg and tab_prefix1_seg. */
      if (progx86_is_para_less_than(TAB_ALLOC_PARA_COUNT_15)) fatal_out_of_memory();
      segment = progx86_para_reuse_start() + TAB_ALLOC_PARA_COUNT_15 - 256U;
      tab_prefix0_seg = segment;    /* Prefix for even codes. */
      tab_prefix1_seg = segment + (unsigned short)(((1UL << 15) - 256U) >> 4);  /* Prefix for odd codes. */
    } else {  /* maxbits == 16. */
      uncompress_ducml_write_buffer_size = 0xe000;
#  define TAB_ALLOC_PARA_COUNT_16 (unsigned short)((((1UL << 16) - 256U) * 3 + 15) >> 4)  /* Number of 16-byte paragraphs needed by tab_suffix_seg, tab_prefix0_seg and tab_prefix1_seg above. */
      if (progx86_is_para_less_than(TAB_ALLOC_PARA_COUNT_16)) fatal_out_of_memory();
      segment = progx86_para_reuse_start() + TAB_ALLOC_PARA_COUNT_16 - 256U;
      tab_prefix0_seg = segment;    /* Prefix for even codes. */
      tab_prefix1_seg = segment += (unsigned short)(((1UL << 16) - 256U) >> 4);  /* Prefix for odd codes. */
      tab_suffix_seg  = segment +  (unsigned short)(((1UL << 16) - 256U) >> 4);  /* For each code, it contains the last byte. */
    }
  }
#endif
#if IS_DOS_16 && !defined(LUUZCAT_SMALLBUF) && !defined(LUUZCAT_DUCML) && defined(_DOSCOMSTART_UNCOMPRC) && defined(_DOSCOMSTART) && !defined(_PROGX86_NOALLOC) && defined(__WATCOMC__) && !defined(__TURBOC__)
  /* This is the optimized far pointer implementation of the large arrays in
   * uncomprc.com for the OpenWatcom C compiler targeting DOS 8086.
   *
   * uncomprc.com is a DOS .com program which can decompress only
   * LZW-compressed data, but it uses less memory than luuzcat.com (and
   * luuzcatc.com). It uses (in addition to the MCB and kernel buffers):
   *
   * * For maxbits <= 14, 64 KiB.
   * * For maxbits == 15, 127.5 KiB.
   * * For maxbits == 16, 255.75 KiB.
   */
  /* !! Check for out-of-memory for DOS .com program (also for other DOS .com targets). Doesn't DOS already check at program load time that there is 64 KiB free? */
#  define BITS 16
#  define COMPRESS_WRITE_BUFFER_SIZE WRITE_BUFFER_SIZE
#  define lzw_stack big.compress.u.noa.crc32_table_dummy  /* static uc8 lzw_stack[1]; */  /* Used only to check its size. */
  static __segment tab_prefix0_seg, tab_prefix1_seg, tab_suffix_seg;
  static um16 tab_prefix_get(um16 code);
  /* tab_prefix_get(...) works with __modify [__es] and __modify [__es __bx], but it doesn't work with __modify __exact [es __bx] */
  /* The manual register allocation of __value [__bx] __parm [__bx] is useful for the code = tab_prefix_get(code) below. */
#  pragma aux tab_prefix_get = "shr bx, 1"  "jc odd"  "mov es, tab_prefix0_seg"  "jmp have"  "odd: mov es, tab_prefix1_seg"  "have: add bx, bx"  "mov bx, es:[bx]" __value [__bx] __parm [__bx] __modify [__es __bx]
  static uc8 tab_suffix_get(um16 code);
#  pragma aux tab_suffix_get = "mov es, tab_suffix_seg"  "mov al, es:[bx]" __value [__al] __parm [__bx] __modify __exact [__es]
  static void tab_prefix_suffix_set(um16 code, um16 prefix, uc8 suffix);
#  pragma aux tab_prefix_suffix_set = "mov es, tab_suffix_seg"  "mov es:[bx], al"  "shr bx, 1"  "jc odd"  "mov es, tab_prefix0_seg"  "jmp have"  "odd: mov es, tab_prefix1_seg"  "have: add bx, bx"  "mov es:[bx], dx" __parm [__bx] [__dx] [__al] __modify __exact [__es __bx]
  static unsigned short get_start_segment(const void *p);  /* Rounds down. */
#  pragma aux get_start_segment = "shr bx, 1"  "shr bx, 1"  "shr bx, 1"  "shr bx, 1"  "mov ax, ds"  "add ax, bx"  __parm [__bx] __value [__ax] __modify __exact [__bx]
  static uc8 tables_upto_14[((1U << 14) - 256U) * 3 + 0xf];  /* A bit less than 48 KiB. */
  static void tab_init(unsigned int maxbits) {
    unsigned short segment, array1_para_count;
#  if 0  /* Not needed. */
    if (tab_suffix_seg) return;  /* Prevent subsequent initialization. !! Disable below as well. */
#  endif
    if (maxbits <= 14) {
      /* TODO(pts): Does an alternative, faster implementation of tab_prefix_get(...) for maxbits <= 15 fit here? How much faster is it? */
      segment = get_start_segment(tables_upto_14 + 0xf);  /* Dynamic memory allocation (malloc(...) etc.) not needed. */
      array1_para_count = (unsigned short)(((1UL << 14) - 256U) >> 4);
      goto init_based_on_segment;
    } else if (maxbits == 15) {
#  define TAB_ALLOC_PARA_COUNT_15 (unsigned short)((((1UL << 15) - 256U) * 2 + 15) >> 4)  /* Number of 16-byte paragraphs needed by tab_prefix1_seg and tab_suffix_seg. */
      tab_prefix0_seg = get_start_segment(tables_upto_14 + 0xf) - (256U >> 4);  /* Dynamic memory allocation (malloc(...) etc.) not needed for tab_prefix0_seg, but it is needed for tab_prefix1_seg and tab_suffix_seg. */
      if (is_para_less_than(TAB_ALLOC_PARA_COUNT_15)) fatal_out_of_memory();
      segment = para_reuse_start() + TAB_ALLOC_PARA_COUNT_15;
      array1_para_count = (unsigned short)(((1UL << 15) - 256U) >> 4);
      tab_prefix1_seg = segment -= (256U >> 4);  /* Prefix for odd codes. */
      tab_suffix_seg  = segment +  array1_para_count;  /* For each code, it contains the last byte. */
    } else {
#  define TAB_ALLOC_PARA_COUNT_16 (unsigned short)((((1UL << BITS) - 256U) * 3 + 15) >> 4)  /* Number of 16-byte paragraphs needed by the tables above. */
#  if 1  /* Shorter than para_reuse(...) */
      if (is_para_less_than(TAB_ALLOC_PARA_COUNT_16)) fatal_out_of_memory();
      segment = para_reuse_start() + TAB_ALLOC_PARA_COUNT_16;
#  else
      if ((segment = para_reuse(TAB_PARA_COUNT)) == 0U) fatal_out_of_memory();
#  endif
      array1_para_count = (unsigned short)(((1UL << BITS) - 256U) >> 4);
     init_based_on_segment:
      tab_prefix0_seg = segment -= (256U >> 4);  /* Prefix for even codes. */
      tab_prefix1_seg = segment += array1_para_count;  /* Prefix for odd codes. */
      tab_suffix_seg  = segment +  array1_para_count;  /* For each code, it contains the last byte. */
    }
  }
#endif
#if IS_DOS_16 && !defined(LUUZCAT_SMALLBUF) && !defined(LUUZCAT_DUCML) && !defined(_DOSCOMSTART_UNCOMPRC) && !defined(_PROGX86_NOALLOC) && defined(__WATCOMC__) && !defined(__TURBOC__)
  /* This is the optimized far pointer implementation of the large arrays
   * for the OpenWatcom C compiler targeting DOS 8086.
   */
#  define BITS 16
#  define COMPRESS_WRITE_BUFFER_SIZE WRITE_BUFFER_SIZE
#  define lzw_stack big.compress.u.noa.crc32_table_dummy  /* static uc8 lzw_stack[1]; */  /* Used only to check its size. */
  static __segment tab_prefix0_seg, tab_prefix1_seg, tab_suffix_seg;
#  if !(defined(__SMALL__) || defined(__MEDIUM__))  /* This works in any memory model. */
    static um16 tab_prefix_get(um16 code) { return *((const um16 __far*)(((code & 1 ? tab_prefix1_seg : tab_prefix0_seg) :> (code & ~1U)))); }  /* Indexes 0 <= code < 256 are invalid and unused. */
    static uc8  tab_suffix_get(um16 code) { return *((const uc8 __far*)((tab_suffix_seg :> code))); }  /* Indexes 0 <= code < 256 are invalid and unused. */
    static void tab_prefix_suffix_set(um16 code, um16 prefix, uc8 suffix) { *((uc8 __far*)((tab_suffix_seg :> code))) = suffix; *((um16 __far*)(((code & 1 ? tab_prefix1_seg : tab_prefix0_seg) :> (code & ~1U)))) = prefix; }  /* Indexes 0 <= code < 256 are invalid and unused. */
#  else  /* This is the optimized alternative in __WATCOMC__ 8086 assembly. */
    static um16 tab_prefix_get(um16 code);
    /* tab_prefix_get(...) works with __modify [__es] and __modify [__es __bx], but it doesn't work with __modify __exact [es __bx] */
    /* The manual register allocation of __value [__bx] __parm [__bx] is useful for the code = tab_prefix_get(code) below. */
#    pragma aux tab_prefix_get = "shr bx, 1"  "jc odd"  "mov es, tab_prefix0_seg"  "jmp have"  "odd: mov es, tab_prefix1_seg"  "have: add bx, bx"  "mov bx, es:[bx]" __value [__bx] __parm [__bx] __modify [__es __bx]
    static uc8 tab_suffix_get(um16 code);
#    pragma aux tab_suffix_get = "mov es, tab_suffix_seg"  "mov al, es:[bx]" __value [__al] __parm [__bx] __modify __exact [__es]
    static void tab_prefix_suffix_set(um16 code, um16 prefix, uc8 suffix);
#    pragma aux tab_prefix_suffix_set = "mov es, tab_suffix_seg"  "mov es:[bx], al"  "shr bx, 1"  "jc odd"  "mov es, tab_prefix0_seg"  "jmp have"  "odd: mov es, tab_prefix1_seg"  "have: add bx, bx"  "mov es:[bx], dx" __parm [__bx] [__dx] [__al] __modify __exact [__es __bx]
#  endif
#  if !defined(_DOSCOMSTART) && !defined(_PROGX86)
#    include <malloc.h>  /* For OpenWatcom __DOS__ halloc(...). */
#  endif
#  ifndef LUUZCAT_MALLOC_OK
#    error Change luuzcat.h to allow malloc.
#  endif
#  define tab_init(maxbits) tab_init_noarg()
  static void tab_init_noarg(void) {
    /* We use OpenWatcom-specific halloc(...), because all our attempts to
     * create __far arrays (such as uc8 `__far tab_suffix_ary[(1UL << BITS)
     * - 256U];') have added lots of NUL bytes to the .exe.
     *
     * halloc(...) also zero-initializes. We don't need that, but it doesn't
     * hurt either.
     */
#  define TAB_PARA_COUNT (unsigned short)((((1UL << BITS) - 256U) * 3 + 15) >> 4)  /* Number of 16-byte paragraphs needed by the tables above. */
#  if !defined(_DOSCOMSTART) && !defined(_PROGX86)
    void __far *a;
#  endif
    unsigned short segment;
    if (tab_suffix_seg) return;  /* Prevent subsequent initialization. */
#  ifdef _DOSCOMSTART
#    if 1  /* Shorter than para_reuse(...) */
      if (is_para_less_than(TAB_PARA_COUNT)) fatal_out_of_memory();
      segment = para_reuse_start() + TAB_PARA_COUNT;
#    else
      if ((segment = para_reuse(TAB_PARA_COUNT)) == 0U) fatal_out_of_memory();
#    endif
#  else
#    ifdef _PROGX86
#      if _PROGX86_HAVE_PARA_REUSE_START  /* Shorter than the progx86_para_reuse_or_alloc call. */
        if (progx86_is_para_less_than(TAB_PARA_COUNT)) fatal_out_of_memory();
        segment = progx86_para_reuse_start() + TAB_PARA_COUNT;
#      else
        if ((segment = progx86_para_reuse_or_alloc(TAB_PARA_COUNT)) == 0U) fatal_out_of_memory();
#      endif
#    else
      a = halloc(((unsigned long)TAB_PARA_COUNT) << 4, 1);  /* Always returns offset = 0. */  /* !! Reuse this between different decompressors (hfree(...)?). Currently none of the others need it. */
      segment = (unsigned long)a >> 16;
      if (segment == 0U || (unsigned short)a != 0U) fatal_out_of_memory();
#  endif
#  endif
    /* !! For _PROGX86, compile a variant without segment arithmetic (so
     *    that it works in 286 16-bit protected mode). For that, we'd have
     *    to call progx86_para_alloc(...) 3 times. Currently we don't need
     *    this, because both DOS and ELKS support segment arithmetic, and
     *    elksemu(1) on Linux doesn't support far memory allocation.
     *    Xenix/86 and OS/2 1.x NE command-line would be the first one to
     *    need it.
     */
    tab_prefix0_seg = segment - (256U >> 4);    /* Prefix for even codes. */
    tab_prefix1_seg = segment - (256U >> 4) + (unsigned short)(((1UL << BITS) - 256U) >> 4);  /* Prefix for odd codes. */
    tab_suffix_seg  = segment - (256U >> 4) + (unsigned short)(((1UL << BITS) - 256U) >> 4) * 2;  /* For each code, it contains the last byte. */
  }
#endif
#if IS_DOS_16 && !defined(LUUZCAT_SMALLBUF) && !defined(_PROGX86_NOALLOC) && 0 && defined(__WATCOMC__) && !defined(__TURBOC__)
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
#  define BITS 16
#  define COMPRESS_WRITE_BUFFER_SIZE WRITE_BUFFER_SIZE
#  define lzw_stack big.compress.u.noa.crc32_table_dummy  /* static uc8 lzw_stack[1]; */  /* Used only to check its size. */
  static uc8  __far tab_suffix_ary[(1UL << BITS) - 256U];  /* For each code, it contains the last byte. */
  static um16 __far tab_prefix0_ary[((1UL << BITS) - 256U) >> 1];  /* Prefix for even codes. */
  static um16 __far tab_prefix1_ary[((1UL << BITS) - 256U) >> 1];  /* Prefix for odd  codes. */
  um16 __far *tab_prefix_fptr_ary[2];
#  define tab_prefixof(code) tab_prefix_fptr_ary[(code) & 1][((code) - 256U) >> 1]  /* Indexes 0 <= code < 256 are invalid and unused. */
#  define tab_prefix_get(code) tab_prefixof(code)  /* Indexes 0 <= code < 256 are invalid and unused. */
#  define tab_suffixof(code) tab_suffix_ary[(code) - 256U]  /* Indexes 0 <= code < 256 are invalid and unused. */
#  define tab_suffix_get(code) tab_suffixof(code)  /* Indexes 0 <= code < 256 are invalid and unused. */
#  define tab_prefix_suffix_set(code, prefix, suffix) (tab_prefixof(code) = (prefix), tab_suffixof(code) = (suffix))  /* Indexes 0 <= code < 256 are invalid and unused. */
#  define tab_init(maxbits) do { tab_prefix_fptr_ary[0] = tab_prefix0_ary; tab_prefix_fptr_ary[1] = tab_prefix1_ary; } while (0)
#endif
#if IS_DOS_16 && !defined(LUUZCAT_SMALLBUF) && !defined(_PROGX86_NOALLOC) && !defined(__WATCOMC__) && defined(__TURBOC__)
  /* This is the memory-optimized (but not speed-optimized) far pointer
   * implementation of the large arrays for the Turbo C++ 1.00 or 1.01
   * compilers targeting DOS 8086. Don't specify the `-A' flag at compile
   * time, it will hide the `far' keyword needed here.
   *
   * To avoid adding NUL bytes to the .exe, we use allocmem(...) to allocate
   * these arrays. We don't need them initialized.
   */
#  define BITS 16
#  define COMPRESS_WRITE_BUFFER_SIZE WRITE_BUFFER_SIZE
#  include <dos.h>  /* For Turbo C++ allocmem(...), MK_FP(...) and FP_SEG(...). */
#  define lzw_stack big.compress.u.noa.crc32_table_dummy  /* static uc8 lzw_stack[1]; */  /* Used only to check its size. */
  static uc8 far *tab_suffix_fptr;  /* [(1UL << BITS) - 256U]. For each code, it contains the last byte. */
  um16 far *tab_prefix_fptr_ary[2];  /* [((1UL << BITS) - 256U) >> 1]. [0] is prefix for even codes, [1] is prefix for odd codes. */
#  define tab_prefixof(code) tab_prefix_fptr_ary[(code) & 1][(code) >> 1]  /* Indexes 0 <= code < 256 are invalid and unused. */
#  define tab_prefix_get(code) tab_prefixof(code)  /* Indexes 0 <= code < 256 are invalid and unused. */
#  define tab_suffixof(code) tab_suffix_fptr[(code)]  /* Indexes 0 <= code < 256 are invalid and unused. */
#  define tab_suffix_get(code) tab_suffixof(code)  /* Indexes 0 <= code < 256 are invalid and unused. */
#  define tab_prefix_suffix_set(code, prefix, suffix) (tab_prefixof(code) = (prefix), tab_suffixof(code) = (suffix))  /* Indexes 0 <= code < 256 are invalid and unused. */
#  ifndef LUUZCAT_MALLOC_OK
#    error Change luuzcat.h to allow malloc.
#  endif
#  define tab_init(maxbits) tab_init_noarg()
  static void tab_init_noarg(void) {
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
#if IS_DOS_16 && !defined(LUUZCAT_SMALLBUF) && !defined(_PROGX86_NOALLOC) && 0 && !defined(__WATCOMC__) && defined(__TURBOC__)
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
#  define BITS 16
#  define COMPRESS_WRITE_BUFFER_SIZE WRITE_BUFFER_SIZE
#  define lzw_stack big.compress.u.noa.crc32_table_dummy  /* static uc8 lzw_stack[1]; */  /* Used only to check its size. */
  static uc8  far tab_suffix_ary[(1UL << BITS) - 256U];  /* For each code, it contains the last byte. */
  static um16 far tab_prefix0_ary[((1UL << BITS) - 256U) >> 1];  /* Prefix for even codes. */
  static um16 far tab_prefix1_ary[((1UL << BITS) - 256U) >> 1];  /* Prefix for odd  codes. */
  um16 far *tab_prefix_fptr_ary[2];
#  define tab_prefixof(code) tab_prefix_fptr_ary[(code) & 1][((code) - 256U) >> 1]  /* Indexes 0 <= code < 256 are invalid and unused. */
#  define tab_prefix_get(code) tab_prefixof(code)  /* Indexes 0 <= code < 256 are invalid and unused. */
#  define tab_suffixof(code) tab_suffix_ary[(code) - 256U]  /* Indexes 0 <= code < 256 are invalid and unused. */
#  define tab_suffix_get(code) tab_suffixof(code)  /* Indexes 0 <= code < 256 are invalid and unused. */
#  define tab_prefix_suffix_set(code, prefix, suffix) (tab_prefixof(code) = (prefix), tab_suffixof(code) = (suffix))  /* Indexes 0 <= code < 256 are invalid and unused. */
#  define tab_init(maxbits) do { tab_prefix_fptr_ary[0] = tab_prefix0_ary; tab_prefix_fptr_ary[1] = tab_prefix1_ary; } while (0)
#endif

#ifndef BITS
#  define BITS 16
#endif
#if BITS < 9 || BITS > 16
#  error Bad BITS.
#endif

#define INIT_BITS 9  /* Initial number of bits per code */

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
   * larger: it's maximum value, COMPRESS_READ_BUFFER_SIZE + READ_BUFFER_EXTRA is
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

/* Below p is a const uc8* (const unsigned char*). */
#ifdef IS_UNALIGNED_LE
#  define LOAD_UINT_16BP(p) (((const um16*)(p))[0])  /* This does READ_BUFFER_OVERSHOOT. */
#  define LOAD_UINT_24BP(p) (((const unsigned int*)(p))[0])  /* This works only if sizeof(unsigned int) > 2. */  /* This does READ_BUFFER_OVERSHOOT. */
#else
#  define LOAD_UINT_16BP(p) ((p)[0] | (p)[1] << 8)
#  define LOAD_UINT_24BP(p) ((p)[0] | (p)[1] << 8 | (p)[2] << 16)  /* This works only if sizeof(unsigned int) > 2. */
#endif

#if COMPRESS_READ_BUFFER_SIZE + READ_BUFFER_EXTRA > 0xffffU - READ_BUFFER_EXTRA - BITS  /* BITS is included here for posbyte_after_read calculations. */
#  error Input buffer too large for 16-bit compress (LZW) calculations.
#endif
#if COMPRESS_READ_BUFFER_SIZE < 15  /* Needed by `global_inptr += discard_byte_count'. */
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

#ifdef LUUZCAT_COMPRESS_FORK
  /* This code is based on decomp16 (decompress 16bit compressed files on a
   * 16bit Intel processor) by John N. White and Will Rose, version 1.3 on
   * 1992-03-25.
   *
   * History:
   *
   * * v1.00 1991-06-30 by John N. White: Original, public domain decomp16.c.
   * * v1.10 1992-02 by Will Rose: Patched to run under news by Will Rose.
   * * v1.11 1992-02-20 by John N. White: Earlier patches
   *   applied by Will Rose.
   * * v1.2  1992-03-24 by Will Rose: Unsigned int increment/wrap bug fixed.
   * * v1.3  1992-03-25 by Will Rose: Argument bug fixed, stdio generalized.
   * * v1.4  2025-11-18 by Peter Szabo (pts): Improved portability; added
   *   error propagation to parent; added extra error handling; made calls
   *   to dup(...) in ffork(...) more resilient; made it part of luuzcat;
   *   reduced fork() count (pnum) for maxbits <= 15, saving memory; made
   *   reversing the chars faster (much smaller constant for O(n**2)).
   *
   * Below you can read the explanantion of decomp16 1.3.
   *
   * decomp16 can use as as little as 512 bytes of stack; since it forks
   * four additional copies, it's probably worth using minimum stack rather
   * than the 8192 byte Minix default.  To reduce memory still further,
   * change BUFSZ below to 256; it is currently set to 1024 for speed.  The
   * minimal decomp16 needs about 280k to run in pipe mode (56k per copy).
   *
   * This program acts as a filter:
   *    decomp16 < compressed_file > decompressed_file
   * The arguments -0 to -4 run only the corresponding pass.
   * Thus:
   *    decomp16 -4 < compressed_file > 3;
   *    decomp16 -3 < 3 > 2;
   *    decomp16 -2 < 2 > 1;
   *    decomp16 -1 < 1 > 0;
   *    decomp16 -0 < 0 > decompressed_file
   * will also work, as will connecting the passes by explicit pipes if
   * there is enough memory to do so.  File name arguments can also be
   * given directly on the command line.
   *
   * Compress uses a modified LZW compression algorithm. A compressed file
   * is a set of indices into a dictionary of strings. The number of bits
   * used to store each index depends on the number of entries currently
   * in the dictionary. If there are between 257 and 512 entries, 9 bits
   * are used. With 513 entries, 10 bits are used, etc. The initial dictionary
   * consists of 0-255 (which are the corresponding chars) and 256 (which
   * is a special CLEAR code). As each index in the compressed file is read,
   * a new entry is added to the dictionary consisting of the current string
   * with the first char of the next string appended. When the dictionary
   * is full, no further entries are added. If a CLEAR code is received,
   * the dictionary will be completely reset. The first two bytes of the
   * compressed file are a magic number, and the third byte indicates the
   * maximum number of bits, and whether the CLEAR code is used (older versions
   * of compress didn't have CLEAR).
   *
   * This program works by forking four more copies of itself. The five
   * programs form a pipeline. Copy 0 writes to stdout, and forks copy 1
   * to supply its input, which in turn forks and reads from copy 2, etc.
   * This sequence is used so that when the program exits, all writes
   * are completed and a program that has exec'd uncompress (such as news)
   * can immediately use the uncompressed data when the wait() call returns.
   *
   * If given a switch -#, where # is a digit from 0 to 4 (example: -2), the
   * program will run as that copy, reading from stdin and writing to stdout.
   * This allows decompressing with very limited RAM because only one of the
   * five passes is in memory at a time.
   *
   * The compressed data is a series of string indices (and a header at
   * the beginning and an occasional CLEAR code). As these indices flow
   * through the pipes, each program decodes the ones it can. The result
   * of each decoding will be indices that the following programs can handle.
   *
   * Each of the 65536 strings in the dictionary is an earlier string with
   * some character added to the end (except for the the 256 predefined
   * single char strings). When new entries are made to the dictionary,
   * the string index part will just be the last index to pass through.
   * But the char part is the first char of the next string, which isn't
   * known yet. So the string can be stored as a pair of indices. When
   * this string is specified, it is converted to this pair of indices,
   * which are flagged so that the first will be decoded in full while
   * the second will be decoded to its first char. The dictionary takes
   * 256k to store (64k strings of 2 indices of 2 bytes each). This is
   * too big for a 64k data segment, so it is divided into 5 equal parts.
   * Copy 4 of the program maintains the high part and copy 0 holds the
   * low part.
   */

#  if COMPRESS_FORK_BUFSIZE & 0x1f
#    error For pipe reads in LUUZCAT_COMPRESS_FORK, READ_BUFER_SIZE must be divisible by 0x20.
#  endif

  typedef char assert_sizeof_us16_for_uncompress[sizeof(us16) == 2 ? 1 : -1];  /* The byte order doesn't matter. */

  static unsigned int fork_obufind;  /* output buffer zero-initialized; index; also used by putpipe(...) with pnum == 1 and done_with_fork(...) */
  static unsigned int ipbufind;    /* pipe buffer indices; no need to initialize */
  static unsigned int opbufind;  /* output buffer word pipe index; zero-initialized; */
  static um8 pnum;  /* ID of this copy: 0..4; zero-initialized; also used by ffork() and done_with_fork(...) */
  static unsigned int iindex;  /* no need to initialize; holds index being processed */
  static unsigned int base;      /* where in global dict local dict starts; no need to initialize; also used by getpipe() */
  static unsigned int curend;    /* current end of global dict; no need to initialize; also used by getpipe() */
  static unsigned int curbits;      /* number of bits for getbits() to read: 9..16; no need to initialize */
  static um8 inmod;      /* mod 8 for getbits(); zero-initialized; also used by getbits() */
  static us16 getpipe_flags;  /* no need to initialize */
  static um8 getpipe_flagn;    /* number of flags in flags: 0..15; zero-initialized */
  static us16 putpipe_flags;  /* also used by putpipe(...); no need to initialize */
  static us16 *putpipe_flagp;  /* also used by putpipe(...); no need to initialize */
  static um8 putpipe_flagn;    /* number of flags in flags: 0..16; zero-initialized */
  static um8 getbits_curbyte;  /* byte having bits extracted from it; zero-initialized */
  static um8 getbits_left;    /* how many bits are left in getbits_curbyte: 0..8; zero-initialized */
  static int pipe_fd_out;  /* no need to initialize */
  static uc8 *fork_inptr;  /* no need to initialize; also used by get_byte_with_fork(...) */
  static uc8 *fork_inend;  /* no need to initialize; also used by get_byte_with_fork(...) */

#  define DSTATUS_OK 0
#  define DSTATUS_BAD_STDIN_READ 1
#  define DSTATUS_BAD_STDOUT_WRITE 2
#  define DSTATUS_UNEXPECTED_EOF 3
#  define DSTATUS_CORRUPTED_INPUT 4
#  define DSTATUS_PIPE_ERROR 5
#  define DSTATUS_FORK_ERROR 6
#  define DSTATUS_CLOSE_0_ERROR 7
#  define DSTATUS_CLOSE_1_ERROR 8
#  define DSTATUS_DUP_0_ERROR 9
#  define DSTATUS_DUP_1_ERROR 10
#  define DSTATUS_BAD_PIPE_READ 11
#  define DSTATUS_SHORT_PIPE_READ 12
#  define DSTATUS_BAD_PIPE_WRITE 13
  /**/
#  define DSTATUS_MAX 13U

  static const char * const error_messages[] = {
      /* DSTATUS_PIPE_ERROR: */  "pipe() error\n",
      /* DSTATUS_FORK_ERROR: */  "fork() error\n",
      /* DSTATUS_CLOSE_0_ERROR: */  "close(0) error\n",
      /* DSTATUS_CLOSE_1_ERROR: */  "close(1) error\n",
      /* DSTATUS_DUP_0_ERROR: */  "dup(0) error\n",
      /* DSTATUS_DUP_1_ERROR: */  "dup(1) error\n",
      /* DSTATUS_BAD_PIPE_READ: */  "bad pipe read\n",
      /* DSTATUS_SHORT_PIPE_READ: */  "short pipe read\n",
      /* DSTATUS_BAD_PIPE_WRITE: */  "bad pipe write\n",
  };

  static ub8 flush_obuf_to(int fd) {
    char *p;
    unsigned int p_remaining;
    int got;

    for (p = (char*)big.compress.u.sf.sf_write_buffer, p_remaining = fork_obufind; p_remaining != 0; p += got, p_remaining -= got) {
      if ((got = write(fd, p, p_remaining)) <= 0) return 0;  /* Indicate error. */
    }
    fork_obufind = 0;
    return 1;  /* Indicate success. */
  }

  /* If s is a message, write it to stderr. Flush buffers if needed. Then exit. */
  static __noreturn void done_with_fork(unsigned int dstatus) {
    if (pnum == 0) {  /* Write error directly to stderr. */
      if (!flush_obuf_to(STDOUT_FILENO) && !dstatus) dstatus = DSTATUS_BAD_STDOUT_WRITE;  /* Flush stdout buffer if needed. */
      /* !! size optimization: Make this shorter in assembly, using register AL instead of BX etc. */
      if (dstatus == DSTATUS_OK) { do_exit: exit(EXIT_SUCCESS); }
      if (dstatus == DSTATUS_BAD_STDIN_READ) fatal_read_error();
      if (dstatus == DSTATUS_BAD_STDOUT_WRITE) fatal_write_error();
      if (dstatus == DSTATUS_UNEXPECTED_EOF) fatal_unexpected_eof();
      if (dstatus == DSTATUS_CORRUPTED_INPUT) fatal_corrupted_input();
      fatal_msg((error_messages - DSTATUS_PIPE_ERROR)[dstatus]);
    } else {
#  ifdef USE_DEBUG
      fprintf(stderr, "die propagating dstatus=%u\n", dstatus);
#  endif
      dstatus = ~dstatus;
      do {
        if (putpipe_flagn == 0) {  /* if we need to reserve a flag entry */
          putpipe_flags = 0;
          putpipe_flagp = (us16*)big.compress.u.sf.sf_write_buffer + opbufind;
          opbufind++;
        }
        do {
          ((us16*)big.compress.u.sf.sf_write_buffer)[opbufind++] = dstatus;  /* add large enough dstatus value to the buffer. */
          putpipe_flags = putpipe_flags << 1;  /* Add flag value 0. */
        } while (++putpipe_flagn < 15);
        putpipe_flagn = 0;
        *putpipe_flagp = putpipe_flags;  /* insert flags entry */
      } while (opbufind != (COMPRESS_FORK_BUFSIZE >> 1));
      fork_obufind = COMPRESS_FORK_BUFSIZE;
      flush_obuf_to(pipe_fd_out);  /* Flush pipe. Ignore error while flushing. */
    }
    goto do_exit;
  }

  /* Fork off the previous pass - the parent reads from the child. Returns 0 for the parent, and the positive new pnum for the child. */
  static um8 ffork(void) {
    int j, pfd[2];

    if (pipe(pfd) == -1) done_with_fork(DSTATUS_PIPE_ERROR);
    if ((j = fork()) == -1) done_with_fork(DSTATUS_FORK_ERROR);
    if (j == 0) {  /* this is the child process */
#if 0
      (void)!write(STDERR_FILENO, "C", 1);  /* For debugging, indicate that a child has been successfully forked. */
#endif
      /* !! Fix `fork() error' on ELKS 0.2.0 and ELSK 0.1.4. Not even the 3 forks for maxbits == 13 work. Is this even possible? */
      pipe_fd_out = pfd[1];  /* Make done_with_fork(...) propagate the error code to the parent. */
      if (pfd[1] != 1  /* STDOUT_FILENO */) {
        if (close(1) == -1) {
          j = dup(pfd[1]);
          if (j == 0) {
            j = dup(pfd[1]);
            if (j != 1) goto close1_error;
            (void)!close(0);
          } else if (j != 1) { close1_error:
            done_with_fork(DSTATUS_CLOSE_1_ERROR);
          }
        } else {
          if (dup(pfd[1]) != 1) done_with_fork(DSTATUS_DUP_1_ERROR);
        }
        pipe_fd_out = 1  /* STDOUT_FILENO */;
        (void)!close(pfd[1]);
      }
      (void)close(pfd[0]);
      return ++pnum;
    } else {  /* this is still the parent process */
      if (pfd[0] != 0  /* STDIN_FILENO */) {
        if (close(0) == -1) done_with_fork(DSTATUS_CLOSE_0_ERROR);
        if (dup(pfd[0]) != 0) done_with_fork(DSTATUS_DUP_0_ERROR);
        (void)!close(pfd[0]);
      }
      (void)close(pfd[1]);
      return 0;
    }
  }

  /* Get a char from stdin. If EOF, then done_with_fork(...) and exit. */
  static unsigned int get_byte_with_fork(unsigned int dstatus_on_eof) {
    int got;

    if (fork_inptr == fork_inend) {
      if ((got = read(STDIN_FILENO, (char*)big.compress.u.sf.sf_read_buffer, (int)COMPRESS_FORK_BUFSIZE)) < 0) done_with_fork(DSTATUS_BAD_STDIN_READ);
      if (got == 0) done_with_fork(dstatus_on_eof);
      fork_inend = big.compress.u.sf.sf_read_buffer + got;
      fork_inptr = big.compress.u.sf.sf_read_buffer;
    }
    return *fork_inptr++;
  }

  /* Put curbits bits into iindex from stdin. Note: only copy 4 uses this.
   * The bits within a byte are in the correct order. But when the bits
   * cross a byte boundry, the lowest bits will be in the higher part of
   * the current byte, and the higher bits will be in the lower part of
   * the next byte.
   */
  static void getbits(void) {
    unsigned int have;
    unsigned int dstatus_on_eof;

    ++inmod;  /* count input mod 8 */
    iindex = getbits_curbyte;
    have = getbits_left;
    if (curbits > have + 8) {
      iindex |= get_byte_with_fork(DSTATUS_OK) << have;
      have += 8;
      dstatus_on_eof = DSTATUS_UNEXPECTED_EOF;
    } else {
      dstatus_on_eof = DSTATUS_OK;
    }
    iindex |= ((getbits_curbyte = get_byte_with_fork(dstatus_on_eof)) << have) & ~(0xffffU << curbits);
#  if 0  /* Simulate an error in the middle. */
    if (curbits == 16 && iindex == 29891) done_with_fork(DSTATUS_DUP_1_ERROR);
#  endif
#  ifdef USE_DEBUG
    fprintf(stderr, "getbits(%u) == %u == 0x%x\n", curbits, iindex, iindex);
#  endif
    getbits_curbyte >>= curbits - have;
    getbits_left = 8 + have - curbits;
  }

  /* put an index into the pipeline. */
  static void putpipe(um16 u, ub8 flag) {
    char *p;
    unsigned int p_remaining;
    int got;

    if (pnum == 0) {    /* if we should write to stdout */
#  ifdef USE_DEBUG
      if (u > 255) { wi_too_large: fprintf(stderr, "fatal: assert: written index too large: pnum=%u index=%u\n", pnum, u); exit(99); }
#  endif
      big.compress.u.sf.sf_write_buffer[fork_obufind++] = u;
      if (fork_obufind == COMPRESS_FORK_BUFSIZE) {  /* If stdout buffer full. */
        if (!flush_obuf_to(STDOUT_FILENO)) done_with_fork(DSTATUS_BAD_STDOUT_WRITE);
      }
      return;
    }
    if (putpipe_flagn == 0) {  /* if we need to reserve a flag entry */
      putpipe_flags = 0;
      putpipe_flagp = (us16*)big.compress.u.sf.sf_write_buffer + opbufind;
      opbufind++;
    }
#  ifdef USE_DEBUG
    if ((pnum == 1 && u > 13311) || (pnum == 2 && u > 26367) || (pnum == 3 && u > 39423) || (pnum == 4 && u > 52479)) goto wi_too_large;  /* Smaller values are OK. */
#  endif
    ((us16*)big.compress.u.sf.sf_write_buffer)[opbufind++] = u;  /* add index to buffer */
    putpipe_flags = (putpipe_flags << 1) | flag;  /* add firstonly flag */
    if (++putpipe_flagn == 15) {    /* if block of 15 indices */
      putpipe_flagn = 0;
      *putpipe_flagp = putpipe_flags;    /* insert flags entry */
      if (opbufind == (COMPRESS_FORK_BUFSIZE >> 1)) {  /* if pipe out buffer full */
        opbufind = 0;
        p = (char*)big.compress.u.sf.sf_write_buffer; p_remaining = COMPRESS_FORK_BUFSIZE;
        do {
          if ((got = write(STDOUT_FILENO, p, p_remaining)) <= 0) done_with_fork(DSTATUS_BAD_PIPE_WRITE);
          p += got;
        } while ((p_remaining -= got) > 0);
      }
    }
  }

  /* Get an index from the pipeline. If flagged firstonly, handle it here. */
  static void getpipe(void) {
#  ifdef USE_DEBUG
    static unsigned int max_iindex;
#  endif
    char *p;
    unsigned int p_remaining;
    int got;

    for (;;) {    /* while index with firstonly flag set */
      if (getpipe_flagn == 0) {
        if (ipbufind >= (COMPRESS_FORK_BUFSIZE >> 1)) {  /* if pipe input buffer empty */
          p = (char*)big.compress.u.sf.sf_read_buffer; p_remaining = COMPRESS_FORK_BUFSIZE;
          do {
            if ((got = read(STDIN_FILENO, p, p_remaining)) <= 0) done_with_fork(got == 0 ? DSTATUS_SHORT_PIPE_READ : DSTATUS_BAD_PIPE_READ);
            p += got;
          } while ((p_remaining -= got) > 0);
          ipbufind = 0;
        }
        getpipe_flags = ((us16*)big.compress.u.sf.sf_read_buffer)[ipbufind++];
#  ifdef USE_DEBUG
        if (getpipe_flags >= 0x8000U) { fprintf(stderr, "fatal: assert: getpipe_flags too large: %u\n", getpipe_flags); exit(98); }
#  endif
        getpipe_flagn = 15;
      }
      iindex = ((us16*)big.compress.u.sf.sf_read_buffer)[ipbufind++];
      if (iindex >= (~DSTATUS_MAX & 0xffffU)) {
#  ifdef USE_DEBUG
        fprintf(stderr, "pnum=%u max_iindex=%u\n", pnum, max_iindex);
#  endif
        done_with_fork(~iindex & 0xffffU);  /* Propagate dstatus to parent process or user. */
      }
#  ifdef USE_DEBUG
      if (iindex > max_iindex) max_iindex = iindex;
      /* Maximum values for curend here: pnum=0 max_curend=13311; pnum=1 max_curend=26367; pnum=2 max_curend=39423; pnum=3 max_curend=52479; not reached for pnum=4. */
#  endif
      if (iindex > curend) done_with_fork(DSTATUS_CORRUPTED_INPUT);
      getpipe_flags <<= 1;
      getpipe_flagn--;
      if ((getpipe_flags & 0x8000U) != 0) {  /* if firstonly flag for index is not set */
        while (iindex >= base) iindex = big.compress.u.sf.dindex[iindex - base];
        putpipe(iindex, 1);
      } else
        return;    /* return with valid non-firstonly index */
    }
  }

  /* Make sure that there is enough room in big.compress.u.sf.dchar for the initial bytes copied from global_read_buffer. */
  typedef char assert_sizeof_dchar[sizeof(big.compress.u.sf.dchar) >= 3UL * READ_BUFFER_SIZE ? 1 : -1];

  static void copy_read_buffer_with_fork(void) {
#  if !LUUZCAT_WRITE_BUFFER_IS_EMPTY_AT_START_OF_DECOMPRESS
#    error LUUZCAT_WRITE_BUFFER_IS_EMPTY_AT_START_OF_DECOMPRESS must be true for decompress_compress_with_fork(...).
#  endif
    const unsigned int size = global_insize - global_inptr;

    fork_inptr = (fork_inend = (uc8*)big.compress.u.sf.dchar + sizeof(big.compress.u.sf.dchar)) - size;
    memmove(fork_inptr, global_read_buffer + global_inptr, size);
#  if 0  /* No initialization needed, it works with arbitrary values. */
    memset(big.compress.u.sf.dindex, 1, sizeof(big.compress.u.sf.dindex));
    memset(big.compress.u.sf.dchar,  1, sizeof(big.compress.u.sf.dchar) - size);
#  endif
    /* From now, global_read_buffer[...] will be unused, and thus it can overlap with big.compress. */
    /* From now until the next read(...) call in get_byte_with_fork(...), uncompressed input bytes will be copied from the temporary read buffer in big.compress.u.sf.dchar[...]. */
  }

  static __noreturn void decompress_compress_with_fork(um8 hdrbyte) {
    unsigned int clrend;  /* end of global dict at clear time; no need to initialize */
    unsigned int tbase1;  /* Maximum tindex1 not to spawn at the current pnum in the current iteration. */
    unsigned int maxbits;  /* limit on number of bits */
    unsigned int dcharp;  /* ptr to dchar that needs next index entry */
    unsigned int maxdchari1;  /* maximum value of dchar index set, plus 1 */
    unsigned int locend;  /* where in global dict local dict ends */
    unsigned int maxend;  /* max end of global dict */
    unsigned int tindex1;  /* Temporary variable based on iindex. */
    unsigned int tindex2;  /* Temporary variable based on iindex. */

    copy_read_buffer_with_fork();
    maxdchari1 = 0;
    /* Check header of compressed file */
    maxbits = hdrbyte & 0x1f;
    clrend = 255;
    if (hdrbyte & 0x80) ++clrend;
    ipbufind = COMPRESS_FORK_BUFSIZE >> 1;  /* This value isn't used by the bottom child (pnum == 4). */
    /* maxbits 0..8 is not supported. (n)compress supports decompressing it, but there is no compressor released to generate such a file; also such a file would provide a terrible compression ratio */
    if ((BITS < 8 && maxbits < 9) || maxbits > 16) done_with_fork(DSTATUS_CORRUPTED_INPUT);  /* check for valid maxbits */
#  if BITS >= 16
#    error BITS too large, decompress_compress_with_fork(...) is unnecessary.  /* If the caller can handle BITS >= 16, decompress_compress_with_fork(...) is unnecessary. */
#  else
#    if BITS == 15  /* maxbits == 16 ensured by the caller. */
    if (ffork() && ffork() && ffork()) ffork();  /* Fork off children as needed. ffork() increments pnum. */
#    else
#      if BITS == 14  /* maxbits == 15 || maxbits == 16 ensured by the caller. */
    curend = maxbits - 12;  /* (maxbits == 16) ? 4 : 3. Number of child processes to fork(). In the main loop below, it will be the dictionary size. */
    while (pnum < curend && ffork()) {}  /* Fork off children as needed. ffork() increments pnum. */
#      else
#        if BITS == 13  /* maxbits == 14 || maxbits == 15 || maxbits == 16 ensured by the caller. */
    curend = (maxbits > 15) ? 4 : (maxbits == 15) ? 3 : 2;  /* Number of child processes to fork(). In the main loop below, it will be the dictionary size. */
    while (pnum < curend && ffork()) {}  /* Fork off children as needed. ffork() increments pnum. */
#        else
#          error BITS too small for decompress_compress_with_fork(...).  /* The caller should cover this with an alternative, faster implementation. */
#        endif
#      endif
#    endif
#  endif

    /* Preliminary inits. Note: end/maxend/curend are highest, not highest + 1. */
    base = COMPRESS_FORK_DICTSIZE * pnum + 256;  /* Using `+ clrend + 1' instead of `+ 256' would also work here, and it would populate big.compress.u.sf.dindex[...] andbig.compress.u.sf.dchar[...] from the beginning. */
    locend = base + COMPRESS_FORK_DICTSIZE - 1;
    maxend = (1 << maxbits) - 1;
    if (maxend > locend) maxend = locend;
#  if BITS < 15
    if (pnum == curend) pnum = 4;  /* For the `if (pnum === 4)' check for the bottom child in the main loop below. */
#  endif
#  ifdef USE_DEBUG
    /* pnum=0 maxend=13311; pnum=1 maxend=26367; pnum=2 maxend=39423; pnum=3 maxend=52479; pnum=4 maxend=65535. */
    fprintf(stderr, "pnum=%u maxend=%u\n", pnum, maxend);
#  endif

    goto do_clear;
    for (;;) {  /* For each index in the input. */
      if (pnum == 4) {  /* get index using get_byte_with_fork(...) and getbits() */
        if (curbits < maxbits && (1U << curbits) <= curend) {  /* curbits needs to be increased. */
          /* Due to ugliness in compress, these indices in the compressed file are wasted. This code is only needed when incrementing curbits from 9 to 10 with clrend == 255 (non-block mode). */
          while (inmod & 7) getbits();
          curbits++;
        }
        getbits();
      } else {
        getpipe();  /* get next index */
      }
      if (iindex == 256 && clrend == 256) {
        if (pnum > 0) putpipe(iindex, 0);
        /* Due to uglyiess in compress, these indices in the compressed file are wasted. */
        while (inmod & 7) getbits();
       do_clear:
        curend = clrend;  /* Initialize dictionary size. */
        dcharp = COMPRESS_FORK_DICTSIZE;  /* flag for none needed */
        curbits = 9;    /* init curbits (for copy 0) */
        continue;
      }
      tindex1 = iindex;
      /* Convert the index part, ignoring spawned chars */
      while (tindex1 >= base) tindex1 = big.compress.u.sf.dindex[tindex1 - base];
      /* Pass on the index */
      putpipe(tindex1, 0);
      /* Save the char of the last added entry, if any */
      if (dcharp < COMPRESS_FORK_DICTSIZE) big.compress.u.sf.dchar[dcharp++] = tindex1;
      if (curend < maxend && ++curend >= base) big.compress.u.sf.dindex[dcharp = (curend - base)] = iindex;
      /* Now: 256U <= base <= iindex <= curend <= maxend <= locend <= 65535. */

      /* Do spawned chars. They are naturally produced in
       * the wrong order. To get them in the right order
       * without using memory, a series of passes,
       * progressively less deep, are used */
      for (tbase1 = base - 1; (tindex1 = iindex) > tbase1; ) {  /* Emit (spawn) each char from big.compress.u.sf.dchar[...]. */  /* !!! speed optimization: This is too slow: O(n**2). */
        /* Now: 256U <= base <= tbase1 + 1U <= tindex1 == iindex <= curend <= maxend <= locend <= 65535U. */
        for (; (tindex2 = big.compress.u.sf.dindex[tindex1 - base]) > tbase1; tindex1 = tindex2) {}  /* Scan to desired char. This loop doesn't increase tindex1. */
        /* Now: 256U <= base <= tbase1 + 1U <= tindex1 <= iindex <= curend <= maxend <= locend <= 65535U. */
        putpipe(big.compress.u.sf.dchar[tindex1 - base], 1);  /* put it to the pipe */
        tbase1 = tindex1;  /* Stop before tindex1 in the next iteration of this loop. */
      }
    }
  }

#  define decompress_noreturn __noreturn
  decompress_noreturn void decompress_compress_nohdr_low(um8 hdrbyte);
  void decompress_compress_nohdr(void) {
    um8 hdrbyte;

    hdrbyte = get_byte();
    if ((hdrbyte & LZW_RESERVED) != 0) fatal_corrupted_input();  /* A reserved bit is 1. No known (current or earlier) version of (n)compress in 2025 sets that. */
    if ((hdrbyte & BITMASK) > BITS) decompress_compress_with_fork(hdrbyte);
    decompress_compress_nohdr_low(hdrbyte);  /* This is faster for small maxbits values. */
  }
#  define read_force_eof() exit(EXIT_SUCCESS)
#else
#  define decompress_compress_nohdr_low(arg) decompress_compress_nohdr(void)
#  define decompress_noreturn
#endif

#ifdef LUUZCAT_SMALLBUF
  /* Make sure that there is enough room in big.compress.u.sn.tab_prefix_ary for the initial bytes copied from global_read_buffer. */
  typedef char assert_sizeof_tab_prefix_ary[sizeof(big.compress.u.sn.tab_prefix_ary) >= 3UL * READ_BUFFER_SIZE ? 1 : -1];

  static uc8 *compress_incurp, *compress_inendp;

  static void copy_read_buffer(void) {
#  if !LUUZCAT_WRITE_BUFFER_IS_EMPTY_AT_START_OF_DECOMPRESS
#    error LUUZCAT_WRITE_BUFFER_IS_EMPTY_AT_START_OF_DECOMPRESS must be true for decompress_compress_nohdr_low(...).
#  endif
    const unsigned int size = global_insize - global_inptr;

    compress_incurp = (compress_inendp = (uc8*)big.compress.u.sn.tab_prefix_ary + sizeof(big.compress.u.sn.tab_prefix_ary)) - size;
    memmove(compress_incurp, global_read_buffer + global_inptr, size);
    /* From now, global_read_buffer[...] will be unused, and thus it can overlap with big.compress. */
    /* From now until the next compress_read(...) call in decompress_compress_nohdr_low(...), uncompressed input bytes will be copied from the temporary read buffer in big.compress.u.sn.tab_prefix_ary[...]. */
    global_write_buffer_to_flush = compress_write_buffer;
  }

#  undef  read_force_eof
#  define read_force_eof() exit(EXIT_SUCCESS)
#  define compress_read_buffer big.compress.u.sn.sn_read_buffer
#  define compress_incur compress_incurp[0]
#  define compress_incurii compress_incurp++[0]
#  define compress_incurdelta(delta) compress_incurp[(delta)]
#  define compress_inremaining (compress_inendp - compress_incurp)
#  define compress_inskip1() (++compress_incurp)
#  define compress_inskip(n) (compress_incurp += (n))
#  define compress_inclear() (compress_incurp = compress_inendp = (uc8*)big.compress.u.sn.tab_prefix_ary)
#  define compress_inreadp compress_inendp
#  define compress_add_inreadp(delta) (compress_inendp += (delta))
#  define compress_debug_inptr 0  /* Not useful. */
#  define compress_debug_insize compress_inremaining  /* Partially useful. */
#else
#  define compress_read_buffer global_read_buffer
#  define compress_incur global_read_buffer[global_inptr]
#  define compress_incurii global_read_buffer[global_inptr++]
#  define compress_incurdelta(delta) global_read_buffer[(delta) + global_inptr]
#  define compress_inremaining (global_insize - global_inptr)
#  define compress_inskip1() (++global_inptr)
#  define compress_inskip(n) (global_inptr += (n))
#  define compress_inclear() (global_inptr = global_insize = 0)
#  define compress_inreadp (compress_read_buffer + global_insize)
#  define compress_add_inreadp(delta) (global_insize += (delta))
#  define compress_debug_inptr global_inptr
#  define compress_debug_insize global_insize
#endif

decompress_noreturn void decompress_compress_nohdr_low(um8 hdrbyte) {
  uint code, incode, oldcode;  /* 0 <= ... <= 0ffffU. */
  ub8 is_very_first_code;
  uc8 finchar;
  uint code_count;
  uint code_count_remainder;  /* Only the low 3 bits matter, so it can overflow. */
  um8 posbiti;  /* 0 <= posbiti <= 7. */
  uint discard_byte_count;  /* 0 <= discard_byte_count <= 15. */
  uint write_idx;  /* 0 <= write_idx <= COMPRESS_WRITE_BUFFER_SIZE. */
  uint bitmask;  /* 0 <= bitmask <= 0xffffU. */
  uint free_ent1;  /* 255 <= free_ent1 <= 0xffffU. */
  uint maxcode;  /* 0 <= maxcode <= 0x10000U. If sizeof(uint) == 2, then the maximum value overflows to 0U. */
  uint maxmaxcode1;  /* 511 <= maxmaxcode1 <= 0xffffU. It's always 1 less than a power of 2. */
  uint n_bits;  /* 9 <= n_bits <= 16. */
  uint rsize;
  uint maxbits;  /* max bits per code for LZW */
  ub8 block_mode;  /* Nonzero for block mode (compress 3.0 without `-C'), zero for non-block mode (compress 2.0). */
  uint w_finchar;  /* 0 <= w_finchar <= 256. */
  uint w_idx;  /* 0 <= write_idx <= COMPRESS_WRITE_BUFFER_SIZE. */
  uint w_code;  /* 0 <= w_code <= 0xffffU. */
  uint w_size;  /* 0 <= w_size <= 0xffffU. */
  uint w_skip;  /* 0 <= w_skip <= 0xffffU. */
  uc8 *stack_top;

#ifdef LUUZCAT_COMPRESS_FORK
  maxbits = hdrbyte;
#else
  maxbits = get_byte();
#endif
  block_mode = (maxbits & BLOCK_MODE) ? 1 : 0;
#ifdef USE_DEBUG
  fprintf(stderr, "COMPRESSED maxbits=%u block_mode=is_3plus=%u reserved=%u\n", maxbits & BITMASK, block_mode, (maxbits & LZW_RESERVED) != 0 ? 1 : 0);
#endif
#ifndef LUUZCAT_COMPRESS_FORK  /* For LUUZCAT_COMPRESS_FORK, it's already checked in decompress_compress_nohdr(void) above. */
  if ((maxbits & LZW_RESERVED) != 0) fatal_corrupted_input();  /* A reserved bit is 1. No known (current or earlier) version of (n)compress in 2025 sets that. */
#endif
  maxbits &= BITMASK;
#if defined(__i386__) && defined(__amd64__) || defined(__386__) || defined(_M_I86) || defined(_M_I386)
  maxmaxcode1 = (1U << maxbits) - 1U;  /* On x86, it's OK to shift a 16-bit value by 16 and get 0. */
#else
  maxmaxcode1 = (sizeof(maxmaxcode1) > 2) ? (1U << maxbits) - 1U : (maxbits == 16 ? 0xffffU : (1U << maxbits) - 1U);
#endif
#ifdef USE_DEBUG
  fprintf(stderr, "maxmaxcode1=%lu\n", (unsigned long)maxmaxcode1);
#endif
  /* (n)compress never generates maxbits < INIT_BITS == 9 here, and it
   * doesn't make sense, because n_bits remains 9, and 9 bits of input are
   * used for each 8 bits of output. (CLEAR codes can make it even more
   * wasteful.) But we allow it here, because the (n)compress 4.2.4
   * decompressor and gzip-1.14/unlzw.c both accept it, and it's possible to
   * do it without memory corruption.
   */
#ifdef LUUZCAT_COMPRESS_FORK
  /* Aborting maxbits > BITS and for maxbits > 16 has been taken care of by the caller. */
#else
  if (maxbits > 16) fatal_corrupted_input();
#  if BITS < 16  /* Without this, wcc(1) isn't smart enough to omit the string literal from const. */
  if (BITS < 16 && maxbits > BITS) fatal_msg("LZW bits not implemented" LUUZCAT_NL);
#  endif
#endif
  tab_init(maxbits);
#ifdef LUUZCAT_SMALLBUF
  copy_read_buffer();
#endif
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
    if (compress_inremaining < discard_byte_count) {
      discard_byte_count -= compress_inremaining;
      compress_inclear();
    } else {
      compress_inskip(discard_byte_count);
      discard_byte_count = 0;
      if (sizeof(unsigned int) > 2) {
        code_count = (((compress_inremaining) << 3) - posbiti) / n_bits;  /* The `<< 3' won't overflow. Good. */
      } else {
        code_count = divmodcalc(compress_inremaining, n_bits, posbiti);
      }
      if (code_count != 0) goto after_read;
      /* Now we have code_count == 0, which implies that there aren't enough
       * bits remaining for a single code. Thus
       * compress_inremaining == global_insize - global_inptr >= 3
       * is impossible, because that would imply that there are at
       * least 24 - posbiti bits available, and minimum_bits_needed ==
       * n_bits <= 16 < 17 == 24 - 7 <= 24 - posbiti <= bits_available.
       */
#ifdef USE_CHECK
      if (compress_inremaining > 2) abort();
#endif
#ifdef LUUZCAT_SMALLBUF
      if (compress_incurp != compress_read_buffer) {
        for (w_idx = 0; compress_incurp + w_idx != compress_inendp; ++w_idx) {  /* No need for memmove(...), since iteration count is <= 2. */
          compress_read_buffer[w_idx] = compress_incurp[w_idx];
        }
        compress_inendp = (compress_incurp = compress_read_buffer) + w_idx;
      }
#else
      if (global_insize > READ_BUFFER_EXTRA) {
        global_insize -= global_inptr;  /* After this, global_insize <= 2. */
#  ifdef USE_CHECK
        if (global_insize > READ_BUFFER_EXTRA) abort();  /* This can't happen, because now global_insize <= 2 == READ_BUFFER_EXTRA. */
#  endif
#  ifdef USE_DEBUG
        fprintf(stderr, "RESET_COPY insize=%lu inptr=%lu n_bits=%lu\n", (unsigned long)compress_debug_insize, (unsigned long)compress_debug_inptr, (unsigned long)n_bits);
#  endif
        for (w_idx = 0 ; w_idx < global_insize; ++w_idx) {  /* No need for memmove(...), global_insize is small: <= 2. */
          compress_read_buffer[w_idx] = compress_incurdelta(w_idx);
        }
        global_inptr = 0;
      }
#endif
    }
    /* Doing == (uint)-1U, because with `(int)... < 0', 0x8000U would fail. */
    if ((rsize = compress_read(STDIN_FILENO, compress_inreadp, COMPRESS_READ_BUFFER_SIZE)) == (uint)-1U) fatal_read_error();
#ifdef USE_DEBUG
    if (1) fprintf(stderr, "READ(%u)=%lu\n", COMPRESS_READ_BUFFER_SIZE, (unsigned long)rsize);
#endif
    if (rsize == 0) break;
    compress_add_inreadp(rsize);
    continue;
   after_read:
#ifdef USE_CHECK
    if (code_count == 0) abort();  /* Can't happen here. */
#endif
    code_count_remainder += code_count;
    if (is_very_first_code) {  /* Happens at the very beginning of the input only. */
      code = compress_incurii;
      if ((compress_incur & 1) != 0) fatal_corrupted_input();  /* First code must be 0 <= code <= 255. */
      ++posbiti;  /* Advance 9 bits by increasing this from 0 to 1. */
#ifdef USE_DEBUG
      if (code >= 256U) abort();  /* Implied by the assignment above. */
#endif
      --is_very_first_code;  /* = 0. */
      flush_full_write_buffer_in_the_beginning();  /* Usually this is a no-op, because the write buffer is not full in the beginning. */
      compress_write_buffer[write_idx++] = (finchar = (uc8)(oldcode = code));
      if (--code_count == 0) continue;
    }
   maybe_next_code:
#ifdef USE_DEBUG
    if (0) fprintf(stderr, "READ code_count=%lu inptr=%lu %lu/%lu\n", (unsigned long)code_count, (unsigned long)compress_debug_inptr, (unsigned long)free_ent1, (unsigned long)(unsigned short)maxcode);
#endif
    if ((sizeof(maxcode) > 2) ? (free_ent1 >= maxcode) : (free_ent1 > (uint)(maxcode - 1U))) {
#ifdef USE_DEBUG
      fprintf(stderr, "INCBITS inptr=%lu\n", (unsigned long)compress_debug_inptr);
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
    if (n_bits > (maxbits == 9 ? 10 : maxbits) && maxbits >= INIT_BITS) abort();
#endif
    /* Read n_bits (9 <= n_bits <= 16) from compress_incur at posbits, advance posbits. */
    if (n_bits == 16)  {  /* Shortcut. Now posbiti == 0. */
      code = LOAD_UINT_16BP(&compress_incur) & 0xffffU;
      compress_inskip(2);
    } else {
      if (sizeof(unsigned int) > 2) {  /* Works for 1 <= n_bits <= 17, but we only use it for 9 <= n_bits <= 16. */
        code = (LOAD_UINT_24BP(&compress_incur) >> posbiti) & bitmask;
      } else {  /* Works for 9 <= n_bits <= 16. */
        code = ((LOAD_UINT_16BP(&compress_incur) >> posbiti) & 0xffU) | (((LOAD_UINT_16BP(&compress_incurdelta(1)) >> posbiti) & bitmask) << 8);
      }
      compress_inskip1();
      if ((posbiti += n_bits - 8) > 7) {
        compress_inskip1();
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
      if (posbiti != 0) { compress_inskip1(); posbiti = 0; }
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
        compress_write_buffer[write_idx++] = *stack_top++;
        if (write_idx == COMPRESS_WRITE_BUFFER_SIZE) write_idx = flush_write_buffer(write_idx);
      }
#else
      w_size = lzw_stack + sizeof(lzw_stack) - stack_top;
      if (w_size <= 4) {  /* Write a few bytes quickly, without memcpy(...) etc. */
        while (stack_top != lzw_stack + sizeof(lzw_stack)) {
          compress_write_buffer[write_idx++] = *stack_top++;
          if (write_idx == COMPRESS_WRITE_BUFFER_SIZE) write_idx = flush_write_buffer(write_idx);
        }
      } else {
        while (w_size >= (w_skip = COMPRESS_WRITE_BUFFER_SIZE - write_idx)) {
          memcpy(compress_write_buffer + write_idx, stack_top, w_skip);
          write_idx = flush_write_buffer(COMPRESS_WRITE_BUFFER_SIZE);
          stack_top += w_skip;
          w_size -= w_skip;
        }
        memcpy(compress_write_buffer + write_idx, stack_top, w_size);
        write_idx += w_size;  /* It doesn't fill it fully. */
#ifdef USE_CHECK
        if (write_idx == COMPRESS_WRITE_BUFFER_SIZE) abort();  /* Must not be full. */
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
        if (w_size > COMPRESS_WRITE_BUFFER_SIZE - write_idx) {  /* This is the rare case with long string or string near the end of compress_write_buffer. */
#ifdef USE_DEBUG
          if (1) fprintf(stderr, "WRITE w_size=%lu remaining=%lu code=%lu\n", (unsigned long)w_size, (unsigned long)(COMPRESS_WRITE_BUFFER_SIZE - write_idx), (unsigned long)code);
#endif
          w_idx = COMPRESS_WRITE_BUFFER_SIZE;
          w_skip = w_size -= (COMPRESS_WRITE_BUFFER_SIZE - write_idx);  /* After this, w_skip > 0 && w_size > 0. */
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
            compress_write_buffer[--w_idx] = (uc8)w_finchar - 1;
            if (w_idx == write_idx) break;
          }
          code = w_code;
          w_skip = 1;
        }
        for (; --w_idx != write_idx; code = tab_prefix_get(code)) {
          compress_write_buffer[w_idx] = tab_suffix_get(code);
        }
        if (code >= 256U) code = tab_suffix_get(code);
        compress_write_buffer[w_idx] = code;   /* Last code is always a literal. */
        if (w_skip != 0) break;  /* This was the last iteration, the entire string has been printed. */
        write_idx = flush_write_buffer(COMPRESS_WRITE_BUFFER_SIZE);
      }
      if ((write_idx += w_size) == COMPRESS_WRITE_BUFFER_SIZE) write_idx = flush_write_buffer(write_idx);
    }
    if (free_ent1 < maxmaxcode1) {  /* Generate the new entry. */
      ++free_ent1;  /* Never overflows 0xffffU. */
      tab_prefix_suffix_set(free_ent1, oldcode, finchar);
#ifdef USE_DEBUG
      if (0) fprintf(stderr, "free_ent1=%lu ", (unsigned long)free_ent1);
#endif
    }
    oldcode = incode;   /* Remember previous code. */
    if (code_count != 0) goto maybe_next_code;
  }
  if (compress_inremaining != (posbiti > 0)) fatal_corrupted_input();  /* Unexpected EOF. */
  flush_write_buffer(write_idx);
  read_force_eof();
}

#ifdef _DOSCOMSTART_UNCOMPRC  /* Code copied from luuzcat.c for uncomprc.com. */
  __noreturn void fatal_msg(const char *msg) {
    write_nonzero_void(STDERR_FILENO, WRITE_PTR_ARG_CAST(msg), strlen(msg));
    exit(EXIT_FAILURE);
  }
  __noreturn void fatal_read_error(void) { fatal_msg("read error" LUUZCAT_NL); }
  __noreturn void fatal_write_error(void) { fatal_msg("write error" LUUZCAT_NL); }
  __noreturn void fatal_unexpected_eof(void) { fatal_msg("unexpected EOF" LUUZCAT_NL); }
  __noreturn void fatal_corrupted_input(void) { fatal_msg("corrupted input" LUUZCAT_NL); }
  __noreturn void fatal_out_of_memory(void) { fatal_msg("out of memory" LUUZCAT_NL); }

  uc8 global_read_buffer[READ_BUFFER_SIZE + READ_BUFFER_EXTRA + READ_BUFFER_OVERSHOOT];
  unsigned int global_insize; /* Number of valid bytes in global_read_buffer. */
  unsigned int global_inptr;  /* Index of next byte to be processed in global_read_buffer. */
  ub8 global_read_had_eof;  /* Was there an EOF already when reading? */

  unsigned int read_byte(ub8 is_eof_ok) {
    int got;
    if (global_inptr < global_insize) return global_read_buffer[global_inptr++];
    if (global_read_had_eof) goto already_eof;
    if ((got = read(STDIN_FILENO, (char*)global_read_buffer, (int)READ_BUFFER_SIZE)) < 0) fatal_read_error();
    if (got == 0) {
      ++global_read_had_eof;  /* = 1. */
     already_eof:
      if (is_eof_ok) return BEOF;
      fatal_unexpected_eof();
    }
#  if 0
    global_total_read_size += global_insize = got;
#  else
    global_insize = got;
#  endif
    global_inptr = 1;
    return global_read_buffer[0];
  }

  uc8 global_write_buffer[WRITE_BUFFER_SIZE];

  /* Always returns 0, which is the new buffer write index. */
  unsigned int flush_write_buffer(unsigned int size) {
    unsigned int size_i;
    int got;
    for (size_i = 0; size_i < size; ) {
      got = (int)(size - size_i);
      if (sizeof(int) == 2 && WRITE_BUFFER_SIZE >= 0x8000U && got < 0) got = 0x4000;  /* !! Not needed for DOS. Check for == -1 below instead.  */
      if ((got = write_nonzero(STDOUT_FILENO, WRITE_PTR_ARG_CAST(global_write_buffer + size_i), got)) <= 0) fatal_write_error();
      size_i += got;
    }
    return 0;
  }
#endif

#ifdef _DOSCOMSTART_UNCOMPRC  /* Main function for uncomprc.com. */
  /* The more usual `static const char usage_msg[] = "...";' also works, but __WATCOMC__ would align it to 4 or 2. */
#  define USAGE_MSG ("Usage: uncomprc <input.Z >output" LUUZCAT_NL "https://github.com/pts/luuzcat" LUUZCAT_NL)
#  define CO_INPUT_MSG ("compressed input expected" LUUZCAT_NL)

#  if ' ' == 32  /* ASCII system. */
#    define IS_PROGARG_TERMINATOR(c) ((unsigned char)(c) <= ' ')  /* ASCII only: matches '\0' (needed for Unix), '\t', '\n', '\r' (needed for DOS) and ' '. */
#  else  /* Non-ASCII system. */
    static ub8 is_progarg_terminator(char c) { return c == '\0' || c == '\t' || c == '\n' || c == '\r' || c == ' '; }
#    define IS_PROGARG_TERMINATOR(c) is_progarg_terminator(c)
#  endif

  main0() {
    unsigned int b;
    const char *argv1 = main0_argv1();

    /* We display the usage message if the are command-line arguments (or the
     * first argument is empty), and stdin is a terminal (TTY).
     */
    if ((main0_is_progarg_null(argv1) || IS_PROGARG_TERMINATOR(*argv1)) && isatty(STDIN_FILENO)) { do_usage:
      fatal_msg(USAGE_MSG);
    }
    if (!main0_is_progarg_null(argv1)) {
      while (!IS_PROGARG_TERMINATOR(b = *(const unsigned char*)argv1++)) {
        b |= 0x20;  /* Convert uppercase A-Z to lowercase a-z. */
        if (b == 'h') goto do_usage;  /* --help. */
      }
    }

#  if O_BINARY  /* For DOS, Windows (Win32 and Win64) and OS/2. */
    setmode(STDIN_FILENO, O_BINARY);
    /* if (!isatty(STDOUT_FILENO)) -- we don't do this, because isatty(...) always returns true in some emulators. */
    setmode(STDOUT_FILENO, O_BINARY);  /* Prevent writing (in write(2)) LF as CRLF on DOS, Windows (Win32) and OS/2. */
#  endif
    if ((int)(b = try_byte()) >= 0) {  /* zcat in gzip also accepts empty stdin. */
      if (b != 0x1f || get_byte() != 0x9d) fatal_msg(CO_INPUT_MSG);
      decompress_compress_nohdr();  /* Reads stdin until EOF. */
    }
    main0_exit0();  /* return EXIT_SUCCESS; */
  }
#endif
