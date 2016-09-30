#include "TCPFighter_serv.h"

UINT64 uiSessionCount;

/*-------------------------------------------------------------------------------------*/
// Server Setup
/*-------------------------------------------------------------------------------------*/
void netSetup()
{
	int retval;

	//-----------------------------------------------------------------------------------
	// Winsock �ʱ�ȭ
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
	// socket ���
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
					// Accept ó��
					//---------------------------------------------------------------------------
					if ((*ReadIter)->fd_array[iCnt] == g_ListenSock)
						netProc_Accept((*ReadIter)->fd_array[iCnt]);

					//---------------------------------------------------------------------------
					// Receive ó��
					//---------------------------------------------------------------------------
					else
						netProc_Recv((*ReadIter)->fd_array[iCnt]);
				}

				//-------------------------------------------------------------------------------
				// Send ó��
				//-------------------------------------------------------------------------------
				else if (FD_ISSET((*WriteIter)->fd_array[iCnt], *WriteIter))
					netProc_Send((*WriteIter)->fd_array[iCnt]);
			}
		}
	}

	//---------------------------------------------------------------------------------
	// FD_SET List ����
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
	{
		_LOG(dfLOG_LEVEL_ERROR, L"Accept Socket Error \n");
		return FALSE;
	}
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
		_LOG(dfLOG_LEVEL_ERROR, L"Send Error - SessionID : %d \n", pSession->dwSessionID);
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

	pSession->dwTrafficTick = timeGetTime();

	pSession->RecvQ.MoveWritePos(retval);

	if (retval == 0)
	{
		DisconnectSession(socket);
		return;
	}

	//-----------------------------------------------------------------------------------
	// RecvQ�� �����Ͱ� �����ִ� �� ��� ��Ŷ ó��
	//-----------------------------------------------------------------------------------
	while (pSession->RecvQ.GetUseSize() > 0)
	{
		if (retval < 0)
		{
			_LOG(dfLOG_LEVEL_ERROR, L"Recv Error - SessionID : %d \n", pSession->dwSessionID);
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
	//RecvQ �뷮�� header���� ������ �˻�
	/*--------------------------------------------------------------------------------------*/
	if (pSession->RecvQ.GetUseSize() < sizeof(st_NETWORK_PACKET_HEADER))
		return FALSE;

	pSession->RecvQ.Peek((char *)&header, sizeof(st_NETWORK_PACKET_HEADER));

	/*--------------------------------------------------------------------------------------*/
	//header + payload �뷮�� RecvQ�뷮���� ū�� �˻�
	/*--------------------------------------------------------------------------------------*/
	if (pSession->RecvQ.GetUseSize() < header.bySize + sizeof(st_NETWORK_PACKET_HEADER))
		return FALSE;

	pSession->RecvQ.RemoveData(sizeof(st_NETWORK_PACKET_HEADER));

	/*--------------------------------------------------------------------------------------*/
	//payload�� cPacket�� �̰� ������ �˻�
	/*--------------------------------------------------------------------------------------*/
	if (header.bySize !=
		pSession->RecvQ.Get((char *)cPacket.GetBufferPtr(), header.bySize))
		return FALSE;

	if (dfNETWORK_PACKET_END != pSession->RecvQ.Get((char *)byEndCode, 1))
		return FALSE;

	wprintf(L"PacketRecv [Session:%d][Type:%d]\n", pSession->dwSessionID, header.byType);

	/*--------------------------------------------------------------------------------------*/
	// Message Ÿ�Կ� ���� Packet ó��
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
		_LOG(dfLOG_LEVEL_ERROR, L"Packet Proc Error [SessionID : %d][PacketType : %d] \n", 
			pSession->dwSessionID, header.byType);
		DisconnectSession(pSession->socket);
		break;
	}

	return TRUE;
}

/*-------------------------------------------------------------------------------------*/
// ���� ã��
/*-------------------------------------------------------------------------------------*/
st_SESSION *FindSession(SOCKET socket)
{
	Session::iterator sessionIter;
	sessionIter = g_Session.find(socket);

	return sessionIter->second;
}

/*-------------------------------------------------------------------------------------*/
// ���� ����
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
// ���� ����
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
// Client -> Server ��Ŷó��

//---------------------------------------------------------------------------------------
// ĳ���� �̵� ����
//---------------------------------------------------------------------------------------
BOOL recvProc_MoveStart(st_SESSION *pSession, CNPacket *pPacket)
{
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
		_LOG(dfLOG_LEVEL_ERROR, L"# MOVESTART > SessionID:%d Character Not Found!\n", 
			pSession->dwSessionID);
		return FALSE;
	}

	//-----------------------------------------------------------------------------------
	// ���� ��ǥ�� ���� ��ǥ�� �ʹ� �ٸ��� ��ǥ ����
	//-----------------------------------------------------------------------------------
	if (abs(pCharacter->shX - shX) > dfERROR_RANGE || abs(pCharacter->shY - shY) > dfERROR_RANGE)
	{
		int idrX, idrY;
		// DeadReckoning���� ��ġ Ȯ��
		int iDeadRecFrame = DeadReckoningPos(pCharacter->dwAction, pCharacter->dwActionTick,
			pCharacter->shActionX, pCharacter->shActionY, &idrX, &idrY);

		// ���̳��� ��ǥ ����
		if (abs(idrX - shX) > dfERROR_RANGE || abs(idrY - shY) > dfERROR_RANGE)
		{
			//Sync Packet ����
		}

		shX = idrX;
		shY = idrY;
	}

	//���� �׼� ����
	pCharacter->dwAction = byDirection;

	//���� �̵����� ����
	pCharacter->byMoveDirection = byDirection;

	//ĳ���� ���� ����
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

	//���� ������Ʈ

	//tick���� ����
	pCharacter->dwActionTick = timeGetTime();
	pCharacter->shActionX = pCharacter->shX;
	pCharacter->shActionY = pCharacter->shY;

	sendProc_MoveStart(pCharacter);
	return TRUE;
}

//---------------------------------------------------------------------------------------
// ĳ���� �̵� ����
//---------------------------------------------------------------------------------------
BOOL recvProc_MoveStop(st_SESSION *pSession, CNPacket *pPacket)
{

}

//---------------------------------------------------------------------------------------
// ĳ���� ���� 1
//---------------------------------------------------------------------------------------
BOOL recvProc_Attack1(st_SESSION *pSession, CNPacket *pPacket)
{

}

//---------------------------------------------------------------------------------------
// ĳ���� ���� 2
//---------------------------------------------------------------------------------------
BOOL recvProc_Attack2(st_SESSION *pSession, CNPacket *pPacket)
{

}

//---------------------------------------------------------------------------------------
// ĳ���� ���� 3
//---------------------------------------------------------------------------------------
BOOL recvProc_Attack3(st_SESSION *pSession, CNPacket *pPacket)
{

}

/*-------------------------------------------------------------------------------------*/


/*-------------------------------------------------------------------------------------*/
// Send Packet

//---------------------------------------------------------------------------------------
// �ڽ��� ĳ���� �Ҵ�
//---------------------------------------------------------------------------------------
BOOL sendProc_CreateMyCharacter(st_SESSION *pSession, DWORD ID)
{
	st_NETWORK_PACKET_HEADER header;
	CNPacket cPacket;

	makePacket_CreateMyCharacter(&header, &cPacket, ID);

	SendPacket_Unicast(pSession, &header, &cPacket);
}

//---------------------------------------------------------------------------------------
// �ٸ� Ŭ���̾�Ʈ�� ĳ���� ����
//---------------------------------------------------------------------------------------
BOOL sendProc_CreateOtherCharacter()
{

}

//---------------------------------------------------------------------------------------
// ĳ���� ����
//---------------------------------------------------------------------------------------
BOOL sendProc_DeleteCharacter()
{

}

//---------------------------------------------------------------------------------------
// ĳ���� �̵�����
//---------------------------------------------------------------------------------------
BOOL sendProc_MoveStart(st_CHARACTER *pCharacter)
{
	st_NETWORK_PACKET_HEADER header;
	CNPacket cPacket;

	makePacket_MoveStart(&header, &cPacket, pCharacter);

	SendPacket_Around(pCharacter->pSession, &header, &cPacket);
}

//---------------------------------------------------------------------------------------
// ĳ���� �̵�����
//---------------------------------------------------------------------------------------
BOOL sendProc_MoveStop()
{

}

//---------------------------------------------------------------------------------------
// ĳ���� ���� 1
//---------------------------------------------------------------------------------------
BOOL sendProc_Attack1()
{

}

//---------------------------------------------------------------------------------------
// ĳ���� ���� 2
//---------------------------------------------------------------------------------------
BOOL sendProc_Attack2()
{

}

//---------------------------------------------------------------------------------------
// ĳ���� ���� 3
//---------------------------------------------------------------------------------------
BOOL sendProc_Attack3()
{

}

//---------------------------------------------------------------------------------------
// ĳ���� ������
//---------------------------------------------------------------------------------------
BOOL sendProc_Damage()
{

}

//---------------------------------------------------------------------------------------
// ����ȭ
//---------------------------------------------------------------------------------------
BOOL sendProc_Sync()
{

}

/*-------------------------------------------------------------------------------------*/


/*-------------------------------------------------------------------------------------*/
// Make Packet

//---------------------------------------------------------------------------------------
// ĳ���� �̵� ���� ��Ŷ
//---------------------------------------------------------------------------------------
void makePacket_CreateMyCharacter(st_NETWORK_PACKET_HEADER *pHeader, CNPacket *pPacket, DWORD ID)
{
	int len;

	*pPacket << (unsigned int)ID;
	//����
	*pPacket << (short)(rand() % 6400);
	*pPacket << (short)(rand() % 6400);
	*pPacket << (BYTE)100;

	len = pPacket->GetDataSize();
	*pPacket << dfNETWORK_PACKET_END;

	pHeader->byCode = dfNETWORK_PACKET_CODE;
	pHeader->bySize = len;
	pHeader->byType = dfPACKET_SC_CREATE_MY_CHARACTER;
}

//---------------------------------------------------------------------------------------
// ĳ���� �̵� ���� ��Ŷ
//---------------------------------------------------------------------------------------
void makePacket_CreateOtherCharacter()
{

}

//---------------------------------------------------------------------------------------
// ĳ���� �̵� ���� ��Ŷ
//---------------------------------------------------------------------------------------
void makePacket_DeleteCharacter()
{

}

//---------------------------------------------------------------------------------------
// ĳ���� �̵� ���� ��Ŷ
//---------------------------------------------------------------------------------------
void makePacket_MoveStart(st_NETWORK_PACKET_HEADER *pHeader, CNPacket *pPacket, st_CHARACTER *pCharacter)
{
	int len;

	*pPacket << (unsigned int)pCharacter->dwSessionID;
	*pPacket << pCharacter->byMoveDirection;
	*pPacket << pCharacter->shX;
	*pPacket << pCharacter->shY;

	len = pPacket->GetDataSize();
	*pPacket << dfNETWORK_PACKET_END;

	pHeader->byCode = dfNETWORK_PACKET_CODE;
	pHeader->bySize = len;
	pHeader->byType = dfPACKET_SC_MOVE_START;
}

//---------------------------------------------------------------------------------------
// ĳ���� �̵� ���� ��Ŷ
//---------------------------------------------------------------------------------------
void makePacket_MoveStop()
{

}

//---------------------------------------------------------------------------------------
// ĳ���� �̵� ���� ��Ŷ
//---------------------------------------------------------------------------------------
void makePacket_Attack1()
{

}

//---------------------------------------------------------------------------------------
// ĳ���� �̵� ���� ��Ŷ
//---------------------------------------------------------------------------------------
void makePacket_Attack2()
{

}

//---------------------------------------------------------------------------------------
// ĳ���� �̵� ���� ��Ŷ
//---------------------------------------------------------------------------------------
void makePacket_Attack3()
{

}

//---------------------------------------------------------------------------------------
// ĳ���� �̵� ���� ��Ŷ
//---------------------------------------------------------------------------------------
void makePacket_Damage()
{

}

//---------------------------------------------------------------------------------------
// ĳ���� �̵� ���� ��Ŷ
//---------------------------------------------------------------------------------------
void makePacket_Sync()
{

}


/*-------------------------------------------------------------------------------------*/



/*-------------------------------------------------------------------------------------*/
// ��Ŷ ������ �Լ�

//---------------------------------------------------------------------------------------
// �ش� Sector�� ����
//---------------------------------------------------------------------------------------
void SendPacket_SectorOne(int iSectorX, int iSectorY, st_NETWORK_PACKET_HEADER *pHeader
	,CNPacket *pPacket, st_SESSION *pExceptSession)
{

}

//---------------------------------------------------------------------------------------
// �ش� Session�� ����
//---------------------------------------------------------------------------------------
void SendPacket_Unicast(st_SESSION *pSession, st_NETWORK_PACKET_HEADER *pHeader, 
	CNPacket *pPacket)
{
	pSession->SendQ.Put((char *)pHeader, sizeof(st_NETWORK_PACKET_HEADER));
	pSession->SendQ.Put((char *)pPacket, pPacket->GetDataSize());
}

//---------------------------------------------------------------------------------------
// ���� Sector�� ����
//---------------------------------------------------------------------------------------
void SendPacket_Around(st_SESSION *pSession, st_NETWORK_PACKET_HEADER *pHeader, 
	CNPacket *pPacket, bool bSendMe = false)
{
	
}

//---------------------------------------------------------------------------------------
// ��ü ����
//---------------------------------------------------------------------------------------
void Sendpacket_Broadcast(st_SESSION *pSession, st_NETWORK_PACKET_HEADER *pHeader, 
	CNPacket *pPacket)
{

}

/*-------------------------------------------------------------------------------------*/


/*-------------------------------------------------------------------------------------*/
// DeadReckoning
/*-------------------------------------------------------------------------------------*/
int DeadReckoningPos(DWORD dwAction, DWORD dwActionTick, int iOldPosX, int iOldPosY, int *pPosX, int *pPosY)
{
	// ������ ���
	DWORD dwIntervalTick = timeGetTime() - dwActionTick;

	int iActionFrame = dwIntervalTick / 20;

	int iRPosX = iOldPosX;
	int iRPosY = iOldPosY;

	//-----------------------------------------------------------------------------------
	// ��� �� ���������� x, y�� ��ǥ �̵��� ���ϱ�
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

	// ȭ�� ������ ����� �� ó��
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