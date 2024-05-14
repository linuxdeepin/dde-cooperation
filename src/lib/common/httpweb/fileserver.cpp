// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "fileserver.h"
#include "tokencache.h"

//#include "server/http/https_session.h"
#include "server/http/http_session.h"
#include "server/http/http_response.h"

#include "system/uuid.h"

//#include "cache/filecache.h"
#include "picojson/picojson.h"

//#include <filesystem>

#define BLOCK_SIZE 4096

class HTTPFileSession : public CppServer::HTTP::HTTPSession
{
public:
    using CppServer::HTTP::HTTPSession::HTTPSession;

protected:
    void serveFileInfo(const CppCommon::Path &path)
    {
        CppCommon::File temp(path);
        if (temp.IsFileExists()) {
            picojson::array arr;
            picojson::object obj;
            obj["name"] = picojson::value(temp.filename().string());
            obj["size"] = picojson::value(std::to_string(temp.size()));
            arr.push_back(picojson::value(obj));

            picojson::value val(arr);
            std::string json_str = val.serialize();

            SendResponseAsync(response().MakeGetResponse(json_str, "application/json; charset=UTF-8"));
        } else {
            SendResponseAsync(response().MakeErrorResponse(404, "File not found."));
        }
    }

    void serveDirectoryInfo(const CppCommon::Path &path)
    {
        picojson::array arr;
        for (const auto &item : CppCommon::Directory(path)) {
            const CppCommon::Path entry = item.IsSymlink() ? CppCommon::Symlink(item).target() : item;

            picojson::object obj;
            arr.push_back(picojson::value(obj));

            auto name = entry.filename().string();
            if (entry.IsDirectory()) {
                obj["name"] = picojson::value(std::string(name + "/"));
                obj["size"] = picojson::value(std::to_string(0)); // mask as folder flag.
            } else {
                CppCommon::File temp(entry);
                temp.Open(true, false);
                obj["name"] = picojson::value(name);
                obj["size"] = picojson::value(std::to_string(temp.size())); // mask as file flag.
                temp.Close();
            }
            arr.push_back(picojson::value(obj));
        }

        picojson::value val(arr);
        std::string json_str = val.serialize();

        // Response with all dir/file values
        SendResponseAsync(response().MakeGetResponse(json_str, "application/json; charset=UTF-8"));
    }

    void serveFile(const CppCommon::Path &path)
    {
        std::string mediaType = path.extension().string();

        CppCommon::File temp(path);
        temp.Open(true, false);
        if (temp.IsFileExists()) {
            size_t sz = temp.size();
            temp.Seek(0);

            response().Clear();
            response().SetBegin(200);
            response().SetContentType(mediaType);
            response().SetBodyLength(sz);
            response().SetHeader("Server: ", "FileServer");

            // send headers
            // SendResponseAsync();
            SendResponse(response());

            std::cout << "response header:" << response() << std::endl;

            size_t read_sz = 0;
            char buff[4096];
            do {
                read_sz = temp.Read(buff, sizeof(buff));
                // std::cout << "read len:" << read_sz << std::endl;
                SendResponseBody(buff, read_sz);
            } while (read_sz > 0);
        } else {
            SendResponseAsync(response().MakeErrorResponse(404, "File not found."));
        }
        temp.Close();
        std::cout << "response body end:" << std::endl;
    }

    void serveDirectory(const CppCommon::Path &path)
    {
        std::string result;
        result += "[\n";
        for (const auto &item : CppCommon::Directory(path)) {
            const CppCommon::Path entry = item.IsSymlink() ? CppCommon::Symlink(item).target() : item;
            std::string href;

            auto name = entry.filename().string();
            if (entry.IsDirectory()) {
                href = name + "/";
            } else {
                href = name;
            }
            result += "\"file\": \"" + href + "\",\n";
        }
        result += "]\n";

        // Response with all dir/file values
        SendResponseAsync(response().MakeGetResponse(result, "application/json; charset=UTF-8"));
    }

    void serveInfo(const CppCommon::Path &path)
    {
        CppCommon::File info(path);
        if (info.IsExists()) {
            picojson::array arr;
            if (info.IsDirectory()) {
                for (const auto &item : CppCommon::Directory(path)) {
                    const CppCommon::Path entry = item.IsSymlink() ? CppCommon::Symlink(item).target() : item;

                    picojson::object obj;
                    arr.push_back(picojson::value(obj));

                    auto name = entry.filename().string();
                    obj["name"] = picojson::value(name);
                    if (entry.IsDirectory()) {
//                        obj["name"] = picojson::value(std::string(name + "/"));
                        obj["size"] = picojson::value(std::to_string(0)); // mask as folder flag.
                    } else {
                        CppCommon::File temp(entry);
                        temp.Open(true, false);
//                        obj["name"] = picojson::value(name);
                        obj["size"] = picojson::value(std::to_string(temp.size())); // mask as file flag.
                        temp.Close();
                    }
                    arr.push_back(picojson::value(obj));
                }
            } else if (info.IsRegularFile()){
                info.Open(true, false);
                picojson::object obj;
                obj["name"] = picojson::value(info.filename().string());
                obj["size"] = picojson::value(std::to_string(info.size())); // mask as file flag.
                arr.push_back(picojson::value(obj));
                info.Close();
            } else {
                std::cout << "this is link file: " << path.absolute() << std::endl;
            }

            picojson::value val(arr);
            std::string json_str = val.serialize();

            // Response with all dir/file values
            SendResponseAsync(response().MakeGetResponse(json_str, "application/json; charset=UTF-8"));
        } else {
            SendResponseAsync(response().MakeErrorResponse(404, "Not found."));
        }
    }

    void serveContent(const CppCommon::Path &path, size_t offset)
    {
        CppCommon::File info(path);
        if (info.IsExists()) {
            response().Clear();
            response().SetBegin(200);

            if (info.IsDirectory()) {
                response().SetBodyLength(0);
                response().SetHeader("Flag", "dir");
                response().SetBody("");

                SendResponseAsync(response());
            } else if (info.IsRegularFile()){
                info.Open(true, false);

                size_t sz = info.size();
                if (offset < sz)
                    info.Seek(offset); // seek to offset for breakpoint continue

                response().SetContentType(info.extension().string());
                response().SetBodyLength(sz);
                response().SetHeader("Flag", "file");

                // send headers first
                SendResponse(response());

                std::cout << "response header:" << response() << std::endl;

                size_t read_sz = 0;
                char buff[BLOCK_SIZE];
                do {
                    read_sz = info.Read(buff, sizeof(buff));
                    // std::cout << "read len:" << read_sz << std::endl;
                    SendResponseBody(buff, read_sz);
                } while (read_sz > 0);

                info.Close();
            } else {
                std::cout << "this is link file: " << path.absolute() << std::endl;
            }
        } else {
            SendResponseAsync(response().MakeErrorResponse(404, "Not found."));
        }

        std::string mediaType = path.extension().string();

        CppCommon::File temp(path);
        temp.Open(true, false);
        if (temp.IsFileExists()) {
            size_t sz = temp.size();
            temp.Seek(0);

            response().Clear();
            response().SetBegin(200);
            response().SetContentType(mediaType);
            response().SetBodyLength(sz);
            response().SetHeader("Server: ", "FileServer");

            // send headers
            // SendResponseAsync();
            SendResponse(response());

            std::cout << "response header:" << response() << std::endl;

            size_t read_sz = 0;
            char buff[4096];
            do {
                read_sz = temp.Read(buff, sizeof(buff));
                // std::cout << "read len:" << read_sz << std::endl;
                SendResponseBody(buff, read_sz);
            } while (read_sz > 0);
        } else {
            SendResponseAsync(response().MakeErrorResponse(404, "File not found."));
        }
        temp.Close();
        std::cout << "response body end:" << std::endl;
    }

    // 解析URL中的query参数
    std::unordered_map<std::string, std::string> parseQueryParams(const std::string &query)
    {
        std::unordered_map<std::string, std::string> params;

        size_t start = 0;
        size_t end = 0;

        while ((end = query.find('&', start)) != std::string::npos) {
            std::string param = query.substr(start, end - start);
            size_t eqPos = param.find('=');
            if (eqPos != std::string::npos) {
                params[param.substr(0, eqPos)] = param.substr(eqPos + 1);
            }
            start = end + 1;
        }

        std::string lastParam = query.substr(start);
        size_t eqPos = lastParam.find('=');
        if (eqPos != std::string::npos) {
            params[lastParam.substr(0, eqPos)] = lastParam.substr(eqPos + 1);
        }

        return params;
    }

    void onReceivedRequest(const CppServer::HTTP::HTTPRequest &request)
    {
        // Show HTTP request content
        std::cout << std::endl << request;

        // Process HTTP request methods
        if (request.method() == "HEAD")
            SendResponseAsync(response().MakeHeadResponse());
        else if (request.method() == "GET") {
            // std::string url = "info/pathname&token=xxx";
            // std::string url = "download/pathname&token=xxx&offset=xxx";
            std::string url = std::string(request.url());

            size_t pathEnd = url.find('&');
            std::string path = url.substr(0, pathEnd);

            size_t queryStart = url.find('&') + 1;
            std::string query = url.substr(queryStart);

            std::unordered_map<std::string, std::string> queryParams = parseQueryParams(query);
            std::string name = path.substr(path.find('/') + 1);
            std::string token = queryParams["token"];
            std::string pathvalue;

            std::string method = path.substr(0, path.find('/'));

            // 检查token是否正确
            if (TokenCache::GetInstance().getCacheValue(token, pathvalue)) {
                CppCommon::Path diskpath = WebBinder::GetInstance().getPath(name); //pathvalue + "/" + name;
                // 处理predownload或download请求的name
                if (method.find("info") != std::string::npos) {
                    // 处理predownload请求的name
                    std::cout << "处理info请求的name: " << name << std::endl;
                    serveInfo(diskpath);
                } else if (method.find("download") != std::string::npos) {
                    // 处理download请求的name
                    std::cout << "处理download请求的name: " << name << std::endl;
                    std::string offstr = queryParams["offset"];
                    size_t offset = 0;
                    if (!offstr.empty()) {
                        offset = std::stoll(offstr);
                    }

                    serveContent(diskpath, offset);
                } else {
                    SendResponseAsync(response().MakeErrorResponse("Unsupported HTTP request: " + method));
                }
            } else {
                std::cout << "Token不正确" << std::endl;
                // Response reject
                SendResponseAsync(response().MakeErrorResponse(404, "Invalid auth token!"));
            }
        } else
            SendResponseAsync(response().MakeErrorResponse("Unsupported HTTP method: " + std::string(request.method())));
    }

    void onReceivedRequestError(const CppServer::HTTP::HTTPRequest &request, const std::string &error) override
    {
        std::cout << "Request error: " << error << std::endl;
    }

    void onError(int error, const std::string &category, const std::string &message) override
    {
        std::cout << "HTTP session caught an error with code " << error << " and category '" << category << "': " << message << std::endl;
    }
};

class HTTPFileServer : public CppServer::HTTP::HTTPServer
{
public:
    using CppServer::HTTP::HTTPServer::HTTPServer;

protected:
    std::shared_ptr<CppServer::Asio::TCPSession> CreateSession(const std::shared_ptr<CppServer::Asio::TCPServer> &server) override
    {
        return std::make_shared<HTTPFileSession>(std::dynamic_pointer_cast<CppServer::HTTP::HTTPServer>(server));
    }

protected:
    void onError(int error, const std::string &category, const std::string &message) override
    {
        std::cout << "HTTP server caught an error with code " << error << " and category '" << category << "': " << message << std::endl;
    }
};

FileServer::FileServer(const std::shared_ptr<CppServer::Asio::Service> &service, int port)
{
    // Create HTTP client if the current one is empty
    if (!_httpServer) {
        _httpServer = std::make_shared<HTTPFileServer>(service, port);
        _httpServer->SetupReuseAddress(true);
        _httpServer->SetupReusePort(true);
    }
}

FileServer::~FileServer()
{
    if (_httpServer) {
        _httpServer->DisconnectAll();
        _httpServer->Stop();
        _httpServer = nullptr;
    }
}

bool FileServer::start()
{
    if (_httpServer) {
        return _httpServer->IsStarted() ? _httpServer->Restart() : _httpServer->Start();
    }

    return false;
}

bool FileServer::stop()
{
    if (_httpServer) {
        return _httpServer->Stop();
    }

    return false;
}

std::string FileServer::addFileContent(const CppCommon::Path &path)
{
    // genrate token for this path, and save them into TokenCache<token, abspath>
    // Put the cache value
    std::string abspath(path.absolute().string());
    std::string token(CppCommon::UUID::Random().string());
    TokenCache::GetInstance().putCacheValue(token, abspath);

    return token;
}

void FileServer::setWeb(std::string &token, const std::string &path)
{
    CppCommon::Path web(path);
    std::string abspath(web.absolute().string());
    TokenCache::GetInstance().putCacheValue(token, abspath);
}

// bind: "/images", "C:/Users/username/Pictures/images"
// getpath(): resturn
//    "/images/photo.jpg" -> C:/Users/username/Pictures/images/photo.jpg
//    "/images/2022/" -> C:/Users/username/Pictures/images/2022/

// bind: "/images/*", "C:/Users/username/Pictures/"
// getpath(): resturn
//    "/images/photo.jpg" -> C:/Users/username/Pictures/photo.jpg
//    "/images/2022/" -> C:/Users/username/Pictures/2022/
int FileServer::webBind(std::string webDir, std::string diskDir)
{
    int bind =  WebBinder::GetInstance().bind(webDir, diskDir);
    if (bind == -1) throw std::invalid_argument("Web binding exists.");
    if (bind == -2) throw std::invalid_argument("Not a valid web path.");
    if (bind == -3) throw std::invalid_argument("Not a valid disk path.");
    if (bind == -4) throw std::invalid_argument("Not a valid combinaton of web and disk path.");
    return bind;
}

int FileServer::webUnbind(std::string webDir)
{
    return WebBinder::GetInstance().unbind(webDir);
}

void FileServer::clearBind()
{
    WebBinder::GetInstance().clear();
}

void FileServer::clearToken()
{
    TokenCache::GetInstance().clearCache();
}
