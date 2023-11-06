#pragma once

#include <d2d1.h>
#include <dwrite.h>

#ifndef SAFE_RELEASE_D2D
#define SAFE_RELEASE_D2D(p) if (p) { (p)->Release(); }
#endif

namespace render
{
	void InitializeDirect2D(void);
	void UninitializeDirect2D(void);

	ID2D1HwndRenderTarget* CreateWindowRenderTarget(HWND hWnd);
	ID2D1GdiInteropRenderTarget* GetInteropGDIRenderTarget(ID2D1RenderTarget* pRenderTarget);
	IDWriteTextFormat* CreateTextFormat(LPCWSTR lpszFontName, float size, LPCWSTR lpszLocale);
	ID2D1PathGeometry* CreatePathGeometry(void);

	void FitRenderTargetToClient(ID2D1HwndRenderTarget* pRenderTarget);
}
