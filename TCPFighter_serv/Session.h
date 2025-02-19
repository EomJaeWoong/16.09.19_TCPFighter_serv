#ifndef __SESSION__H__
#define __SESSION__H__

#define dfSession_MAX 1000

/*------------------------------------------------------------------*/
// 네트워크 세션
/*------------------------------------------------------------------*/
struct st_SESSION
{
	SOCKET socket;

	DWORD dwSessionID;

	CAyaStreamSQ RecvQ;
	CAyaStreamSQ SendQ;

	DWORD dwTrafficTick;
	DWORD dwTrafficCount;
	DWORD dwTrafficSecondTick;
};

typedef map<SOCKET, st_SESSION *> Session;

#endif