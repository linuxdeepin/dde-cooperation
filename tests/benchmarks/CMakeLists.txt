# httpweb transfer benchmark (dataset generator + legacy baseline runner).
# Enabled with -DBUILD_HTTPWEB_BENCH=ON; independent from DOTEST so it can run
# on machines without GTest.

find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

add_executable(httpweb_transfer_benchmark httpweb_transfer_benchmark.cpp)

target_link_libraries(httpweb_transfer_benchmark PRIVATE
    httpweb
    sslconf
    ${OPENSSL_LIBRARIES}
    Threads::Threads
)

target_include_directories(httpweb_transfer_benchmark PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/lib/common
    ${CMAKE_SOURCE_DIR}/src/lib/common/httpweb
    ${CMAKE_SOURCE_DIR}/3rdparty/include
)

set_target_properties(httpweb_transfer_benchmark PROPERTIES FOLDER "benchmarks")
