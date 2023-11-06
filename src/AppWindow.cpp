#include "AppWindow.h"
#include "Utility.h"

#include "MainTab.h"
#include "ExportTab.h"
#include "HistoryTab.h"
#include "SettingsTab.h"
#include "ObjectTab.h"

#include "resource.h"

#include "sqlite/sqlite3.h"

#include <stdexcept>

#define WINDOW_CLASS_NAME L"GatekeeperWindowClass"

#define SAFE_RELEASE_PTR(ptr) if (ptr) { delete (ptr); (ptr) = nullptr; }

LRESULT GatekeeperProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
/*++
* 
* Routine Description:
* 
*   Calls the matching member function of the AppWindow object.
* 
* Arguments:
* 
*   Standard Win32 API Procedure Arguments.
* 
* Return Value:
* 
*   Return code for the Win32 API.
* 
--*/
{
    AppWindow* pAppWindow;

    switch (uMsg)
    {
    case WM_CREATE:
        // Save the address of the AppWindow passed as the last argument to CreateWindow
        // so it can be retrieved later to call the object's window procedure
        util::RegisterObject(hWnd, ((LPCREATESTRUCT)lParam)->lpCreateParams);
        return 0;

    default:
        // The usage of a single equal sign is not a mistake
        // Whilst processing messages before WM_CREATE, the GetRegisteredObject
        // function returns NULL because the object has not yet been registered,
        // and will therefore return null, so we have to watch out for that
        if (pAppWindow = (AppWindow*)util::GetRegisteredObject(hWnd))
        {
            return pAppWindow->WindowProcedure(hWnd, uMsg, wParam, lParam);
        }
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

AppWindow::~AppWindow(void)
{
    SAFE_RELEASE_PTR(m_pViewList);
}

LRESULT AppWindow::WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
/*++
* 
* 
* 
--*/
{
    switch (uMsg)
    {
    case WM_CLOSE:
        PostQuitMessage(EXIT_SUCCESS);
        return 0;

    case WM_DPICHANGED:
        return OnDPIChanged(hWnd, lParam);

    case WM_SIZE:
        return OnSize(LOWORD(lParam), HIWORD(lParam));

    case WM_GETMINMAXINFO:
        return OnGetMinMaxInfo(reinterpret_cast<LPMINMAXINFO>(lParam));
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void AppWindow::Initialize(void)
/*++
* 
* Routine Description:
* 
*   Registers the window class and creates the window.
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
    const HINSTANCE hInstance = GetModuleHandle(NULL);

    hAccelerator = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR1));

    InitWindowClass(hInstance);
    InitWindow(hInstance);
    
    m_pTabManager = new TabManager(m_hWnd, Size(0, 0), Point(0, 0));
    THROW_IF_NULL(m_pTabManager, "Out of memory");

    MainTab* pMainTab         = new MainTab(m_pTabManager, L"Άδειες");
    THROW_IF_NULL(pMainTab, "Out of memory");

    ExportTab* pExportTab     = new ExportTab(m_pTabManager, L"Έκδοση");
    THROW_IF_NULL(pExportTab, "Out of memory");


    ObjectTab* pObjectTab = new ObjectTab(m_pTabManager, L"Αντικείμενα");
    THROW_IF_NULL(pObjectTab, "Out of memory");

    HistoryTab* pHistoryTab   = new HistoryTab(m_pTabManager, L"Ιστορικό");
    THROW_IF_NULL(pHistoryTab, "Out of memory");

    SettingsTab* pSettingsTab = new SettingsTab(m_pTabManager, L"Ρυθμίσεις");
    THROW_IF_NULL(pSettingsTab, "Out of memory");

    pHistoryTab->GetAccessToLoadedUserData(pExportTab);
}

void AppWindow::InitWindowClass(HINSTANCE hInstance)
/*++
* 
* Routine Description:
* 
*   Registers the window class.
* 
* Arguments:
* 
*   hInstance - Handle to the process instance
* 
* Return Value:
* 
*   None.
* 
--*/
{
    WNDCLASSEX wcex = { 0 };
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.hInstance = hInstance;
    wcex.lpszClassName = WINDOW_CLASS_NAME;
    wcex.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.lpfnWndProc = GatekeeperProcedure;
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    
    if (!RegisterClassEx(&wcex))
    {
        throw std::runtime_error("Window class registration failed");
    }
}

void AppWindow::InitWindow(HINSTANCE hInstance)
/*++
* 
* Routine Description:
* 
*   Creates the main window centered relative to the screen.
* 
* Arguments:
* 
*   hInstance - Handle to the process instance
* 
* Return Value:
* 
*   None
* 
--*/
{
    constexpr unsigned int iInitialWindowWidth = 1280;
    constexpr unsigned int iInitialWindowHeight = 720;

    // Position of the top left point of the window used to center the window relative to the screen
    // Too lazy to explain why this works, it's pretty simple, use your brain lol
    const int iCenteredWindowX = (GetSystemMetrics(SM_CXSCREEN) - iInitialWindowWidth) / 2;
    const int iCenteredWindowY = (GetSystemMetrics(SM_CYSCREEN) - iInitialWindowHeight) / 2;

    m_hWnd = CreateWindow(
        WINDOW_CLASS_NAME,
        L"Gatekeeper",
        WS_OVERLAPPEDWINDOW,
        iCenteredWindowX,
        iCenteredWindowY,
        iInitialWindowWidth,
        iInitialWindowHeight,
        GetDesktopWindow(),   // This was equal to NULL when it was written, however 
        NULL,                 // in order to be sure this will work in the future I'll use this function call
        hInstance,
        this
    );

    if (!m_hWnd)
    {
        throw std::runtime_error("Window creation failed");
    }
}

void AppWindow::Show(int nCmdShow)
/*++
* 
* Routine Description:
* 
*   Displays the window using the given setting.
* 
* Arguments:
* 
*   nCmdShow - Win32 Show option.
* 
* Return Value:
* 
*   None.
* 
--*/
{
    ShowWindow(m_hWnd, nCmdShow);
}

void AppWindow::StartMessageLoop(void)
/*++
* 
* Routine Description:
* 
*   Basically begins running the program.
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
    MSG msg = {};

    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (hAccelerator && m_pTabManager)
        {
            if (TranslateAccelerator(m_pTabManager->GetHandle(), hAccelerator, &msg))
            {
                continue;
            }
        }

        // Have to make sure that the message isn't a KEYDOWN message,
        // otherwise the Subclass of the listview never receives a VK_RETURN
        if (m_pTabManager && !(msg.message == WM_KEYDOWN && (msg.wParam == VK_RETURN || msg.wParam == VK_UP || msg.wParam == VK_DOWN)))
        {
            Tab* pSelectedTab = m_pTabManager->GetCurrentTab();

            if (pSelectedTab)
            {
                if (IsDialogMessage(pSelectedTab->GetHandle(), &msg))
                {
                    continue;
                }
            }
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

LRESULT AppWindow::OnDPIChanged(HWND hWnd, LPARAM lParam)
/*++
* 
* Routine Description:
* 
*   Resizes the window when the DPI changes so it can be scaled correctly.
* 
* Arguments:
* 
*   hWnd - Handle to the window (this)
*   lParam - Pointer to a RECT that contains the new position & dimensions of the window
* 
* Return Value:
* 
*   Zero.
* 
--*/
{
    LPRECT pNewWindowRect = reinterpret_cast<LPRECT>(lParam);

    if (pNewWindowRect)
    {
        SetWindowPos(
            hWnd,
            NULL,
            pNewWindowRect->left,
            pNewWindowRect->top,
            pNewWindowRect->right,
            pNewWindowRect->bottom,
            SWP_NOZORDER
        );
    }

    return 0;
}

LRESULT AppWindow::OnSize(WORD wNewWidth, WORD wNewHeight)
/*++
* 
* Routine Description:
* 
*   Resizes and repositions the children windows.
* 
* Arguments:
* 
*   wNewWidth - The new width of the client, in pixels.
*   wNewHeight - The new height of the client, in pixels.
* 
* Return Value:
* 
*   Zero.
* 
--*/
{
    if (m_pTabManager)
    {
        m_pTabManager->SetPos(0, static_cast<int>(util::GetDPIScale(m_hWnd) * 10));
        m_pTabManager->SetSize(wNewWidth, wNewHeight - m_pTabManager->GetY());
    }

    return 0;
}

LRESULT AppWindow::OnGetMinMaxInfo(LPMINMAXINFO pMMI)
{
    pMMI->ptMinTrackSize.x = 1280;
    pMMI->ptMinTrackSize.y = 720;

    return 0;
}