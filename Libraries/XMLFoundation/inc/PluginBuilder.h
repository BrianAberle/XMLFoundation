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
#include "BaseInterface.h"
#include "GHTTPMultiPartPOST.h"


typedef void (*DriverExec)(void *pCBArg, const char *pzCmd);
typedef void (*CBfn)(const char *, void *);




class PlugInController : public DataController
{
	DriverExec m_Exec;
	CBDataStruct *m_pcb;
	char m_StringResultBuffer[64];
public:

	PlugInController(void *pHandle, DriverExec Exec)
	{
		m_Exec = Exec;
		m_pcb = (CBDataStruct *)pHandle;
	}
	void SetProcessInstruction(const char *pzPI)
	{
		// add the Process Instruction if it's not already added
		if (stristr( m_pcb->piBuf,pzPI ) == 0)
			strcpy( m_pcb->piBuf,pzPI );
	}
	void PreAllocReturnResults(int nSize) // estimate the size that you will be returning before calling AppendResults() or using the overloaded <<
	{
		m_pcb->arg[0].Int = nSize;
		m_Exec(m_pcb,"pre"); // concatinate into send queue
	}
	void AppendResults(const char *pzString) // appends from pzString up to the first null
	{
		m_pcb->arg[0].CChar = pzString;
		m_Exec(m_pcb,"cat"); // concatinate into send queue
	}
	void AppendResults(const char *pzString, int nLen) // appends from pzString (can be binary) nLen bytes
	{
		m_pcb->arg[0].Int = nLen;
		m_pcb->arg[1].CChar = pzString;
		m_Exec(m_pcb,"wrt"); // concatinate into send queue
	}

	int GetTotalBytesIn()
	{
		m_Exec(m_pcb,"tot");			
		return  m_pcb->arg[0].Int;
	}

	int GetExpectedDataSize()
	{
		m_Exec(m_pcb,"siz");			// get the size of the expected transfer
		return  m_pcb->arg[0].Int;
	}
	int SendDataChunk( char *pData, int nDataLen )
	{
		m_pcb->arg[0].Char = pData;
		m_pcb->arg[1].Int = nDataLen;
		m_Exec(m_pcb,"xtx"); //Transmit
		return m_pcb->arg[0].Int; // -1 is fail, 1 is success
	}
	int NextDataChunk(int *pnDataSize)
	{
		m_Exec(m_pcb,"xrx");// Receive
		*pnDataSize = m_pcb->arg[1].Int;
		return m_pcb->arg[0].Int; // -1 = error, 0 = no data here yet, 1 got it
	}
	int NextDataChunk(int *pnDataSize, char *pDest, int nMax, int nSecondsTimeout, int nMicrosecondsTimeout)
	{
		m_pcb->arg[0].Int = -1;
		m_pcb->arg[1].Char = pDest;
		m_pcb->arg[2].Int = nMax;
		m_pcb->arg[3].Int = nSecondsTimeout;
		m_pcb->arg[4].Int = nMicrosecondsTimeout;

		m_Exec(m_pcb,"xrx");// Receive
		*pnDataSize = m_pcb->arg[1].Int;
		return m_pcb->arg[0].Int; // -1 = error, 0 = no data here yet, 1 got it
	}
	int GetArgumentSize(int nIndex = 1) // 1 based index, 1 is the 1st arg, 2 is the 2nd
	{
		m_pcb->arg[0].Int = nIndex;
		m_Exec(m_pcb,"map");
		return m_pcb->arg[0].Int;
	}
	char *GetUserName() 
	{
		m_pcb->arg[0].Char = m_StringResultBuffer;
		m_pcb->arg[1].Int = sizeof(m_StringResultBuffer);
		m_Exec(m_pcb,"usr");
		return 	m_StringResultBuffer;
	}
	char *MessageArgs(int nIndex) // only available in Messaging protocol
	{
		m_pcb->arg[0].Int = nIndex;
		m_pcb->arg[1].Char = m_StringResultBuffer;
		m_pcb->arg[2].Int = sizeof(m_StringResultBuffer);
		m_Exec(m_pcb,"msg");
		return 	m_StringResultBuffer;
	}
	char *GetCallerIPAddress()
	{
		m_pcb->arg[0].Char = m_StringResultBuffer;
		m_Exec(m_pcb,"ip");
		return 	m_StringResultBuffer;
	}
	int GetServerThreadID()
	{
		m_Exec(m_pcb,"tid");
		return m_pcb->arg[0].Int;
	}
	void Disconnect()
	{
		m_Exec(m_pcb,"dis");
	}
	int GetServerSocketHandle()
	{
		m_Exec(m_pcb,"fd");
		return m_pcb->arg[0].Int;
	}
	int GetKeepAliveCount()	// this only applies to plugins called by the HTTP Server
	{
		m_Exec(m_pcb,"keepalive");
		if (m_pcb->arg[0].PInt)
			return *m_pcb->arg[0].PInt;
		return 0; // this must be protocol Xfer if the Keep Alive Count is not set
	}
	int SetKeepAliveCount(int nNewCount) // this only applies to plugins called by the HTTP Server
	{
		m_Exec(m_pcb,"keepalive");
		*m_pcb->arg[0].PInt = nNewCount;
	}
	char *GetBuffer1()
	{
		return (char *)m_pcb->wkBuf;
	}
	char *GetBuffer2()
	{
		return (char *)m_pcb->wkBuf2;
	}
	int GetBufferSize()
	{
		return m_pcb->nWKBufferSize;
	}

	PlugInController & operator<<(const char *p)
	{
		AppendResults(p);
		return *this;
	}
	PlugInController & operator<<(int n)
	{
		char  szBuffer[25];
		sprintf(szBuffer, "%li", n);
		AppendResults(szBuffer);
		return *this;
	}
	PlugInController & operator<<(const GString &g)
	{
		AppendResults(g, (int)g.Length());
		return *this;
	}

/*
	PlugInController & operator<<(__int64);
	PlugInController & operator<<(const signed char *);
	PlugInController & operator<<(char);
	PlugInController & operator<<(unsigned char);
	PlugInController & operator<<(signed char);
	PlugInController & operator<<(short);
	PlugInController & operator<<(unsigned short);
	PlugInController & operator<<(unsigned int);
	PlugInController & operator<<(long);
	PlugInController & operator<<(unsigned long);
	PlugInController & operator<<(float);
	PlugInController & operator<<(double);
	PlugInController & operator<<(long double);
	PlugInController & operator<<(const GString &);
*/

};





