// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QHostInfo>
#include <QSettings>
#include <QString>
#include <QTemporaryDir>

#include "lib/cooperation/core/share/cooconfig.h"

class CooConfigTest : public ::testing::Test {
protected:
    QTemporaryDir tmpDir;
    QString iniPath;
    CooConfig *cfg = nullptr;
    QSettings *settings = nullptr;

    void SetUp() override
    {
        iniPath = tmpDir.path() + "/test.ini";
        settings = new QSettings(iniPath, QSettings::IniFormat);
        cfg = new CooConfig(settings);
    }

    void TearDown() override
    {
        // CooConfig destructor does NOT delete m_pSettings (only calls
        // saveSettings()), so we own and delete the QSettings here.
        delete cfg;
        cfg = nullptr;
        delete settings;
        settings = nullptr;
    }
};

TEST_F(CooConfigTest, DefaultPortIsStandardSharePort)
{
    // loadSettings() defaults port to UNI_SHARE_SERVER_PORT (24802) when the
    // settings file has no "port" key.
    EXPECT_EQ(cfg->port(), 24802);
}

TEST_F(CooConfigTest, DefaultCryptoEnabledIsTrue)
{
    // loadSettings() defaults cryptoEnabled to true.
    EXPECT_TRUE(cfg->getCryptoEnabled());
}

TEST_F(CooConfigTest, DefaultScreenNameIsLocalHostName)
{
    // loadSettings() defaults screenName to QHostInfo::localHostName().
    EXPECT_EQ(cfg->screenName().toStdString(),
              QHostInfo::localHostName().toStdString());
}

TEST_F(CooConfigTest, SetPortAndGetReturnsValue)
{
    cfg->setPort(24800);
    cfg->saveSettings();
    EXPECT_EQ(cfg->port(), 24800);
}

TEST_F(CooConfigTest, SetServerIpAndGetReturnsValue)
{
    cfg->setServerIp("192.168.1.100");
    cfg->saveSettings();
    EXPECT_EQ(cfg->serverIp().toStdString(), "192.168.1.100");
}

TEST_F(CooConfigTest, SetScreenNameAndGetReturnsValue)
{
    cfg->setScreenName("MyScreen");
    cfg->saveSettings();
    EXPECT_EQ(cfg->screenName().toStdString(), "MyScreen");
}

TEST_F(CooConfigTest, CryptoEnabledRoundTrip)
{
    cfg->setCryptoEnabled(false);
    cfg->saveSettings();
    EXPECT_FALSE(cfg->getCryptoEnabled());
    cfg->setCryptoEnabled(true);
    cfg->saveSettings();
    EXPECT_TRUE(cfg->getCryptoEnabled());
}

TEST_F(CooConfigTest, ProfileDirRoundTrip)
{
    cfg->setProfileDir("/tmp/test-profile");
    EXPECT_EQ(cfg->profileDir().toStdString(), "/tmp/test-profile");
}

TEST_F(CooConfigTest, SettingsPersistAcrossInstances)
{
    // Write values through one CooConfig instance, then create a new instance
    // pointing at the same QSettings file and confirm the values survive.
    cfg->setPort(12345);
    cfg->setServerIp("10.0.0.5");
    cfg->setScreenName("Persisted");
    cfg->setCryptoEnabled(false);
    cfg->saveSettings();
    settings->sync();

    CooConfig cfg2(settings);
    EXPECT_EQ(cfg2.port(), 12345);
    EXPECT_EQ(cfg2.serverIp().toStdString(), "10.0.0.5");
    EXPECT_EQ(cfg2.screenName().toStdString(), "Persisted");
    EXPECT_FALSE(cfg2.getCryptoEnabled());
}
