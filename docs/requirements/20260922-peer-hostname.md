# 会话顶栏主机名（hostname）

- 状态：已实现
- 背景：选中对端后，会话顶栏副行首段原先是系统类型（如 `windows`），参考图为类似 `ubuntu-nas.local` 的主机名，再接 Ping 与链路。
- 目标：副行显示对端主机名；发现广播与 `/api/info` 携带 `hostname`；旧端无该字段时可回退。
- 非目标：
  - 改本机显示名规则、强制 `.local` 后缀
  - mDNS 主机名解析、反向 DNS
  - 改 Ping / 链路已有逻辑

## 用户故事

作为使用者，我希望一眼看到对端机器的主机名，而不只是笼统的系统类型，以便区分同系统多台设备。

## 行为

1. 本机对外报告：`hostname` = 操作系统主机名（`QHostInfo::localHostName()`，空则省略该字段）。
2. 发现 UDP 与 `GET /api/info` 均包含可选字段 `hostname`。
3. 会话副行格式：`{hostname}  ·  Ping …  ·  {链路}`；无 hostname 时回退为设备名（`label`），再无则用 IP。
4. 旧版本对端不发 hostname：不报错，按回退规则显示。

## 验收标准

- [x] H1：两台均升级后，选中对端可见副行首段为本机主机名（与系统主机名一致，允许无 `.local`）
- [x] H2：副行仍含 Ping 与链路文案，不因 hostname 丢失
- [x] H3：仅一端升级时，升级端选中旧端不崩溃，首段回退为设备名或 IP

## 影响范围

- `src/discovery.h` / `discovery.cpp`
- `src/httpserver.cpp`：`/api/info`
- `src/mainwindow.cpp`：`updatePeerSession`、探测刷新
- `docs/requirements/20260921-chat-session-header.md`
- `docs/test/mvp-cases.md`

## 待确认

- 无。不强制追加 `.local`。

## 建议下一手

手测 H1–H3；后续可做共享拖放或部门标签。
