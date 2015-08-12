// --------------------------------------------------------------------------
//						United Business Technologies
//			  Copyright (c) 2015  All Rights Reserved.
//
// Source in this file is released to the public under the following license:
// --------------------------------------------------------------------------
// This toolkit may be used free of charge for any purpose including corporate
// and academic use.  For profit, and Non-Profit uses are permitted.
//
// This source code and any work derived from this source code must retain 
// this copyright at the top of each source file.
// --------------------------------------------------------------------------




//       ---Usage Example---
//
//	GString strRunningProcessData;
//	GProcessList( &strRunningProcessData );    // get data on all running processes
//
//	GStringList l("\n",strRunningProcessData); // each row divided by "\n"
//	GStringIterator it(&l);
//	while(it())
//	{
//		GProcessListRow *pRow = new GProcessListRow(it++);
//      now do something with each running process in pRow
//      delete pRow;
//	}

#ifndef __GPROCESS_LIST__
#define __GPROCESS_LIST__
#include <GString.h>
//#include <GStringIterator.h>

#define NET_FLAG_REDUCE_INFO			0x01 // reduced information set with labels in the data - not as good to parse - better to view directly
#define NET_FLAG_NO_UDP					0x02 // exclude UDP from the results
#define NET_FLAG_NEXT					0x04 // in XML woud be nice then we can throw XSL at it to make HTML or whatever

bool GetNetworkConnections(GString &strResults, int nFlags);
void InternalIPs(GStringList*plstBoundIPAddresses);


void GetProcessList( GString *pstrResults );

class GProcessListRow
{
public:
	GString strUnparsedData;
	GString strName;
	GString strPID;
	GString strCPU;
	GString strMem;
	GString strRunTime;
	GString strBinaryPath;
	GString strPriority;
	GString strThreads;
	GString strHandles;
	GProcessListRow(){}
	GProcessListRow(const char *pzRow);

};


#endif // __GPROCESS_LIST__