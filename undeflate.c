/* written by pts@fazekas.hu at Fri Sep 26 00:32:20 CEST 2025
 *
 * This files has decompressors for the following Deflate-based file formats:
 *
 * * [raw Deflate](https://www.rfc-editor.org/rfc/rfc1951.txt) and
 * * [gzip](https://www.rfc-editor.org/rfc/rfc1952.txt) and the format used by gzip(1) (which is slightly different in the header fields)
 * * [4.3BSD-Quasijarus strong compression](http://fileformats.archiveteam.org/wiki/Quasijarus_Strong_Compression)
 * * [zlib](https://www.rfc-editor.org/rfc/rfc1950.txt)
 *
 * This implementation contains lots of error handling. It is believed that
 * it is memory-safe and it detects incorrect input, and fails with an error
 * message immediately.
 *
 * Deflate is Huffman+LZ, with a 32 KiB ring buffer window, an ability to
 * restart the Huffman codes (i.e. send new codes), and space-efficient
 * framing for large uncompressed blocks.
 *
 * !! Add command-line flag -r for decompressing raw Deflate.
 * !! Add decompression of ZIP archives, with concatenation of members to stdout.
 */

#include "luuzcat.h"

/* CRC-32 uses polynomial
 * x^32+x^26+x^23+x^22+x^16+x^12+x^11+x^10+x^8+x^7+x^5+x^4+x^2+x+1 over
 * GF(2). The corresponding 32-bit CRC32_GEN_VALUE is (taking the terms from
 * right to left, ignoring x^32 is 0b11101101101110001000001100100000 ==
 * 0xedb88320.
 */
#define CRC32_GEN_VALUE_LOW 0x8320U
#define CRC32_GEN_VALUE_HIGH 0xedb8U

#if IS_DOS_16  /* Otherwise data (DGROUP) too large, it doesn't fit 64 KiB anymore. */
#  define crc32_table big.deflate.crc32_table  /* !!! reinit */
#else
  static um32 crc32_table[256];  /* By putting it outside `big', we can keep it across different decompressor calls. */
#endif

#if defined(__WATCOMC__) && defined(_M_I86) && (defined(__SMALL__) || defined(__MEDIUM__))  /* Optimized assembly implementation. */
static void build_crc32_table_if_needed_inline(um32 __near *our_crc32_table);
#  pragma aux build_crc32_table_if_needed_inline = \
      "xor bx, bx"  "cmp byte ptr [di+4], bl" \
      "jne done"  "next_byte: mov ax, bx"  "cwd"  "mov cx, 8" \
      "next_bit: shr dx, 1"  "rcr ax, 1"  "jnc updated"  "xor ax, 8320h" \
      "xor dx, 0edb8h"  "updated: loop next_bit"  "stosw"  "xchg ax, dx" \
      "stosw"  "inc bl"  "jnz next_byte"  "done:" \
      __parm [__di] __modify __exact [__ax __bx __cx __dx __di]
  static void build_crc32_table_if_needed(void) { build_crc32_table_if_needed_inline(crc32_table); }
#else
  static void build_crc32_table_if_needed(void) {
    unsigned int u, biti;
    um32 value;

    /* Return if already built. This is also correct for IS_DOS_16 (with
     * big.deflate.crc32_table): the caller has set it to zero when it
     * called a non-Deflate decompressor.
     */
    if ((um8)crc32_table[1] != 0) return;
    u = 255;
    do {
      biti = 8; value = u;
      do {
        if ((value & 1) != 0) {
          value >>= 1; value ^= CRC32_GEN_VALUE_LOW | ((um32)CRC32_GEN_VALUE_HIGH << 16);
        } else {
          value >>= 1;
        }
      } while (--biti != 0);
      crc32_table[u] = value;
    } while (u-- != 0);
  }
#endif

static union single_checksum {
  um32 crc32;
  unsigned int adler32_s12[2];  /* [0] is s2, [1] is s1. */
} global_checksum;
static um32 global_usize;

#define CRC32_INITIAL ((um32)0xffffffffUL)
#define crc32_init() do { global_checksum.crc32 = CRC32_INITIAL; } while (0)

static unsigned int flush_write_buffer_and_crc32_usize(unsigned int size) {
  const uc8 *p = global_write_buffer;
  um32 crc32_value = global_checksum.crc32;
  global_usize += size;

  while (size-- != 0) {
    crc32_value = crc32_table[(uc8)crc32_value ^ *p++] ^ (crc32_value >> 8);
  }
  global_checksum.crc32 = crc32_value;
  return flush_write_buffer(p - global_write_buffer);
}

#define ADLER32_INITIAL_S1 1U
#define ADLER32_INITIAL_S2 0U
#define adler32_init() do { global_checksum.adler32_s12[1] = ADLER32_INITIAL_S1; global_checksum.adler32_s12[0] = ADLER32_INITIAL_S2; } while (0)

static unsigned int flush_write_buffer_and_adler32(unsigned int size) {  /* https://www.rfc-editor.org/rfc/rfc1950.txt */
  const uc8 *p = global_write_buffer;
  unsigned int s1 = global_checksum.adler32_s12[1], s2 = global_checksum.adler32_s12[0], byte_value;

  if (sizeof(s1) > 2) {
    byte_value = 0;  /* Avoid warning about possible use by (void)byte_value below. */
    (void)byte_value;
  }
  while (size-- != 0) {
    if (sizeof(s1) > 2) {
      if ((s1 += *p++) >= 65521U) s1 -= 65521U;
      if ((s2 += s1) >= 65521U) s2 -= 65521U;
    } else {  /* Handle overflow manually, without using longer than 16-bit integers. */
      byte_value = *p++;  /* Also zero-entends it to an unsigned int. */
      if ((s1 += byte_value) >= 65521U || s1 < byte_value) s1 -= 65521U;
      if ((s2 += s1) >= 65521U || s2 < s1) s2 -= 65521U;
    }
  }
  global_checksum.adler32_s12[1] = s1; global_checksum.adler32_s12[0] = s2;
  return flush_write_buffer(p - global_write_buffer);
}

unsigned int (*global_flush_write_buffer_func)(unsigned int size);

#define MAX_TMP_SIZE 19
#define MAX_INCOMPLETE_BRANCH_COUNT (7 + 15 + 15)  /* Because of DO_CHECK_HUFFMAN_TREE_100_PERCENT_COVERAGE == 1. */
#define MAX_LITERAL_AND_LEN_SIZE (0x100 + 32)
#define MAX_DISTANCE_SIZE 30
#define MAX_TREE_SIZE ((6U - (3 << 2)) + ((MAX_TMP_SIZE + MAX_LITERAL_AND_LEN_SIZE + MAX_DISTANCE_SIZE) << 1 << 1) + (MAX_INCOMPLETE_BRANCH_COUNT << 2))  /* <<1 is for the max node_count/leaf_node_count for a tree. <<1 is for node_count/element_count. */
#define BIT_COUNT_ARY_SIZE 0x100 + 32 + 30  /* [288]. The tokens 286 and 287 are placeholders needed by correct fixed Huffman codes. */

#if MAX_TREE_SIZE != DEFLATE_MAX_TREE_SIZE  /* Same value as above, change DEFLATE_MAX_TREE_SIZE in luuzcat.h if needed. */
#  error Bad DEFLATE_MAX_TREE_SIZE.
#endif
#if BIT_COUNT_ARY_SIZE != DEFLATE_BIT_COUNT_ARY_SIZE  /* Same value as above, change DEFLATE_BIT_COUNT_ARY_SIZE in luuzcat.h if needed. */
#  error Bad DEFLATE_BIT_COUNT_ARY_SIZE.
#endif

#define TREE_TMP_ROOT_IDX 0
#define TREE_LITERAL_AND_LEN_ROOT_IDX 2
#define TREE_DISTANCE_ROOT_IDX 4
#define TREE_FIRST_FREE_IDX 6
#if USE_NZTREELEAF
#  define LEAF_IDX 0xfaceU  /* Can be 0 or someting large (e.g. 0xffffU or a bit less). */
#else
#  define LEAF_IDX 0U
#endif
#define BAD_LEAF_VALUE ((um16)-1)

static um8 global_bitbuf_bits_remaining;
static uc8 global_bitbuf;
static unsigned int global_tree_free_idx;

/* 0 <= bit_count <= 8. */
static unsigned int read_bits_max_8(um8 bit_count) {
  unsigned int result;
  if (global_bitbuf_bits_remaining < bit_count) {
    result = global_bitbuf;
    global_bitbuf = get_byte();
    bit_count -= global_bitbuf_bits_remaining;
    result |= (global_bitbuf & ((1U << bit_count) - 1)) << global_bitbuf_bits_remaining;
    global_bitbuf_bits_remaining = 8 - bit_count;
    global_bitbuf >>= bit_count;
  } else {
    result = global_bitbuf & ((1U << bit_count) - 1);
    global_bitbuf >>= bit_count;
    global_bitbuf_bits_remaining -= bit_count;
  }
  return result;
}

/* 0 <= bit_count <= 16. */
static unsigned int read_bits_max_16(um8 bit_count) {
  unsigned int result;
  if (bit_count > 8) {
    result = read_bits_max_8(8);
    result += read_bits_max_8(bit_count - 8) << 8;
  } else {
    result = read_bits_max_8(bit_count);
  }
  return result;
}

static um16 read_using_huffman_tree(unsigned int node_idx) {
  while (big.deflate.huffman_trees_ary[node_idx] != LEAF_IDX) {
    if (global_bitbuf_bits_remaining-- == 0) {
      global_bitbuf = get_byte();
      global_bitbuf_bits_remaining = 7;
    }
    if ((global_bitbuf & 1) != 0) ++node_idx;
    global_bitbuf >>= 1;
    node_idx = big.deflate.huffman_trees_ary[node_idx];
  }
  return big.deflate.huffman_trees_ary[node_idx + 1];
}

/* 1 <= size <= 318. */
static void read_bit_count_ary(deflate_huffman_bit_count_t *bit_count_ary_ptr, unsigned int size) {
  unsigned int count;
  unsigned int value;
  const unsigned int orig_size = size;
  while (size) {
    value = read_using_huffman_tree(TREE_TMP_ROOT_IDX);
    if (value > 18) goto corrupted_input;  /* For value == BAD_LEAF_VALUE. */
    if (value < 16) {
      *bit_count_ary_ptr++ = value;
      --size;
    } else if (value == 16) {
      if (size == orig_size) { corrupted_input: fatal_corrupted_input(); }
      value = bit_count_ary_ptr[-1];
      count = read_bits_max_8(2) + 3;
     append_many:
      if (count > size) goto corrupted_input;
      size -= count;
      while (count-- != 0) {
        *bit_count_ary_ptr++ = value;
      }
    } else if (value == 17) {
      count = read_bits_max_8(3) + 3;
     set_value_to_missing:
      value = 0;
      goto append_many;
    } else {
      count = read_bits_max_8(7) + 11;
      goto set_value_to_missing;
    }
  }
}

/* 1 <= size <= 288. */
static void build_huffman_tree(const deflate_huffman_bit_count_t *bit_count_ary_ptr, unsigned int size, unsigned int root_idx) {
  unsigned int size_i, bit_count;
  um16 bit_count_histogram_and_g_ary[16];
  um16 *bit_count_histogram_and_g_ptr;
  unsigned int total_g;
  unsigned int node_idx;
  um16 code, code_mask_i;
  memset(bit_count_histogram_and_g_ary, 0, sizeof(bit_count_histogram_and_g_ary));
  for (size_i = size; size_i-- != 0; ) {
    ++bit_count_histogram_and_g_ary[bit_count_ary_ptr[size_i]];
  }
  bit_count_histogram_and_g_ptr = bit_count_histogram_and_g_ary;
  *bit_count_histogram_and_g_ptr++ = total_g = 0;
  do {
    if (total_g > 0x4000U) fatal_corrupted_input();
    total_g <<= 1;
    total_g += *bit_count_histogram_and_g_ptr;
    *bit_count_histogram_and_g_ptr++ = total_g << 1;
  } while (bit_count_histogram_and_g_ptr != bit_count_histogram_and_g_ary + 16);
  for (size_i = 0; size_i < size; ++size_i) {
    if ((bit_count = bit_count_ary_ptr[size_i]) == 0) continue;
    node_idx = root_idx;
    --bit_count;
    code = bit_count_histogram_and_g_ary[bit_count]++;
    code_mask_i = 1U << bit_count;
    do {
      if (big.deflate.huffman_trees_ary[node_idx] == LEAF_IDX) {
        big.deflate.huffman_trees_ary[node_idx] = global_tree_free_idx;
        big.deflate.huffman_trees_ary[global_tree_free_idx++] = LEAF_IDX;
        ++global_tree_free_idx;  /* Skip setting the value of child 0 to BAD_LEAF_VALUE, because the next iteration in the first descent will overwrite it anyway. */
        big.deflate.huffman_trees_ary[node_idx + 1] = global_tree_free_idx;
        big.deflate.huffman_trees_ary[global_tree_free_idx++] = LEAF_IDX;
        big.deflate.huffman_trees_ary[global_tree_free_idx++] = BAD_LEAF_VALUE;
      }
      if ((code & code_mask_i) != 0) ++node_idx;
      node_idx = big.deflate.huffman_trees_ary[node_idx];
    } while ((code_mask_i >>= 1) != 0);
    big.deflate.huffman_trees_ary[node_idx + 1] = size_i;
  }
}

static void decompress_deflate_low(void) {  /* https://www.rfc-editor.org/rfc/rfc1951.txt */
  ub8 bfinal;
  unsigned int uncompressed_block_size;
  unsigned int literal_and_len_size;
  unsigned int distance_size;
  unsigned int tmp_size;
  unsigned int size_i;
  unsigned int token;
  unsigned int match_length;
  unsigned int match_distance_extra_bits;
  unsigned int match_distance;
  unsigned int match_distance_limit;
  unsigned int write_idx;
  deflate_huffman_bit_count_t tmp_bit_count_ary[19];

  global_bitbuf_bits_remaining = 0;
  write_idx = 0;
  match_distance_limit = 0;
  do {  /* Decompress next block. */
    bfinal = read_bits_max_8(1);  /* BFINAL. */  /* This would be the only caller of prog_decompress_read_bit(). We don't waste code size on the short implementation. */
    switch (read_bits_max_8(2)) {  /* BTYPE. */
     case 0:  /* Uncompressed block. */
      global_bitbuf_bits_remaining = 0;  /* Discard partially read byte. */
      uncompressed_block_size = read_bits_max_16(16);  /* LEN. */
      if (read_bits_max_16(16) != ((um16)~uncompressed_block_size & 0xffffU)) { corrupted_input: fatal_corrupted_input(); }  /* NLEN. */
      if (uncompressed_block_size >= 0x8000U || ((match_distance_limit += uncompressed_block_size) >= 0x8000U)) match_distance_limit = 0x8000U;
      while (uncompressed_block_size-- != 0) {
        global_write_buffer[write_idx] = get_byte();
        if (++write_idx == WRITE_BUFFER_SIZE) write_idx = global_flush_write_buffer_func(WRITE_BUFFER_SIZE);
      }
      break;
     case 1:  /* Block compressed with fixed Huffman codes. */
      literal_and_len_size = 288;
      for (size_i = 0; size_i < 144; big.deflate.huffman_bit_count_ary[size_i++] = 8) {}
      for (; size_i < 256; big.deflate.huffman_bit_count_ary[size_i++] = 9) {}
      for (; size_i < 280; big.deflate.huffman_bit_count_ary[size_i++] = 7) {}
      /* The tokens 286 and and 287 are invalid, but we still add them here because their presence affects the Huffman codes for 9-bit tokens 144..255. */
      for (; size_i < 288; big.deflate.huffman_bit_count_ary[size_i++] = 8) {}
      distance_size = 30;
      /* The distance values 30 and 31 are invalid, and there is no need to add them here, because their presence doesn't affect the Huffman codes of other distance values. */
      for (; size_i < 288 + 30; big.deflate.huffman_bit_count_ary[size_i++] = 5) {}
      global_tree_free_idx = TREE_FIRST_FREE_IDX;
      goto common_compressed_block;
     case 2:  /* Block compressed with dynamic Huffman codes. */
      literal_and_len_size = read_bits_max_8(5) + 257;
      if (literal_and_len_size > 286) goto corrupted_input;
      distance_size = read_bits_max_8(5) + 1;
      if (distance_size > 30) goto corrupted_input;
      tmp_size = read_bits_max_8(4) + 4;
      memset(tmp_bit_count_ary, 0, sizeof(tmp_bit_count_ary));
      for (size_i = 0; size_i < tmp_size; ++size_i) {
        tmp_bit_count_ary[(size_i < 3 ? size_i + 16 : size_i == 3 ? 0 : ((size_i & 1) != 0 ? 19 - size_i : size_i + 12) >> 1)] = read_bits_max_8(3);
      }
      global_tree_free_idx = TREE_FIRST_FREE_IDX;
      big.deflate.huffman_trees_ary[TREE_TMP_ROOT_IDX] = LEAF_IDX;
      build_huffman_tree(tmp_bit_count_ary, 19, TREE_TMP_ROOT_IDX);  /* All tmp_bit_count_ary[...] elements are in 0..7. */
      /* We must read the two bit count arrays together, because it's OK for them to overlap during RLE decompression. */
      /* After this, all elements are in 0..15. */
      read_bit_count_ary(big.deflate.huffman_bit_count_ary, literal_and_len_size + distance_size);
     common_compressed_block:
      big.deflate.huffman_trees_ary[TREE_LITERAL_AND_LEN_ROOT_IDX] = big.deflate.huffman_trees_ary[TREE_DISTANCE_ROOT_IDX] = LEAF_IDX;
      build_huffman_tree(big.deflate.huffman_bit_count_ary, literal_and_len_size, TREE_LITERAL_AND_LEN_ROOT_IDX);  /* All literal_and_len elements are in 0..15. */
      build_huffman_tree(big.deflate.huffman_bit_count_ary + literal_and_len_size, distance_size, TREE_DISTANCE_ROOT_IDX);  /* All distance elements are in 0..15. */
      for (;;) {
        token = read_using_huffman_tree(TREE_LITERAL_AND_LEN_ROOT_IDX);
        if (token < 0x100) { /* LZ literal token. */
          if (match_distance_limit < 0x8000U) ++match_distance_limit;
          global_write_buffer[write_idx] = token;
          if (++write_idx == WRITE_BUFFER_SIZE) write_idx = global_flush_write_buffer_func(write_idx);
          continue;
        }
        token -= 0x101;
        if ((int)token < 0) break;  /* End-of-block token (old value 256). Go to next block. */
        /* Now we have the LZ match token with the some info about the length in token. */
        if (token >= 28) {
          if (token > 28) goto corrupted_input;  /* Old token was BAD_LEAF_VALUE. This can also happen for tokens 286 and 287 in fixed Huffman codes. */
          match_length = 0x102;
        } else if (token < 8) {
          match_length = 3 + token;
        } else {
          match_length = 3 + ((4 + (token & 3)) << ((token >> 2) - 1)) + read_bits_max_8((token >> 2) - 1);
        }
        /* Now we have the final value of match_length: 1..258. */
        match_distance = read_using_huffman_tree(TREE_DISTANCE_ROOT_IDX);
        if (match_distance > 29) goto corrupted_input;  /* match_distance == BAD_LEAF_VALUE. This can also happen for distances 30 and 31 in fixed Huffman codes. */
        if (match_distance >= 4) {
          match_distance_extra_bits = (match_distance >> 1) - 1;
          match_distance = read_bits_max_16(match_distance_extra_bits) + (((match_distance & 1) + 2) << match_distance_extra_bits);
        }
        /* Now we have the final value of match_distance: 0..32767 == 0..0x7fff. */
        /* Now match_length contains the LZ match length and match_distance contains the LZ match distance. */
        if (match_distance >= match_distance_limit) goto corrupted_input;  /* LZ match refers back too much, before the first (literal) byte. */
        if ((match_distance_limit += match_length) >= 0x8000U) match_distance_limit = 0x8000U;  /* This doesn't overflow an um16, because old match_distance_limit <= 0x8000U and match_length < 0x8000U. */
        match_distance = (write_idx - 1 - match_distance);  /* After this, match_distance doesn't contain the LZ match distance. */
        do {
          global_write_buffer[write_idx] = global_write_buffer[match_distance++ & (WRITE_BUFFER_SIZE - 1U)];
          if (++write_idx == WRITE_BUFFER_SIZE) write_idx = global_flush_write_buffer_func(write_idx);
        } while (--match_length != 0);
      }
      break;
     default:  /* case 3. Reserved block type (error). */
      goto corrupted_input;
    }
  } while (!bfinal);
  global_flush_write_buffer_func(write_idx);
}

void decompress_deflate(void) {  /* https://www.rfc-editor.org/rfc/rfc1951.txt */
  global_flush_write_buffer_func = flush_write_buffer;
  decompress_deflate_low();
}

#if 0  /* Not implementing it here, because it's the same as decompress_deflate(). */
void decompress_quasijarus_nohdr(void) {  /* http://fileformats.archiveteam.org/wiki/Quasijarus_Strong_Compression */
  uc8 b, flg;

#if 0  /* The caller has already done this. */
  /* 4.3BSD-Quasijarus strong compression (compress -s) produces the 0x1f 0xa1 header and then raw Deflate. */
  unsigned int i = try_byte();
  if (i == BEOF) fatal_msg("empty compressed input file" LUUZCAT_NL);  /* This is not an error for `gzip -cd'. */
  if (i != 0x1f || try_byte() != 0xa1) fatal_msg("missing Quasijarus strong compression signature" LUUZCAT_NL);
#endif
  decompress_deflate();
}
#endif

static unsigned int get_be16(void) {
  const unsigned int i = get_byte() << 8;
  return i | get_byte();
}

void decompress_zlib_nohdr(void) {
#if 0  /* The caller has already done this. */
  uc8 b;
  if (((b = get_byte()) & 0xf) != 8 || (b >> 4) > 7) fatal_corrupted_input();  /* CM byte. Check that CM == 8, check that CINFO <= 7, otherwise ignore CINFO (sliding window size). */
  if (((b = get_byte()) & 0x20)) fatal_corrupted_input();  /* FLG byte. Check that FDICT == 0. Ignore FLEVEL and FCHECK. */
#endif
  global_flush_write_buffer_func = flush_write_buffer_and_adler32;
  adler32_init();
  decompress_deflate_low();
  if (get_be16() != global_checksum.adler32_s12[0] ||  /* Bad Adler-32 s2 in ADLER32. */
      get_be16() != global_checksum.adler32_s12[1]) fatal_corrupted_input();  /* Bad Adler-32 s1 in ADLER32. */
}

static unsigned int get_le16(void) {
  const unsigned int i = get_byte();
  return i | (get_byte() << 8);
}

static um32 get_le32(void) {
  const um32 i = get_le16();
  return i | ((um32)get_le16() << 16);
}

void decompress_gzip_nohdr(void) {  /* https://www.rfc-editor.org/rfc/rfc1952.txt */
  unsigned int i;
  uc8 flg;

#if 0  /* The caller has already done this. */
  unsigned int i = try_byte();  /* First byte is ID1, must be 0x1f. */
  if (i == BEOF) fatal_msg("empty compressed input file" LUUZCAT_NL);  /* This is not an error for `gzip -cd'. */
  if (i != 0x1f || try_byte() != 0x8b) fatal_msg("missing gzip signature" LUUZCAT_NL);
#endif
  if (get_byte() != 8) fatal_corrupted_input();  /* CM byte. Check that CM == 8. */
  flg = get_byte();  /* FLG byte. */
  for (i = 6; i-- != 0; ) {
    (void)get_byte();  /* Ignore MTIME (4 bytes), XFL (1 byte) and OS (1 byte). */
  }
  if ((flg & 2) != 0) fatal_corrupted_input();  /* The FHCRC enables GCONT (2-byte continuation part number) here for gzip(1), and header CRC16 later for https://www.rfc-editor.org/rfc/rfc1950.txt . We just fail if it's set. */
  if ((flg & 4) != 0) {  /* FEXTRA. Ignore the extra data. */
    for (i = read_bits_max_16(16); i-- != 0; ) {  /* Little-endian. It's OK to overlap bit reads with byte reads here, because we remain on byte boundary. */
      (void)get_byte();
    }
  }
  if ((flg & 8) != 0) {  /* FNAME. Ignore the filename. */
    while (get_byte() != 0) {}
  }
  if ((flg & 16) != 0) {  /* FCOMMENT. Ignore the comment. */
    while (get_byte() != 0) {}
  }
  /* Now we could ignore the 2-byte CRC16 if (flg & 2), but we've disallowed it above. */
  if ((flg & 32) != 0) {  /* Ignore gzip(1) the encryption header. */
    for (i = 12; i-- != 0; ) {
      (void)get_byte();
    }
  }
  build_crc32_table_if_needed();
  global_flush_write_buffer_func = flush_write_buffer_and_crc32_usize;
  crc32_init();
  global_usize = 0;
  decompress_deflate_low();
  if (get_le32() != (global_checksum.crc32 ^ CRC32_INITIAL)) fatal_corrupted_input();  /* Bad CRC-32 in CRC32. */
  if (get_le32() != global_usize) fatal_corrupted_input();  /* Bad uncompressed size in ISIZE. */
}
