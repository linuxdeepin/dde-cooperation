// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOGGING_PROCESSORS_SYNC_PROCESSOR_H
#define LOGGING_PROCESSORS_SYNC_PROCESSOR_H

#include "logging/processor.h"

#include "threads/critical_section.h"

namespace Logging {

//! Synchronous logging processor
/*!
    Synchronous logging processor process the given logging record
    under the critical section to avoid races in not thread-safe
    layouts, filters and appenders.

    Thread-safe.
*/
class SyncProcessor : public Processor
{
public:
    //! Initialize synchronous logging processor with a given layout interface
    /*!
         \param layout - Logging layout interface
    */
    explicit SyncProcessor(const std::shared_ptr<Layout>& layout) : Processor(layout) {}
    SyncProcessor(const SyncProcessor&) = delete;
    SyncProcessor(SyncProcessor&&) = delete;
    virtual ~SyncProcessor() = default;

    SyncProcessor& operator=(const SyncProcessor&) = delete;
    SyncProcessor& operator=(SyncProcessor&&) = delete;

    // Implementation of Processor
    bool ProcessRecord(Record& record) override;
    void Flush() override;

private:
    BaseKit::CriticalSection _lock;
};

} // namespace Logging

/*! \example sync.cpp Synchronous logger processor example */

#endif // LOGGING_PROCESSORS_SYNC_PROCESSOR_H
