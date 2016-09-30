#include "TCPFighter_serv.h"

/*-------------------------------------------------------------------------------------*/
// Server Setup
/*-------------------------------------------------------------------------------------*/
void DataSetup()
{

}



/*-------------------------------------------------------------------------------------*/
// ���� ���� ó��
/*-------------------------------------------------------------------------------------*/
void Update()
{
	//���� ������Ʈ ó�� Ÿ�̹� ���

	Character::iterator cIter;
	st_CHARACTER *pCharacter = NULL;

	for (cIter = g_CharacterMap.begin; cIter != g_CharacterMap.end();)
	{
		pCharacter = cIter->second;

		//���
		if (0 >= pCharacter->chHP)
			DisconnectSession(pCharacter->pSession->socket);

		else
		{
			//��Ŷ �ȿ��� ����

			switch (pCharacter->dwAction)
			{
				case dfACTION_MOVE_LL :
					if (pCharacter->shX - dfSPEED_PLAYER_X > dfRANGE_MOVE_LEFT)
						pCharacter->shX -= dfSPEED_PLAYER_X;

					break;

				case dfACTION_MOVE_LU :
					if ((pCharacter->shX - dfSPEED_PLAYER_X > dfRANGE_MOVE_LEFT) &&
						(pCharacter->shY - dfSPEED_PLAYER_Y > dfRANGE_MOVE_TOP))
					{
						pCharacter->shX -= dfSPEED_PLAYER_X;
						pCharacter->shY -= dfSPEED_PLAYER_Y;
					}

					break;

				case dfACTION_MOVE_UU :
					if (pCharacter->shY - dfSPEED_PLAYER_Y > dfRANGE_MOVE_TOP)
						pCharacter->shY -= dfSPEED_PLAYER_Y;

					break;

				case dfACTION_MOVE_RU :
					if ((pCharacter->shX + dfSPEED_PLAYER_X < dfRANGE_MOVE_RIGHT) &&
						(pCharacter->shY - dfSPEED_PLAYER_Y > dfRANGE_MOVE_TOP))
					{
						pCharacter->shX += dfSPEED_PLAYER_X;
						pCharacter->shY -= dfSPEED_PLAYER_Y;
					}

					break;

				case dfACTION_MOVE_RR :
					if (pCharacter->shX + dfSPEED_PLAYER_X < dfRANGE_MOVE_RIGHT)
						pCharacter->shX += dfSPEED_PLAYER_X;

					break;

				case dfACTION_MOVE_RD :
					if ((pCharacter->shX + dfSPEED_PLAYER_X < dfRANGE_MOVE_RIGHT) &&
						(pCharacter->shY + dfSPEED_PLAYER_Y < dfRANGE_MOVE_TOP))
					{
						pCharacter->shX += dfSPEED_PLAYER_X;
						pCharacter->shY += dfSPEED_PLAYER_Y;
					}

					break;

				case dfACTION_MOVE_DD :
					if (pCharacter->shY + dfSPEED_PLAYER_Y < dfRANGE_MOVE_TOP)
						pCharacter->shY += dfSPEED_PLAYER_Y;

					break;

				case dfACTION_MOVE_LD :
					if ((pCharacter->shX - dfSPEED_PLAYER_X > dfRANGE_MOVE_LEFT) &&
						(pCharacter->shY + dfSPEED_PLAYER_Y < dfRANGE_MOVE_TOP))
					{
						pCharacter->shX -= dfSPEED_PLAYER_X;
						pCharacter->shY += dfSPEED_PLAYER_Y;
					}
					break;
			}

			//�̵��Ҷ� ���� ������Ʈ
			if (pCharacter->dwAction >= dfACTION_MOVE_LL && pCharacter->dwAction <= dfACTION_MOVE_LD)
			{
				if (Sector_UpdateCharacter(pCharacter))
				{
					//���� ����� Ŭ�󿡰� ���� ��
				}
			}
		}
	}
}

st_CHARACTER *CreateCharacter(DWORD dwSessionID)
{
	Session::iterator sIter;

	sIter = g_Session.find(dwSessionID);

	st_CHARACTER *pCharacter = new st_CHARACTER;
}

st_CHARACTER *FindCharacter(DWORD dwSessionID)
{
	st_CHARACTER *pCharacter = NULL;
	Character::iterator cIter = g_CharacterMap.find(dwSessionID);

	if (cIter != g_CharacterMap.end())
		pCharacter = cIter->second;

	return pCharacter;
}