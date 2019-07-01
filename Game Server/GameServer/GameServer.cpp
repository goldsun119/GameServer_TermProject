#include "Server.h"
#include "2019_����_protocol.h"

int getRandomNumber( int min, int max ) {
	//< 1�ܰ�. �õ� ����
	std::random_device rn;
	std::mt19937_64 rnd( rn() );
	//< 2�ܰ�. ���� ���� ( ���� )
	std::uniform_int_distribution<int> range( min, max );
	//< 3�ܰ�. �� ����
	return range( rnd );
}

HANDLE g_hiocp;
SOCKET g_ssocket;
CLIENT g_clients[NPC_ID_START + NUM_NPC];
std::chrono::high_resolution_clock::time_point serverTimer;

int main() {
	std::vector <std::thread * > worker_threads;
	init();
	check_Player_Level( 0 );
	serverTimer = high_resolution_clock::now();

	std::thread TimerThread{ Timer_Thread };

	for ( int i = 0; i < 6; ++i ) {
		worker_threads.emplace_back( new std::thread{ Worker_Thread } );
	}

	std::thread accept_tread{ Accept_Thread };
	TimerThread.join();
	accept_tread.join();
	for ( auto pth : worker_threads ) {
		pth->join();
		delete pth;
	}

	worker_threads.clear();
	Shutdown_Server();
}

void init() {
	std::wcout.imbue( std::locale( "korean" ) );
	read_map();
	init_NPC();
	init_DB();

	WSADATA	wsadata;
	WSAStartup( MAKEWORD( 2, 2 ), &wsadata );

	g_hiocp = CreateIoCompletionPort( INVALID_HANDLE_VALUE, 0, NULL, 0 );

	g_ssocket = WSASocket( AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED );

	SOCKADDR_IN ServerAddr;
	ZeroMemory( &ServerAddr, sizeof( SOCKADDR_IN ) );
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_port = htons( SERVER_PORT );
	ServerAddr.sin_addr.s_addr = INADDR_ANY;


	bind( g_ssocket, reinterpret_cast<sockaddr *>(&ServerAddr), sizeof( ServerAddr ) );
	listen( g_ssocket, 5 );

	for ( int i = 0; i < MAX_USER; ++i )
		g_clients[i].connect = false;
}

void Worker_Thread() {
	while ( true ) {
		DWORD io_size;
		unsigned long long ci;
		OverlappedEx *over;
		BOOL ret = GetQueuedCompletionStatus( g_hiocp, &io_size, &ci, reinterpret_cast<LPWSAOVERLAPPED *>(&over), INFINITE );

		if ( FALSE == ret ) {
			int err_no = WSAGetLastError();
			if ( err_no == 64 )
				DisconnectClient( ci );
			else
				err_display( "QOCS : ", WSAGetLastError() );

		}
		if ( 0 == io_size ) {
			DisconnectClient( ci );
			continue;
		}

		if ( OP_RECV == over->event_type ) {
#if (DebugMod == TRUE )
			//printf( "Send Incomplete Error!\n" );
			printf( "RECV from Client : %d\n", ci );
			printf( "IO_SIZE : %d\n", io_size );
#endif
			unsigned char *buf = g_clients[ci].recv_over.IOCP_buf;
			unsigned psize = g_clients[ci].curr_packet_size;
			unsigned pr_size = g_clients[ci].prev_packet_data;
			while ( io_size != 0 ) {
				if ( 0 == psize ) psize = buf[0];
				// 0�� ���[�ٷ��� ��Ŷ�� ó���� ������ ����Ŷ���� �����ص� �ȴ�. / ó�� �޴´�] ������ �����ش�.
				if ( io_size + pr_size >= psize ) {
					// ���� ��Ŷ �ϼ��� �����ϴ�.
					char packet[MAX_PACKET_SIZE];
					memcpy( packet, g_clients[ci].packet_buf, pr_size );
					memcpy( packet + pr_size, buf, psize - pr_size );
					ProcessPacket( static_cast<int>(ci), packet );
					io_size -= psize - pr_size;
					buf += psize - pr_size;
					psize = 0; pr_size = 0;
				}
				else {
					memcpy( g_clients[ci].packet_buf + pr_size, buf, io_size );
					pr_size += io_size;
					io_size = 0;
				}
			}
			g_clients[ci].curr_packet_size = psize;
			g_clients[ci].prev_packet_data = pr_size;
			DWORD recv_flag = 0;
			WSARecv( g_clients[ci].client_socket, &g_clients[ci].recv_over.wsabuf, 1, NULL, &recv_flag, &g_clients[ci].recv_over.over, NULL );

		}
		else if ( OP_SEND == over->event_type ) {
			if ( io_size != over->wsabuf.len ) { // ���ƾ� �Ǵµ� �ٸ��Ƿ� 
#if (DebugMod == TRUE )
				printf( "Send Incomplete Error!\n" );
#endif
				closesocket( g_clients[ci].client_socket );
				g_clients[ci].connect = false;
				exit( -1 );
			}
			delete over;
		}
		else if ( OP_DO_AI == over->event_type ) {
			check_Monster_HP( ci );
			if ( g_clients[ci].level != 0 && g_clients[ci].npc_Client == -1 ) {
				NPC_Random_Move( ci );
			}
			delete over;
		}
		else if ( E_PLAYER_MOVE_NOTIFY == over->event_type ) {
			g_clients[ci].vl_lock.lock();
			// ���⼭�� ci �� npc �̰�, target_id �� client ���̵� �̴�.
			//std::cout << "Hi : " << ci << std::endl;
			lua_State *L = g_clients[ci].L;
			lua_getglobal( L, "player_move_notify" );
			lua_pushnumber( L, over->target_id );
			if ( 0 != lua_pcall( L, 1, 1, 0 ) ) {
				std::cout << "LUA error calling [player_move_notify] : " << (char *)lua_tostring( L, -1 ) << std::endl;
			}
			lua_pop( L, 1 );
			g_clients[ci].vl_lock.unlock();
			delete over;
		}
		else if ( OP_Attack_Move == over->event_type ) {
			//printf( "NPC[%d] Move Client[%d]\t", ci, g_clients[ci].npc_Client );
			Move_NPCtoClient( g_clients[ci].npc_Client, ci );
			delete over;
		}
		else if ( OP_Responder == over->event_type ) {
			NPC_Responder( ci );
			delete over;
		}
		else {
#if (DebugMod == TRUE )
			printf( "Unknown GQCS event!\n" );
#endif
			exit( -1 );
			////while ( true );
		}

	}
}

void Accept_Thread() {
	SOCKADDR_IN ClientAddr;
	ZeroMemory( &ClientAddr, sizeof( SOCKADDR_IN ) );
	ClientAddr.sin_family = AF_INET;
	ClientAddr.sin_port = htons( SERVER_PORT );
	ClientAddr.sin_addr.s_addr = INADDR_ANY;
	int addr_size = sizeof( ClientAddr );
	while ( true ) {
		SOCKET new_client = WSAAccept( g_ssocket, reinterpret_cast<sockaddr *>(&ClientAddr), &addr_size, NULL, NULL );

		if ( INVALID_SOCKET == new_client ) {
			int err_no = WSAGetLastError();
			err_display( "WSAAccept : ", err_no );
		}

		int new_id = -1;

		for ( int i = 0; i < MAX_USER; ++i ) {
			if ( g_clients[i].connect == false ) {
				new_id = i;
#if (DebugMod == TRUE )
				printf( "New Client : %d\n", new_id );
#endif
				break;
			}
		}

		if ( -1 == new_id ) {
#if (DebugMod == TRUE )
			printf( "������ �ο� �̻����� �����Ͽ� �����Ͽ����ϴ�..!\n" );
#endif
			closesocket( new_client );
			continue;
		}

		char  buf[255];
		int retval = recv( new_client, buf, 10, 0 );
		if ( retval == SOCKET_ERROR ) {
			std::cout << "Not Recv Game_ID : " << new_id << std::endl;
			closesocket( new_client );
			continue;
		}
		buf[retval] = '\0';

		strcpy( g_clients[new_id].game_id, buf );
#if (DebugMod == TRUE )
		printf( "Game ID : %s\n", game_id );
#endif


		int is_accept = get_DB_Info( new_id );
		if ( is_accept == DB_Success  && db_connect == 0 ) {
			strcpy( buf, "Okay" );
			retval = send( new_client, buf, strlen( buf ), 0 );
		}
		else if ( is_accept == DB_NoConnect ) {
			strcpy( buf, "False" );
			retval = send( new_client, buf, strlen( buf ), 0 );
		}
		else if ( db_connect == 1 ) {
			strcpy( buf, "Overlap" );
			retval = send( new_client, buf, strlen( buf ), 0 );
			closesocket( new_client );
			continue;
		}
		else if ( is_accept == DB_NoData ) {
			strcpy( buf, "Newid" );
			g_clients[new_id].x = 9;
			g_clients[new_id].y = 9;
			g_clients[new_id].exp = 0;
			g_clients[new_id].level = 1;
			g_clients[new_id].hp = 100;
			g_clients[new_id].Max_hp = g_clients[new_id].hp;
			new_DB_Id( new_id );
			get_DB_Info( new_id );
			retval = send( new_client, buf, strlen( buf ), 0 );
		}
		//---------------------------------------------------------------------------------------------------------------------------------------------------
		// ���� ���� ���̵� init ó��
		std::cout << "id : " << new_id << std::endl;
		g_clients[new_id].connect = true;
		g_clients[new_id].client_socket = new_client;
		g_clients[new_id].curr_packet_size = 0;
		g_clients[new_id].prev_packet_data = 0;
		ZeroMemory( &g_clients[new_id].recv_over, sizeof( g_clients[new_id].recv_over ) );
		g_clients[new_id].recv_over.event_type = OP_RECV;
		g_clients[new_id].recv_over.wsabuf.buf = reinterpret_cast<CHAR *>(g_clients[new_id].recv_over.IOCP_buf);
		g_clients[new_id].recv_over.wsabuf.len = sizeof( g_clients[new_id].recv_over.IOCP_buf );
		g_clients[new_id].x = db_x;
		g_clients[new_id].y = db_y;
		g_clients[new_id].level = db_level;
		g_clients[new_id].skill_1 = db_skill[0];
		g_clients[new_id].skill_2 = db_skill[1];
		g_clients[new_id].skill_3 = db_skill[2];
		g_clients[new_id].hp_timer = 0;
		g_clients[new_id].hp = db_hp;
		g_clients[new_id].Max_hp = db_maxhp;
		g_clients[new_id].direction = 2;
		g_clients[new_id].movement = 0;
		//---------------------------------------------------------------------------------------------------------------------------------------------------
		DWORD recv_flag = 0;
		CreateIoCompletionPort( reinterpret_cast<HANDLE>(new_client), g_hiocp, new_id, 0 );
		int ret = WSARecv( new_client, &g_clients[new_id].recv_over.wsabuf, 1, NULL, &recv_flag, &g_clients[new_id].recv_over.over, NULL );

		if ( 0 != ret ) {
			int error_no = WSAGetLastError();
			if ( WSA_IO_PENDING != error_no ) {
				err_display( "RecvPacket:WSARecv", error_no );
				//while ( true );
			}
		}

		SendPutPlayerPacket( new_id, new_id );

		std::unordered_set<int> local_view_list;
		for ( int i = 0; i < NPC_ID_START + NUM_NPC; ++i )
			if ( g_clients[i].connect == true )
				if ( i != new_id )
					if ( Distance( i, new_id, VIEW_RADIUS ) == true ) {
						local_view_list.insert( i );
						SendPutPlayerPacket( new_id, i );

						if ( Is_NPC( i ) )
							WakeUpNPC( i );

						if ( g_clients[i].connect == false )
							continue;

						g_clients[i].vl_lock.lock();
						if ( g_clients[i].view_list.count( new_id ) == 0 ) {
							g_clients[i].view_list.insert( new_id );
							g_clients[i].vl_lock.unlock();
							SendPutPlayerPacket( i, new_id );
						}
						else g_clients[i].vl_lock.unlock();
					}
		g_clients[new_id].vl_lock.lock();
		for ( auto p : local_view_list ) g_clients[new_id].view_list.insert( p );
		g_clients[new_id].vl_lock.unlock();
	}

}

void Shutdown_Server() {
	for ( int i = NPC_ID_START; i < NPC_ID_START + NUM_NPC; ++i )
		lua_close( g_clients[i].L );
	closesocket( g_ssocket );
	CloseHandle( g_hiocp );
	WSACleanup();
}

void DisconnectClient( int ci ) {
	closesocket( g_clients[ci].client_socket );
	set_DB_Shutdown( ci );
	g_clients[ci].connect = false;

	std::unordered_set<int> lvl;
	g_clients[ci].vl_lock.lock();//LOCK
	lvl = g_clients[ci].view_list;
	g_clients[ci].vl_lock.unlock();//UNLOCK

	for ( auto target : lvl ) {
		g_clients[target].vl_lock.lock();//LOCK
		if ( 0 != g_clients[target].view_list.count( ci ) ) {
			g_clients[target].view_list.erase( ci );
			g_clients[target].vl_lock.unlock();//UNLOCK
			SendRemovePlayerPacket( target, ci );
		}
		else {
			g_clients[target].vl_lock.unlock();//UNLOCK
		}
	}
	g_clients[ci].vl_lock.lock();//LOCK
	g_clients[ci].view_list.clear();
	g_clients[ci].vl_lock.unlock();//UNLOCK
	std::cout << "Disconnect Client : " << ci << std::endl;
}

void ProcessPacket( int ci, char *packet ) {
	switch (packet[1])
	{
	case CS_MOVE:
	{
		cs_packet_move *my_packet = reinterpret_cast<cs_packet_move *>(packet);

		switch (my_packet->direction)
	{
	case 0:
		g_clients[ci].direction = 0;
		if (g_clients[ci].y > 0 && CollisionCheck(ci, 0)) {
			g_clients[ci].y--;
			g_clients[ci].movement += 1;
			g_clients[ci].last_move_time = std::chrono::high_resolution_clock::now();
		}
		break;
	case 1:
		g_clients[ci].direction = 1;
		if (g_clients[ci].y < WORLD_HEIGHT - 1 && CollisionCheck(ci, 1)) {
			g_clients[ci].y++;
			g_clients[ci].movement += 1;
			g_clients[ci].last_move_time = std::chrono::high_resolution_clock::now();
		}
		break;
	case 2:
		g_clients[ci].direction = 2;
		if (g_clients[ci].x > 0 && CollisionCheck(ci, 2)) {
			g_clients[ci].x--;
			g_clients[ci].movement += 1;
			g_clients[ci].last_move_time = std::chrono::high_resolution_clock::now();
		}
		break;
	case 3:
		g_clients[ci].direction = 3;
		if (g_clients[ci].x < WORLD_WIDTH - 1 && CollisionCheck(ci, 3)) {
			g_clients[ci].x++;
			g_clients[ci].movement += 1;
			g_clients[ci].last_move_time = std::chrono::high_resolution_clock::now();
		}
		break;
	default:
		break;
	}
	}
		break;

	case CS_ATTACK:
	{
		cs_packet_attack *my_packet = reinterpret_cast<cs_packet_attack *>(packet);
		//printf( "%d�� ���� %d��ŭ\n",ci, my_packet->damage );
		my_packet->size = packet[0];
		my_packet->type = packet[1];
		my_packet->damage = packet[2];
		my_packet->skill_num = packet[3];
		check_Attack(ci, packet); 
	}
		break;
	case CS_TELEPORT:
	{
		cs_packet_teleport *my_packet = reinterpret_cast<cs_packet_teleport *>(packet);
		//printf( "%d�� ���� %d��ŭ\n",ci, my_packet->damage );
		Teleport_Move(ci, packet);
	}
		break;
	default:
		break;
	}

	if ( g_clients[ci].movement >= 2 ) g_clients[ci].movement = 0; // �������� 0~2���� �ִ�.

	SendPositionPacket( ci, ci ); // �ڱ� �ڽ����� �Ⱥ����༭

	std::unordered_set<int> new_view_list;
	for ( int i = 0; i < MAX_USER; ++i )
		if ( g_clients[i].connect == true )
			if ( i != ci ) {
				// �ڱ� �ڽ��� ������ �ȵȴ�.
				if ( true == Distance( ci, i, VIEW_RADIUS ) )
					new_view_list.insert( i );
			}

	//------------------------------------------------------------------------------
	for ( int i = NPC_ID_START; i < NPC_ID_START + NUM_NPC; ++i )
		if ( Distance( ci, i, VIEW_RADIUS ) == true ) new_view_list.insert( i );
	//------------------------------------------------------------------------------

	// ���� �߰��Ǵ� ��ü
	std::unordered_set<int> vlc;
	g_clients[ci].vl_lock.lock();
	vlc = g_clients[ci].view_list;
	g_clients[ci].vl_lock.unlock();

	for ( auto target : new_view_list )
		if ( vlc.count( target ) == 0 ) {
			// ���� �߰��Ǵ� ��ü
			SendPutPlayerPacket( ci, target );
			vlc.insert( target );

			if ( Is_NPC( target ) == true )
				WakeUpNPC( target );

			if ( true != IsPlayer( target ) ) continue;

			g_clients[target].vl_lock.lock();
			// ũ��ƼĮ ������ �ʹ� ��� �׷��� �ٿ��� �Ѵ�.
			if ( g_clients[target].view_list.count( ci ) == 0 ) {
				g_clients[target].view_list.insert( ci );
				g_clients[target].vl_lock.unlock();
				SendPutPlayerPacket( target, ci );
			}
			else {
				g_clients[target].vl_lock.unlock();
				SendPositionPacket( target, ci );
			}

		}
		else {
			// �������� �����ϴ� ��ü
			if ( Is_NPC( target ) == true ) continue;
			// npc�� ó���� �ʿ䰡 ����.

			g_clients[target].vl_lock.lock();
			if ( 0 != g_clients[target].view_list.count( ci ) ) {
				g_clients[target].vl_lock.unlock();
				SendPositionPacket( target, ci );
			}
			else {
				g_clients[target].view_list.insert( ci );
				g_clients[target].vl_lock.unlock();
				SendPutPlayerPacket( target, ci );
			}
		}

		// �þ߿��� �־��� ��ü
		std::unordered_set<int> removed_id_list;
		for ( auto target : vlc ) // ������ �־��µ�
			if ( new_view_list.count( target ) == 0 ) { //���ο� ����Ʈ���� ����.
				removed_id_list.insert( target );
				SendRemovePlayerPacket( ci, target );

				if ( Is_NPC( target ) == true ) continue;
				// npc�� �ʿ� ����.

				g_clients[target].vl_lock.lock();
				if ( 0 != g_clients[target].view_list.count( ci ) ) {
					g_clients[target].view_list.erase( ci );
					g_clients[target].vl_lock.unlock();
					SendRemovePlayerPacket( target, ci );
				}
				else {
					g_clients[target].vl_lock.unlock();
				}
			}

		g_clients[ci].vl_lock.lock();
		for ( auto p : vlc )
			g_clients[ci].view_list.insert( p );
		for ( auto d : removed_id_list )
			g_clients[ci].view_list.erase( d );
		g_clients[ci].vl_lock.unlock();

		for ( auto npc : new_view_list ) {
			if ( false == Is_NPC( npc ) ) continue;
			OverlappedEx *over = new OverlappedEx;
			over->event_type = E_PLAYER_MOVE_NOTIFY;
			over->target_id = ci;
			PostQueuedCompletionStatus( g_hiocp, 1, npc, &over->over );
		}
	}

bool Distance( int me, int  you, int Radius ) {
	return (g_clients[me].x - g_clients[you].x) * (g_clients[me].x - g_clients[you].x) +
		(g_clients[me].y - g_clients[you].y) * (g_clients[me].y - g_clients[you].y) <= Radius * Radius;
}

bool Is_NPC( int id ) {
	return (id >= NPC_ID_START) && (id < NUM_NPC);
}

void Timer_Thread() {
	for ( ;;) {
		Sleep( 10 );
		for ( ;;) {
			if ( high_resolution_clock::now() - serverTimer >= 1s ) {
				check_Player_HP();
				serverTimer = high_resolution_clock::now();
			}
			tq_lock.lock();
			if ( 0 == timer_queue.size() ) {
				tq_lock.unlock();
				break;
			} // ť�� ��������� ������ �ȵǴϱ�
			Timer_Event t = timer_queue.top(); // ���� �̺�Ʈ �߿� ����ð��� ���� �ֱ��� �̺�Ʈ�� �����ؾ� �ϹǷ� �켱���� ť�� ����
			if ( t.exec_time > high_resolution_clock::now() ) {
				tq_lock.unlock();
				break; // ����ð����� ũ�ٸ�, ��ٷ���
			}
			timer_queue.pop();
			tq_lock.unlock();
			OverlappedEx *over = new OverlappedEx;
			if ( E_MOVE == t.event ) {
				over->event_type = OP_DO_AI;
			}
			if ( E_Attack_Move == t.event ) {
				over->event_type = OP_Attack_Move;
			}
			if ( E_Responder == t.event ) {
				over->event_type = OP_Responder;
			}
			PostQueuedCompletionStatus( g_hiocp, 1, t.object_id, &over->over );
		}
	}
}

bool IsPlayer( int ci ) {
	if ( ci < MAX_USER ) return true;
	else return false;
}

void check_Player_HP() {
	for ( int ci = 0; ci < MAX_USER; ++ci ) {
		if ( g_clients[ci].connect == false ) continue;
		if ( g_clients[ci].hp <= 0 ) {
			// HP�� 0�� �Ǿ������ �ʱ� ��ġ�� �̵� & ��ġ �����ֱ�
			g_clients[ci].x = 9;
			g_clients[ci].y = 9;
			g_clients[ci].hp = g_clients[ci].Max_hp;
			g_clients[ci].exp /= 2;
			SendPositionPacket( ci, ci );
		}
		if ( g_clients[ci].skill_1 != 0 )  g_clients[ci].skill_1 -= 1;
		if ( g_clients[ci].skill_2 != 0 )  g_clients[ci].skill_2 -= 1;
		if ( g_clients[ci].skill_3 != 0 )  g_clients[ci].skill_3 -= 1;

		set_DB_Info( ci );

		//5�ʸ��� 10% hp ȸ��
		if ( g_clients[ci].hp_timer >= 5 ) {
			g_clients[ci].hp_timer = 0;
			if ( g_clients[ci].Max_hp <= g_clients[ci].hp + (g_clients[ci].Max_hp % 10) ) {
				g_clients[ci].hp = g_clients[ci].Max_hp;
			}
			else {
				g_clients[ci].hp += g_clients[ci].Max_hp % 10;
			}
		}
		g_clients[ci].hp_timer += 1;


		check_Player_Level( ci );
		SendInfoPacket( ci, ci );
	}
}

void check_Player_Level( int ci ) {
	// 1���� 100
	// 2���� 200
	// 3���� 400
	unsigned long long exp_data = 100;

	//for ( int i = 1; i < 50; ++i ) {
	//	printf( "%d���� : %lld\n", i , exp_data );
	//	exp_data += exp_data;
	//}

	for ( int i = 1; i < g_clients[ci].level; ++i ) {
		exp_data += exp_data;
	}

	if ( g_clients[ci].exp >= exp_data ) {
		g_clients[ci].exp = 0;
		g_clients[ci].Max_hp += (g_clients[ci].level * 20);
		g_clients[ci].hp = g_clients[ci].Max_hp;
		g_clients[ci].level += 1;
	}

}

void Teleport_Move( int ci, char * ptr ) {
	cs_packet_teleport *my_packet = reinterpret_cast<cs_packet_teleport *>(ptr);

	if (my_packet->key == 1)
	{
		g_clients[ci].x = 21;
		g_clients[ci].y = 77;
	}
	if (my_packet->key == 2)
	{

		g_clients[ci].x = 129;
		g_clients[ci].y = 14;
	}
	if (my_packet->key == 3)
	{
		g_clients[ci].x = 247;
		g_clients[ci].y = 59;

	}
	if (my_packet->key == 4)
	{

		g_clients[ci].x = 81;
		g_clients[ci].y = 184;
	}
	if (my_packet->key == 5)
	{

		g_clients[ci].x = 245;
		g_clients[ci].y = 232;
	}


	SendPositionPacket( ci, ci );
}