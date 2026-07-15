// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include "common/filesystem.h"

#include <QStandardPaths>
#include <QFile>
#include <QString>
#include <fstream>
#include <iterator>
#include <string>

using namespace deepin_cross;

namespace {
QString tempFilePath(const QString &name)
{
    return QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/" + name;
}
}

TEST(FileSystemTest, OfstreamIfstreamRoundTrip)
{
    QString qpath = tempFilePath("coop_fs_roundtrip.txt");
    fs::path p(qpath.toStdString());
    const std::string payload = "roundtrip data 12345";

    {
        std::ofstream out;
        open_utf8_path(out, p);
        EXPECT_TRUE(out.is_open());
        out << payload;
        out.close();
    }

    {
        std::ifstream in;
        open_utf8_path(in, p);
        EXPECT_TRUE(in.is_open());
        std::string content((std::istreambuf_iterator<char>(in)),
                            std::istreambuf_iterator<char>());
        in.close();
        EXPECT_EQ(content, payload);
    }

    QFile::remove(qpath);
}

TEST(FileSystemTest, IfstreamMissingFileIsOpenFalse)
{
    QString qpath = tempFilePath("coop_fs_does_not_exist.txt");
    QFile::remove(qpath);
    fs::path p(qpath.toStdString());

    std::ifstream in;
    open_utf8_path(in, p);
    EXPECT_FALSE(in.is_open());
}

TEST(FileSystemTest, FstreamWriteAndRead)
{
    QString qpath = tempFilePath("coop_fs_fstream.txt");
    fs::path p(qpath.toStdString());
    const std::string payload = "fstream payload";

    {
        std::fstream fs_out;
        open_utf8_path(fs_out, p, std::ios::out | std::ios::trunc);
        EXPECT_TRUE(fs_out.is_open());
        fs_out << payload;
        fs_out.close();
    }

    {
        std::fstream fs_in;
        open_utf8_path(fs_in, p, std::ios::in);
        EXPECT_TRUE(fs_in.is_open());
        std::string content((std::istreambuf_iterator<char>(fs_in)),
                            std::istreambuf_iterator<char>());
        EXPECT_EQ(content, payload);
    }

    QFile::remove(qpath);
}

TEST(FileSystemTest, OfstreamOverwritesExisting)
{
    QString qpath = tempFilePath("coop_fs_overwrite.txt");
    fs::path p(qpath.toStdString());

    {
        std::ofstream out;
        open_utf8_path(out, p);
        out << "first content";
        out.close();
    }

    {
        std::ofstream out;
        open_utf8_path(out, p, std::ios::out | std::ios::trunc);
        out << "second";
        out.close();
    }

    {
        std::ifstream in;
        open_utf8_path(in, p);
        std::string content((std::istreambuf_iterator<char>(in)),
                            std::istreambuf_iterator<char>());
        EXPECT_EQ(content, "second");
    }

    QFile::remove(qpath);
}
