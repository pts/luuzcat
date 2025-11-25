/* by pts@fazekas.hu at Thu Oct  2 22:32:09 CEST 2025 */

#define LUUZCAT_MAIN 1  /* Make luuzcat.h generate some functions early for the DOS .com target. */
#include "luuzcat.h"

#if defined(__TURBOC__) && defined(__MSDOS__)
  unsigned _stklen = 0x300;  /* Global variable used by Turbo C++ 1.x libc _start. */
#endif

#if defined(_WIN32) && defined(__WATCOMC__) && defined(_WCDATA)
/* Overrides lib386/nt/clib3r.lib / mbcupper.o
 * Source: https://github.com/open-watcom/open-watcom-v2/blob/master/bld/clib/mbyte/c/mbcupper.c
 * Overridden implementation calls CharUpperA in USER32.DLL:
 * https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-charuppera
 *
 * This function is a transitive dependency of _cstart() with main() in
 * OpenWatcom. By overridding it, we remove the transitive dependency of all
 * .exe files compiled with `owcc -bwin32' on USER32.DLL.
 *
 * This is a simplified implementation, it keeps non-ASCII characters intact.
 */
unsigned int _mbctoupper(unsigned int c) {
  return (c - 'a' + 0U <= 'z' - 'a' + 0U)  ? c + 'A' - 'a' : c;
}
#endif

/* --- Common error reporting. */

#ifdef LUUZCAT_DUCML  /* This functionality is implemented in uncompress.c. */
#else
  __noreturn void fatal_msg(const char *msg) {
#  if USE_WRITE_FIX  /* Workaround for write(2) failing with EFAULT on OpenBSD 7.3--7.8 i386. (It works on 6.0, 7.0 and 7.2.). https://stackoverflow.com/q/79806755 */
    const unsigned size = strlen(msg);
    memcpy(global_write_buffer, msg, size);
    write_nonzero_void(STDOUT_FILENO, global_write_buffer, size);
#  else
    write_nonzero_void(STDERR_FILENO, WRITE_PTR_ARG_CAST(msg), strlen(msg));
#  endif
    exit(EXIT_FAILURE);
  }

  /* !! Add different exit codes. */
  __noreturn void fatal_read_error(void) { fatal_msg("read error" LUUZCAT_NL); }
  __noreturn void fatal_write_error(void) { fatal_msg("write error" LUUZCAT_NL); }
  __noreturn void fatal_unexpected_eof(void) { fatal_msg("unexpected EOF" LUUZCAT_NL); }
  __noreturn void fatal_corrupted_input(void) { fatal_msg("corrupted input" LUUZCAT_NL); }
#  ifdef LUUZCAT_MALLOC_OK  /* !! Also omit from luuzcatn.com. */
    __noreturn void fatal_out_of_memory(void) { fatal_msg("out of memory" LUUZCAT_NL); }
#  endif
#endif

/* --- Additional error reporting. */

__noreturn void fatal_unsupported_feature(void) { fatal_msg("unsupported feature" LUUZCAT_NL); }

/* --- Reading. */

void read_force_eof(void) {
  global_insize = global_inptr = 0;
  global_read_had_eof = 1;
}

#ifdef LUUZCAT_DUCML
  /* For LUUZCAT_DUCML, these global variables are defined in luuzcat.h:
   * global_read_buffer, global_read_had_eof, global_insize, global_inptr, global_total_read_size.
   *
   * For LUUZCAT_DUCML, read_byte(...) is implemented in uncompress.c.
   */
#else
#  ifndef LUUZCAT_SMALLBUF
    uc8 global_read_buffer[READ_BUFFER_SIZE + READ_BUFFER_EXTRA + READ_BUFFER_OVERSHOOT];
#  endif
  ub8 global_read_had_eof;  /* Was there an EOF already when reading? */
  unsigned int global_insize; /* Number of valid bytes in global_read_buffer. */
  unsigned int global_inptr;  /* Index of next byte to be processed in global_read_buffer. */
  um32 global_total_read_size;  /* read_byte(...) increses it after each read from the filehandle to global_read_buffer. */

  unsigned int LUUZCAT_WATCALL_FROM_ASM read_byte(ub8 is_eof_ok) {
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
    global_total_read_size += global_insize = got;
    global_inptr = 1;
    return global_read_buffer[0];
  }

#endif

um16 global_bitbuf8;

#if defined(__WATCOMC__) && ((IS_X86_16 && (defined(__SMALL__) || defined(__MEDIUM__))) || (defined(__386__) && defined(__FLAT__)))
#  define LUUZCAT_WATCALL_FROM_ASM __watcall  /* Such a function is called from assembly, so we must fix the calling convention. */
#else
#  define LUUZCAT_WATCALL_FROM_ASM
#endif

#if IS_X86_16 && defined(__WATCOMC__) && (defined(__SMALL__) || defined(__MEDIUM__))
  static unsigned int read_bit_using_bitbuf8_impl(void);
#  pragma aux read_bit_using_bitbuf8_impl = \
      "mov ax, global_bitbuf8"  "add ax, ax"  "jnc save2"  "mov ax, global_inptr"  "cmp ax, global_insize"  "jae full"  "push bx"  "xchg bx, ax" \
      GLOBAL_READ_BUFFER_MOV_X86_16  "inc bx"  "mov global_inptr, bx"  "pop bx"  "jmp fromnewbyte"  "full: xor ax, ax"  "call read_byte" \
      "fromnewbyte: mov ah, 1"  "save2: mov global_bitbuf8, ax"  "rol al, 1"  "and ax, 1" __value [__ax] __modify __exact [__ax]
  unsigned int LUUZCAT_WATCALL_FROM_ASM read_bit_using_bitbuf8(void) { return read_bit_using_bitbuf8_impl(); }
#else
#if defined(__386__) && defined(__WATCOMC__) && defined(__FLAT__)
    static unsigned int read_bit_using_bitbuf8_impl(void);
#    pragma aux read_bit_using_bitbuf8_impl = \
      "mov ax, global_bitbuf8"  "add ax, ax"  "jnc save2"  "mov eax, global_inptr"  "cmp eax, global_insize"  "jae full" \
      "push ebx"  "xchg ebx, eax"  GLOBAL_READ_BUFFER_MOV_I386  "inc ebx"  "mov global_inptr, ebx"  "pop ebx"  "jmp fromnewbyte" \
      "full: xor eax, eax"  "call read_byte"  "fromnewbyte: mov ah, 1"  "save2: mov global_bitbuf8, ax"  "rol al, 1"  "and eax, 1" \
      __value [__eax] __modify __exact [__eax]
    unsigned int LUUZCAT_WATCALL_FROM_ASM read_bit_using_bitbuf8(void) { return read_bit_using_bitbuf8_impl(); }
#  else
    unsigned int LUUZCAT_WATCALL_FROM_ASM read_bit_using_bitbuf8(void) {
      /* !! Add longer but faster implementation (no call) with inline assembly for __WATCOMC__ IS_X86_17 and __386__: is_bit_15_set(global_bitbuf8) ? ... : read_bit_using_bitbuf8(). */
      register unsigned int bb = global_bitbuf8;
      if (is_bit_15_set(bb)) {  /* For IS_X86_16 && defined(__WATCOMC__), this is shorter here than is_bit_15_set_func(bb). */
        bb = add_set_higher_byte_1(get_byte());
      } else {
        bb <<= 1;
      }
      return is_bit_7_set_func(global_bitbuf8 = bb);  /* For IS_X86_16 && defined(__WATCOMC__), this is shorter here than is_bit_7_set(bb). */
    }
#  endif
#endif

unsigned int get_le16(void) {
  const unsigned int i = get_byte();
  return i | (get_byte() << 8);
}

/* --- Writing. */

#ifdef LUUZCAT_SMALLBUF
  uc8 *global_write_buffer_to_flush = global_write_buffer;  /* copy_read_buffer() in uncompress.c changes it. */
#else
  uc8 global_write_buffer[WRITE_BUFFER_SIZE];
#  define global_write_buffer_to_flush global_write_buffer
#endif

#ifdef LUUZCAT_DUCML  /* For LUUZCAT_DUCML, flush_write_buffer(...) is implemented in uncompress.c. */
#else
  /* Always returns 0, which is the new buffer write index. */
  unsigned int flush_write_buffer(unsigned int size) {
    unsigned int size_i;
    int got;
    for (size_i = 0; size_i < size; ) {
      got = (int)(size - size_i);
      if (sizeof(int) == 2 && WRITE_BUFFER_SIZE >= 0x8000U && got < 0) got = 0x4000;  /* !! size optimization elsewhere: Not needed for DOS 3.00 if we check for == -1 below, and we return on == 0.  */
      if ((got = write_nonzero(STDOUT_FILENO, WRITE_PTR_ARG_CAST(global_write_buffer_to_flush + size_i), got)) <= 0) fatal_write_error();
      size_i += got;
    }
    return 0;
  }
#endif

/* --- main. */

union big_big big;

/* The more usual `static const char usage_msg[] = "...";' also works, but __WATCOMC__ would align it to 4 or 2. */
#define USAGE_MSG ("Usage: luuzcat [-][r][m] <input.gz >output" LUUZCAT_NL "https://github.com/pts/luuzcat" LUUZCAT_NL)
#define MORE_MSG ("more compressed input expected" LUUZCAT_NL)

#define MAIN_FLAG_STATE_HAD_ARCHIVE_MEMBER 32
#define MAIN_FLAG_MULTIPLE_ARCHIVE_MEMBERS 16
#define MAIN_FLAG_RAW_DEFLATE 8
#define MAIN_FLAG_SUBSEQUENT 5  /* This is strlen("more "), of the prefix of MORE_MSG. */

#if ' ' == 32  /* ASCII system. */
#  define IS_PROGARG_TERMINATOR(c) ((unsigned char)(c) <= ' ')  /* ASCII only: matches '\0' (needed for Unix), '\t', '\n', '\r' (needed for DOS) and ' '. */
#else  /* Non-ASCII system. */
  static ub8 is_progarg_terminator(char c) { return c == '\0' || c == '\t' || c == '\n' || c == '\r' || c == ' '; }
#  define IS_PROGARG_TERMINATOR(c) is_progarg_terminator(c)
#endif

#ifndef main0  /* True for standard C. */
#  define main0() int main(int argc, char **argv)
#  define main0_exit0() return EXIT_SUCCESS
#  define main0_argv1() ((void)argc, argv[0] ? argv[1] : NULL)
#  define main0_is_progarg_null(arg) ((arg) == NULL)
#endif

main0() {
  unsigned int b;
  const char *argv1 = main0_argv1();
  ub8 flags = 0;

#ifdef USE_DEBUG_ARGV
  (void)!write(STDERR_FILENO, WRITE_PTR_ARG_CAST(argv[0]), strlen(argv[0]));
  if (argv[1] != NULL) {
    (void)!write(STDERR_FILENO, WRITE_PTR_ARG_CAST(";"), 1);
    (void)!write(STDERR_FILENO, WRITE_PTR_ARG_CAST(argv[1]), strlen(argv[1]));
  }
  (void)!write(STDERR_FILENO, WRITE_PTR_ARG_CAST(".\r\n"), 3);
  return 0;
#endif
  /* We display the usage message if the are command-line arguments (or the
   * first argument is empty), and stdin is a terminal (TTY).
   */
  if ((main0_is_progarg_null(argv1) || IS_PROGARG_TERMINATOR(*argv1)) && isatty(STDIN_FILENO)) { do_usage:
    fatal_msg(USAGE_MSG);
  }
  if (!main0_is_progarg_null(argv1)) {
    while (!IS_PROGARG_TERMINATOR(b = *(const unsigned char*)argv1++)) {
      b |= 0x20;  /* Convert uppercase A-Z to lowercase a-z. */
      if (b == 'r') flags |= MAIN_FLAG_RAW_DEFLATE;
      if (b == 'm') flags |= MAIN_FLAG_MULTIPLE_ARCHIVE_MEMBERS;
      if (b == 'h') goto do_usage;  /* --help. */
    }
  }

#if O_BINARY  /* For DOS, Windows (Win32 and Win64) and OS/2. */
  setmode(STDIN_FILENO, O_BINARY);
  /* if (!isatty(STDOUT_FILENO)) -- we don't do this, because isatty(...) always returns true in some emulators. */
  setmode(STDOUT_FILENO, O_BINARY);  /* Prevent writing (in write(2)) LF as CRLF on DOS, Windows (Win32) and OS/2. */
#endif
  /* !! Test reinitialization by decompressing the same file again, and also a different file. */
  while ((int)(b = try_byte()) >= 0) {  /* zcat in gzip also accepts empty stdin. */
    /* !! Display different error message if subsequent bytes have a bad signature. */
    /* !! Skip over NUL bytes. */
    if (b == 0x1f) {
      if ((b = try_byte()) == 0xa0) {
        decompress_scolzh_nohdr();  /* This is based on Deflate, no need for invalidate_deflate_crc32_table(). */
      } else if (b == 0x8b) {
        decompress_gzip_nohdr();  /* This is based on Deflate, no need for invalidate_deflate_crc32_table(). */
      } else if (b == 0xa1) {
        decompress_quasijarus_nohdr();  /* 4.3BSD-Quasijarus Strong compression */  /* This is based on Deflate, no need for invalidate_deflate_crc32_table(). */
      } else if (b == 0xff) {  /* This is the less common signature for Compact. */
        do_compact: decompress_compact_nohdr();
        do_invalidate: invalidate_deflate_crc32_table();
      } else if (b == 0x1e) {
        decompress_pack_nohdr(); goto do_invalidate;
      } else if (b == 0x1f) {
        decompress_opack_nohdr(); goto do_invalidate;
      } else if (b == 0x9d) {
        decompress_compress_nohdr();  /* This doesn't use big, no need for invalidate_deflate_crc32_table(). */
      } else if (b == 0x9e) {
        decompress_freeze1_nohdr(); goto do_invalidate;
      } else if (b == 0x9f) {
        decompress_freeze2_nohdr(); goto do_invalidate;
      } else {
        goto bad_signature;
      }
    } else if (b == 0xff) {
      if ((b = try_byte()) == 0x1f) {  /* This is the more common signature for Compact. */
        goto do_compact;
      } else {
        goto bad_signature;
      }
    } else if ((flags & MAIN_FLAG_RAW_DEFLATE) != 0) {  /* The file formats below this are ambigious with raw Deflate. The latter takes precedence iff the -r flag has been specified. */
      --global_inptr;  /* Unread the first byte (b). */
      decompress_deflate();  /* This is based on Deflate, no need for invalidate_deflate_crc32_table(). */
    } else if ((b & 0xf) == 8 && (b >> 4) < 8) {  /* CM byte for zlib. Valid values are 0x08, 0x18, ..., 0x78. Check that CM == 8, check that CINFO <= 7, otherwise ignore CINFO (sliding window size). */
      if (((b = get_byte()) & 0x20)) goto bad_signature;  /* FLG byte. Check that FDICT == 0. Ignore FLEVEL and FCHECK. */
      decompress_zlib_nohdr();  /* This is based on Deflate, no need for invalidate_deflate_crc32_table(). */
    } else if (b == 0x50) {  /* 'P'. */
      if ((b = try_byte()) == 0x4b) {  /* 'K'. */
        b = get_byte();
        if (b == 3) {
          if ((flags & (MAIN_FLAG_MULTIPLE_ARCHIVE_MEMBERS | MAIN_FLAG_STATE_HAD_ARCHIVE_MEMBER)) == MAIN_FLAG_STATE_HAD_ARCHIVE_MEMBER) fatal_msg("multiple archive members" LUUZCAT_NL);
          flags |= MAIN_FLAG_STATE_HAD_ARCHIVE_MEMBER;
        }
        decompress_zip_struct_nohdr(b);  /* This is based on Deflate, no need for invalidate_deflate_crc32_table(). */
      } else {
        goto bad_signature;
      }
    } else if (b == 0) {  /* Allow NUL bytes. */
    } else { bad_signature:
      fatal_msg(MORE_MSG + 5 - (flags & MAIN_FLAG_SUBSEQUENT));
    }
    flags |= MAIN_FLAG_SUBSEQUENT;
  }
  main0_exit0();  /* return EXIT_SUCCESS; */
}
