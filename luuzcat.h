/* by pts@fazekas.hu at Thu Oct  2 22:32:09 CEST 2025 */

#ifndef _LUUZCAT_H
#define _LUUZCAT_H
 
#if defined(_DOSCOMSTART) && !defined(LIBC_PREINCLUDED)
  /* Tiny libc for OpenWatcom C compiler creating a DOS 8086 .com program.
   * For compilation instructions, see _DOSCOMSTART above.
   */
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
        __asm { mov di, offset _edata } __asm { mov cx, offset _end }
        __asm { sub cx, di } __asm { rep stosb } __asm { jmp main0 } }

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
      "mov ax, -1"  "ok:"  \
      __value [__ax] __parm [__bx] [__dx] [__cx] __modify __exact [__ax]

  int write(int fd, const void *buf, unsigned count);
#  pragma aux write = "xor ax, ax"  "jcxz ok"  "mov ah, 40h"  "int 21h" \
      "jnc ok"  "mov ax, -1"  "ok:"  \
      __value [__ax] __parm [__bx] [__dx] [__cx] __modify __exact [__ax]

  int read(int fd, void *buf, unsigned int count);
#  pragma aux read = "mov ah, 3fh"  "int 21h"  "jnc ok"  "mov ax, -1"  "ok:" \
      __value [__ax] __parm [__bx] [__dx] [__cx] __modify __exact [__ax]

  __declspec(noreturn) void exit(unsigned char exit_code);
#  pragma aux exit = "mov ah, 4ch"  "int 21h" \
      __parm [__al] __modify __exact []

  unsigned int strlen(const char *s);
  /* To prevent wlink: Error! E2028: strlen_ is an undefined reference */
#  pragma intrinsic(strlen)  /* !! Add shorter `#pragma aux' or static. */
  void *memset(void *s, int c, unsigned int n);
#  pragma intrinsic(memset)  /* !! Add shorter `#pragma aux' or static. */

#  define LUUZCAT_NL "\r\n"  /* Line ending for error messages on stderr. */
#endif

#ifndef LIBC_PREINCLUDED
#  ifdef __MMLIBC386__
#    include <mmlibc386.h>
#  else
#    include <stdlib.h>  /* exit(...), abort(...). */
#    include <string.h>
#    include <limits.h>   /* INT_MAX. */
    /* Turbo C++ 1.01 defines __MSDOS__. OpenWatcom C compiler defines __DOS__, __COM__, __NT__ or __OS2__. */
#    if defined(MSDOS) || defined(_MSDOS) || defined(_WIN32) || defined(_WIN64) || defined(__DOS__) || defined(__COM__) || defined(__NT__) || defined(__MSDOS__) || defined(__OS2__)
#      include <fcntl.h>  /* O_BINARY. */
#      include <io.h>  /* read(...), write(...), setmode(...). */
#    else
#      include <unistd.h>  /* read(...), write(...). */
#    endif
#    ifdef USE_DEBUG
#      include <stdio.h>  /* fprintf(stderr, ...); */
#    endif
#  endif
#  define LUUZCAT_NL "\n"  /* On DOS etc. write(STDERR_FILENO) will append an "\r" because the lack of O_BINARY. */
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

/* --- Reading. */

#define BEOF (-1U)

extern uc8 global_read_buffer[];
extern unsigned int global_insize; /* Number of valid bytes in global_read_buffer. */
extern unsigned int global_inptr;  /* Index of next byte to be processed in global_read_buffer. */
unsigned int read_byte(ub8 is_eof_ok);

/* These are fast wrappers around read_byte(...) for speed. */
#define get_byte() (global_inptr < global_insize ? global_read_buffer[global_inptr++] : (uc8)read_byte(0))  /* Returns uc8. Fails with a fatal error on EOF. */
#define try_byte() (global_inptr < global_insize ? (unsigned int)global_read_buffer[global_inptr++] : read_byte(1))  /* Returns unsigned int: either 0..255 for a byte or BEOF == (-1U) to indicate EOF. */

/* --- Writing. */

#define WRITE_BUFFER_SIZE 0x8000U
extern uc8 global_write_buffer[WRITE_BUFFER_SIZE];

unsigned int flush_write_buffer(unsigned int size);

/* --- Decompression. */

#define SCOLZH_NC (255 + 1 + 256 + 2 - 3)
#define SCOLZH_NPT 19

/* We specify this struct in the .h file so what we overlap it in memory with other big struct in `big' below. */
struct scolzh_big {
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
  struct compact_node dict[COMPACT_NF];
  struct compact_fpoint in[COMPACT_NF];
  struct compact_index dir[COMPACT_NF << 1];
};

#define OPACK_TREESIZE 1024

struct opack_big {
  um16 tree[OPACK_TREESIZE];
};

#define PACK_EOF_IDX 256

struct pack_big {
  um16 leaf_count[24];  /* leaf_count[i] is the number of leaves on level i. Can be 0..257. */
  um8 intnode_count[24];  /* intnode_count[i] is the number of internal nodes on level i. Can be 0..254. */
  um16 byte_indexes[24];  /* This is an index in .bytes (0..255), or 1 more to indicate EOF. Even a valid index can be EOF if there are less bytes. */
  um8 bytes[256];
};

typedef um8 deflate_huffman_bit_count_t;

#define DEFLATE_MAX_TREE_SIZE 1490
#define DEFLATE_BIT_COUNT_ARY_SIZE 318

struct deflate_big {
  um16 huffman_trees_ary[DEFLATE_MAX_TREE_SIZE];
  deflate_huffman_bit_count_t huffman_bit_count_ary[DEFLATE_BIT_COUNT_ARY_SIZE];
};

/* This contains the large arrays other than global_read_buffer and global_write_buffer. */
extern union big_big {
  struct scolzh_big scolzh;
  struct compact_big compact;
  struct opack_big opack;
  struct pack_big pack;
  struct deflate_big deflate;
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

#endif  /* Of #ifndef _LUUZCAT_H */
