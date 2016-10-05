#ifndef __CHARACTER__H__
#define __CHARACTER__H__

//-----------------------------------------------------------------
// 캐릭터 정의
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
// 캐릭터 객체 관리 map type 정의
//------------------------------------------------------------------
typedef map<DWORD, st_CHARACTER *> Character;

#endif