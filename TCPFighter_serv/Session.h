#ifndef __SESSION__H__
#define __SESSION__H__

/*------------------------------------------------------------------*/
// 匙飘况农 技记
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