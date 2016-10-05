#include <WinSock2.h>
#include <conio.h>
#include <wchar.h>
#include <mmsystem.h>
#include <time.h>
#include <list>
#include <map>

using namespace std;

#pragma comment (lib, "winmm.lib")

#include "ObjectType.h"
#include "StreamQueue.h"
#include "NPacket.h"
#include "Session.h"
#include "SectorDef.h"
#include "Character.h"
#include "Sector.h"
#include "Network.h"
#include "Content.h"
#include "TCPFighter_serv.h"

DWORD			g_dwLoopTime;					//루프 확인용
DWORD			g_dwLoopCount = 0;					//루프 카운트
bool			g_bShutdown = false;			//서버 플래그

void main()
{
	timeBeginPeriod(1);

	DataSetup();
	netSetup();
	
	while (!g_bShutdown)
	{
		netIOProcess();

		Update();

		ServerControl();
		monitor();
	}
}

void ServerControl()
{
	// 키보드 컨트롤 잠금, 풀림 변수
	static bool bControlMode = false;

	//-----------------------------------------------------------------------------------
	// L : 컨트롤 Lock, U : 컨트롤 Unlock, Q : 서버 종료
	//-----------------------------------------------------------------------------------
	if (_kbhit())
	{
		WCHAR ControlKey = _getwch();

		// 컨트롤 Unlock
		if (L'u' == ControlKey || L'U' == ControlKey)
		{
			bControlMode = true;

			wprintf(L"Control Mode : Press Q - Quit \n");
			wprintf(L"Control Mode : Press L - Key Lock \n");
		}

		// 컨트롤 Lock
		if (L'l' == ControlKey || L'L' == ControlKey)
		{
			bControlMode = false;

			wprintf(L"Control Mode : Press U - Key Unlock \n");
		}

		// 특정 기능들
		// 종료
		if (L'q' == ControlKey || L'Q' == ControlKey)
		{
			g_bShutdown = true;
		}
	}
}

void monitor()
{
	DWORD dwEndTime = timeGetTime();

	if (dwEndTime - g_dwLoopTime > 1000)
	{
		time_t timer;
		tm today;

		time(&timer);

		localtime_s(&today, &timer); // 초 단위의 시간을 분리하여 구조체에 넣기

		wprintf(L"[%4d/%02d/%02d][%02d:%02d:%02d]  Frame : %d  Loop : %d\n",
			today.tm_year + 1900, today.tm_mon + 1, today.tm_mday, 
			today.tm_hour, today.tm_min, today.tm_sec,
			dfSERVER_FRAME, g_dwLoopCount);
		g_dwLoopTime = dwEndTime;
		g_dwLoopCount = 0;
	}

	else
		g_dwLoopCount++;
}