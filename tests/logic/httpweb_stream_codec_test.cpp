// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include "streamcodec.h"

#include <algorithm>
#include <random>
#include <string>

using namespace dtp;

namespace {

std::string makePayload(size_t size, unsigned seed)
{
    std::string out;
    out.reserve(size);
    std::mt19937 rng(seed);
    for (size_t i = 0; i < size; ++i) {
        if ((i / 97) % 3 == 0)
            out.push_back(static_cast<char>(rng() & 0xff));
        else
            out.push_back(static_cast<char>('a' + (i % 26)));
    }
    return out;
}

struct RoundTrip {
    bool ok {false};
    std::string decoded;
    std::string error;
};

RoundTrip roundTrip(Codec codec, const std::string &input, size_t chunk, bool flushEveryChunk = false)
{
    RoundTrip result;
    if (chunk == 0) {
        result.error = "chunk must be > 0";
        return result;
    }

    StreamEncoder encoder;
    if (!encoder.init(codec)) {
        result.error = "encoder init: " + encoder.lastError();
        return result;
    }

    std::string compressed;
    for (size_t offset = 0; offset < input.size(); offset += chunk) {
        const size_t size = std::min(chunk, input.size() - offset);
        if (!encoder.encode(input.data() + offset, size, &compressed)) {
            result.error = "encode: " + encoder.lastError();
            return result;
        }
        if (flushEveryChunk && !encoder.flush(&compressed)) {
            result.error = "flush: " + encoder.lastError();
            return result;
        }
    }
    if (!encoder.finish(&compressed)) {
        result.error = "finish: " + encoder.lastError();
        return result;
    }

    StreamDecoder decoder;
    if (!decoder.init(codec)) {
        result.error = "decoder init: " + decoder.lastError();
        return result;
    }

    result.decoded.reserve(input.size());
    for (size_t offset = 0; offset < compressed.size(); offset += chunk) {
        const size_t size = std::min(chunk, compressed.size() - offset);
        if (!decoder.decode(compressed.data() + offset, size, &result.decoded)) {
            result.error = "decode: " + decoder.lastError();
            return result;
        }
    }

    if (codec != Codec::None && !decoder.finished()) {
        result.error = "decoder did not reach the end of the compressed stream";
        return result;
    }
    result.ok = (result.decoded == input);
    if (!result.ok)
        result.error = "payload mismatch";
    return result;
}

const Codec kSupportedCodecs[] = {Codec::None, Codec::Deflate, Codec::Zstd};

} // namespace

TEST(HttpWebStreamCodecTest, CodecNamesRoundTrip)
{
    EXPECT_STREQ(codecName(Codec::None), "none");
    EXPECT_STREQ(codecName(Codec::Deflate), "deflate");
    EXPECT_STREQ(codecName(Codec::Zstd), "zstd");

    Codec parsed = Codec::None;
    EXPECT_TRUE(codecFromName("deflate", &parsed));
    EXPECT_EQ(parsed, Codec::Deflate);
    EXPECT_TRUE(codecFromName("zstd", &parsed));
    EXPECT_EQ(parsed, Codec::Zstd);
    EXPECT_TRUE(codecFromName("none", &parsed));
    EXPECT_EQ(parsed, Codec::None);

    EXPECT_FALSE(codecFromName("gzip", &parsed));
    EXPECT_FALSE(codecFromName("", &parsed));
    EXPECT_FALSE(codecFromName("DEFLATE", &parsed));
    EXPECT_FALSE(codecFromName("deflate", nullptr));
}

TEST(HttpWebStreamCodecTest, BaselineCodecsAreAlwaysAvailable)
{
    EXPECT_TRUE(codecSupported(Codec::None));
    EXPECT_TRUE(codecSupported(Codec::Deflate));
#ifdef HTTPWEB_HAVE_ZSTD
    EXPECT_TRUE(codecSupported(Codec::Zstd));
#endif
}

TEST(HttpWebStreamCodecTest, CompressionLevels)
{
    EXPECT_EQ(defaultCompressionLevel(Codec::None), 0);
    EXPECT_EQ(defaultCompressionLevel(Codec::Deflate), 6);
    EXPECT_EQ(defaultCompressionLevel(Codec::Zstd), 3);

    EXPECT_EQ(clampCompressionLevel(Codec::Deflate, 1), 1);
    EXPECT_EQ(clampCompressionLevel(Codec::Deflate, 9), 9);
    EXPECT_EQ(clampCompressionLevel(Codec::Deflate, 42), 9);
    EXPECT_EQ(clampCompressionLevel(Codec::Deflate, 0), 6);
    EXPECT_EQ(clampCompressionLevel(Codec::Deflate, -3), 6);

    EXPECT_EQ(clampCompressionLevel(Codec::Zstd, 1), 1);
    EXPECT_EQ(clampCompressionLevel(Codec::Zstd, 19), 19);
    EXPECT_EQ(clampCompressionLevel(Codec::Zstd, 200), 19);
    EXPECT_EQ(clampCompressionLevel(Codec::Zstd, 0), 3);
}

TEST(HttpWebStreamCodecTest, InvalidCodecIsRejected)
{
    StreamEncoder encoder;
    EXPECT_FALSE(encoder.init(static_cast<Codec>(99)));
    EXPECT_FALSE(encoder.lastError().empty());

    StreamDecoder decoder;
    EXPECT_FALSE(decoder.init(static_cast<Codec>(99)));
    EXPECT_FALSE(decoder.lastError().empty());
}

TEST(HttpWebStreamCodecTest, UseBeforeInitFails)
{
    StreamEncoder encoder;
    std::string out;
    EXPECT_FALSE(encoder.encode("x", 1, &out));
    EXPECT_FALSE(encoder.flush(&out));
    EXPECT_FALSE(encoder.finish(&out));

    StreamDecoder decoder;
    EXPECT_FALSE(decoder.decode("x", 1, &out));
}

TEST(HttpWebStreamCodecTest, EncodeAfterFinishFails)
{
    StreamEncoder encoder;
    ASSERT_TRUE(encoder.init(Codec::Deflate));
    std::string out;
    ASSERT_TRUE(encoder.encode("abc", 3, &out));
    ASSERT_TRUE(encoder.finish(&out));
    EXPECT_FALSE(encoder.encode("more", 4, &out));
    EXPECT_FALSE(encoder.flush(&out));
}

TEST(HttpWebStreamCodecTest, DecodeAfterEndOfStreamFails)
{
    std::string compressed;
    {
        StreamEncoder encoder;
        ASSERT_TRUE(encoder.init(Codec::Deflate));
        ASSERT_TRUE(encoder.encode("abc", 3, &compressed));
        ASSERT_TRUE(encoder.finish(&compressed));
    }
    StreamDecoder decoder;
    ASSERT_TRUE(decoder.init(Codec::Deflate));
    std::string out;
    ASSERT_TRUE(decoder.decode(compressed.data(), compressed.size(), &out));
    EXPECT_TRUE(decoder.finished());
    EXPECT_EQ(out, "abc");

    std::string extra = compressed + "trailing";
    StreamDecoder again;
    ASSERT_TRUE(again.init(Codec::Deflate));
    std::string out2;
    EXPECT_FALSE(again.decode(extra.data(), extra.size(), &out2));
}

TEST(HttpWebStreamCodecTest, NoneCodecIsPassThrough)
{
    const std::string input = makePayload(4096, 1);
    StreamEncoder encoder;
    ASSERT_TRUE(encoder.init(Codec::None));
    std::string out;
    ASSERT_TRUE(encoder.encode(input.data(), input.size(), &out));
    ASSERT_TRUE(encoder.flush(&out));
    ASSERT_TRUE(encoder.finish(&out));
    EXPECT_EQ(out, input);

    StreamDecoder decoder;
    ASSERT_TRUE(decoder.init(Codec::None));
    std::string decoded;
    ASSERT_TRUE(decoder.decode(out.data(), out.size(), &decoded));
    EXPECT_EQ(decoded, input);
    // Codec::None has no end marker; the archive END frame terminates the stream.
    EXPECT_FALSE(decoder.finished());
    EXPECT_EQ(decoder.totalOutput(), input.size());
}

TEST(HttpWebStreamCodecTest, RoundTripEverySupportedCodec)
{
    const size_t sizes[] = {0, 1, 2, 17, 4096, 100000, 1u << 20};
    const size_t chunks[] = {1, 7, 4096, 65536};
    for (Codec codec : kSupportedCodecs) {
        if (!codecSupported(codec))
            continue;
        for (size_t size : sizes) {
            const std::string input = makePayload(size, static_cast<unsigned>(size));
            for (size_t chunk : chunks) {
                const RoundTrip result = roundTrip(codec, input, chunk);
                EXPECT_TRUE(result.ok) << "codec=" << codecName(codec) << " size=" << size
                                       << " chunk=" << chunk << " error=" << result.error;
            }
        }
    }
}

TEST(HttpWebStreamCodecTest, RoundTripWithFlushAtEveryChunk)
{
    for (Codec codec : kSupportedCodecs) {
        if (!codecSupported(codec))
            continue;
        const std::string input = makePayload(70000, 42);
        const RoundTrip result = roundTrip(codec, input, 997, true);
        EXPECT_TRUE(result.ok) << "codec=" << codecName(codec) << " error=" << result.error;
    }
}

TEST(HttpWebStreamCodecTest, FlushProducesDecodablePartialOutput)
{
    for (Codec codec : kSupportedCodecs) {
        if (!codecSupported(codec) || codec == Codec::None)
            continue;

        StreamEncoder encoder;
        ASSERT_TRUE(encoder.init(codec));
        std::string stream;
        ASSERT_TRUE(encoder.encode("first-part", 10, &stream));
        ASSERT_TRUE(encoder.flush(&stream));
        const size_t flushed = stream.size();

        StreamDecoder decoder;
        ASSERT_TRUE(decoder.init(codec));
        std::string partial;
        ASSERT_TRUE(decoder.decode(stream.data(), flushed, &partial));
        EXPECT_EQ(partial, "first-part") << "codec=" << codecName(codec);
        EXPECT_FALSE(decoder.finished());

        ASSERT_TRUE(encoder.encode("second-part", 11, &stream));
        ASSERT_TRUE(encoder.finish(&stream));

        std::string rest;
        ASSERT_TRUE(decoder.decode(stream.data() + flushed, stream.size() - flushed, &rest));
        EXPECT_EQ(partial + rest, "first-partsecond-part") << "codec=" << codecName(codec);
        EXPECT_TRUE(decoder.finished());
    }
}

TEST(HttpWebStreamCodecTest, DecoderRejectsTruncatedStream)
{
    for (Codec codec : kSupportedCodecs) {
        if (!codecSupported(codec) || codec == Codec::None)
            continue;

        const std::string input = makePayload(50000, 7);
        StreamEncoder encoder;
        ASSERT_TRUE(encoder.init(codec));
        std::string compressed;
        ASSERT_TRUE(encoder.encode(input.data(), input.size(), &compressed));
        ASSERT_TRUE(encoder.finish(&compressed));

        const size_t cut = compressed.size() / 2;
        StreamDecoder decoder;
        ASSERT_TRUE(decoder.init(codec));
        std::string decoded;
        decoder.decode(compressed.data(), cut, &decoded);
        EXPECT_FALSE(decoder.finished()) << "codec=" << codecName(codec);
        EXPECT_TRUE(decoded.size() < input.size());
    }
}

TEST(HttpWebStreamCodecTest, DecoderRejectsCorruptedPayload)
{
    for (Codec codec : kSupportedCodecs) {
        if (!codecSupported(codec) || codec == Codec::None)
            continue;

        const std::string input = makePayload(20000, 11);
        StreamEncoder encoder;
        ASSERT_TRUE(encoder.init(codec));
        std::string compressed;
        ASSERT_TRUE(encoder.encode(input.data(), input.size(), &compressed));
        ASSERT_TRUE(encoder.finish(&compressed));

        std::string corrupted = compressed;
        corrupted[corrupted.size() / 2] ^= 0x5a;
        StreamDecoder decoder;
        ASSERT_TRUE(decoder.init(codec));
        std::string decoded;
        const bool ok = decoder.decode(corrupted.data(), corrupted.size(), &decoded);
        // Corruption must be detected, or at least must not reproduce the input.
        EXPECT_TRUE(!ok || decoded != input) << "codec=" << codecName(codec);
    }
}

TEST(HttpWebStreamCodecTest, DecoderHonoursOutputLimit)
{
    for (Codec codec : kSupportedCodecs) {
        if (!codecSupported(codec))
            continue;

        const std::string input = makePayload(10000, 3);
        StreamEncoder encoder;
        ASSERT_TRUE(encoder.init(codec));
        std::string compressed;
        ASSERT_TRUE(encoder.encode(input.data(), input.size(), &compressed));
        ASSERT_TRUE(encoder.finish(&compressed));

        StreamDecoder decoder;
        ASSERT_TRUE(decoder.init(codec, 100));
        std::string decoded;
        EXPECT_FALSE(decoder.decode(compressed.data(), compressed.size(), &decoded));
        EXPECT_FALSE(decoder.lastError().empty());
    }
}

TEST(HttpWebStreamCodecTest, ResetAllowsReuse)
{
    StreamEncoder encoder;
    ASSERT_TRUE(encoder.init(Codec::Deflate));
    std::string first;
    ASSERT_TRUE(encoder.encode("abc", 3, &first));
    ASSERT_TRUE(encoder.finish(&first));
    encoder.reset();
    ASSERT_TRUE(encoder.init(Codec::Deflate));
    std::string second;
    ASSERT_TRUE(encoder.encode("abc", 3, &second));
    ASSERT_TRUE(encoder.finish(&second));
    EXPECT_EQ(first, second);
}

TEST(HttpWebStreamCodecTest, CompressedTextIsSmaller)
{
    std::string input;
    for (int i = 0; i < 20000; ++i)
        input += "the quick brown fox jumps over the lazy dog\n";

    StreamEncoder encoder;
    ASSERT_TRUE(encoder.init(Codec::Deflate));
    std::string compressed;
    ASSERT_TRUE(encoder.encode(input.data(), input.size(), &compressed));
    ASSERT_TRUE(encoder.finish(&compressed));
    EXPECT_LT(compressed.size(), input.size() / 4);

#ifdef HTTPWEB_HAVE_ZSTD
    StreamEncoder zstdEncoder;
    ASSERT_TRUE(zstdEncoder.init(Codec::Zstd));
    std::string zstdData;
    ASSERT_TRUE(zstdEncoder.encode(input.data(), input.size(), &zstdData));
    ASSERT_TRUE(zstdEncoder.finish(&zstdData));
    EXPECT_LT(zstdData.size(), input.size() / 4);
#endif
}
