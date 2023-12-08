// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "fsadapter.h"
#include "message.pb.h"
#include "utils/utils.h"
#include "utils/config.h"

#include "co/log.h"
#include "co/path.h"

//using namespace deamon_core;

FSAdapter::FSAdapter(QObject *parent)
    : QObject(parent)
{
}

int FSAdapter::getFileEntry(const char *path, FileEntry **entry)
{
    FileEntry *temp = *entry;
    if (!fs::exists(path)) {
        ELOG << "FSAdapter::getFileEntry path not exists: " << path;
        return -1;
    }

    if (fs::isdir(path)) {
        temp->filetype = (FileType::DIR);
    } else {
        temp->filetype = (FileType::FILE_B);
    }

    fastring name = Util::parseFileName(path);
    temp->name = (name.c_str());
    if (name.starts_with('.')) {
        temp->hidden = (true);
    } else {
        temp->hidden = (false);
    }

    temp->size = (fs::fsize(path));
    temp->modified_time = (fs::mtime(path));

    return 0;
}

bool FSAdapter::newFile(const char *path, bool isdir)
{
    fastring fullpath = path::join(DaemonConfig::instance()->getStorageDir(), path);
    if (isdir) {
        fs::mkdir(fullpath, true);
    } else {
        fastring parent = path::dir(fullpath);
        fs::mkdir(parent, true); // 创建文件保存的根/子目录
        if (!fs::exists(fullpath)) {
            fs::file fx(fullpath, 'm');
            fx.close();
        }
    }
//    LOG << "new file -> fullpath: " << fullpath;

    return fs::exists(fullpath);
}

bool FSAdapter::newFileByFullPath(const char *fullpath, bool isdir)
{
    if (isdir) {
        fs::mkdir(fullpath, true);
    } else {
        fastring parent = path::dir(fullpath);
        fs::mkdir(parent, true); // 创建文件保存的根/子目录
        if (!fs::exists(fullpath)) {
            fs::file fx(fullpath, 'm');
            fx.close();
        }
    }
//    LOG << "new file -> fullpath: " << fullpath;

    return fs::exists(fullpath);
}

bool FSAdapter::noneExitFileByFullPath(const char *fullpath, bool isdir, fastring *path)
{
    auto nonePath = noneExitPath(fullpath);
    if (isdir) {
        fs::mkdir(nonePath, true);
    } else {
        fastring parent = path::dir(nonePath);
        fs::mkdir(parent, true); // 创建文件保存的根/子目录
        if (!fs::exists(nonePath)) {
            fs::file fx(nonePath, 'm');
            fx.close();
        }
    }
    // LOG << "new file -> fullpath: " << fullpath;
    if (path)
        *path = nonePath;

    return fs::exists(nonePath);
}

bool FSAdapter::writeBlock(const char *name, int64 seek_len, const char *data, size_t size)
{
    fs::file fx(name, 'm');
    if (!fx.exists()) {
        ELOG << "writeBlock File does not exist: " << name;
        return false;
    }

    size_t wirted_size = 0;
    size_t rem_size = size;
    fx.seek(seek_len);
    do {
        size_t wsize = fx.write(data, rem_size);
        if (wsize <= 0) {
            ELOG << "fx write done: " << rem_size << " => " << wsize;
            break;
        }
        wirted_size += wsize;
        rem_size = size - wsize;
    } while (wirted_size < size);
    fx.close();

    return true;
}

fastring FSAdapter::noneExitPath(const char *name)
{
    fastring path(name);
    if (!fs::exists(name))
        return path;
    auto index1 = path.find_last_of("/");
    auto tm = path;
    // 没找到/
    if (index1 < path.size()) {
        tm = path.substr(path.find_last_of("/"));
    }
    size_t index = tm.find_last_of(".");
    fastring suffix = index >= tm.size() ? "" : tm.substr(path.find_last_of("."));
    int n = 1;
    fastring tmpName = path.replace(suffix, "");
    fastring org = tmpName;
    do {
        tmpName = org + "_" + QString::number(n).toStdString();
        path = tmpName + suffix;
        n++;
    } while (fs::exists(path));
    return path;
}
