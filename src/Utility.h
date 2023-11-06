#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <string>

#define WM_ROW_SELECTED         (WM_APP + 1)
#define WM_ROW_UNSELECTED       (WM_APP + 2)
#define WM_SWITCH_TO_EXPORT_TAB (WM_APP + 3)
#define WM_IMPORT_PERSON        (WM_APP + 4)
#define WM_EXPORT_TICKET        (WM_APP + 5)
#define WM_DELETE_PERSON_TICKET (WM_APP + 6)
#define WM_PREPARE_FOR_EXPORT   (WM_APP + 7)
#define WM_GET_SEARCH_HANDLE    (WM_APP + 8)

// In order to use the RGB macro to initialize a Direct2D color, we have to enter
// the r, g, b values in reverse order, so we'll use this macro to make the program more readable
#ifndef RGB_D2D
#define RGB_D2D(r, g, b) RGB(b, g, r)
#endif

#ifndef SADE_DELETE
#define SAFE_DELETE(ptr)    \
    if (ptr) {              \
        delete (ptr);       \
        (ptr) = nullptr;    \
    }
#endif

#ifndef SADE_DELETE_GDIOBJ
#define SAFE_DELETE_GDIOBJ(gdiobj) \
    if (gdiobj) {                  \
        DeleteObject(gdiobj);      \
        (gdiobj) = NULL;           \
    }
#endif

#ifndef THROW_IF_NULL
#define THROW_IF_NULL(p, msg)          \
     if (!p) {                         \
        throw std::runtime_error(msg); \
     }
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

#define SetWindowFont(hWnd, hFont)         SendMessage((hWnd), WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE)
#define SetButtonIcon(hWnd, hIcon)         SendMessage((hWnd), BM_SETIMAGE, IMAGE_ICON, (LPARAM)(hIcon));
#define SetPlaceholderText(hWnd, lpszText) SendMessage((hWnd), EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(lpszText))
#define SetEditCharLimit(hWnd, lim)        SendMessage((hWnd), EM_SETLIMITTEXT, (lim), NULL);

#define INVALID_DATE (-1)

namespace util
{
    ////////////////////////////////////////////////////////
    ////////////// Date & Time functions ///////////////////
    ////////////////////////////////////////////////////////

    struct Date
    {
        int day   = INVALID_DATE;
        int month = INVALID_DATE;
        int year  = INVALID_DATE;
    };

    bool IsMilitaryTime(const std::wstring& time);
    bool IsValidDate(const util::Date& date);

    std::wstring GetLocalTime(void);
    std::wstring GetLocalDate(void);

    Date ConvertStringToDate(const std::wstring& str);

    //////////////////////////////////////////////////////////////
    //////////// Person role conversion functions ////////////////
    //////////////////////////////////////////////////////////////

    enum class PersonRole
    {
        INVALID = -1,
        EMPLOYEE = 0,
        CAMPER,
    };

    std::wstring EnumToString(PersonRole role);

    PersonRole StringToEnum(std::wstring role);
    
    //////////////////////////////////////////////////////////////
    /////////////////// Win32 API Functions //////////////////////
    //////////////////////////////////////////////////////////////

	void RegisterObject(HWND hWnd, void* object);
	void* GetRegisteredObject(HWND hWnd);

	double GetDPIScale(HWND hWnd);

	HFONT CreateStandardUIFont(int size, const wchar_t* lpszName = L"Segoe UI");

    LRESULT CALLBACK MultilineEditTabstopSubclassProcedure(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);

    BOOL FileExists(LPCTSTR szPath);

    void TrimEditControlContent(HWND hEditCtrl);

    ///////////////////////////////////
    /////// Encoding/Decoding//////////
    ///////////////////////////////////
    void EncodeWideTextToMultibyte(const wchar_t* lpszText, char* out, size_t out_len);
    void DecodeMultibyteToWideText(const char* lpszText, wchar_t* out, size_t out_len);
}