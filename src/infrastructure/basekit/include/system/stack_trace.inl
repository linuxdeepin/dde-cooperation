// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later


#if defined(FMT_VERSION)
template <> struct fmt::formatter<BaseKit::StackTrace> : ostream_formatter {};
#endif
