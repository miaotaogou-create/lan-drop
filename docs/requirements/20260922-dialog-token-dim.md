# 弹窗 chrome token 对齐与轻 dim

- 状态：已实现
- 背景：确认框与设置/共享/加 IP/编辑节点各写一套头脚与主次按钮色；打开弹窗时背后界面仍满屏抢注意力。
- 目标：表单弹窗共用 chrome token；打开时主窗轻半透明 dim；确认框同样带 dim。
- 非目标：点空白关闭；全屏遮罩动画；QFileDialog 换皮；改弹窗业务字段。

## 用户故事

作为使用者，我希望设置、共享、加 IP 和确认框看起来是同一套组件；打开时后面暗一点，焦点在弹窗上。

## 行为

1. `formDialogChromeQss` / `formDialogSecondaryBtnQss`：共用头脚字段与主次钮。
2. `showDialogDim`：主窗（或锚点 window）轻墨色衬底，关闭后移除。
3. 设置 / 共享 / 加 IP / 编辑 / app 确认框均接入。

## 验收标准

- [x] D1：设置 / 加 IP / 确认框头脚与主次钮同系
- [x] D2：打开弹窗时主窗变暗，关闭后恢复
- [x] D3：共享关闭钮白底灰边
- [x] D4：业务功能未改

## 影响范围

- `src/uidialogs.cpp` / `uidialogs.h`
- `src/mainwindow.cpp`
- `docs/test/mvp-cases.md`（TC-166）

## 待确认

- 无
