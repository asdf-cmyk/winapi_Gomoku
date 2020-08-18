// server.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "server.h"
#include <WinSock2.h>
#include <vector>
#include <ctime>
#pragma comment(lib, "ws2_32.lib")

#define WM_ASYNC	WM_USER+2
#define CHATLOGLEN 8
#define BOARDMAX 19

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

WSADATA wsadata;
SOCKET s, cs;
char buffer[100];
TCHAR msg[200];
TCHAR msg2[200][100];
TCHAR playMsg[8];
SOCKADDR_IN addr = { 0 }, c_addr;
TCHAR str[100];
int chatLogLen, chatLogNum = 0;
//int size, msgLen;

SOCKET room[2];
bool color[2] = { 0, 0 };
int roomCnt = 0;
int /*selRet, */ClientNum, i2;
fd_set read, readTmp;
TIMEVAL timeVal;
//SOCKET toClient;
bool sendFlag = true;
bool gameEndFlag = false;

struct gamePlay
{
	unsigned short header;
	int x;
	int y;
	bool playerColor;
} playData;

std::vector<gamePlay> playLog;

unsigned short boardState[BOARDMAX][BOARDMAX];

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

//void checkWin();
bool InitConnect(HWND);
bool Async_accept(HWND);
bool Async_read();
bool checkVictory(unsigned short, int, int);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: Place code here.

	// Initialize global strings
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_SERVER, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SERVER));

	MSG msg;

	// Main message loop:
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SERVER));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_SERVER);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // Store instance handle in our global variable

	HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

	if (!hWnd)
	{
		return FALSE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HDC hdc;
	PAINTSTRUCT ps;
	//int size, msgLen;
	//std::vector<char[200]>

	switch (message)
	{
	case WM_CREATE:
		srand((unsigned int)std::time(NULL));
		if (!InitConnect(hWnd))
			return 0;
		break;
	case WM_ASYNC:
		switch (lParam)
		{
		case FD_ACCEPT:
			if (!Async_accept(hWnd))
				return 0;
			break;
		case FD_READ:
			//
			//			/*if (playData.header == 2)
			//			{
			//#ifdef _UNICODE
			//				msgLen = MultiByteToWideChar(CP_ACP, 0, buffer, strlen(buffer), NULL, NULL);
			//				MultiByteToWideChar(CP_ACP, 0, buffer, strlen(buffer), msg, msgLen);
			//				msg[msgLen] = NULL;
			//				if (read.fd_array[i] == room[0])
			//					msg[0] = '1';
			//				else if (read.fd_array[i] == room[1])
			//					msg[0] = '2';
			//				for (int i = 0; i <= msgLen; i++)
			//					msg2[chatLogNum][i] = msg[i];
			//				chatLogNum++;
			//#else
			//				strcpy_s(msg, buffer);
			//#endif
			//			}*/

			if (!Async_read())
				return 0;
			if (!gameEndFlag &&
				checkVictory(playLog[playLog.size() - 1].playerColor + 1,
				playLog[playLog.size() - 1].x, playLog[playLog.size() - 1].y))
			{
				Sleep(1000);

				gameEndFlag = true;
				buffer[0] = 2;
				buffer[1] = true;
				buffer[2] = '\0';
				send(read.fd_array[ClientNum], (LPSTR)buffer, 3, 0);

				buffer[1] = false;
				send(read.fd_array[3 - ClientNum], (LPSTR)buffer, 3, 0);
			}
			//InvalidateRgn(hWnd, NULL, TRUE);
			break;
		default:
			break;
		}
		break;
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	break;
	case WM_PAINT:
	{
		hdc = BeginPaint(hWnd, &ps);
		/*if (_tcscmp(msg, _T("")))
		TextOut(hdc, 0, 0, msg, (int)_tcslen(msg));*/
		int i = chatLogNum <= CHATLOGLEN ? 0 : chatLogNum - CHATLOGLEN;
		int chatLog = chatLogNum <= CHATLOGLEN ? chatLogNum : i + CHATLOGLEN;
		int j = 0;
		for (i; i < chatLog; i++)
			if (_tcscmp(msg, _T("")))
				TextOut(hdc, 0, 0 + 50 * j++, msg2[i], (int)_tcslen(msg2[i]));
		TCHAR tmpStr[6] = L"ют╥б : ";
		TextOut(hdc, 0, 500, tmpStr, (int)_tcslen(tmpStr));
		TextOut(hdc, 50, 500, str, (int)_tcslen(str));
		EndPaint(hWnd, &ps);
	}
	break;
	case WM_CHAR:
		/*if (wParam == VK_RETURN)
		{
		if (read.fd_array[i] == INVALID_SOCKET)
		return 0;
		else
		{
		#ifdef _UNICODE
		msgLen = WideCharToMultiByte(CP_ACP, 0, str, -1, NULL,
		0, NULL, NULL);
		WideCharToMultiByte(CP_ACP, 0, str, -1, buffer, msgLen,
		NULL, NULL);
		#else
		strcpy_s(buffer, str);
		#endif
		for (i2 = 0; i2 < read.fd_count; i2++)
		send(read.fd_array[i2], (LPSTR)buffer, strlen(buffer) + 1, 0);
		chatLogLen = 0;
		return 0;
		}
		}
		str[chatLogLen++] = wParam;
		str[chatLogLen] = NULL;*/
		InvalidateRgn(hWnd, NULL, TRUE);
		return 0;
	case WM_DESTROY:
		closesocket(s);
		WSACleanup();
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

bool InitConnect(HWND hWnd)
{
	WSAStartup(MAKEWORD(2, 2), &wsadata);
	addr.sin_family = AF_INET;
	addr.sin_port = 20;
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	s = socket(AF_INET, SOCK_STREAM, 0);
	bind(s, (LPSOCKADDR)&addr, sizeof(addr));
	WSAAsyncSelect(s, hWnd, WM_ASYNC, FD_ACCEPT);
	if (listen(s, 5) == -1) return false;
	FD_ZERO(&read);
	FD_SET(s, &read);

	return true;
}

bool Async_accept(HWND hWnd)
{
	readTmp = read;
	timeVal.tv_sec = 1;
	timeVal.tv_usec = 0;
	int selRet = select(NULL, &readTmp, NULL, NULL, &timeVal);
	if (selRet == SOCKET_ERROR) return false;
	if (selRet == 0) return false;
	int size = sizeof(c_addr);
	cs = accept(s, (LPSOCKADDR)&c_addr, &size);
	FD_SET(cs, &read);
	WSAAsyncSelect(cs, hWnd, WM_ASYNC, FD_READ);

	if (roomCnt < 3)
	{
		room[roomCnt++] = cs;
		if (roomCnt >= 2 && !color[0] && !color[1])
			color[1] = (color[0] = rand() % 2) ? 0 : 1;
	}

	return true;
}

bool Async_read()
{
	readTmp = read;
	timeVal.tv_sec = 1;
	timeVal.tv_usec = 0;
	int selRet = select(NULL, &readTmp, NULL, NULL, &timeVal);
	if (selRet == SOCKET_ERROR) return false;
	if (selRet == 0) return false;

	for (ClientNum = 1; ClientNum < read.fd_count; ClientNum++)
		if (FD_ISSET(read.fd_array[ClientNum], &readTmp))
			break;

	int msgLen = recv(read.fd_array[ClientNum], buffer, 100, 0);
	if (msgLen == 0)
	{
		FD_CLR(read.fd_array[ClientNum], &read);
		closesocket(readTmp.fd_array[ClientNum]);
	}

	if (gameEndFlag) return false;

	if (buffer[0] == 1)
	{
		playData.header = buffer[0];
		playData.x = (int)buffer[1] * 10 + (int)buffer[2];
		playData.y = (int)buffer[3] * 10 + (int)buffer[4];
		if (read.fd_array[ClientNum] == room[0])
			playData.playerColor = buffer[msgLen++] = color[0];
		else if (read.fd_array[ClientNum] == room[1])
			playData.playerColor = buffer[msgLen++] = color[1];
		buffer[msgLen] = NULL;

		if (playLog.size() > 0 &&
			(boardState[playData.x][playData.y] || buffer[5] == playLog[playLog.size() - 1].playerColor))
		{
			sendFlag = false;
			return false;
		}
		else
		{
			sendFlag = true;
			if (!buffer[5])
				boardState[playData.x][playData.y] = 1;
			else
				boardState[playData.x][playData.y] = 2;
			playLog.push_back(playData);
		}

		if (sendFlag)
		{
			for (int i = 1; i < read.fd_count; i++)
				send(read.fd_array[i], (LPSTR)buffer, msgLen, 0);
			sendFlag = false;
		}
	}

	return true;
}

bool checkVictory(unsigned short stoneColor, int x, int y)
{
	bool existAround[3][3] = { false, };

	for (int i = -1; i <= 1; i++)
	{
		for (int j = -1; j <= 1; j++)
		{
			if (!i && !j) continue;
			else if (boardState[x + i][y + j] == stoneColor)
				existAround[1 + i][1 + j] = true;
		}
	}

	for (int i = -1; i <= 1; i++)
	{
		for (int j = -1; j <= 1; j++)
		{
			int stoneCnt = 0;
			if (existAround[1 + i][1 + j])
			{
				int i2 = x, j2 = y;
				while (boardState[i2][j2] == stoneColor && i2 >= 0 && j2 >= 0)
				{
					i2 += i;
					j2 += j;
					stoneCnt++;
				}
				if (existAround[1 - i][1 - j])
				{
					i2 = y, j2 = x;
					while (boardState[i2][j2] == stoneColor && i2 < 7 && j2 < 7)
					{
						i2 -= i;
						j2 -= j;
						stoneCnt++;
					}
				}
			}
			if (stoneCnt >= 5)
			{
				return true;
				break;
			}
		}
	}

	return false;
}