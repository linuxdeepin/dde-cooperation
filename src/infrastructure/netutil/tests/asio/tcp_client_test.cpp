#include <catch2/catch_all.hpp>
#include <asio/tcp_client.h>
#include <asio/service.h>
#include <asio/tcp_server.h>
#include <time/timespan.h>
#include <thread>
#include <chrono>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>

using namespace NetUtil::Asio;

// 回显会话类
class EchoTCPSession : public TCPSession {
public:
    explicit EchoTCPSession(const std::shared_ptr<TCPServer>& server)
        : TCPSession(server) {}
        
protected:
    void onReceived(const void* buffer, size_t size) override {
        // 回显接收到的数据
        SendAsync(buffer, size);
    }
};

// 回显服务器类
class EchoTCPServer : public TCPServer {
public:
    EchoTCPServer(const std::shared_ptr<Service>& service, const std::string& address, int port)
        : TCPServer(service, address, port) {}
        
protected:
    std::shared_ptr<TCPSession> CreateSession(const std::shared_ptr<TCPServer>& server) override {
        return std::make_shared<EchoTCPSession>(server);
    }
};

// 辅助函数：创建简单的回显服务器用于测试
class EchoServer {
public:
    explicit EchoServer(int port) : _port(port), _running(false) {}
    
    void Start() {
        if (_running)
            return;
            
        _running = true;
        _thread = std::thread([this]() {
            auto service = std::make_shared<Service>();
            service->Start();
            
            // 使用自定义的回显服务器
            auto server = std::make_shared<EchoTCPServer>(service, "127.0.0.1", _port);
            
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
    
    ~EchoServer() {
        Stop();
    }
    
private:
    int _port;
    std::atomic<bool> _running;
    std::thread _thread;
    std::mutex _mutex;
    std::condition_variable _cv;
};

// 接收数据的客户端类
class TestTCPClient : public TCPClient {
public:
    TestTCPClient(const std::shared_ptr<Service>& service, 
                 const std::string& address, int port)
        : TCPClient(service, address, port), 
          dataReceived(false) {}

    std::mutex mutex;
    std::condition_variable cv;
    bool dataReceived;
    std::string receivedData;
    
protected:
    void onReceived(const void* buffer, size_t size) override {
        std::lock_guard<std::mutex> lock(mutex);
        receivedData.assign(static_cast<const char*>(buffer), size);
        dataReceived = true;
        cv.notify_one();
    }
};

TEST_CASE("TCPClient basic functionality", "[tcp_client]") {
    // 使用随机端口启动回显服务器
    const int PORT = 19000 + (std::rand() % 1000);
    EchoServer server(PORT);
    server.Start();
    
    // 短暂延迟确保服务器启动
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    SECTION("Create TCP client") {
        auto service = std::make_shared<Service>();
        service->Start();
        auto client = std::make_shared<TCPClient>(service, "127.0.0.1", PORT);
        
        REQUIRE(client->address() == "127.0.0.1");
        REQUIRE(client->port() == PORT);
        REQUIRE_FALSE(client->IsConnected());
        
        service->Stop();
    }
    
    SECTION("Connect to server") {
        auto service = std::make_shared<Service>();
        service->Start();
        auto client = std::make_shared<TCPClient>(service, "127.0.0.1", PORT);
        
        REQUIRE(client->Connect());
        REQUIRE(client->IsConnected());
        
        client->Disconnect();
        REQUIRE_FALSE(client->IsConnected());
        
        service->Stop();
    }
    
    SECTION("Send and receive data") {
        auto service = std::make_shared<Service>();
        service->Start();
        auto client = std::make_shared<TCPClient>(service, "127.0.0.1", PORT);
        
        REQUIRE(client->Connect());
        
        const std::string test_data = "Hello, TCP Server!";
        size_t sent = client->Send(test_data);
        REQUIRE(sent == test_data.size());
        
        // 等待服务器回显
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // 接收回显数据
        std::string received = client->Receive(test_data.size());
        REQUIRE(received == test_data);
        
        client->Disconnect();
        service->Stop();
    }
    
    SECTION("Test with timeout") {
        auto service = std::make_shared<Service>();
        service->Start();
        auto client = std::make_shared<TCPClient>(service, "127.0.0.1", PORT);
        
        REQUIRE(client->Connect());
        
        const std::string test_data = "Timeout Test";
        BaseKit::Timespan timeout(1000); // 1秒超时
        
        size_t sent = client->Send(test_data, timeout);
        REQUIRE(sent == test_data.size());
        
        // 等待服务器回显
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // 接收回显数据
        std::string received = client->Receive(test_data.size(), timeout);
        REQUIRE(received == test_data);
        
        client->Disconnect();
        service->Stop();
    }
    
    SECTION("Test asynchronous API") {
        auto service = std::make_shared<Service>();
        service->Start();
        auto client = std::make_shared<TestTCPClient>(service, "127.0.0.1", PORT);
        
        REQUIRE(client->ConnectAsync());
        
        // 等待连接成功
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        REQUIRE(client->IsConnected());
        
        // 异步发送数据
        const std::string test_data = "Async Test";
        REQUIRE(client->SendAsync(test_data));
        
        // 启动异步接收
        client->ReceiveAsync();
        
        // 等待接收完成
        {
            std::unique_lock<std::mutex> lock(client->mutex);
            bool result = client->cv.wait_for(lock, std::chrono::seconds(2), [&client]() { return client->dataReceived; });
            REQUIRE(result);
            REQUIRE(client->receivedData == test_data);
        }
        
        client->DisconnectAsync();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        REQUIRE_FALSE(client->IsConnected());
        
        service->Stop();
    }
    
    // 停止服务器
    server.Stop();
} 