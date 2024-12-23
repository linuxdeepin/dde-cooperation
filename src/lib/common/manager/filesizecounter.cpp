// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "filesizecounter.h"

#include <QDir>
#include <QFileInfo>

FileSizeCounter::FileSizeCounter(QObject *parent)
    : QThread{parent}
{

}

quint64 FileSizeCounter::countFiles(const QString &targetIp, const QStringList paths)
{
    _targetIp = "";
    _paths.clear();

    quint64 totalSize = 0;
    foreach (const QString &path, paths) {
        QFileInfo fileInfo(path);
        if (fileInfo.isDir()) {
            _paths = paths;
            _targetIp = targetIp;
            start();
            return 0;
        } else {
            totalSize += fileInfo.size();
        }
    }

    return totalSize;
}

void FileSizeCounter::stop()
{
    _stoped = true;
}

void FileSizeCounter::run()
{
    _stoped = false;
    _totalSize = 0;
    QStringList names;
    foreach (const QString &path, _paths) {
        if (_stoped)
            return;
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
    if (_stoped)
        return;

    QDir dir(path);
    QFileInfoList infoList = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
    foreach (const QFileInfo &info, infoList) {
        if (_stoped)
            return;

        if (info.isSymLink()) {
            // 处理符号链接
            QFileInfo targetInfo(info.symLinkTarget());
            if (targetInfo.exists()) {
                if (targetInfo.isDir()) {
                    countFilesInDir(targetInfo.filePath());
                } else {
                    _totalSize += targetInfo.size();
                }
            }
        } else if (info.isDir()) {
            countFilesInDir(info.filePath()); // 递归遍历子目录
        } else {
            _totalSize += info.size(); // 统计文件大小
        }
    }
}
