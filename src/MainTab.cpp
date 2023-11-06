#include "MainTab.h"
#include "Utility.h"
#include "resource.h"
#include "Database.h"

#include "ExportTab.h"

#include <stdexcept>

#define DEACTIVATED_ICON IDI_ICON4
#define CANCEL_ICON      IDI_ICON2
#define EDIT_ICON        IDI_ICON3
#define SEARCH_ICON      IDI_ICON5
#define CHECKMARK_ICON   IDI_ICON6
#define PLUS_ICON        IDI_ICON7
#define ACTIVE_ICON      IDI_ICON10
#define INFORMED_ICON    IDI_ICON11

#define LV_ID_INDEX     0
#define LV_STATE_INDEX  2
#define LV_ROLE_INDEX   3
#define LV_FNAME_INDEX  4    // Firstname
#define LV_LNAME_INDEX  5    // Lastname
#define LV_PNAME_INDEX  6    // Paternalname
#define LV_DDATE_INDEX  7    // Departure Date
#define LV_DTIME_INDEX  8    // Departure Time
#define LV_ADATE_INDEX  9    // Arrival Date
#define LV_ATIME_INDEX  10    // Arrival time
#define LV_AATIME_INDEX 11    // Actual Arrival Time
#define LV_NOTES_INDEX  12

#define VK_D 0x44
#define VK_E 0x45
#define VK_X 0x58

LRESULT CALLBACK TicketListViewSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR dwRefData)
{
    MainTab* pMainTab;

    switch (uMsg)
    {
    case WM_KEYDOWN:
        if (pMainTab = (MainTab*)dwRefData)
        {
            pMainTab->HandleListViewKeyDownMessage(wParam);
        }
        break;
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

MainTab::MainTab(TabManager* pTabManager, const std::wstring& name)
	: Tab(pTabManager, name)
{
    m_hHugeFont = util::CreateStandardUIFont(static_cast<int>(28 * util::GetDPIScale(m_hWndSelf)), L"Arial");
    THROW_IF_NULL(m_hHugeFont, "Unable to create MainTab font");

    m_hLargeFont = util::CreateStandardUIFont(static_cast<int>(24 * util::GetDPIScale(m_hWndSelf)));
    THROW_IF_NULL(m_hLargeFont, "Unable to create MainTab font");

    m_hSmallFont = util::CreateStandardUIFont(static_cast<int>(18 * util::GetDPIScale(m_hWndSelf)));
    THROW_IF_NULL(m_hLargeFont, "Unable to create MainTab font");

    InitTicketListView();
    InitLVRelatedControls();
    InitRowEditControls();
}

MainTab::~MainTab(void)
{
    SAFE_DELETE_GDIOBJ(m_hSmallFont);
    SAFE_DELETE_GDIOBJ(m_hLargeFont);
    SAFE_DELETE_GDIOBJ(m_hHugeFont);

    SAFE_DELETE_GDIOBJ(m_hCancelIcon);
    SAFE_DELETE_GDIOBJ(m_hDeactivateIcon);
    SAFE_DELETE_GDIOBJ(m_hEditIcon);
    SAFE_DELETE_GDIOBJ(m_hSearchIcon);
    SAFE_DELETE_GDIOBJ(m_hCheckmarkIcon);
    SAFE_DELETE_GDIOBJ(m_hPlusIcon);
    SAFE_DELETE_GDIOBJ(m_hActiveIcon);
    SAFE_DELETE_GDIOBJ(m_hInformedIcon);

    SAFE_DELETE(m_pTicketListView);
}

void MainTab::InitTicketListView(void)
{
    m_pTicketListView = new ListView(m_hWndSelf, Size(0, 0), Point(0, 0));

    if (!m_pTicketListView)
    {
        throw std::runtime_error("Unable to create the ticket list view [MainTab]");
    }

    SetWindowSubclass(m_pTicketListView->GetHandle(), ::TicketListViewSubclassProc, NULL, (DWORD_PTR)this);

    m_pTicketListView->SetColorRule(L"Ενεργή", RGB(0, 200, 0), 2);
    m_pTicketListView->SetColorRule(L"Αναμονή", RGB(255, 216, 76), 2);
    m_pTicketListView->SetColorRule(L"Ανενεργή", RGB(255, 0, 0), 2);

    m_pTicketListView->SetColorRule(L"Στέλεχος", RGB(73, 150, 183), 3);
    m_pTicketListView->SetColorRule(L"Κατασκηνωτής/ρια", RGB(0, 0, 255), 3);

    m_pTicketListView->AddColumn(L"#", 30);
    m_pTicketListView->AddColumn(L"Ενημ.", 50);
    m_pTicketListView->AddColumn(L"Κατάσταση", 100);
    m_pTicketListView->AddColumn(L"Ιδιότητα", 140);
    m_pTicketListView->AddColumn(L"Όνομα", 140);
    m_pTicketListView->AddColumn(L"Επώνυμο", 140);
    m_pTicketListView->AddColumn(L"Πατρώνυμο", 140);
    m_pTicketListView->AddColumn(L"Ημ/νια Αποχώρησης", 140);
    m_pTicketListView->AddColumn(L"Δηλ. Ώρα Αποχ.", 100);
    m_pTicketListView->AddColumn(L"Ημ/νια Επιστροφής", 140);
    m_pTicketListView->AddColumn(L"Δηλ. Ώρα Επ.", 100);
    m_pTicketListView->AddColumn(L"Ώρα Επιστροφής", 100);
    m_pTicketListView->AddColumn(L"Σημείωση", 200);

    LoadTicketsFromDatabaseFile();
}

void MainTab::LoadTicketsFromDatabaseFile(void)
{
    std::vector<db::Ticket> tickets;
    db::LoadTicketsFromDatabase(tickets);
    
    for (db::Ticket& ticket : tickets)
    {
        m_pTicketListView->AddRow(ticket);
    }
}

void MainTab::InitLVRelatedControls(void)
{
    HINSTANCE hInstance = GetModuleHandle(NULL);

    m_hSearchEdit = CreateWindow(
        L"Edit",
        L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
        0, 0, 0, 0,
        m_hWndSelf,
        NULL,
        hInstance,
        NULL
    );

    SetWindowFont(m_hSearchEdit, m_hLargeFont);
    SetPlaceholderText(m_hSearchEdit, L"Αναζήτηση");

    m_hCancelIcon     = LoadIcon(hInstance, MAKEINTRESOURCE(CANCEL_ICON));
    m_hDeactivateIcon = LoadIcon(hInstance, MAKEINTRESOURCE(DEACTIVATED_ICON));
    m_hEditIcon       = LoadIcon(hInstance, MAKEINTRESOURCE(EDIT_ICON));
    m_hSearchIcon     = LoadIcon(hInstance, MAKEINTRESOURCE(SEARCH_ICON));
    m_hPlusIcon       = LoadIcon(hInstance, MAKEINTRESOURCE(PLUS_ICON));
    m_hActiveIcon     = LoadIcon(hInstance, MAKEINTRESOURCE(ACTIVE_ICON));
    m_hInformedIcon   = LoadIcon(hInstance, MAKEINTRESOURCE(INFORMED_ICON));

    m_hDeactivateButton = CreateChildButtonWithIcon(L"  Απενεργοποίηση",  m_hDeactivateIcon);
    m_hDeleteButton     = CreateChildButtonWithIcon(L"        Διαγραφή",   m_hCancelIcon);
    m_hEditButton       = CreateChildButtonWithIcon(L"      Επεξεργασία", m_hEditIcon);
    m_hAddButton        = CreateChildButtonWithIcon(L"         Έκδοση",    m_hPlusIcon);
    m_hInformedButton   = CreateChildButtonWithIcon(L"    Ενημερώθηκε", m_hInformedIcon);
    m_hActivateButton   = CreateChildButtonWithIcon(L"    Ενεργοποίηση", m_hActiveIcon);

    EnableWindow(m_hAddButton, TRUE);
}

void MainTab::InitRowEditControls(void)
{
    const HINSTANCE hInstance = GetModuleHandle(NULL);

    m_hFirstnameEdit    = CreateChildEdit(L"*Όνομα",      ES_AUTOHSCROLL);
    m_hLastnameEdit     = CreateChildEdit(L"*Επώνυμο",    ES_AUTOHSCROLL);
    m_hPaternalnameEdit = CreateChildEdit(L"*Πατρώνυμο",  ES_AUTOHSCROLL);

    SendMessage(m_hFirstnameEdit, EM_SETREADONLY, TRUE, 0);
    SendMessage(m_hLastnameEdit, EM_SETREADONLY, TRUE, 0);
    SendMessage(m_hPaternalnameEdit, EM_SETREADONLY, TRUE, 0);

    m_hDepartDateEdit   = CreateChildEdit(L"*Ημ/νια αποχώρησης", ES_AUTOHSCROLL);
    m_hDepartureEdit    = CreateChildEdit(L"*00:00");
    m_hArrivalDateEdit  = CreateChildEdit(L"*Ημ/νια επιστροφής", ES_AUTOHSCROLL);
    m_hArrivalEdit      = CreateChildEdit(L"*00:00");
    m_hNotesEdit        = CreateChildEdit(L"Σημειώσεις", ES_AUTOVSCROLL | ES_MULTILINE);

    SetWindowSubclass(m_hNotesEdit, util::MultilineEditTabstopSubclassProcedure, NULL, NULL);

    SetEditCharLimit(m_hDepartureEdit, 5);
    SetEditCharLimit(m_hArrivalEdit, 5);

    m_hCheckmarkIcon    = LoadIcon(hInstance, MAKEINTRESOURCE(CHECKMARK_ICON));
    m_hSubmitButton     = CreateChildButtonWithIcon(L"               Υποβολή", m_hCheckmarkIcon);

    SetWindowLong(m_hSubmitButton, GWL_STYLE, GetWindowLong(m_hSubmitButton, GWL_STYLE) | WS_TABSTOP);
}

HWND MainTab::CreateChildButtonWithIcon(const wchar_t* lpszText, HICON hIcon)
/*++
* 
* Routine Description:
* 
*   Creates a button as a child window to the tab, with an icon, and sets the default font.
* 
*   If the creation failed this function will throw an exception.
* 
* Arguments:
* 
*   lpszText - Text that will be displayed on the button.
*   hIcon    - Handle to the icon that will be drawn on the bitmap.
* 
* Return Value:
* 
*   Handle to the created button.
* 
--*/
{
    HINSTANCE hInstance = GetModuleHandle(NULL);

    HWND hButton = CreateWindow(
        L"Button",
        lpszText,
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_LEFT | WS_DISABLED,
        0, 0, 0, 0,
        m_hWndSelf,
        NULL,
        hInstance,
        NULL
    );

    THROW_IF_NULL(hButton, "Unable to create button [MainTab]");

    SetWindowFont(hButton, m_hSmallFont);

    if (hIcon != NULL)
    {
        SetButtonIcon(hButton, hIcon);
    }

    return hButton;
}

HWND MainTab::CreateChildEdit(const wchar_t* lpszPlaceholder, DWORD dwExtraStyles)
/*++
*
* Routine Description:
*
*   Creates an edit control as a child to the tab, with some specified placeholder text.
* 
*   If the creation failed this function will throw an exception.
*
* Arguments:
*
*   lpszPlaceholder - Text that will be displayed in the control when there is no text.
*                     If this parameter is null no text is displayed.
*   dwExtraStyles   - Extra styles that will be used in CreateWindow.
*
* Return Value:
*
*   Handle to the created edit control.
*
--*/
{
    HWND hEdit = CreateWindow(
        L"Edit",
        L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | dwExtraStyles,
        0, 0, 0, 0,
        m_hWndSelf,
        NULL,
        GetModuleHandle(NULL),
        NULL
    );

    THROW_IF_NULL(hEdit, "Unable to create edit control [MainTab]");

    SetWindowFont(hEdit, m_hSmallFont);

    if (lpszPlaceholder != nullptr)
    {
        SetPlaceholderText(hEdit, lpszPlaceholder);
    }

    return hEdit;
}

void MainTab::OnResize(int width, int height)
{
    const double dpiScale = util::GetDPIScale(m_hWndSelf);

    // This must not be null because the position of all the controls
    // surrounding it depend on its position
    if (m_pTicketListView) 
    {
        UpdateTicketListPos(width, height, dpiScale);
        UpdateSearchbarPos(width, height, dpiScale);
        UpdateAddButtonPos(width, height, dpiScale);
        UpdateDeactivateButtonPos(width, height, dpiScale);
        UpdateCancelButtonPos(width, height, dpiScale);
        UpdateEditButtonPos(width, height, dpiScale);
        UpdateInformedButtonPos(width, height, dpiScale);
        UpdateActivateButtonPos(width, height, dpiScale);
    }

    int iDistanceBetweenControls = static_cast<int>(40 * util::GetDPIScale(m_hWndSelf));

    const int cxEditControl = static_cast<int>(GetEditAreaWidth() - iDistanceBetweenControls * 2);
    const int cyEditControl = static_cast<int>(dpiScale * 25);
    
    POINT ptGroupTopLeft = {};
    ptGroupTopLeft.x = GetWidth() - GetEditAreaWidth() + iDistanceBetweenControls;
    ptGroupTopLeft.y = static_cast<LONG>(100 * dpiScale);

    iDistanceBetweenControls = static_cast<int>(16 * dpiScale);

    UpdateFirstnameEditPos(ptGroupTopLeft, cxEditControl, cyEditControl, iDistanceBetweenControls);
    UpdateLastnameEditPos(ptGroupTopLeft, cxEditControl, cyEditControl, iDistanceBetweenControls);
    UpdatePaternalnameEditPos(ptGroupTopLeft, cxEditControl, cyEditControl, iDistanceBetweenControls);
    UpdateDepartureEditPos(ptGroupTopLeft, cxEditControl, cyEditControl, iDistanceBetweenControls);
    UpdateArrivalEditPos(ptGroupTopLeft, cxEditControl, cyEditControl, iDistanceBetweenControls);
    UpdateNotesEditPos(ptGroupTopLeft, cxEditControl, cyEditControl, iDistanceBetweenControls);

    SetWindowPos(
        m_hSubmitButton,
        NULL,
        ptGroupTopLeft.x,
        ptGroupTopLeft.y + 7 * (iDistanceBetweenControls + cyEditControl),
        cxEditControl,
        static_cast<int>(40 * dpiScale),
        SWP_NOZORDER
    );
}

void MainTab::UpdateTicketListPos(int width, int height, double dpiScale)
/*++
* 
* Routine Description:
* 
*   Sets the list view position and size.
* 
*   All the related controls will be positioned relative to the list.
* 
* Arguments:
* 
*   width    - Width of the parent window.
*   height   - Height of the parent window.
*   dpiScale - The scaling factor for everything.
* 
* Return Value:
* 
*   None.
* 
--*/
{
    const int iDistanceFromEdges = static_cast<int>(80 * dpiScale);

    const int iButtonWidth = GetLVButtonWidth();
    const int iDistanceFromListView = static_cast<int>(32 * dpiScale);

    m_pTicketListView->SetSize(
        ((int)(GetWidth()) - GetEditAreaWidth()) - iButtonWidth - iDistanceFromListView - iDistanceFromEdges * 2,
        height - iDistanceFromEdges * 2
    );

    m_pTicketListView->SetPos(
        (((int)(GetWidth()) - this->GetEditAreaWidth()) - (m_pTicketListView->GetWidth() + iButtonWidth + iDistanceFromListView)) / 2,
        iDistanceFromEdges
    );
}

void MainTab::UpdateSearchbarPos(int width, int height, double dpiScale)
/*++
*
* Routine Description:
*
*   Positions the searchbar above the list view, centered horizontally.
*
* Arguments:
*
*   width    - Width of the parent window.
*   height   - Height of the parent window.
*   dpiScale - The scaling factor for everything.
*
* Return Value:
*
*   None.
*
--*/
{
    const int iSearchEditWidth = static_cast<int>(240 * dpiScale);
    const int iSearchEditHeight = static_cast<int>(32 * dpiScale);
    const int iGapBetweenEditAndList = static_cast<int>(16 * dpiScale);

    SetWindowPos(
        m_hSearchEdit,
        NULL,
        m_pTicketListView->GetX() + (m_pTicketListView->GetWidth() - iSearchEditWidth) / 2,
        m_pTicketListView->GetY() - iSearchEditHeight - iGapBetweenEditAndList,
        iSearchEditWidth, 
        iSearchEditHeight,
        SWP_NOZORDER
    );
}

void MainTab::UpdateAddButtonPos(int width, int height, double dpiScale)
/*++
*
* Routine Description:
*
*   Positions the add button to the right of the list view.
*
* Arguments:
*
*   width    - Width of the parent window.
*   height   - Height of the parent window.
*   dpiScale - The scaling factor for everything.
*
* Return Value:
*
*   None.
*
--*/
{
    const int iDistanceFromListView = static_cast<int>(32 * dpiScale);
    const int iButtonWidth          = GetLVButtonWidth();
    const int iButtonHeight         = static_cast<int>(40 * dpiScale);

    const int iDistBetweenButtons = GetButtonDistanceFromLV();

    SetWindowPos(
        m_hAddButton,
        NULL,
        m_pTicketListView->GetX() + m_pTicketListView->GetWidth() + iDistanceFromListView,
        m_pTicketListView->GetY(),
        iButtonWidth,
        iButtonHeight,
        SWP_NOZORDER
    );
}

void MainTab::UpdateInformedButtonPos(int width, int height, double dpiScale)
{
    const int iDistanceFromListView = static_cast<int>(32 * dpiScale);
    const int iButtonWidth = GetLVButtonWidth();
    const int iButtonHeight = static_cast<int>(40 * dpiScale);

    const int iDistBetweenButtons = GetButtonDistanceFromLV();

    SetWindowPos(
        m_hInformedButton,
        NULL,
        m_pTicketListView->GetX() + m_pTicketListView->GetWidth() + iDistanceFromListView,
        m_pTicketListView->GetY() + (iButtonHeight + iDistBetweenButtons) * 2,
        iButtonWidth,
        iButtonHeight,
        SWP_NOZORDER
    );
}

void MainTab::UpdateActivateButtonPos(int width, int height, double dpiScale)
{
    const int iDistanceFromListView = static_cast<int>(32 * dpiScale);
    const int iButtonWidth = GetLVButtonWidth();
    const int iButtonHeight = static_cast<int>(40 * dpiScale);

    const int iDistBetweenButtons = GetButtonDistanceFromLV();

    SetWindowPos(
        m_hActivateButton,
        NULL,
        m_pTicketListView->GetX() + m_pTicketListView->GetWidth() + iDistanceFromListView,
        m_pTicketListView->GetY() + (iButtonHeight + iDistBetweenButtons) * 3,
        iButtonWidth,
        iButtonHeight,
        SWP_NOZORDER
    );
}

void MainTab::UpdateDeactivateButtonPos(int width, int height, double dpiScale)
/*++
*
* Routine Description:
*
*   Positions the deactivate button to the right of the list view, under the add button.
*
* Arguments:
*
*   width    - Width of the parent window.
*   height   - Height of the parent window.
*   dpiScale - The scaling factor for everything.
*
* Return Value:
*
*   None.
*
--*/
{
    const int iDistanceFromListView = static_cast<int>(32 * dpiScale);
    const int iButtonWidth          = GetLVButtonWidth();
    const int iButtonHeight         = static_cast<int>(40 * dpiScale);

    const int iDistBetweenButtons = GetButtonDistanceFromLV();

    SetWindowPos(
        m_hDeactivateButton,
        NULL,
        m_pTicketListView->GetX() + m_pTicketListView->GetWidth() + iDistanceFromListView,
        m_pTicketListView->GetY() + (iButtonHeight + iDistBetweenButtons) * 4,
        iButtonWidth,
        iButtonHeight,
        SWP_NOZORDER
    );
}

void MainTab::UpdateCancelButtonPos(int width, int height, double dpiScale)
/*++
*
* Routine Description:
*
*   Positions the cancel button right under the deactivate button.
*
* Arguments:
*
*   width    - Width of the parent window.
*   height   - Height of the parent window.
*   dpiScale - The scaling factor for everything.
*
* Return Value:
*
*   None.
*
--*/
{
    const int iDistanceFromListView = static_cast<int>(32 * dpiScale);
    const int iButtonWidth          = GetLVButtonWidth();
    const int iButtonHeight         = static_cast<int>(40 * dpiScale);

    const int iDistBetweenButtons   = GetButtonDistanceFromLV();

    SetWindowPos(
        m_hDeleteButton,
        NULL,
        m_pTicketListView->GetX() + m_pTicketListView->GetWidth() + iDistanceFromListView,
        m_pTicketListView->GetY() + (iButtonHeight + iDistBetweenButtons) * 5,
        iButtonWidth,
        iButtonHeight,
        SWP_NOZORDER
    );
}

void MainTab::UpdateEditButtonPos(int width, int height, double dpiScale)
/*++
*
* Routine Description:
*
*   Positions the edit button under the cancel button.
*
* Arguments:
*
*   width    - Width of the parent window.
*   height   - Height of the parent window.
*   dpiScale - The scaling factor for everything.
*
* Return Value:
*
*   None.
*
--*/
{
    const int iDistanceFromListView = static_cast<int>(32 * dpiScale);
    const int iButtonWidth          = GetLVButtonWidth();
    const int iButtonHeight         = static_cast<int>(40 * dpiScale);

    const int iDistBetweenButtons = GetButtonDistanceFromLV();

    SetWindowPos(
        m_hEditButton,
        NULL,
        m_pTicketListView->GetX() + m_pTicketListView->GetWidth() + iDistanceFromListView,
        m_pTicketListView->GetY() + iDistBetweenButtons + iButtonHeight,
        iButtonWidth,
        iButtonHeight,
        SWP_NOZORDER
    );
}

void MainTab::UpdateFirstnameEditPos(const POINT& ptGroup, int cx, int cy, int dist)
/*++
* 
* Routine Description:
* 
*   Positions the first edit in the group. The given point
*   basically defines the top left point of this control.
* 
* Arguments:
* 
*   ptGroup - The top left point of the group of controls.
*   cx      - Default width of an edit control in the group, in pixels.
*   cy      - Default height of an edit control in the group, in pixels.
*   dist    - Distance between the controls in the group, in pixels.
* 
* Return Value:
* 
*   None.
* 
--*/
{
    SetWindowPos(
        m_hFirstnameEdit,
        NULL,
        ptGroup.x,
        ptGroup.y,
        cx,
        cy,
        SWP_NOZORDER
    );
}

void MainTab::UpdateLastnameEditPos(const POINT& ptGroup, int cx, int cy, int dist)
/*++
*
* Routine Description:
*
*   Positions the second control in the group such that it is to
*   the right of the first one, and there is a distance equal to dist
*   between them.
*
* Arguments:
*
*   ptGroup - The top left point of the group of controls.
*   cx      - Default width of an edit control in the group, in pixels.
*   cy      - Default height of an edit control in the group, in pixels.
*   dist    - Distance between the controls in the group, in pixels.
*
* Return Value:
*
*   None.
*
--*/
{
    SetWindowPos(
        m_hLastnameEdit,
        NULL,
        ptGroup.x,
        ptGroup.y + dist + cy,
        cx,
        cy,
        SWP_NOZORDER
    );
}

void MainTab::UpdatePaternalnameEditPos(const POINT& ptGroup, int cx, int cy, int dist)
/*++
*
* Routine Description:
*
*   Positions the third control in the group, right under the first one,
*   such that there is a distance equal to dist between them.
*
* Arguments:
*
*   ptGroup - The top left point of the group of controls.
*   cx      - Default width of an edit control in the group, in pixels.
*   cy      - Default height of an edit control in the group, in pixels.
*   dist    - Distance between the controls in the group, in pixels.
*
* Return Value:
*
*   None.
*
--*/
{
    SetWindowPos(
        m_hPaternalnameEdit,
        NULL,
        ptGroup.x,
        ptGroup.y + (cy + dist) * 2,
        cx,
        cy,
        SWP_NOZORDER
    );
}

void MainTab::UpdateDepartureEditPos(const POINT& ptGroup, int cx, int cy, int dist)
/*++
*
* Routine Description:
*
*   Positions the fourth and fifth controls of the group next to each other, under the third.
*
* Arguments:
*
*   ptGroup - The top left point of the group of controls.
*   cx      - Default width of an edit control in the group, in pixels.
*   cy      - Default height of an edit control in the group, in pixels.
*   dist    - Distance between the controls in the group, in pixels.
*
* Return Value:
*
*   None.
*
--*/
{
    SetWindowPos(
        m_hDepartDateEdit,
        NULL,
        ptGroup.x,
        ptGroup.y + (dist + cy) * 3,
        cx * 3 / 4,
        cy,
        SWP_NOZORDER
    );

    SetWindowPos(
        m_hDepartureEdit,
        NULL,
        ptGroup.x + cx * 3 / 4 + 2,
        ptGroup.y + (dist + cy) * 3,
        cx / 4 - 2,
        cy,
        SWP_NOZORDER
    );
}

void MainTab::UpdateArrivalEditPos(const POINT& ptGroup, int cx, int cy, int dist)
/*++
*
* Routine Description:
*
*   Positions the sixth and seventh controls in the group next to each other, under the fourth and fifth.
*
* Arguments:
*
*   ptGroup - The top left point of the group of controls.
*   cx      - Default width of an edit control in the group, in pixels.
*   cy      - Default height of an edit control in the group, in pixels.
*   dist    - Distance between the controls in the group, in pixels.
*
* Return Value:
*
*   None.
*
--*/
{
    SetWindowPos(
        m_hArrivalDateEdit,
        NULL,
        ptGroup.x,
        ptGroup.y + (dist + cy) * 4,
        cx * 3 / 4,
        cy,
        SWP_NOZORDER
    );

    SetWindowPos(
        m_hArrivalEdit,
        NULL,
        ptGroup.x + cx * 3 / 4 + 2,
        ptGroup.y + (dist + cy) * 4,
        cx / 4 - 2,
        cy,
        SWP_NOZORDER
    );
}

void MainTab::UpdateNotesEditPos(const POINT& ptGroup, int cx, int cy, int dist)
/*++
*
* Routine Description:
*
*   Positions the last edit control under every other edit control, taking up
*   the same horizontal as the other controls together.
*
* Arguments:
*
*   ptGroup - The top left point of the group of controls.
*   cx      - Default width of an edit control in the group, in pixels.
*   cy      - Default height of an edit control in the group, in pixels.
*   dist    - Distance between the controls in the group, in pixels.
*
* Return Value:
*
*   None.
*
--*/
{
    SetWindowPos(
        m_hNotesEdit,
        NULL,
        ptGroup.x,
        ptGroup.y + 5 * (dist + cy),
        cx,
        cy * 3,
        SWP_NOZORDER
    );
}

void MainTab::OnCommand(HWND hWnd)
{
    WCHAR buffer[MAX_NOTES_LENGTH];

    if (hWnd == m_hSearchEdit) 
    {
        GetWindowText(m_hSearchEdit, buffer, MAX_NOTES_LENGTH - 1);

        m_pTicketListView->ApplyRowFilter(buffer);
    }

    else if (hWnd == m_hAddButton)
    {
        PostMessage(m_hWndParent, WM_SWITCH_TO_EXPORT_TAB, NULL, NULL);
    }

    else if (m_pTicketListView && m_pTicketListView->IsSomeRowSelected())
    {
        if (hWnd == m_hDeactivateButton)
        {
            OnDeactivateButtonClicked();
        }

        else if (hWnd == m_hDeleteButton)
        {   
            OnDeleteButtonClicked();
        }

        else if (hWnd == m_hEditButton)
        {
            OnEditRowButtonClicked();
        }

        else if (hWnd == m_hSubmitButton)
        {
            OnSubmitButtonClicked();
        }

        else if (hWnd == m_hActivateButton)
        {
            OnActivateButtonClicked();
        }
        
        else if (hWnd == m_hInformedButton)
        {
            OnInformedButtonClicked();
        }
    }
}

LRESULT MainTab::OnCustomMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
/*++
* 
* Routine Description:
* 
*   Each tab subclass may want to use a custom message for its own reasons.
*   Because we don't want to alter the Tab window procedure code every time 
*   a custom message is added, we'll use a custom message function that is
*   called when the message exceeds WM_APP.
* 
*   In this case we handle row selected/unselected messages, in order to 
*   enable and disable the appropriate buttons.
* 
* Arguments:
* 
*   uMsg - The message code.
*   wParam - User data
*   lParam - User data
* 
* Return Value:
* 
*   None.
* 
--*/
{
    switch (uMsg)
    {
    case WM_ROW_SELECTED:
        EnableWindow(m_hDeactivateButton, m_pTicketListView->GetSelectedRow()[LV_STATE_INDEX] == L"Ενεργή");
        EnableWindow(m_hDeleteButton, TRUE);
        EnableWindow(m_hEditButton, TRUE);
        if (m_pTicketListView->GetSelectedRow()[1] == L"✕")
            EnableWindow(m_hInformedButton, TRUE);
        if (m_pTicketListView->GetSelectedRow()[2] == L"Αναμονή")
            EnableWindow(m_hActivateButton, TRUE);
        if (m_isEditing) {
            DisableEditing();
        }
        break;
        
    case WM_ROW_UNSELECTED:
        EnableWindow(m_hDeleteButton, FALSE);
        EnableWindow(m_hDeactivateButton, FALSE);
        EnableWindow(m_hEditButton, FALSE);
        EnableWindow(m_hSubmitButton, FALSE);
        EnableWindow(m_hActivateButton, FALSE);
        EnableWindow(m_hInformedButton, FALSE);
        m_isEditing = false;
        break;

    case WM_EXPORT_TICKET:
        SaveTicketToDatabase(*((std::vector<std::wstring>*)lParam));
        break;

    case WM_DELETE_PERSON_TICKET: {
        std::vector<db::Ticket> tickets;
        db::GetTicketsOfPerson(wParam, tickets);
        for (db::Ticket& ticket : tickets) {
            db::DeleteTicket(std::stoi(ticket[0]));
            for (size_t i = 0; i < m_pTicketListView->GetRowCount(); ++i) {
                if (m_pTicketListView->GetCellContent(i, 0) == ticket[0]) {
                    m_pTicketListView->RemoveRow(i);
                    break;
                }
            }
        }
        }
        break;
    }

    return 0;
}

void MainTab::Draw(ID2D1RenderTarget* pRenderTarget)
/*++
* 
* Routine Description:
* 
*   Draws the background of the edit area using hardware acceleration.
* 
* Arguments:
* 
*   pRenderTarget - Pointer to the Direct2D render target.
* 
* Return Value:
* 
*   None.
* 
--*/
{
    ID2D1SolidColorBrush* pBrush = GetSolidColorBrush();

    if (pBrush)
    {
        const D2D1_COLOR_F crBeforeCall = pBrush->GetColor();

        pBrush->SetColor(D2D1::ColorF(RGB_D2D(150, 150, 150)));

        int x = GetEditAreaWidth();

        pRenderTarget->DrawLine(
            D2D1::Point2F(GetWidth() - x, 0),
            D2D1::Point2F(GetWidth() - x, GetHeight()),
            m_pSolidColorBrush
        );

        pBrush->SetColor(crBeforeCall);
    }
}

void MainTab::Draw(HDC hDC)
/*++
*
* Routine Description:
*
*   Draws the search icon and edit text.
*
* Arguments:
*
*   hDC - Handle to the device context.
*
* Return Value:
*
*   None.
*
--*/
{
    SIZE textSize;

    const double dpiScale = util::GetDPIScale(m_hWndSelf);

    const int iSearchEditWidth       = static_cast<int>(240 * dpiScale);
    const int iSearchEditHeight      = static_cast<int>(32 * dpiScale);
    const int iGapBetweenEditAndList = static_cast<int>(16 * dpiScale);

    DrawIconEx(hDC,
        m_pTicketListView->GetX() + (m_pTicketListView->GetWidth() - iSearchEditWidth) / 2 - 52, // I don't remember why i picked 52 lol
        m_pTicketListView->GetY() - iSearchEditHeight - iGapBetweenEditAndList,
        m_hSearchIcon,
        0,
        0,
        NULL,
        NULL,
        DI_NORMAL
    );

    SelectObject(hDC, m_hHugeFont);
    SetBkMode(hDC, TRANSPARENT);
    
    if (GetTextExtentPoint32(hDC, L"ΕΠΕΞΕΡΓΑΣΙΑ", 11, &textSize))
    {
        TextOut(
            hDC,
            static_cast<int>(GetWidth() - (GetEditAreaWidth() + textSize.cx) / 2),
            static_cast<int>((100 - textSize.cy - 10) * dpiScale),
            L"ΕΠΕΞΕΡΓΑΣΙΑ",
            11
        );
    }
}

void MainTab::OnEditRowButtonClicked(void)
/*++
* 
--*/
{
    std::vector<std::wstring> selectedRow = m_pTicketListView->GetSelectedRow();

    SetWindowText(m_hFirstnameEdit,    selectedRow[LV_FNAME_INDEX].c_str());
    SetWindowText(m_hLastnameEdit,     selectedRow[LV_LNAME_INDEX].c_str());
    SetWindowText(m_hPaternalnameEdit, selectedRow[LV_PNAME_INDEX].c_str());
    SetWindowText(m_hDepartDateEdit,   selectedRow[LV_DDATE_INDEX].c_str());
    SetWindowText(m_hDepartureEdit,    selectedRow[LV_DTIME_INDEX].c_str());
    SetWindowText(m_hArrivalDateEdit,  selectedRow[LV_ADATE_INDEX].c_str());
    SetWindowText(m_hArrivalEdit,      selectedRow[LV_ATIME_INDEX].c_str());
    SetWindowText(m_hNotesEdit,        selectedRow[LV_NOTES_INDEX].c_str());

    EnableWindow(m_hSubmitButton, TRUE);

    m_isEditing = true;

    SetFocus(m_hDepartDateEdit);
}

void MainTab::OnSubmitButtonClicked(void)
/*++
*
* Routine Description:
*
*   If the data entered is valid, it becomes the new data of the selected row.
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
    wchar_t buffer[MAX_NOTES_LENGTH];

    if (!WindowTextIsMilitaryTime(m_hDepartureEdit))
    {
        MessageBox(m_hWndSelf, L"Η ώρα αποχώρησης δεν είναι έγκυρη", L"Σφάλμα", MB_OK | MB_ICONERROR);
        return;
    }

    if (!WindowTextIsMilitaryTime(m_hArrivalEdit))
    {
        MessageBox(m_hWndSelf, L"Η ώρα επιστροφής δεν είναι έγκυρη", L"Σφάλμα", MB_OK | MB_ICONERROR);
        return;
    }

    const HWND hEditControlsInOrder[] = {
        m_hFirstnameEdit,
        m_hLastnameEdit,
        m_hPaternalnameEdit,
        m_hDepartDateEdit,
        m_hDepartureEdit,
        m_hArrivalDateEdit,
        m_hArrivalEdit,
        m_hNotesEdit
    };

    for (int i = 0; i < (sizeof(hEditControlsInOrder) / sizeof(HWND)) - 1; ++i)
    {
        if (GetWindowTextLength(hEditControlsInOrder[i]) == 0)
        {
            MessageBox(m_hWndSelf, L"Πρέπει να συμπληρώσετε όλα τα πεδία που έχουν αστερίσκο", L"Σφάλμα", MB_ICONERROR | MB_OK);
            return;
        }
    }

    GetWindowText(m_hDepartDateEdit, buffer, MAX_NOTES_LENGTH);

    if (!util::IsValidDate(util::ConvertStringToDate(buffer)))
    {
        MessageBox(m_hWndSelf, L"Η ημερομηνία αποχώρησης δεν είναι έγκυρη", L"Σφάλμα", MB_OK | MB_ICONERROR);
        return;
    }

    GetWindowText(m_hArrivalDateEdit, buffer, MAX_NOTES_LENGTH);

    if (!util::IsValidDate(util::ConvertStringToDate(buffer)))
    {
        MessageBox(m_hWndSelf, L"Η ημερομηνία επιστροφής δεν είναι έγκυρη", L"Σφάλμα", MB_OK | MB_ICONERROR);
        return;
    }

    std::vector<std::wstring> newRowData;

    const int iSelectedRowIndex = m_pTicketListView->GetSelectedRowIndex();

    // The first cell of the row contains the state of the ticket, which cannot be edited
    // by the user, so we want to maintain the value after editing. We're also certain that
    // a row is selected because otherwise this function couldn't have been called, so
    // there is no need to check anything
    newRowData.emplace_back(m_pTicketListView->GetCellContent(iSelectedRowIndex, 0));
    newRowData.emplace_back(m_pTicketListView->GetCellContent(iSelectedRowIndex, 1));
    newRowData.emplace_back(m_pTicketListView->GetCellContent(iSelectedRowIndex, 2));
    
    for (int i = 0; i < sizeof(hEditControlsInOrder) / sizeof(HWND); ++i)
    {
        GetWindowText(hEditControlsInOrder[i], buffer, MAX_NOTES_LENGTH);
        SetWindowText(hEditControlsInOrder[i], L"");

        newRowData.emplace_back(buffer);
    }

    newRowData.insert(newRowData.begin() + 10, m_pTicketListView->GetCellContent(iSelectedRowIndex, 10));

    db::UpdateTicket(newRowData);

    m_pTicketListView->SetDisplayedRowContent(iSelectedRowIndex, newRowData);

    EnableWindow(m_hSubmitButton, FALSE);
    
    SetFocus(m_pTicketListView->GetHandle());
}

void MainTab::OnDeleteButtonClicked(void)
{
    if (m_pTicketListView->IsSomeRowSelected())
    {
        if (MessageBox(m_hWndSelf,
            L"Είστε βέβαιοι ότι θέλετε να διαγράψετε την άδεια; Η πράξη αυτή δεν είναι αντιστρέψιμη.",
            L"Επιβεβαίωση", MB_ICONEXCLAMATION | MB_OKCANCEL) == IDOK)
        {
            db::DeleteTicket(std::stoi(m_pTicketListView->GetCellContent(m_pTicketListView->GetSelectedRowIndex(), 0)));

            m_pTicketListView->RemoveDisplayedRow(m_pTicketListView->GetSelectedRowIndex());
        }

        SetFocus(m_pTicketListView->GetHandle());
    }
}

void MainTab::OnDeactivateButtonClicked(void)
{
    if (m_pTicketListView && m_pTicketListView->IsSomeRowSelected())
    {
        if (MessageBox(m_hWndSelf,
            L"Είστε βέβαιοι ότι θέλετε να απενεργοποιήσετε την άδεια;",
            L"Επιβεβαίωση", MB_ICONEXCLAMATION | MB_OKCANCEL) == IDOK)
        {
            int ticket_id = std::stoi(m_pTicketListView->GetCellContent(m_pTicketListView->GetSelectedRowIndex(), 0));

            SYSTEMTIME sysTime;
            GetLocalTime(&sysTime);

            wchar_t buffer[6];
            swprintf_s(buffer, 6, L"%02d:%02d", sysTime.wHour, sysTime.wMinute);

            db::DeactivateTicket(ticket_id, buffer);

            m_pTicketListView->SetCellContent(m_pTicketListView->GetSelectedRowIndex(), LV_STATE_INDEX, L"Ανενεργή");
            m_pTicketListView->SetCellContent(m_pTicketListView->GetSelectedRowIndex(), LV_AATIME_INDEX, buffer);

            EnableWindow(m_hDeactivateButton, FALSE);
        }

        SetFocus(m_pTicketListView->GetHandle());
    }
}

bool MainTab::WindowTextIsMilitaryTime(HWND hWnd)
{
    // We'll make the buffer contain 6 spaces so that if there is an extra
    // character after the military time, it will be detected. Also + 1 for the \0
    wchar_t buffer[6 + 1];
    GetWindowText(hWnd, buffer, 6);

    std::wstring windowText = buffer;

    // If the string length is 4, there is a chance that the string
    // is for example 6:00, which is valid, but the util::IsMilitaryTime
    // function only considers 06:00 valid, so we'll add a leading zero first
    // and then test it. And no, 06:5 is not valid (as 06:05) so we don't consider it.
    if (windowText.length() == 4)
    {
        windowText.insert(windowText.begin(), '0');
    }

    if (util::IsMilitaryTime(windowText))
    {
        // Yes I know that this is theoritally a yes/no function and there is a side
        // effect but It makes my life easier and I can't think of a better name
        // right now so stay mad lol
        SetWindowText(hWnd, windowText.c_str());
        return true;
    }

    return false;
}

int MainTab::GetEditAreaWidth(void)
{
    return static_cast<int>((300 - 30 * (1 - (GetWidth() / 1920.0F)))  * util::GetDPIScale(m_hWndSelf));
}

int MainTab::GetButtonDistanceFromLV(void)
{
    // Basically f(x) * dpiScale, such that f is linear and f(720) = 8 and f(1080) = 16
    return static_cast<int>(round((((int)(GetHeight()) - 720) / 45.0 + 8) * util::GetDPIScale(m_hWndSelf)));
}

int MainTab::GetLVButtonWidth(void)
{
    return static_cast<int>(176 * util::GetDPIScale((m_hWndSelf)));
}

void MainTab::DisableEditing(void) 
{
    const HWND hWindowsToBeCleared[] = {
        m_hFirstnameEdit,
        m_hLastnameEdit,
        m_hPaternalnameEdit,
        m_hDepartDateEdit,
        m_hDepartureEdit,
        m_hArrivalDateEdit,
        m_hArrivalEdit,
        m_hNotesEdit
    };

    for (HWND hWnd : hWindowsToBeCleared)
    {
        SetWindowText(hWnd, L"");
    }

    EnableWindow(m_hSubmitButton, FALSE);
    m_isEditing = false;
}

void MainTab::HandleListViewKeyDownMessage(WPARAM wParam)
{
    if (m_pTicketListView && m_pTicketListView->IsSomeRowSelected())
    {
        switch (wParam)
        {
        case VK_E:
            OnEditRowButtonClicked();
            break;

        case VK_D:
            OnDeactivateButtonClicked();
            break;

        case VK_X:
            OnDeleteButtonClicked();
            break;
        }
    }
}

void MainTab::SaveTicketToDatabase(db::Ticket& ticket)
{
    db::Ticket toBeInserted = ticket;

    toBeInserted.erase(toBeInserted.begin() + 3); // Erase role
    toBeInserted.erase(toBeInserted.begin() + 3); // Erase first name
    toBeInserted.erase(toBeInserted.begin() + 3); // Erase last name
    toBeInserted.erase(toBeInserted.begin() + 3); // Eraes father's name

    ticket[1] = (ticket[1] == L"1") ? L"✓" : L"✕";

    try {
        db::InsertTicketToDatabase(toBeInserted);
        ticket.erase(ticket.begin() + 2);
        std::swap(ticket[0], ticket[1]);
        ticket.insert(ticket.begin(), std::to_wstring(db::GetLastInsertedRowId()));
        m_pTicketListView->AddRow(ticket);
    } catch (std::exception& e) {
        MessageBoxA(m_hWndSelf, e.what(), "SQLite Error", MB_OK | MB_ICONERROR);
    }
}

void MainTab::OnActivateButtonClicked(void)
{
    if (m_pTicketListView && m_pTicketListView->IsSomeRowSelected())
    {
        int selectedRowIndex = m_pTicketListView->GetSelectedRowIndex();
        int id = std::stoi(m_pTicketListView->GetCellContent(selectedRowIndex, 0));

        m_pTicketListView->SetCellContent(selectedRowIndex, 2, L"Ενεργή");

        EnableWindow(m_hActivateButton, FALSE);

        db::ActivateTicket(id);
    }
}

void MainTab::OnInformedButtonClicked(void)
{
    if (m_pTicketListView && m_pTicketListView->IsSomeRowSelected())
    {
        int selectedRowIndex = m_pTicketListView->GetSelectedRowIndex();
        int id = std::stoi(m_pTicketListView->GetCellContent(selectedRowIndex, 0));

        m_pTicketListView->SetCellContent(selectedRowIndex, 1, L"✓");

        EnableWindow(m_hInformedButton, FALSE);

        db::TickInformed(id);
    }
}