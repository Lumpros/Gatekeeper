#include "ListView.h"
#include "Utility.h"
#include "Renderer.h"


#include <CommCtrl.h>
#include <windowsx.h>
#include <Shlwapi.h>

#include <cassert>
#include <stdexcept>
#include <algorithm>

#define MINIMUM_COLUMN_WIDTH 30

#define COLOR_ROW_UNSELECTED RGB(247, 247, 247)
#define COLOR_ROW_HOVERING   RGB(240, 240, 240)
#define COLOR_ROW_SELECTED   RGB(230, 230, 230)

LRESULT CALLBACK ListViewProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
/*++
*
* Routine Description:
*
*	Registers the ListView object and calls its member procedure function.
*
* Arguments:
*
*	hWnd - Handle to the window the received the message.
*	uMsg - Message code
*	wParam - Message info; depends on the message.
*	lParam - Message info; depends on the message.
*
* Return Value:
*
*	Window procedure return code.
*
--*/
{
	ListView* pViewList;

	switch (uMsg)
	{
	case WM_CREATE:
		util::RegisterObject(hWnd, ((LPCREATESTRUCT)lParam)->lpCreateParams);
		return 0;

	default:
		if (pViewList = (ListView*)util::GetRegisteredObject(hWnd))
		{
			return pViewList->WindowProcedure(hWnd, uMsg, wParam, lParam);
		}
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

LRESULT ListView::WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
/*++
* 
* Routine Description:
* 
*	Calls the appropriate function based on uMsg.
* 
* Arguments:
* 
*	hWnd - Handle to the window the received the message.
*	uMsg - Message code
*	wParam - Message info; depends on the message.
*	lParam - Message info; depends on the message.
* 
* Return Value:
* 
*	Window procedure return code.
* 
--*/
{
	switch (uMsg)
	{
	case WM_MOUSEWHEEL:
		if ((LOWORD(wParam) & MK_SHIFT) == 0) {
			return OnVScroll(GET_WHEEL_DELTA_WPARAM(wParam) > 0 ? SB_LINEUP : SB_LINEDOWN, GET_WHEEL_DELTA_WPARAM(wParam));
		} else {
			return OnHScroll(GET_WHEEL_DELTA_WPARAM(wParam) > 0 ? SB_LINEUP : SB_LINEDOWN, GET_WHEEL_DELTA_WPARAM(wParam));
		}

	case WM_MOUSEHWHEEL:
		return OnHScroll(GET_WHEEL_DELTA_WPARAM(wParam) < 0 ? SB_LINEUP : SB_LINEDOWN, GET_WHEEL_DELTA_WPARAM(wParam));

	case WM_PAINT:
		return OnPaint();

	case WM_LBUTTONDOWN:
		return OnLeftMouseDown(wParam, lParam);

	case WM_LBUTTONUP:
		return OnLeftMouseUp(wParam, lParam);

	case WM_ERASEBKGND:
		return OnEraseBackground(reinterpret_cast<HDC>(wParam));

	case WM_MOUSEMOVE:
		return OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

	case WM_VSCROLL:
		return OnVScroll(LOWORD(wParam), HIWORD(wParam));

	case WM_HSCROLL:
		return OnHScroll(LOWORD(wParam), HIWORD(wParam));

	case WM_SIZE:
		return OnSize(LOWORD(lParam), HIWORD(lParam));

	case WM_MOUSELEAVE:
		return OnMouseLeave();

	case WM_KEYDOWN:
		return OnKeyDown(wParam);
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

ListView::ListView(HWND hParentWindow, Size size, Point ptPos)
	: Window(hParentWindow, size, ptPos)
{
	const HINSTANCE hInstance = GetModuleHandle(NULL);
	const double lfDpiScale = util::GetDPIScale(hParentWindow);

	cyLabelBar       = static_cast<size_t>(lfDpiScale * 30.0);
	cyRow            = cyLabelBar - static_cast<size_t>(6 * lfDpiScale);
	cxScrollbarWidth = static_cast<size_t>(20.0 * lfDpiScale);

	m_refIndexesOfShownRows = &m_IndexesOfShownRows;
	m_refRows = &m_Rows;

	RegisterViewListClass(hInstance);
	InitializeViewListWindow(hInstance);
	InitializeGraphicsResources();
}

void ListView::InitializeGraphicsResources(void)
/*++
* 
* Routine Description:
* 
*	Creates all the resources that are needed in order to draw the window.
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
	m_hFont = util::CreateStandardUIFont(16 * util::GetDPIScale(m_hWndSelf));

	if (!m_hFont)
	{
		throw std::runtime_error("Unable to create GDI Font for ListView");
	}

	m_pRenderTarget = render::CreateWindowRenderTarget(m_hWndSelf);

	if (!m_pRenderTarget)
	{
		throw std::runtime_error("Unable to create Direct2D Render Target for ListView window");
	}

	m_pGDIRT = render::GetInteropGDIRenderTarget(m_pRenderTarget);

	if (!m_pGDIRT)
	{
		throw std::runtime_error("Unable to get the GDI Interop render target from the ListView Render Target");
	}

	m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0xFF, 0xFF, 0xFF), &m_pSolidColorBrush);

	if (!m_pSolidColorBrush)
	{
		throw std::runtime_error("Unable to create ListView Solid Color brush");
	}
}

void ListView::RegisterViewListClass(HINSTANCE hInstance)
/*++
*
* Routine Description:
*
*	Registers the ticket list window class.
*
* Arguments:
*
*	hInstance - Handle to the process instance.
*
* Return Value:
*
*	None.
*
--*/
{
	assert(hInstance != NULL);

	static bool hasBeenRegistered = false;

	if (!hasBeenRegistered)
	{
		WNDCLASSEX wcex;
		ZeroMemory(&wcex, sizeof(wcex));
		wcex.cbSize = sizeof(wcex);
		wcex.hInstance = hInstance;
		wcex.lpszClassName = L"ViewListClass";
		wcex.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
		wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
		wcex.lpfnWndProc = ListViewProcedure;
		wcex.style = CS_HREDRAW | CS_VREDRAW;

		if (!RegisterClassEx(&wcex))
		{
			throw std::runtime_error("Unable to register the ListView class");
		}

		hasBeenRegistered = true;
	}
}

void ListView::InitializeViewListWindow(HINSTANCE hInstance)
/*++
*
* Routine Description:
*
*	Creates the ticket list window.
*
* Arguments:
*
*	hInstance - Handle to the process instance
*
* Return Value:
*
*	None.
*
--*/
{
	assert(hInstance);

	m_hWndSelf = CreateWindow(
		L"ViewListClass",
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
		throw std::runtime_error("Unable to initialize the ListView window");
	}

	m_hVertSB = CreateWindow(
		L"SCROLLBAR",
		L"",
		WS_CHILD | WS_VISIBLE | SBS_VERT,
		0, 0, 0, 0,
		m_hWndSelf,
		NULL,
		hInstance,
		nullptr
	);

	if (!m_hVertSB)
	{
		throw std::runtime_error("Unable to create the ListView's vertical scrollbar");
	}

	m_hHorzSB = CreateWindow(
		L"SCROLLBAR",
		L"",
		WS_CHILD | WS_VISIBLE | SBS_HORZ,
		0, 0, 0, 0,
		m_hWndSelf,
		NULL,
		hInstance,
		nullptr
	);

	if (!m_hHorzSB)
	{
		throw std::runtime_error("Unable to create the ListView's horizontal scrollbar");
	}

	FitScrollbarsToClient();
}

void ListView::FitScrollbarsToClient(void)
/*++
* 
* Routine Description:
* 
*	Sets the scrollbars' point and size so that they're exactly as wanted.
* 
*	The vertical scrollbar must be at the most-right point of the window, under the
*	label bar, and its bottom must end a little above the window height in order to 
*   leave an empty square.
* 
*   The horizontal scrollbar must be at the lowest point of the window, and should
*	leave some space at the right side of the window for the empty square.
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
	const int iSelfWidth = static_cast<int>(GetWidth());
	const int iSelfHeight = static_cast<int>(GetHeight());

	SetWindowPos(
		m_hHorzSB,
		NULL,
		1,
		(int)(iSelfHeight - cxScrollbarWidth - 1),
		(int)(iSelfWidth - cxScrollbarWidth - 1),
		(int)cxScrollbarWidth,
		SWP_NOZORDER
	);

	SetWindowPos(
		m_hVertSB,
		NULL,
		(int)(iSelfWidth - cxScrollbarWidth - 1),               // Width minus the width minus 1 for the line around the window
		(int)cyLabelBar + 1,                                    // Plus one for the line around the window
		(int)cxScrollbarWidth,
		(int)(iSelfHeight - cyLabelBar - cxScrollbarWidth - 1), // Minus the label bar because the scrollbar starts below it,
																// minus the scrollbar width because we will add a horizontal
																// bar below it, and minus 1 pixel for the lines around it
		SWP_NOZORDER
	);
}

ListView::~ListView(void)
{
	ReleaseGraphicsResources();
}

void ListView::ReleaseGraphicsResources(void)
/*++
* 
* Routine Description:
* 
*	Releases every resource created in InitializeGraphicsResources.
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
	if (m_hFont)
	{
		DeleteObject(m_hFont);
		m_hFont = nullptr;
	}

	SAFE_RELEASE_D2D(m_pRenderTarget);
	SAFE_RELEASE_D2D(m_pGDIRT);
	SAFE_RELEASE_D2D(m_pSolidColorBrush);
}

LRESULT ListView::OnPaint(void)
/*++
* 
* Routine Description:
* 
*	Draws the column labels.
* 
* Arguments:
* 
*	None.
* 
* Return Value:
* 
*	Window Procedure return code.
* 
--*/
{
	HDC hDC = NULL;
	PAINTSTRUCT ps;
	HRESULT hr;
	
	if (m_pGDIRT && m_pRenderTarget)
	{
		const size_t uSelfHeight = GetHeight();
		const size_t uSelfWidth = GetWidth();

		BeginPaint(m_hWndSelf, &ps);
		m_pRenderTarget->BeginDraw();

		// These functions must be called before m_pGDIRT->GetDC
		// Otherwise the planet will literally explode.
		DrawBackground(uSelfWidth, uSelfHeight);
		DrawRowBackground(uSelfWidth, uSelfHeight);

		hr = m_pGDIRT->GetDC(D2D1_DC_INITIALIZE_MODE_COPY, &hDC);

		if (SUCCEEDED(hr))
		{
			SelectObject(hDC, m_hFont);

			DrawRows(hDC, uSelfWidth, uSelfHeight);
			DrawLabelBar(hDC, uSelfWidth, uSelfHeight);
			DrawColumns(hDC, uSelfWidth, uSelfHeight);

			// Square at the bottom right corner below the scrollbars.
			SetDCBrushColor(hDC, RGB(230, 230, 230));
			SetDCPenColor(hDC, RGB(230, 230, 230));

			Rectangle(
				hDC,
				static_cast<int>(GetWidth() - cxScrollbarWidth),
				static_cast<int>(GetHeight() - cxScrollbarWidth),
				static_cast<int>(GetWidth() - 1),
				static_cast<int>(GetHeight() - 1)
			);

			m_pGDIRT->ReleaseDC(NULL);
		}

		m_pRenderTarget->EndDraw();

		EndPaint(m_hWndSelf, &ps);
	}

	return 0;
}

void ListView::DrawBackground(size_t uWidth, size_t uHeight)
{
	m_pSolidColorBrush->SetColor(D2D1::ColorF(0xFF, 0xFF, 0xFF));
	m_pRenderTarget->FillRectangle(
		D2D1::RectF(0, 0, uWidth, uHeight),
		m_pSolidColorBrush
	);

	m_pSolidColorBrush->SetColor(D2D1::ColorF(0, 0, 0));
	m_pRenderTarget->DrawRectangle(
		D2D1::RectF(0, 0, uWidth, uHeight),
		m_pSolidColorBrush
	);
}

void ListView::DrawRowBackground(size_t uWidth, size_t uHeight)
/*++
* 
* Routine Description:
* 
*	Draws the grey area behind the row text.
* 
* Arguments:
* 
*	uWidth - ListView width.
*	uHeight - ListView height.
* 
* Return Value:
* 
*	None.
* 
--*/
{
	m_pSolidColorBrush->SetColor(D2D1::ColorF(COLOR_ROW_UNSELECTED));

	// We'll use a transformation here because if the window has been
	// scrolled, the rectangle must be drawn in a different position.
	m_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Translation(
		D2D1::SizeF(0, m_cyOffset)
	));

	m_pRenderTarget->FillRectangle(
		D2D1::RectF(1, cyLabelBar, uWidth - cxScrollbarWidth, cyLabelBar + m_refIndexesOfShownRows->size() * cyRow),
		m_pSolidColorBrush);

	// Reset the transformation
	m_pRenderTarget->SetTransform(D2D1::Matrix3x2F::Translation(D2D1::SizeF(0, 0)));
}

void ListView::DrawRows(HDC hDC, size_t uWidth, size_t uHeight)
/*++
*
* Routine Description:
*
*	Draws every single visible row.
*
* Arguments:
*
*	hDC - Handle to the device context
*	uWidth - ListView width.
*	uHeight - ListView height.
*
* Return Value:
*
*	None.
*
--*/
{
	POINT ptOldOrigin;

	SetBkMode(hDC, TRANSPARENT);

	SelectObject(hDC, GetStockObject(DC_BRUSH));
	SelectObject(hDC, GetStockObject(DC_PEN));

	SetDCBrushColor(hDC, COLOR_ROW_UNSELECTED);
	SetDCPenColor(hDC, COLOR_ROW_UNSELECTED);
	
	SetViewportOrgEx(hDC, m_cxOffset, m_cyOffset, &ptOldOrigin);
	
	// The index of the first row that is visible on screen
	// We're basically calculating how many rows have been scrolled off screen by
	// dividing the vertical offset with the row height
	const int indexFirstRowVisible = min(-(m_cyOffset / (int)cyRow), (int)m_refIndexesOfShownRows->size());

	// The index of the last row that is visible on screen
	const int indexLastRowVisible = min(
		((int)m_refIndexesOfShownRows->size()) + indexFirstRowVisible - GetExtraRowsOffScreenCount() + 1,
		(int)m_refIndexesOfShownRows->size()
	);

	for (int i = indexFirstRowVisible; i < indexLastRowVisible; ++i)
	{
		DrawRow(hDC, uWidth, uHeight, i);
	}

	SetViewportOrgEx(hDC, ptOldOrigin.x, ptOldOrigin.y, NULL);
}

void ListView::DrawRow(HDC hDC, size_t uWidth, size_t uHeight, size_t index)
/*++
* 
* Routine Description:
* 
*	Draws the row specified.
* 
* Arguments:
* 
*	hDC - Handle to the device context
*	uWidth - Width of the ListView
*	uHeight - Height of the ListView
*	index - Index of data in IndexesOfShownRows
* 
* Return Value:
* 
*	None.
* 
--*/
{
	COLORREF crSpecialRow;

	assert(index < m_refIndexesOfShownRows->size());

	// Top point of the given row in client coordinates
	const int iRowTop = static_cast<int>(cyRow * index + cyLabelBar);

	// In the case that a row is selected or hovered by the cursor, we'll draw
	// a different color below it to indicate such event to the user.
	// m_iHoveringRowIndex may be negative so we'll do this conversion to int
	if (m_iHoveringRowIndex == static_cast<int>(index) || m_iSelectedIndex == static_cast<int>(index))
	{
		crSpecialRow = (m_iSelectedIndex == static_cast<int>(index) ? COLOR_ROW_SELECTED : COLOR_ROW_HOVERING);

		SetDCBrushColor(hDC, crSpecialRow);

		// We don't want any horizontal offset in this case
		SetViewportOrgEx(hDC, 0, m_cyOffset, NULL);

		Rectangle(
			hDC,
			1,
			iRowTop,
			uWidth - 1,
			iRowTop + cyRow
		);

		SetDCBrushColor(hDC, COLOR_ROW_UNSELECTED);

		// Reset the viewport
		SetViewportOrgEx(hDC, m_cxOffset, m_cyOffset, NULL);
	}

	// It is possible that a column may not have been used by a row
	// This could happen if for example a column was added after a row
	// This is why we do the folllowing
	size_t usedColumnCount = min(m_Columns.size(), (*m_refRows)[(*m_refIndexesOfShownRows)[index]].size());

	// The x coordinate in client terms of the next line to be drawn vertically to seperate columns
	int iNextLineX = 0;

	for (size_t i = 0; i < usedColumnCount; ++i)
	{
		iNextLineX += m_Columns[i].cxWidth;

		RECT rcText = {};
		rcText.left = iNextLineX - m_Columns[i].cxWidth;
		rcText.right = iNextLineX;
		rcText.top = iRowTop;
		rcText.bottom = iRowTop + cyRow;

		SetTextColor(hDC, GetWordColor((*m_refRows)[(*m_refIndexesOfShownRows)[index]][i], i));

		DrawText(
			hDC,
			(*m_refRows)[(*m_refIndexesOfShownRows)[index]][i].c_str(),
			(*m_refRows)[(*m_refIndexesOfShownRows)[index]][i].length(),
			&rcText,
			DT_CENTER | DT_END_ELLIPSIS | DT_VCENTER | DT_SINGLELINE
		);
	}
}

void ListView::DrawLabelBar(HDC hDC, size_t uWidth, size_t uHeight)
{
	SetViewportOrgEx(hDC, 0, 0, NULL);

	SelectObject(hDC, GetStockObject(DC_BRUSH));
	SetDCBrushColor(hDC, RGB(240, 240, 240));

	SelectObject(hDC, GetStockObject(DC_PEN));
	SetDCPenColor(hDC, RGB(230, 230, 230));

	// Top Half of Label bar rectangle
	Rectangle(hDC, 1, 1, GetWidth() - 1, cyLabelBar / 2 + 1);

	SetDCBrushColor(hDC, RGB(230, 230, 230));

	// Bottom Half of label bar rectangle
	Rectangle(hDC, 1, cyLabelBar / 2 + 1, GetWidth() - 1, cyLabelBar);

	if (m_iHoveredColumnLabel != ROW_INDEX_NONE)
	{
		SetViewportOrgEx(hDC, m_cxOffset, 0, NULL);

		const int iHoveredColumnPos = GetRelativeColumnHorizontalPosition(m_iHoveredColumnLabel);

		COLORREF crTop, crBottom;

		if (m_iClickedColumnLabel != ROW_INDEX_NONE) {
			crTop = RGB(230, 230, 230);
			crBottom = RGB(220, 220, 220);
		} else {
			crTop = RGB(235, 235, 235);
			crBottom = RGB(225, 225, 225);
		}

		SetDCBrushColor(hDC, crTop);
		SetDCPenColor(hDC, crBottom);
		Rectangle(
			hDC,
			max(iHoveredColumnPos + 1, 1 - m_cxOffset),
			1,
			min(iHoveredColumnPos + m_Columns[m_iHoveredColumnLabel].cxWidth, GetWidth() - m_cxOffset - 1),
			cyLabelBar / 2 + 1
		);

		SetDCBrushColor(hDC, crBottom);
		Rectangle(
			hDC,
			max(iHoveredColumnPos + 1, 1 - m_cxOffset),
			cyLabelBar / 2 + 1,
			min(iHoveredColumnPos + m_Columns[m_iHoveredColumnLabel].cxWidth, GetWidth() - m_cxOffset - 1),
			cyLabelBar
		);

		SetViewportOrgEx(hDC, 0, 0, NULL);
	}

	SetDCPenColor(hDC, RGB(109, 131, 147));
	
	// Line below the label bar
	MoveToEx(hDC, 0, cyLabelBar, NULL);
	LineTo(hDC, uWidth, cyLabelBar);

	SetDCPenColor(hDC, RGB(128, 154, 173));

	MoveToEx(hDC, 0, 0, NULL);
	LineTo(hDC, uWidth, 0);

	MoveToEx(hDC, 0, uHeight - 1, NULL);
	LineTo(hDC, uWidth, uHeight - 1);

	MoveToEx(hDC, uWidth - 1, cyLabelBar, NULL);
	LineTo(hDC, uWidth - 1, uHeight - 1);
}

void ListView::DrawColumns(HDC hDC, size_t uWidth, size_t uHeight)
{
	// Apply the scroll
	SetViewportOrgEx(hDC, m_cxOffset, 0, NULL);

	int iNextLineX = 0;

	for (size_t i = 0; i < m_Columns.size(); ++i)
	{
		ColumnInfo& column = m_Columns[i];

		// Calculate the position of the vertical line to the right of the text
		iNextLineX += column.cxWidth;

		// The first statement pretty much says that if the line to the right of
		// a column is off screen, we'll obviously skip drawing the said column.
		// The second statement says that if the line to the left of a column is
		// off screen (to the right) then we'll skip drawing the column
		if (iNextLineX + m_cxOffset <= 0 || iNextLineX + m_cxOffset - column.cxWidth >= (int)uWidth)
		{
			continue;
		}

		// Draw the forementioned line
		MoveToEx(hDC, iNextLineX, 0, NULL);
		LineTo(hDC, iNextLineX, uHeight - cxScrollbarWidth);

		RECT rcText = {};
		rcText.left = iNextLineX - column.cxWidth;
		rcText.right = iNextLineX;
		rcText.top = 0;
		rcText.bottom = cyLabelBar;

		DrawText(
			hDC,
			column.strName.c_str(),
			column.strName.length(),
			&rcText,
			DT_CENTER | DT_END_ELLIPSIS | DT_VCENTER | DT_SINGLELINE
		);
	}

	// Reset the viewport
	SetViewportOrgEx(hDC, 0, 0, NULL);
}

LRESULT ListView::OnEraseBackground(HDC hDC)
{
	return FALSE; // FALSE means we didn't erase the background.
}

LRESULT ListView::OnLeftMouseDown(WPARAM wParam, LPARAM lParam)
/*++
* 
* Routine Description:
* 
*	Initiates the dragging process, if it may begin.
* 
* Arguments:
* 
* Return Value:
* 
*	Win32 API Return Code. Always Zero.
* 
--*/
{
	if (GetFocus() != m_hWndSelf)
	{
		SetFocus(m_hWndSelf);
	}

	// If this is true it means the cursor is over a column line and we may start dragging.
	if (m_canDrag)
	{
		m_isDragging = true;
		m_iDraggingStartX = GET_X_LPARAM(lParam) - m_cxOffset; // Remove the scroll offset
		m_widthBeforeDragging = m_Columns[m_iDraggingIndex].cxWidth;
	}

	else if (GET_Y_LPARAM(lParam) <= cyLabelBar)
	{
		// Find which column label the cursor is over
		int iCurrentX = m_cxOffset;
		int iCursorX = GET_X_LPARAM(lParam);
		int iColumnIndex = COLUMN_INDEX_NONE;

		for (size_t i = 0; i < m_Columns.size(); ++i)
		{
			if (iCursorX >= iCurrentX && iCursorX < m_Columns[i].cxWidth + iCurrentX)
			{
				iColumnIndex = i;
				break;
			}

			iCurrentX += m_Columns[i].cxWidth;
		}

		m_iClickedColumnLabel = m_iHoveredColumnLabel;
		InvalidateColumnLabelBox(m_iClickedColumnLabel);
	}

	// If this is true then some row has been clicked, so we must mark it as such
	else if (m_iHoveringRowIndex != m_iSelectedIndex)
	{
		InvalidateRow(m_iHoveringRowIndex);
		
		if (m_iSelectedIndex != ROW_INDEX_NONE)
		{
			InvalidateRow(m_iSelectedIndex);
		}

		if (m_iHoveringRowIndex != ROW_INDEX_NONE) {
			PostMessage(m_hWndParent, WM_ROW_SELECTED, NULL, NULL);
		} else {
			PostMessage(m_hWndParent, WM_ROW_UNSELECTED, NULL, NULL);
		}

		m_iSelectedIndex = m_iHoveringRowIndex;
	}

	return 0;
}

LRESULT ListView::OnSize(WORD newWidth, WORD newHeight)
/*++
* 
* Routine Description:
* 
*	Resizes the scrollbars.
* 
* Arguments:
* 
*	newWidth - The new window width, in pixels.
*	newHeight - The new window height, in pixels.
* 
* Return Value:
* 
*	Win32 API Return Code. Always Zero.
* 
--*/
{
	m_rcSelf.right = m_rcSelf.left + newWidth;
	m_rcSelf.bottom = m_rcSelf.top + newHeight;

	if (m_pRenderTarget)
	{
		render::FitRenderTargetToClient(m_pRenderTarget);
	}

	FitScrollbarsToClient();

	return 0;
}

LRESULT ListView::OnMouseMove(int iCursorX, int iCursorY)
/*++
* 
* Routine Description:
* 
*	Does most of the work related to the resizing of columns.
* 
* Arguments:
* 
*	iCursorX - X Coordinate of the cursor.
*	iCursorY - Y Coordinate of the cursor.
* 
* Return Value:
* 
*	Win32 API Return Code. Always Zero.
* 
--*/
{
	if (!m_isTrackingCursor)
	{
		TRACKMOUSEEVENT tme = {};
		tme.cbSize = sizeof(tme);
		tme.dwFlags = TME_LEAVE;
		tme.hwndTrack = m_hWndSelf;
		tme.dwHoverTime = 0; // doesn't matter
		TrackMouseEvent(&tme);

		m_isTrackingCursor = true;
	}

	// Remove the scroll offset so we can calculate correctly whether the cursor is above
	// a vertical line or not, in order to resize a column
	iCursorX -= m_cxOffset;

	// We'll initially check if the user is resizing a column, because if he/she isn't,
	// then we may move on to the next if statement and check if the cursor is above a line
	if (m_isDragging)
	{
		DoColumnResizeByDragging(iCursorX);
	}

	else if (iCursorY <= static_cast<int>(cyLabelBar))
	{
		HandleLabelBarHovering(iCursorX);
	}

	else
	{
		HandleRowHovering(iCursorY);
	}

	return 0;
}

void ListView::DoColumnResizeByDragging(int iCursorX)
/*++
* 
* Routine Description:
* 
*	Given that the user has clicked on a vertical line to the right of a column
*	and has moved the cursor, this function resizes the column and updates the scrollbar.
* 
* Arguments:
* 
*	iCursorX - The position of the cursor relative to the scroll. This is done because
*	           once we have scrolled the window the lines on screen aren't using absolute
*	           coordinates, but relative, meaning that a line may be at x=100 on screen
*	           but it's actually located at x=200.
* 
* Return Value:
* 
*	None.
* 
--*/
{
	// Obviously if a row is being dragged we're not hovering over any row
	m_iHoveringRowIndex = ROW_INDEX_NONE;

	// And if we're dragging a column then we want to unselect any potential column labels we were hovering
	// Even if the cursor is within a column label rectangle, if we're dragging we don't consider it as hovering
	ResetColumnLabelHovering();

	// This value was chosen pretty much arbitrarly, but the essence is that
	// we want to restrict how small a column can be
	const int iMinimumColumnWidth = static_cast<int>(MINIMUM_COLUMN_WIDTH * util::GetDPIScale(m_hWndSelf));

	// Remember that at this point the column is being dragged, and this function is ONLY called when the
	// cursor has moved, which means we want to redraw the column because it has been resized. If the width
	// however has become less than the minimum width, there is no point to redraw everything
	if (iMinimumColumnWidth <= m_Columns[m_iDraggingIndex].cxWidth)
	{
		// Also, the reason we're redrawing pretty much everything is because almost everything is affected
		// Though we could validate the columns to the left of the resized one, but that's a TODO for later
		InvalidateRect(m_hWndSelf, NULL, TRUE);
		ValidateScrollbarArea();
	}

	// We'll use this in order to calculate the sum of all column widths so that we don't have to recalculate it
	// every single time the user scrolls the window horizontally
	const int iWidthBeforeResize = m_Columns[m_iDraggingIndex].cxWidth;

	// Resize the column, but don't let it get too tiny
	m_Columns[m_iDraggingIndex].cxWidth = max(m_widthBeforeDragging + (iCursorX - m_iDraggingStartX), iMinimumColumnWidth);

	// Now all we have to do is add the difference in size to the sum of widths and we're good to go
	sumOfColumnWidths += static_cast<size_t>(m_Columns[m_iDraggingIndex].cxWidth - iWidthBeforeResize);

	// Finally after resizing the column, we must update the scrollbar thumb position
	UpdateHorizontalScrollbar();
}

void ListView::HandleLabelBarHovering(int iCursorX)
{
	// Since the cursor is over the label bar, the user is obviously not hovering
	// over any row, so the row that we were hovering over may be invalidated 
	InvalidateRow(m_iHoveringRowIndex);
	// We can also let the system know that no row is being hovered
	m_iHoveringRowIndex = ROW_INDEX_NONE;

	const long distanceToTriggerDrag = static_cast<long>(5L * util::GetDPIScale(m_hWndSelf));

	// We will iteratively calculate where each column ends and check if the cursor
	// is close enough to enable the resizing of columns
	int iColumnEndX = 0;

	for (size_t i = 0; i < m_Columns.size(); ++i)
	{
		iColumnEndX += m_Columns[i].cxWidth;

		if (labs(iColumnEndX - iCursorX) <= distanceToTriggerDrag)
		{
			SetCursor(LoadCursor(NULL, IDC_SIZEWE));
			
			m_canDrag = true;
			m_iDraggingIndex = i;
			ResetColumnLabelHovering();
			break;
		}

		// If the column end is after the cursor x, then none of the
		// next column ends will collide with the cursor, therefore
		// the loop may terminate
		else if (iColumnEndX > iCursorX)
		{
			m_canDrag = false;
			m_iHoveredColumnLabel = static_cast<int>(i);
			InvalidateColumnLabelBox(m_iHoveredColumnLabel);
			break;
		}
	}
}

void ListView::InvalidateColumnLabelBox(int index)
{
	if (index >= 0 && index < m_Columns.size())
	{
		const int iColumnEndX = GetRelativeColumnHorizontalPosition(index) + m_Columns[index].cxWidth;

		RECT rcRedrawnLabelBox = {};
		rcRedrawnLabelBox.left = iColumnEndX - m_Columns[index].cxWidth + m_cxOffset;
		rcRedrawnLabelBox.right = iColumnEndX + m_cxOffset;
		rcRedrawnLabelBox.top = 0;
		rcRedrawnLabelBox.bottom = static_cast<int>(cyLabelBar);
		InvalidateRect(m_hWndSelf, &rcRedrawnLabelBox, TRUE);
	}
}

void ListView::HandleRowHovering(int iCursorY)
{
	const int iPrevHovering = m_iHoveringRowIndex;

	// Well, this was an interesting bug.
	// If this line isn't included and the user places the cursor on a column line in the label bar,
	// and moves the cursor straight down, this variable never becomes false so when the user clicks a row
	// the program tries to start dragging a column, and fails, so nothing happens.
	m_canDrag = false;

	// The index of the row the cursor is hovering over
	m_iHoveringRowIndex = static_cast<int>((iCursorY - cyLabelBar - m_cyOffset) / cyRow);

	if (m_iHoveringRowIndex >= static_cast<int>(m_refIndexesOfShownRows->size())) 
	{
		m_iHoveringRowIndex = ROW_INDEX_NONE;
	}

	ResetColumnLabelHovering();

	if (iPrevHovering != m_iHoveringRowIndex)
	{
		if (iPrevHovering != ROW_INDEX_NONE && iPrevHovering < m_refRows->size())
		{
			InvalidateRow(iPrevHovering);
		}

		if (m_iHoveringRowIndex != ROW_INDEX_NONE && m_iHoveringRowIndex < m_refRows->size())
		{
			InvalidateRow(m_iHoveringRowIndex);
		}

		// We have to make this check otherwise because we're in an 'else' statement,
		// this code will be executed the first time the window is drawn, and since
		// we will have validated the scrollbar area the scrollbars won't be drawn.
		if (!m_firstTimeDrawing)
		{
			ValidateScrollbarArea();
		}

		m_firstTimeDrawing = false;
	}
}

LRESULT ListView::OnMouseLeave(void)
{
	m_isTrackingCursor = false;
	m_isDragging = false;
	m_canDrag = false;
	m_iHoveringRowIndex = ROW_INDEX_NONE;
	m_iHoveredColumnLabel = COLUMN_INDEX_NONE;

	InvalidateRect(m_hWndSelf, NULL, TRUE); // TODO: Improve

	return 0;
}

LRESULT ListView::OnLeftMouseUp(WPARAM wParam, LPARAM lParam)
/*++
* 
* Routine Description:
* 
*	Stops resizing the column that may have been being resized.
* 
* Arguments:
* 
*	wParam
*	lParam
* 
* Return Code:
* 
*	Win32 API return code. Always zero.
* 
--*/
{
	if (m_iClickedColumnLabel != COLUMN_INDEX_NONE)
	{
		if (m_iSelectedIndex != ROW_INDEX_NONE)
		{
			PostMessage(m_hWndParent, WM_ROW_UNSELECTED, NULL, NULL);
		}

		SortColumnData(m_iClickedColumnLabel);
		m_iSelectedIndex = ROW_INDEX_NONE;
		m_iClickedColumnLabel = COLUMN_INDEX_NONE;
	}

	m_isDragging = false;
	
	return 0;
}

LRESULT ListView::OnVScroll(WORD scrollType, WORD scrollPosition)
/*++
* 
* Routine Description:
* 
*	Scrolls the window vertically.
*
* Arguments:
* 
*	scrollType - Code showing how the user scrolled, For example: SB_LINEDOWN means the user
*                clicked the down arrow, SB_THUMBTRACK means the user is dragging the thumb, etc.
*
*	scrollPosition - If scrollType is SB_THUMBTRACK, then this is the position of the thumb in the range [0, nMax]
*                    nMax can be retrieved using the GetScrollInfo function.
* 
* Return Value:
* 
*	Win32 Api return code. Always Zero.
* 
--*/
{
	const int iExtraRowsOffscreen = GetExtraRowsOffScreenCount();

	if (iExtraRowsOffscreen > 0)
	{
		int iOldCyOffset = m_cyOffset;

		switch (scrollType)
		{
		case SB_LINEDOWN:
			m_cyOffset = max(-iExtraRowsOffscreen * (int)(cyRow) - 100, m_cyOffset - 10);
			break;

		case SB_LINEUP:
			m_cyOffset = min(0, m_cyOffset + 10);
			break;

		case SB_THUMBTRACK:
			SCROLLINFO si = {};
			si.cbSize = sizeof(SCROLLINFO);
			si.fMask = SIF_RANGE;
			GetScrollInfo(m_hVertSB, SB_CTL, &si);
			m_cyOffset = -static_cast<int>(((double)scrollPosition / si.nMax) * (iExtraRowsOffscreen * cyRow + 100));
			break;
		}

		// We'll redraw everything only if the window was scrolled, otherwise there is no point
		if (iOldCyOffset != m_cyOffset)
		{
			InvalidateRect(m_hWndSelf, NULL, TRUE);
			ValidateScrollbarArea();

			// Since we're scrolling vertically, there is no reason to redraw
			// the label bar because it won't change, so we validate it.
			RECT rcLabelBar = {};
			rcLabelBar.left = 0;
			rcLabelBar.top = 0;
			rcLabelBar.right = GetWidth();
			rcLabelBar.bottom = cyLabelBar + 1;
			ValidateRect(m_hWndSelf, &rcLabelBar);

			UpdateVerticalScrollbar();
		}
	}

	return 0;
}

LRESULT ListView::OnHScroll(WORD scrollType, WORD scrollPosition)
/*++
* 
* Routine Description:
* 
*	Scrolls the list horizontally.
* 
* Arguments:
* 
*	scrollType - Code showing how the user scrolled, For example: SB_LINEDOWN means the user
*                clicked the right arrow, SB_THUMBTRACK means the user is dragging the thumb, etc.
* 
*	scrollPosition - If scrollType is SB_THUMBTRACK, then this is the position of the thumb in the range [0, nMax]
*                    nMax can be retrieved using the GetScrollInfo function.
* 
* Return Value:
* 
*	Win32 API return code. Always Zero.
* 
--*/
{
	const int iOldCxOffset = m_cxOffset;

	switch (scrollType)
	{
	case SB_LINEDOWN:
		m_cxOffset = max(-(int)(sumOfColumnWidths), m_cxOffset - 20);
		break;

	case SB_LINEUP:
		m_cxOffset = min(0, m_cxOffset + 20);
		break;

	case SB_THUMBTRACK:
		SCROLLINFO si = {};
		si.cbSize = sizeof(SCROLLINFO);
		si.fMask = SIF_RANGE;
		GetScrollInfo(m_hHorzSB, SB_CTL, &si);
		m_cxOffset = -static_cast<int>(((double)scrollPosition / si.nMax) * sumOfColumnWidths);
		break;
	}

	if (iOldCxOffset != m_cxOffset)
	{
		InvalidateRect(m_hWndSelf, NULL, TRUE);
		ValidateScrollbarArea();

		UpdateHorizontalScrollbar();
	}

	return 0;
}

LRESULT ListView::OnKeyDown(WPARAM wParam)
{

	switch (wParam)
	{
	case VK_UP:
		if (m_iSelectedIndex == ROW_INDEX_NONE) {
			m_iSelectedIndex = (int)(m_refRows->size()) - 1;
			InvalidateRow(m_iSelectedIndex);
		} else if (m_iSelectedIndex > 0) {
			InvalidateRow(m_iSelectedIndex);
			--m_iSelectedIndex;
			InvalidateRow(m_iSelectedIndex);
		} 
		break;

	case VK_DOWN:
		if (m_iSelectedIndex == ROW_INDEX_NONE) {
			m_iSelectedIndex = 0;
			InvalidateRow(m_iSelectedIndex);
		} else if (m_iSelectedIndex < m_refRows->size() - 1) {
			InvalidateRow(m_iSelectedIndex);
			++m_iSelectedIndex;
			InvalidateRow(m_iSelectedIndex);
		}
		break;
	}

	return 0;
}

void ListView::AddColumn(const wchar_t* lpszColumnName, int cxWidth)
/*++
* 
* Routine Description:
* 
*	Adds a new column with a specified width.
*	The width may be changed later by the user.
* 
* Arguments:
* 
*	lpszColumnName - The name of the label that is displayed above the column.
*	cxWidth - The width of the column in pixelss
* 
* Return Value:
* 
*	None.
* 
--*/
{
	assert(lpszColumnName != nullptr);
	assert(cxWidth > 0);
	
	cxWidth = max(cxWidth, MINIMUM_COLUMN_WIDTH);
	cxWidth = static_cast<int>(cxWidth * util::GetDPIScale(m_hWndSelf));

	sumOfColumnWidths += cxWidth;

	m_Columns.emplace_back(ColumnInfo(std::wstring(lpszColumnName), cxWidth));
	m_NextColumnSortOrder.emplace_back(ListViewNextSort::ASCENDING);

	UpdateHorizontalScrollbar();
}

void ListView::AddRow(std::vector<std::wstring>& info)
{
	m_refIndexesOfShownRows->emplace_back(m_refRows->size());
	m_refRows->emplace_back(std::move(info));

	InvalidateRow(m_refRows->size() - 1);
	UpdateVerticalScrollbar();
}

void ListView::OnDPIChanged(void)
{
	
}

void ListView::UpdateHorizontalScrollbar(void)
/*++
* 
* Routine Description:
* 
*	Updates the horizontal scrollbar's position, maximum distance, width, etc.
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
	SCROLLINFO si = {};
	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
	si.nMin = 0;
	si.nMax = static_cast<int>(m_Columns.size() * 100U);
	si.nPage = m_Columns.size() * 10;
	si.nPos = (int)(-((double)m_cxOffset / (sumOfColumnWidths)) * si.nMax);
	SetScrollInfo(m_hHorzSB, SB_CTL, &si, TRUE);
}

void ListView::UpdateVerticalScrollbar(void)
/*++
*
* Routine Description:
*
*	Updates the vertical scrollbar's position, maximum distance, width, etc.
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
	assert(cyRow != 0);

	const int iExtraRowsOffScreen = GetExtraRowsOffScreenCount();

	SCROLLINFO si = {};
	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
	si.nMin = 0;
	// The 100 factor is pretty arbtitrary, it just works, can be changedd though
	si.nMax = iExtraRowsOffScreen * 100;
	si.nPage = iExtraRowsOffScreen * 10 + 1;
	// The + 100 is there to give a little extra space after scrolling all the way down
	si.nPos = (iExtraRowsOffScreen != 0) ? -(int)((m_cyOffset / (double)(iExtraRowsOffScreen * cyRow + 100)) * si.nMax) : 0;
	SetScrollInfo(m_hVertSB, SB_CTL, &si, TRUE);
}

int ListView::GetExtraRowsOffScreenCount(void)
/*++
* 
* Routine Description:
* 
*	Calculates how many rows of data aren't displayed on screen.
*	This is used in order to determine how much the window can
*	be scrolled vertically by the scrollbar.
* 
* Arguments:
* 
*	None.
* 
* Return Value:
* 
*	See Routine Description.
* 
--*/
{
	// Height - labelBarHeight = Height of area that the rows are displayed
	// Divide by the height of the rows and we have an answer of how many rows fit on the screen
	const int iRowCountFitOnScreen = static_cast<int>(((int)(GetHeight()) - (int)(cyLabelBar)) / (int)cyRow);

	// Now we simply remove the number of rows that can be displayed from the total number of rows
	// If the answer is negative we return zero, since there are no rows offscreen in this case.
	return max(0, static_cast<int>(m_refIndexesOfShownRows->size()) - iRowCountFitOnScreen);
}

void ListView::ValidateScrollbarArea(void)
/*++
* 
* Routine Description:
* 
*	Validates the rectangles of the scrollbars because there is no reason to redraw them
*	upon redrawing the rest of the window.
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
	// This was written before switching to Direct2D, there may not be a point in doing this anymore however I'll
	// leave it here for the time being and check it out later, TODO

	RECT rcVerticalScrollbar = {};
	rcVerticalScrollbar.left = (LONG)(GetWidth()) - cxScrollbarWidth;
	rcVerticalScrollbar.right = rcVerticalScrollbar.left + cxScrollbarWidth + 1;
	rcVerticalScrollbar.top = cyLabelBar;
	rcVerticalScrollbar.bottom = (LONG)(GetHeight()) - cxScrollbarWidth;
	ValidateRect(m_hWndSelf, &rcVerticalScrollbar);

	RECT rcHorizontalScrollbar = {};
	rcHorizontalScrollbar.left = 0;
	rcHorizontalScrollbar.right = (LONG)(GetWidth());
	rcHorizontalScrollbar.top = (LONG)(GetHeight()) - cxScrollbarWidth;
	rcHorizontalScrollbar.bottom = rcHorizontalScrollbar.top + cxScrollbarWidth + 1;
	ValidateRect(m_hWndSelf, &rcHorizontalScrollbar);
}

void ListView::InvalidateRow(int index)
/*++
* 
* Routine Description:
* 
*	Invalidates the rectangle of the specified row.
* 
* Arguments:
* 
*	The index of the row to be invalidated, starting from 0.
* 
* Return Value:
* 
*	None.
* 
--*/
{
	if (index >= 0 && index <= m_refIndexesOfShownRows->size())
	{
		RECT rcCurrentHover = {};
		rcCurrentHover.left = 1;
		rcCurrentHover.right = GetWidth() - cxScrollbarWidth - 1;
		rcCurrentHover.top = cyLabelBar + index * cyRow + m_cyOffset;
		rcCurrentHover.bottom = rcCurrentHover.top + cyRow;
		InvalidateRect(m_hWndSelf, &rcCurrentHover, TRUE);
	}
}

void ListView::SetColorRule(const std::wstring& word, COLORREF cr, int column)
/*++
* 
* Routine Description:
* 
*	Sets a color rule, meaning a color that a word should be colored in when drawn.
* 
* Arguments:
* 
*	word - The word (or string) that will be colored.
*	cr - The color that will be used
* 
* Return Value:
* 
*	None.
* 
--*/
{
	ColorRule rule;
	rule.columnAffected = column;
	rule.cr = cr;

	if (m_WordColoring.find(word) == m_WordColoring.end())
	{
		
		m_WordColoring.insert(std::make_pair(word, rule));
	}

	else
	{
		m_WordColoring[word] = rule;
	}
}

COLORREF ListView::GetWordColor(const std::wstring& word, int column)
/*++
* 
* Routine Description:
* 
*	Finds the color of a word based on a set rule.
* 
* Arguments:
* 
*	word - The word whose color will be returned.
* 
* Return Value:
* 
*	The color of the word.
* 
--*/
{
	if (m_WordColoring.find(word) == m_WordColoring.end())
	{
		return RGB(0, 0, 0);
	}

	if (m_WordColoring[word].columnAffected == ALL_COLUMNS || m_WordColoring[word].columnAffected == column)
	{
		return m_WordColoring[word].cr;
	}

	return RGB(0, 0, 0);
}

void ListView::ApplyRowFilter(const std::wstring& filter_word)
/*++
* 
* Routine Description:
* 
*	Displays the rows which match at least one of the following criteria:
* 
*		- At least one of the row's cell's content is contained in the filter_word
*	 or - Vice versa
* 
* Arguments:
* 
*	filter_word - Word used to filter out rows.
* 
* Return Value:
* 
*	None.
* 
--*/
{
	m_refIndexesOfShownRows->clear();

	for (size_t i = 0; i < m_refRows->size(); ++i)
	{
		for (std::wstring& entry : (*m_refRows)[i])
		{
			if (filter_word.empty() 
				|| StrStrNIW(entry.c_str(), filter_word.c_str(), entry.length()))
			{
				m_refIndexesOfShownRows->emplace_back(i);
				break;
			}
		}
	}

	UpdateVerticalScrollbar();
	
	InvalidateRect(m_hWndSelf, NULL, TRUE);
	m_iHoveredColumnLabel = COLUMN_INDEX_NONE;
	m_iClickedColumnLabel = COLUMN_INDEX_NONE;

	UnselectSelectedRow();
}

void ListView::SortColumnData(int index)
/*++
* 
* Routine Description:
* 
*	Sorts the rows of the list alphabetically, using the values of the specified row.
* 
* Arguments:
* 
*	index - Index of column in m_Columns.
* 
--*/
{
	if (index < 0 || index >= m_Columns.size())
	{
		return;
	}

	if (m_NextColumnSortOrder[index] == ListViewNextSort::ASCENDING) 
	{
		std::sort(m_refIndexesOfShownRows->begin(), m_refIndexesOfShownRows->end(), [&](int first, int second) {
			return (*m_refRows)[first][index] < (*m_refRows)[second][index];
		});
		m_NextColumnSortOrder[index] = ListViewNextSort::DESCENDING;
	}

	else 
	{
		std::sort(m_refIndexesOfShownRows->begin(), m_refIndexesOfShownRows->end(), [&](int first, int second) {
			return (*m_refRows)[first][index] > (*m_refRows)[second][index];
		});
		m_NextColumnSortOrder[index] = ListViewNextSort::ASCENDING;
	}

	RECT rcRows = {};
	rcRows.top = cyLabelBar;
	rcRows.bottom = (int)(GetHeight() - cxScrollbarWidth);
	rcRows.left = 0;
	rcRows.right = (int)(GetWidth() - cxScrollbarWidth);
	InvalidateRect(m_hWndSelf, &rcRows, TRUE);
}

int ListView::GetRelativeColumnHorizontalPosition(int index)
/*++
* 
* Routine Description:
* 
*	Relative means relative to the actual x=0 of the window, doesn't matter
*	if the windo whas been scrolled. Result is in pixels
* 
* Arguments:
* 
*	index - Index of the column in m_Columns.
* 
--*/
{
	assert(index < m_Columns.size());

	const int column_count = static_cast<int>(m_Columns.size());

	int iColumnPositionX = 0;

	for (int i = 1; i <= index; ++i)
	{
		iColumnPositionX += m_Columns[i - 1].cxWidth;
	}

	return iColumnPositionX;
}

void ListView::ResetColumnLabelHovering(void)
/*++
* 
* Routine Description:
* 
*	Declares that no column label is being hovered, or is clicked, and redraws it.
* 
* Arguments:
* 
*	None.
* 
--*/
{
	if (m_iHoveredColumnLabel != COLUMN_INDEX_NONE)
	{
		InvalidateColumnLabelBox(m_iHoveredColumnLabel);

		m_iHoveredColumnLabel = COLUMN_INDEX_NONE;
		m_iClickedColumnLabel = COLUMN_INDEX_NONE;
	}
}

std::vector<std::wstring> ListView::GetSelectedRow(void)
{
	if (m_iSelectedIndex == ROW_INDEX_NONE)
	{
		throw std::runtime_error("No row is selected");
	}

	return (*m_refRows)[(*m_refIndexesOfShownRows)[m_iSelectedIndex]];
}

bool ListView::IsSomeRowSelected(void)
{
	return m_iSelectedIndex >= 0 && m_iSelectedIndex < m_refIndexesOfShownRows->size();
}

void ListView::SetDisplayedRowContent(int row, const std::vector<std::wstring>& newData)
/*++
* 
* Routine Description:
* 
*	Replaces the content of the vector at the given row with the new data.
* 
* Arguments:
* 
*	row - Row of the displayed row, not in memory.
*	newData - Vector containing the new data. Doesn't need to have the same size as the other one.
* 
--*/
{
	size_t uInsertedDataSize;
	size_t uOldDataSize;

	if (row >= 0 && row < static_cast<int>(m_refIndexesOfShownRows->size()))
	{
		uInsertedDataSize = newData.size();
		uOldDataSize = (*m_refRows)[(*m_refIndexesOfShownRows)[row]].size();

		for (size_t i = 0; i < min(uInsertedDataSize, uOldDataSize); ++i)
		{
			(*m_refRows)[(*m_refIndexesOfShownRows)[row]][i] = newData[i];
		}

		InvalidateRow(row);
	}
}

void ListView::SetCellContent(int row, int column, const std::wstring& content)
/*++
* 
* Routine Description:
* 
*	Sets the content of a cell on screen.
* 
* Arguments:
* 
*	row    - Row of the cell.
*	column - Column of the cell.
* 
--*/
{
	if (IsValidCellPosition(row, column))
	{
		(*m_refRows)[(*m_refIndexesOfShownRows)[row]][column] = content;
		InvalidateRow(row);
	}
}

std::wstring ListView::GetCellContent(int row, int column)
/*++
* 
* Routine Description:
* 
*	Retrieves the string saved at a specified cell.
* 
*	It goes without saying that the cell must be displayed on screen.
* 
* Arguments:
* 
*	row    - Row of the cell.
*	column - Column of the cell.
* 
--*/
{
	if (IsValidCellPosition(row, column))
	{
		return (*m_refRows)[(*m_refIndexesOfShownRows)[row]][column];
	}

	return L"";
}

bool ListView::IsValidCellPosition(int row, int column)
/*++
* 
* Routine Description:
* 
*	Checks whether the index vector contains something at [row][column], or if it sout of bounds.
* 
*	Used as a helper function when working with cells.
* 
--*/
{
	assert(m_refIndexesOfShownRows);
	assert(m_refRows);

	if (row >= 0 && row < static_cast<int>(m_refIndexesOfShownRows->size()))
	{
		if (column >= 0 && column < static_cast<int>((*m_refRows)[(*m_refIndexesOfShownRows)[row]].size()))
		{
			return true;
		}
	}

	return false;
}

void ListView::RemoveDisplayedRow(int index)
/*++
* 
* Routine Description:
* 
*	Deletes the row at the given index.
* 
* Arguments:
* 
*	index - Index of the displayed row, starting from 0.
* 
* Return Value:
* 
*	None.
* 
--*/
{
	const int iIndexesVectorSize = static_cast<int>(m_refIndexesOfShownRows->size());

	if (index >= 0 && index < iIndexesVectorSize)
	{
		const int iIndexOfRemovedRow = (*m_refIndexesOfShownRows)[index];

		// Let's suppose for example's sake that we're going to remove the row at index = 1.
		// Let's also suppose that there are 4 rows in total in the vector.
		// Once we remove the row at index 1 in the m_Rows array, all the entries in
		// the m_IndexesOfShownRows vector after index = 1, are incorrect, because they're
		// pointing to the row AFTER the one they're supposed to be pointing at
		//
		//                                Before:
		// 
		//     m_IndexesOfShownRows                            m_Rows
		//              1 --------------------|      |----------> x
		//              2 -----------------|  |------|----------> y
		//              0 -----------------|---------|   |------> z
		//              3 -------|         |-------------|   |--> w
		//                       |---------------------------|
		//
		//
		//                                 After:
		//     m_IndexesOfShownRows                            m_Rows
		//              1 --------------------|      |----------> x
		//                                    |------|----------> y
		//              0 ---------------------------|            w
		//              3 --------------------------------------> -
		//
		// As we can see, the last element is now pointing one position after the one it's
		// supposed, to be, therefore we must decrement it by one, and all the ones that
		// could potentially come after it
		for (int i = 0; i < iIndexesVectorSize; ++i)
		{
			// Also make sure that if it's the last element in the array, we don't go off bounds
			if ((*m_refIndexesOfShownRows)[i] > (*m_refIndexesOfShownRows)[index])
			{
				--((*m_refIndexesOfShownRows)[i]);
			}

			// In addition to decrementing, we also need to redraw every row from and after the row we're deleting.
			if (i >= index)
			{
				InvalidateRow(i);
			}
		}

		// Now we delete the row at the index we saved at the beginning, because
		// the data may have been altered in the previous loop.
		m_refRows->erase(m_refRows->begin() + iIndexOfRemovedRow);

		// And finally we remove the row from display.
		m_refIndexesOfShownRows->erase(m_refIndexesOfShownRows->begin() + index);

		// If the row we just deleted was the final one, we must unselect the row
		if (m_refIndexesOfShownRows->empty())
		{
			UnselectSelectedRow();
		}

		// If it wasn't the final one in the list, but it was the bottom one, we select the previous one
		else if (m_iSelectedIndex == iIndexesVectorSize - 1) 
		{
			PostMessage(m_hWndParent, WM_ROW_SELECTED, NULL, NULL);

			--m_iSelectedIndex;
		}
	}
}

void ListView::RemoveRow(int index)
/*++
* 
* Routine Description:
* 
*	Removes a row stored internally, meaning it might not be necessarily displayed.
* 
*	The way we're going to handle this is, we'll check if the index vector contains an
*	entry that points to the row that we want to remove; if it does, great! We can call
*	the RemoveDisplayedRow function to do the work for us. If it doesn't exist... well,
*	then we'll add it to the index vector and call the same function.
* 
* Arguments:
* 
*	index - Index of row in the internal structure of the list
* 
--*/
{
	// One might ask, why wouldn't RemoveDisplayedRow call RemoveRow instead of vice versa?
	// Simple answer: I wrote RemoveDisplayedRow before RemoveRow, and since the other algorithm
	// works I don't feel like doing it all over again

	assert(m_refIndexesOfShownRows);

	const size_t indexSize = m_refIndexesOfShownRows->size();

	for (size_t i = 0; i < indexSize; ++i)
	{
		if ((*m_refIndexesOfShownRows)[i] == index)
		{
			RemoveDisplayedRow(i);
			return;
		}
	}

	m_refIndexesOfShownRows->emplace_back(index);

	// Remember that index size was initialized before adding the last element
	// so it shoud be the index of the last element after calling emplace_back.
	RemoveDisplayedRow(static_cast<int>(indexSize));
}

void ListView::UnselectSelectedRow(void)
{
	if (m_iSelectedIndex != ROW_INDEX_NONE)
	{
		PostMessage(m_hWndParent, WM_ROW_UNSELECTED, NULL, NULL);
		InvalidateRow(m_iSelectedIndex);
		m_iSelectedIndex = ROW_INDEX_NONE;
	}
}

// As of right now if a column is added after calling this function,
// The column vectors will be out of sync between the two objects,
// however since I doubt it will ever happen, I won't bother making
// the column vector a pointer as well
void ListView::Mirror(ListView* pList)
/*++
* 
* Routine Description:
* 
*	Makes the ListView display the content of an already existing list view.
*	
* Arguments:
* 
*	pList - Pointer to the list that will be mirrored.
* 
--*/
{
	assert(pList);

	// Clear any rows that are stored internally because they won't be displayed after mirroring
	// Note that we shouldn't call the Clear() function because that clears the mirrored content,
	// And if the list view is already mirroring another listview we don't want to clear it
	m_IndexesOfShownRows.clear();
	m_Rows.clear();

	m_refIndexesOfShownRows = &pList->m_IndexesOfShownRows;
	m_refRows               = &pList->m_Rows;

	// The right thing would be to use pointers for all these as well but I can't be bothered right now. TODO?
	m_Columns             = pList->m_Columns;
	m_NextColumnSortOrder = pList->m_NextColumnSortOrder;
	sumOfColumnWidths     = pList->sumOfColumnWidths;

	UpdateHorizontalScrollbar();
	UpdateVerticalScrollbar();
}

void ListView::Clear(void)
{
	if (!m_refRows->empty())
	{
		m_refIndexesOfShownRows->clear();
		m_refRows->clear();

		InvalidateRect(m_hWndSelf, NULL, FALSE);
		ValidateScrollbarArea();
		UpdateVerticalScrollbar();
	}
}

void ListView::FilterOutColumnContent(int iColIndex, const std::wstring& filter_word)
{
	assert(iColIndex >= 0 && iColIndex < m_Columns.size());

	for (size_t i = 0; i < columnFilters.size(); ++i)
	{
		if (columnFilters[i].filter_word == filter_word)
		{
			return;
		}
	}

	ColumnFilter cFilter;
	cFilter.filter_word = filter_word;
	cFilter.iColumnIndex = iColIndex;
	columnFilters.emplace_back(cFilter);
}

bool ListView::RowObeysToColumnFilters(int iRowIndex)
{
	if (iRowIndex >= 0 && iRowIndex < m_refIndexesOfShownRows->size())
	{
		const std::vector<std::wstring> row = (*m_refRows)[(*m_refIndexesOfShownRows)[iRowIndex]];

		for (ColumnFilter& filter : columnFilters)
		{
			if (row[filter.iColumnIndex] == filter.filter_word)
			{
				return false;
			}
		}
	}

	return true;
}