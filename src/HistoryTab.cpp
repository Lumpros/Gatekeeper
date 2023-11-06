#include "HistoryTab.h"
#include "resource.h"
#include "TabManager.h"

#include <cassert>
#include <stdexcept>

HistoryTab::HistoryTab(TabManager* pManager, const std::wstring& name)
	: Tab(pManager, name)
{
	m_pTabManager = pManager;

	m_hSmallFont = util::CreateStandardUIFont(static_cast<int>(16 * util::GetDPIScale(m_hWndSelf)));
	THROW_IF_NULL(m_hSmallFont, "Unable to create font [HistoryTab]");

	m_hLargeFont = util::CreateStandardUIFont(static_cast<int>(24 * util::GetDPIScale(m_hWndSelf)));
	THROW_IF_NULL(m_hLargeFont, "Unable to create font [HistoryTab]");

	InitPersonList();
	InitHistoryList();

	HINSTANCE hInstance = GetModuleHandle(NULL);

	m_hSearchIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON5));
	m_hArrowIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON9));

	m_hGetHistoryButton = CreateWindow(
		L"Button",
		L" ",
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_DISABLED,
		0, 0, 0, 0,
		m_hWndSelf,
		NULL,
		GetModuleHandle(NULL),
		NULL
	);

	SetButtonIcon(m_hGetHistoryButton, m_hArrowIcon);

	m_hPersonSearchWnd = CreateSearchEdit(hInstance);
	m_hTicketSearchWnd = CreateSearchEdit(hInstance);	
}

HistoryTab::~HistoryTab(void)
{
	SAFE_DELETE(m_pPersonList);
	SAFE_DELETE(m_pHistoryList);

	SAFE_DELETE_GDIOBJ(m_hSearchIcon);
	SAFE_DELETE_GDIOBJ(m_hSmallFont);
	SAFE_DELETE_GDIOBJ(m_hLargeFont);
	SAFE_DELETE_GDIOBJ(m_hArrowIcon);
}

void HistoryTab::InitHistoryList(void)
{
	m_pPersonList = new ListView(m_hWndSelf, Size(0, 0), Point(0, 0));
	THROW_IF_NULL(m_pPersonList, "Out of memory");

	m_pPersonList->SetColorRule(L"Στέλεχος", RGB(73, 150, 183), 1);
	m_pPersonList->SetColorRule(L"Κατασκηνωτής/ρια", RGB(0, 0, 255), 1);
}

void HistoryTab::InitPersonList(void)
{
	m_pHistoryList = new ListView(m_hWndSelf, Size(0, 0), Point(0, 0));
	THROW_IF_NULL(m_pHistoryList, "Out of memory");

	m_pHistoryList->SetColorRule(L"Ενεργή", RGB(0, 200, 0), 1);
	m_pHistoryList->SetColorRule(L"Ανενεργή", RGB(200, 0, 0), 1);

	m_pHistoryList->SetColorRule(L"Στέλεχος", RGB(73, 150, 183), 2);
	m_pHistoryList->SetColorRule(L"Κατασκηνωτής/ρια", RGB(0, 0, 255), 2);

	m_pHistoryList->AddColumn(L"#", 30);
	m_pHistoryList->AddColumn(L"Κατάσταση", 100);
	m_pHistoryList->AddColumn(L"Ιδιότητα", 140);
	m_pHistoryList->AddColumn(L"Όνομα", 140);
	m_pHistoryList->AddColumn(L"Επώνυμο", 140);
	m_pHistoryList->AddColumn(L"Πατρώνυμο", 140);
	m_pHistoryList->AddColumn(L"Ημ/νια Αποχώρησης", 140);
	m_pHistoryList->AddColumn(L"Δηλ. Ώρα Αποχ.", 100);
	m_pHistoryList->AddColumn(L"Ημ/νια Επιστροφής", 140);
	m_pHistoryList->AddColumn(L"Δηλ. Ώρα Επ.", 100);
	m_pHistoryList->AddColumn(L"Ώρα Επιστροφής", 100);
	m_pHistoryList->AddColumn(L"Σημείωση", 200);
}

void HistoryTab::GetAccessToLoadedUserData(ExportTab* pExportTab)
{
	assert(pExportTab);

	m_pPersonList->Mirror(pExportTab->m_pPeopleList);
}

void HistoryTab::OnResize(int width, int height)
{
	const int distanceFromEdges = static_cast<int>(80 * util::GetDPIScale(m_hWndSelf));
	const int iViewWidth = ((int)(GetWidth()) - distanceFromEdges * 3) / 2;

	const double dpiScale = util::GetDPIScale(m_hWndSelf);

	const int iSearchEditWidth = static_cast<int>(240 * dpiScale);
	const int iSearchEditHeight = static_cast<int>(32 * dpiScale);
	const int iGapBetweenEditAndList = static_cast<int>(16 * dpiScale);

	if (m_pPersonList)
	{
		m_pPersonList->SetPos(distanceFromEdges, distanceFromEdges);

		m_pPersonList->SetSize(iViewWidth, (int)(GetHeight()) - distanceFromEdges * 2);

		SetWindowPos(
			m_hGetHistoryButton,
			NULL,
			m_pPersonList->GetX() + m_pPersonList->GetWidth() + 5,
			m_pPersonList->GetY() + distanceFromEdges,
			distanceFromEdges - 10,
			m_pPersonList->GetHeight() - distanceFromEdges * 2,
			SWP_NOZORDER
		);

		SetWindowPos(
			m_hPersonSearchWnd,
			NULL,
			m_pPersonList->GetX() + (iViewWidth - iSearchEditWidth) / 2,
			m_pPersonList->GetY() - iSearchEditHeight - iGapBetweenEditAndList,
			iSearchEditWidth,
			iSearchEditHeight,
			SWP_NOZORDER
		);
	}

	if (m_pHistoryList)
	{
		m_pHistoryList->SetSize(iViewWidth, (int)(GetHeight()) - distanceFromEdges * 2);

		m_pHistoryList->SetPos(
			GetWidth() - m_pHistoryList->GetWidth() - distanceFromEdges, distanceFromEdges
		);

		SetWindowPos(
			m_hTicketSearchWnd,
			NULL,
			m_pHistoryList->GetX() + (iViewWidth - iSearchEditWidth) / 2,
			m_pHistoryList->GetY() - iSearchEditHeight - iGapBetweenEditAndList,
			iSearchEditWidth,
			iSearchEditHeight,
			SWP_NOZORDER
		);
	}
}

void HistoryTab::OnCommand(HWND hWnd)
{
	wchar_t buffer[MAX_NOTES_LENGTH];

	if (hWnd == m_hGetHistoryButton)
	{
		FillHistoryListWithUserHistory();
	}

	else if (hWnd == m_hPersonSearchWnd)
	{
		if (m_pPersonList)
		{
			GetWindowText(m_hPersonSearchWnd, buffer, MAX_NOTES_LENGTH - 1);
			m_pPersonList->ApplyRowFilter(buffer);
		}
	}

	else if (hWnd == m_hTicketSearchWnd)
	{
		if (m_pHistoryList)
		{
			GetWindowText(m_hTicketSearchWnd, buffer, MAX_NOTES_LENGTH - 1);
			m_pHistoryList->ApplyRowFilter(buffer);
		}
	}
}

LRESULT HistoryTab::OnCustomMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_ROW_SELECTED:
		EnableWindow(m_hGetHistoryButton, TRUE);
		break;

	case WM_ROW_UNSELECTED:
		EnableWindow(m_hGetHistoryButton, FALSE);
		break;

	case WM_GET_SEARCH_HANDLE:
		return (LRESULT)m_hPersonSearchWnd;
	}

	return 0;
}

void HistoryTab::FillHistoryListWithUserHistory(void)
{
	if (m_pPersonList && m_pPersonList->IsSomeRowSelected())
	{
		m_pHistoryList->Clear();

		std::wstring person_id = m_pPersonList->GetCellContent(m_pPersonList->GetSelectedRowIndex(), 0);

		std::vector<db::Ticket> tickets;
		db::GetTicketsOfPerson(std::stoi(person_id), tickets);

		for (db::Ticket& ticket : tickets) {
			m_pHistoryList->AddRow(ticket);
		}
	}
}

void HistoryTab::OnSwitchedToOther(void)
{
	m_pHistoryList->Clear();
}

void HistoryTab::Draw(HDC hDC)
{
	const double dpiScale = util::GetDPIScale(m_hWndSelf);

	const int iSearchEditWidth = static_cast<int>(240 * dpiScale);
	const int iSearchEditHeight = static_cast<int>(32 * dpiScale);
	const int iGapBetweenEditAndList = static_cast<int>(16 * dpiScale);

	DrawIconEx(hDC,
		m_pHistoryList->GetX() + (m_pHistoryList->GetWidth() - iSearchEditWidth) / 2 - 52, // I don't remember why i picked 52 lol
		m_pHistoryList->GetY() - iSearchEditHeight - iGapBetweenEditAndList,
		m_hSearchIcon,
		0,
		0,
		NULL,
		NULL,
		DI_NORMAL
	);

	DrawIconEx(hDC,
		m_pPersonList->GetX() + (m_pPersonList->GetWidth() - iSearchEditWidth) / 2 - 52,
		m_pPersonList->GetY() - iSearchEditHeight - iGapBetweenEditAndList,
		m_hSearchIcon,
		0,
		0,
		NULL,
		NULL,
		DI_NORMAL
	);
}

HWND HistoryTab::CreateSearchEdit(HINSTANCE hInstance)
{
	HWND hSearchEdit = CreateWindow(
		L"Edit",
		L"",
		WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | WS_BORDER | WS_TABSTOP,
		0, 0, 0, 0,
		m_hWndSelf,
		NULL,
		hInstance,
		NULL
	);

	THROW_IF_NULL(hSearchEdit, "Unable to create search edit [HistoryTab]");

	SetWindowFont(hSearchEdit, m_hLargeFont);
	SetPlaceholderText(hSearchEdit, L"Αναζήτηση");

	return hSearchEdit;
}

void HistoryTab::OnSwitchedToSelf(void)
{
	HWND hSearchEdit = (HWND)SendMessage(m_pTabManager->GetTab(1)->GetHandle(), WM_GET_SEARCH_HANDLE, NULL, NULL);

	int length = GetWindowTextLength(hSearchEdit);

	wchar_t* lpszBuffer = new wchar_t[length + 1];

	THROW_IF_NULL(lpszBuffer, "Out of memory");

	GetWindowText(hSearchEdit, lpszBuffer, length + 1);
	SetWindowText(m_hPersonSearchWnd, lpszBuffer);

	delete[] lpszBuffer;
}