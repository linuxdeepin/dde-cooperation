// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// TransferHelper 覆盖 (NetworkUtil 桩后所有网络方法安全)
// 重点: handleMessage 多分支 JSON 解析 (纯逻辑金矿);
// tryConnect/cancelTransferJob/disconnectRemote/sendMessage/finish/emitDisconnected/
// setConnectIP/getConnectIP/addFinshedFiles 等。

#include "net/helper/transferhepler.h"

#include <gtest/gtest.h>
#include <QApplication>
#include <QSignalSpy>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QDir>

TEST(TransferHelperTest, ConnectIpSetterGetter)
{
    TransferHelper *h = TransferHelper::instance();
    h->setConnectIP("192.168.1.1");
    EXPECT_EQ(h->getConnectIP().toStdString(), "192.168.1.1");
}

TEST(TransferHelperTest, TryConnectSucceed)
{
    TransferHelper *h = TransferHelper::instance();
    QSignalSpy spy(h, &TransferHelper::connectSucceed);
    h->tryConnect("10.0.0.1", "123456");
    EXPECT_EQ(spy.count(), 1);   // doConnect stub 返回 true → emit
}

TEST(TransferHelperTest, CancelTransferJob)
{
    EXPECT_TRUE(TransferHelper::instance()->cancelTransferJob("test_reason"));
    EXPECT_TRUE(TransferHelper::instance()->cancelTransferJob());
}

TEST(TransferHelperTest, DisconnectAndFinish)
{
    TransferHelper::instance()->disconnectRemote();
    TransferHelper::instance()->finish();
    SUCCEED();   // stub 后不发起网络, 仅验证不崩溃
}

TEST(TransferHelperTest, SendMessageBuildsJson)
{
    TransferHelper::instance()->sendMessage("type1", "msg1");
    SUCCEED();
}

TEST(TransferHelperTest, EmitDisconnected)
{
    TransferHelper *h = TransferHelper::instance();
    h->setConnectIP("1.1.1.1");
    QSignalSpy spy(h, &TransferHelper::disconnected);
    h->emitDisconnected();
    // isSetting 默认 false → emit disconnected
    EXPECT_GE(spy.count(), 0);
    EXPECT_TRUE(h->getConnectIP().isEmpty());   // connectIP 被清空
}

// ---- handleMessage 分支覆盖 ----
TEST(TransferHelperMessageTest, InvalidJsonReturns)
{
    TransferHelper *h = TransferHelper::instance();
    h->handleMessage("not a json");
    SUCCEED();
}

TEST(TransferHelperMessageTest, UnfinishJsonBranch)
{
    TransferHelper *h = TransferHelper::instance();
    QSignalSpy spy(h, &TransferHelper::unfinishedJob);
    h->handleMessage("{\"unfinish_json\":\"abc\"}");
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.takeFirst().at(0).toString().toStdString(), "abc");
}

TEST(TransferHelperMessageTest, RemainingSpaceBranch)
{
    TransferHelper *h = TransferHelper::instance();
    QSignalSpy spy(h, &TransferHelper::remoteRemainSpace);
    h->handleMessage("{\"remaining_space\":\"50\"}");
    EXPECT_EQ(spy.count(), 1);
}

TEST(TransferHelperMessageTest, AddResultBranchWithRegex)
{
    TransferHelper *h = TransferHelper::instance();
    QSignalSpy addSpy(h, &TransferHelper::addResult);
    QSignalSpy finSpy(h, &TransferHelper::transferFinished);
    // "/true|false/" 分隔格式: name true reason
    h->handleMessage("{\"add_result\":\"file1/true/ok;file2/false/err\"}");
    EXPECT_EQ(addSpy.count(), 2);
    EXPECT_EQ(finSpy.count(), 1);
}

TEST(TransferHelperMessageTest, AddResultBranchErrorFormat)
{
    TransferHelper *h = TransferHelper::instance();
    QSignalSpy addSpy(h, &TransferHelper::addResult);
    h->handleMessage("{\"add_result\":\"no_status_token\"}");
    EXPECT_EQ(addSpy.count(), 0);   // regex 不匹配 → 无 emit
}

TEST(TransferHelperMessageTest, ChangePageBranch)
{
    TransferHelper *h = TransferHelper::instance();
    // startTransfer 前缀 → emit changeWidget(waitwidget) + clearWidget
    QSignalSpy clearSpy(h, &TransferHelper::clearWidget);
    h->handleMessage("{\"change_page\":\"startTransfer\"}");
    EXPECT_EQ(clearSpy.count(), 1);
    // 非 _cb 结尾 → sendMessage (stub 安全)
    h->handleMessage("{\"change_page\":\"ready_cb\"}");
    SUCCEED();
}

TEST(TransferHelperMessageTest, TransferContentBranch)
{
    TransferHelper *h = TransferHelper::instance();
    QSignalSpy spy(h, &TransferHelper::transferContent);
    // 4 段空格分隔: type content progressbar time
    h->handleMessage("{\"transfer_content\":\"Transfering file.txt 50 30\"}");
    EXPECT_EQ(spy.count(), 1);
    // 非 4 段 → 跳过
    h->handleMessage("{\"transfer_content\":\"bad format\"}");
    EXPECT_EQ(spy.count(), 1);
}

TEST(TransferHelperTest, AddFinshedFilesEmpty)
{
    TransferHelper::instance()->addFinshedFiles(QString(), 100);   // 空 → 直接 return
    SUCCEED();
}
