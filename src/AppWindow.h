#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>

#include "ListView.h"
#include "TabManager.h"

class AppWindow
{
public:
	~AppWindow(void);

	LRESULT WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	void Initialize(void);

	void StartMessageLoop(void);
	void Show(int nCmdShow);

private:
	void InitWindowClass(HINSTANCE hInstance);
	void InitWindow(HINSTANCE hInstance);

	LRESULT OnDPIChanged(HWND hWnd, LPARAM lParam);
	LRESULT OnSize(WORD wNewWidth, WORD wNewHeight);
	LRESULT OnGetMinMaxInfo(LPMINMAXINFO pMMI);

private:
	HWND m_hWnd = nullptr;
	HACCEL hAccelerator;

	ListView* m_pViewList = nullptr;
	TabManager* m_pTabManager = nullptr;
};

