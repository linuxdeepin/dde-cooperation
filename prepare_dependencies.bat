@echo off
REM This script downloads the required dependencies for building Windows installers
REM Usage: prepare_dependencies.bat [x64|x86|both]

set DEPS_DIR=%~dp03rdparty\ext\BonjourSDK
REM Use the same URLs as in dist\inno\scripts\products\vcredist2015-2022.iss
set VCREDIST_URL_X64=http://download.visualstudio.microsoft.com/download/pr/a061be25-c14a-489a-8c7c-bb72adfb3cab/4DFE83C91124CD542F4222FE2C396CABEAC617BB6F59BDCBDF89FD6F0DF0A32F/VC_redist.x64.exe
set VCREDIST_URL_X86=http://download.visualstudio.microsoft.com/download/pr/a061be25-c14a-489a-8c7c-bb72adfb3cab/C61CEF97487536E766130FA8714DD1B4143F6738BFB71806018EEE1B5FE6F057/VC_redist.x86.exe
REM win-airplay.zip contains Bonjour.msi and Bonjour64.msi
set BONJOUR_URL=https://github.com/Hambber/scrcpy-airplay/blob/master/win-airplay.zip

set BUILD_ARCH=%~1
if "%BUILD_ARCH%"=="" set BUILD_ARCH=both

echo.
echo ========================================
echo   Windows Installer Dependencies
echo ========================================
echo.

REM Create directories
if not exist "%DEPS_DIR%" mkdir "%DEPS_DIR%"

REM Download VC++ Redistributables
if "%BUILD_ARCH%"=="x64" goto x64_only
if "%BUILD_ARCH%"=="both" (
    echo.
    echo [1/3] Downloading 64-bit VC++ Redistributable...
    if not exist "%DEPS_DIR%\vc_redist.x64.exe" (
        powershell -Command "& {Invoke-WebRequest -Uri '%VCREDIST_URL_X64%' -OutFile '%DEPS_DIR%\vc_redist.x64.exe'}"
        if errorlevel 1 (
            echo Failed to download vc_redist.x64.exe
        ) else (
            echo Downloaded: %DEPS_DIR%\vc_redist.x64.exe
        )
    ) else (
        echo Already exists: %DEPS_DIR%\vc_redist.x64.exe
    )
)

:x64_only
if "%BUILD_ARCH%"=="x86" goto x86_only

REM Download 32-bit VC++ Redistributable
if "%BUILD_ARCH%"=="x86" goto x86_only
if "%BUILD_ARCH%"=="both" (
    echo.
    echo [2/3] Downloading 32-bit VC++ Redistributable...
    if not exist "%DEPS_DIR%\vc_redist.x86.exe" (
        powershell -Command "& {Invoke-WebRequest -Uri '%VCREDIST_URL_X86%' -OutFile '%DEPS_DIR%\vc_redist.x86.exe'}"
        if errorlevel 1 (
            echo Failed to download vc_redist.x86.exe
        ) else (
            echo Downloaded: %DEPS_DIR%\vc_redist.x86.exe
        )
    ) else (
        echo Already exists: %DEPS_DIR%\vc_redist.x86.exe
    )
)

:x86_only
if "%BUILD_ARCH%"=="x64" goto download_bonjour

REM Download 32-bit VC++ Redistributable for x86-only build
if "%BUILD_ARCH%"=="x86" (
    echo.
    echo [1/2] Downloading 32-bit VC++ Redistributable...
    if not exist "%DEPS_DIR%\vc_redist.x86.exe" (
        powershell -Command "& {Invoke-WebRequest -Uri '%VCREDIST_URL_X86%' -OutFile '%DEPS_DIR%\vc_redist.x86.exe'}"
        if errorlevel 1 (
            echo Failed to download vc_redist.x86.exe
        ) else (
            echo Downloaded: %DEPS_DIR%\vc_redist.x86.exe
        )
    ) else (
        echo Already exists: %DEPS_DIR%\vc_redist.x86.exe
    )
)

:download_bonjour
REM Download Bonjour (universal for both architectures)
if "%BUILD_ARCH%"=="both" (
    set STEP_NUM=3/3
) else (
    set STEP_NUM=2/2
)

echo.
echo [%STEP_NUM%] Downloading Bonjour...
if not exist "%DEPS_DIR%\win-airplay.zip" (
    powershell -Command "& {Invoke-WebRequest -Uri '%BONJOUR_URL%' -OutFile '%DEPS_DIR%\win-airplay.zip'}"
    if errorlevel 1 (
        echo Failed to download win-airplay.zip
        echo You can download it manually from: %BONJOUR_URL%
    ) else (
        echo Downloaded: %DEPS_DIR%\win-airplay.zip
    )
) else (
    echo Already exists: %DEPS_DIR%\win-airplay.zip
)

:done
echo.
echo ========================================
echo   Dependency Download Complete
echo ========================================
echo.
echo Dependencies directory: %DEPS_DIR%
echo.
echo Contents:
dir /B "%DEPS_DIR%\*.exe" "%DEPS_DIR%\*.msi" 2>nul
echo.
