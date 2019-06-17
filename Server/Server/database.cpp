#include "server.h"

#define LEN 10
SQLHENV henv;
SQLHDBC hdbc;
SQLHSTMT hstmt = 0;
SQLRETURN retcode;
SQLWCHAR sz_id[LEN];
int dbX, dbY, dbLevel, dbHP, dbEXP, dbMaxHP, dbSkill[3], db_connect;
SQLLEN cb_id = 0, cbX = 0, cbY = 0, cbLevel = 0, cbHP = 0, cbEXP = 0, cbMaxHP = 0, cbSkill[3]{ 0 }, cb_connect = 0;

char buf[256];
wchar_t sql_data[256];
SQLWCHAR sqldata[256] = { 0 };

void DB_init()
{
	setlocale(LC_ALL, "korean");
	retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER*)SQL_OV_ODBC3, 0);
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
				SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);
				retcode = SQLConnect(hdbc, (SQLWCHAR*)L"2016180038", SQL_NTS, (SQLWCHAR*)NULL, 0, NULL, 0);
				if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
					printf("DB Access OK!!\n");
				}
			}
		}
	}
}

int DB_get_info(int id)
{
	//int dbX, dbY, dbLevel, dbHP, dbEXP, dbSkill[3];
	retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
	wsprintf((LPWSTR)buf, L"SELECT user_id,user_x,user_y,user_hp,user_level,user_exp,user_maxhp,user_skill1,user_skill2,user_skill3 FROM user_table WHERE id = %s", id);
	retcode = SQLExecDirect(hstmt, (SQLWCHAR *)buf, SQL_NTS);

	retcode = SQLBindCol(hstmt, 1, SQL_WCHAR, sz_id, LEN, &cb_id);
	retcode = SQLBindCol(hstmt, 2, SQL_C_LONG, &dbX, LEN, &cbX);
	retcode = SQLBindCol(hstmt, 3, SQL_C_LONG, &dbY, LEN, &cbY);
	retcode = SQLBindCol(hstmt, 4, SQL_C_LONG, &dbHP, LEN, &cbHP);
	retcode = SQLBindCol(hstmt, 5, SQL_C_LONG, &dbLevel, LEN, &cbLevel);
	retcode = SQLBindCol(hstmt, 6, SQL_C_LONG, &dbEXP, LEN, &cbEXP);
	retcode = SQLBindCol(hstmt, 7, SQL_C_LONG, &dbMaxHP, LEN, &cbMaxHP);
	retcode = SQLBindCol(hstmt, 8, SQL_C_LONG, &dbSkill[0], LEN, &cbSkill[0]);
	retcode = SQLBindCol(hstmt, 9, SQL_C_LONG, &dbSkill[1], LEN, &cbSkill[1]);
	retcode = SQLBindCol(hstmt, 10, SQL_C_LONG, &dbSkill[2], LEN, &cbSkill[2]);
	retcode = SQLBindCol(hstmt, 10, SQL_C_LONG, &db_connect, LEN, &cb_connect);

	retcode = SQLFetch(hstmt);

	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) 
	{
		return DB_Success;
	}
	if (retcode == SQL_NO_DATA)
	{
		return DB_NoData;
	}
	if (retcode == SQL_ERROR || retcode == SQL_SUCCESS_WITH_INFO) 
	{
		return DB_NoConnect;
	}
}

void DB_set_info(int id)
{
	retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

	wsprintf((LPWSTR)buf, L"UPDATE user_table SET user_x = %d, user_y = %d, user_hp = %d, user_level = %d, user_exp = %d, user_maxhp = %d, user_skill1 = %d, user_skill2 = %d, user_skill3 = %d WHERE id = %s", clients[id].x, clients[id].y, clients[id].hp, clients[id].level, clients[id].exp, clients[id].max_hp, clients[id].skill_1, clients[id].skill_2, clients[id].skill_3, clients[id].nickname);
	retcode = SQLExecDirect(hstmt, (SQLWCHAR *)buf, SQL_NTS);
}

void DB_new_id(int id)
{
	retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
	wsprintf((LPWSTR)buf, L"INSERT INTO user_table VALUES(%s, %d, %d, %d, %d, %d, %d, %d, %d, %d)", clients[id].nickname, clients[id].x, clients[id].y, clients[id].hp, clients[id].level, clients[id].exp, clients[id].max_hp, clients[id].skill_1, clients[id].skill_2, clients[id].skill_3);
	retcode = SQLExecDirect(hstmt, (SQLWCHAR *)buf, SQL_NTS);

}