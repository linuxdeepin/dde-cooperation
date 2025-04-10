#include "fmt.h"
#include <regex>

namespace fmt {

void format(std::ostream& out, const std::string& fmt,
                      const std::vector<std::string>& args) {
    std::regex pattern(R"(\{(\d*)(?::([^}]*))?\})");
    std::string::const_iterator start = fmt.begin();
    std::smatch match;
    size_t arg_index = 0;

    while (std::regex_search(start, fmt.end(), match, pattern)) {
        // 输出前缀文本
        out << std::string(start, match[0].first);
        
        // 解析参数索引
        if (!match[1].length()) {
            if (arg_index >= args.size()) {
                throw FormatError("Too few arguments");
            }
        } else {
            arg_index = std::stoi(match[1]);
            if (arg_index >= args.size()) {
                throw FormatError("Invalid argument index");
            }
        }

        // 解析格式说明符
        FormatSpec spec;
        if (match[2].length()) {
            spec = detail::core_formatter().parse_format_spec(std::string(match[2]).c_str());
        }

        // 格式化值
        detail::core_formatter().format_value(out, args[arg_index], spec);
        
        start = match[0].second;
        arg_index++;
    }

    // 输出剩余文本
    out << std::string(start, fmt.end());
}

} // namespace fmt