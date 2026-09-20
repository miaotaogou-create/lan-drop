$ErrorActionPreference = "Stop"
$env:Path = "C:\ZYL\tools\go\bin;" + $env:Path
Set-Location (Split-Path -Parent $PSScriptRoot)

# 先编前端进 web/dist，再打进单文件客户端
Push-Location web
if (-not (Test-Path node_modules)) { npm install }
npm run build
Pop-Location

go test ./...
go build -ldflags "-H windowsgui -s -w" -o landrop.exe ./cmd/landrop

$env:GOOS = "linux"
$env:GOARCH = "arm64"
$env:CGO_ENABLED = "0"
go build -ldflags "-s -w" -o landrop-linux-arm64 ./cmd/landrop
Remove-Item Env:GOOS, Env:GOARCH, Env:CGO_ENABLED

Write-Host "已生成:"
Write-Host "  landrop.exe          Windows 单文件客户端（内嵌界面，双击开窗口）"
Write-Host "  landrop-linux-arm64  ARM64 Linux（内嵌界面，启动后打开浏览器）"
