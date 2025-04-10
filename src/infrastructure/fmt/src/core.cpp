#include "core.h"
#include "fmt_utils.h"
#include <string>
#include <cstring>
#include <iostream>

namespace fmt {
namespace detail {

FormatSpec core_formatter::parse_format_spec(const char* fmt) {
    FormatSpec spec;
    const char* ptr = fmt;
    
    // 跳过冒号
    if (*ptr == ':') ++ptr;
    
    // 检查填充字符和对齐方式
    if (ptr[0] && ptr[1] && (ptr[1] == '<' || ptr[1] == '>' || ptr[1] == '^')) {
        spec.fill = *ptr++;
        if (*ptr == '<') {
            spec.align_left = true;
        }
        ++ptr;
    } else {
        // 检查只有对齐符号的情况
        if (*ptr == '<') {
            spec.align_left = true;
            ++ptr;
        } else if (*ptr == '>') {
            spec.align_left = false;
            ++ptr;
        } else if (*ptr == '^') {
            // 居中对齐
            ++ptr;
        } else if (*ptr == '0') {
            // 零填充
            spec.fill = '0';
            spec.zero_padding = true;
            ++ptr;
        }
    }
    
    // 解析宽度
    while (*ptr && std::isdigit(*ptr)) {
        spec.width = spec.width * 10 + (*ptr - '0');
        ++ptr;
    }
    
    // 解析类型
    if (*ptr && *ptr != '\0') {
        spec.type = *ptr;
        // 标记是否为数字类型
        if (*ptr == 'd') {
            spec.numeric = true;
        }
        ++ptr;
    }
    
    return spec;
}

template<typename Context>
typename Context::iterator core_formatter::format(const char* fmt, Context& ctx) {
    auto& buffer = ctx.buffer();
    const char* ptr = fmt;
    
    while (*ptr) {
        if (*ptr == '{') {
            // 处理转义 {{
            if (ptr[1] == '{') {
                buffer.push_back('{');
                ptr += 2;
                continue;
            }
            
            // 处理格式说明符
            ++ptr;  // 跳过'{'
            const char* spec_start = ptr;
            
            // 寻找匹配的}
            while (*ptr && *ptr != '}') ptr++;
            if (!*ptr) throw FormatError("unmatched '{'");
            
            const value_type* arg = ctx.next_arg();
            if (!arg) throw FormatError("invalid format argument index");

            std::string value = arg->to_string();
            
            // 调试输出
            std::cout << "Formatting argument: " << value << std::endl;
            
            FormatSpec spec = parse_format_spec(spec_start);
            format_value(buffer, value, spec);
            ++ptr;  // 跳过'}'
        } else if (*ptr == '}') {
            // 处理转义}}
            if (ptr[1] == '}') {
                buffer.push_back('}');
                ptr += 2;
            } else {
                throw FormatError("invalid '}' in format string");
            }
        } else {
            buffer.push_back(*ptr++);
        }
    }
    
    return buffer.data();
}

void core_formatter::format_value(memory_buffer& buf, const std::string& value,
                                const FormatSpec& spec) {
    // 如果没有宽度要求，直接输出
    if (spec.width == 0 || value.length() >= static_cast<size_t>(spec.width)) {
        buf.append(value.data(), value.length());
        return;
    }

    size_t padding = static_cast<size_t>(spec.width) - value.length();
    char fill_char = spec.fill;

    // 处理数字的零填充
    if (spec.numeric && spec.zero_padding && spec.type == 'd') {
        std::string padded_value(spec.width, '0');
        std::copy(value.begin(), value.end(), padded_value.end() - value.length());
        buf.append(padded_value.data(), padded_value.length());
        return;
    }

    // 根据对齐方式填充
    if (spec.align_left) {
        // 左对齐：先值后填充
        buf.append(value.data(), value.length());
        buf.append(padding, fill_char);
    } else {
        // 右对齐：先填充后值
        buf.append(padding, fill_char);
        buf.append(value.data(), value.length());
    }
}

// 添加显式实例化
template format_context::iterator 
core_formatter::format<format_context>(const char*, format_context&);

size_t char_traits::count_code_points(const char* str) {
    size_t count = 0;
    while (*str) {
        if ((*str & 0xC0) != 0x80) count++;
        str++;
    }
    return count;
}

size_t char_traits::code_point_length(const char* str) {
    unsigned char first = static_cast<unsigned char>(*str);
    if (first <= 0x7F) return 1;
    if (first <= 0xDF) return 2;
    if (first <= 0xEF) return 3;
    return 4;
}

}} // namespace fmt::detail