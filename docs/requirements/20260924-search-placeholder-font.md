# 侧栏搜索占位字体与文案

- 状态：已实现（待手测）
- 背景：搜索框占位字发虚、中英粗细割裂、颜色过淡；文案「搜索名称、IP 或标签…」偏长，框内拥挤。
- 目标：锁定现代中文字体并抗锯齿；占位色 `#94a3b8`；文案缩短；输入字色沉稳深蓝灰。
- 非目标：改搜索过滤逻辑；改放大镜图标；改 Ctrl+F / Esc 行为。

## 用户故事

作为使用者，我希望侧栏搜索占位字清晰可读、不过长，以便一眼知道可搜设备名或 IP。

## 行为

1. `#search`：优先 `Microsoft YaHei UI`（回退雅黑 / Segoe UI / Noto / 苹方），`PreferAntialias`，字号 12px。
2. 占位色 `QPalette::PlaceholderText = #94a3b8`（Qt 5.14 不用不可靠的 `::placeholder` QSS）。
3. 输入字色 `#1e293b`。
4. 占位文案精简为：`搜索设备、IP…（Ctrl+F）`（tooltip 仍说明 Esc / Ctrl+F）。

## 验收

- [ ] S1：Windows 下占位字笔画清晰，无明显狗牙/发虚。
- [ ] S2：占位色为冷板岩灰，白底上可读。
- [ ] S3：文案为「搜索设备、IP…（Ctrl+F）」，框内不显拥挤。
- [ ] S4：Ctrl+F 聚焦、Esc 清空行为不变。

## 影响范围

- `src/mainwindow.cpp`（`m_search` 字体/调色板/文案与 `#search` QSS 字色）
