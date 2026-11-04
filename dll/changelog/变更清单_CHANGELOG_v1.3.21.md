# backimg v1.3.21 变更清单 —— Deepseek V4 Pro 审计修复（外壳 G1~G3 / DLL D1~D5）

- **发布日期**：2026-10-28
- **基线**：v1.3.20（桌面原样保留，未改动）
- **触发**：用户让 Deepseek V4 Pro 对 v1.3.20 做只读审计，产出 8 条发现
  （外壳 3 条 G1~G3、DLL 5 条 D1~D5）。逐条对照源码核验，**8 条全部属实**，
  按"属实即修"原则全部修复，无一驳回、无一折中。

---

## 一、GUI 外壳（3 项）

### G1【高优先级 · 用户实机报告】定时轮换下拉按钮面永远空白
- **现象**：下拉菜单里勾了"每 1 分钟"，但按钮面上什么字都不显示；选"自定义…"同样空白。
- **根因**：`rot_ui_init` 创建下拉按钮后**漏了一行句柄回填**——
  `g_cb_rot.btn = g_app.hwndRotBtn;` 没写。`combo_state_set` 里
  `if (cb->btn) SetWindowTextW(...)` 整句被跳过，按钮文字停留在创建时的空串。
  对照 `g_cb_clsid` / `g_cb_img` 两处均有回填，唯独 `g_cb_rot` 遗漏。
- **修复**：补上回填（main.c `rot_ui_init`），重编 EXE。

### G2【中优先级】自定义分钟数与摘要不同步
- **根因 1**：自定义输入框（ID_EDIT_ROTMIN）没挂 EN_CHANGE——手改数字时
  `v_rotmin` 不实时更新，只在点"保存配置"时才读一次。
- **根因 2**：`on_rot_menu` 选完档位后没调 `update_summary()`。
- **修复**：新增 `on_rotmin_edit_change()`（钳位 0~1440 写回 `v_rotmin`，
  与保存路径同一套解析；空串按 0，无 SetWindowText 回写故 EN_CHANGE 不成环），
  接入 WM_COMMAND 的 EN_CHANGE 分支；`on_rot_menu` 两个选档分支末尾补
  `update_summary()`（摘要面板"本次刷新"时间戳立即跟上）。
- **备注**：Deepseek 报告称"摘要面板里的 randomMin 行不会刷新"——实测
  摘要网格只有 9 行、**没有 randomMin 行**（该说法不准确）；本版不新增
  摘要行（加行需动右卡填满规划算式，布局回归风险大于收益），
  以时间戳刷新 + 实时变量同步兑现"选档即反馈"。

### G3【低优先级】保存后输入框不回写钳位结果
- **现象**：输入 9999 保存，落盘正确钳成 1440，但框里仍显示 9999，要重开程序才变对。
- **修复**：`save_config` 钳位后把最终值 SetWindowText 回写输入框（仅自定义态可见时）。

## 二、DLL（5 项）

### D1【中优先级】线程级缩放缓存与"按窗口记账"不匹配
- **根因**：缩放缓存是 thread_local（同线程多标签共享一份）。v1.3.20 把窗口
  记账改成按窗口登记后，`MyDestroyWindow` 注销**任一**窗口仍无条件
  `ReleaseThreadCache()`——把仍存活标签的共享缓存也清了，其它标签下次重绘
  要整图重解码+重缩放（性能退化，非崩溃）。
- **修复**：`MyData` 新增 `tid` 字段（建窗登记时记属主线程）；销毁时在
  `g_wndDataMx` 锁内扫一遍：本线程**已无其他登记窗口**才释放缓存，否则保留。
  锁纪律不变（g_wndDataMx 出作用域后才调 ReleaseThreadCache，与内部的
  g_registryMx 本就互不嵌套）。线程意外消失时的残留缓存仍由
  SweepDeadThreadCaches / ReleaseAllCaches 兜底回收。

### D2【低优先级】ApplyRotateTimerAll 每次开窗都全量重装计时器
- **根因**：每个窗口创建都会走 `LoadSettings(true)`，而它无条件调
  `ApplyRotateTimerAll()`——开一个新文件夹窗口会把所有窗口的轮换倒计时
  重置回整周期（注释说"读到配置变化后"，实际没判断）。
- **修复**：用"随机开 + 间隔分钟"拼成签名（不轮换=0），存 `static std::atomic<int>`
  与上次比较，**签名真变了才全量重装**；首次（-1）必装。新窗口自己的定时器
  由建窗登记路径 `ApplyRotateTimer` 负责，不受影响。

### D3【低优先级】RotateTimerProc 每跳无条件重设定时器
- **根因**：注释写"间隔被改过才重设"，实际每跳都 SetTimer（功能正确，纯冗余）。
- **修复**：`static std::atomic<int>` 记录上次间隔，不同才重设；周期初装由
  ApplyRotateTimer 负责，与回调共享同一配置值，最多多设一次，无正确性问题。

### D4【低优先级】RandomIndex 注释与实现不一致
- **根因**：注释写 GetTickCount64，实际代码用 GetTickCount（v1.3.20 编译期
  因工具链头文件未暴露该符号而改，注释没跟上）。回绕不影响播种质量（另有
  熵源兜底），仅注释不准。
- **修复**：注释对齐为 GetTickCount 并注明缘由（代码零改动）。

### D5【低优先级】RandomIndex((int)imgSize) 强转
- **根因**：图片列表尺寸是 size_t，调用点 `(int)` 强转在数量超过 INT_MAX 时
  属未定义行为（现实中受内存限制几乎不可能，属健壮性加固）。
- **修复**：WinAPI.h 新增 `inline int RandomIndex(size_t)` 重载（钳位
  INT_MAX 后转发 int 版本）；两处调用点（RotateTimerProc / MyCreateWindowExW）
  去掉强转直传 size_t。自检程序调用 int 版本，不受影响。

## 三、验证记录（全部实际执行）

| 项 | 结果 |
|---|---|
| DLL 编译 | MinGW g++ -std=c++17 -O2 -Wall 仅既有良性警告（#pragma comment），零错误 |
| EXE 编译 | MinGW gcc -std=c11 -municode -Wall，零错误（含前置声明补充：update_summary / on_rotmin_edit_change） |
| 字节特征 | DLL 含 v1.3.21 日志头（UTF-16 ×1）、零 v1.3.20 残留；EXE 含 v1.3.21、零 v1.3.20 残留；EXE 内嵌 DLL 经字节比对确认为 v1.3.21 新版 |
| test_random | 单线程 41 档 × 40000 次卡方 37.2（自由度 40，阈值 120，均匀）；边界 n≤1 通过；8 线程 × 20000 次零越界 |
| test_render | **116/116 全过**（v1.3.20 的 115/1 失败项系样图不透明；本版补了带 alpha 渐变的 RGBA 标准样图后真跑全过） |
| test_filter | **4/4 全过**（v1.3.20 的 1 FAIL 系样图缺失；补样图后真跑全过） |
| test_imageheader | 13/16（3 项失败与 v1.3.20 逐字一致；ImageHeader.h 与 v1.3.20 diff 为空，纯合成样本期望值差异，非本版引入） |
| test_geometry | 227/227 全过 |
| test_bugs / test_follow | 全过 |
| test_leak | 三轮重建列表内存 +0MB |
| test_race_loop | 8 线程 × 300 轮 × 5 轮 = 12000 次零错误，GDI 句柄有界 |
| 版本资源 | resource.rc FILEVERSION 修正为 1,3,21,0（v1.3.20 曾漏改停在 1,3,18）；字符串版本 1.3.21.0 |

## 四、指纹

- ExplorerBgTool.dll：**936,501 字节**，SHA256 `260828d95bf40795d5f73877c292dd549df607bc647d076b8eec0968b1dc49c0`
- backimg图形板_v1.3.21.exe：**1,251,835 字节**，SHA256 `bd41bdbf51811c9858c7d9f33aaf40e133e7a7c90f49aed0c54a1b4baed21185`

## 五、使用者须知

- 本版**零新功能、零界面布局改动**：G1 修复后"定时轮换"按钮面会正常显示
  当前档位文字（v1.3.20 的空白是确定性 bug）。
- 升级安装：`backimg图形板_v1.3.21.exe` 与 `plugin\ExplorerBgTool.dll`
  **成对覆盖**，保存配置后重启资源管理器。
- 多标签开合不再互相清缓存（D1），大图多标签场景重解码次数显著下降；
  开新窗口不再重置其它窗口的轮换倒计时（D2）。
- explorer 实机表现（下拉回显、定时轮换手感）需安装后由用户实测确认。
