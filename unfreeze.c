/* based on and derived from freeze-2.5.0/{bitio.h,freeze.h,huf.h,bitio.c,decode.c,huf.c}
 * porting by pts@fazekas.hu at Sat Oct  4 03:56:40 CEST 2025
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

static unsigned int t, huffman_r, chars;

/* notes :
   big.freeze.parent[Tx .. Tx + N_CHARx - 1] used by
   indicates leaf position that corresponding to code.
*/

/* Initializes Huffman tree, bit I/O variables, etc.
   Static array is initialized with `table', dynamic Huffman tree
   has `n_char' leaves.
*/
static void StartHuff(unsigned int n_char) {
  register unsigned int i, j;
  t = n_char * 2 - 1;
  huffman_r = t - 1;
  chars = n_char;

/* A priori frequences are 1 */

  for (i = 0; i < n_char; i++) {
    big.freeze.freq[i] = 1;
    big.freeze.child[i] = i + t;
    big.freeze.parent[i + t] = i;
  }
  i = 0; j = n_char;

/* Building the balanced tree */

  while (j <= huffman_r) {
    big.freeze.freq[j] = big.freeze.freq[i] + big.freeze.freq[i + 1];
    big.freeze.child[j] = i;
    big.freeze.parent[i] = big.freeze.parent[i + 1] = j;
    i += 2; j++;
  }
  big.freeze.freq[t] = 0xffff;
  big.freeze.parent[huffman_r] = 0;
}

/* Reconstructs tree with `chars' leaves */
static void reconst(void) {
  register unsigned int i, j, k, f;

/* correct leaf node into of first half,
   and set these freqency to (freq+1)/2
*/
  j = 0;
  for (i = 0; i < t; i++) {
    if (big.freeze.child[i] >= t) {
      big.freeze.freq[j] = (big.freeze.freq[i] + 1) / 2;
      big.freeze.child[j] = big.freeze.child[i];
      j++;
    }
  }
/* Build tree.  Link big.freeze.child[...] first */

  for (i = 0, j = chars; j < t; i += 2, j++) {
    k = i + 1;
    f = big.freeze.freq[j] = big.freeze.freq[i] + big.freeze.freq[k];
    for (k = j - 1; f < big.freeze.freq[k]; k--);
    k++;
    {       register um16 *p, *e;
      for (p = &big.freeze.freq[j], e = &big.freeze.freq[k]; p > e; p--)
        p[0] = p[-1];
      big.freeze.freq[k] = f;
    }
    {       register um16 *p, *e;
      for (p = &big.freeze.child[j], e = &big.freeze.child[k]; p > e; p--)
        p[0] = p[-1];
      big.freeze.child[k] = i;
    }
  }

/* Link parents */
  for (i = 0; i < t; i++) {
    if ((k = big.freeze.child[i]) >= t) {
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
  c = big.freeze.parent[c + t];
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
      if (i < t) big.freeze.parent[i + 1] = l;

      j = big.freeze.child[l];
      big.freeze.child[l] = i;

      big.freeze.parent[j] = c;
      if (j < t) big.freeze.parent[j + 1] = c;
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
  while ((c = big.freeze.child[c]) < t) {
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
  update(c -= t);
  return c;
}

/* Decodes the position info and returns it */
static unsigned int decode_distance(ub8 is_freeze1) {
  register unsigned int i, j;

  /* Upper 6 bits can be coded by a byte (8 bits) or less,
   * plus 7 bits literally ...
   */
#ifdef DO_FILL_BITS
  FillBits();
#endif
  /* decode upper 6 bits from the table */
  i = GetByte();
  if (is_freeze1) {
    j = (big.freeze.code[i] << 6) | ((i << big.freeze.d_len[i]) & 0x3F);
  } else {
    j = (big.freeze.code[i] << 7) | ((i << big.freeze.d_len[i]) & 0x7F);
  }

  /* get lower 7 bits literally */
#ifdef DO_FILL_BITS
  if (sizeof(bitbuf) < 3) FillBits();
#endif
  return j | GetNBits(big.freeze.d_len[i]);  /* Always 0 <= big.freeze.d_len[i] <= 7. */
}

static const um8 table1[9] = { 0, 0, 0, 1, 3, 8, 12, 24, 16 };

/* Initializes static Huffman arrays */
static void init(const um8 *table) {
  unsigned int i, j, k, num, d;
  num = 0;
  d = (table == table1) ? 2 : 1;  /* 2 for Freeze 1.x, 1 for Freeze 2.x. */

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

static void decompress_freeze_common(unsigned int i, ub8 is_freeze1) {
  register unsigned int j, write_idx, c;
  for (; i < WRITE_BUFFER_SIZE; ++i) {  /* Why is this needed? Why not initialize the entire global_write_buffer? */
    global_write_buffer[i] = ' ';
  }
  InitIO();
  write_idx = 0;
  for (;;) {
    if ((c = decode_token()) == ENDOF) break;
    if (c < 256) {
      global_write_buffer[write_idx] = c;
      if (++write_idx == WRITE_BUFFER_SIZE) write_idx = flush_write_buffer(WRITE_BUFFER_SIZE);
    } else {
      i = write_idx - decode_distance(is_freeze1) - 1;
      j = c - 256 + THRESHOLD;
      do {
        global_write_buffer[write_idx] = global_write_buffer[i++ & (WRITE_BUFFER_SIZE - 1)];
        if (++write_idx == WRITE_BUFFER_SIZE) write_idx = flush_write_buffer(WRITE_BUFFER_SIZE);
      } while (--j != 0);
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
  init(table1);
  decompress_freeze_common(WRITE_BUFFER_SIZE - F1, 1);
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
  big.freeze.table2[0] = 0;  /* It looks like this is unused. */
  big.freeze.table2[1] = i & 1; i >>= 1;
  big.freeze.table2[2] = i & 3; i >>= 2;
  big.freeze.table2[3] = i & 7; i >>= 3;
  big.freeze.table2[4] = i & 0xF; i >>= 4;
  big.freeze.table2[5] = i & 0x1F; i >>= 5;

  if (i & 1 || (i = get_byte()) & 0xC0) fatal_corrupted_input();  /* Unknown header format. */

  big.freeze.table2[6] = i & 0x3F;

  i = big.freeze.table2[1] + big.freeze.table2[2] + big.freeze.table2[3] + big.freeze.table2[4] +
  big.freeze.table2[5] + big.freeze.table2[6];

  i = 62 - i;     /* free variable length codes for 7 & 8 bits */

  j = 128 * big.freeze.table2[1] + 64 * big.freeze.table2[2] + 32 * big.freeze.table2[3] +
  16 * big.freeze.table2[4] + 8 * big.freeze.table2[5] + 4 * big.freeze.table2[6];

  j = 256 - j;    /* free byte images for these codes */

  /* Equations: big.freeze.table2[7] + big.freeze.table2[8] = i; 2 * big.freeze.table2[7] + big.freeze.table2[8] == j. */
  if (j < i) fatal_corrupted_input();
  j -= i;
  if (i < j) fatal_corrupted_input();
  big.freeze.table2[7] = j;
  big.freeze.table2[8] = i - j;
  StartHuff(N_CHAR2);
  init(big.freeze.table2);
  decompress_freeze_common(WRITE_BUFFER_SIZE - LOOKAHEAD, 0);
}
