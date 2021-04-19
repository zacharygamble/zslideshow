// zslideshow.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <Windowsx.h>
#include <ShObjIdl_core.h>
#include <gl/GL.h>
#include <iostream>
#include <filesystem>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <cassert>
#include <chrono>

#include <stb_image.h>

#include "filedialog.h"
#include "slideshow.h"
#include "resource.h"

using namespace std;
using namespace std::filesystem;
namespace fs = std::filesystem;

// Constants
const wchar_t* CLASS_NAME = L"MAIN_CLASS";

// Main window
HWND mainWindow;
SlideShow slide_show;

int width = -1;
int height = -1;

fs::path current_dir;

LRESULT CALLBACK win_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
bool register_class();
bool create_window();
bool create_gl_context();
void gl_init();
void toggle_fullscreen();
void shutdown();

void set_directory(const path& dir);
void refresh_directory_index(const path& dir);
void load_images();
void render();

int main()
{
	if (FAILED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE))) {
		cout << "Common Control Init failed" << endl;
	}

	if (!register_class())
		return 1;
	if (!create_window())
		return 1;
	if (!create_gl_context())
		return 1;

	gl_init();
	slide_show.init();

	path data_dir = GetFolderOpen();
	if (data_dir.empty())
		return 0;
	refresh_directory_index(data_dir);
	load_images();

	toggle_fullscreen();

	MSG msg;
	while (GetMessage(&msg, NULL, NULL, NULL)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	shutdown();
	return 0;
}

bool register_class()
{
	HINSTANCE hinst = GetModuleHandle(NULL);

	WNDCLASSEX wnd = {0};
	wnd.cbSize = sizeof(WNDCLASSEX);
	wnd.style = CS_DBLCLKS | CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	wnd.lpfnWndProc = win_proc;
	wnd.hInstance = hinst;
	wnd.hCursor = LoadCursor(NULL,IDC_ARROW);
	//wnd.hbrBackground = (HBRUSH)GetStockObject(COLOR_BACKGROUND);

	wnd.lpszClassName = CLASS_NAME;
	
	return RegisterClassEx(&wnd);
}

bool create_window()
{
	mainWindow = CreateWindowEx(0, CLASS_NAME, L"Main", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720, HWND_DESKTOP, NULL, NULL, NULL);
	if (mainWindow == INVALID_HANDLE_VALUE || mainWindow == 0)
		return false;

	ShowWindow(mainWindow, SW_SHOWDEFAULT);

	return true;
}

void toggle_fullscreen()
{
	static bool fullscreen = false;
	static RECT minRect;
	fullscreen = !fullscreen;

	if (fullscreen) {
		GetWindowRect(mainWindow, &minRect);
		// Get fullscreen monitor info
		HMONITOR mon = MonitorFromWindow(mainWindow, MONITOR_DEFAULTTOPRIMARY);
		MONITORINFO mnfo; mnfo.cbSize = sizeof(mnfo);
		GetMonitorInfo(mon, &mnfo);

		SetWindowLongPtr(mainWindow, GWL_STYLE, WS_SYSMENU | WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE);
		int x = mnfo.rcMonitor.left;
		int y = mnfo.rcMonitor.top;
		int width = mnfo.rcMonitor.right - x;
		int height = mnfo.rcMonitor.bottom - y;
		MoveWindow(mainWindow, x, y, width, height, TRUE);
	}
	else {
		SetWindowLongPtr(mainWindow, GWL_STYLE, WS_VISIBLE | WS_OVERLAPPEDWINDOW);
		MoveWindow(mainWindow, minRect.left, minRect.top, minRect.right - minRect.left, minRect.bottom - minRect.top, TRUE);
	}
}

bool create_gl_context()
{
	HDC hdc = GetDC(mainWindow);

	PIXELFORMATDESCRIPTOR pfd = {
		sizeof(PIXELFORMATDESCRIPTOR),
		1, // version number
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, 
		PFD_TYPE_RGBA,
		24, // color depth
		0,0,0,0,0,0, // color bits ignored
		0, // no alpha buffer
		0, // shift bit ignored
		0, // no accumulation buffer
		0,0,0,0, // accum bits ignored
		32, // zbuffer depth
		0, // stencil buffer
		0, // aux buffer
		PFD_MAIN_PLANE, // main layer
		0, // reserved
		0, 0, 0 // layer masks ignored
	};
	int pixelFormat = ChoosePixelFormat(hdc, &pfd);
	if (!SetPixelFormat(hdc, pixelFormat, &pfd)) {
		cout << "Pixel format selection failed." << endl;
		return false;
	}

	HGLRC hrc = wglCreateContext(hdc);
	if (!wglMakeCurrent(hdc, hrc)) {
		cout << "Context creation failed." << endl;
		return false;
	}

	// cool, now we should have our amazing gl1.0 context (2.0 maybe?)

	return true;
}

void gl_init()
{
	RECT size;
	GetClientRect(mainWindow, &size);
	glViewport(0, 0, size.right, size.bottom);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	glEnable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	//glShadeModel(GL_FLAT);
}

void shutdown()
{
	HGLRC hrc = wglGetCurrentContext();
	wglDeleteContext(hrc);
	DestroyWindow(mainWindow);
	UnregisterClass(CLASS_NAME, GetModuleHandle(NULL));
}

const unsigned ADVANCE_SLIDE = 1;
void WINAPI timer_handler(HWND hwnd, UINT msg, UINT_PTR id, DWORD tickCount)
{
	switch (id) {
	case ADVANCE_SLIDE:
		if (slide_show.slide_count()) {
			slide_show.next_slide();
			RedrawWindow(hwnd, NULL, NULL, RDW_INTERNALPAINT);
		}
		break;
	}
}

void SelectDir()
{

}

LRESULT CALLBACK win_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	static int millis = 1000;
	static HMENU menu;

	switch (msg) {
	case WM_CREATE:
	{
		menu = LoadMenu(NULL, MAKEINTRESOURCE(IDR_MENU1));
		menu = GetSubMenu(menu, 0);
	}
		break;
	case WM_SHOWWINDOW:
		//FIXME maybe replace mainWindow with hwnd?
		//FIXME SetTimer here?
		SetTimer(mainWindow, ADVANCE_SLIDE, millis, timer_handler);
		break;
	case WM_PAINT:
		//BeginPaint()
		render();
		SwapBuffers(GetDC(mainWindow));
		return DefWindowProc(hwnd, msg, wparam, lparam);
	case WM_SIZE:
		width = GET_X_LPARAM(lparam);
		height = GET_Y_LPARAM(lparam);
		gl_init();
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_CLOSE:
		//if (MessageBox(hwnd, L"Close the window?", L"Confirmation", MB_YESNO | MB_APPLMODAL) == IDYES)
		DestroyWindow(hwnd);
		return 0;
	case WM_COMMAND:
		switch (LOWORD(wparam)) {
		case ID_EDIT_SELECT:
			cout << GetFolderOpen() << endl;
			break;
		}
		return 0;
	case WM_KEYUP:
		switch (wparam) {
		case VK_ESCAPE:
			PostMessage(hwnd, WM_CLOSE, 0, 0);
			break;
		case VK_F11:
			toggle_fullscreen();
			break;
		case VK_LEFT:
			slide_show.prev_slide();
			RedrawWindow(hwnd, NULL, NULL, RDW_INTERNALPAINT);
			break;
		case VK_RIGHT:
			slide_show.next_slide();
			RedrawWindow(hwnd, NULL, NULL, RDW_INTERNALPAINT);
			break;
		}
		return 0;
	case WM_CONTEXTMENU:
		{
		int x = GET_X_LPARAM(lparam);
		int y = GET_Y_LPARAM(lparam);
		TrackPopupMenuEx(menu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON, x, y, hwnd, NULL);
		}
		return 0;
	case WM_MOUSEWHEEL:
	{
		int d = GET_WHEEL_DELTA_WPARAM(wparam);
		if (d > 0) {
			if (millis / 2 > 0)
				millis /= 2;
			SetTimer(mainWindow, ADVANCE_SLIDE, millis, timer_handler);
		}
		else if (d < 0) {
			if (millis * 2 > 0) // avoid overflow
				millis *= 2;
			SetTimer(mainWindow, ADVANCE_SLIDE, millis, timer_handler);
		}
		cout << "Wheel: " << d << "-- Millis: " << millis << endl;
	}
		break; // FIXME

	default:
		return DefWindowProc(hwnd, msg, wparam, lparam);
	}
	return 0;
}

void set_directory(const path& dir)
{
	if (dir.empty()) {
		assert(false && "Directory empty");
		return;
	}

	current_dir = dir;
	refresh_directory_index(dir);
	// watch the directory

}

void refresh_directory_index(const path& dir)
{
	std::unordered_set<string> present;

	for (auto& entry : directory_iterator(dir)) {
		// check if it is a file
		if (!entry.is_regular_file())
			continue;

		slide_show.add_or_update(entry.path(), true);
		present.emplace(entry.path().string());
	}

	// prune deleted entries
	slide_show.prune(present);
}

void load_images()
{
	// Determine which files to load
	//...
	slide_show.apply_deferred();
}

void draw_blank()
{
	glBegin(GL_QUADS);

	glColor3f(1.0f, 0.0f, 0.0f);
	glVertex2f(-1.0f, -1.0f);
	glVertex2f(1.0f, -1.0f);
	glVertex2f(1.0f, 1.0f);
	glVertex2f(-1.0f, 1.0f);

	glEnd();
}

void draw_image(ImageRaw* raw)
{
	float imageRatio = (float)raw->width / raw->height;
	float screenRatio = (float)width / height;
	float x_scale = imageRatio / screenRatio;

	glBindTexture(GL_TEXTURE_2D, raw->tex_name);

	glBegin(GL_QUADS);

	glTexCoord2f(0.0f, 0.0f);
	glVertex2f(-1.0f * x_scale, 1.0f);
	glTexCoord2f(0.0f, 1.0f);
	glVertex2f(-1.0f * x_scale, -1.0f);
	glTexCoord2f(1.0f, 1.0f);
	glVertex2f(1.0f * x_scale, -1.0f);
	glTexCoord2f(1.0f, 0.0f);
	glVertex2f(1.0f * x_scale, 1.0f);

	glEnd();
}

void render()
{
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	if (slide_show.slide_count() == 0) {
		draw_blank();
	}
	else {
		draw_image(&slide_show.current_slide().raw);
	}
}
