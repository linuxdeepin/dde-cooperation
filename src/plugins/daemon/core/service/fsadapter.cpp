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

bool FSAdapter::newFile(const char *name, bool isdir)
{
    fastring fullpath = path::join(DaemonConfig::instance()->getStorageDir(), name);
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
