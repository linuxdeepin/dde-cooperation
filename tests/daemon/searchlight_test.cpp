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

// ===========================================================================
// SearchlightCov2Test — additional coverage for Announcer dedup / updateBase
// propagation, Discoverer handle_message edge cases, handleChanges update
// branches and remove_idle_services mixed scenarios.
// ===========================================================================

// updateBase changes the base info, and the new value propagates into the
// "os" object emitted by udpSendPackage (there is no separate version field;
// baseInfo() is the canonical state).
TEST(SearchlightCov2Test, UpdateBaseReflectedInUdpPackage)
{
    searchlight::Announcer ann("svc", 51597, "{\"uuid\":\"v1\"}");
    EXPECT_EQ(ann.baseInfo(), fastring("{\"uuid\":\"v1\"}"));
    ann.updateBase("{\"uuid\":\"v2\"}");
    EXPECT_EQ(ann.baseInfo(), fastring("{\"uuid\":\"v2\"}"));

    fastring pkg = ann.udpSendPackage();
    co::Json j;
    ASSERT_TRUE(j.parse_from(pkg));
    EXPECT_EQ(j.get("info", "os", "uuid").as_string(), fastring("v2"));
}

// appendApp with the same appname but different payload replaces the old entry
// (dedup), leaving a single latest entry.
TEST(SearchlightCov2Test, AppendAppDedupDifferentJson)
{
    searchlight::Announcer ann("svc", 51597, "{\"uuid\":\"base\"}");
    fastring a1 = "{\"appname\":\"dup\",\"json\":\"v1\"}";
    fastring a2 = "{\"appname\":\"dup\",\"json\":\"v2\"}";

    ann.appendApp(a1);
    ASSERT_EQ(ann._app_infos.size(), static_cast<size_t>(1));
    ann.appendApp(a2); // same appname -> dedup+replace

    EXPECT_EQ(ann._app_infos.size(), static_cast<size_t>(1));
    EXPECT_EQ(ann._app_infos[0], a2);
}

// removeApp on a name that was never appended leaves the list unchanged.
TEST(SearchlightCov2Test, RemoveAppNonexistent)
{
    searchlight::Announcer ann("svc", 51597, "{\"uuid\":\"base\"}");
    ann.appendApp("{\"appname\":\"app1\",\"json\":\"{}\"}");
    ASSERT_EQ(ann._app_infos.size(), static_cast<size_t>(1));

    ann.removeApp("{\"appname\":\"other\",\"json\":\"{}\"}");

    EXPECT_EQ(ann._app_infos.size(), static_cast<size_t>(1));
}

// appendApp three distinct apps then removeApp the middle one -> size 2 and the
// remaining entries are the first and third.
TEST(SearchlightCov2Test, AppendRemoveMultiple)
{
    searchlight::Announcer ann("svc", 51597, "{\"uuid\":\"base\"}");
    ann.appendApp("{\"appname\":\"a1\",\"json\":\"{}\"}");
    ann.appendApp("{\"appname\":\"a2\",\"json\":\"{}\"}");
    ann.appendApp("{\"appname\":\"a3\",\"json\":\"{}\"}");
    ASSERT_EQ(ann._app_infos.size(), static_cast<size_t>(3));

    ann.removeApp("{\"appname\":\"a2\",\"json\":\"{}\"}");

    ASSERT_EQ(ann._app_infos.size(), static_cast<size_t>(2));
    co::Json j0; ASSERT_TRUE(j0.parse_from(ann._app_infos[0]));
    co::Json j1; ASSERT_TRUE(j1.parse_from(ann._app_infos[1]));
    EXPECT_EQ(j0.get("appname").as_string(), fastring("a1"));
    EXPECT_EQ(j1.get("appname").as_string(), fastring("a3"));
}

// handle_message with valid JSON that is NOT an object (e.g. an array) must be
// rejected gracefully without crashing.
TEST(SearchlightCov2Test, HandleMessageNonObjectJson)
{
    searchlight::Discoverer::on_services_changed_t cb = [](const QList<searchlight::Discoverer::service>&){};
    searchlight::Discoverer dis("myservice", cb);
    dis.handle_message("[]", "1.1.1.1:80");
    EXPECT_TRUE(dis._discovered_services.isEmpty());
}

// handle_message with a JSON object missing the "name" field: name/info resolve
// to empty strings and no service matches the listen prefix.
TEST(SearchlightCov2Test, HandleMessageMissingName)
{
    searchlight::Discoverer::on_services_changed_t cb = [](const QList<searchlight::Discoverer::service>&){};
    searchlight::Discoverer dis("myservice", cb);
    co::Json node;
    node.add_member("info", "{\"os\":{\"uuid\":\"u\"}}");
    dis.handle_message(node.str(), "2.2.2.2:80");
    EXPECT_TRUE(dis._discovered_services.isEmpty());
}

// handle_message with a JSON object missing the "info" field: handled without
// crashing, nothing inserted.
TEST(SearchlightCov2Test, HandleMessageMissingInfo)
{
    searchlight::Discoverer::on_services_changed_t cb = [](const QList<searchlight::Discoverer::service>&){};
    searchlight::Discoverer dis("myservice", cb);
    co::Json node;
    node.add_member("name", "myservice");
    dis.handle_message(node.str(), "3.3.3.3:80");
    EXPECT_TRUE(dis._discovered_services.isEmpty());
}

// handle_message whose "name" does NOT start with the listen prefix is ignored
// (the non-matching branch constructs a throwaway service but inserts nothing).
TEST(SearchlightCov2Test, HandleMessageNameMismatch)
{
    searchlight::Discoverer::on_services_changed_t cb = [](const QList<searchlight::Discoverer::service>&){};
    searchlight::Discoverer dis("myservice", cb); // listen "myservice"
    co::Json node;
    node.add_member("name", "other_service");
    node.add_member("info", "{\"os\":{\"uuid\":\"x\"}}");
    dis.handle_message(node.str(), "4.4.4.4:80", false);
    EXPECT_TRUE(dis._discovered_services.isEmpty());
}

// setSearchIp installs the ip into the static filter; a subsequent
// handle_message whose advertised ipv4 equals that filter and whose payload
// matches the listen prefix ("{") reaches handleChanges, even with isFilter
// enabled. The static filter is reset at the end so other tests are unaffected.
TEST(SearchlightCov2Test, SetSearchIpThenFilterMatch)
{
    searchlight::Discoverer::on_services_changed_t cb = [](const QList<searchlight::Discoverer::service>&){};
    searchlight::Discoverer dis("{", cb); // listen "{" so "{"-prefixed JSON matches
    dis.setSearchIp("192.168.1.1");

    Stub stub;
    stub.set((void *)deepin_cross::CommonUitls::getFirstIp, fakeFirstIpOther); // 10.0.0.99

    co::Json os;
    os.add_member("uuid", "match");
    os.add_member("ipv4", "192.168.1.1"); // == filtered ip
    co::Json info;
    info.add_member("os", os);
    co::Json node;
    node.add_member("name", "myservice");
    node.add_member("info", info);

    dis.handle_message(node.str(), "192.168.1.1:80", true); // isFilter=true
    EXPECT_FALSE(dis._discovered_services.value("192.168.1.1").isNull());

    dis.setSearchIp(""); // reset static filter
}

// handleChanges updating an existing endpoint with empty info refreshes
// last_seen, stores the new info, and queues a flags=2 (info-changed) entry.
TEST(SearchlightCov2Test, HandleChangesUpdateEmptyInfo)
{
    searchlight::Discoverer::on_services_changed_t cb = [](const QList<searchlight::Discoverer::service>&){};
    searchlight::Discoverer dis("myservice", cb);
    dis.handleChanges("9.9.9.9", "{\"os\":{\"uuid\":\"u1\"}}", 1000);
    ASSERT_EQ(dis._discovered_services.size(), 1);

    dis.handleChanges("9.9.9.9", "", 2000);

    auto ser = dis._discovered_services.value("9.9.9.9");
    ASSERT_FALSE(ser.isNull());
    EXPECT_EQ(ser->info, fastring(""));
    EXPECT_EQ(ser->last_seen, static_cast<qint64>(2000));
    bool foundChange = false;
    for (const auto &s : dis._change_sevices) {
        if (s.endpoint == "9.9.9.9" && s.flags == 2) foundChange = true;
    }
    EXPECT_TRUE(foundChange);
}

// handleChanges updating an existing endpoint with identical info refreshes
// last_seen but does NOT queue a flags=2 change entry.
TEST(SearchlightCov2Test, HandleChangesUpdateSameInfoNoChangeFlag)
{
    searchlight::Discoverer::on_services_changed_t cb = [](const QList<searchlight::Discoverer::service>&){};
    searchlight::Discoverer dis("myservice", cb);
    dis.handleChanges("8.8.8.8", "{\"os\":{\"uuid\":\"u\"}}", 1000);

    dis.handleChanges("8.8.8.8", "{\"os\":{\"uuid\":\"u\"}}", 2000);

    auto ser = dis._discovered_services.value("8.8.8.8");
    ASSERT_FALSE(ser.isNull());
    EXPECT_EQ(ser->last_seen, static_cast<qint64>(2000));
    int flags2 = 0;
    for (const auto &s : dis._change_sevices) {
        if (s.endpoint == "8.8.8.8" && s.flags == 2) flags2++;
    }
    EXPECT_EQ(flags2, 0);
}

// handleChanges inserting a second endpoint keeps both entries distinct.
TEST(SearchlightCov2Test, HandleChangesInsertMultiple)
{
    searchlight::Discoverer::on_services_changed_t cb = [](const QList<searchlight::Discoverer::service>&){};
    searchlight::Discoverer dis("myservice", cb);
    dis.handleChanges("1.1.1.1", "{\"os\":{\"uuid\":\"a\"}}", 1000);
    dis.handleChanges("2.2.2.2", "{\"os\":{\"uuid\":\"b\"}}", 1000);

    EXPECT_EQ(dis._discovered_services.size(), 2);
    EXPECT_FALSE(dis._discovered_services.value("1.1.1.1").isNull());
    EXPECT_FALSE(dis._discovered_services.value("2.2.2.2").isNull());
}

// remove_idle_services on an empty discovered map returns false and changes
// nothing.
TEST(SearchlightCov2Test, RemoveIdleServicesEmpty)
{
    searchlight::Discoverer::on_services_changed_t cb = [](const QList<searchlight::Discoverer::service>&){};
    searchlight::Discoverer dis("myservice", cb);
    EXPECT_FALSE(dis.remove_idle_services());
    EXPECT_TRUE(dis._discovered_services.isEmpty());
}

// remove_idle_services with a mix of one stale and one fresh service evicts
// only the stale entry.
TEST(SearchlightCov2Test, RemoveIdleServicesMixed)
{
    searchlight::Discoverer::on_services_changed_t cb = [](const QList<searchlight::Discoverer::service>&){};
    searchlight::Discoverer dis("myservice", cb);

    // service A: drive last_seen far into the past via the update path
    dis.handleChanges("1.1.1.1", "{\"os\":{\"uuid\":\"a\"}}", 0);
    dis.handleChanges("1.1.1.1", "{\"os\":{\"uuid\":\"a2\"}}", -50000); // stale
    // service B: freshly inserted (insert path stores _timer.ms() ~ now)
    dis.handleChanges("2.2.2.2", "{\"os\":{\"uuid\":\"b\"}}", 0);
    ASSERT_EQ(dis._discovered_services.size(), 2);

    bool removed = dis.remove_idle_services();

    EXPECT_TRUE(removed);
    EXPECT_EQ(dis._discovered_services.size(), 1);
    EXPECT_TRUE(dis._discovered_services.value("1.1.1.1").isNull()); // evicted
    EXPECT_FALSE(dis._discovered_services.value("2.2.2.2").isNull()); // kept
}
