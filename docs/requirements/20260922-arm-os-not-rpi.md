# ARM 桌面勿默认归为树莓派

- 状态：已实现（待部署 22 机后手测）
- 背景：22 机为 ARM 麒麟桌面，本机 `localOsTag()` 凡 ARM/aarch 一律报 `arm-linux`；列表头像又对 `arm` 子串优先映射树莓派，故显示「树」与芯片标。
- 目标：ARM 麒麟 / Ubuntu 等桌面报 `linux`，头像走 Ubuntu；仅树莓派或手动选「工控/树莓派」才用树莓派头像。
- 非目标：细拆所有发行版图标；改设备角标笔记本/手机/平板逻辑。

## 用户故事

作为使用者，我希望 ARM 麒麟测试机在列表里显示为 Ubuntu/Linux，而不是树莓派。

## 行为

1. 本机对外 `os`：Windows→`windows`；一般 Linux（含 ARM 麒麟/Ubuntu ARM）→`linux`；能识别为树莓派时→`arm-linux`。
2. 头像：`raspberry` / `rpi` / `arm-linux` → 树莓派；`linux` / `ubuntu` / `kylin` / `debian` → Ubuntu；不再因裸 `arm`/`aarch` 子串判树莓派。
3. 手动加节点「ARM64 Linux (工控/树莓派)」仍为 `arm-linux`。

## 验收标准

- [ ] O1：ARM 麒麟本机 `--self-check` 或 `/api/info` 的 os 为 `linux`（非树莓派机）。
- [ ] O2：对端列表中该机头像为 Ubuntu 系，不再是「树」。
- [ ] O3：手动选「工控/树莓派」仍为树莓派头像。

## 影响范围

- `src/files.cpp`（`localOsTag`）
- `src/mainwindow.cpp`（`avatarSvgForOs`）
- `docs/test/mvp-cases.md`
- 需重编并部署 22 机后对端才改上报

## 待确认

- 无

## 建议下一手

软件工程师改映射；重编 Windows + ARM 并部署 22。
