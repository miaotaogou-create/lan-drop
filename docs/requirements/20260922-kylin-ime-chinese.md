# ARM 麒麟中文输入

- 状态：已实现（待 22 机手测确认上屏）
- 背景：自带 Qt 包只含 ibus/compose 输入法插件，且 `QT_PLUGIN_PATH` 只指向包内目录；银河麒麟 V10 默认 fcitx（含搜狗），桌面会话里的 `QT_IM_MODULE=fcitx` 即使有，也加载不到系统 fcitx 插件，输入框无法出中文候选。
- 目标：在 ARM 麒麟（fcitx）上，聊天/设置等文本框可正常调起中文输入法并上屏。
- 非目标：
  - 自研输入法或内嵌虚拟键盘
  - 打包完整 fcitx/搜狗引擎（仍用系统已装输入法）
  - Windows/macOS 输入法改动

## 用户故事

作为麒麟用户，我希望在局域快传里能像系统其他软件一样输入中文，以便发消息、改设备名、填路径。

## 行为

1. Linux 启动脚本在未指定时默认按 fcitx 设置 `QT_IM_MODULE` / `XMODIFIERS` / `GTK_IM_MODULE`（已有环境变量则不覆盖）。
2. 若包内尚无 fcitx 的 `platforminputcontexts` 插件，且系统存在该插件，则启动时链接或复制到包内插件目录，使自带 `QT_PLUGIN_PATH` 能加载。
3. 仍保留 ibus/compose 插件，便于非 fcitx 环境。
4. 不修改业务窗口逻辑；纯启动与打包侧修复。

## 验收标准

- [ ] A1：麒麟（fcitx 运行中）打开聊天输入框，可切换中文输入法并上屏汉字。
- [ ] A2：设置里可改中文设备名并保存。
- [x] A3：进程环境可见 `QT_IM_MODULE=fcitx`，且包内 `plugins/platforminputcontexts/` 能解析到 fcitx 插件。
- [x] A4：Windows 构建与行为不受影响。

## 影响范围

- `scripts/build-linux-arm64.sh`（生成 `landrop.sh`）
- `dist/linux-arm64/landrop.sh`
- `docs/test/mvp-cases.md`

## 待确认

- 无（22 机已确认为 fcitx + sogouimebs）

## 建议下一手

软件工程师按上述改启动脚本并重新部署 22 机验证。
