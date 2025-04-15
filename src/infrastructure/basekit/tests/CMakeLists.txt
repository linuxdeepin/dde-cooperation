cmake_minimum_required(VERSION 3.13)

# Find GTest package
find_package(GTest REQUIRED)

# 收集所有测试源文件
file(GLOB TEST_SOURCES
    "*.cpp"
    "common/*.cpp"
    "utility/*.cpp"
    "cache/*.cpp"
    "string/*.cpp"
    "errors/*.cpp"
    "math/*.cpp"
    "time/*.cpp"
    "system/*.cpp"
    "memory/*.cpp"
    "algorithms/*.cpp"
    "threads/*.cpp"
    "filesystem/*.cpp"
    "containers/*.cpp"
)

# Add test executable
add_executable(basekit_tests
    ${TEST_SOURCES}
)

# Link test executable with GTest and basekit
target_link_libraries(basekit_tests
    PRIVATE
    GTest::GTest
    GTest::Main
    basekit
)

# Include directories
target_include_directories(basekit_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

# Add test to CTest
add_test(NAME basekit_tests COMMAND basekit_tests)

# 添加各个单独的测试套件
add_test(NAME TokenBucketTest COMMAND basekit_tests --gtest_filter=TokenBucketTest.*)
add_test(NAME QueueTest COMMAND basekit_tests --gtest_filter=QueueTest.*)
add_test(NAME MemoryTest COMMAND basekit_tests --gtest_filter=MemoryTest.*)
add_test(NAME StackTest COMMAND basekit_tests --gtest_filter=StackTest.*)
add_test(NAME HashMapTest COMMAND basekit_tests --gtest_filter=HashMapTest.*) 