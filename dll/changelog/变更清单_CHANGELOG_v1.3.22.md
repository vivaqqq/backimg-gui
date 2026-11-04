# backimg v1.3.22 变更清单 —— Deepseek 复审加固（R1 跨线程销毁 / R2~R4 低风险加固）

- **发布日期**：2026-10-29
- **基线**：v1.3.21（桌面原样保留，未改动）
- **触发**：用户让 Deepseek V4 Pro 对 v1.3.21 做只读复审。复审结论：v1.3.21
  的 8 处修复全部属实、落实到位，无回归；新发现 4 处"平时不发作"的隐性
  隐患。逐条核验**4 条全部属实**，按"能修则修"原则全部处理（1 条代码加固
  + 1 条魔数替换 + 2 条注释固化），行为对正常路径零变化。

---

## 一、核验更正（对复审报告的一处引用纠错，留档）

复审第 1 条称"Windows 本身允许跨线程 DestroyWindow（MSDN 明确如此）"——
**引用有误**：MSDN 实际明确规定 DestroyWindow 不能销毁其他线程创建的窗口
（跨线程调用失败返回错误）。但隐患机理**仍然成立**：本插件的钩子在真
DestroyWindow **之前**就执行记账逻辑，跨线程调用即使真销毁失败，注销/
清缓存/清会话也已经发生——污染照样造成。修复照做。

## 二、修复明细（DLL 4 项，外壳零改动）

### R1【中优先级 · 唯一实质加固】MyDestroyWindow 线程归属加固
- **隐患**：v1.3.21 的 D1 用【销毁时】的 `GetCurrentThreadId()` 去匹配
  登记表里【创建时】记下的 tid。跨线程销毁时（钩子已进入、真销毁未发生）：
  tid 全部匹配不上 → 误判"本线程已无窗口"；`ReleaseThreadCache()` 释放
  **销毁线程**的缓存（错对象）；`t_sessionHwnd/t_duiPaintDepth` 清的是
  **销毁线程**的 thread_local（错对象）。
- **修复**：以登记时的属主 tid 为准——
  - 属主 == 当前线程（正常路径，explorer 常规全走这条）：行为与 v1.3.21
    **逐字节一致**（同线程无其他登记窗口才释放缓存、清本线程会话）；
  - 属主 != 当前线程（异常路径）：只注销登记 + KillTimer，缓存与会话
    状态一概不动。孤儿缓存由 SweepDeadThreadCaches / ReleaseAllCaches
    兜底回收（机制现成，复审第 4 节已核验有效）。

### R2【低优先级】threadEmpty 快照的 TOCTOU 形态固化
- **隐患**：快照在 `g_wndDataMx` 锁内计算、锁外使用（"检查-使用"分离）。
  当前安全性依赖"同线程 Create/Destroy 走同一条消息队列、天然串行"。
- **处理**：不改为嵌套锁（会引入 g_wndDataMx → g_registryMx 嵌套，违背
  锁纪律），改为在 MyDestroyWindow 头部注释**固化该不变量**并警示后人。

### R3【低优先级】RandomIndex(size_t) 钳位魔数
- 2147483647u → `(size_t)INT_MAX`（WinAPI.h 加 `#include <climits>`），
  显式表达"钳到 int 上界"，不再依赖 32 位 unsigned 字面量。

### R4【低优先级】s_rotTimerCur 全局共享语义说明（非 bug，行为不变）
- 复审明确"不必改动"；按建议在 RotateTimerProc 注释固化：全局共享当前
  不会出错（rotateMin 是全局配置 + 配置变化全量重装），若未来间隔改为
  按窗口独立配置，此处必须随之改为按窗口记录。

## 三、验证记录（全部实际执行）

| 项 | 结果 |
|---|---|
| DLL 编译 | MinGW g++ -std=c++17 -O2 -Wall 零错误、仅既有良性警告 |
| EXE 编译 | MinGW gcc -std=c11 -municode -Wall 零错误 |
| 字节特征 | DLL 含 v1.3.22 日志头（UTF-16 ×1）、零 v1.3.21 残留；EXE 含 v1.3.22、零 v1.3.21 残留；EXE 内嵌 DLL 字节比对确认为 v1.3.22 新版 |
| test_geometry | 227/227 全过 |
| test_bugs / test_filter / test_follow / test_leak | 全过（leak 三轮 +0MB） |
| test_render | 116/116 全过（RGBA 标准样图） |
| test_race_loop | 8 线程 × 300 轮 × 5 轮 = 12000 次零错误 |
| test_random | 卡方均匀通过；边界 n≤1 通过；8 线程 × 20000 次零越界 |
| test_imageheader | 13/16（3 项失败与 v1.3.20/21 历版逐字一致；ImageHeader.h 与上版 diff 为空，非本版引入） |
| 样图环境 | 桌面 111222333 样图目录再度被清理（与代码无关），已重建带 alpha 渐变的 RGBA 标准样图（307 字节）后实测 |

## 四、指纹

- ExplorerBgTool.dll：**936,501 字节**，SHA256 `aff84abc49dea9214ff07f3bc13004fdb66176b4e127d5a867a5c9a35726516a`
- backimg图形板_v1.3.22.exe：**1,251,835 字节**，SHA256 `53e3982362c1573b1aa2add9913ec1ac07e070a5c9bf873131f0a88ceeb2c069`

## 五、使用者须知

- 本版**零界面改动、零配置变化**，正常使用与 v1.3.21 完全一致；改动只在
  DLL 的异常路径加固与注释。
- 升级安装：`backimg图形板_v1.3.22.exe` 与 `plugin\ExplorerBgTool.dll`
  **成对覆盖**，重启资源管理器。
- explorer 实机行为需安装后由用户实测确认。
