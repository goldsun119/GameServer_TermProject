#pragma once

constexpr int MAX_USER = 1000;
constexpr int MAX_STR_LEN = 200;

constexpr int NPC_ID_START = 1000;
constexpr int NUM_NPC = 2000;

#define WORLD_WIDTH		300
#define WORLD_HEIGHT	300

#define VIEW_RADIUS		10
#define NPC_RADIUS		10

#define SERVER_PORT		3500

#define CS_LOGIN		1
#define CS_MOVE			2
#define CS_ATTACK		3
#define CS_NPC		4
#define CS_CHAT			4
#define CS_LOGOUT		5
#define CS_TELEPORT 6

#define SC_LOGIN_OK			1
#define SC_LOGIN_FAIL		2
#define SC_POSITION			3
#define SC_CHAT				4
#define SC_STAT_CHANGE		5
#define SC_REMOVE_OBJECT	6
#define SC_ADD_OBJECT		7

#pragma pack(push ,1)

struct sc_packet_login_ok {
	char size;
	char type;
	int id;
	int	x, y;
	int	HP, LEVEL, EXP;
};

struct sc_packet_login_fail {
	char size;
	char type;
};

struct sc_packet_position {
	char size;
	char type;
	int id;
	int x, y;
	int direction;
	int movement;
};

struct sc_packet_chat {
	char size;
	char type;
	int	id;
	wchar_t	message[MAX_STR_LEN];
};

struct sc_packet_stat_change {
	char size;
	char type;
	int	id;
	int	HP, LEVEL, EXP, MaxHp;
	int skill_1;
	int skill_2;
	int skill_3;
};


struct sc_packet_remove_object {
	char size;
	char type;
	int id;
};

struct sc_packet_add_object {
	char size;
	char type;
	int id;
	int obj_class;		// 1: PLAYER,    2:ORC,  3:Dragon, ¡¦..
	int x, y;
	int	HP, LEVEL, EXP,MaxHp;
	int direction;
	int movement;

};


struct cs_packet_login {
	char	size;
	char	type;
	wchar_t player_id[10];
};

struct cs_packet_move {
	char	size;
	char	type;
	char	direction;		// 0:Up, 1:Down, 2:Left, 3:Right
};

struct cs_packet_attack {
	char	size;
	char	type;
	int damage;
	int skill_num;
};

struct cs_packet_chat {
	char	size;
	char	type;
	wchar_t	message[MAX_STR_LEN];
};

struct cs_packet_logout {
	char	size;
	char	type;
};

struct cs_packet_teleport {
	char size;
	char type;
	char key;
};
#pragma pack (pop)