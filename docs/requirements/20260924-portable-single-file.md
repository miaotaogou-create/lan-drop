# Windows / ARM 麒麟单文件便携版

- 状态：待实现
- 背景：现有产物为目录包（Windows `dist/windows-x64` + windeployqt；麒麟 `dist/linux-arm64` + `landrop.sh`）。希望拷贝时只需一个可执行文件，双击即可运行，减少「缺 DLL / 忘带 lib」类问题。
- 目标：各平台交付「一个可执行文件走天下」的便携版；功能与现目录包一致。
- 非目标：不做成系统安装包（MSI / deb）；不追求真正静态链接整棵 Qt；不改变协议与业务行为。

## 用户故事

作为出差/厂区用户，我希望只带一个文件到目标机就能打开局域快传，而不必解压一整个目录或记启动脚本。

## 功能需求

1. **Windows amd64**：在 `windeployqt` 目录包之上，用 Enigma Virtual Box（或等价虚拟化装箱）打成**单个** `.exe`；双击可运行；任务栏/托盘/资源管理器图标与官方应用图标一致。
2. **Linux arm64（银河麒麟等）**：交付**单个**可执行文件（用户侧观感与「一个 exe」同类）；内部可自解压或挂载只读文件系统，启动后行为与当前 `./landrop.sh` 一致（含 fcitx 中文输入路径约定）。
3. 便携版与目录版可并存；发布说明写清推荐用法。

## 平台能力说明（已确认事实）

| 平台 | 能否「一个文件」 | 推荐手段 | 说明 |
|------|------------------|----------|------|
| Windows | 能 | Enigma Virtual Box 装箱 `dist/windows-x64` | 与现网常见做法一致；虚拟化装箱，非静态链接 |
| Linux ARM 麒麟 | **能做成「一个文件」**，但**没有**与 Enigma 同款工具 | **AppImage（aarch64）** 或 **makeself 自解压脚本** | Enigma 仅 Windows；Linux 靠打包格式，不是虚拟化 PE |

### Linux 方案对比（选型用）

| 方案 | 优点 | 风险 / 成本 |
|------|------|-------------|
| AppImage | 业界便携标准；双击/chmod+x 即可；可签名 | 需 aarch64 runtime；麒麟上偶发 FUSE/`AppImageLauncher`；须把现有 `lib`+`plugins`+fcitx 挂载逻辑收进 AppDir |
| makeself / 自解压 tar | 实现简单；与现 `dist/linux-arm64` 几乎一一对应 | 首次运行解压到 `/tmp` 或旁路目录；杀毒/只读盘需留意；体感略逊于 AppImage |
| 真·静态 Qt | 理论上单 ELF | Qt Widgets + XCB 静态链极重，维护成本高，**不推荐** |
| Flatpak / Snap | 分发标准 | 不是「拷一个文件就走」，偏离本需求 |

**建议默认**：麒麟侧优先 **AppImage**；若交叉环境难产 AppImage 工具链，则退化为 **makeself**（仍满足「一个文件」）。

## 验收标准

- [ ] W1：Windows 单文件 exe 在干净机（无 Qt 在 PATH）可启动，能发现对端、收发文字与小文件。
- [ ] W2：该 exe 在资源管理器中显示官方应用图标。
- [ ] L1：ARM 麒麟上单个便携文件可启动（无需用户手动解压目录），能发现对端、收发文字与小文件。
- [ ] L2：麒麟便携版聊天框仍可按既有约定使用 fcitx 中文输入（与 `20260922-kylin-ime-chinese.md` 不冲突）。
- [ ] D1：`README` 或发布说明写明：目录版与单文件版各自路径、如何生成。

## 约束与已知限制

- 麒麟仍依赖系统 **glibc ≥ 2.31**、X11/fontconfig 等（与现目录包相同）；「一个文件」不等于零系统依赖。
- Windows 单文件体积约等于目录包之和；杀毒软件可能对装箱 exe 误报（已知行业现象，文档中提示即可）。
- Enigma Virtual Box 为闭源第三方工具：生成步骤可脚本化到能自动化处，许可证由发布者自备。

## 待确认

- [ ] 麒麟侧最终选 AppImage 还是 makeself（默认建议 AppImage）。
- [ ] Windows 单文件是否由脚本半自动调用 Enigma CLI，或先保留手工装箱清单 + 验收。

## 关联

- 现构建：`scripts/build-windows.ps1`、`scripts/build-linux-arm64.sh`
- 图标：`icons/landrop.ico` / `landrop_app_icon.svg`
- 麒麟输入法：`docs/requirements/20260922-kylin-ime-chinese.md`
