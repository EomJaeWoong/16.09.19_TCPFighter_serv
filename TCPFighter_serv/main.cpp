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
	// Ű���� ��Ʈ�� ���, Ǯ�� ����
	static bool bControlMode = false;

	//-----------------------------------------------------------------------------------
	// L : ��Ʈ�� Lock, U : ��Ʈ�� Unlock, Q : ���� ����
	//-----------------------------------------------------------------------------------
	if (_kbhit())
	{
		WCHAR ControlKey = _getwch();

		// ��Ʈ�� Unlock
		if (L'u' == ControlKey || L'U' == ControlKey)
		{
			bControlMode = true;

			wprintf(L"Control Mode : Press Q - Quit \n");
			wprintf(L"Control Mode : Press L - Key Lock \n");
		}

		// ��Ʈ�� Lock
		if (L'l' == ControlKey || L'L' == ControlKey)
		{
			bControlMode = false;

			wprintf(L"Control Mode : Press U - Key Unlock \n");
		}

		// Ư�� ��ɵ�
		// ����
		if (L'q' == ControlKey || L'Q' == ControlKey)
		{
			//���� ��Ű��
		}
	}
}

void monitor()
{

}