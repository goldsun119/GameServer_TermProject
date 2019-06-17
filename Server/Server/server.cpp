#include "server.h"
#include "2019_화목_protocol.h"

SOCKETINFO clients[NPC_ID_START + NUM_NPC];

HANDLE g_iocp;

std::chrono::high_resolution_clock::time_point server_timer;

int main() 
{
	std::vector <std::thread * > worker_threads;
	std::wcout.imbue(std::locale("korean"));
	initialize();
	check_player_level(0);
	g_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, NULL, 0);
	server_timer = high_resolution_clock::now();

	std::thread timer_thread{ do_timer };

	for (int i = 0; i < 6; ++i) {
		worker_threads.emplace_back(new std::thread{ worker_thread });
	}

	std::thread accept_tread{ do_accept };
	timer_thread.join();
	accept_tread.join();
	for (auto pth : worker_threads) {
		pth->join();
		delete pth;
	}

	worker_threads.clear();
	shutdown_server();
}


void initialize()
{
	read_map();
	init_npc();
	DB_init();

	for (int i = 0; i < MAX_USER; ++i)
		clients[i].connected = false;
}


void worker_thread()
{
	while (true) 
	{
		DWORD io_byte;
		unsigned long long key;
		OVER_EX *lpover_ex;
		BOOL is_error = GetQueuedCompletionStatus(g_iocp, &io_byte, &key, reinterpret_cast<LPWSAOVERLAPPED *>(&lpover_ex), INFINITE);

		if (FALSE == is_error) 
		{
			int err_no = WSAGetLastError();
			if (err_no != 64)
				error_display("GQCS : ", WSAGetLastError());
			else
			{
				disconnect(static_cast<int>(key));
			}
		}
		if (0 == io_byte) {
			disconnect(static_cast<int>(key));
			continue;
		}

		if (EV_RECV == lpover_ex->event) {
			int rest_size = io_byte;
			char *ptr = lpover_ex->messageBuffer;
			char packet_size = 0;
			if (0 < clients[key].prev_size) packet_size = clients[key].packet_buf[0];
			while (rest_size > 0)
			{
				if (0 == packet_size)packet_size = ptr[0];
				int required = packet_size - clients[key].prev_size;
				if (rest_size >= required)
				{
					memcpy(clients[key].packet_buf + clients[key].prev_size, ptr, required);
					process_packet(static_cast<int>(key), clients[key].packet_buf);
					rest_size -= required;
					ptr += required;
					packet_size = 0;
				}
				else
				{
					memcpy(clients[key].packet_buf + clients[key].prev_size, ptr, rest_size);
					rest_size = 0;
				}
			}
			do_recv(static_cast<int>(key));
		}
		else if (EV_SEND == lpover_ex->event) {
			//if (io_byte != lpover_ex->dataBuffer.len) { // 같아야 되는데 다르므로 
			//	closesocket(clients[key].socket);
			//	clients[key].connected = false;
			//	exit(-1);
			//}
			delete lpover_ex;
		}
		else if (EV_DO_AI == lpover_ex->event) {
			check_npc_hp(key);
			if (clients[key].level != 0 && clients[key].npc_chase_player == -1) {
				random_move_npc(key);
			}
			delete lpover_ex;
		}
		else if (EV_PLAYER_MOVE == lpover_ex->event) {
			clients[key].v_lock.lock();
			lua_State *L = clients[key].L;
			lua_getglobal(L, "event_player_move");
			lua_pushnumber(L, lpover_ex->event_from);
			if (0 != lua_pcall(L, 1, 0, 0)) {
				std::cout << "LUA error calling [event_player_move] : " << (char *)lua_tostring(L, -1) << std::endl;
			}
			clients[key].v_lock.unlock();
			delete lpover_ex;
		}
		else if (EV_MONSTER_ATTACK_MOVE == lpover_ex->event) {
			//printf( "NPC[%d] Move Client[%d]\t", ci, g_clients[ci].npc_Client );
			move_npc_to_player(clients[key].npc_chase_player, key);
			delete lpover_ex;
		}
		else if (EV_RESPAWN == lpover_ex->event) {
			respawn_npc(key);
			delete lpover_ex;
		}
	}
}

void do_accept() {

	WSADATA	wsadata;
	WSAStartup(MAKEWORD(2, 2), &wsadata);


	SOCKET listensocket = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

	SOCKADDR_IN ServerAddr;
	ZeroMemory(&ServerAddr, sizeof(SOCKADDR_IN));
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_port = htons(SERVER_PORT);
	ServerAddr.sin_addr.s_addr = INADDR_ANY;


	bind(listensocket, reinterpret_cast<sockaddr *>(&ServerAddr), sizeof(ServerAddr));
	listen(listensocket, 5);


	SOCKADDR_IN ClientAddr;
	memset(&ClientAddr, 0, sizeof(SOCKADDR_IN));
	ClientAddr.sin_family = PF_INET;
	ClientAddr.sin_port = htons(SERVER_PORT);
	ClientAddr.sin_addr.s_addr = INADDR_ANY;
	int addr_size = sizeof(ClientAddr);
	SOCKET new_client;
	while (true) {
		new_client = accept(listensocket, (struct sockaddr *)&ClientAddr, &addr_size);
		if (INVALID_SOCKET == new_client) {
			int err_no = WSAGetLastError();
			//error_display("WSAAccept : ", err_no);
		}

		int new_id = -1;

		for (int i = 0; i < MAX_USER; ++i) {
			if (clients[i].connected == false) {
				new_id = i;
			}
		}

		if (-1 == new_id) {
			closesocket(new_client);
			continue;
		}

		char  buf[255];
		int retval = recv(new_client, buf, 10, 0);
		if (retval == SOCKET_ERROR) {
			std::cout << "Not Recv Game_ID : " << new_id << std::endl;
			closesocket(new_client);
			continue;
		}
		buf[retval] = '\0';

		strcpy(clients[new_id].nickname, buf);
		int is_accept = DB_get_info(new_id);
		if (is_accept == DB_Success&& db_connect == 0) {
			strcpy(buf, "Okay");
			retval = send(new_client, buf, strlen(buf), 0);
		}
		else if (is_accept == DB_NoConnect) {
			strcpy(buf, "False");
			retval = send(new_client, buf, strlen(buf), 0);
		}
		else if (db_connect == 1) {
			strcpy(buf, "Overlap");
			retval = send(new_client, buf, strlen(buf), 0);
			closesocket(new_client);
			continue;
		}
		else if (is_accept == DB_NoData) {
			strcpy(buf, "Newid");
			clients[new_id].x = START_X;
			clients[new_id].y = START_Y;
			clients[new_id].exp = 0;
			clients[new_id].level = 1;
			clients[new_id].hp = 100;
			clients[new_id].max_hp = clients[new_id].hp;
			DB_new_id(new_id);
			DB_get_info(new_id);
			retval = send(new_client, buf, strlen(buf), 0);
		}
		//---------------------------------------------------------------------------------------------------------------------------------------------------
		// 들어온 접속 아이디 init 처리
		std::cout << "id : " << new_id << std::endl;
		clients[new_id].connected = true;
		clients[new_id].socket = new_client;
		clients[new_id].prev_size = 0;
		ZeroMemory(&clients[new_id].over.overlapped, sizeof(WSAOVERLAPPED));
		clients[new_id].over.event = EV_RECV;
		clients[new_id].over.dataBuffer.len = MAX_BUFFER;
		clients[new_id].over.dataBuffer.buf = clients[new_id].over.messageBuffer;
		clients[new_id].x = dbX;
		clients[new_id].y = dbY;
		clients[new_id].level = dbLevel;
		clients[new_id].skill_1 = dbSkill[0];
		clients[new_id].skill_2 = dbSkill[1];
		clients[new_id].skill_3 = dbSkill[2];
		clients[new_id].hp_timer = 0;
		clients[new_id].hp = dbHP;
		clients[new_id].max_hp = dbMaxHP;
		clients[new_id].dir = 2;
		clients[new_id].ani = 0;
		//---------------------------------------------------------------------------------------------------------------------------------------------------
		DWORD recv_flag = 0;
		CreateIoCompletionPort(reinterpret_cast<HANDLE>(new_client), g_iocp, new_id, 0);
		int ret = WSARecv(new_client, &clients[new_id].over.dataBuffer, 1, NULL, &recv_flag, &clients[new_id].over.overlapped, NULL);

		if (0 != ret) {
			int error_no = WSAGetLastError();
			if (WSA_IO_PENDING != error_no) {
				//error_display("RecvPacket:WSARecv", error_no);
				//while ( true );
			}
		}

		send_add_object_packet(new_id, new_id);

		std::unordered_set<int> local_view_list;
		for (int i = 0; i < NPC_ID_START + NUM_NPC; ++i)
			if (clients[i].connected == true)
				if (i != new_id)
					if (distance(i, new_id, VIEW_RADIUS) == true) {
						local_view_list.insert(i);
						send_add_object_packet(new_id, i);

						if (is_npc(i))
							wake_up_npc(i);

						if (clients[i].connected == false)
							continue;

						clients[i].v_lock.lock();
						if (clients[i].viewlist.count(new_id) == 0) {
							clients[i].viewlist.insert(new_id);
							clients[i].v_lock.unlock();
							send_add_object_packet(i, new_id);
						}
						else clients[i].v_lock.unlock();
					}
		clients[new_id].v_lock.lock();
		for (auto p : local_view_list) clients[new_id].viewlist.insert(p);
		clients[new_id].v_lock.unlock();
	}
}

void shutdown_server() 
{
	for (int i = NPC_ID_START; i < NPC_ID_START + NUM_NPC; ++i)
		lua_close(clients[i].L);
	CloseHandle(g_iocp);
}

void disconnect(int id) {
	for (int i = 0; i < MAX_USER; ++i)
	{
		if (false == clients[i].connected) continue;
		clients[i].v_lock.lock();
		if (0 != clients[i].viewlist.count(id))
		{
			clients[i].viewlist.erase(id);
			clients[i].v_lock.unlock();
			send_remove_object_packet(i, id);
		}
		else
		{
			clients[i].v_lock.unlock();
		}
	}
	closesocket(clients[id].socket);
	clients[id].v_lock.lock();//LOCK
	clients[id].viewlist.clear();
	clients[id].v_lock.unlock();//UNLOCK
	clients[id].connected = false;
	std::cout << "Disconnect Client : " << id << std::endl;
}

void process_packet(int ci, char *packet) {
	switch (packet[1]) {
	case CS_MOVE:
	{
		cs_packet_move *my_packet = reinterpret_cast<cs_packet_move *>(packet);
		switch (my_packet->direction)
		{
		case 0:
			clients[ci].dir = 0;
			if (clients[ci].y > 0 && collision_check(ci, 0)) {
				clients[ci].y--;
				clients[ci].ani += 1;
				clients[ci].last_move_time = std::chrono::high_resolution_clock::now();
			}
			break;
		case 1:
			clients[ci].dir = 1;
			if (clients[ci].y < WORLD_HEIGHT - 1 && collision_check(ci, 1)) {
				clients[ci].y++;
				clients[ci].ani += 1;
				clients[ci].last_move_time = std::chrono::high_resolution_clock::now();
			}
			break;
		case 2:
			clients[ci].dir = 2;
			if (clients[ci].x > 0 && collision_check(ci, 2)) {
				clients[ci].x--;
				clients[ci].ani += 1;
				clients[ci].last_move_time = std::chrono::high_resolution_clock::now();
			}
			break;
		case 3:
			clients[ci].dir = 3;
			if (clients[ci].x < WORLD_WIDTH - 1 && collision_check(ci, 3)) {
				clients[ci].x++;
				clients[ci].ani += 1;
				clients[ci].last_move_time = std::chrono::high_resolution_clock::now();
			}
			break;
		}

	}
	break;
	case CS_ATTACK:
	{//printf( "%d가 공격 %d만큼\n",ci, my_packet->damage );
		cs_packet_attack *my_packet = reinterpret_cast<cs_packet_attack *>(packet);
		check_attack(ci, packet);
	}
	break;
	case CS_TELEPORT:
	{	//printf( "%d가 공격 %d만큼\n",ci, my_packet->damage );
		cs_packet_attack *my_packet = reinterpret_cast<cs_packet_attack *>(packet);
		player_teleport(ci, packet);
	}
	break;
	default:
		exit(-1);
	}

	if (clients[ci].ani >= 2) clients[ci].ani = 0; // 움직임은 0~2까지 있다.

	send_position_packet(ci, ci); // 자기 자신한테 안보내줘서

	std::unordered_set<int> new_view_list;
	for (int i = 0; i < MAX_USER; ++i)
		if (clients[i].connected == true)
			if (i != ci) {
				// 자기 자신은 넣으면 안된다.
				if (true == distance(ci, i, VIEW_RADIUS))
					new_view_list.insert(i);
			}

	//------------------------------------------------------------------------------
	for (int i = NPC_ID_START; i < NPC_ID_START + NUM_NPC; ++i)
		if (distance(ci, i, VIEW_RADIUS) == true) new_view_list.insert(i);
	//------------------------------------------------------------------------------

	// 새로 추가되는 객체
	std::unordered_set<int> vlc;
	clients[ci].v_lock.lock();
	vlc = clients[ci].viewlist;
	clients[ci].v_lock.unlock();

	for (auto target : new_view_list)
		if (vlc.count(target) == 0) {
			// 새로 추가되는 객체
			send_add_object_packet(ci, target);
			vlc.insert(target);

			if (is_npc(target) == true)
				wake_up_npc(target);

			if (true != is_player(target)) continue;

			clients[target].v_lock.lock();
			if (clients[target].viewlist.count(ci) == 0) {
				clients[target].viewlist.insert(ci);
				clients[target].v_lock.unlock();
				send_add_object_packet(target, ci);
			}
			else {
				clients[target].v_lock.unlock();
				send_position_packet(target, ci);
			}

		}
		else {
			// 변동없이 존재하는 객체
			if (is_npc(target) == true) continue;
			// npc는 처리할 필요가 없다.

			clients[target].v_lock.lock();
			if (0 != clients[target].viewlist.count(ci)) {
				clients[target].v_lock.unlock();
				send_position_packet(target, ci);
			}
			else {
				clients[target].viewlist.insert(ci);
				clients[target].v_lock.unlock();
				send_add_object_packet(target, ci);
			}
		}

	// 시야에서 멀어진 객체
	std::unordered_set<int> removed_id_list;
	for (auto target : vlc) // 전에는 있었는데
		if (new_view_list.count(target) == 0) { //새로운 리스트에는 없다.
			removed_id_list.insert(target);
			send_remove_object_packet(ci, target);

			if (is_npc(target) == true) continue;
			// npc는 필요 없다.

			clients[target].v_lock.lock();
			if (0 != clients[target].viewlist.count(ci)) {
				clients[target].viewlist.erase(ci);
				clients[target].v_lock.unlock();
				send_remove_object_packet(target, ci);
			}
			else {
				clients[target].v_lock.unlock();
			}
		}

	clients[ci].v_lock.lock();
	for (auto p : vlc)
		clients[ci].viewlist.insert(p);
	for (auto d : removed_id_list)
		clients[ci].viewlist.erase(d);
	clients[ci].v_lock.unlock();

	for (auto npc : new_view_list) {
		if (false == is_npc(npc)) continue;
		OVER_EX *over = new OVER_EX;
		over->event = EV_PLAYER_MOVE;
		over->event_from = ci;
		PostQueuedCompletionStatus(g_iocp, 1, npc, &over->overlapped);
	}
}

void do_timer() {
	for (;;) {
		Sleep(10);
		for (;;) {
			if (high_resolution_clock::now() - server_timer >= 1s) {
				check_player_hp();
				server_timer = high_resolution_clock::now();
			}
			timer_lock.lock();
			if (0 == timer_queue.size()) {
				timer_lock.unlock();
				break;
			} 
			T_EVENT t = timer_queue.top(); // 여러 이벤트 중에 실행시간이 제일 최근인 이벤트를 실행해야 하므로 우선순위 큐를 만듬
			if (t.start_time > high_resolution_clock::now()) {
				timer_lock.unlock();
				break; // 현재시간보다 크다면, 기다려줌
			}
			timer_queue.pop();
			timer_lock.unlock();
			OVER_EX *over = new OVER_EX;
			if (EV_MOVE == t.event_type) {
				over->event = EV_DO_AI;
			}
			if (EV_MONSTER_ATTACK_MOVE == t.event_type) {
				over->event = EV_MONSTER_ATTACK_MOVE;
			}
			if (EV_RESPAWN == t.event_type) {
				over->event = EV_RESPAWN;
			}
			PostQueuedCompletionStatus(g_iocp, 1, t.do_object, &over->overlapped);
		}
	}
}

bool is_player(int ci) {
	if (ci < MAX_USER) return true;
	else return false;
}

bool is_npc(int id) {
	if ((id >= NPC_ID_START) && (id < NPC_ID_START + NUM_NPC)) return true;
	else return false;
}

void check_player_hp() {
	for (int ci = 0; ci < MAX_USER; ++ci) {
		if (clients[ci].connected == false) continue;
		if (clients[ci].hp <= 0) {
			// HP가 0이 되었을경우 초기 위치로 이동 & 위치 보내주기
			clients[ci].x = START_X;
			clients[ci].y = START_Y;
			clients[ci].hp = clients[ci].max_hp;
			clients[ci].exp /= 2;
			send_position_packet(ci, ci);
		}
		if (clients[ci].skill_1 != 0)  clients[ci].skill_1 -= 1;
		if (clients[ci].skill_2 != 0)  clients[ci].skill_2 -= 1;
		if (clients[ci].skill_3 != 0)  clients[ci].skill_3 -= 1;

		DB_set_info(ci);

		//5초마다 10% hp 회복
		if (clients[ci].hp_timer >= 5) {
			clients[ci].hp_timer = 0;
			if (clients[ci].max_hp <= clients[ci].hp + (clients[ci].max_hp % 10)) {
				clients[ci].hp = clients[ci].max_hp;
			}
			else {
				clients[ci].hp += clients[ci].max_hp % 10;
			}
		}
		clients[ci].hp_timer += 1;


		check_player_level(ci);
		send_stat_change_packet(ci, ci);
	}
}

void check_player_level(int ci) 
{
	// 1레벨 100
	// 2레벨 200
	// 3레벨 400
	unsigned long long exp_data = 100;

	//for ( int i = 1; i < 50; ++i ) {
	//	printf( "%d레벨 : %lld\n", i , exp_data );
	//	exp_data += exp_data;
	//}

	for (int i = 1; i < clients[ci].level; ++i) {
		exp_data += exp_data;
	}

	if (clients[ci].exp >= exp_data) {
		clients[ci].exp = 0;
		clients[ci].max_hp += (clients[ci].level * 20);
		clients[ci].hp = clients[ci].max_hp;
		clients[ci].level += 1;
	}

}

void player_teleport(int ci, char * ptr) 
{
	cs_packet_teleport *my_packet = reinterpret_cast<cs_packet_teleport *>(ptr);
	clients[ci].x = my_packet->x;
	clients[ci].y = my_packet->y;

	send_position_packet(ci, ci);
}

void error_display(const char *msg, int err_no) {
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, err_no,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	printf("[%s] %s\n", msg, (char *)lpMsgBuf);
	LocalFree(lpMsgBuf);
}

void send_packet(int key, void *packet) {
	if (clients[key].connected == true) {
		int psize = reinterpret_cast<char *>(packet)[0];
		OVER_EX *over = new OVER_EX;
		ZeroMemory(&over->overlapped, sizeof(WSAOVERLAPPED));
		over->event = EV_SEND;
		memcpy(over->messageBuffer, packet, psize);
		over->dataBuffer.len = psize;
		over->dataBuffer.buf = over->messageBuffer;
		int res = WSASend(clients[key].socket, &over->dataBuffer, 1, NULL, 0, &over->overlapped, NULL);
		if (0 != res) {
			int error_no = WSAGetLastError();
			if (WSA_IO_PENDING != error_no) {
				error_display("SendPacket:WSASend", error_no);
				disconnect(key);
			}
		}
	}
}

void send_chat_packet(int to, int from, wchar_t *message) 
{
	sc_packet_chat packet;
	packet.id = from;
	packet.size = sizeof(packet);
	packet.type = SC_CHAT;
	wcsncpy_s(packet.message, message,MAX_STR_LEN);
	send_packet(to, reinterpret_cast<void *>(&packet));
}

void send_stat_change_packet(int client, int object) {
	sc_packet_stat_change packet;
	packet.id = object;
	packet.size = sizeof(packet);
	packet.type = SC_STAT_CHANGE;
	packet.HP = clients[object].hp;
	packet.LEVEL = clients[object].level;
	packet.EXP = clients[object].exp;
	packet.skill1 = clients[object].skill_1;
	packet.skill2 = clients[object].skill_2;
	packet.skill3 = clients[object].skill_3;
	send_packet(client, &packet);
}

void send_add_object_packet(int client, int object) {
	sc_packet_add_object packet;
	packet.id = object;
	packet.size = sizeof(packet);
	packet.type = SC_ADD_OBJECT;
	packet.x = clients[object].x;
	packet.y = clients[object].y;
	packet.dir = clients[object].dir;
	packet.ani = clients[object].ani;

	send_stat_change_packet(client, object);
	send_packet(client, &packet);
}

void send_position_packet(int client, int object) {
	sc_packet_position packet;
	packet.id = object;
	packet.size = sizeof(packet);
	packet.type = SC_POSITION;
	packet.x = clients[object].x;
	packet.y = clients[object].y;
	packet.dir = clients[object].dir;
	packet.ani = clients[object].ani;
	send_stat_change_packet(client, object);
	send_packet(client, &packet);
}

void send_remove_object_packet(int client, int object) {
	sc_packet_remove_object packet;
	packet.id = object;
	packet.size = sizeof(packet);
	packet.type = SC_REMOVE_OBJECT;

	send_packet(client, &packet);
}

void do_recv(int id)
{
	DWORD flags = 0;

	SOCKET clients_s = clients[id].socket;
	OVER_EX *over = &clients[id].over;

	over->dataBuffer.len = MAX_BUFFER;
	over->dataBuffer.buf = over->messageBuffer;
	ZeroMemory(&(over->overlapped), sizeof(WSAOVERLAPPED));
	if (WSARecv(clients_s, &over->dataBuffer, 1, NULL, &flags, &(over->overlapped), NULL) == SOCKET_ERROR)
	{
		int err_no = WSAGetLastError();
		if (err_no != WSA_IO_PENDING)
		{
			error_display("RECV_ERROR", err_no);
		}
	}
}