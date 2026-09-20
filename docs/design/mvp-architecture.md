# MVP 技术方案

## 架构

```
┌─────────────┐     UDP 发现(8850)      ┌─────────────┐
│  本机 UI    │◄───────────────────────►│  对端进程   │
│  (Web静态页)│     HTTP :8848          │             │
└──────┬──────┘  文本/文件/info         └─────────────┘
       │
       ▼
   Go 单进程：发现 + HTTP API + 静态资源
```

## 为何先不用 Wails

MVP 优先把发现、聊天、传文件跑通并交叉编译到 arm64。UI 用内嵌 Web 页（浏览器或日后套 Wails 窗口）。托盘列为后续。

## 发现

- UDP 广播 JSON 心跳，默认端口 **8850**
- 过滤明显虚拟网卡（名称含 VMware、VirtualBox、Hyper-V、vEthernet、WSL、Loopback 等）
- 支持手动添加 IP:端口，并 `GET http://ip:port/api/info` 探测

## HTTP API（摘要）

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/api/info` | 本机节点信息 |
| GET | `/api/peers` | 在线/手动节点列表 |
| POST | `/api/peers` | 手动添加 |
| POST | `/api/peers/probe` | 探测连通 |
| GET | `/api/messages?peerId=` | 与某节点会话 |
| POST | `/api/inbox` | 对端推送文本/信令 |
| POST | `/api/send-text` | 本机发文本到指定节点 |
| POST | `/api/send-file` | 本机上传文件到指定节点 |
| POST | `/api/upload` | 接收文件流并落盘 |
| GET/PUT | `/api/settings` | 本机设置 |
| GET | `/` | Web UI（有 `web/dist` 则托管，否则占位页） |

### 请求字段

- 手动添加 `POST /api/peers`：`{"ip","port","alias","os?"}`，`port` 省略时按 8848
- 探测 `POST /api/peers/probe`：`{"ip","port"}`，成功时返回 `{"ok":true,"info":{...}}`
- 发文本 `POST /api/send-text`：`{"peerId?"}` 或 `{"ip","port"}`，以及 `text`。本机再 `POST` 对端 `/api/inbox`
- 收文本 `POST /api/inbox`：`{"fromId","fromName","fromPort","text"}`
- 发文件 `POST /api/send-file`：multipart 字段 `peerId` / `ip` / `port` / `file`。本机再以 multipart 流式 `POST` 对端 `/api/upload`
- 收文件 `POST /api/upload`：multipart 字段 `file`，流式写入下载目录，同名不覆盖
- 设置 `PUT /api/settings`：`deviceName`、`port`、`discoverPort`、`downloadDir`。设备名和下载目录立即生效；两个端口下次启动才重新绑定

`GET /api/info` 返回 `id`、`name`、`port`（实际监听端口）、`os`、`ips`。

发现心跳为 UDP JSON：`{"id","name","port","os"}`。

## 已知限制

- 聊天记录只在内存中，最多每个会话 500 条，重启清空
- 本阶段只传单个文件，不打包文件夹
- 同一网段互信，接口没有鉴权
- 发送文件时会先写一份临时文件再转发，避免整文件进内存

## 构建

- Windows：`go build -o landrop.exe ./cmd/landrop`
- ARM64 Linux：`GOOS=linux GOARCH=arm64 go build -o landrop-linux-arm64 ./cmd/landrop`
