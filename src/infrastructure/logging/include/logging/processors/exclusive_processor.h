// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOGGING_PROCESSORS_EXCLUSIVE_PROCESSOR_H
#define LOGGING_PROCESSORS_EXCLUSIVE_PROCESSOR_H

#include "logging/processor.h"

namespace Logging {

//! Exclusive logging processor
/*!
    Exclusive logging processor filters out the given logging record
    and process it exclusively without providing the record to other
    processors.

    Not thread-safe.
*/
class ExclusiveProcessor : public Processor
{
public:
    //! Initialize exclusive logging processor with a given layout interface
    /*!
         \param layout - Logging layout interface
    */
    explicit ExclusiveProcessor(const std::shared_ptr<Layout>& layout) : Processor(layout) {}
    ExclusiveProcessor(const ExclusiveProcessor&) = delete;
    ExclusiveProcessor(ExclusiveProcessor&&) = delete;
    virtual ~ExclusiveProcessor() = default;

    ExclusiveProcessor& operator=(const ExclusiveProcessor&) = delete;
    ExclusiveProcessor& operator=(ExclusiveProcessor&&) = delete;

    // Implementation of Processor
    bool ProcessRecord(Record& record) override;
};

} // namespace Logging

#endif // LOGGING_PROCESSORS_EXCLUSIVE_PROCESSOR_H
