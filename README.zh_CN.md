# dde-cooperation - 深度数据协作工具

由深度团队开发的跨平台数据传输与设备协作解决方案，提供无缝的文件共享和设备协作能力。

## 功能特性
- 设备间双向文件传输
- 支持加密的安全传输协议
- 跨平台兼容(Linux、Windows、macOS)
- 基于Qt的Material Design风格GUI界面

## 依赖项
- Qt 5.15+(Core, Gui, Network, Widgets)
- Protocol Buffers 3.0+
- ASIO网络库
- QuaZIP压缩库

## 安装指南
### Linux系统
#### Qt6 构建:
```bash
mkdir build && cd build
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DQT_DIR=/usr/lib/x86_64-linux-gnu/cmake/Qt6 -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build -j$(nproc)
sudo make install
```

#### Qt5 构建:
```bash
mkdir build && cd build
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DQT_DIR=/usr/lib/x86_64-linux-gnu/cmake/Qt5 -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build -j$(nproc)
sudo make install
```

#### Debian 打包:
```bash
dpkg-buildpackage -us -uc -b -tc -j$(nproc)
```

### Windows系统
#### 编译和安装:
1. 安装依赖项:
   - Qt 5.15+/Qt6
   - CMake
   - Visual Studio 2022
   - Inno Setup 6 (用于创建安装包)
   - OpenSSL (可选)

2. 根据需要修改clean_build.bat中的变量:
```bat
set B_QT_ROOT=D:\Qt  # Qt安装路径
set B_QT_VER=5.15.2  # Qt版本
set B_QT_MSVC=msvc2019_64  # Qt MSVC版本
set OPENSSL_ROOT_DIR=C:\Program Files\OpenSSL-Win64  # OpenSSL路径
```

3. 运行构建脚本:
```bat
clean_build.bat [版本号]  # 版本号应与debian/changelog一致
```

4. 脚本将执行以下操作:
   - 使用CMake配置项目
   - 使用Visual Studio编译
   - 在build/installer-inno目录下创建安装包

## 参与贡献
我们欢迎您报告问题并提交改进。请遵循我们的贡献指南。

## 开发者贡献指南
参见[CONTRIBUTING.zh_CN.md](CONTRIBUTING.zh_CN.md)获取详细的中文开发指南

## 许可证
dde-cooperation采用[GPL-3.0-or-later](LICENSES/GPL-3.0-or-later.txt)许可证
