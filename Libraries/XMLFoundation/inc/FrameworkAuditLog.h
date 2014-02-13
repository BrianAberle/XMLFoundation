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

#ifndef __XMLFrameworkTrace_H__
#define __XMLFrameworkTrace_H__

#include <stdarg.h> // for va_list definition
#include "GString.h"


#ifdef __TRACE_STACK__
	class UBTStackTrace
	{
		char *m_szFile;
		char *m_szTrace;
		void FormatV(const char *lpszFormat, va_list argList);

	public:

		// Traces out the file name and message
		UBTStackTrace(const char *szFile,
					  const char *lpszFormat, 
					  ...);

		// Traces out the file name and message
		// and cleans up all memory allocation.
		~UBTStackTrace();

	};
#else
	#define UBTStackTrace 
	#define _stk_trc //
#endif

// values for g_pchLogFile
#define LOGGING_UNASSIGNED (const char *)0
#define LOGGING_OFF (const char *)-1
// any other value in g_pchLogFile is a  valid 
// pointer to the AuditLogFile path and file name


// storage in XMLObject.cpp
void UBTAuditLog(const char *pzText, bool bNewLine);
bool UBTAuditLogIsEnabled();
void SetLogFile(const char *pzText);

// Create one of these to redirect logging 
// output during 'this' scope
extern char *g_pchLogFile;
class CLogFileOverride
{
	GString strOldValue;
	GString strNewValue;
public:
	CLogFileOverride(const char *pzFile)
	{
		strNewValue = pzFile;
		if ( g_pchLogFile && g_pchLogFile[0] )
			strOldValue = g_pchLogFile;
		g_pchLogFile = (char *)(const char *)strNewValue;
	}
	~CLogFileOverride()
	{
		g_pchLogFile = (char *)(const char *)strOldValue;
	}
};



#define TRACE_ERROR(pszMessage)   { if (g_pchLogFile && g_pchLogFile != LOGGING_OFF){ UBTAuditLog("****ERROR: ",0); UBTAuditLog((const char *)pszMessage,1); }}
#define TRACE_WARNING(pszMessage) { if (g_pchLogFile && g_pchLogFile != LOGGING_OFF){ UBTAuditLog("**WARNING: ",0); UBTAuditLog((const char *)pszMessage,1); }}
#define TRACE_MESSAGE(pszMessage) { if (g_pchLogFile && g_pchLogFile != LOGGING_OFF){ UBTAuditLog("**MESSAGE: ",0); UBTAuditLog((const char *)pszMessage,1); }}

#define TRACE_ERROR2(pszMessage1, pszMessage) { if (g_pchLogFile && g_pchLogFile != LOGGING_OFF){ UBTAuditLog("****ERROR: ",0); UBTAuditLog(pszMessage1,0); UBTAuditLog((const char *)pszMessage,1); }}
#define TRACE_WARNING2(pszMessage1, pszMessage) { if (g_pchLogFile && g_pchLogFile != LOGGING_OFF){ UBTAuditLog("**WARNING: ",0); UBTAuditLog(pszMessage1,0); UBTAuditLog((const char *)pszMessage,1); }}
#define TRACE_MESSAGE2(pszMessage1, pszMessage) { if (g_pchLogFile && g_pchLogFile != LOGGING_OFF){ UBTAuditLog("**MESSAGE: ",0); UBTAuditLog(pszMessage1,0); UBTAuditLog((const char *)pszMessage,1); }}

#endif //__XMLFrameworkTrace_H__
