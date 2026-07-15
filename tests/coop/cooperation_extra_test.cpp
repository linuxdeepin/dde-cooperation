// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include <QString>
#include <QVariantMap>

#include "lib/cooperation/core/utils/cooperationutil.h"
#include "lib/cooperation/core/discover/deviceinfo.h"
#include "lib/cooperation/core/net/helper/sharehelper.h"
#include "lib/cooperation/core/net/helper/transferhelper.h"
#include "base/baseutils.h"

using cooperation_core::CooperationUtil;
using cooperation_core::DeviceInfo;
using cooperation_core::ShareHelper;
using cooperation_core::TransferHelper;
using ::DeviceInfoPointer;
using deepin_cross::BaseUtils;

TEST(CooperationExtraTest, MainWindowWidgetNullWithoutRegisteredWindow)
{
    EXPECT_EQ(CooperationUtil::instance()->mainWindowWidget(), nullptr);
}

TEST(CooperationExtraTest, ActivateWindowNoCrashWithoutWindow)
{
    EXPECT_NO_FATAL_FAILURE(CooperationUtil::instance()->activateWindow());
}

TEST(CooperationExtraTest, RegisterDeviceOperationNoCrashWithoutWindow)
{
    QVariantMap map;
    map.insert("id", QString("test-op"));
    map.insert("description", QString("desc"));
    EXPECT_NO_FATAL_FAILURE(CooperationUtil::instance()->registerDeviceOperation(map));
}

TEST(CooperationExtraTest, DeviceInfoReturnsExpectedKeys)
{
    QVariantMap info = CooperationUtil::deviceInfo();
    EXPECT_FALSE(info.isEmpty());
    EXPECT_TRUE(info.contains(AppSettings::IPAddress));
    EXPECT_TRUE(info.contains(AppSettings::OSType));
    EXPECT_TRUE(info.contains(AppSettings::DeviceNameKey));
    EXPECT_TRUE(info.contains(AppSettings::DiscoveryModeKey));
    EXPECT_TRUE(info.contains(AppSettings::TransferModeKey));
    EXPECT_TRUE(info.contains(AppSettings::StoragePathKey));
    EXPECT_TRUE(info.contains(AppSettings::LinkDirectionKey));
}

TEST(CooperationExtraTest, DeviceInfoDefaultOsTypeIsOther)
{
    DeviceInfoPointer info(new DeviceInfo);
    EXPECT_EQ(info->osType(), BaseUtils::OS_TYPE::kOther);
}

TEST(CooperationExtraTest, DeviceInfoOsTypeRoundTripAllValues)
{
    DeviceInfoPointer info(new DeviceInfo("1.1.1.1", "host"));

    info->setOsType(BaseUtils::OS_TYPE::kLinux);
    EXPECT_EQ(info->osType(), BaseUtils::OS_TYPE::kLinux);

    info->setOsType(BaseUtils::OS_TYPE::kWindows);
    EXPECT_EQ(info->osType(), BaseUtils::OS_TYPE::kWindows);

    info->setOsType(BaseUtils::OS_TYPE::kMacOS);
    EXPECT_EQ(info->osType(), BaseUtils::OS_TYPE::kMacOS);

    info->setOsType(BaseUtils::OS_TYPE::kOther);
    EXPECT_EQ(info->osType(), BaseUtils::OS_TYPE::kOther);
}

TEST(CooperationExtraTest, DeviceInfoOsTypePersistsViaVariantMap)
{
    DeviceInfoPointer original(new DeviceInfo("2.2.2.2", "host2"));
    original->setOsType(BaseUtils::OS_TYPE::kWindows);

    QVariantMap map = original->toVariantMap();
    EXPECT_EQ(map.value(AppSettings::OSType).toInt(), static_cast<int>(BaseUtils::OS_TYPE::kWindows));

    DeviceInfoPointer restored = DeviceInfo::fromVariantMap(map);
    ASSERT_NE(restored, nullptr);
    EXPECT_EQ(restored->osType(), BaseUtils::OS_TYPE::kWindows);
}

TEST(CooperationExtraTest, DeviceInfoDeviceTypeDirectSetterRoundTrip)
{
    DeviceInfoPointer pc(new DeviceInfo("3.3.3.3", "pc"));
    pc->setDeviceType(DeviceInfo::DeviceType::PC);
    EXPECT_EQ(pc->deviceType(), DeviceInfo::DeviceType::PC);

    DeviceInfoPointer mob(new DeviceInfo("4.4.4.4", "phone"));
    mob->setDeviceType(DeviceInfo::DeviceType::Mobile);
    EXPECT_EQ(mob->deviceType(), DeviceInfo::DeviceType::Mobile);
}

TEST(CooperationExtraTest, ShareHelperInstanceNonNull)
{
    EXPECT_NE(ShareHelper::instance(), nullptr);
}

TEST(CooperationExtraTest, ShareHelperInstanceSamePointer)
{
    auto *a = ShareHelper::instance();
    auto *b = ShareHelper::instance();
    EXPECT_EQ(a, b);
}

TEST(CooperationExtraTest, ShareHelperRegistConnectBtnNoCrash)
{
    EXPECT_NO_FATAL_FAILURE(ShareHelper::instance()->registConnectBtn());
}

TEST(CooperationExtraTest, TransferHelperInstanceNonNull)
{
    EXPECT_NE(TransferHelper::instance(), nullptr);
}

TEST(CooperationExtraTest, TransferHelperInstanceSamePointer)
{
    auto *a = TransferHelper::instance();
    auto *b = TransferHelper::instance();
    EXPECT_EQ(a, b);
}

TEST(CooperationExtraTest, TransferHelperRegistBtnNoCrash)
{
    EXPECT_NO_FATAL_FAILURE(TransferHelper::instance()->registBtn());
}

TEST(CooperationExtraTest, TransferHelperStatusEnumValues)
{
    EXPECT_NE(TransferHelper::Idle, TransferHelper::Confirming);
    EXPECT_NE(TransferHelper::Connecting, TransferHelper::Transfering);
    EXPECT_EQ(TransferHelper::instance()->transferStatus(), TransferHelper::Idle);
}
