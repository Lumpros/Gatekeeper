#pragma once

#include "Window.h"
#include "Tab.h"

#include <string>
#include <vector>

#include <d2d1.h>

class TabManager : public Window
{
public:
	TabManager(HWND hParentWnd, Size size, Point point);
	~TabManager(void);

	LRESULT WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	size_t GetTabShapeHeight(void) const noexcept;
	void AppendTab(Tab* pTab);
	Tab* GetCurrentTab(void);
	Tab* GetTab(int index);

private:
	void RegisterTabManagerClass(HINSTANCE hInstance);
	void InitializeTabManagerWindow(HINSTANCE hInstance);
	void InitializeGraphics(void);
	void InitializeMetrics(void);

	void DrawBackground(void);
	void DrawTab(int index, COLORREF crFill, COLORREF crOutline);
	void DrawTabs(void);
	void DrawTabLines(void);

	void SwitchToTab(int iTabIndex);

	ID2D1PathGeometry* CreateTabPathGeometry(int index);
	
	int GetIndexOfTabUnderPoint(int x, int y);

	void InvalidateTabShape(int index);
	bool GetTabShapeBounds(int index, RECT* pRect);

	LRESULT OnPaint(HWND hWnd);
	LRESULT OnSize(WORD wNewWidth, WORD wNewHeight);
	LRESULT OnLeftMouseDown(int cursorX, int cursorY);
	LRESULT OnMouseMove(int cursorX, int cursorY);
	LRESULT OnMouseLeave(void);
	LRESULT OnCommand(int id);

private:
	std::vector<Tab*> m_Tabs;
	std::vector<ID2D1PathGeometry*> m_TabGeometries;

	// Used for Direct2D drawing because GDI is extremely slow and I don't want the window to blink
	ID2D1HwndRenderTarget* m_pRenderTarget = nullptr;

	// Honestly used because I don't feel like rewriting every drawing section in Direct2D, especially
	// the text parts, so I'd rather use this and make it backwards compatible, it's much faster as well
	ID2D1GdiInteropRenderTarget* m_pGDIRT = nullptr;

	// The brush used for all the rectangles in OnPaint. Its color and opacity are changed throughout the program.
	ID2D1SolidColorBrush* m_pSolidColorBrush = nullptr;

	size_t cyTabBar = 0;
	size_t cxTab = 0;

	HFONT m_hFont = NULL;
	HBITMAP m_hLogoBitmap = NULL;

	int m_iHoveredTabIndex = 0;
	int m_iSelectedTabIndex = 0;

	bool m_isTrackingCursor = false;
};

