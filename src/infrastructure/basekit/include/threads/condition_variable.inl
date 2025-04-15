// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

namespace BaseKit {

template <typename TPredicate>
void ConditionVariable::Wait(CriticalSection& cs, TPredicate predicate)
{
    while (!predicate())
        Wait(cs);
}

template <typename TPredicate>
bool ConditionVariable::TryWaitFor(CriticalSection& cs, const Timespan& timespan, TPredicate predicate)
{
    Timestamp timeout = UtcTimestamp() + timespan;
    while (!predicate())
        if (!TryWaitFor(cs, timeout - UtcTimestamp()))
            return predicate();
    return true;
}

} // namespace BaseKit
