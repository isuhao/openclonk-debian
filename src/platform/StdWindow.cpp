/*
 * OpenClonk, http://www.openclonk.org
 *
 * Copyright (c) 2005-2007  Sven Eberhardt
 * Copyright (c) 2005, 2007, 2009  Peter Wortmann
 * Copyright (c) 2005-2006, 2008-2009  Günther Brammer
 * Copyright (c) 2006  Armin Burgmeier
 * Copyright (c) 2009  Nicolas Hake
 * Copyright (c) 2001-2009, RedWolf Design GmbH, http://www.clonk.de
 *
 * Portions might be copyrighted by other authors who have contributed
 * to OpenClonk.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * See isc_license.txt for full license and disclaimer.
 *
 * "Clonk" is a registered trademark of Matthes Bender.
 * See clonk_trademark_license.txt for full license.
 */

/* A wrapper class to OS dependent event and window interfaces, WIN32 version */

#include "C4Include.h"
#include <StdRegistry.h>
#ifdef USE_GL
#include <StdGL.h>
#endif
#ifdef USE_DIRECTX
#include <StdD3D.h>
#endif
#include <StdWindow.h>
#include <mmsystem.h>
#include <stdio.h>
#include <io.h>
#include <ctype.h>
#include <conio.h>

// multimon.h comes with DirectX, some people don't have DirectX.
#ifdef HAVE_MULTIMON_H

// Lets try this unconditionally so that older windowses get the benefit
// even if the engine was compiled with a newer sdk. Or something.
#define COMPILE_MULTIMON_STUBS
#include <multimon.h>

#endif

#include "resource.h"
#include "C4Version.h"

#define C4FullScreenClassName L"C4FullScreen"
LRESULT APIENTRY FullScreenWinProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

CStdWindow::CStdWindow (): Active(false), pSurface(0), hWindow(0)
{
}
CStdWindow::~CStdWindow ()
{
}

BOOL CStdWindow::RegisterWindowClass(HINSTANCE hInst)
{
	WNDCLASSEXW WndClass = {0};
	WndClass.cbSize        = sizeof(WNDCLASSEX);
	WndClass.style         = CS_DBLCLKS;
	WndClass.lpfnWndProc   = FullScreenWinProc;
	WndClass.hInstance     = hInst;
	WndClass.hbrBackground = (HBRUSH) COLOR_BACKGROUND;
	WndClass.lpszClassName = C4FullScreenClassName;
	WndClass.hIcon         = LoadIcon (hInst, MAKEINTRESOURCE (IDI_00_C4X) );
	WndClass.hIconSm       = LoadIcon (hInst, MAKEINTRESOURCE (IDI_00_C4X) );
	return RegisterClassExW(&WndClass);
}

#define ADDL2(s) L##s
#define ADDL(s) ADDL2(s)

CStdWindow * CStdWindow::Init(CStdApp * pApp)
{
	Active = true;

	// Register window class
	if (!RegisterWindowClass(pApp->hInstance)) return NULL;

	// Create window
	hWindow = CreateWindowExW  (
	            0,
	            C4FullScreenClassName,
	            ADDL(C4ENGINENAME),
	            WS_OVERLAPPEDWINDOW,
	            CW_USEDEFAULT,CW_USEDEFAULT,0,0,
	            NULL,NULL,pApp->hInstance,NULL);

#ifndef USE_CONSOLE
	// Show & focus
	ShowWindow(hWindow,SW_SHOWNORMAL);
	SetFocus(hWindow);
#endif

	return this;
}

void CStdWindow::Clear()
{
	// Destroy window
	if (hWindow) DestroyWindow(hWindow);
	hWindow = NULL;
}

bool CStdWindow::StorePosition(const char *szWindowName, const char *szSubKey, bool fStoreSize)
{
	return StoreWindowPosition(hWindow, szWindowName, szSubKey, fStoreSize) != 0;
}

bool CStdWindow::RestorePosition(const char *szWindowName, const char *szSubKey, bool fHidden)
{
	if (!RestoreWindowPosition(hWindow, szWindowName, szSubKey, fHidden))
		ShowWindow(hWindow,SW_SHOWNORMAL);
	return true;
}

void CStdWindow::SetTitle(const char *szToTitle)
{
	if (hWindow) SetWindowText(hWindow, szToTitle ? szToTitle : "");
}

bool CStdWindow::GetSize(RECT * pRect)
{
	if (!(hWindow && GetClientRect(hWindow,pRect))) return false;
	return true;
}

void CStdWindow::SetSize(unsigned int cx, unsigned int cy)
{
	if (hWindow)
	{
		// If bordered, add border size
		RECT rect = {0, 0, cx, cy};
		::AdjustWindowRectEx(&rect, GetWindowLong(hWindow, GWL_STYLE), FALSE, GetWindowLong(hWindow, GWL_EXSTYLE));
		cx = rect.right - rect.left;
		cy = rect.bottom - rect.top;
		::SetWindowPos(hWindow, NULL, 0, 0, cx, cy, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOREDRAW | SWP_NOZORDER);
	}
}

void CStdWindow::FlashWindow()
{
	// please activate me!
	if (hWindow)
		::FlashWindow(hWindow, FLASHW_ALL | FLASHW_TIMERNOFG);
}

/* CStdTimerProc */

int CStdMultimediaTimerProc::iTimePeriod = 0;

CStdMultimediaTimerProc::CStdMultimediaTimerProc(uint32_t iDelay) :
		uCriticalTimerDelay(28),
		idCriticalTimer(0),
		uCriticalTimerResolution(5),
		Event(true)
{

	if (!iTimePeriod)
	{
		// Get resolution caps
		TIMECAPS tc;
		timeGetDevCaps(&tc, sizeof(tc));
		// Establish minimum resolution
		uCriticalTimerResolution = BoundBy(uCriticalTimerResolution, tc.wPeriodMin, tc.wPeriodMax);
		timeBeginPeriod(uCriticalTimerResolution);
	}
	iTimePeriod++;

	SetDelay(iDelay);

}

CStdMultimediaTimerProc::~CStdMultimediaTimerProc()
{
	if (idCriticalTimer)
	{
		timeKillEvent(idCriticalTimer);
		idCriticalTimer = 0;

		iTimePeriod--;
		if (!iTimePeriod)
			timeEndPeriod(uCriticalTimerResolution);
	}
}

void CStdMultimediaTimerProc::SetDelay(uint32_t iDelay)
{

	// Kill old timer (of any)
	if (idCriticalTimer)
		timeKillEvent(idCriticalTimer);

	// Set critical timer
	idCriticalTimer=timeSetEvent(
	                  uCriticalTimerDelay,uCriticalTimerResolution,
	                  (LPTIMECALLBACK) Event.GetEvent(),0,TIME_PERIODIC | TIME_CALLBACK_EVENT_SET);

}

bool CStdMultimediaTimerProc::CheckAndReset()
{
	if (!Check()) return false;
	Event.Reset();
	return true;
}

/* CStdMessageProc */

bool CStdMessageProc::Execute(int iTimeout, pollfd *)
{
	// Peek messages
	MSG msg;
	while (PeekMessage(&msg,NULL,0,0,PM_REMOVE))
	{
		// quit?
		if (msg.message == WM_QUIT)
		{
			pApp->fQuitMsgReceived = true;
			return false;
		}
		// Dialog message transfer
		if (!pApp->DialogMessageHandling(&msg))
		{
			TranslateMessage(&msg); DispatchMessage(&msg);
		}
	}
	return true;
}

/* CStdApp */

CStdApp::CStdApp() :
		Active(false), fQuitMsgReceived(false), hInstance(NULL),
		fDspModeSet(false)
{
	ZeroMemory(&pfd, sizeof(pfd)); pfd.nSize = sizeof(pfd);
	ZeroMemory(&dspMode, sizeof(dspMode)); dspMode.dmSize =  sizeof(dspMode);
	ZeroMemory(&OldDspMode, sizeof(OldDspMode)); OldDspMode.dmSize =  sizeof(OldDspMode);
	hMainThread = NULL;
#ifdef _WIN32
	MessageProc.SetApp(this);
	Add(&MessageProc);
#endif
}

CStdApp::~CStdApp()
{
}

const char *LoadResStr(const char *id);

bool CStdApp::Init(int argc, char * argv[])
{
	// Set instance vars
	hMainThread = ::GetCurrentThread();
	// Custom initialization
	return DoInit (argc, argv);
}

void CStdApp::Clear()
{
	hMainThread = NULL;
}

void CStdApp::Quit()
{
	PostQuitMessage(0);
}

bool CStdApp::FlushMessages()
{

	// Always fail after quit message
	if (fQuitMsgReceived)
		return false;

	return MessageProc.Execute(0);
}

int GLMonitorInfoEnumCount;

BOOL CALLBACK GLMonitorInfoEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
	// get to indexed monitor
	if (GLMonitorInfoEnumCount--) return true;
	// store it
	CStdApp *pApp = (CStdApp *) dwData;
	pApp->hMon = hMonitor;
	pApp->MonitorRect = *lprcMonitor;
	return true;
}

bool CStdApp::GetIndexedDisplayMode(int32_t iIndex, int32_t *piXRes, int32_t *piYRes, int32_t *piBitDepth, int32_t *piRefreshRate, uint32_t iMonitor)
{
	// prepare search struct
	DEVMODE dmode;
	ZeroMemory(&dmode, sizeof(dmode)); dmode.dmSize = sizeof(dmode);
	StdStrBuf Mon;
	if (iMonitor)
		Mon.Format("\\\\.\\Display%d", iMonitor+1);
	// check if indexed mode exists
	if (!EnumDisplaySettings(Mon.getData(), iIndex, &dmode)) return false;
	// mode exists; return it
	if (piXRes) *piXRes = dmode.dmPelsWidth;
	if (piYRes) *piYRes = dmode.dmPelsHeight;
	if (piBitDepth) *piBitDepth = dmode.dmBitsPerPel;
	if (piRefreshRate) *piRefreshRate = dmode.dmDisplayFrequency;
	return true;
}

void CStdApp::RestoreVideoMode()
{
}

bool CStdApp::SetVideoMode(unsigned int iXRes, unsigned int iYRes, unsigned int iColorDepth, unsigned int iRefreshRate, unsigned int iMonitor, bool fFullScreen)
{
#ifdef USE_DIRECTX
	if (pD3D)
	{
		if (!pD3D->SetVideoMode(iXRes, iYRes, iColorDepth, iMonitor, fFullScreen))
			return false;
		OnResolutionChanged(iXRes, iYRes);
		return true;
	}
#endif
#ifdef USE_GL
	SetWindowLong(pWindow->hWindow, GWL_EXSTYLE,
	              GetWindowLong(pWindow->hWindow, GWL_EXSTYLE) | WS_EX_APPWINDOW);
	bool fFound=false;
	DEVMODE dmode;
	// if a monitor is given, search on that instead
	// get monitor infos
	GLMonitorInfoEnumCount = iMonitor;
	hMon = NULL;
	EnumDisplayMonitors(NULL, NULL, GLMonitorInfoEnumProc, (LPARAM) this);
	// no monitor assigned?
	if (!hMon)
	{
		// Okay for primary; then just use a default
		if (!iMonitor)
		{
			MonitorRect.left = MonitorRect.top = 0;
			MonitorRect.right = iXRes; MonitorRect.bottom = iYRes;
		}
		else return false;
	}
	StdStrBuf Mon;
	if (iMonitor)
		Mon.Format("\\\\.\\Display%d", iMonitor+1);

	ZeroMemory(&dmode, sizeof(dmode)); dmode.dmSize = sizeof(dmode);
	
	// Get current display settings
	if (!EnumDisplaySettings(Mon.getData(), ENUM_CURRENT_SETTINGS, &dmode))
		return false;
	if (!iRefreshRate)
	{
		// Default to current
		iRefreshRate = dmode.dmDisplayFrequency;
	}
	int orientation = dmode.dmDisplayOrientation;
	// enumerate modes
	int i=0;
	while (EnumDisplaySettings(Mon.getData(), i++, &dmode))
		// compare enumerated mode with requested settings
		if (dmode.dmPelsWidth==iXRes && dmode.dmPelsHeight==iYRes && dmode.dmBitsPerPel==iColorDepth && dmode.dmDisplayOrientation==orientation && dmode.dmDisplayFrequency==iRefreshRate)
		{
			fFound=true;
			dspMode=dmode;
			break;
		}
	if (!fFound) return false;
	// change mode
	if (!fFullScreen)
	{
		ChangeDisplaySettings(NULL, CDS_RESET);
		SetWindowLong(pWindow->hWindow, GWL_STYLE,
		              GetWindowLong(pWindow->hWindow, GWL_STYLE) | (WS_CAPTION|WS_THICKFRAME|WS_BORDER));
	}
	else
	{
			dspMode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;
		if (ChangeDisplaySettingsEx(iMonitor ? Mon.getData() : NULL, &dspMode, NULL, CDS_FULLSCREEN, NULL) != DISP_CHANGE_SUCCESSFUL)
			{
				return false;
			}

		SetWindowLong(pWindow->hWindow, GWL_STYLE,
		              GetWindowLong(pWindow->hWindow, GWL_STYLE) & ~ (WS_CAPTION|WS_THICKFRAME|WS_BORDER));
	}

	::SetWindowPos(pWindow->hWindow, NULL, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOREDRAW|SWP_FRAMECHANGED);
	pWindow->SetSize(dspMode.dmPelsWidth, dspMode.dmPelsHeight);
	OnResolutionChanged(iXRes, iYRes);
	return true;
#endif
}

void CStdApp::MessageDialog(const char * message)
{
	MessageBox(0, message, C4ENGINECAPTION, MB_ICONERROR);
}

// Clipboard functions
bool CStdApp::Copy(const StdStrBuf & text, bool fClipboard)
{
	if (!fClipboard) return false;
	bool fSuccess = true;
	// gain clipboard ownership
	if (!OpenClipboard(GetWindowHandle())) return false;
	// must empty the global clipboard, so the application clipboard equals the Windows clipboard
	EmptyClipboard();
	int size = MultiByteToWideChar(CP_UTF8, 0, text.getData(), text.getSize(), 0, 0);
	HANDLE hglbCopy = GlobalAlloc(GMEM_MOVEABLE, size * sizeof(wchar_t));
	if (hglbCopy == NULL) { CloseClipboard(); return false; }
	// lock the handle and copy the text to the buffer.
	wchar_t *szCopyChar = (wchar_t *) GlobalLock(hglbCopy);
	fSuccess = !!MultiByteToWideChar(CP_UTF8, 0, text.getData(), text.getSize(), szCopyChar, size);
	GlobalUnlock(hglbCopy);
	// place the handle on the clipboard.
	fSuccess = fSuccess && !!SetClipboardData(CF_UNICODETEXT, hglbCopy);
	// close clipboard
	CloseClipboard();
	// return whether copying was successful
	return fSuccess;
}

StdStrBuf CStdApp::Paste(bool fClipboard)
{
	if (!fClipboard) return StdStrBuf();
	// open clipboard
	if (!OpenClipboard(NULL)) return StdStrBuf();
	// get text from clipboard
	HANDLE hglb = GetClipboardData(CF_UNICODETEXT);
	if (!hglb) return StdStrBuf();
	StdStrBuf text((LPCWSTR) GlobalLock(hglb));
	// unlock mem
	GlobalUnlock(hglb);
	// close clipboard
	CloseClipboard();
	return text;
}

bool CStdApp::IsClipboardFull(bool fClipboard)
{
	if (!fClipboard) return false;
	return !!IsClipboardFormatAvailable(CF_UNICODETEXT);
}

void CStdApp::ClearClipboard(bool fClipboard)
{
}
