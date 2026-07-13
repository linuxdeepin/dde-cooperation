// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// text_layout 扩展覆盖用例：补充 placeholder / level / thread / timezone /
// sub-second / tokenizer / cache 等未覆盖分支。

#include <catch2/catch_all.hpp>
#include <logging/layouts/text_layout.h>
#include <logging/record.h>

#include <algorithm>
#include <cstring>
#include <string>

using namespace Logging;

namespace {
Record makeLayoutRecord(Level level, const std::string& msg, const std::string& logger = "TL")
{
    Record r;
    // Pick a date with small (< 10) components to also exercise ConvertNumber
    // single-digit path and ConvertTimezone (minutes|hours) < 9 branch.
    r.timestamp = 1704067200ull * 1000000000ull;  // 2024-01-01T00:00:00Z (in ns)
    r.thread = 0x1122334455667788ull;
    r.level = level;
    r.logger = logger;
    r.message = msg;
    return r;
}

// Apply a TextLayout to a record and return the produced text, stripping the
// trailing NUL byte that TextLayout always appends.
//
// TextLayout caches date/time/thread/level strings in thread_local state that
// is only refreshed on call N+1 for placeholders output on call N. To make
// every test deterministic regardless of process-wide warm-up order, we render
// twice and return the second (fully cache-consistent) result.
std::string formatWith(const std::string& pattern, Record& rec)
{
    TextLayout layout(pattern);
    layout.LayoutRecord(rec);
    layout.LayoutRecord(rec);
    std::string out(rec.raw.size(), '\0');
    if (!rec.raw.empty())
        std::memcpy(out.data(), rec.raw.data(), rec.raw.size());
    const auto pos = out.find('\0');
    if (pos != std::string::npos)
        out.resize(pos);
    return out;
}
}  // namespace

TEST_CASE("text_layout extra coverage", "[layout][text_layout]")
{
    SECTION("pattern getter returns constructed pattern")
    {
        TextLayout layout("{Level}|{Message}");
        REQUIRE(layout.pattern() == "{Level}|{Message}");
    }

    SECTION("default pattern is used when none provided")
    {
        TextLayout layout;
        REQUIRE(layout.pattern().find("{UtcDateTime}") != std::string::npos);
        REQUIRE(layout.pattern().find("{Message}") != std::string::npos);
        REQUIRE(layout.pattern().find("{EndLine}") != std::string::npos);
    }

    // ---- Level coverage: every enum value ----
    SECTION("Level placeholder for all enum values")
    {
        struct Case { Level lvl; const char* name; };
        // ConvertLevel zero-pads the level name right to a fixed width of 5.
        const Case cases[] = {
            {Level::NONE, "NONE "},
            {Level::FATAL, "FATAL"},
            {Level::ERROR, "ERROR"},
            {Level::WARN, "WARN "},
            {Level::INFO, "INFO "},
            {Level::DEBUG, "DEBUG"},
            {Level::ALL, "ALL  "},
        };
        for (const auto& c : cases)
        {
            auto rec = makeLayoutRecord(c.lvl, "x");
            std::string out = formatWith("{Level}", rec);
            INFO("level=0x" << std::hex << static_cast<int>(c.lvl)
                            << " expected=" << c.name << " got=" << out);
            REQUIRE(out == c.name);
        }
    }

    SECTION("Level placeholder default branch for out-of-range value")
    {
        // 0x01 lies between NONE(0x00) and FATAL(0x1F) -> no switch match -> default "<???>"
        auto rec = makeLayoutRecord(Level::INFO, "x");
        rec.level = static_cast<Level>(0x01);
        std::string out = formatWith("{Level}", rec);
        REQUIRE(out.find("???") != std::string::npos);
    }

    // ---- Thread coverage ----
    SECTION("Thread placeholder formats as 0x-prefixed hex")
    {
        auto rec = makeLayoutRecord(Level::INFO, "x");
        rec.thread = 0xAB;
        std::string out = formatWith("[{Thread}]", rec);
        REQUIRE(out.substr(0, 3) == "[0x");
        REQUIRE(out.find("AB") != std::string::npos);
        REQUIRE(out.back() == ']');
    }

    SECTION("Thread placeholder with zero thread id")
    {
        auto rec = makeLayoutRecord(Level::INFO, "x");
        rec.thread = 0;
        std::string out = formatWith("{Thread}", rec);
        REQUIRE(out.substr(0, 2) == "0x");
    }

    SECTION("Thread placeholder renders full 16-hex-digit width")
    {
        auto rec = makeLayoutRecord(Level::INFO, "x");
        rec.thread = 0xDEADBEEFCAFEBABEull;
        std::string out = formatWith("{Thread}", rec);
        // "0x" + 16 hex digits
        REQUIRE(out.size() == 18);
        REQUIRE(out.find("DEADBEEFCAFEBABE") != std::string::npos);
    }

    // ---- Logger / Message / EndLine ----
    SECTION("Logger and Message placeholders")
    {
        auto rec = makeLayoutRecord(Level::INFO, "hello", "MyLogger");
        std::string out = formatWith("{Logger}={Message}", rec);
        REQUIRE(out == "MyLogger=hello");
    }

    SECTION("EndLine placeholder emits platform line separator")
    {
        auto rec = makeLayoutRecord(Level::INFO, "x");
        std::string out = formatWith("a{EndLine}b", rec);
        REQUIRE(out.front() == 'a');
        REQUIRE(out.back() == 'b');
        REQUIRE(out.size() >= 3);  // 'a' + at least '\n' + 'b'
    }

    // ---- UTC date/time placeholders ----
    SECTION("UTC aggregate date/time placeholders")
    {
        auto rec = makeLayoutRecord(Level::INFO, "x");
        std::string out = formatWith("{UtcDateTime}|{UtcDate}|{UtcTime}", rec);
        REQUIRE(out.find('T') != std::string::npos);
        REQUIRE(out.find('|') != std::string::npos);
        // {UtcTime} ends with the UTC timezone marker 'Z'
        REQUIRE(out.back() == 'Z');
    }

    SECTION("UTC individual component placeholders ending with timezone")
    {
        auto rec = makeLayoutRecord(Level::INFO, "x");
        std::string out = formatWith(
            "{UtcYear}-{UtcMonth}-{UtcDay}T{UtcHour}:{UtcMinute}:{UtcSecond}{UtcTimezone}", rec);
        REQUIRE(out.find('-') != std::string::npos);
        REQUIRE(out.find(':') != std::string::npos);
        REQUIRE(out.find('T') != std::string::npos);
        REQUIRE(out.back() == 'Z');
    }

    // ---- Local date/time placeholders ----
    SECTION("Local aggregate date/time placeholders")
    {
        auto rec = makeLayoutRecord(Level::INFO, "x");
        std::string out = formatWith("{LocalDateTime}|{LocalDate}|{LocalTime}", rec);
        REQUIRE(out.find('T') != std::string::npos);
        REQUIRE(out.find('|') != std::string::npos);
    }

    SECTION("Local individual component placeholders")
    {
        auto rec = makeLayoutRecord(Level::INFO, "x");
        std::string out = formatWith(
            "{LocalYear}-{LocalMonth}-{LocalDay} {LocalHour}:{LocalMinute}:{LocalSecond}"
            "{LocalTimezone}", rec);
        REQUIRE(out.find('-') != std::string::npos);
        REQUIRE(out.find(':') != std::string::npos);
        // LocalTimezone output shape: "+HH:MM" / "-HH:MM" (6 chars), ends with a digit
        REQUIRE(out.size() >= 6);
    }

    // ---- Sub-second placeholders ----
    SECTION("Millisecond / Microsecond / Nanosecond placeholders")
    {
        auto rec = makeLayoutRecord(Level::INFO, "x");
        std::string out = formatWith("{Millisecond}/{Microsecond}/{Nanosecond}", rec);
        REQUIRE(out.find('/') != std::string::npos);
        // Each component is zero-padded to 3 digits: 3 + 1 + 3 + 1 + 3 = 11
        REQUIRE(out.size() == 11);
    }

    // ---- Cache update branches ----
    SECTION("Time cache updates when timestamp changes between calls")
    {
        TextLayout layout("{UtcYear}-{UtcMonth}-{UtcDay}");
        auto rec1 = makeLayoutRecord(Level::INFO, "a");
        layout.LayoutRecord(rec1);
        // Second call with a different timestamp forces the seconds-changed update path
        auto rec2 = makeLayoutRecord(Level::INFO, "b");
        rec2.timestamp = (1704067200ull + 86400ull) * 1000000000ull;  // +1 day
        layout.LayoutRecord(rec2);
        std::string s2(rec2.raw.begin(), rec2.raw.end());
        REQUIRE_FALSE(s2.empty());
    }

    SECTION("Level cache updates on different consecutive levels")
    {
        TextLayout layout("{Level}");
        const Level levels[] = {Level::INFO, Level::ERROR, Level::DEBUG,
                                Level::NONE, Level::ALL};
        for (Level lv : levels)
        {
            auto rec = makeLayoutRecord(lv, "x");
            layout.LayoutRecord(rec);
            std::string s(rec.raw.begin(), rec.raw.end());
            REQUIRE_FALSE(s.empty());
        }
    }

    SECTION("Thread cache updates on different thread ids")
    {
        TextLayout layout("{Thread}");
        for (uint64_t t : {1ull, 2ull, 3ull, 0xABCDEFull, 0ull})
        {
            auto rec = makeLayoutRecord(Level::INFO, "x");
            rec.thread = t;
            layout.LayoutRecord(rec);
            std::string s(rec.raw.begin(), rec.raw.end());
            REQUIRE(s.substr(0, 2) == "0x");
        }
    }

    SECTION("Sub-second cache branches exercised via repeated formatting")
    {
        for (uint64_t ts : {1704067200ull * 1000000000ull,
                            1704067200ull * 1000000000ull + 1234567ull,
                            1704067200ull * 1000000000ull + 999999999ull})
        {
            auto rec = makeLayoutRecord(Level::INFO, "x");
            rec.timestamp = ts;
            std::string s = formatWith("{Millisecond}/{Microsecond}/{Nanosecond}", rec);
            // Each component is zero-padded to 3 digits: 3 + 1 + 3 + 1 + 3 = 11
            REQUIRE(s.size() == 11);
            REQUIRE(s[3] == '/');
            REQUIRE(s[7] == '/');
        }
    }

    // ---- Message / logger edge cases ----
    SECTION("Multiline message is preserved verbatim")
    {
        auto rec = makeLayoutRecord(Level::INFO, "line1\nline2\nline3");
        std::string out = formatWith("{Message}", rec);
        REQUIRE(out.find("line1") != std::string::npos);
        REQUIRE(out.find("line2") != std::string::npos);
        REQUIRE(out.find("line3") != std::string::npos);
        REQUIRE(std::count(out.begin(), out.end(), '\n') == 2);
    }

    SECTION("Long message boundary")
    {
        const std::string big(4096, 'X');
        auto rec = makeLayoutRecord(Level::INFO, big);
        std::string out = formatWith("{Message}", rec);
        REQUIRE(out.size() == 4096);
        REQUIRE(out == big);
    }

    SECTION("Empty logger name")
    {
        auto rec = makeLayoutRecord(Level::INFO, "msg", "");
        std::string out = formatWith("[{Logger}]", rec);
        REQUIRE(out == "[]");
    }

    SECTION("Whitespace-only logger name preserved")
    {
        auto rec = makeLayoutRecord(Level::INFO, "msg", "   ");
        std::string out = formatWith("[{Logger}]", rec);
        REQUIRE(out == "[   ]");
    }

    SECTION("Empty message")
    {
        auto rec = makeLayoutRecord(Level::INFO, "");
        std::string out = formatWith("[{Message}]", rec);
        REQUIRE(out == "[]");
    }

    SECTION("Embedded braces-like content in message")
    {
        auto rec = makeLayoutRecord(Level::INFO, "value={not-a-placeholder}");
        std::string out = formatWith("{Message}", rec);
        REQUIRE(out == "value={not-a-placeholder}");
    }

    // ---- Tokenizer edge cases ----
    SECTION("Unknown placeholder is emitted verbatim")
    {
        auto rec = makeLayoutRecord(Level::INFO, "msg");
        std::string out = formatWith("x{Bogus}y", rec);
        REQUIRE(out == "x{Bogus}y");
    }

    SECTION("Empty placeholder is ignored")
    {
        auto rec = makeLayoutRecord(Level::INFO, "msg");
        std::string out = formatWith("a{}b", rec);
        REQUIRE(out == "ab");
    }

    SECTION("Unclosed brace becomes literal pattern")
    {
        auto rec = makeLayoutRecord(Level::INFO, "msg");
        std::string out = formatWith("before{Level", rec);
        // No closing '}' -> the partial text is treated as a literal pattern
        REQUIRE(out.find("Level") != std::string::npos);
    }

    SECTION("Standalone closing brace is literal")
    {
        auto rec = makeLayoutRecord(Level::INFO, "msg");
        std::string out = formatWith("a}b", rec);
        REQUIRE(out.find('}') != std::string::npos);
        REQUIRE(out.front() == 'a');
    }

    SECTION("Pattern with only literal text")
    {
        auto rec = makeLayoutRecord(Level::INFO, "msg");
        std::string out = formatWith("just text", rec);
        REQUIRE(out == "just text");
    }

    SECTION("Repeated placeholders concatenate")
    {
        auto rec = makeLayoutRecord(Level::INFO, "m");
        std::string out = formatWith("{Level}{Level}", rec);
        // Each {Level} is a 5-char field ("INFO "), so concatenation includes spaces.
        REQUIRE(out == "INFO INFO ");
    }

    SECTION("Adjacent string fragments merge in tokenizer")
    {
        auto rec = makeLayoutRecord(Level::INFO, "msg");
        // "{}" empty placeholders split "ab" and "cd"; consecutive literal runs
        // are merged via the append-to-last-string branch.
        std::string out = formatWith("ab{}cd{}ef", rec);
        REQUIRE(out == "abcdef");
    }

    // ---- Interaction with Record format storage ----
    SECTION("LayoutRecord restores stored format into message")
    {
        Record rec;
        rec.timestamp = 1704067200ull * 1000000000ull;
        rec.thread = 1;
        rec.level = Level::INFO;
        rec.logger = "L";
        rec.StoreFormat("hello {}", 42);
        REQUIRE(rec.IsFormatStored());

        TextLayout layout("{Message}");
        layout.LayoutRecord(rec);

        std::string out(rec.raw.begin(), rec.raw.end());
        REQUIRE(out.find("hello 42") != std::string::npos);
    }

    SECTION("raw is cleared and repopulated on each LayoutRecord call")
    {
        TextLayout layout("{Message}");
        auto rec = makeLayoutRecord(Level::INFO, "first");
        layout.LayoutRecord(rec);
        REQUIRE_FALSE(rec.raw.empty());

        rec.message = "second";
        layout.LayoutRecord(rec);
        std::string out(rec.raw.begin(), rec.raw.end());
        REQUIRE(out.find("second") != std::string::npos);
        REQUIRE(out.find("first") == std::string::npos);
    }
}
