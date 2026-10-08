@echo off
setlocal
cd /d "%~dp0"
set "AUDIO_VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%AUDIO_VSWHERE%" (
  echo Visual Studio Installer not found. Install Visual Studio or Build Tools with Desktop development with C++.
  exit /b 1
)
for /f "usebackq tokens=*" %%i in (`"%AUDIO_VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "AUDIO_VSROOT=%%i"
if not defined AUDIO_VSROOT (
  echo No Visual Studio C++ toolchain found.
  exit /b 1
)
call "%AUDIO_VSROOT%\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul
if errorlevel 1 exit /b 1
if not exist build mkdir build
cl /nologo /O1 /MT /W4 /utf-8 /EHsc /DWIN32_LEAN_AND_MEAN src\AudioSwitch.cpp /Fo:build\AudioSwitch.obj /Fe:build\AudioSwitch.exe /link /SUBSYSTEM:WINDOWS ole32.lib user32.lib shell32.lib uuid.lib advapi32.lib
if errorlevel 1 exit /b 1
echo Built build\AudioSwitch.exe
endlocal
