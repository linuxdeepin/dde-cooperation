@echo off
REM WiX Build Script for dde-cooperation and data-transfer using CPack
REM Usage: wix_build.bat <version> [architecture]
REM   version: Version number (e.g., 1.1.23)
REM   architecture: x64 (default) or x86

if "%~1"=="" (
    echo "Usage: wix_build.bat ^<version^> [architecture]"
    echo  " version: Version number (e.g., 1.1.23)"
    echo  " architecture: x64 ^(default^) or x86"
    exit /B 1
)

set APP_VERSION=%~1
echo Setting APP_VERSION: %APP_VERSION%

REM Set architecture (default: x64)
if "%~2"=="" (
    set ARCH=x64
) else (
    set ARCH=%~2
)
echo Build architecture: %ARCH%

REM Validate architecture
if /i "%ARCH%" neq "x64" if /i "%ARCH%" neq "x86" (
    echo Error: ARCH must be x64 or x86
    exit /B 1
)

REM Set Visual Studio environment
set VCINSTALLDIR=C:\Program Files\Microsoft Visual Studio\2022\Community\VC
echo VCINSTALLDIR: %VCINSTALLDIR%

if /i "%ARCH%"=="x64" (
    call "%VCINSTALLDIR%\Auxiliary\Build\vcvars64.bat"
    set CMAKE_ARCH=-A x64
    set "OPENSSLDIR=C:\Program Files\OpenSSL-Win64L"
) else (
    call "%VCINSTALLDIR%\Auxiliary\Build\vcvars32.bat"
    set CMAKE_ARCH=-A Win32
    set "OPENSSLDIR=C:\Program Files (x86)\OpenSSL"
)

REM Defaults
set B_BUILD_TYPE=Release
set B_QT_ROOT=D:\Qt
set B_QT_VER=5.15.2

if "%ARCH%"=="x64" (
    set B_QT_MSVC=msvc2019_64
) else (
    set B_QT_MSVC=msvc2019_86
)

REM Set OpenSSL env
set OPENSSL_ROOT_DIR=%OPENSSLDIR=%

if exist build_env.bat call build_env.bat

set B_QT_FULLPATH=%B_QT_ROOT%\%B_QT_VER%\%B_QT_MSVC%
echo Qt: %B_QT_FULLPATH%
echo OpenSSL: %OPENSSL_ROOT_DIR%

set savedir=%cd%
cd /d %~dp0

REM Detect CMake generator
if "%VisualStudioVersion%"=="15.0" (
    set cmake_gen=Visual Studio 15 2017
) else if "%VisualStudioVersion%"=="16.0" (
    set cmake_gen=Visual Studio 16 2019
) else (
    echo Using Visual Studio 2022
    set cmake_gen=Visual Studio 17 2022
)

REM Clean and create build directory
@REM rmdir /q /s build 2>NUL
@REM mkdir build
cd build

echo ------------Starting CMake configuration------------

cmake -G "%cmake_gen%" %CMAKE_ARCH% ^
    -D CMAKE_BUILD_TYPE=%B_BUILD_TYPE% ^
    -D CMAKE_PREFIX_PATH="%B_QT_FULLPATH%" ^
    -D QT_VERSION=%B_QT_VER% ^
    -D APP_VERSION=%APP_VERSION% ^
    -D ENABLE_WIX=ON ^
    ..
if ERRORLEVEL 1 goto failed
@REM exit /B 1
echo ------------Building project------------
cmake --build . --config %B_BUILD_TYPE%
if ERRORLEVEL 1 goto failed

echo ------------Copying dependency files------------

if exist output\%B_BUILD_TYPE% (
    if "%ARCH%"=="x64" (
        copy "%OPENSSL_ROOT_DIR%\bin\libcrypto-3-x64.dll" output\dde-cooperation\%B_BUILD_TYPE%\ > NUL
        copy "%OPENSSL_ROOT_DIR%\bin\libssl-3-x64.dll" output\dde-cooperation\%B_BUILD_TYPE%\ > NUL
        copy "%OPENSSL_ROOT_DIR%\bin\libcrypto-3-x64.dll" output\data-transfer\%B_BUILD_TYPE%\ > NUL
        copy "%OPENSSL_ROOT_DIR%\bin\libssl-3-x64.dll" output\data-transfer\%B_BUILD_TYPE%\ > NUL
    ) else (
        copy "%OPENSSL_ROOT_DIR%\bin\libcrypto-3-x86.dll" output\dde-cooperation\%B_BUILD_TYPE%\ > NUL
        copy "%OPENSSL_ROOT_DIR%\bin\libssl-3-x86.dll" output\dde-cooperation\%B_BUILD_TYPE%\ > NUL
        copy "%OPENSSL_ROOT_DIR%\bin\libcrypto-3-x86.dll" output\data-transfer\%B_BUILD_TYPE%\ > NUL
        copy "%OPENSSL_ROOT_DIR%\bin\libssl-3-x86.dll" output\data-transfer\%B_BUILD_TYPE%\ > NUL
    )
) else (
    echo Warning: output directory not found
)

echo ------------Building WiX MSI packages using CMake package target------------
cmake --build . --config %B_BUILD_TYPE% --target package
if ERRORLEVEL 1 goto wixfailed

echo.
echo ========================================
echo WiX MSI Build completed successfully!
echo Architecture: %ARCH%
echo Output directory: build\_CPack_Packages
echo ========================================
echo.
echo MSI installers:
dir /s /b "_CPack_Packages\*.msi" 2>NUL
echo.

set BUILD_FAILED=0
goto done

:wixfailed
echo WiX MSI build failed
set BUILD_FAILED=1
goto done

:failed
echo Build failed
set BUILD_FAILED=%ERRORLEVEL%

:done
cd /d %savedir%

set B_BUILD_TYPE=
set B_QT_ROOT=
set B_QT_VER=
set B_QT_MSVC=
set savedir=
set cmake_gen=
set ARCH=

EXIT /B %BUILD_FAILED%
