#pragma once

#include "Window.h"
#include "Utility.h"

#include <string>
#include <vector>
#include <d2d1.h>

#define BACKGROUND_COLOR RGB_D2D(248, 248, 252)
//#define BACKGROUND_COLOR RGB_D2D(242, 242, 252)

#define MAX_FIRSTNAME_LENGTH 64
#define MAX_LASTNAME_LENGTH  32
#define MAX_NOTES_LENGTH     128

class TabManager;

class Tab : public Window
{
	friend class TabManager;

public:
	Tab(TabManager* pTabManager, const std::wstring& name);
	virtual ~Tab(void);

	void SetName(const std::wstring& name) noexcept;
	const std::wstring& GetName(void) const noexcept;

	void FitToTabManager(void) noexcept;

	LRESULT WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

protected:
	// These functions MUST NOT be pure virtual.
	// At least, the on resize function shouldn't.
	// The issue is that the way the windows API works, OnResize
	// may be called when the object is still being constructed,
	// and will therefore cause an error, so we must give them a function body
	// Also, because of this we must call InitContent from the constructor
	// of the derived class, otherwise it won't be called, ever.
	virtual void OnResize(int width, int height) {}
	virtual void OnCommand(HWND hWnd) {}
	virtual LRESULT OnCustomMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) { return 0; }
	virtual void Draw(ID2D1RenderTarget* pRenderTarget) {}
	virtual void Draw(HDC hDC) {}

	virtual void OnSwitchedToOther(void) {}
	virtual void OnSwitchedToSelf(void) {}

	ID2D1SolidColorBrush* GetSolidColorBrush(void) const noexcept;

protected:
	ID2D1HwndRenderTarget* m_pRenderTarget = nullptr;
	ID2D1GdiInteropRenderTarget* m_pGDIRT = nullptr;
	ID2D1SolidColorBrush* m_pSolidColorBrush = nullptr;

private:
	void RegisterTabClass(HINSTANCE hInstance);
	void InitializeTabWindow(HINSTANCE hInstance);

	LRESULT OnPaint(HWND hWnd);

private:
	// The name of the tab which is also displayed at the top of the screen
	// We'll leave it as an empty string in case it is used before initialization
	std::wstring m_name = L"";

	HBRUSH hBackgroundBrush = NULL;
	TabManager* m_pTabManager = nullptr;
};