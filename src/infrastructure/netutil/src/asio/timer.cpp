// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "asio/timer.h"

namespace NetUtil {
namespace Asio {

Timer::Timer(const std::shared_ptr<Service>& service)
    : _service(service),
    _io_service(_service->GetAsioService()),
    _strand(*_io_service),
    _strand_required(_service->IsStrandRequired()),
    _timer(*_io_service)
{
    assert((service != nullptr) && "Asio service is invalid!");
    if (service == nullptr)
        throw BaseKit::ArgumentException("Asio service is invalid!");
}

Timer::Timer(const std::shared_ptr<Service>& service, const BaseKit::UtcTime& time)
    : _service(service),
    _io_service(_service->GetAsioService()),
    _strand(*_io_service),
    _strand_required(_service->IsStrandRequired()),
    _timer(*_io_service, time.chrono())
{
    assert((service != nullptr) && "Asio service is invalid!");
    if (service == nullptr)
        throw BaseKit::ArgumentException("Asio service is invalid!");
}

Timer::Timer(const std::shared_ptr<Service>& service, const BaseKit::Timespan& timespan)
    : _service(service),
    _io_service(_service->GetAsioService()),
    _strand(*_io_service),
    _strand_required(_service->IsStrandRequired()),
    _timer(*_io_service, timespan.chrono())
{
    assert((service != nullptr) && "Asio service is invalid!");
    if (service == nullptr)
        throw BaseKit::ArgumentException("Asio service is invalid!");
}

Timer::Timer(const std::shared_ptr<Service>& service, const std::function<void(bool)>& action)
    : _service(service),
    _io_service(_service->GetAsioService()),
    _strand(*_io_service),
    _strand_required(_service->IsStrandRequired()),
    _timer(*_io_service),
    _action(action)
{
    assert((service != nullptr) && "Asio service is invalid!");
    if (service == nullptr)
        throw BaseKit::ArgumentException("Asio service is invalid!");
    assert((action) && "Action function is invalid!");
    if (!action)
        throw BaseKit::ArgumentException("Action function is invalid!");
}

Timer::Timer(const std::shared_ptr<Service>& service, const std::function<void(bool)>& action, const BaseKit::UtcTime& time)
    : _service(service),
    _io_service(_service->GetAsioService()),
    _strand(*_io_service),
    _strand_required(_service->IsStrandRequired()),
    _timer(*_io_service, time.chrono()),
    _action(action)
{
    assert((service != nullptr) && "Asio service is invalid!");
    if (service == nullptr)
        throw BaseKit::ArgumentException("Asio service is invalid!");
    assert((action) && "Action function is invalid!");
    if (!action)
        throw BaseKit::ArgumentException("Action function is invalid!");
}

Timer::Timer(const std::shared_ptr<Service>& service, const std::function<void(bool)>& action, const BaseKit::Timespan& timespan)
    : _service(service),
    _io_service(_service->GetAsioService()),
    _strand(*_io_service),
    _strand_required(_service->IsStrandRequired()),
    _timer(*_io_service, timespan.chrono()),
    _action(action)
{
    assert((service != nullptr) && "Asio service is invalid!");
    if (service == nullptr)
        throw BaseKit::ArgumentException("Asio service is invalid!");
    assert((action) && "Action function is invalid!");
    if (!action)
        throw BaseKit::ArgumentException("Action function is invalid!");
}

BaseKit::UtcTime Timer::expire_time() const
{
    return BaseKit::UtcTime(_timer.expires_at());
}

BaseKit::Timespan Timer::expire_timespan() const
{
    return BaseKit::Timespan(_timer.expires_from_now());
}

bool Timer::Setup(const BaseKit::UtcTime& time)
{
    asio::error_code ec;
    _timer.expires_at(time.chrono(), ec);

    // Check for error
    if (ec)
    {
        SendError(ec);
        return false;
    }

    return true;
}

bool Timer::Setup(const BaseKit::Timespan& timespan)
{
    asio::error_code ec;
    _timer.expires_from_now(timespan.chrono(), ec);

    // Check for error
    if (ec)
    {
        SendError(ec);
        return false;
    }

    return true;
}

bool Timer::Setup(const std::function<void(bool)>& action)
{
    assert((action) && "Action function is invalid!");
    if (!action)
        return false;

    _action = action;
    return true;
}

bool Timer::Setup(const std::function<void(bool)>& action, const BaseKit::UtcTime& time)
{
    assert((action) && "Action function is invalid!");
    if (!action)
        return false;

    _action = action;
    return Setup(time);
}

bool Timer::Setup(const std::function<void(bool)>& action, const BaseKit::Timespan& timespan)
{
    assert((action) && "Action function is invalid!");
    if (!action)
        return false;

    _action = action;
    return Setup(timespan);
}

bool Timer::WaitAsync()
{
    auto self(this->shared_from_this());
    auto async_wait_handler = [this, self](const std::error_code& ec)
    {
        // Call the timer aborted handler
        if (ec == asio::error::operation_aborted)
            SendTimer(true);

        // Check for error
        if (ec)
        {
            SendError(ec);
            return;
        }

        // Call the timer expired handler
        SendTimer(false);
    };
    if (_strand_required)
        _timer.async_wait(bind_executor(_strand, async_wait_handler));
    else
        _timer.async_wait(async_wait_handler);

    return true;
}

bool Timer::WaitSync()
{
    asio::error_code ec;
    _timer.wait(ec);

    // Call the timer aborted handler
    if (ec == asio::error::operation_aborted)
        SendTimer(true);

    // Check for error
    if (ec)
    {
        SendError(ec);
        return false;
    }

    // Call the timer expired handler
    SendTimer(false);

    return true;
}

bool Timer::Cancel()
{
    asio::error_code ec;
    _timer.cancel(ec);

    // Check for error
    if (ec)
    {
        SendError(ec);
        return false;
    }

    return true;
}

void Timer::SendError(std::error_code ec)
{
    // Skip Asio abort error
    if (ec == asio::error::operation_aborted)
        return;

    onError(ec.value(), ec.category().name(), ec.message());
}

void Timer::SendTimer(bool canceled)
{
    // Call the timer handler
    onTimer(canceled);

    // Call the timer action
    if (_action)
        _action(canceled);
}

} // namespace Asio
} // namespace NetUtil
