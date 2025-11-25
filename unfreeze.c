/* based on and derived from freeze-2.5.0/{bitio.h,freeze.h,huf.h,bitio.c,decode.c,huf.c}
 * porting and extra error handling by pts@fazekas.hu at Sat Oct  4 03:56:40 CEST 2025
 *
 * pts has added some error handling, mostly with match_distance_limit.
 *
 * The base source file are part of the archive downloaded from
 * https://ibiblio.org/pub/linux/utils/compress/freeze-2.5.0.tar.gz
 *
 * Both Freeze 1.x and 2.x use adaptive Huffman + LZ. The LZ ring buffer
 * window size is 4 KiB for Freeze 1.x and 8 KiB for Freeze 2.x.
 *
 * The compressed data format LZSS + dyna is similar in design to that of LHA's "lh"
 * compression methods.
 *
 * Freeze is written by Leonid A. Broukhis.
 *
 * Freeze 2.1 was released on 1991-03-25, see e.g. this Usenet post on
 * comp.sources.misc:
 * http://cd.textfiles.com/sourcecode/usenet/compsrcs/misc/volume17/freeze/part01
 *
 * Freeze 2.5 was released on 1999-05-20.
 *
 * The code bit lengths for the LZ match distance Huffman table are
 * hardcoded for Freeze 1.x in table1: 0 1-bit-codes, 0 2-bit codes, 1 3-bit
 * code, 3 4-bit codes, 8 5-bit codes, 12 6-bit codes, 24 7-bit-codes, 16
 * 8-bit codes. They sum up to 0 + 0 + 1 + 3 + 8 + 12 + 24 + 16 = 64, so the
 * Huffman-coded values are 0..63. When decoding a distance, 6 additional
 * literal bits are read (as low bits) to form a 12-bit distance value, the
 * maximum distance is ~4096.
 *
 * The code bit lengths for the LZ match distance Huffman table are sent
 * right after 2-byte signature for Freeze 2.x. See their encoding below.
 * They sum up to 62, so the Huffman-coded values are 0..61. When
 * decoding a distance, 7 additional literal bits are read (as low bits) to
 * form a 13-bit distance value, the maximum distance is 7935 (not 8191,
 * because Huffman-coded values 62 and 63 are disallowed).
 *
 * The RLE-encoding of the distance bit lengths for Freeze 2.x is the
 * following: after the 2-byte signature, a 16-bit little-endian word is
 * read to x (the highest bit must be 0), and a byte is read to y (the 2
 * high bits must be 00). Then the bits a, bb, ccc, dddd, eeeee are
 * extracted (from low to high) from x, and ffffff from y. There are *a* 1-bit
 * codes, bb 2-bit codes, ccc 3-bit codes, dddd 4-bit codes, eeeee 5-bit
 * codes, ffffff 6-bit codes, n7 7-bit codes (see below) and n8 8-bit codes
 * (see below). n7 and n8 are the unique solutions of these set of equations:
 * a + bb + ccc + dddd + eeeee + ffffff + n7 + n8 == 62,
 * 128 * a + 64 * bb + 32 * ccc + 16 * dddd + 8 * eeeee + 4 * ffffff + 2 *
 * n7 + n8 == 256.
*
 * Then the Huffman code bit lengths of the LZ match distance values are the
 * following (encoded as RLE):
*
 * * Distance values 0...a (i.e. including 0, but excluding a) have bit length 1.
 * * Distance values a...a+bb have bit length 2.
 * * Distance values a+bb...a+bb+ccc have bit length 3.
 * * Distance values a+bb+ccc...a+bb+ccc+dddd have bit length 4.
 * * Distance values a+bb+ccc+dddd...a+bb+ccc+dddd+eeeee have bit length 5.
 * * Distance values a+bb+ccc+dddd+eeeee...a+bb+ccc+dddd+eeeee+ffffff have bit length 6.
 * * Distance values a+bb+ccc+dddd+eeeee+ffffff...a+bb+ccc+dddd+eeeee+ffffff+n7 have bit length 7.
 * * Distance values a+bb+ccc+dddd+eeeee+ffffff+n7...a+bb+ccc+dddd+eeeee+ffffff+n7+n8 (largest is 62) have bit length 8.
 *
 * This is an exmplanation in the documentation of Freeze 2.5 why the
 * maximum LZ match distance is of 7935 (== 8191 - 256) rather than 8191:
 * ``Because buffer length is 8192 bytes and maximum match length is 256
 * bytes, the position of matched string cannot be greater than 8192 - 256,
 * that's why there are only (8192-256)/2**7 = 62 static codes.'' This is
 * suboptimal, and the explanation doesn't make sense from decompressor's
 * point of view, but it does make sense if we (artificially) limit the
 * compressor's ring buffer window size to 8192 bytes.
 *
 * The *freeze* command-line tool in Freeze 2.x doesn't autodetect the best
 * Huffman table for LZ match distances, but has a hardcoded Huffman table
 * as a default, which can be changed in the command line either by
 * specifying the 8 bit lengths or specifying an entry which is looked up in
 * the config file *freeze.cnf*. This is suboptimal.
 *
 * In addition to the Huffman table for the LZ match distances, there is a
 * separate, adaptive Huffman table for tokens (which include literal bytes
 * and LZ match lengths). The token bit lengths in this adaptive Huffman
 * table are not sent, but they start from a hardcoded, evenly distributed
 * list of 315 (for Freeze 1.x) or 511 (for Freeze 2.x) bit lengths, and the
 * bit lengths are updated after each token (that's the essence of adaptive
 * Huffman coding). The adaptive Huffman format is derived from LZHUF.
 *
 * After the 2-byte signature and (only for Freeze 2.x) the 3-byte LZ match
 * distance Huffman code bit length descriptor (x and y), a stream of bits
 * follows, all the way to the end. Bits are stored big-endian (i.e.
 * most-significant bit first), i.e. the very first bit is the highest bit
 * of the very first byte (b & 0x80).
 *
 * When decoding the stream of bits, a token is decoded using the adaptive
 * Huffman table (and the table is updated accordingly). If the token value
 * is less than 256, then the token is appended as a literal byte to the
 * uncompressed output. The token value 256 indicates end-of-stream, and
 * decompression is stopped. Larger token values indicate an LZ match, and
 * they also contain the LZ match length: token - 256 + 2, thus token values
 * 257..315 mean lengths 3..60, and (for Freeze 2.x only) token values
 * 316..511 mean lengths 61..256. Freeze 1.x never generates match length
 * larger than 60, but the decompressor happily accepts larger values. After
 * tn LZ match length token, the LZ match distance is decoded using the LZ
 * match distance Huffman table (plus 6 or 7 additional literal bits, see
 * above), so the distance value is 0..4095 for Freeze 1.x and 0..7935 for
 * Freeze 2.x. The copying is done for LZ match, and decoding continues
 * with the next token.
 *
 * ``Versions 2.1--2.5 were posted to the comp.sources.misc Usenet group in 1991–1993'' in http://fileformats.archiveteam.org/wiki/Freeze/Melt . Can 2.5 be found on Usenet in 1993? Or is it 1999?
 * how does adaptive Huffman different from BSD compact -- seems simpler? is it taster?
 */

#include "luuzcat.h"

#if 0
#  define um16 unsigned long  /* Works even with this in luuzcat.h. */
#endif

#define LOOKAHEAD 256  /* pre-sence buffer size */
#define MAXDIST 7936
#define WINSIZE (MAXDIST + LOOKAHEAD)  /* Ring buffer windows size for Freeze 2. Must be a power of 2. */
#if WINSIZE > WRITE_BUFFER_SIZE
# error Write buffer too small.
#endif
#if !WRITE_BUFFER_SIZE || (WRITE_BUFFER_SIZE) & (WRITE_BUFFER_SIZE - 1)
#  error Size of write buffer is not a power of 2.  /* This is needed for `& (WRITE_BUFFER_SIZE - 1U)' below. */
#endif
#define WINMASK (WINSIZE - 1)

#define THRESHOLD 2

#define N_CHAR2 (256 - THRESHOLD + LOOKAHEAD + 1)  /* code: 0 .. N_CHARi - 1 */
#if N_CHAR2 != FREEZE_N_CHAR2  /* Same value as above, change FREEZE_N_CHAR2 in luuzcat.h if needed. */
# error Bad N_CHAR2.
#endif
#define T2 ((N_CHAR2 << 2) - 1)  /* size of table */
#if T2 != FREEZE_T2  /* Same value as above, change FREEZE_T2 in luuzcat.h if needed. */
# error Bad T2.
#endif

#define ENDOF 256  /* pseudo-literal */

/* big.freeze.dh_len is really the number of bits to read to complete literal
 * part of position information.
 *
 * big.freeze.dh_plen and big.freeze.dh_len are built from values in the header of the frozen file.
 */

#define F1 60  /* Lookahead. */
#define N1 4096  /* Ring buffer window size for Freeze 1. */
#define N_CHAR1 (256 - THRESHOLD + F1 + 1)

#define MAX_FREQ (um16)0x8000U  /* For Adaptive Huffman tree update timing. */

/* The bitbuf8 implementation is slower than Freeze 2.5.0, but it doesn't do
 * any overruns (i.e. reading more bytes than absolutely necessary).
 */

static unsigned int huffman_t, huffman_r, chars;

/* notes :
   big.freeze.th_parent[Tx .. Tx + N_CHARx - 1] used by
   indicates leaf position that corresponding to code.
*/

/* Initializes Huffman tree, bit I/O variables, etc.
   Static array is initialized with `table', dynamic Huffman tree
   has `n_char' leaves.
*/
static void StartHuff(unsigned int init_chars) {
  register unsigned int i, j;

  chars = init_chars;
  huffman_t = init_chars * 2 - 1;
  huffman_r = huffman_t - 1;
  for (i = 0; i < init_chars; i++) {  /* A priori frequences are 1. */
    big.freeze.th_freq[i] = 1;
    big.freeze.th_child[i] = i + huffman_t;
    big.freeze.th_parent[i + huffman_t] = i;
  }
  for (j = 0; i <= huffman_r; j += 2, ++i) {  /* Build the balanced tree. */
    big.freeze.th_freq[i] = big.freeze.th_freq[j] + big.freeze.th_freq[j + 1];
    big.freeze.th_child[i] = j;
    big.freeze.th_parent[j] = big.freeze.th_parent[j + 1] = i;
  }
  big.freeze.th_freq[huffman_t] = 0xffffU;
  big.freeze.th_parent[huffman_r] = 0;
}

/* Reconstructs tree with `chars' leaves. */
static void reconst(void) {
  register unsigned int i, j, k, f;
  register um16 *p, *e;

  for (i = j = 0; i < huffman_t; i++) {  /* Correct leaf node into of first half,  and set these freqency to (freq+1)/2. */
    if (big.freeze.th_child[i] >= huffman_t) {
      big.freeze.th_freq[j] = (big.freeze.th_freq[i] + 1) / 2;
      big.freeze.th_child[j] = big.freeze.th_child[i];
      j++;
    }
  }
  for (i = 0, j = chars; j < huffman_t; i += 2, j++) {  /* Build tree.  Link big.freeze.th_child[...] first. */
    k = i + 1;
    f = big.freeze.th_freq[j] = big.freeze.th_freq[i] + big.freeze.th_freq[k];
    for (k = j - 1; f < big.freeze.th_freq[k]; k--);
    k++;
    for (p = &big.freeze.th_freq[j], e = &big.freeze.th_freq[k]; p > e; p--) {
      p[0] = p[-1];
    }
    big.freeze.th_freq[k] = f;
    for (p = &big.freeze.th_child[j], e = &big.freeze.th_child[k]; p > e; p--) {
      p[0] = p[-1];
    }
    big.freeze.th_child[k] = i;
  }
  for (i = 0; i < huffman_t; i++) {  /* Link parents. */
    if ((k = big.freeze.th_child[i]) >= huffman_t) {
      big.freeze.th_parent[k] = i;
    } else {
      big.freeze.th_parent[k] = big.freeze.th_parent[k + 1] = i;
    }
  }
}

/* Updates given code's frequency, and updates tree */
static void update(register unsigned int c) {
  register um16 *p;
  register unsigned i, j, k, l;

  if (big.freeze.th_freq[huffman_r] == MAX_FREQ) {
    reconst();
  }
  c = big.freeze.th_parent[c + huffman_t];
  do {
    k = ++big.freeze.th_freq[c];

    /* swap nodes when become wrong frequency order. */
    if (k > big.freeze.th_freq[l = c + 1]) {
      for (p = big.freeze.th_freq+l+1; k > *p++; ) ;
      l = p - big.freeze.th_freq - 2;
      big.freeze.th_freq[c] = p[-2];
      p[-2] = k;

      i = big.freeze.th_child[c];
      big.freeze.th_parent[i] = l;
      if (i < huffman_t) big.freeze.th_parent[i + 1] = l;

      j = big.freeze.th_child[l];
      big.freeze.th_child[l] = i;

      big.freeze.th_parent[j] = c;
      if (j < huffman_t) big.freeze.th_parent[j + 1] = c;
      big.freeze.th_child[c] = j;

      c = l;
    }
  } while ((c = big.freeze.th_parent[c]) != 0);  /* loop until reach to root */
}

/* Decodes the literal or length info and returns its value. Returns ENDOF, if the file is corrupt. */
static unsigned int decode_token(void) {
  register unsigned int c = huffman_r;

  /* trace from root to leaf,
     got bit is 0 to small(big.freeze.th_child[]), 1 to large (big.freeze.th_child[]+1) child node */
  while ((c = big.freeze.th_child[c]) < huffman_t) {
    c += read_bit_using_bitbuf8();
  }
  update(c -= huffman_t);
  return c;
}

/* Decodes and returns the LZ match distance value, and returns it.
 * For Freeze 1.x, its high 6 bits are Huffman-coded with fixed   Huffman (0..63), and the low 6 bits are coded literally, 12 bits in total, maximum distance is ~4096.
 * For Freeze 2.x, its high 6 bits are Huffman-coded with dynamic Huffman (0..62), and the low 7 bits are coded literally, 13 bits in total, maximum distance is ~7936.
 */
static unsigned int decode_distance(um8 freeze12_code67) {
  register unsigned int i;

  i = read_bits_using_bitbuf8(8);
  return (big.freeze.dh_code[i] << freeze12_code67) | ((i << big.freeze.dh_len[i]) & (freeze12_code67 == 6 ? 0x3f : 0x7f)) | read_bits_using_bitbuf8(big.freeze.dh_len[i]);  /* Always 0 <= big.freeze.dh_len[i] <= 7. */
}

#if defined(__WATCOMC__) && defined(IS_X86_16) && (defined(_PROGX86_CSEQDS) || defined(_DOSCOMSTART) || defined(__COM__))  /* Hack to avoid alignment NUL byte before it. */
  /* Neither `#pragma data_seg' nor `__based(__segname(...))' is able to put data a byte-aligned section (segment), so we put it to _TEXT. */
  __declspec(naked) void __watcall dh_table1_func(void) { __asm { db 0, 0, 1, 3, 8, 12, 24, 16 } }  /* Luckily, the disassembly of this is self-contained for 8086. */
  #define dh_table1 ((const um8*)dh_table1_func)
#else
  static const um8 dh_table1[8] = { 0, 0, 1, 3, 8, 12, 24, 16 };  /* Sum is 64 for Freeze 1.x, and 62 for Freeze 2.x. */
#endif

/* Initializes static Huffman arrays.
 * With __WATCMC__, table index here is 1-based, just like in freeze-2.5.0/huf.c.
 */
static void build_distance_huffman(const um8 *dh_table, unsigned int freeze12_code21) {
  unsigned int i, j, k, num;
#ifndef __WATCOMC__
  --dh_table;
#endif

#if 0  /* It's shorter to pass this as an argument. */
  freeze12_code21 = (dh_table + 1 == dh_table1) ? 2 : 1;  /* 2 for Freeze 1.x, 1 for Freeze 2.x. */
#endif

  /* This is the RLE decompession of the LZ match distance Huffman table bit lengths from table[1:] to big.freeze.dh_plen[...]. There are `dh_table[i - 1]' `i'-bit Huffman codes. */
  for (i = 1, j = num = 0; i <= 8; ++i) {
    num <<= 1;
    num += dh_table[i];
    for (k = dh_table[i]; k != 0; ++j, --k)
      big.freeze.dh_plen[j] = i;  /* Values for big.freeze.dh_plen[j]: 1..8. For dh_table1 (Freeze 1.x) hardcoded: 3..8. */
  }
  if (num != 0x100) fatal_corrupted_input();  /* Invalid Huffman table. */

  /* Build the tables (big.freeze.dh_code and big.freeze.dh_plen) for LZ match distance Huffman decoding. */
  k = j = 0;
  do {
    for (i = 0x100 >> big.freeze.dh_plen[j]; (int)~0U == -1 ? (int)--i >= 0 : i-- != 0; ++k) {
      big.freeze.dh_code[k] = j;
      /* Values for big.freeze.dh_len[j] for dh_table1 (Freeze 1.x) hardcoded: : 1..6. For big.freeze.dh_table2 (Freeze 2.x), it's always 0..7.
       * 0 can be achieved with big.freeze.dh_table2 == {?, 2, 0, 0, 0, 0, 0, 0, 0}.
       * 7 can be achieved with big.freeze.dh_table2 == {?, 1, 1, 1, 1, 1, 1, 1, 2}.
       */
      big.freeze.dh_len[k] = big.freeze.dh_plen[j] - freeze12_code21;
    }
    ++j;
  } while (k != 0x100);

#if USE_DEBUG
  fprintf(stderr, "debug: big.freeze.dh_plen:");
  for (i = 0; i < num; ++i) {
    fprintf(stderr, " %u", big.freeze.dh_plen[i]);
  }
  fprintf(stderr, "\n");
  fprintf(stderr, "debug: big.freeze.dh_len:");
  for (i = 0; i < 256; ++i) {
    fprintf(stderr, " %u", big.freeze.dh_len[i]);
  }
  fprintf(stderr, "\n");
#endif
}

static void decompress_freeze_common(unsigned int match_distance_limit, um8 freeze12_code67) {
  register unsigned int match_length, write_idx, c, match_distance;

  /* Initialize the dictionary (ring buffer window) to match_distance_limit
   * number of spaces (' '). This a quirk of the Freeze format. In other
   * formats, match_distance limit is 0 here.
   */
  memset_void(global_write_buffer + WRITE_BUFFER_SIZE - match_distance_limit, ' ', match_distance_limit);
  init_bitbuf8();
  write_idx = 0;
  for (;;) {
    if ((c = decode_token()) == ENDOF) break;
    if (c < 256) {
      global_write_buffer[write_idx] = c;
      if (++write_idx == WRITE_BUFFER_SIZE) write_idx = flush_write_buffer(write_idx);
      if (match_distance_limit < 0x8000U) ++match_distance_limit;
    } else {
      match_length = c - 256 + THRESHOLD;
      /* Now 3 <= match_length <= 256, corresponding to 257 <= c <= 510. */
      /* Freeze 1.x never generates match_length > 60 here, but we don't check that. The Freeze 2.5 decompressor doesn't check it either. */
      /* Now match_length contains the LZ match length and (after the assignment in the next line match_distance contains the LZ match distance. */
      if ((match_distance = decode_distance(freeze12_code67)) >= match_distance_limit) fatal_corrupted_input();  /* LZ match refers back too much, before the first (literal) byte. */
      if ((match_distance_limit += match_length) >= 0x8000U) match_distance_limit = 0x8000U;  /* This doesn't overflow an um16, because old match_distance_limit <= 0x8000U and match_length < 0x8000U. */
      match_distance = write_idx - (match_distance + 1);  /* After this, match_distance doesn't contain the LZ match distance. */
      do {
        global_write_buffer[write_idx] = global_write_buffer[match_distance++ & (WRITE_BUFFER_SIZE - 1U)];
        if (++write_idx == WRITE_BUFFER_SIZE) write_idx = flush_write_buffer(write_idx);
      } while (--match_length != 0);
    }
  }
  flush_write_buffer(write_idx);
}

void decompress_freeze1_nohdr(void) {  /* Decompress Freeze 1.x compressed data. */
#if 0  /* The caller has already done this. */
  unsigned int i = try_byte();  /* First byte is ID1, must be 0x1f. */
  if (i == BEOF) fatal_msg("empty compressed input file" LUUZCAT_NL);  /* This is not an error for `gzip -cd'. */
  if (i != 0x1f || try_byte() != 0x9e) fatal_msg("missing Freeze 1.x signature" LUUZCAT_NL);
#endif
  StartHuff(N_CHAR1);
#ifdef __WATCOMC__
  build_distance_huffman(dh_table1 - 1, 2);
#else  /* Longer code to prvent GCC warning: array subscript is below array bounds [-Warray-bounds] */
  build_distance_huffman(dh_table1, 2);
#endif
  decompress_freeze_common(F1, 6);
}

void decompress_freeze2_nohdr(void) {  /* Decompress Freeze 1.x compressed data. */
  register unsigned int i, j;

#if 0  /* The caller has already done this. */
  i = try_byte();  /* First byte is ID1, must be 0x1f. */
  if (i == BEOF) fatal_msg("empty compressed input file" LUUZCAT_NL);  /* This is not an error for `gzip -cd'. */
  if (i != 0x1f || try_byte() != 0x9f) fatal_msg("missing Freeze 2.x signature" LUUZCAT_NL);
#endif
  /* Reconstruct `big.freeze.dh_table2' from the header of the frozen file and checks its correctness. */
  i = get_byte();
  i |= get_byte() << 8;  /* i := 16-bit little-endian header word y. */
  /* big.freeze.dh_table2[0] = 0; */  /* Unused. */
  big.freeze.dh_table2[0] = i & 1; i >>= 1;
  big.freeze.dh_table2[1] = i & 3; i >>= 2;
  big.freeze.dh_table2[2] = i & 7; i >>= 3;
  big.freeze.dh_table2[3] = i & 0xf; i >>= 4;
  big.freeze.dh_table2[4] = i & 0x1f; i >>= 5;

  if (i & 1 || (i = get_byte()) & 0xc0) fatal_corrupted_input();  /* The highest 1 bit of header word y must be 1. The 2 high bits of header byte y must be 0. */

  big.freeze.dh_table2[5] = i & 0x3f;

  i = big.freeze.dh_table2[0] + big.freeze.dh_table2[1] + big.freeze.dh_table2[2] + big.freeze.dh_table2[3] + big.freeze.dh_table2[4] + big.freeze.dh_table2[5];
  i = 62 - i;  /* After this, i is the free variable-length codes for 7 & 8 bits. !! Why only 62? How can we get up to 64? */

  j = (((((4U - big.freeze.dh_table2[0] * 2 - big.freeze.dh_table2[1]) * 2 - big.freeze.dh_table2[2]) * 2 - big.freeze.dh_table2[3]) * 2 - big.freeze.dh_table2[4]) * 2 - big.freeze.dh_table2[5]) * 4;
  /* Now j is free byte images for these codes. */

  /* Equations: big.freeze.dh_table2[7] + big.freeze.dh_table2[8] = i; 2 * big.freeze.dh_table2[7] + big.freeze.dh_table2[8] == j. */
  if (j < i) fatal_corrupted_input();
  j -= i;
  if (i < j) fatal_corrupted_input();
  big.freeze.dh_table2[6] = j;
  big.freeze.dh_table2[7] = i - j;
  StartHuff(N_CHAR2);
#ifdef __WATCOMC__
  build_distance_huffman(big.freeze.dh_table2 - 1, 1);
#else  /* Longer code to prvent GCC warning: array subscript is below array bounds [-Warray-bounds] */
  build_distance_huffman(big.freeze.dh_table2, 1);
#endif
  decompress_freeze_common(LOOKAHEAD, 7);
}
