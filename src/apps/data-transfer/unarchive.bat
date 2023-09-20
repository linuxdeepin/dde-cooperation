@echo off
set zipFilePath=%1
set extractPath=%2
mkdir "%extractPath%"
tar -xzvf "%zipFilePath%" -C "%extractPath%"
