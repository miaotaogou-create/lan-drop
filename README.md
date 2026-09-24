# 局域快传

同一网段互传文字和文件。默认传输端口 **8848**（TCP），发现端口 **8850**（UDP）。

这是 Qt 桌面客户端：Windows 和 ARM 麒麟都是自己的窗口，不依赖浏览器，也不依赖 WebView2。界面和 Qt 库打在安装目录里，拷过去就能开。

## 怎么用

### 目录版（调试 / 完整目录）

Windows：打开 `dist/windows-x64`，双击 `landrop.exe`。

麒麟（aarch64）：把 `dist/linux-arm64` 整个目录拷过去，在目录里执行：

```sh
chmod +x landrop.sh landrop
./landrop.sh
```

### 单文件便携版（推荐分发）

Windows：拷贝 `dist/windows-portable/landrop-portable.exe`，双击即可（内嵌 Qt DLL，无需同目录其它文件）。

麒麟：拷贝 `dist/linux-arm64-portable/landrop-aarch64.AppImage`，在图形桌面下：

```sh
chmod +x landrop-aarch64.AppImage
./landrop-aarch64.AppImage
```

仍需本机已有图形桌面，以及 glibc ≥ 2.31（与目录版相同）。输入法用系统 fcitx/ibus。

中文使用系统里已有的字体（Windows 微软雅黑，麒麟文泉驿 / 思源 / Noto）。X11、fontconfig 用系统自带的，不另外安装 Qt。

## 编译

Windows（本机 Qt 5.14.2 msvc2017_64 + VS2017）：

```powershell
powershell -File scripts\build-windows.ps1
# 单文件便携（需已安装 Enigma Virtual Box）
powershell -File scripts\pack-windows-portable.ps1
```

ARM 麒麟（WSL Ubuntu 20.04，已装 `/opt/Qt5.14.2-host`、`/opt/Qt5.14.2-arm64` 和 aarch64 交叉编译器）：

```sh
sh scripts/build-linux-arm64.sh
# 单文件 AppImage
sh scripts/pack-linux-appimage.sh
```

工具链的 glibc 是 2.31，和银河麒麟 V10 SP1 一致。

## 防火墙

- TCP **8848**：文字和文件
- UDP **8850**：局域网发现

只手动填对方 IP 时，至少放行 TCP 8848。

## 网页共享

顶栏「网页共享 (HTTP)」选一个本机目录并开启后，同网段浏览器打开：

`http://本机IP:8848/share/`

可看顶层文件列表并下载。关掉共享即停止暴露。无鉴权，仅适合受信局域网。

## 说明

配置在用户配置目录的 `lan-drop/settings.json`（写不了则用程序旁边的 `data/settings.json`）。下载目录默认是程序旁边的 `downloads`。

协议说明见 `docs/design/mvp-architecture.md`。单文件便携需求见 `docs/requirements/20260924-portable-single-file.md`。
