enable_testing()

# 添加Google Test
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# 测试源文件
set(TEST_SOURCES
    fmt_test.cpp
    utils_test.cpp
)

# 创建测试可执行文件
add_executable(fmt_tests ${TEST_SOURCES})
target_link_libraries(fmt_tests
    PRIVATE 
    fmt
    GTest::GTest 
    GTest::Main
    pthread
)

# 启用 CTest 集成
gtest_discover_tests(fmt_tests)
