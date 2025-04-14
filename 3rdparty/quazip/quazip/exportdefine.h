// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef GLOBAL_H
#define GLOBAL_H

#include <QtCore/QtGlobal>

/**
  This is automatically defined when building a static library, but when
  including QuaZip sources directly into a project, BUILD_STATIC should
  be defined explicitly to avoid possible troubles with unnecessary
  importing/exporting.
  */
#ifdef BUILD_STATIC
#define DLL_EXPORT
#else
/**
 * When building a DLL with MSVC, BUILD must be defined.
 * qglobal.h takes care of defining Q_DECL_* correctly for msvc/gcc.
 */
#if defined(BUILD)
	#define DLL_EXPORT Q_DECL_EXPORT
#else
	#define DLL_EXPORT Q_DECL_IMPORT
#endif
#endif // BUILD_STATIC

#ifdef __GNUC__
#define UNUSED __attribute__((__unused__))
#else
#define UNUSED
#endif

#define EXTRA_NTFS_MAGIC 0x000Au
#define EXTRA_NTFS_TIME_MAGIC 0x0001u

#endif // GLOBAL_H
