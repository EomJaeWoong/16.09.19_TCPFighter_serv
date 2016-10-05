#ifndef __OBJECTTYPE__H__
#define __OBJECTTYPE__H__

#define dfSERVER_FRAME	 500

//-----------------------------------------------------------------
// Action
//-----------------------------------------------------------------
#define dfACTION_MOVE_LL 0
#define dfACTION_MOVE_LU 1
#define dfACTION_MOVE_UU 2
#define dfACTION_MOVE_RU 3
#define dfACTION_MOVE_RR 4
#define dfACTION_MOVE_RD 5
#define dfACTION_MOVE_DD 6
#define dfACTION_MOVE_LD 7

#define dfACTION_ATTACK1 8
#define dfACTION_ATTACK2 9
#define dfACTION_ATTACK3 10

#define dfACTION_STAND	 11

//-----------------------------------------------------------------
// 맵 크기
//-----------------------------------------------------------------
#define dfMAP_WIDTH			6400
#define dfMAP_HEIGHT		6400

//-----------------------------------------------------------------
// 화면 이동 범위.
//-----------------------------------------------------------------
#define dfRANGE_MOVE_TOP	50
#define dfRANGE_MOVE_LEFT	10
#define dfRANGE_MOVE_RIGHT	6390
#define dfRANGE_MOVE_BOTTOM	6390

//-----------------------------------------------------------------
// 캐릭터 이동 속도
//-----------------------------------------------------------------
#define dfSPEED_PLAYER_X	3
#define dfSPEED_PLAYER_Y	2

//-----------------------------------------------------------------
// 이동 오류체크 범위 / X축 또는 Y축으로 50 Pixel 이상 오차발생시 끊음
//-----------------------------------------------------------------
#define dfERROR_RANGE		50

#endif