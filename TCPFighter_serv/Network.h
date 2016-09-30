#ifndef __NETWORK__H__
#define __NETWORK__H__

#define dfNETWORK_PORT 5000
#define dfNETWORK_PACKET_RECV_TIMEOUT 2000

#define dfRECKONING_SPEED_PLAYER_X 3
#define dfRECKONING_SPEED_PLAYER_Y 2

/*------------------------------------------------------------------*/
// 匙飘况农 技记
/*------------------------------------------------------------------*/
struct st_SESSION
{
	SOCKET socket;

	DWORD dwSessionID;

	CAyaStreamSQ RecvQ;
	CAyaStreamSQ SendQ;

	DWORD dwTrafficTick;
	DWORD dwTrafficCount;
	DWORD dwTrafficSecondTick;
};

typedef list<FD_SET *> SetList;
typedef map<SOCKET, st_SESSION *> Session;

extern SOCKET	g_ListenSock;
extern Session	g_Session;

void netSetup();
void netIOProcess();
BOOL netProc_Accept(SOCKET socket);
BOOL netProc_Send(SOCKET socket);
void netProc_Recv(SOCKET socket);

st_SESSION *FindSession(SOCKET socket);
st_SESSION *CreateSession(SOCKET socket);
void DisconnectSession(SOCKET socket);

BOOL PacketProc(st_SESSION *pSession);

BOOL recvProc_MoveStart(st_SESSION *pSession, CNPacket *pPacket);
BOOL recvProc_MoveStop(st_SESSION *pSession, CNPacket *pPacket);
BOOL recvProc_Attack1(st_SESSION *pSession, CNPacket *pPacket);
BOOL recvProc_Attack2(st_SESSION *pSession, CNPacket *pPacket);
BOOL recvProc_Attack3(st_SESSION *pSession, CNPacket *pPacket);

BOOL sendProc_CreateMyCharacter();
BOOL sendProc_CreateOtherCharacter();
BOOL sendProc_DeleteCharacter();
BOOL sendProc_MoveStart(st_CHARACTER *pCharacter);
BOOL sendProc_MoveStop();
BOOL sendProc_Attack1();
BOOL sendProc_Attack2();
BOOL sendProc_Attack3();
BOOL sendProc_Damage();
BOOL sendProc_Sync();

void makePacket_CreateMyCharacter(st_NETWORK_PACKET_HEADER *pHeader, CNPacket *pPacket, DWORD ID);
void makePacket_CreateOtherCharacter();
void makePacket_DeleteCharacter();
void makePacket_MoveStart(st_NETWORK_PACKET_HEADER *pHeader, CNPacket *pPacket, st_CHARACTER *pCharacter);
void makePacket_MoveStop();
void makePacket_Attack1();
void makePacket_Attack2();
void makePacket_Attack3();
void makePacket_Damage();
void makePacket_Sync();

void SendPacket_SectorOne(int iSectorX, int iSectorY, st_NETWORK_PACKET_HEADER *pHeader, 
	CNPacket *pPacket, st_SESSION *pExceptSession);
void SendPacket_Unicast(st_SESSION *pSession, st_NETWORK_PACKET_HEADER *pHeader, CNPacket *pPacket);
void SendPacket_Around(st_SESSION *pSession, st_NETWORK_PACKET_HEADER *pHeader, CNPacket *pPacket, bool bSendMe = false);
void Sendpacket_Broadcast(st_SESSION *pSession, st_NETWORK_PACKET_HEADER *pHeader, CNPacket *pPacket);

int DeadReckoningPos(DWORD dwAction, DWORD dwActionTick, int iOldPosX, int iOldPosY, int *pPosX, int *pPosY);

#endif