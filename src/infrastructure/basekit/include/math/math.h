// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later


#ifndef BASEKIT_MATH_MATH_H
#define BASEKIT_MATH_MATH_H

#include <cstdint>

namespace BaseKit {

//! Math static class
/*!
    Contains useful math functions.

    Thread-safe.
*/
class Math
{
public:
    Math() = delete;
    Math(const Math&) = delete;
    Math(Math&&) = delete;
    ~Math() = delete;

    Math& operator=(const Math&) = delete;
    Math& operator=(Math&&) = delete;

    //! Computes the greatest common divisor of a and b
    /*!
        \param a - Value a
        \param b - Value b
        \return Greatest common divisor of a and b
    */
    template <typename T>
    static T GCD(T a, T b);

    //! Finds the smallest value x >= a such that x % k == 0
    /*!
        \param a - Value a
        \param k - Value k
        \return Value x
    */
    template <typename T>
    static T RoundUp(T a, T k);

    //! Calculate (operant * multiplier / divider) with 64-bit unsigned integer values
    /*!
        \param operant - Operant
        \param multiplier - Multiplier
        \param divider - Divider
        \return Calculated value of (operant * multiplier / divider) expression
    */
    static uint64_t MulDiv64(uint64_t operant, uint64_t multiplier, uint64_t divider);
};


} // namespace BaseKit

#include "math.inl"

#endif // BASEKIT_MATH_MATH_H
