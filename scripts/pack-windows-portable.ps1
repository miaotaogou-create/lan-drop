# 把 dist/windows-x64 打成单个便携 exe（Enigma Virtual Box）。
# 依赖：已安装 Enigma Virtual Box（含 enigmavbconsole.exe），且目录包已由 build-windows.ps1 生成。
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$dist = Join-Path $root "dist\windows-x64"
$inputExe = Join-Path $dist "landrop.exe"
$outDir = Join-Path $root "dist\windows-portable"
$outputExe = Join-Path $outDir "landrop-portable.exe"
$evb = Join-Path $outDir "landrop.evb"
$gen = Join-Path $PSScriptRoot "gen_enigma_evb.py"

$enigmaCandidates = @(
    "${env:ProgramFiles(x86)}\Enigma Virtual Box\enigmavbconsole.exe",
    "$env:ProgramFiles\Enigma Virtual Box\enigmavbconsole.exe",
    "C:\ZYL\tools\enigma-vb\enigmavbconsole.exe"
)
$enigma = $enigmaCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $enigma) { throw "找不到 enigmavbconsole.exe，请先安装 Enigma Virtual Box" }
if (-not (Test-Path $inputExe)) { throw "缺少 $inputExe，请先运行 scripts\build-windows.ps1" }

New-Item -ItemType Directory -Force -Path $outDir | Out-Null
if (Test-Path $outputExe) { Remove-Item -Force $outputExe }

python $gen --input $inputExe --output $outputExe --pack-dir $dist --evb $evb --skip "landrop.exe"
if ($LASTEXITCODE -ne 0) { throw "生成 .evb 失败" }

Write-Host "Enigma 装箱: $enigma"
& $enigma $evb
if ($LASTEXITCODE -ne 0) { throw "enigmavbconsole 失败: $LASTEXITCODE" }
if (-not (Test-Path $outputExe)) { throw "未生成 $outputExe" }

# 自检：单文件旁不应依赖外部 Qt DLL
$check = Start-Process -FilePath $outputExe -ArgumentList "--self-check" -Wait -PassThru -NoNewWindow
if ($check.ExitCode -ne 0) {
    Write-Warning "便携版 --self-check 退出码 $($check.ExitCode)（若仅 GUI 相关可手测双击）"
}

Write-Host "Windows 便携版: $outputExe"
Write-Host ("大小: {0:N1} MB" -f ((Get-Item $outputExe).Length / 1MB))
