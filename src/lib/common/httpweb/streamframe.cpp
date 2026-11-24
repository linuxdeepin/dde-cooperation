// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "streamframe.h"

#include <algorithm>
#include <cstring>

#include <zlib.h>

namespace dtp {
namespace {

constexpr char kMagic[4] = {'D', 'T', 'P', '2'};
constexpr size_t kMaxZlibFeed = 1u << 30;

void putU16(std::string *out, uint16_t value)
{
    out->push_back(static_cast<char>(value & 0xff));
    out->push_back(static_cast<char>((value >> 8) & 0xff));
}

void putU32(std::string *out, uint32_t value)
{
    for (int shift = 0; shift < 32; shift += 8)
        out->push_back(static_cast<char>((value >> shift) & 0xff));
}

void putU64(std::string *out, uint64_t value)
{
    for (int shift = 0; shift < 64; shift += 8)
        out->push_back(static_cast<char>((value >> shift) & 0xff));
}

uint16_t readU16(const char *data)
{
    return static_cast<uint16_t>(static_cast<uint8_t>(data[0])
                                 | (static_cast<uint16_t>(static_cast<uint8_t>(data[1])) << 8));
}

uint32_t readU32(const char *data)
{
    uint32_t value = 0;
    for (int shift = 0; shift < 32; shift += 8)
        value |= static_cast<uint32_t>(static_cast<uint8_t>(data[shift / 8])) << shift;
    return value;
}

uint64_t readU64(const char *data)
{
    uint64_t value = 0;
    for (int shift = 0; shift < 64; shift += 8)
        value |= static_cast<uint64_t>(static_cast<uint8_t>(data[shift / 8])) << shift;
    return value;
}

uint32_t crcUpdate(uint32_t crc, const char *data, size_t size)
{
    uLong value = crc;
    size_t offset = 0;
    while (offset < size) {
        uInt take = static_cast<uInt>(std::min(size - offset, kMaxZlibFeed));
        value = crc32(value, reinterpret_cast<const Bytef *>(data + offset), take);
        offset += take;
    }
    return static_cast<uint32_t>(value);
}

bool isRootFrame(FrameType type)
{
    return type == FrameType::RootDir || type == FrameType::RootFile;
}

} // namespace

const char *frameTypeName(FrameType type)
{
    switch (type) {
    case FrameType::RootDir:
        return "root_dir";
    case FrameType::RootFile:
        return "root_file";
    case FrameType::Dir:
        return "dir";
    case FrameType::File:
        return "file";
    case FrameType::Error:
        return "error";
    case FrameType::End:
        return "end";
    }
    return "";
}

void encodePreamble(const StreamPreamble &preamble, std::string *out)
{
    if (!out)
        return;
    out->append(kMagic, sizeof(kMagic));
    out->push_back(static_cast<char>(preamble.version));
    out->push_back(static_cast<char>(static_cast<uint8_t>(preamble.codec)));
    out->push_back(static_cast<char>(preamble.flags));
    out->push_back('\0');
}

bool isStreamPreamble(const char *data, size_t size)
{
    return data && size >= sizeof(kMagic) && std::memcmp(data, kMagic, sizeof(kMagic)) == 0;
}

bool decodePreamble(const char *data, size_t size, StreamPreamble *out, std::string *error)
{
    if (!data || size < kStreamPreambleSize) {
        if (error)
            *error = "stream preamble truncated";
        return false;
    }
    if (!isStreamPreamble(data, size)) {
        if (error)
            *error = "stream preamble has bad magic";
        return false;
    }
    const uint8_t version = static_cast<uint8_t>(data[4]);
    if (version != kStreamVersion) {
        if (error)
            *error = "unsupported DTP version " + std::to_string(version);
        return false;
    }
    const uint8_t codecValue = static_cast<uint8_t>(data[5]);
    if (codecValue > static_cast<uint8_t>(Codec::Zstd)) {
        if (error)
            *error = "unknown codec id " + std::to_string(codecValue);
        return false;
    }
    const Codec codec = static_cast<Codec>(codecValue);
    if (!codecSupported(codec)) {
        if (error)
            *error = std::string("codec not available in this build: ") + codecName(codec);
        return false;
    }
    if (static_cast<uint8_t>(data[6]) != 0 || static_cast<uint8_t>(data[7]) != 0) {
        if (error)
            *error = "stream preamble reserved bits set";
        return false;
    }
    if (out) {
        out->version = version;
        out->codec = codec;
        out->flags = 0;
    }
    return true;
}

bool isSafeRelativePath(const std::string &path, std::string *error)
{
    if (path.empty()) {
        if (error)
            *error = "empty path";
        return false;
    }
    if (path.find('\0') != std::string::npos) {
        if (error)
            *error = "NUL byte in path";
        return false;
    }
    if (path.front() == '/') {
        if (error)
            *error = "absolute path";
        return false;
    }
    if (path.find('\\') != std::string::npos) {
        if (error)
            *error = "backslash in path";
        return false;
    }
    size_t start = 0;
    for (;;) {
        const size_t slash = path.find('/', start);
        const std::string segment =
                path.substr(start, slash == std::string::npos ? std::string::npos : slash - start);
        if (segment.empty()) {
            if (error)
                *error = "empty path segment";
            return false;
        }
        if (segment == "." || segment == "..") {
            if (error)
                *error = "relative path segment in path";
            return false;
        }
        for (char raw : segment) {
            const unsigned char ch = static_cast<unsigned char>(raw);
            if (ch < 0x20) {
                if (error)
                    *error = "control character in path";
                return false;
            }
#ifdef _WIN32
            if (ch == ':' || ch == '<' || ch == '>' || ch == '"' || ch == '|' || ch == '?' || ch == '*') {
                if (error)
                    *error = "character not allowed on Windows in path";
                return false;
            }
#endif
        }
        if (slash == std::string::npos)
            break;
        start = slash + 1;
    }
    return true;
}

bool isSafeLeafName(const std::string &name, std::string *error)
{
    if (!isSafeRelativePath(name, error))
        return false;
    if (name.find('/') != std::string::npos) {
        if (error)
            *error = "root path must be a single component";
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// FrameWriter
// ---------------------------------------------------------------------------

struct FrameWriter::Impl {
    Options options;
    std::string error;
    uint32_t streamCrc {0};
    uint32_t fileCrc {0};
    uint64_t entryCount {0};
    uint64_t totalBytes {0};
    uint64_t declaredSize {0};
    uint64_t writtenSize {0};
    bool inFile {false};
    bool ended {false};

    bool fail(const std::string &message)
    {
        error = message;
        return false;
    }

    bool checkPath(const std::string &path, bool leaf)
    {
        if (path.size() > options.maxPathLen)
            return fail("path exceeds " + std::to_string(options.maxPathLen) + " bytes");
        if (path.find('\0') != std::string::npos)
            return fail("NUL byte in path");
        if (!options.validatePaths)
            return true;
        std::string reason;
        const bool ok = leaf ? isSafeLeafName(path, &reason) : isSafeRelativePath(path, &reason);
        if (!ok)
            return fail(reason + ": " + path);
        return true;
    }

    bool appendHeader(std::string *out, FrameType type, const std::string &path, uint64_t size,
                      uint64_t offset, uint32_t mode, uint64_t mtime, bool countCrc)
    {
        std::string header;
        header.reserve(kFrameHeaderFixedSize + path.size());
        header.push_back(static_cast<char>(static_cast<uint8_t>(type)));
        uint8_t flags = 0;
        if (mode != 0)
            flags |= FrameFlagHasMode;
        if (mtime != 0)
            flags |= FrameFlagHasMtime;
        header.push_back(static_cast<char>(flags));
        putU16(&header, 0);
        putU32(&header, static_cast<uint32_t>(path.size()));
        header.append(path);
        putU64(&header, size);
        putU64(&header, offset);
        putU32(&header, mode);
        putU64(&header, mtime);

        out->append(header);
        if (countCrc)
            streamCrc = crcUpdate(streamCrc, header.data(), header.size());
        return true;
    }

    bool beginFrame(FrameType type, std::string *out, const std::string &path, uint64_t size,
                    uint64_t offset, uint32_t mode, uint64_t mtime)
    {
        if (!out)
            return fail("null output buffer");
        if (ended)
            return fail("frame after END frame");
        if (inFile)
            return fail("frame started while a file frame is still open");
        if (!checkPath(path, isRootFrame(type)))
            return false;
        if (size > options.maxFrameSize)
            return fail("frame size exceeds limit");
        if (offset > options.maxFrameSize || size > options.maxFrameSize - offset)
            return fail("frame range exceeds limit");
        return appendHeader(out, type, path, size, offset, mode, mtime, true);
    }
};

FrameWriter::FrameWriter()
    : d(new Impl)
{
}

FrameWriter::FrameWriter(Options opts)
    : d(new Impl)
{
    d->options = opts;
}

FrameWriter::~FrameWriter() = default;

bool FrameWriter::writeRootDir(std::string *out, const std::string &path, uint32_t mode,
                               uint64_t mtime)
{
    if (!d->beginFrame(FrameType::RootDir, out, path, 0, 0, mode, mtime))
        return false;
    d->entryCount++;
    return true;
}

bool FrameWriter::writeDir(std::string *out, const std::string &path, uint32_t mode,
                           uint64_t mtime)
{
    if (!d->beginFrame(FrameType::Dir, out, path, 0, 0, mode, mtime))
        return false;
    d->entryCount++;
    return true;
}

bool FrameWriter::beginRootFile(std::string *out, const std::string &path, uint64_t size,
                                uint64_t offset, uint32_t mode, uint64_t mtime)
{
    if (!d->beginFrame(FrameType::RootFile, out, path, size, offset, mode, mtime))
        return false;
    d->fileCrc = 0;
    d->declaredSize = size;
    d->writtenSize = 0;
    d->inFile = true;
    d->entryCount++;
    d->totalBytes += size;
    return true;
}

bool FrameWriter::beginFile(std::string *out, const std::string &path, uint64_t size,
                            uint64_t offset, uint32_t mode, uint64_t mtime)
{
    if (!d->beginFrame(FrameType::File, out, path, size, offset, mode, mtime))
        return false;
    d->fileCrc = 0;
    d->declaredSize = size;
    d->writtenSize = 0;
    d->inFile = true;
    d->entryCount++;
    d->totalBytes += size;
    return true;
}

bool FrameWriter::appendFileData(std::string *out, const char *data, size_t size)
{
    if (!out)
        return d->fail("null output buffer");
    if (!d->inFile)
        return d->fail("file data without an open file frame");
    if (size == 0)
        return true;
    if (!data)
        return d->fail("null file data");
    if (size > d->declaredSize - d->writtenSize)
        return d->fail("file data exceeds declared frame size");
    d->fileCrc = crcUpdate(d->fileCrc, data, size);
    d->streamCrc = crcUpdate(d->streamCrc, data, size);
    out->append(data, size);
    d->writtenSize += size;
    return true;
}

bool FrameWriter::appendFileData(std::string *out, const std::string &data)
{
    return appendFileData(out, data.data(), data.size());
}

bool FrameWriter::endFile(std::string *out)
{
    if (!out)
        return d->fail("null output buffer");
    if (!d->inFile)
        return d->fail("endFile without an open file frame");
    if (d->writtenSize != d->declaredSize)
        return d->fail("file data shorter than declared frame size");

    std::string trailer;
    putU32(&trailer, d->fileCrc);
    out->append(trailer);
    d->streamCrc = crcUpdate(d->streamCrc, trailer.data(), trailer.size());
    d->inFile = false;
    return true;
}

bool FrameWriter::writeError(std::string *out, const std::string &path, const std::string &message)
{
    if (!out)
        return d->fail("null output buffer");
    if (d->ended)
        return d->fail("frame after END frame");
    if (d->inFile)
        return d->fail("error frame while a file frame is still open");
    if (message.size() > d->options.maxErrorMessage)
        return d->fail("error message exceeds limit");
    if (path.size() > d->options.maxPathLen)
        return d->fail("error path exceeds limit");
    if (path.find('\0') != std::string::npos)
        return d->fail("NUL byte in path");
    if (!d->appendHeader(out, FrameType::Error, path, message.size(), 0, 0, 0, true))
        return false;
    if (!message.empty()) {
        out->append(message);
        d->streamCrc = crcUpdate(d->streamCrc, message.data(), message.size());
    }
    return true;
}

bool FrameWriter::writeEnd(std::string *out)
{
    return writeEnd(out, d->entryCount, d->totalBytes);
}

bool FrameWriter::writeEnd(std::string *out, uint64_t entryCount, uint64_t totalBytes)
{
    if (!out)
        return d->fail("null output buffer");
    if (d->ended)
        return d->fail("duplicate END frame");
    if (d->inFile)
        return d->fail("END frame while a file frame is still open");
    if (!d->appendHeader(out, FrameType::End, std::string(), 0, 0, 0, 0, false))
        return false;
    std::string payload;
    payload.reserve(kFrameEndPayloadSize);
    putU64(&payload, entryCount);
    putU64(&payload, totalBytes);
    putU32(&payload, d->streamCrc);
    out->append(payload);
    d->ended = true;
    return true;
}

uint64_t FrameWriter::entryCount() const
{
    return d->entryCount;
}

uint64_t FrameWriter::totalBytes() const
{
    return d->totalBytes;
}

uint32_t FrameWriter::streamCrc() const
{
    return d->streamCrc;
}

bool FrameWriter::inFile() const
{
    return d->inFile;
}

bool FrameWriter::ended() const
{
    return d->ended;
}

bool FrameWriter::valid() const
{
    return d->error.empty();
}

const std::string &FrameWriter::lastError() const
{
    return d->error;
}

void FrameWriter::reset()
{
    d->error.clear();
    d->streamCrc = 0;
    d->fileCrc = 0;
    d->entryCount = 0;
    d->totalBytes = 0;
    d->declaredSize = 0;
    d->writtenSize = 0;
    d->inFile = false;
    d->ended = false;
}

// ---------------------------------------------------------------------------
// FrameParser
// ---------------------------------------------------------------------------

struct FrameParser::Impl {
    enum class State {
        Prefix,
        Path,
        Trailer,
        Data,
        DataCrc,
        ErrorMessage,
        EndBody,
        Done,
        Failed,
    };

    Options options;
    HeaderCallback headerCb;
    DataCallback dataCb;
    ErrorCallback errorCb;
    EndCallback endCb;

    State state {State::Prefix};
    std::string error;

    char prefixBuf[kFrameHeaderPrefixSize] = {};
    size_t prefixPos {0};
    char trailerBuf[kFrameHeaderTrailerSize] = {};
    size_t trailerPos {0};
    FrameHeader header;
    uint32_t pathLen {0};
    bool suppressCrc {false};

    std::string path;
    std::string message;
    size_t textPos {0};

    uint64_t dataRemaining {0};
    uint32_t fileCrc {0};
    char crcBuf[4] = {};
    size_t crcPos {0};

    char endBuf[kFrameEndPayloadSize] = {};
    size_t endPos {0};
    FrameEnd end;

    uint32_t streamCrc {0};
    uint64_t entryCount {0};
    uint64_t totalBytes {0};

    bool fail(const std::string &text)
    {
        if (error.empty())
            error = text;
        state = State::Failed;
        return false;
    }

    bool callHeader()
    {
        if (headerCb && !headerCb(header))
            return fail("consumer rejected frame: " + header.path);
        return true;
    }

    bool nextFrame()
    {
        header = FrameHeader {};
        path.clear();
        prefixPos = 0;
        trailerPos = 0;
        pathLen = 0;
        crcPos = 0;
        endPos = 0;
        suppressCrc = false;
        state = State::Prefix;
        return true;
    }

    bool finishPrefix()
    {
        const uint8_t type = static_cast<uint8_t>(prefixBuf[0]);
        if (type == 0 || type > static_cast<uint8_t>(FrameType::End))
            return fail("unknown frame type " + std::to_string(type));
        header.type = static_cast<FrameType>(type);
        header.flags = static_cast<uint8_t>(prefixBuf[1]);
        if (readU16(prefixBuf + 2) != 0)
            return fail("reserved frame header bits set");
        pathLen = readU32(prefixBuf + 4);
        if (pathLen > options.maxPathLen)
            return fail("frame path exceeds " + std::to_string(options.maxPathLen) + " bytes");
        if (header.type == FrameType::End && pathLen != 0)
            return fail("END frame must not carry a path");

        suppressCrc = (header.type == FrameType::End);
        if (!suppressCrc)
            streamCrc = crcUpdate(streamCrc, prefixBuf, kFrameHeaderPrefixSize);

        path.clear();
        textPos = 0;
        if (pathLen == 0) {
            state = State::Trailer;
        } else {
            path.reserve(pathLen);
            state = State::Path;
        }
        return true;
    }

    bool finishTrailer()
    {
        if (!suppressCrc)
            streamCrc = crcUpdate(streamCrc, trailerBuf, kFrameHeaderTrailerSize);
        header.size = readU64(trailerBuf);
        header.offset = readU64(trailerBuf + 8);
        header.mode = readU32(trailerBuf + 16);
        header.mtime = readU64(trailerBuf + 20);
        return finishFrame();
    }

    bool finishFrame()
    {
        // END carries no path; an ERROR path is diagnostic only and may name
        // something the producer itself could not represent.
        if (options.validatePaths && header.type != FrameType::End
            && header.type != FrameType::Error) {
            std::string reason;
            const bool ok = isRootFrame(header.type) ? isSafeLeafName(path, &reason)
                                                     : isSafeRelativePath(path, &reason);
            if (!ok)
                return fail(reason + " (" + frameTypeName(header.type) + ")");
        }
        header.path = path;

        switch (header.type) {
        case FrameType::RootDir:
        case FrameType::Dir:
            if (header.size != 0 || header.offset != 0)
                return fail("directory frame carries a payload");
            entryCount++;
            if (!callHeader())
                return false;
            return nextFrame();

        case FrameType::RootFile:
        case FrameType::File:
            if (header.offset > options.maxFrameSize
                || header.size > options.maxFrameSize - header.offset)
                return fail("file frame exceeds size limit");
            entryCount++;
            totalBytes += header.size;
            if (!callHeader())
                return false;
            fileCrc = 0;
            dataRemaining = header.size;
            if (dataRemaining == 0) {
                crcPos = 0;
                state = State::DataCrc;
            } else {
                state = State::Data;
            }
            return true;

        case FrameType::Error:
            if (header.size > options.maxErrorMessage)
                return fail("error message exceeds limit");
            message.clear();
            textPos = 0;
            if (header.size == 0) {
                if (errorCb && !errorCb(header.path, message))
                    return fail("consumer rejected error frame");
                return nextFrame();
            }
            message.reserve(header.size);
            state = State::ErrorMessage;
            return true;

        case FrameType::End:
            if (header.size != 0 || header.offset != 0)
                return fail("END frame carries unexpected fields");
            endPos = 0;
            state = State::EndBody;
            return true;
        }
        return fail("unreachable frame type");
    }

    bool finishEnd()
    {
        end.entryCount = readU64(endBuf);
        end.totalBytes = readU64(endBuf + 8);
        end.streamCrc = readU32(endBuf + 16);
        if (options.verifyStreamCrc && end.streamCrc != streamCrc)
            return fail("stream CRC mismatch: header " + std::to_string(end.streamCrc)
                        + ", computed " + std::to_string(streamCrc));
        if (options.verifyEndTotals) {
            if (end.entryCount != entryCount)
                return fail("END entry count mismatch: header " + std::to_string(end.entryCount)
                            + ", computed " + std::to_string(entryCount));
            if (end.totalBytes != totalBytes)
                return fail("END byte count mismatch: header " + std::to_string(end.totalBytes)
                            + ", computed " + std::to_string(totalBytes));
        }
        state = State::Done;
        if (endCb && !endCb(end))
            return fail("consumer rejected END frame");
        return true;
    }
};

FrameParser::FrameParser()
    : d(new Impl)
{
}

FrameParser::FrameParser(Options opts)
    : d(new Impl)
{
    d->options = opts;
}

FrameParser::~FrameParser() = default;

void FrameParser::onHeader(HeaderCallback callback)
{
    d->headerCb = std::move(callback);
}

void FrameParser::onData(DataCallback callback)
{
    d->dataCb = std::move(callback);
}

void FrameParser::onError(ErrorCallback callback)
{
    d->errorCb = std::move(callback);
}

void FrameParser::onEnd(EndCallback callback)
{
    d->endCb = std::move(callback);
}

bool FrameParser::feed(const char *data, size_t size)
{
    if (d->state == Impl::State::Failed)
        return false;
    if (size == 0)
        return true;
    if (!data)
        return d->fail("null input buffer");
    if (d->state == Impl::State::Done)
        return d->fail("trailing data after END frame");

    size_t pos = 0;
    while (pos < size && d->state != Impl::State::Done && d->state != Impl::State::Failed) {
        switch (d->state) {
        case Impl::State::Prefix: {
            const size_t need = kFrameHeaderPrefixSize - d->prefixPos;
            const size_t take = std::min(need, size - pos);
            std::memcpy(d->prefixBuf + d->prefixPos, data + pos, take);
            d->prefixPos += take;
            pos += take;
            if (d->prefixPos < kFrameHeaderPrefixSize)
                break;
            if (!d->finishPrefix())
                return false;
            break;
        }

        case Impl::State::Path: {
            const size_t need = d->pathLen - d->path.size();
            const size_t take = std::min(need, size - pos);
            d->path.append(data + pos, take);
            if (!d->suppressCrc)
                d->streamCrc = crcUpdate(d->streamCrc, data + pos, take);
            pos += take;
            if (d->path.size() == d->pathLen)
                d->state = Impl::State::Trailer;
            break;
        }

        case Impl::State::Trailer: {
            const size_t need = kFrameHeaderTrailerSize - d->trailerPos;
            const size_t take = std::min(need, size - pos);
            std::memcpy(d->trailerBuf + d->trailerPos, data + pos, take);
            d->trailerPos += take;
            pos += take;
            if (d->trailerPos < kFrameHeaderTrailerSize)
                break;
            if (!d->finishTrailer())
                return false;
            break;
        }

        case Impl::State::Data: {
            const size_t take = static_cast<size_t>(
                    std::min<uint64_t>(d->dataRemaining, size - pos));
            d->fileCrc = crcUpdate(d->fileCrc, data + pos, take);
            d->streamCrc = crcUpdate(d->streamCrc, data + pos, take);
            if (d->dataCb && take > 0 && !d->dataCb(data + pos, take))
                return d->fail("consumer rejected file data: " + d->header.path);
            pos += take;
            d->dataRemaining -= take;
            if (d->dataRemaining == 0) {
                d->crcPos = 0;
                d->state = Impl::State::DataCrc;
            }
            break;
        }

        case Impl::State::DataCrc: {
            const size_t need = 4 - d->crcPos;
            const size_t take = std::min(need, size - pos);
            std::memcpy(d->crcBuf + d->crcPos, data + pos, take);
            d->crcPos += take;
            d->streamCrc = crcUpdate(d->streamCrc, data + pos, take);
            pos += take;
            if (d->crcPos == 4) {
                const uint32_t expected = readU32(d->crcBuf);
                if (d->options.verifyFileCrc && expected != d->fileCrc)
                    return d->fail("file CRC mismatch for " + d->header.path + ": header "
                                   + std::to_string(expected) + ", computed "
                                   + std::to_string(d->fileCrc));
                if (!d->nextFrame())
                    return false;
            }
            break;
        }

        case Impl::State::ErrorMessage: {
            const size_t need = static_cast<size_t>(d->header.size) - d->message.size();
            const size_t take = std::min(need, size - pos);
            d->message.append(data + pos, take);
            d->streamCrc = crcUpdate(d->streamCrc, data + pos, take);
            pos += take;
            if (d->message.size() == d->header.size) {
                if (d->errorCb && !d->errorCb(d->header.path, d->message))
                    return d->fail("consumer rejected error frame");
                if (!d->nextFrame())
                    return false;
            }
            break;
        }

        case Impl::State::EndBody: {
            const size_t need = kFrameEndPayloadSize - d->endPos;
            const size_t take = std::min(need, size - pos);
            std::memcpy(d->endBuf + d->endPos, data + pos, take);
            d->endPos += take;
            pos += take;
            if (d->endPos == kFrameEndPayloadSize) {
                if (!d->finishEnd())
                    return false;
            }
            break;
        }

        case Impl::State::Done:
        case Impl::State::Failed:
            break;
        }
    }

    if (d->state == Impl::State::Failed)
        return false;
    if (d->state == Impl::State::Done && pos < size)
        return d->fail("trailing data after END frame");
    return true;
}

bool FrameParser::finished() const
{
    return d->state == Impl::State::Done;
}

bool FrameParser::failed() const
{
    return d->state == Impl::State::Failed;
}

const std::string &FrameParser::lastError() const
{
    return d->error;
}

uint32_t FrameParser::streamCrc() const
{
    return d->streamCrc;
}

uint64_t FrameParser::entryCount() const
{
    return d->entryCount;
}

uint64_t FrameParser::totalBytes() const
{
    return d->totalBytes;
}

const FrameHeader &FrameParser::currentHeader() const
{
    return d->header;
}

FrameEnd FrameParser::endInfo() const
{
    return d->end;
}

void FrameParser::reset()
{
    d->state = Impl::State::Prefix;
    d->error.clear();
    d->prefixPos = 0;
    d->trailerPos = 0;
    d->header = FrameHeader {};
    d->pathLen = 0;
    d->suppressCrc = false;
    d->path.clear();
    d->message.clear();
    d->textPos = 0;
    d->dataRemaining = 0;
    d->fileCrc = 0;
    d->crcPos = 0;
    d->endPos = 0;
    d->end = FrameEnd {};
    d->streamCrc = 0;
    d->entryCount = 0;
    d->totalBytes = 0;
    std::memset(d->prefixBuf, 0, sizeof(d->prefixBuf));
    std::memset(d->trailerBuf, 0, sizeof(d->trailerBuf));
    std::memset(d->crcBuf, 0, sizeof(d->crcBuf));
    std::memset(d->endBuf, 0, sizeof(d->endBuf));
}

} // namespace dtp
