#pragma once
#include <string>
#include <vector>

namespace fmt {
namespace utils {

std::vector<std::string> split(const std::string& str, char delimiter);
std::string trim(const std::string& str);
bool starts_with(const std::string& str, const std::string& prefix);
bool ends_with(const std::string& str, const std::string& suffix);

// 添加新的工具函数
template<typename T>
std::string to_string(const T& value);

// UTF8相关函数
size_t count_code_points(const char* str);
size_t code_point_length(const char* str);
std::string to_utf8(uint32_t code_point);
uint32_t from_utf8(const char* str, size_t* size = nullptr);

std::string to_utf8(uint32_t code_point);
uint32_t from_utf8(const char* str, size_t* size);

class utf8_iterator {
public:
    explicit utf8_iterator(const char* str) : ptr_(str) {}
    uint32_t operator*() const;
    utf8_iterator& operator++();
private:
    const char* ptr_;
};

} // namespace utils
} // namespace fmt
