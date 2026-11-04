@echo off
rem ============================================================================
rem  ExplorerBgTool.dll backimg v1.3.17
rem
rem  MinGW-w64 (your MinGW-w64 install path)
rem
rem     build.cmd                 g++ PATH
rem     build.cmd D:\path\mingw64\bin    MinGW  bin
rem   build\ExplorerBgTool.dll
rem ============================================================================
setlocal
cd /d "%~dp0"

set "BIN=%~1"
if "%BIN%"=="" set "BIN="
set "GXX=%BIN%\g++.exe"

if not exist "%GXX%" (
    echo [??] ??? %GXX%
    echo        ??: build.cmd [MinGW?bin??]
    exit /b 1
)

if not exist "..\build" mkdir "..\build"

echo [1/1] ?? ExplorerBgTool.dll ...
"%GXX%" -std=c++17 -O2 -shared -DUNICODE -D_UNICODE -Wall ^
    -Wl,--image-base,0x10000000 ^
    -o "..\build\ExplorerBgTool.dll" ^
    dllmain.cpp ShellLoader.cpp WinAPI.cpp ^
    minihook/hook.cpp minihook/buffer.cpp minihook/trampoline.cpp ^
    minihook/hde/hde64.cpp minihook/hde/hde32.cpp ^
    Source.def ^
    -lgdiplus -lmsimg32 -lshlwapi -lgdi32 -luser32 -lole32 -luuid -static

if errorlevel 1 (
    echo [??] ?????????????
    exit /b 1
)

echo.
echo [??] ..\build\ExplorerBgTool.dll
dir "..\build\ExplorerBgTool.dll" | findstr "ExplorerBgTool"
exit /b 0
