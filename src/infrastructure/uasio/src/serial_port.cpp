// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "serial_port.h"
#include <mutex>
#include <thread>
#include <condition_variable>
#include <queue>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <cstring>

namespace asio {

// 将枚举类型转换为系统特定的值
namespace detail {

// 转换波特率
speed_t to_speed_t(unsigned int rate) {
    switch (rate) {
        case 0: return B0;
        case 50: return B50;
        case 75: return B75;
        case 110: return B110;
        case 134: return B134;
        case 150: return B150;
        case 200: return B200;
        case 300: return B300;
        case 600: return B600;
        case 1200: return B1200;
        case 1800: return B1800;
        case 2400: return B2400;
        case 4800: return B4800;
        case 9600: return B9600;
        case 19200: return B19200;
        case 38400: return B38400;
        case 57600: return B57600;
        case 115200: return B115200;
        case 230400: return B230400;
#ifdef B460800
        case 460800: return B460800;
#endif
#ifdef B500000
        case 500000: return B500000;
#endif
#ifdef B576000
        case 576000: return B576000;
#endif
#ifdef B921600
        case 921600: return B921600;
#endif
#ifdef B1000000
        case 1000000: return B1000000;
#endif
#ifdef B1152000
        case 1152000: return B1152000;
#endif
#ifdef B1500000
        case 1500000: return B1500000;
#endif
#ifdef B2000000
        case 2000000: return B2000000;
#endif
#ifdef B2500000
        case 2500000: return B2500000;
#endif
#ifdef B3000000
        case 3000000: return B3000000;
#endif
#ifdef B3500000
        case 3500000: return B3500000;
#endif
#ifdef B4000000
        case 4000000: return B4000000;
#endif
        default: return B9600;
    }
}

// 从speed_t转换为整数波特率
unsigned int from_speed_t(speed_t speed) {
    switch (speed) {
        case B0: return 0;
        case B50: return 50;
        case B75: return 75;
        case B110: return 110;
        case B134: return 134;
        case B150: return 150;
        case B200: return 200;
        case B300: return 300;
        case B600: return 600;
        case B1200: return 1200;
        case B1800: return 1800;
        case B2400: return 2400;
        case B4800: return 4800;
        case B9600: return 9600;
        case B19200: return 19200;
        case B38400: return 38400;
        case B57600: return 57600;
        case B115200: return 115200;
        case B230400: return 230400;
#ifdef B460800
        case B460800: return 460800;
#endif
#ifdef B500000
        case B500000: return 500000;
#endif
#ifdef B576000
        case B576000: return 576000;
#endif
#ifdef B921600
        case B921600: return 921600;
#endif
#ifdef B1000000
        case B1000000: return 1000000;
#endif
#ifdef B1152000
        case B1152000: return 1152000;
#endif
#ifdef B1500000
        case B1500000: return 1500000;
#endif
#ifdef B2000000
        case B2000000: return 2000000;
#endif
#ifdef B2500000
        case B2500000: return 2500000;
#endif
#ifdef B3000000
        case B3000000: return 3000000;
#endif
#ifdef B3500000
        case B3500000: return 3500000;
#endif
#ifdef B4000000
        case B4000000: return 4000000;
#endif
        default: return 9600;
    }
}

// 转换数据位
tcflag_t to_character_size(character_size size) {
    switch (size) {
        case character_size::size_5: return CS5;
        case character_size::size_6: return CS6;
        case character_size::size_7: return CS7;
        case character_size::size_8: return CS8;
        default: return CS8;
    }
}

// 从tcflag_t转换为数据位
character_size from_character_size(tcflag_t flags) {
    flags &= CSIZE;
    if (flags == CS5) return character_size::size_5;
    if (flags == CS6) return character_size::size_6;
    if (flags == CS7) return character_size::size_7;
    return character_size::size_8;
}

// 应用停止位配置
void apply_stop_bits(termios& options, stop_bits bits) {
    switch (bits) {
        case stop_bits::one:
            options.c_cflag &= ~CSTOPB;
            break;
        case stop_bits::two:
        case stop_bits::onepointfive:  // 在POSIX系统中1.5停止位通常被当作2位处理
            options.c_cflag |= CSTOPB;
            break;
        default:
            options.c_cflag &= ~CSTOPB;  // 默认一个停止位
            break;
    }
}

// 获取停止位配置
stop_bits get_stop_bits(const termios& options) {
    if (options.c_cflag & CSTOPB) {
        return stop_bits::two;
    }
    return stop_bits::one;
}

// 应用奇偶校验配置
void apply_parity(termios& options, parity p) {
    switch (p) {
        case parity::none:
            options.c_cflag &= ~PARENB;  // 禁用奇偶校验
            options.c_iflag &= ~(INPCK | ISTRIP);
            break;
        case parity::odd:
            options.c_cflag |= (PARENB | PARODD);  // 启用奇偶校验，设置为奇校验
            options.c_iflag |= (INPCK | ISTRIP);   // 启用输入奇偶校验
            break;
        case parity::even:
            options.c_cflag |= PARENB;   // 启用奇偶校验
            options.c_cflag &= ~PARODD;  // 设置为偶校验
            options.c_iflag |= (INPCK | ISTRIP);   // 启用输入奇偶校验
            break;
        default:
            options.c_cflag &= ~PARENB;  // 默认禁用奇偶校验
            options.c_iflag &= ~(INPCK | ISTRIP);
            break;
    }
}

// 获取奇偶校验配置
parity get_parity(const termios& options) {
    if (!(options.c_cflag & PARENB)) {
        return parity::none;
    } else if (options.c_cflag & PARODD) {
        return parity::odd;
    } else {
        return parity::even;
    }
}

// 应用流控制配置
void apply_flow_control(termios& options, flow_control fc) {
    switch (fc) {
        case flow_control::none:
            options.c_cflag &= ~CRTSCTS;  // 禁用硬件流控制
            options.c_iflag &= ~(IXON | IXOFF | IXANY);  // 禁用软件流控制
            break;
        case flow_control::hardware:
            options.c_cflag |= CRTSCTS;   // 启用硬件流控制
            options.c_iflag &= ~(IXON | IXOFF | IXANY);  // 禁用软件流控制
            break;
        case flow_control::software:
            options.c_cflag &= ~CRTSCTS;  // 禁用硬件流控制
            options.c_iflag |= (IXON | IXOFF);  // 启用软件流控制
            break;
        default:
            options.c_cflag &= ~CRTSCTS;  // 默认禁用流控制
            options.c_iflag &= ~(IXON | IXOFF | IXANY);
            break;
    }
}

// 获取流控制配置
flow_control get_flow_control(const termios& options) {
    if (options.c_cflag & CRTSCTS) {
        return flow_control::hardware;
    } else if (options.c_iflag & (IXON | IXOFF)) {
        return flow_control::software;
    } else {
        return flow_control::none;
    }
}

} // namespace detail

// 串行端口服务类实现
class serial_port::serial_port_service {
public:
    // 构造函数
    explicit serial_port_service(io_context& io_context)
        : io_context_(io_context),
          fd_(-1),
          running_(false) {
    }

    // 析构函数
    ~serial_port_service() {
        close();
    }

    // 获取IO上下文
    io_context& get_io_context() {
        return io_context_;
    }

    // 打开串行设备
    bool open(const std::string& device, error_code& ec) {
        // 关闭已存在的连接
        close();

        // 打开串行设备
        fd_ = ::open(device.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
        if (fd_ < 0) {
            ec = error_code(errno, std::system_category());
            return false;
        }

        // 获取当前配置
        struct termios options;
        if (tcgetattr(fd_, &options) < 0) {
            ec = error_code(errno, std::system_category());
            ::close(fd_);
            fd_ = -1;
            return false;
        }

        // 设置为原始模式
        cfmakeraw(&options);
        options.c_cflag |= (CLOCAL | CREAD);  // 忽略控制线，启用接收器

        // 应用配置
        if (tcsetattr(fd_, TCSANOW, &options) < 0) {
            ec = error_code(errno, std::system_category());
            ::close(fd_);
            fd_ = -1;
            return false;
        }

        // 设置默认配置
        error_code temp_ec;
        set_baud_rate(9600, temp_ec);
        set_character_size(character_size::size_8, temp_ec);
        set_stop_bits(stop_bits::one, temp_ec);
        set_parity(parity::none, temp_ec);
        set_flow_control(flow_control::none, temp_ec);

        // 启动工作线程
        start_worker_thread();

        ec = error_code();
        return true;
    }

    // 检查串行端口是否打开
    bool is_open() const {
        return (fd_ >= 0);
    }

    // 关闭串行端口
    void close(error_code& ec) {
        if (fd_ < 0) {
            ec = error_code();
            return;
        }

        // 停止工作线程
        stop_worker_thread();

        // 关闭文件描述符
        if (::close(fd_) < 0) {
            ec = error_code(errno, std::system_category());
        } else {
            ec = error_code();
        }

        fd_ = -1;
    }

    // 无错误码版本的关闭
    void close() {
        error_code ec;
        close(ec);
    }

    // 发送数据
    std::size_t send(const void* data, std::size_t size, error_code& ec) {
        if (fd_ < 0) {
            ec = make_error_code(error::not_connected);
            return 0;
        }

        ssize_t bytes_written = ::write(fd_, data, size);
        if (bytes_written < 0) {
            ec = error_code(errno, std::system_category());
            return 0;
        }

        ec = error_code();
        return static_cast<std::size_t>(bytes_written);
    }

    // 异步发送数据
    template <typename WriteHandler>
    void async_send(const void* data, std::size_t size, WriteHandler&& handler) {
        if (fd_ < 0) {
            io_context_.post([handler = std::forward<WriteHandler>(handler)]() {
                handler(make_error_code(error::not_connected), 0);
            });
            return;
        }

        // 创建写操作并添加到队列
        auto op = std::make_shared<write_operation>();
        op->data.resize(size);
        std::memcpy(op->data.data(), data, size);
        op->handler = [handler = std::forward<WriteHandler>(handler)](const error_code& ec, std::size_t bytes_transferred) {
            handler(ec, bytes_transferred);
        };

        {
            std::lock_guard<std::mutex> lock(write_mutex_);
            write_queue_.push(op);
        }
        
        // 通知工作线程有新的写操作
        write_cv_.notify_one();
    }

    // 接收数据
    std::size_t receive(void* data, std::size_t max_size, error_code& ec) {
        if (fd_ < 0) {
            ec = make_error_code(error::not_connected);
            return 0;
        }

        ssize_t bytes_read = ::read(fd_, data, max_size);
        if (bytes_read < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // 非阻塞操作，没有数据可读
                ec = make_error_code(error::would_block);
                return 0;
            }
            ec = error_code(errno, std::system_category());
            return 0;
        }

        ec = error_code();
        return static_cast<std::size_t>(bytes_read);
    }

    // 异步接收数据
    template <typename ReadHandler>
    void async_receive(void* data, std::size_t max_size, ReadHandler&& handler) {
        if (fd_ < 0) {
            io_context_.post([handler = std::forward<ReadHandler>(handler)]() {
                handler(make_error_code(error::not_connected), 0);
            });
            return;
        }

        // 创建读操作并添加到队列
        auto op = std::make_shared<read_operation>();
        op->buffer = data;
        op->max_size = max_size;
        op->handler = [handler = std::forward<ReadHandler>(handler)](const error_code& ec, std::size_t bytes_transferred) {
            handler(ec, bytes_transferred);
        };

        {
            std::lock_guard<std::mutex> lock(read_mutex_);
            read_queue_.push(op);
        }
        
        // 通知工作线程有新的读操作
        read_cv_.notify_one();
    }

    // 设置波特率
    bool set_baud_rate(unsigned int rate, error_code& ec) {
        if (fd_ < 0) {
            ec = make_error_code(error::not_connected);
            return false;
        }

        // 获取当前配置
        struct termios options;
        if (tcgetattr(fd_, &options) < 0) {
            ec = error_code(errno, std::system_category());
            return false;
        }

        // 设置波特率
        speed_t speed = detail::to_speed_t(rate);
        if (cfsetispeed(&options, speed) < 0 || cfsetospeed(&options, speed) < 0) {
            ec = error_code(errno, std::system_category());
            return false;
        }

        // 应用配置
        if (tcsetattr(fd_, TCSANOW, &options) < 0) {
            ec = error_code(errno, std::system_category());
            return false;
        }

        ec = error_code();
        return true;
    }

    // 获取波特率
    unsigned int baud_rate(error_code& ec) const {
        if (fd_ < 0) {
            ec = make_error_code(error::not_connected);
            return 0;
        }

        // 获取当前配置
        struct termios options;
        if (tcgetattr(fd_, &options) < 0) {
            ec = error_code(errno, std::system_category());
            return 0;
        }

        // 获取输入波特率（通常输入和输出波特率相同）
        speed_t speed = cfgetispeed(&options);
        ec = error_code();
        return detail::from_speed_t(speed);
    }

    // 设置数据位
    bool set_character_size(character_size size, error_code& ec) {
        if (fd_ < 0) {
            ec = make_error_code(error::not_connected);
            return false;
        }

        // 获取当前配置
        struct termios options;
        if (tcgetattr(fd_, &options) < 0) {
            ec = error_code(errno, std::system_category());
            return false;
        }

        // 清除当前数据位设置并应用新设置
        options.c_cflag &= ~CSIZE;
        options.c_cflag |= detail::to_character_size(size);

        // 应用配置
        if (tcsetattr(fd_, TCSANOW, &options) < 0) {
            ec = error_code(errno, std::system_category());
            return false;
        }

        ec = error_code();
        return true;
    }

    // 获取数据位
    character_size character_size(error_code& ec) const {
        if (fd_ < 0) {
            ec = make_error_code(error::not_connected);
            return character_size::size_8;
        }

        // 获取当前配置
        struct termios options;
        if (tcgetattr(fd_, &options) < 0) {
            ec = error_code(errno, std::system_category());
            return character_size::size_8;
        }

        ec = error_code();
        return detail::from_character_size(options.c_cflag);
    }

    // 设置停止位
    bool set_stop_bits(stop_bits bits, error_code& ec) {
        if (fd_ < 0) {
            ec = make_error_code(error::not_connected);
            return false;
        }

        // 获取当前配置
        struct termios options;
        if (tcgetattr(fd_, &options) < 0) {
            ec = error_code(errno, std::system_category());
            return false;
        }

        // 应用停止位配置
        detail::apply_stop_bits(options, bits);

        // 应用配置
        if (tcsetattr(fd_, TCSANOW, &options) < 0) {
            ec = error_code(errno, std::system_category());
            return false;
        }

        ec = error_code();
        return true;
    }

    // 获取停止位
    stop_bits stop_bits(error_code& ec) const {
        if (fd_ < 0) {
            ec = make_error_code(error::not_connected);
            return stop_bits::one;
        }

        // 获取当前配置
        struct termios options;
        if (tcgetattr(fd_, &options) < 0) {
            ec = error_code(errno, std::system_category());
            return stop_bits::one;
        }

        ec = error_code();
        return detail::get_stop_bits(options);
    }

    // 设置奇偶校验
    bool set_parity(parity p, error_code& ec) {
        if (fd_ < 0) {
            ec = make_error_code(error::not_connected);
            return false;
        }

        // 获取当前配置
        struct termios options;
        if (tcgetattr(fd_, &options) < 0) {
            ec = error_code(errno, std::system_category());
            return false;
        }

        // 应用奇偶校验配置
        detail::apply_parity(options, p);

        // 应用配置
        if (tcsetattr(fd_, TCSANOW, &options) < 0) {
            ec = error_code(errno, std::system_category());
            return false;
        }

        ec = error_code();
        return true;
    }

    // 获取奇偶校验
    parity parity(error_code& ec) const {
        if (fd_ < 0) {
            ec = make_error_code(error::not_connected);
            return parity::none;
        }

        // 获取当前配置
        struct termios options;
        if (tcgetattr(fd_, &options) < 0) {
            ec = error_code(errno, std::system_category());
            return parity::none;
        }

        ec = error_code();
        return detail::get_parity(options);
    }

    // 设置流控制
    bool set_flow_control(flow_control fc, error_code& ec) {
        if (fd_ < 0) {
            ec = make_error_code(error::not_connected);
            return false;
        }

        // 获取当前配置
        struct termios options;
        if (tcgetattr(fd_, &options) < 0) {
            ec = error_code(errno, std::system_category());
            return false;
        }

        // 应用流控制配置
        detail::apply_flow_control(options, fc);

        // 应用配置
        if (tcsetattr(fd_, TCSANOW, &options) < 0) {
            ec = error_code(errno, std::system_category());
            return false;
        }

        ec = error_code();
        return true;
    }

    // 获取流控制
    flow_control flow_control(error_code& ec) const {
        if (fd_ < 0) {
            ec = make_error_code(error::not_connected);
            return flow_control::none;
        }

        // 获取当前配置
        struct termios options;
        if (tcgetattr(fd_, &options) < 0) {
            ec = error_code(errno, std::system_category());
            return flow_control::none;
        }

        ec = error_code();
        return detail::get_flow_control(options);
    }

private:
    // 读操作结构体
    struct read_operation {
        void* buffer;
        std::size_t max_size;
        std::function<void(const error_code&, std::size_t)> handler;
    };

    // 写操作结构体
    struct write_operation {
        std::vector<char> data;
        std::function<void(const error_code&, std::size_t)> handler;
    };

    // 启动工作线程
    void start_worker_thread() {
        if (running_) return;

        running_ = true;
        worker_thread_ = std::thread(&serial_port_service::worker_thread_func, this);
    }

    // 停止工作线程
    void stop_worker_thread() {
        if (!running_) return;

        {
            std::lock_guard<std::mutex> lock(write_mutex_);
            running_ = false;
        }
        
        write_cv_.notify_all();
        read_cv_.notify_all();
        
        if (worker_thread_.joinable()) {
            worker_thread_.join();
        }
    }

    // 工作线程函数
    void worker_thread_func() {
        while (running_) {
            process_io_operations();
        }

        // 处理剩余的操作，通知它们已被取消
        cancel_pending_operations();
    }

    // 处理IO操作
    void process_io_operations() {
        if (fd_ < 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            return;
        }

        // 准备select操作的描述符集
        fd_set read_fds, write_fds;
        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);
        FD_SET(fd_, &read_fds);
        
        bool has_write_ops = false;
        {
            std::lock_guard<std::mutex> lock(write_mutex_);
            has_write_ops = !write_queue_.empty();
        }
        
        if (has_write_ops) {
            FD_SET(fd_, &write_fds);
        }

        // 超时设置
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000;  // 100ms

        // 等待文件描述符就绪
        int ret = select(fd_ + 1, &read_fds, &write_fds, nullptr, &timeout);
        if (ret < 0) {
            // 选择错误
            if (errno != EINTR) {
                error_code ec(errno, std::system_category());
                // 在此处理错误，可能需要通知用户
            }
            return;
        }

        // 处理写操作
        if (FD_ISSET(fd_, &write_fds)) {
            process_write_operations();
        }

        // 处理读操作
        if (FD_ISSET(fd_, &read_fds)) {
            process_read_operations();
        }
    }

    // 处理写操作
    void process_write_operations() {
        // 获取下一个写操作
        std::shared_ptr<write_operation> op;
        {
            std::lock_guard<std::mutex> lock(write_mutex_);
            if (write_queue_.empty()) return;
            op = write_queue_.front();
        }

        // 执行写操作
        error_code ec;
        std::size_t bytes_written = send(op->data.data(), op->data.size(), ec);

        // 调用处理程序
        io_context_.post([op, ec, bytes_written]() {
            op->handler(ec, bytes_written);
        });

        // 从队列中移除操作
        {
            std::lock_guard<std::mutex> lock(write_mutex_);
            if (!write_queue_.empty()) {
                write_queue_.pop();
            }
        }
    }

    // 处理读操作
    void process_read_operations() {
        // 获取下一个读操作
        std::shared_ptr<read_operation> op;
        {
            std::lock_guard<std::mutex> lock(read_mutex_);
            if (read_queue_.empty()) {
                // 如果没有待处理的读操作，读取数据并丢弃
                char buffer[1024];
                error_code ec;
                receive(buffer, sizeof(buffer), ec);
                return;
            }
            op = read_queue_.front();
        }

        // 执行读操作
        error_code ec;
        std::size_t bytes_read = receive(op->buffer, op->max_size, ec);

        // 调用处理程序
        io_context_.post([op, ec, bytes_read]() {
            op->handler(ec, bytes_read);
        });

        // 从队列中移除操作
        {
            std::lock_guard<std::mutex> lock(read_mutex_);
            if (!read_queue_.empty()) {
                read_queue_.pop();
            }
        }
    }

    // 取消所有挂起的操作
    void cancel_pending_operations() {
        // 取消读操作
        {
            std::lock_guard<std::mutex> lock(read_mutex_);
            while (!read_queue_.empty()) {
                auto op = read_queue_.front();
                read_queue_.pop();
                
                io_context_.post([op]() {
                    op->handler(make_error_code(error::operation_aborted), 0);
                });
            }
        }

        // 取消写操作
        {
            std::lock_guard<std::mutex> lock(write_mutex_);
            while (!write_queue_.empty()) {
                auto op = write_queue_.front();
                write_queue_.pop();
                
                io_context_.post([op]() {
                    op->handler(make_error_code(error::operation_aborted), 0);
                });
            }
        }
    }

    io_context& io_context_;
    int fd_;                // 文件描述符
    bool running_;          // 工作线程是否运行
    std::thread worker_thread_;  // 工作线程

    // 读操作队列
    std::mutex read_mutex_;
    std::condition_variable read_cv_;
    std::queue<std::shared_ptr<read_operation>> read_queue_;

    // 写操作队列
    std::mutex write_mutex_;
    std::condition_variable write_cv_;
    std::queue<std::shared_ptr<write_operation>> write_queue_;
};

// serial_port 类实现

serial_port::serial_port(io_context& io_context)
    : service_(std::make_shared<serial_port_service>(io_context)) {
}

serial_port::serial_port(io_context& io_context, const std::string& device, error_code& ec)
    : service_(std::make_shared<serial_port_service>(io_context)),
      device_() {
    open(device, ec);
}

serial_port::~serial_port() = default;

bool serial_port::open(const std::string& device, error_code& ec) {
    if (service_->open(device, ec)) {
        device_ = device;
        return true;
    }
    return false;
}

bool serial_port::is_open() const {
    return service_ && service_->is_open();
}

void serial_port::close(error_code& ec) {
    if (service_) {
        service_->close(ec);
    } else {
        ec = error_code();
    }
}

std::string serial_port::device() const {
    return device_;
}

std::size_t serial_port::send(const void* data, std::size_t size, error_code& ec) {
    if (!service_ || !is_open()) {
        ec = make_error_code(error::not_connected);
        return 0;
    }
    
    return service_->send(data, size, ec);
}

std::size_t serial_port::send(const const_buffer& buffer, error_code& ec) {
    return send(buffer.data(), buffer.size(), ec);
}

std::size_t serial_port::receive(void* data, std::size_t max_size, error_code& ec) {
    if (!service_ || !is_open()) {
        ec = make_error_code(error::not_connected);
        return 0;
    }
    
    return service_->receive(data, max_size, ec);
}

std::size_t serial_port::receive(const mutable_buffer& buffer, error_code& ec) {
    return receive(buffer.data(), buffer.size(), ec);
}

bool serial_port::set_baud_rate(baud_rate rate, error_code& ec) {
    return set_baud_rate(static_cast<unsigned int>(rate), ec);
}

bool serial_port::set_baud_rate(unsigned int rate, error_code& ec) {
    if (!service_ || !is_open()) {
        ec = make_error_code(error::not_connected);
        return false;
    }
    
    return service_->set_baud_rate(rate, ec);
}

unsigned int serial_port::baud_rate(error_code& ec) const {
    if (!service_ || !is_open()) {
        ec = make_error_code(error::not_connected);
        return 0;
    }
    
    return service_->baud_rate(ec);
}

bool serial_port::set_character_size(character_size size, error_code& ec) {
    if (!service_ || !is_open()) {
        ec = make_error_code(error::not_connected);
        return false;
    }
    
    return service_->set_character_size(size, ec);
}

character_size serial_port::character_size(error_code& ec) const {
    if (!service_ || !is_open()) {
        ec = make_error_code(error::not_connected);
        return character_size::size_8;
    }
    
    return service_->character_size(ec);
}

bool serial_port::set_stop_bits(stop_bits bits, error_code& ec) {
    if (!service_ || !is_open()) {
        ec = make_error_code(error::not_connected);
        return false;
    }
    
    return service_->set_stop_bits(bits, ec);
}

stop_bits serial_port::stop_bits(error_code& ec) const {
    if (!service_ || !is_open()) {
        ec = make_error_code(error::not_connected);
        return stop_bits::one;
    }
    
    return service_->stop_bits(ec);
}

bool serial_port::set_parity(parity p, error_code& ec) {
    if (!service_ || !is_open()) {
        ec = make_error_code(error::not_connected);
        return false;
    }
    
    return service_->set_parity(p, ec);
}

parity serial_port::parity(error_code& ec) const {
    if (!service_ || !is_open()) {
        ec = make_error_code(error::not_connected);
        return parity::none;
    }
    
    return service_->parity(ec);
}

bool serial_port::set_flow_control(flow_control fc, error_code& ec) {
    if (!service_ || !is_open()) {
        ec = make_error_code(error::not_connected);
        return false;
    }
    
    return service_->set_flow_control(fc, ec);
}

flow_control serial_port::flow_control(error_code& ec) const {
    if (!service_ || !is_open()) {
        ec = make_error_code(error::not_connected);
        return flow_control::none;
    }
    
    return service_->flow_control(ec);
}

} // namespace asio 