#include "TabManager.h"
#include "Utility.h"
#include "Renderer.h"
#include "Tab.h"
#include "MainTab.h"
#include "ExportTab.h"

#include "resource.h"

#include <cassert>
#include <stdexcept>
#include <windowsx.h>

#define SELECTED_TAB_COLOR   BACKGROUND_COLOR
#define UNSELECTED_TAB_COLOR RGB_D2D(230, 230, 237)
#define HOVERED_TAB_COLOR    RGB_D2D(217, 217, 224)

#define INVALID_TAB_INDEX (-1)


LRESULT CALLBACK TabManagerProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	TabManager* pTabManager;

	switch (uMsg)
	{
	case WM_CREATE:
		util::RegisterObject(hWnd, ((LPCREATESTRUCT)lParam)->lpCreateParams);
		return 0;

	default:
		if (pTabManager = (TabManager*)util::GetRegisteredObject(hWnd))
		{
			return pTabManager->WindowProcedure(hWnd, uMsg, wParam, lParam);
		}
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

TabManager::TabManager(HWND hParentWnd, Size size, Point point)
	: Window(hParentWnd, size, point)
{
	HINSTANCE hInstance = GetModuleHandle(NULL);

	InitializeMetrics();
	RegisterTabManagerClass(hInstance);
	InitializeTabManagerWindow(hInstance);
	InitializeGraphics();
}

TabManager::~TabManager(void)
{
	for (Tab* pTab : m_Tabs)
	{
		delete pTab;
	}

	m_Tabs.clear();

	SAFE_RELEASE_D2D(m_pSolidColorBrush);
	SAFE_RELEASE_D2D(m_pGDIRT);
	SAFE_RELEASE_D2D(m_pRenderTarget);
	
	for (ID2D1PathGeometry* pGeometry : m_TabGeometries)
	{
		SAFE_RELEASE_D2D(pGeometry);
	}

	m_TabGeometries.clear();

	SAFE_DELETE_GDIOBJ(m_hFont);
	SAFE_DELETE_GDIOBJ(m_hLogoBitmap);
}

void TabManager::InitializeMetrics(void)
{
	double lfScale = util::GetDPIScale(m_hWndParent);

	cyTabBar = static_cast<size_t>(lfScale * 40);
	cxTab = static_cast<size_t>(lfScale * 132);
}

void TabManager::RegisterTabManagerClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex = { 0 };
	wcex.cbSize        = sizeof(WNDCLASSEX);
	wcex.hInstance     = hInstance;
	wcex.lpszClassName = L"TabManagerClass";
	wcex.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wcex.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wcex.lpfnWndProc   = TabManagerProcedure;
	wcex.style         = CS_HREDRAW | CS_VREDRAW;

	if (!RegisterClassEx(&wcex))
	{
		throw std::runtime_error("Unable to register the TabManager window class");
	}
}

void TabManager::InitializeTabManagerWindow(HINSTANCE hInstance)
{
	m_hWndSelf = CreateWindow(
		L"TabManagerClass",
		L"",
		WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
		GetX(),
		GetY(),
		static_cast<int>(GetWidth()),
		static_cast<int>(GetHeight()),
		m_hWndParent,
		NULL,
		hInstance,
		this
	);

	if (!m_hWndSelf)
	{
		throw std::runtime_error("Unable to create the TabManager windows");
	}
}

void TabManager::InitializeGraphics(void)
{
	m_pRenderTarget = render::CreateWindowRenderTarget(m_hWndSelf);

	if (!m_pRenderTarget)
	{
		throw std::runtime_error("Unable to create the TabManager render target");
	}

	m_pGDIRT = render::GetInteropGDIRenderTarget(m_pRenderTarget);

	if (!m_pGDIRT)
	{
		throw std::runtime_error("Unable to get the TabManager interop GDI render target");
	}

	m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(BACKGROUND_COLOR), &m_pSolidColorBrush);

	if (!m_pSolidColorBrush)
	{
		throw std::runtime_error("Unable to create a solid color brush for the TabManager");
	}

	m_hFont = util::CreateStandardUIFont(static_cast<int>(20 * util::GetDPIScale(m_hWndSelf)));

	m_hLogoBitmap = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BITMAP1));
}

LRESULT TabManager::WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_ERASEBKGND:
		return FALSE;

	case WM_COMMAND:
		return OnCommand(LOWORD(wParam));

	case WM_MOUSEMOVE:
		return OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

	case WM_LBUTTONDOWN:
		return OnLeftMouseDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

	case WM_PAINT:
		return OnPaint(hWnd);

	case WM_SIZE:
		return OnSize(LOWORD(lParam), HIWORD(lParam));

	case WM_MOUSELEAVE:
		return OnMouseLeave();

	case WM_SWITCH_TO_EXPORT_TAB:
		GetTab(1)->OnCustomMessage(WM_PREPARE_FOR_EXPORT, 0, 0);
		SwitchToTab(1);
		return 0;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

LRESULT TabManager::OnCommand(int id)
{
	switch (id)
	{
	case ID_ACCELERATOR40001: 
		if (!m_Tabs.empty()) {
			SwitchToTab((m_iSelectedTabIndex + 1) % m_Tabs.size());
		} break;

	case ID_ACCELERATOR40003:
		if (!m_Tabs.empty()) {
			SwitchToTab(m_iSelectedTabIndex > 0 ? m_iSelectedTabIndex - 1 : (int)(m_Tabs.size()) - 1);
		} break;
	}

	return 0;
}

LRESULT TabManager::OnMouseMove(int cursorX, int cursorY)
/*++
*
* Routine Description:
*
*	Changes the color of the hovered tab, if it exists.
*
* Arguments:
*
*	cursorX - The x coordinate of the cursor when clicked, in pixels, relatiive to the client.
*	cursorY - The y coordinate of the cursor when clicked, in pixels, relatiive to the client.
*
* Return Value:
*
*	Zero.
*
--*/
{
	D2D1_RECT_F rcHoveredTabBounds;
	
	int index = GetIndexOfTabUnderPoint(cursorX, cursorY);

	// Tab under cursor
	if (index != INVALID_TAB_INDEX)
	{
		InvalidateTabShape(m_iHoveredTabIndex);
		InvalidateTabShape(index);

		m_iHoveredTabIndex = index;

		if (!m_isTrackingCursor)
		{
			TRACKMOUSEEVENT tme = {};
			tme.cbSize      = sizeof(TRACKMOUSEEVENT);
			tme.dwFlags     = TME_LEAVE;
			tme.hwndTrack   = m_hWndSelf;
			tme.dwHoverTime = 0;

			m_isTrackingCursor = TrackMouseEvent(&tme);
		}
	}

	// If we were hovering before not having a tab under cursor, redraw it and unselect it
	else if (m_iHoveredTabIndex != INVALID_TAB_INDEX)
	{
		InvalidateTabShape(m_iHoveredTabIndex);

		m_iHoveredTabIndex = INVALID_TAB_INDEX;
	}

	return 0;
}

LRESULT TabManager::OnLeftMouseDown(int cursorX, int cursorY)
/*++
* 
* Routine Description:
* 
*	Changes the selected tab, if clicked.
* 
* Arguments:
* 
*	cursorX - The x coordinate of the cursor when clicked, in pixels, relatiive to the client.
*	cursorY - The y coordinate of the cursor when clicked, in pixels, relatiive to the client.
* 
* Return Value:
* 
*	Zero.
* 
--*/
{
	if (cursorY <= static_cast<int>(cyTabBar) && m_iHoveredTabIndex != INVALID_TAB_INDEX)
	{
		SwitchToTab(m_iHoveredTabIndex);
	}

	return 0;
}

LRESULT TabManager::OnPaint(HWND hWnd)
{
	HRESULT hr;
	PAINTSTRUCT ps;
	HDC hDC = NULL;

	if (m_pRenderTarget && m_pGDIRT)
	{
		BeginPaint(m_hWndSelf, &ps);
		m_pRenderTarget->BeginDraw();

		DrawBackground();
		DrawTabs();
		DrawTabLines();

		hr = m_pGDIRT->GetDC(D2D1_DC_INITIALIZE_MODE_COPY, &hDC);

		if (SUCCEEDED(hr))
		{
			SelectObject(hDC, m_hFont);
			SetBkMode(hDC, TRANSPARENT);

			for (size_t i = 0; i < m_Tabs.size(); ++i)
			{
				RECT rc;
				GetTabShapeBounds((int)i, &rc);

				if (i == m_iSelectedTabIndex) {
					SetTextColor(hDC, RGB(40, 40, 40));
				} else {
					SetTextColor(hDC, RGB(100, 100, 100));
				}

				DrawText(hDC, m_Tabs[i]->GetName().c_str(), m_Tabs[i]->GetName().length(), &rc, DT_VCENTER | DT_CENTER | DT_SINGLELINE);
			}

			m_pGDIRT->ReleaseDC(NULL);
		}

		m_pRenderTarget->EndDraw();

		EndPaint(m_hWndSelf, &ps);
	}

	return 0;
}

void TabManager::DrawTabs(void)
{
	const int iTabCount = static_cast<int>(m_TabGeometries.size());

	for (int i = 0; i < iTabCount; ++i)
	{
		if (i == m_iSelectedTabIndex) {
			DrawTab(i, BACKGROUND_COLOR, RGB_D2D(150, 150, 150));
		} else if (i == m_iHoveredTabIndex) {
			DrawTab(i, HOVERED_TAB_COLOR, RGB_D2D(130, 130, 130));
		} else {
			DrawTab(i, UNSELECTED_TAB_COLOR, RGB_D2D(130, 130, 130));
		}
	}
}

void TabManager::DrawTabLines(void)
{
	m_pSolidColorBrush->SetColor(D2D1::ColorF(RGB_D2D(150, 150, 150)));

	if (m_iSelectedTabIndex > 0)
	{
		m_pRenderTarget->DrawLine(
			D2D1::Point2F(0.0F, static_cast<FLOAT>(cyTabBar)),
			D2D1::Point2F(m_iSelectedTabIndex * (cxTab + 3), static_cast<FLOAT>(cyTabBar)),
			m_pSolidColorBrush
		);
	}

	m_pRenderTarget->DrawLine(
		D2D1::Point2F((m_iSelectedTabIndex) * (cxTab + 3) + cxTab, cyTabBar),
		D2D1::Point2F(GetWidth(), cyTabBar),
		m_pSolidColorBrush
	);
}

void TabManager::DrawBackground(void)
/*++
*
* Routine Description:
*
*	Draws the background of the tab as well as the area behind the tabs,
*	because for some reason it is rendered as black by Direct2D.
*
* Arguments:
*
*	None.
*
* Return Value:
*
*	None.
*
--*/
{
	const FLOAT fClientWidth = static_cast<FLOAT>(GetWidth());

	// So here's what's happening
	// If we don't draw this, then a black line appears at the top of the window
	// That ruins the way that the tab connects to the main area
	// The reason we wouldn't want to draw this in the first place is because
	// We have to redraw the tab background anyway to erase the windows upon
	// swapping, which means this is an extra drawing call
	m_pSolidColorBrush->SetColor(D2D1::ColorF(BACKGROUND_COLOR));

	m_pRenderTarget->FillRectangle(
		D2D1::RectF(0.0F, cyTabBar, fClientWidth, cyTabBar * 2),
		m_pSolidColorBrush
	);	

	DWORD GetSysColor(COLOR_ACTIVECAPTION);

	// Area behind tabs
	m_pSolidColorBrush->SetColor(D2D1::ColorF(D2D1::ColorF::White));

	m_pRenderTarget->FillRectangle(
		D2D1::RectF(0.0F, 0.0F, fClientWidth, static_cast<FLOAT>(cyTabBar)),
		m_pSolidColorBrush
	);
}

void TabManager::DrawTab(int index, COLORREF crFill, COLORREF crOutline)
/*++
*
* Routine Description:
*
*	This draws the trapezoid shape and outline of a tab at the top of the screen.
*	The horizontal position of the drawn shape depends on the index.
*
* Arguments:
*
*	index - Index of the tab, must be within the bounds of m_TabGeometries.
*	crFill - Color used to fill the shape.
*	crOutline - Color used to draw the outline.
*
* Return Value:
*
*	None.
*
--*/
{
	assert(index >= 0 && index < m_Tabs.size());

	const D2D1_COLOR_F brushColorBeforeCall = m_pSolidColorBrush->GetColor();

	m_pSolidColorBrush->SetColor(D2D1::ColorF(crFill));
	m_pRenderTarget->FillGeometry(m_TabGeometries[index], m_pSolidColorBrush);

	m_pSolidColorBrush->SetColor(D2D1::ColorF(crOutline));
	m_pRenderTarget->DrawGeometry(m_TabGeometries[index], m_pSolidColorBrush);

	m_pSolidColorBrush->SetColor(brushColorBeforeCall);
}

ID2D1PathGeometry* TabManager::CreateTabPathGeometry(int index)
/*++
* 
* Routine Description:
* 
*	Creates a path geometry where the tab with the given index will be located.
* 
*	We'll use this method to avoid creating a path geometry every time WM_PAINT
*	is sent, and because we can't reuse the same object and because we cant
*	move the geometry around as far as I know.
* 
*	MSDN says that this object is device independent and can therefore stay
*	alive for the whole duration of the program, so this is acceptable.
* 
* Arguments:
* 
*	index - The index of the tab in m_Tabs. 
*	        Notice that the index is not restriced to any value range, because it is only
*	        used to determine the position of the shape. This is because for example there
*	        may be a setting in the future to have the tabs on the right instead of the left.
* 
* Return Value:
* 
*	Pointer to the created path geometry.
* 
--*/
{
	ID2D1PathGeometry* pPathGeometry = render::CreatePathGeometry();
	ID2D1GeometrySink* pSink = nullptr;
	
	D2D1_POINT_2F ptBegin;
	HRESULT hr;

	if (!pPathGeometry)
	{
		throw std::runtime_error("Unable to create tab path geometry");
	}

	hr = pPathGeometry->Open(&pSink);

	if (SUCCEEDED(hr))
	{
		// This is the bottom left point of the polygon from where we'll begin drawing
		ptBegin = D2D1::Point2F((FLOAT)(index * cxTab) + index * 3, (FLOAT)(cyTabBar));

		pSink->BeginFigure(D2D1::Point2F(ptBegin.x, ptBegin.y), D2D1_FIGURE_BEGIN_FILLED);

		// Top left point of the polygon
		pSink->AddLine(D2D1::Point2F((FLOAT)(ptBegin.x + cxTab / 11), (FLOAT)(ptBegin.y - cyTabBar)));

		// Top right point of the polygon
		pSink->AddLine(D2D1::Point2F((FLOAT)(ptBegin.x + cxTab * 10 / 11), ptBegin.y - cyTabBar));

		// Bottom right point of the polygon
		pSink->AddLine(D2D1::Point2F((FLOAT)(ptBegin.x + cxTab), ptBegin.y));

		// We won't add the initial point and we'll leave the shape open, so that we can:
		// - Use the FillGeometry function and fill it as we wish
		// - Use the DrawGeometry function afterwards having avoided drawing the bottom line
		pSink->EndFigure(D2D1_FIGURE_END::D2D1_FIGURE_END_OPEN);

		pSink->Close();
		pSink->Release();
	}

	return pPathGeometry;
}

LRESULT TabManager::OnSize(WORD wNewWidth, WORD wNewHeight)
/*++
*
* Routine Description:
*
*	Resizes the render target and all the tabs.
*
* Arguments:
*
*	wNewWidth  - New width of the window.
*	wNewHeight - New height of the window.
*
* Return Value:
*
*	Zero.
*
--*/
{
	if (m_pRenderTarget)
	{
		render::FitRenderTargetToClient(m_pRenderTarget);
	}

	if (m_iSelectedTabIndex >= 0 && m_iSelectedTabIndex < m_Tabs.size())
	{
		m_Tabs[m_iSelectedTabIndex]->FitToTabManager();
	}

	return 0;
}

LRESULT TabManager::OnMouseLeave(void)
{
	m_isTrackingCursor = false;

	if (m_iHoveredTabIndex != INVALID_TAB_INDEX)
	{
		InvalidateTabShape(m_iHoveredTabIndex);
		m_iHoveredTabIndex = INVALID_TAB_INDEX;
	}

	return 0;
}

void TabManager::AppendTab(Tab* pTab)
/*++
*
* Routine Description:
*
*	Adds a tab to the collection of tabs.
*	If it is the first tab to be added, it is also displayed.
* 
*	This function is called by the Tab constructor and should therefore not be called directly.
*
* Arguments:
*
*	pTab - Pointer to the tab.
*
* Return Value:
*
*	None.
*
--*/
{
	assert(pTab);

	if (std::find(m_Tabs.begin(), m_Tabs.end(), pTab) == m_Tabs.end())
	{
		// When we create a tab, it is hidden by default, so as not to have all tabs displayed at the same time.
		// However if the appended tab is the first tab to be added, then we can and we must display its content
		if (m_Tabs.empty())
		{
			pTab->Show();
		}

		m_Tabs.emplace_back(pTab);
		m_TabGeometries.emplace_back(CreateTabPathGeometry((int)m_Tabs.size() - 1));
	}
}

int TabManager::GetIndexOfTabUnderPoint(int x, int y)
/*++
* 
* Routine Description:
* 
*	Calculates which tab is below a given point.
* 
* Arguments:
* 
*	x - The x coordinate of the cursor when clicked, in pixels, relatiive to the client.
*	y - The y coordinate of the cursor when clicked, in pixels, relatiive to the client.
* 
* Return Value:
* 
*	The index of the tab below the given point.
*	If no tab is below the given point, INVALID_TAB_INDEX is returned.
* 
--*/
{
	BOOL bFillContainsPoint;
	HRESULT hr;

	if (y > cyTabBar)
	{
		return INVALID_TAB_INDEX;
	}

	const int potentialHoveredTab = x / cxTab;

	if (potentialHoveredTab >= 0 && potentialHoveredTab < m_Tabs.size())
	{
		hr = m_TabGeometries[potentialHoveredTab]->FillContainsPoint(D2D1::Point2F((FLOAT)x, (FLOAT)y), NULL, &bFillContainsPoint);

		if (SUCCEEDED(hr) && bFillContainsPoint)
		{
			return potentialHoveredTab;
		}
	}

	return INVALID_TAB_INDEX;
}

void TabManager::InvalidateTabShape(int index)
/*++
* 
* Routine Description:
* 
*	Invalidates the tab shape of a given tab.
* 
*	If the given index does not correspond do an existing tab, this function does nothing.
* 
* Arguments:
* 
*	index - The index of the tab in m_Tabs.
* 
* Return Value:
* 
*	None.
* 
--*/
{
	D2D1_RECT_F rcHoveredTabBounds;
	RECT rcInvalidatedArea;

	if (index >= 0 && index < static_cast<int>(m_TabGeometries.size()))
	{
		if (GetTabShapeBounds(index, &rcInvalidatedArea))
		{
			InvalidateRect(m_hWndSelf, &rcInvalidatedArea, FALSE);
		}
	}
}

bool TabManager::GetTabShapeBounds(int index, RECT* pRect)
/*++
* 
* Routine Description:
* 
*	Gets the rectangle that contains a given tab shape.
* 
* Arguments:
* 
*	index - The index of the tab in m_Tabs.
*	pRect - Pointer to a RECT structure that receives the bounds.
* 
* Return Value:
* 
*	Returns true upon success, otherwise false.
* 
--*/
{
	HRESULT hr;
	D2D1_RECT_F rcD2DBounds;
	RECT rcBounds;

	if (!pRect || index < 0 || index >= static_cast<int>(m_Tabs.size()))
	{
		return false;
	}

	hr = m_TabGeometries[index]->GetBounds(NULL, &rcD2DBounds);

	if (SUCCEEDED(hr))
	{
		pRect->left   = static_cast<LONG>(rcD2DBounds.left);
		pRect->top    = static_cast<LONG>(rcD2DBounds.top);
		pRect->right  = static_cast<LONG>(rcD2DBounds.right);
		pRect->bottom = static_cast<LONG>(rcD2DBounds.bottom);

		return true;
	}

	return false;
}

size_t TabManager::GetTabShapeHeight(void) const noexcept
{
	return cyTabBar;
}

Tab* TabManager::GetCurrentTab(void)
{
	if (m_iSelectedTabIndex == INVALID_TAB_INDEX)
	{
		return nullptr;
	}

	return m_Tabs[m_iSelectedTabIndex];
}

void TabManager::SwitchToTab(int iTabIndex)
{
	assert(iTabIndex >= 0 && iTabIndex < m_Tabs.size());

	if (iTabIndex != m_iSelectedTabIndex)
	{
		if (m_iSelectedTabIndex != INVALID_TAB_INDEX)
		{
			m_Tabs[m_iSelectedTabIndex]->OnSwitchedToOther();
			InvalidateTabShape(m_iSelectedTabIndex);
			m_Tabs[m_iSelectedTabIndex]->Hide();
		}

		m_Tabs[iTabIndex]->Show();
		m_Tabs[iTabIndex]->FitToTabManager();

		InvalidateTabShape(iTabIndex);
		InvalidateRect(m_Tabs[iTabIndex]->GetHandle(), NULL, FALSE);

		m_iSelectedTabIndex = iTabIndex;

		m_Tabs[iTabIndex]->OnSwitchedToSelf();
	}
}

Tab* TabManager::GetTab(int index)
{
	if (index >= 0 && index < m_Tabs.size())
	{
		return m_Tabs[index];
	}

	return nullptr;
}