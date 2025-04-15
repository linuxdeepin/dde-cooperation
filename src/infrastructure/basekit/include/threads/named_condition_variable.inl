// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

namespace BaseKit {

template <typename TPredicate>
void NamedConditionVariable::Wait(TPredicate predicate)
{
    while (!predicate())
        Wait();
}

template <typename TPredicate>
bool NamedConditionVariable::TryWaitFor(const Timespan& timespan, TPredicate predicate)
{
    Timestamp timeout = UtcTimestamp() + timespan;
    while (!predicate())
        if (!TryWaitFor(timeout - UtcTimestamp()))
            return predicate();
    return true;
}

} // namespace BaseKit
