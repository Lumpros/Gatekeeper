#include "Utility.h"

#include <cctype>
#include <CommCtrl.h>
#include <assert.h>
#include <stdexcept>

#ifndef GWL_USERDATA
#define GWL_USERDATA (-21)
#endif

static const std::wstring g_personRoles[] = {
	L"Στέλεχος",
	L"Κατασκηνωτής/ρια"
};

void util::RegisterObject(HWND hWnd, void* object)
/*++
* 
* Routine Description:
* 
*	Saves an object in memory that is associated with the given window handle.
*	The object can be later retrieved using the GetRegisteredObject function.
* 
* Arguments:
* 
*	hWnd - Handle to window.
*	object - Pointer to the data to be registered.
* 
* Return Value:
* 
*	None.
* 
--*/
{
	SetWindowLongPtr(hWnd, GWL_USERDATA, reinterpret_cast<LONG_PTR>(object));
}

void* util::GetRegisteredObject(HWND hWnd)
/*++
* 
* Routine Description:
* 
*	Returns the object associated with the window handle.
* 
* Arguments:
* 
*	hWnd - Handle to the window that is associated with the registered object.
* 
* Return Value:
* 
*	Pointer to the registered object.
* 
--*/
{
	return (void*)GetWindowLongPtr(hWnd, GWL_USERDATA);
}

double util::GetDPIScale(HWND hWnd)
/*++
* 
* Routine Description:
* 
*	Calculates how much the GUI should be scaled based on the window's DPI.
* 
* Arguments:
* 
*	hWnd - Handle to the window whose DPI will be used.
* 
* Return Value:
* 
*	The scale factor.
* 
--*/
{
	return GetDpiForWindow(hWnd) / 96.0;
}

HFONT util::CreateStandardUIFont(int size, const wchar_t* lpszName)
{
	return CreateFont(
		size,
		0,
		0,
		0,
		FW_NORMAL,
		FALSE,
		FALSE,
		FALSE,
		DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY,
		DEFAULT_PITCH,
		lpszName
	);
}

static bool IsNonNegativeInteger(const std::wstring& str)
{
	const size_t len = str.length();

	for (size_t i = 0; i < len; ++i)
	{
		if (!iswdigit(str[i]))
		{
			return false;
		}
	}

	return true;
}

static bool _IsMilitaryTime(const std::wstring& time)
/*++
* 
* Routine Description:
* 
*	Checks whether the given string is in the form of xx:yy,
*	where xx is a value in the range of [00, 23] and yy in [00, 59]
* 
* Arguments:
* 
*	time - The string to be tested.
* 
* Return Value:
* 
*	True if it is in the forementioned form, otherwise false.
* 
--*/
{
	if (time.length() == 5 && time.find_first_of(':') == 2)
	{
		const std::wstring firstNumberStr = time.substr(0, 2);
		const std::wstring secondNumberStr = time.substr(3, 2);

		if (IsNonNegativeInteger(firstNumberStr) && IsNonNegativeInteger(secondNumberStr))
		{
			const int firstNumber = std::stoi(firstNumberStr);
			const int secondNumber = std::stoi(secondNumberStr);

			if (firstNumber < 24 && secondNumber < 60)
			{
				return true;
			}
		}
	}

	return false;
}

bool util::IsMilitaryTime(const std::wstring& time)
/*++
*
* Routine Description:
*
*	Checks whether the given string is in the form of xx:yy or x:yy.
*
* Arguments:
*
*	time - The string to be tested.
*
* Return Value:
*
*	True if it is in the forementioned form, otherwise false.
*
--*/
{
	if (time.length() == 4) 
	{
		std::wstring padded = time;
		padded.insert(padded.begin(), L'0');
		return _IsMilitaryTime(padded);
	}

	return _IsMilitaryTime(time);
}

inline static bool IsLeapYear(int year)
{
	return ((year % 400 == 0 || year % 100 != 0) && (year % 4 == 0));
}

bool util::IsValidDate(const util::Date& date)
/*++
* 
* Routine Description:
* 
*	Checks if a date returned by ConvertStringToDate is a real date.
*	Leap years are taken into consideration.
* 
* Arguments:
* 
*	date - Structure containing the date information.
* 
--*/
{
	if (date.day != -1 && date.month != -1 && date.year != -1)
	{
		if (date.month >= 1 && date.month <= 12)
		{
			int daysInEachMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

			if (IsLeapYear(date.year))
			{
				daysInEachMonth[1] = 29;
			}

			return (date.day >= 1 && date.day <= daysInEachMonth[date.month - 1]);
		}
	}

	return false;
}

util::Date util::ConvertStringToDate(const std::wstring& str)
/*++
* 
* Routine Description:
* 
*	Returns a struct containing the date information given in a string.
* 
*	If any member of the struct is equal to -1, then the string is not a valid date.
* 
* Arguments:
* 
*	str - String in question.
* 
--*/
{
	util::Date date;

	if (std::count(str.begin(), str.end(), '/') == 2)
	{
		size_t firstSplit = str.find_first_of('/');
		size_t secondSplit = str.find_last_of('/');

		// First check is to avoid strings like "5//2034"
		// Second check is to avoid strings like "/12/2022"
		// Third check is to avoid strings like "5/12/"
		// Basically ensures that it the string is of the form "x/y/z", where x,y,z could be anything at this point
		if (secondSplit - firstSplit > 1 && firstSplit != 0 && secondSplit != str.length() - 1)
		{
			std::wstring day = str.substr(0, firstSplit);
			std::wstring month = str.substr(firstSplit + 1, secondSplit - firstSplit - 1);
			std::wstring year = str.substr(secondSplit + 1);

			if (IsNonNegativeInteger(day))
			{
				date.day = std::stoi(day);
			}

			if (IsNonNegativeInteger(month))
			{
				date.month = std::stoi(month);
			}

			if (IsNonNegativeInteger(year))
			{
				date.year = std::stoi(year);
			}
		}
	}

	return date;
}

std::wstring util::GetLocalDate(void)
/*++
* 
* Routine Description:
* 
*	Returns the local date (time shown at the bottom right of the desktop) in string form.
* 
* Arguments:
* 
*	None.
* 
--*/
{
	SYSTEMTIME st;
	GetLocalTime(&st);

	wchar_t buffer[32];
	swprintf_s(buffer, 32, L"%02d/%02d/%04d", st.wDay, st.wMonth, st.wYear);

	return std::wstring(buffer);
}

std::wstring util::GetLocalTime(void)
/*++
*
* Routine Description:
*
*	Returns the local time (time shown at the bottom right of the desktop) in string form.
*
* Arguments:
*
*	None.
*
--*/
{
	SYSTEMTIME st;
	GetLocalTime(&st);

	wchar_t buffer[32];
	swprintf_s(buffer, 32, L"%02d:%02d", st.wHour, st.wMinute);

	return std::wstring(buffer);
}

LRESULT CALLBACK util::MultilineEditTabstopSubclassProcedure(HWND hWnd, UINT uMsg, WPARAM w, LPARAM l, UINT_PTR u, DWORD_PTR d)
/*++
* 
* Routine Description:
* 
*	A multiline edit control cannot be used as a tabstop because it has the DLGC_WANTALLKEYS flag set.
*	
*	Use this procedure as a subclass procedure for a multiline edit control if you want to use WM_TABSTOP.
*	This procedure simply removes the forementioned flag upon receiving the WM_GETDLGCODE message.
* 
--*/
{
	LRESULT result = DefSubclassProc(hWnd, uMsg, w, l);

	switch (uMsg)
	{
	case WM_GETDLGCODE:
		result &= ~DLGC_WANTALLKEYS;
		break;
	}
	
	return result;
}

BOOL util::FileExists(LPCTSTR szPath)
/*++
* 
* Routine Description:
* 
*	Checks if a file exists at the given path.
*	Code taken from https://stackoverflow.com/questions/3828835/how-can-we-check-if-a-file-exists-or-not-using-win32-program.
* 
* Arguments:
* 
*	szPath - String of the path. May be absolute or relative.
* 
--*/
{
	DWORD dwAttrib = GetFileAttributes(szPath);

	return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

std::wstring util::EnumToString(PersonRole role)
/*++
* 
* Routine Description:
* 
*	Returns the string representation of an enum value.
* 
* Arguments:
* 
*	role - Enum value of role.
* 
--*/
{
	return g_personRoles[static_cast<int>(role)];
}

util::PersonRole util::StringToEnum(std::wstring role)
/*++
* 
* Routine Description:
* 
*	Matches a given string to an enum value.
*	If the role doesn't exist, PersonRole::INVALID is returned.
* 
* Arguments:
* 
*	role - The role of the person, in text.
* 
--*/
{
	if (role == g_personRoles[static_cast<int>(util::PersonRole::EMPLOYEE)])
	{
		return util::PersonRole::EMPLOYEE;
	}

	else if (role == g_personRoles[static_cast<int>(util::PersonRole::CAMPER)])
	{
		return util::PersonRole::CAMPER;
	}

	return util::PersonRole::INVALID;
}

void util::EncodeWideTextToMultibyte(const wchar_t* lpszText, char* out, size_t out_len)
{
	WideCharToMultiByte(
		CP_UTF8,
		0,
		lpszText,
		-1,
		out,
		out_len,
		NULL,
		FALSE
	);
}

void util::DecodeMultibyteToWideText(const char* lpszText, wchar_t* out, size_t out_len)
{
	MultiByteToWideChar(
		CP_UTF8,
		MB_PRECOMPOSED,
		lpszText,
		-1,
		out,
		out_len
	);
}

void util::TrimEditControlContent(HWND hEditCtrl)
{
	assert(hEditCtrl != nullptr);
		
	const int iTextLength = GetWindowTextLength(hEditCtrl);

	wchar_t* lpszTrimmed = new wchar_t[iTextLength + 1];

	THROW_IF_NULL(lpszTrimmed, "Out of memory");

	wchar_t const* original_address = lpszTrimmed;

	GetWindowText(hEditCtrl, lpszTrimmed, iTextLength + 1);

	int text_length = wcslen(lpszTrimmed);

	while (iswspace(*lpszTrimmed))
	{
		++lpszTrimmed;
		--text_length;
	}

	while (iswspace(lpszTrimmed[text_length - 1]))
	{
		--text_length;
	}

	lpszTrimmed[text_length] = '\0';

	SetWindowText(hEditCtrl, lpszTrimmed);

	delete[] original_address;
}