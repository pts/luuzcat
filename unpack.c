/* inspired by unpack.c in OpenSolaris (2005-06-14) and illumos
 * porting by pts@fazekas.hu at Sat Oct  4 00:14:54 CEST 2025
 *
 * This code is based on unpack.c in
 * [gzip 1.2.4](https://web.archive.org/web/20251122220330/https://mirror.netcologne.de/gnu/gzip/gzip-1.2.4.tar.gz)
 * and [gzip 1.14](https://web.archive.org/web/20250918225753/https://ftp.gnu.org/gnu/gzip/gzip-1.14.tar.gz)
 * by Jean-loup Gailly.
 *
 * Some other implementations:
 * [unpack.c](https://web.archive.org/web/20251123001300/https://raw.githubusercontent.com/illumos/illumos-gate/7c478bd95313f5f23a4c958a745db2134aa03244/usr/src/cmd/unpack/unpack.c)
 * in OpenSolaris (2005-06-14) and illumos, which is based on
 * [pcat.c](https://minnie.tuhs.org/cgi-bin/utree.pl?file=SysIII/usr/src/cmd/pcat.c)
 * in Unix System III (1978-04), adapted from a program by Thomas G.
 * Szymanski in 1978-03. Unlike unpack.c and pcat.c, this code has lots of
 * extra error handling to make it fail explicitly and safely for any
 * incorrect compressed data input.
 *
 * The base source file was downloaded from
 * https://minnie.tuhs.org/cgi-bin/utree.pl?file=SysIII/usr/src/cmd/pcat.c
 *
 * This is a Huffman decompressor.
 */

#include "luuzcat.h"

static um16 get_16be(void) {  /* Big-endian word. */
  const um8 high = get_byte();
  return (high << 8) | get_byte();
}

void decompress_pack_nohdr(void) {
  register unsigned int c, i, node_count, level, max_level, bit_count, write_idx, eof_idx;
  um32 usize;

#if 0  /* The caller has already done this. */
  unsigned int i = try_byte();  /* First byte is ID1, must be 0x1f. */
  if (i == BEOF) fatal_msg("empty compressed input file" LUUZCAT_NL);  /* This is not an error for `gzip -cd'. */
  if (i != 0x1f || try_byte() != 0x1e) fatal_msg("missing old pack signature" LUUZCAT_NL);
#endif
  usize = (um32)get_16be() << 16;
  usize |= get_16be();  /* Get uncompressed size as 32-bit big endian. */
  if ((max_level = get_byte()) > 24 || max_level == 0) { corrupted_input: fatal_corrupted_input(); }  /* Get number of levels. gzip-1.2.4 and gzip-1.14 check for `> 25' here. pack always generates <= 24, pcat.c checks for `> 24'. !! Allow `> 25' for compatibility with gzip. */
  for (i = c = 0, node_count = 1; i < max_level; ) {  /* Get number of leaves on level i. There is no less-than or greater-than ordering in big.pack.leaf_count[i]. */
    c += bit_count = big.pack.leaf_count[i] = get_byte();  /* Set big.pack.leaf_count[i] to 0..255. */
    if (c > 255) goto corrupted_input;  /* Too many literal byte values. gzip-1.2.4/unpack.c is buggy here, it checks (c > 256) instead. This has been fixed in gzip-1.14/unpack.c. */
    ++i;
    if (bit_count > node_count || (bit_count >= node_count && i == max_level)) goto corrupted_input;  /* gzip-1.2.4/unpack.c and pcat.c don't check it, but gzip-1.14/unpack.c does: too many leaves in the Huffman tree. */
    node_count = ((node_count - bit_count + 1) << 1) - 1;
  }
  for (i = eof_idx = 0; i < max_level; ++i) {  /* set big.pack.byte_indexes[i] to point to leaves for level i. */
    big.pack.byte_indexes[i] = eof_idx;
    for (c = big.pack.leaf_count[i]; c-- != 0; ) {
      big.pack.bytes[eof_idx++] = get_byte();  /* This is never out-of-bounds, we've checked `c > 255' above. After this, eof_idx is 1..255. */
    }
  }
  big.pack.bytes[eof_idx++] = get_byte();  /* This is never out-of-bounds, we've checked `c > 255' above. After this, eof_idx is 1..256. */
  /* big.pack.leaf_count[max_level - 1] += 2; */  /* We do it below. This wouldn't work for max_level == 0. */
  /* Now: all big.pack.leaf_count[i...] are 0..255. */
  /* Set big.pack.intnode_count[i...] to be number of internal nodes possessed by level i. */
  /* There is no less-than or greater-than ordering in big.pack.intnode_count[i...]. */
  node_count = big.pack.leaf_count[i = max_level - 1] += 2;  /* += 1 for the extra byte at level max_level - 1, += 1 for the end-of-stream marker. After this, 2 <= node_count <= 257 and 0 <= i <= 23. */
  big.pack.intnode_count[i] = 0;
  while (i-- != 0) {
    big.pack.intnode_count[i] = node_count >>= 1;  /* Set big.pack.intnode_count[i] to 0..254. */
    node_count += big.pack.leaf_count[i];
  }
  /* Now: all big.pack.leaf_count[i...] are 0..254; node_count is 0..509. */
  if ((node_count >> 1) != 1) goto corrupted_input;  /* gzip-1.2.4/unpack.c and pcat.c don't check it, but gzip-1.14/unpack.c does: too few leaves in the Huffman tree. */

  bit_count = write_idx = 0;
  for (;;) {  /* Read and decode the Huffman code bits, and write the decompressed output bytes. */
    for (i = level = 0; ; ++level) {  /* Walk the tree until a leaf is reached. */
#ifdef USE_CHECK
      if (level == max_level) abort();  /* No need to check this, because by this point the `break;' below has exited from the loop because of `big.pack.intnode_count[i] = 0;' above. */
#endif
      if (bit_count-- == 0) {
        c = get_byte();
        bit_count = 7;
      }
      i <<= 1;
      if (c & 0x80) i++;
      c <<= 1;
      if (i >= big.pack.intnode_count[level]) break;
    }
    i -= big.pack.intnode_count[level];
#ifdef USE_CHECK
    if (i >= big.pack.leaf_count[level]) abort();  /* No need to check this, because big.pack.leaf_count and big.pack_intnode_count were constructed this way (compilicated). */
#endif
    if ((i += big.pack.byte_indexes[level]) == eof_idx) break;  /* End-of-stream marker found. */
    if (usize-- == 0) goto corrupted_input;  /* Too many outbut bytes before end-of-stream marker. gzip-1.2.4/unpack.c and pcat.c doen't have this error check, but they do it only later. */
    global_write_buffer[write_idx] = big.pack.bytes[i];
    if (++write_idx == WRITE_BUFFER_SIZE) write_idx = flush_write_buffer(WRITE_BUFFER_SIZE);
  }
  if (usize != 0) goto corrupted_input;
  flush_write_buffer(write_idx);
}
