#ifndef __CONTENT__H__
#define __CONTENT__H__

void DataSetup();
void Update();

st_CHARACTER *CreateCharacter(st_SESSION *pSession);		//ĳ���� ����
st_CHARACTER *FindCharacter(DWORD dwSessionID);			//ĳ���� ã��
void			DeleteCharacter(DWORD dwSessionID);	//ĳ���� ����

extern Character	g_CharacterMap;
extern DWORD		g_dwLoopTime;

#endif