#include "AppWindow.h"
#include "Database.h"
#include "Renderer.h"

#include <stdexcept>
#include <CommCtrl.h>

#pragma comment(lib, "ComCtl32.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "Dwrite.lib")
#pragma comment(lib, "Shlwapi.lib")

#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' " \
	"version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

HANDLE g_hSingleInstanceMutex = NULL;

void BringAlreadyRunningInstanceToTop(void)
{
    HWND hExistingWindow = FindWindow(L"GatekeeperWindowClass", L"Gatekeeper");

    if (hExistingWindow)
    {
        ShowWindow(hExistingWindow, SW_NORMAL);
        SetForegroundWindow(hExistingWindow);
    }
}

void Initialize(void)
{
    g_hSingleInstanceMutex = CreateMutex(0, TRUE, L"com.sportsvillage.gatekeeper");

    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        BringAlreadyRunningInstanceToTop();
        ExitProcess(EXIT_FAILURE);
    }

    // Important call otherwise on Windows 10 machines that have a zoom scale greater
    // than 1, the program will look blurry
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    db::Init();
    render::InitializeDirect2D();
}

INT APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow
)
{
    AppWindow window;

    try
    {
        Initialize();
        window.Initialize();
        window.Show(SW_MAXIMIZE);
        window.StartMessageLoop();
    } catch (std::exception& e) {
        MessageBoxA(NULL, e.what(), ("Error [" + std::to_string(GetLastError()) + "]").c_str(), MB_ICONERROR | MB_OK);
    }
    
    db::Uninit();
    render::UninitializeDirect2D();
    ReleaseMutex(g_hSingleInstanceMutex);

    return 0;
}