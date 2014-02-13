// --------------------------------------------------------------------------
//						United Business Technologies
//			  Copyright (c) 2000 - 2010  All Rights Reserved.
//
// Source in this file is released to the public under the following license:
// --------------------------------------------------------------------------
// This toolkit may be used free of charge for any purpose including corporate
// and academic use.  For profit, and Non-Profit uses are permitted.
//
// This source code and any work derived from this source code must retain 
// this copyright at the top of each source file.
// --------------------------------------------------------------------------

#ifndef _PERFORMANCE_PROFILE_
#define _PERFORMANCE_PROFILE_
#ifndef WINCE


#ifndef _WIN32
	#define __int64 long long
	#include <stdio.h>
	#include <stdlib.h>
	#include <sys/time.h>
#endif

long getTimeMS();

class GString;
class GPerformanceTimer
{	
	bool m_bRunning;
	long     startTime;
	__int64 *m_plTime;
	int      m_bCout;
	int      m_bMillis;
	
	double   m_fFreq;
	double	 m_fPerfDiff;

	__int64  m_nPerfStart;

#ifndef _WIN32
	timeval startTimeUnix;
#endif

public:

	GPerformanceTimer(__int64 *lTime = 0, const char *pzMessage = 0, int nMillis = 1);
	
	// set nMillis=1 for milliseconds, otherwise CPU cycles and/or Microseconds will be used
	GPerformanceTimer(const char *pzMessage, int nMillis = 1);

	~GPerformanceTimer();

	void PerformanceStart();
	void PerformanceStop(GString *p = 0);
};


#endif // WINCE
#endif //_PERFORMANCE_PROFILE_
