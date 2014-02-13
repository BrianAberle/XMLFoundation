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
#include "GlobalInclude.h"
#include "GPerformanceTimer.h"
#include "GString.h"
#include <stdlib.h>
#include <stdio.h>

#pragma warning (disable:4267)
#pragma warning (disable:4244)

#include <sys/timeb.h>

long getTimeMS()
{
	struct timeb tb;
	ftime(&tb);
	return tb.time * 1000 + tb.millitm;
}

#ifdef _WIN32
	#ifndef	_WIN64
		#ifndef __WINPHONE
			#ifndef WINCE
				#define _XMLF_GPERFTIMER_ASM
			#endif
		#endif
	#endif
	__int64 I64MSRB, I64MSRE;
	void *mrsb = &I64MSRB;
	void *mrse = &I64MSRE;
	int cyclesb;
	int cyclese;
	#include <Windows.h>  // for LARGE_INTEGER type
#endif

void GPerformanceTimer::PerformanceStart()
{
	m_bRunning = 1;
	m_fPerfDiff = 0.0;
#ifdef _WIN32
	if (m_bMillis)
	{
		startTime = getTimeMS();
	}
	else
	{
	// this counts CPU cycles (in 32 bit)
	#ifdef _XMLF_GPERFTIMER_ASM
		_asm mov eax, 0
		_asm cpuid
		_asm rdtsc
		_asm mov ebx, mrsb
		_asm mov dword ptr [ebx], eax
		_asm mov dword ptr [ebx+4], edx
		_asm mov eax, 0
		_asm cpuid
	#endif
		LARGE_INTEGER ltime; 
		QueryPerformanceFrequency(&ltime);
		m_fFreq = double(ltime.QuadPart)/1000000.0; // microseconds

		QueryPerformanceCounter(&ltime);
		m_nPerfStart = ltime.QuadPart;

	}
#else // unix
	if (m_bMillis)
		startTime = getTimeMS();
	else
		gettimeofday(&startTimeUnix, NULL);
#endif

}
//---------------------------------------------------------------------------

void GPerformanceTimer::PerformanceStop(GString *p/* = 0*/)
{
	m_bRunning = 0;
#ifdef _WIN32
	if (m_bMillis)
	{
		if (m_plTime)
			*m_plTime = getTimeMS() - startTime;
	}
	else
	{
	   #ifdef _XMLF_GPERFTIMER_ASM
		_asm mov eax, 0
		_asm cpuid
		_asm rdtsc
		_asm mov ebx, mrse
		_asm mov dword ptr [ebx], eax
		_asm mov dword ptr [ebx+4], edx
		_asm mov eax, 0
		_asm cpuid
		if (m_plTime) 
			*m_plTime = I64MSRE-I64MSRB;
	   #endif
		LARGE_INTEGER ltime; 
		QueryPerformanceCounter(&ltime);
	   	m_fPerfDiff = double(ltime.QuadPart-m_nPerfStart)/m_fFreq;

		if (m_plTime) 
			*m_plTime = m_fPerfDiff;
	}
#else
	if (m_plTime && m_bMillis)
	     *m_plTime = getTimeMS() - startTime;

        timeval endTime;
        long seconds, useconds;
		gettimeofday(&endTime, NULL);

        seconds  = endTime.tv_sec  - startTimeUnix.tv_sec;
        useconds = endTime.tv_usec - startTimeUnix.tv_usec;

        m_fPerfDiff = useconds + (seconds * 1000000);
#endif	

	if ( p || m_bCout )
	{
		if (m_bMillis)
		{
			if (p) *p << getTimeMS() - startTime << " milliseconds";
			if (m_bCout) printf("%ld milliseconds \n",getTimeMS() - startTime);
		}
		else
		{
			GString strFormatted;
#ifdef _WIN32
			char perfmtrbuf[100];

  #ifdef _WIN64
			
		#  if defined(_MSC_VER) && _MSC_VER >= 1400
			_ui64toa_s(cyclesb-cyclese, perfmtrbuf, sizeof(perfmtrbuf), 10);
		#  else
			_ui64toa(cyclesb-cyclese, perfmtrbuf, 10);
		#  endif
  #else
		#  if defined(_MSC_VER) && _MSC_VER >= 1400
			_ui64toa_s(I64MSRE-I64MSRB, perfmtrbuf, sizeof(perfmtrbuf), 10);
		#  else
			_ui64toa(I64MSRE-I64MSRB, perfmtrbuf, 10);
		# endif
  #endif
		
	#ifdef _XMLF_GPERFTIMER_ASM
			strFormatted << perfmtrbuf;
			if (p) *p << strFormatted.CommaNumeric() << " cycles = ";
			if (m_bCout)
			{
				printf(strFormatted.CommaNumeric());
				printf(" cycles = ");
			}
			strFormatted.Empty();
	#endif
#endif

			if (m_fPerfDiff)
			{
				strFormatted << m_fPerfDiff;
				strFormatted.SetLength( strFormatted.Find('.') ); // truncate the float at the decimal
				if (p) *p << strFormatted.CommaNumeric() << " microseconds ";
				if (m_bCout)
				{
					printf(strFormatted.CommaNumeric());
					printf(" microseconds \n");
				}
			}
		}
	}

}
//---------------------------------------------------------------------------




GPerformanceTimer::~GPerformanceTimer()
{
	if (!m_bRunning)
		return;

	PerformanceStop();
}


GPerformanceTimer::GPerformanceTimer(__int64 *lTime, const char *pzMessage, int nMillis) :
    m_bMillis(nMillis),
	m_plTime(lTime)
{
	m_bCout = 0;
	if (pzMessage)
	{
		printf("[");
		printf(pzMessage);
		printf("]=");
		m_bCout = 1;
	}
	PerformanceStart();
}



GPerformanceTimer::GPerformanceTimer(const char *pzMessage, int nMillis) :
    m_bMillis(nMillis)
{
	m_plTime = 0;
	m_bCout = 0;
	if (pzMessage)
	{
		printf("[");
		printf(pzMessage);
		printf("]=");
		m_bCout = 1;
	}
	PerformanceStart();
}
