/*
* WinAPI声明
*
* Author: Maple
* date: 2021-7-13 Create
* Copyright winmoes.com
*/

#include "framework.h"
#include "WinAPI.h"
#include "ImageHeader.h"
#include <io.h>
#include <shlwapi.h>
#include <map>
#include <random>
#pragma comment(lib, "shlwapi.lib")

void Log(std::wstring log)
{
	OutputDebugStringW((L"\n[Debug]: " + log).c_str());
}

void Log(int log)
{
	Log(std::to_wstring(log));
}

std::wstring GetCurDllDir()
{
	/* v1.3.18(审计修复)：补防御 —— GetModuleFileNameW 失败/截断、路径里
	 * 找不到反斜杠时返回空串，而不是拿垃圾路径或 substr(npos) 去赌运气。
	 * g_hModule 在 DllMain 里最先赋值，正常路径永远有效，这里只是把
	 * "万一"变成无害的空目录（上层拿到空串只是读不到配置，不会崩）。 */
	wchar_t sPath[MAX_PATH] = { 0 };
	DWORD n = GetModuleFileNameW(g_hModule, sPath, MAX_PATH);
	if (n == 0 || n >= MAX_PATH)
		return std::wstring();
	std::wstring path = sPath;
	size_t pos = path.rfind(L"\\");
	if (pos == std::wstring::npos)
		return std::wstring();
	return path.substr(0, pos);
}

bool FileIsExist(std::wstring FilePath)
{
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;
	hFind = FindFirstFileW(FilePath.c_str(), &FindFileData);
	if (hFind != INVALID_HANDLE_VALUE) {
		FindClose(hFind);
		return true;
	}
	return false;
}

std::wstring GetWindowTitle(HWND hWnd)
{
	int size = GetWindowTextLengthW(hWnd);
	//v1.3.18: 原写法 new wchar_t[size] 少一个字符，GetWindowTextW 会写 size+1 个字符
	//（含结尾 NUL），堆上越界写 1 个 wchar_t。当前无人调用此函数，属埋着的坑，一并修掉。
	wchar_t* pText = new wchar_t[(size_t)size + 1];
	pText[0] = L'\0';
	GetWindowTextW(hWnd, pText, size + 1);
	std::wstring ret(pText);       //v1.3.18: 原实现直接 return std::wstring(pText) 后从未释放
	delete[] pText;                //           这块内存，每调用一次泄漏 2*(size+1) 字节
	return ret;
}

std::wstring GetWindowClassName(HWND hWnd)
{
	//v1.3.18: 原写法 new wchar_t[MAX_PATH] 用完不释放，而 CreateWindowExW 钩子
	//每个窗口要调 3 次 —— 等于每建一个窗口漏 520*3 字节，explorer 长开必涨。
	//改成栈上缓冲，行为完全一致。
	wchar_t pText[MAX_PATH] = { 0 };
	GetClassNameW(hWnd, pText, MAX_PATH);
	return std::wstring(pText);
}

std::wstring GetIniString(std::wstring FilePath, std::wstring AppName, std::wstring KeyName)
{
	if (!FileIsExist(FilePath))
		return std::wstring();

	HANDLE pFile = CreateFileW(FilePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (pFile == INVALID_HANDLE_VALUE)
		return std::wstring();

	/*v1.3.16 (GetIniString)：原写法 LARGE_INTEGER fileSize; 完全没有初始化，
	* 一旦 CreateFileW 失败（文件被独占、权限不足等），GetFileSizeEx 也不会写它，
	* 接着 new wchar_t[fileSize.QuadPart] 就会申请一个栈上垃圾值那么大的内存，
	* 抛异常或直接崩掉。另外 (DWORD)fileSize.QuadPart 在超大文件时会截断。
	* 现在显式清零、逐级校验，并把可接受的 ini 大小限制在 16MB
	* （配置文件不可能这么大，超了肯定是异常情况）。*/
	LARGE_INTEGER fileSize;
	fileSize.QuadPart = 0;
	if (!GetFileSizeEx(pFile, &fileSize)
		|| fileSize.QuadPart <= 0
		|| fileSize.QuadPart > (16LL * 1024 * 1024))
	{
		CloseHandle(pFile);
		return std::wstring();
	}

	/*原写法按"字节数"申请 wchar_t 个数，实际多分配了一倍（wchar_t 是 2 字节），
	* 本身不会越界，这里保持同样宽松的语义并多留 1 个终止符位，
	* 但把传给 GetPrivateProfileStringW 的 nSize 与真实字符容量对齐。*/
	size_t charCount = (size_t)fileSize.QuadPart + 1;
	wchar_t* data = new wchar_t[charCount];
	ZeroMemory(data, sizeof(wchar_t) * charCount);
	GetPrivateProfileStringW(AppName.c_str(), KeyName.c_str(), NULL, data, (DWORD)charCount, FilePath.c_str());

	std::wstring ret = data;
	delete[] data;

	CloseHandle(pFile);
	return ret;
}

void EnumFiles(std::wstring path, std::wstring append, std::vector<std::wstring>& fileList)
{
	//文件句柄 
	intptr_t  hFile = 0;
	//文件信息 
	struct _wfinddata_t fileinfo;
	std::wstring p;
	if ((hFile = _wfindfirst(p.assign(path).append(L"\\" + append).c_str(), &fileinfo)) != -1)
	{
		do
		{
			if (!(fileinfo.attrib & _A_SUBDIR))
			{
				std::wstring path_ = path + L"\\";
				path_ += fileinfo.name;
				fileList.push_back(path_);
			}
		} while (_wfindnext(hFile, &fileinfo) == 0);
		_findclose(hFile);
	}
}

/* v1.3.18(审计修复 P0-2)：NextScaleOwnerId / m_ownerId 整套机制已删除 ——
 * 缩放缓存（m_items）与源位图缓存（m_sources）统一按【内容哈希】寻址。
 * 原 ownerId 方案的缺陷：id 在 BitmapGDI 构造时全局自增、永不复用，
 * 图片列表一重建（文件集合有任何增删）同一条图就拿到全新 id，
 * 各线程缓存里按旧 id 存的缩放结果全部 miss，只能重新解码+缩放一遍，
 * LRU 淘汰随之被高频触发（纯浪费的 churn）。改按内容哈希后，
 * 重建列表零影响：同一份内容的缩放结果直接复用。
 * 见 WinAPI.h 里 ScaleCache 的说明。 */
static void ScaleCacheDrop(unsigned long long hash);

/* v1.3.17：线程缓存的生命周期管理（见下面"为什么用裸指针"的说明） */
static bool ReleaseThreadCacheImpl();
static void ReleaseAllCachesImpl();

/* ------------------------------------------------------------------
* v1.3.17：GDI+ 串行锁（真正接上了 —— v1.3.16 里它只是定义、零引用）
*
* 为什么要锁：
*   GDI+ 的位图对象不能被多个线程同时使用。v1.3.16 靠"每线程各自解码
*   一份、线程间零共享"规避了共享崩溃，但【解码本身】（FromStream）
*   仍然会在多个 explorer 窗口线程上并发发生。
*   实测多线程并发下 GDI+ 内部状态会被破坏（8 线程压测第 6 轮段错误，
*   单线程 40 轮全过）。
*
* 锁的范围：
*   只包住 GDI+ 调用本身（解码 / DrawImage / GetHBITMAP / delete 位图）。
*   缓存查找、GDI 句柄的创建销毁都在锁外，不拖慢无关操作。
*   真实场景每秒只有几十次缩放需求（拖窗口时的重绘），串行化不构成瓶颈。
*
* 为什么用"故意泄漏"的单例而不是普通静态对象：
*   g_threadCache 是 thread_local，它的析构在【线程退出】时跑；
*   而本 DLL 会被 regsvr32.exe 加载后卸载（进程退出时 DLL 的静态对象
*   先析构，线程局部对象的析构回调后跑）—— 若锁是普通静态对象，
*   那一刻它可能已经被销毁，lock_guard 就是访问已析构对象（UB）。
*   故意 new 出来永不释放即可彻底避开这个顺序问题。
* ------------------------------------------------------------------ */
static std::mutex& GdiplusMutex()
{
    static std::mutex* m = new std::mutex();   /* 故意泄漏，避免析构顺序问题 */
    return *m;
}

/* v1.3.17：GDI+ 是否仍在运行。
* 线程退出时 ~ScaleCache 要 delete 源位图 —— 若此时 GDI+ 已经关闭，
* 那就是非法释放（gdb 实测定位在 WinAPI.h 的 delete m_sources[i].second，
* 必崩）。本 DLL 从不调用 GdiplusShutdown，但把这个状态显式记下来，
* 将来真要关 GDI+ 时先调 MarkGdiplusShutdown() 就不会踩雷。 */
static bool g_gdiplusAlive = true;

/* v1.3.19(审计修复 P1-2B)：GDI+ 启动失败的显式出口。
 * OnWindowLoad 会检查 GdiplusStartup 的返回值，失败时调这里：
 * 之后所有 GDI+ 调用入口（构造探测 / GetScaled）都直接放弃，
 * 背景功能停用，资源管理器照常工作。 */
static bool g_gdiplusUsable = true;

void BitmapGDI::MarkGdiplusShutdown()
{
    g_gdiplusAlive = false;
    g_gdiplusUsable = false;
}

/* v1.3.19(审计修复 P1-2B)：供 dllmain 在 GdiplusStartup 失败时调用 */
void BitmapGDI::NotifyGdiplusStartupFailed()
{
    g_gdiplusUsable = false;
}

bool BitmapGDI::GdiplusUsable()
{
    return g_gdiplusUsable;
}

/* 内容哈希（FNV-1a 64）：源位图缓存的键 */
static unsigned long long HashBytes(const BYTE* data, size_t len)
{
    unsigned long long h = 1469598103934665603ULL;
    for (size_t i = 0; i < len; i++)
    {
        h ^= (unsigned long long)data[i];
        h *= 1099511628211ULL;
    }
    return h;
}




BitmapGDI::BitmapGDI(std::wstring path)
{
	//这样加载是为了防止文件被占用
	FILE* file = nullptr;
	_wfopen_s(&file, path.c_str(), L"rb");
	if (!file)
		return;                       /* 打不开：Size 保持 {0,0}，m_valid 保持 false */

	fseek(file, 0L, SEEK_END);
	long len = ftell(file);
	rewind(file);
	if (len <= 0)                    /* 空文件 / ftell 失败 */
	{
		fclose(file);
		return;
	}

	/* v1.3.19(审计修复 P1-4)：单文件原始字节上限。
	 * 原写法按 ftell 结果直接 resize，一个 1.9GB 的文件就会真读 1.9GB 进内存，
	 * 而且 vector::resize 失败抛出的 std::bad_alloc 会一路穿透到 explorer 的
	 * COM 边界。256MB 远超任何合理壁纸的体积，超限直接按"无效文件"处理。 */
	const long RAW_FILE_MAX = 256L * 1024 * 1024;
	if (len > RAW_FILE_MAX)
	{
		fclose(file);
		return;
	}

	m_raw.resize((size_t)len);
	size_t rd = fread(&m_raw[0], 1, (size_t)len, file);
	fclose(file);
	if (rd != (size_t)len)            /* 读不满：文件可能正被改写 */
	{
		m_raw.clear();
		return;
	}

	/* v1.3.17：内容哈希（FNV-1a 64）。
	 * 源位图缓存按它做键 —— 同一份内容重建图片列表时直接命中缓存，
	 * 不会重复解码、也不会重复占内存（修掉无上限泄漏的关键）。 */
	m_hash = HashBytes(m_raw.data(), m_raw.size());

	/* v1.3.17：先用【轻量图片头解析器】确认这是一张结构完整的图片，并取尺寸。
	 *
	 * 这一步同时解决两个问题：
	 *   1) Size 的赋值只发生在"确证有效"之后 —— 不会再有未初始化的垃圾值；
	 *   2) 结构损坏（截断 / 块长错乱 / 尺寸离谱）的文件在这里就被挡下，
	 *      根本不会送进 GDI+ —— v1.3.16 把完整字节直接喂 GDI+，
	 *      损坏的 PNG 会被真正解析，是新增的崩溃暴露面。 */
	int w = 0, h = 0;
	ImageProbeResult probe = ProbeImageHeader(m_raw.data(), m_raw.size(), &w, &h);

	if (probe == IMG_OK)
	{
		if (w > 0 && h > 0)
		{
			Size.cx = w;
			Size.cy = h;
			m_valid = true;
		}
	}
	else if (probe == IMG_UNKNOWN && GdiplusUsable())
	{
		/* 看不懂的文件（其它格式 / PNG 少了 IEND 等）：【交给 GDI+ 裁决】。
		 * 解析器不下否决结论 —— 只有能证明"确实损坏"时才拒绝，
		 * 免得把用户的合法图片误杀（第一版解析器就犯过这个错）。
		 * 注意这一步要持有 GDI+ 串行锁（v1.3.17 真正接上了这把锁）。 */
		std::lock_guard<std::mutex> lk(GdiplusMutex());
		IStream* stm = SHCreateMemStream((BYTE*)m_raw.data(), (ULONG)m_raw.size());
		if (stm)
		{
			Gdiplus::Bitmap* img = Gdiplus::Bitmap::FromStream(stm);
			if (img && img->GetLastStatus() == Gdiplus::Ok)
			{
				LONG iw = (LONG)img->GetWidth();
				LONG ih = (LONG)img->GetHeight();
				if (iw > 0 && ih > 0)
				{
					Size.cx = iw;
					Size.cy = ih;
					m_valid = true;
				}
			}
			if (img)
				delete img;
			stm->Release();
		}
	}
	/* IMG_SUSPECT：明确是坏文件 —— Size 保持 {0,0}，m_valid 保持 false，
	 * 上层会把它从图片列表里剔掉。绝不送进 GDI+。 */
}

BitmapGDI::~BitmapGDI()
{
	/* v1.3.18(审计修复 P0-2)：不再按 ownerId 清缓存 —— 缓存键已是内容哈希，
	 * 本对象销毁不代表"这份内容作废"：若用户只是增删了别的图片导致列表重建，
	 * 同一份内容的缩放结果/源位图对新对象依然有效，删了纯属浪费。
	 * 内存由"内容哈希 + 32MB/64MB LRU 双预算"自己管住，不会无界增长。 */

	/* v1.3.16：同样必须先把位图从 DC 取消选中再分别释放，
	 * 否则 DeleteDC 与 DeleteObject 会二次释放同一句柄、破坏堆。 */
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

/* ------------------------------------------------------------------
* v1.3.16：等比缩放渲染
*
* 为什么要单独做一层缩放，而不是直接 AlphaBlend(dst, src)：
*   实测 AlphaBlend 在源矩形与目标矩形尺寸不同时会做缩放，
*   但它是"最近邻"——4x4 放大到 40x40 得到的是纯色方块，
*   完全没有任何插值。图片被放大后必然是马赛克。
*
* 为什么不用 StretchBlt 预缩放：
*   实测 StretchBlt(32bpp -> 32bpp DIB) 会把目标位图的 alpha 通道
*   全部清零（本机实测 24000/24000 像素 alpha=0），
*   PNG 自带的透明背景会变成纯黑方块。GDI+ DrawImage 则完整保留 alpha。
*
* 为什么不用 GDI+ 直接画到目标 DC：
*   目标是 explorer 的 DUI 窗口 DC，在里面直接做 GDI+ 绘制会引入
*   额外的状态变化风险。离屏渲染 + AlphaBlend 1:1 贴图，
*   与旧版的合成路径完全一致，只是把"缩放"这一步提前做好了。
* ------------------------------------------------------------------ */

/* 单线程【缩放结果】缓存的字节预算（超出按 LRU 淘汰） */
static const size_t SCALE_CACHE_BUDGET = 32ull * 1024 * 1024;

/* v1.3.17：单线程【源位图】缓存的字节预算与条数上限。
 *
 * 源位图 = 按文件内容解码出来的原始尺寸位图，一张 1024x1536 的图
 * 就是 6MB。v1.3.16 这里完全没有约束（无上限泄漏的根源），
 * 现在给出硬上界：单线程最多 64MB / 16 张。
 *
 * 实际占用远低于此：随机模式下每张图片是【每个窗口创建时选一次】，
 * 不是每帧换（见 dllmain.cpp 的 MyCreateWindowEx），
 * 所以一个窗口线程正常只会用到 1~3 张图的源位图。 */
static const size_t SCALE_SOURCE_BUDGET = 64ull * 1024 * 1024;
static const size_t SCALE_SOURCE_MAX = 16;

/* ------------------------------------------------------------------
* v1.3.17：每线程一份的缩放缓存 —— 【必须用平凡可析构的指针存放】
*
* 为什么不能直接写 "static thread_local ScaleCache g_threadCache;"：
*   ScaleCache 里有 std::vector（非平凡析构），MinGW-w64 会给它注册
*   一个 TLS 析构回调。而这个 MinGW 在【线程退出】时跑 TLS 析构的时机
*   是错的 —— 析构函数跑在【已经被释放并被 ucrt 填成 0xEE 的 TLS 内存】上。
*
*   实测（probe/tls_probe.cpp，最小复现，与 GDI+ 完全无关）：
*       thread_local 里放 std::vector  -> 线程退出时崩
*       thread_local 里只放 int        -> 200 轮全过
*   崩溃时的析构日志：
*       dtor: items=17434477318113328897 cap=17434477263162962423
*   这两个数就是被污染的 vector 指针算出来的垃圾值。
*
*   真实症状（gdb 定位）：
*       ntdll!RtlExitUserThread -> tls_callback -> run_dtor_list
*         -> ScaleCache::~ScaleCache -> vector::~vector -> _M_deallocate
*       被 free 的指针是 0xfeeefeeefeeefeee（ucrt 的"已释放"填充图案）
*       => 二次释放 -> 堆损坏 -> 进程崩
*
*   这一点在 explorer 里是实打实的风险：explorer 的窗口 UI 线程会退出，
*   一退出就会踩到这颗雷，把整个资源管理器带走。
*   （v1.3.15/v1.3.16 里另一个 thread_local 是 MyData，全是 POD，
*     没有析构函数，所以一直没暴露这个问题。）
*
* 修法：thread_local 只放一个【裸指针】（平凡析构，不注册 TLS 回调），
*       真正的缓存对象堆上分配，登记到全局表；
*       窗口销毁时（MyDestroyWindow）主动释放本线程的缓存。
* ------------------------------------------------------------------ */
static thread_local ScaleCache* g_threadCachePtr = NULL;

/* 所有线程缓存的全局登记表（进程级，用于统一释放与防重复） */
struct CacheEntry
{
    ScaleCache* cache = NULL;
    DWORD        tid = 0;        /* 属主线程 id（用于判断线程是否还活着） */
};
static std::mutex              g_registryMx;
static std::vector<CacheEntry> g_registry;
static DWORD                   g_lastSweepTick = 0;

/* 死线程缓存的清扫间隔（毫秒） */
static const DWORD SWEEP_INTERVAL_MS = 500;

/* 线程是否还活着
 *
 * 【踩过的坑】一开始用的是 WaitForSingleObject(h, 0) == WAIT_TIMEOUT，
 * 但对【当前线程自己】等待时，WaitForSingleObject 会【立即返回
 * WAIT_OBJECT_0】（自己等自己，句柄立刻有信号）——
 * 于是"当前线程还活着"被误判成"已死"，清扫直接把【正在使用中】的
 * 缓存删掉，堆被破坏，进程 abort()（而且 abort 走的是 raise()，
 * 不经过 VEH，所以崩溃记录器里什么都看不到）。
 *
 * 正确做法：用 GetExitCodeThread 读线程退出码，跑着的线程是
 * STILL_ACTIVE(259)，已退出的线程是它的真实退出码。
 * 另外先把"当前线程"单独挡掉。
 */
static bool IsThreadAlive(DWORD tid)
{
    if (tid == 0)
        return false;
    if (tid == GetCurrentThreadId())
        return true;                         /* 当前线程永远活着 */

    HANDLE h = OpenThread(THREAD_QUERY_LIMITED_INFORMATION, FALSE, tid);
    if (!h)
        return false;                        /* 打不开 = 线程已经不在了 */

    DWORD code = 0;
    BOOL ok = GetExitCodeThread(h, &code);
    CloseHandle(h);
    if (!ok)
        return false;
    return (code == STILL_ACTIVE);
}

/* ------------------------------------------------------------------
* v1.3.17：清扫"线程已死但缓存还在"的孤儿缓存
*
* 为什么需要：
*   缩放缓存里装的是 GDI 句柄（每个缩放结果 1 个内存 DC + 1 个 HBITMAP）。
*   线程退出时我们【不能】靠 TLS 析构函数去释放（MinGW 那个时机是错的，
*   会崩），所以只能靠主动释放。而线程退出前如果窗口没走销毁流程
*   （或销毁时 t_myData 对不上），这份缓存就成了孤儿，
*   句柄一直占着 —— 实测多线程压测时句柄数 2583->5161->7739->9999，
*   撞上 GDI 句柄上限后 CreateCompatibleDC 失败，背景就画不出来了。
*
* 怎么清：
*   登记表里记着每个缓存的属主线程 id。定期检查：线程已经不在了，
*   说明没人再用它，直接释放（跨线程销毁 GDI 对象实测安全，
*   见 probe/xtest.cpp：3000/3000 通过）。
* ------------------------------------------------------------------ */
static void SweepDeadThreadCaches()
{
    DWORD now = GetTickCount();
    {
        std::lock_guard<std::mutex> lk(g_registryMx);
        if (now - g_lastSweepTick < SWEEP_INTERVAL_MS)
            return;
        g_lastSweepTick = now;
    }

    std::vector<CacheEntry> dead;
    {
        std::lock_guard<std::mutex> lk(g_registryMx);
        for (size_t i = 0; i < g_registry.size();)
        {
            if (!IsThreadAlive(g_registry[i].tid))
            {
                dead.push_back(g_registry[i]);
                g_registry.erase(g_registry.begin() + i);
            }
            else
            {
                i++;
            }
        }
    }

    /* 在锁外释放（~ScaleCache 内部自己会拿 GDI+ 锁）。
     * 跨线程删除 GDI+/GDI 对象实测安全（probe/xdel_probe.cpp 三种模式
     * 各 4 万次无崩溃）。 */
    for (size_t i = 0; i < dead.size(); i++)
        delete dead[i].cache;
}

/* 取本线程的缓存（没有就建一个）。
 *
 * 【注意】这里绝不能触发死线程清扫（SweepDeadThreadCaches）：
 *   本函数会在【持有 GDI+ 串行锁】的状态下被调用（见 GetScaled），
 *   而清扫会 delete 死线程的缓存 -> ~ScaleCache -> 也要拿 GDI+ 锁。
 *   std::mutex 不可重入，同一锁在同一线程里申请第二次就是自我死锁。
 *   （这正是第一版清扫一上线压测就挂死的原因：
 *     进程不是崩了，是 8 个线程全部卡死，最后被外层超时杀掉，
 *     所以崩溃记录器里什么都看不到。）
 *   清扫改由 GetScaled 在【拿锁之前】调用。 */
static ScaleCache& GetThreadCache()
{
    if (!g_threadCachePtr)
    {
        ScaleCache* c = new ScaleCache();
        {
            std::lock_guard<std::mutex> lk(g_registryMx);
            CacheEntry e;
            e.cache = c;
            e.tid = GetCurrentThreadId();
            g_registry.push_back(e);
        }
        g_threadCachePtr = c;
    }
    return *g_threadCachePtr;
}

/* 释放本线程的缓存（窗口销毁时调用）。
 * 返回 true 表示确实释放了一个。 */
static bool ReleaseThreadCacheImpl()
{
    if (!g_threadCachePtr)
        return false;

    ScaleCache* c = g_threadCachePtr;
    g_threadCachePtr = NULL;

    {
        std::lock_guard<std::mutex> lk(g_registryMx);
        for (size_t i = 0; i < g_registry.size(); i++)
        {
            if (g_registry[i].cache == c)
            {
                g_registry.erase(g_registry.begin() + i);
                break;
            }
        }
    }
    delete c;      /* ~ScaleCache 内部会持有 GDI+ 串行锁 */
    return true;
}

/* 释放所有线程的缓存（DLL 卸载时兜底调用） */
static void ReleaseAllCachesImpl()
{
    std::vector<CacheEntry> all;
    {
        std::lock_guard<std::mutex> lk(g_registryMx);
        all.swap(g_registry);
    }
    for (size_t i = 0; i < all.size(); i++)
        delete all[i].cache;
}

/* v1.3.16：GDI+ 序列化锁（在文件前部已声明，此处仅作说明） */

/* v1.3.18(审计修复 P0-2)：g_ownerMx / g_ownerSeq / NextScaleOwnerId 已删除，
 * 归属 id 机制整体退役（见文件前部说明）。 */

/* 丢弃某【内容】在当前线程缓存里的全部条目 */
static void ScaleCacheDrop(unsigned long long hash)
{
    /* 本线程压根没有缓存时不要硬造一个出来（那会让只负责析构的线程
     * 白白分配一份缓存）。 */
    if (!g_threadCachePtr)
        return;
    g_threadCachePtr->Drop(hash);
}

std::shared_ptr<ScaledBitmap> ScaleCache::Find(unsigned long long hash, int w, int h)
{
    for (size_t i = 0; i < m_items.size(); i++)
    {
        if (m_items[i].hash == hash
            && m_items[i].w == w && m_items[i].h == h)
        {
            m_items[i].useTick = ++m_tick;
            return m_items[i].bmp;
        }
    }
    return std::shared_ptr<ScaledBitmap>();
}

void ScaleCache::Put(unsigned long long hash, int w, int h,
    const std::shared_ptr<ScaledBitmap>& bmp)
{
    /* 同 key 已存在就替换（避免同一张图同尺寸堆出多条目） */
    for (size_t i = 0; i < m_items.size(); i++)
    {
        if (m_items[i].hash == hash
            && m_items[i].w == w && m_items[i].h == h)
        {
            m_items[i].bmp = bmp;
            m_items[i].useTick = ++m_tick;
            return;
        }
    }

    Entry e;
    e.hash = hash;
    e.w = w; e.h = h;
    e.bmp = bmp;
    e.useTick = ++m_tick;
    m_items.push_back(e);

    /* 按字节预算淘汰最久未用的条目 */
    size_t bytes = 0;
    for (size_t i = 0; i < m_items.size(); i++)
        bytes += (size_t)m_items[i].w * (size_t)m_items[i].h * 4;

    while (bytes > SCALE_CACHE_BUDGET && m_items.size() > 1)
    {
        size_t oldest = 0;
        for (size_t i = 1; i < m_items.size(); i++)
            if (m_items[i].useTick < m_items[oldest].useTick)
                oldest = i;
        bytes -= (size_t)m_items[oldest].w * (size_t)m_items[oldest].h * 4;
        m_items.erase(m_items.begin() + oldest);
    }
}

void ScaleCache::Drop(unsigned long long hash)
{
    for (size_t i = 0; i < m_items.size();)
    {
        if (m_items[i].hash == hash)
            m_items.erase(m_items.begin() + i);
        else
            i++;
    }
}

void ScaleCache::Clear()
{
    m_items.clear();
    /* v1.3.17：源位图也要清，否则清完还占着几十 MB */
    if (g_gdiplusAlive)
    {
        std::lock_guard<std::mutex> lk(GdiplusMutex());
        for (size_t i = 0; i < m_sources.size(); i++)
            if (m_sources[i].bmp)
                delete m_sources[i].bmp;
    }
    m_sources.clear();
}

/* v1.3.17：取本线程为该【内容】解码的源位图（没有则解码一份）。
 *
 * 【键 = 文件内容哈希】，不是 ownerId。这是修掉无上限泄漏的关键：
 *   同一份内容重建图片列表时哈希相同 -> 直接命中缓存，
 *   既不重复解码，也不重复占内存。
 *   换了内容 -> 旧条目按 LRU 预算淘汰并 delete，不会无限堆积。
 *
 * 【调用方必须已持有 GDI+ 串行锁】（由 GetScaled 统一加锁）。
 *
 * raw  : 图片的原始文件字节
 * hash : 文件内容哈希
 * w/h  : 源图尺寸
 */
Gdiplus::Bitmap* ScaleCache::GetSourceLocked(unsigned long long hash, int w, int h,
                                             const std::vector<BYTE>& raw)
{
    /* 已解码过？直接复用 */
    for (size_t i = 0; i < m_sources.size(); i++)
    {
        if (m_sources[i].hash == hash
            && m_sources[i].w == w && m_sources[i].h == h)
        {
            m_sources[i].useTick = ++m_srcTick;
            return m_sources[i].bmp;
        }
    }

    if (raw.empty() || w <= 0 || h <= 0)
        return NULL;

    /* 尺寸上限：损坏文件若把尺寸写得很离谱，这里直接拒绝，
     * 不让 GDI+ 去申请一个荒谬尺寸的位图。 */
    const unsigned long long need = (unsigned long long)w * (unsigned long long)h * 4ULL;
    if (need > 512ULL * 1024 * 1024)   /* 单张源位图上限 512MB */
        return NULL;

    /* 解码成 32bppPARGB（预乘 alpha）：
     * GDI+ 的合成运算在预乘空间进行，统一格式后
     * DrawImage 与 GetHBITMAP 的结果才稳定一致。 */
    /* v1.3.19(审计修复 P1-1)：流的释放时机。
     * GDI+ 的 Bitmap::FromStream 允许延迟解码 —— 流必须活到位图销毁为止。
     * 旧写法 FromStream 完立刻 Release，后面 DrawImage / delete decoded
     * 都在用"流已被释放"的位图，是明确的 API 所有权错误（探测路径
     * WinAPI.cpp 上方反而是对的：先 delete 位图再释放流）。
     * 现在所有分支统一按"先删位图、后释放流"收尾。 */
    IStream* stream = SHCreateMemStream((BYTE*)raw.data(), (ULONG)raw.size());
    if (!stream)
        return NULL;

    Gdiplus::Bitmap* decoded = Gdiplus::Bitmap::FromStream(stream);
    if (!decoded)
    {
        stream->Release();
        return NULL;
    }

    /* v1.3.19(审计修复 P2-3)：解码状态必须检查。
     * MinGW 下 FromStream 对损坏文件也可能返回非空指针
     * （dllmain.cpp 已有记录），只判空不够。 */
    if (decoded->GetLastStatus() != Gdiplus::Ok)
    {
        delete decoded;
        stream->Release();
        return NULL;
    }

    Gdiplus::Bitmap* pargb = new Gdiplus::Bitmap(w, h, PixelFormat32bppPARGB);
    if (!pargb || pargb->GetLastStatus() != Gdiplus::Ok)
    {
        delete pargb;
        delete decoded;
        stream->Release();
        return NULL;
    }
    bool drawOk = true;
    {
        Gdiplus::Graphics g(pargb);
        g.SetCompositingMode(Gdiplus::CompositingModeSourceCopy);
        g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
        g.Clear(Gdiplus::Color(0, 0, 0, 0));
        g.DrawImage(decoded, Gdiplus::RectF(0, 0, (Gdiplus::REAL)w, (Gdiplus::REAL)h));
        g.Flush(Gdiplus::FlushIntentionSync);
        /* v1.3.19(审计修复 P2-3)：DrawImage 失败不得进缓存 */
        drawOk = (g.GetLastStatus() == Gdiplus::Ok);
    }
    delete decoded;
    stream->Release();          /* v1.3.19：位图已销毁，现在才能释放流 */

    if (!drawOk)
    {
        delete pargb;
        return NULL;
    }

    /* 记进缓存 */
    SourceEntry e;
    e.hash = hash;
    e.w = w; e.h = h;
    e.bmp = pargb;
    e.bytes = (size_t)need;
    e.useTick = ++m_srcTick;
    m_sources.push_back(e);

    /* v1.3.17：按字节预算 + 条数上限淘汰最久未用的源位图。
     * 这两条约束合起来，源位图缓存的内存就有了硬上界，
     * 不会再出现"换一次图就永久多驻留上百 MB"的情况。 */
    size_t bytes = 0;
    for (size_t i = 0; i < m_sources.size(); i++)
        bytes += m_sources[i].bytes;

    while (m_sources.size() > 1
           && (bytes > SCALE_SOURCE_BUDGET || m_sources.size() > SCALE_SOURCE_MAX))
    {
        size_t oldest = 0;
        for (size_t i = 1; i < m_sources.size(); i++)
            if (m_sources[i].useTick < m_sources[oldest].useTick)
                oldest = i;

        bytes -= m_sources[oldest].bytes;
        if (m_sources[oldest].bmp)
            delete m_sources[oldest].bmp;      /* GDI+ 位图，必须在 GDI+ 活着时删 */
        m_sources.erase(m_sources.begin() + oldest);
    }

    return pargb;
}

/* v1.3.17：~ScaleCache 移到 .cpp —— 它要访问 GDI+ 串行锁。
 *
 * 注意【必须在 GDI+ 还活着时删源位图】：
 *   gdb 实测（v1.3.16）：线程退出时若 GDI+ 已被关闭，
 *   这里的 delete 会崩在 "delete m_sources[i].second"。
 *   本 DLL 从不调用 GdiplusShutdown，正常路径不会踩到；
 *   万一将来要关，先调 MarkGdiplusShutdown() 把标志置 false。
 */
ScaleCache::~ScaleCache()
{
    if (g_gdiplusAlive)
    {
        std::lock_guard<std::mutex> lk(GdiplusMutex());
        for (size_t i = 0; i < m_sources.size(); i++)
            if (m_sources[i].bmp)
                delete m_sources[i].bmp;
    }
    m_sources.clear();
    m_items.clear();
}

std::shared_ptr<ScaledBitmap> BitmapGDI::GetScaled(int dstW, int dstH)
{
    /* v1.3.17：源位图改为每线程各自解码，这里不再依赖 pMem/pBmp */
    if (!m_valid || Size.cx <= 0 || Size.cy <= 0 || dstW <= 0 || dstH <= 0)
        return std::shared_ptr<ScaledBitmap>();

    /* v1.3.19(审计修复 P1-2B)：GDI+ 启动失败后彻底停用解码路径 */
    if (!GdiplusUsable())
        return std::shared_ptr<ScaledBitmap>();

    /* v1.3.19(审计修复 P1-3)：目标位图面积硬上限。
     * 旧写法只查 dstW/dstH > 0，"缩放并填充"遇到 100000x1 这类极端宽高比
     * 会算出 1 亿 x 1080 的目标位图（数百 GiB 的分配请求）。
     * 256MB = 6400 万像素，覆盖 8K 全屏（约 3300 万像素）仍有余量；
     * 超限说明这张图配这个窗口不合常理，放弃绘制（不画背景 ≠ 崩溃）。 */
    const unsigned long long dstBytes =
        (unsigned long long)dstW * (unsigned long long)dstH * 4ULL;
    if (dstBytes > 256ULL * 1024 * 1024)
        return std::shared_ptr<ScaledBitmap>();

    /* 先查当前线程的缓存（thread_local，不需要加锁）。
     * v1.3.18：键从 ownerId 换成内容哈希（P0-2），列表重建后照常命中。 */
    std::shared_ptr<ScaledBitmap> hit = GetThreadCache().Find(m_hash, dstW, dstH);
    if (hit)
        return hit;

    /* 顺手清扫死线程留下的孤儿缓存。
     * 【必须放在拿 GDI+ 锁之前】—— 清扫会 delete 死线程的缓存，
     * ~ScaleCache 里要拿同一把锁；若此刻已持锁，就是自我死锁
     * （std::mutex 不可重入）。内部有 500ms 节流，平时几乎零开销。 */
    SweepDeadThreadCaches();

    /* ================================================================
     * v1.3.16：高质量等比缩放
     *
     * 【为什么用 GDI+ 而不是纯 GDI】
     *   实测三条路线：
     *     a) AlphaBlend 缩放：能保留 alpha，但插值是最近邻，放大后是马赛克；
     *     b) StretchBlt 缩放：插值是 HALFTONE 高质量，但它写出的位图
     *        之后不能再作为 AlphaBlend 的源（实测 AlphaBlend 直接返回 0），
     *        而且会把目标位图的 alpha 通道清零；
     *     c) GDI+ DrawImage + GetHBITMAP：插值是高质量双三次，alpha 完整保留，
     *        输出位图能正常用作 AlphaBlend 的源（实测 AlphaBlend 返回 1）。
     *   所以走 (c)。
     *
     * 【线程安全 —— v1.3.17 补齐】
     *   v1.3.16 的做法是"每线程各自解码一份、线程间零共享 GDI+ 对象"，
     *   规避了共享崩溃，但【解码本身】（FromStream）仍会在多个窗口线程上
     *   并发发生 —— 那把 g_gdiplusMx 定义了却一次都没用（审计报告问题 4）。
     *   现在把 GDI+ 的解码 / DrawImage / GetHBITMAP 全部收进这把全局锁里
     *   串行执行，既保留"零共享"的优点，又补上"解码也要串行"这最后一环。
     *
     * 【AlphaBlend 的两个硬性前提】（实测踩过的坑）
     *   1) 源 DC 必须用 CreateCompatibleDC(NULL) 创建（无调色板）；
     *      带调色板的 DC 会让 AlphaBlend 直接失败（返回 0）。
     *   2) 源矩形必须完整落在源位图内，否则同样返回 0。
     * ================================================================ */
    std::lock_guard<std::mutex> lk(GdiplusMutex());

    /* 等锁期间可能已被别的线程算好，再查一次 */
    hit = GetThreadCache().Find(m_hash, dstW, dstH);
    if (hit)
        return hit;

    Gdiplus::Bitmap* srcBmp = GetThreadCache().GetSourceLocked(m_hash,
        (int)Size.cx, (int)Size.cy, m_raw);
    if (!srcBmp)
        return std::shared_ptr<ScaledBitmap>();

    /* 1) 建一块 32bpp PARGB 的离屏位图作为缩放目标。
     *    用 PARGB（预乘）是因为 GDI+ 的合成运算在预乘空间进行，
     *    与源图格式一致，半透明边缘才不会出黑边。 */
    Gdiplus::Bitmap* scaled =
        new Gdiplus::Bitmap((INT)dstW, (INT)dstH, PixelFormat32bppPARGB);
    if (!scaled || scaled->GetLastStatus() != Gdiplus::Ok)
    {
        delete scaled;
        return std::shared_ptr<ScaledBitmap>();
    }

    /* 2) 用 GDI+ 把原图等比插值画进去。
     *    CompositingModeSourceCopy：直接覆盖而不是与底色混合，
     *    保证透明像素仍然是透明的（不会被乘上底色）。
     *    高质量双三次插值：放大后平滑，不是马赛克。 */
    {
        Gdiplus::Graphics g(scaled);
        g.SetCompositingMode(Gdiplus::CompositingModeSourceCopy);
        g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
        g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
        g.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
        g.Clear(Gdiplus::Color(0, 0, 0, 0));
        g.DrawImage(srcBmp, Gdiplus::RectF(0, 0, (Gdiplus::REAL)dstW, (Gdiplus::REAL)dstH),
            0, 0, (Gdiplus::REAL)Size.cx, (Gdiplus::REAL)Size.cy, Gdiplus::UnitPixel);
        g.Flush(Gdiplus::FlushIntentionSync);
    }

    /* 3) 转成 HBITMAP，供 AlphaBlend 1:1 贴图。
     *    背景色传全透明，与旧版 BitmapGDI 构造时的写法保持一致：
     *    GetHBITMAP 会保留源位图的 alpha 通道（实测原样透传）。 */
    HBITMAP hb = NULL;
    scaled->GetHBITMAP(Gdiplus::Color(0, 0, 0, 0), &hb);
    delete scaled;

    if (!hb)
        return std::shared_ptr<ScaledBitmap>();

    std::shared_ptr<ScaledBitmap> out = std::make_shared<ScaledBitmap>();
    out->pBmp = hb;
    out->w = dstW;
    out->h = dstH;

    /* 源 DC 必须是 CreateCompatibleDC(NULL) 型（无调色板），
     * 否则 AlphaBlend 会拒绝合成（返回 0），背景就画不出来。 */
    out->pMem = CreateCompatibleDC(NULL);
    /* v1.3.18(审计修复 P1-3)：失败分支必须释放已到手的 HBITMAP，
     * 否则就是句柄泄漏 —— GDI 句柄紧张时这条路径最容易命中，
     * 而命中后背景必然画不出来，等于故障自锁。 */
    if (!out->pMem)
    {
        DeleteObject(hb);
        out->pBmp = 0;
        return std::shared_ptr<ScaledBitmap>();
    }
    /* v1.3.18(审计修复 P1-3)：SelectObject 选位图失败返回 NULL，
     * 不检查的话 pMem 里没有选中任何位图，AlphaBlend 拿空 DC 当源。 */
    if (!SelectObject(out->pMem, out->pBmp))
    {
        DeleteDC(out->pMem);
        out->pMem = 0;
        DeleteObject(hb);
        out->pBmp = 0;
        return std::shared_ptr<ScaledBitmap>();
    }

    GetThreadCache().Put(m_hash, dstW, dstH, out);
    return out;
}

void BitmapGDI::DropScaled()
{
    /* v1.3.18：按内容哈希丢弃（键已从 ownerId 换成 hash） */
    ScaleCacheDrop(m_hash);
}

/* ------------------------------------------------------------------
* v1.3.17：对外接口（见 WinAPI.h 里的说明）
* ------------------------------------------------------------------ */
bool ReleaseThreadCache()
{
    return ReleaseThreadCacheImpl();
}

void ReleaseAllCaches()
{
    ReleaseAllCachesImpl();
}

/* ------------------------------------------------------------------
* v1.3.20(A)：高质量随机下标（见 WinAPI.h 的说明）
*
* 播种策略：random_device（系统熵源）^ GetTickCount ^ 进程内静态
* 地址（ASLR 基址）。三路混种是为兜底个别 MinGW 工具链的
* random_device 实现质量不稳的情况 —— 任何一路有效都不至于
* 逐字复现序列。引擎进程内只初始化一次（C++11 静态局部线程安全）。
* v1.3.21(D4，Deepseek 审计)：注释原写 GetTickCount64，实际用
* GetTickCount（v1.3.20 编译期因工具链头文件未暴露该符号而改），
* 32 位计数约 49.7 天回绕，但播种另有熵源兜底，回绕无碍，注释对齐。
* ------------------------------------------------------------------ */
int RandomIndex(int n)
{
    if (n <= 1)
        return 0;

    static std::mutex s_rndMx;
    static std::mt19937 s_eng(
        (std::mt19937::result_type)std::random_device{}()
        ^ (std::mt19937::result_type)GetTickCount()
        ^ (std::mt19937::result_type)(uintptr_t)&s_eng);

    std::lock_guard<std::mutex> lk(s_rndMx);
    std::uniform_int_distribution<int> dist(0, n - 1);
    return dist(s_eng);
}
