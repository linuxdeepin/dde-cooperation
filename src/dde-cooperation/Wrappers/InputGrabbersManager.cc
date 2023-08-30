// SPDX-FileCopyrightText: 2015 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "InputGrabbersManager.h"

#include <QDir>
#include <QTimer>
#include <QDebug>
#include <QFileSystemWatcher>

const static QString inputDevicePath = "/dev/input";

InputGrabbersManager::InputGrabbersManager(QObject *parent)
    : QObject(parent)
    , m_isGrabbing(false)
    , m_timer(new QTimer(this)) {
    initGrabbers();

    m_timer->setSingleShot(true);
    connect(m_timer, &QTimer::timeout, this, &InputGrabbersManager::dirChanged);

    auto fileWatcher = new QFileSystemWatcher(this);
    connect(fileWatcher, &QFileSystemWatcher::directoryChanged, this, [this](const QString &path){
        // one device changed, directoryChanged signal emit many times
        Q_UNUSED(path)
        // use timer delay handle
        if (!m_timer->isActive()) {
            m_timer->start(1 * 1000);
        }
    });

    // add path, directoryChanged signal emit many times, use blockSignals not work
    fileWatcher->addPath(inputDevicePath);
}

void InputGrabbersManager::stopGrab() {
    for (auto &inputGrabber : m_inputGrabbers) {
        inputGrabber->stop();
    }

    m_isGrabbing = false;
}

void InputGrabbersManager::startGrabEvents(const std::weak_ptr<Machine> &machine) {
    for (auto &inputGrabber : m_inputGrabbers) {
        inputGrabber->setMachine(machine);
        inputGrabber->start();
    }

    m_curMachine = machine;
    m_isGrabbing = true;
}

void InputGrabbersManager::removeInputGrabber(const QString &path) {
    if (!m_inputGrabbers.contains(path)) {
        return;
    }

    auto grabber = m_inputGrabbers[path];
    m_inputGrabbers.remove(path);

    if (grabber) {
        grabber->deleteLater();
    }
}

void InputGrabbersManager::addInputGrabber(const QString &devPath) {
    // add new device
    auto grabber = new InputGrabberWrapper(this, devPath);
    if (m_isGrabbing) {
        grabber->setMachine(m_curMachine);
        grabber->start();
    }

    m_inputGrabbers.insert(devPath, grabber);
}

void InputGrabbersManager::initGrabbers() {
    QDir dir(inputDevicePath);
    QFileInfoList fileInfoList = dir.entryInfoList(QDir::System);
    foreach (auto &fileInfo, fileInfoList) {
        if(fileInfo.fileName().startsWith("event")) {
            QString filePath = fileInfo.absoluteFilePath();;
            m_inputGrabbers.insert(filePath, new InputGrabberWrapper(this, filePath));
        }
    }
}

void InputGrabbersManager::dirChanged() {
    QDir dir(inputDevicePath);
    if (m_files.isEmpty()) {
        m_files = dir.entryList(QDir::System);
        return;
    }

    QStringList newEntryList = dir.entryList(QDir::System);

    QSet<QString> curDirSet = QSet<QString>::fromList(m_files);;
    QSet<QString> newDirSet = QSet<QString>::fromList(newEntryList);

    // added files
    QSet<QString> newFiles = newDirSet - curDirSet;
    for (auto &fileName : newFiles) {
        if (!fileName.startsWith("event"))
            continue;

        QString filePath = inputDevicePath + "/" + fileName;
        addInputGrabber(filePath);
    }

    // deleted files
    QSet<QString> delFiles = curDirSet - newDirSet;
    for (auto &fileName : delFiles) {
        QString filePath = inputDevicePath + "/" + fileName;
        removeInputGrabber(filePath);
    }

    m_files = newEntryList;
}