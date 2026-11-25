// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// DTP2 receiver pipeline: decompress -> parse frames -> write files.
//
// The consumer is transport agnostic: the caller feeds raw bytes of the HTTP
// response body (preamble first) and calls finish() when the peer closes the
// connection. It performs disk I/O, so it must run on the download/consumer
// thread, never on the Asio IO thread.
//
// Safety:
//   - every path is validated by FrameParser (relative, no "..", no absolute);
//   - the resolved path is additionally checked to stay inside the created
//     root, so a malicious archive cannot escape through a crafted name;
//   - files are written to "<name>.part" and only renamed after their CRC and
//     size trailer were verified;
//   - a failed or canceled stream removes the in-progress .part file.

#ifndef STREAMCONSUMER_H
#define STREAMCONSUMER_H

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace dtp {

struct StreamConsumerOptions {
    // Destination directory. The root frame name is created inside it (a
    // "(1)" suffix is used when the name already exists).
    std::string targetDir;
    // Non-empty on a resumed transfer: continue inside this existing root.
    // The stream then starts after the root frame (RootDir/RootFile frames are
    // rejected) and existing final files are treated as a conflict.
    std::string resumeRoot;
};

struct StreamConsumerStats {
    uint64_t files {0};
    uint64_t directories {0};
    uint64_t symlinks {0};
    uint64_t bytes {0};
    std::string rootPath;
};

class StreamConsumer
{
public:
    enum class Status {
        Active,
        Ok,
        Error,
        Canceled,
    };

    // Return false to cancel the transfer.
    using ProgressCallback = std::function<bool(uint64_t bytes)>;
    using FileCallback = std::function<void(const std::string &path, uint64_t size)>;
    using ErrorCallback = std::function<void(const std::string &path, const std::string &message)>;
    // Called after every completed frame (directories, files, ERROR frames).
    // Used by the checkpoint writer; may be invoked up to once per file.
    using CheckpointCallback = std::function<void(uint64_t completedEntries)>;

    explicit StreamConsumer(StreamConsumerOptions options);
    ~StreamConsumer();

    StreamConsumer(const StreamConsumer &) = delete;
    StreamConsumer &operator=(const StreamConsumer &) = delete;

    void setProgressCallback(ProgressCallback callback);
    void setFileCallback(FileCallback onBegin, FileCallback onEnd);
    void setErrorCallback(ErrorCallback callback);
    void setCheckpointCallback(CheckpointCallback callback);

    // Feeds raw response bytes (preamble included). Returns false on protocol,
    // path or disk error, or when the callback asked to cancel.
    bool feed(const char *data, size_t size);

    // The peer closed the connection. Returns true only when the archive END
    // frame was received and verified.
    bool finish(std::string *error);

    // Stops the transfer and removes the in-progress .part file.
    void abort();

    Status status() const;
    // Number of fully completed frames, including ERROR frames. This is the
    // value sent as `from` on a resumed request.
    uint64_t completedEntries() const;
    const StreamConsumerStats &stats() const;
    const std::string &rootPath() const;
    const std::string &lastError() const;

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace dtp

#endif // STREAMCONSUMER_H
