#ifndef __SECTOR__H__
#define __SECTOR__H__

#define dfSECTOR_PIXEL_WIDTH	200
#define dfSECTOR_PIXEL_HEIGHT	200

#define dfSECTOR_MAX_X			dfMAP_WIDTH / dfSECTOR_PIXEL_WIDTH
#define dfSECTOR_MAX_Y			dfMAP_HEIGHT / dfSECTOR_PIXEL_HEIGHT

typedef list<st_CHARACTER *> Sector;

extern  Sector			g_Sector[dfSECTOR_MAX_Y][dfSECTOR_MAX_X];

// ĳ������ ���� ��ǥ shX, shY ���� ������ġ�� ����Ͽ� �ش� ���Ϳ� ����
void Sector_AddCharacter(st_CHARACTER *pCharacter);

// ĳ������ ���� ��ǥ shX, shY���� ���͸� ����Ͽ� �ش� ���Ϳ��� ����
void Sector_RemoveCharacter(st_CHARACTER *pCharacter);

// ���� RemoveCharacter, AddCharacter�� ����Ͽ�
// ���� ��ġ�� ���Ϳ��� ���� �� ������ ��ǥ�� ���͸� ���Ӱ� ����Ͽ� �ش� ���Ϳ� ����
bool Sector_UpdateCharacter(st_CHARACTER * pCharacter);

// Ư�� ���� ��ǥ ���� �ֺ� ����� ���� ���
void GetSectorAround(int iSectorX, int iSectorY, st_SECTOR_AROUND *pSectorAround);

// ���Ϳ��� ���͸� �̵� �Ͽ��� �� ���� ����ǿ��� ���� ����, ���� �߰��� ������ ���� ���ϴ� �Լ�.
void GetUpdateSectorAround(st_CHARACTER *pCharacter, st_SECTOR_AROUND *pRemoveSector, st_SECTOR_AROUND *pAddSector);

// ���� �̵� ��, ĳ���� ���� ����, ���� ó��
void CharacterSectorUpdatePacket(st_CHARACTER *pCharacter);

#endif