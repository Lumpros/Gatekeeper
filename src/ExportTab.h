#pragma once

#include "Tab.h"
#include "ListView.h"
#include "Database.h"

class ExportTab : public Tab
{
	friend class HistoryTab;

public:
	ExportTab(TabManager* pTabManager, const std::wstring& name);
	~ExportTab(void);

protected:
	void OnResize(int width, int height) override;
	void Draw(HDC hDC) override;
	void Draw(ID2D1RenderTarget* pRenderTarget) override;

	void OnCommand(HWND hWnd) override;
	LRESULT OnCustomMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

	void OnSwitchedToSelf(void) override;

private:
	inline void CreateExportControls(void);
	inline void CreateExportRadioButtons(void);
	inline void CreateLVComplementaryControls(void);
	inline void CreateInsertionControls(void);

	inline int GetExportEditWidth(void) const noexcept;
	inline int GetExportEditHeight(void) const noexcept;
	inline int GetExportButtonHeight(void) const noexcept;
	inline int GetDistanceBetweenControls(void) const noexcept;
	inline POINT GetExportGroupPoint(void) const noexcept;

	inline void UpdateNameEditControlsPos(void);
	inline void UpdateTimeDateControlsPos(void);
	inline void UpdateOtherExportControlsPos(void);
	inline void UpdateSearchbarPos(void);
	inline void UpdateLVComplementaryCtrlPos(void);
	inline void UpdateInsertionControlsPos(void);

	inline void DrawSearchIcon(HDC hDC);

	void LoadPeopleFromDatabaseIntoListView(void);
	bool PersonExistsInDatabase(const db::Person& info);
	bool TicketRefersToExistingPerson(const db::Ticket& ticket);
	void AddPersonToDatabase(const db::Person& info);
	void AddPersonToListView(const db::Person& info);
	void ConvertSpecialSigmasToCapital(db::Person& info);

	void ImportPersonInformationToExportControls(void);
	void AddPersonInformationUsingControlText(void);
	void DeleteSelectedRow(void);

	std::vector<std::wstring> CreateTicketFromEnteredData(util::Date& departure, util::Date& arrival, bool perma);
	bool VerifyTimes(void);
	bool VerifyDates(util::Date* depart_out, util::Date* arrival_out);
	void ExportTicketFromEnteredData(bool perma);

	void SetFocusToAppropriateControl(void);

	HWND CreateExportEditControl(const wchar_t* lpszPlaceholder, DWORD dwFlags = 0);
	
private:
	HWND m_hFirstnameEdit     = NULL;
	HWND m_hLastnameEdit      = NULL;
	HWND m_hPaternalnameEdit  = NULL;
	HWND m_hDepartureDateEdit = NULL;
	HWND m_hDepartureTimeEdit = NULL;
	HWND m_hArrivalDateEdit   = NULL;
	HWND m_hArrivalTimeEdit   = NULL;
	HWND m_hNoteEdit          = NULL;

	HWND m_hExportButton      = NULL;
	HWND m_hImportButton      = NULL;
	HWND m_hAddButton         = NULL;
	HWND m_hPermaButton       = NULL;

	HWND m_hSearchEdit        = NULL;

	HWND m_hEmployeeButton    = NULL;
	HWND m_hCamperButton      = NULL;

	HWND m_hInsFirstnameEdit  = NULL;
	HWND m_hInsLastnameEdit   = NULL;
	HWND m_hInsPtrnalnameEdit = NULL;
	HWND m_hInsEmployeeBtn    = NULL;
	HWND m_hInsCamperBtn      = NULL;
	HWND m_hInsDeleteButton   = NULL;

	HFONT m_hSmallFont        = NULL;
	HFONT m_hLargeFont        = NULL;
	HFONT m_hHugeFont         = NULL;

	HICON m_hPlusIcon         = NULL;
	HICON m_hSearchIcon       = NULL;
	HICON m_hImportIcon       = NULL;
	HICON m_hTickIcon         = NULL;
	HICON m_hXIcon            = NULL;

	ListView* m_pPeopleList = nullptr;
	TabManager* m_pTabManagerParent = nullptr;

	int iSelectedPersonId = -1;
};

