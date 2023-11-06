#pragma once

#include "Tab.h"
#include "ExportTab.h"

class HistoryTab : public Tab
{
public:
	HistoryTab(TabManager* pManager, const std::wstring& name);
	~HistoryTab(void);

	void GetAccessToLoadedUserData(ExportTab* pExportTab);

protected:
	void OnResize(int width, int height) override;
	void OnCommand(HWND hWnd) override;
	LRESULT OnCustomMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
	void Draw(HDC hDC) override;
	void OnSwitchedToSelf(void) override;
	void OnSwitchedToOther(void) override;

	HWND CreateSearchEdit(HINSTANCE hInstance);

private:
	void FillHistoryListWithUserHistory(void);

	inline void InitHistoryList(void);
	inline void InitPersonList(void);

private:
	HICON m_hSearchIcon = NULL;
	HICON m_hArrowIcon = NULL;

	HWND m_hPersonSearchWnd = NULL;
	HWND m_hTicketSearchWnd = NULL;
	HWND m_hGetHistoryButton = NULL;

	HFONT m_hSmallFont = NULL;
	HFONT m_hLargeFont = NULL;

	ListView* m_pPersonList = nullptr;
	ListView* m_pHistoryList = nullptr;

	TabManager* m_pTabManager = nullptr;
};

