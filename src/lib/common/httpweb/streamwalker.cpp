// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "streamwalker.h"

#include <algorithm>
#include <filesystem>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#ifndef _WIN32
#include <sys/stat.h>
#endif

namespace dtp {
namespace {

namespace fs = std::filesystem;

struct EntryMeta {
    uint64_t size {0};
    uint32_t mode {0};
    uint64_t device {0};
    uint64_t inode {0};
    bool hasIdentity {false};
};

bool entryMeta(const std::string &path, EntryMeta *meta)
{
#ifndef _WIN32
    struct stat info = {};
    if (::stat(path.c_str(), &info) != 0)
        return false;
    meta->size = static_cast<uint64_t>(info.st_size);
    meta->mode = static_cast<uint32_t>(info.st_mode & 07777);
    meta->device = static_cast<uint64_t>(info.st_dev);
    meta->inode = static_cast<uint64_t>(info.st_ino);
    meta->hasIdentity = true;
    return true;
#else
    std::error_code ec;
    const auto size = fs::file_size(fs::u8path(path), ec);
    if (ec)
        return false;
    meta->size = static_cast<uint64_t>(size);
    meta->mode = 0;
    meta->hasIdentity = false;
    return true;
#endif
}

// Computes a portable target for an in-root symlink. The wire target is
// relative to the link's parent directory, so the receiver can recreate the
// link under a different destination root. Absolute source paths are therefore
// rewritten to a relative target.
bool isWithinRoot(const fs::path &root, const fs::path &target, std::error_code *error);

bool preservedLinkTarget(const fs::path &root, const fs::path &link,
                         std::string *target, bool *toDirectory, bool *outside,
                         std::string *error)
{
    *outside = false;
    std::error_code ec;
    const fs::path raw = fs::read_symlink(link, ec);
    if (ec) {
        *error = "cannot read symlink: " + ec.message();
        return false;
    }

    fs::path resolved = raw.is_absolute()
            ? fs::weakly_canonical(raw, ec)
            : fs::weakly_canonical(link.parent_path() / raw, ec);
    if (ec) {
        // A dangling link still has a well-defined lexical target; it can be
        // preserved as a dangling link on the receiver.
        resolved = (raw.is_absolute() ? raw : link.parent_path() / raw).lexically_normal();
        ec.clear();
    }

    std::error_code rootEc;
    if (!isWithinRoot(root, resolved, &rootEc)) {
        *outside = true;
        *error = rootEc ? "cannot resolve symlink: " + rootEc.message()
                        : "symlink target is outside the selected directory";
        return false;
    }

    fs::path relative = fs::relative(resolved, link.parent_path(), ec);
    if (ec || relative.is_absolute()) {
        ec.clear();
        relative = resolved.lexically_relative(link.parent_path());
    }
    if (relative.empty())
        relative = ".";
    if (relative.is_absolute()) {
        *error = "cannot express symlink target relative to its directory";
        return false;
    }

    *target = relative.generic_u8string();
    if (target->empty())
        *target = ".";
    *toDirectory = fs::is_directory(resolved, ec) && !ec;
    return true;
}

// Sort key: raw filename bytes. std::string compares as unsigned bytes, which
// matches "sort by original bytes" and stays stable across platforms.
struct DirEntry {
    std::string name;
    std::string absolute;
    bool isDirectory {false};
    bool isSymlink {false};
    std::string linkTarget;
    bool linkToDirectory {false};
    uint64_t size {0};
    uint32_t mode {0};
};

std::string joinRelative(const std::string &parent, const std::string &name)
{
    if (parent.empty())
        return name;
    return parent + "/" + name;
}

// Resolves symlinks and checks that `target` stays under `root`. A link that
// points outside the selection must not be followed: the receiver would get
// files the user did not select.
bool isWithinRoot(const fs::path &root, const fs::path &target, std::error_code *error)
{
    std::error_code ec;
    const fs::path relative = fs::relative(target, root, ec);
    if (ec) {
        if (error)
            *error = ec;
        return false;
    }
    if (relative.is_absolute())
        return false;
    for (const auto &part : relative) {
        if (part == "..")
            return false;
    }
    return true;
}

// Directory identity chain for one traversal branch. A symlink is a cycle only
// when its target is one of the directories above it; a sibling alias pointing
// at another subtree is not a cycle and must still be walked so its path is
// preserved on the receiving side.
struct AncestorNode {
    uint64_t device {0};
    uint64_t inode {0};
    std::shared_ptr<const AncestorNode> parent;
};

using AncestorChain = std::shared_ptr<const AncestorNode>;

bool hasAncestor(const AncestorChain &chain, uint64_t device, uint64_t inode)
{
    for (AncestorChain node = chain; node; node = node->parent) {
        if (node->device == device && node->inode == inode)
            return true;
    }
    return false;
}

struct DirTask {
    std::string absolute;
    std::string relative;
    int depth {0};
    AncestorChain ancestors;
};

} // namespace

StreamWalker::StreamWalker()
    : _options()
{
}

StreamWalker::StreamWalker(Options options)
    : _options(options)
{
}

bool StreamWalker::walk(const std::string &rootPath, const std::string &displayName,
                        const EntryCallback &onEntry, const ErrorCallback &onError,
                        WalkStats *stats)
{
    WalkStats local;
    WalkStats *counters = stats ? stats : &local;
    *counters = WalkStats {};

    const auto report = [&](const std::string &path, const std::string &message) {
        counters->errors++;
        if (onError)
            onError(path, message);
    };

    if (!onEntry)
        return false;

    std::error_code ec;
    const fs::path root = fs::u8path(rootPath);
    const fs::file_status rootStatus = fs::status(root, ec);
    if (ec || !fs::exists(rootStatus)) {
        report(rootPath, "cannot stat the selected path");
        return false;
    }

    // The root frame carries the display name, so it must be a single safe
    // component; a name with '/' or ".." would let the receiver escape.
    std::string nameError;
    if (!isSafeLeafName(displayName, &nameError)) {
        report(displayName, nameError);
        return false;
    }

    if (fs::is_regular_file(rootStatus)) {
        EntryMeta meta;
        if (!entryMeta(rootPath, &meta)) {
            report(rootPath, "cannot stat the selected file");
            return false;
        }
        if (meta.size > _options.maxFileSize) {
            report(rootPath, "file exceeds the frame size limit");
            return false;
        }
        WalkEntry entry;
        entry.type = FrameType::RootFile;
        entry.path = displayName;
        entry.absolute = rootPath;
        entry.size = meta.size;
        entry.mode = meta.mode;
        counters->files++;
        counters->bytes += meta.size;
        return onEntry(entry);
    }

    if (!fs::is_directory(rootStatus)) {
        report(rootPath, "selected path is neither a file nor a directory");
        return false;
    }

    std::vector<DirTask> stack;

    // The root directory frame is emitted here; its children are relative.
    {
        WalkEntry entry;
        entry.type = FrameType::RootDir;
        entry.path = displayName;
        entry.absolute = rootPath;
        counters->directories++;
        if (!onEntry(entry))
            return false;
    }

    stack.push_back(DirTask {rootPath, std::string(), 0, nullptr});

    while (!stack.empty()) {
        DirTask task = std::move(stack.back());
        stack.pop_back();

        if (task.depth > _options.maxDepth) {
            counters->skippedDeep++;
            report(task.absolute, "directory nesting exceeds the depth limit");
            continue;
        }

        // Symlink cycles are cut through the target identity, not the path:
        // a loop produces a different path string on every hop. Only ancestors
        // count: a link to a sibling directory is a legitimate second path.
        EntryMeta dirMeta;
        AncestorChain ancestors = task.ancestors;
        if (entryMeta(task.absolute, &dirMeta) && dirMeta.hasIdentity) {
            if (hasAncestor(ancestors, dirMeta.device, dirMeta.inode)) {
                counters->cycles++;
                report(task.absolute, "symlink loop detected");
                continue;
            }
            ancestors = std::make_shared<const AncestorNode>(
                    AncestorNode {dirMeta.device, dirMeta.inode, ancestors});
        }

        std::vector<DirEntry> entries;
        fs::directory_iterator it(fs::u8path(task.absolute), ec);
        if (ec) {
            report(task.absolute, "cannot list directory: " + ec.message());
            continue;
        }
        for (const auto &item : it) {
            std::error_code linkEc;
            const fs::file_status linkStatus = item.symlink_status(linkEc);
            if (linkEc) {
                report(item.path().u8string(), "cannot stat entry: " + linkEc.message());
                continue;
            }

            const bool isLink = fs::is_symlink(linkStatus);
            if (isLink && _options.preserveSymlinks) {
                DirEntry entry;
                entry.absolute = item.path().u8string();
                entry.name = item.path().filename().u8string();
                entry.isSymlink = true;
                bool outside = false;
                std::string linkError;
                if (!preservedLinkTarget(root, item.path(), &entry.linkTarget,
                                         &entry.linkToDirectory, &outside, &linkError)) {
                    if (outside)
                        counters->skippedOutside++;
                    report(entry.absolute, linkError);
                    continue;
                }
                entries.push_back(std::move(entry));
                continue;
            }
            if (isLink && _options.followSymlinks) {
                std::error_code rootEc;
                if (!isWithinRoot(root, item.path(), &rootEc)) {
                    counters->skippedOutside++;
                    report(item.path().u8string(),
                           rootEc ? "cannot resolve symlink target: " + rootEc.message()
                                  : "symlink target is outside the selected directory");
                    continue;
                }
            }

            const fs::file_status status =
                    _options.followSymlinks ? item.status(ec) : linkStatus;
            if (ec) {
                report(item.path().u8string(), "cannot stat entry: " + ec.message());
                ec.clear();
                continue;
            }

            DirEntry entry;
            entry.absolute = item.path().u8string();
            entry.name = item.path().filename().u8string();

            if (fs::is_directory(status)) {
                entry.isDirectory = true;
                entries.push_back(std::move(entry));
            } else if (fs::is_regular_file(status)) {
                EntryMeta meta;
                if (!entryMeta(entry.absolute, &meta)) {
                    report(entry.absolute, "cannot stat file");
                    continue;
                }
                entry.size = meta.size;
                entry.mode = meta.mode;
                entries.push_back(std::move(entry));
            } else {
                // FIFO/socket/device: opening them can block forever.
                counters->skippedSpecial++;
                report(entry.absolute, "special file skipped");
            }
        }

        std::sort(entries.begin(), entries.end(), [](const DirEntry &lhs, const DirEntry &rhs) {
            return lhs.name < rhs.name;
        });

        std::vector<DirTask> subdirectories;
        for (const auto &entry : entries) {
            const std::string relative = joinRelative(task.relative, entry.name);

            std::string pathError;
            if (!isSafeRelativePath(relative, &pathError)) {
                // The wire format cannot carry this name: report it and keep
                // going with the rest of the tree.
                counters->skippedUnsafe++;
                report(relative, pathError);
                continue;
            }

            WalkEntry wire;
            wire.path = relative;
            wire.absolute = entry.absolute;
            wire.mode = entry.mode;

            if (entry.isSymlink) {
                wire.type = FrameType::Symlink;
                wire.linkTarget = entry.linkTarget;
                wire.linkToDirectory = entry.linkToDirectory;
                counters->symlinks++;
                if (!onEntry(wire))
                    return false;
                continue;
            }

            if (entry.isDirectory) {
                wire.type = FrameType::Dir;
                counters->directories++;
                if (!onEntry(wire))
                    return false;
                subdirectories.push_back(
                        DirTask {entry.absolute, relative, task.depth + 1, ancestors});
                continue;
            }

            if (entry.size > _options.maxFileSize) {
                report(relative, "file exceeds the frame size limit");
                continue;
            }
            wire.type = FrameType::File;
            wire.size = entry.size;
            counters->files++;
            counters->bytes += entry.size;
            if (!onEntry(wire))
                return false;
        }

        // Depth first, preserving sorted order.
        for (auto sub = subdirectories.rbegin(); sub != subdirectories.rend(); ++sub)
            stack.push_back(std::move(*sub));
    }

    return true;
}

} // namespace dtp
