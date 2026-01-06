@echo off

if "%~1"=="" (
    echo please set the app version same as in deiban/changelog
    exit /B 1
)

set APP_VERSION=%~1
echo set APP_VERSION: %APP_VERSION%

REM Set architecture (default to x64 for backward compatibility)
set BUILD_ARCH=%~2
if "%BUILD_ARCH%"=="" set BUILD_ARCH=x64
echo Build architecture: %BUILD_ARCH%

REM Force use of the same compiler as used to build ChimeraX
@REM call "%VS170COMNTOOLS%"\vcvars64.bat

set VCINSTALLDIR=C:\Program Files\Microsoft Visual Studio\2022\Community\VC
echo VCINSTALLDIR: %VCINSTALLDIR%

REM Choose VS environment based on architecture
call "%VCINSTALLDIR%\Auxiliary\Build\vcvarsall.bat" %BUILD_ARCH%

@REM projects
set COO_PROJECT=dde-cooperation
set DT_PROJECT=data-transfer

REM defaults - override them by creating a build_env.bat file
set B_BUILD_TYPE=Release
set B_QT_ROOT=D:\Qt
set B_QT_VER=5.15.2

REM Set Qt MSVC version and OpenSSL based on architecture
if "%BUILD_ARCH%"=="x86" (
    set VS_ARCH=Win32
    set B_QT_MSVC=msvc2019
    set "OPENSSL_ROOT_DIR=C:\Program Files (x86)\OpenSSL-Win32"
) else (
    set VS_ARCH=x64
    set B_QT_MSVC=msvc2019_64
    set "OPENSSL_ROOT_DIR=C:\Program Files\OpenSSL-Win64"
)
set B_BONJOUR=%~dp0\3rdparty\ext\BonjourSDK

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

REM full path to Qt stuff we need
set B_QT_FULLPATH=%B_QT_ROOT%\%B_QT_VER%\%B_QT_MSVC%

echo Qt: %B_QT_FULLPATH%

rmdir /q /s build
mkdir build
if ERRORLEVEL 1 goto failed
cd build
mkdir installer-inno

echo ------------starting cmake------------
REM Set vcpkg triplet based on build architecture
if "%BUILD_ARCH%"=="x86" (
    set VCPKG_TRIPLET=x86-windows
) else (
    set VCPKG_TRIPLET=x64-windows
)
echo Using vcpkg triplet: %VCPKG_TRIPLET%

REM Pre-install vcpkg dependencies (optional but speeds up build)
echo Pre-installing vcpkg dependencies...
set VCPKG_ROOT=D:\vcpkg\vcpkg

set VCPKG_CMAKE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake

echo Configuring with CMake...

cmake -G "%cmake_gen%" -A %VS_ARCH% -D CMAKE_BUILD_TYPE=%B_BUILD_TYPE% ^
    -D CMAKE_PREFIX_PATH="%B_QT_FULLPATH%" -D QT_VERSION=%B_QT_VER% ^
    -D CMAKE_TOOLCHAIN_FILE="%VCPKG_CMAKE%" ^
    -D VCPKG_TARGET_TRIPLET=%VCPKG_TRIPLET% ^
    -D APP_VERSION=%APP_VERSION% ..

if ERRORLEVEL 1 goto failed
cmake --build . --config %B_BUILD_TYPE%
if ERRORLEVEL 1 goto failed

REM Set architecture-specific variables
if "%BUILD_ARCH%"=="x86" (
    set OPENSSL_CRYPTO=libcrypto-3.dll
    set OPENSSL_SSL=libssl-3.dll
    set BONJOUR_FILE=Bonjour.msi
) else (
    set OPENSSL_CRYPTO=libcrypto-3-x64.dll
    set OPENSSL_SSL=libssl-3-x64.dll
    set BONJOUR_FILE=Bonjour64.msi
)

echo OpenSSL DLLs: %OPENSSL_CRYPTO%, %OPENSSL_SSL%

if exist output\%B_BUILD_TYPE% (
    copy output\%B_BUILD_TYPE%\* output\%COO_PROJECT%\%B_BUILD_TYPE%\ > NUL
    del output\%COO_PROJECT%\%B_BUILD_TYPE%\quazip* > NUL
    copy "%OPENSSL_ROOT_DIR%\%OPENSSL_CRYPTO%" output\%COO_PROJECT%\%B_BUILD_TYPE%\ > NUL
    copy "%OPENSSL_ROOT_DIR%\%OPENSSL_SSL%" output\%COO_PROJECT%\%B_BUILD_TYPE%\ > NUL
    copy "%B_BONJOUR%\%BONJOUR_FILE%" output\%COO_PROJECT%\%B_BUILD_TYPE%\ > NUL

    copy output\%B_BUILD_TYPE%\quazip* output\%DT_PROJECT%\%B_BUILD_TYPE%\ > NUL

    copy "%OPENSSL_ROOT_DIR%\%OPENSSL_CRYPTO%" output\%DT_PROJECT%\%B_BUILD_TYPE%\ > NUL
    copy "%OPENSSL_ROOT_DIR%\%OPENSSL_SSL%" output\%DT_PROJECT%\%B_BUILD_TYPE%\ > NUL
) else (
    echo Remember to copy supporting binaries and configuration files!
)

echo Build completed successfully

set INNO_ROOT=C:\Program Files (x86)\Inno Setup 6

echo Building %BUILD_ARCH%-bit Windows installer...

"%INNO_ROOT%\ISCC.exe" /Qp %COO_PROJECT%-setup.iss
if ERRORLEVEL 1 goto issfailed
"%INNO_ROOT%\ISCC.exe" /Qp deepin-%DT_PROJECT%-setup.iss
if ERRORLEVEL 1 goto issfailed

echo Build all Windows installer successfully!!!

set BUILD_FAILED=0
goto done

:issfailed
echo Make Windows installer failed

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
set B_QT_FULLPATH=
set savedir=
set cmake_gen=

set INNO_ROOT=

EXIT /B %BUILD_FAILED%
