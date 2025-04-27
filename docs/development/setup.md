+++
id = "dev-setup-20250427"
title = "Development Environment Setup Guide"
status = "draft"
doc_version = "1.0"
audience = ["developers"]
last_updated = "2025-04-27"
tags = ["development", "setup", "environment"]
+++

# Development Environment Setup Guide

## Prerequisites

### System Requirements
- **Operating System**: Linux (recommended), Windows, or macOS
- **CPU**: x86_64 or ARM64 architecture
- **Memory**: Minimum 8GB RAM (16GB recommended)
- **Disk Space**: Minimum 10GB available space

### Software Dependencies
- **CMake**: Version 3.13 or higher
- **C++ Compiler**: Supporting C++17 standard
  - GCC 9+ (Linux)
  - Clang 10+ (macOS)
  - MSVC 2019+ (Windows)
- **Qt**: Version 5.15 or 6.x
- **Build Tools**:
  - Ninja (recommended) or Make
  - On Windows: Visual Studio 2019/2022

## Setup Instructions

### Linux
```bash
# Install dependencies on Ubuntu/Debian
sudo apt update
sudo apt install -y git cmake build-essential qt6-base-dev ninja-build

# Clone repository
git clone https://github.com/dde-cooperation/dde-cooperation.git
cd dde-cooperation

# Configure and build
mkdir build && cd build
cmake -GNinja -DCMAKE_BUILD_TYPE=Release ..
ninja
```

### Windows
1. Install Visual Studio 2019/2022 with C++ support
2. Install Qt for Windows
3. Install CMake and Ninja
4. Open Developer Command Prompt:
```bat
git clone https://github.com/dde-cooperation/dde-cooperation.git
cd dde-cooperation
mkdir build && cd build
cmake -GNinja -DCMAKE_BUILD_TYPE=Release ..
ninja
```

### macOS
```bash
# Install dependencies using Homebrew
brew install cmake qt ninja

# Clone repository
git clone https://github.com/dde-cooperation/dde-cooperation.git
cd dde-cooperation

# Configure and build
mkdir build && cd build
cmake -GNinja -DCMAKE_BUILD_TYPE=Release ..
ninja
```

## Configuration Options

### Common CMake Options
- `-DENABLE_SLOTIPC=ON/OFF`: Enable/disable compatibility mode (default: ON)
- `-DCMAKE_BUILD_TYPE`: Debug/Release/RelWithDebInfo
- `-DQT_VERSION_MAJOR`: Specify Qt version (5 or 6)

## IDE Setup

### VS Code
1. Install C++ extension
2. Configure CMake Tools extension:
```json
{
  "cmake.buildDirectory": "${workspaceFolder}/build",
  "cmake.configureArgs": ["-GNinja"]
}
```

### Qt Creator
1. Open project root CMakeLists.txt
2. Configure with desired build options
3. Select Ninja generator if available

## Troubleshooting

**Issue**: Missing Qt components
**Solution**: Ensure Qt is properly installed and QTDIR environment variable is set

**Issue**: ASIO not found
**Solution**: Clone ASIO repository to 3rdparty/asio