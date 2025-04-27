# dde-cooperation - Deepin Data Transfer and Cooperation Tool

A cross-platform data transfer and device cooperation solution developed by Deepin team, providing seamless file sharing and device collaboration capabilities.

## Features
- Bidirectional file transfer between devices
- Secure transfer protocol with encryption support
- Cross-platform compatibility (Linux, Windows, macOS)
- Qt-based GUI with material design

## Dependencies
- Qt 5.15+ (Core, Gui, Network, Widgets)
- Protocol Buffers 3.0+
- ASIO network library
- QuaZIP for archive operations

## Installation
### Linux
#### Qt6 Build:
```bash
mkdir build && cd build
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DQT_DIR=/usr/lib/x86_64-linux-gnu/cmake/Qt6 -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build -j$(nproc)
sudo make install
```

#### Qt5 Build:
```bash
mkdir build && cd build
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DQT_DIR=/usr/lib/x86_64-linux-gnu/cmake/Qt5 -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build -j$(nproc)
sudo make install
```

#### Debian Package:
```bash
dpkg-buildpackage -us -uc -b -tc -j$(nproc)
```

### Windows
#### Build and Install:
1. Install prerequisites:
   - Qt 5.15+/Qt6
   - CMake
   - Visual Studio 2022
   - Inno Setup 6 (for creating installers)
   - OpenSSL (optional)

2. Modify clean_build.bat variables as needed:
```bat
set B_QT_ROOT=D:\Qt  # Path to Qt installation
set B_QT_VER=5.15.2  # Qt version
set B_QT_MSVC=msvc2019_64  # Qt MSVC version
set OPENSSL_ROOT_DIR=C:\Program Files\OpenSSL-Win64  # OpenSSL path
```

3. Run the build script:
```bat
clean_build.bat [version]  # version should match debian/changelog
```

4. The script will:
   - Configure the project with CMake
   - Build using Visual Studio
   - Create installers in build/installer-inno directory

## Getting Involved
We encourage you to report issues and contribute changes. Please follow our contribution guidelines.

## Contribution Guide
See [CONTRIBUTING.md](CONTRIBUTING.md) for detailed developer guidelines.

## License
dde-cooperation is licensed under [GPL-3.0-or-later](LICENSES/GPL-3.0-or-later.txt)
