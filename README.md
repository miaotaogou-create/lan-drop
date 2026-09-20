# 局域快传

同一网段互传文字和文件。默认 HTTP 端口 **8848**，发现 UDP 端口 **8850**。

## 运行

需要本机已安装 Go。在仓库根目录：

```text
go test ./...
go build -o landrop.exe ./cmd/landrop
landrop.exe
```

浏览器打开 `http://127.0.0.1:8848/`。没有 `web/dist` 时会看到 API 占位页。

配置优先写在用户配置目录的 `lan-drop/settings.json`，写不了则用 `./data/settings.json`。下载目录默认 `./downloads`（相对启动时的工作目录）。

## 防火墙

Windows 防火墙和安全软件需放行：

- TCP **8848**（接口、传文件、文本）
- UDP **8850**（局域网发现）

只手动填写对方 IP 时，至少要放行 TCP 8848。

## 交叉编译

```powershell
powershell -File scripts\build.ps1
```

也可手工：
$env:GOOS = "linux"
$env:GOARCH = "arm64"
go build -o landrop-linux-arm64 ./cmd/landrop
Remove-Item Env:GOOS, Env:GOARCH
```

接口说明见 `docs/design/mvp-architecture.md`。
