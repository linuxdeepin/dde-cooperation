// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "webbinder.h"

WebBinder::WebBinder()
{

}

int WebBinder::bind(std::string webDir, std::string diskDir)
{
    // std::regex validateWeb(R"([^\0<>:"/\\|?*]*$)");
    // std::regex validateDisk(R"((\/)?(?![.]{1,2}($|\/))[\w.-]+(\/[\w.-]+)*(\\.[\w]+)?)");
    std::regex isFile(R"(\S*\/$)");

    // std::regex validateWeb(R"([^\0<>:"/\\|?*]*$)"), validateDisk("(\\/)?((\\w)+(\\/)?)*(\\.(\\w)+)?"), isFile("\\S*\\/");
    // std::regex validateWeb("(\\/((\\w)*|\\*))*"), validateDisk("(\\/)?((\\w)+(\\/)?)*(\\.(\\w)+)?"), isFile("\\S*\\/");

    // if (!std::regex_match(webDir, validateWeb)) return -2;
    // if (!std::regex_match(diskDir, validateDisk)) return -3;
    if (std::regex_match(webDir, isFile) != std::regex_match(diskDir, isFile)) return -4;

    for (auto &pair : _binds) if (pair.first.compare(webDir) == 0) return -1;

    _binds.insert(_binds.begin(), std::make_pair(webDir, diskDir));

    return 0;
}

int WebBinder::unbind(std::string webDir)
{
    for (size_t i = 0; i < _binds.size(); i++) {
        if (_binds[i].first.compare(webDir) == 0) {
            _binds.erase(_binds.begin() + i);
            return 0;
        }
    }
    return -1;
}

void WebBinder::clear()
{
    _binds.clear();
}

std::string WebBinder::getPath(std::string path)
{
    for (auto &pair : _binds) {
        std::string regexString(pair.first);
        replaceAll(regexString, "*", "(\\w)+");
        replaceAll(regexString, "/", "\\/");
        std::regex fileRegex(regexString);

        if (std::regex_match(path, fileRegex))
            return pair.second;

        regexString.append("(\\w)+(\\.(\\w)+)?");
        std::regex dirRegex(regexString);
        if (std::regex_match(path, dirRegex))
            return pair.second + path.substr(path.find_last_of("/") + 1, path.length());
    }

    return "";
}

//source: https://stackoverflow.com/questions/3418231/replace-part-of-a-string-with-another-string
void WebBinder::replaceAll(std::string &str, const std::string &from, const std::string &to)
{
    if (from.empty())
        return;
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
}

//source: https://stackoverflow.com/questions/3418231/replace-part-of-a-string-with-another-string
bool WebBinder::replace(std::string &str, const std::string &from, const std::string &to)
{
    size_t start_pos = str.find(from);
    if (start_pos == std::string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}
