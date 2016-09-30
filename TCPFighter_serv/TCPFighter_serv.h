#ifndef __TCPFIGHTERSERV__H__
#define __TCPFIGHTERSERV__H__

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <mmsystem.h>
#include <conio.h>
#include <list>
#include <map>

using namespace std;

#include "PacketDefine.h"
#include "StreamQueue.h"
#include "NPacket.h"
#include "Character.h"
#include "Network.h"
#include "Sector.h"
#include "Log.h"

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "winmm.lib")

//-----------------------------------------------------------------
// ȭ�� �̵� ����.
//-----------------------------------------------------------------
#define dfRANGE_MOVE_TOP	50
#define dfRANGE_MOVE_LEFT	10
#define dfRANGE_MOVE_RIGHT	6390
#define dfRANGE_MOVE_BOTTOM	6390

//-----------------------------------------------------------------
// ĳ���� �̵� �ӵ�
//-----------------------------------------------------------------
#define dfSPEED_PLAYER_X	3
#define dfSPEED_PLAYER_Y	2

//-----------------------------------------------------------------
// �̵� ����üũ ���� / X�� �Ǵ� Y������ 50 Pixel �̻� �����߻��� ����
//-----------------------------------------------------------------
#define dfERROR_RANGE		50

SOCKET			g_ListenSock;
Session			g_Session;
Character		g_CharacterMap;
Sector			g_Sector[dfSECTOR_MAX_Y][dfSECTOR_MAX_X];
int				g_iLogLevel;					//���,���� ����� �α� ����
WCHAR			g_szLogBuff[1024];				//�α� ����� �ʿ��� �ӽ� ����

void DataSetup();
void Update();
void ServerControl();
void monitor();

#endif