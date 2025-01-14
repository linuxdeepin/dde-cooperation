// SPDX-FileCopyrightText: 2024 - 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ORG_DEEPIN_DDE_COOPERATION_H
#define ORG_DEEPIN_DDE_COOPERATION_H

#include <QThread>
#include <QVariant>
#include <QDebug>
#include <QAtomicPointer>
#include <QAtomicInteger>
#include <DConfig>

class org_deepin_dde_cooperation : public QObject {
    Q_OBJECT

    Q_PROPERTY(double cooperation.discovery.mode READ cooperation.discovery.mode WRITE setCooperation.discovery.mode NOTIFY cooperation.discovery.modeChanged)
    Q_PROPERTY(double cooperation.transfer.mode READ cooperation.transfer.mode WRITE setCooperation.transfer.mode NOTIFY cooperation.transfer.modeChanged)
public:
    explicit org_deepin_dde_cooperation(QThread *thread, const QString &appId, const QString &name, const QString &subpath, QObject *parent = nullptr)
        : QObject(parent) {

        if (!thread->isRunning()) {
            qWarning() << QStringLiteral("Warning: The provided thread is not running.");
        }
        Q_ASSERT(QThread::currentThread() != thread);
        auto worker = new QObject();
        worker->moveToThread(thread);
        QMetaObject::invokeMethod(worker, [=]() {
            auto config = DTK_CORE_NAMESPACE::DConfig::create(appId, name, subpath, nullptr);
            if (!config) {
                qWarning() << QStringLiteral("Failed to create DConfig instance.");
                worker->deleteLater();
                return;
            }
            config->moveToThread(QThread::currentThread());
            initialize(config);
            worker->deleteLater();
        });
    }
    explicit org_deepin_dde_cooperation(QThread *thread, DTK_CORE_NAMESPACE::DConfigBackend *backend, const QString &appId, const QString &name, const QString &subpath, QObject *parent = nullptr)
        : QObject(parent) {

        if (!thread->isRunning()) {
            qWarning() << QStringLiteral("Warning: The provided thread is not running.");
        }
        Q_ASSERT(QThread::currentThread() != thread);
        auto worker = new QObject();
        worker->moveToThread(thread);
        QMetaObject::invokeMethod(worker, [=]() {
            auto config = DTK_CORE_NAMESPACE::DConfig::create(backend, appId, name, subpath, nullptr);
            if (!config) {
                qWarning() << QStringLiteral("Failed to create DConfig instance.");
                worker->deleteLater();
                return;
            }
            config->moveToThread(QThread::currentThread());
            initialize(config);
            worker->deleteLater();
        });
    }
    explicit org_deepin_dde_cooperation(QThread *thread, const QString &name, const QString &subpath, QObject *parent = nullptr)
        : QObject(parent) {

        if (!thread->isRunning()) {
            qWarning() << QStringLiteral("Warning: The provided thread is not running.");
        }
        Q_ASSERT(QThread::currentThread() != thread);
        auto worker = new QObject();
        worker->moveToThread(thread);
        QMetaObject::invokeMethod(worker, [=]() {
            auto config = DTK_CORE_NAMESPACE::DConfig::create(name, subpath, nullptr);
            if (!config) {
                qWarning() << QStringLiteral("Failed to create DConfig instance.");
                worker->deleteLater();
                return;
            }
            config->moveToThread(QThread::currentThread());
            initialize(config);
            worker->deleteLater();
        });
    }
    explicit org_deepin_dde_cooperation(QThread *thread, DTK_CORE_NAMESPACE::DConfigBackend *backend, const QString &name, const QString &subpath, QObject *parent = nullptr)
        : QObject(parent) {

        if (!thread->isRunning()) {
            qWarning() << QStringLiteral("Warning: The provided thread is not running.");
        }
        Q_ASSERT(QThread::currentThread() != thread);
        auto worker = new QObject();
        worker->moveToThread(thread);
        QMetaObject::invokeMethod(worker, [=]() {
            auto config = DTK_CORE_NAMESPACE::DConfig::create(backend, name, subpath, nullptr);
            if (!config) {
                qWarning() << QStringLiteral("Failed to create DConfig instance.");
                worker->deleteLater();
                return;
            }
            config->moveToThread(QThread::currentThread());
            initialize(config);
            worker->deleteLater();
        });
    }
    ~org_deepin_dde_cooperation() {
        if (m_config.loadRelaxed()) {
            m_config.loadRelaxed()->deleteLater();
        }
    }

    double cooperation.discovery.mode() const {
        return p_cooperation.discovery.mode;
    }
    void setCooperation.discovery.mode(const double &value) {
        auto oldValue = p_cooperation.discovery.mode;
        p_cooperation.discovery.mode = value;
        markPropertySet(0);
        if (auto config = m_config.loadRelaxed()) {
            QMetaObject::invokeMethod(config, [this, value]() {
                m_config.loadRelaxed()->setValue(QStringLiteral("cooperation.discovery.mode"), value);
            });
        }
        if (p_cooperation.discovery.mode != oldValue) {
            Q_EMIT cooperation.discovery.modeChanged();
        }
    }
    double cooperation.transfer.mode() const {
        return p_cooperation.transfer.mode;
    }
    void setCooperation.transfer.mode(const double &value) {
        auto oldValue = p_cooperation.transfer.mode;
        p_cooperation.transfer.mode = value;
        markPropertySet(1);
        if (auto config = m_config.loadRelaxed()) {
            QMetaObject::invokeMethod(config, [this, value]() {
                m_config.loadRelaxed()->setValue(QStringLiteral("cooperation.transfer.mode"), value);
            });
        }
        if (p_cooperation.transfer.mode != oldValue) {
            Q_EMIT cooperation.transfer.modeChanged();
        }
    }
Q_SIGNALS:
    void cooperation.discovery.modeChanged();
    void cooperation.transfer.modeChanged();
private:
    void initialize(DTK_CORE_NAMESPACE::DConfig *config) {
        Q_ASSERT(!m_config.loadRelaxed());
        m_config.storeRelaxed(config);
        if (testPropertySet(0)) {
            config->setValue(QStringLiteral("cooperation.discovery.mode"), QVariant::fromValue(p_cooperation.discovery.mode));
        } else {
            updateValue(QStringLiteral("cooperation.discovery.mode"), QVariant::fromValue(p_cooperation.discovery.mode));
        }
        if (testPropertySet(1)) {
            config->setValue(QStringLiteral("cooperation.transfer.mode"), QVariant::fromValue(p_cooperation.transfer.mode));
        } else {
            updateValue(QStringLiteral("cooperation.transfer.mode"), QVariant::fromValue(p_cooperation.transfer.mode));
        }

        connect(config, &DTK_CORE_NAMESPACE::DConfig::valueChanged, this, [this](const QString &key) {
            updateValue(key);
        }, Qt::DirectConnection);
    }
    void updateValue(const QString &key, const QVariant &fallback = QVariant()) {
        Q_ASSERT(QThread::currentThread() == m_config.loadRelaxed()->thread());
        const QVariant &value = m_config.loadRelaxed()->value(key, fallback);
        if (key == QStringLiteral("cooperation.discovery.mode")) {
            auto newValue = qvariant_cast<double>(value);
            QMetaObject::invokeMethod(this, [this, newValue]() {
                if (p_cooperation.discovery.mode != newValue) {
                    p_cooperation.discovery.mode = newValue;
                    Q_EMIT cooperation.discovery.modeChanged();
                }
            });
            return;
        }
        if (key == QStringLiteral("cooperation.transfer.mode")) {
            auto newValue = qvariant_cast<double>(value);
            QMetaObject::invokeMethod(this, [this, newValue]() {
                if (p_cooperation.transfer.mode != newValue) {
                    p_cooperation.transfer.mode = newValue;
                    Q_EMIT cooperation.transfer.modeChanged();
                }
            });
            return;
        }
    }
    inline void markPropertySet(const int index) {
        if (index < 32) {
            m_propertySetStatus0.fetchAndOrOrdered(1 << (index - 0));
            return;
        }
        Q_UNREACHABLE();
    }
    inline bool testPropertySet(const int index) const {
        if (index < 32) {
            return (m_propertySetStatus0.loadRelaxed() & (1 << (index - 0)));
        }
        Q_UNREACHABLE();
    }
    QAtomicPointer<DTK_CORE_NAMESPACE::DConfig> m_config = nullptr;
    double p_cooperation.discovery.mode { 0 };
    double p_cooperation.transfer.mode { 0 };
    QAtomicInteger<quint32> m_propertySetStatus0 = 0;
};

#endif // ORG_DEEPIN_DDE_COOPERATION_H
