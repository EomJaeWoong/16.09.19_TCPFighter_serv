#ifndef __CONTENT__H__
#define __CONTENT__H__

void DataSetup();
void Update();

st_CHARACTER *CreateCharacter(st_SESSION *pSession);		//캐릭터 생성
st_CHARACTER *FindCharacter(DWORD dwSessionID);			//캐릭터 찾기
void			DeleteCharacter(DWORD dwSessionID);	//캐릭터 삭제

extern Character	g_CharacterMap;
extern DWORD		g_dwLoopTime;

#endif