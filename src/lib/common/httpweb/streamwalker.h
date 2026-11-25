// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Deterministic directory traversal for the DTP2 sender.
//
// The wire order must be identical on every run: resume replays frames by
// index, so a directory listing that changes order between runs would resume
// at the wrong file. Every directory is therefore sorted by raw filename bytes
// before any entry is emitted.
//
// Rules:
//   - explicit stack, never recursion: deep trees must not overflow the stack;
//   - symlinks are followed only when their canonical target stays inside the
//     selected root; outside or unresolvable targets are reported and skipped
//     so selecting a directory never leaks unrelated files;
//   - in preserve mode, in-root symlinks are emitted as Symlink entries and
//     never followed, so the receiver can recreate them without duplicating
//     their targets; out-of-root links are still skipped;
//   - when following links, a (dev, inode) chain of the current path's
//     ancestors cuts symlink loops; a link to a sibling subtree is walked too,
//     so either path is preserved;
//   - socket/FIFO/device entries are reported and skipped, never opened:
//     opening a FIFO would block the producer thread forever;
//   - names that cannot be represented on the wire are reported and skipped
//     instead of aborting the whole transfer;
//   - per directory, entries are emitted in sorted order and subdirectories are
//     descended afterwards, depth first.
//
// Threading: StreamWalker is used by the producer thread only and performs
// blocking filesystem calls by design. It must never run on the Asio IO thread.

#ifndef STREAMWALKER_H
#define STREAMWALKER_H

#include "streamframe.h"

#include <cstdint>
#include <functional>
#include <string>

namespace dtp {

struct WalkEntry {
    FrameType type {FrameType::File};
    std::string path;      // wire path: display name for the root, else relative
    std::string absolute;  // on-disk path, used to read the file
    uint64_t size {0};     // bytes to send (0 for directories/symlinks)
    uint32_t mode {0};     // POSIX permission bits, 0 when unavailable
    // Symlink entries only: target relative to the link's parent directory,
    // normalized so it cannot escape the selected root.
    std::string linkTarget;
    bool linkToDirectory {false};
};

struct WalkStats {
    uint64_t directories {0};
    uint64_t files {0};
    uint64_t symlinks {0};
    uint64_t bytes {0};
    uint64_t skippedSpecial {0};  // FIFO/socket/device
    uint64_t skippedUnsafe {0};   // names the wire format cannot carry
    uint64_t skippedOutside {0};  // symlink target outside the selected root
    uint64_t skippedDeep {0};     // depth limit hit
    uint64_t cycles {0};          // symlink loop that was cut
    uint64_t errors {0};          // unreadable entries
};

class StreamWalker
{
public:
    struct Options {
        bool followSymlinks {true};
        // When true, in-root symlinks are emitted as Symlink entries and never
        // followed; out-of-root links are skipped. Mutually exclusive with the
        // legacy follow behavior above.
        bool preserveSymlinks {false};
        int maxDepth {256};
        uint64_t maxFileSize {kDefaultMaxFrameSize};
    };

    // Returning false aborts the walk (e.g. the sink asked to stop).
    using EntryCallback = std::function<bool(const WalkEntry &)>;
    // Diagnostic only; the walker keeps going unless the entry is the root.
    using ErrorCallback = std::function<void(const std::string &path, const std::string &message)>;

    explicit StreamWalker(Options options);
    StreamWalker();

    // rootPath: the selected file or directory on disk.
    // displayName: the top level name used by the RootDir/RootFile frame.
    // Returns false only when the root itself cannot be served; per-entry
    // problems are reported through onError and counted in stats.
    bool walk(const std::string &rootPath, const std::string &displayName,
              const EntryCallback &onEntry, const ErrorCallback &onError, WalkStats *stats);

private:
    Options _options;
};

} // namespace dtp

#endif // STREAMWALKER_H
