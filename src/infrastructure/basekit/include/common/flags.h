// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef BASEKIT_FLAGS_H
#define BASEKIT_FLAGS_H

#include <bitset>
#include <type_traits>

namespace BaseKit {

//! Enum-based flags false checker
template <typename TEnum>
struct IsEnumFlags : public std::false_type {};

//! Register a new enum-based flags macro
#define ENUM_FLAGS(type)\
using BaseKit::operator&;\
using BaseKit::operator|;\
using BaseKit::operator^;\
namespace BaseKit {\
template <> struct IsEnumFlags<type> : std::true_type {};\
}

//! Enum-based flags
/*! \note This class is not intended to be used directly, use the ENUM_FLAGS macro instead. */
template <typename TEnum>
class Flags
{
    //! Enum underlying type
    typedef typename std::make_unsigned<typename std::underlying_type<TEnum>::type>::type type;

public:
    Flags() noexcept : _value(0) {}
    Flags(type value) noexcept : _value(value) {}
    Flags(TEnum value) noexcept : _value((type)value) {}
    Flags(const Flags&) noexcept = default;
    Flags(Flags&&) noexcept = default;
    ~Flags() noexcept = default;

    Flags& operator=(type value) noexcept
    { _value = value; return *this; }
    Flags& operator=(TEnum value) noexcept
    { _value = (type)value; return *this; }
    Flags& operator=(const Flags&) noexcept = default;
    Flags& operator=(Flags&&) noexcept = default;

    //! Is any flag set?
    explicit operator bool() const noexcept { return isset(); }

    //! Is no flag set?
    bool operator!() const noexcept { return !isset(); }

    //! Reverse all flags
    Flags operator~() const noexcept { return Flags(~_value); }

    //! Flags logical assign operators
    Flags& operator&=(const Flags& flags) noexcept
    { _value &= flags._value; return *this; }
    Flags& operator|=(const Flags& flags) noexcept
    { _value |= flags._value; return *this; }
    Flags& operator^=(const Flags& flags) noexcept
    { _value ^= flags._value; return *this; }

    //! Flags logical friend operators
    friend Flags operator&(const Flags& flags1, const Flags& flags2) noexcept
    { return Flags(flags1._value & flags2._value); }
    friend Flags operator|(const Flags& flags1, const Flags& flags2) noexcept
    { return Flags(flags1._value | flags2._value); }
    friend Flags operator^(const Flags& flags1, const Flags& flags2) noexcept
    { return Flags(flags1._value ^ flags2._value); }

    // Flags comparison
    friend bool operator==(const Flags& flags1, const Flags& flags2) noexcept
    { return flags1._value == flags2._value; }
    friend bool operator!=(const Flags& flags1, const Flags& flags2) noexcept
    { return flags1._value != flags2._value; }

    //! Convert to the enum value
    operator TEnum() const noexcept { return (TEnum)_value; }

    //! Is any flag set?
    bool isset() const noexcept { return (_value != 0); }
    //! Is the given flag set?
    bool isset(type value) const noexcept { return (_value & value) != 0; }
    //! Is the given flag set?
    bool isset(TEnum value) const noexcept { return (_value & (type)value) != 0; }

    //! Get the enum value
    TEnum value() const noexcept { return (TEnum)_value; }
    //! Get the underlying enum value
    type underlying() const noexcept { return _value; }
    //! Get the bitset value
    std::bitset<sizeof(type) * 8> bitset() const noexcept { return {_value}; }

    //! Swap two instances
    void swap(Flags& flags) noexcept { using std::swap; swap(_value, flags._value); }
    template <typename UEnum>
    friend void swap(Flags<UEnum>& flags1, Flags<UEnum>& flags2) noexcept;

private:
    type _value;
};


} // namespace BaseKit

#include "flags.inl"

#endif // BASEKIT_FLAGS_H
