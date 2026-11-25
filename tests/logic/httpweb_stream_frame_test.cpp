// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include "streamcodec.h"
#include "streamframe.h"

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

using namespace dtp;

namespace {

void appendLe16(std::string &out, uint16_t value)
{
    out.push_back(static_cast<char>(value & 0xff));
    out.push_back(static_cast<char>((value >> 8) & 0xff));
}

void appendLe32(std::string &out, uint32_t value)
{
    for (int shift = 0; shift < 32; shift += 8)
        out.push_back(static_cast<char>((value >> shift) & 0xff));
}

void appendLe64(std::string &out, uint64_t value)
{
    for (int shift = 0; shift < 64; shift += 8)
        out.push_back(static_cast<char>((value >> shift) & 0xff));
}

// Builds a frame by hand. The writer intentionally refuses unsafe frames, so
// parser side rejection is tested with crafted bytes.
std::string craftFrameBytes(FrameType type, const std::string &path, uint64_t size,
                            uint64_t offset, const std::string &payload, uint16_t reserved = 0)
{
    std::string out;
    out.push_back(static_cast<char>(static_cast<uint8_t>(type)));
    out.push_back('\0');
    appendLe16(out, reserved);
    appendLe32(out, static_cast<uint32_t>(path.size()));
    out.append(path);
    appendLe64(out, size);
    appendLe64(out, offset);
    appendLe32(out, 0);
    appendLe64(out, 0);
    out.append(payload);
    return out;
}

struct Collected {
    std::vector<FrameHeader> headers;
    std::vector<std::string> files;
    std::vector<std::pair<FrameHeader, std::string>> symlinks;
    std::vector<std::string> errors;
    FrameEnd end;
    bool sawEnd {false};
};

void attach(FrameParser *parser, Collected *collected)
{
    parser->onHeader([collected](const FrameHeader &header) {
        collected->headers.push_back(header);
        if (header.type == FrameType::File || header.type == FrameType::RootFile)
            collected->files.emplace_back();
        return true;
    });
    parser->onData([collected](const char *data, size_t size) {
        if (collected->files.empty())
            return false;
        collected->files.back().append(data, size);
        return true;
    });
    parser->onError([collected](const std::string &path, const std::string &message) {
        collected->errors.push_back(path + "|" + message);
        return true;
    });
    parser->onSymlink([collected](const FrameHeader &header, const std::string &target) {
        collected->symlinks.emplace_back(header, target);
        return true;
    });
    parser->onEnd([collected](const FrameEnd &end) {
        collected->end = end;
        collected->sawEnd = true;
        return true;
    });
}

struct SampleArchive {
    std::string bytes;
    std::string fileA {"hello, dtp2 world\n"};
    std::string fileB {std::string("\x00\x01\x02\xff", 4)};
};

SampleArchive buildSampleArchive()
{
    SampleArchive archive;
    FrameWriter writer;
    EXPECT_TRUE(writer.writeRootDir(&archive.bytes, "photos"));
    EXPECT_TRUE(writer.writeDir(&archive.bytes, "2024"));
    EXPECT_TRUE(writer.beginFile(&archive.bytes, "2024/a.txt", archive.fileA.size()));
    EXPECT_TRUE(writer.appendFileData(&archive.bytes, archive.fileA));
    EXPECT_TRUE(writer.endFile(&archive.bytes));
    EXPECT_TRUE(writer.beginFile(&archive.bytes, "2024/empty.txt", 0));
    EXPECT_TRUE(writer.endFile(&archive.bytes));
    EXPECT_TRUE(writer.beginFile(&archive.bytes, "b.bin", archive.fileB.size()));
    EXPECT_TRUE(writer.appendFileData(&archive.bytes, archive.fileB));
    EXPECT_TRUE(writer.endFile(&archive.bytes));
    EXPECT_TRUE(writer.writeError(&archive.bytes, "c.locked", "permission denied"));
    EXPECT_TRUE(writer.writeEnd(&archive.bytes));
    return archive;
}

// Feeds the whole byte string, possibly one byte at a time, and returns whether
// the parser accepted it.
bool parseAll(FrameParser *parser, const std::string &bytes, size_t step = 0)
{
    if (step == 0)
        return parser->feed(bytes.data(), bytes.size());
    for (size_t offset = 0; offset < bytes.size(); offset += step) {
        const size_t size = std::min(step, bytes.size() - offset);
        if (!parser->feed(bytes.data() + offset, size))
            return false;
    }
    return true;
}

} // namespace

TEST(HttpWebStreamFrameTest, PreambleRoundTrip)
{
    for (Codec codec : {Codec::None, Codec::Deflate, Codec::Zstd}) {
        if (!codecSupported(codec))
            continue;
        StreamPreamble preamble;
        preamble.codec = codec;
        std::string bytes;
        encodePreamble(preamble, &bytes);
        EXPECT_EQ(bytes.size(), kStreamPreambleSize);
        EXPECT_TRUE(isStreamPreamble(bytes.data(), bytes.size()));

        StreamPreamble decoded;
        std::string error;
        EXPECT_TRUE(decodePreamble(bytes.data(), bytes.size(), &decoded, &error)) << error;
        EXPECT_EQ(decoded.version, kStreamVersion);
        EXPECT_EQ(decoded.codec, codec);
    }
}

TEST(HttpWebStreamFrameTest, PreambleRejectsBadInput)
{
    StreamPreamble decoded;
    std::string error;

    EXPECT_FALSE(decodePreamble("DTP", 3, &decoded, &error));
    EXPECT_FALSE(error.empty());

    std::string bytes;
    StreamPreamble preamble;
    encodePreamble(preamble, &bytes);

    std::string badMagic = bytes;
    badMagic[0] = 'X';
    EXPECT_FALSE(decodePreamble(badMagic.data(), badMagic.size(), &decoded, &error));

    std::string badVersion = bytes;
    badVersion[4] = static_cast<char>(kStreamVersion + 1);
    EXPECT_FALSE(decodePreamble(badVersion.data(), badVersion.size(), &decoded, &error));

    std::string badCodec = bytes;
    badCodec[5] = 99;
    EXPECT_FALSE(decodePreamble(badCodec.data(), badCodec.size(), &decoded, &error));

    std::string badReserved = bytes;
    badReserved[7] = 1;
    EXPECT_FALSE(decodePreamble(badReserved.data(), badReserved.size(), &decoded, &error));
}

TEST(HttpWebStreamFrameTest, WriterRoundTripAllFrameTypes)
{
    const SampleArchive archive = buildSampleArchive();
    ASSERT_FALSE(archive.bytes.empty());

    FrameParser parser;
    Collected collected;
    attach(&parser, &collected);
    ASSERT_TRUE(parseAll(&parser, archive.bytes));
    EXPECT_TRUE(parser.finished());
    EXPECT_TRUE(collected.sawEnd);

    ASSERT_EQ(collected.headers.size(), size_t(5));
    EXPECT_EQ(collected.headers[0].type, FrameType::RootDir);
    EXPECT_EQ(collected.headers[0].path, "photos");
    EXPECT_EQ(collected.headers[1].type, FrameType::Dir);
    EXPECT_EQ(collected.headers[1].path, "2024");
    EXPECT_EQ(collected.headers[2].type, FrameType::File);
    EXPECT_EQ(collected.headers[2].path, "2024/a.txt");
    EXPECT_EQ(collected.headers[2].size, archive.fileA.size());
    EXPECT_EQ(collected.headers[3].type, FrameType::File);
    EXPECT_EQ(collected.headers[3].path, "2024/empty.txt");
    EXPECT_EQ(collected.headers[3].size, 0u);
    EXPECT_EQ(collected.headers[4].type, FrameType::File);
    EXPECT_EQ(collected.headers[4].path, "b.bin");

    ASSERT_EQ(collected.files.size(), size_t(3));
    EXPECT_EQ(collected.files[0], archive.fileA);
    EXPECT_EQ(collected.files[1], std::string());
    EXPECT_EQ(collected.files[2], archive.fileB);

    ASSERT_EQ(collected.errors.size(), size_t(1));
    EXPECT_EQ(collected.errors[0], "c.locked|permission denied");

    EXPECT_EQ(collected.end.entryCount, 5u);
    EXPECT_EQ(collected.end.totalBytes, archive.fileA.size() + archive.fileB.size());
    EXPECT_EQ(collected.end.streamCrc, parser.streamCrc());
}

TEST(HttpWebStreamFrameTest, SymlinkRoundTrip)
{
    std::string bytes;
    FrameWriter writer;
    ASSERT_TRUE(writer.writeRootDir(&bytes, "src"));
    ASSERT_TRUE(writer.writeDir(&bytes, "sub"));
    ASSERT_TRUE(writer.writeSymlink(&bytes, "sub/file-link", "../real.txt", false));
    ASSERT_TRUE(writer.writeSymlink(&bytes, "dir-link", ".", true));
    ASSERT_TRUE(writer.writeEnd(&bytes));

    FrameParser parser;
    Collected collected;
    attach(&parser, &collected);
    ASSERT_TRUE(parseAll(&parser, bytes, 1));
    EXPECT_TRUE(parser.finished());

    ASSERT_EQ(collected.headers.size(), size_t(4));
    EXPECT_EQ(collected.headers[2].type, FrameType::Symlink);
    EXPECT_EQ(collected.headers[2].path, "sub/file-link");
    EXPECT_FALSE((collected.headers[2].flags & FrameFlagLinkDirectory) != 0);
    EXPECT_EQ(collected.headers[2].size, std::strlen("../real.txt"));
    EXPECT_EQ(collected.headers[3].type, FrameType::Symlink);
    EXPECT_TRUE((collected.headers[3].flags & FrameFlagLinkDirectory) != 0);

    ASSERT_EQ(collected.symlinks.size(), size_t(2));
    EXPECT_EQ(collected.symlinks[0].second, "../real.txt");
    EXPECT_EQ(collected.symlinks[1].second, ".");
    EXPECT_EQ(collected.end.entryCount, 4u);
    EXPECT_EQ(collected.end.totalBytes, 0u);
}

TEST(HttpWebStreamFrameTest, WriterRejectsUnsafeSymlinkTargets)
{
    std::string bytes;
    FrameWriter writer;
    EXPECT_FALSE(writer.writeSymlink(&bytes, "a/link", "../../escape", false));
    EXPECT_FALSE(writer.writeSymlink(&bytes, "link", "/absolute", false));
    EXPECT_FALSE(writer.writeSymlink(&bytes, "link", "", false));
}

TEST(HttpWebStreamFrameTest, ParserRejectsEscapingSymlinkTarget)
{
    const std::string frame =
            craftFrameBytes(FrameType::Symlink, "a/link", 12, 0, "../../escape");
    FrameParser parser;
    Collected collected;
    attach(&parser, &collected);
    EXPECT_FALSE(parser.feed(frame.data(), frame.size()));
    EXPECT_TRUE(parser.failed());
    EXPECT_NE(parser.lastError().find("escapes"), std::string::npos) << parser.lastError();
}

TEST(HttpWebStreamFrameTest, RootFileCarriesData)
{
    const std::string payload = "single file selection";
    std::string bytes;
    FrameWriter writer;
    ASSERT_TRUE(writer.beginRootFile(&bytes, "movie.mkv", payload.size()));
    ASSERT_TRUE(writer.appendFileData(&bytes, payload));
    ASSERT_TRUE(writer.endFile(&bytes));
    ASSERT_TRUE(writer.writeEnd(&bytes));

    FrameParser parser;
    Collected collected;
    attach(&parser, &collected);
    ASSERT_TRUE(parseAll(&parser, bytes));
    ASSERT_EQ(collected.headers.size(), size_t(1));
    EXPECT_EQ(collected.headers[0].type, FrameType::RootFile);
    EXPECT_EQ(collected.headers[0].path, "movie.mkv");
    ASSERT_EQ(collected.files.size(), size_t(1));
    EXPECT_EQ(collected.files[0], payload);
}

TEST(HttpWebStreamFrameTest, ParserAcceptsEveryPrefixThenCompletion)
{
    const SampleArchive archive = buildSampleArchive();
    // Every proper prefix must leave the parser waiting, never failing, and the
    // remainder must complete it.
    for (size_t cut = 0; cut < archive.bytes.size(); ++cut) {
        FrameParser parser;
        Collected collected;
        attach(&parser, &collected);
        ASSERT_TRUE(parser.feed(archive.bytes.data(), cut));
        EXPECT_FALSE(parser.finished()) << "cut=" << cut;
        EXPECT_FALSE(parser.failed()) << "cut=" << cut << " error=" << parser.lastError();

        ASSERT_TRUE(parser.feed(archive.bytes.data() + cut, archive.bytes.size() - cut))
                << "cut=" << cut << " error=" << parser.lastError();
        EXPECT_TRUE(parser.finished()) << "cut=" << cut;
        EXPECT_EQ(collected.files.size(), size_t(3)) << "cut=" << cut;
        EXPECT_EQ(collected.files[0], archive.fileA) << "cut=" << cut;
        EXPECT_EQ(collected.files[2], archive.fileB) << "cut=" << cut;
    }
}

TEST(HttpWebStreamFrameTest, ParserAcceptsByteByByteFeed)
{
    const SampleArchive archive = buildSampleArchive();
    FrameParser parser;
    Collected collected;
    attach(&parser, &collected);
    ASSERT_TRUE(parseAll(&parser, archive.bytes, 1));
    EXPECT_TRUE(parser.finished());
    ASSERT_EQ(collected.files.size(), size_t(3));
    EXPECT_EQ(collected.files[0], archive.fileA);
}

TEST(HttpWebStreamFrameTest, ParserRejectsFileCrcMismatch)
{
    SampleArchive archive;
    FrameWriter writer;
    const std::string payload = "0123456789abcdef";
    ASSERT_TRUE(writer.beginFile(&archive.bytes, "a.bin", payload.size()));
    ASSERT_TRUE(writer.appendFileData(&archive.bytes, payload));
    ASSERT_TRUE(writer.endFile(&archive.bytes));
    ASSERT_TRUE(writer.writeEnd(&archive.bytes));

    // The first byte of file data sits right after the fixed header and path.
    const size_t dataOffset = kFrameHeaderFixedSize + std::string("a.bin").size();
    archive.bytes[dataOffset] ^= 0x01;

    FrameParser parser;
    Collected collected;
    attach(&parser, &collected);
    EXPECT_FALSE(parseAll(&parser, archive.bytes));
    EXPECT_TRUE(parser.failed());
    EXPECT_TRUE(parser.lastError().find("CRC mismatch") != std::string::npos)
            << parser.lastError();
}

TEST(HttpWebStreamFrameTest, ParserRejectsStreamCrcMismatch)
{
    SampleArchive archive = buildSampleArchive();
    // END is the final frame: header (36) + entryCount (8) + totalBytes (8) +
    // streamCrc (4); streamCrc is the last four bytes of the stream.
    archive.bytes[archive.bytes.size() - 1] ^= 0xff;

    FrameParser parser;
    Collected collected;
    attach(&parser, &collected);
    EXPECT_FALSE(parseAll(&parser, archive.bytes));
    EXPECT_TRUE(parser.lastError().find("stream CRC mismatch") != std::string::npos)
            << parser.lastError();
}

TEST(HttpWebStreamFrameTest, ParserRejectsEndTotalsMismatch)
{
    SampleArchive archive = buildSampleArchive();
    archive.bytes[archive.bytes.size() - 20] ^= 0xff;  // entryCount low byte

    FrameParser parser;
    Collected collected;
    attach(&parser, &collected);
    EXPECT_FALSE(parseAll(&parser, archive.bytes));
    EXPECT_TRUE(parser.lastError().find("entry count mismatch") != std::string::npos)
            << parser.lastError();
}

TEST(HttpWebStreamFrameTest, ParserRejectsUnknownFrameType)
{
    FrameParser parser;
    Collected collected;
    attach(&parser, &collected);
    const std::string bytes = craftFrameBytes(static_cast<FrameType>(9), "x", 0, 0, std::string());
    EXPECT_FALSE(parser.feed(bytes.data(), bytes.size()));
    EXPECT_TRUE(parser.lastError().find("unknown frame type") != std::string::npos)
            << parser.lastError();
}

TEST(HttpWebStreamFrameTest, ParserRejectsReservedHeaderBits)
{
    FrameParser parser;
    const std::string bytes = craftFrameBytes(FrameType::Dir, "x", 0, 0, std::string(), 1);
    EXPECT_FALSE(parser.feed(bytes.data(), bytes.size()));
    EXPECT_TRUE(parser.lastError().find("reserved") != std::string::npos) << parser.lastError();
}

TEST(HttpWebStreamFrameTest, ParserRejectsOversizedPath)
{
    FrameParser::Options options;
    options.maxPathLen = 16;
    FrameParser parser(options);
    const std::string bytes = craftFrameBytes(FrameType::Dir, std::string(17, 'a'), 0, 0,
                                              std::string());
    EXPECT_FALSE(parser.feed(bytes.data(), bytes.size()));
    EXPECT_TRUE(parser.lastError().find("path exceeds") != std::string::npos) << parser.lastError();
}

TEST(HttpWebStreamFrameTest, ParserRejectsUnsafePaths)
{
    const std::vector<std::string> unsafe = {
            "../escape",
            "/absolute",
            "a/../b",
            "a//b",
            "a\\b",
            ".",
            "..",
            std::string("nul\0byte", 8),
    };
    for (const std::string &path : unsafe) {
        FrameParser parser;
        const std::string bytes = craftFrameBytes(FrameType::Dir, path, 0, 0, std::string());
        EXPECT_FALSE(parser.feed(bytes.data(), bytes.size())) << "path=" << path;
    }
}

TEST(HttpWebStreamFrameTest, WriterRejectsUnsafePaths)
{
    const std::vector<std::string> unsafe = {
            "../escape",
            "/absolute",
            "a/../b",
            "a//b",
            "a\\b",
            ".",
            "..",
            "",
    };
    for (const std::string &path : unsafe) {
        FrameWriter writer;
        std::string bytes;
        EXPECT_FALSE(writer.writeDir(&bytes, path)) << "path=" << path;
        EXPECT_FALSE(writer.lastError().empty()) << "path=" << path;
    }
}

TEST(HttpWebStreamFrameTest, RootFramePathMustBeASingleComponent)
{
    FrameWriter writer;
    std::string bytes;
    EXPECT_FALSE(writer.writeRootDir(&bytes, "a/b"));
    EXPECT_TRUE(writer.lastError().find("single component") != std::string::npos)
            << writer.lastError();
}

TEST(HttpWebStreamFrameTest, UnicodePathRoundTrip)
{
    const std::string path = u8"文档/фото/été.txt";
    const std::string payload = "unicode payload";
    std::string bytes;
    FrameWriter writer;
    ASSERT_TRUE(writer.beginFile(&bytes, path, payload.size()));
    ASSERT_TRUE(writer.appendFileData(&bytes, payload));
    ASSERT_TRUE(writer.endFile(&bytes));
    ASSERT_TRUE(writer.writeEnd(&bytes));

    FrameParser parser;
    Collected collected;
    attach(&parser, &collected);
    ASSERT_TRUE(parseAll(&parser, bytes));
    ASSERT_EQ(collected.headers.size(), size_t(1));
    EXPECT_EQ(collected.headers[0].path, path);
    ASSERT_EQ(collected.files.size(), size_t(1));
    EXPECT_EQ(collected.files[0], payload);
}

TEST(HttpWebStreamFrameTest, PathLengthLimit)
{
    FrameWriter::Options options;
    options.maxPathLen = 32;

    std::string okPath(32, 'a');
    std::string tooLong(33, 'a');

    FrameWriter writer(options);
    std::string bytes;
    EXPECT_TRUE(writer.writeDir(&bytes, okPath));
    EXPECT_FALSE(writer.writeDir(&bytes, tooLong));

    FrameParser::Options parserOptions;
    parserOptions.maxPathLen = 32;
    FrameParser parser(parserOptions);
    const std::string crafted = craftFrameBytes(FrameType::Dir, tooLong, 0, 0, std::string());
    EXPECT_FALSE(parser.feed(crafted.data(), crafted.size()));
}

TEST(HttpWebStreamFrameTest, ZeroByteFileRoundTrip)
{
    std::string bytes;
    FrameWriter writer;
    ASSERT_TRUE(writer.beginFile(&bytes, "empty.bin", 0));
    ASSERT_TRUE(writer.endFile(&bytes));
    ASSERT_TRUE(writer.writeEnd(&bytes));

    FrameParser parser;
    Collected collected;
    attach(&parser, &collected);
    ASSERT_TRUE(parseAll(&parser, bytes));
    ASSERT_EQ(collected.headers.size(), size_t(1));
    EXPECT_EQ(collected.headers[0].size, 0u);
    ASSERT_EQ(collected.files.size(), size_t(1));
    EXPECT_TRUE(collected.files[0].empty());
    EXPECT_EQ(collected.end.totalBytes, 0u);
}

TEST(HttpWebStreamFrameTest, ErrorFrameRoundTrip)
{
    std::string bytes;
    FrameWriter writer;
    ASSERT_TRUE(writer.writeError(&bytes, "dir/secret", "permission denied"));
    ASSERT_TRUE(writer.writeEnd(&bytes));

    FrameParser parser;
    Collected collected;
    attach(&parser, &collected);
    ASSERT_TRUE(parseAll(&parser, bytes));
    ASSERT_EQ(collected.errors.size(), size_t(1));
    EXPECT_EQ(collected.errors[0], "dir/secret|permission denied");
    EXPECT_TRUE(collected.headers.empty());
}

TEST(HttpWebStreamFrameTest, MissingEndNeverFinishes)
{
    SampleArchive archive = buildSampleArchive();
    const std::string withoutEnd = archive.bytes.substr(0, archive.bytes.size() - 56);

    FrameParser parser;
    Collected collected;
    attach(&parser, &collected);
    EXPECT_TRUE(parser.feed(withoutEnd.data(), withoutEnd.size()));
    EXPECT_FALSE(parser.finished());
    EXPECT_FALSE(parser.failed());
}

TEST(HttpWebStreamFrameTest, TrailingDataAfterEndRejected)
{
    SampleArchive archive = buildSampleArchive();
    std::string bytes = archive.bytes;
    bytes.append("garbage");

    FrameParser parser;
    Collected collected;
    attach(&parser, &collected);
    EXPECT_FALSE(parseAll(&parser, bytes));
    EXPECT_TRUE(parser.lastError().find("trailing data") != std::string::npos)
            << parser.lastError();
}

TEST(HttpWebStreamFrameTest, WriterMisuseIsReported)
{
    FrameWriter writer;
    std::string bytes;

    EXPECT_FALSE(writer.endFile(&bytes));
    EXPECT_FALSE(writer.appendFileData(&bytes, "x", 1));

    ASSERT_TRUE(writer.beginFile(&bytes, "a.bin", 2));
    EXPECT_FALSE(writer.writeDir(&bytes, "dir"));
    EXPECT_FALSE(writer.appendFileData(&bytes, "abc", 3));  // more than declared
    EXPECT_TRUE(writer.appendFileData(&bytes, "ab", 2));
    EXPECT_TRUE(writer.endFile(&bytes));

    EXPECT_TRUE(writer.writeEnd(&bytes));
    EXPECT_FALSE(writer.writeEnd(&bytes));
    EXPECT_FALSE(writer.writeDir(&bytes, "after-end"));
}

TEST(HttpWebStreamFrameTest, ParserCallbacksCanAbort)
{
    SampleArchive archive = buildSampleArchive();
    FrameParser parser;
    parser.onHeader([](const FrameHeader &) { return false; });
    EXPECT_FALSE(parseAll(&parser, archive.bytes));
    EXPECT_TRUE(parser.lastError().find("consumer rejected") != std::string::npos)
            << parser.lastError();
}

TEST(HttpWebStreamFrameTest, ParserResetAllowsReuse)
{
    const SampleArchive archive = buildSampleArchive();
    FrameParser parser;
    Collected collected;
    attach(&parser, &collected);
    ASSERT_TRUE(parseAll(&parser, archive.bytes));
    EXPECT_TRUE(parser.finished());

    parser.reset();
    collected = Collected {};
    ASSERT_TRUE(parseAll(&parser, archive.bytes));
    EXPECT_TRUE(parser.finished());
    ASSERT_EQ(collected.files.size(), size_t(3));
}

TEST(HttpWebStreamFrameTest, HundredThousandFilesThroughCodec)
{
    constexpr uint64_t kFileCount = 100000;
    const std::string payload = "payload!";

    std::string raw;
    raw.reserve(8 * 1024 * 1024);
    {
        FrameWriter writer;
        ASSERT_TRUE(writer.writeRootDir(&raw, "big"));
        for (uint64_t index = 0; index < kFileCount; ++index) {
            const std::string path = "d/" + std::to_string(index);
            ASSERT_TRUE(writer.beginFile(&raw, path, payload.size()));
            ASSERT_TRUE(writer.appendFileData(&raw, payload));
            ASSERT_TRUE(writer.endFile(&raw));
        }
        ASSERT_TRUE(writer.writeEnd(&raw));
        EXPECT_EQ(writer.entryCount(), kFileCount + 1);
        EXPECT_EQ(writer.totalBytes(), kFileCount * payload.size());
    }

    for (Codec codec : {Codec::Deflate, Codec::Zstd}) {
        if (!codecSupported(codec))
            continue;

        StreamEncoder encoder;
        ASSERT_TRUE(encoder.init(codec));
        std::string compressed;
        ASSERT_TRUE(encoder.encode(raw.data(), raw.size(), &compressed));
        ASSERT_TRUE(encoder.finish(&compressed));
        EXPECT_LT(compressed.size(), raw.size());

        StreamDecoder decoder;
        ASSERT_TRUE(decoder.init(codec));
        std::string decoded;
        const size_t step = 8192;
        for (size_t offset = 0; offset < compressed.size(); offset += step) {
            const size_t size = std::min(step, compressed.size() - offset);
            ASSERT_TRUE(decoder.decode(compressed.data() + offset, size, &decoded))
                    << decoder.lastError();
        }
        EXPECT_EQ(decoded.size(), raw.size());

        FrameParser parser;
        uint64_t headers = 0;
        uint64_t lastFileBytes = 0;
        uint64_t totalFileBytes = 0;
        parser.onHeader([&](const FrameHeader &header) {
            headers++;
            lastFileBytes = header.size;
            return true;
        });
        parser.onData([&](const char *, size_t size) {
            totalFileBytes += size;
            return true;
        });
        ASSERT_TRUE(parseAll(&parser, decoded, 8192)) << parser.lastError();
        EXPECT_TRUE(parser.finished());
        EXPECT_EQ(headers, kFileCount + 1);
        EXPECT_EQ(lastFileBytes, payload.size());
        EXPECT_EQ(totalFileBytes, kFileCount * payload.size());
    }
}
