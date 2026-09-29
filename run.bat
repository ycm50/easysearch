@echo off
set "QT_PATH=A:\msys2\ucrt64\bin"
set "EXE_DIR=%~dp0build"
set "PATH=%QT_PATH%;%EXE_DIR%;%PATH%"
start "" "%EXE_DIR%\easysearch.exe"
