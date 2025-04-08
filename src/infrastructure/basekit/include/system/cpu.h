// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later


#ifndef BASEKIT_SYSTEM_CPU_H
#define BASEKIT_SYSTEM_CPU_H

#include <cstdint>
#include <string>

namespace BaseKit {

//! CPU management static class
/*!
    Provides CPU management functionality such as architecture, cores count,
    clock speed, Hyper-Threading feature.

    Thread-safe.
*/
class CPU
{
public:
    CPU() = delete;
    CPU(const CPU&) = delete;
    CPU(CPU&&) = delete;
    ~CPU() = delete;

    CPU& operator=(const CPU&) = delete;
    CPU& operator=(CPU&&) = delete;

    //! CPU architecture string
    static std::string Architecture();
    //! CPU affinity count
    static int Affinity();
    //! CPU logical cores count
    static int LogicalCores();
    //! CPU physical cores count
    static int PhysicalCores();
    //! CPU total cores count
    static std::pair<int, int> TotalCores();
    //! CPU clock speed in Hz
    static int64_t ClockSpeed();
    //! Is CPU Hyper-Threading enabled?
    static bool HyperThreading();
};


} // namespace BaseKit

#endif // BASEKIT_SYSTEM_CPU_H
