#ifndef __etimezone_h
#define __etimezone_h

void e_tzset(void);

#if defined(__sh__)
bool e_daylight(void);
#endif

#endif

