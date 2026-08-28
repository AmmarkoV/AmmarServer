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

static const char pzp_version[]="v0.02";
static const char pzp_header[4]={"PZP0"};

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

static inline void pzp_thread_cleanup(void)
{
    if (_pzp_zstd_dctx)
    {
        ZSTD_freeDCtx(_pzp_zstd_dctx);
        _pzp_zstd_dctx = NULL;
    }
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
    USE_LZ4          = 1 << 4   // 10000 — use LZ4 instead of ZSTD (faster decompress on ramdisk)
} PZPFlags;

/* Bit 31 of the 4-byte frame-prefix uint32 encodes the codec.
 * Bit 31 = 0: ZSTD (all existing files — backward compatible)
 * Bit 31 = 1: LZ4
 * Bits 0–30 hold the uncompressed payload size (max ~2 GB). */
#define PZP_CODEC_LZ4_FLAG  0x80000000u
#define PZP_CODEC_SIZE_MASK 0x7FFFFFFFu

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

/* Parse palette data from src. Returns bytes consumed. */
static unsigned int pzp_palette_read(
        const unsigned char *src, unsigned int channels,
        unsigned char palette[8][256], unsigned int counts[8])
{
    unsigned int off = 0;
    for (unsigned int ch = 0; ch < channels; ch++)
    {
        counts[ch] = (unsigned int)src[off++] + 1;
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
 * NOTE: modifies buffers[] in-place (palette encoding then delta filter).
 */
static unsigned char *pzp_compress_frame_to_memory(
        unsigned char **buffers,
        unsigned int width,    unsigned int height,
        unsigned int bpp_ext,  unsigned int ch_ext,
        unsigned int bpp_int,  unsigned int ch_int,
        unsigned int configuration,
        size_t *out_size)
{
    /* ── palette encoding ── */
    unsigned char palette[8][256];
    unsigned int  palette_counts[8];
    unsigned int  paletteDataBytes = 0;

    if (configuration & USE_PALETTE)
    {
        paletteDataBytes = pzp_palette_build_and_encode(
                buffers, width * height, ch_int, palette, palette_counts);
        fprintf(stderr, "Palette mode: %u channels, palette data %u bytes\n",
                ch_int, paletteDataBytes);
        for (unsigned int ch = 0; ch < ch_int; ch++)
            fprintf(stderr, "  ch%u: %u unique values\n", ch, palette_counts[ch]);
    }

    /* ── delta filter ── */
    if (configuration & USE_RLE)
    {
        fprintf(stderr, "Using RLE for compression (mode %u)\n", configuration);
        pzp_RLE_filter(buffers, ch_int, width, height);
    }

    /* ── build uncompressed payload ── */
    unsigned int pixel_data_size = width * height * (bpp_int / 8) * ch_int;
    unsigned int payload_size    = (unsigned int)headerSize + paletteDataBytes + pixel_data_size;

    unsigned char *payload = (unsigned char *)malloc(payload_size);
    if (!payload) return NULL;

    /* inner frame header (10 × uint32 = 40 bytes) */
    unsigned int *h = (unsigned int *)payload;
    h[0] = convert_header(pzp_header);
    h[1] = bpp_ext;
    h[2] = ch_ext;
    h[3] = width;
    h[4] = height;
    h[5] = bpp_int;
    h[6] = ch_int;
    /* h[7] = checksum, filled after interleave */
    h[8] = configuration;
    h[9] = paletteDataBytes;

    /* palette prefix */
    unsigned char *write_ptr = payload + headerSize;
    if (paletteDataBytes > 0)
    {
        pzp_palette_write(write_ptr, ch_int, palette, palette_counts);
        write_ptr += paletteDataBytes;
    }

    /* interleave planar channel buffers */
    for (unsigned int i = 0; i < width * height; i++)
        for (unsigned int ch = 0; ch < ch_int; ch++)
            write_ptr[i * ch_int + ch] = buffers[ch][i];

    h[7] = hash_checksum(write_ptr, pixel_data_size);

    /* ── compress (LZ4 or ZSTD) ── */
    /* result layout: [4-byte prefix][compressed bytes]
     * prefix bit 31 = codec: 0=ZSTD, 1=LZ4  (PZP_CODEC_LZ4_FLAG)
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
            "  compress: %u B → %zu B  ratio=%.2f×%s%s\n",
            payload_size, comp_size,
            (float)payload_size / (float)comp_size,
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
           can serve as the reference for the next frame. */
        unsigned char **cur_orig = (unsigned char **)malloc(ch_int * sizeof(unsigned char *));
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

    fwrite(&hdr, sizeof(unsigned int), 12, out);

    for (unsigned int f = 0; f < frame_count; f++)
    {
        PZPFrameEntry entry;
        entry.frame_offset    = frame_offsets[f];
        entry.compressed_size = (unsigned int)frame_sizes[f];
        entry.delay_ms        = delay_ms_arr ? delay_ms_arr[f] : 0;
        entry.reserved        = 0;
        fwrite(&entry, sizeof(unsigned int), 4, out);
    }

    for (unsigned int f = 0; f < frame_count; f++)
    {
        fwrite(frame_bufs[f], 1, frame_sizes[f], out);
        free(frame_bufs[f]);
    }

    if (metadata && metadata_bytes > 0)
        fwrite(metadata, 1, metadata_bytes, out);

    if (audio && audio_bytes > 0)
        fwrite(audio, 1, audio_bytes, out);

    fclose(out);
    free(frame_bufs);
    free(frame_sizes);
    free(frame_offsets);
    return 1;
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

    unsigned int new_meta_offset  = (metadata && metadata_bytes > 0) ? data_end : 0;
    unsigned int new_audio_offset = 0;
    if (audio && audio_bytes > 0)
        new_audio_offset = new_meta_offset ? (new_meta_offset + metadata_bytes) : data_end;

    /* update header fields */
    unsigned int flags = hdr.container_flags;
    if (metadata && metadata_bytes > 0) flags |= PZP_CONTAINER_HAS_METADATA;
    if (audio    && audio_bytes    > 0) flags |= PZP_CONTAINER_HAS_AUDIO;

    hdr.container_flags = flags;
    hdr.metadata_offset = new_meta_offset;
    hdr.metadata_bytes  = (metadata && metadata_bytes > 0) ? metadata_bytes : 0;
    hdr.audio_offset    = new_audio_offset;
    hdr.audio_bytes     = (audio && audio_bytes > 0) ? audio_bytes : 0;
    hdr.audio_format    = audio_format;
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

    fwrite(&hdr, sizeof(unsigned int), 12, out);
    fwrite(entries, sizeof(unsigned int), 4 * hdr.frame_count, out);

    /* copy all frame data from original file */
    fwrite((const unsigned char *)file_data + frame_area_start, 1, total_frame_bytes, out);

    if (metadata && metadata_bytes > 0)
        fwrite(metadata, 1, metadata_bytes, out);

    if (audio && audio_bytes > 0)
        fwrite(audio, 1, audio_bytes, out);

    fclose(out);
    free(entries);
    free(file_data);
    return 1;
}

//-----------------------------------------------------------------------------------------------
/* Returns 1 on success, 0 on failure - see pzp_container_write() , the only thing this can fail on. */
static int pzp_compress_combined(unsigned char **buffers,
                              unsigned int width,unsigned int height,
                              unsigned int bitsperpixelExternal, unsigned int channelsExternal,
                              unsigned int bitsperpixelInternal, unsigned int channelsInternal,
                              unsigned int configuration,
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
                        cfgs, delays,
                        1 /* loop_count */,
                        NULL, 0,    /* no metadata */
                        NULL, 0, 0  /* no audio    */);
}

//-----------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
#if INTEL_OPTIMIZATIONS
static void pzp_prefix_sum_sse2(unsigned char *src, unsigned char *dst, unsigned int size)
{
    __m128i carry = _mm_setzero_si128();
    unsigned int i = 0;
    for (; i + 15 < size; i += 16)
    {
        __m128i v = _mm_loadu_si128((__m128i *)(src + i));
        v = _mm_add_epi8(v, _mm_slli_si128(v, 1));
        v = _mm_add_epi8(v, _mm_slli_si128(v, 2));
        v = _mm_add_epi8(v, _mm_slli_si128(v, 4));
        v = _mm_add_epi8(v, _mm_slli_si128(v, 8));
        v = _mm_add_epi8(v, carry);
        _mm_storeu_si128((__m128i *)(dst + i), v);
        unsigned char last = (unsigned char)_mm_cvtsi128_si32(_mm_srli_si128(v, 15));
        carry = _mm_set1_epi8((char)last);
    }
    for (; i < size; i++)
        dst[i] = src[i] + (i > 0 ? dst[i - 1] : 0);
}

static void pzp_extractAndReconstruct_SSE2(unsigned char *decompressed_bytes, unsigned char *reconstructed, unsigned int width, unsigned int height, unsigned int channels, int restoreRLEChannels)
{
    unsigned int total_size = width * height;
    unsigned char *src = decompressed_bytes;
    unsigned char *r   = reconstructed;

    if (restoreRLEChannels)
    {
        switch (channels)
        {
            case 1:
            {
                pzp_prefix_sum_sse2(src, r, total_size);
                break;
            }
            case 2:
            {
                // Each channel accumulates independently with stride 2.
                // Kogge-Stone with shifts 2, 4, 8 covers all 8 pixel pairs in 16 bytes.
                // Carry = last pixel (2 bytes), broadcast to all 8 pixel positions.
                __m128i carry = _mm_setzero_si128();
                unsigned int i = 0;

                for (; i + 7 < total_size; i += 8)
                {
                    __m128i v = _mm_loadu_si128((__m128i *)(src + i * 2));

                    // Within-block prefix sum per channel (stride 2)
                    v = _mm_add_epi8(v, _mm_slli_si128(v, 2));
                    v = _mm_add_epi8(v, _mm_slli_si128(v, 4));
                    v = _mm_add_epi8(v, _mm_slli_si128(v, 8));

                    // Add cross-block carry to every element
                    v = _mm_add_epi8(v, carry);
                    _mm_storeu_si128((__m128i *)(r + i * 2), v);

                    // Carry: last 2 bytes (one pixel) broadcast to all 8 pixel positions.
                    // _mm_set1_epi16 replicates a 16-bit pattern to all 8 epi16 lanes,
                    // which is exactly the [ch0_acc, ch1_acc] pair repeated 8 times.
                    int tmp = _mm_cvtsi128_si32(_mm_srli_si128(v, 14));
                    carry = _mm_set1_epi16((short)(tmp & 0xFFFF));
                }

                // Scalar tail (also handles total_size < 8)
                for (; i < total_size; i++)
                {
                    r[i * 2]     = src[i * 2]     + (i > 0 ? r[(i - 1) * 2]     : 0);
                    r[i * 2 + 1] = src[i * 2 + 1] + (i > 0 ? r[(i - 1) * 2 + 1] : 0);
                }
                break;
            }
            case 3:
            {
                // Scalar prefix sum for 3-channel interleaved data.
                r[0] = src[0]; r[1] = src[1]; r[2] = src[2];
                for (unsigned int i = 1; i < total_size; i++)
                {
                    r[i*3]   = src[i*3]   + r[(i-1)*3];
                    r[i*3+1] = src[i*3+1] + r[(i-1)*3+1];
                    r[i*3+2] = src[i*3+2] + r[(i-1)*3+2];
                }
                break;
            }
            default:
            {
                for (unsigned int ch = 0; ch < channels; ch++) { r[ch] = src[ch]; }
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
    }
    else // Non-RLE: data is already in final interleaved layout
    {
        memcpy(r, src, total_size * channels);
    }
}


static void pzp_memcpy_avx2(unsigned char *dst, unsigned char *src, unsigned int size)
{
    unsigned int i = 0;
    __m256i v;

    // Process 32 bytes at a time
    for (; i + 31 < size; i += 32)
    {
        v = _mm256_loadu_si256((__m256i *)(src + i));
        _mm256_storeu_si256((__m256i *)(dst + i), v);
    }

    // Process remaining bytes
    for (; i < size; i++)
    {
        dst[i] = src[i];
    }
}

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


static void pzp_extractAndReconstruct_AVX2(unsigned char *decompressed_bytes, unsigned char *reconstructed, unsigned int width, unsigned int height, unsigned int channels, int restoreRLEChannels)
{
    unsigned int total_size = width * height;
    unsigned char *src = decompressed_bytes;
    unsigned char *r = reconstructed;

    if (restoreRLEChannels)
    {
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
                unsigned char *ch0 = (unsigned char *)malloc(total_size);
                unsigned char *ch1 = (unsigned char *)malloc(total_size);
                unsigned char *ch2 = (unsigned char *)malloc(total_size);
                if (ch0 && ch1 && ch2) {
                    pzp_deinterleave_3ch(src, ch0, ch1, ch2, total_size);
                    pzp_prefix_sum_avx2(ch0, ch0, total_size);
                    pzp_prefix_sum_avx2(ch1, ch1, total_size);
                    pzp_prefix_sum_avx2(ch2, ch2, total_size);
                    pzp_interleave_3ch(ch0, ch1, ch2, r, total_size);
                } else {
                    // OOM fallback: scalar
                    r[0] = src[0]; r[1] = src[1]; r[2] = src[2];
                    for (unsigned int i = 1; i < total_size; ++i) {
                        r[i*3]   = src[i*3]   + r[(i-1)*3];
                        r[i*3+1] = src[i*3+1] + r[(i-1)*3+1];
                        r[i*3+2] = src[i*3+2] + r[(i-1)*3+2];
                    }
                }
                free(ch0); free(ch1); free(ch2);
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
    } else
    {
        // Non-RLE path
        switch (channels)
        {
            case 1:
                memcpy(r, src, total_size);
                break;
            case 2:
            {
                // Copy 32 bytes at a time (16 pixels)
                unsigned int i = 0;
                for (; i + 15 < total_size; i += 16)
                {
                    __m256i data = _mm256_loadu_si256((__m256i*)(src + 2 * i));
                    _mm256_storeu_si256((__m256i*)(r + 2 * i), data);
                }
                // Remaining elements
                for (; i < total_size; ++i)
                {
                    r[2 * i] = src[2 * i];
                    r[2 * i + 1] = src[2 * i + 1];
                }
                break;
            }
            case 3:
            {
                // Copy 24 bytes at a time (8 pixels)
                unsigned int i = 0;
                for (; i + 7 < total_size; i += 8)
                {
                    __m256i data = _mm256_loadu_si256((__m256i*)(src + 3 * i));
                    _mm256_storeu_si256((__m256i*)(r + 3 * i), data);
                }
                // Remaining elements
                for (; i < total_size; ++i)
                {
                    r[3 * i]     = src[3 * i];
                    r[3 * i + 1] = src[3 * i + 1];
                    r[3 * i + 2] = src[3 * i + 2];
                }
                break;
            }
            default: {
                // Generic case (scalar fallback)
                for (unsigned int i = 0; i < total_size; ++i)
                {
                    for (unsigned int ch = 0; ch < channels; ++ch)
                    {
                        r[i * channels + ch] = src[i * channels + ch];
                    }
                }
                break;
            }
        }
    }
}
#endif // INTEL_OPTIMIZATIONS
static void pzp_extractAndReconstruct_Naive(unsigned char *decompressed_bytes, unsigned char *reconstructed, unsigned int width, unsigned int height, unsigned int channels, int restoreRLEChannels)
{
    unsigned int total_size = width * height;
    unsigned char *src = decompressed_bytes;
    unsigned char *r   = reconstructed;

    if (restoreRLEChannels)
    {
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
    else // Non-RLE path: data is already in final interleaved layout
    {
        memcpy(reconstructed, src, total_size * channels);
    }
}
//-----------------------------------------------------------------------------------------------
static void pzp_extractAndReconstruct(unsigned char *decompressed_bytes, unsigned char *reconstructed, unsigned int width, unsigned int height, unsigned int channels, int restoreRLEChannels)
{
   #if INTEL_OPTIMIZATIONS
     pzp_extractAndReconstruct_AVX2(decompressed_bytes,reconstructed,width,height,channels,restoreRLEChannels);
   #else
     pzp_extractAndReconstruct_Naive(decompressed_bytes,reconstructed,width,height,channels,restoreRLEChannels);
   #endif // INTEL_OPTIMIZATIONS
}

//-----------------------------------------------------------------------------------------------
// ─── Inner frame decompressor (from memory) ──────────────────────────────────
//
// Forward-declared here so container reader functions below can call it.
// Full definition follows after this section.
//-----------------------------------------------------------------------------------------------
static unsigned char* pzp_decompress_combined_from_memory(
                                const void *file_data, size_t file_size,
                                unsigned int *widthOutput, unsigned int *heightOutput,
                                unsigned int *bitsperpixelExternalOutput, unsigned int *channelsExternalOutput,
                                unsigned int *bitsperpixelInternalOutput, unsigned int *channelsInternalOutput,
                                unsigned int *configuration);

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

/*
 * Decompress frame 'frame_index' from a container held in memory.
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

    PZPFrameEntry e = entries[frame_index];
    free(entries);

    if ((size_t)e.frame_offset + e.compressed_size > file_size)
    {
        fprintf(stderr, "pzp: frame %u extends beyond file end\n", frame_index);
        return NULL;
    }

    const void *frame_data = (const unsigned char *)file_data + e.frame_offset;
    unsigned char *out = pzp_decompress_combined_from_memory(
            frame_data, e.compressed_size,
            width, height, bpp_ext, ch_ext, bpp_int, ch_int, configuration);
    if (!out) return NULL;

    /* If this frame was encoded as a delta, reconstruct by adding frame[N-1]. */
    if ((*configuration & USE_INTER_DELTA) && frame_index > 0)
    {
        unsigned int pw = 0, ph = 0, pbe = 0, pce = 0, pbi = 0, pci = 0, pcfg = 0;
        unsigned char *prev = pzp_container_read_frame_from_memory(
                file_data, file_size, frame_index - 1,
                &pw, &ph, &pbe, &pce, &pbi, &pci, &pcfg);
        if (prev && pw == *width && ph == *height && pci == *ch_int)
        {
            /* Internal buffers are always 8-bit planar; plain byte addition
               reconstructs the original pixel values (wrapping mod 256). */
            size_t n = (size_t)(*width) * (*height) * (*ch_int);
            for (size_t i = 0; i < n; i++)
                out[i] += prev[i];
        }
        free(prev);
    }

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

static unsigned char* pzp_decompress_combined_from_memory(
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

    /* ── Container detection ────────────────────────────────────────────────
     * If the first 4 bytes equal the PZP0 magic this is a new-format container.
     * Read frame 0 through the container reader.
     * The inner frame data the container reader passes back here will NOT start
     * with the magic (it starts with a small uncompressed_size uint32), so
     * there is no infinite recursion risk.
     */
    if (file_size >= (size_t)containerHeaderSize)
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

    /* ── Legacy inner-frame format ──────────────────────────────────────── */
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

    unsigned int runtimeVersion = convert_header(pzp_header);
    if (runtimeVersion != *headerSource)
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

    // After the 40-byte header comes optional palette data, then the pixel/index data.
    unsigned char *after_header = (unsigned char *)decompressed_buffer + headerSize;
    unsigned char *index_data   = after_header + paletteDataBytes;

    // Parse palette (if present) before checksum so we can validate index data.
    unsigned char palette[8][256];
    unsigned int  palette_counts[8];
    if (compressionCfg & USE_PALETTE)
        pzp_palette_read(after_header, channelsIn, palette, palette_counts);

    size_t pixel_size = (size_t)width * height * (bitsperpixelIn / 8) * channelsIn;

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

    unsigned int restoreRLEChannels = compressionCfg & USE_RLE;

    // ── Non-RLE path ──────────────────────────────────────────────────────────
    if (!restoreRLEChannels)
    {
        memmove(decompressed_buffer, index_data, pixel_size);
        if (compressionCfg & USE_PALETTE)
            pzp_palette_apply((unsigned char *)decompressed_buffer,
                              width * height, channelsIn, palette);
        return (unsigned char *)decompressed_buffer;
    }

    // ── RLE path ──────────────────────────────────────────────────────────────
    unsigned char *reconstructed = malloc(pixel_size);
    if (reconstructed == NULL)
    {
        free(decompressed_buffer);
        return NULL;
    }
    pzp_extractAndReconstruct(index_data, reconstructed, width, height, channelsIn, restoreRLEChannels);
    free(decompressed_buffer);

    if (compressionCfg & USE_PALETTE)
        pzp_palette_apply(reconstructed, width * height, channelsIn, palette);

    return reconstructed;
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
