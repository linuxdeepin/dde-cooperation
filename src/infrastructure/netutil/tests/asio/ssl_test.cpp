#include <catch2/catch_all.hpp>
#include <asio/ssl_client.h>
#include <asio/ssl_server.h>
#include <asio/service.h>
#include <asio/ssl_context.h>
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <string>
#include <functional>
#include <memory>
#include <fstream>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/evp.h>

using namespace NetUtil::Asio;

// 测试用自签名证书和私钥（仅用于测试，实际使用应该使用正式证书）
const std::string TEST_CERT = 
"-----BEGIN CERTIFICATE-----\n"
"MIIDazCCAlOgAwIBAgIUEDQLaYpUPvqNkrJxZlC+T6cPXEIwDQYJKoZIhvcNAQEL\n"
"BQAwRTELMAkGA1UEBhMCQVUxEzARBgNVBAgMClNvbWUtU3RhdGUxITAfBgNVBAoM\n"
"GEludGVybmV0IFdpZGdpdHMgUHR5IEx0ZDAeFw0yMDAxMDEwMDAwMDBaFw0zMDAx\n"
"MDEwMDAwMDBaMEUxCzAJBgNVBAYTAkFVMRMwEQYDVQQIDApTb21lLVN0YXRlMSEw\n"
"HwYDVQQKDBhJbnRlcm5ldCBXaWRnaXRzIFB0eSBMdGQwggEiMA0GCSqGSIb3DQEB\n"
"AQUAA4IBDwAwggEKAoIBAQDBDN7JfHBUXI/I6aBl+Ze5+MvbMBBN1EViBCtv1kkM\n"
"wAjZR2M7lgh3NYfZbZW+B6H/HVa3RPcZYbXXfQP4BmQvMPYpyPLtDDAY+lRr42C/\n"
"kYwqM6YJ+rL9aYJvt+Q7RIbQ4ZGciC9z7xnqQeE9oyqC6qtbH9eV9B8D2HGLhYva\n"
"DYIzT6lfUlDKEOIVWNIb0ezIKNGYGsL4v42o4HYCbgSK1p5T4U77E2v+wQ9Qvn0J\n"
"t9vJ2UW9IFsAWrWnkjuQhO3SQV0TJpXZV+GXRpFUQI2nDgQJ71xtM4Ou7KoRoXkL\n"
"aQFclvAWuOPHnzYpD5ljuXJM+nS5LALW+l47OfnRAgMBAAGjUzBRMB0GA1UdDgQW\n"
"BBRI0aSVTfZo0qTfTs/YJNHSZEXs9jAfBgNVHSMEGDAWgBRI0aSVTfZo0qTfTs/Y\n"
"JNHSZEXs9jAPBgNVHRMBAf8EBTADAQH/MA0GCSqGSIb3DQEBCwUAA4IBAQAZr6A2\n"
"EU2S2HKVKlHWvJ9GNbfMKgbNC0GyGRrF/RbUceZZGpgNQ11y5bUtSiOxQvmfELyx\n"
"7t2ZvNNaLUbL7MZpfaZGGvPX1S3myY5NKr7PaL/UTaaS7ggk1F8Y9RKizZnMY7dC\n"
"RGPVjcy2epYl3LnWwXkVQTDvVmL+0MJc5t11LJCQPzNB7MnMMUFZ9TW/wMFEr32Q\n"
"M8AmFPzQXJ5MHLNcZ3MeQQNGYCrELDj0oR9MPPkcGbI0KvGrmMmpsQv1e+13FPDi\n"
"3ZJUv0pfSmqVV6MsOLOCJ+dOC/7Xn0GxQDtQRd8tSGxCqxPTmP7RsvgSI/18+dLn\n"
"1WcbS1BV1zl2lUk4\n"
"-----END CERTIFICATE-----\n";

const std::string TEST_KEY = 
"-----BEGIN PRIVATE KEY-----\n"
"MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQDBDN7JfHBUXI/I\n"
"6aBl+Ze5+MvbMBBN1EViBCtv1kkMwAjZR2M7lgh3NYfZbZW+B6H/HVa3RPcZYbXX\n"
"fQP4BmQvMPYpyPLtDDAY+lRr42C/kYwqM6YJ+rL9aYJvt+Q7RIbQ4ZGciC9z7xnq\n"
"QeE9oyqC6qtbH9eV9B8D2HGLhYvaDYIzT6lfUlDKEOIVWNIb0ezIKNGYGsL4v42o\n"
"4HYCbgSK1p5T4U77E2v+wQ9Qvn0Jt9vJ2UW9IFsAWrWnkjuQhO3SQV0TJpXZV+GX\n"
"RpFUQI2nDgQJ71xtM4Ou7KoRoXkLaQFclvAWuOPHnzYpD5ljuXJM+nS5LALW+l47\n"
"OfnRAgMBAAECggEALKjnp4tO6QnMZkRxcRQdoUi2IJ92FPHYvFtB2NT46fymOz07\n"
"4GjnZUwUlVDgH4/JVaZxXyOcEkzjiOVUYfgNQJvfyOqX4KKgkpA4hW25GZQQZf6l\n"
"bUpGtK2HQEGZuI0FnILBMZ4TfQbXfNrKIfS+LGzpLXeqJZSe7WihJF1BXvkfXKtX\n"
"CjpzKTr1UNJ+OXI7dn1qJzwgbDUlb9O5I+iSuG+xR7ZttDrKNmAr1WdzjDRlZiO1\n"
"6IqVK0pGmOQFw4+0AKvExb5M+TAeYQX7LVs+e91t2Ip2KW9bO/3ZfI2t2yK85W4i\n"
"VP9lOH0f7bkA/3aQWkG9L9Jwk3YKo4AdyprMlAAqAQKBgQDsXX2/xDckA+o5ijmL\n"
"UJUfO9D1WmTIGLTrr4HJyMxZGYj2WF0fWrIBJEtJYCVYgG16OqTkqEpC2Z7QtAH4\n"
"mCZYCDm2e6/QkCVW3YYNfN1+3kyQHpHB5Hy8yTiRkkE6PaBBzm9V1tDmk5PiX5oo\n"
"tVJvKWuB0EF5rBqXHp44Q2pxsQKBgQDRKDg4+jJLiPigEWcjphJa+EwuHVxfYQkQ\n"
"PPEzMtrNcJDvZ3HxyZONJYZ5Svf0SuI7zVQZ4ZZ0qlyZm0KaYohoQbvXxHJhnuEw\n"
"qZrT9IFwYlOjsvBPNfQn2I2eqKDt2e8XHlQ7IBP+dtuZMhJxLUnUgRpkO8GijMQ2\n"
"JJsjmwsaIQKBgC066h4L24hHnC76Ha1ITPtrZHR97bmL3kUQgZnjYuzlV9MzYt5x\n"
"n/wxsLIKgdmY1JBFRYYpFbA0l9RvRUVPhMYLBHGXWQPmh8r7jOY1KDvwZDNQRWmv\n"
"n1PUvrJJSh9NOO1jcryTMw9xKXUlCwj3b5miQ3Gc8RFvZITpvHNVIOhhAoGBAKsn\n"
"tKKjxDqdP5NSKlXinCoz5aZmKvpCh2vHzUlKGLLvLlBRzJZjJdaXdvvOHMADXA2M\n"
"Bi1Jh4DlKkADwcRbDN0TUjHu7ItxrUj5tk9JiOvIXXYZatQyPKMkKSS5JUnbKf6I\n"
"bZCJ6fLAIt0N9NcV5UvNBSbEMlr4tBEzH/BcZ1ehAoGAa8XfV8y+6FxZr/LrhnbZ\n"
"SdgL+UX0GPSbCdKEZv2XQ7eI3YCqXXjIlJQrcQoH0PvB9DVbAx1qiZ5SAw3i9+lJ\n"
"KuOGzKFXGmfzWrK8UrGj0S4UZPrAjM834iqd91YMmDiMVkJv70Uw2dCPL9UL+VVs\n"
"OVa8NjXAh4sBh7pIlQlVLRk=\n"
"-----END PRIVATE KEY-----\n";

// 用于回显的SSL会话类
class EchoSSLSession : public SSLSession {
public:
    explicit EchoSSLSession(const std::shared_ptr<SSLServer>& server)
        : SSLSession(server) {}
        
protected:
    void onReceived(const void* buffer, size_t size) override {
        // 回显接收的数据
        SendAsync(buffer, size);
    }
};

// 自定义SSL服务器，重写CreateSession方法
class EchoSSLServer : public SSLServer {
public:
    EchoSSLServer(const std::shared_ptr<Service>& service, 
                 const std::shared_ptr<SSLContext>& context,
                 const std::string& address, int port)
        : SSLServer(service, context, address, port) {}
    
protected:
    // 重写创建会话的方法，返回我们的自定义会话类
    std::shared_ptr<SSLSession> CreateSession(const std::shared_ptr<SSLServer>& server) override {
        return std::make_shared<EchoSSLSession>(server);
    }
};

// 用于测试的SSL服务器
class TestSSLServer {
public:
    explicit TestSSLServer(int port) : _port(port), _running(false) {}
    
    void Start() {
        if (_running)
            return;
            
        _running = true;
        _thread = std::thread([this]() {
            auto service = std::make_shared<Service>();
            service->Start();
            
            // 创建SSL上下文
            auto context = std::make_shared<SSLContext>(asio::ssl::context_base::tls);
            
            // 从字符串加载证书和私钥
            BIO* cert_bio = BIO_new_mem_buf(TEST_CERT.data(), -1);
            X509* cert = PEM_read_bio_X509(cert_bio, NULL, NULL, NULL);
            BIO_free(cert_bio);
            
            BIO* key_bio = BIO_new_mem_buf(TEST_KEY.data(), -1);
            EVP_PKEY* key = PEM_read_bio_PrivateKey(key_bio, NULL, NULL, NULL);
            BIO_free(key_bio);
            
            SSL_CTX_use_certificate(context->native_handle(), cert);
            SSL_CTX_use_PrivateKey(context->native_handle(), key);
            
            X509_free(cert);
            EVP_PKEY_free(key);
            
            // 使用自定义的SSL服务器，它会创建EchoSSLSession
            auto server = std::make_shared<EchoSSLServer>(service, context, "127.0.0.1", _port);
            
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
    
    ~TestSSLServer() {
        Stop();
    }
    
private:
    int _port;
    std::atomic<bool> _running;
    std::thread _thread;
    std::mutex _mutex;
    std::condition_variable _cv;
};

// 用于接收数据的客户端
class TestSSLClient : public SSLClient {
public:
    TestSSLClient(const std::shared_ptr<Service>& service, 
                  const std::shared_ptr<SSLContext>& context,
                  const std::string& address, int port)
        : SSLClient(service, context, address, port), 
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

TEST_CASE("SSL functionality", "[ssl]") {
    // 使用随机端口启动SSL服务器
    const int PORT = 19000 + (std::rand() % 1000);
    TestSSLServer server(PORT);
    server.Start();
    
    // 短暂延迟确保服务器启动
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    SECTION("SSL context creation") {
        auto context = std::make_shared<SSLContext>(asio::ssl::context_base::tls);
        
        // 使用OpenSSL的底层API加载证书和私钥
        BIO* cert_bio = BIO_new_mem_buf(TEST_CERT.data(), -1);
        X509* cert = PEM_read_bio_X509(cert_bio, NULL, NULL, NULL);
        BIO_free(cert_bio);
        
        BIO* key_bio = BIO_new_mem_buf(TEST_KEY.data(), -1);
        EVP_PKEY* key = PEM_read_bio_PrivateKey(key_bio, NULL, NULL, NULL);
        BIO_free(key_bio);
        
        REQUIRE(SSL_CTX_use_certificate(context->native_handle(), cert) == 1);
        REQUIRE(SSL_CTX_use_PrivateKey(context->native_handle(), key) == 1);
        
        X509_free(cert);
        EVP_PKEY_free(key);
    }
    
    SECTION("Connect to SSL server") {
        auto service = std::make_shared<Service>();
        service->Start();
        auto context = std::make_shared<SSLContext>(asio::ssl::context_base::tls);
        
        // 禁用证书验证（仅用于测试）
        context->set_verify_mode(asio::ssl::verify_none);
        
        auto client = std::make_shared<SSLClient>(service, context, "127.0.0.1", PORT);
        
        REQUIRE(client->Connect());
        REQUIRE(client->IsConnected());
        
        client->Disconnect();
        REQUIRE_FALSE(client->IsConnected());
        
        service->Stop();
    }
    
    SECTION("Send and receive data over SSL") {
        auto service = std::make_shared<Service>();
        service->Start();
        auto context = std::make_shared<SSLContext>(asio::ssl::context_base::tls);
        
        // 禁用证书验证（仅用于测试）
        context->set_verify_mode(asio::ssl::verify_none);
        
        auto client = std::make_shared<TestSSLClient>(service, context, "127.0.0.1", PORT);
        
        REQUIRE(client->Connect());
        
        // 发送数据
        const std::string testData = "Hello, SSL Server!";
        size_t sent = client->Send(testData);
        REQUIRE(sent == testData.size());
        
        // 启动异步接收
        client->ReceiveAsync();
        
        // 等待接收回显数据
        {
            std::unique_lock<std::mutex> lock(client->mutex);
            bool result = client->cv.wait_for(lock, std::chrono::seconds(2), [&client]() { return client->dataReceived; });
            REQUIRE(result);
            REQUIRE(client->receivedData == testData);
        }
        
        client->Disconnect();
        service->Stop();
    }
    
    SECTION("Test SSL asynchronous operations") {
        auto service = std::make_shared<Service>();
        service->Start();
        auto context = std::make_shared<SSLContext>(asio::ssl::context_base::tls);
        
        // 禁用证书验证（仅用于测试）
        context->set_verify_mode(asio::ssl::verify_none);
        
        auto client = std::make_shared<TestSSLClient>(service, context, "127.0.0.1", PORT);
        
        REQUIRE(client->ConnectAsync());
        
        // 等待连接成功
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        REQUIRE(client->IsConnected());
        
        // 异步发送数据
        const std::string testData = "Async SSL Test";
        REQUIRE(client->SendAsync(testData));
        
        // 启动异步接收
        client->ReceiveAsync();
        
        // 等待接收回显数据
        {
            std::unique_lock<std::mutex> lock(client->mutex);
            bool result = client->cv.wait_for(lock, std::chrono::seconds(2), [&client]() { return client->dataReceived; });
            REQUIRE(result);
            REQUIRE(client->receivedData == testData);
        }
        
        client->DisconnectAsync();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        REQUIRE_FALSE(client->IsConnected());
        
        service->Stop();
    }
    
    // 停止服务器
    server.Stop();
} 