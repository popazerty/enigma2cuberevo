#include "etimezone.h"
#include <time.h>

void e_tzset()
{
	tzset();
}

#if defined(__sh__)
bool e_daylight()
{
	if(daylight==0)
		return false;
	else
		return true;
}
#endif
