#include <sstream>
#include <string>
#include <vector>
#include "fmt_utils.h"
#include <algorithm>

namespace fmt {
namespace utils {

std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::istringstream tokenStream(str);
    std::string token;
    
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    
    return tokens;
}

std::string trim(const std::string& str) {
    auto wsfront = std::find_if_not(str.begin(), str.end(), 
                                   [](int c){return std::isspace(c);});
    auto wsback = std::find_if_not(str.rbegin(), str.rend(), 
                                  [](int c){return std::isspace(c);}).base();
    return (wsback <= wsfront ? std::string() : 
            std::string(wsfront, wsback));
}

bool starts_with(const std::string& str, const std::string& prefix) {
    return str.size() >= prefix.size() && 
           str.compare(0, prefix.size(), prefix) == 0;
}

bool ends_with(const std::string& str, const std::string& suffix) {
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string to_utf8(uint32_t code_point) {
    std::string result;
    if (code_point <= 0x7F) {
        result.push_back(static_cast<char>(code_point));
    } else if (code_point <= 0x7FF) {
        result.push_back(static_cast<char>(0xC0 | (code_point >> 6)));
        result.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
    } else if (code_point <= 0xFFFF) {
        result.push_back(static_cast<char>(0xE0 | (code_point >> 12)));
        result.push_back(static_cast<char>(0x80 | ((code_point >> 6) & 0x3F)));
        result.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
    } else {
        result.push_back(static_cast<char>(0xF0 | (code_point >> 18)));
        result.push_back(static_cast<char>(0x80 | ((code_point >> 12) & 0x3F)));
        result.push_back(static_cast<char>(0x80 | ((code_point >> 6) & 0x3F)));
        result.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
    }
    return result;
}

size_t count_code_points(const char* str) {
    size_t count = 0;
    while (*str) {
        if ((*str & 0xC0) != 0x80) count++;
        str++;
    }
    return count;
}

size_t code_point_length(const char* str) {
    unsigned char first = static_cast<unsigned char>(*str);
    if (first <= 0x7F) return 1;
    if (first <= 0xDF) return 2;
    if (first <= 0xEF) return 3;
    return 4;
}

} // namespace utils
} // namespace fmt
