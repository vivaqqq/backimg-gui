@echo off
rem Build backimg GUI v1.3.16 (pure C + Win32)
rem Requires: MinGW-w64 (gcc + windres) in PATH
setlocal
set VER=1.3.29
cd /d "%~dp0"
windres -O coff resource.rc -o resource.res || goto :err
gcc -O2 -mwindows -municode -std=c11 -Wall main.c resource.res -o "backimg-gui_en_v%VER%.exe" -lgdiplus -lcomctl32 -luxtheme -ldwmapi -lshlwapi -lole32 || goto :err
echo Build OK: backimg-gui_en_v%VER%.exe
exit /b 0
:err
echo Build FAILED
exit /b 1
