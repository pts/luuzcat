/* decompressor for the old pack format, based on and derived from pcat.c written by Steve Zucker earlier than 1977-07-13
 *
 * pts has ported it to C89 and added lots of extra error handling.
 *
 * The base source file pcat.c by Steve Zucker can be found as 2/rand/s2/pcat.c in
 * https://tuhs.v6sh.org/UnixArchiveMirror/Applications/Usenix_77/ug091377.tar.gz
 *
 * Based on https://retrocomputing.stackexchange.com/a/32121:
 *
 * > On the TUHS mailing list, [Clem Cole
 * > stated](https://www.tuhs.org/pipermail/tuhs/2020-November/022485.html):
 * >
 * > \[Steve\] Zucker wrote \[pack\] at Rand - early/mid 1970s. IIRC, It was
 * > later included in the original Harvard USENIX tape in the `Rand`
 * > directory.
 * >
 * > This [Unix User Tape from September
 * > 1977](https://tuhs.v6sh.org/UnixArchiveMirror/Applications/Usenix_77/ug091377.tar.gz)
 * > includes the source and documentation for pack, unpack, and pcat,
 * > indeed, in the rand directory. The archive files within
 * > [ug091377-ar.tar.gz](https://tuhs.v6sh.org/UnixArchiveMirror/Applications/Usenix_77/ug091377-ar.tar.gz)
 * > have a date of September 14, 1977. The files within the archive seem to
 * > have a date of July 13, 1977.
 *
 * Some other decompressor implementations for both the old and new pack formats:
 * [unpack.c](https://web.archive.org/web/20251123001300/https://raw.githubusercontent.com/illumos/illumos-gate/7c478bd95313f5f23a4c958a745db2134aa03244/usr/src/cmd/unpack/unpack.c)
 * in OpenSolaris (2005-06-14) and illumos, which is based on
 * [pcat.c](https://minnie.tuhs.org/cgi-bin/utree.pl?file=SysIII/usr/src/cmd/pcat.c)
 * in Unix System III (1978-04), adapted from a program by Thomas G.
 * Szymanski in 1978-03. Unlike unpack.c and pcat.c, this code has lots of
 * extra error handling to make it fail explicitly and safely for any
 * incorrect compressed data input. unpack.c is
 * also part of Unix V8, but not Unix V7.
 *
 * This is a Huffman decompressor. Its compressed input file format:
 *
 * * Signature: 2 bytes: 0x1f 0x1f.
 * * Number of chars in expanded file (PDP-11 dword).
 * * Number of words in expanded tree (PDP-11 word).
 * * Tree in 'compressed' form:
 * * * If 0<=byte<=0xfe, expand by zero padding to left
 * * * If byte=0xff, next two bytes for one PDP-11 word
 * * Terminal nodes: First word is zero; second is character
 * * Non-terminal nodes: Incremental (diff) 0/1 pointers.
 * * Code string for number of characters in expanded file.
 */

#include "luuzcat.h"

#define MAX_EDGE_DEPTH (256U - 1U)  /* A 100%-covered Huffman tree with 256 leaves has a maximum edge-depth of 256 - 1. */

void decompress_opack_nohdr(void) {
  unsigned int tp, dp, bit_count, write_idx, keysize, word, bits_remaining;
  um16 *t;
  um32 usize;

#if 0  /* The caller has already done this. */
  unsigned int i = try_byte();  /* First byte is ID1, must be 0x1f. */
  if (i == BEOF) fatal_msg("empty compressed input file" LUUZCAT_NL);  /* This is not an error for `gzip -cd'. */
  if (i != 0x1f || try_byte() != 0x1f) fatal_msg("missing old pack signature" LUUZCAT_NL);
#endif

  /* https://archive.org/download/bitsavers_decpdp11meoatingPointFormat_1047674/701110_The_PDP-11_Floating_Point_Format.pdf */
  if ((word = get_le16()) > 0x4000) fatal_corrupted_input();  /* Specifying size as PDP-11 32-bit float not supported. */
  /* PDP-11 middle-endian dword: https://en.wikipedia.org/wiki/Endianness#Middle-endian */
  usize = ((um32)word << 16) | get_le16();
  if ((keysize = get_le16()) - 2U > OPACK_TREESIZE - 2U) fatal_corrupted_input();
#ifdef USE_DEBUG
  fprintf(stderr, "usize=%lu keysize=%u\n", (unsigned long)usize, keysize);
#endif

  t = big.opack.tree;
  while (keysize-- != 0) {
    if ((tp = get_byte()) == 0xffU) {
      if ((tp = get_le16()) < 0xffU) fatal_corrupted_input();
    }
    if (tp >= keysize - 1U && keysize > 0xffU) fatal_corrupted_input();
    *t++ = tp;
  }
  bit_count = 0;
  write_idx = 0;  /* Assumes LUUZCAT_WRITE_BUFFER_IS_EMPTY_AT_START_OF_DECOMPRESS. */
  while (usize-- != 0) {
#ifdef USE_DEBUG
    fprintf(stderr, "/%lu\n", (unsigned long)usize);
#endif
    for (tp = 0, bits_remaining = MAX_EDGE_DEPTH; ; ) {
      if (bit_count == 0) {
        word = get_le16();
        bit_count = 16;
      }
#ifdef USE_DEBUG
      if (0) fputc(is_bit_15_set(word) ? '1' : '0', stderr);
#endif
      dp = tp;
      if (is_bit_15_set(word)) ++dp;  /* For IS_X86_16 && defined(__WATCOMC__), this is shorter here than is_bit_15_set_func(word) and combining with the lines above and below. */
      tp += dp = big.opack.tree[dp];
      if (sizeof(tp) > 2 && is_bit_15_set(dp)) tp -= (um16)0x10000UL;  /* Sign-extend the +dp. !! Is it needed in practice? */
#ifdef USE_DEBUG
      fprintf(stderr, "tp=%u\n", tp);
#endif
      if (tp >= keysize - 1U) fatal_corrupted_input();
      word <<= 1;
      --bit_count;
      if (big.opack.tree[tp] == 0) break;
      if (bits_remaining-- == 0) fatal_corrupted_input();  /* Fail on infinite loop in the tree pointers. */
    }
    if ((dp = big.opack.tree[tp + 1]) > 0xffU) fatal_corrupted_input();
    write_byte_using_write_idx(dp);
  }
}
