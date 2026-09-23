# 共享收声、空态统一与复制就地反馈

- 状态：已实现
- 背景：顶栏「网页共享 (HTTP)」偏长；共享弹窗「关闭窗口」黑底与全局脚栏脱节；右侧空态仍是 HTML 段落，与侧栏位图白卡两套皮；复制只有鼠标旁 toast，顶栏控件无就地反馈。
- 目标：共享文案收声；关闭钮改 ghost；右侧空态位图卡；复制后 pill/addr 短暂绿态。
- 非目标：改共享协议；重做空态引导文案业务含义。

## 用户故事

作为使用者，我希望：顶栏更干净；共享弹窗脚栏和设置一样；没设备时左右空态同一套卡；点复制后被点的控件自己也会亮一下。

## 行为

1. 共享钮：idle「网页共享」、active「共享中」；tooltip 保留 HTTP 说明。
2. 共享弹窗关闭：白底灰边 ghost（非黑底）；文案「关闭」。
3. 右侧空态：`renderMainEmptyHintHtml` 位图白卡（有/无搜索）；外层 `#emptyCardHost` 透明无第二层阴影。
4. `copyLocalAddr` / `copyPeerAddr`：控件 `copied=true` 约 1.2s（绿边/浅绿底），并保留迷你 toast。

## 验收标准

- [x] K1：顶栏共享文案无「(HTTP)」字样
- [x] K2：共享弹窗关闭钮为白底灰边
- [x] K3：未选设备时右侧为空态白卡（与侧栏同语言）
- [x] K4：点本机 pill / 对端地址后控件短暂变绿成功态

## 影响范围

- `src/mainwindow.cpp` / `mainwindow.h`
- `src/chatrender.cpp` / `chatrender.h`
- `docs/test/mvp-cases.md`（TC-162）

## 待确认

- 无
