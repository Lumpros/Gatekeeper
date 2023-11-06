#include "Renderer.h"
#include "Utility.h"

#include <stdexcept>
#include <cassert>

#define SAFE_RELEASE(ptr) if (ptr) { (ptr)->Release(); }

ID2D1Factory* g_pD2DFactory = nullptr;
IDWriteFactory* g_pWriteFactory = nullptr;

void render::InitializeDirect2D(void)
/*++
* 
* Routine Description:
* 
*   Initializes everything necessary to use Direct2D.
* 
* Arguments:
* 
*   None.
* 
* Return Value:
* 
*   None.
* 
--*/
{
    HRESULT hr;

    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &g_pD2DFactory);

    if (FAILED(hr))
    {
        throw std::runtime_error("Unable to create Direct2D Factory");
    }

    hr = DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(&g_pWriteFactory)
    );

    if (FAILED(hr))
    {
        throw std::runtime_error("Unable to create Write Factory");
    }
}

void render::UninitializeDirect2D(void)
/*++
* 
* Routine Description:
* 
*   Uninitializes everything that was initialized during the call to InitializeDirect2D
* 
* Arguments:
* 
*   None.
* 
* Return Value:
* 
*   none.
* 
--*/
{
    SAFE_RELEASE(g_pD2DFactory);
    SAFE_RELEASE(g_pWriteFactory);
}

ID2D1HwndRenderTarget* render::CreateWindowRenderTarget(HWND hWnd)
/*++
* 
* Routine Description:
* 
*   Creates a Direct2D window render target.
* 
* Arguments:
* 
*   hWnd - Handle to the window.
* 
* Return Value:
* 
*   The created render target.
* 
--*/
{
    ID2D1HwndRenderTarget* pRenderTarget = nullptr;
    D2D1_SIZE_U uWindowSize;
    D2D1_RENDER_TARGET_PROPERTIES rtProps;
    RECT rcClient;

    if (g_pD2DFactory)
    {
        GetClientRect(hWnd, &rcClient);

        uWindowSize = D2D1::SizeU(
            rcClient.right - rcClient.left,
            rcClient.bottom - rcClient.top
        );

        // Since we scale the size of everything in our windows in order to match
        // the DPI scaling in GDI, we will use 96.0F for DPI because the Direct2D
        // functions do the scaling automatically. Meaning for example if x = 20
        // when DPI = 96.0 but x = 25 when DPI = 120. for example, if we call
        // Draw(x) after we've scaled x to 25, x will be scaled again by the Direct2D
        // function and will be therefore displayed incorrectly.
        rtProps = D2D1::RenderTargetProperties(
            D2D1_RENDER_TARGET_TYPE_DEFAULT,
            D2D1::PixelFormat(
                DXGI_FORMAT_B8G8R8A8_UNORM,
                D2D1_ALPHA_MODE_IGNORE),
            96.0F,
            96.0F,
            D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE, // Must be gdi compatible to create InteropGDIRt
            D2D1_FEATURE_LEVEL_DEFAULT
        );

        HRESULT hr = g_pD2DFactory->CreateHwndRenderTarget(
            rtProps,
            D2D1::HwndRenderTargetProperties(hWnd, uWindowSize),
            &pRenderTarget
        );
    }

    return pRenderTarget;
}

ID2D1GdiInteropRenderTarget* render::GetInteropGDIRenderTarget(ID2D1RenderTarget* pRenderTarget)
/*++
* 
* Routine Description:
* 
*   Creates a GDI Interop Render Target similar to the given render target.
* 
* Arguments:
* 
*   pRenderTarget - See Routine Description:
* 
* Return Value:
* 
*   The created GDI Interop render target.
* 
--*/
{
    ID2D1GdiInteropRenderTarget* pGDIRT = nullptr;

    if (pRenderTarget)
    {
        pRenderTarget->QueryInterface(__uuidof(ID2D1GdiInteropRenderTarget), (void**)&pGDIRT);
    }

    return pGDIRT;
}

IDWriteTextFormat* render::CreateTextFormat(LPCWSTR lpszFontName, float size, LPCWSTR lpszLocale)
{
    IDWriteTextFormat* pTextFormat = nullptr;

    if (g_pWriteFactory)
    {
        g_pWriteFactory->CreateTextFormat(
            lpszFontName,
            NULL,
            DWRITE_FONT_WEIGHT_REGULAR,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            size,
            lpszLocale,
            &pTextFormat
        );
    }

    return pTextFormat;
}

void render::FitRenderTargetToClient(ID2D1HwndRenderTarget* pRenderTarget)
/*++
* 
* Routine Description:
* 
*   Resizes the render target to fit its window client size.
*   This is done because if a window is resized without resizing
*   its render target, drawing will be distorted/stretched.
* 
* Arguments:
* 
*   pRenderTarget - The forementioned render target.
* 
* Return Value:
* 
*   None.
* 
--*/
{
    assert(pRenderTarget);

    RECT rcClient;
    GetClientRect(pRenderTarget->GetHwnd(), &rcClient);

    D2D1_SIZE_U uWindowSize = D2D1::SizeU(
        rcClient.right - rcClient.left,
        rcClient.bottom - rcClient.top
    );

    pRenderTarget->Resize(uWindowSize);
}

ID2D1PathGeometry* render::CreatePathGeometry(void)
{
    ID2D1PathGeometry* pPathGeometry = nullptr;

    if (g_pD2DFactory)
    {
        g_pD2DFactory->CreatePathGeometry(&pPathGeometry);
    }

    return pPathGeometry;
}