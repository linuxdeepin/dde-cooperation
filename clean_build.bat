@echo off


REM Force use of the same compiler as used to build ChimeraX
@REM call "%VS170COMNTOOLS%"\vcvars64.bat

set VCINSTALLDIR=C:\Program Files\Microsoft Visual Studio\2022\Community\VC
echo VCINSTALLDIR: %VCINSTALLDIR%
call "%VCINSTALLDIR%\Auxiliary\Build\vcvars64.bat"

REM defaults - override them by creating a build_env.bat file
set B_BUILD_TYPE=Release
set B_QT_ROOT=C:\Qt
set B_QT_VER=5.15.2
set B_QT_MSVC=msvc2019_64
set B_BONJOUR=3rdparty\ext\BonjourSDKLike

if "%OPENSSL_ROOT_DIR%"=="" (
    set OPENSSL_ROOT_DIR=C:\Program Files\OpenSSL-Win64
)

set savedir=%cd%
cd /d %~dp0

REM cmake generator name for the target build system
if "%VisualStudioVersion%"=="15.0" (
    set cmake_gen=Visual Studio 15 2017
) else if "%VisualStudioVersion%"=="16.0" (
    set cmake_gen=Visual Studio 16 2019
) else (
    echo Visual Studio version was not detected: %VisualStudioVersion%
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

echo ------------starting cmake------------

cmake -G "%cmake_gen%" -A x64 -D CMAKE_BUILD_TYPE=%B_BUILD_TYPE% -D CMAKE_PREFIX_PATH="%B_QT_FULLPATH%" -D DNSSD_LIB="%B_BONJOUR%\Lib\x64\dnssd.lib" -D QT_VERSION=%B_QT_VER% ..
if ERRORLEVEL 1 goto failed
cmake --build . --config %B_BUILD_TYPE%
if ERRORLEVEL 1 goto failed
if exist output\Debug (
    copy output\Debug\QtZeroConf.dll output\dde-cooperation\Debug\ > NUL
    copy "%OPENSSL_ROOT_DIR%\libcrypto-1_1-x64.dll" output\dde-cooperation\Debug\ > NUL
    copy "%OPENSSL_ROOT_DIR%\libssl-1_1-x64.dll" output\dde-cooperation\Debug\ > NUL
) else if exist output\Release (
    copy output\Release\QtZeroConf.dll output\dde-cooperation\Release\ > NUL
    copy "%OPENSSL_ROOT_DIR%\libcrypto-1_1-x64.dll" output\dde-cooperation\Release\ > NUL
    copy "%OPENSSL_ROOT_DIR%\libssl-1_1-x64.dll" output\dde-cooperation\Release\ > NUL
) else (
    echo Remember to copy supporting binaries and configuration files!
)

echo Build completed successfully

@REM echo ------------cmake again forgenerate sources------------
@REM cmake -G "%cmake_gen%" -A x64 -D CMAKE_BUILD_TYPE=%B_BUILD_TYPE% -D CMAKE_PREFIX_PATH="%B_QT_FULLPATH%" -D DNSSD_LIB="%B_BONJOUR%\Lib\x64\dnssd.lib" -D QT_VERSION=%B_QT_VER% ..

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
