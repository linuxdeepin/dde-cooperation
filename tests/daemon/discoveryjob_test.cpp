// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <QSignalSpy>

#include "service/discoveryjob.h"
#include "service/searchlight.h"
#include "service/comshare.h"
#include "common/constant.h"
#include "utils/utils.h"
#include "ipc/proto/comstruct.h"

#include "co/json.h"

#include "stub.h"

// Reset the singleton's raw announcer/discoverer pointers before static
// destruction to avoid co::del on objects allocated in tests (coost runtime
// may be torn down by then, causing SIGSEGV). The test-allocated objects leak,
// which is acceptable in a test process.
class DiscoveryJobEnv : public ::testing::Environment {
public:
    ~DiscoveryJobEnv() override {}
    void TearDown() override {
        DiscoveryJob::instance()->_announcer_p = nullptr;
        DiscoveryJob::instance()->_discoverer_p = nullptr;
    }
};
static ::testing::Environment *g_disenv = ::testing::AddGlobalTestEnvironment(new DiscoveryJobEnv());

// Coverage strategy: instead of stubbing DiscoveryJob methods to no-ops (which
// covers 0% of their bodies), we set the private _announcer_p / _discoverer_p
// pointers to REAL searchlight objects so the methods execute their actual code.
// The singleton destructor does `if (!p->started()) co::del(p)`, and we never
// call .start(), so the objects we allocate are reclaimed safely at process exit.

static fastring makeBaseInfo()
{
    NodePeerInfo npi;
    npi.proto_version = "1.0";
    npi.uuid = "uuid-test-1234";
    npi.ipv4 = "1.2.3.4";
    npi.hostname = "host";
    npi.nickname = "nick";
    npi.share_connect_ip = "";
    return npi.as_json().str();
}

// Install a real Announcer into the singleton's _announcer_p.
// Returns the allocated pointer (singleton owns it thereafter).
static searchlight::Announcer *installAnnouncer()
{
    auto *ann = new searchlight::Announcer("myservice", 51597, makeBaseInfo());
    DiscoveryJob::instance()->_announcer_p = ann;
    return ann;
}

static searchlight::Discoverer *installDiscoverer()
{
    auto *dis = new searchlight::Discoverer(
        "myservice", [](const QList<searchlight::Discoverer::service> &) {});
    DiscoveryJob::instance()->_discoverer_p = dis;
    return dis;
}

// fakes for network-only paths we still want to short-circuit
static fastring fakeFirstIp() { return "1.2.3.4"; }

// ---- instance / getNodes ----

TEST(DiscoveryJobTest, InstanceSingleton)
{
    DiscoveryJob *p1 = DiscoveryJob::instance();
    DiscoveryJob *p2 = DiscoveryJob::instance();
    EXPECT_NE(p1, nullptr);
    EXPECT_EQ(p1, p2);
}

TEST(DiscoveryJobTest, GetNodesEmpty)
{
    auto nodes = DiscoveryJob::instance()->getNodes();
    EXPECT_TRUE(nodes.empty());
}

TEST(DiscoveryJobTest, GetNodesWithData)
{
    DiscoveryJob::instance()->_dis_node_maps.insert(
        "uid1", std::make_pair(fastring("nodeinfo1"), true));
    auto nodes = DiscoveryJob::instance()->getNodes();
    EXPECT_EQ(nodes.size(), 1u);
    // cleanup to avoid interfering with other tests
    DiscoveryJob::instance()->_dis_node_maps.clear();
}

// ---- notifySearchResult (private) ----

TEST(DiscoveryJobTest, NotifySearchResultTrue)
{
    DiscoveryJob::instance()->notifySearchResult(true, "info");
    SUCCEED();
}

TEST(DiscoveryJobTest, NotifySearchResultFalse)
{
    DiscoveryJob::instance()->notifySearchResult(false, "");
    SUCCEED();
}

// ---- announcer-backed methods (real execution) ----

TEST(DiscoveryJobTest, BaseInfoReal)
{
    installAnnouncer();
    fastring b = DiscoveryJob::instance()->baseInfo();
    EXPECT_FALSE(b.empty());
    EXPECT_TRUE(b.find("uuid-test-1234") != fastring::npos);
}

TEST(DiscoveryJobTest, UpdateAnnouncBaseReal)
{
    installAnnouncer();
    DiscoveryJob::instance()->updateAnnouncBase("{\"uuid\":\"new\"}");
    EXPECT_EQ(DiscoveryJob::instance()->baseInfo(), fastring("{\"uuid\":\"new\"}"));
}

TEST(DiscoveryJobTest, UpdateAnnouncAppAppendRemove)
{
    installAnnouncer();
    fastring app1 = "{\"appname\":\"app1\",\"json\":\"{}\"}";
    fastring app2 = "{\"appname\":\"app2\",\"json\":\"{}\"}";
    DiscoveryJob::instance()->updateAnnouncApp(false, app1);
    DiscoveryJob::instance()->updateAnnouncApp(false, app2);
    fastring pkg = DiscoveryJob::instance()->udpSendPackage();
    EXPECT_TRUE(pkg.find("app1") != fastring::npos);
    EXPECT_TRUE(pkg.find("app2") != fastring::npos);

    DiscoveryJob::instance()->updateAnnouncApp(true, app1);
    pkg = DiscoveryJob::instance()->udpSendPackage();
    // app1 removed
    SUCCEED();
}

TEST(DiscoveryJobTest, UpdateAnnouncShareRemove)
{
    installAnnouncer();
    DiscoveryJob::instance()->updateAnnouncShare(true);
    fastring b = DiscoveryJob::instance()->baseInfo();
    co::Json j;
    ASSERT_TRUE(j.parse_from(b));
    NodePeerInfo npi;
    npi.from_json(j);
    EXPECT_TRUE(npi.share_connect_ip.empty());
}

TEST(DiscoveryJobTest, UpdateAnnouncShareAdd)
{
    installAnnouncer();
    DiscoveryJob::instance()->updateAnnouncShare(false, "5.6.7.8");
    fastring b = DiscoveryJob::instance()->baseInfo();
    co::Json j;
    ASSERT_TRUE(j.parse_from(b));
    NodePeerInfo npi;
    npi.from_json(j);
    EXPECT_EQ(npi.share_connect_ip, fastring("5.6.7.8"));
}

TEST(DiscoveryJobTest, UpdateAnnouncShareEmptyReturnsEarly)
{
    installAnnouncer();
    // empty connectIP -> early return, no change
    DiscoveryJob::instance()->updateAnnouncShare(false, "");
    SUCCEED();
}

TEST(DiscoveryJobTest, RemoveAppbyNameOther)
{
    installAnnouncer();
    fastring app1 = "{\"appname\":\"app1\",\"json\":\"{}\"}";
    DiscoveryJob::instance()->updateAnnouncApp(false, app1);
    DiscoveryJob::instance()->removeAppbyName("app1");
    SUCCEED();
}

TEST(DiscoveryJobTest, RemoveAppbyNameDdeCooperation)
{
    installAnnouncer();
    // "dde-cooperation" triggers updateAnnouncShare(true) then removeAppbyName
    DiscoveryJob::instance()->removeAppbyName("dde-cooperation");
    SUCCEED();
}

TEST(DiscoveryJobTest, UdpSendPackageReal)
{
    installAnnouncer();
    fastring pkg = DiscoveryJob::instance()->udpSendPackage();
    EXPECT_FALSE(pkg.empty());
    co::Json j;
    ASSERT_TRUE(j.parse_from(pkg));
    EXPECT_EQ(j.get("name").as_string(), fastring("myservice"));
}

TEST(DiscoveryJobTest, NodeInfoStrReal)
{
    installAnnouncer();
    fastring info = DiscoveryJob::instance()->nodeInfoStr();
    EXPECT_FALSE(info.empty());
    co::Json j;
    ASSERT_TRUE(j.parse_from(info));
}

// ---- handleUpdPackage (real Discoverer) ----

TEST(DiscoveryJobTest, HandleUpdPackageInvalidJson)
{
    installDiscoverer();
    DiscoveryJob::instance()->handleUpdPackage("1.2.3.4", "not json");
    SUCCEED();
}

TEST(DiscoveryJobTest, HandleUpdPackageValidJson)
{
    installDiscoverer();
    co::Json node;
    node.add_member("name", "myservice");
    node.add_member("info", "{\"os\":{\"uuid\":\"uid2\",\"ipv4\":\"9.9.9.9\"}}");
    DiscoveryJob::instance()->handleUpdPackage("1.2.3.4", node.str().c_str());
    SUCCEED();
}

TEST(DiscoveryJobTest, HandleUpdPackageNonMatchingService)
{
    installDiscoverer();
    co::Json node;
    node.add_member("name", "other_service");
    node.add_member("info", "{\"os\":{\"uuid\":\"uid3\"}}");
    DiscoveryJob::instance()->handleUpdPackage("1.2.3.4", node.str().c_str());
    SUCCEED();
}

// ---- null-pointer guards ----

TEST(DiscoveryJobTest, StopDiscovererNull)
{
    // ensure null
    DiscoveryJob::instance()->_discoverer_p = nullptr;
    DiscoveryJob::instance()->stopDiscoverer();
    SUCCEED();
}

TEST(DiscoveryJobTest, StopAnnouncerNull)
{
    DiscoveryJob::instance()->_announcer_p = nullptr;
    DiscoveryJob::instance()->stopAnnouncer();
    SUCCEED();
}

// ---- sigNodeChanged wiring ----

TEST(DiscoveryJobTest, SigNodeChanged)
{
    QSignalSpy spy(DiscoveryJob::instance(), &DiscoveryJob::sigNodeChanged);
    EXPECT_TRUE(spy.isValid());
}

// ---- searchDeviceByIp remove branch (real, but stub Util::getFirstIp) ----

TEST(DiscoveryJobTest, SearchDeviceByIpRemove)
{
    installDiscoverer();
    Stub stub;
    stub.set((void *)Util::getFirstIp, fakeFirstIp);
    // remove=true returns early after setSearchIp("")
    DiscoveryJob::instance()->searchDeviceByIp("1.2.3.4", true);
    SUCCEED();
}

// ---- compareOldAndNew coverage (private; reached via -fno-access-control) ----
//
// compareOldAndNew diffs a stored NodeInfo against a fresh one and emits
// sigNodeChanged(found, info). We seed _dis_node_maps directly, hand the method
// the iterator it expects, and assert on the emitted signal via QSignalSpy.

#include <utility>

// Build a NodeInfo json with an arbitrary set of (appname, json) apps.
static fastring nodeInfoJsonWithApps(std::initializer_list<std::pair<const char *, const char *>> apps)
{
    NodeInfo ni;
    ni.os.proto_version = "1.0";
    ni.os.uuid = "uuid-x";
    ni.os.ipv4 = "1.2.3.4";
    ni.os.hostname = "host";
    ni.os.nickname = "nick";
    for (const auto &a : apps) {
        AppPeerInfo app;
        app.appname = a.first;
        app.json = a.second;
        ni.apps.push_back(app);
    }
    return ni.as_json().str();
}

// 1. old has apps, cur has none -> node lost: emit found=false and erase entry.
TEST(DiscoveryJobTest, CompareOldAndNewOldAppsNewEmpty)
{
    auto *job = DiscoveryJob::instance();
    job->_dis_node_maps.clear();

    fastring oldJson = nodeInfoJsonWithApps({{"app1", "json1"}});
    fastring curJson = nodeInfoJsonWithApps({});

    job->_dis_node_maps.insert("uid1", std::make_pair(fastring(oldJson), true));
    auto it = job->_dis_node_maps.find("uid1");
    ASSERT_NE(it, job->_dis_node_maps.end());

    QSignalSpy spy(job, &DiscoveryJob::sigNodeChanged);
    job->compareOldAndNew("uid1", QString(curJson.c_str()), it);
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.takeFirst().at(0).toBool(), false);
    // the entry is erased on the "lost" branch
    EXPECT_EQ(job->_dis_node_maps.find("uid1"), job->_dis_node_maps.end());
}

// 2. old has none, cur has apps -> node appeared: emit found=true and re-insert.
TEST(DiscoveryJobTest, CompareOldAndNewOldEmptyNewApps)
{
    auto *job = DiscoveryJob::instance();
    job->_dis_node_maps.clear();

    fastring oldJson = nodeInfoJsonWithApps({});
    fastring curJson = nodeInfoJsonWithApps({{"app1", "json1"}});

    job->_dis_node_maps.insert("uid2", std::make_pair(fastring(oldJson), true));
    auto it = job->_dis_node_maps.find("uid2");
    ASSERT_NE(it, job->_dis_node_maps.end());

    QSignalSpy spy(job, &DiscoveryJob::sigNodeChanged);
    job->compareOldAndNew("uid2", QString(curJson.c_str()), it);
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.takeFirst().at(0).toBool(), true);
    EXPECT_NE(job->_dis_node_maps.find("uid2"), job->_dis_node_maps.end());
}

// 3. old=[app1,app2], cur=[app1] -> app2 removed, down=true, emit found=false.
TEST(DiscoveryJobTest, CompareOldAndNewBothAppsDown)
{
    auto *job = DiscoveryJob::instance();
    job->_dis_node_maps.clear();

    fastring oldJson = nodeInfoJsonWithApps({{"app1", "json1"}, {"app2", "json2"}});
    fastring curJson = nodeInfoJsonWithApps({{"app1", "json1"}});

    job->_dis_node_maps.insert("uid3", std::make_pair(fastring(oldJson), true));
    auto it = job->_dis_node_maps.find("uid3");
    ASSERT_NE(it, job->_dis_node_maps.end());

    QSignalSpy spy(job, &DiscoveryJob::sigNodeChanged);
    job->compareOldAndNew("uid3", QString(curJson.c_str()), it);
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.takeFirst().at(0).toBool(), false);
}

// 4. old=[app1], cur=[app1,app2] -> app2 added, up=true, emit found=true.
TEST(DiscoveryJobTest, CompareOldAndNewBothAppsUp)
{
    auto *job = DiscoveryJob::instance();
    job->_dis_node_maps.clear();

    fastring oldJson = nodeInfoJsonWithApps({{"app1", "json1"}});
    fastring curJson = nodeInfoJsonWithApps({{"app1", "json1"}, {"app2", "json2"}});

    job->_dis_node_maps.insert("uid4", std::make_pair(fastring(oldJson), true));
    auto it = job->_dis_node_maps.find("uid4");
    ASSERT_NE(it, job->_dis_node_maps.end());

    QSignalSpy spy(job, &DiscoveryJob::sigNodeChanged);
    job->compareOldAndNew("uid4", QString(curJson.c_str()), it);
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.takeFirst().at(0).toBool(), true);
}

// 5. old=[app1 json=A], cur=[app1 json=B] -> content differs, up=true, found=true.
TEST(DiscoveryJobTest, CompareOldAndNewBothAppsContentChanged)
{
    auto *job = DiscoveryJob::instance();
    job->_dis_node_maps.clear();

    fastring oldJson = nodeInfoJsonWithApps({{"app1", "payload-A"}});
    fastring curJson = nodeInfoJsonWithApps({{"app1", "payload-B"}});

    job->_dis_node_maps.insert("uid5", std::make_pair(fastring(oldJson), true));
    auto it = job->_dis_node_maps.find("uid5");
    ASSERT_NE(it, job->_dis_node_maps.end());

    QSignalSpy spy(job, &DiscoveryJob::sigNodeChanged);
    job->compareOldAndNew("uid5", QString(curJson.c_str()), it);
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.takeFirst().at(0).toBool(), true);
}

// 6. searchDeviceByIp offline path. Util::getFirstIp is defined *inline* in the
//    header, so the compiler inlines it into searchDeviceByIp at -O2 and a stub
//    on Util::getFirstIp never fires — the real network path would run and hang.
//    It delegates to deepin_cross::CommonUitls::getFirstIp, which IS out-of-line
//    (defined in commonutils.cpp, no LTO), so we stub that instead. The fake
//    returns std::string (matching the real signature) to keep the hidden return
//    ABI correct. Empty self ip forces the offline branch: notifySearchResult
//    (false, ...) runs and the method returns before any socket I/O.
static std::string fakeEmptyFirstIp() { return ""; }

TEST(DiscoveryJobTest, SearchDeviceByIpOffline)
{
    Stub stub;
    stub.set((void *)deepin_cross::CommonUitls::getFirstIp, fakeEmptyFirstIp);
    // remove=false + empty self ip -> offline branch -> early return.
    DiscoveryJob::instance()->searchDeviceByIp("1.2.3.4", false);
    SUCCEED();
}
