// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

namespace BaseKit {

inline TokenBucket::TokenBucket(uint64_t rate, uint64_t burst)
    : _time(0),
      _time_per_token(1000000000 / rate),
      _time_per_burst(burst * _time_per_token)
{
}

inline TokenBucket::TokenBucket(const TokenBucket& tb)
    : _time(tb._time.load()),
      _time_per_token(tb._time_per_token.load()),
      _time_per_burst(tb._time_per_burst.load())
{
}

inline TokenBucket& TokenBucket::operator=(const TokenBucket& tb)
{
    _time = tb._time.load();
    _time_per_token = tb._time_per_token.load();
    _time_per_burst = tb._time_per_burst.load();
    return *this;
}

} // namespace BaseKit
