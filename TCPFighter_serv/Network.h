#ifndef __NETWORK__H__
#define __NETWORK__H__

#define dfNETWORK_PORT 5000
#define dfNETWORK_PACKET_RECV_TIMEOUT 2000

#define dfRECKONING_SPEED_PLAYER_X 6
#define dfRECKONING_SPEED_PLAYER_Y 4

typedef list<FD_SET *> SetList;

extern Session	g_Session;

void netSetup();
void netIOProcess();
void SelectProc(SOCKET *SocketTable, FD_SET *ReadSet, FD_SET *WriteSet, int iSize);

BOOL netProc_Accept(SOCKET socket);						//Accept 贸府
BOOL netProc_Send(SOCKET socket);						//Send 贸府
void netProc_Recv(SOCKET socket);						//receive 贸府

st_SESSION *FindSession(SOCKET socket);					//技记 茫扁
st_SESSION *CreateSession(SOCKET socket);				//技记 积己
void DisconnectSession(SOCKET socket);					//技记 谗扁
void DisconnectClient(DWORD dwSessionID);

BOOL PacketProc(st_SESSION *pSession);

/*-------------------------------------------------------------------------------------*/
// Client -> Server 菩哦贸府
/*-------------------------------------------------------------------------------------*/
BOOL recvProc_MoveStart(st_SESSION *pSession, CNPacket *pPacket);
BOOL recvProc_MoveStop(st_SESSION *pSession, CNPacket *pPacket);
BOOL recvProc_Attack1(st_SESSION *pSession, CNPacket *pPacket);
BOOL recvProc_Attack2(st_SESSION *pSession, CNPacket *pPacket);
BOOL recvProc_Attack3(st_SESSION *pSession, CNPacket *pPacket);

/*-------------------------------------------------------------------------------------*/
// Make Packet
/*-------------------------------------------------------------------------------------*/
void makePacket_CreateMyCharacter(CNPacket *pPacket, DWORD ID, BYTE byDirection, short shX,
	short shY, BYTE chHP);
void makePacket_CreateOtherCharacter(CNPacket *pPacket, DWORD ID, BYTE byDirection, short shX,
	short shY, BYTE chHP);
void makePacket_DeleteCharacter(CNPacket *pPacket, DWORD ID);
void makePacket_MoveStart(CNPacket *pPacket, DWORD ID, BYTE byMoveDirection, short shX, short shY);
void makePacket_MoveStop(CNPacket *pPacket, DWORD ID, BYTE byDirection, short shX, short shY);
void makePacket_Attack1(CNPacket *pPacket, DWORD ID, BYTE byDirection, short shX, short shY);
void makePacket_Attack2(CNPacket *pPacket, DWORD ID, BYTE byDirection, short shX, short shY);
void makePacket_Attack3(CNPacket *pPacket, DWORD ID, BYTE byDirection, short shX, short shY);
void makePacket_Damage(CNPacket *pPacket, DWORD AttackID, DWORD DamageID, BYTE DamageHP);
void makePacket_Sync(CNPacket *pPacket, DWORD ID, short shX, short shY);

/*-------------------------------------------------------------------------------------*/
// 菩哦 焊郴绰 窃荐
/*-------------------------------------------------------------------------------------*/
void SendPacket_SectorOne(int iSectorX, int iSectorY, CNPacket *pPacket, st_SESSION *pExceptSession);
void SendPacket_Unicast(st_SESSION *pSession, CNPacket *pPacket);
void SendPacket_Around(st_SESSION *pSession, CNPacket *pPacket, bool bSendMe = false);
void SendPacket_Broadcast(st_SESSION *pSession, CNPacket *pPacket);

/*-------------------------------------------------------------------------------------*/
// DeadReckoning
/*-------------------------------------------------------------------------------------*/
int DeadReckoningPos(DWORD dwAction, DWORD dwActionTick, int iOldPosX, int iOldPosY, int *pPosX, int *pPosY);

#endif