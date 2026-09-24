# Windows / ARM 麒麟单文件便携版

- 状态：已交付（GitHub Release `v0.1.0`，可现场分发）
- 背景：现有产物为目录包（Windows `dist/windows-x64`；麒麟 `dist/linux-arm64` + `landrop.sh`）。希望只拷一个可执行文件即可运行。
- 目标：各平台交付**单个可执行文件**；Linux 侧在技术可行范围内**尽量自带库**；目标机为**有图形桌面**的 ARM 麒麟。
- 非目标：不做成 MSI/deb；不改变协议与业务；不做无桌面场景。

## 用户故事

作为出差/厂区用户，我希望只带一个文件到目标机就能打开局域快传，而不必解压目录或安装 Qt/依赖包。

## 「走天下」边界

| 依赖 | 单文件内 |
|------|----------|
| Qt / 插件 / 业务 so | 打包 |
| X11 客户端库（可选加深） | 当前仍可依赖系统；后续可再捆 |
| 中文字体 | 当前用系统字体；后续可再捆 |
| glibc | 不搬家；构建基线 2.31 |
| 图形桌面 / 内核 | 系统提供 |

## 功能需求

1. **Windows**：`scripts/pack-windows-portable.ps1` → `dist/windows-portable/landrop-portable.exe`（Enigma Virtual Box）。
2. **Linux arm64**：`scripts/pack-linux-appimage.sh`（WSL）→ `dist/linux-arm64-portable/landrop-aarch64.AppImage`。
3. 目录版保留；`README` 写清单文件用法。

## 验收标准

- [x] W1：Windows 单文件 `--self-check` 通过；本机可启动（无旁路 Qt DLL）。
- [x] W2：exe 含官方 ICO（目录版已 `RC_ICONS`；装箱继承输入 exe 图标）。
- [x] L1：甲方 ARM 麒麟桌面双击/执行 AppImage 可启动，文字/小文件互通（22 机已验）。
- [x] L2：AppImage 上 fcitx 中文输入可用（22 机已验）。
- [x] D1：`README` 已写目录版与单文件版路径及生成命令。
- [x] D2：Release `v0.1.0` 已挂两个便携产物，供现场下载。
## 生成命令

```powershell
powershell -File scripts\build-windows.ps1
powershell -File scripts\pack-windows-portable.ps1
```

```sh
sh scripts/build-linux-arm64.sh
sh scripts/pack-linux-appimage.sh
```

## 待确认

- [x] 有桌面麒麟；AppImage；WSL Ubuntu 20.04 交叉构建。
- [x] Windows：脚本调用 `enigmavbconsole.exe` 装箱。

## 关联

- `scripts/gen_enigma_evb.py`、`pack-windows-portable.ps1`、`pack-linux-appimage.sh`
- `icons/landrop.ico`、`landrop_app_icon.svg`、`landrop.png`
- `docs/requirements/20260922-kylin-ime-chinese.md`
