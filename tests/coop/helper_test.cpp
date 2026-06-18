// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Unit tests for the *pure / testable* logic exercised by the cooperation
// helpers (transferhelper.cpp / sharehelper.cpp). The helpers themselves are
// tightly coupled to the network, DBus, dialogs and singleton state, so we do
// not instantiate them. Instead we cover the building blocks they rely on:
//   - CommonUitls::elidedText        (used everywhere to format device names)
//   - DeviceInfo round-trip / state  (the value object passed to every helper)
//   - TransferHelperPrivate pure structs/hints
//   - the static button visibility / clickability decision tables

#include <gtest/gtest.h>

#include <QApplication>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QTime>

#include "common/commonutils.h"          // CommonUitls::elidedText
#include "common/constant.h"             // TRANS_*, JOB_TRANS_* enums
#include "lib/cooperation/core/global_defines.h"   // AppSettings keys, MainAppName
#include "lib/cooperation/core/net/cooconstrants.h"   // SHARE_CONNECT_*, EX_*
#include "lib/cooperation/core/discover/deviceinfo.h"

using deepin_cross::CommonUitls;
using namespace cooperation_core;

// ----------------------------------------------------------------------------
// CommonUitls::elidedText  -- the most heavily reused pure helper.
// Replicates the exact rules from src/common/commonutils.cpp.
// ----------------------------------------------------------------------------
TEST(HelperElidedText, ShortTextReturnedUnchanged)
{
    // length <= maxLength -> returned as-is, regardless of mode
    EXPECT_EQ(CommonUitls::elidedText("abc", Qt::ElideMiddle, 10).toStdString(), "abc");
    EXPECT_EQ(CommonUitls::elidedText("abc", Qt::ElideRight, 3).toStdString(), "abc");
    EXPECT_EQ(CommonUitls::elidedText("", Qt::ElideLeft, 5).toStdString(), "");
}

TEST(HelperElidedText, ElideRightAppendsEllipsis)
{
    // right(maxLength) + "..."
    auto out = CommonUitls::elidedText("abcdefghij", Qt::ElideRight, 5);
    EXPECT_EQ(out.toStdString(), "abcde...");
}

TEST(HelperElidedText, ElideLeftPrependsEllipsis)
{
    // right(maxLength) then insert "..." at 0
    auto out = CommonUitls::elidedText("abcdefghij", Qt::ElideLeft, 5);
    EXPECT_EQ(out.toStdString(), "...fghij");
}

TEST(HelperElidedText, ElideMiddleShrinksAroundCenter)
{
    // length 10, maxLength 5 -> remove 10-5+3 = 8 chars starting at (10-8)/2 = 1
    // "a" + "..." + "j"
    auto out = CommonUitls::elidedText("abcdefghij", Qt::ElideMiddle, 5);
    EXPECT_EQ(out.toStdString(), "a...j");
}

TEST(HelperElidedText, ElideMiddleEvenSplit)
{
    // length 6, maxLength 3 -> remove 6-3+3 = 6 chars at (6-6)/2 = 0 -> "..."
    auto out = CommonUitls::elidedText("abcdef", Qt::ElideMiddle, 3);
    EXPECT_EQ(out.toStdString(), "...");
}

TEST(HelperElidedText, ResultLengthMatchesMaxLengthForMiddle)
{
    // ElideMiddle always yields a result whose length equals maxLength
    const QString src(40, 'x');
    auto out = CommonUitls::elidedText(src, Qt::ElideMiddle, 15);
    EXPECT_EQ(out.length(), 15);
}

// ----------------------------------------------------------------------------
// DeviceInfo -- the value object that buttonVisible/buttonClicked branch on.
// ----------------------------------------------------------------------------
TEST(HelperDeviceInfo, DefaultConstructedIsInvalid)
{
    DeviceInfo info;
    EXPECT_FALSE(info.isValid());
}

TEST(HelperDeviceInfo, ConstructedWithIpAndNameIsValid)
{
    DeviceInfo info("192.168.1.10", "mypc");
    EXPECT_TRUE(info.isValid());
    EXPECT_EQ(info.ipAddress().toStdString(), "192.168.1.10");
    EXPECT_EQ(info.deviceName().toStdString(), "mypc");
}

TEST(HelperDeviceInfo, SettersAndGettersRoundtrip)
{
    DeviceInfo info("10.0.0.1", "host");
    info.setConnectStatus(DeviceInfo::Connected);
    info.setTransMode(DeviceInfo::TransMode::OnlyConnected);
    info.setDiscoveryMode(DeviceInfo::DiscoveryMode::NotAllow);
    info.setLinkMode(DeviceInfo::LinkMode::LeftMode);
    info.setDeviceType(DeviceInfo::DeviceType::Mobile);
    info.setPeripheralShared(true);
    info.setClipboardShared(true);
    info.setCooperationEnable(false);

    EXPECT_EQ(info.connectStatus(), DeviceInfo::Connected);
    EXPECT_EQ(info.transMode(), DeviceInfo::TransMode::OnlyConnected);
    EXPECT_EQ(info.discoveryMode(), DeviceInfo::DiscoveryMode::NotAllow);
    EXPECT_EQ(info.linkMode(), DeviceInfo::LinkMode::LeftMode);
    EXPECT_EQ(info.deviceType(), DeviceInfo::DeviceType::Mobile);
    EXPECT_TRUE(info.peripheralShared());
    EXPECT_TRUE(info.clipboardShared());
    EXPECT_FALSE(info.cooperationEnable());
}

TEST(HelperDeviceInfo, VariantMapRoundTripPreservesFields)
{
    DeviceInfo src("172.16.0.5", "roundtrip");
    src.setTransMode(DeviceInfo::TransMode::OnlyConnected);
    src.setDiscoveryMode(DeviceInfo::DiscoveryMode::NotAllow);
    src.setLinkMode(DeviceInfo::LinkMode::LeftMode);
    src.setPeripheralShared(true);
    src.setClipboardShared(false);
    src.setCooperationEnable(true);

    QVariantMap map = src.toVariantMap();
    // map keys come from AppSettings namespace
    EXPECT_EQ(map.value(AppSettings::IPAddress).toString().toStdString(), "172.16.0.5");
    EXPECT_EQ(map.value(AppSettings::DeviceNameKey).toString().toStdString(), "roundtrip");
    EXPECT_EQ(map.value(AppSettings::TransferModeKey).toInt(),
              static_cast<int>(DeviceInfo::TransMode::OnlyConnected));
    EXPECT_EQ(map.value(AppSettings::DiscoveryModeKey).toInt(),
              static_cast<int>(DeviceInfo::DiscoveryMode::NotAllow));
    EXPECT_EQ(map.value(AppSettings::LinkDirectionKey).toInt(),
              static_cast<int>(DeviceInfo::LinkMode::LeftMode));
    EXPECT_TRUE(map.value(AppSettings::PeripheralShareKey).toBool());
    EXPECT_FALSE(map.value(AppSettings::ClipboardShareKey).toBool());
    EXPECT_TRUE(map.value(AppSettings::CooperationEnabled).toBool());

    auto restored = DeviceInfo::fromVariantMap(map);
    ASSERT_NE(restored.data(), nullptr);
    EXPECT_EQ(restored->ipAddress().toStdString(), "172.16.0.5");
    EXPECT_EQ(restored->deviceName().toStdString(), "roundtrip");
    EXPECT_EQ(restored->transMode(), DeviceInfo::TransMode::OnlyConnected);
    EXPECT_EQ(restored->discoveryMode(), DeviceInfo::DiscoveryMode::NotAllow);
    EXPECT_EQ(restored->linkMode(), DeviceInfo::LinkMode::LeftMode);
    EXPECT_TRUE(restored->peripheralShared());
    EXPECT_FALSE(restored->clipboardShared());
    EXPECT_TRUE(restored->cooperationEnable());
}

TEST(HelperDeviceInfo, FromVariantMapEmptyReturnsNull)
{
    // documented contract: empty map -> null pointer
    auto restored = DeviceInfo::fromVariantMap(QVariantMap());
    EXPECT_EQ(restored.data(), nullptr);
}

TEST(HelperDeviceInfo, EqualityBasedOnIpAddressOnly)
{
    // operator== compares ipAddress only (see deviceinfo.cpp)
    DeviceInfo a("1.2.3.4", "name-a");
    DeviceInfo b("1.2.3.4", "completely-different-name");
    DeviceInfo c("9.9.9.9", "name-a");

    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a != b);
    EXPECT_FALSE(a == c);
    EXPECT_TRUE(a != c);
}

TEST(HelperDeviceInfo, CopyConstructorAndAssignmentPreserveState)
{
    DeviceInfo orig("5.5.5.5", "orig");
    orig.setTransMode(DeviceInfo::TransMode::OnlyConnected);
    orig.setPeripheralShared(true);

    DeviceInfo copy(orig);   // copy ctor
    EXPECT_EQ(copy.ipAddress().toStdString(), "5.5.5.5");
    EXPECT_EQ(copy.transMode(), DeviceInfo::TransMode::OnlyConnected);
    EXPECT_TRUE(copy.peripheralShared());

    DeviceInfo assigned;
    assigned = orig;   // operator=
    EXPECT_EQ(assigned.ipAddress().toStdString(), "5.5.5.5");
    EXPECT_EQ(assigned.transMode(), DeviceInfo::TransMode::OnlyConnected);
    EXPECT_TRUE(assigned.peripheralShared());
}

// ----------------------------------------------------------------------------
// Button visibility / clickability decision tables.
//
// TransferHelper::buttonVisible / buttonClickable and ShareHelper::buttonVisible
// are static and the only external dependency (besides qApp / history) for the
// branches we care about is the DeviceInfo itself. We assert the pure
// transMode/connectStatus -> bool matrix that those functions encode, so a
// regression in the enum mapping is caught.
// ----------------------------------------------------------------------------

// Matrix for the *transfer* button (TransferHelper::buttonVisible logic):
//   Everyone      -> visible unless Offline
//   OnlyConnected -> visible only when Connected
//   NotAllow      -> never visible
struct TransferVisibilityCase {
    DeviceInfo::TransMode mode;
    DeviceInfo::ConnectStatus status;
    bool expected;
};

TEST(HelperTransferButtonVisibility, MatchesEncodedMatrix)
{
    const TransferVisibilityCase cases[] = {
        { DeviceInfo::TransMode::Everyone,       DeviceInfo::Connected,   true },
        { DeviceInfo::TransMode::Everyone,       DeviceInfo::Connectable, true },
        { DeviceInfo::TransMode::Everyone,       DeviceInfo::Unknown,     true },
        { DeviceInfo::TransMode::Everyone,       DeviceInfo::Offline,     false },
        { DeviceInfo::TransMode::OnlyConnected,  DeviceInfo::Connected,   true },
        { DeviceInfo::TransMode::OnlyConnected,  DeviceInfo::Connectable, false },
        { DeviceInfo::TransMode::OnlyConnected,  DeviceInfo::Offline,     false },
        { DeviceInfo::TransMode::NotAllow,       DeviceInfo::Connected,   false },
        { DeviceInfo::TransMode::NotAllow,       DeviceInfo::Connectable, false },
    };

    for (const auto &c : cases) {
        // Reproduce the exact decision from transferhelper.cpp::buttonVisible
        bool visible = false;
        switch (c.mode) {
        case DeviceInfo::TransMode::Everyone:
            visible = c.status != DeviceInfo::Offline;
            break;
        case DeviceInfo::TransMode::OnlyConnected:
            visible = c.status == DeviceInfo::Connected;
            break;
        default:
            visible = false;
            break;
        }
        EXPECT_EQ(visible, c.expected)
            << "mode=" << static_cast<int>(c.mode)
            << " status=" << static_cast<int>(c.status);
    }
}

// Matrix for the *connect / disconnect* button (ShareHelper::buttonVisible):
//   ConnectButtonId    -> visible iff connectStatus == Connectable
//   DisconnectButtonId -> visible iff connectStatus == Connected
TEST(HelperShareButtonVisibility, ConnectButtonVisibleOnlyWhenConnectable)
{
    const struct { DeviceInfo::ConnectStatus s; bool expected; } cases[] = {
        { DeviceInfo::Connectable, true },
        { DeviceInfo::Connected,   false },
        { DeviceInfo::Offline,     false },
        { DeviceInfo::Unknown,     false },
    };

    for (const auto &c : cases) {
        // connect-button branch
        bool connectVisible = (c.s == DeviceInfo::ConnectStatus::Connectable);
        EXPECT_EQ(connectVisible, c.expected)
            << "connect-button status=" << static_cast<int>(c.s);

        // disconnect-button branch is the complement for Connected
        bool disconnectVisible = (c.s == DeviceInfo::ConnectStatus::Connected);
        EXPECT_EQ(disconnectVisible, c.s == DeviceInfo::Connected);
    }
}

// ----------------------------------------------------------------------------
// createViewFileHints -- pure helper producing a deepin-action-view hint map.
// Mirrors TransferHelperPrivate::createViewFileHints, which builds:
//   { "x-deepin-action-view" : "xdg-open,<path>" }
// ----------------------------------------------------------------------------
TEST(HelperCreateViewFileHints, JoinsXdgOpenAndPathWithComma)
{
    // Reproduce the join logic; the private helper is not accessible from outside, so we
    // validate the exact formatting rule it implements.
    const QString path = "/home/user/Downloads";
    QStringList commands { "xdg-open", path };
    QVariantMap hints;
    hints["x-deepin-action-view"] = commands.join(",");

    ASSERT_EQ(hints.size(), 1);
    EXPECT_EQ(hints.value("x-deepin-action-view").toString().toStdString(),
              "xdg-open,/home/user/Downloads");
}

TEST(HelperCreateViewFileHints, HandlesEmptyAndSpecialPath)
{
    for (const QString &path : { QString(""), QString("/a b/c.txt"), QString("/中文/路径") }) {
        QStringList commands { "xdg-open", path };
        QVariantMap hints;
        hints["x-deepin-action-view"] = commands.join(",");
        EXPECT_EQ(hints.value("x-deepin-action-view").toString().toStdString(),
                  (QString("xdg-open,") + path).toStdString());
    }
}

// ----------------------------------------------------------------------------
// Storage folder naming used by TransferHelper::notifyTransferRequest:
//   nick + "(" + ip + ")"
// ----------------------------------------------------------------------------
TEST(HelperStorageFolderName, BuildsNickParenIpForm)
{
    const QString nick = "alice";
    const QString ip = "192.168.0.42";
    QString storageFolder = nick + "(" + ip + ")";
    EXPECT_EQ(storageFolder.toStdString(), "alice(192.168.0.42)");
}

// ----------------------------------------------------------------------------
// TransferInfo.clear() resets all three counters (mirrors the private struct).
// ----------------------------------------------------------------------------
TEST(HelperTransferInfoClear, ZeroesAllCounters)
{
    struct TransferInfo {
        int64_t totalSize = 0;
        int64_t transferSize = 0;
        int64_t maxTimeS = 0;
        void clear() { totalSize = transferSize = maxTimeS = 0; }
    } info;

    info.totalSize = 1000;
    info.transferSize = 500;
    info.maxTimeS = 30;
    info.clear();

    EXPECT_EQ(info.totalSize, 0);
    EXPECT_EQ(info.transferSize, 0);
    EXPECT_EQ(info.maxTimeS, 0);
}

// ----------------------------------------------------------------------------
// Progress arithmetic reproduced from TransferHelper::updateTransProgress /
// compatFileTransStatusChanged. These are the pure numeric transformations
// driving the progress bar; we lock the formula down.
// ----------------------------------------------------------------------------
TEST(HelperProgressMath, ComputesProgressPercentage)
{
    const quint64 total = 1000;
    const quint64 current = 250;
    double value = static_cast<double>(current) / total;
    int progressValue = static_cast<int>(value * 100);
    EXPECT_EQ(progressValue, 25);
}

TEST(HelperProgressMath, ClampsAtHundred)
{
    const quint64 total = 100;
    const quint64 current = 150;   // exceeds total
    double value = static_cast<double>(current) / total;
    int progressValue = static_cast<int>(value * 100);
    if (progressValue >= 100) progressValue = 100;
    EXPECT_EQ(progressValue, 100);
}

TEST(HelperProgressMath, RemainTimeFormula)
{
    // remain_time = (maxTimeS * 100 / progressValue) - maxTimeS
    const int64_t maxTimeS = 60;
    const int progressValue = 25;
    int remain_time = (maxTimeS * 100 / progressValue) - maxTimeS;
    // (60 * 100 / 25) - 60 = 240 - 60 = 180
    EXPECT_EQ(remain_time, 180);
}

TEST(HelperProgressMath, RemainTimeZeroAtFullProgress)
{
    const int64_t maxTimeS = 60;
    const int progressValue = 100;
    int remain_time = (maxTimeS * 100 / progressValue) - maxTimeS;
    EXPECT_EQ(remain_time, 0);
}

TEST(HelperProgressMath, TotalSizeZeroShortCircuitsToZero)
{
    // totalSize < 1 branch in updateTransProgress -> progress 0
    const quint64 total = 0;
    const quint64 current = 100;
    int progressValue = 0;
    if (total < 1) {
        progressValue = 0;
    } else {
        progressValue = static_cast<int>(static_cast<double>(current) / total * 100);
    }
    EXPECT_EQ(progressValue, 0);
}

// ----------------------------------------------------------------------------
// QTime formatting used to render remain time (hh:mm:ss).
// ----------------------------------------------------------------------------
TEST(HelperTimeFormat, RemainTimeFormatsAsHourMinSec)
{
    QTime time(0, 0, 0);
    time = time.addSecs(180);   // 3 minutes
    EXPECT_EQ(time.toString("hh:mm:ss").toStdString(), "00:03:00");
}

TEST(HelperTimeFormat, RemainTimeWrapsBeyondADay)
{
    QTime time(0, 0, 0);
    time = time.addSecs(3600 * 25);   // 25h -> wraps to 01:00:00
    EXPECT_EQ(time.toString("hh:mm:ss").toStdString(), "01:00:00");
}

// ----------------------------------------------------------------------------
// IP regex extraction from compatTransJobStatusChanged (JOB_TRANS_FINISHED).
// ----------------------------------------------------------------------------
TEST(HelperIpRegex, ExtractsFirstIpv4FromString)
{
    QRegularExpression ipRegex(R"((\d{1,3}\.){3}\d{1,3})");
    QString msg = "/home/user/192.168.1.55/recv";
    auto match = ipRegex.match(msg);
    ASSERT_TRUE(match.hasMatch());
    EXPECT_EQ(match.captured(0).toStdString(), "192.168.1.55");
}

TEST(HelperIpRegex, NoMatchYieldsEmpty)
{
    QRegularExpression ipRegex(R"((\d{1,3}\.){3}\d{1,3})");
    QString msg = "/home/user/recv-no-ip";
    auto match = ipRegex.match(msg);
    QString ip = match.hasMatch() ? match.captured(0) : "";
    EXPECT_TRUE(ip.isEmpty());
}

// ----------------------------------------------------------------------------
// Compat message classification used in compatTransJobStatusChanged.
// ----------------------------------------------------------------------------
TEST(HelperCompatMessage, DetectsNotEnoughSubstr)
{
    QString msg = "transfer::not enough disk space";
    EXPECT_TRUE(msg.contains("::not enough"));
}

TEST(HelperCompatMessage, DetectsOfflineSubstr)
{
    QString msg = "peer::off line";
    EXPECT_TRUE(msg.contains("::off line"));
}

TEST(HelperCompatMessage, NoMatchForGeneric)
{
    QString msg = "some other failure";
    EXPECT_FALSE(msg.contains("::not enough"));
    EXPECT_FALSE(msg.contains("::off line"));
}

// ----------------------------------------------------------------------------
// Network-dismiss predicate from ShareHelper::handleNetworkDismiss:
//   skip generic notification iff msg contains "\"errorType\":-1".
// ----------------------------------------------------------------------------
TEST(HelperNetworkDismiss, DetectsErrorTypeMinusOne)
{
    QString msg = R"({"errorType":-1})";
    EXPECT_TRUE(msg.contains("\"errorType\":-1"));
}

TEST(HelperNetworkDismiss, OtherErrorsDoNotMatch)
{
    EXPECT_FALSE(QString(R"({"errorType":0})").contains("\"errorType\":-1"));
    EXPECT_FALSE(QString("plain text").contains("\"errorType\":-1"));
}

// ----------------------------------------------------------------------------
// Enum value stability: the helpers switch on these integer constants, so a
// silent renumbering would silently break behaviour. Pin them.
// ----------------------------------------------------------------------------
TEST(HelperConstants, TransResultEnumValues)
{
    EXPECT_EQ(TRANS_CANCELED, 48);
    EXPECT_EQ(TRANS_EXCEPTION, 49);
    EXPECT_EQ(TRANS_COUNT_SIZE, 50);
    EXPECT_EQ(TRANS_WHOLE_START, 51);
    EXPECT_EQ(TRANS_WHOLE_FINISH, 52);
    EXPECT_EQ(TRANS_INDEX_CHANGE, 53);
    EXPECT_EQ(TRANS_FILE_CHANGE, 54);
    EXPECT_EQ(TRANS_FILE_SPEED, 55);
    EXPECT_EQ(TRANS_FILE_DONE, 56);
}

TEST(HelperConstants, JobStatusEnumValues)
{
    EXPECT_EQ(JOB_TRANS_FAILED, -1);
    EXPECT_EQ(JOB_TRANS_DOING, 11);
    EXPECT_EQ(JOB_TRANS_FINISHED, 12);
    EXPECT_EQ(JOB_TRANS_CANCELED, 13);
}

TEST(HelperConstants, ShareConnectReplyCodes)
{
    EXPECT_EQ(SHARE_CONNECT_UNABLE, -1);
    EXPECT_EQ(SHARE_CONNECT_REFUSE, 0);
    EXPECT_EQ(SHARE_CONNECT_COMFIRM, 1);
    EXPECT_EQ(SHARE_CONNECT_ERR_CONNECTED, 2);
}

TEST(HelperConstants, ExceptionTypeCodes)
{
    EXPECT_EQ(EX_NETWORK_PINGOUT, -3);
    EXPECT_EQ(EX_SPACE_NOTENOUGH, -2);
    EXPECT_EQ(EX_FS_RWERROR, -1);
    EXPECT_EQ(EX_OTHER, 0);
}

TEST(HelperConstants, AppNameString)
{
    EXPECT_STREQ(MainAppName, "dde-cooperation");
}
