// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "quacrc32.h"

#include "zlib.h"
#include <QDebug>

QuaCrc32::QuaCrc32()
{
    qInfo() << "Constructing QuaCrc32 object and resetting checksum";
	reset();
}

quint32 QuaCrc32::calculate(const QByteArray &data)
{
    qInfo() << "Calculating static CRC32 for data of size:" << data.size();
	return crc32( crc32(0L, Z_NULL, 0), (const Bytef*)data.data(), data.size() );
}

void QuaCrc32::reset()
{
    qInfo() << "Resetting CRC32 checksum to initial value";
	checksum = crc32(0L, Z_NULL, 0);
}

void QuaCrc32::update(const QByteArray &buf)
{
    qInfo() << "Updating CRC32 checksum with buffer of size:" << buf.size();
	checksum = crc32( checksum, (const Bytef*)buf.data(), buf.size() );
}

quint32 QuaCrc32::value()
{
    qInfo() << "Getting current CRC32 checksum value";
	return checksum;
}
