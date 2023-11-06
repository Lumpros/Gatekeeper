#pragma once

#include "Window.h"

#include <vector>
#include <string>
#include <unordered_map>

#include <CommCtrl.h>
#include <d2d1.h>

#ifndef ROW_INDEX_NONE
#define ROW_INDEX_NONE (-1)
#endif

#ifndef COLUMN_INDEX_NONE
#define COLUMN_INDEX_NONE (-1)
#endif

struct ColumnInfo
{
	std::wstring strName = L"";
	int cxWidth = 0;

	ColumnInfo(std::wstring name, int width) {
		this->strName = std::move(name);
		this->cxWidth = width;
	}
};

struct ColumnFilter
{
	std::wstring filter_word = L"";
	int iColumnIndex = COLUMN_INDEX_NONE;
};

enum class ListViewNextSort
{
	ASCENDING,
	DESCENDING
};

#define ALL_COLUMNS (-1)

struct ColorRule
{
	COLORREF cr = 0;
	int columnAffected = ALL_COLUMNS;

	ColorRule(void) = default;
	ColorRule(COLORREF cr, int columnAffected = ALL_COLUMNS)
		: cr(cr), columnAffected(columnAffected) {}
};

class ListView : public Window
{
public:
	ListView(HWND hParentWindow, Size size, Point ptPos);
	~ListView(void);

	LRESULT WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	/////////////////// Content manipulation ///////////////////////////
	void AddColumn(const wchar_t* lpszColumnName, int cxWidth);
	void AddRow(std::vector<std::wstring>& info);
	void RemoveDisplayedRow(int index);
	void RemoveRow(int index);
	void ApplyRowFilter(const std::wstring& filter_word);
	void FilterOutColumnContent(int iColIndex, const std::wstring& filter_word);
	void SetDisplayedRowContent(int row, const std::vector<std::wstring>& newData);
	void SetCellContent(int row, int column, const std::wstring& content);
	void SetColorRule(const std::wstring& word, COLORREF cr, int column = ALL_COLUMNS);
	void Clear(void);

	void Mirror(ListView* pList);

	////////////// Getters /////////////////////
	std::wstring GetCellContent(int row, int column);
	std::vector<std::wstring> GetSelectedRow(void);
	inline int GetSelectedRowIndex(void) { return m_iSelectedIndex; };
	inline size_t GetDisplayedRowCount(void) const { return m_refIndexesOfShownRows->size(); }
	inline size_t GetRowCount(void) const { return m_refRows->size(); }

	bool IsSomeRowSelected(void);
	void UnselectSelectedRow(void);
	
	void OnDPIChanged(void);

private:
	/////////// (Un)initialization ////////////
	void RegisterViewListClass(HINSTANCE hInstance);
	void InitializeViewListWindow(HINSTANCE hInstance);
	void InitializeGraphicsResources(void);
	void ReleaseGraphicsResources(void);

	//////////// Scrollbars //////////////////
	void UpdateHorizontalScrollbar(void);
	void UpdateVerticalScrollbar(void);
	void FitScrollbarsToClient(void);

	///////// Area Validation & Invalidation /////////////
	void InvalidateColumnLabelBox(int index);
	void InvalidateRow(int index);
	void ValidateScrollbarArea(void);

	
	void DoColumnResizeByDragging(int iCursorX);

	/////////// Related to hovering over parts /////////////
	void ResetColumnLabelHovering(void);
	void HandleLabelBarHovering(int iCursorX);
	void HandleRowHovering(int iCursorY);

	//////////////// Drawing functions ///////////////////////
	void DrawBackground(size_t uWidth, size_t uHeight);
	void DrawRowBackground(size_t uWidth, size_t uHeight);
	void DrawRow(HDC hDC, size_t uWidth, size_t uHeight, size_t index);
	void DrawRows(HDC hDC, size_t uWidth, size_t uHeight);
	void DrawLabelBar(HDC hDC, size_t uWidth, size_t uHeight);
	void DrawColumns(HDC hDC, size_t uWidth, size_t uHeight);

	////////////// Related to list content ////////////////
	bool IsValidCellPosition(int row, int col);
	int GetExtraRowsOffScreenCount(void);
	int GetRelativeColumnHorizontalPosition(int index);
	void SortColumnData(int index);

	COLORREF GetWordColor(const std::wstring& word, int column);

	bool RowObeysToColumnFilters(int iRowIndex);

private:
	LRESULT OnPaint(void);
	LRESULT OnLeftMouseDown(WPARAM wParam, LPARAM lParam);
	LRESULT OnLeftMouseUp(WPARAM wParam, LPARAM lParam);
	LRESULT OnMouseMove(int iCursorX, int iCursorY);
	LRESULT OnEraseBackground(HDC hDC);
	LRESULT OnVScroll(WORD scrollType, WORD scrollPosition);
	LRESULT OnHScroll(WORD scrollType, WORD scrollPosition);
	LRESULT OnSize(WORD newWidth, WORD newHeight);
	LRESULT OnMouseLeave(void);
	LRESULT OnKeyDown(WPARAM wParam);

private:
	// Contains the name and width of each column
	std::vector<ColumnInfo> m_Columns;
	std::vector<ListViewNextSort> m_NextColumnSortOrder;

	// Contains all the data for the rows in the ListView
	std::vector<std::vector<std::wstring>> m_Rows;
	std::vector<std::vector<std::wstring>>* m_refRows;

	// This vector contains the indexes in m_Rows of the rows that are currently being drawn on screen.
	// Whenever a filter is applied or the data is sorted, this is the only vector that is affected,
	// not the m_Rows vector.
	std::vector<int>* m_refIndexesOfShownRows;
	std::vector<int> m_IndexesOfShownRows;

	// Hashmap that matches string -> Color used when drawing it
	std::unordered_map<std::wstring, ColorRule> m_WordColoring;

	// The width of the column that is being resized before the resizing started.
	int m_widthBeforeDragging = 0;
	
	// The initial position of the cursor relative to the window when the dragging started.
	int m_iDraggingStartX = 0;

	// The index in m_Columns of the column which is being dragged.
	int m_iDraggingIndex = 0;

	// The index in m_IndexesOfShownRows of the row tha thas been clicked the most recently.
	int m_iSelectedIndex = ROW_INDEX_NONE;

	// The index in m_IndexesOfShownRows of the row which is being hovered.
	int m_iHoveringRowIndex = ROW_INDEX_NONE;

	// Horizontal scroll offset in pixels
	int m_cxOffset = 0; 

	// Vertical scroll offset in pixels
	int m_cyOffset = 0; 

	// The index in m_Columns of the column label that is being hovered over.
	// Used because we want to draw a shadow above the said label when it is hovered.
	int m_iHoveredColumnLabel = COLUMN_INDEX_NONE;

	// The index in m_Columns of the column label that has been clicked
	// Used because we want to draw a shadow above the said label when it is clicked.
	int m_iClickedColumnLabel = COLUMN_INDEX_NONE;

	// This variable indicates whether a column is currently being dragged
	bool m_isDragging = false;

	// This variable indicates whether the cursor is hovering over a vertical line
	// that can be dragged in order to resize a column. If this variable is true
	// then when the user clicks the program should start resizing the column.
	bool m_canDrag = false;

	// Used in HandleRowHovering because we avoid redrawing areas that aren't needed,
	// But they must be drawn the first time the function is called otherwise they won't
	// be displayed on screen.
	bool m_firstTimeDrawing = true;

	// Used in order not to call TrackMouseEvent twice
	bool m_isTrackingCursor = false;

	// Used for Direct2D drawing because GDI is extremely slow and I don't want the window to blink
	ID2D1HwndRenderTarget* m_pRenderTarget = nullptr;

	// Honestly used because I don't feel like rewriting every drawing section in Direct2D, especially
	// the text parts, so I'd rather use this and make it backwards compatible, it's much faster as well
	ID2D1GdiInteropRenderTarget* m_pGDIRT = nullptr;

	// The brush used for all the rectangles in OnPaint. Its color and opacity are changed throughout the program.
	ID2D1SolidColorBrush* m_pSolidColorBrush = nullptr;

	// Width (Height when it's horizontal) of the scrollbar in pixels
	size_t cxScrollbarWidth = 0;

	// Height of the label bar in pixels
	size_t cyLabelBar = 0;

	// Height of a row in pixels
	size_t cyRow = 0;

	// The sum of the column widths in pixels, used in order to avoid recalculating it
	// every time we want to adjust the horizontal scrollbar.
	size_t sumOfColumnWidths = 0;
	
	HFONT m_hFont = NULL;
	HWND m_hVertSB = NULL;
	HWND m_hHorzSB = NULL;

	std::vector<ColumnFilter> columnFilters;
	std::vector<std::wstring> rowFilters;
};