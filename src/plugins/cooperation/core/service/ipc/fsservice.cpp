// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "fsservice.h"
#include "common/constant.h"

#include "co/co.h"
#include "co/time.h"

using namespace ipc;

void FSImpl::compatible(co::Json& req, co::Json& res)
{

}

void FSImpl::readDir(co::Json& req, co::Json& res)
{
    
}

void FSImpl::removeDir(co::Json& req, co::Json& res)
{

}

void FSImpl::create(co::Json& req, co::Json& res)
{

}

void FSImpl::rename(co::Json& req, co::Json& res)
{

}

void FSImpl::removeFiles(co::Json& req, co::Json& res)
{

}

void FSImpl::sendFiles(co::Json& req, co::Json& res)
{

}

void FSImpl::receiveFiles(co::Json& req, co::Json& res)
{

}

void FSImpl::resumeJob(co::Json& req, co::Json& res)
{

}

void FSImpl::cancelJob(co::Json& req, co::Json& res)
{

}

void FSImpl::fsReport(co::Json& req, co::Json& res)
{

}


FSService::FSService(QObject *parent) : QObject(parent)
{


}

FSService::~FSService()
{
}