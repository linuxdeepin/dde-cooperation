@echo off
set sourcePath=%1
set zipFilePath=%2

tar -czvf "%zipFilePath%" -C "%sourcePath%" .
