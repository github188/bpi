#include "header.h"

void printf_time()
{
	time_t tt = time(0);
	struct tm *ttm = localtime(&tt);
	printf("%d-%02d-%02d %02d:%02d:%02d",
			ttm->tm_year + 1900, ttm->tm_mon + 1, ttm->tm_mday,
			ttm->tm_hour, ttm->tm_min, ttm->tm_sec);
}
