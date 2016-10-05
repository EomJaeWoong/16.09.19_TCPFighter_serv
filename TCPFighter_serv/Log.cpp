#include <winsock2.h>
#include <wchar.h>
#include <time.h>

#include "Log.h"

int				g_iLogLevel;					//출력,저장 대상의 로그 레벨
WCHAR			g_szLogBuff[1024];				//로그 저장시 필요한 임시 버퍼

void Log(WCHAR *szString, int iLogLevel)
{
	time_t timer;
	tm today;

	time(&timer);

	localtime_s(&today, &timer); // 초 단위의 시간을 분리하여 구조체에 넣기

	wprintf(L"[%4d/%02d/%02d][%02d:%02d:%02d] ",
		today.tm_year + 1900, today.tm_mon + 1, today.tm_mday,
		today.tm_hour, today.tm_min, today.tm_sec);

	wprintf(L"%s \n", szString);
}