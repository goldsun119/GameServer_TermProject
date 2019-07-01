#pragma once
#include <WinSock2.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <atlimage.h> // CImage
#include "resource.h"
#include "2019_����_protocol.h"
#pragma comment(lib, "ws2_32")
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#define DebugMod FALSE		
#define BUF_SIZE				1024
#define MAX_BUFF_SIZE   4000
#define MAX_PACKET_SIZE  255
													// NPC ����
#define MAX_Chat 10
#define MAX_Chat_View 100
#define MAX_Skill_View 10

#define	WM_SOCKET				WM_USER + 1
#define BOB_ATTR_VISIBLE        16  // bob is visible

extern char ipAddres[MAX_PATH];
extern char game_id[MAX_PATH];
static int isGameData[WORLD_WIDTH][WORLD_HEIGHT];						// ������ �����ϱ�


//--------------------------------------------------------------------------------------------------------------
extern CImage m_image; // Map �̹���
extern CImage m_Status; // ����â
extern CImage Ch_image[5]; // ���� ĳ����
extern CImage You_image[5]; // ���� ĳ����
extern CImage Monster_image[10]; // ���� ĳ����
extern CImage Effect_image[5]; // ���� ����Ʈ

void init_Image(); // �̹��� �Լ� �ε�
void Status_Draw( HDC hdc);
void cimage_draw( HDC hdc, int x, int y, int r_x, int r_y );
void Character_Draw( HDC hdc, int x, int y, int direction, int movement, int hp, int exp, int level );
void Character_You_Draw( HDC hdc, int x, int y, int direction, int movement, int hp );
void Monster_Draw( HDC hdc, int x, int y, int Kind, int hp );
void AttackEffect_Draw( HDC hdc, int x, int y, int Kind );
//--------------------------------------------------------------------------------------------------------------

SOCKET init_sock();

void err_quit( char *msg );
void err_display( char *msg, int err_no );

void clienterror();
void ReadPacket( SOCKET sock );
void ProcessPacket( char *ptr );
void Send_Attack_Packet( int skill_num, int damage );
void Send_Move_Packet(char a );

BOOL CALLBACK DlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );

typedef struct BOB_TYP {
	int x, y;          // position bitmap will be displayed at
	int attr;           // attributes pertaining to the object (general)
	int hp;
	int MaxHp;
	int exp;
	int level;
	int direction = 2;		// ĳ���� ����
	int movement = 0;	// ĳ���� ������
	int skill_num = -1;
	int skillTimer_1 = 0;
	int skillTimer_2 = 0;
	int skillTimer_3 = 0;
	wchar_t message[256];
	DWORD message_time;

} BOB, *BOB_PTR;

extern BOB player;				// �÷��̾� Unit