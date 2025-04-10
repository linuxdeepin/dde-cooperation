// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file reliable_udp_example.cpp
 * @brief 演示使用 UDP 实现简单可靠传输的示例
 * 
 * 该示例展示了如何在不可靠的 UDP 协议上实现基本的可靠传输，
 * 包括序列号、确认机制、超时重传等功能。
 */

#include "../src/asio.h"

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <mutex>
#include <chrono>
#include <map>
#include <atomic>
#include <functional>
#include <iomanip>
#include <cstring>
#include <cstdint>

// 线程安全的打印函数
std::mutex print_mutex;
template<typename... Args>
void safe_print(Args&&... args) {
    std::lock_guard<std::mutex> lock(print_mutex);
    (std::cout << ... << std::forward<Args>(args)) << std::endl;
}

// 生成随机数用于模拟丢包
int random_number(int min, int max) {
    static std::mt19937 mt(std::random_device{}());
    std::uniform_int_distribution<int> dist(min, max);
    return dist(mt);
}

// 用于获取当前时间戳的辅助函数
uint64_t current_time_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

// 输出当前时间
std::string timestamp() {
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count() % 1000;
    
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&now_time_t), "%H:%M:%S") 
        << '.' << std::setfill('0') << std::setw(3) << now_ms;
    return oss.str();
}

// 消息头部结构
#pragma pack(push, 1)
struct MessageHeader {
    uint32_t seq_number;      // 序列号
    uint32_t ack_number;      // 确认号
    uint16_t flags;           // 标志位
    uint16_t payload_size;    // 负载大小
    
    // 标志位常量
    enum Flags {
        ACK = 1 << 0,         // 确认标志
        SYN = 1 << 1,         // 同步标志
        FIN = 1 << 2,         // 结束标志
        RETRANSMIT = 1 << 3,  // 重传标志
    };
};
#pragma pack(pop)

// 最大消息大小
constexpr size_t MAX_PAYLOAD_SIZE = 1024;
constexpr size_t MAX_MESSAGE_SIZE = sizeof(MessageHeader) + MAX_PAYLOAD_SIZE;

// 超时和重传参数
constexpr int ACK_TIMEOUT_MS = 500;     // 确认超时时间
constexpr int MAX_RETRIES = 5;          // 最大重试次数
constexpr int PACKET_LOSS_PERCENT = 20; // 模拟丢包率（百分比）

// 可靠 UDP 客户端
class ReliableUdpClient {
public:
    ReliableUdpClient(asio::io_context& io_context)
        : io_context_(io_context),
          socket_(io_context),
          timer_(io_context),
          next_seq_number_(1),
          expected_ack_(0),
          server_endpoint_(),
          stopping_(false) {
        
        asio::error_code ec;
        socket_.open(asio::address_family::ipv4, ec);
        if (ec) {
            safe_print("[客户端] 打开套接字失败: ", ec.message());
            return;
        }
        
        // 绑定到任意端口
        asio::endpoint endpoint(asio::ip_address::any(), 0);
        socket_.bind(endpoint, ec);
        if (ec) {
            safe_print("[客户端] 绑定套接字失败: ", ec.message());
            return;
        }
        
        safe_print("[客户端] 已初始化，本地端口: ", 
                 socket_.local_endpoint(ec).port());
    }
    
    ~ReliableUdpClient() {
        stop();
    }
    
    // 连接到服务器
    bool connect(const std::string& host, unsigned short port) {
        asio::error_code ec;
        server_endpoint_ = asio::endpoint(asio::ip_address::from_string(host, ec), port);
        if (ec) {
            safe_print("[客户端] 无效的主机地址: ", ec.message());
            return false;
        }
        
        // 发送 SYN 消息
        MessageHeader header{};
        header.seq_number = next_seq_number_++;
        header.flags = MessageHeader::SYN;
        header.payload_size = 0;
        
        expected_ack_ = header.seq_number;
        
        safe_print("[客户端] 连接到 ", host, ":", port, " [SYN, SEQ=", header.seq_number, "]");
        
        // 发送 SYN 并等待 SYN+ACK
        send_message(header, nullptr, 0);
        
        // 开始接收响应
        start_receive();
        
        return true;
    }
    
    // 发送数据
    bool send_data(const std::string& data) {
        if (data.empty() || data.size() > MAX_PAYLOAD_SIZE) {
            safe_print("[客户端] 无效的数据大小: ", data.size());
            return false;
        }
        
        MessageHeader header{};
        header.seq_number = next_seq_number_++;
        header.flags = 0;  // 普通数据包
        header.payload_size = static_cast<uint16_t>(data.size());
        
        expected_ack_ = header.seq_number;
        
        safe_print("[客户端] 发送数据 [SEQ=", header.seq_number, ", 大小=", data.size(), "]");
        
        send_message(header, data.data(), data.size());
        
        return true;
    }
    
    // 断开连接
    void disconnect() {
        if (stopping_) return;
        
        MessageHeader header{};
        header.seq_number = next_seq_number_++;
        header.flags = MessageHeader::FIN;
        header.payload_size = 0;
        
        expected_ack_ = header.seq_number;
        
        safe_print("[客户端] 断开连接 [FIN, SEQ=", header.seq_number, "]");
        
        send_message(header, nullptr, 0);
        
        // 等待一段时间后关闭
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        stop();
    }
    
    // 停止客户端
    void stop() {
        stopping_ = true;
        asio::error_code ec;
        timer_.cancel(ec);
        socket_.close(ec);
    }
    
private:
    // 发送消息
    void send_message(const MessageHeader& header, const void* payload, size_t size) {
        asio::error_code ec;
        
        // 检查是否添加延迟的 ACK（针对正常包）
        if (!(header.flags & MessageHeader::ACK) && 
            random_number(1, 100) <= PACKET_LOSS_PERCENT) {
            safe_print("[客户端] 模拟丢包，不发送消息 [SEQ=", header.seq_number, "]");
            
            // 依然设置定时器，以便重传
            if (expected_ack_ == header.seq_number) {
                pending_messages_[header.seq_number] = create_message(header, payload, size);
                start_retransmission_timer(header.seq_number);
            }
            return;
        }
        
        // 创建消息
        std::vector<char> message = create_message(header, payload, size);
        
        // 发送消息
        socket_.send_to(message.data(), message.size(), server_endpoint_, ec);
        if (ec) {
            safe_print("[客户端] 发送消息失败: ", ec.message());
            return;
        }
        
        // 如果是需要确认的消息，保存并设置重传定时器
        if (expected_ack_ == header.seq_number) {
            pending_messages_[header.seq_number] = std::move(message);
            start_retransmission_timer(header.seq_number);
        }
    }
    
    // 创建完整消息（消息头 + 负载）
    std::vector<char> create_message(const MessageHeader& header, const void* payload, size_t size) {
        std::vector<char> message(sizeof(MessageHeader) + size);
        std::memcpy(message.data(), &header, sizeof(MessageHeader));
        if (payload && size > 0) {
            std::memcpy(message.data() + sizeof(MessageHeader), payload, size);
        }
        return message;
    }
    
    // 开始接收消息
    void start_receive() {
        if (stopping_) return;
        
        socket_.async_receive_from(
            buffer_, sizeof(buffer_), receiver_endpoint_,
            [this](const asio::error_code& ec, std::size_t bytes_transferred) {
                handle_receive(ec, bytes_transferred);
            });
    }
    
    // 处理接收到的消息
    void handle_receive(const asio::error_code& ec, std::size_t bytes_transferred) {
        if (stopping_) return;
        
        if (!ec && bytes_transferred >= sizeof(MessageHeader)) {
            MessageHeader* header = reinterpret_cast<MessageHeader*>(buffer_);
            
            safe_print("[客户端] 收到消息 [",
                     (header->flags & MessageHeader::ACK ? "ACK" : ""),
                     (header->flags & MessageHeader::SYN ? " SYN" : ""),
                     (header->flags & MessageHeader::FIN ? " FIN" : ""),
                     (header->flags & MessageHeader::RETRANSMIT ? " RETRANSMIT" : ""),
                     ", SEQ=", header->seq_number,
                     ", ACK=", header->ack_number, "]");
            
            // 处理确认
            if (header->flags & MessageHeader::ACK) {
                handle_ack(header->ack_number);
            }
            
            // 处理 SYN+ACK
            if ((header->flags & MessageHeader::SYN) && (header->flags & MessageHeader::ACK)) {
                safe_print("[客户端] 连接已建立");
                
                // 回复 ACK
                MessageHeader ack_header{};
                ack_header.seq_number = next_seq_number_++;
                ack_header.ack_number = header->seq_number;
                ack_header.flags = MessageHeader::ACK;
                ack_header.payload_size = 0;
                
                send_message(ack_header, nullptr, 0);
            }
            
            // 处理来自服务器的数据
            if (!(header->flags & MessageHeader::ACK) && 
                !(header->flags & MessageHeader::SYN) && 
                !(header->flags & MessageHeader::FIN)) {
                
                char* payload = buffer_ + sizeof(MessageHeader);
                std::string data(payload, header->payload_size);
                
                safe_print("[客户端] 收到数据: ", data);
                
                // 发送确认
                MessageHeader ack_header{};
                ack_header.seq_number = next_seq_number_++;
                ack_header.ack_number = header->seq_number;
                ack_header.flags = MessageHeader::ACK;
                ack_header.payload_size = 0;
                
                send_message(ack_header, nullptr, 0);
            }
            
            // 处理 FIN
            if (header->flags & MessageHeader::FIN) {
                safe_print("[客户端] 收到断开连接请求");
                
                // 回复 ACK
                MessageHeader ack_header{};
                ack_header.seq_number = next_seq_number_++;
                ack_header.ack_number = header->seq_number;
                ack_header.flags = MessageHeader::ACK;
                ack_header.payload_size = 0;
                
                send_message(ack_header, nullptr, 0);
                
                // 客户端主动断开连接
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                stop();
            }
        }
        else if (ec) {
            safe_print("[客户端] 接收错误: ", ec.message());
        }
        
        // 继续接收
        start_receive();
    }
    
    // 处理确认
    void handle_ack(uint32_t ack_number) {
        // 移除已确认的消息
        auto it = pending_messages_.find(ack_number);
        if (it != pending_messages_.end()) {
            safe_print("[客户端] 消息已确认 [ACK=", ack_number, "]");
            pending_messages_.erase(it);
        }
    }
    
    // 启动重传定时器
    void start_retransmission_timer(uint32_t seq_number) {
        if (stopping_) return;
        
        auto timeout = std::chrono::milliseconds(ACK_TIMEOUT_MS);
        
        timer_.expires_after(timeout);
        timer_.async_wait([this, seq_number](const asio::error_code& ec) {
            if (!ec) handle_timeout(seq_number);
        });
    }
    
    // 处理超时
    void handle_timeout(uint32_t seq_number) {
        if (stopping_) return;
        
        auto it = pending_messages_.find(seq_number);
        if (it != pending_messages_.end()) {
            const auto& message = it->second;
            MessageHeader* header = reinterpret_cast<MessageHeader*>(const_cast<char*>(message.data()));
            
            // 增加重传标志
            header->flags |= MessageHeader::RETRANSMIT;
            
            static std::map<uint32_t, int> retry_count;
            retry_count[seq_number] += 1;
            
            if (retry_count[seq_number] <= MAX_RETRIES) {
                safe_print("[客户端] 重传消息 [SEQ=", seq_number, 
                         ", 尝试=", retry_count[seq_number], "/", MAX_RETRIES, "]");
                
                asio::error_code send_ec;
                socket_.send_to(message.data(), message.size(), server_endpoint_, send_ec);
                if (send_ec) {
                    safe_print("[客户端] 重传失败: ", send_ec.message());
                }
                
                // 设置下一次重传定时器
                start_retransmission_timer(seq_number);
            }
            else {
                safe_print("[客户端] 达到最大重试次数，放弃消息 [SEQ=", seq_number, "]");
                pending_messages_.erase(it);
                retry_count.erase(seq_number);
            }
        }
    }
    
private:
    asio::io_context& io_context_;
    asio::datagram_socket socket_;
    asio::steady_timer timer_;
    
    uint32_t next_seq_number_;
    uint32_t expected_ack_;
    
    asio::endpoint server_endpoint_;
    asio::endpoint receiver_endpoint_;
    
    char buffer_[MAX_MESSAGE_SIZE];
    std::map<uint32_t, std::vector<char>> pending_messages_;
    
    std::atomic<bool> stopping_;
};

// 可靠 UDP 服务器
class ReliableUdpServer {
public:
    ReliableUdpServer(asio::io_context& io_context, unsigned short port)
        : io_context_(io_context),
          socket_(io_context),
          timer_(io_context),
          next_seq_number_(1),
          stopping_(false) {
        
        asio::error_code ec;
        socket_.open(asio::address_family::ipv4, ec);
        if (ec) {
            safe_print("[服务器] 打开套接字失败: ", ec.message());
            return;
        }
        
        asio::endpoint endpoint(asio::ip_address::any(), port);
        socket_.bind(endpoint, ec);
        if (ec) {
            safe_print("[服务器] 绑定套接字失败: ", ec.message());
            return;
        }
        
        safe_print("[服务器] 已初始化，监听端口: ", port);
    }
    
    ~ReliableUdpServer() {
        stop();
    }
    
    // 启动服务器
    void start() {
        safe_print("[服务器] 开始接收连接");
        start_receive();
    }
    
    // 停止服务器
    void stop() {
        if (stopping_) return;
        stopping_ = true;
        
        asio::error_code ec;
        timer_.cancel(ec);
        socket_.close(ec);
        
        safe_print("[服务器] 已停止");
    }
    
private:
    // 开始接收消息
    void start_receive() {
        if (stopping_) return;
        
        socket_.async_receive_from(
            buffer_, sizeof(buffer_), sender_endpoint_,
            [this](const asio::error_code& ec, std::size_t bytes_transferred) {
                handle_receive(ec, bytes_transferred);
            });
    }
    
    // 处理接收到的消息
    void handle_receive(const asio::error_code& ec, std::size_t bytes_transferred) {
        if (stopping_) return;
        
        if (!ec && bytes_transferred >= sizeof(MessageHeader)) {
            MessageHeader* header = reinterpret_cast<MessageHeader*>(buffer_);
            
            safe_print("[服务器] 收到消息来自 ", 
                     sender_endpoint_.address().to_string(), ":", sender_endpoint_.port(),
                     " [",
                     (header->flags & MessageHeader::ACK ? "ACK" : ""),
                     (header->flags & MessageHeader::SYN ? " SYN" : ""),
                     (header->flags & MessageHeader::FIN ? " FIN" : ""),
                     (header->flags & MessageHeader::RETRANSMIT ? " RETRANSMIT" : ""),
                     ", SEQ=", header->seq_number,
                     ", ACK=", header->ack_number, "]");
            
            // 检查是否添加延迟的 ACK（针对正常包，但不包括重传的包）
            bool should_respond = true;
            if (!(header->flags & MessageHeader::ACK) && 
                !(header->flags & MessageHeader::RETRANSMIT) &&
                random_number(1, 100) <= PACKET_LOSS_PERCENT) {
                safe_print("[服务器] 模拟丢包，不处理消息 [SEQ=", header->seq_number, "]");
                should_respond = false;
            }
            
            if (should_respond) {
                // 处理确认
                if (header->flags & MessageHeader::ACK) {
                    handle_ack(header->ack_number);
                }
                
                // 处理 SYN（新连接）
                if (header->flags & MessageHeader::SYN) {
                    // 保存客户端端点
                    clients_[sender_endpoint_] = ClientInfo{};
                    
                    // 发送 SYN+ACK
                    MessageHeader syn_ack{};
                    syn_ack.seq_number = next_seq_number_++;
                    syn_ack.ack_number = header->seq_number;
                    syn_ack.flags = MessageHeader::SYN | MessageHeader::ACK;
                    syn_ack.payload_size = 0;
                    
                    safe_print("[服务器] 发送 SYN+ACK 响应 [SEQ=", syn_ack.seq_number, 
                             ", ACK=", syn_ack.ack_number, "]");
                    
                    send_message(syn_ack, nullptr, 0, sender_endpoint_);
                }
                
                // 处理数据
                if (!(header->flags & MessageHeader::ACK) && 
                    !(header->flags & MessageHeader::SYN) && 
                    !(header->flags & MessageHeader::FIN)) {
                    
                    char* payload = buffer_ + sizeof(MessageHeader);
                    std::string data(payload, header->payload_size);
                    
                    safe_print("[服务器] 收到数据: ", data);
                    
                    // 记录客户端的最后一个序列号
                    auto& client = clients_[sender_endpoint_];
                    client.last_seq_received = header->seq_number;
                    
                    // 发送确认
                    MessageHeader ack_header{};
                    ack_header.seq_number = next_seq_number_++;
                    ack_header.ack_number = header->seq_number;
                    ack_header.flags = MessageHeader::ACK;
                    ack_header.payload_size = 0;
                    
                    send_message(ack_header, nullptr, 0, sender_endpoint_);
                    
                    // 可选：响应一些数据
                    std::string response = "收到你的消息: " + data;
                    MessageHeader resp_header{};
                    resp_header.seq_number = next_seq_number_++;
                    resp_header.ack_number = 0;
                    resp_header.flags = 0;  // 普通数据包
                    resp_header.payload_size = static_cast<uint16_t>(response.size());
                    
                    send_message(resp_header, response.data(), response.size(), sender_endpoint_);
                }
                
                // 处理 FIN（断开连接）
                if (header->flags & MessageHeader::FIN) {
                    safe_print("[服务器] 客户端请求断开连接");
                    
                    // 发送 ACK
                    MessageHeader ack_header{};
                    ack_header.seq_number = next_seq_number_++;
                    ack_header.ack_number = header->seq_number;
                    ack_header.flags = MessageHeader::ACK;
                    ack_header.payload_size = 0;
                    
                    send_message(ack_header, nullptr, 0, sender_endpoint_);
                    
                    // 移除客户端
                    clients_.erase(sender_endpoint_);
                }
            }
        }
        else if (ec) {
            safe_print("[服务器] 接收错误: ", ec.message());
        }
        
        // 继续接收
        start_receive();
    }
    
    // 处理确认
    void handle_ack(uint32_t ack_number) {
        // 移除已确认的消息
        auto it = pending_messages_.find(ack_number);
        if (it != pending_messages_.end()) {
            safe_print("[服务器] 消息已确认 [ACK=", ack_number, "]");
            pending_messages_.erase(it);
        }
    }
    
    // 发送消息
    void send_message(const MessageHeader& header, const void* payload, 
                     size_t size, const asio::endpoint& endpoint) {
        // 创建消息
        std::vector<char> message = create_message(header, payload, size);
        
        asio::error_code ec;
        socket_.send_to(message.data(), message.size(), endpoint, ec);
        if (ec) {
            safe_print("[服务器] 发送消息失败: ", ec.message());
            return;
        }
        
        // 如果是需要确认的消息（不是 ACK），保存并设置重传定时器
        if (!(header.flags & MessageHeader::ACK)) {
            PendingMessage pm;
            pm.message = std::move(message);
            pm.endpoint = endpoint;
            pm.retry_count = 0;
            pending_messages_[header.seq_number] = std::move(pm);
            
            start_retransmission_timer(header.seq_number);
        }
    }
    
    // 创建完整消息（消息头 + 负载）
    std::vector<char> create_message(const MessageHeader& header, const void* payload, size_t size) {
        std::vector<char> message(sizeof(MessageHeader) + size);
        std::memcpy(message.data(), &header, sizeof(MessageHeader));
        if (payload && size > 0) {
            std::memcpy(message.data() + sizeof(MessageHeader), payload, size);
        }
        return message;
    }
    
    // 启动重传定时器
    void start_retransmission_timer(uint32_t seq_number) {
        if (stopping_) return;
        
        auto timeout = std::chrono::milliseconds(ACK_TIMEOUT_MS);
        
        timer_.expires_after(timeout);
        timer_.async_wait([this, seq_number](const asio::error_code& ec) {
            if (!ec) handle_timeout(seq_number);
        });
    }
    
    // 处理超时
    void handle_timeout(uint32_t seq_number) {
        if (stopping_) return;
        
        auto it = pending_messages_.find(seq_number);
        if (it != pending_messages_.end()) {
            PendingMessage& pm = it->second;
            MessageHeader* header = reinterpret_cast<MessageHeader*>(pm.message.data());
            
            // 增加重传标志
            header->flags |= MessageHeader::RETRANSMIT;
            
            pm.retry_count += 1;
            if (pm.retry_count <= MAX_RETRIES) {
                safe_print("[服务器] 重传消息 [SEQ=", seq_number, 
                         ", 尝试=", pm.retry_count, "/", MAX_RETRIES, "]");
                
                asio::error_code send_ec;
                socket_.send_to(pm.message.data(), pm.message.size(), pm.endpoint, send_ec);
                if (send_ec) {
                    safe_print("[服务器] 重传失败: ", send_ec.message());
                }
                
                // 设置下一次重传定时器
                start_retransmission_timer(seq_number);
            }
            else {
                safe_print("[服务器] 达到最大重试次数，放弃消息 [SEQ=", seq_number, "]");
                pending_messages_.erase(it);
            }
        }
    }
    
private:
    struct ClientInfo {
        uint32_t last_seq_received = 0;
        // 可以添加更多客户端相关信息
    };
    
    struct PendingMessage {
        std::vector<char> message;
        asio::endpoint endpoint;
        int retry_count;
    };
    
    asio::io_context& io_context_;
    asio::datagram_socket socket_;
    asio::steady_timer timer_;
    
    uint32_t next_seq_number_;
    
    char buffer_[MAX_MESSAGE_SIZE];
    asio::endpoint sender_endpoint_;
    
    std::map<asio::endpoint, ClientInfo> clients_;
    std::map<uint32_t, PendingMessage> pending_messages_;
    
    std::atomic<bool> stopping_;
};

int main() {
    try {
        asio::io_context io_context;
        
        // 创建服务器
        unsigned short server_port = 12345;
        ReliableUdpServer server(io_context, server_port);
        server.start();
        
        // 创建客户端
        ReliableUdpClient client(io_context);
        
        // 运行 IO 服务线程
        std::thread io_thread([&io_context]() {
            asio::error_code ec;
            io_context.run(ec);
            if (ec) {
                safe_print("IO 上下文运行错误: ", ec.message());
            }
        });
        
        // 连接到服务器
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (!client.connect("127.0.0.1", server_port)) {
            safe_print("连接服务器失败");
            server.stop();
            io_context.stop();
            io_thread.join();
            return 1;
        }
        
        // 稍微等待连接建立
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        // 发送多条消息进行测试
        std::vector<std::string> test_messages = {
            "Hello, 这是测试消息 #1",
            "你好，这是测试消息 #2，会有一些丢包和重传",
            "这是最后一条测试消息 #3，很长的消息测试传输可靠性和分片机制",
            "再见！"
        };
        
        for (const auto& msg : test_messages) {
            client.send_data(msg);
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
        
        // 断开连接
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        client.disconnect();
        
        // 等待一段时间以处理断开连接
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        
        // 停止服务器
        server.stop();
        io_context.stop();
        io_thread.join();
        
        safe_print("示例完成");
        return 0;
    }
    catch (const std::exception& e) {
        safe_print("异常: ", e.what());
        return 1;
    }
} 