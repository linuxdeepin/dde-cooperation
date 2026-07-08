// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// 测试用 Q_OBJECT subject — 服务端被调对象, 有可远端调用的 slot/signal。
// AUTOMOC OFF 下由 CMakeLists 的 QT6_WRAP_CPP 单独生成 moc。

#ifndef TESTSUBJECT_H
#define TESTSUBJECT_H

#include <QObject>

class TestSubject : public QObject
{
    Q_OBJECT
public:
    explicit TestSubject(QObject *parent = nullptr) : QObject(parent), m_lastArg(0), m_addResult(0) {}

    int lastArg() const { return m_lastArg; }
    int addResult() const { return m_addResult; }

public slots:
    // 供 remoteConnect(remote signal → local method) 接收
    void voidSlot(int v) { m_lastArg = v; }
    // 供 call()/callNoReply() 调用
    void receiveInt(int v) { m_lastArg = v; }
    // 带返回值的 slot, 覆盖 call() 成功 + demarshallResponse 成功路径
    int add(int a, int b)
    {
        m_addResult = a + b;
        return a + b;
    }

signals:
    // 远端信号, 触发服务端 SignalHandler 创建
    void progress(int v);

private:
    int m_lastArg;
    int m_addResult;
};

#endif // TESTSUBJECT_H
