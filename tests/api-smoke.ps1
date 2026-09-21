# 对已编译的 Windows 客户端做黑盒检查。不改产品代码。
# 用法：powershell -File tests\api-smoke.ps1
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$exe = Join-Path $root "dist\windows-x64\landrop.exe"
if (-not (Test-Path $exe)) { throw "找不到 $exe" }

$check = Start-Process -FilePath $exe -ArgumentList "--self-check" -Wait -PassThru -NoNewWindow
if ($check.ExitCode -ne 0) { throw "self-check 失败: $($check.ExitCode)" }
Write-Host "self-check ok"

$tmp = Join-Path $env:TEMP ("landrop-smoke-" + [guid]::NewGuid().ToString("n"))
New-Item -ItemType Directory -Force -Path $tmp | Out-Null

function Resolve-Saved([string]$path) {
    if ([System.IO.Path]::IsPathRooted($path)) { return $path }
    $rel = $path -replace '^[.][/\\]', ''
    return Join-Path $tmp $rel
}

$proc = $null
try {
    $proc = Start-Process -FilePath $exe -WorkingDirectory $tmp -PassThru
    $info = $null
    $deadline = (Get-Date).AddSeconds(8)
    while ((Get-Date) -lt $deadline) {
        try {
            $info = Invoke-RestMethod -Uri "http://127.0.0.1:8848/api/info" -TimeoutSec 1
            break
        } catch {
            Start-Sleep -Milliseconds 200
        }
    }
    if (-not $info) { throw "8 秒内没有 /api/info" }
    if (-not $info.name -or -not $info.port) { throw "/api/info 缺少 name 或 port" }
    Write-Host ("info name={0} port={1}" -f $info.name, $info.port)

    $inboxBody = '{"fromId":"smoke","fromName":"测试机","fromPort":8848,"text":"中文命令 echo 你好"}'
    $inbox = Invoke-WebRequest -Uri "http://127.0.0.1:8848/api/inbox" -Method POST -Body ([System.Text.Encoding]::UTF8.GetBytes($inboxBody)) -ContentType "application/json; charset=utf-8" -UseBasicParsing
    if ($inbox.StatusCode -ne 200 -or $inbox.Content -notmatch '"ok"\s*:\s*true') {
        throw "中文文本未被接受: $($inbox.StatusCode) $($inbox.Content)"
    }
    Write-Host "inbox ok"

    $empty = $false
    try {
        Invoke-WebRequest -Uri "http://127.0.0.1:8848/api/inbox" -Method POST -Body '{"text":"  "}' -ContentType "application/json" -UseBasicParsing | Out-Null
    } catch {
        $empty = $true
        if ($_.Exception.Response.StatusCode.value__ -ne 400) { throw "空文本应返回 400，实际 $($_.Exception.Response.StatusCode)" }
    }
    if (-not $empty) { throw "空文本不应成功" }
    Write-Host "empty inbox rejected"

    $small = Join-Path $tmp "note.txt"
    [System.IO.File]::WriteAllText($small, "文件内容-你好", [System.Text.UTF8Encoding]::new($false))
    $cn = -join ([char]0x8BF4, [char]0x660E, ".txt")
    $jsonFile = Join-Path $tmp "up.json"
    curl.exe -sS -o $jsonFile -F "file=@${small};filename=$cn" "http://127.0.0.1:8848/api/upload" | Out-Null
    $up = [System.IO.File]::ReadAllText($jsonFile, [System.Text.UTF8Encoding]::new($false))
    if ($up -notmatch '"ok"\s*:\s*true') { throw "上传失败: $up" }
    $saved = Resolve-Saved (($up | ConvertFrom-Json).path)
    if (-not (Test-Path -LiteralPath $saved)) { throw "上传响应的路径不存在: $saved" }
    $got = [System.IO.File]::ReadAllText($saved, [System.Text.UTF8Encoding]::new($false))
    if ($got -ne "文件内容-你好") { throw "落下的文件内容不对" }
    if ((Split-Path -Leaf $saved) -ne $cn) { throw "中文文件名被改掉: $saved" }
    Write-Host "upload chinese name ok"

    curl.exe -sS -o $jsonFile -F "file=@${small};filename=../../outside.txt" "http://127.0.0.1:8848/api/upload" | Out-Null
    $escape = [System.IO.File]::ReadAllText($jsonFile, [System.Text.UTF8Encoding]::new($false))
    $escapePath = Resolve-Saved (($escape | ConvertFrom-Json).path)
    if ((Split-Path -Parent $escapePath) -ne (Split-Path -Parent $saved)) { throw "路径穿越写出了下载目录: $escapePath" }
    if ((Split-Path -Leaf $escapePath) -ne "outside.txt") { throw "穿越文件名未收成基本名: $escapePath" }
    Write-Host "path escape kept inside download dir"

    curl.exe -sS -o $jsonFile -F "file=@${small};filename=$cn" "http://127.0.0.1:8848/api/upload" | Out-Null
    $again = [System.IO.File]::ReadAllText($jsonFile, [System.Text.UTF8Encoding]::new($false))
    $againName = ($again | ConvertFrom-Json).name
    if ($againName -eq $cn) { throw "同名文件被覆盖" }
    Write-Host "same name not overwritten: $againName"

    $big = Join-Path $tmp "big.bin"
    $fs = [System.IO.File]::Create($big)
    $fs.SetLength(100MB)
    $fs.Close()
    $before = (Get-Process -Id $proc.Id).WorkingSet64
    curl.exe -sS -o $jsonFile -F "file=@${big};filename=big.bin" "http://127.0.0.1:8848/api/upload" | Out-Null
    $bigUp = [System.IO.File]::ReadAllText($jsonFile, [System.Text.UTF8Encoding]::new($false))
    $after = (Get-Process -Id $proc.Id).WorkingSet64
    $bigPath = Resolve-Saved (($bigUp | ConvertFrom-Json).path)
    if ((Get-Item -LiteralPath $bigPath).Length -ne 100MB) { throw "100MB 文件大小不对" }
    $deltaMb = [math]::Round(($after - $before) / 1MB, 1)
    Write-Host ("100MB saved, working set delta {0} MB" -f $deltaMb)
    if ($deltaMb -gt 80) { throw "发送/接收后工作集大约涨了 $deltaMb MB，不像流式" }
    Write-Host "smoke ok"
} finally {
    if ($proc -and -not $proc.HasExited) {
        Stop-Process -Id $proc.Id -Force
        Start-Sleep -Milliseconds 500
    }
    if ($tmp -and (Test-Path $tmp)) {
        Remove-Item -Recurse -Force $tmp -ErrorAction SilentlyContinue
    }
}
