/* SPDX-License-Identifier: 0BSD */

#ifndef TFLAC_HEADER_GUARD
#define TFLAC_HEADER_GUARD

/*
Copyright (C) 2024 John Regan <john@jrjrtech.com>

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.
*/



/*

TFLAC
=====

A single-header FLAC encoder that doesn't use the standard library
and doesn't allocate memory (it makes you do it instead!).

Building
========

In one C file, define TFLAC_IMPLEMENTATION before including this header, like:

    #define TFLAC_IMPLEMENTATION
    #include "tflac.h"

You'll need to allocate a tflac, initialize it with tflac_init,
then set the blocksize, bitdepth, channels, and samplerate fields.

    tflac t;
    
    tflac_init(&t);
    t.channels = 2;
    t.blocksize = 1152;
    t.bitdepth = 16;
    t.samplerate = 44100;

You'll need to allocate a block of memory for tflac to use for residuals calcuation.
You can get the needed memory size with either the TFLAC_SIZE_MEMORY macro or tflac_size_memory()
function. Both versions just require a blocksize.

Then call tflac_validate with your memory block and the size, it will validate that all
your fields make sense and record pointers into the memory block.

    uint32_t mem_size = tflac_size_memory(1152);
    void* block = malloc(mem_size);
    tflac_validate(&t, block, mem_size);

You'll also need to allocate a block of memory for storing your encoded
audio. You can find this size with wither the TFLAC_SIZE_FRAME macro
or tflac_size_frame() function. Both versions need the blocksize,
channels, and bitdepth, and return the maximum amount of bytes required
to hold a frame of audio.

    uint32_t buffer_len = tflac_size_frame(1152, 2, 16);
    void* buffer = malloc(buffer_len);

Finally to encode audio, you'll call the appropriate tflac_encode function,
depending on how your source data is laid out:
    - int32_t** (planar)      => tflac_encode_int32p
    - int32_t*  (interleaved) => tflac_encode_int32i
    - int16_t** (planar)      => tflac_encode_int16p
    - int16_t*  (interleaved) => tflac_encode_int16i

You'll also provide the current block size (all blocks except the last need to
use the same block size), a buffer to encode data to, and your buffer's length.

    int32_t* samples[2];
    uint32_t used = 0;
    samples[0] = { 0, 1,  2,  3,  4,  5,  6,  7 };
    samples[1] = { 8, 9, 10, 11, 12, 13, 14, 15 };
    tflac_encode_int32p(&t, 8, samples, buffer, buffer_len, &used);

You can now write out used bytes of buffer.

The library also has a convenience function for creating a STREAMINFO block:

    tflac_streaminfo(&t, 1,  buffer, bufferlen, &used);

The first argument is whether to set the "is-last-block" flag.

Other metadata blocks are out-of-scope, as is writing the "fLaC" stream marker.
*/

#if defined(__GNUC__) && __GNUC__ > 3 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 5)
#define TFLAC_CONST __attribute__((__const__))
#else
#define TFLAC_CONST
#endif

#if defined(__GNUC__) && __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)
#define TFLAC_PURE __attribute__((__pure__))
#else
#define TFLAC_PURE
#endif

#if defined(__GNUC__)
#define TFLAC_ASSUME_ALIGNED(x,a) __builtin_assume_aligned(x,a)
#else
#define TFLAC_ASSUME_ALIGNED(x,a) x
#endif

#if __STDC_VERSION__ >= 199901L
#define TFLAC_INLINE inline
#define TFLAC_RESTRICT restrict
#elif defined(__GNUC__) && __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)
#define TFLAC_INLINE __inline__
#define TFLAC_RESTRICT __restrict
#elif defined(__GNUC__) && __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1)
#define TFLAC_INLINE
#define TFLAC_RESTRICT __restrict
#else
#define TFLAC_INLINE
#define TFLAC_RESTRICT
#endif

#ifndef TFLAC_PUBLIC
#define TFLAC_PUBLIC
#endif

#ifndef TFLAC_PRIVATE
#define TFLAC_PRIVATE static
#endif


#define TFLAC_SIZE_STREAMINFO 38
#define TFLAC_SIZE_MEMORY(blocksize) (15 + (5 * ((15 + (blocksize * 8)) & 0xFFFFFFF0)))
#define TFLAC_SIZE_FRAME(blocksize, channels, bitdepth) \
    (18 + \
      ((blocksize * channels * bitdepth)/8) + \
      ( !!((blocksize * channels * bitdepth) % 8) ) + \
      channels)


#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

enum TFLAC_SUBFRAME_TYPE {
    TFLAC_SUBFRAME_CONSTANT   = 0,
    TFLAC_SUBFRAME_VERBATIM   = 1,
    TFLAC_SUBFRAME_FIXED      = 2,
    TFLAC_SUBFRAME_LPC        = 3,
    TFLAC_SUBFRAME_TYPE_COUNT = 4,
};

typedef enum TFLAC_SUBFRAME_TYPE TFLAC_SUBFRAME_TYPE;

struct tflac_bitwriter {
    uint64_t val;
    uint8_t  bits;
    uint8_t  crc8;
    uint16_t crc16;
    uint32_t pos;
    uint32_t len;
    uint8_t* buffer;
};

typedef struct tflac_bitwriter tflac_bitwriter;

struct tflac_md5 {
    uint32_t a;
    uint32_t b;
    uint32_t c;
    uint32_t d;
    uint8_t buffer[64];
    uint64_t total;
    uint8_t pos;
};

typedef struct tflac_md5 tflac_md5;

struct tflac {
    tflac_bitwriter bw;
    tflac_md5 md5_ctx;
    uint32_t blocksize;
    uint32_t samplerate;
    uint32_t channels;
    uint8_t bitdepth;

    uint8_t max_rice_value; /* defaults to 14 if bitdepth < 16; 30 otherwise */
    uint8_t min_partition_order; /* defaults to 0 */
    uint8_t max_partition_order; /* defaults to 0, should be <=8 to be in streamable subset */
    uint8_t partition_order;

    uint8_t enable_constant_subframe;
    uint8_t enable_fixed_subframe;
    uint8_t enable_md5;

    uint64_t samplecount;
    uint32_t frameno;
    uint32_t verbatim_subframe_len;
    uint32_t cur_blocksize;

    uint32_t min_frame_size;
    uint32_t max_frame_size;

    uint8_t wasted_bits;
    uint8_t constant;

    uint8_t md5_digest[16];

    uint64_t subframe_type_counts[8][TFLAC_SUBFRAME_TYPE_COUNT]; /* stores stats on what
    subframes were used per-channel */

    uint64_t residual_errors[5];
    int64_t* residuals[5]; /* orders 0, 1, 2, 3, 4 */
};
typedef struct tflac tflac;

#define TFLAC_BITWRITER_ZERO { \
  .val = 0, \
  .bits = 0, \
  .crc8 = 0, \
  .crc16 = 0, \
  .pos = 0, \
  .len = 0, \
  .buffer = NULL \
}

#define TFLAC_MD5_ZERO { \
    .a = 0x67452301, \
    .b = 0xefcdab89, \
    .c = 0x98badcfe, \
    .d = 0x10325476, \
    .buffer = { 0 }, \
    .total = 0, \
    .pos = 0, \
}

#define TFLAC_ZERO { \
  .bw = TFLAC_BITWRITER_ZERO, \
  .md5_ctx = TFLAC_MD5_ZERO, \
  .blocksize  = 0, \
  .samplerate = 0, \
  .channels   = 0, \
  .bitdepth   = 0, \
  .max_rice_value = 0, \
  .min_partition_order = 0, \
  .max_partition_order = 0, \
  .partition_order = 0, \
  .frameno    = 0, \
  .enable_constant_subframe = 1, \
  .enable_fixed_subframe = 1, \
  .enable_md5 = 1, \
  .verbatim_subframe_len = 0, \
  .cur_blocksize = 0, \
  .min_frame_size = 0, \
  .max_frame_size = 0, \
  .wasted_bits = 0, \
  .constant = 0, \
  .md5_digest = { \
      0, 0, 0, 0, \
      0, 0, 0, 0, \
      0, 0, 0, 0, \
      0, 0, 0, 0, \
  }, \
  .subframe_type_counts = { \
      { 0, 0, 0, 0 }, \
      { 0, 0, 0, 0 }, \
      { 0, 0, 0, 0 }, \
      { 0, 0, 0, 0 }, \
      { 0, 0, 0, 0 }, \
      { 0, 0, 0, 0 }, \
      { 0, 0, 0, 0 }, \
      { 0, 0, 0, 0 }, \
  }, \
  .residual_errors = { 0, 0, 0, 0, 0, }, \
  .residuals  = { NULL, NULL, NULL, NULL, NULL, }, \
}

extern const tflac_bitwriter tflac_bitwriter_zero;
extern const tflac tflac_zero;
extern const char* const tflac_subframe_types[4];

/* returns the maximum number of bytes to store a whole FLAC frame */
TFLAC_CONST
uint32_t tflac_size_frame(uint32_t blocksize, uint32_t channels, uint32_t bitdepth);

/* returns the memory size of the tflac struct */
TFLAC_CONST
uint32_t tflac_size(void);

/* returns how much memory is required for storing residuals */
TFLAC_CONST
uint32_t tflac_size_memory(uint32_t blocksize);

/* return the size needed to write a STREAMINFO block */
TFLAC_CONST
uint32_t tflac_size_streaminfo(void);

/* initialize a tflac struct */
TFLAC_PUBLIC
void tflac_init(tflac *);

/* validates that the encoder settings are OK, sets up needed memory pointers */
TFLAC_PUBLIC
int tflac_validate(tflac *, void* ptr, uint32_t len);

TFLAC_PUBLIC
int tflac_encode_int16p(tflac *, uint32_t blocksize, int16_t** samples, void* buffer, uint32_t len, uint32_t* used);

TFLAC_PUBLIC
int tflac_encode_int16i(tflac *, uint32_t blocksize, int16_t* samples, void* buffer, uint32_t len, uint32_t* used);

TFLAC_PUBLIC
int tflac_encode_int32p(tflac *, uint32_t blocksize, int32_t** samples, void* buffer, uint32_t len, uint32_t* used);

TFLAC_PUBLIC
int tflac_encode_int32i(tflac *, uint32_t blocksize, int32_t* samples, void* buffer, uint32_t len, uint32_t* used);

/* computes the final MD5 digest, if it was enabled */
TFLAC_PUBLIC
void tflac_finalize(tflac *);

/* encode a STREAMINFO block */
TFLAC_PUBLIC
int tflac_encode_streaminfo(const tflac *, uint32_t lastflag, void* buffer, uint32_t len, uint32_t* used);

/* setters for various fields */
TFLAC_PUBLIC
void tflac_set_blocksize(tflac *t, uint32_t blocksize);

TFLAC_PUBLIC
void tflac_set_samplerate(tflac *t, uint32_t samplerate);

TFLAC_PUBLIC
void tflac_set_channels(tflac *t, uint32_t channels);

TFLAC_PUBLIC
void tflac_set_bitdepth(tflac *t, uint32_t bitdepth);

TFLAC_PUBLIC
void tflac_set_max_rice_value(tflac *t, uint32_t max_rice_value);

TFLAC_PUBLIC
void tflac_set_min_partition_order(tflac *t, uint32_t min_partition_order);

TFLAC_PUBLIC
void tflac_set_max_partition_order(tflac *t, uint32_t max_partition_order);

TFLAC_PUBLIC
void tflac_set_constant_subframe(tflac* t, uint32_t enable);

TFLAC_PUBLIC
void tflac_set_fixed_subframe(tflac* t, uint32_t enable);

TFLAC_PUBLIC
void tflac_set_enable_md5(tflac* t, uint32_t enable);

/* getters for various fields */
TFLAC_PURE
TFLAC_PUBLIC
uint32_t tflac_get_blocksize(const tflac* t);

TFLAC_PURE
TFLAC_PUBLIC
uint32_t tflac_get_samplerate(const tflac* t);

TFLAC_PURE
TFLAC_PUBLIC
uint32_t tflac_get_channels(const tflac* t);

TFLAC_PURE
TFLAC_PUBLIC
uint32_t tflac_get_bitdepth(const tflac* t);

TFLAC_PURE
TFLAC_PUBLIC
uint32_t tflac_get_max_rice_value(const tflac* t);

TFLAC_PURE
TFLAC_PUBLIC
uint32_t tflac_get_min_partition_order(const tflac* t);

TFLAC_PURE
TFLAC_PUBLIC
uint32_t tflac_get_max_partition_order(const tflac* t);

TFLAC_PURE
TFLAC_PUBLIC
uint32_t tflac_get_constant_subframe(const tflac* t);

TFLAC_PURE
TFLAC_PUBLIC
uint32_t tflac_get_fixed_subframe(const tflac* t);

TFLAC_PURE
TFLAC_PUBLIC
uint32_t tflac_get_enable_md5(const tflac* t);

#ifdef __cplusplus
}
#endif

#endif /* ifndef TFLAC_HEADER_GUARD */

#ifdef TFLAC_IMPLEMENTATION

#ifdef _WIN32
#include <intrin.h>
#if _MSC_VER
#pragma intrinsic(_BitScanForward)
#endif
#endif

typedef void (*tflac_md5_calculator)(tflac*, void* samples);
typedef void (*tflac_sample_analyzer)(tflac*, uint32_t channels, void* samples);

struct tflac_encode_params {
    uint32_t blocksize;
    uint32_t buffer_len;
    void* buffer;
    void* samples;
    uint32_t *used;
    tflac_md5_calculator calculate_md5;
    tflac_sample_analyzer analyze;
};
typedef struct tflac_encode_params tflac_encode_params;

#define TFLAC_ENCODE_PARAMS_ZERO { \
  .blocksize = 0, \
  .buffer_len = 0, \
  .buffer = NULL, \
  .samples = NULL, \
  .used = NULL, \
  .calculate_md5 = NULL, \
  .analyze = NULL, \
}

const tflac_bitwriter tflac_bitwriter_zero = TFLAC_BITWRITER_ZERO;
const tflac tflac_zero = TFLAC_ZERO;
const char* const tflac_subframe_types[4] = {
    "CONSTANT",
    "VERBATIM",
    "FIXED",
    "LPC",
};

TFLAC_CONST
uint32_t tflac_size_frame(uint32_t blocksize, uint32_t channels, uint32_t bitdepth) {
    /* since blocksize is really a 16-bit value and everything else is way under that
     * we don't have to worry about overflow */

    /* the max bytes used for frame headers and footer is 18:
     *   2 for frame sync + blocking strategy
     *   1 for block size + sample rate
     *   1 for channel assignment and sample size
     *   7 for maximum sample number (we use frame number so it's really 6)
     *   2 for optional 16-bit blocksize
     *   2 for optional 16-bit samplerate
     *   1 for crc8
     *   2 for crc16 */
    return 18 +
      ((blocksize * channels * bitdepth)/8) +
      /* if we have an odd bitdepth we'll need an extra byte for alignment */
      ( !!((blocksize * channels * bitdepth) % 8) ) +
      /* each subframe header takes 1 byte */
      channels;
}

TFLAC_CONST
uint32_t tflac_size_memory(uint32_t blocksize) {
    /* assuming we need everything on a 16-byte alignment */
    return
      15 + (5 * ( (15 + (blocksize * 8)) & 0xFFFFFFF0));
}

TFLAC_PRIVATE int tflac_bitwriter_align(tflac_bitwriter*);
TFLAC_PRIVATE int tflac_bitwriter_add(tflac_bitwriter*, uint8_t bits, uint64_t val);
TFLAC_PRIVATE int tflac_encode(tflac* t, const tflac_encode_params* p);
TFLAC_PRIVATE int tflac_encode_frame_header(tflac *);

TFLAC_CONST TFLAC_PRIVATE TFLAC_INLINE
uint8_t tflac_wasted_bits_int32(int32_t sample);

TFLAC_CONST TFLAC_PRIVATE TFLAC_INLINE
uint8_t tflac_wasted_bits_int16(int16_t sample);

/* analyzes the samples and checks for:
 *   - are all samples constant?
 *   - wasted bps
 */
TFLAC_PRIVATE void tflac_analyze_samples_int16_planar(tflac*, uint32_t channel, const int16_t** samples);
TFLAC_PRIVATE void tflac_analyze_samples_int16_interleaved(tflac*, uint32_t channel, const int16_t* samples);
TFLAC_PRIVATE void tflac_analyze_samples_int32_planar(tflac*, uint32_t channel, const int32_t** samples);
TFLAC_PRIVATE void tflac_analyze_samples_int32_interleaved(tflac*, uint32_t channel, const int32_t* samples);

TFLAC_PRIVATE void tflac_calculate_fixed_residuals(tflac*);

/* encodes a constant subframe iff value is constant, this should always be tried first */
TFLAC_PRIVATE int tflac_encode_subframe_constant(tflac*);

/* encodes a fixed subframe only if the length < verbatim */
TFLAC_PRIVATE int tflac_encode_subframe_fixed(tflac*);

/* encodes a subframe verbatim, only fails if the buffer runs out of room */
TFLAC_PRIVATE int tflac_encode_subframe_verbatim(tflac*);

/* encodes a subframe, tries constant, fixed, then verbatim */
TFLAC_PRIVATE int tflac_encode_subframe(tflac*, uint8_t channel);

TFLAC_PRIVATE int tflac_encode_residuals(tflac*, uint8_t predictor_order, uint8_t partition_order);

TFLAC_PRIVATE const uint8_t tflac_bitwriter_crc8_table[256] = {
  0x00, 0x07, 0x0e, 0x09, 0x1c, 0x1b, 0x12, 0x15,
  0x38, 0x3f, 0x36, 0x31, 0x24, 0x23, 0x2a, 0x2d,
  0x70, 0x77, 0x7e, 0x79, 0x6c, 0x6b, 0x62, 0x65,
  0x48, 0x4f, 0x46, 0x41, 0x54, 0x53, 0x5a, 0x5d,
  0xe0, 0xe7, 0xee, 0xe9, 0xfc, 0xfb, 0xf2, 0xf5,
  0xd8, 0xdf, 0xd6, 0xd1, 0xc4, 0xc3, 0xca, 0xcd,
  0x90, 0x97, 0x9e, 0x99, 0x8c, 0x8b, 0x82, 0x85,
  0xa8, 0xaf, 0xa6, 0xa1, 0xb4, 0xb3, 0xba, 0xbd,
  0xc7, 0xc0, 0xc9, 0xce, 0xdb, 0xdc, 0xd5, 0xd2,
  0xff, 0xf8, 0xf1, 0xf6, 0xe3, 0xe4, 0xed, 0xea,
  0xb7, 0xb0, 0xb9, 0xbe, 0xab, 0xac, 0xa5, 0xa2,
  0x8f, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9d, 0x9a,
  0x27, 0x20, 0x29, 0x2e, 0x3b, 0x3c, 0x35, 0x32,
  0x1f, 0x18, 0x11, 0x16, 0x03, 0x04, 0x0d, 0x0a,
  0x57, 0x50, 0x59, 0x5e, 0x4b, 0x4c, 0x45, 0x42,
  0x6f, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7d, 0x7a,
  0x89, 0x8e, 0x87, 0x80, 0x95, 0x92, 0x9b, 0x9c,
  0xb1, 0xb6, 0xbf, 0xb8, 0xad, 0xaa, 0xa3, 0xa4,
  0xf9, 0xfe, 0xf7, 0xf0, 0xe5, 0xe2, 0xeb, 0xec,
  0xc1, 0xc6, 0xcf, 0xc8, 0xdd, 0xda, 0xd3, 0xd4,
  0x69, 0x6e, 0x67, 0x60, 0x75, 0x72, 0x7b, 0x7c,
  0x51, 0x56, 0x5f, 0x58, 0x4d, 0x4a, 0x43, 0x44,
  0x19, 0x1e, 0x17, 0x10, 0x05, 0x02, 0x0b, 0x0c,
  0x21, 0x26, 0x2f, 0x28, 0x3d, 0x3a, 0x33, 0x34,
  0x4e, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5c, 0x5b,
  0x76, 0x71, 0x78, 0x7f, 0x6a, 0x6d, 0x64, 0x63,
  0x3e, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2c, 0x2b,
  0x06, 0x01, 0x08, 0x0f, 0x1a, 0x1d, 0x14, 0x13,
  0xae, 0xa9, 0xa0, 0xa7, 0xb2, 0xb5, 0xbc, 0xbb,
  0x96, 0x91, 0x98, 0x9f, 0x8a, 0x8d, 0x84, 0x83,
  0xde, 0xd9, 0xd0, 0xd7, 0xc2, 0xc5, 0xcc, 0xcb,
  0xe6, 0xe1, 0xe8, 0xef, 0xfa, 0xfd, 0xf4, 0xf3,
};

TFLAC_PRIVATE const uint16_t tflac_bitwriter_crc16_table[256] = {
  0x0000, 0x8005, 0x800f, 0x000a, 0x801b, 0x001e, 0x0014, 0x8011,
  0x8033, 0x0036, 0x003c, 0x8039, 0x0028, 0x802d, 0x8027, 0x0022,
  0x8063, 0x0066, 0x006c, 0x8069, 0x0078, 0x807d, 0x8077, 0x0072,
  0x0050, 0x8055, 0x805f, 0x005a, 0x804b, 0x004e, 0x0044, 0x8041,
  0x80c3, 0x00c6, 0x00cc, 0x80c9, 0x00d8, 0x80dd, 0x80d7, 0x00d2,
  0x00f0, 0x80f5, 0x80ff, 0x00fa, 0x80eb, 0x00ee, 0x00e4, 0x80e1,
  0x00a0, 0x80a5, 0x80af, 0x00aa, 0x80bb, 0x00be, 0x00b4, 0x80b1,
  0x8093, 0x0096, 0x009c, 0x8099, 0x0088, 0x808d, 0x8087, 0x0082,
  0x8183, 0x0186, 0x018c, 0x8189, 0x0198, 0x819d, 0x8197, 0x0192,
  0x01b0, 0x81b5, 0x81bf, 0x01ba, 0x81ab, 0x01ae, 0x01a4, 0x81a1,
  0x01e0, 0x81e5, 0x81ef, 0x01ea, 0x81fb, 0x01fe, 0x01f4, 0x81f1,
  0x81d3, 0x01d6, 0x01dc, 0x81d9, 0x01c8, 0x81cd, 0x81c7, 0x01c2,
  0x0140, 0x8145, 0x814f, 0x014a, 0x815b, 0x015e, 0x0154, 0x8151,
  0x8173, 0x0176, 0x017c, 0x8179, 0x0168, 0x816d, 0x8167, 0x0162,
  0x8123, 0x0126, 0x012c, 0x8129, 0x0138, 0x813d, 0x8137, 0x0132,
  0x0110, 0x8115, 0x811f, 0x011a, 0x810b, 0x010e, 0x0104, 0x8101,
  0x8303, 0x0306, 0x030c, 0x8309, 0x0318, 0x831d, 0x8317, 0x0312,
  0x0330, 0x8335, 0x833f, 0x033a, 0x832b, 0x032e, 0x0324, 0x8321,
  0x0360, 0x8365, 0x836f, 0x036a, 0x837b, 0x037e, 0x0374, 0x8371,
  0x8353, 0x0356, 0x035c, 0x8359, 0x0348, 0x834d, 0x8347, 0x0342,
  0x03c0, 0x83c5, 0x83cf, 0x03ca, 0x83db, 0x03de, 0x03d4, 0x83d1,
  0x83f3, 0x03f6, 0x03fc, 0x83f9, 0x03e8, 0x83ed, 0x83e7, 0x03e2,
  0x83a3, 0x03a6, 0x03ac, 0x83a9, 0x03b8, 0x83bd, 0x83b7, 0x03b2,
  0x0390, 0x8395, 0x839f, 0x039a, 0x838b, 0x038e, 0x0384, 0x8381,
  0x0280, 0x8285, 0x828f, 0x028a, 0x829b, 0x029e, 0x0294, 0x8291,
  0x82b3, 0x02b6, 0x02bc, 0x82b9, 0x02a8, 0x82ad, 0x82a7, 0x02a2,
  0x82e3, 0x02e6, 0x02ec, 0x82e9, 0x02f8, 0x82fd, 0x82f7, 0x02f2,
  0x02d0, 0x82d5, 0x82df, 0x02da, 0x82cb, 0x02ce, 0x02c4, 0x82c1,
  0x8243, 0x0246, 0x024c, 0x8249, 0x0258, 0x825d, 0x8257, 0x0252,
  0x0270, 0x8275, 0x827f, 0x027a, 0x826b, 0x026e, 0x0264, 0x8261,
  0x0220, 0x8225, 0x822f, 0x022a, 0x823b, 0x023e, 0x0234, 0x8231,
  0x8213, 0x0216, 0x021c, 0x8219, 0x0208, 0x820d, 0x8207, 0x0202,
};

TFLAC_PRIVATE
const uint32_t tflac_md5_K[64] = {
    0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
    0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
    0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
    0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
    0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
    0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
    0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
    0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
    0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
    0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
    0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
    0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
    0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
    0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
    0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
    0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391,
};

TFLAC_PRIVATE
const uint8_t tflac_md5_s[64] = {
    7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,
    5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,
    4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,
    6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21,
};

TFLAC_PRIVATE
TFLAC_INLINE
uint32_t tflac_unpack_u32le(const uint8_t* d) {
    return (((uint32_t)d[0])    ) |
           (((uint32_t)d[1])<< 8) |
           (((uint32_t)d[2])<<16) |
           (((uint32_t)d[3])<<24);
}

TFLAC_PRIVATE
TFLAC_INLINE
uint32_t tflac_unpack_u32be(const uint8_t* d) {
    return (((uint32_t)d[3])    ) |
           (((uint32_t)d[2])<< 8) |
           (((uint32_t)d[1])<<16) |
           (((uint32_t)d[0])<<24);
}

TFLAC_PRIVATE
TFLAC_INLINE
void tflac_pack_u32le(uint8_t* d, uint32_t n) {
    d[0] = ( n       );
    d[1] = ( n >> 8  );
    d[2] = ( n >> 16 );
    d[3] = ( n >> 24 );
}

TFLAC_PRIVATE
TFLAC_INLINE
void tflac_pack_u64le(uint8_t* d, uint64_t n) {
    d[0] = ( n       );
    d[1] = ( n >> 8  );
    d[2] = ( n >> 16 );
    d[3] = ( n >> 24 );
    d[4] = ( n >> 32 );
    d[5] = ( n >> 40 );
    d[6] = ( n >> 48 );
    d[7] = ( n >> 56 );
}

TFLAC_CONST TFLAC_PRIVATE TFLAC_INLINE
uint8_t tflac_wasted_bits_int16(int16_t sample) {
#if defined(_WIN32)
    unsigned long index;
    if(sample == 0) return 16;
    _BitScanForward(&index, 0xFF00 | ((unsigned long)sample) );
    return index;
#elif __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
    if(sample == 0) return 16;
    return __builtin_ctz( 0xFF00 | ((unsigned int)sample) );
#else
    /* TODO optimize for other platforms? */
    uint16_t s = (uint16_t)sample;
    unsigned int i = 0;
    while(i < 15) {
        if(s >> i & 1) break;
        i++;
    }
    return i;
#endif
}

TFLAC_CONST TFLAC_PRIVATE TFLAC_INLINE
uint8_t tflac_wasted_bits_int32(int32_t sample) {
#if defined(_WIN32)
    unsigned long index;
    if(sample == 0) return 32;
    _BitScanForward(&index, (unsigned long)sample);
    return index;
#elif __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
    if(sample == 0) return 32;
    return __builtin_ctz((unsigned int)sample);
#else
    /* TODO optimize for other platforms? */
    uint32_t s = (uint32_t)sample;
    unsigned int i = 0;
    while(i < 31) {
        if(s >> i & 1) break;
        i++;
    }
    return i;
#endif
}

TFLAC_PRIVATE
TFLAC_INLINE
int tflac_bitwriter_flush(tflac_bitwriter *bw) {
    uint64_t avail = bw->len - bw->pos;
    uint64_t mask  = -1LL;
    uint8_t  byte  = 0;

    while(avail && bw->bits > 7) {
        bw->bits -= 8;
        byte = (uint8_t)((bw->val >> bw->bits) & 0xFF);
        bw->buffer[bw->pos++] = byte;

        bw->crc8  = tflac_bitwriter_crc8_table[bw->crc8 ^ byte];
        bw->crc16 = tflac_bitwriter_crc16_table[ (bw->crc16 >> 8) ^ byte ] ^ (( bw->crc16 & 0x00FF ) << 8);
        avail--;
    }

    if(bw->bits == 0) { /* all bits flushed */
        bw->val = 0;
        return 0;
    }

    mask >>= 64 - bw->bits;
    bw->val &= mask;

    /* if we stopped writing out because he ran out of space,
     * return an error */
    return avail == 0 ? -1 : 0;
}

TFLAC_PRIVATE
TFLAC_INLINE
int tflac_bitwriter_align(tflac_bitwriter *bw) {
    uint8_t r = bw->bits % 8;
    if(r) {
        tflac_bitwriter_add(bw,8-r,0);
        return tflac_bitwriter_flush(bw);
    }
    return 0;
}

#define TFLAC_MD5_LEFTROTATE(x, s) (x << s | x >> (32-s))

TFLAC_PRIVATE
TFLAC_INLINE
int tflac_bitwriter_add(tflac_bitwriter *bw, uint8_t bits, uint64_t val) {
    uint64_t mask  = -1LL;

    if(bits == 0) return 0;

    if(bw->bits + bits > 64) {
        return -1;
    }

    mask >>= (64 - bits);
    bw->val <<= bits;
    bw->val |= val & mask;
    bw->bits += bits;

    return tflac_bitwriter_flush(bw);
}

TFLAC_PRIVATE
void tflac_md5_transform(tflac_md5* m) {
    uint32_t A = m->a;
    uint32_t B = m->b;
    uint32_t C = m->c;
    uint32_t D = m->d;

    uint32_t F = 0;
    uint32_t g = 0;
    uint32_t i = 0;

    uint32_t M[16];

    M[0]  = tflac_unpack_u32le(&m->buffer[0  * 4]);
    M[1]  = tflac_unpack_u32le(&m->buffer[1  * 4]);
    M[2]  = tflac_unpack_u32le(&m->buffer[2  * 4]);
    M[3]  = tflac_unpack_u32le(&m->buffer[3  * 4]);
    M[4]  = tflac_unpack_u32le(&m->buffer[4  * 4]);
    M[5]  = tflac_unpack_u32le(&m->buffer[5  * 4]);
    M[6]  = tflac_unpack_u32le(&m->buffer[6  * 4]);
    M[7]  = tflac_unpack_u32le(&m->buffer[7  * 4]);
    M[8]  = tflac_unpack_u32le(&m->buffer[8  * 4]);
    M[9]  = tflac_unpack_u32le(&m->buffer[9  * 4]);
    M[10] = tflac_unpack_u32le(&m->buffer[10  * 4]);
    M[11] = tflac_unpack_u32le(&m->buffer[11  * 4]);
    M[12] = tflac_unpack_u32le(&m->buffer[12  * 4]);
    M[13] = tflac_unpack_u32le(&m->buffer[13  * 4]);
    M[14] = tflac_unpack_u32le(&m->buffer[14  * 4]);
    M[15] = tflac_unpack_u32le(&m->buffer[15  * 4]);


    while(i < 16) {
        F = (B & C) | ( (~B) & D);
        g = i;

        F += A + tflac_md5_K[i] + M[g];
        A = D;
        D = C;
        C = B;
        B = B + TFLAC_MD5_LEFTROTATE(F, tflac_md5_s[i]);

        i++;
    }
    while(i < 32) {
        F = (D & B) | ( (~D) & C);
        g = ( (5*i) + 1) % 16;

        F += A + tflac_md5_K[i] + M[g];
        A = D;
        D = C;
        C = B;
        B = B + TFLAC_MD5_LEFTROTATE(F, tflac_md5_s[i]);

        i++;
    }
    while(i<48) {
        F = B ^ C ^ D;
        g = ( (3*i) + 5) % 16;

        F += A + tflac_md5_K[i] + M[g];
        A = D;
        D = C;
        C = B;
        B = B + TFLAC_MD5_LEFTROTATE(F, tflac_md5_s[i]);

        i++;
    }
    while(i<64) {
        F = C ^ (B | (~D) );
        g = (7 * i) % 16;

        F += A + tflac_md5_K[i] + M[g];
        A = D;
        D = C;
        C = B;
        B = B + TFLAC_MD5_LEFTROTATE(F, tflac_md5_s[i]);

        i++;
    }

    m->a += A;
    m->b += B;
    m->c += C;
    m->d += D;

    m->pos = 0;
}

TFLAC_PRIVATE
TFLAC_INLINE
void tflac_md5_addsample(tflac_md5* m, uint8_t bits, uint64_t val) {
    /* ensure bits are aligned on a byte */
    uint8_t byte;
    bits = (7 + bits) & 0xF8;
    m->total += bits;

    while(bits) {
        byte = (uint8_t)val;
        m->buffer[m->pos++] = byte;
        if(m->pos == 64) {
            tflac_md5_transform(m);
        }

        bits -= 8;
        val >>= 8;
    }
}

TFLAC_PRIVATE
void tflac_md5_finalize(tflac_md5* m) {
    uint64_t len = m->total;
    /* add the stop bit */
    tflac_md5_addsample(m, 8, 0x80);

    /* pad zeroes until we have 8 bytes left */
    while(m->pos != 56) {
        tflac_md5_addsample(m, 8, 0x00);
    }

    tflac_pack_u64le(&m->buffer[56],len);
    tflac_md5_transform(m);
}

TFLAC_PRIVATE
void tflac_md5_digest(const tflac_md5* m, uint8_t out[16]) {
    out[0]  = (uint8_t)(m->a);
    out[1]  = (uint8_t)(m->a >> 8);
    out[2]  = (uint8_t)(m->a >> 16);
    out[3]  = (uint8_t)(m->a >> 24);
    out[4]  = (uint8_t)(m->b);
    out[5]  = (uint8_t)(m->b >> 8);
    out[6]  = (uint8_t)(m->b >> 16);
    out[7]  = (uint8_t)(m->b >> 24);
    out[8]  = (uint8_t)(m->c);
    out[9]  = (uint8_t)(m->c >> 8);
    out[10] = (uint8_t)(m->c >> 16);
    out[11] = (uint8_t)(m->c >> 24);
    out[12] = (uint8_t)(m->d);
    out[13] = (uint8_t)(m->d >> 8);
    out[14] = (uint8_t)(m->d >> 16);
    out[15] = (uint8_t)(m->d >> 24);
}

/* this will technically go over a byte for odd bit-depths but, whatever */
TFLAC_CONST TFLAC_PRIVATE
uint32_t tflac_verbatim_subframe_len(uint32_t blocksize, uint8_t bitdepth) {
    return 1 + ( (blocksize * bitdepth) / 8) +
      ( !!((blocksize * bitdepth % 8)));
}

TFLAC_PRIVATE
int tflac_encode_subframe_verbatim(tflac* t) {
    uint32_t i = 0;
    uint8_t  w = t->wasted_bits;
    const int64_t* residuals_0 = TFLAC_ASSUME_ALIGNED(t->residuals[0], 16);
    int r;

    if( (r = tflac_bitwriter_add(&t->bw, 1, 0x00)) != 0) return r;
    if( (r = tflac_bitwriter_add(&t->bw, 6, 0x01)) != 0) return r;
    if( w == 0) {
        if( (r = tflac_bitwriter_add(&t->bw, 1, 0x00)) != 0) return r;
    } else {
        if( (r = tflac_bitwriter_add(&t->bw, 1, 0x01)) != 0) return r;
        while(--w) {
            if( (r = tflac_bitwriter_add(&t->bw, 1, 0x00)) != 0) return r;
        }
        if( (r = tflac_bitwriter_add(&t->bw, 1, 0x01)) != 0) return r;
    }

    for(i=0;i<t->cur_blocksize;i++) {
        if( (r = tflac_bitwriter_add(&t->bw, t->bitdepth - t->wasted_bits, residuals_0[i])) != 0) return r;
    }

    return 0;
}

TFLAC_PRIVATE
int tflac_encode_subframe_constant(tflac* t) {
    int r;
    if( (r = tflac_bitwriter_add(&t->bw, 1, 0x00)) != 0) return r;
    if( (r = tflac_bitwriter_add(&t->bw, 6, 0x00)) != 0) return r;
    if( (r = tflac_bitwriter_add(&t->bw, 1, 0x00)) != 0) return r;
    return tflac_bitwriter_add(&t->bw, t->bitdepth, ((uint64_t) t->residuals[0][0]) << t->wasted_bits);
}

TFLAC_PRIVATE TFLAC_INLINE void tflac_rescale_samples(tflac* t) {
    uint32_t i = 0;
    int64_t* TFLAC_RESTRICT residuals_0 = TFLAC_ASSUME_ALIGNED(t->residuals[0], 16);

    if( (!t->constant) && t->wasted_bits) {
        /* rescale residuals for order 0 */
        for(i=0;i<t->cur_blocksize;i++) {
            residuals_0[i] = residuals_0[i] / (1 << t->wasted_bits);
        }
    }

    return;
}

TFLAC_PRIVATE void tflac_update_md5_int16_planar(tflac* t, const int16_t** samples) {
    uint32_t i = 0;
    uint32_t c = 0;

    for(i=0;i<t->cur_blocksize;i++) {
        for(c=0;i<t->channels;c++) {
            tflac_md5_addsample(&t->md5_ctx,t->bitdepth,samples[c][i]);
        }
    }
}

TFLAC_PRIVATE void tflac_update_md5_int16_interleaved(tflac* t, const int16_t* samples) {
    uint32_t i = 0;

    for(i=0;i<t->cur_blocksize * t->channels;i++) {
        tflac_md5_addsample(&t->md5_ctx,t->bitdepth,samples[i]);
    }
}

TFLAC_PRIVATE void tflac_update_md5_int32_planar(tflac* t, const int32_t** samples) {
    uint32_t i = 0;
    uint32_t c = 0;

    for(i=0;i<t->cur_blocksize;i++) {
        for(c=0;i<t->channels;c++) {
            tflac_md5_addsample(&t->md5_ctx,t->bitdepth,samples[c][i]);
        }
    }
}

TFLAC_PRIVATE void tflac_update_md5_int32_interleaved(tflac* t, const int32_t* samples) {
    uint32_t i = 0;

    for(i=0;i<t->cur_blocksize * t->channels;i++) {
        tflac_md5_addsample(&t->md5_ctx,t->bitdepth,samples[i]);
    }
}

TFLAC_PRIVATE void tflac_analyze_samples_int16_planar(tflac* t, uint32_t channel, const int16_t** _samples) {
    uint8_t wasted_bits = 0;
    uint16_t non_constant = 0;
    uint32_t i = 0;
    const int16_t* samples = _samples[channel];
    int64_t* TFLAC_RESTRICT residuals_0 = TFLAC_ASSUME_ALIGNED(t->residuals[0], 16);

    t->wasted_bits = 16;
    t->constant    =  0;

    for(i=0;i<t->cur_blocksize;i++) {
        wasted_bits = tflac_wasted_bits_int16(samples[i]);
        if(wasted_bits < t->wasted_bits) t->wasted_bits = wasted_bits;

        non_constant |= samples[i] ^ samples[0];

        residuals_0[i] = (int64_t)samples[i];
    }

    t->constant = !non_constant;
    t->wasted_bits &= 0x0f;

    return;
}

TFLAC_PRIVATE void tflac_analyze_samples_int16_interleaved(tflac* t, uint32_t channel, const int16_t* samples) {
    uint8_t wasted_bits = 0;
    uint16_t non_constant = 0;
    uint32_t i = 0;
    uint32_t j = 0;
    int64_t* TFLAC_RESTRICT residuals_0 = TFLAC_ASSUME_ALIGNED(t->residuals[0], 16);

    t->wasted_bits = 16;
    t->constant    =  0;

    j=channel;
    for(i=0;i<t->cur_blocksize;i++) {
        wasted_bits = tflac_wasted_bits_int16(samples[j]);
        if(wasted_bits < t->wasted_bits) t->wasted_bits = wasted_bits;

        non_constant |= samples[j] ^ samples[0];

        residuals_0[i] = (int64_t)samples[j];
        j+=t->channels;
    }

    t->constant = !non_constant;
    t->wasted_bits &= 0x0f;

    return;
}

TFLAC_PRIVATE void tflac_analyze_samples_int32_planar(tflac* t, uint32_t channel, const int32_t** _samples) {
    uint8_t wasted_bits = 0;
    uint32_t non_constant = 0;
    uint32_t i = 0;
    const int32_t* samples = _samples[channel];
    int64_t* TFLAC_RESTRICT residuals_0 = TFLAC_ASSUME_ALIGNED(t->residuals[0], 16);

    t->wasted_bits = 32;
    t->constant    = 0;

    for(i=0;i<t->cur_blocksize;i++) {
        wasted_bits = tflac_wasted_bits_int32(samples[i]);
        if(wasted_bits < t->wasted_bits) t->wasted_bits = wasted_bits;

        non_constant |= samples[i] ^ samples[0];

        /* go ahead and store the sample as a residual for order 0 since we're
         * going through all of these anyway */
        residuals_0[i] = (int64_t)samples[i];
    }

    t->constant = !non_constant;
    t->wasted_bits &= 0x1f;

    return;
}

TFLAC_PRIVATE void tflac_analyze_samples_int32_interleaved(tflac* t, uint32_t channel, const int32_t* samples) {
    uint8_t wasted_bits = 0;
    uint32_t non_constant = 0;
    uint32_t i = 0;
    uint32_t j = 0;
    int64_t* TFLAC_RESTRICT residuals_0 = TFLAC_ASSUME_ALIGNED(t->residuals[0], 16);

    t->wasted_bits = 32;
    t->constant    = 0;

    j = channel;
    for(i=0;i<t->cur_blocksize;i++) {
        wasted_bits = tflac_wasted_bits_int32(samples[j]);
        if(wasted_bits < t->wasted_bits) t->wasted_bits = wasted_bits;

        non_constant |= samples[j] ^ samples[0];

        /* go ahead and store the sample as a residual for order 0 since we're
         * going through all of these anyway */
        residuals_0[i] = (int64_t)samples[j];
        j += t->channels;
    }

    t->constant = !non_constant;
    t->wasted_bits &= 0x1f;

    return;
}

TFLAC_PRIVATE void tflac_calculate_fixed_residuals(tflac *t) {
    uint8_t min_check_order = 0;
    uint32_t i = 0;
    uint32_t j = 0;

    int64_t* TFLAC_RESTRICT residuals_0 = TFLAC_ASSUME_ALIGNED(t->residuals[0], 16);
    int64_t* TFLAC_RESTRICT residuals_1 = TFLAC_ASSUME_ALIGNED(t->residuals[1], 16);
    int64_t* TFLAC_RESTRICT residuals_2 = TFLAC_ASSUME_ALIGNED(t->residuals[2], 16);
    int64_t* TFLAC_RESTRICT residuals_3 = TFLAC_ASSUME_ALIGNED(t->residuals[3], 16);
    int64_t* TFLAC_RESTRICT residuals_4 = TFLAC_ASSUME_ALIGNED(t->residuals[4], 16);

    t->residual_errors[0] = 0;
    t->residual_errors[1] = 0;
    t->residual_errors[2] = 0;
    t->residual_errors[3] = 0;
    t->residual_errors[4] = 0;

    if(t->cur_blocksize < 5) {
        /* we won't be calculating any residuals so let's bail */
        t->residual_errors[1] = UINT64_MAX;
        t->residual_errors[2] = UINT64_MAX;
        t->residual_errors[3] = UINT64_MAX;
        t->residual_errors[4] = UINT64_MAX;
        return;
    }

    /* handle the first 4 residuals manually */
    residuals_1[0] = residuals_0[1] - residuals_0[0];

    residuals_1[1] = residuals_0[2] - residuals_0[1];
    residuals_2[0] = residuals_0[2] - (2 * residuals_0[1]) + residuals_0[0];

    residuals_1[2] = residuals_0[3] - residuals_0[2];
    residuals_2[1] = residuals_0[3] - (2 * residuals_0[2]) + residuals_0[1];
    residuals_3[0] = residuals_0[3] - (3 * residuals_0[2]) + (3 * residuals_0[1]) - residuals_0[0];

    /* now we can do the rest in a loop */
    for(i=4;i<t->cur_blocksize;i++) {
        residuals_1[i-1] = residuals_0[i] - residuals_0[i-1];
        residuals_2[i-2] = residuals_0[i] - (2 * residuals_0[i-1]) + residuals_0[i-2];
        residuals_3[i-3] = residuals_0[i] - (3 * residuals_0[i-1]) + (3 * residuals_0[i-2]) - residuals_0[i-3];
        residuals_4[i-4] = residuals_0[i] - (4 * residuals_0[i-1]) + (6 * residuals_0[i-2]) - (4 * residuals_0[i-3]) + residuals_0[i-4];

        t->residual_errors[0] += residuals_0[i]   < 0 ? -(uint64_t)residuals_0[i]   : (uint64_t)residuals_0[i];
        t->residual_errors[1] += residuals_1[i-1] < 0 ? -(uint64_t)residuals_1[i-1] : (uint64_t)residuals_1[i-1];
        t->residual_errors[2] += residuals_2[i-2] < 0 ? -(uint64_t)residuals_2[i-2] : (uint64_t)residuals_2[i-2];
        t->residual_errors[3] += residuals_3[i-3] < 0 ? -(uint64_t)residuals_3[i-3] : (uint64_t)residuals_3[i-3];
        t->residual_errors[4] += residuals_4[i-4] < 0 ? -(uint64_t)residuals_4[i-4] : (uint64_t)residuals_4[i-4];
    }

    /* it's possible for some of these residuals to be too large, each
     * order requires +order bits so for example
     *   if the audio is 16-bit we need:
     *     17 bits for order 1
     *     18 bits for order 2
     *     19 bits for order 3
     *     20 bits for order 4
     *
     * it's required that all residuals are 32-bit so if we're at higher bitdepths
     * we need to check, so this works out such that:
     *   bps = 29, only need to check order 4
     *   bps = 30, need to check 3,4
     *   bps = 31, need to check 2,3,4
     *   bps = 32, need to check 0,1,2,3,4
     *
     * because 32-bit input could have a INT32_MIN, that's a special
     * case that needs to check order 0 */

    switch(t->bitdepth) {
        case 32: min_check_order=0; break;
        case 31: min_check_order=2; break;
        case 30: min_check_order=3; break;
        case 29: min_check_order=4; break;
        default: return;
    }

    for(i=min_check_order;i<5;i++) {
        for(j=0;j<t->cur_blocksize-i;j++) {
            if(t->residuals[i][j] > INT32_MAX ||
               t->residuals[i][j] <= INT32_MIN) {
                t->residual_errors[i] = UINT64_MAX;
                break;
            }
        }
    }

    return;
}


TFLAC_PRIVATE
int tflac_encode_residuals(tflac* t, uint8_t predictor_order, uint8_t partition_order) {
    int r;
    uint8_t rice = 0;
    uint32_t i = 0;
    uint32_t j = 0;
    uint64_t v = 0;
    uint8_t  w = t->wasted_bits;
    uint32_t partition_length = 0;
    uint32_t offset = 0;
    uint64_t sum = 0;
    uint64_t msb = 0;
    uint64_t lsb = 0;
    uint32_t s = t->bw.pos;
    int64_t* residuals[5];

    residuals[0] = TFLAC_ASSUME_ALIGNED(t->residuals[0], 16);
    residuals[1] = TFLAC_ASSUME_ALIGNED(t->residuals[1], 16);
    residuals[2] = TFLAC_ASSUME_ALIGNED(t->residuals[2], 16);
    residuals[3] = TFLAC_ASSUME_ALIGNED(t->residuals[3], 16);
    residuals[4] = TFLAC_ASSUME_ALIGNED(t->residuals[4], 16);

    if( (r = tflac_bitwriter_add(&t->bw, 1, 0)) != 0) return r;
    if( (r = tflac_bitwriter_add(&t->bw, 3, 1)) != 0) return r;
    if( (r = tflac_bitwriter_add(&t->bw, 3, predictor_order)) != 0) return r;
    if( w == 0) {
        if( (r = tflac_bitwriter_add(&t->bw, 1, 0x00)) != 0) return r;
    } else {
        if( (r = tflac_bitwriter_add(&t->bw, 1, 0x01)) != 0) return r;
        while(--w) {
            if( (r = tflac_bitwriter_add(&t->bw, 1, 0x00)) != 0) return r;
        }
        if( (r = tflac_bitwriter_add(&t->bw, 1, 0x01)) != 0) return r;
    }

    for(i=0;i<predictor_order;i++) {
        if( (r = tflac_bitwriter_add(&t->bw, t->bitdepth - t->wasted_bits, residuals[0][i])) != 0) return r;
    }

    if(t->max_rice_value > 14) {
        if( (r = tflac_bitwriter_add(&t->bw, 2, 1)) != 0) return r;
    } else {
        if( (r = tflac_bitwriter_add(&t->bw, 2, 0)) != 0) return r;
    }
    if( (r = tflac_bitwriter_add(&t->bw, 4, partition_order)) != 0) return r;

    for(i=0;i < (1ULL << partition_order) ; i++) {

        partition_length = t->cur_blocksize >> partition_order;
        if(i == 0) partition_length -= (uint32_t)predictor_order;

        sum = 0;
        for(j=0;j<partition_length;j++) {
            sum += residuals[predictor_order][j+offset] < 0 ? -residuals[predictor_order][j+offset] : residuals[predictor_order][j+offset];
        }

        /* find the rice parameter for this partition */
        while( partition_length << (rice + 1) < sum ) {
            if(rice == t->max_rice_value) break;
            rice++;
        }

        if(t->max_rice_value > 14) {
            if( (r = tflac_bitwriter_add(&t->bw, 5, rice)) != 0) return r;
        } else {
            if( (r = tflac_bitwriter_add(&t->bw, 4, rice)) != 0) return r;
        }

        for(j=0;j<partition_length;j++) {
            v = residuals[predictor_order][j+offset] < 0 ?
                (((uint64_t) -residuals[predictor_order][j+offset] - 1) << 1) + 1:
                ((uint64_t)residuals[predictor_order][j+offset]) << 1;
            msb = v >> rice;
            lsb = v - (msb << rice);
            while(msb--) {
                if( (r = tflac_bitwriter_add(&t->bw, 1, 0)) != 0) return r;
            }
            if( (r = tflac_bitwriter_add(&t->bw, 1, 1)) != 0) return r;
            if( (r = tflac_bitwriter_add(&t->bw, rice, lsb)) != 0) return r;
        }

        offset += partition_length;
    }

    if(t->bw.pos - s > t->verbatim_subframe_len) {
        /* we somehow took more space than we would with a verbatim subframe? */
        return -1;
    }

    return 0;
}


TFLAC_PRIVATE
int tflac_encode_subframe_fixed(tflac* t) {
    uint8_t i = 0;
    uint8_t order = 5;
    uint8_t max_order = 4;
    uint64_t error = UINT64_MAX;
    uint8_t partition_order = t->partition_order;

    while( t->cur_blocksize >> t->partition_order <= max_order ) max_order--;

    for(i=0;i<=max_order;i++) {
        if(t->residual_errors[i] < error) {
            error = t->residual_errors[i];
            order = i;
        }
    }

    if(order == max_order+1) return -1;

    return tflac_encode_residuals(t, order, partition_order);
}

TFLAC_PRIVATE
int tflac_encode_subframe(tflac *t, uint8_t channel) {
    int r;
    tflac_bitwriter bw = t->bw;

    if(t->enable_constant_subframe && t->constant) {
        if(tflac_encode_subframe_constant(t) == 0) {
            t->subframe_type_counts[channel][TFLAC_SUBFRAME_CONSTANT]++;
            return 0;
        }
        t->bw = bw;
    }

    tflac_calculate_fixed_residuals(t);

    if(t->enable_fixed_subframe) {
        if(tflac_encode_subframe_fixed(t) == 0) {
            t->subframe_type_counts[channel][TFLAC_SUBFRAME_FIXED]++;
            return 0;
        }
        t->bw = bw;
    }

    if( (r = tflac_encode_subframe_verbatim(t)) == 0) {
        t->subframe_type_counts[channel][TFLAC_SUBFRAME_VERBATIM]++;
    }
    return r;
}

TFLAC_PRIVATE
int tflac_encode_frame_header(tflac *t) {
    int r;
    uint8_t blocksize_flag = 0;
    uint8_t samplerate_flag = 0;
    uint8_t bitdepth_flag = 0;
    uint8_t frameno_bytes[6];
    unsigned int i = 0;
    uint8_t frameno_len = 0;

    if( (r = tflac_bitwriter_add(&t->bw, 14, 0x3FFE)) != 0) return r;
    if( (r = tflac_bitwriter_add(&t->bw, 1, 0)) != 0) return r;
    if( (r = tflac_bitwriter_add(&t->bw, 1, 0)) != 0) return r;

    switch(t->cur_blocksize) {
        case 192:   blocksize_flag =  1; break;
        case 576:   blocksize_flag =  2; break;
        case 1152:  blocksize_flag =  3; break;
        case 2304:  blocksize_flag =  4; break;
        case 4608:  blocksize_flag =  5; break;
        case 256:   blocksize_flag =  8; break;
        case 512:   blocksize_flag =  9; break;
        case 1024:  blocksize_flag = 10; break;
        case 2048:  blocksize_flag = 11; break;
        case 4096:  blocksize_flag = 12; break;
        case 8192:  blocksize_flag = 13; break;
        case 16384: blocksize_flag = 14; break;
        case 32768: blocksize_flag = 15; break;
        default: {
            blocksize_flag = t->cur_blocksize > 256 ? 7 : 6;
        }
    }

    if( (r = tflac_bitwriter_add(&t->bw, 4, blocksize_flag)) != 0) return r;

    switch(t->samplerate) {
        case 96000: samplerate_flag++; /* fall-through */
        case 48000: samplerate_flag++; /* fall-through */
        case 44100: samplerate_flag++; /* fall-through */
        case 32000: samplerate_flag++; /* fall-through */
        case 24000: samplerate_flag++; /* fall-through */
        case 22050: samplerate_flag++; /* fall-through */
        case 16000: samplerate_flag++; /* fall-through */
        case 8000: samplerate_flag++; /* fall-through */
        case 192000: samplerate_flag++; /* fall-through */
        case 176400: samplerate_flag++; /* fall-through */
        case 882000: {
            samplerate_flag++;
            break;
        }
        default: {
            if(t->samplerate % 1000 == 0) {
                if(t->samplerate / 1000 < 256) {
                    samplerate_flag = 12;
                }
            } else if(t->samplerate % 10 == 0) {
                if(t->samplerate / 10 < 65536) {
                    samplerate_flag = 13;
                }
            } else if(t->samplerate < 65536) {
                samplerate_flag = 14;
            }
        }
    }

    if( (r = tflac_bitwriter_add(&t->bw, 4, samplerate_flag)) != 0) return r;

    if( (r = tflac_bitwriter_add(&t->bw, 4, t->channels - 1)) != 0) return r;

    switch(t->bitdepth) {
        case 8: bitdepth_flag = 1; break;
        case 12: bitdepth_flag = 2; break;
        case 16: bitdepth_flag = 4; break;
        case 20: bitdepth_flag = 5; break;
        case 24: bitdepth_flag = 6; break;
        case 32: bitdepth_flag = 7; break;
        default: break;
    }
    if( (r = tflac_bitwriter_add(&t->bw, 3, bitdepth_flag)) != 0) return r;
    if( (r = tflac_bitwriter_add(&t->bw, 1, 0)) != 0) return r;

    if(t->frameno < ( (uint32_t)1 << 7) ) {
        frameno_bytes[0] = t->frameno & 0x7F;
        frameno_len = 1;
    } else if(t->frameno < ((uint32_t)1 << 11)) {
        frameno_bytes[0] = 0xC0 | ((t->frameno >> 6) & 0x1F);
        frameno_bytes[1] = 0x80 | ((t->frameno     ) & 0x3F);
        frameno_len = 2;
    } else if(t->frameno < ((uint32_t)1 << 16)) {
        frameno_bytes[0] = 0xE0 | ((t->frameno >> 12) & 0x0F);
        frameno_bytes[1] = 0x80 | ((t->frameno >>  6) & 0x3F);
        frameno_bytes[2] = 0x80 | ((t->frameno      ) & 0x3F);
        frameno_len = 3;
    } else if(t->frameno < ((uint32_t)1 << 21)) {
        frameno_bytes[0] = 0xF0 | ((t->frameno >> 18) & 0x07);
        frameno_bytes[1] = 0x80 | ((t->frameno >> 12) & 0x3F);
        frameno_bytes[2] = 0x80 | ((t->frameno >>  6) & 0x3F);
        frameno_bytes[3] = 0x80 | ((t->frameno      ) & 0x3F);
        frameno_len = 4;
    } else if(t->frameno < ((uint32_t)1 << 26)) {
        frameno_bytes[0] = 0xF8 | ((t->frameno >> 24) & 0x03);
        frameno_bytes[1] = 0x80 | ((t->frameno >> 18) & 0x3F);
        frameno_bytes[2] = 0x80 | ((t->frameno >> 12) & 0x3F);
        frameno_bytes[3] = 0x80 | ((t->frameno >>  6) & 0x3F);
        frameno_bytes[4] = 0x80 | ((t->frameno      ) & 0x3F);
        frameno_len = 5;
    } else {
        frameno_bytes[0] = 0xFC | ((t->frameno >> 30) & 0x01);
        frameno_bytes[1] = 0x80 | ((t->frameno >> 24) & 0x3F);
        frameno_bytes[2] = 0x80 | ((t->frameno >> 18) & 0x3F);
        frameno_bytes[3] = 0x80 | ((t->frameno >> 12) & 0x3F);
        frameno_bytes[4] = 0x80 | ((t->frameno >>  6) & 0x3F);
        frameno_bytes[5] = 0x80 | ((t->frameno      ) & 0x3F);
        frameno_len = 6;
    }

    for(i=0;i<frameno_len;i++) {
        if( (r = tflac_bitwriter_add(&t->bw, 8, frameno_bytes[i])) != 0) return r;
    }

    switch(blocksize_flag) {
        case 6: {
            if( (r = tflac_bitwriter_add(&t->bw, 8, t->cur_blocksize - 1)) != 0) return r;
            break;
        }
        case 7: {
            if( (r = tflac_bitwriter_add(&t->bw, 16, t->cur_blocksize - 1)) != 0) return r;
            break;
        }
        default: break;
    }

    switch(samplerate_flag) {
        case 12: {
            if( (r = tflac_bitwriter_add(&t->bw, 8, t->samplerate / 1000)) != 0) return r;
            break;
        }
        case 13: {
            if( (r = tflac_bitwriter_add(&t->bw, 16, t->samplerate)) != 0) return r;
            break;
        }
        case 14: {
            if( (r = tflac_bitwriter_add(&t->bw, 16, t->samplerate / 10)) != 0) return r;
            break;
        }
        default: break;
    }

    return tflac_bitwriter_add(&t->bw, 8, t->bw.crc8);
}

TFLAC_PUBLIC
void tflac_init(tflac *t) {
    *t = tflac_zero;
}

TFLAC_PUBLIC
int tflac_validate(tflac *t, void* ptr, uint32_t len) {
    uint32_t res_len = 0;
    uintptr_t p;

    if(t->blocksize < 16) return -1;
    if(t->blocksize > 65535) return -1;
    if(t->samplerate == 0) return -1;
    if(t->samplerate > 655350) return -1;
    if(t->channels == 0) return -1;
    if(t->channels > 8) return -1;
    if(t->bitdepth == 0) return -1;
    if(t->bitdepth > 32) return -1;

    if(t->max_rice_value == 0) {
        if(t->bitdepth <= 16) {
            t->max_rice_value = 14;
        } else {
            t->max_rice_value = 30;
        }
    } else if(t->max_rice_value > 30) {
        return -1;
    }

    if(t->max_partition_order > 15) {
        return -1;
    }

    if(t->min_partition_order > t->max_partition_order) return -1;

    if(len < tflac_size_memory(t->blocksize)) return -1;

    p = ((uintptr_t)ptr+15) & ~ (uintptr_t)0x0F;
    res_len = (15 + (t->blocksize * 8)) & 0xFFFFFFF0;

    t->residuals[0] = (int64_t*)(p + (0 * res_len));
    t->residuals[1] = (int64_t*)(p + (1 * res_len));
    t->residuals[2] = (int64_t*)(p + (2 * res_len));
    t->residuals[3] = (int64_t*)(p + (3 * res_len));
    t->residuals[4] = (int64_t*)(p + (4 * res_len));

    t->partition_order = t->min_partition_order;
    while( (t->blocksize % (1<<(t->partition_order+1)) == 0) && t->partition_order < t->max_partition_order) {
        t->partition_order++;
    }
    t->verbatim_subframe_len = tflac_verbatim_subframe_len(t->blocksize, t->bitdepth);
    t->cur_blocksize = t->blocksize;

    return 0;
}

TFLAC_PRIVATE
int tflac_encode(tflac* t, const tflac_encode_params* p) {
    uint8_t c = 0;
    int r;

    if(t->cur_blocksize != p->blocksize) {
        t->cur_blocksize = p->blocksize;
        t->verbatim_subframe_len = tflac_verbatim_subframe_len(t->cur_blocksize, t->bitdepth);

        t->partition_order = t->min_partition_order;
        while( (t->cur_blocksize % (1<<(t->partition_order+1)) == 0) && t->partition_order < t->max_partition_order) {
            t->partition_order++;
        }
    }

    if(t->enable_md5) p->calculate_md5(t, p->samples);

    t->bw = tflac_bitwriter_zero;
    t->bw.buffer = p->buffer;
    t->bw.len    = p->buffer_len;

    if( (r = tflac_encode_frame_header(t)) != 0) return r;

    for(c=0;c<t->channels;c++) {
        p->analyze(t, c, p->samples);
        tflac_rescale_samples(t);
        if( (r = tflac_encode_subframe(t, c)) != 0) return r;
    }
    if( (r = tflac_bitwriter_align(&t->bw)) != 0) return r;

    if( (r = tflac_bitwriter_add(&t->bw, 16, t->bw.crc16)) != 0) return r;

    *(p->used) = t->bw.pos;
    if(t->bw.pos < t->min_frame_size || t->min_frame_size == 0) {
        t->min_frame_size = t->bw.pos;
    }

    if(t->bw.pos > t->max_frame_size) {
        t->max_frame_size = t->bw.pos;
    }

    t->frameno++;
    t->frameno &= 0x7FFFFFFF; /* cap to 31 bits */
    t->samplecount += t->cur_blocksize;
    t->samplecount &= 0x0000000FFFFFFFFFULL; /* cap to 36 bits */

    return 0;
}

TFLAC_PUBLIC
int tflac_encode_int16p(tflac* t, uint32_t blocksize, int16_t** samples, void* buffer, uint32_t len, uint32_t* used) {
    tflac_encode_params p = TFLAC_ENCODE_PARAMS_ZERO;

    p.blocksize = blocksize;
    p.buffer_len = len;
    p.buffer = buffer;
    p.used = used;
    p.samples = samples;
    p.calculate_md5 = (tflac_md5_calculator)tflac_update_md5_int16_planar;
    p.analyze       = (tflac_sample_analyzer)tflac_analyze_samples_int16_planar;

    return tflac_encode(t, &p);
}

TFLAC_PUBLIC
int tflac_encode_int16i(tflac* t, uint32_t blocksize, int16_t* samples, void* buffer, uint32_t len, uint32_t* used) {
    tflac_encode_params p = TFLAC_ENCODE_PARAMS_ZERO;

    p.blocksize = blocksize;
    p.buffer_len = len;
    p.buffer = buffer;
    p.used = used;
    p.samples = samples;
    p.calculate_md5 = (tflac_md5_calculator)tflac_update_md5_int16_interleaved;
    p.analyze = (tflac_sample_analyzer)tflac_analyze_samples_int16_interleaved;

    return tflac_encode(t, &p);
}

TFLAC_PUBLIC
int tflac_encode_int32p(tflac* t, uint32_t blocksize, int32_t** samples, void* buffer, uint32_t len, uint32_t* used) {
    tflac_encode_params p = TFLAC_ENCODE_PARAMS_ZERO;

    p.blocksize = blocksize;
    p.buffer_len = len;
    p.buffer = buffer;
    p.used = used;
    p.samples = samples;
    p.calculate_md5 = (tflac_md5_calculator)tflac_update_md5_int32_planar;
    p.analyze = (tflac_sample_analyzer)tflac_analyze_samples_int32_planar;

    return tflac_encode(t, &p);
}

TFLAC_PUBLIC
int tflac_encode_int32i(tflac* t, uint32_t blocksize, int32_t* samples, void* buffer, uint32_t len, uint32_t* used) {
    tflac_encode_params p = TFLAC_ENCODE_PARAMS_ZERO;

    p.blocksize = blocksize;
    p.buffer_len = len;
    p.buffer = buffer;
    p.used = used;
    p.samples = samples;
    p.calculate_md5 = (tflac_md5_calculator)tflac_update_md5_int32_interleaved;
    p.analyze = (tflac_sample_analyzer)tflac_analyze_samples_int32_interleaved;

    return tflac_encode(t, &p);
}

TFLAC_CONST
TFLAC_PUBLIC
uint32_t tflac_size_streaminfo(void) {
    return TFLAC_SIZE_STREAMINFO;
}

TFLAC_CONST
TFLAC_PUBLIC
uint32_t tflac_size(void) {
    return sizeof(tflac);
}

TFLAC_PUBLIC
void tflac_finalize(tflac* t) {
    if(t->enable_md5) {
        tflac_md5_finalize(&t->md5_ctx);
        tflac_md5_digest(&t->md5_ctx, t->md5_digest);
    }
}

TFLAC_PUBLIC
int tflac_encode_streaminfo(const tflac* t, uint32_t lastflag, void* buffer, uint32_t len, uint32_t* used) {
    int r;
    tflac_bitwriter bw = TFLAC_BITWRITER_ZERO;

    bw.buffer = buffer;
    bw.len = len;

    if( (r = tflac_bitwriter_add(&bw, 1, lastflag)) != 0) return r;
    if( (r = tflac_bitwriter_add(&bw, 7, 0)) != 0) return r;
    if( (r = tflac_bitwriter_add(&bw, 24, 34)) != 0) return r;

    /* min/max block sizes */
    if( (r = tflac_bitwriter_add(&bw, 16, t->blocksize)) != 0) return r;
    if( (r = tflac_bitwriter_add(&bw, 16, t->blocksize)) != 0) return r;

    /* min/max frame sizes */
    if( (r = tflac_bitwriter_add(&bw, 24, t->min_frame_size)) != 0) return r;
    if( (r = tflac_bitwriter_add(&bw, 24, t->max_frame_size)) != 0) return r;

    /* sample rate */
    if( (r = tflac_bitwriter_add(&bw, 20, t->samplerate)) != 0) return r;

    /* channels -1 */
    if( (r = tflac_bitwriter_add(&bw, 3, t->channels - 1)) != 0) return r;

    /* bps - 1 */
    if( (r = tflac_bitwriter_add(&bw, 5, t->bitdepth - 1)) != 0) return r;

    /* total samples */
    if( (r = tflac_bitwriter_add(&bw, 36, t->samplecount)) != 0) return r;

    /* md5 checksum */
    if( (r = tflac_bitwriter_add(&bw, 8, t->md5_digest[0])) != 0) return r;
    if( (r = tflac_bitwriter_add(&bw, 8, t->md5_digest[1])) != 0) return r;
    if( (r = tflac_bitwriter_add(&bw, 8, t->md5_digest[2])) != 0) return r;
    if( (r = tflac_bitwriter_add(&bw, 8, t->md5_digest[3])) != 0) return r;
    if( (r = tflac_bitwriter_add(&bw, 8, t->md5_digest[4])) != 0) return r;
    if( (r = tflac_bitwriter_add(&bw, 8, t->md5_digest[5])) != 0) return r;
    if( (r = tflac_bitwriter_add(&bw, 8, t->md5_digest[6])) != 0) return r;
    if( (r = tflac_bitwriter_add(&bw, 8, t->md5_digest[7])) != 0) return r;
    if( (r = tflac_bitwriter_add(&bw, 8, t->md5_digest[8])) != 0) return r;
    if( (r = tflac_bitwriter_add(&bw, 8, t->md5_digest[9])) != 0) return r;
    if( (r = tflac_bitwriter_add(&bw, 8, t->md5_digest[10])) != 0) return r;
    if( (r = tflac_bitwriter_add(&bw, 8, t->md5_digest[11])) != 0) return r;
    if( (r = tflac_bitwriter_add(&bw, 8, t->md5_digest[12])) != 0) return r;
    if( (r = tflac_bitwriter_add(&bw, 8, t->md5_digest[13])) != 0) return r;
    if( (r = tflac_bitwriter_add(&bw, 8, t->md5_digest[14])) != 0) return r;
    if( (r = tflac_bitwriter_add(&bw, 8, t->md5_digest[15])) != 0) return r;

    *used = bw.pos;
    return 0;
}

TFLAC_PUBLIC void tflac_set_blocksize(tflac *t, uint32_t blocksize) {
    t->blocksize = blocksize;
}

TFLAC_PUBLIC void tflac_set_samplerate(tflac *t, uint32_t samplerate) {
    t->samplerate = samplerate;
}

TFLAC_PUBLIC void tflac_set_channels(tflac *t, uint32_t channels) {
    t->channels = channels;
}

TFLAC_PUBLIC void tflac_set_bitdepth(tflac *t, uint32_t bitdepth) {
    t->bitdepth = (uint8_t)bitdepth;
}

TFLAC_PUBLIC void tflac_set_max_rice_value(tflac *t, uint32_t max_rice_value) {
    t->max_rice_value = (uint8_t)max_rice_value;
}

TFLAC_PUBLIC void tflac_set_min_partition_order(tflac *t, uint32_t min_partition_order) {
    t->min_partition_order = (uint8_t)min_partition_order;
}

TFLAC_PUBLIC void tflac_set_max_partition_order(tflac *t, uint32_t max_partition_order) {
    t->max_partition_order = (uint8_t)max_partition_order;
}

TFLAC_PUBLIC void tflac_set_constant_subframe(tflac* t, uint32_t enable) {
    t->enable_constant_subframe = (uint8_t)enable;
}

TFLAC_PUBLIC void tflac_set_fixed_subframe(tflac* t, uint32_t enable) {
    t->enable_fixed_subframe = (uint8_t)enable;
}

TFLAC_PUBLIC void tflac_set_enable_md5(tflac* t, uint32_t enable) {
    t->enable_md5 = (uint8_t)enable;
}

TFLAC_PURE TFLAC_PUBLIC uint32_t tflac_get_blocksize(const tflac* t) {
    return t->blocksize;
}

TFLAC_PURE TFLAC_PUBLIC uint32_t tflac_get_samplerate(const tflac* t) {
    return t->samplerate;
}

TFLAC_PURE TFLAC_PUBLIC uint32_t tflac_get_channels(const tflac* t) {
    return t->channels;
}

TFLAC_PURE TFLAC_PUBLIC uint32_t tflac_get_bitdepth(const tflac* t) {
    return t->bitdepth;
}

TFLAC_PURE TFLAC_PUBLIC uint32_t tflac_get_max_rice_value(const tflac* t) {
    return t->max_rice_value;
}

TFLAC_PURE TFLAC_PUBLIC uint32_t tflac_get_min_partition_order(const tflac* t) {
    return t->min_partition_order;
}

TFLAC_PURE TFLAC_PUBLIC uint32_t tflac_get_max_partition_order(const tflac* t) {
    return t->max_partition_order;
}

TFLAC_PURE TFLAC_PUBLIC uint32_t tflac_get_constant_subframe(const tflac* t) {
    return t->enable_constant_subframe;
}

TFLAC_PURE TFLAC_PUBLIC uint32_t tflac_get_fixed_subframe(const tflac* t) {
    return t->enable_fixed_subframe;
}

TFLAC_PURE TFLAC_PUBLIC uint32_t tflac_get_wasted_bits(const tflac* t) {
    return t->wasted_bits;
}

TFLAC_PURE TFLAC_PUBLIC uint32_t tflac_get_constant(const tflac* t) {
    return t->constant;
}

TFLAC_PURE TFLAC_PUBLIC uint32_t tflac_get_enable_md5(const tflac* t) {
    return t->enable_md5;
}

/* TODO:
 *
 *   For SUBFRAME_FIXED:
 *
 *       I'm not sure if the best thing to is is pick the largest partition
 *       order since that results in the smallest partition size, or to try
 *       all partition orders until the one that results in the smallest
 *       length is found?
 *
 *       Using the largest order seems to be seems to be really, really fast
 *       and still get decent compression results.
 */

#endif

