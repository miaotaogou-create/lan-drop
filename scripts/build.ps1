$ErrorActionPreference = "Stop"
$env:Path = "C:\ZYL\tools\go\bin;" + $env:Path
Set-Location (Split-Path -Parent $PSScriptRoot)
go build -o landrop.exe ./cmd/landrop
$env:GOOS = "linux"
$env:GOARCH = "arm64"
$env:CGO_ENABLED = "0"
go build -o landrop-linux-arm64 ./cmd/landrop
Remove-Item Env:GOOS, Env:GOARCH, Env:CGO_ENABLED
Write-Host "已生成 landrop.exe 与 landrop-linux-arm64"
