@echo off
setlocal EnableExtensions EnableDelayedExpansion
title PCScreen 1.7.0 - ESP32-S3 4MB
set "DIR=%~dp0"
set "IMAGE=%DIR%PCScreen-S3-4MB-v1.7.0-FULL.bin"
set "ESPTOOL=%DIR%portable_esptool\esptool.py"
where py >nul 2>nul && (set "PY=py -3") || (set "PY=python")
set "PYTHONPATH=%DIR%portable_esptool"
echo ======================================================
echo  PCScreen 1.7.0 - ESP32-S3 Super Mini 4 MB
echo ======================================================
%PY% -c "import serial.tools.list_ports as p; [print(x.device+' - '+x.description) for x in p.comports()]"
set /p "PORT=Enter the ESP32-S3 port, for example COM5: "
%PY% "%ESPTOOL%" --chip esp32s3 --port "%PORT%" --baud 115200 flash_id > "%DIR%flash-windows.log" 2>&1
type "%DIR%flash-windows.log"
findstr /C:"Detected flash size: 4MB" "%DIR%flash-windows.log" >nul || (echo Could not confirm 4 MB flash.& pause & exit /b 2)
echo A full USB installation erases saved Wi-Fi and settings.
set /p "ANSWER=Type YES to continue: "
if /I not "%ANSWER%"=="YES" exit /b 0
%PY% "%ESPTOOL%" --chip esp32s3 --port "%PORT%" --baud 115200 erase_flash || goto failed
set "OK=0"
for /L %%A in (1,1,3) do (
  echo Flash attempt %%A/3 at stable 115200 baud...
  %PY% "%ESPTOOL%" --chip esp32s3 --port "%PORT%" --baud 115200 write_flash --flash_size 4MB 0x0 "%IMAGE%" && set "OK=1" && goto flashed
  timeout /t 3 /nobreak >nul
)
goto failed
:flashed
if not "!OK!"=="1" goto failed
echo FLASH COMPLETED. Press RESET and scan the Wi-Fi QR code shown on the TFT.
echo Setup Wi-Fi: PCScreen-Setup / pcscreen123
echo Open http://192.168.4.1
pause
exit /b 0
:failed
echo Flash failed. See flash-windows.log and try another USB data cable or port.
pause
exit /b 4
