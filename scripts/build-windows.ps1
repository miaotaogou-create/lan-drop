$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$vcvars = "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat"
$qmake = "C:\Qt\Qt5.14.2\5.14.2\msvc2017_64\bin\qmake.exe"
$windeploy = "C:\Qt\Qt5.14.2\5.14.2\msvc2017_64\bin\windeployqt.exe"
$build = Join-Path $root "build\win"
$dist = Join-Path $root "dist\windows-x64"
$pro = Join-Path $root "lan-drop.pro"

if (-not (Test-Path $qmake)) { throw "找不到 Qt：$qmake" }
if (-not (Test-Path $vcvars)) { throw "找不到 VS2017：$vcvars" }

if (Test-Path $build) { Remove-Item -Recurse -Force $build }
New-Item -ItemType Directory -Force -Path $build | Out-Null
if (Test-Path $dist) { Remove-Item -Recurse -Force $dist }
New-Item -ItemType Directory -Force -Path $dist | Out-Null

$cmd = "call `"$vcvars`" x64 && cd /d `"$build`" && `"$qmake`" `"$pro`" -spec win32-msvc CONFIG+=release && nmake"
cmd /c $cmd
if ($LASTEXITCODE -ne 0) { throw "Windows 编译失败" }

$exe = Join-Path $build "release\landrop.exe"
if (-not (Test-Path $exe)) { $exe = Join-Path $build "landrop.exe" }
Copy-Item $exe (Join-Path $dist "landrop.exe") -Force
& $windeploy --release --compiler-runtime --no-translations (Join-Path $dist "landrop.exe")
if ($LASTEXITCODE -ne 0) { throw "windeployqt 失败" }
# 文件对话框要中文
$trSrc = "C:\Qt\Qt5.14.2\5.14.2\msvc2017_64\translations\qt_zh_CN.qm"
$trDir = Join-Path $dist "translations"
New-Item -ItemType Directory -Force -Path $trDir | Out-Null
if (Test-Path $trSrc) { Copy-Item $trSrc $trDir -Force }

& (Join-Path $dist "landrop.exe") --self-check
if ($LASTEXITCODE -ne 0) { throw "self-check 失败" }

Write-Host "Windows 安装目录: $dist"
