#pragma once
#include <WinSock2.h>
#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <atlimage.h>
#include "resource.h"

#pragma comment(lib,"ws2_32")

#include "..\..\Server\Server\2019_화목_protocol.h"

#define width 840
#define height 860
#define blk 10
#define WM_SOCKET WM_USER + 1
#define BUF_SIZE				1024
#define MAX_BUFF_SIZE   5000
#define MAX_PACKET_SIZE  255

#define MAX_Chat 10
#define MAX_Chat_View 100
#define MAX_Skill_View 10

extern char ipAddres[MAX_PATH];
extern char game_id[MAX_PATH];
static int isGameData[WORLD_WIDTH][WORLD_HEIGHT];						// 게임판 설정하기

#define CS_UP    1
#define CS_DOWN  2
#define CS_LEFT  3
#define CS_RIGHT    4

extern CImage m_image; // Map 이미지
extern CImage m_Status; // 상태창
extern CImage Ch_image[5]; // 본인 캐릭터
extern CImage You_image[5]; // 상대방 캐릭터
extern CImage Monster_image[10]; // 몬스터 캐릭터
extern CImage Effect_image[5]; // 공격 이펙트

void init_Image(); // 이미지 함수 로드
void Status_Draw(HDC hdc);
void cimage_draw(HDC hdc, int x, int y, int r_x, int r_y);
void Character_Draw(HDC hdc, int x, int y, int direction, int movement, int hp, int exp, int level);
void Character_You_Draw(HDC hdc, int x, int y, int direction, int movement, int hp);
void Monster_Draw(HDC hdc, int x, int y, int Kind, int hp);
void AttackEffect_Draw(HDC hdc, int x, int y, int Kind);

void err_display(const char *msg, int err_no);

void clienterror();
void ReadPacket(SOCKET sock);
void ProcessPacket(char *ptr);
void Send_Attack_Packet(int skill_num, int damage);
void Send_Move_Packet(int x, int y);

BOOL CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

typedef struct BOB_TYP {
	int x, y;
	int animation;
	int hp;
	int max_hp;
	int exp;
	int level;
	int direction;
	int skill_num;
	int skillTimer_1;
	int skillTimer_2;
	int skillTimer_3;
	WCHAR message[MAX_STR_LEN];
	DWORD message_time;
	int attr;           // attributes pertaining to the object (general)
}BOB, *BOB_PTR;

extern BOB player;				// 플레이어 Unit