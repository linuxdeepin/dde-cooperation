// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#include <catch2/catch_all.hpp>
#include <logging/record.h>
#include <logging/appenders/syslog_appender.h>
using namespace Logging;
namespace { Record mk(Level l,const std::string&m){Record r;r.timestamp=1;r.thread=1;r.level=l;r.logger="S";r.message=m;return r;} }
TEST_CASE("SyslogAppender no throw","[appender][syslog]"){
    SyslogAppender app; REQUIRE(app.Start());
    Record r=mk(Level::WARN,"syslog-line"); app.AppendRecord(r); app.Flush(); app.Stop();
}
TEST_CASE("SyslogAppender stop before start safe","[appender][syslog]"){
    SyslogAppender app; REQUIRE_FALSE(app.Stop());
}
