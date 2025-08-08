// SPDX-FileCopyrightText: 2022 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "commandparser.h"

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QDebug>
#include <QVariant>

CommandParser &CommandParser::instance()
{
    qDebug() << "Accessing CommandParser singleton instance";
    static CommandParser ins;
    return ins;
}

QStringList CommandParser::processCommand(const QString &name)
{
    qDebug() << "Processing command:" << name;
    QStringList args;
    if (cmdParser->isSet(name)) {
        qDebug() << "Command" << name << "is set, getting arguments";
        args = cmdParser->positionalArguments();
    } else {
        qDebug() << "Command" << name << "is not set";
    }
    return args;
}

void CommandParser::process()
{
    return process(qApp->arguments());
}

void CommandParser::process(const QStringList &arguments)
{
    qDebug() << "Processing command line arguments:" << arguments;
    cmdParser->process(arguments);
    qDebug() << "Command line arguments processed";
}

void CommandParser::initialize()
{
    qDebug() << "Initializing command parser";
    cmdParser->setApplicationDescription(QString("%1 helper").arg(QCoreApplication::applicationName()));
    initOptions();
    cmdParser->addHelpOption();
    cmdParser->addVersionOption();
    qDebug() << "Command parser initialized";
}

void CommandParser::initOptions()
{
    qDebug() << "Initializing command line options";
    QCommandLineOption sendFiles(QStringList() << "s"
                                               << "send-files",
                                 "send files");
    QCommandLineOption detail(QStringList() << "d"
                                            << "detail",
                              "Enable detail log");
    QCommandLineOption minimize(QStringList() << "m"
                                              << "minimize",
                                "Launch with minimize UI");
    QCommandLineOption forward(QStringList() << "f"
                                            << "forward",
                              "Forward files to target with IP and name");

    addOption(sendFiles);
    addOption(detail);
    addOption(minimize);
    addOption(forward);
    qDebug() << "Command line options initialized";
}

void CommandParser::addOption(const QCommandLineOption &option)
{
    qDebug() << "Adding command line option";
    cmdParser->addOption(option);
}

CommandParser::CommandParser(QObject *parent)
    : QObject(parent),
      cmdParser(new QCommandLineParser)
{
    qDebug() << "Creating new CommandParser instance";
    initialize();
    qDebug() << "CommandParser instance created";
}

CommandParser::~CommandParser()
{
    qDebug() << "Destroying CommandParser instance";
}
