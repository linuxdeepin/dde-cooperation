// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef BASEKIT_ALGORITHMS_TOKEN_BUCKET_H

#include <atomic>
#include <cstdint>

namespace BaseKit {

//! Token bucket rate limit algorithm
/*!
    Token bucket is a rate limit algorithm that allows a burst of traffic
    to be sent at once, but limits the average rate of traffic over time.
    It is commonly used in network traffic shaping and bandwidth management.

    https://en.wikipedia.org/wiki/Token_bucket
*/
class TokenBucket
{
public:
    //! Initialize the token bucket
    /*!
        Initializes the token bucket to accumulate the given count of tokens
        per second, with a maximum of burst tokens.

        \param rate - Rate of tokens per second to accumulate in the token bucket
        \param burst - Maximum of burst tokens in the token bucket
    */
    TokenBucket(uint64_t rate, uint64_t burst);
    TokenBucket(const TokenBucket& tb);
    TokenBucket(TokenBucket&&) = delete;
    ~TokenBucket() = default;

    TokenBucket& operator=(const TokenBucket& tb);
    TokenBucket& operator=(TokenBucket&&) = delete;

    //! Try to consume the given count of tokens
    /*!
        \param tokens - Tokens to consume (default is 1)
        \return 'true' if all tokens were successfully consumed, 'false' if the token bucket is lack of required count of tokens
    */
    bool Consume(uint64_t tokens = 1);

private:
    std::atomic<uint64_t> _time;
    std::atomic<uint64_t> _time_per_token;
    std::atomic<uint64_t> _time_per_burst;
};


} // namespace BaseKit

#include "token_bucket.inl"

#endif // BASEKIT_ALGORITHMS_TOKEN_BUCKET_H
