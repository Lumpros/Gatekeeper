#pragma once

#include "sqlite/sqlite3.h"
#include "Tab.h"

#include <vector>
#include <string>

namespace db
{
	struct Person
	{
		int id;
		util::PersonRole role;
		wchar_t firstname[MAX_FIRSTNAME_LENGTH];
		wchar_t lastname[MAX_LASTNAME_LENGTH];
		wchar_t fathername[MAX_FIRSTNAME_LENGTH];
	};

	using Ticket = std::vector<std::wstring>;

	void Init(void);
	void Execute1K(const wchar_t* lpszCommand);
	void Uninit(void);

	void InsertPersonToDatabase(const Person& info);
	void InsertTicketToDatabase(const Ticket& ticket);

	int GetPersonID(const Person& info);

	std::vector<std::wstring> GetPersonInfo(int person_id);

	void LoadPeopleFromDatabase(std::vector<db::Person>&);
	void LoadTicketsFromDatabase(std::vector<db::Ticket>&);

	void GetTicketsOfPerson(int person_id, std::vector<db::Ticket>& tickets);
	void DeletePerson(int id);
	void DeleteTicket(int id);
	void DeactivateTicket(int id, const std::wstring& time);
	void ActivateTicket(int id);
	void TickInformed(int id);
	void UpdateTicket(db::Ticket& ticket);
	
	void GetPersonFromID(int id, db::Person& out);

	int GetLastInsertedRowId(void);
}