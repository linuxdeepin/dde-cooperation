#include <catch2/catch_all.hpp>
#include <asio/service.h>
#include <asio/udp_client.h>
#include <asio/udp_server.h>
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <atomic>

using namespace NetUtil::Asio;

TEST_CASE("UDP client and server functionality", "[udp]") {
    const int PORT = 19000 + (std::rand() % 1000);
    
    SECTION("Create UDP server") {
        auto service = std::make_shared<Service>();
        service->Start();
        auto server = std::make_shared<UDPServer>(service, PORT);
        
        REQUIRE(server->port() == PORT);
        REQUIRE_FALSE(server->IsStarted());
        
        service->Stop();
    }
    
    SECTION("Start and stop UDP server") {
        auto service = std::make_shared<Service>();
        service->Start();
        auto server = std::make_shared<UDPServer>(service, PORT);
        
        REQUIRE(server->Start());
        REQUIRE(server->IsStarted());
        
        REQUIRE(server->Stop());
        REQUIRE_FALSE(server->IsStarted());
        
        service->Stop();
    }
    
    SECTION("Create UDP client") {
        auto service = std::make_shared<Service>();
        service->Start();
        auto client = std::make_shared<UDPClient>(service, "127.0.0.1", PORT);
        
        REQUIRE(client->address() == "127.0.0.1");
        REQUIRE(client->port() == PORT);
        
        service->Stop();
    }
    
    SECTION("Send and receive UDP data") {
        auto serverService = std::make_shared<Service>();
        serverService->Start();
        
        // 创建继承UDPServer的自定义类处理回调
        class TestUDPServer : public UDPServer {
        public:
            TestUDPServer(
                const std::shared_ptr<Service>& service, 
                int port,
                std::mutex& mutex,
                std::condition_variable& cv,
                bool& dataReceived,
                std::string& receivedData,
                asio::ip::udp::endpoint& senderEndpoint
            ) : UDPServer(service, port),
                _mutex(mutex),
                _cv(cv),
                _dataReceived(dataReceived),
                _receivedData(receivedData),
                _senderEndpoint(senderEndpoint) {}
        
        protected:
            void onReceived(const asio::ip::udp::endpoint& endpoint, const void* buffer, size_t size) override {
                std::lock_guard<std::mutex> lock(_mutex);
                _senderEndpoint = endpoint;
                _receivedData.assign(static_cast<const char*>(buffer), size);
                _dataReceived = true;
                _cv.notify_one();
                
                // 回显数据
                SendAsync(endpoint, buffer, size);
            }
            
        private:
            std::mutex& _mutex;
            std::condition_variable& _cv;
            bool& _dataReceived;
            std::string& _receivedData;
            asio::ip::udp::endpoint& _senderEndpoint;
        };
        
        std::mutex mutex;
        std::condition_variable cv;
        bool dataReceived = false;
        std::string receivedData;
        asio::ip::udp::endpoint senderEndpoint;
        
        auto server = std::make_shared<TestUDPServer>(
            serverService, PORT, mutex, cv, dataReceived, receivedData, senderEndpoint);
        
        REQUIRE(server->Start());
        
        // 创建继承UDPClient的自定义类处理回调
        class TestUDPClient : public UDPClient {
        public:
            TestUDPClient(
                const std::shared_ptr<Service>& service, 
                const std::string& address, 
                int port,
                std::mutex& mutex,
                std::condition_variable& cv,
                bool& dataReceived,
                std::string& receivedData
            ) : UDPClient(service, address, port),
                _mutex(mutex),
                _cv(cv),
                _dataReceived(dataReceived),
                _receivedData(receivedData) {}
                
        protected:
            void onReceived(const asio::ip::udp::endpoint& endpoint, const void* buffer, size_t size) override {
                std::lock_guard<std::mutex> lock(_mutex);
                _receivedData.assign(static_cast<const char*>(buffer), size);
                _dataReceived = true;
                _cv.notify_one();
            }
            
        private:
            std::mutex& _mutex;
            std::condition_variable& _cv;
            bool& _dataReceived;
            std::string& _receivedData;
        };
        
        auto clientService = std::make_shared<Service>();
        clientService->Start();
        
        std::mutex clientMutex;
        std::condition_variable clientCv;
        bool clientDataReceived = false;
        std::string clientReceivedData;
        
        auto client = std::make_shared<TestUDPClient>(
            clientService, "127.0.0.1", PORT, 
            clientMutex, clientCv, clientDataReceived, clientReceivedData);
        
        // 发送数据
        const std::string testData = "Hello, UDP Server!";
        REQUIRE(client->SendAsync(testData));
        
        // 启动客户端异步接收
        client->ReceiveAsync();
        
        // 等待服务器接收数据
        {
            std::unique_lock<std::mutex> lock(mutex);
            bool result = cv.wait_for(lock, std::chrono::seconds(2), [&]() { return dataReceived; });
            REQUIRE(result);
            REQUIRE(receivedData == testData);
        }
        
        // 等待客户端接收回显数据
        {
            std::unique_lock<std::mutex> lock(clientMutex);
            bool result = clientCv.wait_for(lock, std::chrono::seconds(2), [&]() { return clientDataReceived; });
            REQUIRE(result);
            REQUIRE(clientReceivedData == testData);
        }
        
        // 停止服务
        client->Disconnect();
        clientService->Stop();
        server->Stop();
        serverService->Stop();
    }
    
    SECTION("Test UDP multicast") {
        // 多播地址
        const std::string MULTICAST_ADDRESS = "239.255.0.1";
        auto serverService = std::make_shared<Service>();
        serverService->Start();
        
        // 创建自定义的UDP服务器处理多播接收
        class MulticastUDPServer : public UDPServer {
        public:
            MulticastUDPServer(
                const std::shared_ptr<Service>& service, 
                int port,
                std::mutex& mutex,
                std::condition_variable& cv,
                std::atomic<int>& receiveCount
            ) : UDPServer(service, port),
                _mutex(mutex),
                _cv(cv),
                _receiveCount(receiveCount) {}
                
            // 自定义的加入多播组方法
            bool JoinMulticastGroup(const std::string& address) {
                try {
                    // 实现加入多播组的逻辑
                    auto multicast_endpoint = asio::ip::udp::endpoint(
                        asio::ip::address::from_string(address), port());
                    return Start(multicast_endpoint);
                } catch (const std::exception&) {
                    return false;
                }
            }
        
        protected:
            void onReceived(const asio::ip::udp::endpoint& endpoint, const void* buffer, size_t size) override {
                _receiveCount++;
                _cv.notify_one();
            }
            
        private:
            std::mutex& _mutex;
            std::condition_variable& _cv;
            std::atomic<int>& _receiveCount;
        };
        
        std::mutex mutex;
        std::condition_variable cv;
        std::atomic<int> receiveCount{0};
        
        auto server = std::make_shared<MulticastUDPServer>(
            serverService, PORT, mutex, cv, receiveCount);
        
        // 设置服务器加入多播组
        REQUIRE(server->JoinMulticastGroup(MULTICAST_ADDRESS));
        
        // 创建多个UDP客户端
        const int CLIENT_COUNT = 3;
        std::vector<std::shared_ptr<Service>> clientServices;
        std::vector<std::shared_ptr<UDPClient>> clients;
        
        for (int i = 0; i < CLIENT_COUNT; ++i) {
            auto clientService = std::make_shared<Service>();
            clientService->Start();
            auto client = std::make_shared<UDPClient>(clientService, MULTICAST_ADDRESS, PORT);
            
            // 设置多播选项
            client->SetupMulticast(true);
            
            clientServices.push_back(clientService);
            clients.push_back(client);
        }
        
        // 从第一个客户端发送多播消息
        const std::string testData = "Multicast Test";
        REQUIRE(clients[0]->SendAsync(testData));
        
        // 等待服务器接收到多播消息
        {
            std::unique_lock<std::mutex> lock(mutex);
            bool result = cv.wait_for(lock, std::chrono::seconds(2), [&]() { return receiveCount > 0; });
            REQUIRE(result);
        }
        
        // 停止所有客户端
        for (auto& client : clients) {
            client->Disconnect();
        }
        
        // 停止所有客户端服务
        for (auto& clientService : clientServices) {
            clientService->Stop();
        }
        
        // 停止服务器
        server->Stop();
        serverService->Stop();
    }
    
    SECTION("Test UDP broadcast") {
        // 广播地址
        const std::string BROADCAST_ADDRESS = "255.255.255.255";
        auto serverService = std::make_shared<Service>();
        serverService->Start();
        
        // 创建自定义UDP服务器处理广播接收
        class BroadcastUDPServer : public UDPServer {
        public:
            BroadcastUDPServer(
                const std::shared_ptr<Service>& service, 
                int port,
                std::mutex& mutex,
                std::condition_variable& cv,
                bool& dataReceived
            ) : UDPServer(service, port),
                _mutex(mutex),
                _cv(cv),
                _dataReceived(dataReceived) {}
        
        protected:
            void onReceived(const asio::ip::udp::endpoint& endpoint, const void* buffer, size_t size) override {
                std::lock_guard<std::mutex> lock(_mutex);
                _dataReceived = true;
                _cv.notify_one();
            }
            
        private:
            std::mutex& _mutex;
            std::condition_variable& _cv;
            bool& _dataReceived;
        };
        
        std::mutex mutex;
        std::condition_variable cv;
        bool dataReceived = false;
        
        auto server = std::make_shared<BroadcastUDPServer>(
            serverService, PORT, mutex, cv, dataReceived);
        
        REQUIRE(server->Start());
        
        auto clientService = std::make_shared<Service>();
        clientService->Start();
        auto client = std::make_shared<UDPClient>(clientService, BROADCAST_ADDRESS, PORT);
        
        // 启用广播 (使用正确的方法)
        client->SetupMulticast(true);
        
        // 发送广播消息
        const std::string testData = "Broadcast Test";
        REQUIRE(client->SendAsync(testData));
        
        // 等待服务器接收到广播消息
        {
            std::unique_lock<std::mutex> lock(mutex);
            bool result = cv.wait_for(lock, std::chrono::seconds(2), [&]() { return dataReceived; });
            REQUIRE(result);
        }
        
        // 停止客户端和服务器
        client->Disconnect();
        clientService->Stop();
        server->Stop();
        serverService->Stop();
    }
} 