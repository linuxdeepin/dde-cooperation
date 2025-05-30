// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef QUAADLER32_H
#define QUAADLER32_H

#include <QByteArray>

#include "quachecksum32.h"

/// Adler32 checksum
/** \class QuaAdler32 quaadler32.h <quazip/quaadler32.h>
 * This class wrappers the adler32 function with the QuaChecksum32 interface.
 * See QuaChecksum32 for more info.
 */
class DLL_EXPORT QuaAdler32 : public QuaChecksum32
{

public:
	QuaAdler32();

	quint32 calculate(const QByteArray &data);

	void reset();
	void update(const QByteArray &buf);
	quint32 value();

private:
	quint32 checksum;
};

#endif //QUAADLER32_H
