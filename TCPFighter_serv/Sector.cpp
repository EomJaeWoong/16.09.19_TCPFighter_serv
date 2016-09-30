#include "TCPFighter_serv.h"
#include "Character.h"
#include "Sector.h"

void Sector_AddCharacter(st_CHARACTER *pCharacter)
{
	if (pCharacter->CurSecter.iX != -1 || pCharacter->CurSecter.iY != -1)
		return;

	int iSectorX = pCharacter->shX / dfSECTOR_PIXEL_WIDTH;
	int iSectorY = pCharacter->shY / dfSECTOR_PIXEL_HEIGHT;

	//Sector영역 벗어날 때
	if (iSectorX >= dfSECTOR_MAX_X || iSectorY >= dfSECTOR_MAX_Y)
		return;

	g_Sector[iSectorY][iSectorX].push_back(pCharacter);

	pCharacter->OldSecter.iX = pCharacter->CurSecter.iX;
	pCharacter->OldSecter.iY = pCharacter->CurSecter.iY;

	pCharacter->CurSecter.iX = iSectorX;
	pCharacter->CurSecter.iY = iSectorY;
}

void Sector_RemoveCharacter(st_CHARACTER *pCharacter)
{
	if (pCharacter->CurSecter.iX != -1 || pCharacter->CurSecter.iY != -1)
		return;

	Sector &SectorList = g_Sector[pCharacter->CurSecter.iY][pCharacter->CurSecter.iX];
	Sector::iterator sIter;

	for (sIter = SectorList.begin(); sIter != SectorList.end();++sIter)
	{
		if (pCharacter == *sIter)
		{
			SectorList.erase(sIter);
			break;
		}
	}

	pCharacter->OldSecter.iX = pCharacter->CurSecter.iX;
	pCharacter->OldSecter.iY = pCharacter->CurSecter.iY;
	pCharacter->CurSecter.iX = -1;
	pCharacter->CurSecter.iY = -1;
}

bool Sector_UpdateCharacter(st_CHARACTER * pCharacter)
{
	int iBeforeSectorX = pCharacter->CurSecter.iX;
	int iBeforeSectorY = pCharacter->CurSecter.iY;

	int iNewSectorX = pCharacter->shX / dfSECTOR_PIXEL_WIDTH;
	int iNewSectorY = pCharacter->shY / dfSECTOR_PIXEL_HEIGHT;

	if (iBeforeSectorX == iNewSectorX && iBeforeSectorY == iNewSectorY)
		return false;

	Sector_RemoveCharacter(pCharacter);
	Sector_AddCharacter(pCharacter);

	pCharacter->OldSecter.iX = iBeforeSectorX;
	pCharacter->OldSecter.iY = iBeforeSectorY;

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

	GetSectorAround(pCharacter->OldSecter.iX, pCharacter->OldSecter.iY, &OldSectorAround);
	GetSectorAround(pCharacter->CurSecter.iX, pCharacter->CurSecter.iY, &CurSectorAround);

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

			if (bFind == false)
			{
				pRemoveSector->Around[pRemoveSector->iCount] = OldSectorAround.Around[iCntOld];
				pRemoveSector->iCount++;
			}
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

			if (bFind == false)
			{
				pAddSector->Around[pAddSector->iCount] = CurSectorAround.Around[iCntCur];
				pAddSector->iCount++;
			}
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

}