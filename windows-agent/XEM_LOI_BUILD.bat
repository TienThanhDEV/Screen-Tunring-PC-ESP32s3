@echo off
cd /d "%~dp0"
if not exist "BUILD-WINDOWS.log" (
  echo Chua co file BUILD-WINDOWS.log.
  echo Hay chay BUILD_EXE_WINDOWS.bat truoc.
  pause
  exit /b 1
)
start "" notepad.exe "%CD%\BUILD-WINDOWS.log"
