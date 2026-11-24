// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "streamproducer.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <vector>

namespace dtp {
namespace {

namespace fs = std::filesystem;
using Clock = std::chrono::steady_clock;

// Reads at most `size` bytes starting at 0. Returns false on open/read error.
bool openSource(const std::string &path, uint64_t *size, std::ifstream *in)
{
    std::error_code ec;
    if (!fs::is_regular_file(fs::u8path(path), ec) || ec)
        return false;
    *size = static_cast<uint64_t>(fs::file_size(fs::u8path(path), ec));
    if (ec)
        return false;
    in->open(fs::u8path(path), std::ios::binary);
    return in->is_open();
}

} // namespace

StreamProducer::StreamProducer()
    : _options()
{
}

StreamProducer::StreamProducer(StreamProducerOptions options)
    : _options(options)
{
}

void StreamProducer::setEventCallback(EventCallback callback)
{
    _onEvent = std::move(callback);
}

bool StreamProducer::run(const std::string &sourcePath, const std::string &displayName,
                         const Sink &sink, const CancelCheck &cancel, const ErrorCallback &onError,
                         StreamProducerStats *stats, std::string *error)
{
    StreamProducerStats local;
    StreamProducerStats *counters = stats ? stats : &local;
    *counters = StreamProducerStats {};
    const auto started = Clock::now();

    const auto fail = [error](const std::string &message) {
        if (error)
            *error = message;
        return false;
    };
    const auto stopped = [&cancel]() { return cancel && cancel(); };

    if (!sink)
        return fail("null sink");
    if (error)
        error->clear();

    FrameWriter writer;
    StreamEncoder encoder;
    if (!encoder.init(_options.codec, _options.level))
        return fail("codec init failed: " + encoder.lastError());

    std::string pending;
    pending.reserve(_options.readBlock + 4096);
    std::string compressed;
    uint64_t bytesSent = 0;

    // The sink sees every byte, so a refusal stops the pipeline immediately.
    const auto emit = [&](const char *data, size_t size) -> bool {
        if (size == 0)
            return true;
        if (!sink(data, size))
            return false;
        bytesSent += size;
        return true;
    };

    // Encodes whatever is in `pending` and hands it to the sink.
    const auto flushPending = [&]() -> bool {
        if (pending.empty())
            return true;
        if (!encoder.encode(pending.data(), pending.size(), &compressed))
            return false;
        pending.clear();
        return true;
    };

    const auto drainEncoded = [&]() -> bool {
        if (compressed.empty())
            return true;
        const bool ok = emit(compressed.data(), compressed.size());
        compressed.clear();
        return ok;
    };

    // Preamble is never compressed: the receiver needs the codec first.
    StreamPreamble preamble;
    preamble.codec = _options.codec;
    std::string header;
    encodePreamble(preamble, &header);
    if (!emit(header.data(), header.size()))
        return fail("sink refused the stream preamble");

    StreamWalker walker(_options.walker);
    WalkStats walkStats;
    std::ifstream source;
    std::vector<char> buffer(_options.readBlock);
    std::string fatal;
    uint64_t seen = 0;

    const auto onEntry = [&](const WalkEntry &entry) -> bool {
        ++seen;
        if (seen <= _options.skipEntries) {
            // Already delivered and verified by the receiver on a previous
            // attempt. Enumerating (without reading) keeps the frame indices
            // aligned with the receiver's completed count.
            if (stopped()) {
                fatal = "cancelled";
                return false;
            }
            return true;
        }
        if (stopped()) {
            fatal = "cancelled";
            return false;
        }

        const bool isRoot = entry.type == FrameType::RootDir || entry.type == FrameType::RootFile;
        pending.clear();

        if (entry.type == FrameType::RootDir) {
            if (!writer.writeRootDir(&pending, entry.path, entry.mode))
                { fatal = writer.lastError(); return false; }
        } else if (entry.type == FrameType::Dir) {
            if (!writer.writeDir(&pending, entry.path, entry.mode))
                { fatal = writer.lastError(); return false; }
        } else {
            // Open before writing the frame header: a file that vanished turns
            // into an ERROR frame instead of a truncated FILE frame.
            uint64_t size = 0;
            source.close();
            source.clear();
            if (!openSource(entry.absolute, &size, &source)) {
                counters->walkErrors++;
                if (onError)
                    onError(entry.path, "cannot open source file");
                pending.clear();
                if (!writer.writeError(&pending, entry.path, "cannot open source file"))
                    { fatal = writer.lastError(); return false; }
                if (!flushPending() || !drainEncoded())
                    { fatal = "sink refused data"; return false; }
                return true;
            }

            const bool began = isRoot
                    ? writer.beginRootFile(&pending, entry.path, size, 0, entry.mode)
                    : writer.beginFile(&pending, entry.path, size, 0, entry.mode);
            if (!began) {
                fatal = writer.lastError();
                source.close();
                return false;
            }

            uint64_t read = 0;
            if (_onEvent)
                _onEvent(StreamEvent::FileBegin, entry.path, entry.absolute, size);
            while (read < size) {
                if (stopped()) {
                    fatal = "cancelled";
                    source.close();
                    return false;
                }
                const size_t want =
                        static_cast<size_t>(std::min<uint64_t>(buffer.size(), size - read));
                source.read(buffer.data(), static_cast<std::streamsize>(want));
                const std::streamsize got = source.gcount();
                if (got <= 0) {
                    // The file shrank after we declared its size. The frame can
                    // no longer be completed, so abort instead of emitting a
                    // stream the receiver would have to reject.
                    fatal = "source changed while reading: " + entry.path;
                    source.close();
                    return false;
                }
                if (!writer.appendFileData(&pending, buffer.data(), static_cast<size_t>(got))) {
                    fatal = writer.lastError();
                    source.close();
                    return false;
                }
                read += static_cast<uint64_t>(got);
                counters->bytesRead += static_cast<uint64_t>(got);
                if (_onEvent)
                    _onEvent(StreamEvent::FileProgress, entry.path, entry.absolute,
                             static_cast<uint64_t>(got));

                if (!flushPending() || !drainEncoded()) {
                    fatal = "sink refused data";
                    source.close();
                    return false;
                }
            }
            source.close();

            if (!writer.endFile(&pending)) {
                fatal = writer.lastError();
                return false;
            }
            if (!flushPending() || !drainEncoded())
                { fatal = "sink refused data"; return false; }

            // Flush at every file boundary so the receiver can finish this file
            // without waiting for the rest of the archive.
            if (!encoder.flush(&compressed) || !drainEncoded())
                { fatal = "sink refused data"; return false; }
            if (_onEvent)
                _onEvent(StreamEvent::FileEnd, entry.path, entry.absolute, size);
            return true;
        }

        if (!flushPending() || !drainEncoded())
            { fatal = "sink refused data"; return false; }
        return true;
    };

    if (!walker.walk(sourcePath, displayName, onEntry, onError, &walkStats)) {
        if (!fatal.empty())
            return fail(fatal);
        return fail("walk failed for " + sourcePath);
    }

    if (stopped())
        return fail("cancelled");

    // A resume position past the enumerated tree means the source changed
    // (or the checkpoint is stale). Emitting only END would make the receiver
    // believe the missing files were already delivered.
    if (_options.skipEntries > seen)
        return fail("resume position is beyond the archive");

    // END carries the totals; writeEnd() uses the writer's own counters so the
    // receiver can cross-check what it actually consumed.
    pending.clear();
    if (!writer.writeEnd(&pending))
        return fail(writer.lastError());
    if (!flushPending() || !encoder.finish(&compressed) || !drainEncoded())
        return fail("sink refused the stream trailer");

    counters->files = walkStats.files;
    counters->directories = walkStats.directories;
    counters->frames = writer.entryCount();
    counters->skippedSpecial = walkStats.skippedSpecial;
    counters->skippedUnsafe = walkStats.skippedUnsafe;
    counters->cycles = walkStats.cycles;
    counters->walkErrors = walkStats.errors;
    counters->bytesSent = bytesSent;
    counters->elapsedSeconds = std::chrono::duration<double>(Clock::now() - started).count();
    if (error)
        error->clear();
    return true;
}

} // namespace dtp
