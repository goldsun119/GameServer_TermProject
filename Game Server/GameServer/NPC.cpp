#include "Server.h"
#include "2019_����_protocol.h"

std::mutex tq_lock;
std::priority_queue <Timer_Event, std::vector<Timer_Event>, comparison> timer_queue;

//-----------------------------------------------------------------------------------------------------
// NPC ���� ����.
int init_x[14]{ 4, 93, 137, 137, 137, 225, 199, 37, 209, 6, 54, 123, 40, 183 };
int init_x_E[14]{ 45, 163, 178, 178, 178, 271, 280, 142, 294, 39, 108, 175, 67, 195 };
int init_y[14]{ 50, 6, 55, 85, 115, 44, 109, 157, 253, 244, 260, 260, 7, 151 };
int init_y_E[14]{ 100, 23, 76, 106, 130, 76, 132, 212, 295, 286, 286, 286, 31, 221 };
int init_Level[14]{ 0, 1, 0, 1, 2, 3, 4, 5, 6, 2, 3, 4, 1, 4 };
int init_count[14]{ 50, 30, 20, 20, 20, 50, 50, 100, 100, 100, 50, 50, 50, 100 };
//-----------------------------------------------------------------------------------------------------

void Send_Add_EXP( int ci, int npc ) {
	char chatMSG[MAX_STR_LEN];
	if ( g_clients[ci].connect == true ) {
		int exp = (g_clients[ci].level * 5) + (g_clients[npc].level + 1) * 5;
		if ( g_clients[npc].npc_type == N_War || g_clients[npc].level >= 1 ) {
			exp *= 2;
		}
		exp *= 10;
		g_clients[ci].exp += exp;
		sprintf( chatMSG, "���� %d�� ���񷯼� %d[+%d]�� ����ġ�� ������ϴ�.", npc, exp - (g_clients[npc].level + 1) * 5, (g_clients[npc].level + 1) * 5 );
		SendPositionPacket( ci, ci );
		SendChatPacket( ci, npc, ConverCtoWC( chatMSG ) );
	}
}

void check_Monster_HP(int npc) {
	// ü���� 0�ϰ��
	if ( g_clients[npc].hp <= 0 && g_clients[npc].exp == No_Send_EXP ) {
		g_clients[npc].exp = Yes_Send_EXP;
		Send_Add_EXP( g_clients[npc].npc_recent_Client, npc );
	}
}

void init_Monster_Create() {
	int npc = NPC_ID_START;
	for ( int i = 0; i < 14; ++i ) {
		//std::cout << "init Monster : " << i << std::endl;
		//std::cout << "x : " << init_x[i] << " ~ " << init_x_E[i] << std::endl;
		//std::cout << "y : " << init_y[i] << " ~ " << init_y_E[i] << std::endl;
		for ( int j = 0; j < init_count[i]; ++j ) {
			g_clients[npc].connect = false;
			g_clients[npc].x = getRandomNumber( init_x[i], init_x_E[i] ); // ���ʿ� �ѷ������ �ȱ׷��� ��� npc�� ���Ĺ���
			g_clients[npc].y = getRandomNumber( init_y[i], init_y_E[i] ); // ��������
			while ( CollisionCheck( npc, CS_NPC ) == false ) {
				g_clients[npc].x = getRandomNumber( init_x[i], init_x_E[i] ); // ���ʿ� �ѷ������ �ȱ׷��� ��� npc�� ���Ĺ���
				g_clients[npc].y = getRandomNumber( init_y[i], init_y_E[i] ); // ��������
			}
			g_clients[npc].last_move_time = std::chrono::high_resolution_clock::now(); // ��Ʈ ���� Ÿ�� �ʱ�ȭ
			g_clients[npc].is_active = false;
			g_clients[npc].level = init_Level[i];
			g_clients[npc].exp = No_Send_EXP;	// 1�ϰ�� Ŭ�󿡰� ����ġ�� �Ⱥ�����
			g_clients[npc].hp = 50 + (init_Level[i] * 50);
			if ( init_Level[i] >= 3 ) {
				g_clients[npc].npc_type = N_War;
			}
			else {
				g_clients[npc].npc_type = N_Peace;
			}
			g_clients[npc].npc_Attack = g_clients[npc].hp / 10;
			g_clients[npc].npc_x = g_clients[npc].x;
			g_clients[npc].npc_y = g_clients[npc].y;

			Timer_Event event = { npc, high_resolution_clock::now() + 60s ,E_Responder };  // ���� �������� 60�ʸ��� �ѹ��� Ȯ���Ѵ�.
			tq_lock.lock();
			timer_queue.push( event );
			tq_lock.unlock();

			++npc;
		}
	}
	std::cout << std::endl << "init Monster Complete" << std::endl;
}

void init_NPC() {
	init_Monster_Create();

	for ( int i = NPC_ID_START; i < NPC_ID_START + NUM_NPC; ++i ) {
		lua_State *L = luaL_newstate();
		luaL_openlibs( L );
		luaL_loadfile( L, "AI_Script\\NPC_AI.lua" );


		if ( 0 != lua_pcall( L, 0, 0, 0 ) ) {
			std::cout << "LUA error loading [AI_Script\\NPC_AI.lua] : " << (char *)lua_tostring( L, -1 ) << std::endl;
		}
		lua_getglobal( L, "set_NPC" );
		lua_pushnumber( L, i );
		lua_pushnumber( L, g_clients[i].level );
		lua_pushnumber( L, g_clients[i].hp );
		lua_pushnumber( L, g_clients[i].npc_type );

		if ( 0 != lua_pcall( L, 4, 1, 0 ) ) {
			std::cout << "LUA error calling [set_NPC] : " << (char *)lua_tostring( L, -1 ) << std::endl;
		}

		lua_pop( L, 1 );
		lua_register( L, "print_LUA", print_LUA );
		lua_register( L, "API_get_x", API_get_x );
		lua_register( L, "API_get_y", API_get_y );
		lua_register( L, "API_get_hp", API_get_hp );
		lua_register( L, "API_Send_Message", API_Send_Message );
		g_clients[i].L = L;
	}


	std::cout << std::endl << "Lua Load Complete!" << std::endl;
}

void NPC_Random_Move( int id ) {
	int x = g_clients[id].x;
	int y = g_clients[id].y;


	switch ( getRandomNumber( 0, 3 ) ) {
	case 0: if ( x > 0 )x--; break;
	case 1: if ( y > 0 ) y--; break;
	case 2: if ( x < WORLD_WIDTH - 2 ) x++; break;
	case 3: if ( y > -(WORLD_HEIGHT - 2) ) y++; break;
	}

	g_clients[id].x = x;
	g_clients[id].y = y;

	while ( CollisionCheck( id, CS_NPC ) == false ) {
		switch ( getRandomNumber( 0, 3 ) ) {
		case 0: if ( x > 0 )x--; break;
		case 1: if ( y > 0 ) y--; break;
		case 2: if ( x < WORLD_WIDTH - 2 ) x++; break;
		case 3: if ( y > -(WORLD_HEIGHT - 2) ) y++; break;
		}
		g_clients[id].x = x;
		g_clients[id].y = y;
	}

	//std::cout << id << " : " << g_clients[id].x << ", " << g_clients[id].y << std::endl;
	// ������ �ִ� �÷��̾�鿡�� npc�� �������ٰ� ������
	for ( int i = 0; i < MAX_USER; ++i ) {
		if ( true == g_clients[i].connect ) {

			if ( g_clients[id].npc_type == N_War && Distance( id, i, NPC_RADIUS ) && g_clients[id].npc_Client == -1 && g_clients[id].hp >= 0 ) {
				// ���� ������ 2 �̻��̰�, NPC �þ� ������ �÷��̾ ������
				g_clients[id].npc_dir = getRandomNumber( 0, 3 );
				g_clients[id].npc_Client = i;
				g_clients[id].npc_recent_Client = i;
				Timer_Event event = { id, high_resolution_clock::now() + 1s ,E_Attack_Move };
				tq_lock.lock();
				timer_queue.push( event );
				tq_lock.unlock(); // ���� �ɾ��ְ� ť�� �־���� �ȴ�
			}

			g_clients[i].vl_lock.lock();
			if ( 0 != g_clients[i].view_list.count( id ) ) {
				if ( true == Distance( i, id, VIEW_RADIUS ) ) {
					g_clients[i].vl_lock.unlock();
					SendPositionPacket( i, id );
				}
				else {
					g_clients[i].view_list.erase( id );
					g_clients[i].vl_lock.unlock();
					SendRemovePlayerPacket( i, id ); // npc�� �þ߿��� �־����� ������
				}
			}
			else {
				if ( true == Distance( i, id, VIEW_RADIUS ) ) {
					g_clients[i].view_list.insert( id );
					g_clients[i].vl_lock.unlock();
					SendPutPlayerPacket( i, id );
				}
				else {
					g_clients[i].vl_lock.unlock();
				}
			}
		}
	}

	for ( int i = 0; i < MAX_USER; ++i ) {
		if ( (true == g_clients[i].connect) && (Distance( i, id, VIEW_RADIUS )) ) {
			Timer_Event t = { id, high_resolution_clock::now() + 1s, E_MOVE };
			tq_lock.lock();
			timer_queue.push( t );
			tq_lock.unlock(); // ���� �ɾ��ְ� ť�� �־���� �ȴ�
			return;
		}
	}
	g_clients[id].is_active = false;
}

void NPC_Responder( int id ) {
	//std::cout << id << "responder" << std::endl;
	for ( int i = 0; i < 7; ++i ) {
		if ( g_clients[id].hp <= 0 && g_clients[id].level == i ) {
			//std::cout << id << "responder" << std::endl;
			g_clients[id].hp = 50 + (i * 50);;
			g_clients[id].npc_Attack = g_clients[id].hp / 10;
			g_clients[id].x = g_clients[id].npc_x;
			g_clients[id].y = g_clients[id].npc_y;
			g_clients[id].exp = No_Send_EXP;
			g_clients[id].npc_Client = -1;
			g_clients[id].npc_recent_Client = -1;
		}
	}

	Timer_Event event = { id, high_resolution_clock::now() + 60s ,E_Responder };  // ���� �������� 60�ʸ��� �ѹ��� Ȯ���Ѵ�.
	tq_lock.lock();
	timer_queue.push( event );
	tq_lock.unlock();
}

void Send_Monster_Attack_Chat( int ci, int npc ) {
	if ( g_clients[npc].hp > 0 ) {
		char chatMSG[MAX_STR_LEN];
		g_clients[ci].hp -= g_clients[npc].npc_Attack;
		sprintf( chatMSG, "���Ͱ� ��翡�� %d�� �������� �������ϴ�.", g_clients[npc].npc_Attack );
		SendPositionPacket( ci, ci ); // ü���� �Ҹ� �Ǿ��ٴ� ���� �ٽ� �����ֱ� ����
		SendChatPacket( ci, npc, ConverCtoWC( chatMSG ) );
	}
}

void Check_Attack_Client( int ci, int npc ) {
	switch ( g_clients[npc].npc_dir ) {
	case 0:
		if ( (g_clients[ci].x - 1 == g_clients[npc].x) && (g_clients[ci].y == g_clients[npc].y) ) {
			Send_Monster_Attack_Chat( ci, npc );
		}
		break;
	case 1:
		if ( (g_clients[ci].x + 1 == g_clients[npc].x) && (g_clients[ci].y == g_clients[npc].y) ) {
			Send_Monster_Attack_Chat( ci, npc );
		}
		break;
	case 2:
		if ( (g_clients[ci].x == g_clients[npc].x) && (g_clients[ci].y - 1 == g_clients[npc].y) ) {
			Send_Monster_Attack_Chat( ci, npc );
		}
		break;
	case 3:
		if ( (g_clients[ci].x == g_clients[npc].x) && (g_clients[ci].y + 1 == g_clients[npc].y) ) {
			Send_Monster_Attack_Chat( ci, npc );
		}
		break;
	}
}

void Move_NPCtoClient( int ci, int npc ) {
	int Cx = g_clients[ci].x, Cy = g_clients[ci].y;
	int Nx = g_clients[npc].x, Ny = g_clients[npc].y;

	switch ( g_clients[npc].npc_dir ) {
	case 0:
		Cx -= 1;
		break;
	case 1:
		Cx += 1;
		break;
	case 2:
		Cy -= 1;
		break;
	case 3:
		Cy += 1;
		break;
	}

	if ( Nx > Cx && CollisionCheck( npc, 2 ) )
		g_clients[npc].x -= 1;
	if ( Ny > Cy  && CollisionCheck( npc, 0 ) )
		g_clients[npc].y -= 1;
	if ( Nx < Cx  && CollisionCheck( npc, 3 ) )
		g_clients[npc].x += 1;
	if ( Ny < Cy && CollisionCheck( npc, 1 ) )
		g_clients[npc].y += 1;

	Check_Attack_Client( ci, npc );

	for ( int i = 0; i < MAX_USER; ++i ) {
		if ( true == g_clients[i].connect ) {
			g_clients[i].vl_lock.lock();
			if ( 0 != g_clients[i].view_list.count( npc ) ) {
				if ( true == Distance( i, npc, VIEW_RADIUS ) ) {
					g_clients[i].vl_lock.unlock();
					SendPositionPacket( i, npc );
				}
				else {
					g_clients[i].view_list.erase( npc );
					g_clients[i].vl_lock.unlock();
					SendRemovePlayerPacket( i, npc ); // npc�� �þ߿��� �־����� ������
				}
			}
			else {
				if ( true == Distance( i, npc, VIEW_RADIUS ) ) {
					g_clients[i].view_list.insert( npc );
					g_clients[i].vl_lock.unlock();
					SendPutPlayerPacket( i, npc );
				}
				else {
					g_clients[i].vl_lock.unlock();
				}
			}
		}
	}


	if ( Distance( npc, ci, NPC_RADIUS ) && g_clients[npc].hp >= 0 ) {
		Timer_Event event = { npc, high_resolution_clock::now() + 1s ,E_Attack_Move };
		tq_lock.lock();
		timer_queue.push( event );
		tq_lock.unlock(); // ���� �ɾ��ְ� ť�� �־���� �ȴ�
	}
	else {
		// NPC�� ���� �������� Ŭ�� �Ⱥ��̸� �������� �����.
		g_clients[npc].npc_recent_Client = g_clients[npc].npc_Client;
		g_clients[npc].npc_Client = -1;
		Timer_Event event = { npc, high_resolution_clock::now() + 1s ,E_MOVE };
		tq_lock.lock();
		timer_queue.push( event );
		tq_lock.unlock();
	}
}

void WakeUpNPC( int id ) {
	// �̹� ������ �ָ� �� �����̸� �ȵǹǷ� ���°� �ʿ���
	if ( g_clients[id].is_active == true ) return;
	g_clients[id].is_active = true;
	Timer_Event event = { id, high_resolution_clock::now() + 1s ,E_MOVE };
	tq_lock.lock();
	timer_queue.push( event );
	tq_lock.unlock();
}

void Attack_Move( int npc, int ci, cs_packet_attack * my_packet ) {
	char chatMSG[MAX_STR_LEN];
	if ( g_clients[npc].npc_Client == -1 ) {
		Timer_Event event = { npc, high_resolution_clock::now() + 1s ,E_Attack_Move };
		tq_lock.lock();
		timer_queue.push( event );
		tq_lock.unlock(); // ���� �ɾ��ְ� ť�� �־���� �ȴ�
	}

	if ( g_clients[npc].hp > 0 ) {
		g_clients[npc].npc_Client = ci;
		g_clients[npc].npc_recent_Client = ci;
		g_clients[npc].hp -= my_packet->damage;
		g_clients[npc].npc_dir = getRandomNumber( 0, 3 );
		sprintf( chatMSG, "��簡 ���Ϳ��� %d�� �������� �������ϴ�.", my_packet->damage );
		SendChatPacket( ci, npc, ConverCtoWC( chatMSG ) );
	}
}

void check_Attack( int ci, char * ptr ) {
	// �÷��̾ NPC�� �����ߴ��� Ȯ���ϱ�
	int x = g_clients[ci].x, y = g_clients[ci].y;
	cs_packet_attack *my_packet = reinterpret_cast<cs_packet_attack *>(ptr);

	if ( my_packet->skill_num == 0 ) g_clients[ci].skill_1 = 1;
	if ( my_packet->skill_num == 1 ) g_clients[ci].skill_2 = 2;
	if ( my_packet->skill_num == 2 ) g_clients[ci].skill_3 = 3;

	for ( int i = NPC_ID_START; i < NPC_ID_START + NUM_NPC; ++i ) {
		if ( x - 1 == g_clients[i].x && y == g_clients[i].y ) {
			// ����
			Attack_Move( i, ci, my_packet );
		}
		if ( x + 1 == g_clients[i].x && y == g_clients[i].y ) {
			// ������
			Attack_Move( i, ci, my_packet );
		}
		if ( x == g_clients[i].x && y - 1 == g_clients[i].y ) {
			// ��
			Attack_Move( i, ci, my_packet );
		}
		if ( x == g_clients[i].x && y + 1 == g_clients[i].y ) {
			// �Ʒ�
			Attack_Move( i, ci, my_packet );
		}
	}
}

