/*
* Hook definitions
*
* Author: Maple
* date: 2021-7-13 Create
* Copyright winmoes.com
*/

#pragma once
#include "framework.h"

/* 地址必须用 &fun 取。
* 不要写成 (void*&)fun —— C++ 里这会取到函数入口的"机器码字节"而不是函数地址，
* 喂给 MinHook 会得到 MH_ERROR_NOT_EXECUTABLE（实测返回值 7）。
*/
/* v1.3.16 (B1)：钩子失败不再"弹框后继续跑"。
* 老写法失败只弹一个框，然后照样创建下一个，最后无条件 MH_EnableHook(MH_ALL_HOOKS)。
* 结果是只要有一个装失败，程序就带着"半套钩子"运行：EndPaint 失败会让绘制会话
* 守卫永远不清零，BeginPaint 失败则功能静默失效，而用户只看到几个英文弹框。
* 现在失败只记录第一个出错的序号（0 = 全部成功），由 OnWindowLoad 统一决定
* "六个全成功才启用"，否则回滚全部钩子。
*
* v1.3.17：额外把每个钩子的 MH_STATUS 存进 g_hookStatus[index]，供初始化日志记录，
* 便于查到"究竟是哪一个、因为什么原因装不上"。
*/
extern int g_hookFailIndex;
extern int g_hookStatus[7];

#define CreateMHook(fun, mfun, ofun, index)                                     \
do {                                                                            \
    int _mh_st = (int)MH_CreateHook((LPVOID)&fun, (LPVOID)&mfun, (LPVOID*)&ofun); \
    g_hookStatus[index] = _mh_st;                                               \
    if (_mh_st != 0 && g_hookFailIndex == 0)   /* 0 == MH_OK */                 \
        g_hookFailIndex = (index);                                              \
} while (0)

// Original function
#pragma region Original Function

typedef HWND(WINAPI* O_CreateWindowExW)(DWORD, LPCWSTR, LPCWSTR, DWORD,
    int, int, int, int, HWND, HMENU, HINSTANCE, LPVOID);
O_CreateWindowExW _CreateWindowExW_;

typedef BOOL(WINAPI* O_DestroyWindow)(HWND);
O_DestroyWindow _DestroyWindow_;

typedef HDC(WINAPI* O_BeginPaint)(HWND, LPPAINTSTRUCT);
O_BeginPaint _BeginPaint_;

typedef BOOL(WINAPI* O_EndPaint)(HWND, const LPPAINTSTRUCT);
O_EndPaint _EndPaint_;

typedef int(WINAPI* O_FillRect)(HDC, const RECT*, HBRUSH);
O_FillRect _FillRect_;

typedef HDC(WINAPI* O_CreateCompatibleDC)(HDC);
O_CreateCompatibleDC _CreateCompatibleDC_;

#pragma endregion

extern HWND MyCreateWindowExW(DWORD, LPCWSTR, LPCWSTR, DWORD,
    int, int, int, int, HWND, HMENU, HINSTANCE, LPVOID);

extern BOOL MyDestroyWindow(HWND);

extern HDC MyBeginPaint(HWND, LPPAINTSTRUCT);

extern BOOL MyEndPaint(HWND, const LPPAINTSTRUCT);

extern int MyFillRect(HDC, const RECT*, HBRUSH);

extern HDC MyCreateCompatibleDC(HDC);
