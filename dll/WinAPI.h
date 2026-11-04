/*
* WinAPI声明
*
* Author: Maple
* date: 2021-7-13 Create
* Copyright winmoes.com
*/

#pragma once
#include <string>
#include <comdef.h>
#include <gdiplus.h>
#include <vector>
#include <memory>
#include <mutex>
#include <climits>   /* v1.3.22：INT_MAX（RandomIndex size_t 重载的钳位上界） */

/*调试输出
* Debug output
*/
extern void Log(std::wstring log);
extern void Log(int log);

/*获取当前dll所在目录
* Get current directory
*/
extern std::wstring GetCurDllDir();

/*判断文件是否存在
* file exist
*/
extern bool FileIsExist(std::wstring FilePath);

/* v1.3.17：每线程缩放缓存的生命周期管理（实现在 WinAPI.cpp）
 *
 * 为什么需要显式释放：
 *   缩放缓存不能直接做成 "static thread_local ScaleCache" ——
 *   ScaleCache 含 std::vector（非平凡析构），MinGW-w64 给它注册的
 *   TLS 析构回调在【线程退出】时跑，而那个时机是错的：
 *   析构函数会跑在已被释放、并被 ucrt 填成 0xEE 的 TLS 内存上，
 *   实测必然二次释放崩溃。explorer 的窗口 UI 线程会退出，
 *   一退出就会踩到，直接把资源管理器带走。
 *
 *   所以缓存对象堆上分配，thread_local 只存一个平凡可析构的裸指针；
 *   窗口销毁时（MyDestroyWindow）主动释放本线程的缓存。
 *
 * ReleaseThreadCache : 释放【当前线程】的缓存，返回是否确实释放了
 * ReleaseAllCaches   : 释放所有线程的缓存（DLL 卸载时兜底调用） */
extern bool ReleaseThreadCache();
extern void ReleaseAllCaches();

/*获取窗口标题*/
extern std::wstring GetWindowTitle(HWND hWnd);

/*获取窗口类名*/
extern std::wstring GetWindowClassName(HWND hWnd);

/*读取配置文件内容
* Read config file
*/
extern std::wstring GetIniString(std::wstring FilePath, std::wstring AppName, std::wstring KeyName);

/*枚举某目录下指定文件*/
extern void EnumFiles(std::wstring path, std::wstring append, std::vector<std::wstring>& fileList);

/* v1.3.16：等比缩放渲染结果
 *
 * 改造背景：旧版把整张原图用 AlphaBlend 一次性贴到窗口上，
 * 而 AlphaBlend 的缩放是"最近邻"（实测：4x4 放大到 40x40 得到
 * 纯色方块，无任何插值），放大后满屏马赛克；
 * 旧版 posType=5/6 走的是同一条 AlphaBlend 路径，
 * 整个 DLL 里没有任何"按原图宽高比等比缩放"的实现。
 *
 * 【v1.3.16 用的是 GDI+ 缩放，不是纯 GDI】（v1.3.17 更正注释）
 *   实测三条路线：
 *     a) AlphaBlend 缩放：能保留 alpha，但插值是最近邻，放大后是马赛克；
 *     b) StretchBlt 缩放：插值是 HALFTONE 高质量，但它写出的位图
 *        之后不能再作为 AlphaBlend 的源（实测 AlphaBlend 直接返回 0），
 *        而且会把目标位图的 alpha 通道清零；
 *     c) GDI+ DrawImage + GetHBITMAP：插值是高质量双三次，alpha 完整保留，
 *        输出位图能正常用作 AlphaBlend 的源（实测 AlphaBlend 返回 1）。
 *   所以走 (c)：先在离屏 PARGB 位图上用 GDI+ 双三次插值缩好，
 *   再用 AlphaBlend 做 1:1 贴图（贴图阶段零插值，结构上不可能变形）。
 *
 * 【AlphaBlend 的两个硬性前提】（实测踩过的坑）
 *   1) 源 DC 必须用 CreateCompatibleDC(NULL) 创建（无调色板）；
 *      带调色板的 DC 会让 AlphaBlend 直接失败（返回 0）。
 *   2) 源矩形必须完整落在源位图内，否则同样返回 0。
 */
class ScaledBitmap
{
public:
    ScaledBitmap() {}
    ~ScaledBitmap()
    {
        /* 释放顺序：先删 DC（DC 销毁不会删除位图），再删位图。
         *
         * 注意【不能】用 SelectObject(pMem, NULL) 把位图摘下来：
         * 实测（probe/exact.cpp）一旦这样做过，该 HBITMAP 之后再被
         * 选进任何 DC 时，AlphaBlend 会直接失败（返回 0，背景画不出来）。
         * GDI 会把"曾被置空过"的位图标记为不可用于 alpha 合成。
         * 正确做法就是 DeleteDC 后再 DeleteObject。 */
        if (pMem)
        {
            DeleteDC(pMem);
            pMem = 0;
        }
        if (pBmp)
        {
            DeleteObject(pBmp);
            pBmp = 0;
        }
    }
    HDC     pMem = 0;      /* 承载缩放结果的内存 DC */
    HBITMAP pBmp = 0;      /* 缩放结果位图 */
    int     w = 0;         /* 缩放结果的宽 */
    int     h = 0;         /* 缩放结果的高 */
};

/* v1.3.16/v1.3.17：每线程私有的缩放缓存
 *
 * 为什么必须是 thread_local 而不是全局共享：
 *   缩放结果里存着 GDI 句柄（内存 DC + HBITMAP）。
 *   若全局共享，多个 explorer 窗口（各自 UI 线程）会并发创建/淘汰
 *   同一批 GDI 对象 —— 实测在高频淘汰下会破坏堆、导致进程崩溃。
 *   改为 thread_local 后，句柄的创建、使用、销毁全在同一线程内，
 *   不存在跨线程竞争，也不需要任何全局锁。
 *
 *   这与 v1.3.15 已有的 thread_local MyData 设计思路一致：
 *   每个窗口的绘制状态本来就应该只属于它自己那个线程。
 *
 * v1.3.17 修掉的无上限内存泄漏（审计报告问题 1）：
 *   原来 m_sources（每线程解码的源位图）按 ownerId 存，且【没有任何预算约束】，
 *   而 ScaleCache::Drop() 只清 m_items —— 于是每次换图/换目录，
 *   旧 ownerId 的源位图就永久留在缓存里。
 *   实测（用户 39 张 1024x1536 的 PNG）：
 *       单线程缩放一遍      +280 MB
 *       列表全部析构后      仍残留 177 MB
 *       连续重建 5 次       +885 MB，且每轮线性增长
 *
 *   修法：源位图改按【文件内容哈希】为键，不再按 ownerId。
 *     - 同一份内容重建列表 -> 哈希相同 -> 直接命中缓存，零增长；
 *     - 换了内容 -> 旧条目按 LRU 预算淘汰并 delete，不会无限堆积。
 *   另外加了"字节预算 + 条数上限"双保险。
 *
 * v1.3.18(审计修复 P0-2)：缩放结果（m_items）的键也从 ownerId 改为【内容哈希】。
 *   原因：ownerId 在 BitmapGDI 构造时全局自增、永不复用，而图片列表一重建
 *   （文件集合有任何增删）就产生一批全新 id —— 同一张图在新列表里拿到新 id，
 *   旧线程缓存里按旧 id 存的缩放结果全部作废，只能整个重新缩放一遍。
 *   改按内容哈希后，重建列表不影响命中：同一份内容（同哈希、同源尺寸）
 *   的缩放结果与"哪个 BitmapGDI 对象拥有它"无关，天然可复用。
 *   缩放结果只取决于（源图内容, 目标宽, 目标高）三元组，与对象身份无关，
 *   按内容寻址在语义上本来就更正确。内存仍由 32MB LRU 预算管住。
 */
class ScaleCache
{
public:
    /* 析构移到 .cpp 实现：它要访问 GDI+ 串行锁（见 WinAPI.cpp），
     * 而锁是 .cpp 里的静态对象，头文件里拿不到。 */
    ~ScaleCache();

    /* 取缓存中的缩放结果；没有则返回空 */
    std::shared_ptr<ScaledBitmap> Find(unsigned long long hash, int w, int h);

    /* 存入并按预算淘汰 */
    void Put(unsigned long long hash, int w, int h, const std::shared_ptr<ScaledBitmap>& bmp);

    /* 丢弃某【内容】的全部缩放结果条目 */
    void Drop(unsigned long long hash);

    /* 清空全部（含源位图） */
    void Clear();

    /* 取本线程为该【内容】解码的源位图；没有则解码一份。
     *
     * 【调用方必须已持有 GDI+ 串行锁】（本 DLL 里由 GetScaled 统一加锁）。
     * hash : 文件内容的哈希，由 BitmapGDI 构造时算好
     * w/h  : 源图尺寸
     * raw  : 文件原始字节
     */
    Gdiplus::Bitmap* GetSourceLocked(unsigned long long hash, int w, int h,
                                    const std::vector<BYTE>& raw);

private:
    struct Entry
    {
        unsigned long long hash = 0;   /* v1.3.18：键改为内容哈希 */
        int w = 0, h = 0;
        std::shared_ptr<ScaledBitmap> bmp;
        unsigned long useTick = 0;
    };
    std::vector<Entry> m_items;
    unsigned long m_tick = 0;

    /* 每线程各自解码的源位图（内容哈希 -> Bitmap*）。
     * GDI+ 的位图对象不可跨线程共享 —— 多线程共用同一个 src 做
     * DrawImage 会破坏堆（实测 8 线程压测第 6 轮即崩，单线程全过）。
     * 所以每个线程对每张【内容】各解码一份，线程之间零共享。 */
    struct SourceEntry
    {
        unsigned long long hash = 0;
        int w = 0, h = 0;
        Gdiplus::Bitmap* bmp = NULL;
        size_t bytes = 0;          /* 该位图占用的字节数（用于预算淘汰） */
        unsigned long useTick = 0;
    };
    std::vector<SourceEntry> m_sources;
    unsigned long m_srcTick = 0;
};

/*GDI Bitmap*/
class BitmapGDI
{
public:
	BitmapGDI(std::wstring path);
	~BitmapGDI();

	HDC pMem = 0;
	HBITMAP pBmp = 0;

	/* v1.3.17：加初值。
	 * 旧写法没有初值，而构造函数只在"文件打得开且解码成功"时才赋值，
	 * 文件打不开时 Size 保持堆上的垃圾值（实测 200/200 次读到非零垃圾），
	 * 而上层过滤条件又退化成只比 Size.cx > 0 —— 于是打不开的文件
	 * 会被当成有效图片收进 imageList。 */
	SIZE Size = { 0, 0 };

	/* 图片是否确证有效（结构完整、能安全解码）。
	 * 上层用它做过滤，比单看 Size 更严格也更明确。 */
	bool IsValid() const { return m_valid; }

	/* 取"缩放到 dstW x dstH"的离屏位图；失败返回空指针。
	 * 内部走每线程私有的 LRU 缓存：同图同尺寸直接命中，不重复缩放。
	 * 返回 shared_ptr，调用方持有期间该位图必然存活。 */
	std::shared_ptr<ScaledBitmap> GetScaled(int dstW, int dstH);

	/* 丢弃本图在缓存中的全部条目（图片被销毁时调用） */
	void DropScaled();

	/* v1.3.17：标记 GDI+ 已被关闭。
	 * 线程退出时 ~ScaleCache 会 delete 源位图；若那时 GDI+ 已经关了，
	 * 这就是一次非法释放（gdb 实测定位在 WinAPI.h 的
	 * "delete m_sources[i].second"，必崩）。本 DLL 从不调用
	 * GdiplusShutdown，所以正常路径不会走到；但把这个出口堵上，
	 * 将来若要关 GDI+，先调它就不会崩。 */
	static void MarkGdiplusShutdown();

	/* v1.3.19(审计修复 P1-2B)：GdiplusStartup 失败时由 dllmain 调用，
	 * 之后解码路径全部停用（背景不画，资源管理器不受影响）。 */
	static void NotifyGdiplusStartupFailed();

	/* v1.3.19：GDI+ 是否可用（启动成功且未关闭） */
	static bool GdiplusUsable();

private:
	/* v1.3.18：m_ownerId 已删除 —— 缩放缓存全面改按内容哈希（m_hash）寻址，
	 * 不再需要"归属 id"这个概念。 */

	/* 原始文件字节：各线程自行解码，避免跨线程共享 GDI+ 对象 */
	std::vector<BYTE> m_raw;

	/* v1.3.17：文件内容哈希（FNV-1a 64）。
	 * 源位图缓存按它做键 —— 同一份内容重建图片列表时直接命中缓存，
	 * 不会重复解码、也不会重复占内存（这是修掉无上限泄漏的关键）。 */
	unsigned long long m_hash = 0;

	/* v1.3.17：图片是否确证有效 */
	bool m_valid = false;
};

/* ------------------------------------------------------------------
* v1.3.20(A)：高质量随机下标（替代 rand()）
*
* 旧实现 rand() % n 的毛病：
*  1) 种子只精确到秒（srand(time(0))）—— explorer 秒级重启会拿到
*     同一个种子，整条"随机"序列逐字复现；
*  2) rand() 是 C 库的低质量线性同余生成器，连续取值相关性强；
*  3) 非线程安全的全局状态。
* 新实现：mt19937（梅森旋转）+ random_device/系统计数混种，
* 互斥锁串行化（调用频率极低，锁无竞争），uniform_int_distribution
* 消除模偏差。见 WinAPI.cpp 的 RandomIndex。
* ------------------------------------------------------------------ */
int RandomIndex(int n);   //返回 [0, n) 的均匀随机整数；n <= 1 时恒返回 0
/* v1.3.21(D5，Deepseek 审计)：size_t 重载 —— 图片列表尺寸是 size_t，
 * 调用点 (int) 强转在数量超过 INT_MAX 时属未定义行为。现实中受内存
 * 限制几乎不可能，但钳位兜底零成本，调用点免写强转。
 * v1.3.22（Deepseek 复审第 3 条）：钳位值改用 (size_t)INT_MAX 显式表达，
 * 不再依赖 32 位 unsigned 的字面魔数。 */
inline int RandomIndex(size_t n)
{
    if (n > (size_t)INT_MAX) n = (size_t)INT_MAX;
    return RandomIndex((int)n);
}