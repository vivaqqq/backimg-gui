/*
* BHO���������
*
* Author: Maple
* date: 2022-1-31 Create
* Copyright winmoes.com
*/
#pragma once
#include "framework.h"
#include <Unknwn.h>
#include <exdisp.h>
#include <exdispid.h>
#include <mshtml.h>
#include <mshtmdid.h>
#include <string>

extern const std::wstring CLSID_SHELL_BHO_STR;
extern const CLSID CLSID_SHELL_BHO;

extern void OnWindowLoad();

/*
* һ���򵥵�COM���� ֻʵ�ֱ�Ҫ�Ľӿ�
*/
class CObjectWithSite : public IObjectWithSite
{
public:
	CObjectWithSite();
	virtual ~CObjectWithSite();

	//IUnknown
	STDMETHODIMP QueryInterface(REFIID riid, void** ppv);
	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();

	//IObjectWithSite
	STDMETHODIMP SetSite(IUnknown* pUnkSite);
	STDMETHODIMP GetSite(REFIID riid, void** ppvSite) { return E_NOINTERFACE; }

protected:
	long m_ref = 0;
	IWebBrowser2* m_web = nullptr;
	void ReleaseRes();
};

/*
* �๤�� ���ڴ��������COM����
*/
class ClassFactory : public IClassFactory
{
public:
	//IUnknown
	STDMETHODIMP QueryInterface(REFIID riid, void** ppv);
	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();

	/* v1.3.18(审计修复)：补虚析构 —— Release 归零时会 delete this，
	 * 多态类没有虚析构属未定义行为（编译器 -Wdelete-non-virtual-dtor 警告）。 */
	virtual ~ClassFactory() {}

	//IClassFactory
	IFACEMETHODIMP CreateInstance(LPUNKNOWN pUnkOuter, REFIID riid, LPVOID* ppvObj);
	IFACEMETHODIMP LockServer(BOOL fLock);

private:
	long m_ref = 0;
};