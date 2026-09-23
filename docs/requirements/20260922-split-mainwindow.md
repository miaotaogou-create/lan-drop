# MainWindow 源码按职责拆分

- 状态：已实现（待手测）
- 背景：`mainwindow.cpp` 已近五千行，UI 绘制、聊天气泡 HTML、窗口编排混在一起，难读难改。
- 目标：按职责拆出可独立编译的模块，MainWindow 只保留窗口编排与业务接线；不改对外行为。
- 非目标：
  - 引入新框架 / MVC 全套
  - 一次拆完所有对话框（设置/共享可后续）
  - 为拆而拆的空抽象层

## 用户故事

作为维护者，我希望改气泡样式不必在五千行文件里翻；作为使用者，界面与传输行为应与拆分前一致。

## 行为（拆分边界）

1. `chatmsg.h`：消息结构体 `ChatMsg`（原在 mainwindow.h）。
2. `uiicons.cpp/.h`：标题栏图标、对端头像、SVG 渲染、状态点等纯绘制辅助。
3. `chatrender.cpp/.h`：聊天气泡 / 文件卡 / 系统条 HTML 拼装（依赖 ChatMsg）。
4. `fmtutil.cpp/.h`：体积与 ETA 等纯格式化（`humanBytesChat`、`formatEta` 等）。
5. `mainwindow.cpp`：保留 boot、UI 组装、槽函数、传输与托盘等编排。
6. `lan-drop.pro` 同步 SOURCES/HEADERS。

## 验收标准

- [x] M1：上述新文件已加入工程且 Release 编译通过。
- [x] M2：`--self-check` 通过。
- [ ] M3：启动后收发文字/文件、文件卡链接与拆分前行为一致（冒烟）。
- [x] M4：`mainwindow.cpp` 行数明显下降（约 4800→4000；绘制与气泡 HTML 已迁出）。

## 影响范围

- `src/mainwindow.cpp` / `mainwindow.h`
- 新增 `src/chatmsg.h`、`src/uiicons.*`、`src/chatrender.*`、`src/fmtutil.*`
- `lan-drop.pro`
- `docs/test/mvp-cases.md`

## 待确认

- 无

## 建议下一手

手测 M3；后续可再拆设置/共享对话框。
