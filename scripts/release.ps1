$ErrorActionPreference = "Stop"

$RootDir = Split-Path -Parent $PSScriptRoot
$BuildDir = Join-Path $RootDir "build/release"
$StageDir = Join-Path $RootDir "dist/stage"

cmake -S $RootDir -B $BuildDir -DCMAKE_BUILD_TYPE=Release
cmake --build $BuildDir --config Release
ctest --test-dir $BuildDir -C Release --output-on-failure

if (Test-Path $StageDir) {
    Remove-Item $StageDir -Recurse -Force
}

cmake --install $BuildDir --prefix $StageDir --config Release

$windeployqt = Get-Command windeployqt -ErrorAction SilentlyContinue
if ($windeployqt) {
    & $windeployqt.Source (Join-Path $StageDir "bin/bincalc.exe")
} else {
    Write-Warning "windeployqt not found; package will require Qt runtime DLLs to be present."
}

cmake --build $BuildDir --target package --config Release

Write-Host "staged release in $StageDir"
Write-Host "packaged artifacts in $BuildDir"
