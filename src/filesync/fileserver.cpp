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

#include <filesystem>

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

    // 解析URL中的query参数
    std::unordered_map<std::string, std::string> parseQueryParams(const std::string& query) {
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
            // std::string url = "predownload/pathname&token=xxx";
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

            // 检查token是否正确
            if (TokenCache::GetInstance().GetCacheValue(token, pathvalue)) {
                // 处理predownload或download请求的name
                if (path.find("predownload") != std::string::npos) {
                    // 处理predownload请求的name
                    std::cout << "处理predownload请求的name: " << name << std::endl;
                    CppCommon::Path path = pathvalue + "/" + name;
                    if (path.IsDirectory()) {
                        serveDirectoryInfo(path);
                    } else {
                        serveFileInfo(path);
                    }
                } else if (path.find("download") != std::string::npos) {
                    // 处理download请求的name
                    std::cout << "处理download请求的name: " << name << std::endl;
                    CppCommon::Path path = pathvalue + "/" + name;
                    if (path.IsDirectory()) {
                        // serveDirectory(path);
                        SendResponseAsync(response().MakeGetResponse(path.filename().string(), "application/dir; charset=UTF-8"));
                    } else {
                        serveFile(path);
                    }
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

std::string FileServer::AddFileContent(const CppCommon::Path &path)
{
    // genrate token for this path, and save them into TokenCache<token, abspath>
    // Put the cache value
    std::string abspath(path.absolute().string());
    std::string token(CppCommon::UUID::Random().string());
    TokenCache::GetInstance().PutCacheValue(token, abspath);

    return token;
}
