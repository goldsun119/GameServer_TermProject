#include "Server.h"

extern "C" {
#include "LUA\lua.h"
#include "LUA\lauxlib.h"
#include "LUA\lualib.h"
}

int print_LUA( lua_State *L ) {
	int my_id = (int)lua_tonumber( L, -3 );
	int level = (int)lua_tonumber( L, -2 );
	int hp = (int)lua_tonumber( L, -1 );
	lua_pop( L, 4 );

	std::cout << "id : " << my_id << " HP : " << hp << " Level : " << level<< std::endl;
	return 0;
}

int API_get_x( lua_State *L ) {
	int oid = (int)lua_tonumber( L, -1 );
	lua_pop( L, 2 );
	lua_pushnumber( L, g_clients[oid].x );
	return 1;
}

int API_get_y( lua_State *L ) {
	int oid = (int)lua_tonumber( L, -1 );
	lua_pop( L, 2 );
	lua_pushnumber( L, g_clients[oid].y );
	return 1;
}

int API_get_hp( lua_State *L ) {
	int oid = (int)lua_tonumber( L, -1 );
	lua_pop( L, 2 );
	lua_pushnumber( L, g_clients[oid].hp );
	return 1;
}

int API_Send_Message( lua_State *L ) {
	int target_client = (int)lua_tonumber( L, -3 );
	int my_id = (int)lua_tonumber( L, -2 );
	char *mess = (char *)lua_tostring( L, -1 );
	lua_pop( L, 4 );

	size_t wlen, len = strlen( mess ) + 1;
	wchar_t wmess[MAX_STR_LEN];
	len = (MAX_STR_LEN - 1 < len) ? MAX_STR_LEN - 1 : len;
	mbstowcs_s( &wlen, wmess, len, mess, _TRUNCATE );
	wmess[MAX_STR_LEN - 1] = (wchar_t)0;
	SendChatPacket( target_client, my_id, wmess );
	return 0;
}

char * ConvertWCtoC( wchar_t* str ) {
	//��ȯ�� char* ���� ����
	char* pStr;
	//�Է¹��� wchar_t ������ ���̸� ����
	int strSize = WideCharToMultiByte( CP_ACP, 0, str, -1, NULL, 0, NULL, NULL );
	//char* �޸� �Ҵ�
	pStr = new char[strSize];
	//�� ��ȯ 
	WideCharToMultiByte( CP_ACP, 0, str, -1, pStr, strSize, 0, 0 );
	return pStr;
}

wchar_t* ConverCtoWC( char* str ) {
	//wchar_t�� ���� ����
	wchar_t* pStr;
	//��Ƽ ����Ʈ ũ�� ��� ���� ��ȯ
	int strSize = MultiByteToWideChar( CP_ACP, 0, str, -1, NULL, NULL );
	//wchar_t �޸� �Ҵ�
	pStr = new WCHAR[strSize];
	//�� ��ȯ
	MultiByteToWideChar( CP_ACP, 0, str, strlen( str ) + 1, pStr, strSize );
	return pStr;
}