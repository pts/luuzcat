/* based on and derived from pcat.c written by Steve Zucker earlir than 1977-07-13
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

static um16 get_w(void) {
  /* (PDP-11) little-endian word. */
  const um16 low = get_byte();
  return low | (get_byte() << 8);
}

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
  if ((word = get_w()) > 0x4000) fatal_corrupted_input();  /* Specifying size as PDP-11 32-bit float not supported. */
  /* PDP-11 middle-endian dword: https://en.wikipedia.org/wiki/Endianness#Middle-endian */
  usize = ((um32)word << 16) | get_w();
  if ((keysize = get_w()) - 2U > OPACK_TREESIZE - 2U) fatal_corrupted_input();
#ifdef USE_DEBUG
  fprintf(stderr, "usize=%lu keysize=%u\n", (unsigned long)usize, keysize);
#endif

  t = big.opack.tree;
  while (keysize-- != 0) {
    if ((tp = get_byte()) == 0xffU) {
      if ((tp = get_w()) < 0xffU) fatal_corrupted_input();
    }
    if (tp >= keysize -1U && keysize > 0xffU) fatal_corrupted_input();
    *t++ = tp;
  }
  bit_count = 0;
  write_idx = 0;
  /* !! Output for test_C1_pack_old.z is something weird. */
  while (usize-- != 0) {
#ifdef USE_DEBUG
    fprintf(stderr, "/%lu\n", (unsigned long)usize);
#endif
    for (tp = 0, bits_remaining = MAX_EDGE_DEPTH; ; ) {
      if (bit_count == 0) {
        word = get_w();
        bit_count = 16;
      }
#ifdef USE_DEBUG
      if (0) fputc((word & 0x8000U) ? '1' : '0', stderr);
#endif
      dp = tp;
      if ((word & 0x8000U) != 0) ++dp;
      tp += dp = big.opack.tree[dp];
      if (sizeof(tp) > 2 && (dp & 0x8000U) != 0) tp -= (um16)0x10000UL;  /* Sign-extend the +dp. !! Is it needed in practice? */
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
    global_write_buffer[write_idx] = dp;
    if (++write_idx == WRITE_BUFFER_SIZE) write_idx = flush_write_buffer(write_idx);
#ifdef USE_DEBUG
    flush_write_buffer(write_idx); write_idx = 0;
#endif
  }
  flush_write_buffer(write_idx);
}
