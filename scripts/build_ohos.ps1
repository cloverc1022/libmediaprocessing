param(
    [string]$OhosNativeHome = $env:OHOS_NATIVE_HOME,
    [ValidateSet("arm64-v8a", "armeabi-v7a")]
    [string]$Arch = "arm64-v8a",
    [string]$FfmpegLibrary = "",
    [switch]$Clean
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
$CmakeDir = Join-Path $Root "src\main\cpp\ohos"
$BuildDir = Join-Path $Root "build-ohos-$Arch"

function Resolve-OhosNativeHome {
    param([string]$Hint)
    $candidates = @()
    if ($Hint) { $candidates += $Hint }
    $candidates += @(
        "D:\StudioProjects\Http2p2p_API\.ohos-sdk\native",
        (Join-Path $Root ".ohos-sdk\native")
    )
    foreach ($path in $candidates) {
        $toolchain = Join-Path $path "build\cmake\ohos.toolchain.cmake"
        if (Test-Path $toolchain) { return (Resolve-Path $path).Path }
    }
    throw "OpenHarmony Native SDK not found."
}

$NativeHome = Resolve-OhosNativeHome -Hint $OhosNativeHome
$Cmake = Join-Path $NativeHome "build-tools\cmake\bin\cmake.exe"
$Ninja = Join-Path $NativeHome "build-tools\cmake\bin\ninja.exe"
$Toolchain = Join-Path $NativeHome "build\cmake\ohos.toolchain.cmake"

$FfmpegDir = Join-Path $Root "src\main\jniLibs\ohos\$Arch"
$FfmpegMono = if ($FfmpegLibrary) { $FfmpegLibrary } else { Join-Path $FfmpegDir "libffmpeg.so" }
$FfmpegSplit = Join-Path $FfmpegDir "libavcodec.so"
if (-not (Test-Path $FfmpegMono) -and -not (Test-Path $FfmpegSplit)) {
    throw "Missing OHOS FFmpeg: copy your libffmpeg.so to $FfmpegDir (recommended, same as Android)."
}

if ($Clean -and (Test-Path $BuildDir)) { Remove-Item -Recurse -Force $BuildDir }
New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null

$CmakeArgs = @(
    "-S", $CmakeDir, "-B", $BuildDir, "-G", "Ninja",
    "-DCMAKE_MAKE_PROGRAM=$Ninja",
    "-DCMAKE_TOOLCHAIN_FILE=$Toolchain",
    "-DOHOS_ARCH=$Arch",
    "-DOHOS_PLATFORM=OHOS",
    "-DOHOS_STL=c++_shared",
    "-DCMAKE_BUILD_TYPE=Release"
)
if ($FfmpegLibrary -and (Test-Path $FfmpegLibrary)) {
    $CmakeArgs += "-DFFMPEG_LIBRARY=$((Resolve-Path $FfmpegLibrary).Path)"
}
& $Cmake @CmakeArgs
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& $Cmake --build $BuildDir
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$OutSo = Join-Path $Root "src\main\cpp\output\ohos\release\$Arch\libSE_MP4.so"
if (-not (Test-Path $OutSo)) { throw "Missing output: $OutSo" }
Write-Host "Build succeeded: $OutSo"
