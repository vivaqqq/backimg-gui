/*
* BHO���������
*
* Author: Maple
* date: 2022-1-31 Create
* Copyright winmoes.com
*/

#include "ShellLoader.h"

long g_cDllRef = 0;

/*������޸��˴��� ��ʹ��VS��GUID���������µ�GUID��*/
const std::wstring CLSID_SHELL_BHO_STR = L"{ED15A97D-FE3E-4CDE-98FF-BC46B02896B0}";
const CLSID CLSID_SHELL_BHO = { 0xed15a97d, 0xfe3e, 0x4cde, { 0x98, 0xff, 0xbc, 0x46, 0xb0, 0x28, 0x96, 0xb0 } };

#pragma region CObjectWithSite

CObjectWithSite::CObjectWithSite()
{
	OnWindowLoad();
}

CObjectWithSite::~CObjectWithSite()
{
	ReleaseRes();
}

STDMETHODIMP CObjectWithSite::QueryInterface(REFIID riid, void** ppv)
{
	/* v1.3.19(审计修复 P3-3)：COM 契约 —— 无论成败，输出参数都要有确定值。
	 * 入口判空防呆，失败路径清 *ppv，防调用方拿到未初始化指针。 */
	if (!ppv)
		return E_POINTER;
	*ppv = NULL;
	if (riid == IID_IUnknown)
		*ppv = static_cast<CObjectWithSite*>(this);
	else if (riid == IID_IObjectWithSite)
		*ppv = static_cast<IObjectWithSite*>(this);
	else
		return E_NOINTERFACE;
	AddRef();
	return S_OK;
}

ULONG __stdcall CObjectWithSite::AddRef()
{
	InterlockedIncrement(&g_cDllRef);
	return InterlockedIncrement(&m_ref);
}

ULONG __stdcall CObjectWithSite::Release()
{
	int tmp = InterlockedDecrement(&m_ref);
	if (tmp == 0)
		delete this;
	InterlockedDecrement(&g_cDllRef);
	return tmp;
}


STDMETHODIMP CObjectWithSite::SetSite(IUnknown* pUnkSite)
{
	/* v1.3.18(审计修复)：补 pUnkSite 判空。COM 约定卸载时会调 SetSite(NULL)，
	 * 原写法直接 pUnkSite->QueryInterface，对空指针做虚调用必崩。 */
	ReleaseRes();
	if (!pUnkSite)
		return S_OK;
	HRESULT hr = pUnkSite->QueryInterface(IID_IWebBrowser2, (void**)&m_web);
	if (FAILED(hr))
		ReleaseRes();
	return hr;
}

void CObjectWithSite::ReleaseRes()
{
	if (m_web) m_web->Release();
	m_web = 0;
}

#pragma endregion

#pragma region ClassFactory

STDMETHODIMP ClassFactory::QueryInterface(REFIID riid, void** ppv)
{
	/* v1.3.19(审计修复 P3-3)：同 CObjectWithSite::QueryInterface */
	if (!ppv)
		return E_POINTER;
	*ppv = NULL;
	if (riid == IID_IUnknown || riid == IID_IClassFactory)
	{
		*ppv = this;
		AddRef();
		return S_OK;
	}
	return E_NOINTERFACE;
}

ULONG __stdcall ClassFactory::AddRef()
{
	/* v1.3.18(审计修复)：原写法两行都递增 g_cDllRef，m_ref 从不自增 ——
	 * 引用计数错乱：g_cDllRef 每次 AddRef/Release 循环净涨 1（虚高永不归零），
	 * 工厂自身计数永远为 0。现与 CObjectWithSite::AddRef 同一模式：
	 * 先记模块级计数，再递增并返回对象自身计数。 */
	InterlockedIncrement(&g_cDllRef);
	return InterlockedIncrement(&m_ref);
}

ULONG __stdcall ClassFactory::Release()
{
	int tmp = InterlockedDecrement(&m_ref);
	if (tmp == 0)
		delete this;
	InterlockedDecrement(&g_cDllRef);
	return tmp;
}

STDMETHODIMP ClassFactory::LockServer(BOOL fLock)
{ 
	if (fLock)
		InterlockedIncrement(&g_cDllRef);
	else 
		InterlockedDecrement(&g_cDllRef);
	return S_OK;
}
STDMETHODIMP ClassFactory::CreateInstance(LPUNKNOWN pUnkOuter, REFIID riid, LPVOID* ppvObj)
{
	/* v1.3.19(审计修复 P3-3 + P1-4)：入口清空输出参数；new 失败的
	 * bad_alloc 不允许穿透 COM 边界，转成 HRESULT 返回。 */
	if (!ppvObj)
		return E_POINTER;
	*ppvObj = NULL;
	if (pUnkOuter)
		return CLASS_E_NOAGGREGATION;
	try
	{
		CObjectWithSite* bho = new CObjectWithSite();
		bho->AddRef();
		HRESULT hr = bho->QueryInterface(riid, ppvObj);
		bho->Release();
		return hr;
	}
	catch (...)
	{
		return E_OUTOFMEMORY;
	}
}

#pragma endregion

#pragma region Reg

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppvOut)
{
	/* v1.3.19(审计修复 P3-3 + P1-4)：输出参数判空 + 异常边界同上 */
	if (!ppvOut)
		return E_POINTER;
	*ppvOut = NULL;
	if (rclsid != CLSID_SHELL_BHO)
		return CLASS_E_CLASSNOTAVAILABLE;
	/* v1.3.18(审计修复)：工厂改为【堆分配】。
	 * 原写法是文件级 static 对象 + 引用计数从不递增，两者"负负得正"地
	 * 没出事；但 AddRef 修正后 m_ref 会真实涨落，Release 归零时的
	 * delete this 就会落在 static 存储上 —— 对非堆对象 delete 是未定义行为。
	 * 堆分配后引用计数闭环：调用方 Release 到 0 时 delete 合法且正确。
	 * QI 失败（未加引用）时自行 delete，避免泄漏。 */
	try
	{
		ClassFactory* factory = new ClassFactory();
		HRESULT hr = factory->QueryInterface(riid, ppvOut);
		if (FAILED(hr))
			delete factory;
		return hr;
	}
	catch (...)
	{
		return E_OUTOFMEMORY;
	}
}

STDAPI DllCanUnloadNow(void)
{
	/* v1.3.18(审计修复 P1-4)：恒返回 S_FALSE，把 DLL 钉在进程里。
	 *
	 * 原因：全 DLL 从不调用 MH_DisableHook / MH_Uninitialize，
	 * 6 个钩子的跳板指向本 DLL 的代码页。一旦系统真的采信 S_OK 去
	 * FreeLibrary，DLL 解除映射后 explorer 任何一次 CreateWindowExW /
	 * BeginPaint / FillRect 都会跳进无映射内存 —— 整个资源管理器被带走。
	 * 这是 shell 扩展的常规务实做法：DLL 随进程退出收尾，
	 * 卸载只发生在 regsvr32 /u 的进程退出路径，不存在中途 FreeLibrary。
	 * （g_cDllRef 记账保留，供诊断观察引用情况。） */
	return S_FALSE;
}

STDAPI DllRegisterServer()
{
	HKEY hkey = 0;
	/* v1.3.19(P2-7A): buffer zeroed + GetModuleFileNameW return checked.
	 * old code left dllpath uninitialized and unchecked: on failure or
	 * truncation wcslen reads past the buffer and a garbage path gets
	 * written into the registry. */
	wchar_t dllpath[MAX_PATH] = { 0 };
	DWORD nPath = GetModuleFileNameW(g_hModule, dllpath, MAX_PATH);
	if (nPath == 0 || nPath >= MAX_PATH)
		return SELFREG_E_CLASS;

	//����CLSID
	std::wstring regpath = L"CLSID\\" + CLSID_SHELL_BHO_STR;
	if (RegCreateKeyExW(HKEY_CLASSES_ROOT,regpath.c_str(), 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hkey, NULL) != ERROR_SUCCESS)
		return SELFREG_E_CLASS;

	//����COM�������
	/* v1.3.19(P2-7C): critical value writes now report real results */
	if (RegSetValueExW(hkey, NULL, 0, REG_SZ, (const BYTE*)L"CodeProject Example BHO", 24 * sizeof(wchar_t)) != ERROR_SUCCESS)
	{
		RegCloseKey(hkey);
		return SELFREG_E_CLASS;
	}
	RegCloseKey(hkey);

	//����InProcServer32
	if (RegCreateKeyExW(HKEY_CLASSES_ROOT, (regpath + L"\\InProcServer32").c_str(), 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hkey, NULL) != ERROR_SUCCESS)
		return SELFREG_E_CLASS;

	//����dllλ��
	/* v1.3.19(P2-7C): critical value writes now report real results */
	if (RegSetValueExW(hkey, NULL, 0, REG_SZ, (const BYTE*)dllpath, (DWORD)((wcslen(dllpath) + 1) * sizeof(wchar_t))) != ERROR_SUCCESS)
	{
		RegCloseKey(hkey);
		return SELFREG_E_CLASS;
	}
	// Set the ThreadingModel to Apartment
	RegSetValueExW(hkey, L"ThreadingModel", 0, REG_SZ, (const BYTE*)L"Apartment", 10 * sizeof(wchar_t));
	RegCloseKey(hkey);

	//ע��BHO���
	if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, (LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\Browser Helper Objects\)"
		+ CLSID_SHELL_BHO_STR).c_str(), 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hkey, NULL) != ERROR_SUCCESS)
		return SELFREG_E_CLASS;

	//��ֹIE��������ر����
	DWORD value = 1;
	RegSetValueEx(hkey, L"NoInternetExplorer", 0, REG_DWORD, (const BYTE*)&value, sizeof(DWORD));
	RegCloseKey(hkey);
	return S_OK;
}

STDAPI DllUnregisterServer()
{
	//ע��BHO���
	/* v1.3.19(P2-7B): path unified with the register side (old code had a
	 * doubled backslash here; registry APIs usually normalize it, but both
	 * sides must spell the same key to be reliable) */
	RegDeleteKeyW(HKEY_LOCAL_MACHINE, (LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\Browser Helper Objects\)"
		+ CLSID_SHELL_BHO_STR).c_str());
	//����COM�������
	RegDeleteKey(HKEY_CLASSES_ROOT, (L"CLSID\\" + CLSID_SHELL_BHO_STR + L"\\InProcServer32").c_str());
	RegDeleteKey(HKEY_CLASSES_ROOT, (L"CLSID\\" + CLSID_SHELL_BHO_STR).c_str());
	//v1.3.15 �޶�(��� 4.2)����ɾ�ļ�����չ���������߰� v1.3.14 ע�������
	//����ע�������ٴ���������ע�᲻ɾ�Ļ���������Զ�岻����
	//�����������̾��������ر� DLL����Ȼ�����ſػ�ܾ�פ���������Ǹ��壩��
	RegDeleteKeyW(HKEY_LOCAL_MACHINE, (LR"(SOFTWARE\Classes\Drive\shellex\FolderExtensions\)"
		+ CLSID_SHELL_BHO_STR).c_str());
	return S_OK;
}

#pragma endregion