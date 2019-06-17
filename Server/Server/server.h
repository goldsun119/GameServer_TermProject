#pragma once
#include <iostream>
#include <thread>
#include <map>
#include <unordered_set>
#include <queue>
#include <chrono>
#include <mutex>
#include <random>
#include <winsock2.h>
#include <vector>
#include <windows.h>  

#include <sqlext.h>

extern "C" {
#include "include/lua.h"
#include "include/lauxlib.h"
#include "include/lualib.h"
}

using namespace std;
using namespace chrono;


#include "2019_È­¸ñ_protocol.h"

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "lua53.lib")

#define MAX_BUFFER        1024

#define START_X		10
#define START_Y		10

enum EVENT_TYPE {
	EV_RECV,
	EV_SEND,
	EV_MOVE,
	EV_PLAYER_MOVE,
	EV_MONSTER_ATTACK_MOVE,// e_attack_move
	EV_RESPAWN,
	EV_DO_AI
};

enum NPC_TYPE {
	NPC_PEACE,
	NPC_MOVE_PEACE,
	NPC_WAR
};

enum NPC_EXP {
	NPC_SEND_EXP,
	NPC_NO_SEND_EXP
};

enum DB_Info { 
	DB_Success,
	DB_NoData, 
	DB_NoConnect, 
	DB_Overlap 
};

struct OVER_EX
{
	WSAOVERLAPPED	overlapped;
	WSABUF			dataBuffer;
	char			messageBuffer[MAX_BUFFER];
	EVENT_TYPE		event;
	int				event_from;
	char			direct;
	char			count;
};

struct SOCKETINFO
{
	bool	connected;
	bool	is_sleeping;
	OVER_EX over;
	SOCKET socket;
	char packet_buf[MAX_BUFFER];
	int prev_size;
	high_resolution_clock::time_point last_move_time;

	int x, y;
	char nickname[10];
	int hp;
	int max_hp;
	int hp_timer = 0;
	int level;
	int exp;
	int skill_1;
	int skill_2;
	int skill_3;
	int dir;
	int ani;

	int npc_attack = 0;
	int npc_chase_player = -1;
	int npc_last_attack_player = -1;
	int npc_dir = -1;
	int npc_type = 0;
	int npc_x;
	int npc_y;

	unordered_set <int> viewlist;
	mutex v_lock;

	lua_State *L;
	mutex s_lock;
};

extern SOCKETINFO clients[NPC_ID_START + NUM_NPC];

struct T_EVENT {
	int			do_object;
	high_resolution_clock::time_point start_time;
	EVENT_TYPE	event_type;
	char direct;
	char count;

	constexpr bool operator < (const T_EVENT& _Left) const
	{	// apply operator< to operands
		return (start_time > _Left.start_time);
	}
};

extern priority_queue <T_EVENT> timer_queue;
extern mutex timer_lock;
extern std::chrono::high_resolution_clock::time_point server_timer;;

void initialize();
void worker_thread();
void do_accept();
void shutdown_server();
void disconnect(int id);
void process_packet(int id, char * buf);
void do_timer();
bool is_player(int id);
bool is_npc(int id);
void check_player_hp();
void check_player_level(int ci);
void player_teleport(int ci, char * ptr);

void error_display(const char *msg, int err_no);

void send_packet(int key, char *packet);
void send_chat_packet(int to, int from, wchar_t *message);
void send_stat_change_packet(int to, int obj);
void send_add_object_packet(int to, int obj);
void send_position_packet(int to, int obj);
void send_remove_object_packet(int to, int obj);

void do_recv(int id);

//NPC
void send_add_exp(int id, int npc);
void check_npc_hp(int npc);
void init_npc();
int random_pos(int x);
void random_move_npc(int id);
bool distance(int id, int obj, int r);
void send_monster_attack_chat(int player, int npc);
void check_attack_player(int player, int npc);
void move_npc_to_player(int player, int npc);
void wake_up_npc(int id);
void attack_move(int npc, int ci, cs_packet_attack * my_packet);
void check_attack(int player, char * ptr);
void respawn_npc(int id);

//LUA
void err_display(lua_State *L);
int API_get_x(lua_State *L);
int API_get_y(lua_State *L);
int API_get_hp(lua_State *L);
int API_Send_Message(lua_State *L);


//DB
extern int dbX, dbY, dbLevel, dbHP, dbEXP, dbMaxHP, dbSkill[3], db_connect;
void DB_init();
int DB_get_info(int id);
void DB_set_info(int id);
void DB_new_id(int id);

extern int ground[WORLD_WIDTH][WORLD_HEIGHT];
extern int obstacle[WORLD_WIDTH][WORLD_HEIGHT];
void read_map();
bool collision_check(int id, int dir);
