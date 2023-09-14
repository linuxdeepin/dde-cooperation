
set(CMAKE_CXX_STANDARD 17)

# protobuf.a 需要加“-fPIC”， 否则无法连接到.so
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
# 添加tinyrpc子目录到项目中
add_subdirectory(tinyrpc)


