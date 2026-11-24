// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "streamcodec.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <vector>

#include <zlib.h>

#ifdef HTTPWEB_HAVE_ZSTD
#include <zstd.h>
#endif

namespace dtp {
namespace {

constexpr size_t kChunkSize = 64 * 1024;
// zlib takes uInt for avail_in/avail_out; feed well below UINT_MAX so the cast
// can never truncate on platforms where unsigned int is 32 bit.
constexpr size_t kMaxZlibFeed = 1u << 30;

std::string zlibMessage(int code)
{
    switch (code) {
    case Z_ERRNO:
        return "zlib errno";
    case Z_STREAM_ERROR:
        return "zlib stream error";
    case Z_DATA_ERROR:
        return "zlib data error (corrupt input)";
    case Z_MEM_ERROR:
        return "zlib out of memory";
    case Z_BUF_ERROR:
        return "zlib buffer error";
    case Z_VERSION_ERROR:
        return "zlib version mismatch";
    default:
        return "zlib error " + std::to_string(code);
    }
}

} // namespace

const char *codecName(Codec codec)
{
    switch (codec) {
    case Codec::None:
        return "none";
    case Codec::Deflate:
        return "deflate";
    case Codec::Zstd:
        return "zstd";
    }
    return "";
}

bool codecFromName(const std::string &name, Codec *out)
{
    if (!out)
        return false;
    if (name == "none")
        *out = Codec::None;
    else if (name == "deflate")
        *out = Codec::Deflate;
    else if (name == "zstd")
        *out = Codec::Zstd;
    else
        return false;
    return true;
}

bool codecSupported(Codec codec)
{
    switch (codec) {
    case Codec::None:
    case Codec::Deflate:
        return true;
    case Codec::Zstd:
#ifdef HTTPWEB_HAVE_ZSTD
        return true;
#else
        return false;
#endif
    }
    return false;
}

int defaultCompressionLevel(Codec codec)
{
    switch (codec) {
    case Codec::Deflate:
        return 6;
    case Codec::Zstd:
        return 3;
    case Codec::None:
        break;
    }
    return 0;
}

int clampCompressionLevel(Codec codec, int level)
{
    if (level < 1)
        return defaultCompressionLevel(codec);
    switch (codec) {
    case Codec::Deflate:
        return std::min(level, 9);
    case Codec::Zstd:
        // 19 is the highest classic level; the protocol recommends 1..3.
        return std::min(level, 19);
    case Codec::None:
        break;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// StreamEncoder
// ---------------------------------------------------------------------------

struct StreamEncoder::Impl {
    Codec codec {Codec::Deflate};
    int level {0};
    bool initialized {false};
    bool finished {false};
    std::string error;
    std::vector<char> buffer = std::vector<char>(kChunkSize);

    z_stream zs;
    bool zsActive {false};

#ifdef HTTPWEB_HAVE_ZSTD
    ZSTD_CCtx *cctx {nullptr};
#endif

    bool fail(const std::string &message)
    {
        error = message;
        return false;
    }

    void release()
    {
        if (zsActive) {
            deflateEnd(&zs);
            zsActive = false;
        }
#ifdef HTTPWEB_HAVE_ZSTD
        if (cctx) {
            ZSTD_freeCCtx(cctx);
            cctx = nullptr;
        }
#endif
    }
};

StreamEncoder::StreamEncoder()
    : d(new Impl)
{
}

StreamEncoder::~StreamEncoder()
{
    d->release();
}

bool StreamEncoder::init(Codec codec, int level)
{
    d->release();
    d->codec = codec;
    d->level = clampCompressionLevel(codec, level);
    d->finished = false;
    d->error.clear();
    d->initialized = false;

    if (!codecSupported(codec))
        return d->fail(std::string("codec not available in this build: ") + codecName(codec));

    switch (codec) {
    case Codec::None:
        break;
    case Codec::Deflate: {
        std::memset(&d->zs, 0, sizeof(d->zs));
        int rc = deflateInit2(&d->zs, d->level, Z_DEFLATED, MAX_WBITS, 8, Z_DEFAULT_STRATEGY);
        if (rc != Z_OK)
            return d->fail("deflateInit2 failed: " + zlibMessage(rc));
        d->zsActive = true;
        break;
    }
    case Codec::Zstd: {
#ifdef HTTPWEB_HAVE_ZSTD
        d->cctx = ZSTD_createCCtx();
        if (!d->cctx)
            return d->fail("ZSTD_createCCtx failed");
        size_t rc = ZSTD_CCtx_setParameter(d->cctx, ZSTD_c_compressionLevel, d->level);
        if (ZSTD_isError(rc))
            return d->fail(std::string("zstd set level failed: ") + ZSTD_getErrorName(rc));
        // Keep the wire format plain: standard frames, no dictionary, no checksum.
        rc = ZSTD_CCtx_setParameter(d->cctx, ZSTD_c_checksumFlag, 0);
        if (ZSTD_isError(rc))
            return d->fail(std::string("zstd set checksum failed: ") + ZSTD_getErrorName(rc));
        rc = ZSTD_CCtx_setParameter(d->cctx, ZSTD_c_contentSizeFlag, 0);
        if (ZSTD_isError(rc))
            return d->fail(std::string("zstd set content size failed: ") + ZSTD_getErrorName(rc));
#else
        return d->fail("zstd support not compiled in");
#endif
        break;
    }
    }

    d->initialized = true;
    return true;
}

bool StreamEncoder::encode(const char *data, size_t size, std::string *out)
{
    if (!out)
        return d->fail("null output buffer");
    if (!d->initialized)
        return d->fail("encoder not initialised");
    if (d->finished)
        return d->fail("encode after finish");
    if (size == 0)
        return true;
    if (!data)
        return d->fail("null input with nonzero size");

    if (d->codec == Codec::None) {
        out->append(data, size);
        return true;
    }

    if (d->codec == Codec::Deflate) {
        z_stream &zs = d->zs;
        size_t offset = 0;
        while (offset < size) {
            uInt feed = static_cast<uInt>(std::min(size - offset, kMaxZlibFeed));
            zs.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(data + offset));
            zs.avail_in = feed;
            offset += feed;

            while (zs.avail_in > 0) {
                uInt before = zs.avail_in;
                zs.next_out = reinterpret_cast<Bytef *>(d->buffer.data());
                zs.avail_out = static_cast<uInt>(d->buffer.size());
                int rc = deflate(&zs, Z_NO_FLUSH);
                size_t produced = d->buffer.size() - zs.avail_out;
                if (produced > 0)
                    out->append(d->buffer.data(), produced);
                if (rc != Z_OK && rc != Z_BUF_ERROR)
                    return d->fail("deflate failed: " + zlibMessage(rc));
                if (rc == Z_BUF_ERROR && before == zs.avail_in && produced == 0)
                    return d->fail("deflate stalled with pending input");
            }
        }
        return true;
    }

#ifdef HTTPWEB_HAVE_ZSTD
    ZSTD_inBuffer in {data, size, 0};
    while (in.pos < in.size) {
        ZSTD_outBuffer ob {d->buffer.data(), d->buffer.size(), 0};
        size_t rc = ZSTD_compressStream2(d->cctx, &ob, &in, ZSTD_e_continue);
        if (ZSTD_isError(rc))
            return d->fail(std::string("zstd compress failed: ") + ZSTD_getErrorName(rc));
        if (ob.pos > 0)
            out->append(d->buffer.data(), ob.pos);
        if (ob.pos == 0 && rc == 0 && in.pos < in.size)
            return d->fail("zstd compress made no progress");
    }
    return true;
#else
    return d->fail("zstd support not compiled in");
#endif
}

bool StreamEncoder::flush(std::string *out)
{
    if (!out)
        return d->fail("null output buffer");
    if (!d->initialized)
        return d->fail("encoder not initialised");
    if (d->finished)
        return d->fail("flush after finish");

    if (d->codec == Codec::None)
        return true;

    if (d->codec == Codec::Deflate) {
        z_stream &zs = d->zs;
        zs.next_in = Z_NULL;
        zs.avail_in = 0;
        for (;;) {
            zs.next_out = reinterpret_cast<Bytef *>(d->buffer.data());
            zs.avail_out = static_cast<uInt>(d->buffer.size());
            int rc = deflate(&zs, Z_SYNC_FLUSH);
            size_t produced = d->buffer.size() - zs.avail_out;
            if (produced > 0)
                out->append(d->buffer.data(), produced);
            if (rc != Z_OK && rc != Z_BUF_ERROR)
                return d->fail("deflate flush failed: " + zlibMessage(rc));
            // Z_SYNC_FLUSH is complete when the output buffer was not filled.
            if (zs.avail_out != 0)
                return true;
        }
    }

#ifdef HTTPWEB_HAVE_ZSTD
    ZSTD_inBuffer in {nullptr, 0, 0};
    for (;;) {
        ZSTD_outBuffer ob {d->buffer.data(), d->buffer.size(), 0};
        size_t rc = ZSTD_compressStream2(d->cctx, &ob, &in, ZSTD_e_flush);
        if (ZSTD_isError(rc))
            return d->fail(std::string("zstd flush failed: ") + ZSTD_getErrorName(rc));
        if (ob.pos > 0)
            out->append(d->buffer.data(), ob.pos);
        if (rc == 0)
            return true;
    }
#else
    return d->fail("zstd support not compiled in");
#endif
}

bool StreamEncoder::finish(std::string *out)
{
    if (!out)
        return d->fail("null output buffer");
    if (!d->initialized)
        return d->fail("encoder not initialised");
    if (d->finished)
        return true;

    if (d->codec == Codec::None) {
        d->finished = true;
        return true;
    }

    if (d->codec == Codec::Deflate) {
        z_stream &zs = d->zs;
        zs.next_in = Z_NULL;
        zs.avail_in = 0;
        for (;;) {
            zs.next_out = reinterpret_cast<Bytef *>(d->buffer.data());
            zs.avail_out = static_cast<uInt>(d->buffer.size());
            int rc = deflate(&zs, Z_FINISH);
            size_t produced = d->buffer.size() - zs.avail_out;
            if (produced > 0)
                out->append(d->buffer.data(), produced);
            if (rc == Z_STREAM_END) {
                d->finished = true;
                return true;
            }
            if (rc != Z_OK && rc != Z_BUF_ERROR)
                return d->fail("deflate finish failed: " + zlibMessage(rc));
            if (rc == Z_BUF_ERROR && produced == 0)
                return d->fail("deflate finish stalled");
        }
    }

#ifdef HTTPWEB_HAVE_ZSTD
    ZSTD_inBuffer in {nullptr, 0, 0};
    for (;;) {
        ZSTD_outBuffer ob {d->buffer.data(), d->buffer.size(), 0};
        size_t rc = ZSTD_compressStream2(d->cctx, &ob, &in, ZSTD_e_end);
        if (ZSTD_isError(rc))
            return d->fail(std::string("zstd finish failed: ") + ZSTD_getErrorName(rc));
        if (ob.pos > 0)
            out->append(d->buffer.data(), ob.pos);
        if (rc == 0) {
            d->finished = true;
            return true;
        }
    }
#else
    return d->fail("zstd support not compiled in");
#endif
}

void StreamEncoder::reset()
{
    d->release();
    d->initialized = false;
    d->finished = false;
    d->error.clear();
}

bool StreamEncoder::valid() const
{
    return d->initialized && d->error.empty();
}

Codec StreamEncoder::codec() const
{
    return d->codec;
}

const std::string &StreamEncoder::lastError() const
{
    return d->error;
}

// ---------------------------------------------------------------------------
// StreamDecoder
// ---------------------------------------------------------------------------

struct StreamDecoder::Impl {
    Codec codec {Codec::Deflate};
    bool initialized {false};
    bool finished {false};
    uint64_t totalOutput {0};
    size_t maxOutput {0};
    std::string error;
    std::vector<char> buffer = std::vector<char>(kChunkSize);

    z_stream zs;
    bool zsActive {false};

#ifdef HTTPWEB_HAVE_ZSTD
    ZSTD_DCtx *dctx {nullptr};
#endif

    bool fail(const std::string &message)
    {
        error = message;
        return false;
    }

    bool account(size_t produced)
    {
        if (maxOutput != 0 && totalOutput + produced > maxOutput)
            return fail("decoded output exceeds configured limit");
        totalOutput += produced;
        return true;
    }

    void release()
    {
        if (zsActive) {
            inflateEnd(&zs);
            zsActive = false;
        }
#ifdef HTTPWEB_HAVE_ZSTD
        if (dctx) {
            ZSTD_freeDCtx(dctx);
            dctx = nullptr;
        }
#endif
    }
};

StreamDecoder::StreamDecoder()
    : d(new Impl)
{
}

StreamDecoder::~StreamDecoder()
{
    d->release();
}

bool StreamDecoder::init(Codec codec, size_t maxOutputBytes)
{
    d->release();
    d->codec = codec;
    d->finished = false;
    d->totalOutput = 0;
    d->maxOutput = maxOutputBytes;
    d->error.clear();
    d->initialized = false;

    if (!codecSupported(codec))
        return d->fail(std::string("codec not available in this build: ") + codecName(codec));

    switch (codec) {
    case Codec::None:
        break;
    case Codec::Deflate: {
        std::memset(&d->zs, 0, sizeof(d->zs));
        int rc = inflateInit2(&d->zs, MAX_WBITS);
        if (rc != Z_OK)
            return d->fail("inflateInit2 failed: " + zlibMessage(rc));
        d->zsActive = true;
        break;
    }
    case Codec::Zstd: {
#ifdef HTTPWEB_HAVE_ZSTD
        d->dctx = ZSTD_createDCtx();
        if (!d->dctx)
            return d->fail("ZSTD_createDCtx failed");
#else
        return d->fail("zstd support not compiled in");
#endif
        break;
    }
    }

    d->initialized = true;
    return true;
}

bool StreamDecoder::decode(const char *data, size_t size, std::string *out)
{
    if (!out)
        return d->fail("null output buffer");
    if (!d->initialized)
        return d->fail("decoder not initialised");
    if (d->finished && size > 0)
        return d->fail("compressed data after end of stream");
    if (size == 0)
        return true;
    if (!data)
        return d->fail("null input with nonzero size");

    if (d->codec == Codec::None) {
        if (!d->account(size))
            return false;
        out->append(data, size);
        return true;
    }

    if (d->codec == Codec::Deflate) {
        z_stream &zs = d->zs;
        size_t offset = 0;
        while (offset < size) {
            uInt feed = static_cast<uInt>(std::min(size - offset, kMaxZlibFeed));
            zs.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(data + offset));
            zs.avail_in = feed;
            offset += feed;

            while (zs.avail_in > 0) {
                uInt before = zs.avail_in;
                zs.next_out = reinterpret_cast<Bytef *>(d->buffer.data());
                zs.avail_out = static_cast<uInt>(d->buffer.size());
                int rc = inflate(&zs, Z_NO_FLUSH);
                size_t produced = d->buffer.size() - zs.avail_out;
                if (!d->account(produced))
                    return false;
                if (produced > 0)
                    out->append(d->buffer.data(), produced);

                if (rc == Z_STREAM_END) {
                    if (zs.avail_in > 0 || offset < size)
                        return d->fail("trailing data after end of deflate stream");
                    d->finished = true;
                    return true;
                }
                if (rc != Z_OK && rc != Z_BUF_ERROR)
                    return d->fail("inflate failed: " + zlibMessage(rc));
                if (rc == Z_BUF_ERROR && produced == 0 && before == zs.avail_in)
                    return d->fail("inflate stalled with pending input");
            }
        }
        return true;
    }

#ifdef HTTPWEB_HAVE_ZSTD
    ZSTD_inBuffer in {data, size, 0};
    for (;;) {
        ZSTD_outBuffer ob {d->buffer.data(), d->buffer.size(), 0};
        size_t rc = ZSTD_decompressStream(d->dctx, &ob, &in);
        if (ZSTD_isError(rc))
            return d->fail(std::string("zstd decompress failed: ") + ZSTD_getErrorName(rc));
        if (!d->account(ob.pos))
            return false;
        if (ob.pos > 0)
            out->append(d->buffer.data(), ob.pos);

        if (rc == 0) {
            // A complete frame was decoded. The protocol sends exactly one frame,
            // but concatenated frames are accepted and decoded back to back.
            if (in.pos == in.size) {
                d->finished = true;
                return true;
            }
            continue;
        }
        // Need more input: everything was consumed and the output buffer was
        // not filled.
        if (in.pos == in.size && ob.pos < ob.size)
            return true;
        if (in.pos == in.size && ob.pos == 0)
            return true;
    }
#else
    return d->fail("zstd support not compiled in");
#endif
}

bool StreamDecoder::finished() const
{
    return d->finished;
}

uint64_t StreamDecoder::totalOutput() const
{
    return d->totalOutput;
}

void StreamDecoder::reset()
{
    d->release();
    d->initialized = false;
    d->finished = false;
    d->totalOutput = 0;
    d->error.clear();
}

bool StreamDecoder::valid() const
{
    return d->initialized && d->error.empty();
}

Codec StreamDecoder::codec() const
{
    return d->codec;
}

const std::string &StreamDecoder::lastError() const
{
    return d->error;
}

} // namespace dtp
