param(
    [string]$OhosNativeHome = $env:OHOS_NATIVE_HOME,
    [ValidateSet("arm64-v8a")]
    [string]$Arch = "arm64-v8a",
    [string]$BashPath = "C:\Program Files\Git\bin\bash.exe"
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
$DestDir = Join-Path $Root "src\main\jniLibs\ohos\$Arch"
$Work = Join-Path $Root ".build\ffmpeg-ohos-$Arch"

function Resolve-OhosNativeHome {
    param([string]$Hint)
    $candidates = @()
    if ($Hint) { $candidates += $Hint }
    $candidates += "D:\StudioProjects\Http2p2p_API\.ohos-sdk\native"
    foreach ($path in $candidates) {
        if (Test-Path (Join-Path $path "build\cmake\ohos.toolchain.cmake")) {
            return (Resolve-Path $path).Path
        }
    }
    throw "OpenHarmony Native SDK not found."
}

if (-not (Test-Path $BashPath)) { throw "bash not found at $BashPath" }

$NativeHome = Resolve-OhosNativeHome -Hint $OhosNativeHome
New-Item -ItemType Directory -Force -Path $Work | Out-Null

$Tar = Join-Path $Work "ffmpeg-4.2.2.tar.bz2"
$SrcDir = Join-Path $Work "ffmpeg-4.2.2"
if (-not (Test-Path $SrcDir)) {
    if (-not (Test-Path $Tar)) {
        Write-Host "Downloading ffmpeg 4.2.2..."
        $ProgressPreference = "SilentlyContinue"
        curl.exe -fsSL -o $Tar "https://ffmpeg.org/releases/ffmpeg-4.2.2.tar.bz2"
        if ($LASTEXITCODE -ne 0) { throw "curl download failed" }
    }
    tar -xf $Tar -C $Work
}

$ScriptSh = Join-Path $Root ".build\build_ffmpeg_ohos.sh"
if (-not (Test-Path $ScriptSh)) { throw "Missing $ScriptSh" }

$env:OHOS_NATIVE_HOME = $NativeHome
& $BashPath $ScriptSh $SrcDir $Work $NativeHome
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

New-Item -ItemType Directory -Force -Path $DestDir | Out-Null
$LibOut = Join-Path $Work "out\lib"
foreach ($name in @("libavcodec.so", "libavformat.so", "libavutil.so", "libswresample.so")) {
    $src = Join-Path $LibOut $name
    if (-not (Test-Path $src)) { throw "Missing $src after ffmpeg build" }
    Copy-Item $src (Join-Path $DestDir $name) -Force
}
Write-Host "Installed OHOS FFmpeg shared libs to $DestDir"
