#include "Client.h"
#include "..\..\Server\Server\2019_화목_protocol.h"
#include "mdump.h"
//using namespace Gdiplus;

// 다이얼로그 관련
HINSTANCE hInstance;
HWND cpy_hwnd;
HDC cpy_hdc, cpy_memdc;
HWND main_window_handle = NULL; // save the window handle
HINSTANCE main_instance = NULL; // save the instance

// SOCKET 선언
SOCKET g_mysocket;
WSABUF	send_wsabuf;
char 	send_buffer[BUF_SIZE];
WSABUF	recv_wsabuf;
char	recv_buffer[BUF_SIZE];
char	packet_buffer[BUF_SIZE];
DWORD		in_packet_size = 0;
int		saved_packet_size = 0;
int		g_myid;
int		g_left_x = 0, g_top_y = 0;
int		skill_timer = 0;

BOB player;				// 플레이어 Unit
BOB npc[NUM_NPC];      // NPC Unit
BOB skelaton[MAX_USER];     // 상대 플레이어 Unit

void init();


HWND hwndOwner;
RECT rc, rcDlg, rcOwner;
HWND ipAddr, CharName;
char ipAddres[MAX_PATH] = "127.0.0.1";
char game_id[MAX_PATH] = "jes";

BOOL CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_INITDIALOG:
		//-------------------------------------------------------------------------------------------------------------------------------------
		// 다이얼로그 박스를 중앙으로
		if ((hwndOwner = GetParent(hDlg)) == NULL)
			hwndOwner = GetDesktopWindow();

		GetWindowRect(hwndOwner, &rcOwner);
		GetWindowRect(hDlg, &rcDlg);
		CopyRect(&rc, &rcOwner);

		OffsetRect(&rcDlg, -rcDlg.left, -rcDlg.top);
		OffsetRect(&rc, -rc.left, -rc.top);
		OffsetRect(&rc, -rcDlg.right, -rcDlg.bottom);

		SetWindowPos(hDlg, HWND_TOP, rcOwner.left + (rc.right / 2), rcOwner.top + (rc.bottom / 2), 0, 0, SWP_NOSIZE);
		//-------------------------------------------------------------------------------------------------------------------------------------

		SetDlgItemTextA(hDlg, IDC_IPADDRESS, "127.0.0.1");
		SetDlgItemTextA(hDlg, IDC_EDIT, game_id);
		return TRUE;

	case WM_COMMAND:

		switch (LOWORD(wParam)) {
		case IDC_CONNECT:
			GetDlgItemTextA(hDlg, IDC_IPADDRESS, ipAddres, MAX_PATH);
			GetDlgItemTextA(hDlg, IDC_EDIT, game_id, MAX_PATH);
			//GetWindowTextA(CharName, game_id, MAX_PATH);
			EndDialog(hDlg, IDCANCEL);
			return TRUE;

		case IDCANCEL:
			exit(-1);
			//MessageBox( NULL, "연결을 하셔야 종료를 할 수 있습니다..!", "", 0 );
			return TRUE;

		}
		return FALSE;
	}
	return FALSE;
}

void err_display(const char *msg, int err_no) {
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, err_no,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	printf("[%s] %s\n", msg, (char *)lpMsgBuf);
	LocalFree(lpMsgBuf);
}



LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);

//-------------------------------------------------------------------------
// 이미지 불러오기
//-------------------------------------------------------------------------
// 채팅 관련 함수
bool chat_enabled[MAX_Chat] = { false }; //chat이 들어왔나?
int  chat_ci[MAX_Chat] = { false }; //어떤 NPC가 들어왔나?
int chat_view[MAX_Chat] = { 0 }; // 얼마나 보여줄까?
//-------------------------------------------------------------------------

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpszCmdLine, int nCmdShow) {
	CMiniDump::Begin();
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);
	HWND hwnd;
	MSG msg;
	WNDCLASS WndClass;
	WndClass.style = CS_HREDRAW | CS_VREDRAW;
	WndClass.lpfnWndProc = WndProc;
	WndClass.cbClsExtra = 0;
	WndClass.cbWndExtra = 0;
	WndClass.hInstance = hInstance;
	WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	//WndClass.hbrBackground = CreateSolidBrush( RGB( 0, 255, 0 ) );
	//WndClass.hbrBackground = CreatePatternBrush( LoadBitmap( hInstance, MAKEINTRESOURCE( IDB_BITMAP7 ) ) );
	WndClass.lpszMenuName = NULL;
	WndClass.lpszClassName = "Window Class Name";

	RegisterClass(&WndClass);
	hwnd = CreateWindow("Window Class Name", "2016180038", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 820, 840, NULL, NULL, hInstance, NULL);
	main_window_handle = hwnd;
	main_instance = hInstance;
	init();
	ShowWindow(hwnd, nCmdShow);
	UpdateWindow(hwnd);
	while (1) {
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return msg.wParam;
	CMiniDump::End();
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM  lParam) {
	static HDC hdc, mem0dc, mem1dc;
	PAINTSTRUCT ps;
	static HBITMAP hbmOld, hbmMem, hbmMemOld;			// 더블버퍼링을 위하여!
	static char isDebugData[500];
	static int isXdata, isYdata;
	static int isTempHighNum = 2;
	static RECT rt;
	static HBITMAP img_back;
	switch (iMsg) {

	case WM_SOCKET:
	{
		if (WSAGETSELECTERROR(lParam)) {
			closesocket((SOCKET)wParam);
			clienterror();
			break;
		}
		switch (WSAGETSELECTEVENT(lParam)) {
		case FD_READ:
			ReadPacket((SOCKET)wParam);
			break;
		case FD_CLOSE:
			closesocket((SOCKET)wParam);
			clienterror();
			break;
		}
	}

	case WM_KEYDOWN:
	{
		if (wParam == '1' || wParam == '2' || wParam == '3' || wParam == '4' || wParam == '5') {
			// 1 = (21, 77)
			// 2 = (129,14)
			// 3 = (247,59)
			// 4 = (81, 184)
			// 5 = (253, 282)
			if (wParam == '1')Send_Move_Packet(21, 77);
			if (wParam == '2')Send_Move_Packet(129, 14);
			if (wParam == '3')Send_Move_Packet(247, 59);
			if (wParam == '4')Send_Move_Packet(81, 184);
			if (wParam == '5')Send_Move_Packet(245, 232);
			break;
		}

		if ((wParam == 'a' || wParam == 'A') && player.skillTimer_1 == 0) {
			player.skill_num = 0;
			Send_Attack_Packet(player.skill_num, 5 + (player.level * 5));
			break;
		}
		if ((wParam == 's' || wParam == 'S') && player.skillTimer_2 == 0) {
			player.skill_num = 1;
			Send_Attack_Packet(player.skill_num, 10 + (player.level * 5));
			break;
		}
		if ((wParam == 'd' || wParam == 'D') && player.skillTimer_3 == 0) {
			player.skill_num = 2;
			Send_Attack_Packet(player.skill_num, 15 + (player.level * 5));
			break;
		}

		int x = 0, y = 0;
		if (wParam == VK_RIGHT) {
			x += 1;
		}
		if (wParam == VK_LEFT) {
			x -= 1;
		}
		if (wParam == VK_UP) {
			y -= 1;
		}
		if (wParam == VK_DOWN) {
			y += 1;
		}
		cs_packet_move *my_packet = reinterpret_cast<cs_packet_move *>(send_buffer);
		my_packet->size = sizeof(my_packet);
		my_packet->type = CS_MOVE;
		send_wsabuf.len = sizeof(my_packet);
		DWORD iobyte;
		if (0 != x) {
			if (1 == x) my_packet->direction = 3;
			else my_packet->direction = 2;
			int ret = WSASend(g_mysocket, &send_wsabuf, 1, &iobyte, 0, NULL, NULL);
			if (ret) {
				int error_code = WSAGetLastError();
#if (DebugMod == TRUE)
				printf("Error while sending packet [%d]", error_code);
#endif
			}
		}
		if (0 != y) {
			if (1 == y) my_packet->direction = 1;
			else my_packet->direction = 0;
			WSASend(g_mysocket, &send_wsabuf, 1, &iobyte, 0, NULL, NULL);
		}

		InvalidateRgn(hwnd, NULL, FALSE);
	}
	break;


	case WM_CREATE:
#if (DebugMod == TRUE)
		AllocConsole();
		freopen("CONOUT$", "wt", stdout);
#endif
		cpy_hwnd = hwnd;

		srand((unsigned int)time(NULL) + GetTickCount());
		GetClientRect(hwnd, &rt);
		init_Image();
		SetTimer(hwnd, 1, 1, NULL);
		SetTimer(hwnd, 2, 1000, NULL);
		break;

	case WM_PAINT:
		srand((unsigned int)time(NULL));

		hdc = BeginPaint(hwnd, &ps);
		mem0dc = CreateCompatibleDC(hdc);//2
		cpy_hdc = hdc;
		cpy_memdc = mem0dc;
		hbmMem = CreateCompatibleBitmap(hdc, rt.right, rt.bottom);//3
		SelectObject(mem0dc, hbmMem);
		PatBlt(mem0dc, rt.left, rt.top, rt.right, rt.bottom, BLACKNESS);
		hbmMemOld = (HBITMAP)SelectObject(mem0dc, hbmMem);//4
		mem1dc = CreateCompatibleDC(mem0dc);//5


		for (int i = 0; i < 21; ++i) {
			for (int j = 0; j < 21; ++j) {
				cimage_draw(mem0dc, i, j, i + g_left_x, j + g_top_y);
			}
		}

		for (int i = 0; i < MAX_USER; ++i) {
			if ((skelaton[i].attr & 16)) {
				Character_You_Draw(mem0dc, skelaton[i].x - g_left_x, skelaton[i].y - g_top_y, skelaton[i].direction, skelaton[i].animation, skelaton[i].hp);
			}
		}

		for (int i = 0; i < NUM_NPC; ++i) {
			if ((npc[i].attr & 16) && npc[i].hp > 0) {
				Monster_Draw(mem0dc, npc[i].x - g_left_x, npc[i].y - g_top_y, npc[i].level, npc[i].hp);
			}
		}

		if (player.skill_num != -1) {
			AttackEffect_Draw(mem0dc, player.x - g_left_x, player.y - g_top_y, player.skill_num);
		}

		Character_Draw(mem0dc, 9, 9, player.direction, player.animation, player.hp, player.exp, player.level);

#if (DebugMod == TRUE)
		sprintf(isDebugData, "Postion : %d, %d [%d]", player.x, player.y, player.movement);
		SetTextColor(mem0dc, RGB(255, 0, 0));
		SetBkMode(mem0dc, OPAQUE);
		SetTextAlign(hdc, TA_TOP);
		TextOut(mem0dc, 0 + (strlen(isDebugData) * 3.2), 740, isDebugData, strlen(isDebugData));
#endif
		for (int i = 0; i < MAX_Chat; ++i) {
			if (chat_enabled[i] == true) {
				SetTextColor(mem0dc, RGB(0, 0, 255));
				SetBkMode(mem0dc, OPAQUE);
				SetTextAlign(hdc, TA_TOP);
				wsprintfA(isDebugData, "[NPC : %d] : %ws", chat_ci[i], npc[chat_ci[i]].message);
				TextOutA(mem0dc, 0 + (strlen(isDebugData) * 3.6), 720 - (i * 20), isDebugData, strlen(isDebugData));
			}
		}

		Status_Draw(mem0dc);

		BitBlt(hdc, 0, 0, rt.right, rt.bottom, mem0dc, 0, 0, SRCCOPY);

		SelectObject(mem0dc, hbmMemOld); //-4
		DeleteObject(hbmMem); //-3
		DeleteObject(hbmMemOld); //-3
		DeleteDC(mem0dc); //-2
		DeleteDC(mem1dc); //-2
		DeleteDC(hdc); //-2
		EndPaint(hwnd, &ps);
		break;

	case WM_TIMER:
		switch (wParam) {
		case 0:
			if (player.skill_num != -1) {
				player.skill_num = -1;
			}
			break;
		case 1:
			for (int i = 0; i < MAX_Chat; ++i) {
				if (chat_enabled[i] == true) {
					chat_view[i]++;
					if (chat_view[i] >= MAX_Chat_View) {
						chat_enabled[i] = false;
						chat_view[i] = 0;
						chat_ci[i] = -1;
					}
				}
			}
			break;

		case 2:
			if (skill_timer != 0) {
				skill_timer -= 1;
			}
		}
		InvalidateRgn(hwnd, NULL, FALSE);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		exit(-1);
		break;

	default:break;
	}
	return DefWindowProc(hwnd, iMsg, wParam, lParam);
}

void init() {
	for (int i = 0; i < WORLD_WIDTH; ++i) { // 가로
		for (int j = 0; j < WORLD_HEIGHT; ++j) { // 세로
			if (i % 2 == j % 2)
				isGameData[i][j] = 1;
		}
	}
	WSADATA	wsadata;
	WSAStartup(MAKEWORD(2, 2), &wsadata);

	g_mysocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, 0);

	SOCKADDR_IN ServerAddr;
	ZeroMemory(&ServerAddr, sizeof(SOCKADDR_IN));
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_port = htons(SERVER_PORT);
	ServerAddr.sin_addr.s_addr = inet_addr(ipAddres);

	int Result = WSAConnect(g_mysocket, (sockaddr *)&ServerAddr, sizeof(ServerAddr), NULL, NULL, NULL, NULL);

	//------------------------------------------------------------------------------------------------------------------
	send_wsabuf.buf = send_buffer;
	send_wsabuf.len = BUF_SIZE;
	recv_wsabuf.buf = recv_buffer;
	recv_wsabuf.len = BUF_SIZE;

	int retval = send(g_mysocket, game_id, strlen(game_id), 0);
	char buf[10];
	retval = recv(g_mysocket, buf, 10, 0);
	buf[retval] = '\0';

	if (strcmp(buf, "False") == 0) {
		MessageBox(cpy_hwnd,"DB에서 ID,PW를 확인할 수 없습니다..!\n프로그램을 종료합니다.", "DB 오류", MB_OK);
		exit(-1);
	}
	else if (strcmp(buf, "Overlap") == 0) {
		MessageBox(cpy_hwnd, "이미 접속유저가 있습니다..!\n프로그램을 종료합니다.", "DB 접속", MB_OK);
		exit(-1);
	}
	else if (strcmp(buf, "Newid") == 0) {
		MessageBox(cpy_hwnd, "아이디가 없습니다.\nID를 만들어서 게임을 시작합니다.", "DB 생성", MB_OK);
	}

	//------------------------------------------------------------------------------------------------------------------
	WSAAsyncSelect(g_mysocket, main_window_handle, WM_SOCKET, FD_CLOSE | FD_READ);



#if (DebugMod == TRUE)
	err_display("Connect : ", Result);
#endif
}

void clienterror() {
	exit(-1);
}

void ReadPacket(SOCKET sock) {
	DWORD iobyte, ioflag = 0;

	int ret = WSARecv(sock, &recv_wsabuf, 1, &iobyte, &ioflag, NULL, NULL);
	if (ret) {
		int err_code = WSAGetLastError();
#if (DebugMod == TRUE)
		printf("Recv Error [%d]\n", err_code);
#endif
	}

	BYTE *ptr = reinterpret_cast<BYTE *>(recv_buffer);

	while (0 != iobyte) {
		if (0 == in_packet_size) in_packet_size = ptr[0];
		if (iobyte + saved_packet_size >= in_packet_size) {
			memcpy(packet_buffer + saved_packet_size, ptr, in_packet_size - saved_packet_size);
			ProcessPacket(packet_buffer);
			ptr += in_packet_size - saved_packet_size;
			iobyte -= in_packet_size - saved_packet_size;
			in_packet_size = 0;
			saved_packet_size = 0;
		}
		else {
			memcpy(packet_buffer + saved_packet_size, ptr, iobyte);
			saved_packet_size += iobyte;
			iobyte = 0;
		}
	}
}

void ProcessPacket(char *ptr)
{
	static bool first_time = true;
	switch (ptr[1]) {
	case SC_STAT_CHANGE: {
		sc_packet_stat_change *my_packet = reinterpret_cast<sc_packet_stat_change *>(ptr);
		int id = my_packet->id;
		if (id == g_myid) {
			player.hp = my_packet->HP;
			player.exp = my_packet->EXP;
			player.level = my_packet->LEVEL;
			player.skillTimer_1 = my_packet->skill1;
			player.skillTimer_2 = my_packet->skill2;
			player.skillTimer_3 = my_packet->skill3;
		}
		else if (id < NPC_ID_START) {
			skelaton[id].hp = my_packet->HP;
			skelaton[id].exp = my_packet->EXP;
			skelaton[id].level = my_packet->LEVEL;
			skelaton[id].skillTimer_1 = my_packet->skill1;
			skelaton[id].skillTimer_2 = my_packet->skill2;
			skelaton[id].skillTimer_3 = my_packet->skill3;
		}
		else {
			npc[id - NPC_ID_START].hp = my_packet->HP;
			npc[id - NPC_ID_START].exp = my_packet->EXP;
			npc[id - NPC_ID_START].level = my_packet->LEVEL;
			npc[id - NPC_ID_START].skillTimer_1 = my_packet->skill1;
			npc[id - NPC_ID_START].skillTimer_2 = my_packet->skill2;
			npc[id - NPC_ID_START].skillTimer_3 = my_packet->skill3;
		}
		InvalidateRgn(cpy_hwnd, NULL, FALSE);
		break;
	}

	case SC_ADD_OBJECT: {
		sc_packet_add_object *my_packet = reinterpret_cast<sc_packet_add_object *>(ptr);
		int id = my_packet->id;
		if (first_time) {
			first_time = false;
			g_myid = id;
		}
		if (id == g_myid) {
			player.x = my_packet->x;
			player.y = my_packet->y;
			g_left_x = my_packet->x - 9;
			g_top_y = my_packet->y - 9;
			player.direction = my_packet->dir;
			player.attr |= 16;
		}
		else if (id < NPC_ID_START) {
			skelaton[id].x = my_packet->x;
			skelaton[id].y = my_packet->y;
			skelaton[id].attr |= 16;
		}
		else {
			npc[id - NPC_ID_START].x = my_packet->x;
			npc[id - NPC_ID_START].y = my_packet->y;
			npc[id - NPC_ID_START].attr |= 16;
#if (DebugMod == TRUE)
			printf("%d\t", id - NPC_START);
#endif
		}
		InvalidateRgn(cpy_hwnd, NULL, FALSE);
		break;
	}
	case SC_POSITION:
	{
		sc_packet_position *my_packet = reinterpret_cast<sc_packet_position *>(ptr);
		int other_id = my_packet->id;
		if (other_id == g_myid) {
			g_left_x = my_packet->x - 9;
			g_top_y = my_packet->y - 9;
			player.x = my_packet->x;
			player.y = my_packet->y;
			player.direction = my_packet->dir;
			player.animation = my_packet->ani;
		}
		else if (other_id < NPC_ID_START) {
			skelaton[other_id].x = my_packet->x;
			skelaton[other_id].y = my_packet->y;
			skelaton[other_id].animation = my_packet->ani;
			skelaton[other_id].direction = my_packet->dir;
		}
		else {
			npc[other_id - NPC_ID_START].x = my_packet->x;
			npc[other_id - NPC_ID_START].y = my_packet->y;
		}
		InvalidateRgn(cpy_hwnd, NULL, FALSE);
		break;
	}

	case SC_REMOVE_OBJECT:
	{
		sc_packet_remove_object *my_packet = reinterpret_cast<sc_packet_remove_object *>(ptr);
		int other_id = my_packet->id;
		if (other_id == g_myid) {
			player.attr &= ~16;
		}
		else if (other_id < NPC_ID_START) {
			skelaton[other_id].attr &= ~16;
		}
		else {
			npc[other_id - NPC_ID_START].attr &= ~16;
		}
		break;
	}
	case SC_CHAT: {
		sc_packet_chat *my_packet = reinterpret_cast<sc_packet_chat *>(ptr);
		int other_id = my_packet->id;
		if (other_id == g_myid) {
			wcsncpy_s(player.message, my_packet->message, MAX_STR_LEN);
			player.message_time = GetTickCount();
		}
		else if (other_id < NPC_ID_START) {
			wcsncpy_s(skelaton[other_id].message, my_packet->message, MAX_STR_LEN);
			skelaton[other_id].message_time = GetTickCount();
		}
		else {
			wcsncpy_s(npc[other_id - NPC_ID_START].message, my_packet->message, MAX_STR_LEN);
			npc[other_id - NPC_ID_START].message_time = GetTickCount();
		}

		for (int i = 0; i < MAX_Chat; ++i) {
			if (chat_enabled[i] == true && chat_ci[i] == (other_id - NPC_ID_START)) {
				break;
			}
			if (chat_enabled[i] == false) {
				chat_ci[i] = other_id - NPC_ID_START;
				chat_enabled[i] = true;
				break;
			}
		}

		break;
	}
	default:
#if (DebugMod == TRUE)
		printf("Unknown PACKET type [%d]\n", ptr[1]);
#endif
		break;
	}
}

void Send_Attack_Packet(int skill_num, int damage) {
	//player.skill_timer = 0; // 스킬 타이머를 구현
	SetTimer(cpy_hwnd, 0, 1, NULL);
	cs_packet_attack *my_packet = reinterpret_cast<cs_packet_attack *>(send_buffer);
	my_packet->size = sizeof(my_packet);
	send_wsabuf.len = sizeof(my_packet);
	DWORD iobyte;
	my_packet->type = CS_ATTACK;
	my_packet->skill_num = skill_num;
	my_packet->damage = damage;

	skill_timer = 1;
	int ret = WSASend(g_mysocket, &send_wsabuf, 1, &iobyte, 0, NULL, NULL);
	if (ret) {
		int error_code = WSAGetLastError();
	}
}

void Send_Move_Packet(int x, int y) {
	cs_packet_teleport *my_packet = reinterpret_cast<cs_packet_teleport *>(send_buffer);
	my_packet->size = sizeof(my_packet);
	send_wsabuf.len = sizeof(my_packet);
	DWORD iobyte;
	my_packet->type = CS_TELEPORT;
	my_packet->x = x;
	my_packet->y = y;

	int ret = WSASend(g_mysocket, &send_wsabuf, 1, &iobyte, 0, NULL, NULL);
	if (ret) {
		int error_code = WSAGetLastError();
	}
}