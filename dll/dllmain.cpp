/*
* 文件资源管理器背景工具扩展
*
* Author: Maple
* date: 2021-7-13 Create
* Copyright winmoes.com
*/
/*
* backimg 修复版 v1.3.15（新版本线：用户正在用的作者版 v1.3.14 的下一号）
* 本版内容 = 重建 v1.3.18 的全部 9 项修复 + 本版新增 2 项：
*   新增1 格式白名单：枚举层补 *.jpeg *.bmp *.gif *.tif *.tiff
*         （解码层 GDI+ 本来就认这些；webp 无解码器，加了白加；gif 取第一帧）
*   新增2 RestoreDC 配对：SaveDC 记牌号、按号精确恢复，不再用 -1 弹"最近一张"
* 版本号说明：DLL 自身历史号为 1.1.0.1(作者原版) → 1.3.16~1.3.18(修复线)，
*   v1.3.15 在 DLL 线上是空号，正好接在作者版 v1.3.14 之后。
* 下文注释里的 "v1.3.15/16/17/18" 字样是重建过程留下的历史记录，属旧版本线，
* 与本版本号无关。
* ------------------------------------------------------------------
* v1.3.18(审计修复)：本版修复千问审计报告确认属实的问题 ——
*   P1-3 GetScaled 失败分支漏 DeleteObject(hb)（HBITMAP 泄漏）
*   P0-2 缩放缓存键从 ownerId 改为内容哈希（列表重建后照常命中）
*   P1-4 DllCanUnloadNow 恒 S_FALSE 钉住 DLL；detach 分支去锁去 ReleaseAllCaches
*   第四节 ClassFactory::AddRef 双写 g_cDllRef（工厂改堆分配 + 计数修正）
*   第四节 SetSite 补 pUnkSite 判空；GetCurDllDir 补防御校验
*   注：报告中的 P0-1（跨线程销毁 ScaledBitmap）经核验机制不成立 ——
*   缓存是 thread_local，线程 B 永远取不到线程 A 缓存里的条目，
*   MyFillRect 持有的 shared_ptr 也足以保活到画完；其 detach 路径的
*   真实风险（ReleaseAllCaches 删活线程缓存 -> 悬垂指针）已随 P1-4 修掉。
* ------------------------------------------------------------------
* v1.3.19(审计修复)：本版修复 Kimi K3.1 Pro 复审报告确认属实的问题 ——
*   P1-1 GetSourceLocked 提前释放 IStream（GDI+ 延迟解码要求流活到位图销毁）
*   P1-2 初始化竞态：m_isInitHook/s_gdiStarted 普通 bool 改 once_flag+atomic；
*        GdiplusStartup 返回值不再忽略，失败即停用背景功能
*   P1-3 "缩放并填充"目标位图无上限：DrawScale/GetScaled 加像素总量硬上限
*   P1-4 原始字节无预算：单文件 256MB 上限；COM 导出/钩子回调加异常边界
*   P2-1 MH_EnableHook 失败后回滚（Disable+Remove 全部钩子）
*   P2-2 配置标量改 std::atomic，消除与建窗/绘制线程的数据竞争
*   P2-3 真实解码补 GetLastStatus/DrawImage 状态检查
*   P2-5 图片目录失效后清空旧列表（与"目录为空"分支行为对齐）
*   P2-6 非随机模式跳过坏图继续找有效图；随机切换后正确重建全量列表
*   P2-7 注册路径缓冲初始化+返回值检查；反注册路径反斜杠与注册侧统一
*   P3-1 JPEG 探测器：无 SOF 不再硬判 OK；缺 EOI 交给 GDI+ 裁决
*   P3-2 DllMain 进程名判断改纯 Win32（loader lock 内零动态分配）
*   P3-3 QueryInterface 失败时清空输出参数（COM 契约）
*   注：P2-4（单条超预算条目可长期驻留缓存）按"单项尺寸封顶 + 文档明示"
*   处理而非强制淘汰 —— 强制淘汰会让大图逐帧重解码重渲染，CPU 反而爆。
* ------------------------------------------------------------------
* v1.3.20(随机功能重做)：用户反馈"总觉得不太随机"，核验确认三个真实成因，
*   A 真随机源：rand()/srand(time(0)) 拆除，改 WinAPI 的 RandomIndex
*     （mt19937 + 熵源混种 + 互斥锁）—— 秒级重启不再复现同一条序列；
*   B 按窗口记账：t_myData 每线程单槽改全局 g_wndData（键=窗口句柄，
*     g_wndDataMx 保护）—— 多标签/同线程多窗口不再互相覆盖记录；
*     绘制会话用 thread_local t_sessionHwnd 串联，锁纪律与原有一致；
*   C 定时轮换：新配置 randomVal（秒，0=不轮换，钳位 86400；v1.3.25，
*             兼容旧键 randomMin 整数分钟）；
*     登记窗口挂 Windows 窗口定时器，到点 RotateTimerProc 重抽签 +
*     InvalidateRect 重绘；回调自纠错（随机关/间隔改自动跟随），
*     LoadSettings 变更后 ApplyRotateTimerAll 对全体窗口统一重装。
* ------------------------------------------------------------------
* v1.3.21(Deepseek V4 Pro 审计修复，5 项)：
*   D1 线程缓存销毁策略：同线程多标签共享一份 thread_local 缩放缓存，
*     MyDestroyWindow 注销任一窗口不再无条件整份释放 —— 仅当本线程
*     已无其他登记窗口才释放（MyData 新增 tid 字段支持判断）；
*   D2 LoadSettings 的 ApplyRotateTimerAll 改为"轮换配置签名变化才重装"，
*     开新窗口不再把全体窗口的轮换倒计时重置回整周期；
*   D3 RotateTimerProc 间隔没变就不再每跳重设 SetTimer（纯冗余消除）；
*   D4 RandomIndex 播种注释与实现对齐（GetTickCount64→GetTickCount）；
*   D5 RandomIndex 增加 size_t 重载（钳位 INT_MAX），调用点免强转。
* ------------------------------------------------------------------
* v1.3.22(Deepseek 复审加固，4 条复核属实)：
*   R1【中】MyDestroyWindow 线程归属加固：v1.3.21 的 D1 用"销毁线程"的
*     tid 匹配登记表里"属主线程"的 tid，跨线程销毁（钩子在真销毁前就
*     记账，即使真销毁失败也已污染）会错位释放缓存、清错会话状态。
*     现以登记时的属主 tid 为准：属主==当前线程（正常路径）行为不变；
*     跨线程只注销登记 + KillTimer，缓存与会话状态一概不动（兜底回收
*     由 SweepDeadThreadCaches / ReleaseAllCaches 承担）；
*   R2【低】threadEmpty 快照锁外使用的 TOCTOU 形态：注释固化"同线程
*     Create/Destroy 消息队列串行"不变量，防后人破坏；
*   R3【低】RandomIndex(size_t) 钳位魔数 2147483647u 改 (size_t)INT_MAX；
*   R4【低】s_rotTimerCur 全局共享语义注释固化（非 bug，行为不变）。
* ------------------------------------------------------------------
* v1.3.25(秒级轮换 + 目录自愈，配套 GUI v1.3.25)：
*   D1 轮换间隔秒级化（用户实测"1 分钟都嫌慢"）：权威单位改为秒，
*     新配置键 randomVal（秒）+ randomUnit（面板显示单位，DLL 不换算），
*     旧键 randomMin（分钟）×60 兼容读取；上限 86400 秒（一天）；
*   D2 自定义图片目录"偶发不显示"修复（用户实机：folder 指到
*     G://MyPics 偶发全无背景）：
*     N1 folder 路径规整（首尾空白/引号/尾反斜杠）；
*     N2 目录存在但枚举拿不到句柄（盘未就绪/杀软瞬时锁）→ 原地重试
*        3×200ms 再枚举；真·空目录走不到重试，不拖慢开窗；
*     N3 仍未恢复 → 全体登记窗口挂 3 秒一次性自愈重试定时器
*        （LoadRetryTimerProc 重跑 LoadSettings），恢复后清定时器并
*        整体重绘；持续失败每 3 秒再试直到目录恢复。
* ------------------------------------------------------------------
* v1.3.26(版本联动版，配套 GUI v1.3.26)：DLL 功能零改动，仅内部版本戳
*   1.3.25→1.3.26 与外壳联动升版。本轮全部改动在 GUI 侧 —— 用户红框：
*   自定义间隔输入框+单位框从 random 开关行挪到其下方专用子行，宽度放开。
*   配置键/协议/定时器行为与 v1.3.25 完全一致，可直接互换。
* ------------------------------------------------------------------
* v1.3.27(版本联动版，配套 GUI v1.3.27)：DLL 仍功能零改动，仅版本戳
*   1.3.26→1.3.27 随外壳联动。GUI 侧修复 Deepseek V4 Pro 审查三问题
*   （F1 子行标题随随机开关显隐 / F2 死变量清理 / F3 自定义态独立
*   状态位），见外壳总卷 v1.3.27 节。配置键/协议与 v1.3.26 完全一致。
* ------------------------------------------------------------------
* v1.3.28(版本联动版，配套 GUI v1.3.28)：DLL 仍功能零改动，仅版本戳
*   1.3.27→1.3.28 随外壳联动。GUI 侧修复 Deepseek 复审 v1.3.27 三问题
*   （P1 load_config 重置自定义态标志（「恢复配置」路径）/ P2 hRotLabel
*   死变量清除 / P3 已在自定义态重选「自定义…」不重置用户已填值），
*   见外壳总卷 v1.3.28 节。配置键/协议与 v1.3.27 完全一致。
* ------------------------------------------------------------------
*/
//v1.3.15: windows.h 必须先于 gdiplus.h（PROPID 等类型依赖），MinGW 编译必需
#include "framework.h"
#include <wtypes.h>   //v1.3.15: WIN32_LEAN_AND_MEAN 下 PROPID 所在的 wtypes.h 不会自动引入

#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <mutex>
#include <atomic>
#include <map>
#include <time.h>

//GDI 相关 Using GDI
#ifdef _MSC_VER
#include <comdef.h>   //v1.3.15: MSVC 专属头，代码未使用其功能，MinGW 下跳过
#endif
#include <gdiplus.h>
#pragma comment(lib, "GdiPlus.lib")

#include "MinHook.h"
#include "ShellLoader.h"
#include "WinAPI.h"
#include "HookDef.h"
#include "DrawScale.h"

//AlphaBlend
#pragma comment(lib, "Msimg32.lib")  

using namespace Gdiplus;

//全局变量
#pragma region GlobalVariable

HMODULE g_hModule = NULL;           //全局模块句柄 Global module handle

/* v1.3.19(审计修复 P1-2)：初始化一次性标志改为同步原语。
 * 旧写法 m_isInitHook 是普通 bool，多个 explorer STA 线程同时进
 * OnWindowLoad 时存在数据竞争：B 可能在 A 刚置位、还没 GdiplusStartup 时
 * 就看到 true 并直接去 LoadSettings(true) 解码图片（GDI+ 未启动）；
 * 也可能两线程同时通过检查造成重复初始化。现改为：
 *   call_once 保证初始化体全进程只跑一次（其余线程阻塞等它完成），
 *   s_initDone 保证初始化完成后大家走轻量路径。 */
static std::once_flag s_initOnce;
static std::atomic<bool> s_initDone{ false };

ULONG_PTR m_gdiplusToken;           //GDI初始化标志 GDI Init flag

/* v1.3.15 R5(复审 P3)：本结构保持平凡类型（纯标量）。
 * v1.3.20(B)：从 thread_local 单槽改为按【窗口句柄】全局登记后，
 * 它住在堆容器里，平凡性不再是硬要求，但保持纯标量依旧最稳。 */
struct MyData
{
    HWND hWnd = 0;
    HDC hDC = 0;
    SIZE size = { 0,0 };
    int ImgIndex = 0;
    /* v1.3.21(D1，Deepseek 审计)：登记窗口的属主线程 ID。
     * 缩放缓存是 thread_local（同线程多标签共享一份），销毁窗口时
     * 只有当本线程已无其他登记窗口，才允许释放这份共享缓存。 */
    DWORD tid = 0;
};
/* v1.3.20(B)【随机观感修复 · 按窗口记账】
 *
 * v1.3.15 R4 的"每线程一份 t_myData"在现代资源管理器上有覆盖问题：
 * Win10/11 的多标签窗口共用同一条 UI 线程，同一线程开第二个标签页时，
 * 新记录直接顶掉旧记录 —— 两个标签画同一张图，且旧标签正在用的
 * 下标被换掉。多开窗口时"翻来覆去就那几张"的观感，主因之一就是它。
 *
 * 现在改为全局登记表 g_wndData（键 = 窗口句柄），g_wndDataMx 保护：
 *  - 写：建窗登记（MyCreateWindowExW）、销毁注销（MyDestroyWindow）、
 *        绘制会话刷新 DC、轮换定时器改下标 —— 全是低频小改动；
 *  - 读：每帧绘制取记录 —— 锁内只做查找与标量拷贝，绝不在锁内
 *        解码/画图（与 g_listMx 的纪律一致）。
 * 两把锁（g_wndDataMx / g_listMx）在任何路径都不嵌套持有，无死锁面。
 *
 * 绘制会话仍用 thread_local 的 t_sessionHwnd 串联：BeginPaint~EndPaint
 * 天然发生在窗口自己的线程上，用它代替"每帧查表"，会话内后续
 * FillRect/CreateCompatibleDC 只需按它定位记录。 */
static std::mutex g_wndDataMx;
static std::map<HWND, MyData> g_wndData;      //DUI 窗口登记表（键 = 窗口句柄）
static thread_local HWND t_sessionHwnd = 0;   //本线程当前 DUI 绘制会话的窗口

/*
* v1.3.15 绘制会话守卫
* 老版本缺陷：MyFillRect / MyCreateCompatibleDC 仅凭 DC 句柄值判断是否属于
* 资源管理器 DUI 窗口。Windows 的 DC 句柄是池化复用的，explorer 单 UI 线程上
* 其他窗口（任务栏按钮、托盘等，尤其第三方任务栏重绘）的 DC 句柄值一旦与记录值
* 相同，背景图会被 AlphaBlend 错误刷进别人的绘制现场，且 RestoreDC 会弹出
* 对方的保存栈，造成图标绘制错乱（横条错位）。
* 修复：以 thread_local 标志严格限定 "BeginPaint(DUI窗口) ~ EndPaint" 绘制会话，
* 会话外一律不触碰任何 DC。
*
* v1.3.16 (B2)：从"布尔开关"改成"嵌套计数"。
* 老写法在 BeginPaint 里直接 t_inDuiPaint = isDui，有两处会误关会话：
*   1) DUI 会话中途若对别的窗口调 BeginPaint，开关被改成 false，剩下的背景不画了
*      → 实机表现就是"背景偶发缺一块、刷新又好了"；
*   2) 同一线程 DUI 重入 BeginPaint 时，第一个 EndPaint 就把会话整个关掉。
* 现在只增不减（按登记的 DUI 窗口匹配才增），EndPaint 按同一窗口匹配才减。
*/
static thread_local int t_duiPaintDepth = 0;   // DUI 绘制会话嵌套深度，>0 表示在会话内

//v1.3.16 (B1)：记录第一个创建失败的钩子序号，0 表示全部成功
int g_hookFailIndex = 0;
//v1.3.17：每个钩子的 MH_STATUS（0 = MH_OK），写进初始化日志，便于定位是哪一个失败
int g_hookStatus[7] = { 0 };

struct Config
{
    /* 0 = Left top
    *  1 = Right top
    *  2 = Left bottom
    *  3 = Right bottom
    *  4 = Center
    *  5 = Zoom
    *  6 = Zoom Fill
    */
    /* v1.3.19(审计修复 P2-2)：三个标量改原子 —— LoadSettings 在自己的锁里
     * 写它们，但建窗（MyCreateWindowExW）和绘制（MyFillRect）线程是无锁读，
     * 普通 bool/int 的并发读写构成 C++ 数据竞争（UB）。atomic 后读侧零成本。 */
    std::atomic<int> imgPosMode{ 0 };   //图片定位方式 Image position mode type
    std::atomic<bool> isRandom{ true }; //随机显示图片 Random pictures
    std::atomic<BYTE> imgAlpha{ 255 };  //图片透明度 Image alpha
    /* v1.3.25(D1)：定时轮换间隔改用【秒】为权威单位（配套 GUI v1.3.25）。
     * 0 = 不轮换（默认）；有效范围 1~86400（一天），越界由解析处钳位。
     * 兼容：新键 randomVal（秒）缺失时回落读 randomMin（分钟）×60。
     * 原子：LoadSettings 写它时，轮换定时器回调在其他窗口线程无锁读。 */
    std::atomic<int> rotateSec{ 0 };
    /* v1.3.15 R3(复审 P0)：位图对象改为 shared_ptr 持有——绘制线程取出一份
     * "凭证"后，即使列表被另一线程整体换掉，对象也活到画完为止，
     * use-after-free 从机制上消除。换表由 g_listMx 保护（只握片刻，不在锁内画图）。*/
    std::vector<std::shared_ptr<BitmapGDI>> imageList;  //背景图列表 background image list
} m_config;                             //配置信息 config

/* v1.3.15 R3：保护 imageList 结构（重建侧换表 / 绘制侧取凭证，均只握片刻）*/
static std::mutex g_listMx;

/* ------------------------------------------------------------------
* v1.3.20(C) 定时轮换
*
* 每个登记过的 DUI 窗口挂一个 Windows 窗口定时器，到点在
* RotateTimerProc 里给该窗口重新抽一张图并 InvalidateRect 触发重绘。
* 要点：
*  1) TimerProc 跑在窗口属主线程（窗口定时器的 WM_TIMER 投递到窗口
*     所属消息队列），重抽签/重绘天然同线程，无跨线程窗口操作；
*  2) 自纠错：回调每次都读当前配置 —— 随机被关/间隔清零就自拆定时器；
*     间隔被改就按新周期重设。于是"面板改配置"不需要跨线程碰别的
*     窗口的定时器也能最终生效（下次到点自动对齐）；
*  3) ApplyRotateTimer 供两处调用：建窗登记时按当前配置装/不装；
*     LoadSettings 读到配置变化后对全体登记窗口统一重装（跨线程
*     SetTimer/KillTimer 对窗口定时器是安全的，见 MSDN 对
*     "timer is associated with the window" 的定义）。
* ------------------------------------------------------------------ */
static constexpr UINT_PTR ROTATE_TIMER_ID = 0x42475430;   //'BGT0'

void CALLBACK RotateTimerProc(HWND hWnd, UINT msg, UINT_PTR idEvent, DWORD dwTime)
{
    /* 容器操作可能分配内存，任何异常都不得穿透回 explorer 的消息循环 */
    try
    {
        /* 1) 自纠错：当前配置已不需要轮换 → 自拆定时器 */
        if (!m_config.isRandom.load() || m_config.rotateSec.load() <= 0)
        {
            KillTimer(hWnd, idEvent);
            return;
        }
        /* 2) 间隔被改过 → 按新周期重设（同 id 重设即改周期，回调不变）。
         *    v1.3.21(D3，Deepseek 审计)：v1.3.20 每跳都 SetTimer 一次，
         *    功能正确但纯属冗余；现在只在全局间隔与上次不同时才重设。
         *    周期初装由 ApplyRotateTimer 负责，与这里共享同一配置值，
         *    s_rotTimerCur 滞后一跳也只会多设一次，无正确性问题。
         *    v1.3.22（Deepseek 复审第 4 条，语义说明非 bug）：s_rotTimerCur
         *    是全局一份的"上次间隔"，按窗口语义本应每窗口各自记；因
         *    rotateSec 是全局配置且配置变化时 ApplyRotateTimerAll 全量
         *    重装，全局共享不会出错——多窗口并发下最多多一次冗余
         *    SetTimer（无害）。若未来间隔改为按窗口独立配置，此处必须
         *    随之改为按窗口记录。
         *    v1.3.25(D1)：周期单位分钟→秒（SetTimer 直接吃毫秒）。 */
        static std::atomic<int> s_rotTimerCur{ -1 };
        const int rs = m_config.rotateSec.load();
        if (s_rotTimerCur.exchange(rs) != rs)
            SetTimer(hWnd, idEvent, (UINT)rs * 1000U, RotateTimerProc);

        /* 3) 只有一张图没得换；列表为空同样无事可做 */
        size_t n = 0;
        {
            std::lock_guard<std::mutex> lk(g_listMx);
            n = m_config.imageList.size();
        }
        if (n < 2)
            return;

        /* 4) 重抽签（v1.3.20(A)：高质量随机源）并写回登记表
         *    v1.3.21(D5)：RandomIndex 有 size_t 重载，n 免强转免溢出 */
        int idx = RandomIndex(n);
        {
            std::lock_guard<std::mutex> lk(g_wndDataMx);
            auto it = g_wndData.find(hWnd);
            if (it != g_wndData.end())
                it->second.ImgIndex = idx;
        }

        /* 5) 强制整窗重绘，让新图立刻可见 */
        InvalidateRect(hWnd, 0, TRUE);
    }
    catch (...)
    {
        /* 吞掉：这轮不换图而已，绝不能把异常送进 explorer 的消息循环 */
    }
}

static void ApplyRotateTimer(HWND hWnd)
{
    const bool on = m_config.isRandom.load() && m_config.rotateSec.load() > 0;
    if (on)
        SetTimer(hWnd, ROTATE_TIMER_ID,
            (UINT)m_config.rotateSec.load() * 1000U, RotateTimerProc);
    else
        KillTimer(hWnd, ROTATE_TIMER_ID);
}

/* 配置变更后对全体登记窗口统一重装定时器（LoadSettings 调用） */
static void ApplyRotateTimerAll()
{
    std::vector<HWND> hs;
    {
        std::lock_guard<std::mutex> lk(g_wndDataMx);
        hs.reserve(g_wndData.size());
        for (auto& kv : g_wndData)
            hs.push_back(kv.first);
    }
    for (HWND h : hs)
        ApplyRotateTimer(h);
}

#pragma endregion

extern bool InjectionEntryPoint();      //注入入口点
extern void LoadSettings(bool loadimg); //加载dll设置

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH && !g_hModule) {
        g_hModule = hModule;
        DisableThreadLibraryCalls(hModule);

        //v1.3.15 修订(审计 4.1/4.2)：进程门控 + 长度守卫
        //原写法 path.substr(path.length()-12, 12) 在宿主进程路径短于 12 字符时
        //size_t 下溢抛 std::out_of_range——在 DllMain 里抛异常等于送走宿主进程；
        //且本 DLL 原本在任何进程中都驻留不卸载。现在：
        //  只放行 explorer.exe（真正服务对象）和 regsvr32.exe（注册/反注册），
        //  其余进程返回 FALSE 让系统立即卸载本 DLL，不留驻、不初始化；
        //  ::tolower 作用于 wchar_t 属未定义行为，改 ::towlower。
        /* v1.3.15 R3(复审 P2)：按最后一个反斜杠取文件名做全名比较，
         * 去掉"12 字符"魔法数（explorer.exe/regsvr32.exe 恰好 12 字符属巧合正确）
         * v1.3.19(审计修复 P3-2)：改用纯 Win32（wcsrchr + _wcsicmp）完成比较。
         * 旧写法在这里构造 std::wstring、erase、transform —— 都是可能分配内存
         * 并抛异常的操作，而 DllMain 持 loader lock，低内存时抛异常等于送走宿主。 */
        wchar_t pName[MAX_PATH] = { 0 };
        DWORD nLen = GetModuleFileNameW(NULL, pName, MAX_PATH);
        bool allow = false;
        if (nLen > 0 && nLen < MAX_PATH) {
            const wchar_t* name = wcsrchr(pName, L'\\');
            name = name ? name + 1 : pName;
            allow = (_wcsicmp(name, L"explorer.exe") == 0
                  || _wcsicmp(name, L"regsvr32.exe") == 0);
        }
        if (!allow)
            return FALSE;       /* 非目标进程：拒绝驻留，系统随即卸载本 DLL */
        InjectionEntryPoint();  /* explorer 里真正初始化；regsvr32 只是借道注册 */
    }
    else if (ul_reason_for_call == DLL_PROCESS_DETACH)
    {
        /* v1.3.18(审计修复 P1-4)：detach 分支瘦身。
         *
         * 原写法在这里拿 g_listMx 锁、调 ReleaseAllCaches()（内部还要拿
         * g_registryMx 和 GDI+ 串行锁、delete 一串带析构链的对象）——
         * 这些都是 loader lock 期间不支持的操作：若卸载时另一线程正好
         * 持有同一把锁，卸载路径直接死锁（关机/注销时 explorer 卡住）。
         * 更糟的是 ReleaseAllCaches 会把【仍存活线程】的缓存对象整个
         * delete 掉，而那个线程的 thread_local 指针还指着它 —— 下次
         * GetScaled 就是悬垂指针 UAF。
         *
         * 现在的配套前提：DllCanUnloadNow 恒返回 S_FALSE（见 ShellLoader.cpp），
         * 本 DLL 钉死在进程里，detach 只可能发生在【进程退出】——
         * 那时其他线程已被系统终止、GDI 句柄随进程回收，任何清理都没有
         * 意义也没有风险。这里只做不需要跨线程协调的 imageList 清空
         * （shared_ptr 析构，持凭证者不受影响），锁也不必拿了：
         * 进程退出路径上不会再有并发的持锁者。 */
        m_config.imageList.clear();
    }
    return TRUE;
}

bool InjectionEntryPoint()
{
    /* v1.3.20(A)：原 srand((int)time(0)) 拆除。
     * 随机源已整体换成 WinAPI.cpp 的 RandomIndex（mt19937，random_device
     * + 系统计数混种，进程内只初始化一次，与这里的调用时机无关）。
     * 旧写法的毛病：种子只精确到秒 —— explorer 秒级重启会拿到同一个
     * 种子，整条"随机"序列逐字复现，这正是"感觉不随机"的来源之一。 */
    return true;
}

/* ------------------------------------------------------------------
* v1.3.18【白屏真正的根因】
*
* v1.3.17 修的是"钩子失败→二次 MH_Initialize→FreeLibraryAndExitThread"这条，
* 但用户实机日志证明 6 个钩子全部 MH_OK —— 那条路径根本没走到。
* 真正让资源管理器白屏的是本函数里的两处 MessageBoxW：
*
*   LoadSettings 是在 explorer 的 UI 线程上被调用的。图片目录不存在/为空时
*   弹一个模态框，这条 UI 线程就卡在对话框的消息循环里，此后所有文件夹窗口
*   都画不出来（进程没死、任务管理器看不到崩溃、事件查看器无记录）。
*   强制重启 explorer 时那个框一闪而过 —— 与用户描述完全吻合。
*
* 结论：在别人的 UI 线程里，任何模态框都是致命的。本版把 LoadSettings 里
* 所有 MessageBox 全部拆掉，一律只写日志；缺目录/缺图 = 不画背景，仅此而已。
* ------------------------------------------------------------------ */

/* v1.3.18：安全整数解析。
* std::stoi 遇到 "abc" 会抛 std::invalid_argument，在 explorer 的 UI 线程上
* 没有任何人接这个异常 → 直接送走资源管理器。config.ini 是用户可以手改的文件，
* 必须当不可信输入处理。*/
static int SafeToInt(const std::wstring& s, int defVal)
{
    if (s.empty()) return defVal;
    wchar_t* end = NULL;
    long v = wcstol(s.c_str(), &end, 10);
    if (end == s.c_str()) return defVal;      /* 一个数字都没解析出来 */
    if (v > 1000000L) v = 1000000L;
    if (v < -1000000L) v = -1000000L;
    return (int)v;
}

static bool IsDirExisting(const std::wstring& p)
{
    if (p.empty()) return false;
    DWORD attr = GetFileAttributesW(p.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES) return false;
    return (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

/* 追加写日志（与初始化日志同一个文件，但不清空已有内容）。
* 去重策略：与上次写过的内容相同就跳过 —— 每开一个文件夹窗口都会走一次
* LoadSettings，不去重就会把日志灌满；但只要结果变了（比如用户补上了图片目录），
* 一定会在下一次窗口创建时记下来。*/
static void AppendLoadLog(const std::wstring& body)
{
    /* v1.3.15 修订(审计 4.7)：LoadSettings 可能被多线程同时调，去重状态加锁 */
    static std::wstring s_last;
    static std::mutex s_lastMx;
    std::lock_guard<std::mutex> lk(s_lastMx);
    if (body == s_last) return;
    s_last = body;

    std::wstring path = GetCurDllDir() + L"\\ExplorerBgTool_init.log";
    std::wstring text = L"\r\n[图片加载]\r\n" + body + L"\r\n";

    int need = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), (int)text.size(), NULL, 0, NULL, NULL);
    if (need <= 0) return;
    std::string utf8((size_t)need, '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.c_str(), (int)text.size(), &utf8[0], need, NULL, NULL);

    HANDLE h = CreateFileW(path.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ, NULL,
        OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return;     /* 目录不可写就放弃，绝不影响 explorer */
    DWORD wr = 0;
    WriteFile(h, utf8.data(), (DWORD)utf8.size(), &wr, NULL);
    CloseHandle(h);
}

/* ------------------------------------------------------------------
* v1.3.25(D2) 自定义图片目录"偶发不显示"修复
*
* 用户实机报告：custom=true 且 folder 指向程序目录之外（如 G://MyPics）
* 时，偶然整个背景不显示；改回插件目录 Image 再改回来又好了。
*
* 成因分析（按代码路径逐条核对）：
*   LoadSettings 是【一次性判决】：目录 GetFileAttributes 通过 → 枚举
*   8 个后缀 → 一个都没枚到 → 直接清空 imageList 返回，之后【没有任何
*   补救路径】——已开的窗口不重载（只有下一个新开窗口才会再走一遍），
*   而目录刚恢复就绪的前几秒里开窗仍可能再次枚举失败。
*   "目录存在但枚举拿不到句柄"是真实存在的瞬态：目录所在磁盘未就绪
*   （移动硬盘唤醒/重启后卷延迟挂载）、杀软瞬时锁目录等。此时
*   IsDirExisting 通过（元数据可达）而 FindFirstFile 全部失败。
*
* 修复分三层：
*   N1 路径规整 —— folder 去首尾空白/引号/多余尾反斜杠，杜绝手改 ini
*      时因尾随反斜杠导致的判断歧义；
*   N2 枚举瞬时失败原地重试 —— 目录"存在"但连 "*" 都列不出来时，
*      短暂重试（3×200ms）后再枚举；真·空目录 "*" 可列，走不到重试，
*      不拖慢开窗；
*   N3 空列表自愈重载 —— 以上仍失败时，给全体登记窗口挂 3 秒一次性
*      重试定时器，到点重跑 LoadSettings(true)，恢复后清定时器并整体
*      重绘。持续失败则每 3 秒再试（一条 GetFileAttributes + 一次枚举，
*      开销可忽略），直到目录恢复为止。
* ------------------------------------------------------------------ */

static constexpr UINT_PTR RETRY_TIMER_ID = 0x42475431;   /* 'BGT1' */

/* N3：true = "配置了自定义目录但本次没加载出图"，需要安排自愈重试。
 * 原子：LoadSettings 写、定时器回调与建窗路径无锁读。 */
static std::atomic<bool> s_retryWanted{ false };

void CALLBACK LoadRetryTimerProc(HWND hWnd, UINT, UINT_PTR idEvent, DWORD)
{
    KillTimer(hWnd, idEvent);
    try
    {
        if (!s_retryWanted.load()) return;
        LoadSettings(true);          /* 内部自带串行锁，可安全重入 */
        if (s_retryWanted.load())
        {
            /* 仍未恢复：3 秒后再试（本窗口持续兜底，直到目录恢复） */
            SetTimer(hWnd, idEvent, 3000U, LoadRetryTimerProc);
            return;
        }
        /* 恢复了：清掉其它窗口的重试定时器并整体重绘 */
        std::vector<HWND> hs;
        {
            std::lock_guard<std::mutex> lk(g_wndDataMx);
            hs.reserve(g_wndData.size());
            for (auto& kv : g_wndData) hs.push_back(kv.first);
        }
        for (HWND h : hs)
        {
            KillTimer(h, RETRY_TIMER_ID);
            InvalidateRect(h, 0, TRUE);
        }
    }
    catch (...) { /* 同 RotateTimerProc：异常绝不穿透回 explorer */ }
}

/* 给全体登记窗口挂 3 秒一次性自愈重试定时器（LoadSettings 失败路径调用） */
static void ArmLoadRetryAll()
{
    std::vector<HWND> hs;
    {
        std::lock_guard<std::mutex> lk(g_wndDataMx);
        hs.reserve(g_wndData.size());
        for (auto& kv : g_wndData) hs.push_back(kv.first);
    }
    for (HWND h : hs)
        SetTimer(h, RETRY_TIMER_ID, 3000U, LoadRetryTimerProc);
}

/* N1：路径规整 —— 去首尾空白与引号、去多余尾反斜杠（盘根 "G:\" 保留） */
static std::wstring NormalizeDirPath(const std::wstring& in)
{
    size_t b = in.find_first_not_of(L" \t\"");
    if (b == std::wstring::npos) return std::wstring();
    size_t e = in.find_last_not_of(L" \t\"");
    std::wstring p = in.substr(b, e - b + 1);
    while (p.size() > 3 && (p.back() == L'\\' || p.back() == L'/'))
        p.pop_back();
    return p;
}

/* N2 探针：目录此刻能否真的列出内容（IsDirExisting 只证明元数据可达） */
static bool DirListable(const std::wstring& dir)
{
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW((dir + L"\\*").c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return false;
    FindClose(h);
    return true;
}

void LoadSettings(bool loadimg)
{
    /* v1.3.15 修订(审计 4.3)：多个窗口线程可能同时进入，重建过程串行化，
     * 避免"两个线程同时 delete+重建 imageList"的二次释放。*/
    static std::mutex s_loadMx;
    std::lock_guard<std::mutex> lk(s_loadMx);

    //加载配置 Load config
    std::wstring dllDir = GetCurDllDir();
    std::wstring cfgPath = dllDir + L"\\config.ini";
    m_config.isRandom = GetIniString(cfgPath, L"image", L"random") == L"true" ? true : false;

    //图片定位方式（v1.3.18：改用 SafeToInt，脏值不再抛异常）
    m_config.imgPosMode = SafeToInt(GetIniString(cfgPath, L"image", L"posType"), 0);
    if (m_config.imgPosMode < 0 || m_config.imgPosMode > 6)
        m_config.imgPosMode = 3;

    //图片透明度
    int alpha = SafeToInt(GetIniString(cfgPath, L"image", L"imgAlpha"), 255);
    if (alpha > 255) alpha = 255;
    if (alpha < 0) alpha = 0;
    m_config.imgAlpha = (BYTE)alpha;

    /* v1.3.25(D1)：定时轮换间隔改秒级。
     * 新键 randomVal 单位就是秒（GUI v1.3.25 落盘），randomUnit 只是面板
     * 回显用的显示单位，DLL 不参与换算；旧配置没有 randomVal 时回落读
     * randomMin（分钟）×60。一律按不可信输入处理（SafeToInt）并钳位。 */
    int rotSec = 0;
    {
        std::wstring val = GetIniString(cfgPath, L"image", L"randomVal");
        if (!val.empty())
            rotSec = SafeToInt(val, 0);
        else
            rotSec = SafeToInt(GetIniString(cfgPath, L"image", L"randomMin"), 0) * 60;
    }
    if (rotSec < 0) rotSec = 0;
    if (rotSec > 86400) rotSec = 86400;   /* 一天 */
    m_config.rotateSec = rotSec;

    /* v1.3.20(C)：轮换配置可能变了 —— 对全体登记窗口统一重装定时器。
     * 带 hWnd 的 SetTimer/KillTimer 挂在目标窗口自己的消息队列上，
     * TimerProc 始终在窗口属主线程触发，这里跨线程调用是安全的。
     * v1.3.21(D2，Deepseek 审计)：OnWindowLoad 每开一个窗都会进这里，
     * v1.3.20 无条件全量重装，等于开新文件夹窗口会把所有窗口的轮换
     * 倒计时重置回整周期（注释说"读到配置变化后"，实际没判断）。
     * 现在用"随机开 + 间隔秒数"拼成一个签名，配置真变了才重装；
     * 新窗口自己的定时器由建窗登记路径 ApplyRotateTimer 负责，不受影响。 */
    {
        /* 签名：不轮换 = 0，轮换 = 间隔秒数（isRandom 关同样归 0） */
        const int sig = (m_config.isRandom.load() && rotSec > 0) ? rotSec : 0;
        static std::atomic<int> s_lastRotSig{ -1 };   /* -1 = 首次必装 */
        if (s_lastRotSig.exchange(sig) != sig)
            ApplyRotateTimerAll();
    }

    //加载图像 Load Image
    if (!loadimg) return;

    /* v1.3.18：图片目录按以下优先级解析
    * 1) config.ini 里 custom=true 且 folder 指向的目录真实存在
    *    —— 面板的「自定义图片目录」从这一版起才真正生效（老版本 DLL 根本不读这两个键）
    * 2) DLL 所在目录 \Image（上游默认布局）
    * 3) DLL 上一级目录 \Image（面板把图放在 exe 根目录时的兼容路径）
    */
    std::wstring imgPath;
    std::wstring srcDesc;

    const bool custom = (GetIniString(cfgPath, L"image", L"custom") == L"true");
    /* v1.3.25(D2) N1：folder 先做路径规整（去首尾空白/引号/多余尾反斜杠），
     * 手改 ini 写成 "G://MyPics\" 这类尾巴也不再造成判断歧义 */
    std::wstring folder = NormalizeDirPath(GetIniString(cfgPath, L"image", L"folder"));

    if (custom && IsDirExisting(folder)) {
        imgPath = folder;
        srcDesc = L"config.ini 自定义目录";
    }
    else if (IsDirExisting(dllDir + L"\\Image")) {
        imgPath = dllDir + L"\\Image";
        srcDesc = L"插件目录\\Image";
    }
    else if (IsDirExisting(dllDir + L"\\..\\Image")) {
        imgPath = dllDir + L"\\..\\Image";
        srcDesc = L"上一级目录\\Image（兼容路径）";
    }

    if (imgPath.empty())
    {
        /* v1.3.18：这里原来是 MessageBoxW("缺少图片目录") —— 白屏的直接原因 */
        AppendLoadLog(
            std::wstring(L"结果 : 没找到任何可用的图片目录，本次不加载背景。\r\n")
            + L"       资源管理器不受影响，不会弹任何窗口。\r\n"
            + L"       已查过 :\r\n"
            + L"         " + ((custom && !folder.empty()) ? folder : std::wstring(L"(config.ini 未配置 custom/folder)")) + L"\r\n"
            + L"         " + dllDir + L"\\Image\r\n"
            + L"         " + dllDir + L"\\..\\Image\r\n"
            + L"       处理办法 : 在面板里指定图片目录，或把图片放进上面任意一个路径。\r\n"
            /* v1.3.25(D2) N3：自定义目录配置了但此刻不可达（盘未就绪等瞬态）
             * → 挂 3 秒自愈重试，目录恢复后自动补上背景，无需重启/开新窗口 */
            + (custom && !folder.empty()
               ? L"       提示 : 自定义目录已配置，将每 3 秒自动重试，目录就绪后背景自动恢复"
               : std::wstring(L"")));
        /* v1.3.19(审计修复 P2-5)：目录失效要和"目录为空"分支一样清掉旧列表，
         * 否则目录被删/断开后旧背景一直残留，用户看到的是旧图而不是"没有可用目录"。
         * 注：s_lastFiles 不用清 —— 目录恢复后枚举结果与旧集合一致，
         * 但 imageList 已空，重建条件照样会成立，行为正确。 */
        { std::lock_guard<std::mutex> lk(g_listMx); m_config.imageList.clear(); }
        s_retryWanted.store(custom && !folder.empty());
        if (s_retryWanted.load()) ArmLoadRetryAll();
        return;
    }

    /* v1.3.25(D2)：枚举收进 lambda —— N2 瞬态重试后要整段重来 */
    std::vector<std::wstring> fileList;
    auto enumAll = [&imgPath, &fileList]()
    {
        fileList.clear();
        EnumFiles(imgPath, L"*.png", fileList);
        EnumFiles(imgPath, L"*.jpg", fileList);
        /* v1.3.15(修复版)：原白名单只有 png/jpg，而解码层 GDI+ 本来就认识
         * bmp/gif/jpeg/tiff（此前已实测）。FindFirstFile 对长文件名不做跨后缀
         * 匹配（*.jpg 匹配不到 .jpeg），所以要逐个补齐。
         * 但 *.tif 会经由 .tiff 的 8.3 短名（G~1.TIF）撞上同一个文件——
         * 实测 .tiff 被报两次、解码两份白占内存，因此枚举完必须去重。
         * webp 不补：GDI+ 没有解码器。gif 只出第一帧。*/
        EnumFiles(imgPath, L"*.jpeg", fileList);
        EnumFiles(imgPath, L"*.bmp", fileList);
        EnumFiles(imgPath, L"*.gif", fileList);
        EnumFiles(imgPath, L"*.tif", fileList);
        EnumFiles(imgPath, L"*.tiff", fileList);
        /* v1.3.15(修复版)：去重（8.3 短名导致 *.tif 与 *.tiff 对 .tiff 文件双报）*/
        std::sort(fileList.begin(), fileList.end());
        fileList.erase(std::unique(fileList.begin(), fileList.end()), fileList.end());
    };
    enumAll();

    /* v1.3.25(D2) N2：一个文件都没枚到，且目录此刻连 "*" 都列不出来
     * → 不是"没图"，是"目录暂时不可枚举"（磁盘未就绪/卷延迟挂载/
     * 杀软瞬时锁）。原地短暂重试（3×200ms，只发生在这种异常路径，
     * 正常目录零延迟）；真·空目录 "*" 可列，直接走后面正常分支。 */
    if (fileList.empty() && !DirListable(imgPath))
    {
        for (int i = 0; i < 3 && !DirListable(imgPath); ++i)
            Sleep(200);
        if (DirListable(imgPath))
            enumAll();      /* 目录缓过来了，重新枚举 */
    }

    /* v1.3.15 修订(审计 4.3)：文件集合没变就不删不重建。
     * OnWindowLoad 在每个窗口创建时都会调 LoadSettings(true)，原实现每次都
     * delete 全部位图并从磁盘全量重新解码——既浪费，又让正在绘制的线程
     * 可能拿到刚被 delete 的指针（use-after-free）。现在只有文件集合真的
     * 变化时才重建；posType/alpha 等标量配置在上面已照常更新。
     * 注：替换同名文件的内容需重启 explorer 才生效（与上游行为一致）。*/
    static std::vector<std::wstring> s_lastFiles;
    /* v1.3.19(审计修复 P2-6B)：当前列表是否已是"全量图集"。
     * 非随机模式只装 1 张图；用户切到随机后若文件集合没变，
     * 旧早退条件会带着这张单图继续用 —— 随机功能实际失效。
     * 记住"列表不是全量"，下次加载就强制重建。 */
    static bool s_listIsFullSet = false;
    {
        /* v1.3.15 R3：读列表结构须握 g_listMx（绘制线程可能同时在取凭证）
         * v1.3.15 R4(复审 P3)：s_lastFiles 的写入也挪进锁内，读写口径一致 */
        std::lock_guard<std::mutex> lk(g_listMx);
        if (!m_config.imageList.empty() && fileList == s_lastFiles
            && (m_config.isRandom.load() || s_listIsFullSet))
        {
            /* v1.3.25(D2)：图片集合没变 = 目录已正常工作，撤掉自愈重试标记 */
            s_retryWanted.store(false);
            return;             /* 图片集合没变：继续用现有位图，零开销 */
        }
        s_lastFiles = fileList;
    }

    if (fileList.empty())
    {
        /* v1.3.15 R3：目录空了，放掉旧位图（绘制线程手里的凭证仍保活到画完）*/
        { std::lock_guard<std::mutex> lk(g_listMx); m_config.imageList.clear(); }
        /* v1.3.18：这里原来是 MessageBoxW("缺少图片文件") */
        AppendLoadLog(
            std::wstring(L"结果 : 目录存在但里面没有 png/jpg 图片，本次不加载背景。\r\n")
            + L"       资源管理器不受影响，不会弹任何窗口。\r\n"
            + L"       目录 : " + imgPath);
        /* v1.3.25(D2) N3：自定义目录配置了但此刻没图 —— 可能图还没同步完/
         * 目录刚建好，挂 3 秒自愈重试；真·空目录每 3 秒一次探查，开销可忽略 */
        s_retryWanted.store(custom && !folder.empty());
        if (s_retryWanted.load()) ArmLoadRetryAll();
        return;
    }

    /* v1.3.15 R3(复审 P0)：先在锁外把新列表完整解好，再用 g_listMx 一瞬换表。
     * 绘制线程手里的旧凭证不受换表影响，对象按引用计数自动释放——
     * use-after-free 从机制上消除。*/
    std::vector<std::shared_ptr<BitmapGDI>> fresh;
    for (size_t i = 0; i < fileList.size(); i++)
    {
        auto bmp = std::make_shared<BitmapGDI>(fileList[i]);
        /* v1.3.15 修订(审计 3.10)：MinGW 的 Bitmap::FromStream 解码失败也返回
         * 非空指针（lastStatus=InvalidParameter、宽高 0），只判 bmp->src 会把
         * 伪装成图片的坏文件收进列表——占随机名额且该窗口没有背景。
         * 改为按解码状态与宽高把关，不合格的直接丢弃（自动析构）。
         *
         * v1.3.17：这里改回用 bmp->IsValid() 把关。
         *   v1.3.16 把解码推迟到真正绘制时，构造函数只探测宽高，
         *   而 Size 又没有初值 —— 文件打不开时读到的是堆上的垃圾值，
         *   只比 Size.cx > 0 就等于"垃圾值也算有效"，坏文件会被收进列表。
         *   现在 Size 有初值 {0,0}，且 IsValid() 只有在"图片头解析确认
         *   结构完整、尺寸有效"时才为真 —— 过滤条件重新变严。 */
        if (bmp->IsValid())
            fresh.push_back(std::move(bmp));

        /*非随机 只加载一张
        * Load only one image non randomly
        * v1.3.19(审计修复 P2-6A)：旧写法循环第一圈就 break，排序后第一张
        * 恰好是坏文件时后面的好图全被跳过，fresh 为空、没有背景。
        * 改为"成功收进第一张有效图后"才 break。 */
        if (!m_config.isRandom.load() && !fresh.empty()) break;
    }
    {
        std::lock_guard<std::mutex> lk(g_listMx);
        m_config.imageList.swap(fresh);   /* 旧列表随 fresh 析构释放，持凭证者不受影响 */
    }
    s_listIsFullSet = m_config.isRandom.load();   /* v1.3.19(P2-6B)：随机=全量，非随机=单图 */

    /* v1.3.25(D2)：加载成功的收尾 —— 撤自愈标记；但若一张都没解码成功
     * （文件恰好全部瞬时不可读等），按失败处理挂自愈重试 */
    if (m_config.imageList.empty() && custom && !folder.empty())
    {
        s_retryWanted.store(true);
        ArmLoadRetryAll();
    }
    else
    {
        s_retryWanted.store(false);
    }

    {
        wchar_t line[1024];
        /* v1.3.20(C)：日志带上轮换配置，装完一看便知定时器该不该在跑
         * v1.3.25(D1)：间隔单位分钟→秒，整分钟按分钟显示更好读 */
        if (m_config.isRandom.load() && m_config.rotateSec.load() > 0)
        {
            const int rs = m_config.rotateSec.load();
            if (rs % 60 == 0)
                swprintf(line, 1024,
                    L"结果 : 成功加载 %d 张背景图（定时轮换：每 %d 分钟）\r\n"
                    L"       来源 : %ls\r\n"
                    L"       目录 : %ls",
                    (int)m_config.imageList.size(), rs / 60,
                    srcDesc.c_str(), imgPath.c_str());
            else
                swprintf(line, 1024,
                    L"结果 : 成功加载 %d 张背景图（定时轮换：每 %d 秒）\r\n"
                    L"       来源 : %ls\r\n"
                    L"       目录 : %ls",
                    (int)m_config.imageList.size(), rs,
                    srcDesc.c_str(), imgPath.c_str());
        }
        else
            swprintf(line, 1024,
                L"结果 : 成功加载 %d 张背景图\r\n"
                L"       来源 : %ls\r\n"
                L"       目录 : %ls",
                (int)m_config.imageList.size(), srcDesc.c_str(), imgPath.c_str());
        AppendLoadLog(line);
    }
}

/*
* ShellLoader
* 文件资源管理器创建窗口时会调用 我们可以在这里更新一下配置
*/

/* ------------------------------------------------------------------
* v1.3.17 初始化诊断日志
*
* 为什么写文件而不是弹框：
* OnWindowLoad 是在 explorer 的 UI 线程上被调用的。在这条线程上弹模态框会把整个
* 资源管理器卡住，环境异常时还会表现为"一闪而过的错误框"，用户既看不清也测不准。
* 所以初始化结果一律写进 plugin 目录的 ExplorerBgTool_init.log，谁想看都能打开看。
* ------------------------------------------------------------------ */
static std::wstring WidenAscii(const char* s)
{
    std::wstring r;
    if (!s) return r;
    while (*s) r += (wchar_t)(unsigned char)(*s++);
    return r;
}

static const wchar_t* HookName(int i)
{
    switch (i) {
    case 1: return L"CreateWindowExW";
    case 2: return L"DestroyWindow";
    case 3: return L"BeginPaint";
    case 4: return L"FillRect";
    case 5: return L"CreateCompatibleDC";
    case 6: return L"EndPaint";
    default: return L"?";
    }
}

static void WriteInitLog(const std::wstring& body)
{
    std::wstring path = GetCurDllDir() + L"\\ExplorerBgTool_init.log";

    SYSTEMTIME st;
    GetLocalTime(&st);
    wchar_t head[256];
    swprintf(head, 256,
        L"ExplorerBgTool v1.3.28 (backimg 秒级轮换+目录自愈版)  initialization log\r\n"
        L"%04d-%02d-%02d %02d:%02d:%02d   pid=%lu  tid=%lu\r\n"
        L"dll: %ls\r\n\r\n",
        st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond,
        (unsigned long)GetCurrentProcessId(), (unsigned long)GetCurrentThreadId(),
        (GetCurDllDir() + L"\\ExplorerBgTool.dll").c_str());

    std::wstring text = std::wstring(head) + body;

    int need = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), (int)text.size(), NULL, 0, NULL, NULL);
    if (need <= 0) return;
    std::string utf8((size_t)need, '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.c_str(), (int)text.size(), &utf8[0], need, NULL, NULL);

    HANDLE h = CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return;      /* 目录不可写就放弃，绝不影响 explorer */
    DWORD wr = 0;
    WriteFile(h, utf8.data(), (DWORD)utf8.size(), &wr, NULL);
    CloseHandle(h);
}

/* v1.3.19(审计修复 P1-2)：初始化体抽成独立函数，交给 std::call_once。
 * v1.3.17【白屏事故修复】的历史仍然有效：不管成功还是失败，
 * 本进程只尝试一次初始化，且绝不终止宿主线程。
 * v1.3.19 新增两件事：
 *   1) GdiplusStartup 返回值不再忽略 —— 失败即停用全部背景功能；
 *   2) MH_EnableHook 失败后回滚已启用的钩子（P2-1）。 */
static void InitBackground()
{
    //初始化 Gdiplus（只启动一次）
    GdiplusStartupInput StartupInput;
    Gdiplus::Status gstat = GdiplusStartup(&m_gdiplusToken, &StartupInput, NULL);

    std::wstring log;
    wchar_t line[320];

    /* v1.3.19(审计修复 P1-2B)：启动失败就到此为止 —— 不装钩子不解码，
     * 背景功能整体停用，资源管理器照常工作。 */
    if (gstat != Gdiplus::Ok)
    {
        BitmapGDI::NotifyGdiplusStartupFailed();
        swprintf(line, 320, L"GdiplusStartup : %d\r\n", (int)gstat);
        log += line;
        log += L"结果 : GDI+ 启动失败 —— 背景功能停用，资源管理器不受影响\r\n";
        WriteInitLog(log);
        return;
    }

    int stInit = (int)MH_Initialize();
    swprintf(line, 320, L"MH_Initialize : %d (%ls)\r\n",
        stInit, WidenAscii(MH_StatusToString((MH_STATUS)stInit)).c_str());
    log += line;

    if (stInit != 0)        /* 0 == MH_OK */
    {
        /*v1.3.17：这里原来调 FreeLibraryAndExitThread，是白屏的直接原因。
        * 只记日志、不启用任何钩子，让资源管理器照常工作。*/
        log += L"结果 : MH_Initialize 失败 —— 未启用任何钩子，资源管理器不受影响\r\n";
        WriteInitLog(log);
        return;
    }

    g_hookFailIndex = 0;
    for (int i = 0; i < 7; i++) g_hookStatus[i] = 0;

    //创建钩子 CreateHook
    CreateMHook(CreateWindowExW, MyCreateWindowExW, _CreateWindowExW_, 1);
    CreateMHook(DestroyWindow, MyDestroyWindow, _DestroyWindow_, 2);
    CreateMHook(BeginPaint, MyBeginPaint, _BeginPaint_, 3);
    CreateMHook(FillRect, MyFillRect, _FillRect_, 4);
    CreateMHook(CreateCompatibleDC, MyCreateCompatibleDC, _CreateCompatibleDC_, 5);
    CreateMHook(EndPaint, MyEndPaint, _EndPaint_, 6);

    for (int i = 1; i <= 6; i++)
    {
        swprintf(line, 320, L"  [%d] %ls : %d (%ls)\r\n",
            i, HookName(i), g_hookStatus[i],
            WidenAscii(MH_StatusToString((MH_STATUS)g_hookStatus[i])).c_str());
        log += line;
    }

    /*v1.3.16 (B1)：六个钩子必须全部装上才启用。
    * 只装上一部分比完全装不上更危险——比如缺 EndPaint，绘制会话不会关闭，
    * 背景图就会一直往别的窗口上刷（正是 v1.3.15 想修的那个病）。*/
    if (g_hookFailIndex == 0)
    {
        int stEn = (int)MH_EnableHook(MH_ALL_HOOKS);
        swprintf(line, 320, L"MH_EnableHook : %d (%ls)\r\n",
            stEn, WidenAscii(MH_StatusToString((MH_STATUS)stEn)).c_str());
        log += line;
        if (stEn == 0)
        {
            log += L"结果 : 6 个钩子全部启用，背景功能已生效\r\n";
        }
        else
        {
            /* v1.3.19(审计修复 P2-1)：MinHook 的批量启用是逐项执行的，
             * 中途失败不会自动回滚 —— 比如 BeginPaint 启用了、EndPaint 没有，
             * 绘制会话开而不关，背景会刷到别人的绘制现场。
             * 所以失败必须自己收场：全部禁用 + 全部移除。 */
            int stDis = (int)MH_DisableHook(MH_ALL_HOOKS);
            MH_RemoveHook((LPVOID)&CreateWindowExW);
            MH_RemoveHook((LPVOID)&DestroyWindow);
            MH_RemoveHook((LPVOID)&BeginPaint);
            MH_RemoveHook((LPVOID)&FillRect);
            MH_RemoveHook((LPVOID)&CreateCompatibleDC);
            MH_RemoveHook((LPVOID)&EndPaint);
            swprintf(line, 320,
                L"结果 : 启用失败（已回滚：Disable=%d），钩子已全部移除，\r\n"
                L"       背景功能停用，资源管理器不受影响\r\n", stDis);
            log += line;
        }
    }
    else
    {
        MH_RemoveHook((LPVOID)&CreateWindowExW);
        MH_RemoveHook((LPVOID)&DestroyWindow);
        MH_RemoveHook((LPVOID)&BeginPaint);
        MH_RemoveHook((LPVOID)&FillRect);
        MH_RemoveHook((LPVOID)&CreateCompatibleDC);
        MH_RemoveHook((LPVOID)&EndPaint);

        swprintf(line, 320,
            L"结果 : 第 %d 个钩子（%ls）创建失败，已回滚全部钩子。\r\n"
            L"       背景功能停用，但资源管理器完全不受影响。\r\n"
            L"       常见原因：安全软件拦截、另一款 shell 扩展占用了同一位址。\r\n",
            g_hookFailIndex, HookName(g_hookFailIndex));
        log += line;
    }

    WriteInitLog(log);
}

void OnWindowLoad()
{
    //如果按住ESC键则不加载扩展
    if (GetKeyState(VK_ESCAPE) < 0)
        return;

    /* v1.3.19(审计修复 P1-2 + P1-4)：
     *   同步 —— call_once 保证初始化体全进程只执行一次，其余线程
     *   阻塞等它完成（旧写法的普通 bool 存在"看到置位但 GDI+ 未启动"
     *   的竞态窗口）；完成后走 s_initDone 轻量路径。
     *   异常边界 —— 本函数运行在 explorer 的 COM 调用栈上，任何 C++
     *   异常（bad_alloc 等）穿透出去都可能终结资源管理器，一律就地吞掉。
     *   LoadSettings 内部自带锁，多线程同时进来是安全的。 */
    try
    {
        if (!s_initDone.load(std::memory_order_acquire))
        {
            std::call_once(s_initOnce, InitBackground);
            s_initDone.store(true, std::memory_order_release);
        }
        LoadSettings(true);
    }
    catch (...)
    {
        /* 背景功能是锦上添花，任何异常都不能影响 explorer 本体 */
    }
}

HWND MyCreateWindowExW(
    DWORD     dwExStyle,
    LPCWSTR   lpClassName,
    LPCWSTR   lpWindowName,
    DWORD     dwStyle,
    int       X,
    int       Y,
    int       nWidth,
    int       nHeight,
    HWND      hWndParent,
    HMENU     hMenu,
    HINSTANCE hInstance,
    LPVOID    lpParam
)
{
    HWND hWnd = _CreateWindowExW_(dwExStyle, lpClassName, lpWindowName, dwStyle,
        X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);

    /* v1.3.19(审计修复 P1-4)：识别/登记逻辑会构造 std::wstring（可抛
     * bad_alloc），异常绝不能穿透 CreateWindowExW 回到 explorer。
     * 登记失败 = 这个窗口没有背景，仅此而已。 */
    try
    {
        std::wstring ClassName;
        if (hWnd)
        {
            ClassName = GetWindowClassName(hWnd);
        }

        //explorer window
        if (ClassName == L"DirectUIHWND"
            && GetWindowClassName(hWndParent) == L"SHELLDLL_DefView")
        {
            //继续查找父级 Continue to find parent
            HWND parent = GetParent(hWndParent);
            if (GetWindowClassName(parent) == L"ShellTabWindowClass")
            {
                //记录到列表中 Add to list
                MyData data;
                data.hWnd = hWnd;
                /* v1.3.15 R3：读列表尺寸须握锁（另一线程可能正在换表）*/
                size_t imgSize = 0;
                {
                    std::lock_guard<std::mutex> lk(g_listMx);
                    imgSize = m_config.imageList.size();
                }
                /* v1.3.20(A)：rand() % n 换成高质量随机源 RandomIndex
                 * （mt19937 + 随机设备播种，见 WinAPI.cpp；
                 *   rand() 的秒级种子在 explorer 秒级重启时会复现整条序列）
                 * v1.3.21(D5)：size_t 重载，免 (int) 强转 */
                if (m_config.isRandom.load() && imgSize)
                {
                    data.ImgIndex = RandomIndex(imgSize);
                }
                /* v1.3.20(B)：登记到全局窗口表（不再按线程覆盖 ——
                 * 多标签/同线程多窗口各自持有自己的记录） */
                data.tid = GetCurrentThreadId();   /* v1.3.21(D1)：记属主线程 */
                {
                    std::lock_guard<std::mutex> lk(g_wndDataMx);
                    g_wndData[hWnd] = data;
                }
                /* v1.3.20(C)：按当前配置给这个窗口装/不装轮换定时器。
                 * 此刻窗口刚创建还没进消息循环，WM_TIMER 会等它跑起来
                 * 之后才派发，安全。 */
                ApplyRotateTimer(hWnd);
                /* v1.3.25(D2) N3：此刻图还没加载出来但配置了自定义目录
                 * （目录瞬态不可枚举/重启后盘未就绪）→ 本窗口也挂 3 秒
                 * 自愈重试，目录恢复后本窗口无需重开即自动出背景 */
                size_t imgNow = 0;
                {
                    std::lock_guard<std::mutex> lk(g_listMx);
                    imgNow = m_config.imageList.size();
                }
                if (imgNow == 0 && s_retryWanted.load())
                    SetTimer(hWnd, RETRY_TIMER_ID, 3000U, LoadRetryTimerProc);
            }
        }
    }
    catch (...)
    {
        /* 吞掉：没有背景总比 explorer 崩溃好 */
    }
    return hWnd;
}

BOOL MyDestroyWindow(HWND hWnd)
{
    /* v1.3.20(B)：按窗口句柄注销登记（谁销毁谁注销，不再按线程匹配）。
     *
     * v1.3.22（Deepseek 复审第 1 条）线程归属加固：
     *   系统本只允许窗口属主线程成功销毁窗口（跨线程 DestroyWindow 会失败），
     *   但本钩子在真销毁【之前】就执行记账——即使真销毁失败，注销/清缓存/
     *   清会话也已经发生，跨线程场景会错位污染：
     *     - 用"销毁线程"的 tid 去匹配登记表里"属主线程"的 tid，必然误判；
     *     - ReleaseThreadCache() 释放的是销毁线程的缓存（错对象）；
     *     - t_sessionHwnd/t_duiPaintDepth 清的是销毁线程的 thread_local
     *       （错对象），属主线程的会话标记仍指向这个窗口。
     *   加固：以【登记时记下的属主 tid】为准——
     *     属主 == 当前线程（正常路径，行为与 v1.3.21 完全一致）：
     *       同线程已无其他登记窗口才释放缓存、清本线程会话状态；
     *     属主 != 当前线程（跨线程，异常路径）：只注销登记 + KillTimer，
     *       缓存与会话状态一概不动（孤儿缓存由 SweepDeadThreadCaches /
     *       ReleaseAllCaches 兜底回收，机制现成）。
     *
     * v1.3.22（Deepseek 复审第 2 条）不变量固化：
     *   threadEmpty 快照在锁内计算、锁外使用（TOCTOU 形态），其安全性依赖
     *   "同线程的 CreateWindowExW / DestroyWindow 走同一条消息队列、天然
     *   串行"——同线程不可能在快照计算与释放动作之间插入新的建窗登记。
     *   改动缓存策略时必须重新审视这一不变量，不可默认它永远成立。 */
    bool wasMine = false;
    bool sameThreadEmpty = false;   /* 属主线程已无其他登记窗口（含本次） */
    bool crossThread = false;       /* 本次销毁来自非属主线程 */
    {
        std::lock_guard<std::mutex> lk(g_wndDataMx);
        auto it = g_wndData.find(hWnd);
        if (it != g_wndData.end())
        {
            /* v1.3.22：先取登记时的属主 tid，再注销 */
            const DWORD ownerTid = it->second.tid;
            g_wndData.erase(it);
            wasMine = true;
            crossThread = (ownerTid != GetCurrentThreadId());
            if (!crossThread)
            {
                /* 用属主 tid（== 当前线程）扫剩余登记：还有同线程窗口就不释放 */
                sameThreadEmpty = true;
                for (auto& kv : g_wndData)
                {
                    if (kv.second.tid == ownerTid) { sameThreadEmpty = false; break; }
                }
            }
        }
    }
    if (wasMine)
    {
        /* v1.3.17：本线程的缩放缓存在这里释放。
         *
         * 为什么不靠 thread_local 的析构函数自动释放：
         *   MinGW-w64 在线程退出时跑 TLS 析构的时机是错的 ——
         *   析构函数会跑在已被释放并被填成 0xEE 的 TLS 内存上，
         *   实测必然二次释放崩溃（详见 WinAPI.cpp 里"为什么用裸指针"的说明）。
         *   所以改成"窗口销毁时主动释放"，时机明确、上下文安全。
         *
         * 顺带把内存也收住了：窗口关了还留着几十 MB 缩放缓存没道理。
         * v1.3.21(D1)：仅当属主线程已无其他登记窗口才释放（见上）；
         * v1.3.22：跨线程销毁一概不释放（缓存属于属主线程的 thread_local，
         * 本线程碰不到它，释放是错位操作）。
         * 此处 g_wndDataMx 已出作用域，ReleaseThreadCache 内部的
         * g_registryMx 与它本就互不嵌套。 */
        if (sameThreadEmpty)
            ReleaseThreadCache();

        /* v1.3.20(C)：窗口定时器随窗口销毁由系统自动清理，
         * 这里显式 Kill 一次只是求个明白（幂等，无害）。 */
        KillTimer(hWnd, ROTATE_TIMER_ID);

        if (!crossThread)
        {
            //v1.3.16 (B2)：窗口销毁时强制清零会话深度，
            //防止极端情况下 EndPaint 没被调用而让守卫一直开着
            //v1.3.22：跨线程销毁不清——thread_local 是当前线程的，
            //属主线程的会话状态碰不到；真销毁失败时窗口还活着，
            //清掉属主的会话标记反而会让后续绘制断会话
            t_duiPaintDepth = 0;
            t_sessionHwnd = 0;
        }
    }
    return _DestroyWindow_(hWnd);
}

HDC MyBeginPaint(HWND hWnd, LPPAINTSTRUCT lpPaint)
{
    //开始绘制DUI窗口 BeginPaint dui window
    HDC hDC = _BeginPaint_(hWnd, lpPaint);

    /* v1.3.20(B)：按窗口句柄查登记表。
     * 只在"没有进行中的会话"或"就是当前会话窗口（重入）"时才开启/加深，
     * 与旧版按线程记录的语义一致：会话中途对其他窗口 BeginPaint 不影响
     * 正在进行的 DUI 会话。 */
    {
        std::lock_guard<std::mutex> lk(g_wndDataMx);
        auto it = g_wndData.find(hWnd);
        /* v1.3.23(K6，Kimi 审计)：hDC 必须有效才记录/加深会话——
         * BeginPaint 对无效窗口返回 NULL（窗口在 BeginPaint 前已被销毁等
         * 极罕见时序），此前 NULL 也会被记进登记表并加深会话深度，
         * 之后 MyFillRect 的 hDC 相等匹配（NULL==NULL）会把异常现场
         * 当正常绘制处理。对 NULL DC 的 GDI 调用只失败不崩，但属
         * "异常状态当正常"的边界漏洞，现直接拒绝开会话。 */
        if (hDC && it != g_wndData.end()
            && (t_duiPaintDepth == 0 || t_sessionHwnd == hWnd))
        {
            if (t_duiPaintDepth < 64)   //防御：异常情况下不让深度无限增长
                ++t_duiPaintDepth;
            it->second.hDC = hDC;       //记录到列表 Record values to list
            t_sessionHwnd = hWnd;
        }
    }
    return hDC;
}

BOOL MyEndPaint(HWND hWnd, const LPPAINTSTRUCT lpPaint)
{
    /*v1.3.16 (B2)：只有"结束的确实是登记过的那个 DUI 窗口"才递减。
    * 任务栏/托盘等其他窗口的 EndPaint 不再影响 DUI 的会话状态。*/
    if (t_duiPaintDepth > 0 && t_sessionHwnd == hWnd)
    {
        --t_duiPaintDepth;
        if (t_duiPaintDepth < 0)
            t_duiPaintDepth = 0;
        if (t_duiPaintDepth == 0)
            t_sessionHwnd = 0;      /* v1.3.20(B)：会话结束，归还会话标记 */
    }
    return _EndPaint_(hWnd, lpPaint);
}

int MyFillRect(HDC hDC, const RECT* lprc, HBRUSH hbr)
{
    int ret = _FillRect_(hDC, lprc, hbr);

    //v1.3.15 守卫1：只在 DUI 窗口的 BeginPaint~EndPaint 绘制会话内才允许画背景。
    //老版本对所有 FillRect 生效，DC 句柄值池化复用后会把背景图刷进
    //任务栏/托盘等其他窗口的绘制现场，导致图标绘制错乱。
    if (t_duiPaintDepth <= 0)
        return ret;

    /* v1.3.20(B)：按会话窗口查登记表，先取一份标量快照再干活。
     * 锁内只做查找与标量拷贝；hWnd 在窗口存活期内有效 ——
     * 窗口销毁与绘制同线程，销毁会先注销记录，不存在悬垂。 */
    HWND recHwnd = 0;
    int  recIdx = 0;
    bool found = false;
    {
        std::lock_guard<std::mutex> lk(g_wndDataMx);
        auto it = g_wndData.find(t_sessionHwnd);
        if (it != g_wndData.end() && it->second.hDC == hDC)
        {
            found = true;
            recHwnd = it->second.hWnd;
            recIdx  = it->second.ImgIndex;
        }
    }
    if (found)
    try
        /* v1.3.19(审计修复 P1-4)：绘制路径里有取凭证（shared_ptr 拷贝可分配）、
         * GetScaled（new/容器扩容可抛 bad_alloc）等操作。异常若从 FillRect
         * 穿透出去就是送走 explorer —— 一律就地吞掉，返回原函数结果 ret。 */
        {
            /* v1.3.16：改用客户区尺寸（GetClientRect）作为缩放基准。
             * 旧版用 GetWindowRect：那是"窗口矩形"，含边框/标题栏，
             * 拿来当可绘制区域会偏大——缩放出来的图会超出实际可见区域，
             * 正是"图片超出窗口显示"的直接原因之一。
             * 绘制坐标 hDC 是客户区 DC，与 GetClientRect 同一坐标系。*/
            RECT pRc;
            GetClientRect(recHwnd, &pRc);
            SIZE wndSize = { pRc.right - pRc.left, pRc.bottom - pRc.top };

            /*因图片定位方式不同 如果窗口大小改变 需要全体重绘 否则有残留
            * Due to different image positioning methods,
            * if the window size changes, you need to redraw, otherwise there will be residues*/
            /* v1.3.16：去掉 "m_config.imgPosMode != 0" 的条件。
             * 旧版认为"左上角(0)"模式下图片位置不随窗口变、所以不用重绘。
             * 但 v1.3.16 起所有模式都按窗口尺寸等比缩放，图片大小一定会变，
             * 所以任何模式在窗口尺寸变化后都必须重绘，否则会留下旧尺寸的残影。*/
            /* v1.3.17：重绘不变式修复（审计报告问题 3）。
             *
             * 原写法：InvalidateRect 在最前，而 t_myData.size 只在函数
             * 最末尾更新。中间有四条提前 return（imageList 为空 / 取不到
             * 图片 / 目标矩形非法 / GetScaled 失败），任何一条命中，
             * size 就永远停在旧值 —— 于是每一帧都判定"尺寸变了"，
             * 再次 InvalidateRect，无限重绘，CPU 直接跑满。
             * 实测（复刻控制流，绘制 1000 帧）：
             *     正常路径        自触发重绘 1 次（收敛）
             *     imageList 为空  自触发重绘 1000 次
             *     GetScaled 失败  自触发重绘 1000 次
             *
             * 修法：先用旧值做比较，再【立刻】把新尺寸写回去。
             * 这样无论后面走哪条路径（包括提前 return），
             * "已经处理过这个尺寸"这件事都会被记录下来，不会反复触发。
             */
            SIZE oldSize = { 0, 0 };
            {
                std::lock_guard<std::mutex> lk(g_wndDataMx);
                auto it = g_wndData.find(t_sessionHwnd);
                if (it != g_wndData.end())
                {
                    oldSize = it->second.size;
                    it->second.size = wndSize;   /* 无论后续走哪条路径都先记账 */
                }
            }
            if (oldSize.cx != wndSize.cx || oldSize.cy != wndSize.cy) {
                InvalidateRect(recHwnd, 0, TRUE);
            }

            /* v1.3.15 R3(复审 P0)：use-after-free 根治——在 g_listMx 锁内完成
             * "判空 + 越界钳位 + 取出凭证"，出锁后再画。凭证（shared_ptr）保证
             * 即便此刻另一线程换掉整个列表，这张图也活到画完为止。
             * 取凭证安排在 SaveDC 之前，此处提前退出不留任何 DC 残留。*/
            std::shared_ptr<BitmapGDI> pBgBmp;
            {
                std::lock_guard<std::mutex> lk(g_listMx);
                if (m_config.imageList.empty())
                    return ret;
                /*v1.3.16 (ImgIndex)：越界时退回第 0 张——老窗口记着旧下标，
                 * 图片变少后直接下标会越界崩 explorer（复现路径见 v1.3.16 说明）*/
                if (recIdx < 0 || (size_t)recIdx >= m_config.imageList.size())
                {
                    recIdx = 0;
                }
                pBgBmp = m_config.imageList[recIdx];
            }
            /* v1.3.20(B)：钳位结果写回登记表，后续帧不必反复钳 */
            {
                std::lock_guard<std::mutex> lk(g_wndDataMx);
                auto it = g_wndData.find(t_sessionHwnd);
                if (it != g_wndData.end())
                    it->second.ImgIndex = recIdx;
            }
            if (!pBgBmp)
                return ret;

            /* --------------------------------------------------------------
            * v1.3.16：绘制目标矩形改由 DrawScale.h 统一计算
            *
            * 旧版的问题：
            *   1) posType=0..4 直接按"原图像素尺寸"贴图，图片比窗口大就溢出；
            *   2) posType=5/6 虽然算出了缩放后的尺寸，但真正缩放的是
            *      AlphaBlend 自己做的最近邻插值 —— 放大后满屏马赛克；
            *   3) 窗口尺寸变化时的重绘判定漏掉了 posType==0 的情况。
            *
            * 现在：
            *   目标矩形由 ComputeDrawRect() 按"原图宽高比等比缩放"算出，
            *   缩放本身交给 BitmapGDI::GetScaled() 用 GDI+ 高质量插值完成，
            *   最后 AlphaBlend 只做 1:1 贴图 —— 尺寸一致，不再有拉伸变形。
            * -------------------------------------------------------------- */
            DrawRect dst = ComputeDrawRect(m_config.imgPosMode.load(),
                (int)pBgBmp->Size.cx, (int)pBgBmp->Size.cy,
                (int)wndSize.cx, (int)wndSize.cy);
            if (dst.w <= 0 || dst.h <= 0)
                return ret;

            /* 取缩放好的图（内部带缓存，尺寸不变直接命中） */
            std::shared_ptr<ScaledBitmap> scaled = pBgBmp->GetScaled(dst.w, dst.h);
            if (!scaled || !scaled->pMem)
                return ret;

            //裁剪矩形 Clip rect
            //v1.3.15 修订(审计 3.11)：记下本次存档返回的牌号，画完按号精确恢复。
            int savedDC = SaveDC(hDC);
            /* v1.3.15 修订(审计 3.11)：存档失败就不再裁剪，避免裁剪区残留
             * v1.3.24(N2，混元审计)：lprc 判空——本函数是全局钩子，调用方若
             * 违规传 NULL（FillRect 契约要求非空），原函数不崩的话这里
             * 解引用反而成了 DLL 新引入的崩溃点。钩子要比原函数更保守。 */
            if (savedDC && lprc) IntersectClipRect(hDC, lprc->left, lprc->top, lprc->right, lprc->bottom);

            /*绘制图片 Paint image
             * v1.3.16：源矩形与目标矩形同为 dst.w x dst.h —— 1:1 贴图。
             * 缩放已在 GetScaled() 里用高质量插值做完，这里不再做任何插值，
             * 因此不存在"被拉伸变形"的可能。*/
            BLENDFUNCTION bf = { AC_SRC_OVER, 0, m_config.imgAlpha.load(), AC_SRC_ALPHA };
            AlphaBlend(hDC, dst.x, dst.y, dst.w, dst.h,
                scaled->pMem, 0, 0, dst.w, dst.h, bf);

            //v1.3.15(修复版)：-1 的语义是"撤销最近一次存档"，若本会话中途有其他
            //代码往同一 DC 又 SaveDC 过，弹掉的就是别人的档。按牌号精确恢复。
            if (savedDC) RestoreDC(hDC, savedDC);

            //Log(L"DrawImage");
        }
        catch (...)
        {
            /* 吞掉：本次不画背景，绘制的其余部分照常进行 */
        }
    return ret;
}

HDC MyCreateCompatibleDC(HDC hDC)
{
    //在绘制DUI之前 会调用CreateCompatibleDC 找到它
    //CreateCompatibleDC is called before drawing the DUI
    HDC retDC = _CreateCompatibleDC_(hDC);

    //v1.3.15 守卫2：非 DUI 绘制会话内不顶替记录的 DC
    if (t_duiPaintDepth > 0 && t_sessionHwnd)
    {
        /* v1.3.20(B)：按会话窗口查登记表，把记录的 DC 换成兼容 DC */
        std::lock_guard<std::mutex> lk(g_wndDataMx);
        auto it = g_wndData.find(t_sessionHwnd);
        if (it != g_wndData.end() && it->second.hDC == hDC)
            it->second.hDC = retDC;
    }
    return retDC;
}
