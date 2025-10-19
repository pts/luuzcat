/* by pts@fazekas.hu at Thu Oct  2 22:32:09 CEST 2025 */

#ifndef _LUUZCAT_H
#define _LUUZCAT_H

#ifdef USE_DEBUG
#  define USE_CHECK 1
#endif

#if defined(__i386) || defined(__i386__) || defined(__amd64__) || defined(__x86_64__) || defined(_M_X64) || defined(_M_AMD64) || defined(__386) || \
    defined(__X86_64__) || defined(_M_I386) || defined(_M_I86) || defined(_M_X64) || defined(_M_AMD64) || defined(_M_IX86) || defined(__386__) || \
    defined(__X86__) || defined(__I86__) || defined(_M_I86) || defined(_M_I8086) || defined(_M_I286) || \
    defined(MSDOS) || defined(_MSDOS) || defined(_WIN32) || defined(_WIN64) || defined(__DOS__) || defined(__COM__) || defined(__NT__) || \
    defined(__MSDOS__) || defined(__OS2__)
#  define IS_X86 1
#  define IS_UNALIGNED_LE 1  /* There is unaligned little-endian access of 32-bit integers. */
#else
#  if (defined(__ARMEL__) || defined(__THUMBEL__) || defined(__AARCH64EL__)) && defined(__ARM_FEATURE_UNALIGNED) || \
       ((defined(__LITTLE_ENDIAN__) || (defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) || \
         defined(__LITTLE_ENDIAN) || defined(_LITTLE_ENDIAN)) && \
         (defined(__arm__) || defined(__aarch64__) || defined(__powerpc__) || defined(_M_PPC) || defined(_ARCH_PPC) || defined(__PPC__) || \
          defined(__PPC) || defined(PPC) || defined(__powerpc) || defined(powerpc)))
#    define IS_UNALIGNED_LE 1
#  endif
#endif

#if (!defined(__i386) && !defined(__i386__) && !defined(__amd64__) && !defined(__x86_64__) && !defined(_M_X64) && !defined(_M_AMD64) && !defined(__386) && \
     !defined(__X86_64__) && !defined(_M_I386) && !defined(_M_X64) && !defined(_M_AMD64) && !defined(__386__)) && \
    (defined(_M_I86) || defined(_M_IX86) || defined(__X86__) || defined(__I86__) || defined(_M_I8086) || defined(_M_I286) || defined(__TURBOC__)) && \
    (defined(MSDOS) || defined(_MSDOS) || defined(__DOS__) || defined(__COM__) || defined(__MSDOS__))
#  define IS_X86_16 1
#  define IS_DOS_16 1
#endif

#if defined(_PROGX86) && !defined(LIBC_PREINCLUDED)
  /* Expecting an external tiny libc for OpenWatcom C compiler creating
   * system-independent i386 code.
   */
#  ifndef __WATCOMC__
#    error OpenWatcom C compiler needed by _PROGX86.
#  endif
#  if !defined(__386__) && !IS_X86_16
#    error x86 CPU (16-bit or 32-bit) is needed by _PROGX86.
#  endif
#  define LIBC_PREINCLUDED 1
#  if IS_X86_16
#    define __PROGX86_IO1_CALLCONV __watcall
#  else
#    define __PROGX86_IO1_CALLCONV __cdecl
#  endif
#  ifdef _PROGX86_ONLY_BINARY
#    define write(fd, buf, count) write_binary(fd, buf, count)
    int __PROGX86_IO1_CALLCONV write_binary(int fd, const void *buf, unsigned count);
#  else
#    define O_BINARY 4
    int setmode(int fd, unsigned char mode);
    int __PROGX86_IO1_CALLCONV write(int fd, const void *buf, unsigned count);
#  endif
  int __PROGX86_IO1_CALLCONV read(int fd, void *buf, unsigned int count);
  int isatty(int fd);
  __declspec(noreturn) void exit(unsigned char exit_code);
  unsigned int strlen(const char *s);
  /* To prevent wlink: Error! E2028: strlen_ is an undefined reference */
  void *memset(void *s, int c, unsigned int n);
  void *memcpy(void *dest, const void *src, unsigned n);
#  if IS_X86_16
#    if !(defined(__SMALL__) || defined(__MEDIUM__))  /* This works in any memory model. */
#      pragma intrinsic(strlen)  /* This generates shorter code than the libc implementation. */
#    else
#      pragma aux strlen = "push ds"  "pop es"  "mov cx, -1"  "xor ax, ax"  "repne scasb"  "not cx"  "dec cx"  __parm [__di] __value [__cx] __modify __exact [__es __di __ax]  /* 2 bytes shorter than the `#pragma intrinsic', because `push ds ++ pop es' is shorter than 2 movs. */
#    endif
#    pragma intrinsic(memset)  /* This generates shorter code than the libc implementation. */
#    pragma intrinsic(memcpy)  /* This generates shorter code than the libc implementation. */
    /* Allocates para_count << 4 bytes of memory, aligned to 16 (paragraph)
     * boundary, and returns the segment register value pointing to the
     * beginning of it. Returns 0 on out-of-memory.
     */
    unsigned __watcall progx86_para_alloc(unsigned para_count);
#  endif
#  ifdef _PROGX86_CRLF
#    define LUUZCAT_NL "\r\n"
#  else
#    define LUUZCAT_NL "\n"  /* Line ending for error messages on stderr. */
#  endif
#endif

#if defined(_DOSCOMSTART) && !defined(LIBC_PREINCLUDED)
  /* Tiny libc for OpenWatcom C compiler creating a DOS 8086 .com program.
   * For compilation instructions, see _DOSCOMSTART above.
   */
#  ifndef __WATCOMC__
#    error OpenWatcom C compiler needed by _DOSCOMSTART.
#  endif
#  ifndef IS_X86_16
#    error 8086 CPU is needed by _DOSCOMSTART.  /* And the IS_X86_16 macro must work correctly. */
#  endif
#  ifndef __SMALL__
#    error The small memory model is needed by _DOSCOMSTART.
#  endif
#  define LIBC_PREINCLUDED 1
  /* This must be put to a separate function than _comstart, otherwise wlink
   * wouldn't remove these 0x100 bytes from the beginning of the DOS .com
   * program.
   */
#  ifdef LUUZCAT_MAIN
    __declspec(naked) void __watcall _com_nuls(void) {
        __asm { db 100h dup (?) } }

    extern char _edata[], _end[];
    extern void __watcall main0(void);
    /* It must not modify `bp', otherwise the OpenWatcom C compiler omits the
     * `pop bp' between the `mov sp, bp' and the `ret'.
     */
#    pragma aux main0 __modify [__ax __bx __cx __dx __si __di __es]

    /* This entry point function must be the first function defined in the .c
     * file, because the entry point of DOS .com programs is at the beginning.
     *
     * This function zero-initializes _BSS, then jumps to main0. When main0
     * returns, it will do a successful exit(0). If _BSS is missing (no matter
     * how long _DATA is), then both _end and _edata are NULL here, but that's
     * fine here.
     */
    __declspec(naked) void __watcall _comstart(void) {
        __asm { xor ax, ax }
        __asm { mov di, offset _edata }
#    ifdef _DOSCOMSTART_DISTART
          __asm { mov ax, offset _end }  /* This code is deliberately incorrect (it should be `mov cx, ...'). It's size optimization, because `mov cx, _end-_edata' doesn't work here. shorten_to_bss.pl will fix it. */
#    else
          __asm { mov cx, offset _end } __asm { sub cx, di }
#    endif
        __asm { rep stosb } __asm { jmp main0 } }

    /* At the end, the `ret' instruction will exit(0) the DOS .com program. */
#    define main0() void __watcall main0(void)
#    define main0_exit0() do {} while (0)
#    define main0_exit(exit_code) _exit(exit_code)
#  endif


#  define LIBC_HAVE_WRITE_NONZERO_VOID 1
  /* Like write(...), but count must be nonzero, and it returns void. */
  static void write_nonzero_void(int fd, const void *buf, unsigned int count);
#  pragma aux write_nonzero_void = "mov ah, 40h"  "int 21h" \
      __parm [__bx] [__dx] [__cx] __modify [__ax]

#  define LIBC_HAVE_WRITE_NONZERO 1
  /* Like write(...), but count must be nonzero. */
  int write_nonzero(int fd, const void *buf, unsigned int count);
#  pragma aux write_nonzero = "mov ah, 40h"  "int 21h"  "jnc ok"  \
      "sbb ax, ax"  "ok:"  \
      __value [__ax] __parm [__bx] [__dx] [__cx] __modify __exact [__ax]

  int write(int fd, const void *buf, unsigned count);
#  pragma aux write = "xor ax, ax"  "jcxz ok"  "mov ah, 40h"  "int 21h" \
      "jnc ok"  "sbb ax, ax"  "ok:"  \
      __value [__ax] __parm [__bx] [__dx] [__cx] __modify __exact [__ax]

  int read(int fd, void *buf, unsigned int count);
#  pragma aux read = "mov ah, 3fh"  "int 21h"  "jnc ok"  "sbb ax, ax"  "ok:" \
      __value [__ax] __parm [__bx] [__dx] [__cx] __modify __exact [__ax]

  /* Just returns whether fd is a character device. (On Unix, there are
   * non-TTY character devices, but this implementation pretends that they
   * don't exist.) It works correctly on
   * [kvikdos](https://github.com/pts/kvikdos), emu2 and (windowed) DOSBox.
   * OpenWatcom lib does the same (
   * https://github.com/open-watcom/open-watcom-v2/blob/2d1ea451c2dbde4f1efd26e14c6dea3b15a1b42b/bld/clib/environ/c/isatt.c#L58C10-L59
   * ).
   */
  int isatty(int fd);
#  pragma aux isatty = "mov ax, 4400h"  "int 21h"  "mov ax, 0"  "jc done"  \
      "test dl, dl"  "jns done"  "inc ax"  "done:" \
      __value [__ax] __parm [__bx] __modify __exact [__ax __bx __dx]

  __declspec(noreturn) void exit(unsigned char exit_code);
#  pragma aux exit = "mov ah, 4ch"  "int 21h" \
      __parm [__al] __modify __exact []

  char *main0_argv1(void);
#  pragma aux main0_argv1 = "mov si, 81h"  "next:"  "lodsb"  "cmp al, 32" \
      "je next"  "cmp al, 9"  "je next"  "dec si" \
      __value [__si] __modify __exact [__si __al]  /* This is correct for a DOS .com program, but not for an .exe. */

  unsigned int strlen(const char *s);
  void *memset(void *s, int c, unsigned int n);  /* Not used in luuzcat with IS_X86_16, memset_void(...) is used instead. */
  void *memcpy(void *dest, const void *src, unsigned n);  /* Not used in luuzcat with IS_X86_16. */
  /* To prevent wlink: Error! E2028: strlen_ is an undefined reference */
#  if !(defined(__SMALL__) || defined(__MEDIUM__))  /* This works in any memory model. */
#    pragma intrinsic(strlen)
#  else
#    pragma aux strlen = "push ds"  "pop es"  "mov cx, -1"  "xor ax, ax"  "repne scasb"  "not cx"  "dec cx"  __parm [__di] __value [__cx] __modify __exact [__es __di __ax]  /* 2 bytes shorter than the `#pragma intrinsic', because `push ds ++ pop es' is shorter than 2 movs. */
#  endif
#  if !(defined(__SMALL__) || defined(__MEDIUM__))  /* This works in any memory model. */
#    pragma intrinsic(memset)  /* Not used in luuzcat with IS_X86_16, memset_void(...) is used instead. */
#  else
#    pragma aux memset = "push ds"  "pop es"  "push di"  "rep stosb"  "pop di" __parm [__di] [__al] [__cx] __value [__di] __modify __exact [__es __cx]  /* Shorter than the `#pragma intrinsic', because `push ds ++ pop es' is shorter than 2 movs. */
#  endif
#  pragma intrinsic(memcpy)  /* Not used in luuzcat with IS_X86_16. */

#  define get_prog_mem_end_seg() (*(const unsigned*)2)  /* Fetch it from the DOS Program Segment Prefix (PSP): https://fd.lod.bz/rbil/interrup/dos_kernel/2126.html */
  unsigned get_psp_seg(void);
#  pragma aux get_psp_seg = "mov ax, cs" __value [__ax] __modify []  /* This is correct for a DOS .com program, but not for an .exe. */

#  define LUUZCAT_NL "\r\n"  /* Line ending for error messages on stderr. */
#endif

#ifndef LIBC_PREINCLUDED
#  ifdef __MMLIBC386__
#    include <mmlibc386.h>
#    define LUUZCAT_NL "\r\n"  /* We want \r\n, and write(STDERR_FILENO, ..., ...) doesn't convert "\n" to "\r\n". */
#  else
#    include <stdlib.h>  /* exit(...), abort(...). */
#    include <string.h>
#    include <limits.h>   /* INT_MAX. */
    /* Turbo C++ 1.01 defines __MSDOS__. OpenWatcom C compiler defines __DOS__, __COM__, __NT__ or __OS2__. */
#    if defined(MSDOS) || defined(_MSDOS) || defined(_WIN32) || defined(_WIN64) || defined(__DOS__) || defined(__COM__) || defined(__NT__) || defined(__MSDOS__) || defined(__OS2__)
#      include <fcntl.h>  /* O_BINARY. */
#      include <io.h>  /* read(...), write(...), setmode(...), isatty(0). */
#    else
#      include <unistd.h>  /* read(...), write(...), isatty(...). */
#    endif
#    ifdef USE_DEBUG
#      include <stdio.h>  /* fprintf(stderr, ...); */
#    endif
#    define LUUZCAT_NL "\n"  /* On DOS etc. write(STDERR_FILENO, ..., ...) will append an "\r" because the lack of O_BINARY. */
#  endif
#endif

#ifndef memset_void
#  if defined(__WATCOMC__) && defined(__386__)
    void *memset_void(void *s, char c, unsigned int n);
#    pragma aux memset_void = "rep stosb" __parm [__edi] [__al] [__ecx] __modify __exact [__edi __ecx]  /* Shorter than the `#pragma intrinsic', because `push di ++ ... ++ pop di' is omitted. */
#    define memset_void(s, c, n) memset_void(s, c, n)
#  else
#    if defined(__WATCOMC__) && IS_X86_16 && (defined(__SMALL__) || defined(__MEDIUM__))
      void *memset_void(void *s, char c, unsigned int n);
#      pragma aux memset_void = "push ds"  "pop es"  "rep stosb" __parm [__di] [__al] [__cx] __modify __exact [__es __di __cx]  /* Shorter than the `#pragma intrinsic', because `push ds ++ pop es' is shorter than 2 movs, and `push di ++ ... ++ pop di' is omitted. */
#      define memset_void(s, c, n) memset_void(s, c, n)
#    else
#      define memset_void(s, c, n) memset(s, c, n)
#    endif
#  endif
#endif

#if (INT_MAX >> 15 >> 15 || __INT_MAX__ >> 15 >> 15 || __SIZEOF_INT__ >= 4 || __INTSIZE >= 4 || defined(__LP64__) || defined(_LP64) \
     || defined(vax) || defined(sun) \
     || defined(_WIN32) || defined(_WIN64) || defined(__NT__) || defined(WIN32) || defined(__WIN32__) || defined(__DOS32__) \
     || defined(__i386) || defined(__i386__) || defined(__386) || defined(__386__) || defined(_M_I386) \
     || defined(__amd64__) || defined(__x86_64__) || defined(_M_X64) || defined(_M_AMD64) || defined(__X86_64__) || defined(__x86_64) || defined(_M_X64) \
     || defined(_M_ARM64) || defined(__AARCH64EB__) || defined(__AARCH64EL__) || defined(__aarch64__) \
     || defined(__ia64__) || defined(_M_IA64) || defined(__s390x__) || defined(__ppc64__) || defined(__PPC64__) \
     || defined(__riscv__) || defined(__mips__) \
    ) && !(defined(__WATCOMC__) && _M_I86)
typedef unsigned int um32;  /* Avoid using 8 bytes for um32. */
#else
typedef unsigned long um32;  /* At least 32 bits. */
#endif

#ifndef NULL
#  define NULL ((void*)0)
#endif

#ifndef   STDIN_FILENO
#  define STDIN_FILENO 0
#endif

#ifndef   STDOUT_FILENO
#  define STDOUT_FILENO 1
#endif

#ifndef   STDERR_FILENO
#  define STDERR_FILENO 2
#endif

#ifndef   EXIT_SUCCESS
#  define EXIT_SUCCESS 0
#endif

#ifndef   EXIT_FAILURE
#  define EXIT_FAILURE 1
#endif

#ifdef __TURBOC__  /* Turbo C++ 1.01 emits a subspicious pointer conversion warning without the (char*) cast. */
#  define WRITE_PTR_ARG_CAST(arg) ((char*)(arg))
#else
#  define WRITE_PTR_ARG_CAST(arg) ((const char*)(arg))
#endif

#ifdef __noreturn
#else
#  ifdef __GNUC__
#    define __noreturn __attribute__((__noreturn__))
#  else
#    ifdef __WATCOMC__
#      define __noreturn __declspec(noreturn)
#    else
#      define __noreturn
#    endif
#  endif
#endif

#ifndef LIBC_HAVE_WRITE_NONZERO_VOID
#  define write_nonzero_void(fd, buf, count) (void)!write(fd, buf, count)
#endif

#ifndef LIBC_HAVE_WRITE_NONZERO
#  define write_nonzero(fd, buf, count) write(fd, buf, count)
#endif

#if IS_DOS_16 && (defined(__WATCOMC__) || defined(__TURBOC__))
#  define LUUZCAT_MALLOC_OK 1  /* For uncompress.c. */
#endif

/* --- Typedefs etc. */

typedef unsigned char uc8;  /* Always a single byte. For interfacing with read(2) and write(2). */
typedef unsigned char um8;  /* Unsigned integer type which is at least 8 bits, preferably 8 bits. */
typedef unsigned char ub8;  /* Unsigned integer type which is at least 1 bit (boolean), preferably 8 bits. */
typedef unsigned short um16;  /* Unsigned integer type which is at least 16 bits, preferably 16 bits. Changing this to `unsigned long' would also work, but that would waste memory. */
typedef char assert_sizeof_uc8[sizeof(uc8) == 1 ? 1 : -1];  /* We also rely on the implicit `& 0xff' behavior of global_bitbuf16 for USE_TREE. */
typedef char assert_sizeof_um16[sizeof(um16) >= 2 ? 1 : -1];

/* --- Error reporting. */

__noreturn void fatal_msg(const char *msg);
__noreturn void fatal_read_error(void);
__noreturn void fatal_write_error(void);
__noreturn void fatal_unexpected_eof(void);
__noreturn void fatal_corrupted_input(void);
#ifdef LUUZCAT_MALLOC_OK
  __noreturn void fatal_out_of_memory(void);
#endif
__noreturn void fatal_unsupported_feature(void);

/* --- Reading. */

/* This many bytes should be read to global_read_buffer at a time. It must
 * be a power of 2 divisible by 0x1000 for disk block alignment.
 */
#define READ_BUFFER_SIZE 0x2000
/* We allocate a few more bytes (READ_BUFFER_EXTRA bytes) of read buffer so
 * that compress (LZW) decompression can always read READ_BUFFER_SIZE bytes
 * at a time (even if it doesn't read to the start of the buffer), so it
 * remains on disk block boundary.
 */
#define READ_BUFFER_EXTRA 2
/* We allocate even more bytes (READ_BUFFER_OVERSHOOT bytes) of read buffer
 * so that reading a few more bytes using `((int*)p)[0]' wouldn't be an
 * out-of-bounds array access.
 */
#define READ_BUFFER_OVERSHOOT (sizeof(unsigned int) > 2 ? sizeof(unsigned int) - 2 : 1)
/* uc8 global_read_buffer[READ_BUFFER_SIZE + READ_BUFFER_EXTRA + READ_BUFFER_OVERSHOOT]; */
extern uc8 global_read_buffer[];

#define BEOF (-1U)  /* Returned by read_byte(1) and try_byte(). */

extern unsigned int global_insize; /* Number of valid bytes in global_read_buffer. */
extern unsigned int global_inptr;  /* Index of next byte to be processed in global_read_buffer. */
extern um32 global_total_read_size;  /* read_byte(...) increses it after each read from the filehandle to global_read_buffer. */
extern ub8 global_read_had_eof;  /* Was there an EOF already when reading? */
unsigned int read_byte(ub8 is_eof_ok);
void read_force_eof(void);
unsigned int get_le16(void);

/* These are fast wrappers around read_byte(...) for speed. */
#define get_byte() (global_inptr < global_insize ? global_read_buffer[global_inptr++] : (uc8)read_byte(0))  /* Returns uc8. Fails with a fatal error on EOF. */
#define try_byte() (global_inptr < global_insize ? (unsigned int)global_read_buffer[global_inptr++] : read_byte(1))  /* Returns unsigned int: either 0..255 for a byte or BEOF == (-1U) to indicate EOF. */

/* --- Writing. */

#define WRITE_BUFFER_SIZE 0x8000U
extern uc8 global_write_buffer[WRITE_BUFFER_SIZE];

unsigned int flush_write_buffer(unsigned int size);

#define flush_full_write_buffer_in_the_beginning() do {} while (0)  /* Usually this is a no-op, because the write buffer is not full in the beginning. */

/* --- Decompression. */

/* Value is this because build_crc32_table_if_needed(...) checks crc32_table[1] != 0. */
#define DOS_16_DUMMY_SIZE (sizeof(um32) << 1)

#if IS_DOS_16
#  define DOS_16_DUMMY char dos_16_dummy[DOS_16_DUMMY_SIZE];
#  define DOS_16_INVALIDATE_DEFLATE_CRC32_TABLE do { big.dos_16_dummy[sizeof(um32)] = 0; } while (0)  /* This makes `(um8)crc32_table[1] != 0' false in build_crc32_table_if_needed(...). */
#else
#  define DOS_16_DUMMY
#  define DOS_16_INVALIDATE_DEFLATE_CRC32_TABLE do {} while (0)
#endif

#define SCOLZH_NC (255 + 1 + 256 + 2 - 3)
#define SCOLZH_NPT 19

/* We specify this struct in the .h file so what we overlap it in memory with other big struct in `big' below. */
struct scolzh_big {
  DOS_16_DUMMY
  um16 left[ 2 * SCOLZH_NC - 1];
  um16 right[2 * SCOLZH_NC - 1];

  um8  c_len[SCOLZH_NC];
  um16 c_table[1U << 12];

  um8  pt_len[SCOLZH_NPT];  /* gzip-1.14/unlzh.c has pt_len[1 << TBIT] here, which is larger. It doesn't seem to be needed, there are checks below for indexing. */
  um16 pt_table[1U << 8];
};

#define COMPACT_NF 258

typedef um16 compact_dicti_t;
typedef um16 compact_diri_t;
typedef um16 compact_dictini_t;
typedef um8 compact_flags_t;

struct compact_fpoint {
  compact_dicti_t fpdni;  /* NULL is indicated by the value NFNULL == NF. */
  compact_flags_t flags;
};

struct compact_index {
  compact_dicti_t ipt;
  compact_diri_t nextri;
};

struct compact_son {
  compact_dictini_t spdii;  /* NFNULL value not allowed for the dicti_t. */
  compact_diri_t topri;
  um32 count;
};

struct compact_node {
  struct compact_fpoint fath;
  struct compact_son sons[2];
};

struct compact_big {
  DOS_16_DUMMY
  struct compact_node dict[COMPACT_NF];
  struct compact_fpoint in[COMPACT_NF];
  struct compact_index dir[COMPACT_NF << 1];
};

#define OPACK_TREESIZE 1024

struct opack_big {
  DOS_16_DUMMY
  um16 tree[OPACK_TREESIZE];
};

#define PACK_EOF_IDX 256

struct pack_big {
  DOS_16_DUMMY
  um16 leaf_count[24];  /* leaf_count[i] is the number of leaves on level i. Can be 0..257. */
  um8 intnode_count[24];  /* intnode_count[i] is the number of internal nodes on level i. Can be 0..254. */
  um16 byte_indexes[24];  /* This is an index in .bytes (0..255), or 1 more to indicate EOF. Even a valid index can be EOF if there are less bytes. */
  um8 bytes[256];
};

typedef um8 deflate_huffman_bit_count_t;

#define DEFLATE_MAX_TREE_SIZE 1490
#define DEFLATE_BIT_COUNT_ARY_SIZE 318

struct deflate_big {
#if IS_DOS_16
  um32 crc32_table[256];  /* Replaces DOS_16_DUMMY. */
#endif
  um16 huffman_trees_ary[DEFLATE_MAX_TREE_SIZE];
  deflate_huffman_bit_count_t huffman_bit_count_ary[DEFLATE_BIT_COUNT_ARY_SIZE];
};

#define FREEZE_N_CHAR2 511
#define FREEZE_T2 2043

struct compress_big {
  uc8 dummy[DOS_16_DUMMY_SIZE];  /* Used just as a placeholder used for a sizeof(...) < ~64 KiB check. compress doesn't have any big data structures to contribute. */
};

struct freeze_big {
  DOS_16_DUMMY
  um16 freq[FREEZE_T2 + 1];  /* Frequency table. */
  um16 child[FREEZE_T2];  /* Points to child node (child[i], child[i+1]). */
  um16 parent[FREEZE_T2 + FREEZE_N_CHAR2];  /* Points to parent node. */
  uc8 p_len[64];
  uc8 d_len[256];
  um8 code[256];
  um8 table2[8];
};

/* This contains the large arrays other than global_read_buffer and global_write_buffer. */
extern union big_big {
  DOS_16_DUMMY
  struct scolzh_big scolzh;
  struct compact_big compact;
  struct opack_big opack;
  struct pack_big pack;
  struct deflate_big deflate;
  struct compress_big compress;
  struct freeze_big freeze;
} big;

/* This is always true, otherwise there is no way to communicate the write_idx. !! Add global_write_idx. */
#define LUUZCAT_WRITE_BUFFER_IS_EMPTY_AT_START_OF_DECOMPRESS 1
/* These functions decompress from stdin (fd STDIN_FILENO == 0) to stdout. */
void decompress_scolzh_nohdr(void);
void decompress_compact_nohdr(void);
void decompress_opack_nohdr(void);
void decompress_pack_nohdr(void);
void decompress_deflate(void);
#if 0
  void decompress_quasijarus_nohdr(void)
#else
#  define decompress_quasijarus_nohdr() decompress_deflate()
#endif
void decompress_zlib_nohdr(void);
void decompress_gzip_nohdr(void);
void decompress_zip_struct_nohdr(void);
void decompress_compress_nohdr(void);
void decompress_freeze1_nohdr(void);
void decompress_freeze2_nohdr(void);

#endif  /* Of #ifndef _LUUZCAT_H */
