#ifndef __CHARACTER__H__
#define __CHARACTER__H__

//-----------------------------------------------------------------
// ĳ���� ����
//-----------------------------------------------------------------
struct st_CHARACTER
{
	st_SESSION *pSession;
	DWORD dwSessionID;

	DWORD dwAction;
	DWORD dwActionTick;

	BYTE byDirection;
	BYTE byMoveDirection;

	short shX;
	short shY;
	short shActionX;
	short shActionY;

	st_SECTOR_POS CurSecter;
	st_SECTOR_POS OldSecter;

	char chHP;
};

//------------------------------------------------------------------
// ĳ���� ��ü ���� map type ����
//------------------------------------------------------------------
typedef map<DWORD, st_CHARACTER *> Character;

#endif