#pragma once

#include "Tab.h"

class ObjectTab : public Tab
{
public:
	ObjectTab(TabManager* pManager, const std::wstring& name);
	~ObjectTab(void);
};

