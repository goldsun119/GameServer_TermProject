#include "client.h"
#include "..\..\Server\Server\2019_화목_protocol.h"

CImage m_image; // Map 이미지
CImage Ch_image[5]; // 본인 캐릭터
CImage You_image[5]; // 상대방 캐릭터
CImage Monster_image[10]; // 몬스터 캐릭터
CImage Effect_image[5]; // 공격 이펙트
CImage m_Status; // 상태창
char PrintData[100];

void init_Image() {
	m_image.Load("image\\map\\map.png");
	m_Status.Load("image\\map\\map.png");
	Ch_image[CS_UP].Load("image\\player\\up.png");
	Ch_image[CS_DOWN].Load("image\\player\\down.png");
	Ch_image[CS_LEFT].Load("image\\player\\left.png");
	Ch_image[CS_RIGHT].Load("image\\player\\right.png");
	You_image[CS_UP].Load("image\\player\\up.png");
	You_image[CS_DOWN].Load("image\\player\\down.png");
	You_image[CS_LEFT].Load("image\\player\\left.png");
	You_image[CS_RIGHT].Load("image\\player\\right.png");
	Monster_image[0].Load("image\\monster\\m1.png");
	Monster_image[1].Load("image\\monster\\m2.png");
	Monster_image[2].Load("image\\monster\\m3.png");
	Monster_image[3].Load("image\\monster\\m4.png");
	Monster_image[4].Load("image\\monster\\boss.png");
	Effect_image[0].Load("image\\Effect\\Attack_Effect1.png");
	Effect_image[1].Load("image\\Effect\\Attack_Effect2.png");
	Effect_image[2].Load("image\\Effect\\Attack_Effect3.png");
	Effect_image[4].Load("image\\Effect\\Attack_Monster.png");
}

void cimage_draw(HDC hdc, int x, int y, int r_x, int r_y) {
	m_image.Draw(hdc, 0 + (x * 40), 0 + (y * 40), 40, 40, 0 + (r_x * 16), 0 + (r_y * 16), 16, 16);
}

void Status_Draw(HDC hdc) {
	m_Status.Draw(hdc, 0, 0, 200, 120, 0, 0, m_Status.GetWidth(), m_Status.GetHeight());
	SetTextColor(hdc, RGB(0, 0, 255));
	SetBkMode(hdc, TRANSPARENT);
	SetTextAlign(hdc, TA_LEFT);
	sprintf(PrintData, "LV.%d", player.level);
	TextOutA(hdc, 156, 8, PrintData, strlen(PrintData));
	sprintf(PrintData, "%s", game_id);
	TextOutA(hdc, 13, 8, PrintData, strlen(PrintData));
	sprintf(PrintData, "%d / %d", player.hp, player.max_hp);
	TextOutA(hdc, 13, 38, PrintData, strlen(PrintData));

	sprintf(PrintData, "%d", player.exp);
	TextOutA(hdc, 156, 38, PrintData, strlen(PrintData));

	sprintf(PrintData, "%ds", player.skillTimer_1);
	TextOutA(hdc, 18, 94, PrintData, strlen(PrintData));

	sprintf(PrintData, "%ds", player.skillTimer_2);
	TextOutA(hdc, 66, 94, PrintData, strlen(PrintData));

	sprintf(PrintData, "%ds", player.skillTimer_3);
	TextOutA(hdc, 117, 94, PrintData, strlen(PrintData));
}

/*
TBitmap * p = Image1->Picture->Bitmap;
p->Width  = Image1->Width;
p->Height = Image1->Height;

AnsiString sStr = "출력할 문자열";
int nTextW  = p->Canvas->TextWidth(sStr);  // 문자열 넓이
int nTextH  = p->Canvas->TextHeight(sStr); // 문자열 높이
int nStartW = (p->Width  - nTextW) / 2;   // 좌우 가운데 정렬하기 위한 시작점 찾기
int nStartH = (p->Height - nTextH) / 2;   // 상하 가운데 정렬하기 위한 시작점 찾기

p->Canvas->TextOut(nStartW, nStartH, sStr);
*/
void Character_Draw(HDC hdc, int x, int y, int direction, int movement, int hp, int exp, int level) {
#if (DebugMod == TRUE)
	sprintf(PrintData, "%d | %d | %d", hp, exp, level);
	SetTextColor(hdc, RGB(255, 0, 0));
	SetBkMode(hdc, TRANSPARENT);
	SetTextAlign(hdc, TA_CENTER);
	TextOut(hdc, 20 + (x * 40), -14 + (y * 40), PrintData, strlen(PrintData));
#endif
	Ch_image[direction].Draw(hdc, 0 + (x * 40), 0 + (y * 40), 40, 40, 0 + (movement * 23), 0, 23, 29);
}

void Character_You_Draw(HDC hdc, int x, int y, int direction, int movement, int hp) {
	sprintf(PrintData, "%d", hp);
	SetTextColor(hdc, RGB(79, 67, 178));
	SetBkMode(hdc, TRANSPARENT);
	SetTextAlign(hdc, TA_CENTER);
	TextOutA(hdc, 20 + (x * 40), -14 + (y * 40), PrintData, strlen(PrintData));
	You_image[direction].Draw(hdc, 0 + (x * 40), 0 + (y * 40), 40, 40, 0 + (movement * 23), 0, 23, 26);
}

void Monster_Draw(HDC hdc, int x, int y, int Kind, int hp) {
	sprintf(PrintData, "%d", hp);
	SetTextColor(hdc, RGB(0, 0, 255));
	SetBkMode(hdc, TRANSPARENT);
	SetTextAlign(hdc, TA_CENTER);
	TextOutA(hdc, 20 + (x * 40), -14 + (y * 40), PrintData, strlen(PrintData));
	Monster_image[Kind].Draw(hdc, 0 + (x * 40), 0 + (y * 40), 40, 40, 0, 0, Monster_image[Kind].GetWidth(), Monster_image[Kind].GetHeight());
}

void AttackEffect_Draw(HDC hdc, int x, int y, int Kind) {
	Effect_image[Kind].Draw(hdc, 0 + ((x - 1) * 40), 0 + (y * 40), 40, 40, 0, 0, Effect_image[Kind].GetWidth(), Effect_image[Kind].GetHeight());
	Effect_image[Kind].Draw(hdc, 0 + ((x + 1) * 40), 0 + (y * 40), 40, 40, 0, 0, Effect_image[Kind].GetWidth(), Effect_image[Kind].GetHeight());
	Effect_image[Kind].Draw(hdc, 0 + (x * 40), 0 + ((y - 1) * 40), 40, 40, 0, 0, Effect_image[Kind].GetWidth(), Effect_image[Kind].GetHeight());
	Effect_image[Kind].Draw(hdc, 0 + (x * 40), 0 + ((y + 1) * 40), 40, 40, 0, 0, Effect_image[Kind].GetWidth(), Effect_image[Kind].GetHeight());
}