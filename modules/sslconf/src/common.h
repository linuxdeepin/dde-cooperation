// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SSLCONF_COMMON_H
#define SSLCONF_COMMON_H

#if HAVE_CONFIG_H
#	include "config.h"
#endif

// define NULL
#include <stddef.h>

// make assert available since we use it a lot
#include <assert.h>
#include <stdlib.h>
#include <string.h>

enum {
    kExitSuccess      = 0, // successful completion
    kExitFailed       = 1, // general failure
    kExitTerminated   = 2, // killed by signal
    kExitArgs         = 3, // bad arguments
    kExitConfig       = 4, // cannot read configuration
    kExitSubscription = 5  // subscription error
};

#endif