#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <unordered_set>
#include <queue>
#include <chrono>

using namespace std;

#include <winsock2.h>

#include <windows.h>  
#include <stdio.h>  
#include <locale.h>

#define UNICODE  
#include <sqlext.h>  

#include "2019_화목_protocol.h"

#pragma comment(lib, "Ws2_32.lib")

#define MAX_BUFFER        1024

#define START_X		10
#define START_Y		10

enum ACT
{
	SEND = 1, RECV, MOVE
};

class TIMER_EVENT
{
public:
	int id;
	std::chrono::high_resolution_clock::time_point exec_time; // 이 이벤트가 언제 실행되야 하는가
	int job;
};

class event_comp
{
public:
	event_comp() {}
	bool operator()(const TIMER_EVENT a, const TIMER_EVENT b)const { return (a.exec_time > b.exec_time); }
};

struct OVER_EX
{
	WSAOVERLAPPED	 overlapped;
	WSABUF			dataBuffer;
	char			messageBuffer[MAX_BUFFER];
	char			act;
};

class NPC
{
public:
	short x, y;
	int hp;
};

class USER : public NPC
{
public:
	mutex	access_lock;
	bool	connected;
	bool is_move;
	OVER_EX over;
	SOCKET socket;
	char packet_buf[MAX_BUFFER];
	int prev_size;

	int level;
	int exp;
	std::unordered_set <int> viewlist;
};

USER clients[NPC_ID_START + NUM_NPC];

HANDLE g_iocp;

std::priority_queue < TIMER_EVENT, std::vector<TIMER_EVENT>, event_comp> timer_queue;
std::mutex t_lock;

#define NAME_LEN 10

SQLHENV henv;
SQLHDBC hdbc;
SQLHSTMT hstmt = 0;
SQLRETURN retcode;

void initialize()
{
	for (int i = 0; i < MAX_USER; ++i) {
		clients[i].connected = false;
		clients[i].viewlist.clear();
	}
	for (int i = NPC_ID_START; i < NPC_ID_START + NUM_NPC; ++i) {
		clients[i].connected = true;
		clients[i].is_move = false;
		clients[i].x = rand() % WORLD_WIDTH;
		clients[i].y = rand() % WORLD_HEIGHT;
	}

	setlocale(LC_ALL, "korean");
	retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER*)SQL_OV_ODBC3, 0);
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
				SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);
				retcode = SQLConnect(hdbc, (SQLWCHAR*)L"2016180038_jes", SQL_NTS, (SQLWCHAR*)NULL, 0, NULL, 0);
				if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
					printf("DB Access OK!!\n");
				}
			}
		}
	}
}

void DB_SQL_GET_STATUS(wchar_t *id)
{
	retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
	SQLCHAR UserId;
	SQLINTEGER UserX, UserY, UserEXP, UserLEVEL, UserHP;
	SQLLEN cbUserId = 0, cbUserX = 0, cbUserY = 0, cbUserEXP = 0, cbUserLEVEL = 0, cbUserHP = 0;// 
	WCHAR Query[MAX_BUFFER];

	wsprintf((LPWSTR)Query, L"SELECT posX, posY, hp, lev, exp FROM user_table WHERE id = %s", id);
	retcode = SQLExecDirect(hstmt, (SQLWCHAR *)Query, SQL_NTS);

	retcode = SQLBindCol(hstmt, 1, SQL_C_LONG, &UserX, 100, &cbUserX);// 
	retcode = SQLBindCol(hstmt, 2, SQL_C_LONG, &UserY, 100, &cbUserY);//인티져니까 sLevel에 &

	retcode = SQLFetch(hstmt);

	clients[id].x = UserX;
	clients[id].y = UserY;
}

void DB_SQL_SET_POS(int id)
{
	retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
	SQLINTEGER UserId, UserX, UserY;
	SQLLEN cbUserId = 0, cbUserX = 0, cbUserY = 0;// 
	WCHAR Query[MAX_BUFFER];

	wsprintf((LPWSTR)Query, L"UPDATE user_table SET posX = %d, posY = %d WHERE id = %d", clients[id].x, clients[id].y, id);
	retcode = SQLExecDirect(hstmt, (SQLWCHAR *)Query, SQL_NTS);
}

int get_new_id()
{
	for (int i = 0; i < MAX_USER; ++i)
		if (clients[i].connected == false) {
			clients[i].connected = true;
			return i;
		}
	return -1;
}

bool is_near_object(int a, int b)
{
	if (VIEW_RADIUS < abs(clients[a].x - clients[b].x))
		return false;
	if (VIEW_RADIUS < abs(clients[a].y - clients[b].y))
		return false;
	return true;
}

void error_display(const char *msg, int err_no)
{
	WCHAR *lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, err_no,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	cout << msg;
	wcout << L"에러 " << lpMsgBuf << endl;
	while (true);
	LocalFree(lpMsgBuf);
}

void HandleDiagnosticRecord(SQLHANDLE hHandle, SQLSMALLINT hType, RETCODE RetCode)
{
	SQLSMALLINT iRec = 0;
	SQLINTEGER iError;
	WCHAR wszMessage[1000];
	WCHAR wszState[SQL_SQLSTATE_SIZE + 1];
	if (RetCode == SQL_INVALID_HANDLE) {
		fwprintf(stderr, L"Invalid handle!\n");
		return;
	}
	while (SQLGetDiagRecW(hType, hHandle, ++iRec, wszState, &iError, wszMessage, (SQLSMALLINT)(sizeof(wszMessage) / sizeof(WCHAR)), (SQLSMALLINT *)NULL) == SQL_SUCCESS)
	{
		//Hide data truncated..
		if (wcsncmp(wszState, L"01004", 5))
		{
			fwprintf(stderr, L"[%5.5s] %s (%d) \n", wszState, wszMessage, iError);
		}
	}
}

void add_timer(int id, int job, std::chrono::high_resolution_clock::time_point t)
{
	t_lock.lock();
	timer_queue.push(TIMER_EVENT{ id, t, job });
	t_lock.unlock();
}

void wake_up_npc(int id)
{
	if (true == clients[id].is_move) return;
	clients[id].is_move = true;
	add_timer(id, 0, std::chrono::high_resolution_clock::now() + std::chrono::seconds(1));
}

void send_packet(int key, char *packet)
{
	SOCKET client_s = clients[key].socket;

	OVER_EX *over = reinterpret_cast<OVER_EX *>(malloc(sizeof(OVER_EX)));

	over->act = ACT::SEND;
	over->dataBuffer.buf = over->messageBuffer;
	over->dataBuffer.len = packet[0];
	memcpy(over->messageBuffer, packet, packet[0]);
	ZeroMemory(&(over->overlapped), sizeof(WSAOVERLAPPED));
	if (WSASend(client_s, &over->dataBuffer, 1, NULL,
		0, &(over->overlapped), NULL) == SOCKET_ERROR)
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			cout << "Error - Fail WSASend(error_code : ";
			cout << WSAGetLastError() << ")\n";
		}
	}
}

void send_login_ok_packet(int to)
{
	sc_packet_login_ok packet;
	packet.size = sizeof(packet);
	packet.type = SC_LOGIN_OK;
	packet.id = to;
	packet.x = clients[to].x;
	packet.y = clients[to].y;
	packet.HP = clients[to].hp;
	packet.LEVEL = clients[to].level;
	packet.EXP = clients[to].exp;
	send_packet(to, reinterpret_cast<char *>(&packet));
}

void send_login_fail_packet(int to, wchar_t *message)
{
	sc_packet_login_fail packet;
	packet.size = sizeof(packet);
	packet.type = SC_LOGIN_FAIL;
	wcsncpy_s(packet.message, message, MAX_STR_LEN);
	send_packet(to, reinterpret_cast<char *>(&packet));
}

void send_position_packet(int to, int obj)
{
	sc_packet_position packet;
	packet.size = sizeof(packet);
	packet.type = SC_POSITION;
	packet.id = obj;
	packet.x = clients[obj].x;
	packet.y = clients[obj].y;
	send_packet(to, reinterpret_cast<char *>(&packet));
}

void send_chat_packet(int to, wchar_t *from, wchar_t *message)
{
	sc_packet_chat packet;
	packet.size = sizeof(packet);
	packet.type = SC_CHAT;
	wcsncpy_s(packet.id, from, 10);
	wcsncpy_s(packet.message, message, MAX_STR_LEN);
	send_packet(to, reinterpret_cast<char*>(&packet));
}

void send_stat_change_packet(int to, int obj, int hp, int level, int exp)
{
	sc_packet_stat_change packet;
	packet.size = sizeof(packet);
	packet.type = SC_STAT_CHANGE;
	packet.id = obj;
	packet.HP = hp;
	packet.LEVEL = level;
	packet.EXP = exp;
	send_packet(to, reinterpret_cast<char*>(&packet));
}

void send_add_object_packet(int to, int obj, int obj_class)
{
	sc_packet_add_object packet;
	packet.size = sizeof(packet);
	packet.type = SC_ADD_OBJECT;
	packet.id = obj;
	packet.obj_class = obj_class;
	packet.HP;
	packet.LEVEL;
	packet.EXP;
	packet.x;
	packet.y;
	send_packet(to, reinterpret_cast<char*>(&packet));
}

void send_remove_object_packet(int to, int obj)
{
	sc_packet_remove_object packet;
	packet.size = sizeof(packet);
	packet.type = SC_REMOVE_OBJECT;
	packet.id = obj;
	send_packet(to, reinterpret_cast<char*>(&packet));
}
void disconnect(int id)
{
	for (int i = 0; i < MAX_USER; ++i) {
		if (false == clients[i].connected) continue;
		clients[i].access_lock.lock();
		if (0 != clients[i].viewlist.count(id))	// 접속유저 전체 돌면서 뷰리스트 안에 id 있을때
		{
			clients[i].viewlist.erase(id);
			clients[i].access_lock.unlock();
			send_remove_player_packet(i, id);
		}
		else
		{
			clients[i].access_lock.unlock();
		}
	}
	closesocket(clients[id].socket);
	clients[id].viewlist.clear();
	clients[id].connected = false;
}

void move_npc(int id)
{
	int x = clients[id].x;
	int y = clients[id].y;
	switch (rand() % 4) {
	case 0: if (x > 0) x--; break;
	case 1: if (x < WORLD_WIDTH - 1) x++; break;
	case 2: if (y > 0) y--; break;
	case 3: if (y < WORLD_HEIGHT - 1) y++; break;
	}
	clients[id].x = x;
	clients[id].y = y;

	for (int i = 0; i < MAX_USER; ++i)
	{
		if (false == clients[i].connected) continue;
		if (true == is_near_object(i, id)) {
			clients[i].access_lock.lock();
			if (0 == clients[i].viewlist.count(id)) {
				clients[i].viewlist.insert(id);
				clients[i].access_lock.unlock();
				send_put_player_packet(i, id);
			}
			else {
				clients[i].access_lock.unlock();
				send_pos_packet(i, id);
			}
		}
		else { // 시야에 없다
			clients[i].access_lock.lock();
			if (0 == clients[i].viewlist.count(id)) {
				clients[i].access_lock.unlock();
			}
			else {
				clients[i].viewlist.erase(id);
				clients[i].access_lock.unlock();
				send_remove_player_packet(i, id);
			}
		}
	}
}

void process_packet(int id, char * buf)
{
	cs_packet_up *packet = reinterpret_cast<cs_packet_up *>(buf);

	short x = clients[id].x;
	short y = clients[id].y;
	switch (packet->type) {
	case CS_UP:
		--y;
		if (y < 0) y = 0;
		break;
	case CS_DOWN:
		++y;
		if (y >= WORLD_HEIGHT) y = WORLD_HEIGHT;
		break;
	case CS_LEFT:
		if (0 < x) x--;
		break;
	case CS_RIGHT:
		if (WORLD_WIDTH > x) x++;
		break;
	default:
		cout << "Unknown Packet Type Error\n";
		while (true);
	}
	clients[id].x = x;
	clients[id].y = y;

	send_pos_packet(id, id);

	clients[id].access_lock.lock();
	unordered_set <int> old_vl = clients[id].viewlist;
	clients[id].access_lock.unlock();
	unordered_set <int> new_vl;

	for (int i = 0; i < MAX_USER; ++i) {
		if ((true == clients[i].connected) && (true == is_near_object(id, i)) && (i != id))
			new_vl.insert(i);
	}

	for (int i = 0; i < NUM_NPC; ++i) // 엔피씨 뷰리스트에 넣기
	{
		int npc_id = i + NPC_ID_START;
		if (true == is_near_object(npc_id, id))
		{
			new_vl.insert(npc_id);
		}
	}

	for (auto cl : new_vl) {
		if (0 == old_vl.count(cl)) {
			send_put_player_packet(id, cl);
			clients[id].access_lock.lock();
			clients[id].viewlist.insert(cl);
			clients[id].access_lock.unlock();
			if (cl >= NPC_ID_START) {
				wake_up_npc(cl);
			}
		}

		if (cl >= NPC_ID_START) continue;
		clients[cl].access_lock.lock();
		if (0 != clients[cl].viewlist.count(id)) {
			clients[cl].access_lock.unlock();
			send_pos_packet(cl, id);
		}
		else {
			clients[cl].viewlist.insert(id);
			clients[cl].access_lock.unlock();
			send_put_player_packet(cl, id);
		}
	}

	for (auto cl : old_vl) {
		if (0 == new_vl.count(cl)) {
			// 시야에서 사라진 플레이어
			clients[id].access_lock.lock();
			clients[id].viewlist.erase(cl);
			clients[id].access_lock.unlock();
			send_remove_player_packet(id, cl);

			if (cl >= NPC_ID_START) continue;
			clients[cl].access_lock.lock();
			if (0 != clients[cl].viewlist.count(id)) {
				clients[cl].viewlist.erase(id);
				clients[cl].access_lock.unlock();
				send_remove_player_packet(cl, id);
			}
			else {
				clients[cl].access_lock.unlock();
			}
		}
	}
}

void do_recv(int id)
{
	DWORD flags = 0;

	SOCKET client_s = clients[id].socket;
	OVER_EX *over = &clients[id].over;

	over->dataBuffer.len = MAX_BUFFER;
	over->dataBuffer.buf = over->messageBuffer;
	ZeroMemory(&(over->overlapped), sizeof(WSAOVERLAPPED));
	if (WSARecv(client_s, &over->dataBuffer, 1, NULL,
		&flags, &(over->overlapped), NULL) == SOCKET_ERROR)
	{
		int err_no = WSAGetLastError();
		if (err_no != WSA_IO_PENDING)
		{
			error_display("RECV ERROR", err_no);
		}
	}
}

void worker_thread()
{
	while (true) {
		DWORD io_byte;
		ULONG key;
		OVER_EX *lpover_ex;
		BOOL is_error = GetQueuedCompletionStatus(g_iocp, &io_byte, &key,
			reinterpret_cast<LPWSAOVERLAPPED *>(&lpover_ex), INFINITE);

		if (FALSE == is_error)
		{
			int err_no = WSAGetLastError();
			if (64 != err_no)
				error_display("GQCS ", err_no);
			else {
				disconnect(key);
				continue;
			}
		}
		if (0 == io_byte) {
			disconnect(key);
			continue;
		}

		if (lpover_ex->act == ACT::RECV) {
			int rest_size = io_byte;
			char *ptr = lpover_ex->messageBuffer;
			char packet_size = 0;
			if (0 < clients[key].prev_size)
				packet_size = clients[key].packet_buf[0];
			while (rest_size > 0) {
				if (0 == packet_size)
					packet_size = ptr[0];
				int required = packet_size - clients[key].prev_size;
				if (rest_size >= required) {
					memcpy(clients[key].packet_buf + clients[key].prev_size, ptr, required);
					process_packet(key, clients[key].packet_buf);
					rest_size -= required;
					ptr += required;
					packet_size = 0;
				}
				else {
					memcpy(clients[key].packet_buf + clients[key].prev_size,
						ptr, rest_size);
					rest_size = 0;
				}
			}
			do_recv(key);
		}
		else if (lpover_ex->act == ACT::SEND)
		{
			delete lpover_ex;
		}
		else if (lpover_ex->act == ACT::MOVE)
		{
			move_npc(key);
			bool player_exist = false;
			for (int i = 0; i < MAX_USER; ++i)
			{
				if (is_near_object(i, key))
				{
					player_exist = true;
					break;
				}
			}
			if (player_exist)
				add_timer(key, 0, std::chrono::high_resolution_clock::now() + std::chrono::seconds(1));
			else
				clients[key].is_move = false;

			delete lpover_ex;
		}
		else {
			//delete lpover_ex;
		}
	}
}

void do_accept()
{
	// Winsock Start - windock.dll 로드
	WSADATA WSAData;
	if (WSAStartup(MAKEWORD(2, 2), &WSAData) != 0)
	{
		cout << "Error - Can not load 'winsock.dll' file\n";
		return;
	}

	// 1. 소켓생성  
	SOCKET listenSocket = WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (listenSocket == INVALID_SOCKET)
	{
		cout << "Error - Invalid socket\n";
		return;
	}

	// 서버정보 객체설정
	SOCKADDR_IN serverAddr;
	memset(&serverAddr, 0, sizeof(SOCKADDR_IN));
	serverAddr.sin_family = PF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	serverAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	// 2. 소켓설정
	if (::bind(listenSocket, (struct sockaddr*)&serverAddr, sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
	{
		cout << "Error - Fail bind\n";
		// 6. 소켓종료
		closesocket(listenSocket);
		// Winsock End
		WSACleanup();
		return;
	}

	// 3. 수신대기열생성
	if (listen(listenSocket, 5) == SOCKET_ERROR)
	{
		cout << "Error - Fail listen\n";
		// 6. 소켓종료
		closesocket(listenSocket);
		// Winsock End
		WSACleanup();
		return;
	}

	SOCKADDR_IN clientAddr;
	int addrLen = sizeof(SOCKADDR_IN);
	memset(&clientAddr, 0, addrLen);
	SOCKET clientSocket;
	DWORD flags;

	while (true)
	{
		clientSocket = accept(listenSocket, (struct sockaddr *)&clientAddr, &addrLen);
		if (clientSocket == INVALID_SOCKET)
		{
			cout << "Error - Accept Failure\n";
			return;
		}

		char ID[10];
		int retval = recv(clientSocket, ID, sizeof(ID), 0);
		if (retval == SOCKET_ERROR)
			cout << "Recv Error\n";

		int new_id = atoi(ID);

		if (clients[new_id].connected)
		{
			closesocket(clientSocket);
			continue;
		}
		else if (new_id == -1)
		{
			closesocket(clientSocket);
			continue;
		}

		DB_SQL_GET_POS(new_id);

		clients[new_id].socket = clientSocket;
		clients[new_id].over.dataBuffer.len = MAX_BUFFER;
		clients[new_id].over.dataBuffer.buf = clients[clientSocket].over.messageBuffer;
		clients[new_id].over.act = ACT::RECV;
		clients[new_id].viewlist.clear();
		clients[new_id].prev_size = 0;
		ZeroMemory(&clients[new_id].over.overlapped, sizeof(WSAOVERLAPPED));
		flags = 0;

		CreateIoCompletionPort(reinterpret_cast<HANDLE>(clientSocket), g_iocp, new_id, 0);
		clients[new_id].connected = true;

		send_login_ok_packet(new_id);
		send_put_player_packet(new_id, new_id);

		for (int i = 0; i < MAX_USER; ++i) {
			if (false == clients[i].connected) continue;
			if (i == new_id) continue;
			if (false == is_near_object(i, new_id)) continue;
			clients[new_id].access_lock.lock();
			clients[new_id].viewlist.insert(i);
			clients[new_id].access_lock.unlock();
			send_put_player_packet(new_id, i);
		}

		for (int i = 0; i < MAX_USER; ++i) {
			if (false == clients[i].connected) continue;
			if (i == new_id) continue;
			if (false == is_near_object(i, new_id)) continue;
			clients[i].access_lock.lock();
			clients[i].viewlist.insert(new_id);
			clients[i].access_lock.unlock();
			send_put_player_packet(i, new_id);
		}


		for (int i = 0; i < NUM_NPC; ++i) {
			int npc_id = i + NPC_ID_START;
			if (false == clients[npc_id].connected) continue;
			if (false == is_near_object(npc_id, new_id)) continue;
			wake_up_npc(npc_id);
			clients[new_id].access_lock.lock();
			clients[new_id].viewlist.insert(npc_id);
			clients[new_id].access_lock.unlock();
			send_put_player_packet(new_id, npc_id);
		}
		do_recv(new_id);
	}

	// 6-2. 리슨 소켓종료
	closesocket(listenSocket);

	// Winsock End
	WSACleanup();

	return;
}

void timer()
{
	while (true) {
		this_thread::sleep_for(1ms);
		while (true) {
			t_lock.lock();
			if (true == timer_queue.empty()) {
				t_lock.unlock();  break;
			}
			TIMER_EVENT ev = timer_queue.top();
			if (ev.exec_time <= std::chrono::high_resolution_clock::now()) {
				timer_queue.pop();
				t_lock.unlock();
				OVER_EX *ex_over = new OVER_EX;
				ex_over->act = MOVE;
				PostQueuedCompletionStatus(g_iocp, 1, ev.id, &
					ex_over->overlapped);
			}
			else {
				t_lock.unlock();  break;
			}
		}
	}
}

void save_pos_db()
{
	std::chrono::high_resolution_clock::time_point save_time = std::chrono::high_resolution_clock::now();
	while (true)
	{
		if (save_time + 10s <= std::chrono::high_resolution_clock::now())
		{
			for (int i = 0; i < MAX_USER; ++i)
			{
				if (clients[i].connected)
				{
					DB_SQL_SET_POS(i);
				}
			}
			save_time = std::chrono::high_resolution_clock::now();
		}
	}
}

int main()
{
	std::vector <std::thread> worker_threads;
	wcout.imbue(locale("korean"));
	initialize();
	g_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	for (int i = 0; i < 6; ++i)
		worker_threads.emplace_back(std::thread{ worker_thread });
	std::thread accept_thread{ do_accept };
	std::thread timer_thread{ timer };
	std::thread save_thread{ save_pos_db };

	accept_thread.join();
	timer_thread.join();
	save_thread.join();
	for (auto &th : worker_threads) th.join();
	CloseHandle(g_iocp);
	SQLCancel(hstmt);
	SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
	SQLDisconnect(hdbc);
	SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
	SQLFreeHandle(SQL_HANDLE_ENV, henv);
}

//wsprintf((LPWSTR)Query, L"UPDATE user_table SET user_posx = %d, user_posy = %d WHERE user_id = %d", clients[id].x, clients[id].y, id);
//retcode = SQLExecDirect(hstmt, (SQLWCHAR *)Query, SQL_NTS);
//
//retcode = SQLBindCol(hstmt, 1, SQL_C_CHAR, UserId, NAME_LEN, &cbUserId);// 스트링이니까 최대길이 name_len
//retcode = SQLBindCol(hstmt, 2, SQL_C_LONG, &UserX, 100, &cbUserX);// 
//retcode = SQLBindCol(hstmt, 3, SQL_C_LONG, &UserY, 100, &cbUserY);//인티져니까 sLevel에 &
//
//for (int i = 0; ; i++) {
//	retcode = SQLFetch(hstmt);
//	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
//		wprintf(L"%d: %S %d %d\n", i + 1, UserId, UserX, UserY);// wprintf여서 한글도 제대로 나옴
//}