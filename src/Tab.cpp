#include "Tab.h"
#include "TabManager.h"
#include "Utility.h"
#include "Renderer.h"

#include <cassert>
#include <stdexcept>

LRESULT CALLBACK TabProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	Tab* pTab;

	switch (uMsg)
	{
	case WM_CREATE:
		util::RegisterObject(hWnd, ((LPCREATESTRUCT)lParam)->lpCreateParams);
		break;

	default:
		if (pTab = (Tab*)util::GetRegisteredObject(hWnd))
		{
			return pTab->WindowProcedure(hWnd, uMsg, wParam, lParam);
		}
		break;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

Tab::Tab(TabManager* pTabManager, const std::wstring& name)
	: Window(pTabManager->GetHandle(), Size(0, 0), Point(0, 0))
{
	HINSTANCE hInstance = GetModuleHandle(NULL);
	m_pTabManager = pTabManager;
	m_name = name;

	RegisterTabClass(hInstance);
	InitializeTabWindow(hInstance);

	m_pRenderTarget = render::CreateWindowRenderTarget(m_hWndSelf);
	m_pGDIRT = render::GetInteropGDIRenderTarget(m_pRenderTarget);
	m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(BACKGROUND_COLOR), &m_pSolidColorBrush);

	hBackgroundBrush = CreateSolidBrush(RGB(248, 248, 252));

	m_pTabManager->AppendTab(this);
	FitToTabManager();
}

Tab::~Tab(void)
{
	SAFE_RELEASE_D2D(m_pRenderTarget);
	SAFE_RELEASE_D2D(m_pGDIRT);
	SAFE_RELEASE_D2D(m_pSolidColorBrush);

	SAFE_DELETE_GDIOBJ(hBackgroundBrush);
}

void Tab::RegisterTabClass(HINSTANCE hInstance)
/*++
*
* Routine Description:
*
*	Registers the tab class, if it has not been registered yet.
*
* Arguments:
*
*	hInstance - Handle to the program instance.
*
* Return Value:
*
*	None.
*
--*/
{
	static bool hasBeenRegistered = false;

	if (!hasBeenRegistered)
	{
		WNDCLASSEX wcex = { 0 };
		wcex.cbSize = sizeof(WNDCLASSEX);
		wcex.hInstance = hInstance;
		wcex.lpszClassName = L"TabClass";
		wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
		wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
		wcex.lpfnWndProc = TabProcedure;
		wcex.style = CS_HREDRAW | CS_VREDRAW;

		if (!RegisterClassEx(&wcex))
		{
			throw std::runtime_error("Unable to register the TabManager window class");
		}

		hasBeenRegistered = true;
	}
}

void Tab::InitializeTabWindow(HINSTANCE hInstance)
/*++
*
* Routine Description:
*
*	Creates the tab window.
*
* Arguments:
*
*	hInstance - Handle to the program instance.
*
* Return Value:
*
*	None.
*
--*/
{
	m_hWndSelf = CreateWindowEx(
		WS_EX_CONTROLPARENT,
		L"TabClass",
		L"",
		WS_CHILD,
		static_cast<int>(GetX()),
		static_cast<int>(GetY()),
		static_cast<int>(GetWidth()),
		static_cast<int>(GetHeight()),
		m_hWndParent,
		NULL,
		hInstance,
		this
	);

	if (!m_hWndSelf)
	{
		throw std::runtime_error("Unable to create tab");
	}
}

void Tab::SetName(const std::wstring& name) noexcept
{
	m_name = name;
}

const std::wstring& Tab::GetName(void) const noexcept
{
	return m_name;
}

void Tab::FitToTabManager(void) noexcept
/*++
*
* Routine Description:
*
*	Sets the tab size such that it fills the content of the tab manager.
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
	SetSize(
		static_cast<int>(m_pTabManager->GetWidth()),
		static_cast<int>(m_pTabManager->GetHeight() - m_pTabManager->GetTabShapeHeight() - 1)
	);

	SetPos(
		0,
		m_pTabManager->GetTabShapeHeight() + 1
	);
}

LRESULT Tab::WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_ERASEBKGND:
		return FALSE;

	case WM_SIZE:
		if (m_pRenderTarget)
			render::FitRenderTargetToClient(m_pRenderTarget);
		OnResize(LOWORD(lParam), HIWORD(lParam));
		return 0;

	case WM_PAINT:
		return OnPaint(hWnd);

	case WM_COMMAND:
		OnCommand(reinterpret_cast<HWND>(lParam));
		return 0;

	case WM_CTLCOLORSTATIC:
		wchar_t buf[32];
		GetClassName((HWND)lParam, buf, 32);
		if (!lstrcmpW(buf, L"Edit"))
		{
			return (LRESULT)GetStockObject(WHITE_BRUSH);
		}
		SetBkMode((HDC)wParam, TRANSPARENT);
		return (LRESULT)hBackgroundBrush;
	}

	if (uMsg > WM_APP)
	{
		return OnCustomMessage(uMsg, wParam, lParam);
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

LRESULT Tab::OnPaint(HWND hWnd)
{
	PAINTSTRUCT ps;
	HDC hDC;
	HRESULT hr;

	if (m_pRenderTarget && m_pGDIRT)
	{
		BeginPaint(hWnd, &ps);
		m_pRenderTarget->BeginDraw();

		m_pRenderTarget->FillRectangle(D2D1::RectF(0, 0, GetWidth(), GetHeight()), m_pSolidColorBrush);

		Draw(m_pRenderTarget);

		hr = m_pGDIRT->GetDC(D2D1_DC_INITIALIZE_MODE_COPY, &hDC);

		if (SUCCEEDED(hr))
		{
			Draw(hDC);

			m_pGDIRT->ReleaseDC(NULL);
		}

		m_pRenderTarget->EndDraw();
		EndPaint(hWnd, &ps);
	}

	return 0;
}

ID2D1SolidColorBrush* Tab::GetSolidColorBrush(void) const noexcept
{
	return m_pSolidColorBrush;
}