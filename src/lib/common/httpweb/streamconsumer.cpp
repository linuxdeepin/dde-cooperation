// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "streamconsumer.h"

#include "streamcodec.h"
#include "streamframe.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <set>
#include <system_error>
#include <utility>

namespace dtp {
namespace {

namespace fs = std::filesystem;

// Decompress in small slices: one compressed chunk can expand by ~1000x, so a
// 16 KB slice bounds the temporary decoded buffer to roughly 16 MB.
constexpr size_t kDecodeSlice = 16 * 1024;

bool pathExists(const fs::path &path)
{
    std::error_code ec;
    return fs::symlink_status(path, ec).type() != fs::file_type::not_found && !ec;
}

bool isInside(const fs::path &root, const fs::path &candidate)
{
    const fs::path normalizedRoot = root.lexically_normal();
    const fs::path normalized = candidate.lexically_normal();

    auto rootIt = normalizedRoot.begin();
    auto pathIt = normalized.begin();
    for (; rootIt != normalizedRoot.end(); ++rootIt, ++pathIt) {
        if (pathIt == normalized.end() || *pathIt != *rootIt)
            return false;
    }
    return true;
}

// True when an existing component between root and target is a symlink. Such a
// component could redirect the write outside the destination even though the
// lexical path is inside it.
bool hasSymlinkComponent(const fs::path &root, const fs::path &target)
{
    std::error_code ec;
    fs::path current = root;
    for (const auto &part : target.lexically_relative(root)) {
        current /= part;
        const fs::file_status status = fs::symlink_status(current, ec);
        if (ec) {
            ec.clear();
            continue;
        }
        if (status.type() != fs::file_type::not_found && fs::is_symlink(status))
            return true;
    }
    return false;
}

} // namespace

struct StreamConsumer::Impl {
    StreamConsumerOptions options;
    ProgressCallback progress;
    FileCallback onFileBegin;
    FileCallback onFileEnd;
    ErrorCallback onError;
    CheckpointCallback onCheckpoint;

    StreamDecoder decoder;
    FrameParser parser;
    Codec codec {Codec::Deflate};
    bool decoderReady {false};
    bool finishedOk {false};
    Status status {Status::Active};
    std::string error;
    std::string preamble;
    std::string decoded;

    StreamConsumerStats stats;
    fs::path root;
    bool resumeMode {false};
    uint64_t completed {0};
    fs::path currentFinal;
    fs::path currentPart;
    std::ofstream currentOut;
    uint64_t currentSize {0};
    uint64_t currentWritten {0};

    explicit Impl(StreamConsumerOptions inOptions)
        : options(std::move(inOptions))
    {
        if (!options.resumeRoot.empty()) {
            resumeMode = true;
            root = fs::u8path(options.resumeRoot);
            std::error_code ec;
            if (!fs::is_directory(root, ec) || ec) {
                error = "resume root does not exist: " + options.resumeRoot;
                status = Status::Error;
            } else {
                stats.rootPath = root.u8string();
            }
        }
        parser.onHeader([this](const FrameHeader &header) { return handleHeader(header); });
        parser.onData([this](const char *data, size_t size) { return handleData(data, size); });
        parser.onError([this](const std::string &path, const std::string &message) {
            if (onError)
                onError(path, message);
            markCompleted();
            return true;
        });
        parser.onEnd([this](const FrameEnd &) {
            // The last file frame is only complete at END: its trailer CRC was
            // verified immediately before this callback.
            if (!finalizeCurrent())
                return false;
            finishedOk = true;
            return true;
        });
    }

    void markCompleted()
    {
        ++completed;
        if (onCheckpoint)
            onCheckpoint(completed);
    }

    bool fail(const std::string &message)
    {
        error = message;
        cleanupPartial();
        status = Status::Error;
        return false;
    }

    void cleanupPartial()
    {
        if (currentOut.is_open())
            currentOut.close();
        if (!currentPart.empty()) {
            std::error_code ec;
            fs::remove(currentPart, ec);
        }
        currentPart.clear();
        currentFinal.clear();
        currentSize = 0;
        currentWritten = 0;
    }

    fs::path uniquePath(const fs::path &desired, bool directory) const
    {
        if (!pathExists(desired))
            return desired;

        const fs::path parent = desired.parent_path();
        const std::string filename = desired.filename().u8string();
        std::string prefix = filename;
        std::string suffix;
        if (!directory) {
            const size_t dot = filename.find_last_of('.');
            if (dot != std::string::npos && dot != 0) {
                prefix = filename.substr(0, dot);
                suffix = filename.substr(dot);
            }
        }

        for (int i = 1; i < 100000; ++i) {
            const fs::path candidate =
                    parent / fs::u8path(prefix + "(" + std::to_string(i) + ")" + suffix);
            if (!pathExists(candidate))
                return candidate;
        }
        return fs::path();
    }

    bool createRootDir(const FrameHeader &header)
    {
        if (!root.empty())
            return fail("archive contains more than one root directory");

        if (options.targetDir.empty())
            return fail("target directory is empty");

        std::error_code ec;
        fs::create_directories(fs::u8path(options.targetDir), ec);
        if (ec)
            return fail("cannot create the target directory: " + ec.message());

        const fs::path desired =
                fs::u8path(options.targetDir) / fs::u8path(header.path);
        const fs::path resolved = uniquePath(desired, true);
        if (resolved.empty())
            return fail("cannot find a free name for the root directory");

        if (!fs::create_directory(resolved, ec) || ec)
            return fail("cannot create the root directory: " + ec.message());

        root = resolved;
        stats.directories++;
        stats.rootPath = root.u8string();
        markCompleted();
        return true;
    }

    bool createDir(const FrameHeader &header)
    {
        if (root.empty() || !fs::is_directory(root))
            return fail("directory frame before the root frame");

        const fs::path target = root / fs::u8path(header.path);
        if (!isInside(root, target))
            return fail("directory path escapes the destination: " + header.path);
        if (hasSymlinkComponent(root, target))
            return fail("directory path crosses a symlink: " + header.path);

        std::error_code ec;
        if (pathExists(target)) {
            const fs::file_status status = fs::symlink_status(target, ec);
            if (ec || fs::is_symlink(status) || !fs::is_directory(status))
                return fail("directory conflicts with an existing entry: " + header.path);
        } else if (!fs::create_directory(target, ec) && ec) {
            return fail("cannot create directory " + header.path + ": " + ec.message());
        }

        stats.directories++;
        markCompleted();
        return true;
    }

    bool startFile(const FrameHeader &header, const fs::path &final)
    {
        // A directory cannot become a file and vice versa; uniquePath() only
        // appends a suffix when the name is taken, so this is a genuine error.
        std::error_code ec;
        const fs::file_status existing = fs::symlink_status(final, ec);
        if (ec) {
            ec.clear();
        } else if (existing.type() != fs::file_type::not_found && fs::is_directory(existing)) {
            return fail("file frame conflicts with a directory: " + header.path);
        }

        if (!finalizeCurrent())
            return false;

        currentFinal = final;
        currentPart = final;
        currentPart += ".part";
        currentSize = header.size;
        currentWritten = 0;

        currentOut.open(currentPart, std::ios::binary | std::ios::trunc);
        if (!currentOut.is_open())
            return fail("cannot create file: " + currentPart.u8string());

        if (onFileBegin)
            onFileBegin(final.u8string(), header.size);
        return true;
    }

    bool handleHeader(const FrameHeader &header)
    {
        // A new header means the previous file frame ended and its trailer CRC
        // was verified. Finalise it before resolving names for the next entry,
        // otherwise a duplicate name would still look "free" on disk.
        if (!finalizeCurrent())
            return false;

        if (resumeMode
                && (header.type == FrameType::RootDir || header.type == FrameType::RootFile)) {
            return fail("resumed stream must not start with a root frame");
        }

        switch (header.type) {
        case FrameType::RootDir:
            return createRootDir(header);

        case FrameType::Dir:
            return createDir(header);

        case FrameType::RootFile: {
            if (!root.empty())
                return fail("root file after a root directory");
            std::error_code ec;
            fs::create_directories(fs::u8path(options.targetDir), ec);
            if (ec)
                return fail("cannot create the target directory: " + ec.message());
            const fs::path resolved = uniquePath(
                    fs::u8path(options.targetDir) / fs::u8path(header.path), false);
            if (resolved.empty())
                return fail("cannot find a free name for the file");
            stats.rootPath = resolved.u8string();
            stats.files++;
            return startFile(header, resolved);
        }

        case FrameType::File: {
            if (root.empty())
                return fail("file frame before the root frame");

            const fs::path target = root / fs::u8path(header.path);
            if (!isInside(root, target))
                return fail("file path escapes the destination: " + header.path);
            if (hasSymlinkComponent(root, target))
                return fail("file path crosses a symlink: " + header.path);
            if (resumeMode && pathExists(target))
                return fail("resume conflict, file already exists: " + header.path);

            std::error_code ec;
            fs::create_directories(target.parent_path(), ec);
            if (ec)
                return fail("cannot create parent directory: " + ec.message());

            const fs::path resolved = uniquePath(target, false);
            if (resolved.empty())
                return fail("cannot find a free name for: " + header.path);
            stats.files++;
            return startFile(header, resolved);
        }

        case FrameType::Error:
        case FrameType::End:
            // ERROR carries a message (handled by the parser's onError), END is
            // handled by onEnd. Neither opens a file.
            return true;
        }
        return fail("unknown frame type");
    }

    bool handleData(const char *data, size_t size)
    {
        if (!currentOut.is_open())
            return fail("file data without a file frame");

        currentOut.write(data, static_cast<std::streamsize>(size));
        if (!currentOut.good())
            return fail("write failed: " + currentPart.u8string());

        currentWritten += size;
        stats.bytes += size;

        if (progress && !progress(size)) {
            error = "cancelled";
            cleanupPartial();
            status = Status::Canceled;
            return false;
        }
        return true;
    }

    bool finalizeCurrent()
    {
        if (!currentOut.is_open())
            return true;

        currentOut.flush();
        const bool writeOk = currentOut.good();
        currentOut.close();

        if (!writeOk || currentWritten != currentSize) {
            const std::string message = "file size mismatch: " + currentFinal.u8string();
            cleanupPartial();
            error = message;
            status = Status::Error;
            return false;
        }

        std::error_code ec;
        fs::rename(currentPart, currentFinal, ec);
        if (ec) {
            const std::string message = "cannot finalise " + currentFinal.u8string()
                    + ": " + ec.message();
            cleanupPartial();
            error = message;
            status = Status::Error;
            return false;
        }

        if (onFileEnd)
            onFileEnd(currentFinal.u8string(), currentSize);

        markCompleted();
        currentPart.clear();
        currentFinal.clear();
        currentSize = 0;
        currentWritten = 0;
        return true;
    }

    bool decodeChunk(const char *data, size_t size)
    {
        for (size_t offset = 0; offset < size; offset += kDecodeSlice) {
            decoded.clear();
            const size_t slice = std::min(kDecodeSlice, size - offset);
            if (!decoder.decode(data + offset, slice, &decoded))
                return fail(decoder.lastError());

            if (!decoded.empty() && !parser.feed(decoded.data(), decoded.size())) {
                if (error.empty())
                    error = parser.lastError();
                if (status == Status::Active)
                    status = Status::Error;
                cleanupPartial();
                return false;
            }
        }
        return true;
    }

    bool feed(const char *data, size_t size)
    {
        if (status != Status::Active)
            return false;
        if (!data || size == 0)
            return true;

        if (!decoderReady) {
            preamble.append(data, size);
            if (preamble.size() < kStreamPreambleSize)
                return true;

            StreamPreamble header;
            if (!decodePreamble(preamble.data(), kStreamPreambleSize, &header, &error))
                return fail(error);
            codec = header.codec;
            if (!decoder.init(codec))
                return fail(decoder.lastError());
            decoderReady = true;

            const std::string rest = preamble.substr(kStreamPreambleSize);
            preamble.clear();
            return decodeChunk(rest.data(), rest.size());
        }

        return decodeChunk(data, size);
    }

    bool finish(std::string *outError)
    {
        if (status == Status::Ok)
            return true;

        if (status == Status::Error || status == Status::Canceled) {
            if (outError)
                *outError = error;
            return false;
        }

        if (!decoderReady) {
            fail("stream ended before the preamble");
        } else if (!parser.finished()) {
            fail("stream ended before the END frame");
        } else if (codec != Codec::None && !decoder.finished()) {
            fail("compressed stream is truncated");
        } else if (!finalizeCurrent()) {
            // finalizeCurrent() already stored the reason.
        } else {
            status = Status::Ok;
        }

        if (status != Status::Ok) {
            if (outError)
                *outError = error;
            return false;
        }
        return true;
    }

    void abort()
    {
        if (status != Status::Active)
            return;
        cleanupPartial();
        error = "cancelled";
        status = Status::Canceled;
    }
};

StreamConsumer::StreamConsumer(StreamConsumerOptions options)
    : d(new Impl(std::move(options)))
{
}

StreamConsumer::~StreamConsumer() = default;

void StreamConsumer::setProgressCallback(ProgressCallback callback)
{
    d->progress = std::move(callback);
}

void StreamConsumer::setFileCallback(FileCallback onBegin, FileCallback onEnd)
{
    d->onFileBegin = std::move(onBegin);
    d->onFileEnd = std::move(onEnd);
}

void StreamConsumer::setErrorCallback(ErrorCallback callback)
{
    d->onError = std::move(callback);
}

void StreamConsumer::setCheckpointCallback(CheckpointCallback callback)
{
    d->onCheckpoint = std::move(callback);
}

bool StreamConsumer::feed(const char *data, size_t size)
{
    return d->feed(data, size);
}

bool StreamConsumer::finish(std::string *error)
{
    return d->finish(error);
}

void StreamConsumer::abort()
{
    d->abort();
}

StreamConsumer::Status StreamConsumer::status() const
{
    return d->status;
}

uint64_t StreamConsumer::completedEntries() const
{
    return d->completed;
}

const StreamConsumerStats &StreamConsumer::stats() const
{
    return d->stats;
}

const std::string &StreamConsumer::rootPath() const
{
    return d->stats.rootPath;
}

const std::string &StreamConsumer::lastError() const
{
    return d->error;
}

} // namespace dtp
