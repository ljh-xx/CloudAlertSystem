@echo off
setlocal EnableExtensions

cd /d "%~dp0"

set "CONFIG=Debug"
set "PLATFORM=x64"
set "SOLUTION=CloudAlertSystem.sln"
set "MFC_PROJECT=MFCClient\MFCClient.vcxproj"
set "TOOLSET_PROP="

if not "%~1"=="" (
    if /I "%~1"=="Debug" (
        set "CONFIG=Debug"
        if not "%~2"=="" set "TOOLSET_PROP=/p:PlatformToolset=%~2"
    ) else if /I "%~1"=="Release" (
        set "CONFIG=Release"
        if not "%~2"=="" set "TOOLSET_PROP=/p:PlatformToolset=%~2"
    ) else (
        set "TOOLSET_PROP=/p:PlatformToolset=%~1"
    )
)

echo.
echo ========================================
echo CloudAlertSystem one-click build
echo Configuration: %CONFIG%^|%PLATFORM%
if defined TOOLSET_PROP echo Toolset override: %TOOLSET_PROP%
echo ========================================
echo.

set "MSBUILD="

where msbuild.exe >nul 2>nul
if not errorlevel 1 (
    for /f "delims=" %%I in ('where msbuild.exe') do (
        if not defined MSBUILD set "MSBUILD=%%I"
    )
)

if not defined MSBUILD (
    set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
    if exist "%VSWHERE%" (
        for /f "usebackq delims=" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe`) do (
            if not defined MSBUILD set "MSBUILD=%%I"
        )
    )
)

if not defined MSBUILD if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" set "MSBUILD=%ProgramFiles%\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
if not defined MSBUILD if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe" set "MSBUILD=%ProgramFiles%\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe"
if not defined MSBUILD if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe" set "MSBUILD=%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe"
if not defined MSBUILD if exist "%ProgramFiles%\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe" set "MSBUILD=%ProgramFiles%\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe"
if not defined MSBUILD if exist "%ProgramFiles%\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" set "MSBUILD=%ProgramFiles%\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe"
if not defined MSBUILD if exist "%ProgramFiles%\Microsoft Visual Studio\18\Professional\MSBuild\Current\Bin\MSBuild.exe" set "MSBUILD=%ProgramFiles%\Microsoft Visual Studio\18\Professional\MSBuild\Current\Bin\MSBuild.exe"
if not defined MSBUILD if exist "%ProgramFiles%\Microsoft Visual Studio\18\Enterprise\MSBuild\Current\Bin\MSBuild.exe" set "MSBUILD=%ProgramFiles%\Microsoft Visual Studio\18\Enterprise\MSBuild\Current\Bin\MSBuild.exe"
if not defined MSBUILD if exist "%ProgramFiles%\Microsoft Visual Studio\18\BuildTools\MSBuild\Current\Bin\MSBuild.exe" set "MSBUILD=%ProgramFiles%\Microsoft Visual Studio\18\BuildTools\MSBuild\Current\Bin\MSBuild.exe"

if not defined MSBUILD (
    echo [ERROR] MSBuild.exe was not found.
    echo Install Visual Studio 2022 with "Desktop development with C++", then run this file again.
    echo.
    pause
    exit /b 1
)

echo Using MSBuild:
echo "%MSBUILD%"
echo.

echo [1/2] Building Server and console Client from %SOLUTION% ...
"%MSBUILD%" "%SOLUTION%" /m /restore /t:Build /p:Configuration=%CONFIG% /p:Platform=%PLATFORM% %TOOLSET_PROP%
if errorlevel 1 (
    echo.
    echo [ERROR] Solution build failed.
    echo If the error mentions PlatformToolset, try:
    echo   build.bat %CONFIG% v143
    echo or:
    echo   build.bat v143
    echo.
    pause
    exit /b 1
)

echo.
echo [2/2] Building MFC graphical client ...
"%MSBUILD%" "%MFC_PROJECT%" /m /restore /t:Build /p:Configuration=%CONFIG% /p:Platform=%PLATFORM% %TOOLSET_PROP%
if errorlevel 1 (
    echo.
    echo [ERROR] MFC client build failed.
    echo Make sure the MFC component is installed in Visual Studio Installer.
    echo If the error mentions PlatformToolset, try:
    echo   build.bat %CONFIG% v143
    echo or:
    echo   build.bat v143
    echo.
    pause
    exit /b 1
)

echo.
echo ========================================
echo Build succeeded.
echo Output directory:
echo %CD%\bin\%PLATFORM%\%CONFIG%
echo.
echo Run order:
echo   1. bin\%PLATFORM%\%CONFIG%\Server.exe
echo   2. bin\%PLATFORM%\%CONFIG%\MFCClient.exe
echo      or bin\%PLATFORM%\%CONFIG%\Client.exe
echo ========================================
echo.

pause
exit /b 0
