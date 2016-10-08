#ifndef __OBJECTTYPE__H__
#define __OBJECTTYPE__H__

#define dfSERVER_FRAME			25

//-----------------------------------------------------------------
// Action
//-----------------------------------------------------------------
#define dfACTION_MOVE_LL		0
#define dfACTION_MOVE_LU		1
#define dfACTION_MOVE_UU		2
#define dfACTION_MOVE_RU		3
#define dfACTION_MOVE_RR		4
#define dfACTION_MOVE_RD		5
#define dfACTION_MOVE_DD		6
#define dfACTION_MOVE_LD		7

#define dfACTION_ATTACK1		8
#define dfACTION_ATTACK2		9
#define dfACTION_ATTACK3		10

#define dfACTION_STAND			11

//-----------------------------------------------------------------
// �� ũ��
//-----------------------------------------------------------------
#define dfMAP_WIDTH			6400
#define dfMAP_HEIGHT			6400

//-----------------------------------------------------------------
// ȭ�� �̵� ����.
//-----------------------------------------------------------------
#define dfRANGE_MOVE_TOP		50
#define dfRANGE_MOVE_LEFT		10
#define dfRANGE_MOVE_RIGHT		6390
#define dfRANGE_MOVE_BOTTOM	6390

//-----------------------------------------------------------------
// ĳ���� �̵� �ӵ�
//-----------------------------------------------------------------
#define dfSPEED_PLAYER_X		6
#define dfSPEED_PLAYER_Y		4

//-----------------------------------------------------------------
// �̵� ����üũ ���� / X�� �Ǵ� Y������ 50 Pixel �̻� �����߻��� ����
//-----------------------------------------------------------------
#define dfERROR_RANGE			50


//---------------------------------------------------------------
// ���ݹ��� / �� �κ��� ���������� ���
//---------------------------------------------------------------
#define dfATTACK1_RANGE_X	80
#define dfATTACK2_RANGE_X	90
#define dfATTACK3_RANGE_X	100
#define dfATTACK1_RANGE_Y	10
#define dfATTACK2_RANGE_Y	10
#define dfATTACK3_RANGE_Y	20


//---------------------------------------------------------------
// ���� ������ / �� �κ��� ���������� ���
//---------------------------------------------------------------
#define dfATTACK1_DAMAGE		1
#define dfATTACK2_DAMAGE		2
#define dfATTACK3_DAMAGE		3

#endif