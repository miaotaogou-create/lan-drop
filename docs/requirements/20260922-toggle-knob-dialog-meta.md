# Toggle 白滑块、弹窗脚栏对齐与顶栏 meta 收紧

- 状态：已实现（待手测）
- 背景：设置 Toggle 仍像色条无白圆；加/编辑对端脚栏未对齐设置/共享；共享 URL 22px 抢戏；52px 顶栏 meta 仍重复「在线」。
- 目标：Toggle 有白滑块；加/编辑脚栏灰底圆角；共享 URL 收声；meta 去掉与圆点重复的在线文案。
- 非目标：主标题栏再压高度；右侧空卡重做。

## 用户故事

作为使用者，我希望：设置开关一眼就是现代 Toggle；加 IP 和设置脚栏同一套；共享链接不刺眼；会话顶栏第二行更干净。

## 行为

1. `#settingsToggle` 使用带白圆的开/关图。
2. `#addPeerFoot` / `#editPeerFoot`：背景 `#f8fafc` + 底圆角（对齐设置/共享）。
3. `#shareUrl`：约 15px、字重 600、mono。
4. 会话 meta：保留 hostname/tag、Ping、链路标签；去掉与状态点重复的「在线/离线」二字。

## 验收标准

- [x] V1：设置开关可见白圆滑块（实现已对齐，待手测）
- [x] V2：加 IP / 编辑节点脚栏为灰底（实现已对齐，待手测）
- [x] V3：共享 URL 不再明显大于标题（实现已对齐，待手测）
- [x] V4：顶栏 meta 无重复「在线」字样（圆点仍表示在线）（实现已对齐，待手测）

## 影响范围

- `icons/toggle-off.svg` / `toggle-on.svg` / `icons.qrc`
- `src/mainwindow.cpp`
- `docs/test/mvp-cases.md`

## 待确认

- 无

## 建议下一手

手测 V1–V4。
