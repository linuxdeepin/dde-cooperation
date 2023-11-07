@echo off

start "data-transfer" /B /D "%cd%" deepin-data-transfer.exe

:check_frontend
tasklist | findstr /i "deepin-data-transfer.exe" >nul
if %errorlevel% equ 0 (
    timeout /t 1 /nobreak >nul
    goto check_frontend
)
taskkill /IM cooperation-daemon.exe -F
@REM taskkill /IM /t deepin-data-transfer.exe -F

echo ALL programs have exited
