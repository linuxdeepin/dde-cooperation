// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zlib.h"
#include "ioapi.h"
#include "exportdefine.h"
#include <QIODevice>
#include <QDebug>
#if (QT_VERSION >= 0x050100)
#define QSAVEFILE_BUG_WORKAROUND
#endif
#ifdef QSAVEFILE_BUG_WORKAROUND
#include <QSaveFile>
#endif

/* I've found an old Unix (a SunOS 4.1.3_U1) without all SEEK_* defined.... */

#ifndef SEEK_CUR
#define SEEK_CUR    1
#endif

#ifndef SEEK_END
#define SEEK_END    2
#endif

#ifndef SEEK_SET
#define SEEK_SET    0
#endif

voidpf call_zopen64 (const zlib_filefunc64_32_def* pfilefunc,voidpf file,int mode)
{
    qInfo() << "Entering call_zopen64, mode:" << mode;
    if (pfilefunc->zfile_func64.zopen64_file != NULL) {
        qInfo() << "calling zopen64_file";
        return (*(pfilefunc->zfile_func64.zopen64_file)) (pfilefunc->zfile_func64.opaque,file,mode);
    } else {
        qInfo() << "calling zopen32_file";
        return (*(pfilefunc->zopen32_file))(pfilefunc->zfile_func64.opaque,file,mode);
    }
}

int call_zseek64 (const zlib_filefunc64_32_def* pfilefunc,voidpf filestream, ZPOS64_T offset, int origin)
{
    qInfo() << "Entering call_zseek64, offset:" << offset << "origin:" << origin;
    if (pfilefunc->zfile_func64.zseek64_file != NULL) {
        qInfo() << "calling zseek64_file";
        return (*(pfilefunc->zfile_func64.zseek64_file)) (pfilefunc->zfile_func64.opaque,filestream,offset,origin);
    } else {
        uLong offsetTruncated = (uLong)offset;
        if (offsetTruncated != offset) {
            qInfo() << "offset truncated";
            return -1;
        } else {
            qInfo() << "calling zseek32_file";
            return (*(pfilefunc->zseek32_file))(pfilefunc->zfile_func64.opaque,filestream,offsetTruncated,origin);
        }
    }
}

ZPOS64_T call_ztell64 (const zlib_filefunc64_32_def* pfilefunc,voidpf filestream)
{
    qInfo() << "Entering call_ztell64";
    if (pfilefunc->zfile_func64.zseek64_file != NULL) {
        qInfo() << "calling ztell64_file";
        return (*(pfilefunc->zfile_func64.ztell64_file)) (pfilefunc->zfile_func64.opaque,filestream);
    } else {
        qInfo() << "calling ztell32_file";
        uLong tell_uLong = (*(pfilefunc->ztell32_file))(pfilefunc->zfile_func64.opaque,filestream);
        if ((tell_uLong) == ((uLong)-1)) {
            qInfo() << "ztell32_file failed";
            return (ZPOS64_T)-1;
        } else {
            return tell_uLong;
        }
    }
}

/// @cond internal
struct QIODevice_descriptor {
    // Position only used for writing to sequential devices.
    qint64 pos;
    inline QIODevice_descriptor():
        pos(0)
    {}
};
/// @endcond

voidpf ZCALLBACK qiodevice_open_file_func (
   voidpf opaque,
   voidpf file,
   int mode)
{
    qInfo() << "qiodevice_open_file_func, mode:" << mode;
    QIODevice_descriptor *d = reinterpret_cast<QIODevice_descriptor*>(opaque);
    QIODevice *iodevice = reinterpret_cast<QIODevice*>(file);
    QIODevice::OpenMode desiredMode;
    if ((mode & ZLIB_FILEFUNC_MODE_READWRITEFILTER)==ZLIB_FILEFUNC_MODE_READ) {
        qInfo() << "mode is ReadOnly";
        desiredMode = QIODevice::ReadOnly;
    } else if (mode & ZLIB_FILEFUNC_MODE_EXISTING) {
        qInfo() << "mode is ReadWrite";
        desiredMode = QIODevice::ReadWrite;
    } else if (mode & ZLIB_FILEFUNC_MODE_CREATE) {
        qInfo() << "mode is WriteOnly";
        desiredMode = QIODevice::WriteOnly;
    }
    if (iodevice->isOpen()) {
        qInfo() << "iodevice is already open";
        if ((iodevice->openMode() & desiredMode) == desiredMode) {
            qInfo() << "iodevice open mode is compatible";
            if (desiredMode != QIODevice::WriteOnly
                    && iodevice->isSequential()) {
                // We can use sequential devices only for writing.
                qInfo() << "sequential device can not be read";
                delete d;
                return NULL;
            } else {
                if ((desiredMode & QIODevice::WriteOnly) != 0) {
                    qInfo() << "open for writing, seeking to 0";
                    // open for writing, need to seek existing device
                    if (!iodevice->isSequential()) {
                        qInfo() << "not sequential, seeking to 0";
                        iodevice->seek(0);
                    } else {
                        qInfo() << "sequential, saving pos";
                        d->pos = iodevice->pos();
                    }
                }
            }
            return iodevice;
        } else {
            qInfo() << "iodevice open mode is not compatible";
            delete d;
            return NULL;
        }
    }
    qInfo() << "iodevice is not open, opening with desired mode";
    iodevice->open(desiredMode);
    if (iodevice->isOpen()) {
        qInfo() << "iodevice opened successfully";
        if (desiredMode != QIODevice::WriteOnly && iodevice->isSequential()) {
            // We can use sequential devices only for writing.
            qInfo() << "sequential device can not be read";
            iodevice->close();
            delete d;
            return NULL;
        } else {
            return iodevice;
        }
    } else {
        qInfo() << "iodevice open failed";
        delete d;
        return NULL;
    }
}


uLong ZCALLBACK qiodevice_read_file_func (
   voidpf opaque,
   voidpf stream,
   void* buf,
   uLong size)
{
    qInfo() << "qiodevice_read_file_func, requested size:" << size;
    QIODevice_descriptor *d = reinterpret_cast<QIODevice_descriptor*>(opaque);
    QIODevice *iodevice = reinterpret_cast<QIODevice*>(stream);
    qint64 ret64 = iodevice->read((char*)buf,size);
    uLong ret;
    ret = (uLong) ret64;
    if (ret64 != -1) {
        d->pos += ret64;
    } else {
        qInfo() << "read failed";
    }
    return ret;
}


uLong ZCALLBACK qiodevice_write_file_func (
   voidpf opaque,
   voidpf stream,
   const void* buf,
   uLong size)
{
    qInfo() << "qiodevice_write_file_func, requested size:" << size;
    QIODevice_descriptor *d = reinterpret_cast<QIODevice_descriptor*>(opaque);
    QIODevice *iodevice = reinterpret_cast<QIODevice*>(stream);
    uLong ret;
    qint64 ret64 = iodevice->write((char*)buf,size);
    if (ret64 != -1) {
        d->pos += ret64;
    } else {
        qInfo() << "write failed";
    }
    ret = (uLong) ret64;
    return ret;
}

uLong ZCALLBACK qiodevice_tell_file_func (
   voidpf opaque,
   voidpf stream)
{
    qInfo() << "qiodevice_tell_file_func (32-bit)";
    QIODevice_descriptor *d = reinterpret_cast<QIODevice_descriptor*>(opaque);
    QIODevice *iodevice = reinterpret_cast<QIODevice*>(stream);
    uLong ret;
    qint64 ret64;
    if (iodevice->isSequential()) {
        qInfo() << "device is sequential";
        ret64 = d->pos;
    } else {
        qInfo() << "device is not sequential";
        ret64 = iodevice->pos();
    }
    ret = static_cast<uLong>(ret64);
    return ret;
}

ZPOS64_T ZCALLBACK qiodevice64_tell_file_func (
   voidpf opaque,
   voidpf stream)
{
    qInfo() << "qiodevice64_tell_file_func (64-bit)";
    QIODevice_descriptor *d = reinterpret_cast<QIODevice_descriptor*>(opaque);
    QIODevice *iodevice = reinterpret_cast<QIODevice*>(stream);
    qint64 ret;
    if (iodevice->isSequential()) {
        qInfo() << "device is sequential";
        ret = d->pos;
    } else {
        qInfo() << "device is not sequential";
        ret = iodevice->pos();
    }
    return static_cast<ZPOS64_T>(ret);
}

int ZCALLBACK qiodevice_seek_file_func (
   voidpf /*opaque UNUSED*/,
   voidpf stream,
   uLong offset,
   int origin)
{
    qInfo() << "qiodevice_seek_file_func (32-bit), offset:" << offset << "origin:" << origin;
    QIODevice *iodevice = reinterpret_cast<QIODevice*>(stream);
    if (iodevice->isSequential()) {
        qInfo() << "device is sequential";
        if (origin == ZLIB_FILEFUNC_SEEK_END
                && offset == 0) {
            // sequential devices are always at end (needed in mdAppend)
            qInfo() << "seeking to end of sequential device";
            return 0;
        } else {
            qWarning("qiodevice_seek_file_func() called for sequential device");
            qInfo() << "can not seek on sequential device";
            return -1;
        }
    }
    uLong qiodevice_seek_result=0;
    int ret;
    switch (origin)
    {
    case ZLIB_FILEFUNC_SEEK_CUR :
        qInfo() << "seeking from current";
        qiodevice_seek_result = ((QIODevice*)stream)->pos() + offset;
        break;
    case ZLIB_FILEFUNC_SEEK_END :
        qInfo() << "seeking from end";
        qiodevice_seek_result = ((QIODevice*)stream)->size() - offset;
        break;
    case ZLIB_FILEFUNC_SEEK_SET :
        qInfo() << "seeking from set";
        qiodevice_seek_result = offset;
        break;
    default:
        qInfo() << "invalid origin";
        return -1;
    }
    ret = !iodevice->seek(qiodevice_seek_result);
    qInfo() << "seek result:" << ret;
    return ret;
}

int ZCALLBACK qiodevice64_seek_file_func (
   voidpf /*opaque UNUSED*/,
   voidpf stream,
   ZPOS64_T offset,
   int origin)
{
    qInfo() << "qiodevice64_seek_file_func (64-bit), offset:" << offset << "origin:" << origin;
    QIODevice *iodevice = reinterpret_cast<QIODevice*>(stream);
    if (iodevice->isSequential()) {
        qInfo() << "device is sequential";
        if (origin == ZLIB_FILEFUNC_SEEK_END
                && offset == 0) {
            // sequential devices are always at end (needed in mdAppend)
            qInfo() << "seeking to end of sequential device";
            return 0;
        } else {
            qWarning("qiodevice_seek_file_func() called for sequential device");
            qInfo() << "can not seek on sequential device";
            return -1;
        }
    }
    qint64 qiodevice_seek_result=0;
    int ret;
    switch (origin)
    {
    case ZLIB_FILEFUNC_SEEK_CUR :
        qInfo() << "seeking from current";
        qiodevice_seek_result = ((QIODevice*)stream)->pos() + offset;
        break;
    case ZLIB_FILEFUNC_SEEK_END :
        qInfo() << "seeking from end";
        qiodevice_seek_result = ((QIODevice*)stream)->size() - offset;
        break;
    case ZLIB_FILEFUNC_SEEK_SET :
        qInfo() << "seeking from set";
        qiodevice_seek_result = offset;
        break;
    default:
        qInfo() << "invalid origin";
        return -1;
    }
    ret = !iodevice->seek(qiodevice_seek_result);
    qInfo() << "seek result:" << ret;
    return ret;
}

int ZCALLBACK qiodevice_close_file_func (
   voidpf opaque,
   voidpf stream)
{
    qInfo() << "qiodevice_close_file_func";
    QIODevice_descriptor *d = reinterpret_cast<QIODevice_descriptor*>(opaque);
    delete d;
    QIODevice *device = reinterpret_cast<QIODevice*>(stream);
#ifdef QSAVEFILE_BUG_WORKAROUND
    // QSaveFile terribly breaks the is-a idiom:
    // it IS a QIODevice, but it is NOT compatible with it: close() is private
    QSaveFile *file = qobject_cast<QSaveFile*>(device);
    if (file != NULL) {
        qInfo() << "qsavefile bug workaround";
        // We have to call the ugly commit() instead:
        return file->commit() ? 0 : -1;
    }
#endif
    device->close();
    return 0;
}

int ZCALLBACK qiodevice_fakeclose_file_func (
   voidpf opaque,
   voidpf /*stream*/)
{
    qInfo() << "qiodevice_fakeclose_file_func";
    QIODevice_descriptor *d = reinterpret_cast<QIODevice_descriptor*>(opaque);
    delete d;
    return 0;
}

int ZCALLBACK qiodevice_error_file_func (
   voidpf /*opaque UNUSED*/,
   voidpf /*stream UNUSED*/)
{
    qInfo() << "qiodevice_error_file_func";
    // can't check for error due to the QIODevice API limitation
    return 0;
}

void fill_qiodevice_filefunc (
  zlib_filefunc_def* pzlib_filefunc_def)
{
    qInfo() << "fill_qiodevice_filefunc (32-bit)";
    pzlib_filefunc_def->zopen_file = qiodevice_open_file_func;
    pzlib_filefunc_def->zread_file = qiodevice_read_file_func;
    pzlib_filefunc_def->zwrite_file = qiodevice_write_file_func;
    pzlib_filefunc_def->ztell_file = qiodevice_tell_file_func;
    pzlib_filefunc_def->zseek_file = qiodevice_seek_file_func;
    pzlib_filefunc_def->zclose_file = qiodevice_close_file_func;
    pzlib_filefunc_def->zerror_file = qiodevice_error_file_func;
    pzlib_filefunc_def->opaque = new QIODevice_descriptor;
}

void fill_qiodevice64_filefunc (
  zlib_filefunc64_def* pzlib_filefunc_def)
{
    qInfo() << "fill_qiodevice64_filefunc (64-bit)";
    // Open functions are the same for Qt.
    pzlib_filefunc_def->zopen64_file = qiodevice_open_file_func;
    pzlib_filefunc_def->zread_file = qiodevice_read_file_func;
    pzlib_filefunc_def->zwrite_file = qiodevice_write_file_func;
    pzlib_filefunc_def->ztell64_file = qiodevice64_tell_file_func;
    pzlib_filefunc_def->zseek64_file = qiodevice64_seek_file_func;
    pzlib_filefunc_def->zclose_file = qiodevice_close_file_func;
    pzlib_filefunc_def->zerror_file = qiodevice_error_file_func;
    pzlib_filefunc_def->opaque = new QIODevice_descriptor;
    pzlib_filefunc_def->zfakeclose_file = qiodevice_fakeclose_file_func;
}

void fill_zlib_filefunc64_32_def_from_filefunc32(zlib_filefunc64_32_def* p_filefunc64_32,const zlib_filefunc_def* p_filefunc32)
{
    qInfo() << "fill_zlib_filefunc64_32_def_from_filefunc32";
    p_filefunc64_32->zfile_func64.zopen64_file = NULL;
    p_filefunc64_32->zopen32_file = p_filefunc32->zopen_file;
    p_filefunc64_32->zfile_func64.zerror_file = p_filefunc32->zerror_file;
    p_filefunc64_32->zfile_func64.zread_file = p_filefunc32->zread_file;
    p_filefunc64_32->zfile_func64.zwrite_file = p_filefunc32->zwrite_file;
    p_filefunc64_32->zfile_func64.ztell64_file = NULL;
    p_filefunc64_32->zfile_func64.zseek64_file = NULL;
    p_filefunc64_32->zfile_func64.zclose_file = p_filefunc32->zclose_file;
    p_filefunc64_32->zfile_func64.zerror_file = p_filefunc32->zerror_file;
    p_filefunc64_32->zfile_func64.opaque = p_filefunc32->opaque;
    p_filefunc64_32->zfile_func64.zfakeclose_file = NULL;
    p_filefunc64_32->zseek32_file = p_filefunc32->zseek_file;
    p_filefunc64_32->ztell32_file = p_filefunc32->ztell_file;
}
