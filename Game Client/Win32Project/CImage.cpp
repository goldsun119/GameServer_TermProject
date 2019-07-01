#include "Client.h"
#include "2019_����_protocol.h"

CImage m_image; // Map �̹���
CImage Ch_image[5]; // ���� ĳ����
CImage You_image[5]; // ���� ĳ����
CImage Monster_image[10]; // ���� ĳ����
CImage Effect_image[5]; // ���� ����Ʈ
CImage m_Status; // ����â
char PrintData[100];

void init_Image() {
	m_image.Load( "Image\\Map\\FullMap.png" );
	m_Status.Load( "Image\\Map\\Status.png" );
	Ch_image[0].Load( "Image\\Character\\up.png" );
	Ch_image[1].Load( "Image\\Character\\down.png" );
	Ch_image[2].Load( "Image\\Character\\left.png" );
	Ch_image[3].Load( "Image\\Character\\right.png" );
	You_image[0].Load( "Image\\Character\\You_Up.png" );
	You_image[1].Load( "Image\\Character\\You_Down.png" );
	You_image[2].Load( "Image\\Character\\You_Left.png" );
	You_image[3].Load( "Image\\Character\\You_Right.png" );
	Monster_image[0].Load( "Image\\Monster\\Monster1.png" );
	Monster_image[1].Load( "Image\\Monster\\Monster2.png" );
	Monster_image[2].Load( "Image\\Monster\\Monster3.png" );
	Monster_image[3].Load( "Image\\Monster\\Monster4.png" );
	Monster_image[4].Load( "Image\\Monster\\Monster5.png" );
	Monster_image[5].Load( "Image\\Monster\\Monster6.png" );
	Monster_image[6].Load( "Image\\Monster\\Monster7.png" );
	Effect_image[0].Load( "Image\\Effect\\Attack_Effect1.png" );
	Effect_image[1].Load( "Image\\Effect\\Attack_Effect2.png" );
	Effect_image[2].Load( "Image\\Effect\\Attack_Effect3.png" );
	Effect_image[3].Load( "Image\\Effect\\Attack_Effect4.png" );
	Effect_image[4].Load( "Image\\Effect\\Attack_Monster.png" );
}

void cimage_draw( HDC hdc, int x, int y, int r_x, int r_y ) {
	m_image.Draw( hdc, 0 + (x * 40), 0 + (y * 40), 40, 40, 0 + (r_x * 16), 0 + (r_y * 16), 16, 16 );
}

void Status_Draw( HDC hdc ) {
	m_Status.Draw( hdc, 0, 0, 200, 120, 0, 0, m_Status.GetWidth(), m_Status.GetHeight() );
	SetTextColor( hdc, RGB( 0, 0, 255 ) );
	SetBkMode( hdc, TRANSPARENT );
	SetTextAlign( hdc, TA_LEFT );
	sprintf( PrintData, "LV.%d", player.level );
	TextOut( hdc, 156, 8, PrintData, strlen( PrintData ) );
	sprintf( PrintData, "%s", game_id );
	TextOut( hdc, 13, 8, PrintData, strlen( PrintData ) );
	sprintf( PrintData, "%d / %d", player.hp, player.MaxHp );
	TextOut( hdc, 13, 38, PrintData, strlen( PrintData ) );

	sprintf( PrintData, "%d", player.exp );
	TextOut( hdc, 156, 38, PrintData, strlen( PrintData ) );

	sprintf( PrintData, "%ds", player.skillTimer_1 );
	TextOut( hdc, 18, 94, PrintData, strlen( PrintData ) );

	sprintf( PrintData, "%ds", player.skillTimer_2 );
	TextOut( hdc, 66, 94, PrintData, strlen( PrintData ) );

	sprintf( PrintData, "%ds", player.skillTimer_3 );
	TextOut( hdc, 117, 94, PrintData, strlen( PrintData ) );

}

/*
TBitmap * p = Image1->Picture->Bitmap;
p->Width  = Image1->Width;
p->Height = Image1->Height;

AnsiString sStr = "����� ���ڿ�";
int nTextW  = p->Canvas->TextWidth(sStr);  // ���ڿ� ����
int nTextH  = p->Canvas->TextHeight(sStr); // ���ڿ� ����
int nStartW = (p->Width  - nTextW) / 2;   // �¿� ��� �����ϱ� ���� ������ ã��
int nStartH = (p->Height - nTextH) / 2;   // ���� ��� �����ϱ� ���� ������ ã��

p->Canvas->TextOut(nStartW, nStartH, sStr);
*/
void Character_Draw( HDC hdc, int x, int y, int direction, int movement, int hp, int exp, int level ) {
#if (DebugMod == TRUE)
	sprintf( PrintData, "%d | %d | %d", hp, exp, level );
	SetTextColor( hdc, RGB( 255, 0, 0 ) );
	SetBkMode( hdc, TRANSPARENT );
	SetTextAlign( hdc, TA_CENTER );
	TextOut( hdc, 20 + (x * 40), -14 + (y * 40), PrintData, strlen( PrintData ) );
#endif
	Ch_image[direction].Draw( hdc, 0 + (x * 40), 0 + (y * 40), 40, 40, 0 + (movement * 18), 0, 18, 24 );
}

void Character_You_Draw( HDC hdc, int x, int y, int direction, int movement, int hp ) {
	sprintf( PrintData, "%d", hp );
	SetTextColor( hdc, RGB( 79, 67, 178 ) );
	SetBkMode( hdc, TRANSPARENT );
	SetTextAlign( hdc, TA_CENTER );
	TextOut( hdc, 20 + (x * 40), -14 + (y * 40), PrintData, strlen( PrintData ) );
	You_image[direction].Draw( hdc, 0 + (x * 40), 0 + (y * 40), 40, 40, 0 + (movement * 18), 0, 18, 24 );
}

void Monster_Draw( HDC hdc, int x, int y, int Kind, int hp ) {
	sprintf( PrintData, "%d", hp );
	SetTextColor( hdc, RGB( 0, 0, 255 ) );
	SetBkMode( hdc, TRANSPARENT );
	SetTextAlign( hdc, TA_CENTER );
	TextOut( hdc, 20 + (x * 40), -14 + (y * 40), PrintData, strlen( PrintData ) );
	Monster_image[Kind].Draw( hdc, 0 + (x * 40), 0 + (y * 40), 40, 40, 0, 0, Monster_image[Kind].GetWidth(), Monster_image[Kind].GetHeight() );
}

void AttackEffect_Draw( HDC hdc, int x, int y, int Kind ) {
	Effect_image[Kind].Draw( hdc, 0 + ((x - 1) * 40), 0 + (y * 40), 40, 40, 0, 0, Effect_image[Kind].GetWidth(), Effect_image[Kind].GetHeight() );
	Effect_image[Kind].Draw( hdc, 0 + ((x + 1) * 40), 0 + (y * 40), 40, 40, 0, 0, Effect_image[Kind].GetWidth(), Effect_image[Kind].GetHeight() );
	Effect_image[Kind].Draw( hdc, 0 + (x * 40), 0 + ((y - 1) * 40), 40, 40, 0, 0, Effect_image[Kind].GetWidth(), Effect_image[Kind].GetHeight() );
	Effect_image[Kind].Draw( hdc, 0 + (x * 40), 0 + ((y + 1) * 40), 40, 40, 0, 0, Effect_image[Kind].GetWidth(), Effect_image[Kind].GetHeight() );
}