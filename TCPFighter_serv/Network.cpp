#include "TCPFighter_serv.h"
#include "PacketDefine.h"
#include "StreamQueue.h"
#include "NPacket.h"
#include "Network.h"

UINT64 uiSessionCount;

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
			for (int iCnt = 0; iCnt < FD_SETSIZE; iCnt++){
				if (FD_ISSET((*ReadIter)->fd_array[iCnt], *ReadIter))
				{
					//---------------------------------------------------------------------------
					// Accept 처리
					//---------------------------------------------------------------------------
					if ((*ReadIter)->fd_array[iCnt] == g_ListenSock)
						netProc_Accept((*ReadIter)->fd_array[iCnt]);

					//---------------------------------------------------------------------------
					// Receive 처리
					//---------------------------------------------------------------------------
					else
						netProc_Recv((*ReadIter)->fd_array[iCnt]);
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

	sessionSock = accept(socket, (SOCKADDR *)&sockaddr, &addrlen);
	if (sessionSock == INVALID_SOCKET)
		return FALSE;

	st_SESSION *pSession = CreateSession(sessionSock);

	g_Session.insert(pair<SOCKET, st_SESSION *>(sessionSock, pSession));

	InetNtop(AF_INET, &sockaddr.sin_addr, wcAddr, sizeof(wcAddr));

	wprintf(L"Accept - %s:%d Socket : %d \n", wcAddr, ntohs(sockaddr.sin_port),
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
	}

	if (retval == 0)
		return FALSE;

	else if (retval < 0)
	{
		DisconnectSession(socket);
		return FALSE;
	}
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
			//error
			DisconnectSession(socket);
			return;
		}

		else
			PacketProc(pSession);
	}
}

BOOL PacketProc(st_SESSION *pSession)
{
	st_NETWORK_PACKET_HEADER header;
	CNPacket cPacket;
	BYTE byEndCode;

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

	if (dfNETWORK_PACKET_END != pSession->RecvQ.Get((char *)byEndCode, 1))
		return FALSE;

	wprintf(L"PacketRecv [Session:%d][Type:%d]\n", pSession->dwSessionID, header.byType);

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
	
	shutdown(socket, SD_BOTH);
	delete sessionIter->second;
	g_Session.erase(sessionIter);
}

/*-------------------------------------------------------------------------------------*/
// Client -> Server 패킷처리

//---------------------------------------------------------------------------------------
// 캐릭터 이동 시작
//---------------------------------------------------------------------------------------
BOOL recvProc_MoveStart(st_SESSION *pSession, CNPacket *cPacket)
{

}

//---------------------------------------------------------------------------------------
// 캐릭터 이동 중지
//---------------------------------------------------------------------------------------
BOOL recvProc_MoveStop(st_SESSION *pSession, CNPacket *cPacket)
{

}

//---------------------------------------------------------------------------------------
// 캐릭터 공격 1
//---------------------------------------------------------------------------------------
BOOL recvProc_Attack1(st_SESSION *pSession, CNPacket *cPacket)
{

}

//---------------------------------------------------------------------------------------
// 캐릭터 공격 2
//---------------------------------------------------------------------------------------
BOOL recvProc_Attack2(st_SESSION *pSession, CNPacket *cPacket)
{

}

//---------------------------------------------------------------------------------------
// 캐릭터 공격 3
//---------------------------------------------------------------------------------------
BOOL recvProc_Attack3(st_SESSION *pSession, CNPacket *cPacket)
{

}

/*-------------------------------------------------------------------------------------*/


/*-------------------------------------------------------------------------------------*/
// Send Packet

//---------------------------------------------------------------------------------------
// 자신의 캐릭터 할당
//---------------------------------------------------------------------------------------
BOOL sendProc_CreateMyCharacter()
{

}

//---------------------------------------------------------------------------------------
// 다른 클라이언트의 캐릭터 생성
//---------------------------------------------------------------------------------------
BOOL sendProc_CreateOtherCharacter()
{

}

//---------------------------------------------------------------------------------------
// 캐릭터 삭제
//---------------------------------------------------------------------------------------
BOOL sendProc_DeleteCharacter()
{

}

//---------------------------------------------------------------------------------------
// 캐릭터 이동시작
//---------------------------------------------------------------------------------------
BOOL sendProc_MoveStart()
{

}

//---------------------------------------------------------------------------------------
// 캐릭터 이동중지
//---------------------------------------------------------------------------------------
BOOL sendProc_MoveStop()
{

}

//---------------------------------------------------------------------------------------
// 캐릭터 공격 1
//---------------------------------------------------------------------------------------
BOOL sendProc_Attack1()
{

}

//---------------------------------------------------------------------------------------
// 캐릭터 공격 2
//---------------------------------------------------------------------------------------
BOOL sendProc_Attack2()
{

}

//---------------------------------------------------------------------------------------
// 캐릭터 공격 3
//---------------------------------------------------------------------------------------
BOOL sendProc_Attack3()
{

}

//---------------------------------------------------------------------------------------
// 캐릭터 데미지
//---------------------------------------------------------------------------------------
BOOL sendProc_Damage()
{

}

//---------------------------------------------------------------------------------------
// 동기화
//---------------------------------------------------------------------------------------
BOOL sendProc_Sync()
{

}

/*-------------------------------------------------------------------------------------*/


/*-------------------------------------------------------------------------------------*/
// Make Packet

//---------------------------------------------------------------------------------------
// 캐릭터 이동 시작 패킷
//---------------------------------------------------------------------------------------
void makePacket_CreateMyCharacter();

//---------------------------------------------------------------------------------------
// 캐릭터 이동 시작 패킷
//---------------------------------------------------------------------------------------
void makePacket_CreateOtherCharacter();

//---------------------------------------------------------------------------------------
// 캐릭터 이동 시작 패킷
//---------------------------------------------------------------------------------------
void makePacket_DeleteCharacter();

//---------------------------------------------------------------------------------------
// 캐릭터 이동 시작 패킷
//---------------------------------------------------------------------------------------
void makePacket_MoveStart();

//---------------------------------------------------------------------------------------
// 캐릭터 이동 시작 패킷
//---------------------------------------------------------------------------------------
void makePacket_MoveStop();

//---------------------------------------------------------------------------------------
// 캐릭터 이동 시작 패킷
//---------------------------------------------------------------------------------------
void makePacket_Attack1();

//---------------------------------------------------------------------------------------
// 캐릭터 이동 시작 패킷
//---------------------------------------------------------------------------------------
void makePacket_Attack2();

//---------------------------------------------------------------------------------------
// 캐릭터 이동 시작 패킷
//---------------------------------------------------------------------------------------
void makePacket_Attack3();

//---------------------------------------------------------------------------------------
// 캐릭터 이동 시작 패킷
//---------------------------------------------------------------------------------------
void makePacket_Damage();

//---------------------------------------------------------------------------------------
// 캐릭터 이동 시작 패킷
//---------------------------------------------------------------------------------------
void makePacket_Sync();


/*-------------------------------------------------------------------------------------*/
