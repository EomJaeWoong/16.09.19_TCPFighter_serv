#ifndef __SECTORDEF__H__
#define __SECTORDEF__H__

/*---------------------------------------------------------*/
// Sector 하나의 좌표 정보
/*---------------------------------------------------------*/
struct st_SECTOR_POS
{
	int				iX;
	int				iY;
};


/*---------------------------------------------------------*/
// 특정 위치 주변의 9개 섹터 정보
/*---------------------------------------------------------*/
struct st_SECTOR_AROUND
{
	int				iCount;
	st_SECTOR_POS	Around[9];
};

#endif