// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// httpweb transfer benchmark.
//
// Two jobs:
//   1. Generate reproducible datasets (small files, deep trees, one huge file).
//   2. Run the legacy per-file HTTP transfer over loopback and print wall time,
//      CPU, peak RSS, HTTP request counts, longest no-data stall and failures.
//
// Everything is measured in process, no external tooling required. The output
// is plain key=value lines so it can be pasted into the design docs unchanged.
//
// Build:
//   cmake -S . -B build-bench -DBUILD_HTTPWEB_BENCH=ON -DCMAKE_BUILD_TYPE=Release
//   cmake --build build-bench --target httpweb_transfer_benchmark
//
// Examples:
//   httpweb_transfer_benchmark --generate /tmp/dtp-100k --files 100000 --size 4096
//   httpweb_transfer_benchmark --legacy --src /tmp/dtp-100k --dst /tmp/dtp-out --timeout 900
//   httpweb_transfer_benchmark --legacy --src /tmp/dtp-100k --dst /tmp/dtp-out --repeat 2

#include "fileserver.h"
#include "fileclient.h"
#include "tokencache.h"
#include "webbinder.h"
#include "webproto.h"

#include "asio/service.h"
#include "asio/ssl_context.h"
#include "sslcertconf.h"

#include <openssl/ssl.h>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <mutex>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#ifndef _WIN32
#include <sys/resource.h>
#include <unistd.h>
#endif

#include <zlib.h>

namespace fs = std::filesystem;
using Clock = std::chrono::steady_clock;

namespace {

constexpr size_t kWriteBlock = 128 * 1024;

struct Options {
    bool generate {false};
    bool legacy {false};
    bool stream {false};
    bool verify {true};
    bool serve {false};

    std::string dir;
    std::string src;
    std::string dst;

    // Remote client/server mode: one process hosts the web, the other pulls it
    // over the network. The in-process mode remains the default.
    std::string connectHost;
    std::string bindAddress {"0.0.0.0"};
    std::string token;
    std::string tokenFile;

    uint64_t files {1000};
    uint64_t fileSize {4096};
    uint64_t dirs {64};
    uint64_t depth {1};
    std::string pattern {"text"};
    std::string single;
    uint64_t seed {20261029};

    int port {28443};
    bool noDelay {false};
    int timeoutSeconds {600};
    int repeat {1};
};

void usage()
{
    std::cout <<
            "usage:\n"
            "  --generate DIR [--files N] [--size BYTES] [--dirs N] [--depth N]\n"
            "                 [--pattern text|random|zero] [--single SIZE] [--seed N]\n"
            "  --legacy|--stream --src DIR --dst DIR [--port N] [--timeout SEC]\n"
            "                 [--repeat N] [--no-verify] [--no-delay]\n"
            "  --serve --legacy|--stream --src DIR [--bind ADDR] [--port N]\n"
            "                 [--token-file PATH] [--timeout SEC] [--no-delay]\n"
            "  --connect HOST --legacy|--stream --dst DIR (--token TOKEN|--token-file PATH)\n"
            "                 [--src DIR] [--port N] [--timeout SEC] [--no-delay]\n"
            "\n"
            "Remote mode: start --serve on one machine, then run --connect on the other.\n"
            "SIZE accepts K/M/G suffixes (e.g. 10G).\n";
}

uint64_t parseSize(const std::string &text)
{
    if (text.empty())
        return 0;
    size_t end = 0;
    const uint64_t value = std::stoull(text, &end);
    if (end == text.size())
        return value;
    const char suffix = static_cast<char>(std::tolower(text[end]));
    const uint64_t scale = suffix == 'k' ? 1024ULL
            : suffix == 'm' ? 1024ULL * 1024ULL
            : suffix == 'g' ? 1024ULL * 1024ULL * 1024ULL
            : 0;
    if (scale == 0 || end + 1 != text.size())
        throw std::runtime_error("invalid size: " + text);
    return value * scale;
}

bool parseArgs(int argc, char **argv, Options *options)
{
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto next = [&](const char *name) -> std::string {
            if (i + 1 >= argc)
                throw std::runtime_error(std::string("missing value for ") + name);
            return argv[++i];
        };

        if (arg == "--generate") {
            options->generate = true;
            // The documented shape is "--generate <dir>"; --dir also works.
            if (i + 1 < argc && argv[i + 1][0] != '-')
                options->dir = argv[++i];
        }
        else if (arg == "--legacy")
            options->legacy = true;
        else if (arg == "--stream")
            options->stream = true;
        else if (arg == "--serve")
            options->serve = true;
        else if (arg == "--connect")
            options->connectHost = next("--connect");
        else if (arg == "--bind")
            options->bindAddress = next("--bind");
        else if (arg == "--token")
            options->token = next("--token");
        else if (arg == "--token-file")
            options->tokenFile = next("--token-file");
        else if (arg == "--no-verify")
            options->verify = false;
        else if (arg == "--no-delay")
            options->noDelay = true;
        else if (arg == "--dir")
            options->dir = next("--dir");
        else if (arg == "--src")
            options->src = next("--src");
        else if (arg == "--dst")
            options->dst = next("--dst");
        else if (arg == "--files")
            options->files = parseSize(next("--files"));
        else if (arg == "--size")
            options->fileSize = parseSize(next("--size"));
        else if (arg == "--dirs")
            options->dirs = std::max<uint64_t>(1, parseSize(next("--dirs")));
        else if (arg == "--depth")
            options->depth = std::max<uint64_t>(1, parseSize(next("--depth")));
        else if (arg == "--pattern")
            options->pattern = next("--pattern");
        else if (arg == "--single")
            options->single = next("--single");
        else if (arg == "--seed")
            options->seed = parseSize(next("--seed"));
        else if (arg == "--port")
            options->port = static_cast<int>(parseSize(next("--port")));
        else if (arg == "--timeout")
            options->timeoutSeconds = static_cast<int>(parseSize(next("--timeout")));
        else if (arg == "--repeat")
            options->repeat = std::max(1, static_cast<int>(parseSize(next("--repeat"))));
        else if (arg == "--help" || arg == "-h") {
            usage();
            std::exit(0);
        } else {
            throw std::runtime_error("unknown argument: " + arg);
        }
    }

    if (options->generate && options->dir.empty()) {
        std::cerr << "--generate needs --dir\n";
        return false;
    }
    if (options->legacy && options->stream) {
        std::cerr << "--legacy and --stream are mutually exclusive\n";
        return false;
    }

    const bool transferMode = options->legacy || options->stream;
    const bool remoteClient = !options->connectHost.empty();
    if (options->serve && remoteClient) {
        std::cerr << "--serve and --connect are mutually exclusive\n";
        return false;
    }
    if (options->serve) {
        if (!transferMode) {
            std::cerr << "--serve needs --legacy or --stream\n";
            return false;
        }
        if (options->src.empty()) {
            std::cerr << "--serve needs --src\n";
            return false;
        }
    } else if (remoteClient) {
        if (!transferMode) {
            std::cerr << "--connect needs --legacy or --stream\n";
            return false;
        }
        if (options->dst.empty()) {
            std::cerr << "--connect needs --dst\n";
            return false;
        }
        if (options->token.empty() && options->tokenFile.empty()) {
            std::cerr << "--connect needs --token or --token-file\n";
            return false;
        }
    } else if (transferMode && (options->src.empty() || options->dst.empty())) {
        std::cerr << "--legacy/--stream need --src and --dst\n";
        return false;
    }
    if (options->pattern != "text" && options->pattern != "random" && options->pattern != "zero") {
        std::cerr << "--pattern must be text, random or zero\n";
        return false;
    }
    return true;
}

std::string stripTrailingSeparators(std::string path)
{
    while (path.size() > 1 && (path.back() == '/' || path.back() == '\\'))
        path.pop_back();
    return path;
}

// ---------------------------------------------------------------------------
// Dataset generation
// ---------------------------------------------------------------------------

std::string makeChunk(const std::string &pattern, size_t size, uint64_t seed)
{
    std::string chunk(size, '\0');
    if (pattern == "zero")
        return chunk;

    if (pattern == "random") {
        std::mt19937 rng(static_cast<uint32_t>(seed));
        for (size_t i = 0; i < size; ++i)
            chunk[i] = static_cast<char>(rng() & 0xff);
        return chunk;
    }

    static const char kText[] =
            "The quick brown fox jumps over the lazy dog. "
            "0123456789 abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ\n";
    const size_t textLen = sizeof(kText) - 1;
    for (size_t i = 0; i < size; ++i)
        chunk[i] = kText[i % textLen];
    return chunk;
}

std::string leafRelativePath(uint64_t fileIndex, uint64_t dirs, uint64_t depth)
{
    const uint64_t leaf = fileIndex % dirs;
    // branching^depth >= dirs
    uint64_t branching = 2;
    while (static_cast<uint64_t>(std::pow(branching, static_cast<double>(depth))) < dirs)
        ++branching;

    std::string relative;
    uint64_t remaining = leaf;
    for (uint64_t level = depth; level > 0; --level) {
        uint64_t divisor = 1;
        for (uint64_t i = 1; i < level; ++i)
            divisor *= branching;
        const uint64_t part = (remaining / divisor) % branching;
        char buffer[32];
        std::snprintf(buffer, sizeof(buffer), "d%03llu/", static_cast<unsigned long long>(part));
        relative += buffer;
    }
    char name[64];
    std::snprintf(name, sizeof(name), "f%08llu.bin", static_cast<unsigned long long>(fileIndex));
    relative += name;
    return relative;
}

bool writeFile(const fs::path &path, uint64_t size, const std::string &chunk)
{
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out)
        return false;

    const size_t chunkSize = chunk.size();
    uint64_t written = 0;
    while (written < size) {
        const size_t take = static_cast<size_t>(std::min<uint64_t>(chunkSize, size - written));
        out.write(chunk.data(), static_cast<std::streamsize>(take));
        if (!out)
            return false;
        written += take;
    }
    return true;
}

int generateDataset(const Options &options)
{
    const fs::path root = fs::path(options.dir);
    std::error_code ec;
    fs::create_directories(root, ec);
    if (ec) {
        std::cerr << "cannot create " << root << ": " << ec.message() << "\n";
        return 1;
    }

    const auto started = Clock::now();

    if (!options.single.empty()) {
        const uint64_t size = parseSize(options.single);
        const std::string chunk = makeChunk(options.pattern, kWriteBlock, options.seed);
        const fs::path file = root / "single.bin";
        if (!writeFile(file, size, chunk)) {
            std::cerr << "failed to write " << file << "\n";
            return 1;
        }
        const double seconds = std::chrono::duration<double>(Clock::now() - started).count();
        std::cout << "mode=generate\n"
                  << "files=1\n"
                  << "bytes=" << size << "\n"
                  << "elapsed_s=" << seconds << "\n"
                  << "throughput_mb_s=" << (seconds > 0 ? (size / 1048576.0) / seconds : 0) << "\n"
                  << "result=ok\n";
        return 0;
    }

    const std::string chunk = makeChunk(options.pattern, kWriteBlock, options.seed);
    uint64_t written = 0;
    std::set<std::string> createdDirSet;

    for (uint64_t index = 0; index < options.files; ++index) {
        const std::string relative = leafRelativePath(index, options.dirs, options.depth);
        const fs::path target = root / relative;
        const std::string parent = target.parent_path().string();
        if (createdDirSet.find(parent) == createdDirSet.end()) {
            fs::create_directories(target.parent_path(), ec);
            if (ec) {
                std::cerr << "cannot create " << parent << ": " << ec.message() << "\n";
                return 1;
            }
            createdDirSet.insert(parent);
        }
        if (!writeFile(target, options.fileSize, chunk)) {
            std::cerr << "failed to write " << target << "\n";
            return 1;
        }
        written += options.fileSize;

        if ((index + 1) % 10000 == 0)
            std::cerr << "generated " << (index + 1) << " files\r" << std::flush;
    }
    std::cerr << "\n";

    const double seconds = std::chrono::duration<double>(Clock::now() - started).count();
    std::cout << "mode=generate\n"
              << "files=" << options.files << "\n"
              << "bytes=" << written << "\n"
              << "leaf_dirs=" << options.dirs << "\n"
              << "depth=" << options.depth << "\n"
              << "created_leaf_dirs=" << createdDirSet.size() << "\n"
              << "elapsed_s=" << seconds << "\n"
              << "throughput_mb_s=" << (seconds > 0 ? (written / 1048576.0) / seconds : 0) << "\n"
              << "result=ok\n";
    return 0;
}

// ---------------------------------------------------------------------------
// Tree verification
// ---------------------------------------------------------------------------

struct TreeEntry {
    uint64_t size {0};
    uint32_t crc {0};

    bool operator==(const TreeEntry &other) const { return size == other.size && crc == other.crc; }
};

using TreeIndex = std::map<std::string, TreeEntry>;

bool indexTree(const fs::path &root, TreeIndex *index, std::string *error)
{
    std::error_code ec;
    if (!fs::exists(root, ec)) {
        *error = "missing: " + root.string();
        return false;
    }
    if (fs::is_regular_file(root, ec)) {
        // A single file source is compared by its own name.
        std::ifstream in(root, std::ios::binary);
        if (!in) {
            *error = "cannot read " + root.string();
            return false;
        }
        TreeEntry entry;
        std::vector<char> buffer(kWriteBlock);
        uint32_t crc = 0;
        while (in) {
            in.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
            const std::streamsize got = in.gcount();
            if (got <= 0)
                break;
            crc = static_cast<uint32_t>(crc32(crc, reinterpret_cast<const Bytef *>(buffer.data()),
                                             static_cast<uInt>(got)));
            entry.size += static_cast<uint64_t>(got);
        }
        entry.crc = crc;
        (*index)[root.filename().string()] = entry;
        return true;
    }

    for (fs::recursive_directory_iterator it(root, ec), end; it != end; it.increment(ec)) {
        if (ec) {
            *error = ec.message();
            return false;
        }
        if (!it->is_regular_file(ec))
            continue;
        std::ifstream in(it->path(), std::ios::binary);
        if (!in) {
            *error = "cannot read " + it->path().string();
            return false;
        }
        TreeEntry entry;
        std::vector<char> buffer(kWriteBlock);
        uint32_t crc = 0;
        while (in) {
            in.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
            const std::streamsize got = in.gcount();
            if (got <= 0)
                break;
            crc = static_cast<uint32_t>(crc32(crc, reinterpret_cast<const Bytef *>(buffer.data()),
                                             static_cast<uInt>(got)));
            entry.size += static_cast<uint64_t>(got);
        }
        entry.crc = crc;
        (*index)[fs::relative(it->path(), root, ec).generic_string()] = entry;
    }
    return true;
}

struct VerifyResult {
    bool ok {false};
    uint64_t sourceFiles {0};
    uint64_t sourceBytes {0};
    uint64_t targetFiles {0};
    uint64_t targetBytes {0};
    uint64_t missing {0};
    uint64_t extra {0};
    uint64_t mismatched {0};
    std::string firstProblem;
};

VerifyResult verifyTrees(const fs::path &source, const fs::path &target)
{
    VerifyResult result;
    TreeIndex sourceIndex;
    TreeIndex targetIndex;
    std::string error;

    if (!indexTree(source, &sourceIndex, &error)) {
        result.firstProblem = error;
        return result;
    }
    if (!indexTree(target, &targetIndex, &error)) {
        result.firstProblem = error;
        return result;
    }

    for (const auto &item : sourceIndex) {
        result.sourceFiles++;
        result.sourceBytes += item.second.size;
        const auto found = targetIndex.find(item.first);
        if (found == targetIndex.end()) {
            if (result.firstProblem.empty())
                result.firstProblem = "missing: " + item.first;
            result.missing++;
        } else if (!(found->second == item.second)) {
            if (result.firstProblem.empty())
                result.firstProblem = "mismatch: " + item.first;
            result.mismatched++;
        }
    }
    for (const auto &item : targetIndex) {
        result.targetFiles++;
        result.targetBytes += item.second.size;
        if (sourceIndex.find(item.first) == sourceIndex.end()) {
            if (result.firstProblem.empty())
                result.firstProblem = "extra: " + item.first;
            result.extra++;
        }
    }

    result.ok = result.missing == 0 && result.extra == 0 && result.mismatched == 0;
    return result;
}

// ---------------------------------------------------------------------------
// Process metrics
// ---------------------------------------------------------------------------

struct CpuTimes {
    double user {0};
    double system {0};
};

CpuTimes cpuTimes()
{
    CpuTimes times;
#ifndef _WIN32
    struct rusage usage = {};
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        times.user = usage.ru_utime.tv_sec + usage.ru_utime.tv_usec / 1e6;
        times.system = usage.ru_stime.tv_sec + usage.ru_stime.tv_usec / 1e6;
    }
#endif
    return times;
}

uint64_t peakRssKb()
{
#ifndef _WIN32
    std::ifstream status("/proc/self/status");
    std::string line;
    while (std::getline(status, line)) {
        if (line.rfind("VmHWM:", 0) == 0) {
            const auto colon = line.find(':');
            if (colon != std::string::npos) {
                const std::string value = line.substr(colon + 1);
                return std::strtoull(value.c_str(), nullptr, 10);
            }
        }
    }
#endif
    return 0;
}

// ---------------------------------------------------------------------------
// Legacy transfer run
// ---------------------------------------------------------------------------

struct Collector : ProgressCallInterface {
    mutable std::mutex mutex;
    std::condition_variable cv;

    std::atomic<uint64_t> progressBytes {0};
    std::atomic<uint64_t> fileBegin {0};
    std::atomic<uint64_t> fileEnd {0};
    std::atomic<uint64_t> indexBegin {0};
    std::atomic<uint64_t> failures {0};
    std::atomic<uint64_t> maxStallUs {0};

    bool finished {false};
    bool aborted {false};
    std::string lastError;
    Clock::time_point lastEvent {Clock::now()};

    // Per-file timing: transfer = begin..end, cycle = begin..next begin.
    std::vector<double> fileDurationsMs;
    std::vector<double> fileCyclesMs;
    Clock::time_point currentFileBegin {};
    Clock::time_point previousFileBegin {};

    void touch()
    {
        const auto now = Clock::now();
        const uint64_t gap = static_cast<uint64_t>(
                std::chrono::duration_cast<std::chrono::microseconds>(now - lastEvent).count());
        lastEvent = now;
        uint64_t current = maxStallUs.load();
        while (gap > current && !maxStallUs.compare_exchange_weak(current, gap)) {
        }
    }

    bool onProgress(uint64_t size) override
    {
        progressBytes.fetch_add(size);
        std::lock_guard<std::mutex> lock(mutex);
        touch();
        return false;
    }

    void onWebChanged(int state, std::string msg = "", uint64_t size = 0) override
    {
        (void)size;
        if (state < 1) {
            failures.fetch_add(1);
            {
                std::lock_guard<std::mutex> lock(mutex);
                lastError = msg;
                if (state == WEB_DISCONNECTED || state == WEB_IO_ERROR)
                    aborted = true;
            }
            cv.notify_all();
            return;
        }

        switch (state) {
        case WEB_FILE_BEGIN: {
            fileBegin.fetch_add(1);
            const auto now = Clock::now();
            std::lock_guard<std::mutex> lock(mutex);
            if (previousFileBegin.time_since_epoch().count() != 0) {
                fileCyclesMs.push_back(
                        std::chrono::duration<double, std::milli>(now - previousFileBegin).count());
            }
            previousFileBegin = now;
            currentFileBegin = now;
            break;
        }
        case WEB_FILE_END: {
            fileEnd.fetch_add(1);
            const auto now = Clock::now();
            std::lock_guard<std::mutex> lock(mutex);
            if (currentFileBegin.time_since_epoch().count() != 0) {
                fileDurationsMs.push_back(
                        std::chrono::duration<double, std::milli>(now - currentFileBegin).count());
                currentFileBegin = Clock::time_point {};
            }
            break;
        }
        case WEB_INDEX_BEGIN:
            indexBegin.fetch_add(1);
            break;
        case WEB_TRANS_FINISH: {
            std::lock_guard<std::mutex> lock(mutex);
            finished = true;
            break;
        }
        default:
            break;
        }

        {
            std::lock_guard<std::mutex> lock(mutex);
            touch();
        }
        cv.notify_all();
    }

    bool waitFor(int timeoutSeconds, bool *timedOut)
    {
        std::unique_lock<std::mutex> lock(mutex);
        const auto deadline = Clock::now() + std::chrono::seconds(timeoutSeconds);
        *timedOut = false;
        cv.wait_until(lock, deadline, [this] { return finished || aborted; });
        if (!finished && !aborted) {
            *timedOut = true;
            return false;
        }
        return finished;
    }
};

std::shared_ptr<NetUtil::Asio::SSLContext> makeServerContext(const std::string &profile)
{
    SslCertConf::ins()->generateCertificate(profile);
    const std::string certFile = SslCertConf::ins()->getCertificatePath();
    auto context = std::make_shared<NetUtil::Asio::SSLContext>(asio::ssl::context::tlsv13);
    context->use_certificate_file(certFile, asio::ssl::context::pem);
    context->use_rsa_private_key_file(certFile, asio::ssl::context::pem);
    return context;
}

std::shared_ptr<NetUtil::Asio::SSLContext> makeClientContext(const std::string &profile)
{
    SslCertConf::ins()->generateCertificate(profile);
    const std::string certFile = SslCertConf::ins()->getCertificatePath();
    auto context = std::make_shared<NetUtil::Asio::SSLContext>(asio::ssl::context::tlsv13);
    context->use_certificate_file(certFile, asio::ssl::context::pem);
    return context;
}

struct TimingSummary {
    double minMs {0};
    double avgMs {0};
    double maxMs {0};
};

TimingSummary summarize(const std::vector<double> &values)
{
    TimingSummary summary;
    if (values.empty())
        return summary;
    double total = 0;
    summary.minMs = values.front();
    summary.maxMs = values.front();
    for (double value : values) {
        total += value;
        summary.minMs = std::min(summary.minMs, value);
        summary.maxMs = std::max(summary.maxMs, value);
    }
    summary.avgMs = total / static_cast<double>(values.size());
    return summary;
}

struct RunMetrics {
    bool ok {false};
    bool timedOut {false};
    double elapsed {0};
    CpuTimes cpu;
    uint64_t rssKb {0};
    uint64_t progressBytes {0};
    uint64_t fileBegin {0};
    uint64_t fileEnd {0};
    uint64_t indexBegin {0};
    uint64_t failures {0};
    uint64_t maxStallMs {0};
    uint64_t infoRequests {0};
    uint64_t downloadRequests {0};
    uint64_t headerRequests {0};
    uint64_t connections {0};
    TimingSummary fileDuration;
    TimingSummary fileCycle;
    std::string error;
};

void setEnvValue(const char *name, const char *value)
{
#ifndef _WIN32
    if (value)
        setenv(name, value, 1);
    else
        setenv(name, "", 1);
#else
    _putenv_s(name, value ? value : "");
#endif
}

bool writeTextFile(const std::string &path, const std::string &text)
{
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out)
        return false;
    out << text;
    return static_cast<bool>(out);
}

std::string readTextFile(const std::string &path)
{
    std::ifstream in(path, std::ios::binary);
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

std::string trimCopy(std::string text)
{
    auto notSpace = [](unsigned char ch) { return !std::isspace(ch); };
    text.erase(text.begin(), std::find_if(text.begin(), text.end(), notSpace));
    text.erase(std::find_if(text.rbegin(), text.rend(), notSpace).base(), text.end());
    return text;
}

void printMetrics(const Options &options, int run, const std::string &runDst,
                  const std::string &webName, const RunMetrics &metrics, int *exitCode)
{
    std::cout << "mode=" << (options.stream ? "stream" : "legacy") << "\n"
              << "run=" << run << "\n"
              << "no_delay=" << (options.noDelay ? 1 : 0) << "\n"
              << "src=" << options.src << "\n"
              << "dst=" << runDst << "\n"
              << "elapsed_s=" << metrics.elapsed << "\n"
              << "cpu_user_s=" << metrics.cpu.user << "\n"
              << "cpu_sys_s=" << metrics.cpu.system << "\n"
              << "rss_peak_kb=" << metrics.rssKb << "\n"
              << "progress_bytes=" << metrics.progressBytes << "\n"
              << "file_begin=" << metrics.fileBegin << "\n"
              << "file_end=" << metrics.fileEnd << "\n"
              << "index_begin=" << metrics.indexBegin << "\n"
              << "failures=" << metrics.failures << "\n"
              << "max_stall_ms=" << metrics.maxStallMs << "\n"
              << "http_header_requests=" << metrics.headerRequests << "\n"
              << "http_info_requests=" << metrics.infoRequests << "\n"
              << "http_download_requests=" << metrics.downloadRequests << "\n"
              << "http_connections=" << metrics.connections << "\n"
              << "file_ms_min=" << metrics.fileDuration.minMs << "\n"
              << "file_ms_avg=" << metrics.fileDuration.avgMs << "\n"
              << "file_ms_max=" << metrics.fileDuration.maxMs << "\n"
              << "file_cycle_ms_avg=" << metrics.fileCycle.avgMs << "\n"
              << "file_cycle_ms_max=" << metrics.fileCycle.maxMs << "\n";

    if (!metrics.error.empty())
        std::cout << "error=" << metrics.error << "\n";

    if (options.verify && !options.src.empty() && metrics.error.empty()) {
        // FileClient saves the selection under <saveDir>/<root name>.
        const fs::path targetRoot = fs::path(runDst) / fs::path(webName);
        const VerifyResult verify = verifyTrees(options.src, targetRoot);
        std::cout << "files_src=" << verify.sourceFiles << "\n"
                  << "bytes_src=" << verify.sourceBytes << "\n"
                  << "files_dst=" << verify.targetFiles << "\n"
                  << "bytes_dst=" << verify.targetBytes << "\n"
                  << "missing=" << verify.missing << "\n"
                  << "extra=" << verify.extra << "\n"
                  << "mismatched=" << verify.mismatched << "\n"
                  << "verified=" << (verify.ok ? "ok" : "mismatch") << "\n";
        if (!verify.ok && !verify.firstProblem.empty())
            std::cout << "verify_problem=" << verify.firstProblem << "\n";
        if (!verify.ok)
            *exitCode = 3;
    } else {
        std::cout << "verified=skipped\n";
    }

    if (metrics.timedOut) {
        std::cout << "result=timeout\n";
        *exitCode = 2;
    } else if (!metrics.ok) {
        std::cout << "result=error\n";
        *exitCode = 1;
    } else {
        std::cout << "result=ok\n";
    }
    std::cout.flush();
}

RunMetrics runLegacyOnce(const Options &options, const std::string &runDst, int port)
{
    RunMetrics metrics;

    // The client defaults to stream-first. The A/B run must pin the path so a
    // "--legacy" measurement cannot silently switch modes.
#ifndef _WIN32
    setenv("DDE_COOPERATION_HTTPWEB_STREAM", options.stream ? "1" : "0", 1);
    // FileClient enables TCP_NODELAY in production; pin it here so the
    // pre-D11 baseline (both ends with Nagle) stays reproducible.
    setenv("DDE_COOPERATION_HTTPWEB_CLIENT_NODELAY", options.noDelay ? "1" : "0", 1);
#else
    _putenv_s("DDE_COOPERATION_HTTPWEB_STREAM", options.stream ? "1" : "0");
    _putenv_s("DDE_COOPERATION_HTTPWEB_CLIENT_NODELAY", options.noDelay ? "1" : "0");
#endif

    fs::path source = fs::path(options.src);
    std::error_code ec;
    if (!fs::exists(source, ec)) {
        metrics.error = "source does not exist: " + options.src;
        return metrics;
    }
    const std::string webName = source.filename().string();

    // The destination must be empty: createNextAvailableName() would otherwise
    // add a "(1)" suffix and the tree comparison would trivially fail.
    if (fs::exists(runDst, ec)) {
        if (!fs::is_directory(runDst, ec) || !fs::is_empty(runDst, ec)) {
            metrics.error = "destination is not empty: " + runDst;
            return metrics;
        }
    } else {
        fs::create_directories(runDst, ec);
        if (ec) {
            metrics.error = "cannot create " + runDst + ": " + ec.message();
            return metrics;
        }
    }

    WebBinder::GetInstance().clear();
    TokenCache::GetInstance().clearTokens();

    // Keep the generated certificate out of the compared tree.
    const std::string profile =
            (fs::temp_directory_path() / "dde-cooperation-bench-certs").string();
    // Separate IO services for server and client: they model two peers, not a
    // single process sharing one reactor (which would hide backpressure).
    auto serverService = std::make_shared<NetUtil::Asio::Service>();
    serverService->Start();
    auto clientService = std::make_shared<NetUtil::Asio::Service>();
    clientService->Start();

    auto serverContext = makeServerContext(profile);
    auto clientContext = makeClientContext(profile);
    if (!serverContext || !clientContext) {
        metrics.error = "cannot prepare TLS contexts";
        serverService->Stop();
        clientService->Stop();
        return metrics;
    }

    auto server = std::make_shared<FileServer>(serverService, serverContext, "127.0.0.1", port);
    if (!server->start()) {
        metrics.error = "cannot start server on port " + std::to_string(port);
        serverService->Stop();
        clientService->Stop();
        return metrics;
    }

    // Production now enables TCP_NODELAY in start(); the benchmark must be
    // able to reproduce the old baseline, so it explicitly overrides the
    // option after start() (before any client connects).
    server->SetupNoDelay(options.noDelay);

    const auto shutdown = [&]() {
        if (server)
            server->stop();
        if (serverService)
            serverService->Stop();
        if (clientService)
            clientService->Stop();
    };

    try {
        if (server->webBind(webName, stripTrailingSeparators(source.string())) != 0) {
            metrics.error = "webBind failed for " + webName;
            shutdown();
            return metrics;
        }
    } catch (const std::exception &e) {
        metrics.error = std::string("webBind failed: ") + e.what();
        shutdown();
        return metrics;
    }

    picojson::array names;
    names.push_back(picojson::value(webName));
    const std::string token = server->genToken(picojson::value(names).serialize());

    auto collector = std::make_shared<Collector>();
    collector->lastEvent = Clock::now();

    const CpuTimes cpuBefore = cpuTimes();
    const auto started = Clock::now();

    {
        FileClient client(clientService, clientContext, "127.0.0.1", port);
        client.setCallback(collector);
        client.setConfig(token, runDst);

        std::vector<std::string> webs;
        try {
            webs = client.parseWeb(token);
        } catch (const std::exception &e) {
            metrics.error = std::string("parseWeb failed: ") + e.what();
        }
        if (metrics.error.empty() && webs.empty())
            metrics.error = "token carries no web names";

        if (metrics.error.empty()) {
            client.startFileDownload(webs);

            bool timedOut = false;
            collector->waitFor(options.timeoutSeconds, &timedOut);
            if (timedOut) {
                metrics.timedOut = true;
                client.stop();
            }
            // Let the download thread observe the stop flag; the destructor
            // joins it and must not hang on a live request.
            for (int i = 0; i < 3000 && !collector->finished && !collector->aborted; ++i)
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        const FileClient::WebRequestCounters counters = client.requestCounters();
        metrics.infoRequests = counters.infoRequests;
        metrics.downloadRequests = counters.downloadRequests;
        metrics.headerRequests = counters.headerRequests;
        metrics.connections = counters.connections;

        client.setCallback(nullptr);
    }

    metrics.elapsed = std::chrono::duration<double>(Clock::now() - started).count();
    const CpuTimes cpuAfter = cpuTimes();
    metrics.cpu.user = cpuAfter.user - cpuBefore.user;
    metrics.cpu.system = cpuAfter.system - cpuBefore.system;
    metrics.rssKb = peakRssKb();
    metrics.progressBytes = collector->progressBytes.load();
    metrics.fileBegin = collector->fileBegin.load();
    metrics.fileEnd = collector->fileEnd.load();
    metrics.indexBegin = collector->indexBegin.load();
    metrics.failures = collector->failures.load();
    metrics.maxStallMs = collector->maxStallUs.load() / 1000;
    {
        std::lock_guard<std::mutex> lock(collector->mutex);
        if (metrics.error.empty())
            metrics.error = collector->lastError;
        metrics.ok = collector->finished && !collector->aborted && metrics.error.empty();
        metrics.fileDuration = summarize(collector->fileDurationsMs);
        metrics.fileCycle = summarize(collector->fileCyclesMs);
    }

    shutdown();
    return metrics;
}

int runLegacy(const Options &options)
{
    int exitCode = 0;
    for (int run = 0; run < options.repeat; ++run) {
        const std::string runDst =
                options.repeat == 1 ? options.dst
                                    : (fs::path(options.dst) / ("run" + std::to_string(run))).string();

        const RunMetrics metrics = runLegacyOnce(options, runDst, options.port + run);
        const std::string webName = fs::path(stripTrailingSeparators(options.src)).filename().string();
        printMetrics(options, run, runDst, webName, metrics, &exitCode);
    }
    return exitCode;
}

int runServer(const Options &options)
{
    // A legacy server must behave like the old release: no stream endpoint and
    // no dtp claim. A stream server keeps the production defaults.
    setEnvValue("DDE_COOPERATION_HTTPWEB_SERVER_STREAM", options.stream ? nullptr : "0");
    setEnvValue("DDE_COOPERATION_HTTPWEB_TOKEN_DTP", options.stream ? nullptr : "0");

    const fs::path source = fs::path(stripTrailingSeparators(options.src));
    std::error_code ec;
    if (!fs::exists(source, ec)) {
        std::cout << "error=source does not exist: " << options.src << "\n"
                  << "result=error\n";
        return 1;
    }
    const std::string webName = source.filename().string();

    WebBinder::GetInstance().clear();
    TokenCache::GetInstance().clearTokens();

    const std::string profile =
            (fs::temp_directory_path() / "dde-cooperation-remote-bench-certs").string();
    auto service = std::make_shared<NetUtil::Asio::Service>();
    service->Start();
    auto context = makeServerContext(profile);
    auto server = std::make_shared<FileServer>(service, context, options.bindAddress, options.port);
    if (!server->start()) {
        std::cout << "error=cannot start server on " << options.bindAddress << ":"
                  << options.port << "\n"
                  << "result=error\n";
        service->Stop();
        return 1;
    }

    server->SetupNoDelay(options.noDelay);
    if (server->webBind(webName, source.string()) != 0) {
        std::cout << "error=webBind failed for " << webName << "\n"
                  << "result=error\n";
        server->stop();
        service->Stop();
        return 1;
    }

    picojson::array names;
    names.push_back(picojson::value(webName));
    const std::string token = server->genToken(picojson::value(names).serialize());
    if (!options.tokenFile.empty() && !writeTextFile(options.tokenFile, token)) {
        std::cout << "error=cannot write token file: " << options.tokenFile << "\n"
                  << "result=error\n";
        server->stop();
        service->Stop();
        return 1;
    }

    std::cout << "mode=" << (options.stream ? "stream" : "legacy") << "\n"
              << "role=server\n"
              << "bind=" << options.bindAddress << "\n"
              << "port=" << options.port << "\n"
              << "web=" << webName << "\n"
              << "token=" << token << "\n";
    if (!options.tokenFile.empty())
        std::cout << "token_file=" << options.tokenFile << "\n";
    std::cout << "ready=1\n";
    std::cout.flush();

    const auto deadline = Clock::now() + std::chrono::seconds(std::max(1, options.timeoutSeconds));
    while (Clock::now() < deadline)
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

    server->stop();
    service->Stop();
    std::cout << "result=ok\n";
    return 0;
}

int runClient(const Options &options)
{
    setEnvValue("DDE_COOPERATION_HTTPWEB_STREAM", options.stream ? "1" : "0");
    setEnvValue("DDE_COOPERATION_HTTPWEB_CLIENT_NODELAY", options.noDelay ? "1" : "0");

    std::string token = options.token;
    if (token.empty())
        token = readTextFile(options.tokenFile);
    token = trimCopy(token);
    if (token.empty()) {
        std::cout << "error=empty token\n"
                  << "result=error\n";
        return 1;
    }

    int exitCode = 0;
    for (int run = 0; run < options.repeat; ++run) {
        const std::string runDst =
                options.repeat == 1 ? options.dst
                                    : (fs::path(options.dst) / ("run" + std::to_string(run))).string();

        std::error_code ec;
        if (fs::exists(runDst, ec)) {
            if (!fs::is_directory(runDst, ec) || !fs::is_empty(runDst, ec)) {
                std::cout << "error=destination is not empty: " << runDst << "\n"
                          << "result=error\n";
                return 1;
            }
        } else {
            fs::create_directories(runDst, ec);
            if (ec) {
                std::cout << "error=cannot create " << runDst << ": " << ec.message() << "\n"
                          << "result=error\n";
                return 1;
            }
        }

        const std::string profile =
                (fs::temp_directory_path() / "dde-cooperation-remote-bench-certs").string();
        auto service = std::make_shared<NetUtil::Asio::Service>();
        service->Start();
        auto context = makeClientContext(profile);
        auto collector = std::make_shared<Collector>();
        collector->lastEvent = Clock::now();

        const CpuTimes cpuBefore = cpuTimes();
        const auto started = Clock::now();
        std::string webName;
        RunMetrics metrics;

        {
            FileClient client(service, context, options.connectHost, options.port);
            client.setCallback(collector);
            client.setConfig(token, runDst);

            std::vector<std::string> webs;
            try {
                webs = client.parseWeb(token);
            } catch (const std::exception &e) {
                metrics.error = std::string("parseWeb failed: ") + e.what();
            }
            if (metrics.error.empty() && webs.empty())
                metrics.error = "token carries no web names";

            if (metrics.error.empty()) {
                webName = webs.front();
                client.startFileDownload(webs);

                bool timedOut = false;
                collector->waitFor(options.timeoutSeconds, &timedOut);
                if (timedOut) {
                    metrics.timedOut = true;
                    client.stop();
                }
                for (int i = 0; i < 3000 && !collector->finished && !collector->aborted; ++i)
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }

            const FileClient::WebRequestCounters counters = client.requestCounters();
            metrics.infoRequests = counters.infoRequests;
            metrics.downloadRequests = counters.downloadRequests;
            metrics.headerRequests = counters.headerRequests;
            metrics.connections = counters.connections;

            client.setCallback(nullptr);
        }

        metrics.elapsed = std::chrono::duration<double>(Clock::now() - started).count();
        const CpuTimes cpuAfter = cpuTimes();
        metrics.cpu.user = cpuAfter.user - cpuBefore.user;
        metrics.cpu.system = cpuAfter.system - cpuBefore.system;
        metrics.rssKb = peakRssKb();
        metrics.progressBytes = collector->progressBytes.load();
        metrics.fileBegin = collector->fileBegin.load();
        metrics.fileEnd = collector->fileEnd.load();
        metrics.indexBegin = collector->indexBegin.load();
        metrics.failures = collector->failures.load();
        metrics.maxStallMs = collector->maxStallUs.load() / 1000;
        {
            std::lock_guard<std::mutex> lock(collector->mutex);
            if (metrics.error.empty())
                metrics.error = collector->lastError;
            metrics.ok = collector->finished && !collector->aborted && metrics.error.empty();
            metrics.fileDuration = summarize(collector->fileDurationsMs);
            metrics.fileCycle = summarize(collector->fileCyclesMs);
        }

        service->Stop();
        printMetrics(options, run, runDst, webName, metrics, &exitCode);
        if (exitCode != 0)
            break;
    }
    return exitCode;
}

} // namespace

int main(int argc, char **argv)
{
    Options options;
    try {
        if (!parseArgs(argc, argv, &options)) {
            usage();
            return 1;
        }
    } catch (const std::exception &e) {
        std::cerr << e.what() << "\n";
        usage();
        return 1;
    }

    if (!options.generate && !options.legacy && !options.stream) {
        usage();
        return 1;
    }

    try {
        if (options.generate)
            return generateDataset(options);
        if (options.serve)
            return runServer(options);
        if (!options.connectHost.empty())
            return runClient(options);
        return runLegacy(options);
    } catch (const std::exception &e) {
        std::cerr << "benchmark failed: " << e.what() << "\n";
        return 1;
    }
}
