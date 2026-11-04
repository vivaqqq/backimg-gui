# backimg DLL 变更清单 v1.3.18 → v1.3.19

- 审计来源：Kimi K3.1 Pro 只读复审报告（`Desktop\backimg_GUI_v1.3.18_DLL_Audit_Report.txt`）
- 处理原则：逐条对照 v1.3.18 源码核验，属实才修；修复不得引入新问题
- 结论：**14 条全部属实**（其中 P2-4 属"部分属实"，按折中方案处理），13 条代码修复 + 1 条文档化处理

## 一、核验结论总表

| 编号 | Kimi 报告内容 | 核验结论 | 处理 |
|---|---|---|---|
| P1-1 | GetSourceLocked 提前释放 IStream | **属实**（WinAPI.cpp FromStream 后立刻 Release，后面 DrawImage/delete decoded 仍在用该流；GDI+ 允许延迟解码，流必须活到位图销毁） | ✅ 已修 |
| P1-2 | 初始化普通 bool 竞态 + GdiplusStartup 返回值被忽略 | **属实**（多 STA 线程同时进 OnWindowLoad 时，B 可能在 A 刚置位、GDI+ 未启动时就直接解码） | ✅ 已修 |
| P1-3 | "缩放并填充"目标位图无上限、ScaleRound 可能 int 溢出（UB） | **属实**（100000x1 图 + 1920x1080 窗口 → 约 434GiB 分配请求） | ✅ 已修 |
| P1-4 | 原始字节无预算、bad_alloc 可穿透 COM/Hook 边界 | **属实**（BitmapGDI 构造按 ftell 直接 resize，单文件可达 2GB） | ✅ 已修 |
| P2-1 | MH_EnableHook 失败留下"半套已启用钩子" | **属实**（MinHook 批量启用逐项执行、不自动回滚） | ✅ 已修 |
| P2-2 | 配置标量与建窗/绘制线程数据竞争 | **属实**（isRandom/imgPosMode/imgAlpha 无锁读） | ✅ 已修 |
| P2-3 | 真实解码不检查 GetLastStatus/DrawImage 状态 | **属实**（与项目自己在 dllmain.cpp 记录的"MinGW 解码失败也返回非空指针"相矛盾） | ✅ 已修 |
| P2-4 | 32MB/64MB 预算对单条超预算条目不成立 | **部分属实**：机制上单条确实可超（预算是"预算+1 条"上界） | ⚠️ 折中（见第四节） |
| P2-5 | 图片目录失效后旧列表不清空，旧背景残留 | **属实**（与"目录为空"分支行为不一致） | ✅ 已修 |
| P2-6 | 非随机模式停在第一张坏图；切随机不重建列表 | **属实**（循环第一圈无条件 break；随机切换被文件集合早退条件拦截） | ✅ 已修 |
| P2-7 | 注册路径缓冲未初始化/返回值不看；反注册多一个反斜杠 | **属实**（截断时 wcslen 读越界；`\\{CLSID}` 虽通常被注册表 API 归一化，但两边写法必须一致） | ✅ 已修 |
| P3-1 | JPEG 探测器无 SOF 也返回 IMG_OK；缺 EOI 硬判死 | **属实**（超过 256 次 marker 扫描的合法 JPEG 被误杀，连 GDI+ 回退都没有） | ✅ 已修 |
| P3-2 | DllMain 里 std::wstring 操作可能在 loader lock 内抛异常 | **属实**（低内存时 bad_alloc = 送走宿主进程） | ✅ 已修 |
| P3-3 | QueryInterface 失败不清空输出参数 | **属实**（COM 契约要求） | ✅ 已修 |

## 二、修复明细

### P1-1 IStream 生命周期（WinAPI.cpp GetSourceLocked）
- `stream->Release()` 从 `FromStream` 之后挪到 `delete decoded` 之后
- 所有提前返回分支统一按"先删位图、后释放流"收尾（与构造探测路径的正确顺序对齐）
- 顺带补了 P2-3：`decoded->GetLastStatus()`、`Graphics::DrawImage` 后的 `GetLastStatus()` 都要检查，任一失败不得进缓存

### P1-2 初始化同步（dllmain.cpp）
- `m_isInitHook`/`s_gdiStarted` 两个普通 bool 删除，改为 `std::once_flag` + `std::atomic<bool> s_initDone`
- 初始化体抽成 `InitBackground()`，由 `std::call_once` 执行：其余线程阻塞等它完成，不再存在"看到置位但 GDI+ 未启动"的窗口
- `GdiplusStartup` 返回值检查：失败 → 调 `BitmapGDI::NotifyGdiplusStartupFailed()`，此后构造探测与 GetScaled 全部短路，背景功能整体停用，资源管理器不受影响

### P1-3 目标尺寸硬上限（DrawScale.h + WinAPI.cpp）
- `ScaleRound`：先钳位到 INT_MAX 以内再转 int，消除 double→int 溢出的 UB
- `ComputeDrawRect`：目标像素总量（w×h）超过 **6400 万像素**（32bpp 下 256MB）返回空矩形放弃绘制
- `GetScaled` 入口：64 位算术检查 dstW×dstH×4，超过 256MB 直接返回空
- 6400 万像素覆盖 8K 全屏（约 3300 万像素）仍有余量，正常壁纸零影响（test_extreme 已验证）

### P1-4 内存预算 + 异常边界
- BitmapGDI 构造：单文件原始字节上限 **256MB**（远超任何合理壁纸），超限按无效文件处理
- 异常边界（`try/catch(...)`）：`OnWindowLoad`、`MyCreateWindowExW`、`MyFillRect`、`DllGetClassObject`、`ClassFactory::CreateInstance`——任何 bad_alloc 不再穿透 COM/Hook 边界，就地吞掉（没有背景 ≠ explorer 崩溃）

### P2-1 启用失败回滚（dllmain.cpp InitBackground）
- `MH_EnableHook(MH_ALL_HOOKS)` 失败时：`MH_DisableHook(MH_ALL_HOOKS)` + 逐个 `MH_RemoveHook`，日志记录 Disable 状态——不再留下"BeginPaint 启用了、EndPaint 没有"的半套钩子

### P2-2 配置标量原子化（dllmain.cpp）
- `Config::imgPosMode/isRandom/imgAlpha` 改为 `std::atomic`，读侧显式 `.load()`，消除与建窗/绘制线程的数据竞争（UB）

### P2-5 目录失效清列表（dllmain.cpp LoadSettings）
- `imgPath.empty()` 分支与"目录为空"分支对齐：在 `g_listMx` 下清空 `m_config.imageList`——目录被删/断开后旧背景不再残留；目录恢复后能自动重新加载（imageList 已空，重建条件照样成立）

### P2-6 非随机坏图 + 随机切换（dllmain.cpp LoadSettings）
- 循环改为"成功收进第一张有效图后"才 break——排序后第一张是坏文件时继续找后面的好图
- 新增 `s_listIsFullSet` 状态：非随机=单图（非全量），随机=全量；用户从非随机切到随机时，即使文件集合没变也会强制重建全量列表

### P2-7 注册表（ShellLoader.cpp）
- `dllpath[MAX_PATH] = {0}` + 检查 `GetModuleFileNameW` 返回值（0 或截断即失败）
- CLSID 默认值、InProcServer32 DLL 路径两处关键写入检查返回值，失败返回 `SELFREG_E_CLASS`
- 反注册 BHO 路径的反斜杠与注册侧统一（旧写法 `Objects\\{CLSID}`）

### P3-1 JPEG 探测契约（ImageHeader.h）
- 没拿到有效 SOF 尺寸时返回 `IMG_UNKNOWN`（交 GDI+ 裁决），不再空手返回 `IMG_OK` 导致合法图片被误杀
- 缺 EOI（截断/带尾随数据）从 `IMG_SUSPECT` 放宽为 `IMG_UNKNOWN`——贯彻"解析器绝不单独否决一张图"

### P3-2 DllMain 纯 Win32 化（dllmain.cpp）
- 进程名判断改 `wcsrchr` + `_wcsicmp`，去掉 std::wstring 构造/erase/transform——loader lock 内零动态分配、零异常可能

### P3-3 COM 契约（ShellLoader.cpp）
- 两个 `QueryInterface`：入口判空（E_POINTER）、失败前 `*ppv = NULL`
- `CreateInstance`/`DllGetClassObject`：输出参数判空 + 异常边界（E_OUTOFMEMORY）

## 三、自检程序同步修复（Kim 报告第六节，不计入 DLL bug）
1. `test_render.cpp`：删掉让结果统计不可达的 `_exit(0)`，`g_fail` 现在如实决定退出码
2. `test_follow.cpp`：`return bad == 0 ? 0 : 1`（旧固定 0）
3. `test_race_loop.cpp`：累计 err 并按结果返回退出码（旧固定 0）
4. `test_follow.cpp`/`test_race_loop.cpp`：修掉"CP_ACP 算长度、CP_UTF8 转换"的码页错配（中文路径失真）
5. 新增 `test_extreme.cpp`：P1-3 回归——极端宽高比（100000x1、1x100000、999999x999999）×7 种模式全部不得超 6400 万像素上限，8K 常规填充不得被误杀

## 四、P2-4 的处理决策（如实说明）

Kimi 指出"单条超预算条目可长期驻留缓存，硬上界不成立"。机制属实，但**没有按它建议的"不放入缓存"修**，理由：

- 缩放结果/源位图不进缓存 → 每一帧重解码 + 重渲染（GetScaled 在每次 FillRect 都会被调到），
  大图大窗口场景 CPU 直接跑满——用 CPU 灾难换内存上界，得不偿失
- 实际采用：**单项尺寸封顶**（源位图 512MB 维度上限不变、缩放结果新增 256MB 面积上限），
  缓存上界 = 预算 + 单项最大值，最坏情况有界且已知（单线程 ≤ 512+256MB，按线程数放大同理有界）
- 文档（交付说明"已知行为"）如实写明这一语义，不虚标"硬上界"

## 五、验证记录（全部实际执行）

1. **编译**：MinGW g++ -Wall 零警告（仅既有的 MinGW `#pragma comment` 提示）；DLL 三处副本 SHA256 一致
2. **test_render**：115 通过 / 1 失败——失败项为样图本身是不透明截图导致的 alpha 分布断言（v1.3.18 起即如此，属测试图属性非代码缺陷）；新旧渲染误差 1.33 / alpha 0.00
3. **test_leak**：三轮重建列表，第 2/3 轮 +0MB——哈希键缓存命中正常，内存有界
4. **test_race_loop**：8 线程 × 300 轮 × 3 轮 = 7200 次缩放零错误，GDI 句柄锯齿波动无泄漏；退出码 0（本次起退出码是真实的）
5. **test_follow**：7 种模式 × 多窗口尺寸几何校验全过（无变形、无超窗），退出码 0
6. **test_extreme**（新增）：极端宽高比防护全过，8K 常规填充未被误杀
7. **EXE 字节特征**：含 v1.3.19、零 v1.3.18 残留；版本资源 FileVersion 1.3.19.0

## 六、指纹

| 文件 | 大小 | SHA256 前 32 位 |
|---|---|---|
| ExplorerBgTool.dll | 921,713 B | `8c3ef57614a11ba3767905ece70d8158` |
| backimg图形板_v1.3.19.exe | 1,233,842 B | `f38f03b4191fa5e17bf0a0a4031b2533` |
