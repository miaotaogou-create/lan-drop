# 菜单白卡与会话顶栏 meta 减负

- 状态：已实现
- 背景：托盘与侧栏右键仍是系统灰菜单，与主界面白卡断层最明显；会话顶栏 meta 一行堆 hostname、标签、Ping、链路，52px 内信息过密。
- 目标：菜单与主界面同系；顶栏 meta 只留一行轻量副信息，细节进 tooltip。
- 非目标：改菜单业务项；全量重做侧栏 delegate；弹窗 token 大合并（下轮）；换皮系统文件对话框。

## 用户故事

作为使用者，我希望：右键/托盘菜单看起来也是同一款软件；选中对端时顶栏一眼只看名字和地址，细节悬停再看。

## 行为

1. 托盘菜单与对端右键菜单共用 `styleAppMenu`：Fusion + 白底圆角、浅蓝悬停、浅部分隔线。
2. 会话 `peerMeta`：hostname 与显示名不同时显示 hostname，否则显示短标签；无则清空。
3. Ping、链路、在线态、其余标签写入 meta / 圆点 tooltip。

## 验收标准

- [x] M1：托盘右键菜单为白底圆角，悬停浅蓝
- [x] M2：侧栏对端右键菜单与托盘同系
- [x] M3：顶栏 meta 不再出现「Ping」字样
- [x] M4：悬停 meta / 圆点可看到 Ping、链路或在线说明

## 影响范围

- `src/uidialogs.cpp` / `uidialogs.h`
- `src/mainwindow.cpp`
- `docs/test/mvp-cases.md`（TC-163）

## 待确认

- 无
