@echo off

set BUILD_ARCH=%~1
if "%BUILD_ARCH%"=="" set BUILD_ARCH=x64
echo Build architecture: %BUILD_ARCH%

REM defaults - override them by creating a build_env.bat file
set B_BUILD_TYPE=Release
set B_QT_ROOT=D:\Qt
set B_QT_VER=5.15.2
@REM set B_QT_MSVC=msvc2019_64
set B_BONJOUR=D:\repo\barrier-2.4.0\Bonjour SDK

if "%BUILD_ARCH%"=="x86" (
    set VS_ARCH=Win32
    set B_QT_MSVC=msvc2019
    set "OPENSSL_ROOT_DIR=C:\Program Files (x86)\OpenSSL-Win32"
) else (
    set VS_ARCH=x64
    set B_QT_MSVC=msvc2019_64
    set "OPENSSL_ROOT_DIR=C:\Program Files\OpenSSL-Win64"
)

set savedir=%cd%
cd /d %~dp0

REM cmake generator name for the target build system
if "%VisualStudioVersion%"=="15.0" (
    set cmake_gen=Visual Studio 15 2017
) else if "%VisualStudioVersion%"=="16.0" (
    set cmake_gen=Visual Studio 16 2019
) else (
    echo Visual Studio version was not detected.
    echo Did you forget to run inside a VS developer prompt?
    echo Using the default cmake generator.
    set cmake_gen=Visual Studio 17 2022
)

if exist build_env.bat call build_env.bat

REM needed by cmake to set bonjour include dir
set BONJOUR_SDK_HOME=%B_BONJOUR%

REM full path to Qt stuff we need
set B_QT_FULLPATH=%B_QT_ROOT%\%B_QT_VER%\%B_QT_MSVC%

echo Bonjour: %BONJOUR_SDK_HOME%
echo Qt: %B_QT_FULLPATH%

git submodule update --init --recursive

rmdir /q /s build
mkdir build
if ERRORLEVEL 1 goto failed
cd build
@REM cmake -G "%cmake_gen%" -A %VS_ARCH% -D CMAKE_BUILD_TYPE=%B_BUILD_TYPE% -D CMAKE_PREFIX_PATH="%B_QT_FULLPATH%" -D DNSSD_LIB="%B_BONJOUR%\Lib\%BUILD_ARCH%\dnssd.lib" -D QT_VERSION=%B_QT_VER% ..
cmake -G "%cmake_gen%" -A %VS_ARCH% -D CMAKE_BUILD_TYPE=%B_BUILD_TYPE% -D CMAKE_PREFIX_PATH="%B_QT_FULLPATH%" -D QT_VERSION=%B_QT_VER% ..
if ERRORLEVEL 1 goto failed
cmake --build . --config %B_BUILD_TYPE%
if ERRORLEVEL 1 goto failed
if exist bin\Debug (
    copy %B_QT_FULLPATH%\bin\Qt5Cored.dll bin\Debug\ > NUL
    copy %B_QT_FULLPATH%\bin\Qt5Guid.dll bin\Debug\ > NUL
    copy %B_QT_FULLPATH%\bin\Qt5Networkd.dll bin\Debug\ > NUL
    copy %B_QT_FULLPATH%\bin\Qt5Widgetsd.dll bin\Debug\ > NUL
    copy %B_QT_FULLPATH%\bin\Qt5Cored.dll bin\Debug\ > NUL
    copy ..\ext\openssl\windows\x64\bin\* bin\Debug\ > NUL
    copy ..\res\openssl\barrier.conf bin\Debug\ > NUL
    mkdir bin\Debug\platforms
    copy %B_QT_FULLPATH%\plugins\platforms\qwindowsd.dll bin\Debug\platforms\ > NUL
) else if exist bin\Release (
    copy %B_QT_FULLPATH%\bin\Qt5Core.dll bin\Release\ > NUL
    copy %B_QT_FULLPATH%\bin\Qt5Gui.dll bin\Release\ > NUL
    copy %B_QT_FULLPATH%\bin\Qt5Network.dll bin\Release\ > NUL
    copy %B_QT_FULLPATH%\bin\Qt5Widgets.dll bin\Release\ > NUL
    copy %B_QT_FULLPATH%\bin\Qt5Core.dll bin\Release\ > NUL
    copy ..\ext\openssl\windows\x64\bin\* bin\Release\ > NUL
    copy ..\res\openssl\barrier.conf bin\Release\ > NUL
    mkdir bin\Release\platforms
    copy %B_QT_FULLPATH%\plugins\platforms\qwindows.dll bin\Release\platforms\ > NUL
) else (
    echo Remember to copy supporting binaries and configuration files!
)

echo Build completed successfully
set BUILD_FAILED=0
goto done

:failed
set BUILD_FAILED=%ERRORLEVEL%
echo Build failed

:done
cd /d %savedir%

set B_BUILD_TYPE=
set B_QT_ROOT=
set B_QT_VER=
set B_QT_MSVC=
set B_BONJOUR=
set BONJOUR_SDK_HOME=
set B_QT_FULLPATH=
set savedir=
set cmake_gen=

EXIT /B %BUILD_FAILED%
