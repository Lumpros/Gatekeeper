#pragma once

#include "Tab.h"
#include "ListView.h"
#include "Database.h"

class MainTab : public Tab
{
public:
	MainTab(TabManager* pTabManager, const std::wstring& name);
	~MainTab(void);

	void HandleListViewKeyDownMessage(WPARAM wParam);

protected:
	void OnResize(int width, int height) override;
	void OnCommand(HWND hWnd) override;
	LRESULT OnCustomMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
	void Draw(HDC hDC) override;
	void Draw(ID2D1RenderTarget* pRenderTarget) override;

private:
	void InitTicketListView(void);
	void InitLVRelatedControls(void);
	void InitRowEditControls(void);

	void LoadTicketsFromDatabaseFile(void);

	void OnEditRowButtonClicked(void);
	void OnSubmitButtonClicked(void);
	void OnDeleteButtonClicked(void);
	void OnDeactivateButtonClicked(void);
	void OnActivateButtonClicked(void);
	void OnInformedButtonClicked(void);

	void DisableEditing(void);

	inline int GetButtonDistanceFromLV(void);
	inline int GetLVButtonWidth(void);
	inline int GetEditAreaWidth(void);

	inline void UpdateTicketListPos(int width, int height, double dpiScale);
	inline void UpdateSearchbarPos(int width, int height, double dpiScale);
	inline void UpdateDeactivateButtonPos(int width, int height, double dpiScale);
	inline void UpdateCancelButtonPos(int width, int height, double dpiScale);
	inline void UpdateEditButtonPos(int width, int height, double dpiScale);
	inline void UpdateAddButtonPos(int width, int height, double dpiScale);
	inline void UpdateInformedButtonPos(int width, int height, double dpiScale);
	inline void UpdateActivateButtonPos(int width, int height, double dpiScale);

	inline void UpdateFirstnameEditPos(const POINT& ptGroup, int cx, int cy, int dist);
	inline void UpdateLastnameEditPos(const POINT& ptGroup, int cx, int cy, int dist);
	inline void UpdatePaternalnameEditPos(const POINT& ptGroup, int cx, int cy, int dist);
	inline void UpdateDepartureEditPos(const POINT& ptGroup, int cx, int cy, int dist);
	inline void UpdateArrivalEditPos(const POINT& ptGroup, int cx, int cy, int dist);
	inline void UpdateNotesEditPos(const POINT& ptGroup, int cx, int cy, int dist);

	HWND CreateChildButtonWithIcon(const wchar_t* lpszText, HICON hIcon);
	HWND CreateChildEdit(const wchar_t* lpszPlaceholder, DWORD dwExtraStyles = 0);

	bool WindowTextIsMilitaryTime(HWND hWnd);

	void SaveTicketToDatabase(db::Ticket& ticket);

private:
	ListView* m_pTicketListView = nullptr;

	HWND m_hSearchEdit       = NULL;
	HWND m_hDeactivateButton = NULL;
	HWND m_hDeleteButton     = NULL;
	HWND m_hEditButton       = NULL;
	HWND m_hSubmitButton     = NULL;
	HWND m_hAddButton        = NULL;
	HWND m_hInformedButton   = NULL;
	HWND m_hActivateButton   = NULL;

	HWND m_hFirstnameEdit    = NULL;
	HWND m_hLastnameEdit     = NULL;
	HWND m_hPaternalnameEdit = NULL;
	HWND m_hDepartDateEdit   = NULL;
	HWND m_hDepartureEdit    = NULL;
	HWND m_hArrivalDateEdit  = NULL;
	HWND m_hArrivalEdit      = NULL;
	HWND m_hNotesEdit        = NULL;

	HFONT m_hHugeFont        = NULL;
	HFONT m_hLargeFont       = NULL;
	HFONT m_hSmallFont       = NULL;

	HICON m_hCancelIcon      = NULL;
	HICON m_hEditIcon        = NULL;
	HICON m_hDeactivateIcon  = NULL;
	HICON m_hSearchIcon      = NULL;
	HICON m_hCheckmarkIcon   = NULL;
	HICON m_hPlusIcon        = NULL;
	HICON m_hActiveIcon      = NULL;
	HICON m_hInformedIcon    = NULL;

	bool m_isEditing = false;
};

