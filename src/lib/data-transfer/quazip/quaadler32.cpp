// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "quaadler32.h"

#include "zlib.h"
#include <QDebug>

QuaAdler32::QuaAdler32()
{
    qInfo() << "Constructing QuaAdler32 object and resetting checksum";
	reset();
}

quint32 QuaAdler32::calculate(const QByteArray &data)
{
    qInfo() << "Calculating static Adler-32 for data of size:" << data.size();
	return adler32( adler32(0L, Z_NULL, 0), (const Bytef*)data.data(), data.size() );
}

void QuaAdler32::reset()
{
    qInfo() << "Resetting Adler-32 checksum to initial value";
	checksum = adler32(0L, Z_NULL, 0);
}

void QuaAdler32::update(const QByteArray &buf)
{
    qInfo() << "Updating Adler-32 checksum with buffer of size:" << buf.size();
	checksum = adler32( checksum, (const Bytef*)buf.data(), buf.size() );
}

quint32 QuaAdler32::value()
{
    qInfo() << "Getting current Adler-32 checksum value";
	return checksum;
}
