// SPDX-FileCopyrightText: 2015 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Manager.h"

#include <DApplication>

namespace fs = std::filesystem;

DWIDGET_USE_NAMESPACE

int main(int argc, char *argv[]) {
    std::string runtimeDir = getenv("XDG_RUNTIME_DIR");
    if (runtimeDir.empty()) {
        throw std::runtime_error("XDG_RUNTIME_DIR not set in the environment");
    }

    fs::path dataDir = fs::path(runtimeDir) / "dde-cooperation";

    DApplication::setQuitOnLastWindowClosed(false);
    DApplication app(argc, argv);
    app.setOrganizationName("deepin");
    app.setApplicationName("DDE Cooperation");

    Manager cooperation(dataDir);

    return app.exec();
}
