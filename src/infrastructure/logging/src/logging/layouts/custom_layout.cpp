// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "logging/layouts/custom_layout.h"

namespace Logging {

CustomLayout::CustomLayout(FormatterFn formatter) : _formatter(std::move(formatter))
{
}

void CustomLayout::LayoutRecord(Record& record)
{
    // 使用自定义格式化函数来格式化记录
    std::string formatted = _formatter(record);
    
    // 将格式化后的字符串设置为记录的原始内容
    record.raw.assign(formatted.begin(), formatted.end());
}

} // namespace Logging 