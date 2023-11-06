#include "ExportTab.h"
#include "TicketExporter.h"
#include "Utility.h"
#include "resource.h"
#include "TabManager.h"

#include <stdexcept>

#define SEARCH_ICON      IDI_ICON5
#define CHECKMARK_ICON   IDI_ICON6
#define PLUS_ICON        IDI_ICON7
#define LOAD_ICON        IDI_ICON8

#define LV_ROLE_INDEX  1
#define LV_FNAME_INDEX 2
#define LV_LNAME_INDEX 3
#define LV_PNAME_INDEX 4

#define IDC_EMPLOYEE_RB     100
#define IDC_CAMPER_RB       101
#define IDC_INS_EMPLOYEE_RB 102
#define IDC_INS_CAMPER_RB   103

LRESULT CALLBACK PersonListSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR dwRefData)
{
	switch (uMsg)
	{
	case WM_KEYDOWN:
		if (wParam == VK_RETURN) 
		{
			ExportTab* pExportTab = reinterpret_cast<ExportTab*>(dwRefData);

			if (pExportTab) 
			{
				PostMessage(pExportTab->GetHandle(), WM_IMPORT_PERSON, NULL, NULL);
			}
		}
		break;
	}
	
	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

ExportTab::ExportTab(TabManager* pTabManager, const std::wstring& name)
	: Tab(pTabManager, name)
{
	m_pTabManagerParent = pTabManager;

	m_hSmallFont = util::CreateStandardUIFont(static_cast<int>(16 * util::GetDPIScale(m_hWndSelf)));
	m_hLargeFont = util::CreateStandardUIFont(static_cast<int>(24 * util::GetDPIScale(m_hWndSelf)));
	m_hHugeFont = util::CreateStandardUIFont(static_cast<int>(28 * util::GetDPIScale(m_hWndSelf)), L"Arial");

	CreateExportControls();
	CreateLVComplementaryControls();
	CreateInsertionControls();

	m_pPeopleList = new ListView(m_hWndSelf, Size(0, 0), Point(0, 0));
	THROW_IF_NULL(m_pPeopleList, "Out of memory");

	m_pPeopleList->AddColumn(L"#", 30);
	m_pPeopleList->AddColumn(L"Ιδιότητα", 160);
	m_pPeopleList->AddColumn(L"Όνομα", 160);
	m_pPeopleList->AddColumn(L"Επώνυμο", 160);
	m_pPeopleList->AddColumn(L"Πατρώνυμο", 160);

	m_pPeopleList->SetColorRule(L"Στέλεχος", RGB(73, 150, 183), 1);
	m_pPeopleList->SetColorRule(L"Κατασκηνωτής/ρια", RGB(0, 0, 255), 1);
	
	LoadPeopleFromDatabaseIntoListView();

	SetWindowSubclass(m_pPeopleList->GetHandle(), PersonListSubclassProc, NULL, reinterpret_cast<DWORD_PTR>(this));
}

void ExportTab::CreateExportControls(void)
{
	HINSTANCE hInstance = GetModuleHandle(NULL);

	m_hFirstnameEdit     = CreateExportEditControl(L"*Όνομα", ES_AUTOHSCROLL);
	m_hLastnameEdit      = CreateExportEditControl(L"*Επώνυμο", ES_AUTOHSCROLL);
	m_hPaternalnameEdit  = CreateExportEditControl(L"*Πατρώνυμο", ES_AUTOHSCROLL);
	CreateExportRadioButtons();
	m_hDepartureDateEdit = CreateExportEditControl(L"*Ημ/νια αποχώρησης");
	m_hDepartureTimeEdit = CreateExportEditControl(L"*00:00");
	m_hArrivalDateEdit   = CreateExportEditControl(L"*Ημ/νια επιστροφής");
	m_hArrivalTimeEdit   = CreateExportEditControl(L"*00:00");
	m_hNoteEdit          = CreateExportEditControl(nullptr, ES_MULTILINE);

	// Enable tabstop for the multiline control
	SetWindowSubclass(m_hNoteEdit, util::MultilineEditTabstopSubclassProcedure, NULL, NULL);

	SendMessage(m_hFirstnameEdit, EM_SETREADONLY, TRUE, 0);
	SendMessage(m_hLastnameEdit, EM_SETREADONLY, TRUE, 0);
	SendMessage(m_hPaternalnameEdit, EM_SETREADONLY, TRUE, 0);

	SetEditCharLimit(m_hDepartureDateEdit, 5);
	SetEditCharLimit(m_hArrivalTimeEdit, 5);
	SetEditCharLimit(m_hDepartureDateEdit, 10);
	SetEditCharLimit(m_hArrivalDateEdit, 10);

	m_hExportButton = CreateWindow(
		L"Button",
		L"                 Άδεια",
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_LEFT | WS_TABSTOP,
		0, 0, 0, 0,
		m_hWndSelf,
		NULL,
		hInstance,
		NULL
	);
	
	m_hTickIcon = LoadIcon(hInstance, MAKEINTRESOURCE(CHECKMARK_ICON));
	m_hPlusIcon = LoadIcon(hInstance, MAKEINTRESOURCE(PLUS_ICON));

	m_hPermaButton = CreateWindow(
		L"Button",
		L"               Οριστική",
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP | BS_LEFT,
		0, 0, 0, 0,
		m_hWndSelf,
		NULL,
		hInstance,
		NULL
	);

	SetWindowFont(m_hPermaButton, m_hSmallFont);
	SetButtonIcon(m_hPermaButton, LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON12)));

	SetWindowFont(m_hExportButton, m_hSmallFont);
	SetButtonIcon(m_hExportButton, LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON13)));
}

void ExportTab::CreateExportRadioButtons(void)
{
	HINSTANCE hInstance = GetModuleHandle(NULL);

	m_hEmployeeButton = CreateWindow(
		L"Button",
		L"Στέλεχος",
		WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP | WS_TABSTOP | WS_DISABLED,
		0, 0, 0, 0,
		m_hWndSelf,
		(HMENU)IDC_EMPLOYEE_RB,
		hInstance,
		NULL
	);

	THROW_IF_NULL(m_hEmployeeButton, "Unable to create export employee radio button [ExportTab]");

	m_hCamperButton = CreateWindow(
		L"Button",
		L"Κατασκηνωτής/ρια",
		WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_TABSTOP | WS_DISABLED,
		0, 0, 0, 0,
		m_hWndSelf,
		(HMENU)IDC_CAMPER_RB,
		hInstance,
		NULL
	);

	THROW_IF_NULL(m_hCamperButton, "Unable to create export camper radio button [ExportTab]");

	SetWindowFont(m_hCamperButton, m_hSmallFont);
	SetWindowFont(m_hEmployeeButton, m_hSmallFont);

	CheckDlgButton(m_hWndSelf, IDC_EMPLOYEE_RB, BST_CHECKED);
}

void ExportTab::CreateLVComplementaryControls(void)
{
	m_hSearchIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(SEARCH_ICON));
	m_hSearchEdit = CreateExportEditControl(L"Αναζήτηση", ES_AUTOHSCROLL);

	SetWindowLong(m_hSearchEdit, GWL_STYLE, GetWindowLong(m_hSearchEdit, GWL_STYLE) & ~WS_TABSTOP);
	SetWindowFont(m_hSearchEdit, m_hLargeFont);

	/////////////////////////////////////////////////

	HINSTANCE hInstance = GetModuleHandle(NULL);

	m_hImportButton = CreateWindow(
		L"Button",
		L"              Επιλογή",
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_LEFT | WS_DISABLED,
		0, 0, 0, 0,
		m_hWndSelf,
		NULL,
		hInstance,
		NULL
	);

	THROW_IF_NULL(m_hImportButton, "Unable to create load button [ExportTab]");

	SetWindowFont(m_hImportButton, m_hSmallFont);

	m_hImportIcon = LoadIcon(hInstance, MAKEINTRESOURCE(LOAD_ICON));
	SetButtonIcon(m_hImportButton, m_hImportIcon);

	m_hInsDeleteButton = CreateWindow(
		L"Button",
		L"              Διαγραφή",
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_LEFT | WS_DISABLED,
		0, 0, 0, 0,
		m_hWndSelf,
		NULL,
		hInstance,
		NULL
	);

	THROW_IF_NULL(m_hInsDeleteButton, "Unable to create delete button [ExportTab]");

	SetWindowFont(m_hInsDeleteButton, m_hSmallFont);

	m_hXIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON2));
	SetButtonIcon(m_hInsDeleteButton, m_hXIcon);
}

void ExportTab::CreateInsertionControls(void)
{
	HINSTANCE hInstance = GetModuleHandle(NULL);

	m_hInsFirstnameEdit = CreateExportEditControl(L"*Όνομα", ES_AUTOHSCROLL);
	m_hInsLastnameEdit = CreateExportEditControl(L"*Επώνυμο", ES_AUTOHSCROLL);
	m_hInsPtrnalnameEdit = CreateExportEditControl(L"*Πατρώνυμο", ES_AUTOHSCROLL);

	m_hInsEmployeeBtn = CreateWindow(
		L"Button",
		L"Στέλεχος",
		WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
		0, 0, 0, 0,
		m_hWndSelf,
		(HMENU)IDC_INS_EMPLOYEE_RB,
		hInstance,
		NULL
	);

	SetWindowFont(m_hInsEmployeeBtn, m_hSmallFont);

	m_hInsCamperBtn = CreateWindow(
		L"Button",
		L"Κατασκηνωτής/ρια",
		WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
		0, 0, 0, 0,
		m_hWndSelf,
		(HMENU)IDC_INS_CAMPER_RB,
		hInstance,
		NULL
	);

	SetWindowFont(m_hInsCamperBtn, m_hSmallFont);

	CheckDlgButton(m_hWndSelf, IDC_INS_EMPLOYEE_RB, BST_CHECKED);

	m_hAddButton = CreateWindow(
		L"Button",
		L"             Προσθήκη",
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_LEFT | WS_TABSTOP,
		0, 0, 0, 0,
		m_hWndSelf,
		NULL,
		hInstance,
		NULL
	);

	SetWindowFont(m_hAddButton, m_hSmallFont);
	SetButtonIcon(m_hAddButton, m_hPlusIcon);

	THROW_IF_NULL(m_hAddButton, "Unable to create add button [ExportTab]");
}

ExportTab::~ExportTab(void)
{
	SAFE_DELETE_GDIOBJ(m_hSmallFont);
	SAFE_DELETE_GDIOBJ(m_hLargeFont);
	SAFE_DELETE_GDIOBJ(m_hHugeFont);

	SAFE_DELETE_GDIOBJ(m_hPlusIcon);
	SAFE_DELETE_GDIOBJ(m_hSearchIcon);
	SAFE_DELETE_GDIOBJ(m_hImportIcon);
	SAFE_DELETE_GDIOBJ(m_hTickIcon);

	SAFE_DELETE(m_pPeopleList);
}

int ExportTab::GetExportEditWidth(void) const noexcept
{
	return static_cast<int>(util::GetDPIScale(m_hWndSelf) * 208);
}

int ExportTab::GetExportEditHeight(void) const noexcept
{
	return static_cast<int>(util::GetDPIScale(m_hWndSelf) * 25);
}

int ExportTab::GetExportButtonHeight(void) const noexcept
{
	return static_cast<int>(util::GetDPIScale(m_hWndSelf) * 40);
}

int ExportTab::GetDistanceBetweenControls(void) const noexcept
{
	return static_cast<int>(util::GetDPIScale(m_hWndSelf) * 16);
}

POINT ExportTab::GetExportGroupPoint(void) const noexcept
{
	POINT ptGroup = {};
	ptGroup.x = static_cast<LONG>(40 * util::GetDPIScale(m_hWndSelf));
	ptGroup.y = static_cast<LONG>(80 * util::GetDPIScale(m_hWndSelf));
	return ptGroup;
}

void ExportTab::OnResize(int width, int height)
{
	UpdateNameEditControlsPos();
	UpdateTimeDateControlsPos();
	UpdateOtherExportControlsPos();

	if (m_pPeopleList)
	{
		const double dpiScale = util::GetDPIScale(m_hWndSelf);

		const int distFromEdge = static_cast<int>(80 * dpiScale);
		const int widthOfExportArea = static_cast<int>(300 * dpiScale);
		const int distBetweenLVAndButtons = static_cast<int>(40 * dpiScale);
		// -300 = Width of export area in pixels
		// -
		const int iLVWidth = ((int)GetWidth()) - widthOfExportArea - distBetweenLVAndButtons - distFromEdge * 2 - this->GetExportEditWidth();
		

		// 300 * dpiScale = width of export area in pixels
		m_pPeopleList->SetPos(300 * dpiScale + distFromEdge, 100);
		m_pPeopleList->SetSize(iLVWidth, height - distFromEdge * 2);

		UpdateSearchbarPos();
		UpdateLVComplementaryCtrlPos();
		UpdateInsertionControlsPos();
	}
}

void ExportTab::UpdateNameEditControlsPos(void)
/*++
* 
* Routine Description:
* 
*	Places the first 3 edit controls, that are used for the names, one below
*	the other, such that their top left point is the group point.
* 
* Arguments:
* 
*	None.
* 
--*/
{
	const POINT ptGroup = GetExportGroupPoint();
	const INT cxEdit = GetExportEditWidth();
	const INT cyEdit = GetExportEditHeight();
	const INT dist = GetDistanceBetweenControls();

	const HWND m_hEditControls[] = {
		m_hFirstnameEdit,
		m_hLastnameEdit,
		m_hPaternalnameEdit,
	};

	for (int i = 0; i < ARRAY_SIZE(m_hEditControls); ++i)
	{
		SetWindowPos(
			m_hEditControls[i],
			NULL,
			ptGroup.x,
			ptGroup.y + i * (cyEdit + dist),
			cxEdit,
			cyEdit,
			SWP_NOZORDER
		);
	}
}

void ExportTab::UpdateTimeDateControlsPos(void)
/*++
*
* Routine Description:
*
*	Places the date and time edit controls next to each other.
*
* Arguments:
*
*	None.
*
--*/
{
	const POINT ptGroup = GetExportGroupPoint();
	const INT cxEdit = GetExportEditWidth();
	const INT cyEdit = GetExportEditHeight();
	const INT dist = GetDistanceBetweenControls();

	const double dpiScale = util::GetDPIScale(m_hWndSelf);

	SetWindowPos(
		m_hDepartureDateEdit,
		NULL,
		ptGroup.x,
		ptGroup.y + 3 * (cyEdit + dist) + (int)(dpiScale * 24),
		cxEdit * 3 / 4,
		cyEdit,
		SWP_NOZORDER
	);

	SetWindowPos(
		m_hDepartureTimeEdit,
		NULL,
		ptGroup.x + cxEdit * 3 / 4 + 5,
		ptGroup.y + 3 * (cyEdit + dist) + (int)(dpiScale * 24),
		cxEdit / 4 - 5,
		cyEdit,
		SWP_NOZORDER
	);

	SetWindowPos(
		m_hArrivalDateEdit,
		NULL, 
		ptGroup.x,
		ptGroup.y + 4 * (cyEdit + dist) + (int)(dpiScale * 24),
		cxEdit * 3 / 4, 
		cyEdit, 
		SWP_NOZORDER
	);

	SetWindowPos(
		m_hArrivalTimeEdit,
		NULL,
		ptGroup.x + cxEdit * 3 / 4 + 5,
		ptGroup.y + 4 * (cyEdit + dist) + (int)(dpiScale * 24),
		cxEdit / 4 - 5,
		cyEdit,
		SWP_NOZORDER
	);

	SetWindowPos(
		m_hEmployeeButton,
		NULL,
		ptGroup.x,
		ptGroup.y + (cyEdit + dist) * 3 - 8,
		static_cast<int>(80 * dpiScale),
		static_cast<int>(16 * dpiScale),
		SWP_NOZORDER
	);

	SetWindowPos(
		m_hCamperButton,
		NULL,
		ptGroup.x + static_cast<int>(80 * dpiScale),
		ptGroup.y + (cyEdit + dist) * 3 - 8,
		static_cast<int>(128 * dpiScale),
		static_cast<int>(16 * dpiScale),
		SWP_NOZORDER
	);
}

void ExportTab::UpdateOtherExportControlsPos(void)
/*++
*
* Routine Description:
*
*	Places the rest of the export controls
*
* Arguments:
*
*	None.
*
--*/
{
	const POINT ptGroup = GetExportGroupPoint();
	const INT cxEdit = GetExportEditWidth();
	const INT cyEdit = GetExportEditHeight();
	const INT dist = GetDistanceBetweenControls();

	const double dpiScale = util::GetDPIScale(m_hWndSelf);

	SetWindowPos(
		m_hNoteEdit,
		NULL,
		ptGroup.x,
		ptGroup.y + 5 * (cyEdit + dist) + (int)(dpiScale * 24),
		cxEdit,
		cyEdit * 4,
		SWP_NOZORDER
	);

	SetWindowPos(
		m_hExportButton,
		NULL,
		ptGroup.x,
		ptGroup.y + 6 * (cyEdit + dist) + cyEdit * 3 + (int)(dpiScale * 24),
		cxEdit,
		GetExportButtonHeight(),
		SWP_NOZORDER
	);

	SetWindowPos(
		m_hPermaButton,
		NULL,
		ptGroup.x,
		ptGroup.y + 7 * (cyEdit + dist) + cyEdit * 3 + (int)(dpiScale * 32),
		cxEdit,
		GetExportButtonHeight(),
		SWP_NOZORDER
	);
}

void ExportTab::UpdateSearchbarPos(void)
{
	const double dpiScale = util::GetDPIScale(m_hWndSelf);

	const int iSearchWidth           = static_cast<int>(240 * dpiScale);
	const int iSearchEditHeight      = static_cast<int>(32 * dpiScale);
	const int iGapBetweenEditAndList = static_cast<int>(16 * dpiScale);

	SetWindowPos(
		m_hSearchEdit,
		NULL,
		m_pPeopleList->GetX() + (m_pPeopleList->GetWidth() - iSearchWidth) / 2,
		m_pPeopleList->GetY() - iSearchEditHeight - iGapBetweenEditAndList,
		iSearchWidth,
		iSearchEditHeight,
		SWP_NOZORDER
	);
}

void ExportTab::UpdateLVComplementaryCtrlPos(void)
{
	const int cyEdit = GetExportEditHeight();
	const int cyButton = GetExportButtonHeight();
	const int dist = GetDistanceBetweenControls();

	SetWindowPos(
		m_hImportButton,
		NULL,
		m_pPeopleList->GetX() + (int)m_pPeopleList->GetWidth() + (int)(40 * util::GetDPIScale(m_hWndSelf)),
		m_pPeopleList->GetY() + (cyEdit + dist) * 4 + cyButton + dist / 2,
		GetExportEditWidth(),
		cyButton,
		SWP_NOZORDER
	);
}

void ExportTab::UpdateInsertionControlsPos(void)
{
	const int cxEdit = GetExportEditWidth();
	const int cyEdit = GetExportEditHeight();

	const int dist = GetDistanceBetweenControls();

	POINT ptGroup = {};
	ptGroup.x = m_pPeopleList->GetX() + m_pPeopleList->GetWidth() + (int)(40 * util::GetDPIScale(m_hWndSelf));
	ptGroup.y = m_pPeopleList->GetY();

	SetWindowPos(
		m_hInsFirstnameEdit,
		NULL,
		ptGroup.x,
		ptGroup.y,
		cxEdit,
		cyEdit,
		SWP_NOZORDER
	);

	SetWindowPos(
		m_hInsLastnameEdit,
		NULL,
		ptGroup.x,
		ptGroup.y + cyEdit + dist,
		cxEdit,
		cyEdit,
		SWP_NOZORDER
	);

	SetWindowPos(
		m_hInsPtrnalnameEdit,
		NULL,
		ptGroup.x,
		ptGroup.y + (cyEdit + dist) * 2,
		cxEdit,
		cyEdit,
		SWP_NOZORDER
	);

	const double dpiScale = util::GetDPIScale(m_hWndSelf);

	SetWindowPos(
		m_hInsEmployeeBtn,
		NULL,
		ptGroup.x,
		ptGroup.y + (cyEdit + dist) * 3 - 8,
		static_cast<int>(80 * dpiScale),
		static_cast<int>(16 * dpiScale),
		SWP_NOZORDER
	);

	SetWindowPos(
		m_hInsCamperBtn,
		NULL,
		ptGroup.x + static_cast<int>(80 * dpiScale),
		ptGroup.y + (cyEdit + dist) * 3 - 8,
		static_cast<int>(128 * dpiScale),
		static_cast<int>(16 * dpiScale),
		SWP_NOZORDER
	);

	const int iExportButtonHeight = GetExportButtonHeight();
	
	SetWindowPos(
		m_hAddButton,
		NULL,
		ptGroup.x,
		ptGroup.y + (cyEdit + dist) * 4,
		cxEdit,
		iExportButtonHeight,
		SWP_NOZORDER
	);

	SetWindowPos(
		m_hInsDeleteButton,
		NULL,
		ptGroup.x,
		ptGroup.y + (cyEdit + dist) * 4 + (iExportButtonHeight + dist / 2) * 2,
		cxEdit,
		iExportButtonHeight,
		SWP_NOZORDER
	);
}

void ExportTab::Draw(HDC hDC)
{
	const POINT ptGroup = GetExportGroupPoint();
	const INT cxEdit = GetExportEditWidth();
	const INT cyEdit = GetExportEditHeight();
	const INT dist = GetDistanceBetweenControls();

	SetBkMode(hDC, TRANSPARENT);
	SelectObject(hDC, m_hHugeFont);

	SIZE textSize = {};
	GetTextExtentPoint32(hDC, L"ΕΚΔΟΣΗ", 6, &textSize);

	TextOut(
		hDC,
		ptGroup.x + (cxEdit - textSize.cx) / 2,
		ptGroup.y - textSize.cy - dist / 2,
		L"ΕΚΔΟΣΗ",
		6
	);

	if (m_pPeopleList)
	{
		GetTextExtentPoint32(hDC, L"ΠΡΟΣΘΗΚΗ", 8, &textSize);

		const int iStartingX = m_pPeopleList->GetX() + m_pPeopleList->GetWidth() + (int)(40 * util::GetDPIScale(m_hWndSelf));

		TextOut(
			hDC,
			iStartingX + (GetExportEditWidth() - textSize.cx) / 2,
			m_pPeopleList->GetY() - textSize.cy - 20,
			L"ΠΡΟΣΘΗΚΗ",
			8
		);
	}

	DrawSearchIcon(hDC);
}

void ExportTab::DrawSearchIcon(HDC hDC)
{
	if (m_pPeopleList)
	{
		const double dpiScale = util::GetDPIScale(m_hWndSelf);

		const int iSearchEditWidth = static_cast<int>(240 * dpiScale);
		const int iSearchEditHeight = static_cast<int>(32 * dpiScale);
		const int iGapBetweenEditAndList = static_cast<int>(16 * dpiScale);

		DrawIconEx(
			hDC,
			m_pPeopleList->GetX() + (m_pPeopleList->GetWidth() - iSearchEditWidth) / 2 - 52,
			m_pPeopleList->GetY() - iSearchEditHeight - iGapBetweenEditAndList,
			m_hSearchIcon,
			0,
			0,
			NULL,
			NULL,
			DI_NORMAL
		);
	}
}

void ExportTab::Draw(ID2D1RenderTarget* pRenderTarget)
{
	ID2D1SolidColorBrush* pBrush = GetSolidColorBrush();
	
	if (pBrush)
	{
		const D2D1_COLOR_F crBeforeCall = pBrush->GetColor();

		pBrush->SetColor(D2D1::ColorF(RGB_D2D(150, 150, 150)));

		pRenderTarget->DrawLine(
			D2D1::Point2F(300 * util::GetDPIScale(m_hWndSelf), 0),
			D2D1::Point2F(300 * util::GetDPIScale(m_hWndSelf), static_cast<FLOAT>(GetHeight())),
			m_pSolidColorBrush
		);

		pBrush->SetColor(crBeforeCall);
	}
}

void ExportTab::OnCommand(HWND hWnd)
{
	if (hWnd == m_hSearchEdit)
	{
		if (m_pPeopleList)
		{
			wchar_t buffer[MAX_NOTES_LENGTH];
			GetWindowText(m_hSearchEdit, buffer, MAX_NOTES_LENGTH - 1);

			m_pPeopleList->ApplyRowFilter(buffer);
		}
	}

	else if (hWnd == m_hImportButton)
	{
		ImportPersonInformationToExportControls();
	}

	else if (hWnd == m_hAddButton)
	{
		AddPersonInformationUsingControlText();
	}

	else if (hWnd == m_hExportButton)
	{
		ExportTicketFromEnteredData(false);
	}

	else if (hWnd == m_hPermaButton)
	{
		ExportTicketFromEnteredData(true);
	}

	else if (hWnd == m_hInsDeleteButton)
	{
		DeleteSelectedRow();
	}
}

void ExportTab::ImportPersonInformationToExportControls(void)
{
	if (!m_pPeopleList || !m_pPeopleList->IsSomeRowSelected())
	{
		return;
	}

	const std::vector<std::wstring> personInfo = m_pPeopleList->GetSelectedRow();

	iSelectedPersonId = std::stoi(personInfo[0]);

	SetWindowText(m_hFirstnameEdit,    personInfo[LV_FNAME_INDEX].c_str());
	SetWindowText(m_hLastnameEdit,     personInfo[LV_LNAME_INDEX].c_str());
	SetWindowText(m_hPaternalnameEdit, personInfo[LV_PNAME_INDEX].c_str());

	if (personInfo[LV_ROLE_INDEX] == L"Στέλεχος") {
		CheckDlgButton(m_hWndSelf, IDC_EMPLOYEE_RB, BST_CHECKED);
		CheckDlgButton(m_hWndSelf, IDC_CAMPER_RB, BST_UNCHECKED);
	} else {
		CheckDlgButton(m_hWndSelf, IDC_EMPLOYEE_RB, BST_UNCHECKED);
		CheckDlgButton(m_hWndSelf, IDC_CAMPER_RB, BST_CHECKED);
	}

	const std::wstring todayDate = util::GetLocalDate();
	const std::wstring nowTime = util::GetLocalTime();

	SetWindowText(m_hDepartureDateEdit, todayDate.c_str());
	SetWindowText(m_hArrivalDateEdit, todayDate.c_str());

	SetWindowText(m_hDepartureTimeEdit, nowTime.c_str());
	SetWindowText(m_hArrivalTimeEdit, nowTime.c_str());

	SetFocus(m_hArrivalDateEdit);

	m_pPeopleList->UnselectSelectedRow();
}

void ExportTab::DeleteSelectedRow(void)
{
	if (m_pPeopleList && m_pPeopleList->IsSomeRowSelected())
	{
		if (MessageBox(NULL,
			L"Είστε βέβαιοι ότι θέλετε να διαγράψετε την καταχώρηση; Η ενέργεια αυτή δεν είναι αντιστρέψιμη.",
			L"Επιβεβαίωση", MB_OKCANCEL | MB_ICONEXCLAMATION) == IDOK)
		{
			const int iSelectedRowIndex = m_pPeopleList->GetSelectedRowIndex();
			const int iPersonID = std::stoi(m_pPeopleList->GetCellContent(iSelectedRowIndex, 0));

			// Very important that this is sent before db::DeletePerson
			SendMessage(m_pTabManagerParent->GetTab(0)->GetHandle(), WM_DELETE_PERSON_TICKET, iPersonID, 0);

			db::DeletePerson(iPersonID);

			m_pPeopleList->RemoveDisplayedRow(iSelectedRowIndex);
		}
	}
}

LRESULT ExportTab::OnCustomMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_ROW_SELECTED:
		EnableWindow(m_hImportButton, TRUE);
		EnableWindow(m_hInsDeleteButton, TRUE);
		break;

	case WM_ROW_UNSELECTED:
		EnableWindow(m_hImportButton, FALSE);
		EnableWindow(m_hInsDeleteButton, FALSE);
		break;

	case WM_IMPORT_PERSON:
		if (m_pPeopleList && m_pPeopleList->IsSomeRowSelected())
		{
			ImportPersonInformationToExportControls();
		}
		break;

	case WM_PREPARE_FOR_EXPORT:
		SetFocusToAppropriateControl();
		break;

	case WM_GET_SEARCH_HANDLE:
		return (LRESULT)m_hSearchEdit;
	}

	return 0;
}

void ExportTab::ExportTicketFromEnteredData(bool perma)
/*++
* 
* 
* 
--*/
{
	util::Date departureDate, arrivalDate;

	const HWND hRequiredFillEditControls[] = {
		m_hFirstnameEdit,
		m_hLastnameEdit,
		m_hPaternalnameEdit,
		m_hDepartureDateEdit,
		m_hDepartureTimeEdit,
		m_hArrivalDateEdit,
		m_hArrivalTimeEdit
	};

	for (HWND hWnd : hRequiredFillEditControls) 
	{
		if (GetWindowTextLength(hWnd) == 0)
		{
			MessageBox(m_hWndSelf, L"Πρέπει να συμπληρώσετε όλα τα πεδιά που έχουν αστερίσκο.", L"Σφάλμα", MB_OK | MB_ICONERROR);
			return;
		}
	}

	if (VerifyTimes() && VerifyDates(&departureDate, &arrivalDate))
	{
		db::Ticket ticket = std::move(CreateTicketFromEnteredData(departureDate, arrivalDate, perma));

		if (TicketRefersToExistingPerson(ticket))
		{
			SendMessage(m_pTabManagerParent->GetTab(0)->GetHandle(), WM_EXPORT_TICKET, NULL, (LPARAM)&ticket);

			for (HWND hWnd : hRequiredFillEditControls)
			{
				SetWindowText(hWnd, L"");
			}

			SetWindowText(m_hNoteEdit, L"");
		}

		else
		{
			MessageBox(m_hWndSelf,
				L"Το πρόσωπο για το οποίο προσπαθείτε να εκδόσετε άδεια δεν έχει καταχωρηθεί στη βάση"
				L" δεδομένων. Προσθέστε το και προσπαθήστε ξανά",
				L"Σφάλμα",
				MB_OK | MB_ICONERROR
			);
		}
	}
}

bool ExportTab::VerifyTimes(void)
/*++
* 
* Routine Description:
* 
*	Ensures that the times in the edit controls are in the correct form.
* 
* Arguments:
* 
*	None.
* 
* Return Value:
* 
*	True if they were both in the correct form, false if otherwise.
* 
--*/
{
	//  Checking if the departure time is in valid form
	wchar_t buffer[8];
	GetWindowText(m_hDepartureTimeEdit, buffer, 8 - 1);

	if (!util::IsMilitaryTime(buffer))
	{
		MessageBox(m_hWndSelf, L"Ο χρόνος αποχώρησης δεν είναι σε έγκυρη μορφή.", L"Σφάλμα", MB_OK | MB_ICONERROR);
		return false;
	}

	// Checking if the arrival time is in valid form
	GetWindowText(m_hArrivalTimeEdit, buffer, 8 - 1);

	if (!util::IsMilitaryTime(buffer))
	{
		MessageBox(m_hWndSelf, L"Ο χρόνος επιστροφής δεν είναι σε έγκυρη μορφή.", L"Σφάλμα", MB_OK | MB_ICONERROR);
		return false;
	}

	return true;
}

bool ExportTab::VerifyDates(util::Date* depart_out, util::Date* arrival_out)
/*++
*
* Routine Description:
*
*	Ensures that the dates in the edit controls are in the correct form.
*
* Arguments:
*
*	None.
*
* Return Value:
*
*	True if they were both in the correct form, false if otherwise.
*
--*/
{
	wchar_t buffer[16];

	// Checking if the departure date is in valid form
	GetWindowText(m_hDepartureDateEdit, buffer, 16 - 1);

	*depart_out = util::ConvertStringToDate(buffer);

	if (!util::IsValidDate(*depart_out))
	{
		MessageBox(m_hWndSelf, L"Η ημερομηνία αποχώρησης δεν είναι έγκυρη", L"Σφάλμα", MB_OK | MB_ICONERROR);
		return false;
	}

	// Checking if the arrival date is in valid form
	GetWindowText(m_hArrivalDateEdit, buffer, 16 - 1);

	*arrival_out = util::ConvertStringToDate(buffer);

	if (!util::IsValidDate(*arrival_out))
	{
		MessageBox(m_hWndSelf, L"Η ημερομηνία επιστροφής δεν είναι έγκυρη", L"Σφάλμα", MB_OK | MB_ICONERROR);
		return false;
	}

	return true;
}

bool ExportTab::TicketRefersToExistingPerson(const db::Ticket& ticket)
{
	return true;
}

std::vector<std::wstring> ExportTab::CreateTicketFromEnteredData(util::Date& departureDate, util::Date& arrivalDate, bool perma)
{
	wchar_t buffer[MAX_NOTES_LENGTH];

	std::vector<std::wstring> ticket;
	ticket.emplace_back(L"Αναμονή");

	ticket.emplace_back(L"0");

	ticket.emplace_back(std::to_wstring(iSelectedPersonId));

	if (IsDlgButtonChecked(m_hWndSelf, IDC_EMPLOYEE_RB)) {
		ticket.emplace_back(L"Στέλεχος");
	} else {
		ticket.emplace_back(L"Κατασκηνωτής/ρια");
	}

	/// Append first name

	CharUpperBuff(buffer, GetWindowText(m_hFirstnameEdit, buffer, MAX_NOTES_LENGTH - 1));
	ticket.emplace_back(buffer);

	/// Append last name

	CharUpperBuff(buffer, GetWindowText(m_hLastnameEdit, buffer, MAX_NOTES_LENGTH - 1));
	ticket.emplace_back(buffer);

	/// Append paternal name

	CharUpperBuff(buffer, GetWindowText(m_hPaternalnameEdit, buffer, MAX_NOTES_LENGTH - 1));
	ticket.emplace_back(buffer);

	/// Append departure date

	swprintf_s(buffer, 127, L"%02d/%02d/%04d", departureDate.day, departureDate.month, departureDate.year);
	ticket.emplace_back(buffer);

	/// Append Departure time

	GetWindowText(m_hDepartureTimeEdit, buffer, MAX_NOTES_LENGTH - 1);
	ticket.emplace_back(buffer);

	if (ticket.back().length() == 4)
		ticket.back().insert(ticket.back().begin(), L'0');

	if (!perma)
	{
		// Append arrival date

		swprintf_s(buffer, 127, L"%02d/%02d/%04d", arrivalDate.day, arrivalDate.month, arrivalDate.year);
		ticket.emplace_back(buffer);

		// Append arrival time

		GetWindowText(m_hArrivalTimeEdit, buffer, MAX_NOTES_LENGTH - 1);
		ticket.emplace_back(buffer);

		if (ticket.back().length() == 4)
			ticket.back().insert(ticket.back().begin(), L'0');
	}

	else
	{
		ticket.emplace_back(L"-");
		ticket.emplace_back(L"-");
	}

	// Append actual arrival time

	ticket.emplace_back(L"-");

	// Append notes

	GetWindowText(m_hNoteEdit, buffer, MAX_NOTES_LENGTH - 1);
	ticket.emplace_back(buffer);

	return ticket;
}

void ExportTab::AddPersonInformationUsingControlText(void)
/*++
* 
* Routine Description:
* 
*	This function, checks the validity of the data entered in the insertion controls,
*	adds the specified person to the database (if they don't already exist) and finally
*	add the person to the list view.
* 
* Arguments:
* 
*	None.
* 
--*/
{
	wchar_t smallBuf[32];

	const HWND hWndInfoControls[] = {
		m_hInsFirstnameEdit,
		m_hInsLastnameEdit,
		m_hInsPtrnalnameEdit
	};

	for (HWND hWnd : hWndInfoControls)
	{
		if (GetWindowTextLength(hWnd) == 0)
		{
			MessageBox(m_hWndSelf, L"Πρέπει να συμπληρωθούν όλα τα πεδιά που έχουν αστερίσκο.", L"Σφάλμα", MB_OK | MB_ICONERROR);
			return;
		}

		util::TrimEditControlContent(hWnd);
	}

	db::Person information;
	// 3 x Get text from the edit control, convert it to uppercase and store it into the specified buffer of the Person struct
	CharUpperBuff(information.firstname,  GetWindowText(m_hInsFirstnameEdit,  information.firstname,  ARRAY_SIZE(information.firstname)));
	CharUpperBuff(information.lastname,   GetWindowText(m_hInsLastnameEdit,   information.lastname,   ARRAY_SIZE(information.lastname)));
	CharUpperBuff(information.fathername, GetWindowText(m_hInsPtrnalnameEdit, information.fathername, ARRAY_SIZE(information.fathername)));

	// We'll get the handle of the radio button that is selected...
	const HWND hCheckedButton = (IsDlgButtonChecked(m_hWndSelf, IDC_INS_EMPLOYEE_RB) == BST_CHECKED) ? m_hInsEmployeeBtn : m_hInsCamperBtn;

	// ... get its text ...
	GetWindowText(hCheckedButton, smallBuf, ARRAY_SIZE(smallBuf));

	// ... and match it to the appropriate integer that matches to that role
	information.role = util::StringToEnum(smallBuf);

	// Realistically this shouldn't happen unless I either make a terrible mistake,
	// or somebody uses anothr program to change the radio button's text
	if (information.role == util::PersonRole::INVALID)
	{
		MessageBox(m_hWndSelf, L"Πρέπει να επιλέξετε μία έγκυρη ιδιότητα", L"Σφάλμα", MB_OK | MB_ICONERROR);
		return;
	}
	
	// The UpperChar function doesn't convert 'ς' to 'Σ'
	// so we have to make this conversion ourselves.
	ConvertSpecialSigmasToCapital(information);

	AddPersonToDatabase(information);

	// At this point we do not know which ID was assigned to the person
	// by the DBMS, so we have to use a query to find out
	information.id = db::GetLastInsertedRowId();

	// Now that we have all the information we can add it to the list.
	AddPersonToListView(information);

	for (HWND hWnd : hWndInfoControls)
	{
		SetWindowText(hWnd, L"");
	}
}

bool ExportTab::PersonExistsInDatabase(const db::Person& info)
/*++
* 
* Routine Description:
* 
*	Checks whether there exists an entry in the database that matches the given information.
* 
* Arguments:
* 
*	info - Reference to struct containing the person's information.
* 
--*/
{
	return db::GetPersonID(info) != -1;
}

void ExportTab::AddPersonToDatabase(const db::Person& info)
/*++
* 
* Routine Description:
* 
*	Adds an entry to the database with the specified information.
* 
*	This function doesn't handle the case where the person already exists.
*	If that happens, the behavior of the program depends on the implementation
*	of the database functions.
* 
*	Use the PersonExistsInDatabase function before calling this function to ensure
*	that the program behaves predictably
* 
* Arguments:
* 
*	info - Reference to a struct that contains the person's information.
* 
--*/
{
	db::InsertPersonToDatabase(info);
}

void ExportTab::AddPersonToListView(const db::Person& info)
/*++
* 
* Routine Description:
* 
*	Adds an entry of a person to the matching list view.
* 
*	This function doesn't stop duplicates from being inserted.
* 
* Arguments:
* 
*	info - Reference to a struct that contains the person's information.
* 
--*/
{
	if (m_pPeopleList)
	{
		std::vector<std::wstring> entries;
		entries.emplace_back(std::to_wstring(info.id));
		entries.emplace_back(util::EnumToString(info.role));
		entries.emplace_back(info.firstname);
		entries.emplace_back(info.lastname);
		entries.emplace_back(info.fathername);

		m_pPeopleList->AddRow(entries);
	}
}

void ExportTab::ConvertSpecialSigmasToCapital(db::Person& info)
/*++
* 
* Routine Description:
* 
*	Looks for any 'ς' characters in the info strings, and converts it to a 'Σ'.
* 
*	This is needed because CharUpperBuff doesn't do this, so we have to handle this ourselves.
* 
* Arguments:
* 
*	info - Reference to a struct that contains the person's information.
* 
--*/
{
	auto convertSpecialSigmasInString = [](wchar_t* text) 
	{
		const int length = lstrlenW(text);

		for (int i = 0; i < length; ++i) 
		{
			if (text[i] == L'ς') 
			{
				text[i] = L'Σ';
			}
		}
	};

	convertSpecialSigmasInString(info.firstname);
	convertSpecialSigmasInString(info.lastname);
	convertSpecialSigmasInString(info.fathername);
}

HWND ExportTab::CreateExportEditControl(const wchar_t* lpszPlaceholder, DWORD dwFlags)
/*++
*
* Routine Description:
*
*	Creates an edit control with the specified parameters, sets the font and the placeholder and does some error checking.
*
* Arguments:
*
*	lpszPlaceholder - Pointer to the text of the placeholder. If this is null no placeholder is used.
*	dwFlags - Additional window styles that may be used, use 0 if none are wanted.
*
* Return Value:
*
*	The handle to the created edit control.
*
--*/
{
	HWND hEditControl = CreateWindow(
		L"Edit",
		L"",
		WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | dwFlags,
		0, 0, 0, 0,
		m_hWndSelf,
		NULL,
		GetModuleHandle(NULL),
		NULL
	);

	THROW_IF_NULL(hEditControl, "Unable to create edit control [ExportTab]");

	SetWindowFont(hEditControl, m_hSmallFont);

	if (lpszPlaceholder != nullptr)
	{
		SetPlaceholderText(hEditControl, lpszPlaceholder);
	}

	return hEditControl;
}

void ExportTab::OnSwitchedToSelf(void)
{
	HWND hSearchEdit = (HWND)SendMessage(m_pTabManagerParent->GetTab(2)->GetHandle(), WM_GET_SEARCH_HANDLE, NULL, NULL);

	int length = GetWindowTextLength(hSearchEdit);

	wchar_t* lpszBuffer = new wchar_t[length + 1];

	THROW_IF_NULL(lpszBuffer, "Out of memory");

	GetWindowText(hSearchEdit, lpszBuffer, length + 1);
	SetWindowText(m_hSearchEdit, lpszBuffer);

	delete[] lpszBuffer;
}

void ExportTab::LoadPeopleFromDatabaseIntoListView(void)
/*++
* 
* Routine Description:
* 
*	Loads all the stored people from the database file into the list view.
* 
*	This is called upon first initializing the program.
* 
--*/
{
	std::vector<db::Person> peopleList;
	db::LoadPeopleFromDatabase(peopleList);

	for (db::Person& person : peopleList)
	{
		std::vector<std::wstring> row;
		row.emplace_back(std::to_wstring(person.id));
		row.emplace_back(util::EnumToString(person.role));
		row.emplace_back(person.firstname);
		row.emplace_back(person.lastname);
		row.emplace_back(person.fathername);

		m_pPeopleList->AddRow(row);
	}
}

void ExportTab::SetFocusToAppropriateControl(void)
{
	if (m_pPeopleList)
	{
		if (m_pPeopleList->GetRowCount() == 0)
		{
			SetFocus(m_hInsFirstnameEdit);
		}

		else
		{
			SetFocus(m_pPeopleList->GetHandle());
		}
	}
}