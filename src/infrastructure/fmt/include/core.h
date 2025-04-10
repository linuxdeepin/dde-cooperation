#pragma once
#include "fmt.h"

namespace fmt {
namespace detail {

class char_traits {
public:
    static size_t count_code_points(const char* str);
    static size_t code_point_length(const char* str);
};
} // namespace detail
} // namespace fmt
