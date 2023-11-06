#pragma once

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>

struct Size
{
	size_t width = 0;
	size_t height = 0;

	Size(void) = default;
	Size(size_t width, size_t height)
		: width(width), height(height) {}
};

struct Point
{
	int x;
	int y;

	Point(void) = default;
	Point(int x, int y)
		: x(x), y(y) {}
};

class Window
{
public:
	Window(HWND hParentWindow, Size size, Point point);

	HWND GetHandle(void) const;
	RECT GetRect(void) const;

	Size GetSize(void);
	Point GetPoint(void);

	size_t GetWidth(void) const;
	size_t GetHeight(void) const;

	int GetX(void) { return m_rcSelf.left; }
	int GetY(void) { return m_rcSelf.top; }

	void SetSize(int cx, int cy);
	void SetPos(int x, int y);

	void Show(void);
	void Hide(void);

	bool IsVisible(void) const;

protected:
	HWND m_hWndSelf = nullptr;
	HWND m_hWndParent = nullptr;
	RECT m_rcSelf = { 0 };
};

