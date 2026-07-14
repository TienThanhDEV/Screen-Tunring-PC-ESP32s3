@echo off
setlocal
set "ROOT=%~dp0..\.."
title PCScreen v1.7.0 - Windows Setup
echo PCScreen v1.7.0 - Windows
echo 1. Flash FULL firmware over USB
echo 2. Build or open PCScreen Windows Agent
set /p "ANSWER=Type YES to flash firmware, or press Enter if it is already installed: "
if /I "%ANSWER%"=="YES" call "%ROOT%\installers\usb-flash\NAP_ESP32S3_4MB_WINDOWS.bat"
if exist "%ROOT%\windows-agent\DIST\PCScreen-Windows-Agent.exe" (
  start "" "%ROOT%\windows-agent\DIST\PCScreen-Windows-Agent.exe"
) else (
  call "%ROOT%\windows-agent\BUILD_EXE_WINDOWS.bat"
)
echo PCScreen setup finished. A clean flash shows the Wi-Fi QR setup screen.
pause
