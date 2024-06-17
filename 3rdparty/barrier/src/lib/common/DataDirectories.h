/*
* barrier -- mouse and keyboard sharing utility
* Copyright (C) 2018 Debauchee Open Source Group
*
* This package is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* found in the file LICENSE that should have accompanied this file.
*
* This package is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef BARRIER_LIB_COMMON_DATA_DIRECTORIES_H
#define BARRIER_LIB_COMMON_DATA_DIRECTORIES_H

#include "io/filesystem.h"

namespace barrier {

class DataDirectories
{
public:
    static const fastring& profile();
    static const fastring& profile(const fastring& path);

    static const fastring& global();
    static const fastring& global(const fastring& path);

    static const fastring& systemconfig();
    static const fastring& systemconfig(const fastring& path);

    static fastring ssl_fingerprints_path();
    static fastring local_ssl_fingerprints_path();
    static fastring trusted_servers_ssl_fingerprints_path();
    static fastring trusted_clients_ssl_fingerprints_path();
    static fastring ssl_certificate_path();
private:
    static fastring _profile;
    static fastring _global;
    static fastring _systemconfig;
};

} // namespace barrier

#endif
