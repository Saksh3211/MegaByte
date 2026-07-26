$ErrorActionPreference = "Stop"
$projectRoot = $PSScriptRoot

Write-Host "Configuring with CMake..." -ForegroundColor Cyan
cmake -B build -S $projectRoot
if ($LASTEXITCODE -ne 0) { Write-Host "CMake configure failed." -ForegroundColor Red; exit 1 }

Write-Host "Building..." -ForegroundColor Cyan
cmake --build build --config Release
if ($LASTEXITCODE -ne 0) { Write-Host "Build failed." -ForegroundColor Red; exit 1 }

$exePath = Join-Path $projectRoot "build\Release\megabyte_node.exe"
if (-not (Test-Path $exePath)) { $exePath = Join-Path $projectRoot "build\megabyte_node.exe" }
& $exePath