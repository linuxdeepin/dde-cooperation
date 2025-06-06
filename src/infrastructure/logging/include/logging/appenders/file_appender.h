// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOGGING_APPENDERS_FILE_APPENDER_H
#define LOGGING_APPENDERS_FILE_APPENDER_H

#include "logging/appender.h"

#include "filesystem/filesystem.h"

#include <atomic>

namespace Logging {

//! File appender
/*!
    File appender writes the given logging record into the file with
    the given file name. In case of any IO error this appender will
    lost the logging record, but try to recover from fail in a short
    interval of 100ms.

    Not thread-safe.
*/
class FileAppender : public Appender
{
public:
    //! Initialize the appender with a given file, truncate/append and auto-flush flags
    /*!
         \param file - Logging file
         \param truncate - Truncate flag (default is false)
         \param auto_flush - Auto-flush flag (default is false)
         \param auto_start - Auto-start flag (default is true)
    */
    explicit FileAppender(const BaseKit::Path& file, bool truncate = false, bool auto_flush = false, bool auto_start = true);
    FileAppender(const FileAppender&) = delete;
    FileAppender(FileAppender&&) = delete;
    virtual ~FileAppender();

    FileAppender& operator=(const FileAppender&) = delete;
    FileAppender& operator=(FileAppender&&) = delete;

    // Implementation of Appender
    bool IsStarted() const noexcept override { return _started; }
    bool Start() override;
    bool Stop() override;
    void AppendRecord(Record& record) override;
    void Flush() override;

private:
    std::atomic<bool> _started{false};
    BaseKit::Timestamp _retry{0};
    BaseKit::File _file;
    bool _truncate;
    bool _auto_flush;

    //! Prepare the file for writing
    /*
        - If the file is opened and ready to write immediately returns true
        - If the last retry was earlier than 100ms immediately returns false
        - If the file is closed try to open it for writing, returns true/false

        \return 'true' if the file was successfully prepared, 'false' if the file failed to be prepared
    */
    bool PrepareFile();
    //! Close the file
    /*
        \return 'true' if the file was successfully closed, 'false' if the file failed to close
    */
    bool CloseFile();
};

} // namespace Logging

/*! \example file.cpp File logger example */

#endif // LOGGING_APPENDERS_FILE_APPENDER_H
