#include "TCPFighter_serv.h"

void main()
{
	timeBeginPeriod(1);

	DataSetup();
	netSetup();
	
	while (1)
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
			//종료 시키기
		}
	}
}

void monitor()
{

}