# 局域快传

同一网段互传文字和文件。默认 HTTP 端口 **8848**，发现 UDP 端口 **8850**。

## 客户端（单文件）

Windows 双击 `landrop.exe` 会打开**桌面窗口**（内嵌界面，不依赖旁边的 `web` 目录）。  
若本机没有 WebView2，会自动改用系统浏览器。

ARM 麒麟运行 `landrop-linux-arm64` 后，会自动打开浏览器进入界面（界面已打进二进制）。

```powershell
powershell -File scripts\build.ps1
```

生成：

- `landrop.exe` — Windows 单文件客户端
- `landrop-linux-arm64` — ARM64 Linux 单文件

开发调试（带控制台）：

```text
go test ./...
go build -o landrop-console.exe ./cmd/landrop
.\landrop-console.exe
```

强制浏览器模式：`landrop.exe -browser`

## 防火墙

- TCP **8848**（界面、接口、传文件、文本）
- UDP **8850**（局域网发现）

只手动填写对方 IP 时，至少要放行 TCP 8848。

## 说明

配置写在用户配置目录的 `lan-drop/settings.json`（写不了则用 `./data/settings.json`）。  
下载目录默认 `./downloads`。

接口说明见 `docs/design/mvp-architecture.md`。
