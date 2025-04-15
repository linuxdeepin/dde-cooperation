# uasio

一个基于C++17的轻量级异步IO库，灵感来自非Boost版本的Asio。

## 已实现功能

- **基础设施**
  - `io_context`: 异步IO上下文，管理事件循环和调度任务
  - `error`: 提供统一的错误处理机制
  - `strand`: 线程安全的执行器，确保任务按顺序执行

- **时间相关**
  - `timer`: 定时器功能，基于系统时间（可能受系统时间调整影响）
  - `steady_timer`: 基于稳定时钟的定时器，不受系统时间调整影响

- **网络相关**
  - `socket`: 基础的套接字抽象
  - `socket_service`: 内部套接字服务实现
  - `resolver`: 域名解析器，支持同步和异步解析

- **缓冲区相关**
  - `buffer`: 基础的内存缓冲区抽象，支持只读和可写入的缓冲区
  - `streambuf`: 流缓冲区，支持动态大小的缓冲区，用于高效数据处理

- **系统相关**
  - `signal_set`: 信号集类，用于处理系统信号（如SIGINT, SIGTERM等）

## 验证结果

### 编译状态

目前项目编译存在一些问题：

- `io_context`, `socket` 和 `resolver` 等组件之间存在一些接口不一致问题
- `socket.cpp` 中的实现与 `socket.h` 中的声明不匹配
- `error.cpp` 已经与 `error.h` 正确匹配

### 功能验证

我们已经成功验证了以下组件的功能：

1. **buffer** (`buffer.h`)
   - 支持从原始指针、字符串、向量和数组创建缓冲区
   - 支持缓冲区偏移和大小查询
   - 支持区分只读缓冲区和可写入缓冲区

2. **streambuf** (`streambuf.h` 和 `streambuf.cpp`)
   - 支持创建动态大小的缓冲区
   - 支持写入、提交、读取和消费数据
   - 支持缓冲区大小限制和异常处理
   - 支持缓冲区清空和数据转换

3. **strand** (`strand.h`)
   - 线程安全的执行器
   - 确保任务按顺序执行，避免数据竞争
   - 支持 `dispatch` 和 `post` 两种任务提交方式

4. **signal_set** (`signal_set.h`)
   - 支持添加、移除和清空信号
   - 支持异步等待信号
   - 线程安全的信号处理

5. **steady_timer** (`steady_timer.h` 和 `steady_timer.cpp`)
   - 基于稳定时钟的定时器，不受系统时间调整影响
   - 支持设置绝对时间和相对时间
   - 支持异步等待和取消操作
   - 高精度计时功能

## 下一步工作

1. 修复 `io_context` 和 `socket` 等组件之间的接口不一致问题
2. 完善平台相关的实现 (`linux_epoll.cpp` 和 `win_iocp.cpp`)
3. 增加单元测试，提高代码覆盖率
4. 实现更多组件，完善异步IO库功能
5. 完善示例程序，展示各组件的使用方法

## 构建说明

```bash
# 创建构建目录
mkdir build && cd build

# 配置
cmake ..

# 编译
make

# 运行示例
./examples/streambuf_example
./examples/strand_example
./examples/signal_set_example
./examples/steady_timer_example
```

## 许可证

GPL-3.0-or-later 