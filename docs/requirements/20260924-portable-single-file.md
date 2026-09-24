# Windows / ARM 麒麟单文件便携版

- 状态：待实现（目标已澄清：尽量「一个文件走天下」）
- 背景：现有产物为目录包（Windows `dist/windows-x64`；麒麟 `dist/linux-arm64` + `landrop.sh`）。希望只拷一个可执行文件即可运行。
- 目标：各平台交付**单个可执行文件**；Linux 侧在技术可行范围内**尽量自带库与字体**，减少对目标机软件包的依赖。
- 非目标：不做成 MSI/deb；不改变协议与业务；**不做**无图形桌面场景（甲方 ARM 麒麟均为有桌面环境）。

## 用户故事

作为出差/厂区用户，我希望只带一个文件到目标机就能打开局域快传，而不必解压目录或安装 Qt/依赖包。

## 「走天下」能到哪一步（硬边界）

用户期望：glibc、X11、字体**全都打进**一个 Linux 可执行文件。

| 依赖 | 能否打进单文件 | 说明 |
|------|----------------|------|
| Qt 与插件、自带 so | **能** | AppImage / 自解压包常规做法 |
| libxcb / libX11 等 **X11 客户端库** | **能** | 可随包携带，不依赖目标机 `libx11` 包 |
| 中文字体（如 Noto Sans CJK 子集） | **能** | 体积大（可达数十 MB），可用 `QFontDatabase::addApplicationFont` 或包内 fontconfig |
| **glibc + 动态链接器** | **原则上不建议整库搬家** | glibc 与内核/ABI 强耦合；业界做法是「在足够老的 glibc 上构建」（本项目已按 **2.31** 对齐麒麟），使目标机自带 glibc 能加载，而不是把另一份 glibc 塞进包 |
| **X 服务器 / Wayland 合成器** | **不能** | 显示服务属于桌面会话；GUI 程序必须连已有 `$DISPLAY`（或 Wayland）。无显示器会话 = 无法出窗，与是否单文件无关 |
| 内核 | **不能** | 任何用户态程序都依赖目标机内核 |

**可达成的「一个文件走天下」定义（本需求采纳）**：

1. 用户只拷贝**一个**文件（Windows：装箱 exe；Linux：AppImage 或等价）。
2. 目标机满足：同架构（amd64 / aarch64）、**glibc ≥ 2.31**（或构建基线）、**已登录图形桌面**（有 X11/Wayland）。
3. **不要求**目标机预装 Qt、本应用 so、专用字体包；这些尽量打进单文件。
4. 输入法：仍优先用桌面已有 fcitx/ibus（输入法框架是桌面组件，不宜也不可能完整塞进应用包）。

这与 LocalSend / 多数 Qt AppImage 的实际上限一致；再往上（无桌面、无 glibc、任意发行版）属于另一类产品，不在本仓库范围。

## 功能需求

1. **Windows amd64**：`windeployqt` 目录包 → Enigma Virtual Box（或等价）→ **单个** `.exe`；图标为官方应用图标。
2. **Linux arm64（麒麟等）**：**单个**可执行文件；内嵌现有 `lib`、Qt 插件；**尽量捆绑** X11 客户端库与中文字体；启动观感接近「双击即用」。
3. 目录版可保留作调试；发布说明写清单文件版的假设（glibc / 图形会话）。

## 平台手段

| 平台 | 手段 | 说明 |
|------|------|------|
| Windows | Enigma Virtual Box | 无 Enigma 同款 Linux 工具 |
| Linux ARM | **AppImage**（已选定；makeself 备胎） | 在 WSL Ubuntu 20.04 交叉编出目录包后打成 aarch64 AppImage |

## 验收标准

- [ ] W1：Windows 单文件在干净机（无 Qt PATH）可启动，文字/小文件互通。
- [ ] W2：资源管理器显示官方应用图标。
- [ ] L1：ARM 麒麟上单个文件可启动，无需用户手动解压目录；文字/小文件互通。
- [ ] L3：在「未额外安装 Qt / 本应用字体包」的麒麟桌面上，界面中文可显示（靠包内字体或系统已有字体二选一，包内优先）。
- [ ] L2：fcitx 中文输入仍按 `20260922-kylin-ime-chinese.md` 可用（依赖桌面输入法，不宣称输入法也被打包）。
- [ ] D1：文档写明：单文件**仍需要**图形会话 + 足够新的 glibc；**不能**在纯 SSH 无 X 环境出窗。

## 约束

- Windows 装箱体积 ≈ 目录包；可能被杀毒误报。
- Linux 打进字体后体积显著增大，可接受。
- Enigma 为闭源第三方，许可证发布者自备。

## 待确认

- [x] 「走天下」定义：目标机**有图形桌面**（甲方 ARM 麒麟均为桌面机）；不做无桌面/纯 SSH 出窗。仍需系统 glibc ≥ 构建基线；Qt/客户端库/字体尽量打进单文件。
- [x] 麒麟单文件形态：**AppImage**（makeself 仅作备胎）。
- [x] 构建主机：本机 **WSL Ubuntu 20.04** 交叉编译（既有 `scripts/build-linux-arm64.sh`）；产物已多次在甲方 ARM 麒麟桌面机验证可运行。AppImage 在同一环境、基于现有 `dist/linux-arm64` 目录包再打包。
- [ ] Windows：Enigma 手工装箱清单，或半自动脚本。

## 关联

- `scripts/build-windows.ps1`、`scripts/build-linux-arm64.sh`
- `icons/landrop.ico`、`landrop_app_icon.svg`
- `docs/requirements/20260922-kylin-ime-chinese.md`
