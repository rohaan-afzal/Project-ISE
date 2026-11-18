param(
    [string]$CMakeCommand = "cmake",
    [int]$Jobs = 4
)

$workspace = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$source = Join-Path $workspace "deps/opencv-4.11.0"
$build = Join-Path $workspace "deps/opencv-build"
$install = Join-Path $workspace "deps/opencv-install"
$resolvedCMake = Get-Command $CMakeCommand -ErrorAction SilentlyContinue
if (-not $resolvedCMake) {
    $qtCMake = "C:\Qt\Tools\CMake_64\bin\cmake.exe"
    if (Test-Path $qtCMake) { $CMakeCommand = $qtCMake }
    else { throw "CMake was not found. Pass its path with -CMakeCommand." }
} else {
    $CMakeCommand = $resolvedCMake.Source
}

# Keep MinGW's compiler helper DLLs available to cc1plus/as in a plain PowerShell session.
$cacheFile = Join-Path $build "CMakeCache.txt"
if (Test-Path $cacheFile) {
    $compilerEntry = Select-String -Path $cacheFile -Pattern '^CMAKE_CXX_COMPILER:STRING=(.+)$' |
        Select-Object -First 1
    if ($compilerEntry) {
        $mingwBin = Split-Path $compilerEntry.Matches[0].Groups[1].Value
        $targetBin = Join-Path (Split-Path $mingwBin) "x86_64-w64-mingw32/bin"
        $env:PATH = "$mingwBin;$targetBin;$env:PATH"
    }
}
$compilerTemp = Join-Path $workspace "build/compiler-temp"
New-Item -ItemType Directory -Path $compilerTemp -Force | Out-Null
$env:TEMP = $compilerTemp
$env:TMP = $compilerTemp

& $CMakeCommand -S $source -B $build `
    "-DBUILD_LIST:STRING=core,imgproc,imgcodecs,videoio,objdetect" `
    -DBUILD_opencv_videoio:BOOL=ON `
    -DBUILD_opencv_objdetect:BOOL=ON `
    -DWITH_DSHOW:BOOL=ON `
    "-DCMAKE_INSTALL_PREFIX:PATH=$install"
if ($LASTEXITCODE -ne 0) { throw "OpenCV configuration failed with exit code $LASTEXITCODE." }

& $CMakeCommand --build $build --target install --parallel $Jobs
if ($LASTEXITCODE -ne 0) { throw "OpenCV videoio build/install failed with exit code $LASTEXITCODE." }

$header = Join-Path $install "include/opencv2/videoio.hpp"
if (-not (Test-Path $header)) { throw "OpenCV installed without videoio.hpp: $header" }
$barcodeHeader = Join-Path $install "include/opencv2/objdetect/barcode.hpp"
if (-not (Test-Path $barcodeHeader)) { throw "OpenCV installed without barcode.hpp: $barcodeHeader" }
Write-Host "OpenCV videoio and retail barcode detection are installed. Reconfigure the Qt Creator project and rebuild the application."
