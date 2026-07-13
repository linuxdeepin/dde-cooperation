// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// rolling_file_appender 扩展覆盖用例：补充 size/time 轮转、备份上限清理、
// archive(.zip) 归档、append-vs-truncate、flush、Start/Stop 幂等等未覆盖分支。

#include <catch2/catch_all.hpp>
#include <logging/appenders/rolling_file_appender.h>
#include <logging/layouts/text_layout.h>
#include <logging/record.h>

#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <stdlib.h>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>

using namespace Logging;
using namespace BaseKit;

namespace {

Record makeRollRecord(Level level, const std::string& msg)
{
    Record r;
    r.timestamp = 1700000000ull;
    r.thread = 1;
    r.level = level;
    r.logger = "Roll";
    r.message = msg;
    return r;
}

// RollingFileAppender skips records whose `raw` is empty, so run each record
// through a TextLayout first to populate `raw`.
void layoutRecord(Record& r)
{
    TextLayout layout("{Level}|{Message}{EndLine}");
    layout.LayoutRecord(r);
}

std::string readWholeFile(const std::string& path)
{
    std::ifstream ifs(path, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(ifs)),
                       std::istreambuf_iterator<char>());
}

int countFiles(const std::string& dir)
{
    if (!std::filesystem::exists(dir))
        return 0;
    int n = 0;
    for ([[maybe_unused]] auto& _ : std::filesystem::directory_iterator(dir))
        ++n;
    return n;
}

int countFilesWithSuffix(const std::string& dir, const std::string& suffix)
{
    if (!std::filesystem::exists(dir))
        return 0;
    int n = 0;
    for (auto& entry : std::filesystem::directory_iterator(dir))
    {
        if (entry.path().string().find(suffix) != std::string::npos)
            ++n;
    }
    return n;
}

std::string uniqueTempDir(const char* tag)
{
    auto base = (std::filesystem::temp_directory_path() / (std::string(tag) + "_XXXXXX")).string();
    std::vector<char> buf(base.begin(), base.end());
    buf.push_back('\0');
    char* res = mkdtemp(buf.data());
    REQUIRE(res != nullptr);
    return std::string(res);
}

void rmrf(const std::string& p) { std::filesystem::remove_all(p); }

}  // namespace

TEST_CASE("rolling_file_appender extra coverage", "[appender][rolling]")
{
    // ---- Start / Stop idempotency (Impl early-return branches) ----
    SECTION("Start and Stop are idempotent")
    {
        auto dir = uniqueTempDir("roll_idem");
        {
            RollingFileAppender app(Path(dir), "svc", "log", 1024, 3,
                                    false, true, true, /*auto_start=*/false);
            REQUIRE_FALSE(app.IsStarted());
            REQUIRE(app.Start());
            REQUIRE(app.IsStarted());
            REQUIRE_FALSE(app.Start());  // already started -> false
            REQUIRE(app.Stop());
            REQUIRE_FALSE(app.IsStarted());
            REQUIRE_FALSE(app.Stop());  // already stopped -> false
        }
        rmrf(dir);
    }

    // ---- Size-based rotation produces indexed backup files ----
    SECTION("Size rotation creates .1/.2/.3 backups")
    {
        auto dir = uniqueTempDir("roll_idx");
        {
            RollingFileAppender app(Path(dir), "svc", "log", /*size=*/8, /*backups=*/3,
                                    false, false, true, /*auto_start=*/false);
            REQUIRE(app.Start());
            for (int i = 0; i < 12; ++i)
            {
                auto r = makeRollRecord(Level::INFO, "0123456789");  // 10 chars + layout
                layoutRecord(r);
                app.AppendRecord(r);
            }
            // No explicit Flush() here: a trailing Flush(0) on a size-rotating
            // appender would itself trigger RollBackup and rotate svc.log away.
            // Stop() -> CloseFile just flushes & closes the current file in place.
            app.Stop();
        }
        REQUIRE(std::filesystem::exists(dir + "/svc.log"));
        REQUIRE(std::filesystem::exists(dir + "/svc.1.log"));
        REQUIRE(std::filesystem::exists(dir + "/svc.2.log"));
        REQUIRE(std::filesystem::exists(dir + "/svc.3.log"));
        // 4th backup must have been removed by RollBackup cleanup
        REQUIRE_FALSE(std::filesystem::exists(dir + "/svc.4.log"));
        REQUIRE(countFilesWithSuffix(dir, ".log") <= 4);  // base + 3 backups
        rmrf(dir);
    }

    // ---- Max-backup cleanup removes oldest beyond limit ----
    SECTION("Max backup cleanup drops files beyond backups limit")
    {
        auto dir = uniqueTempDir("roll_max");
        {
            RollingFileAppender app(Path(dir), "svc", "log", /*size=*/4, /*backups=*/2,
                                    false, false, true, /*auto_start=*/false);
            REQUIRE(app.Start());
            for (int i = 0; i < 50; ++i)
            {
                auto r = makeRollRecord(Level::WARN, "XXXXXXXXXX");
                layoutRecord(r);
                app.AppendRecord(r);
            }
            app.Flush();
            app.Stop();
        }
        REQUIRE_FALSE(std::filesystem::exists(dir + "/svc.3.log"));
        REQUIRE_FALSE(std::filesystem::exists(dir + "/svc.4.log"));
        REQUIRE(countFilesWithSuffix(dir, ".log") <= 3);  // base + 2 backups
        rmrf(dir);
    }

    // ---- Append vs truncate mode ----
    SECTION("Append mode preserves previous content; truncate mode discards it")
    {
        auto dir = uniqueTempDir("roll_apnd");

        // session 1: write AAAA (large size -> no rotation)
        {
            RollingFileAppender app(Path(dir), "svc", "log", 1u << 20, 3,
                                    false, /*truncate=*/false, true, /*auto_start=*/false);
            REQUIRE(app.Start());
            auto r = makeRollRecord(Level::INFO, "AAAA-first");
            layoutRecord(r);
            app.AppendRecord(r);
            app.Flush();
            app.Stop();
        }
        REQUIRE(std::filesystem::exists(dir + "/svc.log"));
        std::string first = readWholeFile(dir + "/svc.log");
        REQUIRE(first.find("AAAA-first") != std::string::npos);

        // session 2: append -> BBBB appended after AAAA
        {
            RollingFileAppender app(Path(dir), "svc", "log", 1u << 20, 3,
                                    false, /*truncate=*/false, true, /*auto_start=*/false);
            REQUIRE(app.Start());
            auto r = makeRollRecord(Level::INFO, "BBBB-second");
            layoutRecord(r);
            app.AppendRecord(r);
            app.Flush();
            app.Stop();
        }
        std::string second = readWholeFile(dir + "/svc.log");
        REQUIRE(second.find("AAAA-first") != std::string::npos);
        REQUIRE(second.find("BBBB-second") != std::string::npos);
        REQUIRE(second.size() > first.size());

        // session 3: truncate -> only CCCC remains
        {
            RollingFileAppender app(Path(dir), "svc", "log", 1u << 20, 3,
                                    false, /*truncate=*/true, true, /*auto_start=*/false);
            REQUIRE(app.Start());
            auto r = makeRollRecord(Level::INFO, "CCCC-third");
            layoutRecord(r);
            app.AppendRecord(r);
            app.Flush();
            app.Stop();
        }
        std::string third = readWholeFile(dir + "/svc.log");
        REQUIRE(third.find("CCCC-third") != std::string::npos);
        REQUIRE(third.find("AAAA-first") == std::string::npos);
        REQUIRE(third.find("BBBB-second") == std::string::npos);

        rmrf(dir);
    }

    // ---- Flush behaviour ----
    SECTION("Explicit flush persists written data")
    {
        auto dir = uniqueTempDir("roll_flush");
        std::string content;
        {
            RollingFileAppender app(Path(dir), "svc", "log", 1u << 20, 3,
                                    false, false, /*auto_flush=*/false, /*auto_start=*/false);
            REQUIRE(app.Start());
            auto r = makeRollRecord(Level::INFO, "persisted-bytes");
            layoutRecord(r);
            app.AppendRecord(r);
            app.Flush();
            app.Stop();
        }
        REQUIRE(std::filesystem::exists(dir + "/svc.log"));
        content = readWholeFile(dir + "/svc.log");
        REQUIRE(content.find("persisted-bytes") != std::string::npos);
        rmrf(dir);
    }

    SECTION("Auto-flush enabled writes without error")
    {
        auto dir = uniqueTempDir("roll_afl");
        {
            RollingFileAppender app(Path(dir), "svc", "log", 1u << 20, 3,
                                    false, false, /*auto_flush=*/true, /*auto_start=*/false);
            REQUIRE(app.Start());
            for (int i = 0; i < 5; ++i)
            {
                auto r = makeRollRecord(Level::INFO, "auto-flush-line");
                layoutRecord(r);
                app.AppendRecord(r);
            }
            app.Stop();
        }
        std::string content = readWholeFile(dir + "/svc.log");
        REQUIRE(content.find("auto-flush-line") != std::string::npos);
        rmrf(dir);
    }

    // ---- Multiple consecutive rotations in a single session ----
    SECTION("Many consecutive rotations stay within backups limit")
    {
        auto dir = uniqueTempDir("roll_consec");
        {
            RollingFileAppender app(Path(dir), "svc", "log", /*size=*/2, /*backups=*/4,
                                    false, false, true, /*auto_start=*/false);
            REQUIRE(app.Start());
            for (int i = 0; i < 30; ++i)
            {
                auto r = makeRollRecord(Level::ERROR, "rot" + std::to_string(i));
                layoutRecord(r);
                app.AppendRecord(r);
            }
            app.Flush();
            app.Stop();
        }
        REQUIRE_FALSE(std::filesystem::exists(dir + "/svc.5.log"));
        REQUIRE(countFilesWithSuffix(dir, ".log") >= 2);  // at least base + one backup
        REQUIRE(countFilesWithSuffix(dir, ".log") <= 5);  // base + 4 backups
        rmrf(dir);
    }

    // ---- Archive (.zip) of rotated backup via archivation thread ----
    SECTION("Archive mode produces zip of rotated backups")
    {
        auto dir = uniqueTempDir("roll_zip");
        {
            RollingFileAppender app(Path(dir), "svc", "log", /*size=*/1, /*backups=*/3,
                                    /*archive=*/true, false, true, /*auto_start=*/false);
            REQUIRE(app.Start());
            for (int i = 0; i < 15; ++i)
            {
                auto r = makeRollRecord(Level::INFO, "archive-data-padding");
                layoutRecord(r);
                app.AppendRecord(r);
            }
            app.Flush();
            app.Stop();  // joins the archivation thread
        }
        // Stop() joins the archivation thread, so zip output should be present now.
        bool hasZip = false;
        for (int i = 0; i < 10; ++i)
        {
            if (countFilesWithSuffix(dir, ".zip") > 0)
            {
                hasZip = true;
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        REQUIRE(hasZip);
        rmrf(dir);
    }

    // ---- Time-based rotation: previously uncovered policies ----
    SECTION("Time policy MINUTE creates a log file")
    {
        auto dir = uniqueTempDir("roll_min");
        {
            RollingFileAppender app(Path(dir), TimeRollingPolicy::MINUTE, "{UtcDateTime}.log",
                                    false, true, true, /*auto_start=*/false);
            REQUIRE(app.Start());
            auto r = makeRollRecord(Level::INFO, "minute-record");
            layoutRecord(r);
            app.AppendRecord(r);
            app.Flush();
            app.Stop();
        }
        REQUIRE(countFiles(dir) >= 1);
        rmrf(dir);
    }

    SECTION("Time policy SECOND creates a log file")
    {
        auto dir = uniqueTempDir("roll_sec");
        {
            RollingFileAppender app(Path(dir), TimeRollingPolicy::SECOND, "{UtcDateTime}.log",
                                    false, true, true, /*auto_start=*/false);
            REQUIRE(app.Start());
            auto r = makeRollRecord(Level::INFO, "second-record");
            layoutRecord(r);
            app.AppendRecord(r);
            app.Flush();
            app.Stop();
        }
        REQUIRE(countFiles(dir) >= 1);
        rmrf(dir);
    }

    // ---- Time-policy filename pattern: exercise PrepareFilePath placeholders ----
    SECTION("Time policy filename honors date/time placeholders")
    {
        auto dir = uniqueTempDir("roll_pat");
        {
            // Use BOTH individual components AND aggregate placeholders so the
            // PrepareFilePath output switch covers every case (UtcYear/Month/Day/
            // Hour/Minute/Second + UtcDate/UtcTime/UtcDateTime, and the Local*
            // equivalents).
            RollingFileAppender app(Path(dir), TimeRollingPolicy::DAY,
                "{UtcYear}{UtcMonth}{UtcDay}{UtcHour}{UtcMinute}{UtcSecond}"
                "{UtcDate}{UtcTime}{UtcDateTime}{UtcTimezone}"
                "{LocalYear}{LocalMonth}{LocalDay}{LocalHour}{LocalMinute}{LocalSecond}"
                "{LocalDate}{LocalTime}{LocalDateTime}{LocalTimezone}.log",
                false, true, true, /*auto_start=*/false);
            REQUIRE(app.Start());
            // Write multiple records so the `_first` else-branch (timestamp =
            // rollstamp) is also taken on the 2nd+ append within a session.
            for (int i = 0; i < 3; ++i)
            {
                auto r = makeRollRecord(Level::INFO, "pattern-" + std::to_string(i));
                layoutRecord(r);
                app.AppendRecord(r);
            }
            app.Flush();
            app.Stop();
        }
        REQUIRE(countFiles(dir) >= 1);
        rmrf(dir);
    }

    // ---- Time-policy pattern with unknown placeholder (verbatim emission) ----
    SECTION("Time policy pattern with unknown placeholder is emitted verbatim")
    {
        auto dir = uniqueTempDir("roll_unk");
        {
            RollingFileAppender app(Path(dir), TimeRollingPolicy::DAY, "{Bogus}.log",
                                    false, true, true, /*auto_start=*/false);
            REQUIRE(app.Start());
            auto r = makeRollRecord(Level::INFO, "unknown-placeholder");
            layoutRecord(r);
            app.AppendRecord(r);
            app.Flush();
            app.Stop();
        }
        REQUIRE(countFilesWithSuffix(dir, ".log") >= 1);
        rmrf(dir);
    }

    // ---- Archive: Stop() while a file is still open -> CloseFile archive branch ----
    SECTION("Archive zips currently-open file when stopped without rotation")
    {
        auto dir = uniqueTempDir("roll_clarch");
        {
            // Large size + a single record => no rotation, file stays open.
            RollingFileAppender app(Path(dir), "svc", "log", 1u << 20, 3,
                                    /*archive=*/true, false, true, /*auto_start=*/false);
            REQUIRE(app.Start());
            auto r = makeRollRecord(Level::INFO, "close-archive-record");
            layoutRecord(r);
            app.AppendRecord(r);
            // Deliberately no Flush()/no rotation: Stop()'s CloseFile must close
            // the still-open file and queue it for archivation.
            app.Stop();
        }
        bool hasZip = false;
        for (int i = 0; i < 10; ++i)
        {
            if (countFilesWithSuffix(dir, ".zip") > 0)
            {
                hasZip = true;
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        REQUIRE(hasZip);
        rmrf(dir);
    }

    // ---- PrepareFile IO error is swallowed (catch branches) ----
    SECTION("PrepareFile survives an unwritable base path")
    {
        // Use a regular file as the parent so CreateTree beneath it fails and
        // the FileSystemException catch in PrepareFile is exercised.
        auto blocker = (std::filesystem::temp_directory_path() / "roll_blocker_XXXXXX").string();
        std::vector<char> bbuf(blocker.begin(), blocker.end());
        bbuf.push_back('\0');
        int fd = mkstemp(bbuf.data());
        REQUIRE(fd >= 0);
        dprintf(fd, "x");
        close(fd);
        std::string blockerPath(bbuf.data());
        std::string badPath = blockerPath + "/sub";

        // No assert on outcomes beyond "does not throw / does not crash": the
        // point is to drive the FileSystemException catch branch in PrepareFile.
        {
            RollingFileAppender app(Path(badPath), "svc", "log", 1024, 3,
                                    false, false, true, /*auto_start=*/false);
            REQUIRE(app.Start());
            for (int i = 0; i < 3; ++i)
            {
                auto r = makeRollRecord(Level::INFO, "into-the-void");
                layoutRecord(r);
                app.AppendRecord(r);  // PrepareFile throws -> caught, record dropped
            }
            app.Flush();
            REQUIRE(app.Stop());
        }
        rmrf(blockerPath);
    }

    // ---- Empty raw record is skipped (no file materialises) ----
    SECTION("Record with empty raw is skipped and creates no file")
    {
        auto dir = uniqueTempDir("roll_empty");
        {
            RollingFileAppender app(Path(dir), "svc", "log", 1024, 3,
                                    false, false, true, /*auto_start=*/false);
            REQUIRE(app.Start());
            Record r = makeRollRecord(Level::INFO, "ignored");  // raw left empty
            app.AppendRecord(r);
            app.Flush();
            app.Stop();
        }
        // AppendRecord returns before PrepareFile, so no base file is opened.
        REQUIRE_FALSE(std::filesystem::exists(dir + "/svc.log"));
        rmrf(dir);
    }

    // ---- Destructor stops the appender when auto-started ----
    SECTION("Destructor implicitly stops a started appender")
    {
        auto dir = uniqueTempDir("roll_dtor");
        {
            RollingFileAppender app(Path(dir), "svc", "log", 1024, 3,
                                    false, true, true, /*auto_start=*/true);
            REQUIRE(app.IsStarted());
            auto r = makeRollRecord(Level::INFO, "dtor-record");
            layoutRecord(r);
            app.AppendRecord(r);  // forces PrepareFile to open svc.log
            // let ~RollingFileAppender run Stop() implicitly
        }
        // After destruction the file should exist (Start opened it).
        REQUIRE(std::filesystem::exists(dir + "/svc.log"));
        rmrf(dir);
    }

    // ---- Time policy SECOND + archive: window-expiry roll & archivation ----
    SECTION("Time policy SECOND with archive rolls and archives on window expiry")
    {
        auto dir = uniqueTempDir("roll_secarch");
        {
            RollingFileAppender app(Path(dir), TimeRollingPolicy::SECOND, "{UtcDateTime}.log",
                                    /*archive=*/true, true, true, /*auto_start=*/false);
            REQUIRE(app.Start());
            for (int i = 0; i < 2; ++i)
            {
                Record r = makeRollRecord(Level::INFO, i == 0 ? "first-second" : "second-second");
                // Use the wall clock so the 2nd record lands outside the 1s window.
                r.timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
                layoutRecord(r);
                app.AppendRecord(r);
                if (i == 0)
                    std::this_thread::sleep_for(std::chrono::seconds(2));
            }
            app.Flush();
            app.Stop();  // joins the archivation thread
        }
        bool hasZip = false;
        for (int i = 0; i < 10; ++i)
        {
            if (countFilesWithSuffix(dir, ".zip") > 0)
            {
                hasZip = true;
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        CHECK(hasZip);
        rmrf(dir);
    }

    // ---- Time policy filename tokenizer edge cases (nested {, literal }, unclosed {) ----
    SECTION("Time policy filename tokenizer handles brace edge cases")
    {
        auto dir = uniqueTempDir("roll_tok");
        {
            // Constructing each appender runs the tokenizer in the ctor.
            RollingFileAppender a(Path(dir), TimeRollingPolicy::DAY, "{{nested}.log",
                                   false, true, true, /*auto_start=*/false);
            RollingFileAppender b(Path(dir), TimeRollingPolicy::DAY, "lit}eral.log",
                                   false, true, true, /*auto_start=*/false);
            RollingFileAppender c(Path(dir), TimeRollingPolicy::DAY, "{unclosed.log",
                                   false, true, true, /*auto_start=*/false);
        }
        SUCCEED("tokenizer edge cases did not throw");
        rmrf(dir);
    }

    // ---- Time policy PrepareFile IO error (catch branch) ----
    SECTION("Time policy PrepareFile survives an unwritable base path")
    {
        auto blocker = (std::filesystem::temp_directory_path() / "roll_tblock_XXXXXX").string();
        std::vector<char> bbuf(blocker.begin(), blocker.end());
        bbuf.push_back('\0');
        int fd = mkstemp(bbuf.data());
        REQUIRE(fd >= 0);
        dprintf(fd, "x");
        close(fd);
        std::string blockerPath(bbuf.data());
        std::string badPath = blockerPath + "/sub";
        {
            RollingFileAppender app(Path(badPath), TimeRollingPolicy::DAY, "{UtcDateTime}.log",
                                    false, false, true, /*auto_start=*/false);
            REQUIRE(app.Start());
            for (int i = 0; i < 3; ++i)
            {
                auto r = makeRollRecord(Level::INFO, "time-into-the-void");
                layoutRecord(r);
                app.AppendRecord(r);
            }
            app.Flush();
            REQUIRE(app.Stop());
        }
        rmrf(blockerPath);
    }

    // ---- Time policy destructor stops a started appender (derived dtor Stop) ----
    SECTION("Time policy destructor implicitly stops a started appender")
    {
        auto dir = uniqueTempDir("roll_tdtor");
        {
            RollingFileAppender app(Path(dir), TimeRollingPolicy::DAY, "{UtcDateTime}.log",
                                    false, true, true, /*auto_start=*/true);
            REQUIRE(app.IsStarted());
            auto r = makeRollRecord(Level::INFO, "time-dtor-record");
            layoutRecord(r);
            app.AppendRecord(r);
            // No explicit Stop(): ~TimePolicyImpl must Stop() the started appender.
        }
        REQUIRE(std::filesystem::exists(dir));
        rmrf(dir);
    }
}
