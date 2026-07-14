$ErrorActionPreference = "Stop"
if (Test-Path variable:PSNativeCommandUseErrorActionPreference) {
    $PSNativeCommandUseErrorActionPreference = $false
}
$Host.UI.RawUI.WindowTitle = "PCScreen Windows Agent Builder"
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$project = Join-Path $root "PCScreen.WindowsAgent\PCScreen.WindowsAgent.csproj"
$dist = Join-Path $root "DIST"
$log = Join-Path $root "BUILD-WINDOWS.log"

function Stop-Build([string]$message) {
    Write-Host ""
    Write-Host "BUILD KHONG THANH CONG" -ForegroundColor Red
    Write-Host $message -ForegroundColor Yellow
    Write-Host "Chi tiet da luu tai: $log" -ForegroundColor Gray
    Write-Host ""
    Read-Host "Nhan Enter de ket thuc"
    return
}

Clear-Host
Write-Host "============================================================" -ForegroundColor DarkGray
Write-Host " PCScreen Windows Agent - Tao file EXE win-x64" -ForegroundColor White
Write-Host "============================================================" -ForegroundColor DarkGray
Write-Host "Thu muc: $root"
"PCScreen build started: $(Get-Date -Format o)" | Set-Content -Path $log -Encoding UTF8

if (-not (Test-Path $project)) {
    Stop-Build "Khong tim thay project. Hay giai nen TOAN BO file ZIP roi moi chay BUILD_EXE_WINDOWS.bat."
    return
}

$dotnet = Get-Command dotnet -ErrorAction SilentlyContinue
if (-not $dotnet) {
    Write-Host ""
    Write-Host "Chua cai .NET 8 SDK x64." -ForegroundColor Yellow
    Write-Host "Trinh duyet se mo trang tai .NET 8 SDK chinh thuc."
    Start-Process "https://dotnet.microsoft.com/download/dotnet/8.0"
    Stop-Build "Cai .NET 8 SDK x64, khoi dong lai Windows neu can, sau do chay lai file build."
    return
}

try {
    $sdks = & dotnet --list-sdks 2>&1
    $sdks | Add-Content -Path $log -Encoding UTF8
    Write-Host ""
    Write-Host "Da tim thay .NET SDK:" -ForegroundColor Green
    $sdks | ForEach-Object { Write-Host "  $_" }

    New-Item -ItemType Directory -Force -Path $dist | Out-Null

    Write-Host ""
    Write-Host "[1/2] Dang tai thu vien va khoi phuc project..." -ForegroundColor Cyan
    $ErrorActionPreference = "Continue"
    & dotnet restore $project 2>&1 | Tee-Object -FilePath $log -Append
    $restoreExitCode = $LASTEXITCODE
    $ErrorActionPreference = "Stop"
    if ($restoreExitCode -ne 0) {
        Stop-Build "dotnet restore bi loi. Kiem tra Internet, proxy, antivirus va noi dung log."
        return
    }

    Write-Host ""
    Write-Host "[2/2] Dang dong goi EXE tu chua..." -ForegroundColor Cyan
    $ErrorActionPreference = "Continue"
    & dotnet publish $project `
        -c Release `
        -r win-x64 `
        --self-contained true `
        -p:PublishSingleFile=true `
        -p:IncludeNativeLibrariesForSelfExtract=true `
        -p:IncludeAllContentForSelfExtract=true `
        -p:DebugType=None `
        -p:DebugSymbols=false `
        -o $dist 2>&1 | Tee-Object -FilePath $log -Append
    $publishExitCode = $LASTEXITCODE
    $ErrorActionPreference = "Stop"
    if ($publishExitCode -ne 0) {
        Stop-Build "dotnet publish bi loi. Gui file BUILD-WINDOWS.log de kiem tra."
        return
    }

    $exe = Join-Path $dist "PCScreen-Windows-Agent.exe"
    if (-not (Test-Path $exe)) {
        Stop-Build "Lenh publish ket thuc nhung khong tim thay PCScreen-Windows-Agent.exe."
        return
    }

    $sizeMb = [Math]::Round((Get-Item $exe).Length / 1MB, 1)
    Write-Host ""
    Write-Host "BUILD THANH CONG" -ForegroundColor Green
    Write-Host "File: $exe" -ForegroundColor White
    Write-Host "Dung luong: $sizeMb MB" -ForegroundColor Gray
    "Build succeeded: $exe ($sizeMb MB)" | Add-Content -Path $log -Encoding UTF8
    Start-Process explorer.exe -ArgumentList "/select,`"$exe`""
    Write-Host ""
    Read-Host "Nhan Enter de chay PCScreen Windows Agent"
    Start-Process $exe
}
catch {
    $_ | Out-String | Add-Content -Path $log -Encoding UTF8
    Stop-Build $_.Exception.Message
}
