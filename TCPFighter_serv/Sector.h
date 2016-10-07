#ifndef __SECTOR__H__
#define __SECTOR__H__

#define dfSECTOR_PIXEL_WIDTH	200
#define dfSECTOR_PIXEL_HEIGHT	200

#define dfSECTOR_MAX_X			dfMAP_WIDTH / dfSECTOR_PIXEL_WIDTH
#define dfSECTOR_MAX_Y			dfMAP_HEIGHT / dfSECTOR_PIXEL_HEIGHT

typedef list<st_CHARACTER *> Sector;

extern  Sector			g_Sector[dfSECTOR_MAX_Y][dfSECTOR_MAX_X];

// 캐릭터의 현재 좌표 shX, shY 으로 섹터위치를 계산하여 해당 섹터에 넣음
void Sector_AddCharacter(st_CHARACTER *pCharacter);

// 캐릭터의 현재 좌표 shX, shY으로 섹터를 계산하여 해당 섹터에서 삭제
void Sector_RemoveCharacter(st_CHARACTER *pCharacter);

// 위의 RemoveCharacter, AddCharacter를 사용하여
// 현재 위치한 섹터에서 삭제 후 현재의 좌표로 섹터를 새롭게 계산하여 해당 섹터에 넣음
bool Sector_UpdateCharacter(st_CHARACTER * pCharacter);

// 특정 섹터 좌표 기준 주변 영향권 섹터 얻기
void GetSectorAround(int iSectorX, int iSectorY, st_SECTOR_AROUND *pSectorAround);

// 섹터에서 섹터를 이동 하였을 때 섹터 영향권에서 빠진 섹터, 새로 추가된 섹터의 정보 구하는 함수.
void GetUpdateSectorAround(st_CHARACTER *pCharacter, st_SECTOR_AROUND *pRemoveSector, st_SECTOR_AROUND *pAddSector);

// 섹터 이동 시, 캐릭터 정보 삭제, 생성 처리
void CharacterSectorUpdatePacket(st_CHARACTER *pCharacter);

#endif