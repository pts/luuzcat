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

/* --- Error reporting. */

__noreturn void fatal_msg(const char *msg) {
  write_nonzero_void(STDERR_FILENO, WRITE_PTR_ARG_CAST(msg), strlen(msg));
  exit(EXIT_FAILURE);
}

/* !! Add different exit codes. */
__noreturn void fatal_read_error(void) { fatal_msg("read error" LUUZCAT_NL); }
__noreturn void fatal_write_error(void) { fatal_msg("write error" LUUZCAT_NL); }
__noreturn void fatal_unexpected_eof(void) { fatal_msg("unexpected EOF" LUUZCAT_NL); }
__noreturn void fatal_corrupted_input(void) { fatal_msg("corrupted input" LUUZCAT_NL); }
#ifdef LUUZCAT_MALLOC_OK
  __noreturn void fatal_out_of_memory(void) { fatal_msg("out of memory" LUUZCAT_NL); }
#endif
__noreturn void fatal_unsupported_feature(void) { fatal_msg("unsupported feature" LUUZCAT_NL); }

uc8 global_read_buffer[READ_BUFFER_SIZE + READ_BUFFER_EXTRA + READ_BUFFER_OVERSHOOT];
unsigned int global_insize; /* Number of valid bytes in global_read_buffer. */
unsigned int global_inptr;  /* Index of next byte to be processed in global_read_buffer. */
um32 global_total_read_size;  /* read_byte(...) increses it after each read from the filehandle to global_read_buffer. */
ub8 global_read_had_eof;  /* Was there an EOF already when reading? */

/* --- Reading. */

void read_force_eof(void) {
  global_insize = global_inptr = 0;
  global_read_had_eof = 1;
}

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
  global_total_read_size += global_insize = got;
  global_inptr = 1;
  return global_read_buffer[0];
}

unsigned int get_le16(void) {
  const unsigned int i = get_byte();
  return i | (get_byte() << 8);
}

/* --- Writing. */

uc8 global_write_buffer[WRITE_BUFFER_SIZE];

/* Always returns 0, which is the new buffer write index. */
unsigned int flush_write_buffer(unsigned int size) {
  unsigned int size_i;
  int got;
  for (size_i = 0; size_i < size; ) {
    got = (int)(size - size_i);
    if (sizeof(int) == 2 && got < 0) got = 0x4000;
    if ((got = write_nonzero(STDOUT_FILENO, WRITE_PTR_ARG_CAST(global_write_buffer + size_i), got)) <= 0) fatal_write_error();
    size_i += got;
  }
  return 0;
}

/* --- main. */

union big_big big;

static const char usage_msg[] = "Usage: luuzcat [-r] <input.gz >output" LUUZCAT_NL "https://github.com/pts/luuzcat" LUUZCAT_NL;
static const char more_msg[] = "more compressed input expected" LUUZCAT_NL;

#define MAIN_FLAG_RAW_DEFLATE 8
#define MAIN_FLAG_SUBSEQUENT 5  /* This is strlen("more "), of the prefix of more_msg. */

#if ' ' == 32  /* ASCII system. */
#  define IS_PROGARG_TERMINATOR(c) ((unsigned char)(c) <= ' ')  /* ASCII only: matches '\0' (needed for Unix), '\t', '\n', '\r' (needed for DOS) and ' '. */
#else  /* Non-ASCII system. */
  static ub8 is_progarg_terminator(char c) { return c == '\0' || c == '\t' || c == '\n' || c == '\r' || c == ' '; }
#  define IS_PROGARG_TERMINATOR(c) is_progarg_terminator(c)
#endif

#ifndef main0
#  define main0() int main(int argc, char **argv)
#  define main0_exit0() return EXIT_SUCCESS
#  define main0_argv1() ((void)argc, argv[0] ? argv[1] : NULL)
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
  if ((argv1 == NULL || IS_PROGARG_TERMINATOR(*argv1)) && isatty(STDIN_FILENO)) { do_usage:
    fatal_msg(usage_msg);
  }
  if (argv1 != NULL) {
    while (!IS_PROGARG_TERMINATOR(b = *(const unsigned char*)argv1++)) {
      b |= 0x20;  /* Convert uppercase A-Z to lowercase a-z. */
      if (b == 'r') flags |= MAIN_FLAG_RAW_DEFLATE;
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
        decompress_scolzh_nohdr();  /* This is based on Deflate, no need for DOS_16_INVALIDATE_DEFLATE_CRC32_TABLE. */
      } else if (b == 0x8b) {
        decompress_gzip_nohdr();  /* This is based on Deflate, no need for DOS_16_INVALIDATE_DEFLATE_CRC32_TABLE. */
      } else if (b == 0xa1) {
        decompress_quasijarus_nohdr();  /* This is based on Deflate, no need for DOS_16_INVALIDATE_DEFLATE_CRC32_TABLE. */
      } else if (b == 0xff) {  /* This is the less common signature for Compact. */
        do_compact: decompress_compact_nohdr();
        do_invalidate: DOS_16_INVALIDATE_DEFLATE_CRC32_TABLE;
      } else if (b == 0x1e) {
        decompress_pack_nohdr(); goto do_invalidate;
      } else if (b == 0x1f) {
        decompress_opack_nohdr(); goto do_invalidate;
      } else if (b == 0x9d) {
        decompress_compress_nohdr();  /* This doesn't use big, no need for DOS_16_INVALIDATE_DEFLATE_CRC32_TABLE. */
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
      decompress_deflate();  /* This is based on Deflate, no need for DOS_16_INVALIDATE_DEFLATE_CRC32_TABLE. */
    } else if ((b & 0xf) == 8 && (b >> 4) < 8) {  /* CM byte for zlib. Valid values are 0x08, 0x18, ..., 0x78. Check that CM == 8, check that CINFO <= 7, otherwise ignore CINFO (sliding window size). */
      if (((b = get_byte()) & 0x20)) goto bad_signature;  /* FLG byte. Check that FDICT == 0. Ignore FLEVEL and FCHECK. */
      decompress_zlib_nohdr();  /* This is based on Deflate, no need for DOS_16_INVALIDATE_DEFLATE_CRC32_TABLE. */
    } else if (b == 0x50) {  /* 'P'. */
      if ((b = try_byte()) == 0x4b) {  /* 'K'. */
        decompress_zip_struct_nohdr();  /* This is based on Deflate, no need for DOS_16_INVALIDATE_DEFLATE_CRC32_TABLE. */
      } else {
        goto bad_signature;
      }
    } else if (b == 0) {  /* Allow NUL bytes. */
    } else { bad_signature:
      fatal_msg(more_msg + 5 - (flags & MAIN_FLAG_SUBSEQUENT));
    }
    flags |= MAIN_FLAG_SUBSEQUENT;
  }
  main0_exit0();  /* return EXIT_SUCCESS; */
}
