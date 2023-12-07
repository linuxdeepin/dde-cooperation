// SPDX-FileCopyrightText: 2022 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "commandparser.h"

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QDebug>

CommandParser &CommandParser::instance()
{
    static CommandParser ins;
    return ins;
}

bool CommandParser::isSet(const QString &name) const
{
    return commandParser->isSet(name);
}

void CommandParser::processCommand()
{
    if (isSet("s")) {
        setSendFiles();
        return;
    }
}

void CommandParser::process()
{
    return process(qApp->arguments());
}

void CommandParser::process(const QStringList &arguments)
{
    qDebug() << "App start args: " << arguments;
    commandParser->process(arguments);
}

void CommandParser::initialize()
{
    commandParser->setApplicationDescription(QString("%1 helper").arg(QCoreApplication::applicationName()));
    initOptions();
    commandParser->addHelpOption();
    commandParser->addVersionOption();
}

void CommandParser::initOptions()
{
    QCommandLineOption sendFiles(QStringList() << "s"
                                               << "send-files",
                                 "send files");
    QCommandLineOption detail("d", "Enable detail log");

    addOption(sendFiles);
    addOption(detail);
}

void CommandParser::addOption(const QCommandLineOption &option)
{
    commandParser->addOption(option);
}

void CommandParser::setSendFiles()
{
    const auto &sendFiles = commandParser->positionalArguments();
    qApp->setProperty("sendFiles", sendFiles);
}

CommandParser::CommandParser(QObject *parent)
    : QObject(parent),
      commandParser(new QCommandLineParser)
{
    initialize();
}

CommandParser::~CommandParser()
{
}
