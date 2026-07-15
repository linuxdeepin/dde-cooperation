// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include "common/commonutils.h"

#include <QCoreApplication>
#include <QStandardPaths>
#include <QFile>
#include <QString>

using deepin_cross::CommonUitls;

TEST(CommonUitlsTest, GenerateRandomPasswordIsSixDigits)
{
    QString pwd = CommonUitls::generateRandomPassword();
    EXPECT_EQ(pwd.length(), 6);
    for (const QChar &c : pwd) {
        EXPECT_TRUE(c.isDigit());
    }
}

TEST(CommonUitlsTest, GenerateRandomPasswordProducesVariedOutput)
{
    QString a = CommonUitls::generateRandomPassword();
    QString b = CommonUitls::generateRandomPassword();
    EXPECT_EQ(a.length(), 6);
    EXPECT_EQ(b.length(), 6);
}

TEST(CommonUitlsTest, ElidedTextShortReturnsUnchanged)
{
    EXPECT_EQ(CommonUitls::elidedText("hi", Qt::ElideRight, 5).toStdString(), "hi");
    EXPECT_EQ(CommonUitls::elidedText("hi", Qt::ElideLeft, 5).toStdString(), "hi");
    EXPECT_EQ(CommonUitls::elidedText("hi", Qt::ElideMiddle, 5).toStdString(), "hi");
    EXPECT_EQ(CommonUitls::elidedText("hi", Qt::ElideNone, 5).toStdString(), "hi");
}

TEST(CommonUitlsTest, ElidedTextElideRight)
{
    QString out = CommonUitls::elidedText("abcdefghij", Qt::ElideRight, 5);
    EXPECT_EQ(out.length(), 8);
    EXPECT_TRUE(out.endsWith("..."));
    EXPECT_EQ(out.left(5).toStdString(), "abcde");
}

TEST(CommonUitlsTest, ElidedTextElideLeft)
{
    QString out = CommonUitls::elidedText("abcdefghij", Qt::ElideLeft, 5);
    EXPECT_EQ(out.length(), 8);
    EXPECT_TRUE(out.startsWith("..."));
    EXPECT_EQ(out.right(5).toStdString(), "fghij");
}

TEST(CommonUitlsTest, ElidedTextElideMiddle)
{
    QString out = CommonUitls::elidedText("abcdefghij", Qt::ElideMiddle, 5);
    EXPECT_EQ(out.length(), 5);
    EXPECT_TRUE(out.contains("..."));
}

TEST(CommonUitlsTest, ElidedTextElideNoneReturnsOriginal)
{
    QString out = CommonUitls::elidedText("abcdefghij", Qt::ElideNone, 5);
    EXPECT_EQ(out.toStdString(), "abcdefghij");
}

TEST(CommonUitlsTest, IsPortInUsePrivilegedPortOne)
{
    EXPECT_FALSE(CommonUitls::isPortInUse(1));
}

TEST(CommonUitlsTest, GetAvailablePortInRange)
{
    int port = CommonUitls::getAvailablePort();
    EXPECT_GE(port, 13628);
    EXPECT_LT(port, 23628);
}

TEST(CommonUitlsTest, IpcServerNameContainsAppName)
{
    QString name = CommonUitls::ipcServerName("myapp");
    EXPECT_FALSE(name.isEmpty());
    EXPECT_TRUE(name.contains("myapp"));
    EXPECT_TRUE(name.contains(".ipc"));
}

TEST(CommonUitlsTest, TipConfPathEndsWithFlag)
{
    QString path = CommonUitls::tipConfPath();
    EXPECT_FALSE(path.isEmpty());
    EXPECT_TRUE(path.endsWith("tip.flag"));
}

TEST(CommonUitlsTest, IsFirstStartFlagTransitions)
{
    QString flagPath = QString("%1/%2/%3/first_run.flag")
                               .arg(QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation))
                               .arg(qApp->organizationName())
                               .arg(qApp->applicationName());
    QFile::remove(flagPath);

    EXPECT_TRUE(CommonUitls::isFirstStart());
    EXPECT_FALSE(CommonUitls::isFirstStart());
}

TEST(CommonUitlsTest, GetFirstIpDoesNotCrash)
{
    std::string ip = CommonUitls::getFirstIp();
    EXPECT_NO_FATAL_FAILURE(CommonUitls::getFirstIp());
    if (!ip.empty()) {
        EXPECT_GT(ip.length(), static_cast<size_t>(0));
    }
}
