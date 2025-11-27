/* based on and derived from gzip-1.2.4/unlzh.c
 * porting and extra error handling by pts@fazekas.hu at Thu Oct  2 13:46:01 CEST 2025
 *
 * pts has added lots of extra error handling.
 *
 * gzip-1.2.4/unlzh.c can be found in https://mirror.netcologne.de/gnu/gzip/gzip-1.2.4.tar.gz
 *
 * decompress files in SCO compress -H (LZH) format.
 * gzip-1.2.4/unlzh.c is directly derived from the public domain 'ar002'
 * written by Haruhiko Okumura.
 *
 * unlzh.c,v 1.2 1993/06/24 10:59:01 jloup
 *
 * All differences between gzip-1.2.4/unlzh.c and gzip-1.14/unlzh.c have
 * been considered, and bugfixes (including memory safety fixes) have been
 * applied manually.
 */

#include "luuzcat.h"

/* io.c */
static unsigned int read_bits(um8 bit_count);

/* maketbl.c */

#define DICBIT 13    /* 12(-lh4-) or 13(-lh5-) */
#define DICSIZ (1U << DICBIT)  /* Must be a power of 2. */

#if DICSIZ > WRITE_BUFFER_SIZE
#  error Write buffer too small.
#endif
#if !WRITE_BUFFER_SIZE || (WRITE_BUFFER_SIZE) & (WRITE_BUFFER_SIZE - 1)
#  error Size of write buffer is not a power of 2.  /* This is needed for `& (WRITE_BUFFER_SIZE - 1U)' below. */
#endif

/* encode.c and decode.c */

#define MAXMATCH 256    /* formerly F (not more than 256) */
#define THRESHOLD  3    /* choose optimal value */

/* huf.c */

#define NC (255 + 1 + MAXMATCH + 2 - THRESHOLD)  /* 511. */
#if NC != SCOLZH_NC  /* Same value as above, change SCOLZH_NC in luuzcat.h if needed. */
#  error Bad SCOLZH_NC.
#endif
/* alphabet = {0, 1, 2, ..., NC - 1} */
#define CBIT 9  /* $\lfloor \log_2 NC \rfloor + 1$ */
#define CODE_BIT  16  /* codeword length */

#define NP (DICBIT + 1)  /* 14. */
#define NT (CODE_BIT + 3)  /* 19. */
#define PBIT 4  /* smallest integer suc8 that (1U << PBIT) > NP */
#define TBIT 5  /* smallest integer suc8 that (1U << TBIT) > NT */
#if NT > NP
#  define NPT NT  /* Same value as above, change SCOLZH_NC in luuzcat.h if needed. */
#else
#  define NPT NP
#endif
#if NPT != SCOLZH_NPT
#  error Bad SCOLZH_NPT.
#endif

/* --- io.c */

static um16 bitbuf16;  /* The low 16 bits (typically all bits) are valid, higher bits are 0. First bit to read is bit 15, with mask 1U << 15 == 0x8000U. */
static ub8 subbitbuf8;  /* Contains some extra bits already read (at their original position whan read) beyond the 16 bits in bitbuf16. */
static um8 subbitcount;  /* Number of valid bits in subbitbuf8: 0..7. Temporarily, within discard_bits, subbitcount can also be 8. */

#if USE_DEBUG
  static um8 max_bit_count;
#endif

/* 0 <= bit_count <= 16. */
static void discard_bits(um8 bit_count) {  /* Shift bitbuf16 bit_count bits left, read bit_count bits */
#if USE_DEBUG
  if (bit_count> max_bit_count) {
    fprintf(stderr, "MBC %u\n", max_bit_count = bit_count);  /* 16 happens early by read_bits(16), when starting a new block. */
  }
#endif
  bitbuf16 <<= bit_count;  /* If sizeof(bitbuf16) > 2, then this may make it larger than 0xffffU. */
  while (bit_count > subbitcount) {
#ifdef USE_DEBUG
    if (0) fprintf(stderr, "bitbuf16=0x%x or=0x%x result=0x%x subbitbuf8=0x%02x bit_count=%u subbitcount=%u\n", (unsigned)(bitbuf16), (unsigned)((um16)subbitbuf8 << (bit_count - subbitcount)), (unsigned)(bitbuf16 | (um16)subbitbuf8 << (bit_count - subbitcount)), subbitbuf8, (unsigned)bit_count, subbitcount);
    if (subbitbuf8 >> 8) abort();
#endif
    bitbuf16 |= (um16)subbitbuf8 << (bit_count -= subbitcount);  /* If sizeof(bitbuf16) > 2, then this may make it larger than 0xffffU, with e.g. bit_count == 10 and subbitcount == 0. */
    subbitbuf8 = get_byte();
    subbitcount = 8;
  }
  if (sizeof(bitbuf16) > 2) bitbuf16 &= 0xffffU;
  bitbuf16 |= (um16)subbitbuf8 >> (subbitcount -= bit_count);
  /* Now: 0 <= subbitcount <= 7. */
}

/* 0 <= bit_count <= 16. */
static unsigned int read_bits(um8 bit_count) {
  /* There is no easy way to unify this function with
   * read_bits_using_bitbuf8(...) and read_bit_using_bitbuf8(...) used by
   * uncompat.c, unpack.c and unfreeze.c? discard_bits(...) is complicated.
   * Also for the others we have to prevent reading too many bytes.
   */
  const unsigned int result = bitbuf16 >> (16 - bit_count);

  discard_bits(bit_count);
  return result;
}

/* --- maketbl.c */

/* table_bit_count is 8 or 12. */
static void build_huffman_table(unsigned int size, um8 bit_count_ary_ptr[], unsigned int table_bit_count, um16 table[]) {
  um16 histogram[17], weight[16], start[16];
  unsigned int total, delta;  /* Any sizeof(...) >= 2 works. um16 would also work. */
  um16 *p;
  unsigned int i, len, ch, jutbits, avail, nextcode;

  memset_void(histogram, '\0', sizeof(histogram));
#ifdef USE_DEBUG
  for (i = 0; i < size; i++) {
    if (bit_count_ary_ptr[i] > 16) abort();  /* Can be 16 but not more. */
  }
#endif
  for (i = 0; i < size; i++) ++histogram[bit_count_ary_ptr[i]];

  start[0] = total = 0;
  for (i = 1; i < 16; ++i) {
    if ((histogram[i] >> i) != 0) fatal_corrupted_input();  /* Check for no overflow in coverage. Having no check is a bug in gzip-1.2.4/unlzh.c and gzip-1.14/unlzh.c. !! Report bug for gzip 1.14? */
    if ((delta = histogram[i] << (16 - i)) != 0) {
      total += delta;
      if ((total & 0xffffU) == 0) {  /* Overflow. No need to set start[i] and beyond. */
        if (i <= table_bit_count) start[table_bit_count] = 0;
        for (++i; i < 16; ++i) {
          if (histogram[i] != 0) fatal_corrupted_input();  /* Check for no overflow in coverage. Having no check is a bug in gzip-1.2.4/unlzh.c and gzip-1.14/unlzh.c. !! Report bug for gzip 1.14? */
        }
        break;
      }
#ifdef USE_DEBUG
      if (0) fprintf(stderr, "i=%u total=0x%04x delta=0x%04x\n", i, total, delta);
#endif
      if (total < delta) fatal_corrupted_input();  /* Check for no overflow in coverage. Having no check is a bug in gzip-1.2.4/unlzh.c and gzip-1.14/unlzh.c. */
    }
    start[i] = total;
  }
#ifdef USE_DEBUG
  if (0) fprintf(stderr, "startB: %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x\n", (unsigned)start[1], (unsigned)start[2], (unsigned)start[3], (unsigned)start[4], (unsigned)start[5], (unsigned)start[6], (unsigned)start[7], (unsigned)start[8], (unsigned)start[9], (unsigned)start[10], (unsigned)start[11], (unsigned)start[12], (unsigned)start[13], (unsigned)start[14], (unsigned)start[15]);
  fprintf(stderr, "BHT total=0x%x\n", (unsigned)(total + histogram[16]));
#endif
  if (((total + histogram[16]) & 0xffffU) != 0) fatal_corrupted_input();  /* This checks for 100% coverage in the Huffman tree. It is needed to avoid uninitialized lookups. */
  jutbits = 16 - table_bit_count;
  for (i = 1; i <= table_bit_count; i++) {
    start[i - 1] >>= jutbits;
    weight[i - 1] = 1U << (table_bit_count - i);
  }
  while (i <= 16) {
    weight[i - 1] = 1U << (16 - i);
    i++;
  }

  i = start[table_bit_count] >> jutbits;
#if USE_DEBUG
  fprintf(stderr, "FILL i=0x%x nextcode=0x%x\n", i, nextcode);
#endif
  if (i != 0) {
    nextcode = 1U << table_bit_count;
    while (i != nextcode) table[i++] = 0;
  }

  avail = size;
  total = 1U << table_bit_count;
  for (ch = 0; ch < size; ch++) {
    if ((len = bit_count_ary_ptr[ch]) == 0) continue;  /* Skip value which never appears. */
    i = start[--len];
    nextcode = i + weight[len];  /* After this, nextcode >= 1, because weight[len] >= 1. */
    start[len] = nextcode;
    if (len < table_bit_count) {
      if (nextcode > total) fatal_corrupted_input();  /* Check missing from gzip-1.2.4/unlzh.c, but present in gzip-1.14/unlzh.c . */
      for (; i < nextcode; i++) table[i] = ch;
    } else {
      p = &table[i >> jutbits];
      i <<= table_bit_count;
      for (len -= table_bit_count - 1; len-- != 0; i <<= 1) {
        if (*p == 0) {
          big.scolzh.right[avail] = big.scolzh.left[avail] = 0;
          *p = avail++;
        }
        p = is_bit_15_set(i) ? &big.scolzh.right[*p] : &big.scolzh.left[*p];  /* For IS_X86_16 && defined(__WATCOMC__), this is shorter here than is_bit_15_set_func(maxbits). */
      }
      *p = ch;
    }
  }

#ifdef USE_DEBUG
  if (0) {
    fprintf(stderr, "built table[%u]:", total);
    for (ch = 0; ch < total; ++ch) {
      fprintf(stderr, " %u", (unsigned)table[ch]);
    }
    fprintf(stderr, "\n");
  }
#endif
}

/* --- huf.c: dynamic Huffman coding */

#if IS_X86_16 && defined(__WATCOMC__)
#  define I_SPECIAL_ARG_DECL , unsigned int i_special
#  define I_SPECIAL_ARGN , 3  /* i_special == -1U will never match i. */
#  define I_SPECIAL_ARG , -1U  /* i_special == -1U will never match i. */
#  define I_SPECIAL_COND (i == i_special)
#else  /* Shorter for #if defined(__386__) && defined(__WATCOMC__). */
#  define I_SPECIAL_ARG_DECL
#  define I_SPECIAL_ARGN
#  define I_SPECIAL_ARG
#  define I_SPECIAL_COND (i == 3 && size == NT)
#endif

/* Argument size is either NT == 19 or NP == 14. */
static void read_pt_len(unsigned int size, unsigned int nbit I_SPECIAL_ARG_DECL) {
  unsigned int i, n, c, mask;

  n = read_bits(nbit);  /* nbit == TBIT == 5 for size == NT == 19; nbit == PBIT == 4 for size == NP == 14. */
  if (n == 0) {
    c = read_bits(nbit);  /* Only a single possible value c. This code path is untested. */
    if (c >= size) fatal_corrupted_input();  /* Also to avoid a buffer overflow in big.scolzh.pt_len[...]. !! Report this missing check to gzip-1.14/unlzh.c. */
    big.scolzh.pt_len[c] = 0;  /* No need to set any other value in big.scolzch.c_len[...]. */
    for (i = 0; i < (1U << 8); i++) big.scolzh.pt_table[i] = c;
  } else {  /* Dynamic Huffman. */
    if (n > size) fatal_corrupted_input();  /*  Also to avoid a buffer overflow in big.scolzh.pt_len[...]. !! Report this missing check to gzip-1.14/unlzh.c. */
    for (i = 0; i < n;) {
      /* bit_count (== big.scolzh.c_len[...]) value decoding:
       * 000 -> 0; 001 -> 1; 010 --> 2; 011 -> 3; 100 -> 4; 101 -> 5; 110 -> 6;
       * 1110 -> 7; 11110 -> 8; 111110 -> 9; 1111110 -> 10; 11111110 -> 11; 111111110 -> 12; 1111111110 -> 13; 11111111110 -> 14; 111111111110 -> 15;
       * 1111111111110 -> 16; 11111111111110.
       */
      c = bitbuf16 >> (16 - 3);  /* Peek at the next 3 bits. After this, 0 <= c <= 7. */
      if (c == 7) {
        mask = 1U << (16 - 1 - 3);
        while (mask & bitbuf16) {
          mask >>= 1;
          if (++c > 16) fatal_corrupted_input();  /* Check that bit_count is at most 16. Without this check, the `bitbuf16 & mask' loop in read_c_len_using_pt(...) wouldn't work. Having no check is a bug in gzip-1.2.4/unlzh.c, but gzip-1.14/unlzh.c has the check. */
        }
      }
      /* Now: 0 <= c <= 16. */
      discard_bits((c < 7) ? 3 : c - 3);
      /* if (c >= size) fatal_corrupted_input(); */  /* No need to check this. It can be proven that if the 100% coverage check to build_huffman_table...) doesn't fail, then `c < size' is true here. */
      big.scolzh.pt_len[i++] = c;  /* When the function returns, all big.scolzh.pt_len[...] values are 0..16. big.scolzh.pt_len[i] means that value won't be present. */
      if (I_SPECIAL_COND) {
        c = read_bits(2);
        if (c > n - i) fatal_corrupted_input();  /* Check i < n. Having no check is a bug in gzip-1.2.4/unlzh.c and gzip-1.14/unlzh.c. */
        while (c-- != 0) big.scolzh.pt_len[i++] = 0;
      }
    }
    while (i < size) big.scolzh.pt_len[i++] = 0;
    build_huffman_table(size, big.scolzh.pt_len, 8, big.scolzh.pt_table);
  }
}

static unsigned int decode_huffman_using_pt(unsigned int limit) {
  unsigned int c = big.scolzh.pt_table[bitbuf16 >> (16 - 8)];  /* Peek at the next 8 bits. */
  unsigned int mask;

  if (c >= limit) {
    mask = 1U << (16 - 1 - 8);
    do {  /* Reads up to limit - 1 - 8 (== 10; or 5) extra bits. */
      c = (bitbuf16 & mask) ? big.scolzh.right[c] : big.scolzh.left[c];
      mask >>= 1;
    } while (c >= limit);
  }
  discard_bits(big.scolzh.pt_len[c]);  /* This works only if big.scolzh.pt_len[c] <= 16, which is ensured by read_pt_len(...) above. */
  return c; /* Now 0 <= c < limit. */
}

static void read_c_len_using_pt(void) {
  unsigned int i, c, n;

  n = read_bits(CBIT);  /* CBIT == 9. */
  if (n == 0) {
    c = read_bits(CBIT);  /* Only a single possible value c. This code path is untested. */
    if (c >= NC) fatal_corrupted_input();  /* NC == 511. Also to avoid a buffer overflow in big.scolzh.pt_len[...]. !! Report this missing check to gzip-1.14/unlzh.c. */
    big.scolzh.c_len[c] = 0;  /* No need to set any other value in big.scolzch.c_len[...]. */
    for (i = 0; i < (1U << 12); i++) big.scolzh.c_table[i] = c;
  } else {  /* Dynamic Huffman with RLE-compressed 0 bit length values. */
    if ((1U << CBIT) > (NC + 1) && n > NC) fatal_corrupted_input();  /* NC == 511. Also to avoid a buffer overflow in big.scolzh.pt_len[...]. The check is optimized away, because the condition is never true, becaue n == read_bits(9) <= 511 == NC. */
    for (i = 0; i < n;) {
      c = decode_huffman_using_pt(NT);  /* NT == 19. After this, 0 <= c <= 18 == NT - 1. */
      /* Now: 0 <= c <= NT - 1 == 18. That's because of how the pt Huffman tree was constructed before this call to read_c_len_using_pt(...). */
      /* The maximum c value in the test input file is 16. */
      if (c <= 2) {
        c = (c == 0) ? 1 : (c == 1) ? read_bits(4) + 3 : read_bits(CBIT) + 20;
        if (c > n - i) fatal_corrupted_input();  /* Check i < n. Having no check is a bug in gzip-1.2.4/unlzh.c and gzip-1.14/unlzh.c. */
        while (c-- != 0) big.scolzh.c_len[i++] = 0;
      } else {
        big.scolzh.c_len[i++] = c - 2;  /* The value is 0..16: 0 <= c - 2 < = 16. */
      }
    }
    while (i < NC) big.scolzh.c_len[i++] = 0;
    build_huffman_table(NC, big.scolzh.c_len, 12, big.scolzh.c_table);
  }
}

/* --- decode.c */

void decompress_scolzh_nohdr(void) {
  unsigned int write_idx, match_distance, match_length, c, mask, block_size;

#if 0  /* The caller has already done this. */
  i = try_byte();
  if (i == BEOF) fatal_msg("empty compressed input file" LUUZCAT_NL);  /* This is not an error for `gzip -cd'. */
  if (i != 0x1f || try_byte() != 0xa0) fatal_msg("missing scolzh signature" LUUZCAT_NL);
#endif
  subbitbuf8 = subbitcount = 0;
  /* Prefill bitbuf16 with 16 bits. From then on, keep it fully prefilled. */
  bitbuf16 = get_byte() << 8; bitbuf16 |= get_byte();
  write_idx = 0;  /* Assumes LUUZCAT_WRITE_BUFFER_IS_EMPTY_AT_START_OF_DECOMPRESS. */
  while ((block_size = bitbuf16) != 0) {
    discard_bits(16);  /* Discard 16 bits for the block size. */
    read_pt_len(NT, TBIT I_SPECIAL_ARGN);  /* NT == 19, thus this makes bit lengths and decoded values in the big.scolzh.pt_* Huffman tree 0..18. */
    read_c_len_using_pt();  /* This makes bit lengths in the big.scolzh.c_* Huffman tree 0..16, and the values 0..510. */
    read_pt_len(NP, PBIT I_SPECIAL_ARG);  /* NP == 14, thus this makes bit lengths and decoded values in the big.scolzh.pt_* Huffman tree 0..13. */
    while (block_size-- != 0) {
      c = big.scolzh.c_table[bitbuf16 >> (16 - 12)];  /* Peek at the next 12 bits. */
#if USE_DEBUG
      if (0) fprintf(stderr, "c from table: 0x%x bits12=0x%x\n", c, (unsigned)(bitbuf16 >> (16 - 12)));
#endif
      if (c >= NC) {
        mask = 1U << (16 - 1 - 12);
        do {  /* Reads up to 4 extra bits. */
          c = (bitbuf16 & mask) ? big.scolzh.right[c] : big.scolzh.left[c];
#if USE_DEBUG
          if (0) fprintf(stderr, "c next: 0x%x\n",c);
#endif
          mask >>= 1;
        } while (c >= NC);
      }
      discard_bits(big.scolzh.c_len[c]);  /* This may discard fewer bits than 12, actually all 3..12 happened. */
      if (c <= 255) {  /* Append LZ literal byte. */
        write_byte_using_write_idx(c);
        increment_lz_match_distance_limit();
      } else {
        match_length = c - 256 + THRESHOLD;
        /* Now decode the LZ match distance to i. */
        match_distance = decode_huffman_using_pt(NP);  /* NP == 14. */  /* After this, 0 <= match_distane <= 13. */
        if (match_distance != 0) match_distance = (1U << (match_distance - 1)) + read_bits(match_distance - 1);
        /* Now match_length contains the LZ match length and match_distance contains the LZ match distance. */
        write_idx = copy_and_write_lz_match(match_length, match_distance, write_idx);
      }
    }
  }
  /* Now: bitbuf16 is empty, and subbitbuf8 contains 0..7 bits of the last, partially processed byte. All subsequent bytes are in global_read_buffer. */
}
