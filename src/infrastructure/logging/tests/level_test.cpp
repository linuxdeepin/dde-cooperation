#include <catch2/catch_all.hpp>
#include <logging/level.h>
#include <sstream>

using namespace Logging;

TEST_CASE("Level Enum Values", "[level]") {
    SECTION("Level enum values are correct") {
        REQUIRE(static_cast<uint8_t>(Level::NONE) == 0x00);
        REQUIRE(static_cast<uint8_t>(Level::FATAL) == 0x1F);
        REQUIRE(static_cast<uint8_t>(Level::ERROR) == 0x3F);
        REQUIRE(static_cast<uint8_t>(Level::WARN) == 0x7F);
        REQUIRE(static_cast<uint8_t>(Level::INFO) == 0x9F);
        REQUIRE(static_cast<uint8_t>(Level::DEBUG) == 0xBF);
        REQUIRE(static_cast<uint8_t>(Level::ALL) == 0xFF);
    }
}

TEST_CASE("Level comparison operations", "[level]") {
    SECTION("Level comparison works correctly") {
        // 级别从低到高: NONE < FATAL < ERROR < WARN < INFO < DEBUG < ALL
        REQUIRE(Level::NONE < Level::FATAL);
        REQUIRE(Level::FATAL < Level::ERROR);
        REQUIRE(Level::ERROR < Level::WARN);
        REQUIRE(Level::WARN < Level::INFO);
        REQUIRE(Level::INFO < Level::DEBUG);
        REQUIRE(Level::DEBUG < Level::ALL);
        
        // 测试大于关系
        REQUIRE(Level::ALL > Level::DEBUG);
        REQUIRE(Level::DEBUG > Level::INFO);
        REQUIRE(Level::INFO > Level::WARN);
        REQUIRE(Level::WARN > Level::ERROR);
        REQUIRE(Level::ERROR > Level::FATAL);
        REQUIRE(Level::FATAL > Level::NONE);
    }
}

TEST_CASE("Level to string conversion", "[level]") {
    SECTION("Level to string output is correct") {
        std::stringstream ss;
        
        ss.str("");
        ss << Level::NONE;
        REQUIRE(ss.str() == "NONE");
        
        ss.str("");
        ss << Level::FATAL;
        REQUIRE(ss.str() == "FATAL");
        
        ss.str("");
        ss << Level::ERROR;
        REQUIRE(ss.str() == "ERROR");
        
        ss.str("");
        ss << Level::WARN;
        REQUIRE(ss.str() == "WARN");
        
        ss.str("");
        ss << Level::INFO;
        REQUIRE(ss.str() == "INFO");
        
        ss.str("");
        ss << Level::DEBUG;
        REQUIRE(ss.str() == "DEBUG");
        
        ss.str("");
        ss << Level::ALL;
        REQUIRE(ss.str() == "ALL");
    }
} 