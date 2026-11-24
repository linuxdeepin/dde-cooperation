// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// DTP2 sender pipeline: walk -> frame -> compress -> sink.
//
// The producer owns one archive stream (one top level selection). It runs on a
// dedicated thread: it performs blocking disk reads and compression, and calls
// the sink synchronously. The sink is where backpressure lives: for the HTTP
// path it pushes into a bounded queue and blocks when the queue is full, so the
// producer never outruns the socket.
//
// Never call run() from the Asio IO thread.
//
// Failure model:
//   - a file that cannot be opened/stat'ed becomes an ERROR frame and the
//     transfer continues;
//   - a file that was readable but shrinks or errors mid read aborts the stream
//     (its declared size can no longer be honoured); no END frame is emitted;
//   - cancellation is checked between frames and between read blocks, so a
//     stop request is honoured within one block.

#ifndef STREAMPRODUCER_H
#define STREAMPRODUCER_H

#include "streamcodec.h"
#include "streamframe.h"
#include "streamwalker.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace dtp {

struct StreamProducerOptions {
    Codec codec {Codec::Deflate};
    int level {-1};
    size_t readBlock {128 * 1024};
    // Frames already delivered to the receiver. The walker still enumerates
    // the tree in the same deterministic order, but the first N entries are
    // not emitted or read. Resume alignment assumes the source tree did not
    // change between attempts (same size/mtime); a changed tree may shift the
    // frame indices and is rejected by the receiver's conflict check.
    uint64_t skipEntries {0};
    StreamWalker::Options walker;
};

struct StreamProducerStats {
    uint64_t files {0};
    uint64_t directories {0};
    uint64_t bytesRead {0};   // uncompressed bytes read from disk
    uint64_t bytesSent {0};   // bytes handed to the sink, preamble included
    uint64_t frames {0};
    uint64_t skippedSpecial {0};
    uint64_t skippedUnsafe {0};
    uint64_t cycles {0};
    uint64_t walkErrors {0};
    double elapsedSeconds {0};
};

// File level events, used to drive the sender UI from the producer thread.
// value is the frame size for Begin/End and the number of bytes just read for
// Progress. Errors are reported separately through ErrorCallback: a file that
// cannot be read becomes an ERROR frame and the stream continues.
enum class StreamEvent {
    FileBegin,
    FileProgress,
    FileEnd,
};

class StreamProducer
{
public:
    // Called on the producer thread. Returning false aborts the stream.
    using Sink = std::function<bool(const char *data, size_t size)>;
    // Polled between frames and between read blocks. True means "stop now".
    using CancelCheck = std::function<bool()>;
    using ErrorCallback = std::function<void(const std::string &path, const std::string &message)>;
    // wirePath is relative to the selected root; absolutePath is the file on
    // disk. Both are provided so callers do not have to rebuild either form.
    using EventCallback = std::function<void(StreamEvent event, const std::string &wirePath,
                                             const std::string &absolutePath, uint64_t value)>;

    explicit StreamProducer(StreamProducerOptions options);
    StreamProducer();

    // Optional; may be called before run(). Invoked on the producer thread.
    void setEventCallback(EventCallback callback);

    // sourcePath: selected file or directory. displayName: top level name.
    // Returns false on cancel, sink refusal or a fatal read error; *error then
    // describes the cause. In that case no END frame was emitted, so the
    // receiver treats the stream as failed.
    bool run(const std::string &sourcePath, const std::string &displayName,
             const Sink &sink, const CancelCheck &cancel, const ErrorCallback &onError,
             StreamProducerStats *stats, std::string *error);

private:
    StreamProducerOptions _options;
    EventCallback _onEvent;
};

} // namespace dtp

#endif // STREAMPRODUCER_H
