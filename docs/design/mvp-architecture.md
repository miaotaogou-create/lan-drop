# MVP 技术方案

## 架构

```
┌─────────────┐     UDP 发现(8850)      ┌─────────────┐
│  Qt 窗口   │◄───────────────────────►│  对端进程   │
│  (Widgets)  │     HTTP :8848          │             │
└──────┬──────┘  文本/文件/info         └─────────────┘
       │
       ▼
   同一进程：UDP 发现 + 三个 HTTP 接口 + Qt 窗口
```

## 客户端

客户端是 Qt Widgets。Windows 安装目录用 windeployqt 带上 Qt DLL；ARM 麒麟安装目录带上同一套 Qt 5.14 库，glibc 按 2.31 交叉编译。托盘列为后续。

## 发现

- UDP 广播 JSON 心跳，默认端口 **8850**
- 过滤明显虚拟网卡（名称含 VMware、VirtualBox、Hyper-V、vEthernet、WSL、Loopback 等）
- 支持手动添加 IP:端口，并 `GET http://ip:port/api/info` 探测

## 线上协议

对端客户端只认 `/api/info`、`/api/inbox`、`/api/upload`。设备列表、探测和设置在本机窗口里完成。网页共享是额外的浏览器入口。

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/api/info` | `id`、`name`、`port`、`os`、`ips` |
| POST | `/api/inbox` | JSON：`fromId`、`fromName`、`fromPort`、`text` |
| POST | `/api/upload` | multipart 字段 `file`，流式落盘，同名不覆盖 |
| GET | `/share/` | 网页共享：目录文件列表（HTML）；未开启时 404 |
| GET | `/share/<文件名>` | 网页共享：下载该文件（流式）；仅顶层文件，禁路径穿越 |

对端客户端仍只依赖前三个接口。网页共享给「没有装客户端的浏览器」用。

发现心跳为 UDP JSON：`{"id","name","port","os"}`。

## 已知限制

- 聊天记录只在内存中，最多每个会话 500 条，重启清空
- 本阶段只传单个文件，不打包文件夹
- 同一网段互信，接口没有鉴权
- 网页共享只列共享目录顶层文件，不递归子目录；关掉共享即停止暴露

## 构建

- Windows：`powershell -File scripts\build-windows.ps1`，产物 `dist/windows-x64`
- ARM64 Linux：在 WSL Ubuntu 20.04 执行 `sh scripts/build-linux-arm64.sh`，产物 `dist/linux-arm64`

两边都是 Qt 窗口。Qt 库放在安装目录里，不要求目标机器另外装 Qt、WebView2 或浏览器。
