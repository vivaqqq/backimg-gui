# 变更清单 CHANGELOG — v1.3.23（外壳 v1.3.23 + DLL v1.3.23）

**发布日期**：2026-10-29
**基线**：backimg_GUI_v1.3.22（桌面原样保留，未改动）
**性质**：Kimi 独立复审加固版 —— 用户切回 Kimi 大模型做了一次"全新视角、只看源码"的
独立审计。审计确认 v1.3.22 状态良好（DLL 侧零新发现、跨线程销毁加固被认定机制闭环），
新提出 5 处外壳/DLL 隐患。逐条核验**5 条全部属实**，全部修复（1 条 P2 内存安全 +
4 条 P3），另附审计报告未单列、但与其 P2 同源的一处初始 malloc 判空，一并修复。
**零界面改动、零配置变化**。明细如下，独立审计报告原文由用户留档。

---

## K1【P2 · 中 · 内存安全】remove_folder_section 的 APPENDW2 宏 realloc 未判空

- **位置**：外壳 main.c（原 v1.3.22 行号约 1116-1120）
- **问题**：`out = realloc(out, ...)` 直接覆盖原指针。内存不足时 realloc 返回 NULL：
  ① 原内存块丢失（泄漏）；② 紧接的 `memcpy(out + out_len, ...)` 变成空基址写入，
  直接崩溃。触发条件是点"删除规则"时恰好系统内存不足——概率低但路径真实存在。
  同一文件里 `ini_write_value` 的 APPENDW 宏有完整判空，唯独这里漏了（不对称缺陷）。
- **修复**：写法对齐 APPENDW——临时指针接 realloc、判空后 `free(out); return;`。
  失败即整体放弃：g_ini_text 原样保留，ini 内容不变（规则不删、不留半成品）。

## K1b【同源 · 审计未单列，一并修】remove_folder_section 初始 malloc 未判空

- **位置**：同函数开头
- **问题**：`WCHAR *out = malloc(64 * sizeof(WCHAR)); out[0] = 0;`——malloc 失败时
  立刻空指针写入。与 K1 同属"内存不足崩溃"一类，报告未单列但核验属实。
- **修复**：补 `if (!out) return;`。

## K2【P3 · 低】folded 折叠缓冲 malloc 未判空

- **位置**：同函数"折叠空行"阶段
- **问题**：`folded = malloc(...)` 失败返回 NULL 后，`folded[j++]` 空指针写入。
- **修复**：补 `if (!folded) { free(out); return; }`（同样整体放弃、g_ini_text 不动）。

## K3【P3 · 低】wWinMain：CreateMutexW 在 memset(&g_app) 之前，句柄被清零

- **位置**：外壳 main.c wWinMain 开头
- **问题**：先把互斥体句柄存进 `g_app.hMutex`，随后 `memset(&g_app, 0, sizeof(g_app))`
  把它清成 NULL——退出时 `if (g_app.hMutex) CloseHandle(...)` 被跳过，句柄泄漏。
  单实例锁功能本身不受影响（互斥体对象仍被进程持有），属逻辑顺序错误、后果轻微。
- **修复**：memset 挪到 CreateMutexW 之前（先清再建，顺序归位）。

## K4【P3 · 低 · 防御性】winsize.ini 窗口宽度 bw 无上限

- **位置**：外壳 main.c 读窗口尺寸处
- **问题**：bw 只校验 `>= 760`；手工把 winsize.ini 宽度改成超大数（如 99999999）
  会通过校验，造出异常巨窗。与项目"配置按不可信输入兜底"原则不一致。
- **修复**：补上限 `bw <= 7680`（8K 屏工作区宽度，余量充足）。

## K5【P3 · 低 · 边界】DLL MyBeginPaint：BeginPaint 返回 NULL 仍记录会话

- **位置**：DLL dllmain.cpp MyBeginPaint
- **问题**：BeginPaint 对无效窗口返回 NULL（窗口在 BeginPaint 前已被销毁等极罕见
  时序），此前 NULL 也会被记进登记表并加深会话深度；随后 MyFillRect 的
  `hDC == hDC` 匹配（NULL==NULL）会把异常现场当正常绘制处理。实测对 NULL DC 的
  GDI 调用只失败不崩，但属"异常状态当正常"的边界漏洞。
- **修复**：条件补 `hDC &&`——只有拿到有效 DC 才记录/加深会话。

---

## 设计取舍知悉（非 bug，未改动）

- **图集口径不一致**：外壳图片管理页白名单含 .webp，DLL 枚举不含（GDI+ 无 webp
  解码器）→ 目录里有 .webp 时外壳计 N 张、DLL 实际加载 N-1 张。审计自评已知取舍。
  暂不改动行为（改外壳计数会影响既有用户认知），留档知悉，未来可考虑在列表里
  给 .webp 标注"不支持"。

---

## 验证记录

| 项 | 结果 |
|---|---|
| 编译 | DLL / EXE 零错误、仅既有良性警告 |
| 字节特征 | DLL/EXE 含 v1.3.23、零 v1.3.22 残留；EXE 内嵌 DLL 字节比对确认为 v1.3.23 新版 |
| test_geometry / test_render / test_filter | 227/227、116/116、filter 全过 |
| test_bugs / test_follow / test_leak / test_race_loop / test_random | 全过 / 全过 / 通过 / 5 轮零错误 / 全部通过 |
| test_imageheader | 13/16（与 v1.3.20/21/22 历版逐字一致；ImageHeader.h 与上版 diff 为空，非本版引入） |

## 指纹

- ExplorerBgTool.dll：**936,501 字节**，SHA256 `8B111243652DFC18FA5261F416FBF0F2DEA8A214855785EBB1A36715677166EB`
- backimg图形板_v1.3.23.exe：1,251,835 字节，SHA256 `22783AF3C0DE8F8FDF55F44D78C4F441A8D4279EECE794A1295EA703232A6051`
