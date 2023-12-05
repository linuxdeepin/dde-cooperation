// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "fsadapter.h"
#include "message.pb.h"
#include "utils/utils.h"
#include "utils/config.h"
#include "common/constant.h"

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
    if (size <= 0)
        return true;
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

bool FSAdapter::writeBlock(const char *name, int64 seek_len,
                           const char *data, size_t size, const int flags, fs::file **fx)
{
    if (flags & JobTransFileOp::FIlE_CREATE) {
        if ((*fx) != nullptr) {
            // 这里创建文件发现文件描述符存在，错误返回 -1
            ELOG << "file flags is create, but file fx is not nullptr, flags = " << flags;
            (*fx)->close();
            delete (*fx);
            (*fx) = nullptr;
            return false;
        }
        fastring parent = path::dir(name);
        fs::mkdir(parent, true); // 创建文件保存的根/子目录
        (*fx) = new fs::file(name, 'm');
        if (!(*fx)->exists()) {
            ELOG << " file create error , file = " << name << ", flags = " << flags;
            (*fx)->close();
            delete (*fx);
            (*fx) = nullptr;
            return false;
        }
    }

    if ((*fx) == nullptr) {
        ELOG << "fx is nullptr !!!!!! " << name << " flags = " << flags << " len " << size;
        return false;
    }
    bool write = true;
    if (size != 0) {
        size_t wirted_size = 0;
        size_t rem_size = size;
        (*fx)->seek(seek_len);
        do {
            size_t wsize = (*fx)->write(data, rem_size);
            if (wsize <= 0) {
                write = false;
                ELOG << "fx write done: " << rem_size << " => " << wsize;
                break;
            }
            wirted_size += wsize;
            rem_size = size - wsize;
        } while (wirted_size < size);
    }

    if (flags & JobTransFileOp::FILE_CLOSE || !write) {
        (*fx)->close();
        delete (*fx);
        (*fx) = nullptr;
    }

    return write;
}

fastring FSAdapter::noneExitPath(const char *name)
{
    fastring path(name);
    if (!fs::exists(name))
        return path;
    size_t index = path.find_last_of(".");
    fastring suffix = index >= path.size() ? "" : path.substr(path.find_last_of("."));
    int n = 1;
    fastring tmpName = index >= path.size() ?  path : path.substr(0, path.find_last_of("."));
    do {
        tmpName += " " + QString::number(n).toStdString();
        path = tmpName + suffix;
    } while (!fs::exists(path));
    return path;
}
