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
 * The compressed data format is similar in design to that of LHA's "lh" compression methods.
 *
 * Codes (i.e. literal bytes and LZ match length is the same alphabet) are
 * compressed with adaptive Huffman coding. For the Freeze 2.x format, offsets
 * are decoded with the help of a static Huffman tree stored near the beginning
 * of the file. For the Freeze 1.x format, a predefined Huffman-like code is
 * used. The adaptive Huffman format is derived from LZHUF.
 *
 * Freeze is written by Leonid A. Broukhis.
 *
 * Freeze 2.1 was released on 1991-03-25, see e.g. this Usenet post on
 * comp.sources.misc:
 * http://cd.textfiles.com/sourcecode/usenet/compsrcs/misc/volume17/freeze/part01
 *
 * Freeze 2.5 was released on 1999-05-20.
 */

#include "luuzcat.h"

#if 0
#  define um16 unsigned long  /* Works even with this in luuzcat.h. */
#endif

#define LOOKAHEAD 256  /* pre-sence buffer size */
#define MAXDIST 7936
#define WINSIZE (MAXDIST + LOOKAHEAD)  /* must be a power of 2 */
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

/* big.freeze.d_len is really the number of bits to read to complete literal
 * part of position information.
 *
 * big.freeze.p_len and big.freeze.d_len are built from values in the header of the frozen file.
 */

#define F1 60  /* Lookahead. */
#define N1 4096  /* Ring buffer window size. */
#define N_CHAR1 (256 - THRESHOLD + F1 + 1)

#define MAX_FREQ (um16)0x8000U  /* For Adaptive Huffman tree update timing. */

/* This bitbuf implementation is slower than Freeze 2.5.0, but it doesn't do
 * any overruns (i.e. reading more bytes than absolutely necessary).
 */

static uc8 bitbuf;  /* Bit input buffers. Valid bits are the high bits */
static unsigned int bitlen;  /* Number of valid bits in `bitbuf'. 0 <= bitlen <= 7. */

#define InitIO() bitbuf = bitlen = 0

#undef DO_FILL_BITS  /* Decompression code below has to call FillBits() to make sure there is at least 8 bits in bitbuf to read. */

/* 0 <= bit_count <= 8. */
static unsigned int GetNBits(um8 bit_count) {
  unsigned int result;
  unsigned int tmp_bitbuf;
  /* bitlen <= 8, and the buffered bits start between bits 8 and 7, i.e.
   * they are in bitbuf >> (8 - bitlen). The lower bits are 0.
   */
  if (bitlen < bit_count) {
    tmp_bitbuf = (unsigned int)bitbuf << bitlen;
    tmp_bitbuf += get_byte();
    tmp_bitbuf <<= bit_count - bitlen;
    bitlen += 8 - bit_count;
  } else {
    tmp_bitbuf = (unsigned int)bitbuf << bit_count;
    bitlen -= bit_count;
  }
  result = tmp_bitbuf >> 8;
  bitbuf = (uc8)tmp_bitbuf;
  return result;
}

#define GetByte() GetNBits(8)

static unsigned int huffman_t, huffman_r, chars;

/* notes :
   big.freeze.parent[Tx .. Tx + N_CHARx - 1] used by
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
    big.freeze.freq[i] = 1;
    big.freeze.child[i] = i + huffman_t;
    big.freeze.parent[i + huffman_t] = i;
  }
  for (j = 0; i <= huffman_r; j += 2, ++i) {  /* Build the balanced tree. */
    big.freeze.freq[i] = big.freeze.freq[j] + big.freeze.freq[j + 1];
    big.freeze.child[i] = j;
    big.freeze.parent[j] = big.freeze.parent[j + 1] = i;
  }
  big.freeze.freq[huffman_t] = 0xffffU;
  big.freeze.parent[huffman_r] = 0;
}

/* Reconstructs tree with `chars' leaves. */
static void reconst(void) {
  register unsigned int i, j, k, f;
  register um16 *p, *e;

  for (i = j = 0; i < huffman_t; i++) {  /* Correct leaf node into of first half,  and set these freqency to (freq+1)/2. */
    if (big.freeze.child[i] >= huffman_t) {
      big.freeze.freq[j] = (big.freeze.freq[i] + 1) / 2;
      big.freeze.child[j] = big.freeze.child[i];
      j++;
    }
  }
  for (i = 0, j = chars; j < huffman_t; i += 2, j++) {  /* Build tree.  Link big.freeze.child[...] first. */
    k = i + 1;
    f = big.freeze.freq[j] = big.freeze.freq[i] + big.freeze.freq[k];
    for (k = j - 1; f < big.freeze.freq[k]; k--);
    k++;
    for (p = &big.freeze.freq[j], e = &big.freeze.freq[k]; p > e; p--) {
      p[0] = p[-1];
    }
    big.freeze.freq[k] = f;
    for (p = &big.freeze.child[j], e = &big.freeze.child[k]; p > e; p--) {
      p[0] = p[-1];
    }
    big.freeze.child[k] = i;
  }
  for (i = 0; i < huffman_t; i++) {  /* Link parents. */
    if ((k = big.freeze.child[i]) >= huffman_t) {
      big.freeze.parent[k] = i;
    } else {
      big.freeze.parent[k] = big.freeze.parent[k + 1] = i;
    }
  }
}

/* Updates given code's frequency, and updates tree */
static void update(register unsigned int c) {
  register um16 *p;
  register unsigned i, j, k, l;

  if (big.freeze.freq[huffman_r] == MAX_FREQ) {
    reconst();
  }
  c = big.freeze.parent[c + huffman_t];
  do {
    k = ++big.freeze.freq[c];

    /* swap nodes when become wrong frequency order. */
    if (k > big.freeze.freq[l = c + 1]) {
      for (p = big.freeze.freq+l+1; k > *p++; ) ;
      l = p - big.freeze.freq - 2;
      big.freeze.freq[c] = p[-2];
      p[-2] = k;

      i = big.freeze.child[c];
      big.freeze.parent[i] = l;
      if (i < huffman_t) big.freeze.parent[i + 1] = l;

      j = big.freeze.child[l];
      big.freeze.child[l] = i;

      big.freeze.parent[j] = c;
      if (j < huffman_t) big.freeze.parent[j + 1] = c;
      big.freeze.child[c] = j;

      c = l;
    }
  } while ((c = big.freeze.parent[c]) != 0);  /* loop until reach to root */
}

/* Decodes the literal or length info and returns its value. Returns ENDOF, if the file is corrupt. */
static unsigned int decode_token(void) {
  register unsigned int c = huffman_r;

#ifdef DO_FILL_BITS
  /* As far as MAX_FREQ == 32768, maximum length of a Huffman
   * code cannot exceed 23 (consider Fibonacci numbers),
   * so we don't need additional FillBits while decoding
   * if sizeof(bitbuf) == 4.
   */
  FillBits();
#endif
  /* trace from root to leaf,
     got bit is 0 to small(big.freeze.child[]), 1 to large (big.freeze.child[]+1) child node */
  while ((c = big.freeze.child[c]) < huffman_t) {
    /* c += GetBit(); */
    if (bitlen-- == 0) {
      bitbuf = get_byte();
      bitlen = 7;
    }
    if ((bitbuf & 0x80) != 0) ++c;
    bitbuf <<= 1;
#ifdef DO_FILL_BITS
    if (sizeof(bitbuf) < 3 && bitlen == 0) FillBits();
#endif
  }
  update(c -= huffman_t);
  return c;
}

/* Decodes the position info and returns it */
static unsigned int decode_distance(um8 freeze12_code67) {
  register unsigned int i;

  /* Upper 6 bits can be coded by a byte (8 bits) or less,
   * plus 7 bits literally ...
   */
#ifdef DO_FILL_BITS
  FillBits();
#endif
  /* decode upper 6 bits from the table */
  i = GetByte();

  /* get lower 7 bits literally */
#ifdef DO_FILL_BITS
  if (sizeof(bitbuf) < 3) FillBits();
#endif
  return (big.freeze.code[i] << freeze12_code67) | ((i << big.freeze.d_len[i]) & (freeze12_code67 == 6 ? 0x3f : 0x7f)) | GetNBits(big.freeze.d_len[i]);  /* Always 0 <= big.freeze.d_len[i] <= 7. */
}

#if defined(__WATCOMC__) && defined(IS_X86_16) && (defined(_PROGX86_CSEQDS) || defined(_DOSCOMSTART) || defined(__COM__))  /* Hack to avoid alignment NUL byte before it. */
  /* Neither `#pragma data_seg' nor `__based(__segname(...))' is able to put data a byte-aligned section (segment), so we put it to _TEXT. */
  __declspec(naked) void __watcall table1_func(void) { __asm { db 0, 0, 1, 3, 8, 12, 24, 16 } }  /* Luckily, the disassembly of this is self-contained for 8086. */
  #define table1 ((const um8*)table1_func)
#else
  static const um8 table1[8] = { 0, 0, 1, 3, 8, 12, 24, 16 };
#endif

/* Initializes static Huffman arrays.
 * With __WATCMC__, table index here is 1-based, just like in freeze-2.5.0/huf.c.
 */
static void init(const um8 *table, unsigned int d) {
  unsigned int i, j, k, num;
  num = 0;
#ifndef __WATCOMC__
  --table;
#endif

#if 0  /* It's shorter to pass this as an argument. */
  d = (table + 1 == table1) ? 2 : 1;  /* 2 for Freeze 1.x, 1 for Freeze 2.x. */
#endif

  /* There are `table[i]' `i'-bits Huffman codes */
  for (i = 1, j = 0; i <= 8; i++) {
    num += table[i] << (8 - i);
    for (k = table[i]; k; j++, k--)
      big.freeze.p_len[j] = i;  /* Values for big.freeze.p_len[j]: 1..8. For table1 (Freeze 1.x) hardcoded: 3..8. */
  }
  if (num != 256) fatal_corrupted_input();  /* Invalid position table. */
  num = j;

  /* Decompression: building the table for decoding */
  for (k = j = 0; j < num; j ++)
    for (i = 1 << (8 - big.freeze.p_len[j]); i--;)
      big.freeze.code[k++] = j;
  for (k = j = 0; j < num; j ++)
    for (i = 1 << (8 - big.freeze.p_len[j]); i--;)
      big.freeze.d_len[k++] =  big.freeze.p_len[j] - d;  /* Values for big.freeze.d_len[j] for table1 (Freeze 1.x) hardcoded: : 1..6. For big.freeze.table2 (Freeze 2.x), it's always 0..7. */

#if USE_DEBUG
  fprintf(stderr, "debug: big.freeze.p_len:");
  for (i = 0; i < num; ++i) {
    fprintf(stderr, " %u", big.freeze.p_len[i]);
  }
  fprintf(stderr, "\n");
  fprintf(stderr, "debug: big.freeze.d_len:");
  for (i = 0; i < 256; ++i) {
    fprintf(stderr, " %u", big.freeze.d_len[i]);
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
  memset(global_write_buffer + WRITE_BUFFER_SIZE - match_distance_limit, ' ', match_distance_limit);
  InitIO();
  write_idx = 0;
  for (;;) {
    if ((c = decode_token()) == ENDOF) break;
    if (c < 256) {
      global_write_buffer[write_idx] = c;
      if (++write_idx == WRITE_BUFFER_SIZE) write_idx = flush_write_buffer(write_idx);
      if (match_distance_limit < 0x8000U) ++match_distance_limit;
    } else {
      match_length = c - 256 + THRESHOLD;
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
  init(table1 - 1, 2);
#else  /* Longer code to prvent GCC warning: array subscript is below array bounds [-Warray-bounds] */
  init(table1, 2);
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
  /* Reconstruct `big.freeze.table2' from the header of the frozen file and checks its correctness. */
  i = get_byte();
  i |= get_byte() << 8;
  /* big.freeze.table2[0] = 0; */  /* Unused. */
  big.freeze.table2[0] = i & 1; i >>= 1;
  big.freeze.table2[1] = i & 3; i >>= 2;
  big.freeze.table2[2] = i & 7; i >>= 3;
  big.freeze.table2[3] = i & 0xF; i >>= 4;
  big.freeze.table2[4] = i & 0x1F; i >>= 5;

  if (i & 1 || (i = get_byte()) & 0xC0) fatal_corrupted_input();  /* Unknown header format. */

  big.freeze.table2[5] = i & 0x3F;

  i = big.freeze.table2[0] + big.freeze.table2[1] + big.freeze.table2[2] + big.freeze.table2[3] + big.freeze.table2[4] + big.freeze.table2[5];
  i = 62 - i;  /* After this, i is the free variable length codes for 7 & 8 bits. */

  j = (((((4U - big.freeze.table2[0] * 2 - big.freeze.table2[1]) * 2 - big.freeze.table2[2]) * 2 - big.freeze.table2[3]) * 2 - big.freeze.table2[4]) * 2 - big.freeze.table2[5]) * 4;
  /* Now is free byte images for these codes. */

  /* Equations: big.freeze.table2[7] + big.freeze.table2[8] = i; 2 * big.freeze.table2[7] + big.freeze.table2[8] == j. */
  if (j < i) fatal_corrupted_input();
  j -= i;
  if (i < j) fatal_corrupted_input();
  big.freeze.table2[6] = j;
  big.freeze.table2[7] = i - j;
  StartHuff(N_CHAR2);
#ifdef __WATCOMC__
  init(big.freeze.table2 - 1, 1);
#else  /* Longer code to prvent GCC warning: array subscript is below array bounds [-Warray-bounds] */
  init(big.freeze.table2, 1);
#endif
  decompress_freeze_common(LOOKAHEAD, 7);
}
