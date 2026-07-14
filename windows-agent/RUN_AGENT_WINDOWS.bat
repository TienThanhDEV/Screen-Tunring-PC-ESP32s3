@echo off
cd /d "%~dp0"
if exist "DIST\PCScreen-Windows-Agent.exe" (
  start "" "DIST\PCScreen-Windows-Agent.exe"
  exit /b 0
)
echo Chưa có file EXE. Đang mở trình build...
call "BUILD_EXE_WINDOWS.bat"
