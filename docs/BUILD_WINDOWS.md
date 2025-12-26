# Windows 构建文档

## 概述

本文档介绍如何为 dde-cooperation 和 data-transfer 应用程序构建 Windows 安装包。

## 构建方式对比

### 1. Inno Setup (旧方式)
- **脚本**: `clean_build.bat`
- **工具**: Inno Setup Compiler (ISCC.exe)
- **输出**: EXE 安装程序
- **优点**: 界面美观，配置灵活
- **缺点**: 需要额外工具，不兼容Windows 7

### 2. WiX + CPack (新方式) ⭐
- **脚本**: `wix_build.bat`
- **工具**: WiX Toolset 4+ + CMake/CPack
- **输出**: MSI 安装包
- **优点**: 
  - 原生Windows安装程序
  - 支持Windows 7
  - 与CMake集成，一键打包
  - 更好的系统集成
- **缺点**: 配置相对复杂

## 快速开始

### 前置要求

1. **Visual Studio 2019/2022** (已安装C++构建工具)
2. **Qt 5.15.2** (MSVC版本)
   - Qt 5.15.2 MSVC2019 64-bit (用于x64构建)
   - Qt 5.15.2 MSVC2019 32-bit (用于x86构建，支持Win7)
3. **OpenSSL** (对应架构版本)
   - OpenSSL 3.x 64-bit (用于x64构建)
   - OpenSSL 3.x 32-bit (用于x86构建)
4. **WiX Toolset 4+** (可选，用于MSI打包)
   ```powershell
   winget install WiX.Toolset
   ```

### 构建命令

#### 构建64位版本 (Windows 10/11)

```batch
# 构建dde-cooperation和data-transfer的64位MSI安装包
wix_build.bat 1.1.23 x64
```

#### 构建32位版本 (Windows 7支持)

```batch
# 构建dde-cooperation和data-transfer的32位MSI安装包
wix_build.bat 1.1.23 x86
```

**参数说明**:
- 第一个参数: 版本号 (必须与debian/changelog一致)
- 第二个参数: 架构 (x64或x86，默认x64)

### 使用CI/CD自动打包

在GitHub Actions或类似CI系统中，只需一行命令即可完成打包：

```yaml
- name: Configure
  run: cmake -G "Visual Studio 17 2022" -A x64 -D ENABLE_WIX=ON -D APP_VERSION="1.1.23" ..

- name: Build and Package
  run: cmake --build build --config Release --target package
```

## 构建流程详解

### 1. CMake配置阶段

```batch
cmake -G "Visual Studio 17 2022" -A x64 ^
    -D CMAKE_BUILD_TYPE=Release ^
    -D CMAKE_PREFIX_PATH="D:\Qt\5.15.2\msvc2019_64" ^
    -D QT_VERSION=5.15.2 ^
    -D APP_VERSION=1.1.23 ^
    -D ENABLE_WIX=ON ^
    ..
```

**关键变量**:
- `CMAKE_PREFIX_PATH`: Qt安装路径
- `QT_VERSION`: Qt版本号
- `APP_VERSION`: 应用版本号
- `ENABLE_WIX`: 启用WiX打包

### 2. 编译阶段

```batch
cmake --build . --config Release
```

这会编译两个应用程序：
- `dde-cooperation.exe` (跨端协同)
- `deepin-data-transfer.exe` (数据迁移)

### 3. 打包阶段

```batch
cmake --build . --config Release --target package
```

CPack会自动生成：
- **MSI安装包**: 如果安装了WiX Toolset
- **7Z压缩包**: 作为备选方案

## 输出文件

### 构建目录结构

```
build/
├── output/
│   └── Release/
│       ├── dde-cooperation/
│       │   ├── dde-cooperation.exe
│       │   ├── *.dll (Qt运行时)
│       │   └── translations/
│       └── data-transfer/
│           ├── deepin-data-transfer.exe
│           ├── *.dll (Qt运行时)
│           └── translations/
├── _CPack_Packages/
│   └── win64/
│       └── WIX/
│           ├── deepin-cooperation-1.1.23-win-x64.msi
│           └── deepin-datatransfer-1.1.23-win-x64.msi
└── *.7z (可选)
```

### 安装包命名规则

```
{应用名}-{版本号}-win-{架构}.msi

示例:
- deepin-cooperation-1.1.23-win-x64.msi (64位)
- deepin-cooperation-1.1.23-win-x86.msi (32位，支持Win7)
- deepin-datatransfer-1.1.23-win-x64.msi (64位)
- deepin-datatransfer-1.1.23-win-x86.msi (32位，支持Win7)
```

## 架构特定配置

### 64位构建 (x64)

**依赖库**:
- Qt: `D:\Qt\5.15.2\msvc2019_64`
- OpenSSL: `C:\Program Files\OpenSSL`
- Visual Studio: vcvars64.bat

**目标系统**: Windows 10/11 x64

### 32位构建 (x86) - Windows 7支持

**依赖库**:
- Qt: `D:\Qt\5.15.2\msvc2019_86`
- OpenSSL: `C:\Program Files (x86)\OpenSSL`
- Visual Studio: vcvars32.bat

**目标系统**: Windows 7 SP1+ (32位或64位)

**注意事项**:
1. 确保32位Qt和OpenSSL已安装
2. Windows 7需要安装KB4474419更新
3. 可能需要VC++ 2015-2022 Redistributable

## 高级配置

### 自定义构建环境

创建 `build_env.bat` 文件覆盖默认路径：

```batch
@echo off
set B_QT_ROOT=D:\Qt
set B_QT_VER=5.15.2
set B_QT_MSVC=msvc2019_64
set OPENSSL_ROOT_DIR=C:\OpenSSL
```

### CPack配置文件

每个应用都有自己的CPack配置：

- `src/apps/dde-cooperation/packaging/wix/cpack_config.cmake`
- `src/apps/data-transfer/packaging/wix/cpack_config.cmake`

**可配置项**:
- 安装包名称、厂商、描述
- UI配置（横幅、对话框图片）
- 开始菜单文件夹
- 升级GUID
- 快捷方式设置

### WiX Fragment文件

位置: `src/apps/{应用名}/packaging/wix/fragments/`

**文件说明**:
- `shortcuts.wxs.in`: 快捷方式配置
- `app_files.wxs.in`: 应用程序文件配置

## 故障排查

### 问题1: WiX工具未找到

**错误**: `WiX Toolset 4+ not found`

**解决方案**:
```powershell
# 安装WiX Toolset
winget install WiX.Toolset

# 或从官网下载
# https://wixtoolset.org/releases/
```

### 问题2: OpenSSL DLL缺失

**错误**: `libcrypto-3-x64.dll not found`

**解决方案**:
1. 检查OpenSSL安装路径
2. 手动复制DLL到输出目录:
```batch
copy "C:\Program Files\OpenSSL\bin\libcrypto-3-x64.dll" build\output\Release\dde-cooperation\
copy "C:\Program Files\OpenSSL\bin\libssl-3-x64.dll" build\output\Release\dde-cooperation\
```

### 问题3: 32位构建失败

**错误**: Qt或OpenSSL库不匹配

**解决方案**:
1. 确保使用32位Qt: `msvc2019_86`
2. 确保使用32位OpenSSL: `C:\Program Files (x86)\OpenSSL`
3. 检查Visual Studio环境: `call vcvars32.bat`

### 问题4: MSI安装失败

**错误**: 安装时出现错误代码

**解决方案**:
1. 检查Windows Installer版本
2. 查看MSI日志: `msiexec /i package.msi /l*vx install.log`
3. 确保升级GUID正确

## 从Inno迁移到WiX

### 迁移步骤

1. **保留Inno配置** (备份)
   ```batch
   xcopy dist\inno dist\inno.backup\ /E /I
   ```

2. **测试WiX构建**
   ```batch
   wix_build.bat 1.1.23 x64
   ```

3. **验证MSI安装包**
   - 在虚拟机中安装测试
   - 检查所有功能是否正常
   - 验证卸载功能

4. **部署新安装包**
   - 替换下载链接
   - 更新文档
   - 通知用户

### 主要差异

| 特性 | Inno Setup | WiX MSI |
|------|------------|---------|
| 输出格式 | EXE | MSI |
| Windows 7支持 | ❌ | ✅ |
| 系统集成 | 一般 | 好 |
| 升级机制 | 自定义 | 原生 |
| 网络安装 | 支持 | 支持但复杂 |
| 数字签名 | 容易 | 需要额外配置 |

## 维护建议

1. **版本管理**: 使用Git标签标记每个发布版本
2. **自动化**: 使用CI/CD自动构建和发布
3. **测试**: 在多个Windows版本上测试安装包
4. **签名**: 对安装包进行数字签名以避免安全警告
5. **文档**: 保持构建文档与代码同步更新

## 参考资源

- [WiX Toolset文档](https://wixtoolset.org/documentation/)
- [CPack文档](https://cmake.org/cmake/help/latest/module/CPack.html)
- [Qt Windows部署](https://doc.qt.io/qt-6/windows-deployment.html)
- [项目GitHub](https://github.com/linuxdeepin/dde-cooperation)
