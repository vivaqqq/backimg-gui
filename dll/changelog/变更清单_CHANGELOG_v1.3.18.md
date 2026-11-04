# 变更清单_CHANGELOG_v1.3.18

发布日期：2026-10-28
基线：v1.3.17（桌面上原样保留，未改动）
性质：DLL bug 修复版（千问 3.8 Flash 审计报告核验后修复）+ 外壳界面小改

---

## 一、背景

v1.3.17 交付后，用千问 3.8 Flash 对 DLL 源码做了一轮内存安全审计，
产出《backimg_DLL内存安全审计报告_v1.3.17.txt》。本次对报告逐条
人工核验（逐行读源码对照），确认属实即修、不属实则书面驳回。
核验与修复由 WorkBuddy（GLM）完成。

## 二、核验结论总表

| 报告条目 | 核验结论 | 处理 |
| --- | --- | --- |
| P0-1 缩放缓存跨线程销毁 GDI 句柄 | **机制不成立**（详见第四节） | 不修，但其牵出的 detach 路径真问题已随 P1-4 修掉 |
| P0-2 缩放缓存键用 ownerId 导致重建后全部 miss | **部分属实**（"永不命中"说法夸大，重建后必 miss 属实） | 已修 |
| P1-3 GetScaled 失败分支漏 DeleteObject(hb) | **属实** | 已修 |
| P1-4 DllMain detach 加锁/delete + 钩子永不拆除 | **属实** | 已修（采用"钉住 DLL"方案） |
| 四-1 ClassFactory::AddRef 双写 g_cDllRef | **属实**，但报告建议的"单行改法"有陷阱（见第三节） | 已修（工厂改堆分配） |
| 四-2 SetSite 未判空 pUnkSite | **属实**（COM 卸载约定会调 SetSite(NULL)，原代码必崩） | 已修 |
| 四-3 t_myData 单槽，同线程第二窗口覆盖 | 属实，但与作者原版行为一致，非回归 | 不修，明示为已知限制 |
| 四-4 WriteInitLog CREATE_ALWAYS 截断日志 | 属实，但每进程只调一次，"每次会话重置"语义合理 | 不修 |
| 四-5 GetCurDllDir 未校验返回值/npos | 属实（防御性） | 顺手加固 |

## 三、修复明细（v1.3.18 实际改动）

### 1. GetScaled 失败分支句柄泄漏（P1-3）—— WinAPI.cpp
- `CreateCompatibleDC(NULL)` 失败的分支补 `DeleteObject(hb)`；
- `SelectObject` 返回值判空，失败时 `DeleteDC` + `DeleteObject` 后返回空。
- 意义：GDI 句柄紧张时这条路径最容易命中，命中后背景必然画不出来
  （故障自锁）；v1.3.17 自己声称修掉的"句柄涨到 9999"问题，
  这条是漏网之鱼。

### 2. 缩放缓存键从 ownerId 改为内容哈希（P0-2）—— WinAPI.cpp / WinAPI.h
- `ScaleCache::Find/Put/Drop` 的键、`Entry` 字段统一改为 `m_hash`
  （FNV-1a 64，构造时已算好）；ownerId 分配器（NextScaleOwnerId、
  g_ownerMx、m_ownerId）整套删除；
- `~BitmapGDI` 不再按 ownerId 清缓存：内容寻址下对象销毁不代表内容
  作废（用户增删别的图片导致列表重建时，同一内容的缩放结果对新对象
  依然有效），内存由 32MB LRU 预算管住；
- 效果：图片列表重建（文件集合任何增删）后缓存照常命中，
  不再整批重新解码+缩放；LRU 淘汰频率大幅下降。
- 说明：报告"每次重绘都 miss、缓存几乎永不命中"的说法不成立——
  LoadSettings 只在文件集合变化时才重建列表（有早退判断），
  两次重建之间缓存命中正常。真实影响是"每次改图集后各线程白重算
  一轮 + 旧条目占预算"，修复后归零。

### 3. DLL 卸载路径加固（P1-4）—— ShellLoader.cpp / dllmain.cpp
- `DllCanUnloadNow` 恒返回 `S_FALSE`：全 DLL 从不调用
  MH_DisableHook/MH_Uninitialize，6 个钩子跳板指向本 DLL 代码页，
  一旦被 FreeLibrary，explorer 任何一次 CreateWindowExW/BeginPaint/
  FillRect 都会跳进无映射内存。钉住 DLL 是 shell 扩展的常规务实做法，
  卸载只发生在进程退出路径；
- `DLL_PROCESS_DETACH` 分支瘦身：删掉 `lock_guard(g_listMx)` 和
  `ReleaseAllCaches()` 调用，只留不需要跨线程协调的
  `m_config.imageList.clear()`。原写法在 loader lock 里拿多层互斥锁
  并 delete 一串带析构链的对象（死锁风险）；且 ReleaseAllCaches 会把
  **仍存活线程**的缓存对象整个 delete，而那个线程的 thread_local
  指针还指着它 —— 下次 GetScaled 就是悬垂指针 UAF。这是报告 P0-1
  一节附带提到的、经核验**确实成立**的真问题。

### 4. ClassFactory 引用计数修复（四-1）—— ShellLoader.cpp / ShellLoader.h
- `AddRef` 第二行原来也递增 `g_cDllRef`（应为 `m_ref`），
  每次 AddRef/Release 循环 g_cDllRef 净涨 1；
- **重要**：不能按报告说的"单行改掉"了事 —— 工厂原是 static 对象，
  计数修好后 Release 归零时的 `delete this` 会落在静态存储上（UB 崩溃）。
  因此配套把 `DllGetClassObject` 改为堆分配工厂，引用计数闭环；
- 补 `virtual ~ClassFactory()`（消除 -Wdelete-non-virtual-dtor 警告
  及多态删除 UB）。

### 5. SetSite 判空（四-2）—— ShellLoader.cpp
- `pUnkSite` 为 NULL 时直接返回 S_OK（COM 卸载约定），
  不再对空指针做虚调用。

### 6. GetCurDllDir 防御加固（四-5）—— WinAPI.cpp
- GetModuleFileNameW 失败/截断、路径无反斜杠时返回空串，
  不再 substr(npos) 赌运气。

### 7. 外壳界面：移除状态栏「已提权/未提权」胶囊 —— main.c
- 该胶囊显示的是外壳进程自身的管理员状态，静态不变、与 DLL 注册
  状态无关，紧挨"DLL 未注册"极易误导（用户以为注册/提权出了问题）；
- 外壳版本号同步升至 1.3.18（GUI_VERSION / resource.rc / 内嵌 DLL
  同步替换为新版）。

## 四、为什么 P0-1 不修（核验驳回理由）

报告的核心断言是："线程 A 的缩放缓存条目可能被线程 B 同时取用，
A 淘汰时把 B 正在用的 GDI 句柄删了"。

经逐行核验，该时序在本代码里**走不通**：

1. 缩放缓存是 thread_local（`g_threadCachePtr`），`Find` 只查
   本线程自己的缓存 —— 线程 B 永远取不到线程 A 缓存里的条目；
   B 调 GetScaled 会创建 B 自己的 ScaledBitmap（句柄在 B 线程创建）。
2. `GetScaled` 返回的 shared_ptr 只交给同线程的调用方（MyFillRect
   局部变量），从不存在"另一个线程握着同一 ScaledBitmap 副本"。
3. MyFillRect 持有 shared_ptr 期间（AlphaBlend 前后），本线程不可能
   进入 GetScaled 触发 LRU 淘汰 —— 同线程内使用与淘汰天然串行。
4. 跨线程销毁的路径只有三条：清扫死线程缓存（死线程不在用）、
   线程销窗时释放自己的缓存（同线程）、DLL detach 时 ReleaseAllCaches
   （这条是真问题，已随 P1-4 修掉）。shared_ptr 引用计数保证
   在飞的使用者持有一份凭证，缓存被删只降计数不销对象。

即：报告描述的"跨线程使用已删除句柄"需要"另一线程持有同一缓存
条目的引用"这一前提，而缓存从不跨线程共享，前提不成立。
报告第五节也承认现有压测测不到它 —— 因为它本来就不会发生。

## 五、自检结果（v1.3.18 实测，MinGW-w64 g++ -O2）

| 测试 | 结果 | 备注 |
| --- | --- | --- |
| test_render（渲染/alpha/并发） | 通过 | 同尺寸新旧平均像素误差 1.33；alpha 误差 0.00 |
| test_leak（构造-缩放-销毁 × 3 轮） | 通过 | 第 2/3 轮增长 +0MB —— 哈希键缓存跨列表重建命中生效的直接证据 |
| test_race_loop（8 线程 × 300 轮 × 5 轮） | 通过 | ok=12000/12000，err=0，GDI 句柄 989~1601 锯齿稳定，无单调增长 |

（test_render 中"缩放后 alpha 分布"断言对本次所用测试图不适用：
该图为不透明截图，程序整体判定通过。）

## 六、涉及文件

- DLL源码/WinAPI.cpp、WinAPI.h、ShellLoader.cpp、ShellLoader.h、dllmain.cpp
- 外壳源码/main.c、resource.rc、ExplorerBgTool.dll（内嵌资源同步替换）
- build\ExplorerBgTool.dll、plugin\ExplorerBgTool.dll（同步替换）
