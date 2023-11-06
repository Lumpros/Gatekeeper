#include "Database.h"
#include "Utility.h"

#include "sqlite/sqlite3.h"

#include <stdexcept>
#include <cstdio>
#include <Windows.h>

sqlite3* g_database = nullptr;

static void CreateDatabaseTables(void);

void db::Init(void)
/*++
* 
* Routine Description:
* 
*	Opens the database file and all related files.
* 
* Arguments:
* 
*	None.
* 
* Return Value:
* 
*	Whether the initialization was successful.
* 
--*/
{
	if (sqlite3_open("sva.db", &g_database) != SQLITE_OK)
	{
		throw std::runtime_error(sqlite3_errmsg(g_database));
	}

	if (g_database == NULL)
	{
		throw std::runtime_error("Out of memory: Cannot open database file.");
	}

	CreateDatabaseTables();
}

void db::Execute1K(const wchar_t* lpszCommand)
{
	char* lpszErrorMessage = nullptr;
	char lpszSQLQuery[1024];

	util::EncodeWideTextToMultibyte(lpszCommand, lpszSQLQuery, 1024);

	sqlite3_exec(g_database, lpszSQLQuery, NULL, NULL, &lpszErrorMessage);

	if (lpszErrorMessage)
	{
		sqlite3_free(lpszErrorMessage);

		throw std::runtime_error("db::Execute1K() Error");
	}
}

void db::Uninit(void)
/*++
* 
* Routine Description:
* 
*	Frees all resources.
* 
* Arguments:
* 
*	None.
* 
* Return Value:
* 
*	None.
* 
--*/
{
	sqlite3_close(g_database);
}

static void CreateDatabaseTables(void)
{
	db::Execute1K(
		L"CREATE TABLE IF NOT EXISTS Person("
		L"   id         INTEGER PRIMARY KEY,"
		L"	role       INTEGER NOT NULL,"
		L"	firstname  TEXT NOT NULL,"
		L"	lastname   TEXT NOT NULL,"
		L"	fathername TEXT NOT NULL"
		L");"
	);

	db::Execute1K(
		L"CREATE TABLE IF NOT EXISTS Ticket("
		L"	 id        INTEGER PRIMARY KEY,"
		L"   state     TEXT    NOT NULL,"
		L"   person_id INTEGER NOT NULL,"
		L"   dept_date TEXT    NOT NULL,"
		L"   dept_time TEXT    NOT NULL,"
		L"	 arr_date  TEXT    NOT NULL,"
		L"	 arr_time  TEXT    NOT NULL,"
		L"   aarr_time TEXT    NOT NULL," 
		L"   notes     TEXT,"
		L"   FOREIGN KEY(person_id) REFERENCES Person(id)"
		L");"
	);

	try 
	{
		db::Execute1K(L"ALTER TABLE Ticket ADD COLUMN informed INTEGER DEFAULT 0");
	}

	catch (std::runtime_error& e)
	{
		// Already exists
	}
}

void db::InsertPersonToDatabase(const Person& info)
/*++
* 
* Routine Description:
* 
*	Inserts a person in the database with the given information. The caller is responsible
*	for making sure that none of the fields are empty.
* 
* Arguments:
* 
*	info - Reference to a person struct containing the information.
* 
--*/
{
	wchar_t lpszQuery[sizeof(Person) / sizeof(wchar_t) + 96] = {};

	swprintf_s(
		lpszQuery,
		sizeof(lpszQuery) / sizeof(lpszQuery[0]),
		L"INSERT INTO Person (role, firstname, lastname, fathername) VALUES (%d, \'%s\', \'%s\', \'%s\')",
		info.role, info.firstname, info.lastname, info.fathername
	);

	db::Execute1K(lpszQuery);
}

std::vector<std::wstring> db::GetPersonInfo(int person_id)
{
	std::vector<std::wstring> info;

	char query[128];
	ZeroMemory(query, sizeof(query));
	sprintf_s(query, 128, "SELECT * FROM Person WHERE id=%d", person_id);

	sqlite3_stmt* stmt;
	int rc = sqlite3_prepare_v2(g_database, query, strlen(query), &stmt, NULL);

	wchar_t buf[64];
	ZeroMemory(buf, sizeof(buf));

	while (sqlite3_step(stmt) != SQLITE_DONE)
	{
		int num_cols = sqlite3_column_count(stmt);

		for (int i = 0; i < num_cols; ++i)
		{
			switch (sqlite3_column_type(stmt, i))
			{
			case (SQLITE3_TEXT): {
			const char* str = reinterpret_cast<const char*>(sqlite3_column_text(stmt, i));
				util::DecodeMultibyteToWideText(str, buf, 64);
				info.emplace_back(std::wstring(buf));
			}
				break;
			case (SQLITE_INTEGER):
				info.emplace_back(std::to_wstring(sqlite3_column_int(stmt, i)));
				break;
			}
		}
	}

	sqlite3_finalize(stmt);

	return info;
}

void db::LoadPeopleFromDatabase(std::vector<db::Person>& peopleList)
{   
	peopleList.clear();

	constexpr char lpszQuery[] = "SELECT * FROM Person";

	sqlite3_stmt* statement;
	sqlite3_prepare_v2(g_database, lpszQuery, sizeof(lpszQuery), &statement, NULL);

	if (!statement)
	{
		throw std::runtime_error("Failed to load people from database");
	}

	while (sqlite3_step(statement) != SQLITE_DONE)
	{
		peopleList.emplace_back(db::Person());

		db::Person& info = peopleList.back();
		info.id = sqlite3_column_int(statement, 0);
		info.role = (util::PersonRole)sqlite3_column_int(statement, 1);
		util::DecodeMultibyteToWideText((const char*)(sqlite3_column_text(statement, 2)), info.firstname,  sizeof(info.firstname)  / sizeof(wchar_t));
		util::DecodeMultibyteToWideText((const char*)(sqlite3_column_text(statement, 3)), info.lastname,   sizeof(info.lastname)   / sizeof(wchar_t));
		util::DecodeMultibyteToWideText((const char*)(sqlite3_column_text(statement, 4)), info.fathername, sizeof(info.fathername) / sizeof(wchar_t));
	}

	sqlite3_finalize(statement);
}

int db::GetPersonID(const Person& info)
/*++
* 
* Routine Description:
* 
*	Looks up a person using the rest of their information, and returns their id.
* 
*	If the person doesn't exist, the function returns -1.
* 
* Arguments:
* 
*	info - Structure containing the person's information.
* 
--*/
{
	wchar_t lpszQuery[1024];
	swprintf_s(lpszQuery, sizeof(lpszQuery) / sizeof(wchar_t), 
		L"SELECT id FROM Person WHERE firstname='%s' AND lastname='%s' AND fathername='%s' AND role=%d",
		info.firstname, info.lastname, info.fathername, info.role
	);
	
	char lpszEncodedQuery[2048];
	util::EncodeWideTextToMultibyte(lpszQuery, lpszEncodedQuery, sizeof(lpszEncodedQuery));

	sqlite3_stmt* statement;
	sqlite3_prepare_v2(g_database, lpszEncodedQuery, sizeof(lpszEncodedQuery), &statement, NULL);

	if (!statement)
	{
	throw std::runtime_error("Query Error: gb::GetPersonID()");
	}

	// If sqlite3_step() returns SQLITE_DONE the first time it is called,
	// then the query didn't return any results, meaning the person doesn't exist
	if (sqlite3_step(statement) == SQLITE_DONE)
	{
		return -1;
	}

	const int id = sqlite3_column_int(statement, 0);

	sqlite3_finalize(statement);

	return id;
}

int db::GetLastInsertedRowId(void)
{
	return static_cast<int>(sqlite3_last_insert_rowid(g_database));
}

void db::InsertTicketToDatabase(const Ticket& ticket)
{
	wchar_t lpszQuery[1024] = {};

	swprintf_s(
		lpszQuery,
		sizeof(lpszQuery) / sizeof(lpszQuery[0]),
		L"INSERT INTO Ticket (state, informed, person_id, dept_date, dept_time, arr_date, arr_time, aarr_time, notes) \
          VALUES (\'%s\', %s, %s, \'%s\', \'%s\', \'%s\', \'%s\', \'%s\', \'%s\')",
		ticket[0].c_str(), ticket[1].c_str(), ticket[2].c_str(), ticket[3].c_str(),
		ticket[4].c_str(), ticket[5].c_str(), ticket[6].c_str(), ticket[7].c_str(), ticket[8].c_str()
	);

	db::Execute1K(lpszQuery);
}

void db::LoadTicketsFromDatabase(std::vector<db::Ticket>& tickets)
{
	sqlite3_stmt* statement;
	sqlite3_prepare_v2(g_database, "SELECT * FROM Ticket", -1, &statement, NULL);

	if (statement == NULL)
	{
		throw std::runtime_error("Query error: LoadTicketsFromDatabase()");
	}

	tickets.clear();

	wchar_t buffer[512];

	while (sqlite3_step(statement) != SQLITE_DONE)
	{
		db::Ticket ticket;

		const int columnCount = sqlite3_column_count(statement);

		for (int i = 0; i < columnCount; ++i)
		{
			switch (sqlite3_column_type(statement, i))
			{
			case SQLITE_TEXT: {
				const char* text = reinterpret_cast<const char*>(sqlite3_column_text(statement, i));
				util::DecodeMultibyteToWideText(text, buffer, 511);
				ticket.emplace_back(buffer);
				break;
			}

			case SQLITE_INTEGER: {
				if (i == 2) { // role
					db::Person person;
					db::GetPersonFromID(sqlite3_column_int(statement, i), person);
					ticket.emplace_back(util::EnumToString(person.role));
					ticket.emplace_back(person.firstname);
					ticket.emplace_back(person.lastname);
					ticket.emplace_back(person.fathername);
					break;
				} else if (i == 0) { // id
					ticket.emplace_back(std::to_wstring(sqlite3_column_int(statement, i)));
				} else if (i == 1) {

				} else {
					ticket.insert(ticket.begin() + 1, sqlite3_column_int(statement, i) == 0 ? L"✕" : L"✓");
				}
			}

			}
		}

		tickets.emplace_back(ticket);
	}

	sqlite3_finalize(statement);
}

void db::GetPersonFromID(int id, db::Person& out)
{
	char query[128];
	sprintf_s(query, 128, "SELECT role, firstname, lastname, fathername FROM Person WHERE Person.id=%d", id);

	sqlite3_stmt* statement;
	sqlite3_prepare_v2(g_database, query, -1, &statement, NULL);

	if (sqlite3_step(statement) == SQLITE_DONE)
	{
		out.id = -1;
		return;
	}
	
	wchar_t buffer[192];
	out.role = (util::PersonRole)sqlite3_column_int(statement, 0);
	util::DecodeMultibyteToWideText((const char*)sqlite3_column_text(statement, 1), out.firstname,  sizeof(out.firstname)  / sizeof(out.firstname[0]));
	util::DecodeMultibyteToWideText((const char*)sqlite3_column_text(statement, 2), out.lastname,   sizeof(out.lastname)   / sizeof(out.lastname[0]));
	util::DecodeMultibyteToWideText((const char*)sqlite3_column_text(statement, 3), out.fathername, sizeof(out.fathername) / sizeof(out.fathername[0]));

	sqlite3_finalize(statement);
}

void db::DeleteTicket(int id)
{
	std::wstring query = L"DELETE FROM Ticket WHERE Ticket.id=" + std::to_wstring(id);

	db::Execute1K(query.c_str());
}

void db::DeactivateTicket(int id, const std::wstring& timeOfDeactivation)
{
	std::wstring query = L"UPDATE Ticket SET state='Ανενεργή', aarr_time='" + timeOfDeactivation + L"' WHERE id = " + std::to_wstring(id);

	db::Execute1K(query.c_str());
}

void db::UpdateTicket(db::Ticket& ticket)
{
	// 4-9
	//L"INSERT INTO Ticket (state, person_id, dept_date, dept_time, arr_date, arr_time, aarr_time, notes) 
	std::wstring query =
		L"UPDATE Ticket SET "
		L"	dept_date='" + ticket[6] +
		L"',dept_time='" + ticket[7] +
		L"',arr_date='" + ticket[8] +
		L"',arr_time='" + ticket[9] +
		L"',notes='" + ticket[11] +
		L"' WHERE id=" + ticket[0];

	db::Execute1K(query.c_str());
}

void db::DeletePerson(int id)
{
	std::wstring query = L"DELETE FROM Person WHERE id=" + std::to_wstring(id);

	db::Execute1K(query.c_str());
}

void db::GetTicketsOfPerson(int person_id, std::vector<db::Ticket>& tickets)
{
	const std::wstring str_person_id = std::to_wstring(person_id);

	std::wstring query = L"SELECT * FROM Ticket WHERE person_id=" + str_person_id;

	char encoded[128];
	util::EncodeWideTextToMultibyte(query.c_str(), encoded, 127);

	sqlite3_stmt* statement;
	sqlite3_prepare_v2(g_database, encoded, -1, &statement, NULL);

	THROW_IF_NULL(statement, "Query Error: GetTicketsOfPerson()");

	wchar_t buffer[512];

	while (sqlite3_step(statement) != SQLITE_DONE)
	{
		db::Ticket ticket;

		const int columnCount = sqlite3_column_count(statement);

		for (int i = 0; i < columnCount; ++i)
		{
			switch (sqlite3_column_type(statement, i))
			{
			case SQLITE_TEXT: {
				const char* text = reinterpret_cast<const char*>(sqlite3_column_text(statement, i));
				util::DecodeMultibyteToWideText(text, buffer, 511);
				ticket.emplace_back(buffer);
				break;
			}

			case SQLITE_INTEGER: {
				if (i > 1) {
					db::Person person;
					db::GetPersonFromID(sqlite3_column_int(statement, i), person);
					ticket.emplace_back(std::to_wstring((int)(person.role)));
					ticket.emplace_back(person.firstname);
					ticket.emplace_back(person.lastname);
					ticket.emplace_back(person.fathername);
					break;
				}
				else {
					ticket.emplace_back(std::to_wstring(sqlite3_column_int(statement, i)));
				}
			}

			}
		}

		ticket[2] = util::EnumToString((util::PersonRole)std::stoi(ticket[2]));
		tickets.emplace_back(ticket);
	}

	sqlite3_finalize(statement);
}

void db::TickInformed(int id)
{
	std::wstring query = L"UPDATE Ticket SET informed=1 WHERE id=" + std::to_wstring(id);

	db::Execute1K(query.c_str());
}

void db::ActivateTicket(int id)
{
	std::wstring query = L"UPDATE Ticket SET state='Ενεργή' WHERE id=" + std::to_wstring(id);

	db::Execute1K(query.c_str());
}