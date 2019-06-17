#include "server.h"
#include "2019_ȭ��_protocol.h"

priority_queue <T_EVENT> timer_queue;
mutex timer_lock;

int init_x[5]{ 1, 1, 2, 2, 0 };
int init_y[5]{ 1, 2, 1, 2, 0 };
int init_level[5]{ 2 ,6, 10, 14, 20 };
int init_count[5]{ 200, 200, 200, 200, 20 };


void send_add_exp(int id, int npc)
{
	char m[MAX_STR_LEN];
	wchar_t message[MAX_STR_LEN];
	size_t wlen;
	if (clients[id].connected)
	{
		int exp = clients[npc].level * 5;
		if (clients[npc].npc_type == NPC_WAR|| clients[npc].npc_type == NPC_MOVE_PEACE)
		{
			exp *= 2;
		}
		clients[id].exp += exp;
	
		sprintf(m, "%d�� ���� %d�� ����ġ�� ������ϴ�.", npc, exp);
		mbstowcs_s(&wlen, message, strlen(m), m, _TRUNCATE);
		send_position_packet(id, id);
		send_chat_packet(id, npc, message);
	}
}

void check_npc_hp(int npc)
{
	if (clients[npc].hp <= 0&&clients[npc].exp == NPC_NO_SEND_EXP)
	{
		clients[npc].exp = NPC_SEND_EXP;
		send_add_exp(clients[npc].npc_last_attack_player, npc);

		T_EVENT event = { npc, high_resolution_clock::now() + 30s ,EV_RESPAWN };  // ���� �������� 60�ʸ��� �ѹ��� Ȯ���Ѵ�.
		timer_lock.lock();
		timer_queue.push(event);
		timer_lock.unlock();
	}
}

void init_npc() 
{
	int npc = NPC_ID_START;
	for (int i = 0; i < 5; ++i)
	{
		for (int j = 0; j < init_count[i]; ++j)
		{
			clients[npc].x = random_pos(init_x[i]);
			clients[npc].y = random_pos(init_y[i]);
			while (collision_check(npc, 4))// ��ֹ��� �浹�ΰ�, �浹�̶��
			{
				clients[npc].x = random_pos(init_x[i]);
				clients[npc].y = random_pos(init_y[i]);
			}

			clients[npc].last_move_time = std::chrono::high_resolution_clock::now();
			clients[npc].is_sleeping = true;
			clients[npc].level = init_level[i];
			clients[npc].exp = NPC_NO_SEND_EXP;
			clients[npc].hp = 50 + (init_level[i] * 50);
			if (i == 0)
				clients[npc].npc_type = NPC_PEACE;
			if (i == 1)
				clients[npc].npc_type = NPC_MOVE_PEACE;
			if (i == 2)
				clients[npc].npc_type = NPC_MOVE_PEACE;
			if (i == 3)
				clients[npc].npc_type = NPC_WAR;
			else
				clients[npc].npc_type = NPC_WAR;
			clients[npc].npc_attack = 5 + (init_level[i] * 5);
			clients[npc].npc_x = clients[npc].x;//�ʱ���ġ
			clients[npc].npc_y = clients[npc].y;
		}
	}

	for (int i = NPC_ID_START; i < NPC_ID_START + NUM_NPC; ++i)
	{
		lua_State *L = luaL_newstate();
		luaL_openlibs(L);
		luaL_loadfile(L, "AI\\NPC_AI.lua");
		if (lua_pcall(L, 0, 0, 0) != 0)
		{
			cout << "LUA file loading error" << endl;
		}

		lua_getglobal(L, "set_NPC");
		lua_pushnumber(L, i);
		lua_pushnumber(L, clients[i].level);
		lua_pushnumber(L, clients[i].hp);
		lua_pushnumber(L, clients[i].npc_type);
		lua_pushnumber(L, clients[i].npc_x);
		lua_pushnumber(L, clients[i].npc_y);
		if (lua_pcall(L, 6, 1, 0) != 0)
		{
			cout << "set npc status script error" << endl;
		}

		lua_pop(L, 1);

		lua_register(L, "API_get_x", API_get_x);
		lua_register(L, "API_get_y", API_get_y);
		lua_register(L, "API_get_hp", API_get_hp);
		lua_register(L, "API_Send_Message", API_Send_Message);
		clients[i].L = L;
	}
	cout << "LUA load complete" << endl;
}

int random_pos(int x)
{
	std::random_device rd;
	std::mt19937_64 rnd(rd());
	if (x == 0)
	{
		std::uniform_int_distribution<int> range(0, 300);
		return range(rnd);
	}
	else if (x == 1)
	{
		std::uniform_int_distribution<int> range(150, 300);
		return range(rnd);
	}
	else
	{
		std::uniform_int_distribution<int> range(0, 150);
		return range(rnd);
	}
}

void random_move_npc(int id)
{
	int x = clients[id].x;
	int y = clients[id].y;
	
	char dir = rand() % 4;
	switch (dir)
	{
	case 1: if (y > 0) y--; break;// 0~ 299
	case 3: if (y < WORLD_HEIGHT - 2) y++; break;
	case 0: if (x > 0) x--; break;
	case 2: if (x < WORLD_WIDTH - 2) x++; break;
	default:
		break;
	}

	clients[id].x = x;
	clients[id].y = y;

	while (collision_check(id, 4) == false) {
		dir = rand() % 4;
		switch (dir) {
		case 1: if (y > 0) y--; break;// 0~ 299
		case 3: if (y < WORLD_HEIGHT - 2) y++; break;
		case 0: if (x > 0) x--; break;
		case 2: if (x < WORLD_WIDTH - 2) x++; break;
		}
		clients[id].x = x;
		clients[id].y = y;
	}

	for (int player = 0; player < MAX_USER; ++player)
	{
		if (clients[player].connected)
		{
			if (clients[id].npc_type == NPC_WAR && distance(id, player, 10) && clients[id].npc_chase_player == -1 && clients[id].hp > 0)
			{
				clients[id].npc_dir = rand() % 4;
				clients[id].npc_chase_player = player;
				clients[id].npc_last_attack_player = player;
				T_EVENT e = { id, high_resolution_clock::now() + 1s, EV_MONSTER_ATTACK_MOVE };
				timer_lock.lock();
				timer_queue.push(e);
				timer_lock.unlock();
			}

			clients[player].v_lock.lock();
			if (clients[player].viewlist.count(id))//�÷��̾� �丮��Ʈ�� ���Ǿ� �ִٸ�
			{
				if (distance(player, id, 10))
				{
					clients[player].v_lock.unlock();
					send_position_packet(player, id);

				}
				else
				{
					clients[player].viewlist.erase(id);
					clients[player].v_lock.unlock();
					send_remove_object_packet(player, id);
				}

			}
			else
			{
				if(distance(player, id, 10))
				{
					clients[player].viewlist.insert(id);
					clients[player].v_lock.unlock();
					send_add_object_packet(player, id);
				}
				else
				{
					clients[player].v_lock.unlock();
				}
			}
		}
	 }

	for (int player = 0; player < MAX_USER; ++player)
	{
		if (clients[player].connected && distance(player, id, 10))
		{
			T_EVENT e = { id, high_resolution_clock::now() + 1s, EV_MOVE };
			timer_lock.lock();
			timer_queue.push(e);
			timer_lock.unlock();
			return;
		}
	}
	clients[id].is_sleeping = true;
}

bool distance(int id, int obj, int r)
{
	if (r < abs(clients[id].x - clients[obj].x))
		return false;
	if (r < abs(clients[id].y - clients[obj].y))
		return false;
	return true;
}

void send_monster_attack_chat(int player, int npc)
{
	if (clients[npc].hp > 0)
	{
		char m[MAX_STR_LEN];
		wchar_t message[MAX_STR_LEN];
		size_t wlen;
		clients[player].hp -= clients[npc].npc_attack;
		sprintf(m, "���� %d�� ������ %d�� �������� �������ϴ�.", npc, clients[npc].npc_attack);
		mbstowcs_s(&wlen, message, strlen(m), m, _TRUNCATE);
		send_position_packet(player, player);
		send_chat_packet(player, npc, message);
	}
}

void check_attack_player(int player, int npc)
{
	switch (clients[npc].dir)
	{
	case 0:
		if ((clients[player].x - 1 == clients[npc].x) && (clients[player].y == clients[npc].y)) {
			send_monster_attack_chat(player, npc);
		}
		break;
	case 1:
		if ((clients[player].x + 1 == clients[npc].x) && (clients[player].y == clients[npc].y)) {
			send_monster_attack_chat(player, npc);
		}
		break;
	case 2:
		if ((clients[player].x == clients[npc].x) && (clients[player].y - 1 == clients[npc].y)) {
			send_monster_attack_chat(player, npc);
		}
		break;
	case 3:
		if ((clients[player].x == clients[npc].x) && (clients[player].y + 1 == clients[npc].y)) {
			send_monster_attack_chat(player, npc);
		}
		break;
	default:
		break;
	}
}

void move_npc_to_player(int player, int npc)
{
	int player_x = clients[player].x, player_y = clients[player].y;
	int npc_x = clients[npc].x, npc_y = clients[npc].y;

	switch (clients[npc].npc_dir) {
	case 0:
		player_x -= 1;
		break;
	case 1:
		player_x += 1;
		break;
	case 2:
		player_y -= 1;
		break;
	case 3:
		player_y += 1;
		break;
	}

	if (npc_y > player_y  && collision_check(npc, 0))
		clients[npc].y -= 1;
	if (npc_y < player_y && collision_check(npc, 1))
		clients[npc].y += 1;
	if (npc_x > player_x && collision_check(npc, 2))
		clients[npc].x -= 1;
	if (npc_x < player_x  && collision_check(npc, 3))
		clients[npc].x += 1;

	check_attack_player(player, npc);

	for (int i = 0; i < MAX_USER; ++i) {
		if (true == clients[i].connected) {
			clients[i].v_lock.lock();
			if (0 != clients[i].viewlist.count(npc)) {
				if (true == distance(i, npc, VIEW_RADIUS)) {
					clients[i].v_lock.unlock();
					send_position_packet(i, npc);
				}
				else {
					clients[i].viewlist.erase(npc);
					clients[i].v_lock.unlock();
					send_remove_object_packet(i, npc); // npc�� �þ߿��� �־����� ������
				}
			}
			else {
				if (true == distance(i, npc, VIEW_RADIUS)) {
					clients[i].viewlist.insert(npc);
					clients[i].v_lock.unlock();
					send_add_object_packet(i, npc);
				}
				else {
					clients[i].v_lock.unlock();
				}
			}
		}
	}


	if (distance(npc, player, VIEW_RADIUS) && clients[npc].hp >= 0) {
		T_EVENT event = { npc, high_resolution_clock::now() + 1s ,EV_MONSTER_ATTACK_MOVE };
		timer_lock.lock();
		timer_queue.push(event);
		timer_lock.unlock(); // ���� �ɾ��ְ� ť�� �־���� �ȴ�
	}
	else {
		// NPC�� ���� �������� Ŭ�� �Ⱥ��̸� �������� �����.
		clients[npc].npc_last_attack_player = clients[npc].npc_chase_player;
		clients[npc].npc_chase_player = -1;
		T_EVENT event = { npc, high_resolution_clock::now() + 1s ,EV_MOVE };
		timer_lock.lock();
		timer_queue.push(event);
		timer_lock.unlock();
	}
}

void wake_up_npc(int id)
{
	if (clients[id].is_sleeping == false) return;
	clients[id].is_sleeping = false;
	T_EVENT e = { id, high_resolution_clock::now() + 1s, EV_MOVE };
	timer_lock.lock();
	timer_queue.push(e);
	timer_lock.unlock();
}

void attack_move(int npc, int ci, cs_packet_attack * my_packet) {
	char m[MAX_STR_LEN];
	wchar_t message[MAX_STR_LEN];
	size_t wlen;

	if (clients[npc].npc_chase_player == -1) {
		T_EVENT event = { npc, high_resolution_clock::now() + 1s ,EV_MONSTER_ATTACK_MOVE };
		timer_lock.lock();
		timer_queue.push(event);
		timer_lock.unlock(); // ���� �ɾ��ְ� ť�� �־���� �ȴ�
	}

	if (clients[npc].hp > 0) {
		clients[npc].npc_chase_player = ci;
		clients[npc].npc_last_attack_player = ci;
		clients[npc].hp -= my_packet->damage;
		clients[npc].npc_dir = rand() % 4;
		sprintf(m, "���Ϳ��� %d�� �������� �������ϴ�.", my_packet->damage);
		mbstowcs_s(&wlen, message, strlen(m), m, _TRUNCATE);
		send_chat_packet(ci, npc, message);
	}
}

void check_attack(int player, char * ptr)
{
	// �÷��̾ NPC�� �����ߴ��� Ȯ���ϱ�
	int x = clients[player].x, y = clients[player].y;
	cs_packet_attack *my_packet = reinterpret_cast<cs_packet_attack *>(ptr);

	if (my_packet->skill_num == 0) 
		clients[player].skill_1 = 1;
	if (my_packet->skill_num == 1) 
		clients[player].skill_2 = 2;
	if (my_packet->skill_num == 2) 
		clients[player].skill_3 = 3;

	for (int i = NPC_ID_START; i < NPC_ID_START +  NUM_NPC; ++i) {
		if (x - 1 == clients[i].x && y == clients[i].y) {
			// ����
			attack_move(i, player, my_packet);
		}
		if (x + 1 == clients[i].x && y == clients[i].y) {
			// ������
			attack_move(i, player, my_packet);
		}
		if (x == clients[i].x && y - 1 == clients[i].y) {
			// ��
			attack_move(i, player, my_packet);
		}
		if (x == clients[i].x && y + 1 == clients[i].y) {
			// �Ʒ�
			attack_move(i, player, my_packet);
		}
	}
}

void respawn_npc(int id)
{
	//std::cout << id << "responder" << std::endl;
	clients[id].hp = 50 + clients[id].level * 50;
	clients[id].npc_attack = clients[id].hp / 10;
	clients[id].x = clients[id].npc_x;
	clients[id].y = clients[id].npc_y;
	clients[id].exp = NPC_NO_SEND_EXP;
	clients[id].npc_chase_player = -1;
	clients[id].npc_last_attack_player = -1;
}