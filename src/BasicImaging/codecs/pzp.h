/*
PZP Portable Zipped PNM
Copyright (C) 2025 Ammar Qammaz

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef PZP_H_INCLUDED
#define PZP_H_INCLUDED

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <zstd.h>
//sudo apt install libzstd-dev
#include <lz4.h>
//sudo apt install liblz4-dev

/* Build with PZP_USE_OPENZL=1 ( `make PZP_USE_OPENZL=1` fetches and builds OpenZL v0.3.0 ) to read
 * and write OpenZL-compressed frames ( USE_OPENZL ). Without it such frames are detected and
 * rejected with a message instead of being handed to zstd. */
#ifndef PZP_USE_OPENZL
#define PZP_USE_OPENZL 0
#endif

#if PZP_USE_OPENZL
#include "openzl/zl_compress.h"
#include "openzl/zl_compressor.h"
#include "openzl/zl_decompress.h"
#include "openzl/zl_graph_api.h"
#endif

#if INTEL_OPTIMIZATIONS
#include <immintrin.h>  // AVX intrinsics
#include <emmintrin.h>  // SSE2
#include <stdint.h>
//#warning "Intel Optimizations Enabled"
#endif // INTEL_OPTIMIZATIONS

#ifndef PZP_VERBOSE
#define PZP_VERBOSE 0
#endif

/* Set PZP_VERIFY_CHECKSUM=1 to enable read-time checksum verification.
 * Disabled by default: on large datasets the pixel checksum walk accounts
 * for ~9% of load time with no benefit once files are known-good. */
#ifndef PZP_VERIFY_CHECKSUM
#define PZP_VERIFY_CHECKSUM 0
#endif

static const char pzp_version[]="v0.03";
static const char pzp_header[4]={"PZP0"};
/* Inner frame magic of frames that carry a channel group table ( see PZPChannelGroup ). Frames with
   the "PZP0" inner magic are the pre-0.03 layout and are read as one implied 8-bit group. */
static const char pzp_frame_header_groups[4]={"PZP1"};

/* Inner frame header: 10 × uint32 = 40 bytes (unchanged) */
static const int headerSize =  sizeof(unsigned int) * 10;
//header, width, height, bitsperpixel, channels, internalbitsperpixel, internalchannels, checksum, compression_mode, paletteDataBytes

/* Outer container header: 12 × uint32 = 48 bytes */
static const int containerHeaderSize = sizeof(unsigned int) * 12;

/* Per-frame index entry: 4 × uint32 = 16 bytes */
static const int frameEntrySize = sizeof(unsigned int) * 4;

// ---------------------------------------------------------------------------
// Per-thread ZSTD decompression context
//
// Reusing a ZSTD_DCtx across calls avoids the ~6 KB alloc/init/free overhead
// that ZSTD_decompress() pays on every call.  All pzp decompression paths
// share this context automatically via lazy init.
//
// Optional explicit lifecycle API for worker threads:
//   pzp_thread_init()    — eagerly create the context at thread startup
//   pzp_thread_cleanup() — free it at thread exit (prevents valgrind noise)
// ---------------------------------------------------------------------------
static _Thread_local ZSTD_DCtx *_pzp_zstd_dctx = NULL;

static inline void pzp_thread_init(void)
{
    if (!_pzp_zstd_dctx)
        _pzp_zstd_dctx = ZSTD_createDCtx();
}

#if PZP_USE_OPENZL
/* Per-thread OpenZL state, created lazily like _pzp_zstd_dctx */
static _Thread_local ZL_Compressor *_pzp_zl_compressor = NULL;
static _Thread_local ZL_CCtx       *_pzp_zl_cctx       = NULL;
static _Thread_local ZL_DCtx       *_pzp_zl_dctx       = NULL;
#endif

static inline void pzp_thread_cleanup(void)
{
    if (_pzp_zstd_dctx)
    {
        ZSTD_freeDCtx(_pzp_zstd_dctx);
        _pzp_zstd_dctx = NULL;
    }
#if PZP_USE_OPENZL
    ZL_Compressor_free(_pzp_zl_compressor); _pzp_zl_compressor = NULL;
    ZL_CCtx_free(_pzp_zl_cctx);             _pzp_zl_cctx       = NULL;
    ZL_DCtx_free(_pzp_zl_dctx);             _pzp_zl_dctx       = NULL;
#endif
}


#define NORMAL   "\033[0m"
#define BLACK   "\033[30m"      /* Black */
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define YELLOW  "\033[33m"      /* Yellow */

// Define flags using bitwise shift for clarity
typedef enum
{
    USE_COMPRESSION  = 1 << 0,  // 0001 — zstd entropy coding (always on)
    USE_RLE          = 1 << 1,  // 0010 — intra-frame delta filter before zstd
    USE_PALETTE      = 1 << 2,  // 0100 — per-channel palette indexing
    USE_INTER_DELTA  = 1 << 3,  // 1000 — inter-frame delta: store frame[N] - frame[N-1]
    USE_LZ4          = 1 << 4,  // 10000 — use LZ4 instead of ZSTD (faster decompress on ramdisk)
    USE_OPENZL       = 1 << 5   // 100000 — use OpenZL instead of ZSTD ( needs PZP_USE_OPENZL )
} PZPFlags;

/* ─── Channel groups ─────────────────────────────────────────────────────────
 * A "PZP1" frame stores, right after its 40-byte header, a table saying how its internal channels
 * are predicted and laid out:
 *   [ 1 byte group_count ][ group_count × 4 bytes { channels, sample_bits, predictor, reserved } ]
 * Groups take consecutive internal channels in order and together cover all of them. The pixel data
 * holds each group's block one after another:
 *   sample_bits 8  : `channels` byte channels, interleaved pixel by pixel; predictor NONE or LEFT
 *                    ( previous pixel of the same channel in raster order, the USE_RLE filter ).
 *   sample_bits 16 : channels == 2, the ( high, low ) bytes of one big-endian 16-bit sample;
 *                    predictor GRADIENT ( left + up - upleft, modulo 2^16 ), residuals zigzag coded
 *                    and stored as a high-byte plane followed by a low-byte plane.
 * Separating groups lets zstd/lz4 see each kind of data on its own, and a 16-bit group predicts the
 * real sample instead of its two bytes independently. The palette ( USE_PALETTE ) is applied to the
 * channels before prediction and is only allowed when every group is 8-bit. USE_RLE is not read for
 * "PZP1" frames: the table's predictors replace it.
 * Example - semantic label + 16-bit depth packed as 3 bytes: { {1,8,LEFT,0}, {2,16,GRADIENT,0} }. */
#define PZP_MAX_CHANNEL_GROUPS 8

typedef enum
{
    PZP_PREDICT_NONE     = 0,
    PZP_PREDICT_LEFT     = 1,
    PZP_PREDICT_GRADIENT = 2
} PZPPredictor;

typedef struct
{
    unsigned char channels;     /* internal byte channels in the group ( 2 for a 16-bit group ) */
    unsigned char sample_bits;  /* 8 or 16                                                        */
    unsigned char predictor;    /* PZPPredictor                                                    */
    unsigned char reserved;     /* 0                                                               */
} PZPChannelGroup;

/* Bit 31 of the 4-byte frame-prefix uint32 encodes the codec.
 * Bit 31 = 0: ZSTD (all existing files — backward compatible)
 * Bit 31 = 1: LZ4
 * Bits 0–30 hold the uncompressed payload size (max ~2 GB). */
#define PZP_CODEC_LZ4_FLAG  0x80000000u
#define PZP_CODEC_SIZE_MASK 0x7FFFFFFFu

/* With bit 31 = 0 the stream's own magic tells ZSTD and OpenZL apart: ZSTD frames start with
 * 0xFD2FB528, OpenZL frames with 0xD7B1A5C0 + format version ( low 6 bits, 27 in OpenZL v0.3.0 ).
 * Decoders without OpenZL support would hand an OpenZL frame to zstd, which rejects it. */
#define PZP_OPENZL_MAGIC_BASE 0xD7B1A5C0u
#define PZP_OPENZL_MAGIC_MASK 0xFFFFFFC0u

// ─── Container format constants ──────────────────────────────────────────────

#define PZP_CONTAINER_VERSION      1u

/* container_flags bits */
#define PZP_CONTAINER_HAS_METADATA (1u << 0)
#define PZP_CONTAINER_HAS_AUDIO    (1u << 1)

/* audio_format four-char tags (big-endian ASCII, matching convert_header) */
#define PZP_AUDIO_WAVE  0x57415645u   /* "WAVE" */
#define PZP_AUDIO_MPEG  0x4D504547u   /* "MPEG" */
#define PZP_AUDIO_OGG   0x4F474758u   /* "OGGX" */
#define PZP_AUDIO_FLAC  0x464C4143u   /* "FLAC" */

/*
 * PZP Container File Layout
 * ─────────────────────────
 * Offset 0                   : PZPContainerHeader (48 bytes, uncompressed)
 * Offset 48                  : Frame index — frame_count × PZPFrameEntry (16 bytes each)
 * Offset 48 + frame_count*16 : Frame data (each frame = [4-byte uncompressed_size][zstd stream])
 * metadata_offset            : Opaque metadata blob (if HAS_METADATA)
 * audio_offset               : Raw audio file bytes (if HAS_AUDIO)
 *
 * Detection: if first 4 bytes of file == convert_header("PZP0"), it is a container.
 */

/* 12 × uint32 = 48 bytes, written uncompressed at byte 0 of every PZP file */
typedef struct {
    unsigned int magic;            /* convert_header("PZP0")       */
    unsigned int version;          /* PZP_CONTAINER_VERSION = 1    */
    unsigned int container_flags;  /* PZP_CONTAINER_HAS_*          */
    unsigned int frame_count;      /* >= 1                         */
    unsigned int loop_count;       /* 0 = loop forever             */
    unsigned int metadata_offset;  /* abs file byte offset, 0=none */
    unsigned int metadata_bytes;   /* byte count of metadata blob  */
    unsigned int audio_offset;     /* abs file byte offset, 0=none */
    unsigned int audio_bytes;      /* byte count of audio blob     */
    unsigned int audio_format;     /* PZP_AUDIO_* tag or 0         */
    unsigned int header_checksum;  /* hash_checksum of slots 0–9   */
    unsigned int reserved;         /* 0                            */
} PZPContainerHeader;              /* 48 bytes                     */

/* 4 × uint32 = 16 bytes, one per frame in the index */
typedef struct {
    unsigned int frame_offset;    /* abs file byte offset to frame data */
    unsigned int compressed_size; /* byte count of frame data           */
    unsigned int delay_ms;        /* display duration; 0 = app default  */
    unsigned int reserved;        /* 0                                  */
} PZPFrameEntry;                  /* 16 bytes                           */

// ─────────────────────────────────────────────────────────────────────────────

static unsigned int convert_header(const char header[4])
{
    return ((unsigned int)header[0] << 24) |
           ((unsigned int)header[1] << 16) |
           ((unsigned int)header[2] << 8)  |
           ((unsigned int)header[3]);
}

/* fail() used to unconditionally exit() the whole process here on any internal error ( allocation
 * failure, a bad output path, a failed frame compression - all inside pzp_container_write() , i.e.
 * the WRITE path only ). That's wrong for a function embedded as a library ( pzp_lib.c's libpzp.so ,
 * loaded by the Python bindings, or any other host program ) : one caller's write failure ( disk full,
 * bad permissions, OOM ) would otherwise take the entire host process down with it, not just that one
 * call. fail() now just logs - pzp_container_write() ( the only caller ) returns 0 right after each
 * fail() call instead of relying on it to never return, and that propagates up through
 * pzp_compress_combined() to its callers ( pzp_lib.c, pzp.c ) as an ordinary 0/1 result. A CLI tool
 * that wants exit()-on-failure behavior ( like pzp.c ) gets it by checking that return value itself. */
static void fail(const char * message)
{
  fprintf(stderr,RED "PZP Fatal Error: %s\n" NORMAL,message);
}

static unsigned int hash_checksum(const void *data, size_t dataSize)
{
    const unsigned char *bytes = (const unsigned char *)data;
    unsigned int h1 = 0x12345678, h2 = 0x9ABCDEF0, h3 = 0xFEDCBA98, h4 = 0x87654321;

    while (dataSize >= 4)
    {
        h1 = (h1 ^ bytes[0]) * 31;
        h2 = (h2 ^ bytes[1]) * 37;
        h3 = (h3 ^ bytes[2]) * 41;
        h4 = (h4 ^ bytes[3]) * 43;
        bytes += 4;
        dataSize -= 4;
    }

    // Process remaining bytes
    if (dataSize > 0) h1 = (h1 ^ bytes[0]) * 31;
    if (dataSize > 1) h2 = (h2 ^ bytes[1]) * 37;
    if (dataSize > 2) h3 = (h3 ^ bytes[2]) * 41;

    // Final mix to spread entropy
    return (h1 ^ (h2 >> 3)) + (h3 ^ (h4 << 5));
}


// ─── Per-channel palette helpers ────────────────────────────────────────────

/* Build a sorted palette per channel from the split (planar) buffers, then
   re-encode each buffer in-place: pixel value → palette index (0-based).
   palette[ch][i] = the i-th unique value in channel ch (ascending order).
   counts[ch]     = number of unique values in channel ch (1-256).
   Returns the total serialised byte count for all palettes
   (sum over channels of: 1 byte count-field + counts[ch] bytes values). */
static unsigned int pzp_palette_build_and_encode(
        unsigned char **buffers, unsigned int pixels, unsigned int channels,
        unsigned char palette[8][256], unsigned int counts[8])
{
    unsigned int total_bytes = 0;
    for (unsigned int ch = 0; ch < channels; ch++)
    {
        unsigned char present[256] = {0};
        for (unsigned int i = 0; i < pixels; i++) present[buffers[ch][i]] = 1;

        unsigned char inv[256];
        unsigned int cnt = 0;
        for (unsigned int v = 0; v < 256; v++)
            if (present[v]) { inv[v] = (unsigned char)cnt; palette[ch][cnt++] = (unsigned char)v; }

        counts[ch] = cnt;
        total_bytes += 1 + cnt; /* 1 byte (count-1 field) + cnt bytes values */

        for (unsigned int i = 0; i < pixels; i++) buffers[ch][i] = inv[buffers[ch][i]];
    }
    return total_bytes;
}

/* Serialize palette data to dst. Returns bytes written. */
static unsigned int pzp_palette_write(
        unsigned char *dst, unsigned int channels,
        unsigned char palette[8][256], unsigned int counts[8])
{
    unsigned int off = 0;
    for (unsigned int ch = 0; ch < channels; ch++)
    {
        dst[off++] = (unsigned char)(counts[ch] - 1); /* stored as count-1 so 256 fits in 1 byte */
        memcpy(dst + off, palette[ch], counts[ch]);
        off += counts[ch];
    }
    return off;
}

/* Parse palette data from src ( src_bytes long ). Returns bytes consumed, or 0 if the palette
   would run past src_bytes. */
static unsigned int pzp_palette_read(
        const unsigned char *src, unsigned int src_bytes, unsigned int channels,
        unsigned char palette[8][256], unsigned int counts[8])
{
    unsigned int off = 0;
    for (unsigned int ch = 0; ch < channels; ch++)
    {
        if (off >= src_bytes) return 0;
        counts[ch] = (unsigned int)src[off++] + 1;
        if (counts[ch] > src_bytes - off) return 0;
        memcpy(palette[ch], src + off, counts[ch]);
        off += counts[ch];
    }
    return off;
}

/* In-place palette lookup on interleaved pixel data: index → original value. */
static void pzp_palette_apply(
        unsigned char *data, unsigned int pixels, unsigned int channels,
        unsigned char palette[8][256])
{
    for (unsigned int i = 0; i < pixels; i++)
        for (unsigned int ch = 0; ch < channels; ch++)
            data[i * channels + ch] = palette[ch][data[i * channels + ch]];
}

// ─── Channel groups ─────────────────────────────────────────────────────────

/* Returns 1 if groups[] is a table this version can encode/decode for a frame of channels internal
   channels at bpp_int bits ( see PZPChannelGroup ), 0 otherwise. */
static int pzp_channel_groups_valid(
        const PZPChannelGroup *groups, unsigned int group_count,
        unsigned int channels, unsigned int bpp_int, unsigned int configuration)
{
    if ((group_count == 0) || (group_count > PZP_MAX_CHANNEL_GROUPS) || (bpp_int != 8)) return 0;
    unsigned int covered = 0;
    for (unsigned int g = 0; g < group_count; g++)
    {
        const PZPChannelGroup *gr = &groups[g];
        if ((gr->channels == 0) || (gr->reserved != 0)) return 0;
        if (gr->sample_bits == 8)
        {
            if ((gr->predictor != PZP_PREDICT_NONE) && (gr->predictor != PZP_PREDICT_LEFT)) return 0;
        }
        else if (gr->sample_bits == 16)
        {
            if ((gr->channels != 2) || (gr->predictor != PZP_PREDICT_GRADIENT) ||
                (configuration & USE_PALETTE)) return 0;
        }
        else return 0;
        covered += gr->channels;
    }
    return covered == channels;
}

/* 16-bit gradient predictor ( left + up - upleft ) with zigzag residuals. hi/lo are the big-endian
   byte planes of the samples. Computed as the vertical difference e = d - up followed by
   r = e - e_left, which is the same prediction and lets the decoder undo it with one running sum
   per row plus the row above ( rows and columns outside the image count as 0 ). */
static void pzp_gradient16_encode(
        const unsigned char *hi, const unsigned char *lo,
        unsigned char *res_hi, unsigned char *res_lo,
        unsigned int width, unsigned int height)
{
    for (unsigned int y = 0; y < height; y++)
    {
        unsigned short e_left = 0;
        for (unsigned int x = 0; x < width; x++)
        {
            unsigned int   i  = y * width + x;
            unsigned short d  = (unsigned short)((hi[i] << 8) | lo[i]);
            unsigned short up = y ? (unsigned short)((hi[i - width] << 8) | lo[i - width]) : 0;
            unsigned short e  = (unsigned short)(d - up);
            short          r  = (short)(unsigned short)(e - e_left);
            unsigned short z  = (unsigned short)(((unsigned short)r << 1) ^ (unsigned short)(r >> 15));
            res_hi[i] = (unsigned char)(z >> 8);
            res_lo[i] = (unsigned char)(z & 0xFF);
            e_left = e;
        }
    }
}

/* Inverse of pzp_gradient16_encode: residual planes res_hi/res_lo → sample planes hi/lo. */
static void pzp_gradient16_decode(
        const unsigned char *res_hi, const unsigned char *res_lo,
        unsigned char *hi, unsigned char *lo,
        unsigned int width, unsigned int height)
{
    for (unsigned int y = 0; y < height; y++)
    {
        unsigned short e = 0;
        unsigned int   x = 0;
        #if INTEL_OPTIMIZATIONS
        /* 16 samples per step: unzigzag, running sum along the row ( Kogge-Stone within each
           128-bit lane, then lane 0's last sum into lane 1, then the previous step's last sum ),
           add the row above, split back into the byte planes. */
        const __m256i one        = _mm256_set1_epi16(1);
        const __m256i low_byte   = _mm256_set1_epi16(0xFF);
        const __m256i last_word  = _mm256_setr_epi8(14,15,14,15,14,15,14,15,14,15,14,15,14,15,14,15,
                                                    14,15,14,15,14,15,14,15,14,15,14,15,14,15,14,15);
        __m256i carry = _mm256_setzero_si256();
        for (; x + 16 <= width; x += 16)
        {
            unsigned int i = y * width + x;
            __m256i z = _mm256_or_si256(
                    _mm256_cvtepu8_epi16(_mm_loadu_si128((const __m128i *)(res_lo + i))),
                    _mm256_slli_epi16(_mm256_cvtepu8_epi16(_mm_loadu_si128((const __m128i *)(res_hi + i))), 8));
            __m256i r = _mm256_xor_si256(_mm256_srli_epi16(z, 1),
                                         _mm256_sub_epi16(_mm256_setzero_si256(), _mm256_and_si256(z, one)));
            r = _mm256_add_epi16(r, _mm256_slli_si256(r, 2));
            r = _mm256_add_epi16(r, _mm256_slli_si256(r, 4));
            r = _mm256_add_epi16(r, _mm256_slli_si256(r, 8));
            r = _mm256_add_epi16(r, _mm256_shuffle_epi8(_mm256_permute2x128_si256(r, r, 0x08), last_word));
            __m256i ev = _mm256_add_epi16(r, carry);
            carry = _mm256_shuffle_epi8(_mm256_permute2x128_si256(ev, ev, 0x11), last_word);

            __m256i up = _mm256_setzero_si256();
            if (y)
                up = _mm256_or_si256(
                        _mm256_cvtepu8_epi16(_mm_loadu_si128((const __m128i *)(lo + i - width))),
                        _mm256_slli_epi16(_mm256_cvtepu8_epi16(_mm_loadu_si128((const __m128i *)(hi + i - width))), 8));
            __m256i d = _mm256_add_epi16(up, ev);
            /* packus per lane → [lo0-7 hi0-7 | lo8-15 hi8-15], reorder to [lo0-15 | hi0-15] */
            __m256i packed = _mm256_permute4x64_epi64(
                    _mm256_packus_epi16(_mm256_and_si256(d, low_byte), _mm256_srli_epi16(d, 8)), 0xD8);
            _mm_storeu_si128((__m128i *)(lo + i), _mm256_castsi256_si128(packed));
            _mm_storeu_si128((__m128i *)(hi + i), _mm256_extracti128_si256(packed, 1));
        }
        e = (unsigned short)_mm256_extract_epi16(carry, 0);
        #endif
        for (; x < width; x++)
        {
            unsigned int   i  = y * width + x;
            unsigned short z  = (unsigned short)((res_hi[i] << 8) | res_lo[i]);
            e += (unsigned short)((z >> 1) ^ (unsigned short)-(z & 1));
            unsigned short up = y ? (unsigned short)((hi[i - width] << 8) | lo[i - width]) : 0;
            unsigned short d  = (unsigned short)(up + e);
            hi[i] = (unsigned char)(d >> 8);
            lo[i] = (unsigned char)(d & 0xFF);
        }
    }
}

// ────────────────────────────────────────────────────────────────────────────

static void * pzp_read_file_to_memory(const char *filename, size_t *fileSize)
{
    FILE *fp = fopen(filename, "rb");
    if (!fp)
       {
        fprintf(stderr,"Failed to open file");
        return NULL;
       }

    if (fseek(fp, 0, SEEK_END) != 0)
       {
        fprintf(stderr,"Failed to seek file");
        fclose(fp);
        return NULL;
       }

    long file_size = ftell(fp);
    if (file_size < 0)
       {
        fprintf(stderr,"Failed to tell file size");
        fclose(fp);
        return NULL;
       }
    rewind(fp);

    void *buffer = malloc(file_size);
    if (!buffer)
      {
        fprintf(stderr,"Failed to allocate memory");
        fclose(fp);
        return NULL;
       }

    size_t read_size = fread(buffer, 1, file_size, fp);
    if (read_size != (size_t)file_size)
       {
        fprintf(stderr,"Failed to read file completely");
        free(buffer);
        fclose(fp);
        return NULL;
       }

    fclose(fp);
    if (fileSize) *fileSize = read_size;
    return buffer;
}



static void pzp_split_channels(const unsigned char *image, unsigned char **buffers, int num_buffers, int WIDTH, int HEIGHT)
{
    int total_size = WIDTH * HEIGHT;

    // Split channels
    for (int i = 0; i < total_size; i++)
    {
        for (int ch = 0; ch < num_buffers; ch++)
        {
            buffers[ch][i] = image[i * num_buffers + ch];
        }
    }
}

static void pzp_RLE_filter(unsigned char **buffers, int num_buffers, int WIDTH, int HEIGHT)
{
    int total_size = WIDTH * HEIGHT;

    // Apply left-pixel delta filtering
    for (int i = total_size - 1; i > 0; i--)
    {
        for (int ch = 0; ch < num_buffers; ch++)
        {
            buffers[ch][i] -= buffers[ch][i - 1];
        }
    }
}

//-----------------------------------------------------------------------------------------------
// ─── Frame compression to memory ────────────────────────────────────────────

/* Compress one image frame to a heap buffer.
 *
 * Returns a malloc'd block:
 *   [ 4 bytes uint32 uncompressed_size ][ N bytes zstd stream ]
 * *out_size is set to the total byte count.  Caller must free() the result.
 * Returns NULL on failure.
 *
 * groups/group_count: the channel group table written into the frame ( see PZPChannelGroup ).
 * NULL / 0 = one 8-bit group over every channel, predicted with LEFT if USE_RLE is set.
 *
 * NOTE: modifies buffers[] in-place (palette encoding then prediction).
 */
/* 1 if a bit-31-clear frame stream starts with the OpenZL magic ( see PZP_OPENZL_MAGIC_BASE ) */
static int pzp_is_openzl_stream(const void *stream, size_t size)
{
    unsigned int magic;
    if (size < sizeof(unsigned int)) return 0;
    memcpy(&magic, stream, sizeof(unsigned int));
    return (magic & PZP_OPENZL_MAGIC_MASK) == PZP_OPENZL_MAGIC_BASE;
}

#if PZP_USE_OPENZL
/* The payload goes in as a numeric stream of bytes, split by sparse_num_auto into runs of its most
 * common value plus literals, both sent to the generic graph. On COCO segmentation maps this is
 * ~15% smaller than zstd -19 and decodes faster ( scripts/openzl_bench ). Checksums are off, as
 * for the zstd frames pzp writes. Returns the compressed size, 0 on failure. */
static size_t pzp_openzl_compress(void *dst, size_t dst_capacity, const void *src, size_t src_size)
{
    if (!_pzp_zl_compressor)
    {
        ZL_Compressor *c = ZL_Compressor_create();
        ZL_GraphID start = c ? ZL_Compressor_registerStaticGraph_fromNode(c, ZL_NODE_SPARSE_NUM_AUTO,
                               ZL_GRAPHLIST(ZL_GRAPH_COMPRESS_GENERIC, ZL_GRAPH_COMPRESS_GENERIC)) : ZL_GRAPH_ILLEGAL;
        if (!c || !ZL_GraphID_isValid(start) ||
            ZL_isError(ZL_Compressor_setParameter(c, ZL_CParam_formatVersion, ZL_MAX_FORMAT_VERSION)) ||
            ZL_isError(ZL_Compressor_setParameter(c, ZL_CParam_compressionLevel, ZL_COMPRESSIONLEVEL_DEFAULT)) ||
            ZL_isError(ZL_Compressor_setParameter(c, ZL_CParam_compressedChecksum, ZL_TernaryParam_disable)) ||
            ZL_isError(ZL_Compressor_setParameter(c, ZL_CParam_contentChecksum, ZL_TernaryParam_disable)) ||
            ZL_isError(ZL_Compressor_selectStartingGraphID(c, start)))
        {
            fprintf(stderr, "pzp: could not set up the OpenZL compressor\n");
            ZL_Compressor_free(c);
            return 0;
        }
        _pzp_zl_compressor = c;
    }
    if (!_pzp_zl_cctx && !(_pzp_zl_cctx = ZL_CCtx_create())) return 0;
    if (ZL_isError(ZL_CCtx_refCompressor(_pzp_zl_cctx, _pzp_zl_compressor))) return 0;

    ZL_TypedRef *input = ZL_TypedRef_createNumeric(src, 1, src_size);
    if (!input) return 0;
    ZL_Report r = ZL_CCtx_compressTypedRef(_pzp_zl_cctx, dst, dst_capacity, input);
    ZL_TypedRef_free(input);
    if (ZL_isError(r))
    {
        fprintf(stderr, "pzp: openzl error: %s\n", ZL_CCtx_getErrorContextString(_pzp_zl_cctx, r));
        return 0;
    }
    return ZL_validResult(r);
}

/* Returns the decompressed size, 0 on failure. */
static size_t pzp_openzl_decompress(void *dst, size_t dst_capacity, const void *src, size_t src_size)
{
    if (!_pzp_zl_dctx)
    {
        if (!(_pzp_zl_dctx = ZL_DCtx_create())) return 0;
        ZL_DCtx_setParameter(_pzp_zl_dctx, ZL_DParam_stickyParameters, 1);
        ZL_DCtx_setParameter(_pzp_zl_dctx, ZL_DParam_checkCompressedChecksum, ZL_TernaryParam_disable);
        ZL_DCtx_setParameter(_pzp_zl_dctx, ZL_DParam_checkContentChecksum, ZL_TernaryParam_disable);
    }
    ZL_OutputInfo info;
    ZL_Report r = ZL_DCtx_decompressTyped(_pzp_zl_dctx, &info, dst, dst_capacity, src, src_size);
    if (ZL_isError(r))
    {
        fprintf(stderr, "OpenZL decompression error: %s\n", ZL_DCtx_getErrorContextString(_pzp_zl_dctx, r));
        return 0;
    }
    return (size_t)info.decompressedByteSize;
}
#endif // PZP_USE_OPENZL

static unsigned char *pzp_compress_frame_to_memory(
        unsigned char **buffers,
        unsigned int width,    unsigned int height,
        unsigned int bpp_ext,  unsigned int ch_ext,
        unsigned int bpp_int,  unsigned int ch_int,
        unsigned int configuration,
        const PZPChannelGroup *groups, unsigned int group_count,
        size_t *out_size)
{
    /* ── channel groups ── */
    PZPChannelGroup default_group = { (unsigned char)ch_int, 8,
                                      (configuration & USE_RLE) ? PZP_PREDICT_LEFT : PZP_PREDICT_NONE, 0 };
    if (!groups || group_count == 0) { groups = &default_group; group_count = 1; }
    if ((ch_int > 255) || ((configuration & USE_PALETTE) && (ch_int > 8)) ||   /* palettes hold 8 channels */
        !pzp_channel_groups_valid(groups, group_count, ch_int, bpp_int, configuration))
    {
        fprintf(stderr, "pzp: unsupported channel group table for %u channels @ %u bit (configuration %u)\n",
                ch_int, bpp_int, configuration);
        return NULL;
    }
    if (configuration & USE_OPENZL)
    {
        if (!PZP_USE_OPENZL)
        {
            fprintf(stderr, "pzp: USE_OPENZL needs a build with PZP_USE_OPENZL=1\n");
            return NULL;
        }
        if (configuration & USE_LZ4)
        {
            fprintf(stderr, "pzp: USE_OPENZL and USE_LZ4 cannot be combined\n");
            return NULL;
        }
    }

    /* ── palette encoding ── */
    unsigned char palette[8][256];
    unsigned int  palette_counts[8];
    unsigned int  paletteDataBytes = 0;

    if (configuration & USE_PALETTE)
    {
        paletteDataBytes = pzp_palette_build_and_encode(
                buffers, width * height, ch_int, palette, palette_counts);
        #if PZP_VERBOSE
        fprintf(stderr, "Palette mode: %u channels, palette data %u bytes\n",
                ch_int, paletteDataBytes);
        for (unsigned int ch = 0; ch < ch_int; ch++)
            fprintf(stderr, "  ch%u: %u unique values\n", ch, palette_counts[ch]);
        #endif
    }

    /* ── build uncompressed payload ── */
    unsigned int groupDataBytes  = 1 + group_count * (unsigned int)sizeof(PZPChannelGroup);
    unsigned int pixel_data_size = width * height * (bpp_int / 8) * ch_int;
    unsigned int payload_size    = (unsigned int)headerSize + groupDataBytes + paletteDataBytes + pixel_data_size;

    unsigned char *payload = (unsigned char *)malloc(payload_size);
    if (!payload) return NULL;

    /* inner frame header (10 × uint32 = 40 bytes) */
    unsigned int *h = (unsigned int *)payload;
    h[0] = convert_header(pzp_frame_header_groups);
    h[1] = bpp_ext;
    h[2] = ch_ext;
    h[3] = width;
    h[4] = height;
    h[5] = bpp_int;
    h[6] = ch_int;
    /* h[7] = checksum, filled after the pixel data */
    h[8] = configuration;
    h[9] = paletteDataBytes;

    /* channel group table, then palette prefix */
    unsigned char *write_ptr = payload + headerSize;
    *write_ptr++ = (unsigned char)group_count;
    memcpy(write_ptr, groups, group_count * sizeof(PZPChannelGroup));
    write_ptr += group_count * sizeof(PZPChannelGroup);
    if (paletteDataBytes > 0)
    {
        pzp_palette_write(write_ptr, ch_int, palette, palette_counts);
        write_ptr += paletteDataBytes;
    }

    /* each group's block in turn: predict, then lay out ( see PZPChannelGroup ) */
    unsigned int   pixels = width * height;
    unsigned char *dst    = write_ptr;
    unsigned char **gbuf  = buffers;
    for (unsigned int g = 0; g < group_count; g++)
    {
        unsigned int c = groups[g].channels;
        if (groups[g].sample_bits == 16)
            pzp_gradient16_encode(gbuf[0], gbuf[1], dst, dst + pixels, width, height);
        else
        {
            if (groups[g].predictor == PZP_PREDICT_LEFT)
                pzp_RLE_filter(gbuf, c, width, height);
            for (unsigned int i = 0; i < pixels; i++)
                for (unsigned int ch = 0; ch < c; ch++)
                    dst[i * c + ch] = gbuf[ch][i];
        }
        dst  += (size_t)pixels * c;
        gbuf += c;
    }

    h[7] = hash_checksum(write_ptr, pixel_data_size);

    /* ── compress (LZ4, OpenZL or ZSTD) ── */
    /* result layout: [4-byte prefix][compressed bytes]
     * prefix bit 31 = codec: 0=ZSTD or OpenZL ( told apart by stream magic ), 1=LZ4  (PZP_CODEC_LZ4_FLAG)
     * prefix bits 0-30 = uncompressed payload size (PZP_CODEC_SIZE_MASK) */
    size_t max_comp;
    unsigned char *frame_buf;
    size_t comp_size;

    if (configuration & USE_LZ4)
    {
        max_comp  = (size_t)LZ4_compressBound((int)payload_size);
        frame_buf = (unsigned char *)malloc(sizeof(unsigned int) + max_comp);
        if (!frame_buf) { free(payload); return NULL; }

        int lz4_written = LZ4_compress_default(
                (const char *)payload, (char *)(frame_buf + sizeof(unsigned int)),
                (int)payload_size, (int)max_comp);
        free(payload);

        if (lz4_written <= 0)
        {
            fprintf(stderr, "pzp: lz4 compression failed\n");
            free(frame_buf);
            return NULL;
        }
        comp_size = (size_t)lz4_written;
        unsigned int prefix = (unsigned int)payload_size | PZP_CODEC_LZ4_FLAG;
        memcpy(frame_buf, &prefix, sizeof(unsigned int));
    }
#if PZP_USE_OPENZL
    else if (configuration & USE_OPENZL)
    {
        /* ZL_compressBound() covers a single serial input; the numeric input's split streams can
           carry a little more framing, hence the slack */
        max_comp  = ZL_compressBound(payload_size) + 4096;
        frame_buf = (unsigned char *)malloc(sizeof(unsigned int) + max_comp);
        if (!frame_buf) { free(payload); return NULL; }

        comp_size = pzp_openzl_compress(frame_buf + sizeof(unsigned int), max_comp, payload, payload_size);
        free(payload);

        if (comp_size == 0)
        {
            free(frame_buf);
            return NULL;
        }
        memcpy(frame_buf, &payload_size, sizeof(unsigned int));
    }
#endif
    else
    {
        int    zstd_level = (configuration & USE_PALETTE) ? 19 : 1;
        max_comp  = ZSTD_compressBound(payload_size);
        frame_buf = (unsigned char *)malloc(sizeof(unsigned int) + max_comp);
        if (!frame_buf) { free(payload); return NULL; }

        comp_size = ZSTD_compress(
                frame_buf + sizeof(unsigned int), max_comp,
                payload, payload_size, zstd_level);
        free(payload);

        if (ZSTD_isError(comp_size))
        {
            fprintf(stderr, "pzp: zstd error: %s\n", ZSTD_getErrorName(comp_size));
            free(frame_buf);
            return NULL;
        }
        memcpy(frame_buf, &payload_size, sizeof(unsigned int));
    }

    *out_size = sizeof(unsigned int) + comp_size;

    #if PZP_VERBOSE
    {
        const char *mode_str = "";
        if ((configuration & USE_INTER_DELTA) && (configuration & USE_RLE))  mode_str = " rle+idelta";
        else if (configuration & USE_INTER_DELTA)                             mode_str = " idelta";
        else if (configuration & USE_RLE)                                     mode_str = " rle";
        fprintf(stderr,
            "  compress: %u B → %zu B  ratio=%.2f×%s%s%s\n",
            payload_size, comp_size,
            (float)payload_size / (float)comp_size,
            (configuration & USE_OPENZL)  ? " openzl"  : "",
            (configuration & USE_PALETTE) ? " palette" : "",
            mode_str);
    }
    #endif

    unsigned char *shrunk = (unsigned char *)realloc(frame_buf, *out_size);
    return shrunk ? shrunk : frame_buf;
}

//-----------------------------------------------------------------------------------------------
// ─── Container write ─────────────────────────────────────────────────────────

/*
 * pzp_container_write — write a PZP container file with N frames.
 *
 * all_buffers[f]  : array of planar channel pointers for frame f
 * frame_count     : number of frames (>= 1)
 * widths/heights  : per-frame pixel dimensions
 * bpp_exts/ch_exts: per-frame external bpp and channel count
 * bpp_ints/ch_ints: per-frame internal bpp and channel count
 * configurations  : per-frame PZPFlags bitfield
 * groups/group_count : channel group table used for every frame ( NULL / 0 = default, see
 *                   pzp_compress_frame_to_memory )
 * delay_ms_arr    : per-frame display duration in ms (NULL → all 0)
 * loop_count      : 0 = loop forever, N = play N times
 * metadata        : opaque metadata bytes (NULL = absent)
 * metadata_bytes  : byte count (0 = absent)
 * audio           : raw audio file bytes (NULL = absent)
 * audio_bytes     : byte count (0 = absent)
 * audio_format    : PZP_AUDIO_* tag or 0
 *
 * NOTE: modifies each frame's buffers in-place.
 */
/* Returns 1 on success, 0 on failure ( bad output path, allocation failure, a frame that failed to
 * compress ). Never exits/longjmps - see the comment on fail() above. */
static int pzp_container_write(
        const char    *output_filename,
        unsigned char ***all_buffers,
        unsigned int   frame_count,
        unsigned int  *widths,
        unsigned int  *heights,
        unsigned int  *bpp_exts,
        unsigned int  *ch_exts,
        unsigned int  *bpp_ints,
        unsigned int  *ch_ints,
        unsigned int  *configurations,
        const PZPChannelGroup *groups, unsigned int group_count,
        unsigned int  *delay_ms_arr,
        unsigned int   loop_count,
        const unsigned char *metadata, unsigned int metadata_bytes,
        const unsigned char *audio,    unsigned int audio_bytes,
        unsigned int   audio_format)
{
    /* ── encode all frames to heap buffers ── */
    unsigned char **frame_bufs    = (unsigned char **)malloc(frame_count * sizeof(unsigned char *));
    size_t         *frame_sizes   = (size_t *)        malloc(frame_count * sizeof(size_t));
    unsigned int   *frame_offsets = (unsigned int *)  malloc(frame_count * sizeof(unsigned int));

    if (!frame_bufs || !frame_sizes || !frame_offsets)
    {
        free(frame_bufs); free(frame_sizes); free(frame_offsets);
        fail("pzp_container_write: allocation failed");
        return 0;
    }

    /* Reference buffers for USE_INTER_DELTA: prev_orig[c] holds the ORIGINAL
       planar channel data of frame (f-1), saved before any in-place transforms. */
    unsigned char **prev_orig   = NULL;
    unsigned int    prev_ch_int = 0, prev_w = 0, prev_h = 0;

#if PZP_VERBOSE
    /* Accumulators for the end-of-container summary. */
    size_t       _vb_key_comp = 0,   _vb_dlt_comp = 0;
    unsigned long _vb_key_raw = 0,   _vb_dlt_raw  = 0;
    unsigned int  _vb_n_key   = 0,   _vb_n_dlt    = 0;
    fprintf(stderr, "\n── PZP encode: %u frame(s) ──────────────────────────────\n",
            frame_count);
#endif

    for (unsigned int f = 0; f < frame_count; f++)
    {
        unsigned int cfg    = configurations[f];
        unsigned int ch_int = ch_ints[f];
        unsigned int w      = widths[f], h = heights[f];
        unsigned int pixels = w * h;

        /* Frame 0 is always a keyframe. */
        if (f == 0) cfg &= ~(unsigned int)USE_INTER_DELTA;

        /* Delta only valid when previous frame exists with identical layout. */
        int do_delta = (f > 0)
                    && (cfg & USE_INTER_DELTA)
                    && (prev_orig   != NULL)
                    && (prev_ch_int == ch_int)
                    && (prev_w == w) && (prev_h == h);
        if (!do_delta) cfg &= ~(unsigned int)USE_INTER_DELTA;

        /* Copy the CURRENT original channel data before any modification so it
           can serve as the reference for the next frame - only if that frame
           asks for a delta ( never for single images or the last frame ). */
        int need_orig = (f + 1 < frame_count) && (configurations[f + 1] & USE_INTER_DELTA);
        unsigned char **cur_orig = need_orig ? (unsigned char **)calloc(ch_int, sizeof(unsigned char *)) : NULL;
        int orig_ok = (cur_orig != NULL);
        if (orig_ok)
        {
            for (unsigned int c = 0; c < ch_int && orig_ok; c++)
            {
                cur_orig[c] = (unsigned char *)malloc(pixels);
                if (cur_orig[c]) memcpy(cur_orig[c], all_buffers[f][c], pixels);
                else             orig_ok = 0;
            }
        }
        if (!orig_ok && cur_orig)
        {
            for (unsigned int c = 0; c < ch_int; c++) free(cur_orig[c]);
            free(cur_orig);
            cur_orig = NULL;
        }

        /* Subtract previous frame's channel data in-place (wrapping byte math). */
        if (do_delta)
        {
            for (unsigned int c = 0; c < ch_int; c++)
                for (unsigned int px = 0; px < pixels; px++)
                    all_buffers[f][c][px] -= prev_orig[c][px];
        }

#if PZP_VERBOSE
        /* ── Per-frame header ── */
        fprintf(stderr, "Frame %u/%u [%s] %ux%ux%u@%ubit\n",
                f, frame_count - 1,
                do_delta ? "DELTA   " : "KEYFRAME",
                w, h, ch_exts[f], bpp_exts[f]);

        /* ── Delta statistics (computed on post-subtraction, pre-palette buffers) ── */
        if (do_delta)
        {
            unsigned long zeros = 0, small5 = 0;
            double sum_abs = 0.0;
            int max_abs = 0;
            unsigned long total_s = (unsigned long)ch_int * pixels;
            for (unsigned int c = 0; c < ch_int; c++)
                for (unsigned int px = 0; px < pixels; px++)
                {
                    int d = (int)(signed char)all_buffers[f][c][px];
                    if (d == 0)        zeros++;
                    if (d >= -5 && d <= 5) small5++;
                    int a = d < 0 ? -d : d;
                    sum_abs += a;
                    if (a > max_abs) max_abs = a;
                }
            fprintf(stderr,
                "  delta stats: unchanged=%.1f%%  near-zero(±5)=%.1f%%"
                "  MAD=%.2f  max|Δ|=%d\n",
                100.0 * (double)zeros  / (double)total_s,
                100.0 * (double)small5 / (double)total_s,
                sum_abs / (double)total_s,
                max_abs);
        }
#endif

        frame_bufs[f] = pzp_compress_frame_to_memory(
                all_buffers[f],
                widths[f], heights[f],
                bpp_exts[f], ch_exts[f],
                bpp_ints[f], ch_ints[f],
                cfg,
                groups, group_count,
                &frame_sizes[f]);

        /* Advance the reference window. */
        if (prev_orig)
        {
            for (unsigned int c = 0; c < prev_ch_int; c++) free(prev_orig[c]);
            free(prev_orig);
        }
        prev_orig   = cur_orig;
        prev_ch_int = ch_int;
        prev_w = w; prev_h = h;

        if (!frame_bufs[f])
        {
            for (unsigned int j = 0; j < f; j++) free(frame_bufs[j]);
            if (prev_orig)
            {
                for (unsigned int c = 0; c < prev_ch_int; c++) free(prev_orig[c]);
                free(prev_orig);
            }
            free(frame_bufs); free(frame_sizes); free(frame_offsets);
            fail("pzp_container_write: frame compression failed");
            return 0;
        }

#if PZP_VERBOSE
        {
            unsigned long raw_b = (unsigned long)ch_int * pixels;
            if (do_delta) { _vb_dlt_comp += frame_sizes[f]; _vb_dlt_raw += raw_b; _vb_n_dlt++; }
            else          { _vb_key_comp += frame_sizes[f]; _vb_key_raw += raw_b; _vb_n_key++; }
        }
#endif
    }

    if (prev_orig)
    {
        for (unsigned int c = 0; c < prev_ch_int; c++) free(prev_orig[c]);
        free(prev_orig);
    }

#if PZP_VERBOSE
    /* ── End-of-container summary ── */
    fprintf(stderr, "\n── Encoding summary ─────────────────────────────────────\n");
    if (_vb_n_key > 0)
        fprintf(stderr, "  Keyframes    : %u  compressed %zu B  avg %.0f B/frame  ratio %.2f×\n",
                _vb_n_key, _vb_key_comp,
                (double)_vb_key_comp / _vb_n_key,
                (double)_vb_key_raw  / (double)_vb_key_comp);
    if (_vb_n_dlt > 0)
        fprintf(stderr, "  Delta frames : %u  compressed %zu B  avg %.0f B/frame  ratio %.2f×\n",
                _vb_n_dlt, _vb_dlt_comp,
                (double)_vb_dlt_comp / _vb_n_dlt,
                (double)_vb_dlt_raw  / (double)_vb_dlt_comp);
    if (_vb_n_key > 0 && _vb_n_dlt > 0)
    {
        double avg_key = (double)_vb_key_comp / _vb_n_key;
        double avg_dlt = (double)_vb_dlt_comp / _vb_n_dlt;
        double ratio   = avg_dlt / avg_key;
        fprintf(stderr,
            "  Delta vs keyframe avg size: %.2f×  → delta is %s\n",
            ratio,
            ratio < 0.95 ? "BETTER  ✓" :
            ratio < 1.05 ? "roughly equal" :
                           "WORSE  ✗  (consider --no-delta for this content)");
    }
    fprintf(stderr, "─────────────────────────────────────────────────────────\n");
#endif

    /* ── compute absolute byte offsets ── */
    unsigned int idx_bytes   = frame_count * (unsigned int)frameEntrySize;
    frame_offsets[0]         = (unsigned int)containerHeaderSize + idx_bytes;
    for (unsigned int f = 1; f < frame_count; f++)
        frame_offsets[f] = frame_offsets[f - 1] + (unsigned int)frame_sizes[f - 1];

    unsigned int data_end = frame_offsets[frame_count - 1] + (unsigned int)frame_sizes[frame_count - 1];

    unsigned int meta_offset  = (metadata && metadata_bytes > 0) ? data_end : 0;
    unsigned int audio_offset = 0;
    if (audio && audio_bytes > 0)
        audio_offset = meta_offset ? (meta_offset + metadata_bytes) : data_end;

    /* ── build container header ── */
    unsigned int flags = 0;
    if (metadata && metadata_bytes > 0) flags |= PZP_CONTAINER_HAS_METADATA;
    if (audio    && audio_bytes    > 0) flags |= PZP_CONTAINER_HAS_AUDIO;

    PZPContainerHeader hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.magic           = convert_header(pzp_header);
    hdr.version         = PZP_CONTAINER_VERSION;
    hdr.container_flags = flags;
    hdr.frame_count     = frame_count;
    hdr.loop_count      = loop_count;
    hdr.metadata_offset = meta_offset;
    hdr.metadata_bytes  = (metadata && metadata_bytes > 0) ? metadata_bytes : 0;
    hdr.audio_offset    = audio_offset;
    hdr.audio_bytes     = (audio && audio_bytes > 0) ? audio_bytes : 0;
    hdr.audio_format    = audio_format;
    hdr.header_checksum = hash_checksum(&hdr, sizeof(unsigned int) * 10);
    hdr.reserved        = 0;

    /* ── write file ── */
    FILE *out = fopen(output_filename, "wb");
    if (!out)
    {
        for (unsigned int f = 0; f < frame_count; f++) free(frame_bufs[f]);
        free(frame_bufs); free(frame_sizes); free(frame_offsets);
        fail("pzp_container_write: could not open output file");
        return 0;
    }

    int ok = (fwrite(&hdr, sizeof(unsigned int), 12, out) == 12);

    for (unsigned int f = 0; f < frame_count; f++)
    {
        PZPFrameEntry entry;
        entry.frame_offset    = frame_offsets[f];
        entry.compressed_size = (unsigned int)frame_sizes[f];
        entry.delay_ms        = delay_ms_arr ? delay_ms_arr[f] : 0;
        entry.reserved        = 0;
        ok = ok && (fwrite(&entry, sizeof(unsigned int), 4, out) == 4);
    }

    for (unsigned int f = 0; f < frame_count; f++)
    {
        ok = ok && (fwrite(frame_bufs[f], 1, frame_sizes[f], out) == frame_sizes[f]);
        free(frame_bufs[f]);
    }

    if (metadata && metadata_bytes > 0)
        ok = ok && (fwrite(metadata, 1, metadata_bytes, out) == metadata_bytes);

    if (audio && audio_bytes > 0)
        ok = ok && (fwrite(audio, 1, audio_bytes, out) == audio_bytes);

    ok = (fclose(out) == 0) && ok;   /* a full disk often only shows up here */
    free(frame_bufs);
    free(frame_sizes);
    free(frame_offsets);
    if (!ok) fail("pzp_container_write: writing the output file failed");
    return ok;
}

//-----------------------------------------------------------------------------------------------
// ─── Container attach (re-wrap without recompressing frames) ─────────────────

/*
 * Copy an existing container to a new file, adding or replacing metadata/audio.
 * Frame data is copied verbatim — no recompression.
 * Returns 1 on success, 0 on failure.
 */
static int pzp_container_attach(
        const char *input_filename,
        const char *output_filename,
        const unsigned char *metadata, unsigned int metadata_bytes,
        const unsigned char *audio,    unsigned int audio_bytes,
        unsigned int audio_format)
{
    size_t file_size = 0;
    void  *file_data = pzp_read_file_to_memory(input_filename, &file_size);
    if (!file_data) return 0;

    if (file_size < (size_t)containerHeaderSize) { free(file_data); return 0; }

    PZPContainerHeader hdr;
    memcpy(&hdr, file_data, containerHeaderSize);

    if (hdr.magic != convert_header(pzp_header))
    {
        fprintf(stderr, "pzp_container_attach: not a container file\n");
        free(file_data); return 0;
    }

    /* read original frame index */
    size_t idx_bytes = (size_t)hdr.frame_count * frameEntrySize;
    if (file_size < (size_t)containerHeaderSize + idx_bytes) { free(file_data); return 0; }

    PZPFrameEntry *entries = (PZPFrameEntry *)malloc(idx_bytes);
    if (!entries) { free(file_data); return 0; }
    memcpy(entries, (const unsigned char *)file_data + containerHeaderSize, idx_bytes);

    /* total frame data bytes */
    unsigned int total_frame_bytes = 0;
    for (unsigned int f = 0; f < hdr.frame_count; f++)
        total_frame_bytes += entries[f].compressed_size;

    /* new layout: same frame area, new metadata/audio after */
    unsigned int idx_size         = hdr.frame_count * (unsigned int)frameEntrySize;
    unsigned int frame_area_start = (unsigned int)containerHeaderSize + idx_size;
    unsigned int data_end         = frame_area_start + total_frame_bytes;
    if ((size_t)frame_area_start + total_frame_bytes > file_size)
    {
        fprintf(stderr, "pzp_container_attach: frame data extends beyond file end\n");
        free(entries); free(file_data); return 0;
    }

    /* a blob that is not being replaced is kept from the input file */
    if (!(metadata && metadata_bytes > 0) && (hdr.container_flags & PZP_CONTAINER_HAS_METADATA) &&
        (hdr.metadata_bytes > 0) && ((size_t)hdr.metadata_offset + hdr.metadata_bytes <= file_size))
    {
        metadata       = (const unsigned char *)file_data + hdr.metadata_offset;
        metadata_bytes = hdr.metadata_bytes;
    }
    if (!(audio && audio_bytes > 0) && (hdr.container_flags & PZP_CONTAINER_HAS_AUDIO) &&
        (hdr.audio_bytes > 0) && ((size_t)hdr.audio_offset + hdr.audio_bytes <= file_size))
    {
        audio        = (const unsigned char *)file_data + hdr.audio_offset;
        audio_bytes  = hdr.audio_bytes;
        audio_format = hdr.audio_format;
    }

    unsigned int new_meta_offset  = (metadata && metadata_bytes > 0) ? data_end : 0;
    unsigned int new_audio_offset = 0;
    if (audio && audio_bytes > 0)
        new_audio_offset = new_meta_offset ? (new_meta_offset + metadata_bytes) : data_end;

    /* update header fields */
    unsigned int flags = hdr.container_flags & ~(PZP_CONTAINER_HAS_METADATA | PZP_CONTAINER_HAS_AUDIO);
    if (metadata && metadata_bytes > 0) flags |= PZP_CONTAINER_HAS_METADATA;
    if (audio    && audio_bytes    > 0) flags |= PZP_CONTAINER_HAS_AUDIO;

    hdr.container_flags = flags;
    hdr.metadata_offset = new_meta_offset;
    hdr.metadata_bytes  = (metadata && metadata_bytes > 0) ? metadata_bytes : 0;
    hdr.audio_offset    = new_audio_offset;
    hdr.audio_bytes     = (audio && audio_bytes > 0) ? audio_bytes : 0;
    hdr.audio_format    = (audio && audio_bytes > 0) ? audio_format : 0;
    hdr.header_checksum = hash_checksum(&hdr, sizeof(unsigned int) * 10);

    /* recompute frame offsets (they stay the same relative structure) */
    unsigned int cur = frame_area_start;
    for (unsigned int f = 0; f < hdr.frame_count; f++)
    {
        unsigned int sz = entries[f].compressed_size;
        entries[f].frame_offset = cur;
        cur += sz;
    }

    FILE *out = fopen(output_filename, "wb");
    if (!out) { free(entries); free(file_data); return 0; }

    int ok = (fwrite(&hdr, sizeof(unsigned int), 12, out) == 12);
    ok = ok && (fwrite(entries, sizeof(unsigned int), 4 * hdr.frame_count, out) == 4 * (size_t)hdr.frame_count);

    /* copy all frame data from original file */
    ok = ok && (fwrite((const unsigned char *)file_data + frame_area_start, 1, total_frame_bytes, out) == total_frame_bytes);

    if (metadata && metadata_bytes > 0)
        ok = ok && (fwrite(metadata, 1, metadata_bytes, out) == metadata_bytes);

    if (audio && audio_bytes > 0)
        ok = ok && (fwrite(audio, 1, audio_bytes, out) == audio_bytes);

    ok = (fclose(out) == 0) && ok;
    free(entries);
    free(file_data);
    if (!ok) fprintf(stderr, "pzp_container_attach: writing %s failed\n", output_filename);
    return ok;
}

//-----------------------------------------------------------------------------------------------
/* Returns 1 on success, 0 on failure - see pzp_container_write() , the only thing this can fail on. */
static int pzp_compress_combined(unsigned char **buffers,
                              unsigned int width,unsigned int height,
                              unsigned int bitsperpixelExternal, unsigned int channelsExternal,
                              unsigned int bitsperpixelInternal, unsigned int channelsInternal,
                              unsigned int configuration,
                              const PZPChannelGroup *groups, unsigned int group_count,
                              const char *output_filename)
{
    /* Wrap in a single-frame container */
    unsigned char **all_buffers[1] = { buffers };
    unsigned int   widths[1]       = { width };
    unsigned int   heights[1]      = { height };
    unsigned int   bpp_exts[1]     = { bitsperpixelExternal };
    unsigned int   ch_exts[1]      = { channelsExternal };
    unsigned int   bpp_ints[1]     = { bitsperpixelInternal };
    unsigned int   ch_ints[1]      = { channelsInternal };
    unsigned int   cfgs[1]         = { configuration };
    unsigned int   delays[1]       = { 0 };

    return pzp_container_write(output_filename,
                        all_buffers, 1,
                        widths, heights,
                        bpp_exts, ch_exts,
                        bpp_ints, ch_ints,
                        cfgs, groups, group_count, delays,
                        1 /* loop_count */,
                        NULL, 0,    /* no metadata */
                        NULL, 0, 0  /* no audio    */);
}

//-----------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
#if INTEL_OPTIMIZATIONS
/**
 * @brief Computes the prefix sum of an array using AVX2 SIMD operations.
 *
 * This function processes an array of unsigned 8-bit integers (bytes) in chunks of 32 bytes at a time,
 * using AVX2 intrinsics to perform vectorized addition. The prefix sum means that each element in the
 * output is the sum of all previous elements including itself, i.e.,
 *
 *      dst[i] = src[i] + dst[i-1]
 *
 * The algorithm leverages SIMD parallelism for faster execution by processing 32 elements in a single iteration.
 *
 * @param src  Pointer to the source array of unsigned 8-bit integers.
 * @param dst  Pointer to the destination array where the computed prefix sum will be stored.
 * @param size Number of elements in the source array. Must be a multiple of 32 for best performance.
 * @note
 * - The function assumes `size` is a multiple of 32 for optimal performance. If not, a scalar fallback is needed.
 * - This implementation is efficient for **short sequences** but not ideal for very long sequences,
 *   as it does not fully exploit SIMD-friendly prefix sum techniques like **Hillis-Steele scan**.
 * - If processing very large arrays, consider a **two-pass approach** to propagate values properly across blocks.
 * - Works best when `src` and `dst` are **aligned** to 32-byte boundaries, though `_mm256_loadu_si256`
 *   handles unaligned memory safely but slightly slower than aligned `_mm256_load_si256`.
 */
static void pzp_prefix_sum_avx2(unsigned char *src, unsigned char *dst, unsigned int size)
{
    // 1-channel prefix sum: dst[i] = src[i] + dst[i-1], processing 32 bytes per iteration.
    //
    // AVX2 key constraint: _mm256_slli_si256 shifts within each 128-bit lane independently.
    // It cannot propagate across the lane boundary.  We handle this in two explicit steps:
    //
    //   Step 1 – Kogge-Stone within each 16-byte lane (shifts 1, 2, 4, 8).
    //            Lane 0 (bytes  0-15): correct prefix sums from byte 0.
    //            Lane 1 (bytes 16-31): correct prefix sums from byte 16 only (missing carry).
    //
    //   Step 2 – Cross-lane carry: extract last byte of lane 0, broadcast to all of lane 1,
    //            add to lane 1 only so bytes 16-31 gain the cumulative sum through byte 15.
    //
    //   Step 3 – Cross-block carry: add the carry from the previous 32-byte block to every
    //            element.  Carry = last byte of the previous block's result, broadcast to 32.

    __m256i carry = _mm256_setzero_si256();
    unsigned int i = 0;

    for (; i + 31 < size; i += 32)
    {
        __m256i v = _mm256_loadu_si256((__m256i *)(src + i));

        // Step 1: Kogge-Stone within each 128-bit lane.
        v = _mm256_add_epi8(v, _mm256_slli_si256(v, 1));
        v = _mm256_add_epi8(v, _mm256_slli_si256(v, 2));
        v = _mm256_add_epi8(v, _mm256_slli_si256(v, 4));
        v = _mm256_add_epi8(v, _mm256_slli_si256(v, 8));

        // Step 2: propagate last byte of lane 0 to all bytes of lane 1.
        {
            __m128i lane0 = _mm256_castsi256_si128(v);
            unsigned char last0 = (unsigned char)_mm_cvtsi128_si32(_mm_srli_si128(lane0, 15));
            // Build a 256-bit vector: zeros in lane 0, broadcast of last0 in lane 1.
            __m256i lane_carry = _mm256_set_m128i(_mm_set1_epi8((char)last0),
                                                   _mm_setzero_si128());
            v = _mm256_add_epi8(v, lane_carry);
        }

        // Step 3: add cross-block carry to all 32 bytes.
        v = _mm256_add_epi8(v, carry);
        _mm256_storeu_si256((__m256i *)(dst + i), v);

        // Update carry: broadcast last byte of the result (byte 31 = last of lane 1).
        {
            __m128i hi = _mm256_extracti128_si256(v, 1);
            unsigned char last = (unsigned char)_mm_cvtsi128_si32(_mm_srli_si128(hi, 15));
            carry = _mm256_set1_epi8((char)last);
        }
    }

    // Scalar tail (also covers the case size < 32).
    for (; i < size; i++)
        dst[i] = src[i] + (i > 0 ? dst[i - 1] : 0);
}

/**
 * @brief Computes the prefix sum for a 2-channel interleaved array using AVX2 SIMD operations.
 *
 * This function processes an array where two interleaved unsigned 8-bit integer channels are present.
 * It computes the prefix sum separately for each channel while preserving their interleaved layout.
 * The prefix sum ensures that:
 *
 *      dst[2*i]   = src[2*i]   + dst[2*i - 2]
 *      dst[2*i+1] = src[2*i+1] + dst[2*i - 1]
 *
 * The function uses AVX2 to process 32 elements (16 pairs of channels) per iteration, improving performance.
 *
 * @param src  Pointer to the source array of unsigned 8-bit integers (interleaved 2-channel format).
 * @param dst  Pointer to the destination array where the computed prefix sum will be stored.
 * @param size Number of interleaved channel pairs in the source array (not the byte size).
 * @note
 * - The function assumes `size` is a multiple of 16 for optimal performance. If not, a scalar fallback is needed.
 * - This implementation works efficiently for small to medium-sized sequences but does not fully optimize
 *   long sequences where more sophisticated prefix sum algorithms (such as Hillis-Steele scan) may be required.
 * - Works best when `src` and `dst` are **aligned** to 32-byte boundaries, although `_mm256_loadu_si256`
 *   allows for unaligned memory access at a slight performance cost.
 */
static void pzp_prefix_sum_avx2_2ch(unsigned char *src, unsigned char *dst, unsigned int size)
{
    // 2-channel interleaved prefix sum: 16 pixel-pairs (32 bytes) per iteration.
    // size = number of pixels (pairs); total bytes = size * 2.
    //
    //   dst[2*i]   = src[2*i]   + dst[2*(i-1)]     (channel 0)
    //   dst[2*i+1] = src[2*i+1] + dst[2*(i-1)+1]   (channel 1)
    //
    // The two channels are independent; their stride in the interleaved layout is 2 bytes.
    // Kogge-Stone uses shifts of 2, 4, 8 (not 1, 2, 4, 8) to match that stride.
    // After the intra-lane scan, cross-lane carry propagates the last pixel-pair of lane 0
    // (bytes 14-15) to all 8 pixel positions in lane 1.
    // The _mm256_set1_epi16 broadcast replicates the 2-byte [ch0,ch1] pattern to all 16
    // positions across both lanes, forming the cross-block carry.

    __m256i carry = _mm256_setzero_si256();
    unsigned int i = 0;

    for (; i + 15 < size; i += 16)
    {
        // Load 16 interleaved pixel-pairs = 32 bytes.
        __m256i v = _mm256_loadu_si256((__m256i *)(src + i * 2));

        // Step 1: Kogge-Stone within each 128-bit lane, stride 2 (one pixel-pair per step).
        v = _mm256_add_epi8(v, _mm256_slli_si256(v, 2));
        v = _mm256_add_epi8(v, _mm256_slli_si256(v, 4));
        v = _mm256_add_epi8(v, _mm256_slli_si256(v, 8));
        // Lane 0 (pixels 0-7): correct prefix sums.
        // Lane 1 (pixels 8-15): sums relative to pixel 8 only — missing pixel 7 carry.

        // Step 2: cross-lane carry — last pixel-pair of lane 0 (bytes 14-15) → all of lane 1.
        {
            __m128i lane0 = _mm256_castsi256_si128(v);
            // Shift right by 14: puts bytes 14 and 15 in positions 0 and 1.
            int last_px = _mm_cvtsi128_si32(_mm_srli_si128(lane0, 14));
            // _mm_set1_epi16 replicates the 16-bit [ch0,ch1] pair to all 8 positions.
            __m128i bcast = _mm_set1_epi16((short)(last_px & 0xFFFF));
            // Add to lane 1 only (zeros in lane 0).
            __m256i lane_carry = _mm256_set_m128i(bcast, _mm_setzero_si128());
            v = _mm256_add_epi8(v, lane_carry);
        }

        // Step 3: add cross-block carry to all 32 bytes.
        v = _mm256_add_epi8(v, carry);
        _mm256_storeu_si256((__m256i *)(dst + i * 2), v);

        // Update carry: last pixel-pair of the result (bytes 30-31 of lane 1).
        {
            __m128i hi = _mm256_extracti128_si256(v, 1);
            int last_px = _mm_cvtsi128_si32(_mm_srli_si128(hi, 14));
            carry = _mm256_set1_epi16((short)(last_px & 0xFFFF));
        }
    }

    // Scalar tail (also covers size < 16).
    for (; i < size; i++)
    {
        dst[i * 2]     = src[i * 2]     + (i > 0 ? dst[(i - 1) * 2]     : 0);
        dst[i * 2 + 1] = src[i * 2 + 1] + (i > 0 ? dst[(i - 1) * 2 + 1] : 0);
    }
}


/*
 * pzp_deinterleave_3ch
 *
 * Split n_pixels interleaved RGB bytes (src) into three separate planar
 * buffers ch0/ch1/ch2, each of length n_pixels.
 *
 * SSSE3 path: 16 pixels (48 bytes) per iteration using _mm_shuffle_epi8.
 * Three source chunks a/b/c each cover 16 bytes of the 48-byte input:
 *   a = src[0..15]:  R0 G0 B0 R1 G1 B1 R2 G2 B2 R3 G3 B3 R4 G4 B4 R5
 *   b = src[16..31]: G5 B5 R6 G6 B6 R7 G7 B7 R8 G8 B8 R9 G9 B9 R10 G10
 *   c = src[32..47]: B10 R11 G11 B11 R12 G12 B12 R13 G13 B13 R14 G14 B14 R15 G15 B15
 * Masks place each channel's bytes into the correct output positions (0x80 zeroes
 * unused slots); the three OR'd shuffles produce one 16-byte channel result.
 */
static void pzp_deinterleave_3ch(
    const unsigned char *src,
    unsigned char *ch0, unsigned char *ch1, unsigned char *ch2,
    unsigned int n_pixels)
{
    /* ch0 (R): a→R[0..5] at out[0..5], b→R[6..10] at out[6..10], c→R[11..15] at out[11..15] */
    const __m128i shuf_r_a = _mm_setr_epi8( 0,  3,  6,  9, 12, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
    const __m128i shuf_r_b = _mm_setr_epi8(-1, -1, -1, -1, -1, -1,  2,  5,  8, 11, 14, -1, -1, -1, -1, -1);
    const __m128i shuf_r_c = _mm_setr_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  1,  4,  7, 10, 13);

    /* ch1 (G): a→G[0..4] at out[0..4], b→G[5..10] at out[5..10], c→G[11..15] at out[11..15] */
    const __m128i shuf_g_a = _mm_setr_epi8( 1,  4,  7, 10, 13, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
    const __m128i shuf_g_b = _mm_setr_epi8(-1, -1, -1, -1, -1,  0,  3,  6,  9, 12, 15, -1, -1, -1, -1, -1);
    const __m128i shuf_g_c = _mm_setr_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  2,  5,  8, 11, 14);

    /* ch2 (B): a→B[0..4] at out[0..4], b→B[5..9] at out[5..9], c→B[10..15] at out[10..15] */
    const __m128i shuf_b_a = _mm_setr_epi8( 2,  5,  8, 11, 14, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
    const __m128i shuf_b_b = _mm_setr_epi8(-1, -1, -1, -1, -1,  1,  4,  7, 10, 13, -1, -1, -1, -1, -1, -1);
    const __m128i shuf_b_c = _mm_setr_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  0,  3,  6,  9, 12, 15);

    unsigned int i = 0;
    for (; i + 15 < n_pixels; i += 16, src += 48)
    {
        __m128i a = _mm_loadu_si128((const __m128i *)(src +  0));
        __m128i b = _mm_loadu_si128((const __m128i *)(src + 16));
        __m128i c = _mm_loadu_si128((const __m128i *)(src + 32));

        _mm_storeu_si128((__m128i *)(ch0 + i),
            _mm_or_si128(_mm_shuffle_epi8(a, shuf_r_a),
            _mm_or_si128(_mm_shuffle_epi8(b, shuf_r_b),
                         _mm_shuffle_epi8(c, shuf_r_c))));
        _mm_storeu_si128((__m128i *)(ch1 + i),
            _mm_or_si128(_mm_shuffle_epi8(a, shuf_g_a),
            _mm_or_si128(_mm_shuffle_epi8(b, shuf_g_b),
                         _mm_shuffle_epi8(c, shuf_g_c))));
        _mm_storeu_si128((__m128i *)(ch2 + i),
            _mm_or_si128(_mm_shuffle_epi8(a, shuf_b_a),
            _mm_or_si128(_mm_shuffle_epi8(b, shuf_b_b),
                         _mm_shuffle_epi8(c, shuf_b_c))));
    }
    /* scalar tail */
    for (; i < n_pixels; ++i, src += 3)
    {
        ch0[i] = src[0];
        ch1[i] = src[1];
        ch2[i] = src[2];
    }
}

/*
 * pzp_interleave_3ch
 *
 * Inverse of pzp_deinterleave_3ch: merge three planar channel buffers
 * ch0/ch1/ch2 (each n_pixels bytes) back into interleaved RGB dst.
 *
 * For each 16-pixel block the three 16-byte channel vectors r/g/b are
 * scattered into three 16-byte output chunks (out_a, out_b, out_c = 48 bytes):
 *   out_a = [R0 G0 B0 R1 G1 B1 R2 G2 B2 R3 G3 B3 R4 G4 B4 R5]
 *   out_b = [G5 B5 R6 G6 B6 R7 G7 B7 R8 G8 B8 R9 G9 B9 R10 G10]
 *   out_c = [B10 R11 G11 B11 R12 G12 B12 R13 G13 B13 R14 G14 B14 R15 G15 B15]
 */
static void pzp_interleave_3ch(
    const unsigned char *ch0, const unsigned char *ch1, const unsigned char *ch2,
    unsigned char *dst,
    unsigned int n_pixels)
{
    /* out_a: R at {0,3,6,9,12,15}, G at {1,4,7,10,13}, B at {2,5,8,11,14} */
    const __m128i shuf_a_r = _mm_setr_epi8( 0, -1, -1,  1, -1, -1,  2, -1, -1,  3, -1, -1,  4, -1, -1,  5);
    const __m128i shuf_a_g = _mm_setr_epi8(-1,  0, -1, -1,  1, -1, -1,  2, -1, -1,  3, -1, -1,  4, -1, -1);
    const __m128i shuf_a_b = _mm_setr_epi8(-1, -1,  0, -1, -1,  1, -1, -1,  2, -1, -1,  3, -1, -1,  4, -1);

    /* out_b: G at {0,3,6,9,12,15}, B at {1,4,7,10,13}, R at {2,5,8,11,14} */
    const __m128i shuf_b_g = _mm_setr_epi8( 5, -1, -1,  6, -1, -1,  7, -1, -1,  8, -1, -1,  9, -1, -1, 10);
    const __m128i shuf_b_b = _mm_setr_epi8(-1,  5, -1, -1,  6, -1, -1,  7, -1, -1,  8, -1, -1,  9, -1, -1);
    const __m128i shuf_b_r = _mm_setr_epi8(-1, -1,  6, -1, -1,  7, -1, -1,  8, -1, -1,  9, -1, -1, 10, -1);

    /* out_c: B at {0,3,6,9,12,15}, R at {1,4,7,10,13}, G at {2,5,8,11,14} */
    const __m128i shuf_c_b = _mm_setr_epi8(10, -1, -1, 11, -1, -1, 12, -1, -1, 13, -1, -1, 14, -1, -1, 15);
    const __m128i shuf_c_r = _mm_setr_epi8(-1, 11, -1, -1, 12, -1, -1, 13, -1, -1, 14, -1, -1, 15, -1, -1);
    const __m128i shuf_c_g = _mm_setr_epi8(-1, -1, 11, -1, -1, 12, -1, -1, 13, -1, -1, 14, -1, -1, 15, -1);

    unsigned int i = 0;
    for (; i + 15 < n_pixels; i += 16, dst += 48)
    {
        __m128i r = _mm_loadu_si128((const __m128i *)(ch0 + i));
        __m128i g = _mm_loadu_si128((const __m128i *)(ch1 + i));
        __m128i b = _mm_loadu_si128((const __m128i *)(ch2 + i));

        _mm_storeu_si128((__m128i *)(dst +  0),
            _mm_or_si128(_mm_shuffle_epi8(r, shuf_a_r),
            _mm_or_si128(_mm_shuffle_epi8(g, shuf_a_g),
                         _mm_shuffle_epi8(b, shuf_a_b))));
        _mm_storeu_si128((__m128i *)(dst + 16),
            _mm_or_si128(_mm_shuffle_epi8(g, shuf_b_g),
            _mm_or_si128(_mm_shuffle_epi8(b, shuf_b_b),
                         _mm_shuffle_epi8(r, shuf_b_r))));
        _mm_storeu_si128((__m128i *)(dst + 32),
            _mm_or_si128(_mm_shuffle_epi8(b, shuf_c_b),
            _mm_or_si128(_mm_shuffle_epi8(r, shuf_c_r),
                         _mm_shuffle_epi8(g, shuf_c_g))));
    }
    /* scalar tail */
    for (; i < n_pixels; ++i, dst += 3)
    {
        dst[0] = ch0[i];
        dst[1] = ch1[i];
        dst[2] = ch2[i];
    }
}


static void pzp_extractAndReconstruct_AVX2(unsigned char *decompressed_bytes, unsigned char *reconstructed, unsigned int width, unsigned int height, unsigned int channels)
{
    unsigned int total_size = width * height;
    unsigned char *src = decompressed_bytes;
    unsigned char *r = reconstructed;

    switch (channels)
    {
        case 1: {
            pzp_prefix_sum_avx2(src, r, total_size);
            break;
        }
        case 2: {
            pzp_prefix_sum_avx2_2ch(src,r,total_size);
            break;
        }
        case 3: {
            // De-interleave via SSSE3 shuffle (16 px/iter), run AVX2 prefix
            // sum on each planar buffer, then re-interleave via SSSE3 shuffle.
            // Replaces the previous scalar stride-3 loops (~6.66B instructions
            // callgrind hotspot) with ~16× faster SIMD scatter/gather.
            unsigned char *planes = (unsigned char *)malloc((size_t)total_size * 3);
            if (planes) {
                unsigned char *ch0 = planes, *ch1 = planes + total_size, *ch2 = planes + 2 * (size_t)total_size;
                pzp_deinterleave_3ch(src, ch0, ch1, ch2, total_size);
                pzp_prefix_sum_avx2(ch0, ch0, total_size);
                pzp_prefix_sum_avx2(ch1, ch1, total_size);
                pzp_prefix_sum_avx2(ch2, ch2, total_size);
                pzp_interleave_3ch(ch0, ch1, ch2, r, total_size);
                free(planes);
            } else {
                // OOM fallback: scalar
                r[0] = src[0]; r[1] = src[1]; r[2] = src[2];
                for (unsigned int i = 1; i < total_size; ++i) {
                    r[i*3]   = src[i*3]   + r[(i-1)*3];
                    r[i*3+1] = src[i*3+1] + r[(i-1)*3+1];
                    r[i*3+2] = src[i*3+2] + r[(i-1)*3+2];
                }
            }
            break;
        }
        default: {
            // Generic case (scalar fallback)
            for (unsigned int ch = 0; ch < channels; ++ch)
            {
                r[ch] = src[ch];
            }
            for (unsigned int i = 1; i < total_size; ++i)
            {
                for (unsigned int ch = 0; ch < channels; ++ch)
                {
                    r[i * channels + ch] = src[i * channels + ch] + r[(i - 1) * channels + ch];
                }
            }
            break;
        }
    }
}
#endif // INTEL_OPTIMIZATIONS
static void pzp_extractAndReconstruct_Naive(unsigned char *decompressed_bytes, unsigned char *reconstructed, unsigned int width, unsigned int height, unsigned int channels)
{
    unsigned int total_size = width * height;
    unsigned char *src = decompressed_bytes;
    unsigned char *r   = reconstructed;

    switch (channels)
    {
        case 1:
            r[0] = src[0];
            for (unsigned int i = 1; i < total_size; i++)
            {
                r[i] = src[i] + r[i - 1];
            }
            break;
        case 2:
            r[0] = src[0];
            r[1] = src[1];
            for (unsigned int i = 1; i < total_size; i++)
            {
                r   += 2;
                src += 2;
                r[0] = src[0] + r[-2];
                r[1] = src[1] + r[-1];
            }
            break;
        case 3:
            r[0] = src[0];
            r[1] = src[1];
            r[2] = src[2];
            for (unsigned int i = 1; i < total_size; i++)
            {
                r   += 3;
                src += 3;
                r[0] = src[0] + r[-3];
                r[1] = src[1] + r[-2];
                r[2] = src[2] + r[-1];
            }
            break;
        default:
            for (unsigned int ch = 0; ch < channels; ch++)
            {
                r[ch] = src[ch];
            }
            for (unsigned int i = 1; i < total_size; i++)
            {
                for (unsigned int ch = 0; ch < channels; ch++)
                {
                    r[i * channels + ch] = src[i * channels + ch] + r[(i - 1) * channels + ch];
                }
            }
            break;
    }
}
//-----------------------------------------------------------------------------------------------
/* Undo the USE_RLE left-pixel delta filter: reconstructed[i] = decompressed_bytes[i] + reconstructed[i-1]
 * per channel. Safe in place with reconstructed <= decompressed_bytes ( the decoder passes its own
 * buffer start and the pixel data further inside it ): every path reads a source byte before any
 * write can reach it. */
static void pzp_extractAndReconstruct(unsigned char *decompressed_bytes, unsigned char *reconstructed, unsigned int width, unsigned int height, unsigned int channels)
{
   #if INTEL_OPTIMIZATIONS
     pzp_extractAndReconstruct_AVX2(decompressed_bytes,reconstructed,width,height,channels);
   #else
     pzp_extractAndReconstruct_Naive(decompressed_bytes,reconstructed,width,height,channels);
   #endif // INTEL_OPTIMIZATIONS
}

/* Decode the pixel data of a frame with several channel groups ( see PZPChannelGroup ): undo each
 * group's predictor into planar scratch buffers, then interleave every channel into out
 * ( width*height*channels bytes ). out may overlap src: src is fully consumed before out is written.
 * Returns 0 if the scratch buffers cannot be allocated. */
static int pzp_channel_groups_decode(
        const unsigned char *src, unsigned char *out,
        const PZPChannelGroup *groups, unsigned int group_count,
        unsigned int width, unsigned int height, unsigned int channels)
{
    size_t pixels = (size_t)width * height;
    unsigned char *planes = (unsigned char *)malloc(pixels * channels);
    if (!planes) return 0;

    unsigned char *plane = planes;
    for (unsigned int g = 0; g < group_count; g++)
    {
        unsigned int c = groups[g].channels;
        if (groups[g].sample_bits == 16)
            pzp_gradient16_decode(src, src + pixels, plane, plane + pixels, width, height);
        else if ((c == 1) && (groups[g].predictor == PZP_PREDICT_LEFT))
            pzp_extractAndReconstruct((unsigned char *)src, plane, width, height, 1);
        else
        {
            for (size_t i = 0; i < pixels; i++)
                for (unsigned int ch = 0; ch < c; ch++)
                    plane[ch * pixels + i] = src[i * c + ch];
            if (groups[g].predictor == PZP_PREDICT_LEFT)
                for (unsigned int ch = 0; ch < c; ch++)
                    pzp_extractAndReconstruct(plane + ch * pixels, plane + ch * pixels, width, height, 1);
        }
        src   += pixels * c;
        plane += pixels * c;
    }

    #if INTEL_OPTIMIZATIONS
    if (channels == 3)
        pzp_interleave_3ch(planes, planes + pixels, planes + 2 * pixels, out, (unsigned int)pixels);
    else
    #endif
    for (size_t i = 0; i < pixels; i++)
        for (unsigned int ch = 0; ch < channels; ch++)
            out[i * channels + ch] = planes[ch * pixels + i];

    free(planes);
    return 1;
}

//-----------------------------------------------------------------------------------------------
// ─── Inner frame decompressor (from memory) ──────────────────────────────────
//
// Forward-declared here so container reader functions below can call it.
// Full definition follows after this section. It decodes one inner frame only
// ( no container detection ), so a frame can never recurse into its container.
//-----------------------------------------------------------------------------------------------
static unsigned char* pzp_frame_decode_from_memory(
                                const void *file_data, size_t file_size,
                                unsigned int *widthOutput, unsigned int *heightOutput,
                                unsigned int *bitsperpixelExternalOutput, unsigned int *channelsExternalOutput,
                                unsigned int *bitsperpixelInternalOutput, unsigned int *channelsInternalOutput,
                                unsigned int *configuration);

/* One decoded container frame ( pzp_container_read_frames ). pixels is malloc'd, caller frees. */
typedef struct {
    unsigned char *pixels;
    unsigned int   width, height;
    unsigned int   bpp_ext, ch_ext;
    unsigned int   bpp_int, ch_int;
    unsigned int   configuration;
    unsigned int   delay_ms;
} PZPFrame;

//-----------------------------------------------------------------------------------------------
// ─── Container read helpers ──────────────────────────────────────────────────

/*
 * Parse container header and frame index from a memory buffer.
 * Returns 1 on success, 0 on failure.
 * *entries_out is malloc'd; caller must free().
 */
static int pzp_container_parse_header(
        const void *file_data, size_t file_size,
        PZPContainerHeader *hdr_out,
        PZPFrameEntry **entries_out)
{
    if (file_size < (size_t)containerHeaderSize) return 0;

    memcpy(hdr_out, file_data, containerHeaderSize);

    if (hdr_out->magic != convert_header(pzp_header))
    {
        fprintf(stderr, "pzp: container magic mismatch\n");
        return 0;
    }
    if (hdr_out->version != PZP_CONTAINER_VERSION)
    {
        fprintf(stderr, "pzp: unsupported container version %u\n", hdr_out->version);
        return 0;
    }

#if PZP_VERIFY_CHECKSUM
    {
        unsigned int expected = hash_checksum(file_data, sizeof(unsigned int) * 10);
        if (expected != hdr_out->header_checksum)
        {
            fprintf(stderr, "pzp: container header checksum mismatch (stored 0x%X, computed 0x%X)\n",
                    hdr_out->header_checksum, expected);
            return 0;
        }
    }
#endif

    size_t idx_bytes = (size_t)hdr_out->frame_count * frameEntrySize;
    if (file_size < (size_t)containerHeaderSize + idx_bytes) return 0;

    PZPFrameEntry *entries = (PZPFrameEntry *)malloc(idx_bytes ? idx_bytes : 1);
    if (!entries) return 0;
    memcpy(entries, (const unsigned char *)file_data + containerHeaderSize, idx_bytes);
    *entries_out = entries;
    return 1;
}

/* Decode ( without inter-frame reconstruction ) the frame that index entry e points at. */
static int pzp_container_decode_entry(
        const void *file_data, size_t file_size,
        const PZPFrameEntry *e, unsigned int frame_index,
        PZPFrame *fr)
{
    fr->pixels = NULL;
    if ((size_t)e->frame_offset + e->compressed_size > file_size)
    {
        fprintf(stderr, "pzp: frame %u extends beyond file end\n", frame_index);
        return 0;
    }
    fr->pixels = pzp_frame_decode_from_memory(
            (const unsigned char *)file_data + e->frame_offset, e->compressed_size,
            &fr->width, &fr->height, &fr->bpp_ext, &fr->ch_ext,
            &fr->bpp_int, &fr->ch_int, &fr->configuration);
    fr->delay_ms = e->delay_ms;
    return (fr->pixels != NULL);
}

/* A USE_INTER_DELTA frame stores frame[N] - frame[N-1]: add the reconstructed previous frame back.
 * A reference with a different layout is ignored ( the encoder never deltas across layouts ). */
static void pzp_frame_add_reference(PZPFrame *cur, const PZPFrame *prev)
{
    if (prev->width == cur->width && prev->height == cur->height && prev->ch_int == cur->ch_int)
    {
        /* Internal buffers are always 8-bit planar; plain byte addition
           reconstructs the original pixel values (wrapping mod 256). */
        size_t n = (size_t)cur->width * cur->height * cur->ch_int;
        for (size_t i = 0; i < n; i++)
            cur->pixels[i] += prev->pixels[i];
    }
}

/*
 * Decompress frame 'frame_index' from a container held in memory.
 * A delta frame needs every frame back to the nearest keyframe: those are decoded once each,
 * newest first, then reconstructed oldest first. To decode many frames in order use
 * pzp_container_read_frames(), which decodes each frame exactly once.
 */
static unsigned char *pzp_container_read_frame_from_memory(
        const void *file_data, size_t file_size,
        unsigned int frame_index,
        unsigned int *width, unsigned int *height,
        unsigned int *bpp_ext, unsigned int *ch_ext,
        unsigned int *bpp_int, unsigned int *ch_int,
        unsigned int *configuration)
{
    PZPContainerHeader hdr;
    PZPFrameEntry     *entries = NULL;

    if (!pzp_container_parse_header(file_data, file_size, &hdr, &entries))
        return NULL;

    if (frame_index >= hdr.frame_count)
    {
        fprintf(stderr, "pzp: frame %u out of range (count=%u)\n", frame_index, hdr.frame_count);
        free(entries);
        return NULL;
    }

    /* chain[k] = frame k; frames first..frame_index are decoded, chain[first] is the keyframe */
    PZPFrame    *chain = (PZPFrame *)malloc(((size_t)frame_index + 1) * sizeof(PZPFrame));
    unsigned int first = frame_index + 1;   /* nothing decoded yet */
    int          ok    = (chain != NULL);
    while (ok)
    {
        unsigned int k = first - 1;
        ok = pzp_container_decode_entry(file_data, file_size, &entries[k], k, &chain[k]);
        if (!ok) break;
        first = k;
        if (!(chain[k].configuration & USE_INTER_DELTA) || (k == 0)) break;
    }
    free(entries);

    unsigned char *out = NULL;
    if (ok)
    {
        for (unsigned int k = first + 1; k <= frame_index; k++)
            pzp_frame_add_reference(&chain[k], &chain[k - 1]);
        const PZPFrame *fr = &chain[frame_index];
        out            = fr->pixels;
        *width         = fr->width;   *height = fr->height;
        *bpp_ext       = fr->bpp_ext; *ch_ext = fr->ch_ext;
        *bpp_int       = fr->bpp_int; *ch_int = fr->ch_int;
        *configuration = fr->configuration;
    }
    for (unsigned int k = first; k <= frame_index; k++)
        if (chain[k].pixels != out) free(chain[k].pixels);
    free(chain);
    return out;
}

/*
 * Decompress frame 'frame_index' from a container file on disk.
 */
static unsigned char *pzp_container_read_frame(
        const char   *filename,
        unsigned int  frame_index,
        unsigned int *width,  unsigned int *height,
        unsigned int *bpp_ext, unsigned int *ch_ext,
        unsigned int *bpp_int, unsigned int *ch_int,
        unsigned int *configuration)
{
    size_t file_size = 0;
    void  *file_data = pzp_read_file_to_memory(filename, &file_size);
    if (!file_data) return NULL;

    unsigned char *result = pzp_container_read_frame_from_memory(
            file_data, file_size, frame_index,
            width, height, bpp_ext, ch_ext, bpp_int, ch_int, configuration);
    free(file_data);
    return result;
}

/*
 * Decode the first min(frame_count, max_frames) frames of a container held in memory, in order,
 * each exactly once ( a delta frame reuses the previous, already reconstructed frame ).
 * Fills frames[0..n-1]; the caller frees each frames[i].pixels.
 * Returns n, or 0 on failure ( nothing left allocated ).
 */
static unsigned int pzp_container_read_frames_from_memory(
        const void *file_data, size_t file_size,
        PZPFrame *frames, unsigned int max_frames)
{
    PZPContainerHeader hdr;
    PZPFrameEntry     *entries = NULL;

    if (!pzp_container_parse_header(file_data, file_size, &hdr, &entries))
        return 0;

    unsigned int n = (hdr.frame_count < max_frames) ? hdr.frame_count : max_frames;
    for (unsigned int f = 0; f < n; f++)
    {
        if (!pzp_container_decode_entry(file_data, file_size, &entries[f], f, &frames[f]))
        {
            for (unsigned int j = 0; j < f; j++) { free(frames[j].pixels); frames[j].pixels = NULL; }
            free(entries);
            return 0;
        }
        if ((frames[f].configuration & USE_INTER_DELTA) && (f > 0))
            pzp_frame_add_reference(&frames[f], &frames[f - 1]);
    }
    free(entries);
    return n;
}

/*
 * pzp_container_read_frames_from_memory() on a container file on disk ( read once ).
 */
static unsigned int pzp_container_read_frames(
        const char *filename,
        PZPFrame   *frames, unsigned int max_frames)
{
    size_t file_size = 0;
    void  *file_data = pzp_read_file_to_memory(filename, &file_size);
    if (!file_data) return 0;

    unsigned int n = pzp_container_read_frames_from_memory(file_data, file_size, frames, max_frames);
    free(file_data);
    return n;
}

/*
 * Read container info (header + frame index) from a file.
 * *entries_out is malloc'd; caller must free().
 * Returns 1 on success, 0 on failure.
 */
static int pzp_container_get_info(
        const char         *filename,
        PZPContainerHeader *hdr_out,
        PZPFrameEntry     **entries_out)
{
    size_t file_size = 0;
    void  *file_data = pzp_read_file_to_memory(filename, &file_size);
    if (!file_data) return 0;

    int ok = pzp_container_parse_header(file_data, file_size, hdr_out, entries_out);
    free(file_data);
    return ok;
}

/*
 * Read the metadata blob from a container file.
 * Returns malloc'd bytes (caller frees) and sets *bytes_out.
 * Returns NULL if metadata is absent or on error.
 */
static unsigned char *pzp_container_get_metadata(
        const char   *filename,
        unsigned int *bytes_out)
{
    *bytes_out = 0;
    size_t file_size = 0;
    void  *file_data = pzp_read_file_to_memory(filename, &file_size);
    if (!file_data) return NULL;

    PZPContainerHeader hdr;
    PZPFrameEntry     *entries = NULL;
    if (!pzp_container_parse_header(file_data, file_size, &hdr, &entries))
    { free(file_data); return NULL; }
    free(entries);

    if (!(hdr.container_flags & PZP_CONTAINER_HAS_METADATA) || hdr.metadata_bytes == 0)
    { free(file_data); return NULL; }
    if ((size_t)hdr.metadata_offset + hdr.metadata_bytes > file_size)
    {
        fprintf(stderr, "pzp: metadata extends beyond file end\n");
        free(file_data); return NULL;
    }

    unsigned char *blob = (unsigned char *)malloc(hdr.metadata_bytes);
    if (blob)
    {
        memcpy(blob, (const unsigned char *)file_data + hdr.metadata_offset, hdr.metadata_bytes);
        *bytes_out = hdr.metadata_bytes;
    }
    free(file_data);
    return blob;
}

/*
 * Read the audio blob from a container file.
 * Returns malloc'd bytes (caller frees), sets *bytes_out and *format_out.
 * Returns NULL if audio is absent or on error.
 */
static unsigned char *pzp_container_get_audio(
        const char   *filename,
        unsigned int *bytes_out,
        unsigned int *format_out)
{
    *bytes_out  = 0;
    *format_out = 0;
    size_t file_size = 0;
    void  *file_data = pzp_read_file_to_memory(filename, &file_size);
    if (!file_data) return NULL;

    PZPContainerHeader hdr;
    PZPFrameEntry     *entries = NULL;
    if (!pzp_container_parse_header(file_data, file_size, &hdr, &entries))
    { free(file_data); return NULL; }
    free(entries);

    if (!(hdr.container_flags & PZP_CONTAINER_HAS_AUDIO) || hdr.audio_bytes == 0)
    { free(file_data); return NULL; }
    if ((size_t)hdr.audio_offset + hdr.audio_bytes > file_size)
    {
        fprintf(stderr, "pzp: audio extends beyond file end\n");
        free(file_data); return NULL;
    }

    unsigned char *blob = (unsigned char *)malloc(hdr.audio_bytes);
    if (blob)
    {
        memcpy(blob, (const unsigned char *)file_data + hdr.audio_offset, hdr.audio_bytes);
        *bytes_out  = hdr.audio_bytes;
        *format_out = hdr.audio_format;
    }
    free(file_data);
    return blob;
}

//-----------------------------------------------------------------------------------------------
// ─── Inner frame decompressor (full definition) ──────────────────────────────

static unsigned char* pzp_frame_decode_from_memory(
                                const void *file_data, size_t file_size,
                                unsigned int *widthOutput, unsigned int *heightOutput,
                                unsigned int *bitsperpixelExternalOutput, unsigned int *channelsExternalOutput,
                                unsigned int *bitsperpixelInternalOutput, unsigned int *channelsInternalOutput,
                                unsigned int *configuration)
{
    if (!file_data || file_size < sizeof(unsigned int))
    {
        fprintf(stderr, "Invalid file data or size\n");
        return NULL;
    }

    /* ── Inner frame: [4-byte size prefix][zstd or lz4 stream] ─────────── */
    const unsigned char *input_ptr = (const unsigned char *)file_data;

    // Read stored size prefix (bit 31 = codec: 0=ZSTD, 1=LZ4; bits 0-30 = uncompressed size)
    unsigned int size_prefix;
    memcpy(&size_prefix, input_ptr, sizeof(unsigned int));

    int codec_is_lz4 = (size_prefix & PZP_CODEC_LZ4_FLAG) != 0;
    unsigned int dataSize = size_prefix & PZP_CODEC_SIZE_MASK;

    if (dataSize == 0 || dataSize > 100000000)
    { // sanity check
        fprintf(stderr, "Error: Invalid size read from memory (%u)\n", dataSize);
        return NULL;
    }

    size_t compressed_size = file_size - sizeof(unsigned int);
    const void *compressed_buffer = input_ptr + sizeof(unsigned int);

    size_t decompressed_size = (size_t)dataSize;
    void *decompressed_buffer = malloc(decompressed_size);
    if (!decompressed_buffer)
    {
        return 0;
    }

    size_t actual_decompressed_size;
    if (codec_is_lz4)
    {
        int lz4_result = LZ4_decompress_safe(
                (const char *)compressed_buffer, (char *)decompressed_buffer,
                (int)compressed_size, (int)decompressed_size);
        if (lz4_result < 0)
        {
            free(decompressed_buffer);
            fprintf(stderr, "LZ4 decompression error: %d\n", lz4_result);
            return 0;
        }
        actual_decompressed_size = (size_t)lz4_result;
    }
    else if (pzp_is_openzl_stream(compressed_buffer, compressed_size))
    {
#if PZP_USE_OPENZL
        actual_decompressed_size = pzp_openzl_decompress(decompressed_buffer, decompressed_size,
                                                         compressed_buffer, compressed_size);
        if (actual_decompressed_size == 0)
        {
            free(decompressed_buffer);
            return 0;
        }
#else
        free(decompressed_buffer);
        fprintf(stderr, "pzp: frame is OpenZL-compressed; rebuild with PZP_USE_OPENZL=1 to read it\n");
        return 0;
#endif
    }
    else
    {
        pzp_thread_init();  // no-op if already initialised
        actual_decompressed_size = _pzp_zstd_dctx
            ? ZSTD_decompressDCtx(_pzp_zstd_dctx, decompressed_buffer, decompressed_size, compressed_buffer, compressed_size)
            : ZSTD_decompress(decompressed_buffer, decompressed_size, compressed_buffer, compressed_size);
        if (ZSTD_isError(actual_decompressed_size))
        {
            free(decompressed_buffer);
            fprintf(stderr, "Zstd decompression error: %s\n", ZSTD_getErrorName(actual_decompressed_size));
            return 0;
        }
    }

    if (actual_decompressed_size != decompressed_size)
    {
        free(decompressed_buffer);
        fprintf(stderr, "Actual Decompressed size %lu mismatch with Decompressed size %lu \n", actual_decompressed_size, decompressed_size);
        return 0;
    }

    if (decompressed_size < (size_t)headerSize)
    {
        free(decompressed_buffer);
        fprintf(stderr, "PZP frame too small for its header (%zu bytes)\n", decompressed_size);
        return NULL;
    }

    // Read header information
    unsigned int *memStartAsUINT = (unsigned int *)decompressed_buffer;

    unsigned int *headerSource            = memStartAsUINT + 0;
    unsigned int *bitsperpixelExtSource   = memStartAsUINT + 1;
    unsigned int *channelsExtSource       = memStartAsUINT + 2;
    unsigned int *widthSource             = memStartAsUINT + 3;
    unsigned int *heightSource            = memStartAsUINT + 4;
    unsigned int *bitsperpixelInSource    = memStartAsUINT + 5;
    unsigned int *channelsInSource        = memStartAsUINT + 6;
    unsigned int *checksumSource          = memStartAsUINT + 7;
    unsigned int *compressionConfigSource = memStartAsUINT + 8;
    unsigned int *paletteDataSizeSource   = memStartAsUINT + 9;

    // Move from mapped header memory to our local variables
    unsigned int bitsperpixelExt  = *bitsperpixelExtSource;
    unsigned int channelsExt      = *channelsExtSource;
    unsigned int width            = *widthSource;
    unsigned int height           = *heightSource;
    unsigned int bitsperpixelIn   = *bitsperpixelInSource;
    unsigned int channelsIn       = *channelsInSource;
    unsigned int compressionCfg   = *compressionConfigSource;
    unsigned int paletteDataBytes = *paletteDataSizeSource;

#if PZP_VERBOSE
    fprintf(stderr, "Detected %ux%ux%u@%ubit/", width, height, channelsExt, bitsperpixelExt);
    fprintf(stderr, "%u@%ubit", channelsIn, bitsperpixelIn);
    fprintf(stderr, " | mode %u | CRC:0x%X\n", compressionCfg, *checksumSource);
#endif

    // "PZP1" frames carry a channel group table; "PZP0" frames ( written before v0.03 ) are read as
    // one implied 8-bit group. The PZP0 case can be dropped once no such files are left.
    int hasGroupTable = (*headerSource == convert_header(pzp_frame_header_groups));
    if ( (!hasGroupTable) && (*headerSource != convert_header(pzp_header)) )
    {
        free(decompressed_buffer);
        return 0;
    }

    // Move from our local variables to function output
    *bitsperpixelExternalOutput = bitsperpixelExt;
    *channelsExternalOutput     = channelsExt;
    *widthOutput                = width;
    *heightOutput               = height;
    *bitsperpixelInternalOutput = bitsperpixelIn;
    *channelsInternalOutput     = channelsIn;
    *configuration              = compressionCfg;

    // After the 40-byte header come the channel group table ( PZP1 only ), optional palette data,
    // then the pixel/index data.
    size_t avail = decompressed_size - (size_t)headerSize;
    unsigned char *after_header = (unsigned char *)decompressed_buffer + headerSize;

    PZPChannelGroup groups[PZP_MAX_CHANNEL_GROUPS];
    unsigned int    group_count = 1;
    int             groupsOk    = 1;
    if (hasGroupTable)
    {
        group_count = (avail > 0) ? after_header[0] : 0;
        size_t tableBytes = 1 + (size_t)group_count * sizeof(PZPChannelGroup);
        groupsOk = (group_count > 0) && (group_count <= PZP_MAX_CHANNEL_GROUPS) && (tableBytes <= avail);
        if (groupsOk)
        {
            memcpy(groups, after_header + 1, group_count * sizeof(PZPChannelGroup));
            groupsOk = pzp_channel_groups_valid(groups, group_count, channelsIn, bitsperpixelIn, compressionCfg);
            after_header += tableBytes;
            avail        -= tableBytes;
        }
    }
    else
    {
        PZPChannelGroup legacy = { 0, 8, (compressionCfg & USE_RLE) ? PZP_PREDICT_LEFT : PZP_PREDICT_NONE, 0 };
        groups[0] = legacy;
    }

    // The header fields come from the file: check that the palette and the pixel data they
    // describe fit in what was decompressed ( overflow-safe ) before reading any of it.
    size_t pixel_size = (size_t)width * height;
    unsigned int bytesPerSample = bitsperpixelIn / 8;
    if ( (!groupsOk) || (paletteDataBytes > avail) ||
         (pixel_size == 0) || (channelsIn == 0) || (bytesPerSample == 0) ||
         (pixel_size > (avail - paletteDataBytes) / channelsIn / bytesPerSample) ||
         ((compressionCfg & USE_PALETTE) && (channelsIn > 8)) )
    {
        free(decompressed_buffer);
        fprintf(stderr, "PZP frame header does not match its data (%ux%ux%u@%ubit, palette %u, %zu bytes)\n",
                width, height, channelsIn, bitsperpixelIn, paletteDataBytes, decompressed_size);
        return NULL;
    }
    pixel_size *= (size_t)bytesPerSample * channelsIn;

    unsigned char *index_data = after_header + paletteDataBytes;

    // Parse palette (if present) before checksum so we can validate index data.
    unsigned char palette[8][256];
    unsigned int  palette_counts[8];
    if ( (compressionCfg & USE_PALETTE) &&
         (pzp_palette_read(after_header, paletteDataBytes, channelsIn, palette, palette_counts) != paletteDataBytes) )
    {
        free(decompressed_buffer);
        fprintf(stderr, "PZP palette does not match its declared size (%u bytes)\n", paletteDataBytes);
        return NULL;
    }

#if PZP_VERIFY_CHECKSUM
    // Checksum covers the index/pixel data only (not the palette prefix).
    {
        unsigned int computedChecksum = hash_checksum(index_data, pixel_size);
        if (computedChecksum != *checksumSource)
        {
            free(decompressed_buffer);
            fprintf(stderr, "PZP checksum mismatch (stored 0x%X, computed 0x%X): file may be corrupted\n",
                    *checksumSource, computedChecksum);
            return NULL;
        }
    }
#endif

    // Every path moves the pixels to the start of decompressed_buffer and returns it. A single
    // 8-bit group ( every PZP0 frame ) needs no second image-sized allocation: a plain move, or the
    // left-delta reconstruction done in place.
    unsigned char *reconstructed = (unsigned char *)decompressed_buffer;
    if ( (group_count == 1) && (groups[0].sample_bits == 8) )
    {
        if (groups[0].predictor == PZP_PREDICT_LEFT)
            pzp_extractAndReconstruct(index_data, reconstructed, width, height, channelsIn);
        else
            memmove(reconstructed, index_data, pixel_size);
    }
    else if (!pzp_channel_groups_decode(index_data, reconstructed, groups, group_count, width, height, channelsIn))
    {
        free(decompressed_buffer);
        fprintf(stderr, "PZP: out of memory decoding channel groups\n");
        return NULL;
    }

    if (compressionCfg & USE_PALETTE)
        pzp_palette_apply(reconstructed, width * height, channelsIn, palette);

    return reconstructed;
}


/* Decode a whole .pzp held in memory: frame 0 of a container ( the PZP0 magic ), or a bare inner
 * frame ( the legacy single-image format ). */
static unsigned char* pzp_decompress_combined_from_memory(
                                const void *file_data, size_t file_size,
                                unsigned int *widthOutput, unsigned int *heightOutput,
                                unsigned int *bitsperpixelExternalOutput, unsigned int *channelsExternalOutput,
                                unsigned int *bitsperpixelInternalOutput, unsigned int *channelsInternalOutput,
                                unsigned int *configuration)
{
    if (file_data && file_size >= (size_t)containerHeaderSize)
    {
        unsigned int first_word;
        memcpy(&first_word, file_data, sizeof(unsigned int));
        if (first_word == convert_header(pzp_header))
        {
            return pzp_container_read_frame_from_memory(
                    file_data, file_size, 0,
                    widthOutput, heightOutput,
                    bitsperpixelExternalOutput, channelsExternalOutput,
                    bitsperpixelInternalOutput, channelsInternalOutput,
                    configuration);
        }
    }
    return pzp_frame_decode_from_memory(file_data, file_size,
                                        widthOutput, heightOutput,
                                        bitsperpixelExternalOutput, channelsExternalOutput,
                                        bitsperpixelInternalOutput, channelsInternalOutput,
                                        configuration);
}

static unsigned char* pzp_decompress_combined(const char *input_filename,
                                unsigned int *widthOutput, unsigned int *heightOutput,
                                unsigned int *bitsperpixelExternalOutput, unsigned int *channelsExternalOutput,
                                unsigned int *bitsperpixelInternalOutput, unsigned int *channelsInternalOutput,
                                unsigned int *configuration)
{
    size_t file_size = 0;
    void *file_data = pzp_read_file_to_memory(input_filename, &file_size);
    if (file_data!=NULL)
    {
      unsigned char *result = pzp_decompress_combined_from_memory(
                                                                    file_data, file_size,
                                                                    widthOutput, heightOutput,
                                                                    bitsperpixelExternalOutput, channelsExternalOutput,
                                                                    bitsperpixelInternalOutput, channelsInternalOutput,
                                                                    configuration
                                                                  );

      free(file_data);
      return result;
    }

    fprintf(stderr, "Failed to read file: %s\n", input_filename);
    return NULL;
}

#ifdef __cplusplus
}
#endif

#endif
