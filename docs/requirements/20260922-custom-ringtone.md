# 自定义通知铃声

- 状态：已实现（待手测）
- 背景：通知声只有系统 beep；有人希望用自己的短 wav 区分局域网消息与其它系统提示。
- 目标：设置里可选手动 wav 作为通知声；空路径继续用系统提示音；开关关则仍静音。不引入 Qt Multimedia。
- 非目标：
  - 音量滑条、mp3/ogg 全格式支持（本轮 wav）
  - 按事件区分不同铃声（收文字 / 收文件 / 发送完成）
  - 内置多套预设铃声包

## 用户故事

作为使用者，我希望给局域网消息换一个好认的提示音，也可随时恢复系统默认。

## 行为

1. 通知声开关下方：铃声路径输入框 +「浏览…」+「试听」+「恢复默认」。
2. 浏览：选 `.wav` 文件；路径写入设置键 `soundFile`（空=默认）。
3. 试听：按当前输入框路径播一次（开关关时也可试听，便于选文件）；无效路径则回落系统音并提示。
4. 播放：`soundNotification` 开且 `soundFile` 有效 → 播该 wav；否则系统 beep（Windows `MessageBeep` / 其它 `QApplication::beep`）。
5. Windows 用 `PlaySound` 异步播放；失败回落系统音。Linux 可异步尝试 `paplay`/`aplay`，失败回落 beep。
6. 「恢复默认」清空路径。

## 验收标准

- [ ] C1：选 wav 保存后，收文字/收文件/发文件完成可听到该铃声（开关开）。
- [ ] C2：清空路径或点恢复默认并保存后，回到系统提示音。
- [ ] C3：开关关时收发不响；试听仍可播当前路径。
- [ ] C4：无效路径回落系统音，不崩。
- [ ] C5：`--self-check` 覆盖 `soundFile` 读写往返。

## 影响范围

- `src/settings.*`、`src/mainwindow.cpp`、`lan-drop.pro`（winmm）
- `src/selfcheck.cpp`
- `docs/test/mvp-cases.md`
- 修订：`20260922-sound-notification.md`、`20260921-settings-dialog.md`

## 待确认

- 无

## 建议下一手

手测 C1–C4；下一轮可做并发线程数诚实化（灰显说明暂单队列）。
