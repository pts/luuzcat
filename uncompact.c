/* based on and derived from 4.4BSD/usr/src/old/compact/uncompact/{uncompact.c,tree.c,compact.h}
 * further changes by pts@fazekas.hu at Wed Oct  1 02:58:37 CEST 2025
 *
 * Base source files found in usr/src/old/compact downloaded from
 * https://www.tuhs.org/Archive/Distributions/UCB/4.4BSD-Alpha/src.tar.gz
 * The file uncompact.c has last modification date 1987-12-21.
 *
 * Direct links to base source files:
 *
 * * https://minnie.tuhs.org/cgi-bin/utree.pl?file=4.4BSD/usr/src/old/compact/common_source/compact.h
 * * https://minnie.tuhs.org/cgi-bin/utree.pl?file=4.4BSD/usr/src/old/compact/common_source/tree.c
 * * https://minnie.tuhs.org/cgi-bin/utree.pl?file=4.4BSD/usr/src/old/compact/uncompact/uncompact.c
 *
 * This is an adaptive Huffman decompressor.
 *
 * Info from the 4.4BSD/usr/src/old/compact/uncompact/uncompact.c: on-line
 * algorithm (the compressed input file doesn't contain the decoding tree).
 * Written by Colin L. McMaster (UCB) February 14, 1979
 */

#include "luuzcat.h"

#define LLEAF   8  /* Any power of 2 is OK which is larger than FBIT and SEEN and is the double of RLEAF. */
#define RLEAF   4  /* Any power of 2 is OK which is larger than FBIT and SEEN and is the half of LLEAF. */
#define SEEN    2
#define FBIT    1

#define EF      256  /* End of stream. */
#define NC      257  /* New code found. */

#define NF      (NC+1)  /* 0402 */
#if NF != COMPACT_NF
#  error Bad COMPACT_NF.
#endif
#define NFNULL NF  /* NULL value for struct compact_fpoint::fpdni. */
#define RINULL (NF << 1)

/* !! Test with `unsigned long' instead of `unsigned short' for um16. */
typedef compact_dicti_t dicti_t;  /* An index in big.compact.dict[...] or NULL (indicated by NFNULL == NF). 0 <= value <= NF. */
typedef um16 ini_t;  /* An index in big.compact.in[...]. 0 <= value < NF. */
typedef compact_dictini_t dictini_t;  /* An index in big.compact.dict[...] or big.compact.in[...]. 0 <= value <= NF. */
typedef compact_diri_t diri_t;  /* An index in big.compact.dir[...] or NULL (indicated by RINULL). 0 <= value <= (NF << 1). */
typedef compact_flags_t flags_t;  /* Bitset of FBIT, SEEN, LLEAF, RLEAD. */

/* flistri is the start index of the free list within dir. */
static diri_t headri, flistri, dirpri, dirqri;  /* Zero-initialization not needed, these could be left uninitialized, they will be initialized in main(...). */

#ifdef USE_DEBUG
  static diri_t check_ri(diri_t ri) {
    if (ri >= (NF << 1)) abort();  /* Fails on RINULL. */
    return ri;
  }
#  define CHECK_RI(ri) (check_ri(ri))  /* Fails on RINULL. */
#else
#  define CHECK_RI(ri) (ri)
#endif

#define LEFT    0  /* Index for struct compact_node::sons. */
#define RIGHT   1  /* Index for struct compact_node::sons. */

static dicti_t bottomdi;

#ifdef USE_DEBUG
  static dicti_t check_di(dicti_t di) {
    if (di >= NF) abort();  /* Fails on NFNULL. */
    return di;
  }
  static ini_t check_ii(ini_t ii) {
    if (ii >= NF) abort();
    return ii;
  }
#  define CHECK_DI(di) (check_di(di))  /* Fails on NFNULL. */
#  define CHECK_II(ii) (check_ii(ii))
#else
#  define CHECK_DI(di) (di)
#  define CHECK_II(ii) (ii)
#endif

/* 0 <= ch <= 255. */
static void insert(unsigned int ch) {
  dicti_t ppdi;
  register struct compact_son *pson, *bson;
  register diri_t wtri;
  unsigned int insert_word;  /* An um16 is enough. */

#ifdef USE_DEBUG
  if (ch > 255U) abort();
#endif

  wtri = flistri; flistri = big.compact.dir[CHECK_RI(flistri)].nextri;
  ppdi = CHECK_DI(bottomdi);
  bottomdi++;
  pson = &big.compact.dict[ppdi].sons[RIGHT];
  bson = &big.compact.dict[bottomdi].sons[LEFT];
  big.compact.dict[bottomdi].fath.fpdni = CHECK_DI(ppdi);
  big.compact.in[ch].flags = SEEN | FBIT;
  insert_word = bson->spdii = pson->spdii;  /* Sets the ini_t of spdii. */
  big.compact.in[ch].fpdni = big.compact.in[insert_word].fpdni = CHECK_DI(bottomdi);
  big.compact.dir[CHECK_RI(wtri)].ipt = CHECK_DI(bottomdi);
  pson->spdii = CHECK_DI(bottomdi);  /* Sets the dicti_t of spdii. */
  big.compact.dict[bottomdi].fath.flags = LLEAF | RLEAF | FBIT;
  big.compact.dict[ppdi].fath.flags &= ~RLEAF;
  big.compact.in[insert_word].flags = SEEN;

  bson->count = pson->count;
  bson->topri = pson->topri;
  bson++;
  bson->spdii = ch;  /* Sets the ini_t of spdii. */
  bson->count = 0;
  bson->topri = wtri;
  big.compact.dir[CHECK_RI(pson->topri)].nextri = wtri;
  big.compact.dir[CHECK_RI(wtri)].nextri = RINULL;
}

static void exch(dicti_t vdi, int vs, dictini_t dii, dicti_t wdi, int ws) {
  struct compact_fpoint *fpp;

  if (big.compact.dict[vdi].fath.flags & (vs ? RLEAF : LLEAF)) {
    fpp = &big.compact.in[CHECK_II(dii)];
  } else {
    fpp = &big.compact.dict[CHECK_DI(dii)].fath;
  }
  fpp->fpdni = CHECK_DI(wdi);
  fpp->flags &= ~FBIT;
  fpp->flags |= ws;
}

/* 0 <= ch <= 255 || ch == NC (== 257). */
static void uptree(unsigned int ch) {
  int rs, ts, rflags, tflags;
  um32 rc, qc, sc;
  dicti_t rdi, tdi;
  register struct compact_son *rson, *tson;
  register diri_t rtri, qtri, stri;
  dictini_t q, s;
  struct compact_node *rdip;

#ifdef USE_DEBUG
  if (ch > 255 && ch != NC) abort();
#endif

  rdi = CHECK_DI(big.compact.in[ch].fpdni);
  rs = big.compact.in[ch].flags & FBIT;

  for (;;) {
    rdip = &big.compact.dict[rdi];  /* We cache the &big.compact.dict[rdi] value to avoid the multiplication by 20 upon each use. */
    rson = &rdip[0].sons[rs];
    if (((rc = ++rson->count) & 0xffffffffUL) == 0U) { counter_overflow: fatal_corrupted_input(); }  /* A 32-bit counter has overflowed. */
    rtri = rson->topri;
    for (;;) {
      rdip = &big.compact.dict[rdi];
      if (rs) {
        s = CHECK_DI(rdi + 1);
        if (rdi == CHECK_DI(bottomdi)) {
          sc = rc - 2;
          stri = RINULL;
        } else {
          sc = rdip[1].sons[LEFT].count;
          stri = rdip[1].sons[LEFT].topri;
        }
        qc = rdip[0].sons[LEFT].count;
        qtri = rdip[0].sons[LEFT].topri;
      } else {
        s = CHECK_DI(rdi);
        sc = rdip[0].sons[RIGHT].count;
        stri = rdip[0].sons[RIGHT].topri;
        if (rdi == 0) {
          qc = rc + 1;
          qtri = headri;
          break;
        } else {
          qc = rdip[-1].sons[RIGHT].count;
          qtri = rdip[-1].sons[RIGHT].topri;
        }
      }
      if (rc <= qc)
        break;

      tdi = big.compact.dir[CHECK_RI(qtri)].ipt;
      tson = &big.compact.dict[tdi].sons[LEFT];
      ts = 0;
      if (rc <= tson->count) {
        ++tson;
        ++ts;  /* This requires that FBIT == 1. */
      }

      /* exchange pointers of (t, ts) and (r, rs) */
      q = tson->spdii;
      s = rson->spdii;
      tson->spdii = s;
      rson->spdii = q;
      exch(tdi, ts, q, rdi, rs);
      exch(rdi, rs, s, tdi, ts);

      rflags = (rs ? RLEAF : LLEAF);
      tflags = (ts ? RLEAF : LLEAF);
      /* This requires that LLEAF == (RLEAF << 1) and that FBIT == 1. */
      if (((rdip[0].fath.flags & rflags) << rs) ^ ((big.compact.dict[tdi].fath.flags & tflags) << ts)) {
        rdip[+ 0].fath.flags ^= rflags;
        big.compact.dict[tdi].fath.flags ^= tflags;
      }

      if ((++tson->count & 0xffffffffUL) == 0U) goto counter_overflow;
      rson->count--;
      if (ts) big.compact.dir[CHECK_RI(qtri)].ipt++;
      rdi = tdi;
      rs = ts;
      rson = tson;
    }

    if (rc == qc) {
      rson->topri = qtri;
      if (rc > sc + 1) {
        big.compact.dir[CHECK_RI(qtri)].nextri = stri;
        /* dispose of rtri */
        big.compact.dir[CHECK_RI(rtri)].nextri = flistri;
        flistri = rtri;
      } else {
        big.compact.dir[CHECK_RI(stri)].ipt = CHECK_DI(s);
      }
    } else if (rc == sc + 1) {
      /* create new index at rtri */
      rtri = flistri; flistri = big.compact.dir[CHECK_RI(flistri)].nextri;
      big.compact.dir[CHECK_RI(rtri)].nextri = stri;
      big.compact.dir[CHECK_RI(rtri)].ipt = rdi;
      big.compact.dir[CHECK_RI(qtri)].nextri = rtri;
      if (stri != RINULL) big.compact.dir[CHECK_RI(stri)].ipt = CHECK_DI(s);
      rson->topri = rtri;
    }
    rdip = &big.compact.dict[rdi];
    rs = rdip->fath.flags & FBIT;
    if ((rdi = rdip->fath.fpdni) == NFNULL) break;
    rdi = CHECK_DI(rdi);  /* Check fails on NFNULL. */
  }
  dirpri = big.compact.dir[CHECK_RI(headri)].nextri;
  dirqri = big.compact.dir[CHECK_RI(dirpri)].nextri;
}

/* typedef char assert_son_size[sizeof(big.compact.dict[0].sons[0]) == 8 ? 1 : -1]; */  /* True, but not important. */
/* typedef char assert_dict0_size[sizeof(big.compact.dict[0]) == 20 ? 1 : -1]; */  /* True, but not important. */

static ub8 read_bit(void) {  /* For decompress. */
  register unsigned int bb = big.compact.bitbuf;
  if (is_bit_15_set(bb)) {  /* For IS_X86_16 && defined(__WATCOMC__), this is shorter here than is_bit_15_set_func(bb). */
    bb = add_set_higher_byte_1(get_byte());
  } else {
    bb <<= 1;
  }
  return is_bit_7_set_func(big.compact.bitbuf = bb);  /* For IS_X86_16 && defined(__WATCOMC__), this is shorter here than is_bit_7_set(bb). */
}

#if IS_X86_16 && defined(__WATCOMC__) && 0  /* This works and looks smart, but it makes the code longer. */
  static unsigned int read_8_bits(void);
#  pragma aux read_8_bits = "push bx"  "mov bx, 0xff00"  "next: add bx, bx"  "call read_bit"  "cbw"  "add bx, ax"  "js next"  "xchg ax, bx"  "pop bx"  __value [__ax] __modify __exact []
#  define IS_READ_8_BITS_FUNCTION 1
#else
#  define IS_READ_8_BITS_FUNCTION 0
#endif

/* !! Test this by calling it twice, for different input streams. */
void decompress_compact_nohdr(void) {
  unsigned int write_idx;
  register dicti_t pdi;  /* For decompress. */
  ub8 b;  /* 0 or 1 -- or RLEAF or LLEAF. For decompress. */
  unsigned int decompress_word;  /* Must be at least 16 bits. */
  um8 data_byte;  /* 0..255. */

  /* !! Do we need this initialization? How is it different from per-file initialization below? */
  big.compact.dir[513].nextri = RINULL;
  for (headri = 513; headri--; ) {
    big.compact.dir[CHECK_RI(headri)].nextri = headri + 1;
  }
  headri = 0;
  dirpri = 1;
  big.compact.dir[CHECK_RI(dirpri)].ipt = bottomdi = 0;
  big.compact.dict[0].sons[LEFT].topri = big.compact.dict[0].sons[RIGHT].topri = 1  /* dirpri */;
#if 1  /* Shorter code. */
  dirqri = 2;
#else
  dirqri = big.compact.dir[1  /* CHECK_RI(dirpri) */].nextri;
#endif
  big.compact.in[EF].flags = FBIT | SEEN;

  big.compact.dir[CHECK_RI(big.compact.dict[0  /* bottomdi */].sons[RIGHT].topri)].nextri = RINULL;
  big.compact.dict[0  /* bottomdi */].sons[RIGHT].topri = 1  /* dirpri */;
#if 0  /* Not needed now, it will be overwritten later. */
  flistri = 2  /* dirqri */;
#endif

  /* Setup. */
  data_byte = get_byte();
  write_idx = 0;
#if 0  /* Not needed, write_idx is always 0 here (LUUZCAT_WRITE_BUFFER_IS_EMPTY_AT_START_OF_DECOMPRESS). */
  if (write_idx == WRITE_BUFFER_SIZE) write_idx = flush_write_buffer(WRITE_BUFFER_SIZE);
#endif
  global_write_buffer[write_idx++] = data_byte;

  /* !! This is per-file initialization basd on the first byte read from data_byte. !! What does this first byte mean? Do we need to special-case it? */
  big.compact.dict[0].sons[LEFT].spdii = bottomdi = 1;  /* Sets the dicti_t of spdii. */
  big.compact.in[NC].fpdni = big.compact.in[EF].fpdni = CHECK_DI(1  /* bottomdi */);
  big.compact.dict[1  /* bottomdi */].sons[LEFT].count = big.compact.dict[1  /* bottomdi */].sons[RIGHT].count = big.compact.dict[0].sons[RIGHT].count = 1;
  big.compact.dict[0].sons[RIGHT].topri = big.compact.dict[1  /* bottomdi */].sons[LEFT].topri = big.compact.dict[1  /* bottomdi */].sons[RIGHT].topri = big.compact.dir[CHECK_RI(dirpri)].nextri = 2;  /* flistri */;
#if 0  /* Not needed, it's already 2. */
  dirqri = 2  /* flistri */;
#endif
#if 1  /* Shorter code. */
  flistri = 3;
#else
  flistri = big.compact.dir[CHECK_RI(2  /* flistri */)].nextri;
#endif
  big.compact.dir[CHECK_RI(2  /* dirqri */)].nextri = RINULL;
  big.compact.dict[0].fath.fpdni = NFNULL;
  big.compact.dict[1  /* bottomdi */].fath.fpdni = big.compact.in[data_byte].fpdni = 0;
  big.compact.dir[CHECK_RI(2  /* dirqri */)].ipt = 0;
  big.compact.in[data_byte].flags = FBIT | SEEN;
  big.compact.in[NC].flags = SEEN;
  big.compact.dict[0].fath.flags = RLEAF;
  big.compact.dict[1  /* bottomdi */].fath.flags = LLEAF | RLEAF;
  big.compact.dict[0].sons[LEFT].count = 2;
  big.compact.dict[0].sons[RIGHT].spdii = data_byte;  /* Sets the ini_t of spdii. */
  big.compact.dict[1  /* bottomdi */].sons[LEFT].spdii = NC;  /* Sets the ini_t of spdii. */
  big.compact.dict[1  /* bottomdi */].sons[RIGHT].spdii = EF;  /* Sets the ini_t of spdii. */

  /* Decompress subsequent data bytes using adaptive Huffman code. */
  for (pdi = 0, big.compact.bitbuf = ~(um16)0; ;) {
    b = read_bit();
    decompress_word = CHECK_DI(big.compact.dict[pdi].sons[b].spdii);  /* b is 0 or 1. Uses the ini_t of spdii. */
    if (big.compact.dict[pdi].fath.flags & (b ? RLEAF : LLEAF))  {
      if (decompress_word == EF) break;
      if (decompress_word == NC) {
        uptree(NC);
#if IS_READ_8_BITS_FUNCTION
        insert(decompress_word = read_8_bits());
#else
        decompress_word = 0xffU << ((sizeof(decompress_word) * 8 - 8));  /* Set the 8 highest bits to 1. */
        while (is_high_bit_set(decompress_word)) {  /* Read 8 bits to decompress_word. */
          decompress_word <<= 1;
          if (read_bit()) decompress_word++;
        }
        insert(decompress_word);
#endif
      }
      uptree(decompress_word);
      global_write_buffer[write_idx] = (uc8)decompress_word;
      if (++write_idx == WRITE_BUFFER_SIZE) write_idx = flush_write_buffer(write_idx);
      pdi = 0;
    } else {
      pdi = decompress_word;
    }
  }
  flush_write_buffer(write_idx);
}
