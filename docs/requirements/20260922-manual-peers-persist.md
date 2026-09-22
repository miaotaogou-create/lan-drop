# 手动添加节点重启后仍保留

- 状态：已实现
- 背景：跨 VLAN / VPN / 禁广播场景下靠「+ 加 IP」添加节点；当前仅内存保存，关软件即丢，每次重开都要重填。
- 目标：手动节点写入本机 `settings.json`，启动时自动恢复到设备列表。
- 非目标：
  - 部门 / 标签入库（仍见加节点对话框 D6）
  - 提供「删除手动节点」专门 UI（见 `20260922-remove-manual-peer.md`）
  - 改发现协议 / 超时策略

## 用户故事

作为跨网段用户，我希望手动加过的 IP 节点在重启后仍出现在列表里，以免每次重复填写。

## 行为

1. 「+ 添加并连接」成功后：把当前所有 `manual` 节点写入 `settings.json` 的 `manualPeers` 数组。
2. 启动 / 设置保存触发 `boot()`：在发现服务启动后，按 `manualPeers` 逐条 `addManual`。
3. 设置页保存其它字段时：不得清空已有 `manualPeers`。
4. 条目字段：`ip`（必填）、`port`（缺省 8848）、`alias`、`os`（与对话框系统类型一致）。

## 验收标准

- [x] P1：添加手动节点后，`settings.json` 出现对应 `manualPeers` 项
- [x] P2：完全退出再启动，列表仍有该节点（别名 / os 保留）
- [x] P3：无手动节点时可不写该字段或写空数组；缺字段不影响启动
- [x] P4：`--self-check` 覆盖 manualPeers 读写往返

## 影响范围

- `src/settings.h` / `settings.cpp`
- `src/mainwindow.cpp`（`boot`、`addPeer`）
- `src/selfcheck.cpp`
- `docs/test/mvp-cases.md`

## 待确认

- 无

## 建议下一手

软件工程师实现；测试工程师补 TC 与自检。
