// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef QUAZIP_GLOBAL_H
#define QUAZIP_GLOBAL_H

#include <QtCore/QtGlobal>

/**
  This is automatically defined when building a static library, but when
  including QuaZip sources directly into a project, QUAZIP_STATIC should
  be defined explicitly to avoid possible troubles with unnecessary
  importing/exporting.
  */
#ifdef QUAZIP_STATIC
#define QUAZIP_EXPORT
#else
/**
 * When building a DLL with MSVC, QUAZIP_BUILD must be defined.
 * qglobal.h takes care of defining Q_DECL_* correctly for msvc/gcc.
 */
#if defined(QUAZIP_BUILD)
	#define QUAZIP_EXPORT Q_DECL_EXPORT
#else
	#define QUAZIP_EXPORT Q_DECL_IMPORT
#endif
#endif // QUAZIP_STATIC

#ifdef __GNUC__
#define UNUSED __attribute__((__unused__))
#else
#define UNUSED
#endif

#define QUAZIP_EXTRA_NTFS_MAGIC 0x000Au
#define QUAZIP_EXTRA_NTFS_TIME_MAGIC 0x0001u

#endif // QUAZIP_GLOBAL_H
