#include <catch2/catch_all.hpp>
#include <asio/service.h>
#include <asio/tcp_server.h>
#include <asio/tcp_client.h>
#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>
#include <condition_variable>

using namespace NetUtil::Asio;

// 接收数据的会话类
class TestTCPSession : public TCPSession {
public:
    TestTCPSession(const std::shared_ptr<TCPServer>& server, std::mutex& mutex, 
                  std::condition_variable& cv, bool& dataReceived, std::string& receivedData)
        : TCPSession(server), 
          _mutex(mutex), 
          _cv(cv), 
          _dataReceived(dataReceived), 
          _receivedData(receivedData) {}
    
protected:
    void onReceived(const void* buffer, size_t size) override {
        std::lock_guard<std::mutex> lock(_mutex);
        _receivedData.assign(static_cast<const char*>(buffer), size);
        _dataReceived = true;
        
        // 回显数据
        SendAsync(buffer, size);
        
        _cv.notify_one();
    }
    
private:
    std::mutex& _mutex;
    std::condition_variable& _cv;
    bool& _dataReceived;
    std::string& _receivedData;
};

// 接收连接并创建会话的服务器类
class TestTCPServer : public TCPServer {
public:
    TestTCPServer(const std::shared_ptr<Service>& service, const std::string& address, int port,
                 std::mutex& mutex, std::condition_variable& cv)
        : TCPServer(service, address, port), 
          _mutex(mutex), 
          _cv(cv),
          _connectionCount(0),
          _clientConnected(false),
          _dataReceived(false) {}
    
    std::shared_ptr<TCPSession> connectedSession;
    std::mutex& _mutex;
    std::condition_variable& _cv;
    std::atomic<int> _connectionCount;
    bool _clientConnected;
    bool _dataReceived;
    std::string _receivedData;
    
protected:
    void onConnected(std::shared_ptr<TCPSession>& session) override {
        std::lock_guard<std::mutex> lock(_mutex);
        connectedSession = session;
        _clientConnected = true;
        _connectionCount++;
        _cv.notify_one();
    }
    
    void onDisconnected(std::shared_ptr<TCPSession>& session) override {
        _connectionCount--;
    }
    
    std::shared_ptr<TCPSession> CreateSession(const std::shared_ptr<TCPServer>& server) override {
        return std::make_shared<TestTCPSession>(server, _mutex, _cv, _dataReceived, _receivedData);
    }
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

TEST_CASE("TCPServer basic functionality", "[tcp_server]") {
    const int PORT = 19000 + (std::rand() % 1000);
    
    SECTION("Create TCP server") {
        auto service = std::make_shared<Service>();
        service->Start();
        auto server = std::make_shared<TCPServer>(service, "127.0.0.1", PORT);
        
        REQUIRE(server->port() == PORT);
        REQUIRE_FALSE(server->IsStarted());
        
        service->Stop();
    }
    
    SECTION("Start and stop server") {
        auto service = std::make_shared<Service>();
        service->Start();
        auto server = std::make_shared<TCPServer>(service, "127.0.0.1", PORT);
        
        REQUIRE(server->Start());
        REQUIRE(server->IsStarted());
        
        REQUIRE(server->Stop());
        REQUIRE_FALSE(server->IsStarted());
        
        service->Stop();
    }
    
    SECTION("Start server with multiple ports") {
        auto service = std::make_shared<Service>();
        service->Start();
        std::vector<int> ports = {PORT, PORT+1, PORT+2};
        
        // 创建多个服务器实例，每个使用不同的端口
        std::vector<std::shared_ptr<TCPServer>> servers;
        for (int port : ports) {
            auto server = std::make_shared<TCPServer>(service, "127.0.0.1", port);
            REQUIRE(server->Start());
            servers.push_back(server);
        }
        
        // 验证所有服务器都正常启动
        for (const auto& server : servers) {
            REQUIRE(server->IsStarted());
            server->Stop();
        }
        
        service->Stop();
    }
    
    SECTION("Accept client connections") {
        auto service = std::make_shared<Service>();
        service->Start();
        
        std::mutex mutex;
        std::condition_variable cv;
        auto server = std::make_shared<TestTCPServer>(service, "127.0.0.1", PORT, mutex, cv);
        
        REQUIRE(server->Start());
        
        // 创建客户端连接
        auto clientService = std::make_shared<Service>();
        clientService->Start();
        auto client = std::make_shared<TCPClient>(clientService, "127.0.0.1", PORT);
        REQUIRE(client->Connect());
        
        // 等待服务器接受连接
        {
            std::unique_lock<std::mutex> lock(mutex);
            bool result = cv.wait_for(lock, std::chrono::seconds(2), [&server]() { return server->_clientConnected; });
            REQUIRE(result);
            REQUIRE(server->connectedSession != nullptr);
        }
        
        // 验证会话是否有效
        REQUIRE(server->connectedSession->IsConnected());
        
        // 断开连接
        client->Disconnect();
        clientService->Stop();
        server->Stop();
        service->Stop();
    }
    
    SECTION("Send and receive data through server session") {
        auto service = std::make_shared<Service>();
        service->Start();
        
        std::mutex mutex;
        std::condition_variable cv;
        auto server = std::make_shared<TestTCPServer>(service, "127.0.0.1", PORT, mutex, cv);
        
        REQUIRE(server->Start());
        
        // 创建客户端连接
        auto clientService = std::make_shared<Service>();
        clientService->Start();
        auto client = std::make_shared<TestTCPClient>(clientService, "127.0.0.1", PORT);
        
        REQUIRE(client->ConnectAsync());
        
        // 等待连接成功
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // 发送测试数据
        const std::string testData = "Hello, TCPServer!";
        REQUIRE(client->SendAsync(testData));
        
        // 启动异步接收
        client->ReceiveAsync();
        
        // 等待服务器接收数据
        {
            std::unique_lock<std::mutex> lock(mutex);
            bool result = cv.wait_for(lock, std::chrono::seconds(2), [&server]() { return server->_dataReceived; });
            REQUIRE(result);
            REQUIRE(server->_receivedData == testData);
        }
        
        // 等待客户端接收回显数据
        {
            std::unique_lock<std::mutex> lock(client->mutex);
            bool result = client->cv.wait_for(lock, std::chrono::seconds(2), [&client]() { return client->dataReceived; });
            REQUIRE(result);
            REQUIRE(client->receivedData == testData);
        }
        
        // 断开连接
        client->Disconnect();
        clientService->Stop();
        server->Stop();
        service->Stop();
    }
    
    SECTION("Test multiple connections") {
        auto service = std::make_shared<Service>();
        service->Start();
        
        std::mutex mutex;
        std::condition_variable cv;
        auto server = std::make_shared<TestTCPServer>(service, "127.0.0.1", PORT, mutex, cv);
        
        REQUIRE(server->Start());
        
        const int CLIENT_COUNT = 5;
        std::vector<std::shared_ptr<Service>> clientServices;
        std::vector<std::shared_ptr<TCPClient>> clients;
        
        // 创建多个客户端
        for (int i = 0; i < CLIENT_COUNT; ++i) {
            auto clientService = std::make_shared<Service>();
            clientService->Start();
            auto client = std::make_shared<TCPClient>(clientService, "127.0.0.1", PORT);
            REQUIRE(client->Connect());
            
            clientServices.push_back(clientService);
            clients.push_back(client);
        }
        
        // 等待所有连接建立
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        REQUIRE(server->_connectionCount == CLIENT_COUNT);
        
        // 断开部分连接
        for (int i = 0; i < CLIENT_COUNT / 2; ++i) {
            clients[i]->Disconnect();
        }
        
        // 等待断开连接处理完成
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        REQUIRE(server->_connectionCount == CLIENT_COUNT - (CLIENT_COUNT / 2));
        
        // 断开剩余连接
        for (int i = CLIENT_COUNT / 2; i < CLIENT_COUNT; ++i) {
            clients[i]->Disconnect();
        }
        
        // 等待断开连接处理完成
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        REQUIRE(server->_connectionCount == 0);
        
        // 停止所有客户端服务
        for (auto& clientService : clientServices) {
            clientService->Stop();
        }
        
        server->Stop();
        service->Stop();
    }
} 