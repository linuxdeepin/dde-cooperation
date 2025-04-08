// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

namespace BaseKit {

template <class TOutputStream>
inline TOutputStream& operator<<(TOutputStream& stream, ThreadPriority priority)
{
    switch (priority)
    {
        case ThreadPriority::IDLE:
            stream << "IDLE";
            break;
        case ThreadPriority::LOWEST:
            stream << "LOWEST";
            break;
        case ThreadPriority::LOW:
            stream << "LOW";
            break;
        case ThreadPriority::NORMAL:
            stream << "NORMAL";
            break;
        case ThreadPriority::HIGH:
            stream << "HIGH";
            break;
        case ThreadPriority::HIGHEST:
            stream << "HIGHEST";
            break;
        case ThreadPriority::REALTIME:
            stream << "REALTIME";
            break;
        default:
            stream << "<unknown>";
            break;
    }
    return stream;
}

template <class Fn, class... Args>
inline std::thread Thread::Start(Fn&& fn, Args&&... args)
{
    return std::thread([fn = fn, args...]()
    {
        // Setup exception handler for the new thread
        ExceptionsHandler::SetupThread();

        // Call the base thread function
        fn(std::move(args)...);
    });
}

} // namespace BaseKit
