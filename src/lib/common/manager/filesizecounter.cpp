// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "filesizecounter.h"
#include "common/log.h"

#include <QDir>
#include <QFileInfo>
#include <QSet>
#include <QVector>

#ifndef Q_OS_WIN
#include <sys/stat.h>
#endif

namespace {

// Stable identity of a directory, used to break symlink cycles. Comparing path
// strings is not enough: a symlink loop produces a new string on every hop.
QString directoryIdentity(const QString &path)
{
#ifdef Q_OS_WIN
    return QFileInfo(path).canonicalFilePath().toCaseFolded();
#else
    struct stat st;
    const QByteArray local = QFile::encodeName(path);
    if (::stat(local.constData(), &st) == 0) {
        return QString::number(static_cast<qulonglong>(st.st_dev)) + ":"
                + QString::number(static_cast<qulonglong>(st.st_ino));
    }
    return QFileInfo(path).canonicalFilePath();
#endif
}

} // namespace

FileSizeCounter::FileSizeCounter(QObject *parent)
    : QThread{parent}
{
    DLOG << "Initializing file size counter";
}

quint64 FileSizeCounter::countFiles(const QString &targetIp, const QStringList paths)
{
    DLOG << "Starting file size counting";

    _targetIp = "";
    _paths.clear();

    quint64 totalSize = 0;
    foreach (const QString &path, paths) {
        QFileInfo fileInfo(path);
        if (fileInfo.isDir()) {
            DLOG << "Start counting file size in directory";
            _paths = paths;
            _targetIp = targetIp;
            start();
            return 0;
        } else {
            DLOG << "Counting file size for a single file";
            totalSize += fileInfo.size();
        }
    }

    return totalSize;
}

void FileSizeCounter::stop()
{
    DLOG << "Stopping file size counting";
    _stoped = true;
}

void FileSizeCounter::run()
{
    DLOG << "Starting file size counting in thread";
    _stoped = false;
    _totalSize = 0;
    QStringList names;
    foreach (const QString &path, _paths) {
        if (_stoped.load()) {
            DLOG << "File size counting stopped";
            return;
        }
        QFileInfo fileInfo(path);
        if (fileInfo.isFile()) {
            _totalSize += fileInfo.size();
        } else {
            countFilesInDir(path);
        }
        names.append(fileInfo.fileName());
    }

    emit onCountFinish(_targetIp, names, _totalSize);
}

// Iterative traversal: a symlink loop or a very deep tree must terminate
// instead of overflowing the stack or spinning forever.
void FileSizeCounter::countFilesInDir(const QString &path)
{
    DLOG << "Counting files in directory:" << path.toStdString();

    QSet<QString> visited;
    QVector<QString> pending;
    pending.append(path);

    while (!pending.isEmpty()) {
        if (_stoped.load()) {
            DLOG << "File size counting stopped in directory traversal";
            return;
        }

        const QString current = pending.takeLast();
        const QString identity = directoryIdentity(current);
        if (identity.isEmpty() || visited.contains(identity)) {
            if (!identity.isEmpty())
                DLOG << "Skipping already visited directory:" << current.toStdString();
            continue;
        }
        visited.insert(identity);

        QDir dir(current);
        const QFileInfoList infoList =
                dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
        foreach (const QFileInfo &info, infoList) {
            if (_stoped.load())
                return;

            if (info.isSymLink()) {
                const QFileInfo targetInfo(info.symLinkTarget());
                if (!targetInfo.exists())
                    continue;
                if (targetInfo.isDir())
                    pending.append(targetInfo.filePath());
                else
                    _totalSize += targetInfo.size();
            } else if (info.isDir()) {
                pending.append(info.filePath());
            } else {
                _totalSize += info.size();
            }
        }
    }
}
