@echo off
echo ========================================
echo Cloud Alert System - 邮件测试程序编译
echo ========================================
echo.

REM 查找 Visual Studio 安装路径
set "VS_PATH="
for /f "usebackq tokens=*" %%i in (`"dir /b /s "%ProgramFiles%\Microsoft Visual Studio\*\*\VC\Auxiliary\Build\vcvars64.bat" 2^>nul"`) do (
    set "VS_PATH=%%i"
    goto :found_vs
)

echo 错误: 未找到 Visual Studio
pause
exit /b 1

:found_vs
echo 找到 Visual Studio: %VS_PATH%
echo.

REM 设置环境变量
call "%VS_PATH%"

REM 编译
cl /EHsc /std:c++17 /I"..\Server" EmailTest.cpp /link /LIBPATH:"..\bin\x64\Debug" /OUT:EmailTest.exe

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ========================================
    echo ✅ 编译成功！
    echo ========================================
    echo.
    echo 复制文件到 bin 目录...
    copy /Y EmailTest.exe "..\bin\x64\Debug\" >nul
    echo.
    echo 运行测试...
    cd "..\bin\x64\Debug"
    EmailTest.exe
) else (
    echo.
    echo ========================================
    echo ❌ 编译失败！
    echo ========================================
)

pause
