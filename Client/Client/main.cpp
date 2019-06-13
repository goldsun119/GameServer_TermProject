// 1, 2번은 목요일까지, 3, 4번은 다음주 월요일까지
#include <WinSock2.h>
#include <windows.h> 
#include<stdio.h>
#include<time.h>
#include <atlimage.h> // CImage
#include "resource.h"
#include "..\..\Server\Server\2019_화목_protocol.h"

#pragma comment(lib, "ws2_32")

#define width 504
#define height 520
#define blk 10
#define WM_SOCKET WM_USER + 1
#define BUF_SIZE				1024
#define MAX_BUFF_SIZE   5000
#define MAX_PACKET_SIZE  255

SOCKET g_mysocket;
WSABUF	send_wsabuf;
WSABUF	recv_wsabuf;
char 		send_buffer[BUF_SIZE];
char		recv_buffer[BUF_SIZE];
char		packet_buffer[BUF_SIZE];
DWORD				in_packet_size = 0;
int						saved_packet_size = 0;
int			g_myid;
int			g_left_x = 0, g_top_y = 0;
int			skill_timer = 0;

char server_ip[MAX_PATH];
char my_id[MAX_PATH];

HWND cpy_hwnd;
HWND main_window_handle = NULL; // save the window handle

HINSTANCE g_hInst;
LPCTSTR lpszClass = L"Window Class Name";

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM IParam);
BOOL CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

void init();
void ReadPacket(SOCKET sock);
void ProcessPacket(char *ptr);
void Send_Attack_Packet(int skill_num, int damage);

void err_quit(char *msg);
void err_display(char *msg, int err_no);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdParam, int nCmdShow)
{
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);

	HWND hWnd;
	MSG Message;
	WNDCLASSEX WndClass;
	g_hInst = hInstance;

	WndClass.cbSize = sizeof(WndClass);
	WndClass.style = CS_HREDRAW | CS_VREDRAW;
	WndClass.lpfnWndProc = (WNDPROC)WndProc;
	WndClass.cbClsExtra = 0;
	WndClass.cbWndExtra = 0;
	WndClass.hInstance = hInstance;
	WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	WndClass.lpszMenuName = NULL;
	WndClass.lpszClassName = lpszClass;
	WndClass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	RegisterClassEx(&WndClass);

	hWnd = CreateWindow(lpszClass, L"2016180038 장은선", WS_OVERLAPPEDWINDOW, 0, 0, width, height, NULL, (HMENU)NULL, hInstance, NULL);
	main_window_handle = hWnd;
	init();

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	while (GetMessage(&Message, 0, 0, 0)) {
		TranslateMessage(&Message);
		DispatchMessage(&Message);
	}
	return Message.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam) {
	HDC hDC;
	PAINTSTRUCT ps;
	RECT rect;
	srand((unsigned)time(NULL));
	switch (iMessage) {
	case WM_SOCKET:
	{
		if (WSAGETSELECTERROR(lParam)) {
			closesocket((SOCKET)wParam);
			exit(-1);

			break;
		}
		switch (WSAGETSELECTEVENT(lParam)) {
		case FD_READ:
			ReadPacket((SOCKET)wParam);
			break;
		case FD_CLOSE:
			closesocket((SOCKET)wParam);
			exit(-1);
			break;
		}
		break;
	}
	
	case WM_PAINT:
		hDC = BeginPaint(hWnd, &ps);

		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return(DefWindowProc(hWnd, iMessage, wParam, lParam));

}

BOOL CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	HWND hwndOwner;
	RECT rc, rcDlg, rcOwner;
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
		SetDlgItemText(hDlg, IDC_IPADDRESS, L"127.0.0.1");
		SetDlgItemText(hDlg, IDC_EDIT, L"1");
		return TRUE;

	case WM_COMMAND:

		switch (LOWORD(wParam)) {
		case IDC_CONNECT:
			GetDlgItemTextA(hDlg, IDC_IPADDRESS, server_ip, 20);
			GetDlgItemTextA(hDlg, IDC_EDIT, my_id, 10);
			EndDialog(hDlg, IDCANCEL);
			DestroyWindow(hDlg);
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

void init()
{
	WSADATA	wsadata;
	WSAStartup(MAKEWORD(2, 2), &wsadata);

	g_mysocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, 0);

	SOCKADDR_IN ServerAddr;
	ZeroMemory(&ServerAddr, sizeof(SOCKADDR_IN));
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_port = htons(SERVER_PORT);
	ServerAddr.sin_addr.s_addr = inet_addr(server_ip);

	int Result = WSAConnect(g_mysocket, (sockaddr *)&ServerAddr, sizeof(ServerAddr), NULL, NULL, NULL, NULL);

	send_wsabuf.buf = send_buffer;
	send_wsabuf.len = BUF_SIZE;
	recv_wsabuf.buf = recv_buffer;
	recv_wsabuf.len = BUF_SIZE;

	int retval = send(g_mysocket, my_id, strlen(my_id), 0);
	char buf[10];
	retval = recv(g_mysocket, buf, 10, 0);

	if (strcmp(buf, "False") == 0) {
		MessageBoxA(cpy_hwnd, "DB에서 ID,PW를 확인할 수 없습니다..!\n프로그램을 종료합니다.", "DB 오류", MB_OK);
		exit(-1);
	}
	else if (strcmp(buf, "Exist") == 0) {
		MessageBoxA(cpy_hwnd, "이미 접속유저가 있습니다..!\n프로그램을 종료합니다.", "DB 접속", MB_OK);
		exit(-1);
	}
	else if (strcmp(buf, "Newid") == 0) {
		MessageBoxA(cpy_hwnd, "아이디가 없습니다.\nID를 만들어서 게임을 시작합니다.", "DB 생성", MB_OK);
	}

	WSAAsyncSelect(g_mysocket, main_window_handle, WM_SOCKET, FD_CLOSE | FD_READ);
}

void ReadPacket(SOCKET sock) 
{
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
	case SC_LOGIN_OK:
	{
		break;
	}
	case SC_LOGIN_FAIL:
	{
		break;
	}
	case SC_POSITION:
	{
		break;
	}
	case SC_CHAT:
	{
		break;
	}
	case SC_STAT_CHANGE:
	{
		break;
	}
	case SC_REMOVE_OBJECT:
	{
		break;
	}
	case SC_ADD_OBJECT:
	{
		break;
	}
	}
}

void Send_Attack_Packet(int skill_num, int damage) 
{
	//player.skill_timer = 0; // 스킬 타이머를 구현
	SetTimer(cpy_hwnd, 0, 1, NULL);
	cs_packet_attack *my_packet = reinterpret_cast<cs_packet_attack *>(send_buffer);
	my_packet->size = sizeof(my_packet);
	send_wsabuf.len = sizeof(my_packet);
	DWORD iobyte;
	//my_packet->type = CS_Attack;
	//my_packet->skill_num = skill_num;
	//my_packet->damage = damage;

	skill_timer = 1;
	int ret = WSASend(g_mysocket, &send_wsabuf, 1, &iobyte, 0, NULL, NULL);
	if (ret) {
		int error_code = WSAGetLastError();
	}
}

void err_quit(char *msg) 
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	MessageBoxA(NULL, (LPCSTR)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(1);
}

void err_display(char *msg, int err_no) 
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, err_no,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	printf("[%s] %s\n", msg, (char *)lpMsgBuf);
	LocalFree(lpMsgBuf);
}
