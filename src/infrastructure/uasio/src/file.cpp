// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "file.h"
#include <mutex>
#include <thread>
#include <condition_variable>
#include <queue>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <cstring>
#include <aio.h>

namespace uasio {

// 辅助函数：转换文件打开模式为系统标志
namespace detail {

int convert_file_mode(file_mode mode) {
    int flags = 0;
    
    switch (mode) {
        case file_mode::read_only:
            flags = O_RDONLY;
            break;
        case file_mode::write_only:
            flags = O_WRONLY | O_CREAT;
            break;
        case file_mode::read_write:
            flags = O_RDWR | O_CREAT;
            break;
        case file_mode::append:
            flags = O_WRONLY | O_CREAT | O_APPEND;
            break;
        case file_mode::truncate:
            flags = O_WRONLY | O_CREAT | O_TRUNC;
            break;
        default:
            flags = O_RDONLY;
            break;
    }
    
    // 添加非阻塞标志
    flags |= O_NONBLOCK;
    
    return flags;
}

int convert_seek_basis(seek_basis basis) {
    switch (basis) {
        case seek_basis::start:
            return SEEK_SET;
        case seek_basis::current:
            return SEEK_CUR;
        case seek_basis::end:
            return SEEK_END;
        default:
            return SEEK_SET;
    }
}

} // namespace detail

// 文件服务类实现
class file::file_service {
public:
    // 构造函数
    explicit file_service(io_context& io_context)
        : io_context_(io_context),
          fd_(-1),
          running_(false) {
    }

    // 析构函数
    ~file_service() {
        close();
    }

    // 获取IO上下文
    io_context& get_io_context() {
        return io_context_;
    }

    // 打开文件
    bool open(const std::string& path, file_mode mode, error_code& ec) {
        // 关闭已存在的文件
        close();

        // 转换打开模式
        int flags = detail::convert_file_mode(mode);
        
        // 打开文件
        fd_ = ::open(path.c_str(), flags, 0666);
        if (fd_ < 0) {
            ec = error_code(errno, std::system_category());
            return false;
        }

        // 启动工作线程
        start_worker_thread();

        ec = error_code();
        return true;
    }

    // 检查文件是否打开
    bool is_open() const {
        return (fd_ >= 0);
    }

    // 关闭文件
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

    // 获取文件大小
    std::size_t size(error_code& ec) const {
        if (fd_ < 0) {
            ec = make_error_code(error::not_connected);
            return 0;
        }

        struct stat st;
        if (::fstat(fd_, &st) < 0) {
            ec = error_code(errno, std::system_category());
            return 0;
        }

        ec = error_code();
        return static_cast<std::size_t>(st.st_size);
    }

    // 读取数据
    std::size_t read(void* data, std::size_t size, error_code& ec) {
        if (fd_ < 0) {
            ec = make_error_code(error::not_connected);
            return 0;
        }

        ssize_t bytes_read = ::read(fd_, data, size);
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

    // 异步读取数据
    template <typename ReadHandler>
    void async_read(void* data, std::size_t size, ReadHandler&& handler) {
        if (fd_ < 0) {
            io_context_.post([handler = std::forward<ReadHandler>(handler)]() {
                handler(make_error_code(error::not_connected), 0);
            });
            return;
        }

        // 创建读操作并添加到队列
        auto op = std::make_shared<read_operation>();
        op->buffer = data;
        op->size = size;
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

    // 写入数据
    std::size_t write(const void* data, std::size_t size, error_code& ec) {
        if (fd_ < 0) {
            ec = make_error_code(error::not_connected);
            return 0;
        }

        ssize_t bytes_written = ::write(fd_, data, size);
        if (bytes_written < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // 非阻塞操作，暂时无法写入
                ec = make_error_code(error::would_block);
                return 0;
            }
            ec = error_code(errno, std::system_category());
            return 0;
        }

        ec = error_code();
        return static_cast<std::size_t>(bytes_written);
    }

    // 异步写入数据
    template <typename WriteHandler>
    void async_write(const void* data, std::size_t size, WriteHandler&& handler) {
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

    // 设置文件读写位置
    std::size_t seek(std::int64_t offset, seek_basis basis, error_code& ec) {
        if (fd_ < 0) {
            ec = make_error_code(error::not_connected);
            return 0;
        }

        int whence = detail::convert_seek_basis(basis);
        off_t result = ::lseek(fd_, offset, whence);
        if (result < 0) {
            ec = error_code(errno, std::system_category());
            return 0;
        }

        ec = error_code();
        return static_cast<std::size_t>(result);
    }

    // 获取当前文件读写位置
    std::size_t tell(error_code& ec) const {
        if (fd_ < 0) {
            ec = make_error_code(error::not_connected);
            return 0;
        }

        off_t result = ::lseek(fd_, 0, SEEK_CUR);
        if (result < 0) {
            ec = error_code(errno, std::system_category());
            return 0;
        }

        ec = error_code();
        return static_cast<std::size_t>(result);
    }

    // 刷新缓冲区
    bool flush(error_code& ec) {
        if (fd_ < 0) {
            ec = make_error_code(error::not_connected);
            return false;
        }

        if (::fsync(fd_) < 0) {
            ec = error_code(errno, std::system_category());
            return false;
        }

        ec = error_code();
        return true;
    }

    // 异步刷新缓冲区
    template <typename FlushHandler>
    void async_flush(FlushHandler&& handler) {
        if (fd_ < 0) {
            io_context_.post([handler = std::forward<FlushHandler>(handler)]() {
                handler(make_error_code(error::not_connected));
            });
            return;
        }

        // 创建刷新操作并添加到队列
        auto op = std::make_shared<flush_operation>();
        op->handler = [handler = std::forward<FlushHandler>(handler)](const error_code& ec) {
            handler(ec);
        };

        {
            std::lock_guard<std::mutex> lock(flush_mutex_);
            flush_queue_.push(op);
        }
        
        // 通知工作线程有新的刷新操作
        flush_cv_.notify_one();
    }

private:
    // 读操作结构体
    struct read_operation {
        void* buffer;
        std::size_t size;
        std::function<void(const error_code&, std::size_t)> handler;
    };

    // 写操作结构体
    struct write_operation {
        std::vector<char> data;
        std::function<void(const error_code&, std::size_t)> handler;
    };

    // 刷新操作结构体
    struct flush_operation {
        std::function<void(const error_code&)> handler;
    };

    // 启动工作线程
    void start_worker_thread() {
        if (running_) return;

        running_ = true;
        worker_thread_ = std::thread(&file_service::worker_thread_func, this);
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
        flush_cv_.notify_all();
        
        if (worker_thread_.joinable()) {
            worker_thread_.join();
        }
    }

    // 工作线程函数
    void worker_thread_func() {
        while (running_) {
            bool has_work = process_read_operations() || 
                            process_write_operations() ||
                            process_flush_operations();
                            
            if (!has_work) {
                // 如果没有工作，休眠一小段时间
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }

        // 处理剩余的操作，通知它们已被取消
        cancel_pending_operations();
    }

    // 处理读操作
    bool process_read_operations() {
        std::shared_ptr<read_operation> op;
        
        {
            std::lock_guard<std::mutex> lock(read_mutex_);
            if (read_queue_.empty()) {
                return false;
            }
            op = read_queue_.front();
            read_queue_.pop();
        }
        
        // 执行读操作
        error_code ec;
        std::size_t bytes_read = read(op->buffer, op->size, ec);
        
        // 如果操作会阻塞，重新加入队列以便稍后重试
        if (ec == make_error_code(error::would_block)) {
            std::lock_guard<std::mutex> lock(read_mutex_);
            read_queue_.push(op);
            return true;
        }
        
        // 调用处理程序
        io_context_.post([op, ec, bytes_read]() {
            op->handler(ec, bytes_read);
        });
        
        return true;
    }
    
    // 处理写操作
    bool process_write_operations() {
        std::shared_ptr<write_operation> op;
        
        {
            std::lock_guard<std::mutex> lock(write_mutex_);
            if (write_queue_.empty()) {
                return false;
            }
            op = write_queue_.front();
            write_queue_.pop();
        }
        
        // 执行写操作
        error_code ec;
        std::size_t bytes_written = write(op->data.data(), op->data.size(), ec);
        
        // 如果操作会阻塞，重新加入队列以便稍后重试
        if (ec == make_error_code(error::would_block)) {
            std::lock_guard<std::mutex> lock(write_mutex_);
            write_queue_.push(op);
            return true;
        }
        
        // 调用处理程序
        io_context_.post([op, ec, bytes_written]() {
            op->handler(ec, bytes_written);
        });
        
        return true;
    }
    
    // 处理刷新操作
    bool process_flush_operations() {
        std::shared_ptr<flush_operation> op;
        
        {
            std::lock_guard<std::mutex> lock(flush_mutex_);
            if (flush_queue_.empty()) {
                return false;
            }
            op = flush_queue_.front();
            flush_queue_.pop();
        }
        
        // 执行刷新操作
        error_code ec;
        bool success = flush(ec);
        
        // 调用处理程序
        io_context_.post([op, ec]() {
            op->handler(ec);
        });
        
        return true;
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
        
        // 取消刷新操作
        {
            std::lock_guard<std::mutex> lock(flush_mutex_);
            while (!flush_queue_.empty()) {
                auto op = flush_queue_.front();
                flush_queue_.pop();
                
                io_context_.post([op]() {
                    op->handler(make_error_code(error::operation_aborted));
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
    
    // 刷新操作队列
    std::mutex flush_mutex_;
    std::condition_variable flush_cv_;
    std::queue<std::shared_ptr<flush_operation>> flush_queue_;
};

// file 类实现

file::file(io_context& io_context)
    : service_(std::make_shared<file_service>(io_context)),
      path_() {
}

file::file(io_context& io_context, const std::string& path, file_mode mode, error_code& ec)
    : service_(std::make_shared<file_service>(io_context)),
      path_() {
    open(path, mode, ec);
}

file::~file() = default;

bool file::open(const std::string& path, file_mode mode, error_code& ec) {
    if (service_->open(path, mode, ec)) {
        path_ = path;
        return true;
    }
    return false;
}

bool file::is_open() const {
    return service_ && service_->is_open();
}

void file::close(error_code& ec) {
    if (service_) {
        service_->close(ec);
    } else {
        ec = error_code();
    }
}

std::string file::path() const {
    return path_;
}

std::size_t file::size(error_code& ec) const {
    if (!service_ || !is_open()) {
        ec = make_error_code(error::not_connected);
        return 0;
    }
    
    return service_->size(ec);
}

std::size_t file::read(void* data, std::size_t size, error_code& ec) {
    if (!service_ || !is_open()) {
        ec = make_error_code(error::not_connected);
        return 0;
    }
    
    return service_->read(data, size, ec);
}

std::size_t file::read(const mutable_buffer& buffer, error_code& ec) {
    return read(buffer.data(), buffer.size(), ec);
}

std::size_t file::write(const void* data, std::size_t size, error_code& ec) {
    if (!service_ || !is_open()) {
        ec = make_error_code(error::not_connected);
        return 0;
    }
    
    return service_->write(data, size, ec);
}

std::size_t file::write(const const_buffer& buffer, error_code& ec) {
    return write(buffer.data(), buffer.size(), ec);
}

std::size_t file::seek(std::int64_t offset, seek_basis basis, error_code& ec) {
    if (!service_ || !is_open()) {
        ec = make_error_code(error::not_connected);
        return 0;
    }
    
    return service_->seek(offset, basis, ec);
}

std::size_t file::tell(error_code& ec) const {
    if (!service_ || !is_open()) {
        ec = make_error_code(error::not_connected);
        return 0;
    }
    
    return service_->tell(ec);
}

bool file::flush(error_code& ec) {
    if (!service_ || !is_open()) {
        ec = make_error_code(error::not_connected);
        return false;
    }
    
    return service_->flush(ec);
}

} // namespace uasio 