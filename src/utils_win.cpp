/*
 *  kdAmn is a deviantART Messaging Network client
 *  Copyright (C) 2013-2014  Cedric OCHS
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "common.h"
#include "utils.h"

#ifdef Q_OS_WIN

#ifdef USE_QT5
enum HBitmapFormat
{
    HBitmapNoAlpha,
    HBitmapPremultipliedAlpha,
    HBitmapAlpha
};

QPixmap qt_pixmapFromWinHBITMAP(HBITMAP bitmap, int hbitmapFormat = 0);
#endif

#include <windows.h>
#include <tlhelp32.h>
#include <tchar.h>
#include <stdio.h>
#include <ShellAPI.h>
#include <sdkddkver.h>

#ifdef _WIN32_WINNT_WIN7
	// only supported by Windows 7 Platform SDK
	#include <BaseTyps.h>
	#include <ShObjIdl.h>
	#define TASKBAR_PROGRESS 1
#endif

static QPixmap fancyPants( ICONINFO const &icon_info )
{
	int result;

	HBITMAP h_bitmap = icon_info.hbmColor;

	/// adapted from qpixmap_win.cpp so we can have a _non_ premultiplied alpha
	/// conversion and also apply the icon mask to bitmaps with no alpha channel
	/// remaining comments are Trolltech originals


	////// get dimensions
	BITMAP bitmap;
	memset( &bitmap, 0, sizeof(BITMAP) );

	result = GetObjectW( h_bitmap, sizeof(BITMAP), &bitmap );

	if (!result) return QPixmap();

	int const w = bitmap.bmWidth;
	int const h = bitmap.bmHeight;

	//////
	BITMAPINFO info;
	memset( &info, 0, sizeof(info) );

	info.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
	info.bmiHeader.biWidth       = w;

	info.bmiHeader.biHeight      = -h;
	info.bmiHeader.biPlanes      = 1;

	info.bmiHeader.biBitCount    = 32;
	info.bmiHeader.biCompression = BI_RGB;

	info.bmiHeader.biSizeImage   = w * h * 4;

	// Get bitmap bits
	uchar *data = new uchar[info.bmiHeader.biSizeImage];

	result = GetDIBits( qt_win_display_dc(), h_bitmap, 0, h, data, &info, DIB_RGB_COLORS );

	QPixmap p;
	if (result)
	{
		// test for a completely invisible image
		// we need to do this because there is apparently no way to determine

		// if an icon's bitmaps have alpha channels or not. If they don't the
		// alpha bits are set to 0 by default and the icon becomes invisible
		// so we do this long check. I've investigated everything, the bitmaps
		// don't seem to carry the BITMAPV4HEADER as they should, that would tell
		// us what we want to know if it was there, but apparently MS are SHIT SHIT
		const int N = info.bmiHeader.biSizeImage;

		int x;
		for (x = 3; x < N; x += 4)

			if (data[x] != 0)
				break;

		if (x < N)
		{
			p = QPixmap::fromImage( QImage( data, w, h, QImage::Format_ARGB32 ) );
		}
		else
		{
			QImage image( data, w, h, QImage::Format_RGB32 );

			QImage mask = image.createHeuristicMask();
			mask.invertPixels(); //prolly efficient as is a 1bpp bitmap really

			image.setAlphaChannel( mask );
			p = QPixmap::fromImage( image );
		}

		// force the pixmap to make a deep copy of the image data
		// otherwise `delete data` will corrupt the pixmap
		QPixmap copy = p;
		copy.detach();

		p = copy;
	}

	delete [] data;

	return p;
}

static QPixmap pixmap( const HICON &icon, bool alpha = true )
{
	try
	{
		ICONINFO info;
		::GetIconInfo(icon, &info);

		QPixmap pixmap = alpha ? fancyPants( info )
#ifdef USE_QT5
				: qt_pixmapFromWinHBITMAP(info.hbmColor, HBitmapNoAlpha);
#else
				: QPixmap::fromWinHBITMAP( info.hbmColor, QPixmap::NoAlpha );
#endif

		// gah Win32 is annoying!
		::DeleteObject( info.hbmColor );
		::DeleteObject( info.hbmMask );

		::DestroyIcon( icon );

		return pixmap;
	}
	catch (...)
	{
		return QPixmap();
	}
}

QPixmap associatedIcon( const QString &path )
{
	// performance tuned using:
	//http://www.codeguru.com/Cpp/COM-Tech/shell/article.php/c4511/

	SHFILEINFOW file_info;
	::SHGetFileInfoW((wchar_t*)path.utf16(), FILE_ATTRIBUTE_NORMAL, &file_info, sizeof(SHFILEINFOW), SHGFI_USEFILEATTRIBUTES | SHGFI_ICON | SHGFI_LARGEICON );

	return pixmap( file_info.hIcon );
}

struct TmpWindow
{
	HWND hWnd;
	QString name;
};

static BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM inst)
{
	if (IsWindowVisible(hWnd) && IsWindowEnabled(hWnd))
	{
		LONG style = GetWindowLong(hWnd, GWL_STYLE);

		if (style & (WS_THICKFRAME|WS_DLGFRAME))
		{
			wchar_t WindowName[80];

			int len = GetWindowTextW(hWnd, WindowName, 80);

			if (len > 0)
			{
				QVector<TmpWindow> *windows = (QVector<TmpWindow>*)inst;

				TmpWindow window;

				window.hWnd = hWnd;
				window.name = QString::fromWCharArray(WindowName);

				windows->push_back(window);
			}
		}
	}

	return TRUE;
}

//Windows 2000 = GetModuleFileName()
//Windows XP x32 = GetProcessImageFileName()
//Windows XP x64 = GetProcessImageFileName()

typedef BOOL (WINAPI *QueryFullProcessImageNamePtr)(HANDLE hProcess, DWORD dwFlags, LPTSTR lpExeName, PDWORD lpdwSize);
typedef DWORD (WINAPI *GetProcessImageFileNamePtr)(HANDLE hProcess, LPTSTR lpImageFileName, DWORD nSize);

static QueryFullProcessImageNamePtr pQueryFullProcessImageName = NULL;
static GetProcessImageFileNamePtr pGetProcessImageFileName = NULL;

void CreateWindowsList(QAbstractItemModel *model)
{
	if (pQueryFullProcessImageName == NULL)
	{
		pQueryFullProcessImageName = (QueryFullProcessImageNamePtr) QLibrary::resolve("kernel32", "QueryFullProcessImageNameA");
	}

	if (pGetProcessImageFileName == NULL)
	{
		pGetProcessImageFileName = (GetProcessImageFileNamePtr) QLibrary::resolve("psapi", "GetProcessImageFileNameA");
	}

	HMODULE module = GetModuleHandle(NULL);
	QFileIconProvider icon;

	QVector<TmpWindow> currentWindows;

	// list hWnd
	HANDLE hThreadSnap = INVALID_HANDLE_VALUE;
	THREADENTRY32 te32;

	// Fill in the size of the structure before using it.
	te32.dwSize = sizeof(THREADENTRY32);

	// Take a snapshot of all running threads
	hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);

	// Retrieve information about the first thread,
	if ((hThreadSnap != INVALID_HANDLE_VALUE) && Thread32First(hThreadSnap, &te32))
	{
		// Now walk the thread list of the system,
		// and display information about each thread
		// associated with the specified process
		do
		{
			currentWindows.clear();

			EnumThreadWindows(te32.th32ThreadID, EnumWindowsProc, (LPARAM)&currentWindows);

			if (!currentWindows.empty() && te32.th32OwnerProcessID)
			{
				for(int i = 0; i < currentWindows.size(); ++i)
				{
					HWND hWnd = currentWindows[i].hWnd;

					// get process handle
					DWORD pidwin;
					GetWindowThreadProcessId(hWnd, &pidwin);
					HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pidwin);

					// get process path
					char szProcessPath[MAX_PATH];
					DWORD bufSize = MAX_PATH;

					QString processPath;

					if (pQueryFullProcessImageName != NULL)
					{
						if (pQueryFullProcessImageName(hProcess, 0, (LPTSTR)&szProcessPath, &bufSize) == 0)
						{
							DWORD error = GetLastError();

							qDebug() << "Error" << error;
						}
					}
					else if (pGetProcessImageFileName != NULL)
					{
						bufSize = pGetProcessImageFileName(hProcess, (LPTSTR)&szProcessPath, bufSize);
					}

					processPath = QString::fromLatin1(szProcessPath, bufSize);

					HICON hIcon = NULL;

					UINT count = ExtractIconEx(processPath.toLatin1().data(), -1, NULL, NULL, 1);

					if (count < 1) continue;

					UINT res = ExtractIconEx(processPath.toLatin1().data(), 0, &hIcon, NULL, 1);

					QPixmap pixmap = ::pixmap(hIcon);

					DestroyIcon(hIcon);

					if (pixmap.isNull()) pixmap = icon.icon(QFileIconProvider::File).pixmap(32, 32);

					if (model->insertRow(0))
					{
						QModelIndex index = model->index(0, 0);

						model->setData(index, currentWindows[i].name);
						model->setData(index, pixmap, Qt::DecorationRole);
						model->setData(index, qVariantFromValue((void*)currentWindows[i].hWnd), Qt::UserRole);
					}
				}
			}
		}
		while(Thread32Next(hThreadSnap, &te32));

		CloseHandle(hThreadSnap);

		model->sort(0);
	}

	if (model->insertRow(0))
	{
		QModelIndex index = model->index(0, 0);

		model->setData(index, QObject::tr("Whole screen"));
		model->setData(index, icon.icon(QFileIconProvider::Desktop).pixmap(32, 32), Qt::DecorationRole);
		model->setData(index, qVariantFromValue((void*)NULL), Qt::UserRole);
	}
}

bool RestoreMinimizedWindow(WId &id)
{
	if (id)
	{
		WINDOWPLACEMENT placement;

		if (GetWindowPlacement((HWND)id, &placement))
		{
			if (placement.showCmd == SW_SHOWMINIMIZED)
			{
				ShowWindow((HWND)id, SW_RESTORE);
				// time needed to restore window
				Sleep(500);

				return true;
			}
		}
	}
	else
	{
		id = QApplication::desktop()->winId();
		// time needed to hide capture dialog
		Sleep(500);
	}

	return false;
}

void MinimizeWindow(WId id)
{
	ShowWindow((HWND)id, SW_MINIMIZE);
}

bool IsUsingComposition()
{
	typedef BOOL (*voidfuncPtr)(void);

	HINSTANCE hInst = LoadLibraryA("UxTheme.dll");

	bool ret = false;

	if (hInst)
	{
		voidfuncPtr fctIsCompositionActive = (voidfuncPtr)GetProcAddress(hInst, "IsCompositionActive");

		if (fctIsCompositionActive)
		{
			// only if compositing is not activated
			if (fctIsCompositionActive())
				ret = true;
		}

		FreeLibrary(hInst);
	}

	return ret;
}

void PutForegroundWindow(WId id)
{
	SetForegroundWindow((HWND)id);
	Sleep(500);
}

typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);

bool IsOS64bits()
{
	bool res;

#ifdef _WIN644
	res = true;
#else
	res = false;

	// IsWow64Process is not available on all supported versions of Windows.
	// Use GetModuleHandle to get a handle to the DLL that contains the function
	// and GetProcAddress to get a pointer to the function if available.
	LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS) GetProcAddress(GetModuleHandleA("kernel32"), "IsWow64Process");

	if (fnIsWow64Process)
	{
		BOOL bIsWow64 = FALSE;

		if (fnIsWow64Process(GetCurrentProcess(), &bIsWow64))
		{
			res = bIsWow64 == TRUE;
		}
	}
#endif
	return res;
}

static ITaskbarList3* s_taskbarList = NULL;

bool InitSystemProgress()
{
	bool res = false;

#ifdef TASKBAR_PROGRESS
	s_taskbarList = NULL;
	// instanciate the taskbar control COM object
	res = SUCCEEDED(CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&s_taskbarList)));
#endif

	return res;
}

bool UninitSystemProgress()
{
#ifdef TASKBAR_PROGRESS
    if (s_taskbarList)
    {
        s_taskbarList->Release();
        s_taskbarList = NULL;
    }
#endif

	return true;
}

bool BeginSystemProgress()
{
	bool res = false;

#ifdef TASKBAR_PROGRESS
	// update the taskbar progress
	if (s_taskbarList && GetMainWindowId()) res = SUCCEEDED(s_taskbarList->SetProgressState((HWND)GetMainWindowId(), TBPF_NORMAL));
#endif // TASKBAR_PROGRESS

	return res;
}

bool UpdateSystemProgress(qint64 value, qint64 total)
{
	bool res = false;

#ifdef TASKBAR_PROGRESS
	// update the taskbar progress
	if (s_taskbarList && GetMainWindowId()) res = SUCCEEDED(s_taskbarList->SetProgressValue((HWND)GetMainWindowId(), value, total));
#endif // TASKBAR_PROGRESS

	return res;
}

bool EndSystemProgress()
{
	bool res = false;

#ifdef TASKBAR_PROGRESS
	// update the taskbar progress
	if (s_taskbarList && GetMainWindowId()) res = SUCCEEDED(s_taskbarList->SetProgressState((HWND)GetMainWindowId(), TBPF_NOPROGRESS));
#endif // TASKBAR_PROGRESS

	return res;
}

#endif
