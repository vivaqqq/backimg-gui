# 变更清单 CHANGELOG — v1.3.25（外壳 v1.3.25 + DLL v1.3.25）· 秒级轮换 + 目录自愈版

**发布日期**：2026-10-29
**基线**：backimg_GUI_v1.3.24（桌面原样保留，未改动）
**性质**：两项用户需求驱动 ——
① 【界面】定时轮换自定义时间输入框收窄 + 新增「分钟/秒钟」单位下拉（用户红/绿框）；
② 【DLL 修复】使用程序目录之外的自定义图片目录（如 G://MyPics）时
**偶发整窗无背景**的实机 bug（用户复现路径：设到 G: → 不显示；设回插件目录 →
恢复；再设回 G: → 又恢复。非必然、属瞬态）。

---

## 一、外壳（main.c / resource.rc / config.ini）

### D1【界面】轮换间隔秒级化 + 单位下拉

- **用户原话要点**：自定义时间框（红箭头）"稍微缩小一点，能显示全数字就行"；
  框后面（绿箭头）加一个框选"秒钟或分钟"；三个框大小优化、不挡前面内容。
- **数据模型**：`v_rotmin`（整数分钟）→ **`v_rotsec`（秒，唯一权威单位，0=不轮换，
  上限 86400=一天）** + `v_rotunit`（自定义态显示单位：0=分钟 1=秒钟）。
- **布局**（random 行右侧，从右往左）：
  - 档位下拉按钮不变（cx 起，宽 S(112)）；
  - 新增**单位下拉**「分钟/秒钟」：宽 S(56)，紧贴档位下拉左侧（cx-S(58)）；
  - 自定义输入框 **S(60)→S(42)**：只容 5 位数字（86400），贴在单位框左侧；
  - "定时轮换"说明文字自定义态左移到输入框左侧（cx-S(64) → cx-S(168)），
    非自定义态位置不变；均不侵入左侧开关文字区。
- **单位切换语义**（on_rotunit_menu / rot_ui_sync）：
  - 切「秒钟」：实际间隔秒数不变，只换显示；
  - 切「分钟」：仅当秒数是整 60 的倍数才切换显示；否则**向上取整到分钟并
    写日志"间隔已取整到 N 分钟"**（输入框是 ES_NUMBER 整数，"90 秒"没法用
    分钟如实显示，与其静默跳值不如明说）；
  - rot_ui_sync 显示回退一致性：v_rotunit=分钟但秒数非整分钟时，显示自动
    落回秒钟 **并回写 v_rotunit** —— 否则保存/EN_CHANGE 会把"90 秒"按
    分钟错算成 5400 秒（编码时发现的隐患，已修）。
- **预设档**：不轮换 / 每 1 分钟 / 每 2 分钟 / 每 5 分钟 / 每 10 分钟 /
  每 30 分钟 / 自定义…（选项不变，内部存秒 0/60/120/300/600/1800/-1）。
  选「自定义…」起点保持 3 分钟（180 秒），用户可立刻切秒钟。
- **EN_CHANGE 实时同步**（on_rotmin_edit_change 更名 on_rotval_edit_change）：
  按当前单位换算成秒，钳位 0~86400。
- **配置键**：
  - 新键 `randomVal`（秒）+ `randomUnit`（min/sec，仅面板回显用）；
  - 兼容键 `randomMin` 仍同步写入（= ceil(秒/60)，秒级间隔在旧 DLL 上按
    1 分钟轮换，功能不丢）；
  - 读取优先 randomVal，缺键回落 randomMin×60（老配置无缝升级）。
- **复制摘要**：`randomMin : N 分钟` 行改为 `randomVal : N 分钟（N 秒）`/
  `N 秒` / `不轮换`（on_copy_summary）。
- **资源/版本**：GUI_VERSION / DLL_VERSION = 1.3.25；resource.rc
  FILEVERSION 1,3,25,0；内嵌 config.ini 模板补三键注释。

## 二、DLL（dllmain.cpp）

### D1【配置】轮换间隔秒级化

- `std::atomic<int> rotateMin` → `rotateSec`；RotateTimerProc / ApplyRotateTimer
  周期改 `秒 × 1000ms`（SetTimer 上限内，一天=86,400,000ms < 0x7FFFFFFF）。
- LoadSettings 解析：randomVal（秒）优先，randomMin×60 兼容；钳位 0~86400。
- 轮换签名（ApplyRotateTimerAll 的"变了才重装"）单位同步改为秒。
- 初始化日志/加载日志按可读性显示"每 N 分钟 / 每 N 秒"。

### D2【修复】自定义图片目录偶发不显示（三层防御 N1/N2/N3）

- **成因分析**（按 v1.3.24 代码路径逐条核对）：
  LoadSettings 是一次性判决：`IsDirExisting` 通过（GetFileAttributes 只证明
  元数据可达）→ 枚举 8 个后缀一个都没拿到句柄 → 清空 imageList 返回，
  **之后没有任何补救路径**——已开窗口不重载，只有下一个新开窗口才会再试；
  而"目录存在但 FindFirstFile 拿不到句柄"是真实瞬态（磁盘未就绪/重启后卷
  延迟挂载/杀软瞬时锁目录），正好解释用户"偶然不显示、改来改去又好了"。
- **N1 路径规整**：folder 去首尾空白/引号/多余尾反斜杠（盘根 "G:\" 保留），
  手改 ini 写成 `G://MyPics\` 不再产生判断歧义。
- **N2 枚举瞬态重试**：一个文件都没枚到且目录此刻连 `*` 都列不出（
  DirListable 探针）→ 原地重试 3×200ms 后重新枚举；**真·空目录 `*` 可列，
  走不到重试，不拖慢开窗**（最坏 600ms 只发生在异常路径）。
- **N3 空列表自愈重载**：以上仍失败 → `s_retryWanted=true` 并给全体登记
  窗口挂 3 秒一次性 `RETRY_TIMER_ID('BGT1')` 定时器
  （LoadRetryTimerProc → 重跑 LoadSettings(true)）；恢复后清掉所有窗口的
  重试定时器并 InvalidateRect 整体重绘；持续失败每 3 秒再试（一条
  GetFileAttributes + 一次枚举，开销可忽略）直到目录恢复。
  MyCreateWindowExW 登记新窗口时若列表为空且 s_retryWanted，同样先挂
  重试定时器——重启 explorer 后盘慢就绪的场景也自动恢复。
- **收尾对齐**：加载成功/文件集合未变 → s_retryWanted=false；解码后一张
  都没收进列表（文件恰好全部瞬时不可读）也按失败挂自愈。
- **异常边界**：LoadRetryTimerProc 全程 try/catch 吞异常，与 RotateTimerProc
  同纪律，绝不穿透回 explorer。

---

## 验证记录

| 项 | 结果 |
|---|---|
| DLL 编译 | g++ -std=c++17 -O2，零错误、仅既有 `#pragma comment` 良性警告（历版皆有） |
| EXE 编译 | gcc -std=c11 -O2 -Wall，零错误零警告 |
| 字节特征 | DLL 含 v1.3.25 / randomVal / "每 %d 秒"（UTF-16 校验）；EXE 版本资源 1.3.25.0 |
| 单位一致性 | rot_ui_sync 回写 v_rotunit 后，显示值×单位 ≡ 实际秒数（EN_CHANGE / 保存 / 回读三条路径同一套换算） |

## 指纹

- ExplorerBgTool.dll：**942,255 字节**，SHA256
  `B6A4966A772724977CFA4763E7182115E623425FE8941C3785EE8F0673152474`
- backimg图形板_v1.3.25.exe：1,260,178 字节，SHA256
  `6C42E44008C1870C1105D3B402A466B921A5B996E02CE246960A26D7CB4F3C7E`
