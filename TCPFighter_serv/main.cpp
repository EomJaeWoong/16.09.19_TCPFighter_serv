#include "TCPFighter_serv.h"
#include "StreamQueue.h"
#include "NPacket.h"
#include "Network.h"
#include "Character.h"

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "winmm.lib")

SOCKET	g_ListenSock;
Session	g_Session;

void DataSetup();


void Update();
void ServerControl();
void monitor();

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

/*-------------------------------------------------------------------------------------*/
// Server Setup
/*-------------------------------------------------------------------------------------*/
void DataSetup()
{

}



/*-------------------------------------------------------------------------------------*/
// 게임 로직 처리
/*-------------------------------------------------------------------------------------*/
void Update()
{

}

void ServerControl()
{

}

void monitor()
{

}