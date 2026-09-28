@echo off
setlocal

set "VS_INSTALLER=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer"
if not exist "%VS_INSTALLER%\vswhere.exe" (
    echo with_msvc: vswhere.exe not found, is Visual Studio installed? 1>&2
    exit /b 1
)

for /f "usebackq delims=" %%i in (`"%VS_INSTALLER%\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VS_PATH=%%i"
if not defined VS_PATH (
    echo with_msvc: no Visual Studio with the C++ x64 toolset found 1>&2
    exit /b 1
)

rem vcvars64 needs vswhere on PATH and may report errors from optional
rem extensions while still setting up a complete environment, so its exit
rem code is ignored and cl.exe is checked directly instead.
set "PATH=%VS_INSTALLER%;%PATH%"
call "%VS_PATH%\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
where cl >nul 2>&1 || (
    echo with_msvc: cl.exe is not available after vcvars64.bat 1>&2
    exit /b 1
)

%*
exit /b %ERRORLEVEL%
