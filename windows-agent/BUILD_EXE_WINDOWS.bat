@echo off
set "ROOT=%~dp0"
echo Dang mo PCScreen Windows Builder...
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -NoExit -File "%ROOT%BUILD_EXE_WINDOWS.ps1"
if errorlevel 1 (
  echo.
  echo Khong mo duoc PowerShell Builder.
  echo Hay bam chuot phai BUILD_EXE_WINDOWS.ps1 va chon Run with PowerShell.
  pause
)
