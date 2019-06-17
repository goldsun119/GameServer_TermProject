#include "server.h"

extern "C" {
#include "include/lua.h"
#include "include/lauxlib.h"
#include "include/lualib.h"
}

int API_get_x(lua_State *L)
{
	int id = (int)lua_tonumber(L, -1);
	int x = clients[id].x;
	lua_pop(L, 2);
	lua_pushnumber(L, x);
	return 1;
}

int API_get_y(lua_State *L)
{
	int id = (int)lua_tonumber(L, -1);
	int y = clients[id].y;
	lua_pop(L, 2);
	lua_pushnumber(L, y);
	return 1;
}

int API_get_hp(lua_State *L)
{
	int id = (int)lua_tonumber(L, -1);
	int hp = clients[id].hp;
	lua_pop(L, 2);
	lua_pushnumber(L, hp);
	return 1;
}

int API_Send_Message(lua_State *L)
{
	int to = (int)lua_tonumber(L, -3);
	int from = (int)lua_tonumber(L, -2);
	char *message = (char *)lua_tostring(L, -1);
	wchar_t wmess[MAX_STR_LEN];
	size_t wlen;

	mbstowcs_s(&wlen, wmess, strlen(message), message, _TRUNCATE);
	lua_pop(L, 4);
	send_chat_packet(to, from, wmess);
	return 0;
}

void err_display(lua_State *L)
{
	cout << lua_tostring(L, -1);
	lua_pop(L, 1);
}