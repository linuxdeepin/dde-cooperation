// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// DTP2 archive stream framing.
//
// Wire layout of one response body:
//
//   [preamble 8B, uncompressed]
//   [frame][frame]...[END frame]        <- compressed with the codec from the preamble
//
// Frame layout (all integers little endian):
//
//   offset  width  field
//   0       1      type
//   1       1      flags
//   2       2      reserved (0)
//   4       4      pathLen
//   8       N      path (relative, '/' separated, empty for END)
//   8+N     8      size
//   16+N    8      offset
//   24+N    4      mode
//   28+N    8      mtime
//   36+N    size   file data (FILE/RootFile) or link target (Symlink)
//   36+N+size 4    CRC32 of the file data (FILE/RootFile only)
//
// ERROR frames use `size` as the message length and append the message bytes;
// they have no CRC trailer. Symlink frames use `size` as the target length and
// append the target bytes; they have no CRC trailer. END frames carry 20 extra
// bytes: entryCount u64, totalBytes u64, streamCRC u32.
//
// The CRC trailer sits at the end of the frame because the producer can only
// know it after reading the data; this keeps the sender streaming instead of
// buffering the whole file.
//
// Threading: FrameWriter and FrameParser are not thread-safe. Each instance is
// owned by the producer thread (writer) or the consumer thread (parser). The
// parser never blocks and never performs I/O; callbacks run on the caller's
// thread.

#ifndef STREAMFRAME_H
#define STREAMFRAME_H

#include "streamcodec.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace dtp {

enum class FrameType : uint8_t {
    RootDir = 1,   // top level selection is a directory; path is the display name
    RootFile = 2,  // top level selection is a single file; carries data
    Dir = 3,       // directory relative to the root
    File = 4,      // file relative to the root; carries data
    Error = 5,     // producer side error for a path; transfer continues
    End = 6,       // last frame; validates the whole stream
    Symlink = 7,   // symlink relative to the root; carries the target text
};

const char *frameTypeName(FrameType type);

enum FrameFlag : uint8_t {
    FrameFlagHasMode = 0x01,   // header.mode is meaningful
    FrameFlagHasMtime = 0x02,  // header.mtime is meaningful
    FrameFlagResume = 0x04,    // frame produced for a resumed transfer
    FrameFlagLinkDirectory = 0x08,  // Symlink target is a directory
};

// A frame header is split around the path: type/flags/reserved/pathLen come
// first, then the path, then size/offset/mode/mtime.
constexpr size_t kFrameHeaderPrefixSize = 8;
constexpr size_t kFrameHeaderTrailerSize = 28;
// Total header size excluding the path itself.
constexpr size_t kFrameHeaderFixedSize = kFrameHeaderPrefixSize + kFrameHeaderTrailerSize;
// Payload of the END frame.
constexpr size_t kFrameEndPayloadSize = 20;
// Magic/version/codec/flags of the uncompressed preamble.
constexpr size_t kStreamPreambleSize = 8;
constexpr uint8_t kStreamVersion = 1;
constexpr uint32_t kDefaultMaxPathLen = 4096;
constexpr uint32_t kDefaultMaxErrorMessage = 64 * 1024;
constexpr uint64_t kDefaultMaxFrameSize = 1ULL << 40;  // 1 TiB per frame

struct FrameHeader {
    FrameType type {FrameType::File};
    uint8_t flags {0};
    std::string path;
    uint64_t size {0};    // file data bytes, ERROR message bytes, or link target bytes
    uint64_t offset {0};  // offset of this data inside the file
    uint32_t mode {0};
    uint64_t mtime {0};

    bool hasMode() const { return (flags & FrameFlagHasMode) != 0; }
    bool hasMtime() const { return (flags & FrameFlagHasMtime) != 0; }
};

struct FrameEnd {
    uint64_t entryCount {0};  // Root*/Dir/File frames preceding END
    uint64_t totalBytes {0};  // sum of file data bytes
    uint32_t streamCrc {0};   // CRC32 over the uncompressed frame bytes, END excluded
};

struct StreamPreamble {
    uint8_t version {kStreamVersion};
    Codec codec {Codec::Deflate};
    uint8_t flags {0};
};

void encodePreamble(const StreamPreamble &preamble, std::string *out);
bool decodePreamble(const char *data, size_t size, StreamPreamble *out, std::string *error);
bool isStreamPreamble(const char *data, size_t size);

// Relative path validation shared by writer, parser and consumer.
// Rejects: empty paths, NUL, absolute paths, '.', '..', empty segments and
// backslashes (the wire separator is always '/'; v1 refuses to guess how a
// peer would map '\').
bool isSafeRelativePath(const std::string &path, std::string *error);
// True when path is a single safe component (used for RootDir/RootFile).
bool isSafeLeafName(const std::string &name, std::string *error);

// Encodes one archive stream. Feed file data with appendFileData() between
// beginFile() and endFile(); every other frame is self contained.
class FrameWriter
{
public:
    struct Options {
        uint32_t maxPathLen {kDefaultMaxPathLen};
        uint32_t maxErrorMessage {kDefaultMaxErrorMessage};
        uint64_t maxFrameSize {kDefaultMaxFrameSize};
        bool validatePaths {true};
    };

    explicit FrameWriter(Options options);
    FrameWriter();
    ~FrameWriter();

    FrameWriter(const FrameWriter &) = delete;
    FrameWriter &operator=(const FrameWriter &) = delete;

    bool writeRootDir(std::string *out, const std::string &path,
                      uint32_t mode = 0, uint64_t mtime = 0);
    bool writeDir(std::string *out, const std::string &path,
                  uint32_t mode = 0, uint64_t mtime = 0);

    bool beginRootFile(std::string *out, const std::string &path, uint64_t size,
                       uint64_t offset = 0, uint32_t mode = 0, uint64_t mtime = 0);
    bool beginFile(std::string *out, const std::string &path, uint64_t size,
                   uint64_t offset = 0, uint32_t mode = 0, uint64_t mtime = 0);

    bool appendFileData(std::string *out, const char *data, size_t size);
    bool appendFileData(std::string *out, const std::string &data);
    bool endFile(std::string *out);

    bool writeError(std::string *out, const std::string &path, const std::string &message);
    // target is relative to the link's parent directory and must resolve inside
    // the archive root. toDirectory selects the platform-specific link kind.
    bool writeSymlink(std::string *out, const std::string &path, const std::string &target,
                      bool toDirectory, uint32_t mode = 0, uint64_t mtime = 0);
    // Writes END using the counters accumulated by the writer itself.
    bool writeEnd(std::string *out);
    bool writeEnd(std::string *out, uint64_t entryCount, uint64_t totalBytes);

    uint64_t entryCount() const;
    uint64_t totalBytes() const;
    uint32_t streamCrc() const;
    bool inFile() const;
    bool ended() const;
    bool valid() const;
    const std::string &lastError() const;

    void reset();

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

// Incremental parser. Bytes are consumed exactly as described by the frame
// headers; nothing is buffered except the path, the ERROR message and the END
// payload, so a multi gigabyte file frame never materialises in memory.
class FrameParser
{
public:
    using HeaderCallback = std::function<bool(const FrameHeader &)>;
    using DataCallback = std::function<bool(const char *, size_t)>;
    using ErrorCallback = std::function<bool(const std::string &, const std::string &)>;
    using SymlinkCallback = std::function<bool(const FrameHeader &, const std::string &target)>;
    using EndCallback = std::function<bool(const FrameEnd &)>;

    struct Options {
        uint32_t maxPathLen {kDefaultMaxPathLen};
        uint32_t maxErrorMessage {kDefaultMaxErrorMessage};
        uint64_t maxFrameSize {kDefaultMaxFrameSize};
        bool validatePaths {true};
        bool verifyFileCrc {true};
        bool verifyStreamCrc {true};
        bool verifyEndTotals {true};
    };

    explicit FrameParser(Options options);
    FrameParser();
    ~FrameParser();

    FrameParser(const FrameParser &) = delete;
    FrameParser &operator=(const FrameParser &) = delete;

    // Callbacks may be replaced at any time before the matching bytes arrive.
    // Returning false aborts the parse: feed() then returns false and
    // lastError() reports "consumer rejected frame".
    void onHeader(HeaderCallback callback);
    void onData(DataCallback callback);
    void onError(ErrorCallback callback);
    void onSymlink(SymlinkCallback callback);
    void onEnd(EndCallback callback);

    bool feed(const char *data, size_t size);

    bool finished() const;
    bool failed() const;
    const std::string &lastError() const;

    // CRC32 accumulated over the uncompressed frame bytes consumed so far
    // (END excluded). Compare with FrameEnd::streamCrc.
    uint32_t streamCrc() const;
    uint64_t entryCount() const;
    uint64_t totalBytes() const;
    const FrameHeader &currentHeader() const;
    FrameEnd endInfo() const;

    void reset();

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace dtp

#endif // STREAMFRAME_H
