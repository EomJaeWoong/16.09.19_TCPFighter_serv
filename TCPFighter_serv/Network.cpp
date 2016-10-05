#include <WinSock2.h>
#include <WS2tcpip.h>
#include <list>
#include <map>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;

#include "PacketDefine.h"
#include "ObjectType.h"
#include "StreamQueue.h"
#include "NPacket.h"
#include "Session.h"
#include "SectorDef.h"
#include "Character.h"
#include "Sector.h"
#include "Content.h"
#include "Network.h"
#include "Log.h"

DWORD			uiSessionCount;
SOCKET			g_ListenSock;
Session			g_Session;

/*-------------------------------------------------------------------------------------*/
// Server Setup
/*-------------------------------------------------------------------------------------*/
void netSetup()
{
	int retval;

	//-----------------------------------------------------------------------------------
	// Winsock 초기화
	//-----------------------------------------------------------------------------------
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return;


	//-----------------------------------------------------------------------------------
	// socket()
	//-----------------------------------------------------------------------------------
	g_ListenSock = socket(AF_INET, SOCK_STREAM, 0);
	if (g_ListenSock == INVALID_SOCKET)
		return;

	//-----------------------------------------------------------------------------------
	// bind
	//-----------------------------------------------------------------------------------
	SOCKADDR_IN sockaddr;
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(dfNETWORK_PORT);
	InetPton(AF_INET, L"127.0.0.1", &sockaddr.sin_addr);
	retval = bind(g_ListenSock, (SOCKADDR*)&sockaddr, sizeof(sockaddr));
	if (retval == SOCKET_ERROR)
		return;

	//----------------------------------------------------------------------------------
	// Listen
	//----------------------------------------------------------------------------------
	retval = listen(g_ListenSock, SOMAXCONN);
	if (retval == SOCKET_ERROR)
		return;

	uiSessionCount = 0;
}

/*-------------------------------------------------------------------------------------*/
// Network IO
/*-------------------------------------------------------------------------------------*/
void netIOProcess()
{
	int retval;

	Session::iterator sessionIter;
	SetList ReadList, WriteList;

	//---------------------------------------------------------------------------------
	// socket 등록
	//---------------------------------------------------------------------------------
	FD_SET *ReadSet = new FD_SET;
	FD_SET *WriteSet = new FD_SET;
	int iSocketCount = 0;

	FD_ZERO(ReadSet);
	FD_ZERO(WriteSet);

	ReadList.push_back(ReadSet);
	WriteList.push_back(WriteSet);

	FD_SET(g_ListenSock, ReadSet);
	iSocketCount++;

	sessionIter = g_Session.begin();

	while (sessionIter != g_Session.end()){
		if (iSocketCount > 64)
		{
			ReadSet = new FD_SET;
			WriteSet = new FD_SET;

			ReadList.push_back(ReadSet);
			WriteList.push_back(WriteSet);

			FD_ZERO(&ReadSet);
			FD_ZERO(&WriteSet);

			iSocketCount = 0;
		}

		if (sessionIter != g_Session.end() && sessionIter->second->SendQ.GetUseSize() > 0)
			FD_SET(sessionIter->second->socket, WriteSet);

		FD_SET(sessionIter->second->socket, ReadSet);
		sessionIter++;
		iSocketCount++;
	}
	//---------------------------------------------------------------------------------

	TIMEVAL Time;
	Time.tv_sec = 0;
	Time.tv_usec = 0;

	SetList::iterator ReadIter;
	SetList::iterator WriteIter;

	for (ReadIter = ReadList.begin(), WriteIter = WriteList.begin();
		ReadIter != ReadList.end() || WriteIter != WriteList.end(); ++ReadIter, ++WriteIter){
		retval = select(0, (*ReadIter), (*WriteIter), NULL, &Time);

		if (retval == 0)		break;

		else if (retval < 0)
		{
			DWORD dwError = GetLastError();
			wprintf(L"Select() Error : %d\n", dwError);
			exit(1);
		}

		else
		{
			for (int iCnt = 0; iCnt < retval; iCnt++){
				if (FD_ISSET((*ReadIter)->fd_array[iCnt], *ReadIter))
				{
					//---------------------------------------------------------------------------
					// Accept 처리
					//---------------------------------------------------------------------------
					if ((*ReadIter)->fd_array[iCnt] == g_ListenSock)
					{
						netProc_Accept((*ReadIter)->fd_array[iCnt]);
						continue;
					}

					//---------------------------------------------------------------------------
					// Receive 처리
					//---------------------------------------------------------------------------
					else
					{
						netProc_Recv((*ReadIter)->fd_array[iCnt]);
						continue;
					}
				}

				//-------------------------------------------------------------------------------
				// Send 처리
				//-------------------------------------------------------------------------------
				else if (FD_ISSET((*WriteIter)->fd_array[iCnt], *WriteIter))
					netProc_Send((*WriteIter)->fd_array[iCnt]);
			}
		}
	}

	//---------------------------------------------------------------------------------
	// FD_SET List 정리
	//---------------------------------------------------------------------------------
	if (0 == iSocketCount)
	{
		delete ReadSet;
		delete WriteSet;
	}

	for (ReadIter = ReadList.begin(); ReadIter != ReadList.end(); ++ReadIter)
		delete (*ReadIter);

	for (WriteIter = WriteList.begin(); WriteIter != WriteList.end(); ++WriteIter)
		delete (*WriteIter);

	ReadList.clear();
	WriteList.clear();
}

/*-------------------------------------------------------------------------------------*/
// Accpet Process
/*-------------------------------------------------------------------------------------*/
BOOL netProc_Accept(SOCKET socket)
{
	int addrlen = sizeof(SOCKADDR_IN);
	WCHAR wcAddr[16];
	SOCKET sessionSock;
	SOCKADDR_IN sockaddr;
	CNPacket cPacket;

	sessionSock = accept(socket, (SOCKADDR *)&sockaddr, &addrlen);
	if (sessionSock == INVALID_SOCKET)
	{
		_LOG(dfLOG_LEVEL_ERROR, L"Accept Socket Error");
		return FALSE;
	}

	//-----------------------------------------------------------------------------------
	// Session 생성
	//-----------------------------------------------------------------------------------
	st_SESSION *pSession = CreateSession(sessionSock);
	g_Session.insert(pair<SOCKET, st_SESSION *>(sessionSock, pSession));

	//-----------------------------------------------------------------------------------
	// Character 생성
	//-----------------------------------------------------------------------------------
	st_CHARACTER *pCharacter = CreateCharacter(pSession);
	g_CharacterMap.insert(pair<DWORD, st_CHARACTER *>(pSession->dwSessionID, pCharacter));
	Sector_UpdateCharacter(pCharacter);

	//-----------------------------------------------------------------------------------
	// 캐릭터 생성 전송
	//-----------------------------------------------------------------------------------
	makePacket_CreateMyCharacter(&cPacket, pSession->dwSessionID, pCharacter->byDirection, 
		pCharacter->shX, pCharacter->shY, pCharacter->chHP);
	SendPacket_Unicast(pSession, &cPacket);
	_LOG(dfLOG_LEVEL_DEBUG, L"PACKET_Unicast - CreateMyCharacter [SessionID:%d][X:%d][Y:%d]",
		pCharacter->dwSessionID, pCharacter->shX, pCharacter->shY);

	//-----------------------------------------------------------------------------------
	// 주위 섹터의 정보 보냄
	//-----------------------------------------------------------------------------------
	CharacterSectorUpdatePacket(pCharacter);
	_LOG(dfLOG_LEVEL_DEBUG, L"PACKET_Around - CreateOtherCharacter [SessionID:%d][X:%d][Y:%d]",
		pCharacter->dwSessionID, pCharacter->shX, pCharacter->shY);

	InetNtop(AF_INET, &sockaddr.sin_addr, wcAddr, sizeof(wcAddr));
	_LOG(dfLOG_LEVEL_DEBUG, L"Accept - %s:%d Socket : %d", wcAddr, ntohs(sockaddr.sin_port),
		pSession->socket);

	return TRUE;
}

/*-------------------------------------------------------------------------------------*/
// Send Process
/*-------------------------------------------------------------------------------------*/
BOOL netProc_Send(SOCKET socket)
{
	int retval;
	st_SESSION *pSession = FindSession(socket);

	while (pSession->SendQ.GetUseSize() > 0)
	{
		retval = send(socket, pSession->SendQ.GetReadBufferPtr(),
			pSession->SendQ.GetNotBrokenGetSize(), 0);
		pSession->SendQ.RemoveData(retval);
	}

	if (retval == 0)
		return FALSE;

	else if (retval < 0)
	{
		_LOG(dfLOG_LEVEL_ERROR, L"Send Error - SessionID : %d", pSession->dwSessionID);
		DisconnectSession(socket);
		return FALSE;
	}

	return TRUE;
}

/*-------------------------------------------------------------------------------------*/
// Receive Process
/*-------------------------------------------------------------------------------------*/
void netProc_Recv(SOCKET socket)
{
	int retval;
	st_SESSION *pSession = FindSession(socket);

	retval = recv(pSession->socket, pSession->RecvQ.GetWriteBufferPtr(),
		pSession->RecvQ.GetNotBrokenPutSize(), 0);
	
	//-----------------------------------------------------------------------------------
	// 현 세션이 마지막으로 메시지를 받은 시간
	//-----------------------------------------------------------------------------------
	pSession->dwTrafficTick = timeGetTime();

	pSession->RecvQ.MoveWritePos(retval);

	if (retval == 0)
	{
		DisconnectSession(socket);
		return;
	}

	//-----------------------------------------------------------------------------------
	// RecvQ에 데이터가 남아있는 한 계속 패킷 처리
	//-----------------------------------------------------------------------------------
	while (pSession->RecvQ.GetUseSize() > 0)
	{
		if (retval < 0)
		{
			DisconnectSession(socket);
			return;
		}

		else
			if (PacketProc(pSession))		break;
	}
}

/*-------------------------------------------------------------------------------------*/
// 패킷에 대한 처리
/*-------------------------------------------------------------------------------------*/
BOOL PacketProc(st_SESSION *pSession)
{
	st_NETWORK_PACKET_HEADER header;
	CNPacket cPacket;
	BYTE byEndCode = 0;

	//--------------------------------------------------------------------------------------*/
	//RecvQ 용량이 header보다 작은지 검사
	/*--------------------------------------------------------------------------------------*/
	if (pSession->RecvQ.GetUseSize() < sizeof(st_NETWORK_PACKET_HEADER))
		return FALSE;

	pSession->RecvQ.Peek((char *)&header, sizeof(st_NETWORK_PACKET_HEADER));

	/*--------------------------------------------------------------------------------------*/
	//header + payload 용량이 RecvQ용량보다 큰지 검사
	/*--------------------------------------------------------------------------------------*/
	if (pSession->RecvQ.GetUseSize() < header.bySize + sizeof(st_NETWORK_PACKET_HEADER))
		return FALSE;

	pSession->RecvQ.RemoveData(sizeof(st_NETWORK_PACKET_HEADER));

	/*--------------------------------------------------------------------------------------*/
	//payload를 cPacket에 뽑고 같은지 검사
	/*--------------------------------------------------------------------------------------*/
	if (header.bySize !=
		pSession->RecvQ.Get((char *)cPacket.GetBufferPtr(), header.bySize))
		return FALSE;

	pSession->RecvQ.Get((char *)&byEndCode, 1);
	if (dfNETWORK_PACKET_END != byEndCode)
		return FALSE;

	/*--------------------------------------------------------------------------------------*/
	// Message 타입에 따른 Packet 처리
	/*--------------------------------------------------------------------------------------*/
	switch (header.byType)
	{
	case dfPACKET_CS_MOVE_START :
		return recvProc_MoveStart(pSession, &cPacket);
		break;

	case dfPACKET_CS_MOVE_STOP :
		return recvProc_MoveStop(pSession, &cPacket);
		break;

	case dfPACKET_CS_ATTACK1 :
		return recvProc_Attack1(pSession, &cPacket);
		break;

	case dfPACKET_CS_ATTACK2 :
		return recvProc_Attack2(pSession, &cPacket);
		break;

	case dfPACKET_CS_ATTACK3 :
		return recvProc_Attack3(pSession, &cPacket);
		break;

	default :
		_LOG(dfLOG_LEVEL_ERROR, L"Packet Proc Error [SessionID : %d][PacketType : %d] ", 
			pSession->dwSessionID, header.byType);
		DisconnectSession(pSession->socket);
		break;
	}

	return TRUE;
}

/*-------------------------------------------------------------------------------------*/
// 세션 찾기
/*-------------------------------------------------------------------------------------*/
st_SESSION *FindSession(SOCKET socket)
{
	Session::iterator sessionIter;
	sessionIter = g_Session.find(socket);

	return sessionIter->second;
}

/*-------------------------------------------------------------------------------------*/
// 세션 생성
/*-------------------------------------------------------------------------------------*/
st_SESSION *CreateSession(SOCKET socket)
{
	st_SESSION *pSession = new st_SESSION;

	pSession->socket = socket;
	pSession->dwSessionID = ++uiSessionCount;

	pSession->dwTrafficTick = 0;
	pSession->dwTrafficCount = 0;
	pSession->dwTrafficSecondTick = 0;

	return pSession;
}

/*-------------------------------------------------------------------------------------*/
// 세션 끊기
/*-------------------------------------------------------------------------------------*/
void DisconnectSession(SOCKET socket)
{
	Session::iterator sessionIter;
	sessionIter = g_Session.find(socket);
	
	_LOG(dfLOG_LEVEL_DEBUG, L"Disconnect - [SessionID:%d][socket:%d]",
		sessionIter->second->dwSessionID, socket);
	shutdown(socket, SD_BOTH);
	closesocket(socket);
	delete sessionIter->second;
	g_Session.erase(sessionIter);
}

/*-------------------------------------------------------------------------------------*/
// Client -> Server 패킷처리

//---------------------------------------------------------------------------------------
// 캐릭터 이동 시작
//---------------------------------------------------------------------------------------
BOOL recvProc_MoveStart(st_SESSION *pSession, CNPacket *pPacket)
{
	CNPacket cPacket;
	BYTE byDirection;
	short shX;
	short shY;

	*pPacket >> byDirection;
	*pPacket >> shX;
	*pPacket >> shY;

	_LOG(dfLOG_LEVEL_DEBUG, L"# MOVESTART # [SessionID:%d][Direction:%d][X:%d][Y:%d]",
		pSession->dwSessionID, byDirection, shX, shY);

	st_CHARACTER *pCharacter = FindCharacter(pSession->dwSessionID);

	if (NULL == pCharacter)
	{
		_LOG(dfLOG_LEVEL_ERROR, L"# MOVESTART > SessionID:%d Character Not Found!", 
			pSession->dwSessionID);
		return FALSE;
	}

	//-----------------------------------------------------------------------------------
	// 받은 좌표와 서버 좌표가 너무 다를때 좌표 보정
	//-----------------------------------------------------------------------------------
	if (abs(pCharacter->shX - shX) > dfERROR_RANGE || abs(pCharacter->shY - shY) > dfERROR_RANGE)
	{
		int idrX, idrY;
		// DeadReckoning으로 위치 확인
		int iDeadRecFrame = DeadReckoningPos(pCharacter->dwAction, pCharacter->dwActionTick,
			pCharacter->shActionX, pCharacter->shActionY, &idrX, &idrY);

		// 차이나면 좌표 보정
		if (abs(idrX - shX) > dfERROR_RANGE || abs(idrY - shY) > dfERROR_RANGE)
		{
			makePacket_Sync(&cPacket, pSession->dwSessionID, idrX, idrY);
			SendPacket_Around(pSession, &cPacket, true);
		}

		shX = idrX;
		shY = idrY;
	}

	//현재 액션 설정
	pCharacter->dwAction = byDirection;

	//현재 이동방향 설정
	pCharacter->byMoveDirection = byDirection;

	//캐릭터 방향 설정
	switch (byDirection)
	{
	case dfPACKET_MOVE_DIR_RR :
	case dfPACKET_MOVE_DIR_RU :
	case dfPACKET_MOVE_DIR_RD :
		pCharacter->byDirection = dfPACKET_MOVE_DIR_RR;
		break;

	case dfPACKET_MOVE_DIR_LL :
	case dfPACKET_MOVE_DIR_LU :
	case dfPACKET_MOVE_DIR_LD :
		pCharacter->byDirection = dfPACKET_MOVE_DIR_LL;
		break;
	}

	pCharacter->shX = shX;
	pCharacter->shY = shY;

	//섹터 업데이트
	if (Sector_UpdateCharacter(pCharacter))
		CharacterSectorUpdatePacket(pCharacter);

	//tick정보 저장
	pCharacter->dwActionTick = timeGetTime();
	pCharacter->shActionX = pCharacter->shX;
	pCharacter->shActionY = pCharacter->shY;

	makePacket_MoveStart(&cPacket, pCharacter->dwSessionID, byDirection, shX, shY);
	SendPacket_Around(pCharacter->pSession, &cPacket);

	return TRUE;
}

//---------------------------------------------------------------------------------------
// 캐릭터 이동 중지
//---------------------------------------------------------------------------------------
BOOL recvProc_MoveStop(st_SESSION *pSession, CNPacket *pPacket)
{
	CNPacket cPacket;
	BYTE byDirection;
	short shX;
	short shY;

	*pPacket >> byDirection;
	*pPacket >> shX;
	*pPacket >> shY;

	_LOG(dfLOG_LEVEL_DEBUG, L"# MOVESTOP # [SessionID:%d][Direction:%d][X:%d][Y:%d]",
		pSession->dwSessionID, byDirection, shX, shY);

	st_CHARACTER *pCharacter = FindCharacter(pSession->dwSessionID);

	if (NULL == pCharacter)
	{
		_LOG(dfLOG_LEVEL_ERROR, L"# MOVESTOP > SessionID:%d Character Not Found!\n",
			pSession->dwSessionID);
		return FALSE;
	}

	//-----------------------------------------------------------------------------------
	// 받은 좌표와 서버 좌표가 너무 다를때 좌표 보정
	//-----------------------------------------------------------------------------------
	if (abs(pCharacter->shX - shX) > dfERROR_RANGE || abs(pCharacter->shY - shY) > dfERROR_RANGE)
	{
		int idrX, idrY;
		// DeadReckoning으로 위치 확인
		int iDeadRecFrame = DeadReckoningPos(pCharacter->dwAction, pCharacter->dwActionTick,
			pCharacter->shActionX, pCharacter->shActionY, &idrX, &idrY);

		// 차이나면 좌표 보정
		if (abs(idrX - shX) > dfERROR_RANGE || abs(idrY - shY) > dfERROR_RANGE)
		{
			makePacket_Sync(&cPacket, pSession->dwSessionID, idrX, idrY);
			SendPacket_Around(pSession, &cPacket, true);
		}

		shX = idrX;
		shY = idrY;
	}

	//현재 액션 설정
	pCharacter->dwAction = dfACTION_STAND;

	//캐릭터 방향 설정
	switch (byDirection)
	{
	case dfPACKET_MOVE_DIR_RR:
	case dfPACKET_MOVE_DIR_RU:
	case dfPACKET_MOVE_DIR_RD:
		pCharacter->byDirection = dfPACKET_MOVE_DIR_RR;
		break;

	case dfPACKET_MOVE_DIR_LL:
	case dfPACKET_MOVE_DIR_LU:
	case dfPACKET_MOVE_DIR_LD:
		pCharacter->byDirection = dfPACKET_MOVE_DIR_LL;
		break;
	}

	pCharacter->shX = shX;
	pCharacter->shY = shY;

	//섹터 업데이트
	if (Sector_UpdateCharacter(pCharacter))
		CharacterSectorUpdatePacket(pCharacter);

	//tick정보 저장
	pCharacter->dwActionTick = timeGetTime();
	pCharacter->shActionX = pCharacter->shX;
	pCharacter->shActionY = pCharacter->shY;

	makePacket_MoveStop(&cPacket, pCharacter->dwSessionID, byDirection, shX, shY);
	SendPacket_Around(pCharacter->pSession, &cPacket);

	return TRUE;
}

//---------------------------------------------------------------------------------------
// 캐릭터 공격 1
//---------------------------------------------------------------------------------------
BOOL recvProc_Attack1(st_SESSION *pSession, CNPacket *pPacket)
{
	CNPacket cPacket;

	BYTE byDirection;
	short shX;
	short shY;

	*pPacket >> byDirection;
	*pPacket >> shX;
	*pPacket >> shY;

	_LOG(dfLOG_LEVEL_DEBUG, L"Attack1 - [SessionID:%d][Direction:%d][X:%d][Y:%d]",
		pSession->dwSessionID, byDirection, shX, shY);

	st_CHARACTER *pCharacter = FindCharacter(pSession->dwSessionID);

	if (NULL == pCharacter)
	{
		_LOG(dfLOG_LEVEL_ERROR, L"Attack1 > SessionID:%d Character Not Found!\n",
			pSession->dwSessionID);
		return FALSE;
	}

	//현재 액션 설정
	pCharacter->dwAction = dfACTION_ATTACK1;

	//캐릭터 방향 설정
	switch (byDirection)
	{
	case dfPACKET_MOVE_DIR_RR:
	case dfPACKET_MOVE_DIR_RU:
	case dfPACKET_MOVE_DIR_RD:
		pCharacter->byDirection = dfPACKET_MOVE_DIR_RR;
		break;

	case dfPACKET_MOVE_DIR_LL:
	case dfPACKET_MOVE_DIR_LU:
	case dfPACKET_MOVE_DIR_LD:
		pCharacter->byDirection = dfPACKET_MOVE_DIR_LL;
		break;
	}

	//충돌처리

	//tick정보 저장
	pCharacter->dwActionTick = timeGetTime();
	pCharacter->shActionX = pCharacter->shX;
	pCharacter->shActionY = pCharacter->shY;

	makePacket_Attack1(&cPacket, pCharacter->dwSessionID, byDirection, pCharacter->shX, pCharacter->shY);
	SendPacket_Around(pCharacter->pSession, &cPacket);

	return TRUE;
}

//---------------------------------------------------------------------------------------
// 캐릭터 공격 2
//---------------------------------------------------------------------------------------
BOOL recvProc_Attack2(st_SESSION *pSession, CNPacket *pPacket)
{
	CNPacket cPacket;

	BYTE byDirection;
	short shX;
	short shY;

	*pPacket >> byDirection;
	*pPacket >> shX;
	*pPacket >> shY;

	_LOG(dfLOG_LEVEL_DEBUG, L"Attack2 - [SessionID:%d][Direction:%d][X:%d][Y:%d]",
		pSession->dwSessionID, byDirection, shX, shY);

	st_CHARACTER *pCharacter = FindCharacter(pSession->dwSessionID);

	if (NULL == pCharacter)
	{
		_LOG(dfLOG_LEVEL_ERROR, L"Attack2 > SessionID:%d Character Not Found!\n",
			pSession->dwSessionID);
		return FALSE;
	}

	//현재 액션 설정
	pCharacter->dwAction = dfACTION_ATTACK2;

	//캐릭터 방향 설정
	switch (byDirection)
	{
	case dfPACKET_MOVE_DIR_RR:
	case dfPACKET_MOVE_DIR_RU:
	case dfPACKET_MOVE_DIR_RD:
		pCharacter->byDirection = dfPACKET_MOVE_DIR_RR;
		break;

	case dfPACKET_MOVE_DIR_LL:
	case dfPACKET_MOVE_DIR_LU:
	case dfPACKET_MOVE_DIR_LD:
		pCharacter->byDirection = dfPACKET_MOVE_DIR_LL;
		break;
	}

	//충돌처리

	//tick정보 저장
	pCharacter->dwActionTick = timeGetTime();
	pCharacter->shActionX = pCharacter->shX;
	pCharacter->shActionY = pCharacter->shY;

	makePacket_Attack2(&cPacket, pCharacter->dwSessionID, byDirection, pCharacter->shX, pCharacter->shY);
	SendPacket_Around(pCharacter->pSession, &cPacket);

	return TRUE;
}

//---------------------------------------------------------------------------------------
// 캐릭터 공격 3
//---------------------------------------------------------------------------------------
BOOL recvProc_Attack3(st_SESSION *pSession, CNPacket *pPacket)
{
	CNPacket cPacket;

	BYTE byDirection;
	short shX;
	short shY;

	*pPacket >> byDirection;
	*pPacket >> shX;
	*pPacket >> shY;

	_LOG(dfLOG_LEVEL_DEBUG, L"Attack3 - [SessionID:%d][Direction:%d][X:%d][Y:%d]",
		pSession->dwSessionID, byDirection, shX, shY);

	st_CHARACTER *pCharacter = FindCharacter(pSession->dwSessionID);

	if (NULL == pCharacter)
	{
		_LOG(dfLOG_LEVEL_ERROR, L"Attack3 > SessionID:%d Character Not Found!\n",
			pSession->dwSessionID);
		return FALSE;
	}

	//현재 액션 설정
	pCharacter->dwAction = dfACTION_ATTACK3;

	//캐릭터 방향 설정
	switch (byDirection)
	{
	case dfPACKET_MOVE_DIR_RR:
	case dfPACKET_MOVE_DIR_RU:
	case dfPACKET_MOVE_DIR_RD:
		pCharacter->byDirection = dfPACKET_MOVE_DIR_RR;
		break;

	case dfPACKET_MOVE_DIR_LL:
	case dfPACKET_MOVE_DIR_LU:
	case dfPACKET_MOVE_DIR_LD:
		pCharacter->byDirection = dfPACKET_MOVE_DIR_LL;
		break;
	}

	//충돌처리

	//tick정보 저장
	pCharacter->dwActionTick = timeGetTime();
	pCharacter->shActionX = pCharacter->shX;
	pCharacter->shActionY = pCharacter->shY;

	makePacket_Attack3(&cPacket, pCharacter->dwSessionID, byDirection, pCharacter->shX, pCharacter->shY);
	SendPacket_Around(pCharacter->pSession, &cPacket);

	return TRUE;
}

/*-------------------------------------------------------------------------------------*/


/*-------------------------------------------------------------------------------------*/
// Make Packet

//---------------------------------------------------------------------------------------
// 자신의 캐릭터 생성 패킷
//---------------------------------------------------------------------------------------
void makePacket_CreateMyCharacter(CNPacket *pPacket, DWORD ID, BYTE byDirection, short shX,
	short shY, BYTE chHP)
{
	st_NETWORK_PACKET_HEADER header;

	header.byCode = dfNETWORK_PACKET_CODE;
	header.byType = dfPACKET_SC_CREATE_MY_CHARACTER;
	header.bySize = 10;

	pPacket->Clear();
	pPacket->Put((char *)&header, sizeof(header));

	*pPacket << (unsigned int)ID;
	*pPacket << (BYTE)byDirection;
	*pPacket << (short)shX;
	*pPacket << (short)shY;
	*pPacket << (BYTE)chHP;

	*pPacket << dfNETWORK_PACKET_END;
}

//---------------------------------------------------------------------------------------
// 다른 캐릭터 생성 패킷
//---------------------------------------------------------------------------------------
void makePacket_CreateOtherCharacter(CNPacket *pPacket, DWORD ID, BYTE byDirection, short shX,
	short shY, BYTE chHP)
{
	st_NETWORK_PACKET_HEADER header;

	header.byCode = dfNETWORK_PACKET_CODE;
	header.byType = dfPACKET_SC_CREATE_OTHER_CHARACTER;
	header.bySize = 10;

	pPacket->Clear();
	pPacket->Put((char *)&header, sizeof(header));

	*pPacket << (unsigned int)ID;
	*pPacket << (BYTE)byDirection;
	*pPacket << (short)shX;
	*pPacket << (short)shY;
	*pPacket << (BYTE)chHP;

	*pPacket << dfNETWORK_PACKET_END;
}

//---------------------------------------------------------------------------------------
// 캐릭터 삭제 패킷
//---------------------------------------------------------------------------------------
void makePacket_DeleteCharacter(CNPacket *pPacket, DWORD ID)
{
	st_NETWORK_PACKET_HEADER header;

	header.byCode = dfNETWORK_PACKET_CODE;
	header.byType = dfPACKET_SC_DELETE_CHARACTER;
	header.bySize = 4;

	pPacket->Clear();
	pPacket->Put((char *)&header, sizeof(header));

	*pPacket << (unsigned int)ID;

	*pPacket << dfNETWORK_PACKET_END;
}

//---------------------------------------------------------------------------------------
// 캐릭터 이동 시작 패킷
//---------------------------------------------------------------------------------------
void makePacket_MoveStart(CNPacket *pPacket, DWORD ID, BYTE byMoveDirection, short shX, short shY)
{
	st_NETWORK_PACKET_HEADER header;

	header.byCode = dfNETWORK_PACKET_CODE;
	header.byType = dfPACKET_SC_MOVE_START;
	header.bySize = 9;

	pPacket->Clear();
	pPacket->Put((char *)&header, sizeof(header));

	*pPacket << (unsigned int)ID;
	*pPacket << (BYTE)byMoveDirection;
	*pPacket << (short)shX;
	*pPacket << (short)shY;

	*pPacket << dfNETWORK_PACKET_END;
}

//---------------------------------------------------------------------------------------
// 캐릭터 이동 중지 패킷
//---------------------------------------------------------------------------------------
void makePacket_MoveStop(CNPacket *pPacket, DWORD ID, BYTE byDirection, short shX, short shY)
{
	st_NETWORK_PACKET_HEADER header;

	header.byCode = dfNETWORK_PACKET_CODE;
	header.byType = dfPACKET_SC_MOVE_STOP;
	header.bySize = 4;

	pPacket->Clear();
	pPacket->Put((char *)&header, sizeof(header));

	*pPacket << (unsigned int)ID;
	*pPacket << (BYTE)byDirection;
	*pPacket << (short)shX;
	*pPacket << (short)shY;

	*pPacket << dfNETWORK_PACKET_END;
}

//---------------------------------------------------------------------------------------
// 캐릭터 공격1 패킷
//---------------------------------------------------------------------------------------
void makePacket_Attack1(CNPacket *pPacket, DWORD ID, BYTE byDirection, short shX, short shY)
{
	st_NETWORK_PACKET_HEADER header;

	header.byCode = dfNETWORK_PACKET_CODE;
	header.byType = dfPACKET_SC_ATTACK1;
	header.bySize = 9;

	pPacket->Clear();
	pPacket->Put((char *)&header, sizeof(header));

	*pPacket << (unsigned int)ID;
	*pPacket << (BYTE)byDirection;
	*pPacket << (short)shX;
	*pPacket << (short)shY;

	*pPacket << dfNETWORK_PACKET_END;
}

//---------------------------------------------------------------------------------------
// 캐릭터 공격2 패킷
//---------------------------------------------------------------------------------------
void makePacket_Attack2(CNPacket *pPacket, DWORD ID, BYTE byDirection, short shX, short shY)
{
	st_NETWORK_PACKET_HEADER header;

	header.byCode = dfNETWORK_PACKET_CODE;
	header.byType = dfPACKET_SC_ATTACK2;
	header.bySize = 9;

	pPacket->Clear();
	pPacket->Put((char *)&header, sizeof(header));

	*pPacket << (unsigned int)ID;
	*pPacket << (BYTE)byDirection;
	*pPacket << (short)shX;
	*pPacket << (short)shY;

	*pPacket << dfNETWORK_PACKET_END;
}

//---------------------------------------------------------------------------------------
// 캐릭터 공격3 패킷
//---------------------------------------------------------------------------------------
void makePacket_Attack3(CNPacket *pPacket, DWORD ID, BYTE byDirection, short shX, short shY)
{
	st_NETWORK_PACKET_HEADER header;

	header.byCode = dfNETWORK_PACKET_CODE;
	header.byType = dfPACKET_SC_ATTACK3;
	header.bySize = 9;

	pPacket->Clear();
	pPacket->Put((char *)&header, sizeof(header));

	*pPacket << (unsigned int)ID;
	*pPacket << (BYTE)byDirection;
	*pPacket << (short)shX;
	*pPacket << (short)shY;

	*pPacket << dfNETWORK_PACKET_END;
}

//---------------------------------------------------------------------------------------
// 데미지 패킷
//---------------------------------------------------------------------------------------
void makePacket_Damage(CNPacket *pPacket, DWORD AttackID, DWORD DamageID, BYTE DamageHP)
{
	st_NETWORK_PACKET_HEADER header;

	header.byCode = dfNETWORK_PACKET_CODE;
	header.byType = dfPACKET_SC_DAMAGE;
	header.bySize = 9;

	pPacket->Clear();
	pPacket->Put((char *)&header, sizeof(header));

	*pPacket << (unsigned int)AttackID;
	*pPacket << (unsigned int)DamageID;
	*pPacket << DamageHP;

	*pPacket << dfNETWORK_PACKET_END;
}

//---------------------------------------------------------------------------------------
// Sync 패킷
//---------------------------------------------------------------------------------------
void makePacket_Sync(CNPacket *pPacket, DWORD ID, short shX, short shY)
{
	st_NETWORK_PACKET_HEADER header;

	header.byCode = dfNETWORK_PACKET_CODE;
	header.byType = dfPACKET_SC_SYNC;
	header.bySize = 8;

	pPacket->Clear();
	pPacket->Put((char *)&header, sizeof(header));
	
	*pPacket << (unsigned int)ID;
	*pPacket << shX;
	*pPacket << shY;

	*pPacket << dfNETWORK_PACKET_END;
}


/*-------------------------------------------------------------------------------------*/



/*-------------------------------------------------------------------------------------*/
// 패킷 보내는 함수

//---------------------------------------------------------------------------------------
// 해당 Sector에 보냄
//---------------------------------------------------------------------------------------
void SendPacket_SectorOne(int iSectorX, int iSectorY, CNPacket *pPacket,
	st_SESSION *pExceptSession)
{
	Sector::iterator sIter;

	for (sIter = g_Sector[iSectorY][iSectorX].begin();
		sIter != g_Sector[iSectorY][iSectorX].end(); ++sIter)
	{
		if ((*sIter)->pSession == pExceptSession)
			continue;

		SendPacket_Unicast((*sIter)->pSession, pPacket);
	}
}

//---------------------------------------------------------------------------------------
// 해당 Session에 보냄
//---------------------------------------------------------------------------------------
void SendPacket_Unicast(st_SESSION *pSession, CNPacket *pPacket)
{
	pSession->SendQ.Put((char *)pPacket->GetBufferPtr(), pPacket->GetDataSize());
}

//---------------------------------------------------------------------------------------
// 주위 Sector에 보냄
//---------------------------------------------------------------------------------------
void SendPacket_Around(st_SESSION *pSession, CNPacket *pPacket, bool bSendMe)
{
	st_CHARACTER *pCharacter = FindCharacter(pSession->dwSessionID);

	st_SECTOR_AROUND stDestSector;
	GetSectorAround(pCharacter->CurSector.iX, pCharacter->CurSector.iY, &stDestSector);

	for (int iCnt = 0; iCnt < stDestSector.iCount; iCnt++)
	{
		if (!bSendMe)
			SendPacket_SectorOne(stDestSector.Around[iCnt].iX, stDestSector.Around[iCnt].iY,
			pPacket, pSession);
		else
			SendPacket_SectorOne(stDestSector.Around[iCnt].iX, stDestSector.Around[iCnt].iY,
			pPacket, NULL);
	}
}

//---------------------------------------------------------------------------------------
// 전체 보냄
//---------------------------------------------------------------------------------------
void SendPacket_Broadcast(st_SESSION *pSession,	CNPacket *pPacket)
{
	Session::iterator sIter;

	for (sIter = g_Session.begin(); sIter != g_Session.end(); ++sIter)
	{
		SendPacket_Unicast(pSession, pPacket);
	}
}

/*-------------------------------------------------------------------------------------*/


/*-------------------------------------------------------------------------------------*/
// DeadReckoning
/*-------------------------------------------------------------------------------------*/
int DeadReckoningPos(DWORD dwAction, DWORD dwActionTick, int iOldPosX, int iOldPosY, int *pPosX, int *pPosY)
{
	// 프레임 계산
	DWORD dwIntervalTick = timeGetTime() - dwActionTick;

	int iActionFrame = dwIntervalTick / 20;

	int iRPosX = iOldPosX;
	int iRPosY = iOldPosY;

	//-----------------------------------------------------------------------------------
	// 계산 된 프레임으로 x, y축 좌표 이동값 구하기
	//-----------------------------------------------------------------------------------
	int iDX = iActionFrame * dfRECKONING_SPEED_PLAYER_X;
	int iDY = iActionFrame * dfRECKONING_SPEED_PLAYER_Y;

	switch (dwAction)
	{
	case dfACTION_MOVE_LL :
		iRPosX = iOldPosX - iDX;
		iRPosY = iOldPosY;
		break;

	case dfACTION_MOVE_LU :
		iRPosX = iOldPosX - iDX;
		iRPosY = iOldPosY - iDY;
		break;

	case dfACTION_MOVE_UU:
		iRPosX = iOldPosX;
		iRPosY = iOldPosY - iDY;
		break;

	case dfACTION_MOVE_RU:
		iRPosX = iOldPosX + iDX;
		iRPosY = iOldPosY - iDY;
		break;

	case dfACTION_MOVE_RR:
		iRPosX = iOldPosX + iDX;
		iRPosY = iOldPosY;
		break;

	case dfACTION_MOVE_RD:
		iRPosX = iOldPosX + iDX;
		iRPosY = iOldPosY + iDY;
		break;

	case dfACTION_MOVE_DD:
		iRPosX = iOldPosX;
		iRPosY = iOldPosY + iDY;
		break;

	case dfACTION_MOVE_LD:
		iRPosX = iOldPosX - iDX;
		iRPosY = iOldPosY + iDY;
		break;
	}

	// 화면 영역을 벗어났을 때 처리
	if (iRPosX <= dfRANGE_MOVE_LEFT)
	{
		iRPosX = dfRANGE_MOVE_LEFT;
		iRPosY = iRPosY - ((iRPosY - iOldPosY) / (iRPosX - iOldPosX)) * iRPosX;
	}

	if (iRPosX >= dfRANGE_MOVE_RIGHT)
	{
		iRPosX = dfRANGE_MOVE_RIGHT;
		iRPosY = iRPosY - ((iRPosY - iOldPosY) / (iRPosX - iOldPosX)) * iRPosX;
	}

	if (iRPosY <= dfRANGE_MOVE_TOP)
	{
		if (0 != (iRPosX - iOldPosX))
			iRPosX = iRPosY / ((iRPosY - iOldPosY) / (iRPosX - iOldPosX)) - iRPosX;

		else
			iRPosX = iOldPosX;

		iRPosY = dfRANGE_MOVE_TOP;
	}

	if (iRPosY >= dfRANGE_MOVE_BOTTOM)
	{
		if (0 != (iRPosX - iOldPosX))
			iRPosX = iRPosY / ((iRPosY - iOldPosY) / (iRPosX - iOldPosX)) - iRPosX;
			
		else
			iRPosX = iOldPosX;

		iRPosY = dfRANGE_MOVE_BOTTOM;
	}

	*pPosX = iRPosX;
	*pPosY = iRPosY;

	return iActionFrame;
}