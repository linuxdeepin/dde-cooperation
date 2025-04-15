// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later


namespace BaseKit {

inline Process Process::CurrentProcess()
{
    return Process(CurrentProcessId());
}

inline Process Process::ParentProcess()
{
    return Process(ParentProcessId());
}

inline void swap(Process& process1, Process& process2) noexcept
{
    process1.swap(process2);
}

} // namespace BaseKit
