#include <WinSock2.h>
#include <list>
#include <map>

using namespace std;

#include "ObjectType.h"
#include "NPacket.h"
#include "StreamQueue.h"
#include "Session.h"
#include "SectorDef.h"
#include "Character.h"
#include "Sector.h"
#include "Network.h"
#include "Content.h"
#include "Log.h"

Sector			g_Sector[dfSECTOR_MAX_Y][dfSECTOR_MAX_X];

void Sector_AddCharacter(st_CHARACTER *pCharacter)
{
	//if ((pCharacter->CurSector.iX > 0) || (pCharacter->CurSector.iY >0) ||
	//	(pCharacter->CurSector.iX > dfSECTOR_MAX_X) || (pCharacter->CurSector.iX > dfSECTOR_MAX_Y))
	//	return;

	int iSectorX = pCharacter->shX / dfSECTOR_PIXEL_WIDTH;
	int iSectorY = pCharacter->shY / dfSECTOR_PIXEL_HEIGHT;

	//Sector영역 벗어날 때
	if (iSectorX >= dfSECTOR_MAX_X || iSectorY >= dfSECTOR_MAX_Y)
		return;

	g_Sector[iSectorY][iSectorX].push_back(pCharacter);

	pCharacter->OldSector.iX = pCharacter->CurSector.iX;
	pCharacter->OldSector.iY = pCharacter->CurSector.iY;

	pCharacter->CurSector.iX = iSectorX;
	pCharacter->CurSector.iY = iSectorY;
}

void Sector_RemoveCharacter(st_CHARACTER *pCharacter)
{
	//if ((pCharacter->CurSector.iX > 0) || (pCharacter->CurSector.iY >0) ||
	//	(pCharacter->CurSector.iX > dfSECTOR_MAX_X) || (pCharacter->CurSector.iX > dfSECTOR_MAX_Y))
	//	return;

	Sector &SectorList = g_Sector[pCharacter->CurSector.iY][pCharacter->CurSector.iX];
	Sector::iterator sIter;

	for (sIter = SectorList.begin(); sIter != SectorList.end();++sIter)
	{
		if (pCharacter == *sIter)
		{
			SectorList.erase(sIter);
			break;
		}
	}

	pCharacter->OldSector.iX = pCharacter->CurSector.iX;
	pCharacter->OldSector.iY = pCharacter->CurSector.iY;
	pCharacter->CurSector.iX = -1;
	pCharacter->CurSector.iY = -1;
}

bool Sector_UpdateCharacter(st_CHARACTER *pCharacter)
{
	int iBeforeSectorX = pCharacter->CurSector.iX;
	int iBeforeSectorY = pCharacter->CurSector.iY;

	int iNewSectorX = pCharacter->shX / dfSECTOR_PIXEL_WIDTH;
	int iNewSectorY = pCharacter->shY / dfSECTOR_PIXEL_HEIGHT;

	if (iBeforeSectorX == iNewSectorX && iBeforeSectorY == iNewSectorY)
		return false;

	Sector_RemoveCharacter(pCharacter);
	Sector_AddCharacter(pCharacter);

	pCharacter->OldSector.iX = iBeforeSectorX;
	pCharacter->OldSector.iY = iBeforeSectorY;

	return true;
}

void GetSectorAround(int iSectorX, int iSectorY, st_SECTOR_AROUND *pSectorAround)
{
	iSectorX--;
	iSectorY--;

	pSectorAround->iCount = 0;

	for (int iCntY = 0; iCntY < 3; iCntY++)
	{
		if (iSectorY + iCntY < 0 || iSectorY + iCntY >= dfSECTOR_MAX_Y)
			continue;

		for (int iCntX = 0; iCntX < 3; iCntX++)
		{
			if (iSectorX + iCntX < 0 || iSectorX + iCntX >= dfSECTOR_MAX_X)
				continue;

			pSectorAround->Around[pSectorAround->iCount].iX = iSectorX + iCntX;
			pSectorAround->Around[pSectorAround->iCount].iY = iSectorY + iCntY;
			pSectorAround->iCount++;
		}
	}
}

void GetUpdateSectorAround(st_CHARACTER *pCharacter, st_SECTOR_AROUND *pRemoveSector, st_SECTOR_AROUND *pAddSector)
{
	int iCntOld, iCntCur;
	bool bFind;
	st_SECTOR_AROUND OldSectorAround, CurSectorAround;

	OldSectorAround.iCount = 0;
	CurSectorAround.iCount = 0;

	pRemoveSector->iCount = 0;
	pAddSector->iCount = 0;

	GetSectorAround(pCharacter->OldSector.iX, pCharacter->OldSector.iY, &OldSectorAround);
	GetSectorAround(pCharacter->CurSector.iX, pCharacter->CurSector.iY, &CurSectorAround);

	//---------------------------------------------------------------------------------------------------
	// 이전 섹터 삭제
	//---------------------------------------------------------------------------------------------------
	for (iCntOld = 0; iCntOld < OldSectorAround.iCount; iCntOld++)
	{
		bFind = false;
		for (iCntCur = 0; iCntCur < CurSectorAround.iCount; iCntCur++)
		{
			if (OldSectorAround.Around[iCntOld].iX == CurSectorAround.Around[iCntCur].iX &&
				OldSectorAround.Around[iCntOld].iY == CurSectorAround.Around[iCntCur].iY)
			{
				bFind = true;
				break;
			}
		}

		if (bFind == false)
		{
			pRemoveSector->Around[pRemoveSector->iCount] = OldSectorAround.Around[iCntOld];
			pRemoveSector->iCount++;
		}
	}

	//---------------------------------------------------------------------------------------------------
	// 신규 섹터 삽입
	//---------------------------------------------------------------------------------------------------
	for (iCntCur = 0; iCntCur < CurSectorAround.iCount; iCntCur++)
	{
		bFind = false;
		for (iCntOld = 0; iCntOld < OldSectorAround.iCount; iCntOld++)
		{
			if (OldSectorAround.Around[iCntOld].iX == CurSectorAround.Around[iCntCur].iX &&
				OldSectorAround.Around[iCntOld].iY == CurSectorAround.Around[iCntCur].iY)
			{
				bFind = true;
				break;
			}
		}

		if (bFind == false)
		{
			pAddSector->Around[pAddSector->iCount] = CurSectorAround.Around[iCntCur];
			pAddSector->iCount++;
		}
	}
}

//-------------------------------------------------------------------------------------------------------
// 1. 이전 섹터에서 없어진 부분에 - 캐릭터 삭제 메시지
// 2. 이동하는 캐릭터에게 이전 섹터에서 제외된 섹터의 캐릭터들 삭제 시키는 메시지
// 3. 새로 추가된 섹터에 - 캐릭터 생성 메시지 & 이동 메시지
// 4. 이동하는 캐릭터에게 - 새로 진입한 섹터의 캐릭터들 생성 메시지
//-------------------------------------------------------------------------------------------------------
void CharacterSectorUpdatePacket(st_CHARACTER *pCharacter)
{
	st_SECTOR_AROUND stRemoveSector, stAddSector;
	CNPacket cPacket;

	GetUpdateSectorAround(pCharacter, &stRemoveSector, &stAddSector);

	makePacket_DeleteCharacter(&cPacket, pCharacter->dwSessionID);

	//---------------------------------------------------------------------------------------------------
	// 없어진 섹터 부분에서 캐릭터 삭제 메시지를 날림
	//---------------------------------------------------------------------------------------------------
	for (int iCnt = 0; iCnt < stRemoveSector.iCount; iCnt++)
	{
		SendPacket_SectorOne(stRemoveSector.Around[iCnt].iX, stRemoveSector.Around[iCnt].iY,
			&cPacket, NULL);
	}

	//---------------------------------------------------------------------------------------------------
	// 이동하는 캐릭터에게 이전 섹터에서 제외된 섹터의 캐릭터들 삭제 시키는 메시지를 날림
	//---------------------------------------------------------------------------------------------------
	Sector::iterator sIter;
	for (int iCnt = 0; iCnt < stRemoveSector.iCount; iCnt++)
	{
		for (sIter = g_Sector[stRemoveSector.Around[iCnt].iY][stRemoveSector.Around[iCnt].iX].begin();
			sIter != g_Sector[stRemoveSector.Around[iCnt].iY][stRemoveSector.Around[iCnt].iX].end(); ++sIter)
		{
			makePacket_DeleteCharacter(&cPacket, (*sIter)->dwSessionID);

			SendPacket_Unicast(pCharacter->pSession, &cPacket);
		}
	}

	//---------------------------------------------------------------------------------------------------
	// 새로 추가된 섹터에 - 캐릭터 생성 메시지 & 이동 메시지
	//---------------------------------------------------------------------------------------------------
	makePacket_CreateOtherCharacter(&cPacket, pCharacter->dwSessionID, pCharacter->byDirection,
		pCharacter->shX, pCharacter->shY, pCharacter->chHP);

	// 생성
	for (int iCnt = 0; iCnt < stAddSector.iCount; iCnt++)
	{
		SendPacket_SectorOne(stAddSector.Around[iCnt].iX, stAddSector.Around[iCnt].iY, &cPacket, NULL);
	}

	// 이동
	makePacket_MoveStart(&cPacket, pCharacter->dwSessionID, pCharacter->byMoveDirection,
		pCharacter->shX, pCharacter->shY);

	for (int iCnt = 0; iCnt < stAddSector.iCount; iCnt++)
	{
		SendPacket_SectorOne(stAddSector.Around[iCnt].iX, stAddSector.Around[iCnt].iY, &cPacket, NULL);
	}

	//---------------------------------------------------------------------------------------------------
	// 이동하는 캐릭터에게 - 새로 진입한 섹터의 캐릭터들 생성 메시지
	//---------------------------------------------------------------------------------------------------
	st_CHARACTER *pOtherCharacter;

	for (int iCnt = 0; iCnt < stAddSector.iCount; iCnt++)
	{
		for (sIter = g_Sector[stAddSector.Around[iCnt].iY][stAddSector.Around[iCnt].iX].begin();
			sIter != g_Sector[stAddSector.Around[iCnt].iY][stAddSector.Around[iCnt].iX].end(); ++sIter)
		{
			pOtherCharacter = (*sIter);

			if (pOtherCharacter != pCharacter)
			{
				//---------------------------------------------------------------------------------------
				// 다른 캐릭터들 생성
				//---------------------------------------------------------------------------------------
				makePacket_CreateOtherCharacter(&cPacket, pOtherCharacter->dwSessionID,
					pOtherCharacter->byDirection, pOtherCharacter->shX, pOtherCharacter->shY,
					pOtherCharacter->chHP);

				SendPacket_Unicast(pCharacter->pSession, &cPacket);

				//---------------------------------------------------------------------------------------
				// 다른 캐릭터들 이동, 공격 메시지 보냄
				//---------------------------------------------------------------------------------------
				switch (pOtherCharacter->dwAction)
				{
				case dfACTION_MOVE_LL :
				case dfACTION_MOVE_LU :
				case dfACTION_MOVE_UU :
				case dfACTION_MOVE_RU :
				case dfACTION_MOVE_RR :
				case dfACTION_MOVE_RD :
				case dfACTION_MOVE_DD :
				case dfACTION_MOVE_LD :
					makePacket_MoveStart(&cPacket, pOtherCharacter->dwSessionID,
						pOtherCharacter->byMoveDirection, pOtherCharacter->shX, pOtherCharacter->shY);

					SendPacket_Unicast(pCharacter->pSession, &cPacket);
					break;

				case dfACTION_ATTACK1 :
					makePacket_Attack1(&cPacket, pOtherCharacter->dwSessionID, pOtherCharacter->byDirection,
						pOtherCharacter->shX, pOtherCharacter->shY);

					SendPacket_Unicast(pCharacter->pSession, &cPacket);
					break;

				case dfACTION_ATTACK2:
					makePacket_Attack2(&cPacket, pOtherCharacter->dwSessionID, pOtherCharacter->byDirection,
						pOtherCharacter->shX, pOtherCharacter->shY);

					SendPacket_Unicast(pCharacter->pSession, &cPacket);
					break;

				case dfACTION_ATTACK3:
					makePacket_Attack3(&cPacket, pOtherCharacter->dwSessionID, pOtherCharacter->byDirection,
						pOtherCharacter->shX, pOtherCharacter->shY);

					SendPacket_Unicast(pCharacter->pSession, &cPacket);
					break;
				}
			}
		}
	}
}