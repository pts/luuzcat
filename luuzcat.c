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
__noreturn void fatal_out_of_memory(void) { fatal_msg("out of memory" LUUZCAT_NL); }
__noreturn void fatal_unsupported_feature(void) { fatal_msg("unsupported feature" LUUZCAT_NL); }

uc8 global_read_buffer[READ_BUFFER_SIZE + READ_BUFFER_EXTRA + READ_BUFFER_OVERSHOOT];
unsigned int global_insize; /* Number of valid bytes in global_read_buffer. */
unsigned int global_inptr;  /* Index of next byte to be processed in global_read_buffer. */
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
  global_insize = got;
  global_inptr = 1;
  return global_read_buffer[0];
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

#ifndef main0
#  define main0() int main(void)
#  define main0_exit0() return EXIT_SUCCESS
#endif

main0() {
  unsigned int b;

#if O_BINARY  /* For DOS, Windows (Win32 and Win64) and OS/2. */
  setmode(STDIN_FILENO, O_BINARY);
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
    } else if ((b & 0xf) == 8 && (b >> 4) < 8) {  /* CM byte for zlib. Valid values are 0x08, 0x18, ..., 0x78. Check that CM == 8, check that CINFO <= 7, otherwise ignore CINFO (sliding window size). */
      if (((b = get_byte()) & 0x20)) goto bad_signature;  /* FLG byte. Check that FDICT == 0. Ignore FLEVEL and FCHECK. */
      decompress_zlib_nohdr();  /* This is based on Deflate, no need for DOS_16_INVALIDATE_DEFLATE_CRC32_TABLE. */
    } else if (b == 0x50) {  /* 'P'. */
      if ((b = try_byte()) == 0x4b) {  /* 'K'. */
        decompress_zip_struct_nohdr();  /* This is based on Deflate, no need for DOS_16_INVALIDATE_DEFLATE_CRC32_TABLE. */
      } else {
        goto bad_signature;
      }
    } else {
      bad_signature: fatal_msg("compressed signature not recognized" LUUZCAT_NL);
    }
  }
  main0_exit0();  /* return EXIT_SUCCESS; */
}
