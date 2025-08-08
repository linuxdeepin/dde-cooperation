// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "filesizecounter.h"
#include "common/log.h"

#include <QDir>
#include <QFileInfo>

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
        if (_stoped) {
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

void FileSizeCounter::countFilesInDir(const QString &path)
{
    DLOG << "Counting files in directory:" << path.toStdString();
    if (_stoped) {
        DLOG << "File size counting stopped in directory";
        return;
    }

    QDir dir(path);
    QFileInfoList infoList = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
    foreach (const QFileInfo &info, infoList) {
        if (_stoped) {
            DLOG << "File size counting stopped in directory traversal";
            return;
        }

        if (info.isSymLink()) {
            // DLOG << "Handling symbolic link:" << info.filePath().toStdString();
            // 处理符号链接
            QFileInfo targetInfo(info.symLinkTarget());
            if (targetInfo.exists()) {
                if (targetInfo.isDir()) {
                    // DLOG << "Symbolic link target is a directory, recursively counting";
                    countFilesInDir(targetInfo.filePath());
                } else {
                    // DLOG << "Symbolic link target is a file, adding its size";
                    _totalSize += targetInfo.size();
                }
            }
        } else if (info.isDir()) {
            // DLOG << "Recursively counting files in subdirectory:" << info.filePath().toStdString();
            countFilesInDir(info.filePath()); // 递归遍历子目录
        } else {
            // DLOG << "Adding file size for:" << info.filePath().toStdString();
            _totalSize += info.size(); // 统计文件大小
        }
    }
}
