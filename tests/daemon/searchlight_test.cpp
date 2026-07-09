// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include "service/searchlight.h"
#include "co/json.h"
#include "utils/utils.h"
#include "common/commonutils.h"
#include "stub.h"

// Announcer basic operations
TEST(SearchlightAnnouncerTest, ConstructAndBaseInfo)
{
    searchlight::Announcer ann("myservice", 51597, "{\"uuid\":\"abc\"}");
    EXPECT_EQ(ann.baseInfo(), fastring("{\"uuid\":\"abc\"}"));
}

TEST(SearchlightAnnouncerTest, UpdateBase)
{
    searchlight::Announcer ann("svc", 51597, "{\"uuid\":\"old\"}");
    ann.updateBase("{\"uuid\":\"new\"}");
    EXPECT_EQ(ann.baseInfo(), fastring("{\"uuid\":\"new\"}"));
}

TEST(SearchlightAnnouncerTest, AppendApp)
{
    searchlight::Announcer ann("svc", 51597, "{\"uuid\":\"base\"}");
    fastring app1 = "{\"appname\":\"app1\",\"json\":\"{}\"}";
    ann.appendApp(app1);
    SUCCEED();
}

TEST(SearchlightAnnouncerTest, AppendAppDuplicate)
{
    searchlight::Announcer ann("svc", 51597, "{\"uuid\":\"base\"}");
    fastring app1 = "{\"appname\":\"app1\",\"json\":\"{}\"}";
    ann.appendApp(app1);
    ann.appendApp(app1); // duplicate replaces old
    SUCCEED();
}

TEST(SearchlightAnnouncerTest, RemoveApp)
{
    searchlight::Announcer ann("svc", 51597, "{\"uuid\":\"base\"}");
    fastring app1 = "{\"appname\":\"app1\",\"json\":\"{}\"}";
    ann.appendApp(app1);
    ann.removeApp(app1);
    SUCCEED();
}

TEST(SearchlightAnnouncerTest, RemoveAppByName)
{
    searchlight::Announcer ann("svc", 51597, "{\"uuid\":\"base\"}");
    ann.appendApp("{\"appname\":\"app1\",\"json\":\"{}\"}");
    ann.appendApp("{\"appname\":\"app2\",\"json\":\"{}\"}");
    ann.removeAppbyName("app1");
    SUCCEED();
}

TEST(SearchlightAnnouncerTest, RemoveAppByNameInvalid)
{
    searchlight::Announcer ann("svc", 51597, "{\"uuid\":\"base\"}");
    ann.appendApp("notjson");
    ann.removeAppbyName("app1");
    SUCCEED();
}

TEST(SearchlightAnnouncerTest, SameAppInvalidJson)
{
    searchlight::Announcer ann("svc", 51597, "{\"uuid\":\"base\"}");
    fastring bad = "notjson";
    int idx = ann.sameApp(bad); // private, but -fno-access-control
    EXPECT_EQ(idx, -1);
}

TEST(SearchlightAnnouncerTest, UdpSendPackage)
{
    searchlight::Announcer ann("svc", 51597, "{\"uuid\":\"base\"}");
    fastring pkg = ann.udpSendPackage();
    EXPECT_FALSE(pkg.empty());
    co::Json j;
    EXPECT_TRUE(j.parse_from(pkg));
    EXPECT_EQ(j.get("name").as_string(), fastring("svc"));
}

TEST(SearchlightAnnouncerTest, UdpSendPackageWithApps)
{
    searchlight::Announcer ann("svc", 51597, "{\"uuid\":\"base\"}");
    ann.appendApp("{\"appname\":\"app1\",\"json\":\"{}\"}");
    fastring pkg = ann.udpSendPackage();
    EXPECT_FALSE(pkg.empty());
}

TEST(SearchlightAnnouncerTest, NodeInfoStr)
{
    searchlight::Announcer ann("svc", 51597, "{\"uuid\":\"base\"}");
    fastring info = ann.nodeInfoStr();
    EXPECT_FALSE(info.empty());
    co::Json j;
    EXPECT_TRUE(j.parse_from(info));
}

TEST(SearchlightAnnouncerTest, StartedAndExit)
{
    searchlight::Announcer ann("svc", 51597, "{\"uuid\":\"base\"}");
    // _stop initialized true, started() returns !_stop = false
    EXPECT_FALSE(ann.started());
    ann.exit();
    EXPECT_FALSE(ann.started());
    EXPECT_FALSE(ann.finished());
}

// Discoverer
TEST(SearchlightDiscovererTest, Construct)
{
    searchlight::Discoverer::on_services_changed_t cb = [](const QList<searchlight::Discoverer::service>&){};
    searchlight::Discoverer dis("myservice", cb);
    EXPECT_FALSE(dis.started());
    dis.exit();
    EXPECT_FALSE(dis.started());
}

TEST(SearchlightDiscovererTest, ServiceComparison)
{
    searchlight::Discoverer::service a, b, c;
    a.service_name = "s1"; a.endpoint = "1.1.1.1:80";
    b.service_name = "s1"; b.endpoint = "1.1.1.1:80";
    c.service_name = "s2"; c.endpoint = "2.2.2.2:80";
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
    EXPECT_TRUE(a < c);
}

TEST(SearchlightDiscovererTest, SetSearchIp)
{
    searchlight::Discoverer::on_services_changed_t cb = [](const QList<searchlight::Discoverer::service>&){};
    searchlight::Discoverer dis("svc", cb);
    dis.setSearchIp("192.168.1.1");
    dis.setSearchIp("");
    SUCCEED();
}

TEST(SearchlightDiscovererTest, HandleMessageInvalid)
{
    searchlight::Discoverer::on_services_changed_t cb = [](const QList<searchlight::Discoverer::service>&){};
    searchlight::Discoverer dis("svc", cb);
    dis.handle_message("invalid json", "1.1.1.1:80");
    SUCCEED();
}

TEST(SearchlightDiscovererTest, HandleMessageValidNoListenMatch)
{
    searchlight::Discoverer::on_services_changed_t cb = [](const QList<searchlight::Discoverer::service>&){};
    searchlight::Discoverer dis("myservice", cb);
    co::Json node;
    node.add_member("name", "other_service");
    node.add_member("info", "{\"os\":{\"uuid\":\"uid1\"}}");
    dis.handle_message(node.str(), "1.1.1.1:80", false);
    SUCCEED();
}

// ---- helpers for handle_message self-ip stubbing ----
// We stub CommonUitls::getFirstIp (the real, out-of-line static function that
// Util::getFirstIp delegates to) so the interception holds whether or not the
// inline Util::getFirstIp wrapper was inlined at the searchlight.cpp call site.
static std::string fakeFirstIpOther() { return std::string("10.0.0.99"); }
static std::string fakeFirstIpSame() { return std::string("1.2.3.4"); }

// ---- Discoverer::handleChanges (private, reachable via -fno-access-control) ----

// handleChanges with an unknown endpoint inserts a brand-new service.
TEST(SearchlightDiscovererTest, HandleChangesInsertsNewService)
{
    searchlight::Discoverer::on_services_changed_t cb = [](const QList<searchlight::Discoverer::service>&){};
    searchlight::Discoverer dis("myservice", cb);
    dis.handleChanges("1.2.3.4", "{\"os\":{\"uuid\":\"u1\"}}", 1000);
    EXPECT_FALSE(dis._discovered_services.value("1.2.3.4").isNull());
    EXPECT_EQ(dis._discovered_services.size(), 1);
}

// handleChanges on an existing endpoint refreshes last_seen with the passed time
// and, when info differs, queues a flags=2 change entry.
TEST(SearchlightDiscovererTest, HandleChangesUpdatesExisting)
{
    searchlight::Discoverer::on_services_changed_t cb = [](const QList<searchlight::Discoverer::service>&){};
    searchlight::Discoverer dis("myservice", cb);
    dis.handleChanges("1.2.3.4", "{\"os\":{\"uuid\":\"u1\"}}", 1000);
    dis.handleChanges("1.2.3.4", "{\"os\":{\"uuid\":\"u2\"}}", 2000);
    auto ser = dis._discovered_services.value("1.2.3.4");
    ASSERT_FALSE(ser.isNull());
    EXPECT_EQ(ser->last_seen, static_cast<qint64>(2000));
    EXPECT_EQ(ser->info, fastring("{\"os\":{\"uuid\":\"u2\"}}"));
    bool foundChange = false;
    for (const auto &s : dis._change_sevices) {
        if (s.endpoint == "1.2.3.4" && s.flags == 2) foundChange = true;
    }
    EXPECT_TRUE(foundChange);
}

// An empty endpoint is rejected early (error log); nothing is inserted.
TEST(SearchlightDiscovererTest, HandleChangesEmptyEndpoint)
{
    searchlight::Discoverer::on_services_changed_t cb = [](const QList<searchlight::Discoverer::service>&){};
    searchlight::Discoverer dis("myservice", cb);
    dis.handleChanges("", "info", 1000);
    EXPECT_TRUE(dis._discovered_services.isEmpty());
}

// ---- Discoverer::remove_idle_services (private, reachable via -fno-access-control) ----

// A service whose last_seen predates the deadline (_timer.ms() - 3000) is evicted
// and queued in _change_sevices with flags=1 (offline). We plant a far-past
// last_seen by using the existing-service update path of handleChanges, which
// assigns the caller-supplied time to last_seen.
TEST(SearchlightDiscovererTest, RemoveIdleServicesRemovesStale)
{
    searchlight::Discoverer::on_services_changed_t cb = [](const QList<searchlight::Discoverer::service>&){};
    searchlight::Discoverer dis("myservice", cb);
    dis.handleChanges("1.2.3.4", "{\"os\":{\"uuid\":\"u1\"}}", 0);
    // existing-service update path stores the passed time (-10000) into last_seen,
    // which is well below the negative deadline for a freshly constructed timer.
    dis.handleChanges("1.2.3.4", "{\"os\":{\"uuid\":\"u2\"}}", -10000);
    ASSERT_EQ(dis._discovered_services.size(), 1);
    bool removed = dis.remove_idle_services();
    EXPECT_TRUE(removed);
    EXPECT_TRUE(dis._discovered_services.isEmpty());
    bool foundOffline = false;
    for (const auto &s : dis._change_sevices) {
        if (s.endpoint == "1.2.3.4" && s.flags == 1) foundOffline = true;
    }
    EXPECT_TRUE(foundOffline);
}

// A freshly inserted service (last_seen ~ now) survives remove_idle_services.
TEST(SearchlightDiscovererTest, RemoveIdleServicesKeepsRecent)
{
    searchlight::Discoverer::on_services_changed_t cb = [](const QList<searchlight::Discoverer::service>&){};
    searchlight::Discoverer dis("myservice", cb);
    dis.handleChanges("1.2.3.4", "{\"os\":{\"uuid\":\"u1\"}}", 0);
    ASSERT_EQ(dis._discovered_services.size(), 1);
    bool removed = dis.remove_idle_services();
    EXPECT_FALSE(removed);
    EXPECT_EQ(dis._discovered_services.size(), 1);
    EXPECT_FALSE(dis._discovered_services.value("1.2.3.4").isNull());
}

// ---- Discoverer::handle_message matching branch ----

// handle_message dispatches to handleChanges when the message matches the listen
// prefix and the source ip is not the self ip. The matcher is
// message.starts_with(_listen_for_service); a valid JSON object begins with '{',
// so we set the listen name to "{" to actually reach the match branch, and
// isFilter=false makes the network-filter sub-condition trivially true.
TEST(SearchlightDiscovererTest, HandleMessageValidMatchingService)
{
    searchlight::Discoverer::on_services_changed_t cb = [](const QList<searchlight::Discoverer::service>&){};
    searchlight::Discoverer dis("{", cb);
    Stub stub;
    stub.set((void *)deepin_cross::CommonUitls::getFirstIp, fakeFirstIpOther);
    co::Json os;
    os.add_member("uuid", "x");
    os.add_member("ipv4", "1.2.3.4");
    co::Json info;
    info.add_member("os", os);
    co::Json node;
    node.add_member("name", "myservice");
    node.add_member("info", info);
    dis.handle_message(node.str(), "1.2.3.4:80", false);
    EXPECT_FALSE(dis._discovered_services.value("1.2.3.4").isNull());
}

// When the advertised ipv4 equals the self ip the message is ignored.
TEST(SearchlightDiscovererTest, HandleMessageMatchingServiceSameIp)
{
    searchlight::Discoverer::on_services_changed_t cb = [](const QList<searchlight::Discoverer::service>&){};
    searchlight::Discoverer dis("{", cb);
    Stub stub;
    stub.set((void *)deepin_cross::CommonUitls::getFirstIp, fakeFirstIpSame);
    co::Json os;
    os.add_member("uuid", "x");
    os.add_member("ipv4", "1.2.3.4"); // equals the stubbed self ip
    co::Json info;
    info.add_member("os", os);
    co::Json node;
    node.add_member("name", "myservice");
    node.add_member("info", info);
    dis.handle_message(node.str(), "1.2.3.4:80", false);
    EXPECT_TRUE(dis._discovered_services.isEmpty());
}

// ---- Announcer additional branches ----

// appendApp with non-JSON: sameApp returns -1, but the value is still pushed.
TEST(SearchlightAnnouncerTest, AppendAppInvalidJson)
{
    searchlight::Announcer ann("svc", 51597, "{\"uuid\":\"base\"}");
    ann.appendApp("notjson");
    EXPECT_EQ(ann._app_infos.size(), static_cast<size_t>(1));
}

// removeApp with non-JSON: sameApp returns -1, so nothing is removed.
TEST(SearchlightAnnouncerTest, RemoveAppInvalidJson)
{
    searchlight::Announcer ann("svc", 51597, "{\"uuid\":\"base\"}");
    ann.appendApp("{\"appname\":\"app1\",\"json\":\"{}\"}");
    ann.removeApp("notjson");
    EXPECT_EQ(ann._app_infos.size(), static_cast<size_t>(1));
}

// removeAppbyName does not adjust the loop index after _app_infos.remove(i),
// so when several same-name entries are present it leaves one behind. Plant two
// same-name entries directly (appendApp would dedup) to exercise the buggy loop.
TEST(SearchlightAnnouncerTest, RemoveAppByNameMultiple)
{
    searchlight::Announcer ann("svc", 51597, "{\"uuid\":\"base\"}");
    ann._app_infos.push_back("{\"appname\":\"dup\",\"json\":\"{}\"}");
    ann._app_infos.push_back("{\"appname\":\"dup\",\"json\":\"{}\"}");
    ASSERT_EQ(ann._app_infos.size(), static_cast<size_t>(2));
    ann.removeAppbyName("dup");
    EXPECT_EQ(ann._app_infos.size(), static_cast<size_t>(1));
}

// sameApp on a valid app whose name matches an existing entry returns its index.
TEST(SearchlightAnnouncerTest, SameAppValid)
{
    searchlight::Announcer ann("svc", 51597, "{\"uuid\":\"base\"}");
    ann.appendApp("{\"appname\":\"app1\",\"json\":\"{}\"}");
    int idx = ann.sameApp("{\"appname\":\"app1\",\"json\":\"{}\"}");
    EXPECT_EQ(idx, 0);
}

// nodeInfoStr builds its "apps" array by iterating _app_infos; the loop body is
// only exercised when at least one app has been appended.
TEST(SearchlightAnnouncerTest, NodeInfoStrWithApps)
{
    searchlight::Announcer ann("svc", 51597, "{\"uuid\":\"base\"}");
    ann.appendApp("{\"appname\":\"app1\",\"json\":\"{}\"}");
    fastring info = ann.nodeInfoStr();
    EXPECT_FALSE(info.empty());
    co::Json j;
    EXPECT_TRUE(j.parse_from(info));
}

// sameApp drops un-parseable entries it encounters while scanning. Because it
// does not fix up the index after _app_infos.remove(i), a corrupt entry placed
// before the match causes the scan to end before re-checking the shifted element,
// so it returns -1 while still removing the bad entry.
TEST(SearchlightAnnouncerTest, SameAppCleansBadEntries)
{
    searchlight::Announcer ann("svc", 51597, "{\"uuid\":\"base\"}");
    ann._app_infos.push_back("notjson");
    ann._app_infos.push_back("{\"appname\":\"app1\",\"json\":\"{}\"}");
    ASSERT_EQ(ann._app_infos.size(), static_cast<size_t>(2));
    int idx = ann.sameApp("{\"appname\":\"app1\",\"json\":\"{}\"}");
    EXPECT_EQ(idx, -1);
    EXPECT_EQ(ann._app_infos.size(), static_cast<size_t>(1));
}
