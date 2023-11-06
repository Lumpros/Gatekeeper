#include "Window.h"

Window::Window(HWND hParentWindow, Size size, Point point)
{
	m_hWndParent = hParentWindow;

	m_rcSelf.left = point.x;
	m_rcSelf.top = point.y;
	m_rcSelf.right = m_rcSelf.left + size.width;
	m_rcSelf.bottom = m_rcSelf.top + size.height;
}

Size Window::GetSize(void)
{
	Size size = {};
	size.width = m_rcSelf.right - m_rcSelf.left;
	size.height = m_rcSelf.bottom - m_rcSelf.top;
	return size;
}

Point Window::GetPoint(void)
{
	return Point(m_rcSelf.left, m_rcSelf.top);
}

size_t Window::GetWidth(void) const
{
	return m_rcSelf.right - m_rcSelf.left;
}

size_t Window::GetHeight(void) const
{
	return m_rcSelf.bottom - m_rcSelf.top;
}

HWND Window::GetHandle(void) const
{
	return m_hWndSelf;
}

RECT Window::GetRect(void) const
{
	return m_rcSelf;
}

void Window::SetSize(int cx, int cy)
{
	m_rcSelf.right = m_rcSelf.left + cx;
	m_rcSelf.bottom = m_rcSelf.top + cy;

	SetWindowPos(m_hWndSelf,
		nullptr,
		NULL,
		NULL,
		cx,
		cy,
		SWP_NOZORDER | SWP_NOMOVE
	);
}

void Window::SetPos(int x, int y)
{
	m_rcSelf.right += (x - m_rcSelf.left);
	m_rcSelf.bottom += (y - m_rcSelf.top);
	m_rcSelf.left = x;
	m_rcSelf.top = y;

	SetWindowPos(m_hWndSelf,
		nullptr,
		x,
		y,
		NULL,
		NULL,
		SWP_NOZORDER | SWP_NOSIZE
	);
}

void Window::Show(void)
{
	ShowWindow(m_hWndSelf, SW_SHOW);
}

void Window::Hide(void)
{
	ShowWindow(m_hWndSelf, SW_HIDE);
}

bool Window::IsVisible(void) const
{
	return GetWindowLong(m_hWndSelf, GWL_STYLE) & WS_VISIBLE;
}