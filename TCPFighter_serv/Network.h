#ifndef __NETWORK__H__
#define __NETWORK__H__

#define dfNETWORK_PORT 5000

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

extern SOCKET		g_ListenSock;
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

BOOL recvProc_MoveStart(st_SESSION *pSession, CNPacket *cPacket);
BOOL recvProc_MoveStop(st_SESSION *pSession, CNPacket *cPacket);
BOOL recvProc_Attack1(st_SESSION *pSession, CNPacket *cPacket);
BOOL recvProc_Attack2(st_SESSION *pSession, CNPacket *cPacket);
BOOL recvProc_Attack3(st_SESSION *pSession, CNPacket *cPacket);

BOOL sendProc_CreateMyCharacter();
BOOL sendProc_CreateOtherCharacter();
BOOL sendProc_DeleteCharacter();
BOOL sendProc_MoveStart();
BOOL sendProc_MoveStop();
BOOL sendProc_Attack1();
BOOL sendProc_Attack2();
BOOL sendProc_Attack3();
BOOL sendProc_Damage();
BOOL sendProc_Sync();

void makePacket_CreateMyCharacter();
void makePacket_CreateOtherCharacter();
void makePacket_DeleteCharacter();
void makePacket_MoveStart();
void makePacket_MoveStop();
void makePacket_Attack1();
void makePacket_Attack2();
void makePacket_Attack3();
void makePacket_Damage();
void makePacket_Sync();

#endif