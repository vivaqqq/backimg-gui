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
#include <set>
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

/* v1.3.29(R5/R4-1)：尾部终止符验证 —— 反向搜索最后一处终止符 + 距尾阈值。
 * 纯函数（无 I/O），可独立单测；buf 必须是文件的尾部窗口（末端=文件末尾）。
 * 反向搜索的动机：真实相机/手机 JPEG 的 APP1(EXIF) 段内嵌完整缩略图
 * JPEG（自带 FF D9）——前向搜索首个命中的是缩略图 EOI，带缩略图文件的
 * 截断与否一律误判 FOUND（R4-1 实测复现）；反向搜索从尾部开始，首个命中
 * 即主图真 EOI，天然免疫。JPEG 熵编码有字节填充，图像数据里不会出现
 * FF D9，反向搜索同样安全。
 * FOUND  ：最后一处终止符距尾 <= TAIL_TOL（保留 v1.3.19"容忍尾部少量
 *          追加"语义）；
 * ABSENT ：尾部窗口内无终止符，或最后一处距尾超阈 → 确证截断
 *          （距尾 TAIL_TOL 内无终止符 ⇒ "最后一处终止符"距尾 > TAIL_TOL
 *          或不存在，两种情况同判 ABSENT，无需找全文件）；
 * UNKNOWN：窗口异常小，证据不足（调用方放行，绘制期解码 + S3 TTL 兜底）。 */
enum TailVerdict { TV_FOUND, TV_ABSENT, TV_UNKNOWN };

#define TAIL_TOL 4096              /* 距尾容忍：尾部追加 <= 4KB 视为合法 */
#define TAIL_WIN (TAIL_TOL + 32)   /* 读尾窗口：阈值 + 终止符长度 + 余量 */

static TailVerdict VerifyTailOnBuf(const BYTE* buf, size_t bufLen, bool isJpeg)
{
	const size_t termLen = isJpeg ? 2 : 8;
	if (bufLen < termLen)
		return TV_UNKNOWN;
	long long last = -1;
	if (isJpeg)
	{
		for (long long i = (long long)bufLen - 2; i >= 0; --i)
			if (buf[i] == 0xFF && buf[i + 1] == 0xD9) { last = i; break; }
	}
	else
	{
		/* PNG IEND 块：length(4B=0) + "IEND"；其后还有 4B CRC 在距尾判定之外 */
		for (long long i = (long long)bufLen - 8; i >= 0; --i)
			if (buf[i] == 0 && buf[i + 1] == 0 && buf[i + 2] == 0 && buf[i + 3] == 0
				&& buf[i + 4] == 'I' && buf[i + 5] == 'E'
				&& buf[i + 6] == 'N' && buf[i + 7] == 'D') { last = i; break; }
	}
	if (last < 0)
		return TV_ABSENT;
	const long long tail = (long long)bufLen - (last + (long long)termLen);
	return tail <= TAIL_TOL ? TV_FOUND : TV_ABSENT;
}

/* v1.3.29(R6/B)：EOI 之后是否紧跟已知容器签名（Motion Photo / 追加载荷）。
 * 在 p 起 16 字节窗口内找：MP4(ftyp)/ZIP(PK)/RIFF/Samsung(SEF)/次级
 * JPEG(FF D8 FF)/PNG 签名——熵数据在 16 字节窗口内命中 4 字节签名的
 * 概率 ~2^-30，可忽略，不会把截断图误判成载荷。 */
static bool LooksLikePayload(const BYTE* p, size_t avail)
{
	const size_t W = avail < 16 ? avail : 16;
	if (W < 4) return false;
	for (size_t k = 0; k + 4 <= W; ++k)
	{
		if (memcmp(p + k, "ftyp", 4) == 0) return true;          /* MP4 box */
		if (memcmp(p + k, "PK\x03\x04", 4) == 0) return true;   /* ZIP */
		if (memcmp(p + k, "RIFF", 4) == 0) return true;
		if (memcmp(p + k, "SEFH", 4) == 0) return true;          /* Samsung */
		if (memcmp(p + k, "\x89PNG", 4) == 0) return true;
	}
	if (W >= 3)
		for (size_t k = 0; k + 3 <= W; ++k)
			if (p[k] == 0xFF && p[k + 1] == 0xD8 && p[k + 2] == 0xFF) return true;
	return false;
}

/* v1.3.29(R6/B)：载荷救回 —— JPEG 判 ABSENT 后的二次判定（R5-1）。
 * "尾部无终止符"≠"文件坏了"：Motion Photo（主图 EOI 后挂 MP4，手机
 * 相册常见）、导出工具追加的元数据等合法文件都会落进 ABSENT，而它们
 * 的主图完整、GDI+ 能正常解码。判据：全文件反向找最后一处 FF D9——
 * 其后紧跟容器签名 → 主图完整的载荷文件，放行（截断文件的最后终止符
 * 是缩略图 EOI，其后是主图熵数据，非签名 → 维持剔除；MP4 内部散落的
 * FF D9 非签名，反向扫描自然跳过）；读失败 → UNKNOWN（放行）。 */
static TailVerdict RescuePayloadJPEG(FILE* fp, long long total,
                                     const BYTE* head, size_t headLen)
{
	if (headLen >= (size_t)total)
	{
		/* 小文件：整份在缓冲，零 I/O */
		for (long long i = (long long)headLen - 2; i >= 0; --i)
			if (head[i] == 0xFF && head[i + 1] == 0xD9
				&& LooksLikePayload(head + i + 2, headLen - (size_t)i - 2))
				return TV_FOUND;
		return TV_ABSENT;
	}
	/* 大文件：从尾按 1MB 块反向读，块间重叠 3 字节防 FF D9 跨界。
	 * 只对"尾 4KB 无终止符"的文件走到这里（正常图片第 1 步即结束）。 */
	const long long CHUNK = 1024 * 1024;
	long long end = total;
	std::vector<BYTE> buf;
	while (end > 0)
	{
		long long start = end - CHUNK > 0 ? end - CHUNK : 0;
		size_t len = (size_t)(end - start);
		buf.resize(len);
		if (_fseeki64(fp, start, SEEK_SET) != 0
			|| fread(&buf[0], 1, len, fp) != len)
			return TV_UNKNOWN;
		for (long long i = (long long)len - 2; i >= 0; --i)
		{
			if (buf[i] != 0xFF || buf[i + 1] != 0xD9)
				continue;
			/* 候选命中但后文不足 16 字节（EOI 恰落在块尾附近，签名窗口
			 * 被块边界切断——R6 实测 motion.jpg 因此漏判）：用句柄从文件
			 * 补读 16 字节再判。距尾 >4KB 的 EOI 后文必然完整在文件内。 */
			const BYTE* q = &buf[i + 2];
			size_t avail = len - (size_t)i - 2;
			BYTE extra[16];
			if (avail < 16)
			{
				if (_fseeki64(fp, start + i + 2, SEEK_SET) != 0)
					continue;
				DWORD got = (DWORD)fread(extra, 1, 16, fp);
				q = extra;
				avail = (size_t)got;
			}
			if (LooksLikePayload(q, avail))
				return TV_FOUND;
		}
		if (start == 0) break;
		end = start + 3;
	}
	return TV_ABSENT;
}

/* v1.3.29(R6/A)：扫描剔除登记 —— "某张图为什么没出现"要有据可查
 * （R5-1：静默吞图无日志）。进程内同一文件只记一次，防重建灌日志；
 * 目录不可写静默放弃，绝不影响 explorer。 */
static void LogRejectedFile(const std::wstring& path, const wchar_t* reason)
{
	static std::mutex s_mx;
	static std::set<std::wstring> s_seen;
	{
		std::lock_guard<std::mutex> lk(s_mx);
		if (!s_seen.insert(path).second)
			return;
	}
	SYSTEMTIME st;
	GetLocalTime(&st);
	std::wstring name = path.substr(path.find_last_of(L"\\/") + 1);
	wchar_t line[560];
	swprintf(line, 560, L"[%02d:%02d:%02d] %ls  (%ls)\r\n",
	         st.wHour, st.wMinute, st.wSecond, name.c_str(), reason);
	std::wstring text = L"[扫描剔除] " + std::wstring(line);
	int need = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), (int)text.size(),
	                               NULL, 0, NULL, NULL);
	if (need <= 0) return;
	std::string utf8((size_t)need, '\0');
	WideCharToMultiByte(CP_UTF8, 0, text.c_str(), (int)text.size(),
	                    &utf8[0], need, NULL, NULL);
	HANDLE h = CreateFileW((GetCurDllDir() + L"\\ExplorerBgTool_init.log").c_str(),
	                       FILE_APPEND_DATA, FILE_SHARE_READ, NULL,
	                       OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (h == INVALID_HANDLE_VALUE) return;
	DWORD wr = 0;
	WriteFile(h, utf8.data(), (DWORD)utf8.size(), &wr, NULL);
	CloseHandle(h);
}




BitmapGDI::BitmapGDI(std::wstring path)
{
	//这样加载是为了防止文件被占用
	//v1.3.29(R2/L1)：扫描期只读文件头（≤256KB），不再全量读入——
	//建列表只需要"宽高 + 确证未损坏"，完整解码挪到绘制期按需进行（L3）。
	//旧版把整个文件读进 m_raw 常驻，148 张大图 = GB 级 I/O 与内存。
	const long HEAD_MAX = 256L * 1024;
	FILE* file = nullptr;
	_wfopen_s(&file, path.c_str(), L"rb");
	if (!file)
		return;                       /* 打不开：Size 保持 {0,0}，m_valid 保持 false */

	fseek(file, 0L, SEEK_END);
	long long total = _ftelli64(file);
	rewind(file);
	if (total <= 0 || total > 256LL * 1024 * 1024)   /* 沿用旧 256MB 单文件上限 */
	{
		fclose(file);
		return;
	}

	/* v1.3.29(R2/L2)：轻量指纹 = 头部字节哈希 ^ (文件大小 × FNV 素数)。
	 * 取代旧"全量内容 FNV"作缓存键 —— 同文件重建列表键不变（缓存照常
	 * 命中），内容被替换时头部或大小几乎必然变化（键自然失效）。 */
	size_t headLen = (size_t)(total < (long long)HEAD_MAX ? total : (long long)HEAD_MAX);
	std::vector<BYTE> head(headLen);
	size_t rd = fread(&head[0], 1, headLen, file);
	/* v1.3.29(R4/R3-3)：句柄暂不关——大文件的尾部终止符检查复用同一
	 * 句柄（省一次 open 往返，网络盘收益明显），函数尾统一 fclose。 */
	if (rd != headLen)                /* 读不满：文件可能正被改写 */
	{
		fclose(file);
		return;
	}

	m_hash = HashBytes(head.data(), head.size())
	       ^ ((unsigned long long)total * 1099511628211ULL);

	/* v1.3.29(R2/L1)：headOnly = 缓冲区不足整个文件 —— JPEG 跳过 EOI 尾检、
	 * PNG 校验完 IHDR 即判 OK（截断文件由绘制期解码兜底，见 ImageHeader.h）。
	 * 小于 256KB 的文件整文件在缓冲区里，保持 v1.3.28 的严格语义。 */
	int w = 0, h = 0;
	ImageProbeResult probe = ProbeImageHeader(head.data(), head.size(), &w, &h,
	                                          headLen < (size_t)total);

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
		/* 看不懂的文件（其它格式 / 异形 marker 链等）：【交给 GDI+ 裁决】。
		 * v1.3.29(R2/L1)：改用 Bitmap::FromFile —— 惰性解析，取宽高只读
		 * 文件头，之后立刻 delete（句柄随之关闭），不再整文件进内存。 */
		std::lock_guard<std::mutex> lk(GdiplusMutex());
		Gdiplus::Bitmap* img = Gdiplus::Bitmap::FromFile(path.c_str());
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
	}
	/* IMG_SUSPECT：明确是坏文件 —— Size 保持 {0,0}，m_valid 保持 false，
	 * 上层会把它从图片列表里剔掉。绝不送进 GDI+。 */

	/* v1.3.29(R5/R4-1)：尾部终止符验证（VerifyTailOnBuf 反向搜索 + 4KB 距离
	 * 阈值；三态语义承接 R4：UNKNOWN=读失败放行交绘制期兜底）。大文件复用
	 * 头读句柄读尾部窗口（R4/R3-3 单次开档）；小文件整份在缓冲零 I/O 直取
	 * 尾部窗口。此前的内联判定（R3~R4 累计 ~110 行 / 6 层嵌套 / 第 4 次修改
	 * 同一段）按审计建议抽取为纯函数——本轮 R4-1（前向搜索撞 EXIF 缩略图
	 * EOI）正是第 5 次回同一段代码的风险兑现，抽取后构造函数只留调用。 */
	if (m_valid && total > 12
		&& ((head[0] == 0xFF && head[1] == 0xD8)
			|| memcmp(&head[0], kPngSig, 8) == 0))
	{
		const bool isJpeg = (head[0] == 0xFF && head[1] == 0xD8);
		TailVerdict tv;
		if (headLen < (size_t)total)
		{
			/* 大文件：读尾部窗口。读失败 = 没读到 ≠ 文件坏了（R4/R3-2）。 */
			const long long W = total > TAIL_WIN ? TAIL_WIN : total;
			std::vector<BYTE> buf((size_t)W);
			if (_fseeki64(file, total - W, SEEK_SET) == 0
				&& fread(&buf[0], 1, (size_t)W, file) == (size_t)W)
				tv = VerifyTailOnBuf(&buf[0], (size_t)W, isJpeg);
			else
				tv = TV_UNKNOWN;
		}
		else
		{
			/* 小文件：整份在缓冲，零 I/O 直取尾部窗口（搜索必然完成，
			 * 结论只会是 FOUND / ABSENT）。 */
			const size_t w = headLen > TAIL_WIN ? TAIL_WIN : headLen;
			tv = VerifyTailOnBuf(&head[headLen - w], w, isJpeg);
		}
		if (tv == TV_ABSENT && isJpeg)
		{
			/* v1.3.29(R6/B)：载荷救回二次判定（Motion Photo 等，见函数注释）。 */
			tv = RescuePayloadJPEG(file, total, head.data(), headLen);
		}
		if (tv == TV_ABSENT)
		{
			Size.cx = 0;
			Size.cy = 0;
			m_valid = false;      /* 确证截断：本轮不进列表 */
			/* v1.3.29(R6/A)：剔除必留痕（R5-1）。 */
			LogRejectedFile(path, isJpeg ? L"尾部无终止符且无载荷标记(疑截断)"
			                             : L"尾部无IEND(疑截断)");
		}
		/* TV_UNKNOWN：证据不足，保持 m_valid 放行——绘制期解码与
		 * S3 TTL 兜底（R4/R3-2 语义不变）。 */
	}
	fclose(file);                /* v1.3.29(R4/R3-3)：统一在此关档 */

	/* v1.3.29(R2/L3)：记录路径，绘制期按需解码（旧 m_raw 成员已废除） */
	m_path = path;
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
 * path : 图片文件路径（v1.3.29(R2/L3)：解码按需读文件）
 * decodeFailed : 出参，true = 文件级解码失败（调用方应判死该图）
 * hash : 文件内容哈希
 * w/h  : 源图尺寸
 */
Gdiplus::Bitmap* ScaleCache::GetSourceLocked(unsigned long long hash, int w, int h,
                                             const std::wstring& path, bool* decodeFailed)
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

    if (path.empty() || w <= 0 || h <= 0)
        return NULL;

    /* 尺寸上限：损坏文件若把尺寸写得很离谱，这里直接拒绝，
     * 不让 GDI+ 去申请一个荒谬尺寸的位图。 */
    const unsigned long long need = (unsigned long long)w * (unsigned long long)h * 4ULL;
    if (need > 512ULL * 1024 * 1024)   /* 单张源位图上限 512MB */
        return NULL;

    /* 解码成 32bppPARGB（预乘 alpha）：GDI+ 的合成运算在预乘空间进行，
     * 统一格式后 DrawImage 与 GetHBITMAP 的结果才稳定一致。
     * v1.3.29(R2/L3)：解码改按需读文件 —— Bitmap::FromFile 惰性解析，
     * 真正解码发生在下面 DrawImage；位图 delete 后句柄即关，不长期锁文件。
     * 旧路径 SHCreateMemStream(raw) 依赖全量常驻字节，已随 m_raw 废除。
     * 失败语义：FromFile 失败/状态非 Ok = 文件级失败 → decodeFailed=true，
     * 调用方（GetScaled）将本图判死；其余失败（分配/DrawImage）不判死。 */
    if (decodeFailed) *decodeFailed = false;

    Gdiplus::Bitmap* decoded = Gdiplus::Bitmap::FromFile(path.c_str());
    if (!decoded)
    {
        if (decodeFailed) *decodeFailed = true;
        return NULL;
    }

    /* v1.3.19(审计修复 P2-3)：解码状态必须检查。
     * MinGW 下 FromStream 对损坏文件也可能返回非空指针
     * （dllmain.cpp 已有记录），只判空不够。 */
    if (decoded->GetLastStatus() != Gdiplus::Ok)
    {
        delete decoded;
        if (decodeFailed) *decodeFailed = true;
        return NULL;
    }

    Gdiplus::Bitmap* pargb = new Gdiplus::Bitmap(w, h, PixelFormat32bppPARGB);
    if (!pargb || pargb->GetLastStatus() != Gdiplus::Ok)
    {
        delete pargb;
        delete decoded;
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
    if (IsDead() || !m_valid || Size.cx <= 0 || Size.cy <= 0 || dstW <= 0 || dstH <= 0)
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

    bool decodeFailed = false;
    Gdiplus::Bitmap* srcBmp = GetThreadCache().GetSourceLocked(m_hash,
        (int)Size.cx, (int)Size.cy, m_path, &decodeFailed);
    if (!srcBmp)
    {
        /* v1.3.29(R3/S3)：文件级解码失败 → 判死本图（带 5 分钟 TTL，
         * 瞬时故障自动复活重试；截断类损坏不经过这里——那条防线在
         * 扫描期尾部终止符检查 R3/S1）。 */
        if (decodeFailed)
            m_deadSince.store(GetTickCount64(), std::memory_order_relaxed);
        return std::shared_ptr<ScaledBitmap>();
    }

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
