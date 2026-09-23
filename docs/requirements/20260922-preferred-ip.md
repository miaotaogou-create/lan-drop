# 多网卡可选本机 IP

- 状态：已实现（待手测）
- 背景：多网卡/VPN 时 `localIpv4().first()` 可能不是同事所在网段，复制地址与网页共享易错。
- 目标：设置中可选「本机展示 IP」，持久化；顶栏与共享 URL 使用该 IP。
- 非目标：改 UDP 发现绑卡；自动探测「最佳」网卡。

## 用户故事

作为使用者，我插着 VPN 时希望指定给同事看的局域网 IP，扫码/加 IP 不会连错。

## 行为

1. 设置增加「本机 IP」下拉：自动 + 当前 `localIpv4()` 列表。
2. 写入 `preferredLocalIp`；空或「自动」= 仍取列表首项。
3. 若已选 IP 当前不在列表：回退自动，设置打开时如实显示。
4. `localIpText`、网页共享基址用同一选择。

## 验收标准

- [ ] N1：选非首网卡 IP 后，顶栏与复制地址一致。
- [ ] N2：共享对话框 URL 使用所选 IP。
- [ ] N3：重启后选择保持（IP 仍存在时）。

## 影响范围

- `src/settings.*`、`src/mainwindow.cpp`（设置页、`localIpText`）
- `docs/requirements/20260922-copy-local-addr.md`
- `docs/test/mvp-cases.md`

## 待确认

- 无

## 建议下一手

资深软件工程师实现；资深测试工程师补 TC。
