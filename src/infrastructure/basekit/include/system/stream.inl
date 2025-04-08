// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later


namespace BaseKit {

inline void swap(StdInput& stream1, StdInput& stream2) noexcept
{
    stream1.swap(stream2);
}

inline void swap(StdOutput& stream1, StdOutput& stream2) noexcept
{
    stream1.swap(stream2);
}

inline void swap(StdError& stream1, StdError& stream2) noexcept
{
    stream1.swap(stream2);
}

} // namespace BaseKit
