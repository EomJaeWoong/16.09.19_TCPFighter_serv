#include <WinSock2.h>
#include <list>
#include <map>

using namespace std;

#pragma comment(lib, "winmm.lib")

#include "ObjectType.h"
#include "StreamQueue.h"
#include "NPacket.h"
#include "Session.h"
#include "SectorDef.h"
#include "Character.h"
#include "Sector.h"
#include "Network.h"
#include "Content.h"
#include "Log.h"

Character		g_CharacterMap;
DWORD			g_dwUpdateTick;							//업데이트 시간
DWORD			g_dwTick;

/*-------------------------------------------------------------------------------------*/
// Server Setup
/*-------------------------------------------------------------------------------------*/
void DataSetup()
{
	g_dwUpdateTick = 0;
	g_dwTick = 0;
	g_dwLoopTime = timeGetTime();
	g_iLogLevel = dfLOG_LEVEL_DEBUG;
}



/*-------------------------------------------------------------------------------------*/
// 게임 로직 처리
/*-------------------------------------------------------------------------------------*/
void Update()
{
	//----------------------------------------------------------------------------------
	// 게임 업데이트 처리 타이밍 계산
	//----------------------------------------------------------------------------------
	if (g_dwUpdateTick == 0)
		g_dwUpdateTick = GetTickCount();

	DWORD dwOneFrameTick = GetTickCount() - g_dwUpdateTick;

	if ((dwOneFrameTick + g_dwTick) < 1000 / dfSERVER_FRAME)
		return;

	else
	{
		g_dwTick = dwOneFrameTick + g_dwTick - 1000 / dfSERVER_FRAME;
		g_dwUpdateTick = GetTickCount();
		//wprintf(L"dwOneFrameTick : %d, g_dwUpdateTick : %d, g_dwTick : %d\n",
		//	dwOneFrameTick, g_dwUpdateTick, g_dwTick);
	}

	//----------------------------------------------------------------------------------
	// 플레이어 로직 처리
	//----------------------------------------------------------------------------------
	Character::iterator cIter;
	st_CHARACTER *pCharacter = NULL;

	for (cIter = g_CharacterMap.begin(); cIter != g_CharacterMap.end();)
	{
		pCharacter = cIter->second;
		cIter++;

		//------------------------------------------------------------------------------
		// 캐릭터 사망
		//------------------------------------------------------------------------------
		if (0 >= pCharacter->chHP)
		{
			CNPacket cPacket;

			makePacket_DeleteCharacter(&cPacket, pCharacter->dwSessionID);
			SendPacket_Around(pCharacter->pSession, &cPacket);

			DeleteCharacter(pCharacter->dwSessionID);

			DisconnectSession(pCharacter->pSession->socket);
		}

		else
		{
			/*
			//일정시간 패킷 안오면 끊기
			if (timeGetTime() - pCharacter->pSession->dwTrafficTick > dfNETWORK_PACKET_RECV_TIMEOUT)
			{
				DisconnectSession(pCharacter->pSession->socket);
				continue;
			}
			*/

			switch (pCharacter->dwAction)
			{
				
				case dfACTION_MOVE_LL :
					if (pCharacter->shX - dfSPEED_PLAYER_X > dfRANGE_MOVE_LEFT)
						pCharacter->shX -= dfSPEED_PLAYER_X;
					_LOG(dfLOG_LEVEL_DEBUG, L"Update - Move LL [X:%d][Y:%d][SectorX:%d][SectorY:%d][Session:%d]",
						pCharacter->shX, pCharacter->shY, pCharacter->CurSector.iX, pCharacter->CurSector.iY,
						pCharacter->dwSessionID);
					break;

				case dfACTION_MOVE_LU :
					if ((pCharacter->shX - dfSPEED_PLAYER_X > dfRANGE_MOVE_LEFT) &&
						(pCharacter->shY - dfSPEED_PLAYER_Y > dfRANGE_MOVE_TOP))
					{
						pCharacter->shX -= dfSPEED_PLAYER_X;
						pCharacter->shY -= dfSPEED_PLAYER_Y;
					}
					_LOG(dfLOG_LEVEL_DEBUG, L"Update - Move LU [X:%d][Y:%d][SectorX:%d][SectorY:%d][Session:%d]",
						pCharacter->shX, pCharacter->shY, pCharacter->CurSector.iX, pCharacter->CurSector.iY,
						pCharacter->dwSessionID);
					break;

				case dfACTION_MOVE_UU :
					if (pCharacter->shY - dfSPEED_PLAYER_Y > dfRANGE_MOVE_TOP)
						pCharacter->shY -= dfSPEED_PLAYER_Y;
					_LOG(dfLOG_LEVEL_DEBUG, L"Update - Move UU [X:%d][Y:%d][SectorX:%d][SectorY:%d][Session:%d]",
						pCharacter->shX, pCharacter->shY, pCharacter->CurSector.iX, pCharacter->CurSector.iY,
						pCharacter->dwSessionID);
					break;

				case dfACTION_MOVE_RU :
					if ((pCharacter->shX + dfSPEED_PLAYER_X < dfRANGE_MOVE_RIGHT) &&
						(pCharacter->shY - dfSPEED_PLAYER_Y > dfRANGE_MOVE_TOP))
					{
						pCharacter->shX += dfSPEED_PLAYER_X;
						pCharacter->shY -= dfSPEED_PLAYER_Y;
					}
					_LOG(dfLOG_LEVEL_DEBUG, L"Update - Move RU [X:%d][Y:%d][SectorX:%d][SectorY:%d][Session:%d]",
						pCharacter->shX, pCharacter->shY, pCharacter->CurSector.iX, pCharacter->CurSector.iY,
						pCharacter->dwSessionID);
					break;

				case dfACTION_MOVE_RR :
					if (pCharacter->shX + dfSPEED_PLAYER_X < dfRANGE_MOVE_RIGHT)
						pCharacter->shX += dfSPEED_PLAYER_X;
					_LOG(dfLOG_LEVEL_DEBUG, L"Update - Move RR [X:%d][Y:%d][SectorX:%d][SectorY:%d][Session:%d]",
						pCharacter->shX, pCharacter->shY, pCharacter->CurSector.iX, pCharacter->CurSector.iY,
						pCharacter->dwSessionID);
					break;

				case dfACTION_MOVE_RD :
					if ((pCharacter->shX + dfSPEED_PLAYER_X < dfRANGE_MOVE_RIGHT) &&
						(pCharacter->shY + dfSPEED_PLAYER_Y < dfRANGE_MOVE_BOTTOM))
					{
						pCharacter->shX += dfSPEED_PLAYER_X;
						pCharacter->shY += dfSPEED_PLAYER_Y;
					}
					_LOG(dfLOG_LEVEL_DEBUG, L"Update - Move RD [X:%d][Y:%d][SectorX:%d][SectorY:%d][Session:%d]",
						pCharacter->shX, pCharacter->shY, pCharacter->CurSector.iX, pCharacter->CurSector.iY,
						pCharacter->dwSessionID);
					break;

				case dfACTION_MOVE_DD :
					if (pCharacter->shY + dfSPEED_PLAYER_Y < dfRANGE_MOVE_BOTTOM)
						pCharacter->shY += dfSPEED_PLAYER_Y;
					_LOG(dfLOG_LEVEL_DEBUG, L"Update - Move DD [X:%d][Y:%d][SectorX:%d][SectorY:%d][Session:%d]",
						pCharacter->shX, pCharacter->shY, pCharacter->CurSector.iX, pCharacter->CurSector.iY,
						pCharacter->dwSessionID);
					break;

				case dfACTION_MOVE_LD :
					if ((pCharacter->shX - dfSPEED_PLAYER_X > dfRANGE_MOVE_LEFT) &&
						(pCharacter->shY + dfSPEED_PLAYER_Y < dfRANGE_MOVE_BOTTOM))
					{
						pCharacter->shX -= dfSPEED_PLAYER_X;
						pCharacter->shY += dfSPEED_PLAYER_Y;
					}
					_LOG(dfLOG_LEVEL_DEBUG, L"Update - Move LD [X:%d][Y:%d][SectorX:%d][SectorY:%d][Session:%d]",
						pCharacter->shX, pCharacter->shY, pCharacter->CurSector.iX, pCharacter->CurSector.iY,
						pCharacter->dwSessionID);
					break;
			}

			//이동할때 섹터 업데이트
			if (pCharacter->dwAction >= dfACTION_MOVE_LL && pCharacter->dwAction <= dfACTION_MOVE_LD)
			{
				if (Sector_UpdateCharacter(pCharacter))
				{
					//섹터 변경시 클라에게 정보 쏨
					CharacterSectorUpdatePacket(pCharacter);
					_LOG(dfLOG_LEVEL_DEBUG, L"Sector Change [SectorX:%d][SectorY:%d][Session:%d]",
						pCharacter->CurSector.iX, pCharacter->CurSector.iY, pCharacter->dwSessionID);
				}
			}
		}
	}
}

st_CHARACTER *CreateCharacter(st_SESSION *pSession)
{
	st_CHARACTER *pCharacter = new st_CHARACTER;

	pCharacter->pSession = pSession;
	pCharacter->dwSessionID = pSession->dwSessionID;
	
	pCharacter->dwAction = dfACTION_STAND;
	pCharacter->dwActionTick = 0;
	
	pCharacter->byDirection = dfACTION_MOVE_LL;
	pCharacter->byMoveDirection = dfACTION_STAND;

	pCharacter->shX = rand() % 6400;
	pCharacter->shY = rand() % 6400;
	pCharacter->shActionX = pCharacter->shX;
	pCharacter->shActionY = pCharacter->shY;

	pCharacter->CurSector.iX = pCharacter->shX / dfSECTOR_PIXEL_WIDTH;
	pCharacter->CurSector.iY = pCharacter->shY / dfSECTOR_PIXEL_HEIGHT;
	pCharacter->OldSector.iX = pCharacter->CurSector.iX;
	pCharacter->OldSector.iY = pCharacter->CurSector.iY;

	pCharacter->chHP = 100;

	return pCharacter;
}

st_CHARACTER *FindCharacter(DWORD dwSessionID)
{
	st_CHARACTER *pCharacter = NULL;
	Character::iterator cIter = g_CharacterMap.find(dwSessionID);

	if (cIter != g_CharacterMap.end())
		pCharacter = cIter->second;

	return pCharacter;
}

void	 DeleteCharacter(DWORD dwSessionID)
{
	Character::iterator cIter;

	cIter = g_CharacterMap.find(dwSessionID);
	if (cIter != g_CharacterMap.end())
	{
		delete cIter->second;
		g_CharacterMap.erase(cIter);
	}
}