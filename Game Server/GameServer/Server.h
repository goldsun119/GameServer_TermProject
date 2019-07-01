#ifndef __SERVER_H__
#define __SERVER_H__

#pragma comment(lib, "ws2_32")
#pragma comment(lib, "LUA\\lua53.lib")

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#define MoveHACK TRUE

#include <winsock2.h>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <queue>
#include <unordered_set> // ������ �� ��������. [������ ����������]
#include <random>
#include <chrono>
#include <windows.h>  
#include <sqlext.h>  

#include "2019_����_protocol.h"

extern "C" {
#include "LUA\lua.h"
#include "LUA\lauxlib.h"
#include "LUA\lualib.h"
}

using namespace std::chrono;

//---------------------------------------------------------------------------------------------
// ���� ����
#define BUFSIZE    1024
#define MAX_BUFF_SIZE   4000
#define MAX_PACKET_SIZE  255
//---------------------------------------------------------------------------------------------
// ���� ����
#define DebugMod FALSE
//---------------------------------------------------------------------------------------------

void err_quit( char *msg );
void err_display( char *msg, int err_no );
int getRandomNumber( int min, int max );

void init();
void Worker_Thread();
void Accept_Thread();
void Shutdown_Server();
void SendInfoPacket( int client, int object );
void SendPutPlayerPacket( int client, int object );
void SendPacket( int cl, void *packet );
void DisconnectClient( int ci );
void ProcessPacket( int ci, char *packet );
void SendPositionPacket( int client, int object );
void SendRemovePlayerPacket( int client, int object );
void SendChatPacket( int target_client, int object_id, wchar_t *mess );
bool Distance( int me, int  you, int Radius );
bool IsPlayer( int ci );
void check_Player_HP();
void check_Player_Level( int ci );
void Teleport_Move( int ci, char * ptr );

void Timer_Thread();
void init_NPC(); // npc �ʱ�ȭ �Լ�
bool Is_NPC( int id );
void WakeUpNPC( int id );
void NPC_Random_Move( int id );
void check_Attack( int ci, char * ptr );
void Move_NPCtoClient( int ci, int npc );
void NPC_Responder( int id );
void check_Monster_HP( int id );

int API_get_x( lua_State *L );
int API_get_y( lua_State *L );
int print_LUA( lua_State *L );
int API_get_hp( lua_State *L );
int API_Send_Message( lua_State *L );
char * ConvertWCtoC( wchar_t* str );
wchar_t* ConverCtoWC( char* str );

//------------------------------------------------------------
// DB_init
void init_DB();
extern int db_x, db_y, db_level, db_hp, db_maxhp, db_exp, db_skill[3], db_connect;
int get_DB_Info( int ci );
void set_DB_Info( int ci );
void set_DB_Shutdown( int ci );
void new_DB_Id( int ci );
//------------------------------------------------------------
// �� �浹üũ�� ���Ͽ� ����
extern int map[WORLD_WIDTH][WORLD_HEIGHT];
extern int Tile1[WORLD_WIDTH][WORLD_HEIGHT];
extern int Tile2[WORLD_WIDTH][WORLD_HEIGHT];
void read_map();
bool CollisionCheck( int ci, int dir );
//------------------------------------------------------------


enum OPTYPE { OP_SEND, OP_RECV, OP_DO_AI, E_PLAYER_MOVE_NOTIFY, OP_Attack_Move, OP_Responder };
enum Event_Type { E_MOVE, E_Attack_Move, E_Responder };
enum NPC_Type { N_Peace, N_War };
enum NPC_EXP { No_Send_EXP, Yes_Send_EXP };
enum DB_Info { DB_Success, DB_NoData, DB_NoConnect, DB_Overlap };

struct OverlappedEx {
	WSAOVERLAPPED over;
	WSABUF wsabuf;
	unsigned char IOCP_buf[MAX_BUFF_SIZE];
	OPTYPE event_type;
	int target_id;
};

struct CLIENT {
	int x;
	int y;
	char game_id[10]; // Ŭ�󿡼� �޾ƿ� ���Ӿ��̵� ����
	int hp;
	int Max_hp;
	int hp_timer = 0;
	int level;
	int exp;
	int skill_1 = 0;
	int skill_2 = 0;
	int skill_3 = 0;
	int direction = 2;
	int movement = 0;
	bool connect;

	SOCKET client_socket;
	OverlappedEx recv_over;
	unsigned char packet_buf[MAX_PACKET_SIZE];
	int prev_packet_data; // ���� ó������ �ʴ� ��Ŷ�� �󸶳�
	int curr_packet_size; // ���� ó���ϰ� �ִ� ��Ŷ�� �󸶳�
	std::unordered_set<int> view_list; //�� set���� �ξ�������!
	std::mutex vl_lock;

	std::chrono::high_resolution_clock::time_point last_move_time; // npc�� �����ӽð� ���� + ���ǵ��� ����
	bool is_active; // npc�� ���� �������� �ȿ������� �Ǵ��ϱ� ����
	int npc_Attack = 0; // NPC�� ���� ������
	int npc_Client = -1; // NPC�� � Ŭ���̾�Ʈ�� ������
	int npc_recent_Client = -1; // ���� �ֱٿ� ������ �Ϸ��� Client�� �������� Ȯ���Ѵ�.
	int npc_dir = -1; // NPC�� Ŭ���̾�Ʈ�� ��� ������
	int npc_type = 0;
	int npc_x, npc_y; // �׾������ �ʱ� ��ġ�� �̵��Ѵ�.

	lua_State *L;
};

extern CLIENT g_clients[NPC_ID_START+NUM_NPC];

struct Timer_Event {
	int object_id;
	high_resolution_clock::time_point exec_time; // �� �̺�Ʈ�� ���� ����Ǿ� �ϴ°�
	Event_Type event; // ������ Ȯ���ϸ� ����ܿ� �ٸ��͵��� �߰��� ��
};

class comparison {
	bool reverse;
public:
	comparison() {}
	bool operator() ( const Timer_Event first, const Timer_Event second ) const {
		return first.exec_time > second.exec_time;
	}
};

extern std::mutex tq_lock; // �켱���� ť ��� ���� ��
extern std::priority_queue <Timer_Event, std::vector<Timer_Event>, comparison> timer_queue; // �켱����ť, �׳� ���� �����ϱ� ���� �ɾ������
extern std::chrono::high_resolution_clock::time_point serverTimer;

#endif