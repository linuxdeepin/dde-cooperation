# 添加基本示例
add_executable(basic_example basic.cpp)
target_link_libraries(basic_example PRIVATE fmt)

# 添加自定义类型示例
add_executable(custom_type_example custom_type.cpp)
target_link_libraries(custom_type_example PRIVATE fmt)
