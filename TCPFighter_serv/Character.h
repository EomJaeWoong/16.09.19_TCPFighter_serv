#ifndef __CHARACTER__H__
#define __CHARACTER__H__

#define dfACTION_MOVE_LL 0
#define dfACTION_MOVE_LU 1
#define dfACTION_MOVE_UU 2
#define dfACTION_MOVE_RU 3
#define dfACTION_MOVE_RR 4
#define dfACTION_MOVE_RD 5
#define dfACTION_MOVE_DD 6
#define dfACTION_MOVE_LD 7

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

typedef map<DWORD, st_CHARACTER *> Character;

st_CHARACTER *FindCharacter(DWORD dwSessionID);

#endif