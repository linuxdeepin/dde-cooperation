#include <catch2/catch_all.hpp>
#include <http/http_client.h>
#include <http/http_server.h>
#include <http/http_request.h>
#include <http/http_response.h>
#include <asio/service.h>
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <atomic>

using namespace NetUtil::Asio;
using namespace NetUtil::HTTP;

// 用于测试的简单HTTP服务器
class TestHTTPServer {
public:
    explicit TestHTTPServer(int port) : _port(port), _running(false) {}
    
    void Start() {
        if (_running)
            return;
            
        _running = true;
        _thread = std::thread([this]() {
            auto service = std::make_shared<Service>();
            // 启动服务
            service->Start();
            
            // 创建自定义HTTP服务器类来处理请求
            class TestServer : public HTTPServer {
            public:
                TestServer(const std::shared_ptr<Service>& service, const std::string& address, int port)
                    : HTTPServer(service, address, port) {}
                
            protected:
                // 重写处理HTTP请求的方法
                class TestSession : public HTTPSession {
                public:
                    explicit TestSession(const std::shared_ptr<HTTPServer>& server) 
                        : HTTPSession(server) {}
                    
                protected:
                    void onReceivedRequest(const HTTPRequest& request) override {
                        // 创建响应
                        HTTPResponse response;
                        
                        // 根据请求路径提供不同的响应
                        if (request.url() == "/test") {
                            response.MakeOKResponse();
                            response.SetContentType("text/plain");
                            response.SetBody("Test response");
                        } else if (request.url() == "/json") {
                            response.MakeOKResponse();
                            response.SetContentType("application/json");
                            response.SetBody("{\"status\":\"success\"}");
                        } else if (request.url() == "/echo") {
                            // 回显请求体
                            response.MakeOKResponse();
                            response.SetContentType("text/plain");
                            response.SetBody(request.body());
                        } else if (request.url() == "/headers") {
                            // 返回请求头
                            std::string result;
                            // 使用头部访问方法
                            for (size_t i = 0; i < request.headers(); ++i) {
                                auto [key, value] = request.header(i);
                                result += std::string(key) + ": " + std::string(value) + "\n";
                            }
                            response.MakeOKResponse();
                            response.SetContentType("text/plain");
                            response.SetBody(result);
                        } else {
                            // 404 未找到
                            response.MakeErrorResponse(404, "Not Found");
                        }
                        
                        // 发送响应
                        SendResponseAsync(response);
                    }
                }; // TestSession

                std::shared_ptr<NetUtil::Asio::TCPSession> CreateSession(const std::shared_ptr<NetUtil::Asio::TCPServer>& server) override {
                    return std::make_shared<TestSession>(std::dynamic_pointer_cast<HTTPServer>(server));
                }
            }; // TestServer
            
            auto server = std::make_shared<TestServer>(service, "127.0.0.1", _port);
            
            // 启动服务器
            REQUIRE(server->Start());
            
            // 等待停止信号
            std::unique_lock<std::mutex> lock(_mutex);
            _cv.wait(lock, [this]() { return !_running; });
            
            // 停止服务器
            server->Stop();
            service->Stop();
        });
    }
    
    void Stop() {
        if (!_running)
            return;
            
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _running = false;
        }
        _cv.notify_all();
        
        if (_thread.joinable())
            _thread.join();
    }
    
    ~TestHTTPServer() {
        Stop();
    }
    
private:
    int _port;
    std::atomic<bool> _running;
    std::thread _thread;
    std::mutex _mutex;
    std::condition_variable _cv;
};

// 用于测试的HTTP客户端类，继承HttpClientEx以支持异步操作和回调
class TestHTTPClient : public HTTPClientEx {
public:
    TestHTTPClient(
        const std::shared_ptr<Service>& service, 
        const std::string& address, 
        int port,
        std::mutex& mutex,
        std::condition_variable& cv,
        bool& responseReceived,
        std::shared_ptr<HTTPResponse>& receivedResponse
    ) : HTTPClientEx(service, address, port),
        _mutex(mutex),
        _cv(cv),
        _responseReceived(responseReceived),
        _receivedResponse(receivedResponse) {}
        
protected:
    void onReceivedResponse(const HTTPResponse& response) override {
        std::lock_guard<std::mutex> lock(_mutex);
        _receivedResponse = std::make_shared<HTTPResponse>(response);
        _responseReceived = true;
        _cv.notify_one();
        
        HTTPClientEx::onReceivedResponse(response);
    }
    
private:
    std::mutex& _mutex;
    std::condition_variable& _cv;
    bool& _responseReceived;
    std::shared_ptr<HTTPResponse>& _receivedResponse;
};

TEST_CASE("HTTPClient basic functionality", "[http_client]") {
    // 使用随机端口启动测试HTTP服务器
    const int PORT = 19000 + (std::rand() % 1000);
    TestHTTPServer server(PORT);
    server.Start();
    
    // 短暂延迟确保服务器启动
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    SECTION("Create HTTP client") {
        auto service = std::make_shared<Service>();
        service->Start();
        auto client = std::make_shared<HTTPClient>(service, "127.0.0.1", PORT);
        
        REQUIRE(client->address() == "127.0.0.1");
        REQUIRE(client->port() == PORT);
        
        service->Stop();
    }
    
    SECTION("Send GET request") {
        auto service = std::make_shared<Service>();
        service->Start();
        
        std::mutex mutex;
        std::condition_variable cv;
        bool responseReceived = false;
        std::shared_ptr<HTTPResponse> receivedResponse;
        
        auto client = std::make_shared<TestHTTPClient>(
            service, "127.0.0.1", PORT, 
            mutex, cv, responseReceived, receivedResponse);
        
        // 创建GET请求
        client->request().SetBegin("GET", "/test", "HTTP/1.1");
        
        // 异步发送请求
        std::future<HTTPResponse> future = client->SendRequest();
        
        // 等待响应
        {
            std::unique_lock<std::mutex> lock(mutex);
            bool result = cv.wait_for(lock, std::chrono::seconds(2), [&]() { return responseReceived; });
            REQUIRE(result);
            REQUIRE(receivedResponse != nullptr);
            REQUIRE(receivedResponse->status() == 200);
            REQUIRE(receivedResponse->body() == "Test response");
        }
        
        service->Stop();
    }
    
    SECTION("Send POST request with data") {
        auto service = std::make_shared<Service>();
        service->Start();
        
        std::mutex mutex;
        std::condition_variable cv;
        bool responseReceived = false;
        std::shared_ptr<HTTPResponse> receivedResponse;
        
        auto client = std::make_shared<TestHTTPClient>(
            service, "127.0.0.1", PORT, 
            mutex, cv, responseReceived, receivedResponse);
        
        // 创建POST请求
        const std::string postData = "Hello, HTTP Server!";
        client->request().MakePostRequest("/echo", postData);
        
        // 异步发送请求
        std::future<HTTPResponse> future = client->SendRequest();
        
        // 等待响应
        {
            std::unique_lock<std::mutex> lock(mutex);
            bool result = cv.wait_for(lock, std::chrono::seconds(2), [&]() { return responseReceived; });
            REQUIRE(result);
            REQUIRE(receivedResponse != nullptr);
            REQUIRE(receivedResponse->status() == 200);
            REQUIRE(receivedResponse->body() == postData);
        }
        
        service->Stop();
    }
    
    SECTION("Send request with custom headers") {
        auto service = std::make_shared<Service>();
        service->Start();
        
        std::mutex mutex;
        std::condition_variable cv;
        bool responseReceived = false;
        std::shared_ptr<HTTPResponse> receivedResponse;
        
        auto client = std::make_shared<TestHTTPClient>(
            service, "127.0.0.1", PORT, 
            mutex, cv, responseReceived, receivedResponse);
        
        // 创建带自定义头的请求
        client->request().SetBegin("GET", "/headers", "HTTP/1.1");
        client->request().SetHeader("X-Custom-Header", "CustomValue");
        
        // 异步发送请求
        std::future<HTTPResponse> future = client->SendRequest();
        
        // 等待响应
        {
            std::unique_lock<std::mutex> lock(mutex);
            bool result = cv.wait_for(lock, std::chrono::seconds(2), [&]() { return responseReceived; });
            REQUIRE(result);
            REQUIRE(receivedResponse != nullptr);
            REQUIRE(receivedResponse->status() == 200);
            REQUIRE(receivedResponse->body().find("X-Custom-Header: CustomValue") != std::string::npos);
        }
        
        service->Stop();
    }
    
    SECTION("Request to non-existent URL") {
        auto service = std::make_shared<Service>();
        service->Start();
        
        std::mutex mutex;
        std::condition_variable cv;
        bool responseReceived = false;
        std::shared_ptr<HTTPResponse> receivedResponse;
        
        auto client = std::make_shared<TestHTTPClient>(
            service, "127.0.0.1", PORT, 
            mutex, cv, responseReceived, receivedResponse);
        
        // 创建请求到不存在的URL
        client->request().SetBegin("GET", "/nonexistent", "HTTP/1.1");
        
        // 异步发送请求
        std::future<HTTPResponse> future = client->SendRequest();
        
        // 等待响应
        {
            std::unique_lock<std::mutex> lock(mutex);
            bool result = cv.wait_for(lock, std::chrono::seconds(2), [&]() { return responseReceived; });
            REQUIRE(result);
            REQUIRE(receivedResponse != nullptr);
            REQUIRE(receivedResponse->status() == 404);
        }
        
        service->Stop();
    }
    
    // 停止服务器
    server.Stop();
} 