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
static char SOURCE_FILE[] = __FILE__;


#include "SwitchBoard.h"
#include "GThread.h"
#include "GSocketHelpers.h"
#include "GString.h"
#include "GBTree.h"
#include "GStringList.h"

#include <time.h>		// time()
#include <string.h>		// strlen()
#include <ctype.h>		// toupper()

#ifdef _WIN32
	#include <windows.h> // Sleep()
	#ifdef __WINPHONE
		#include "WinPhoneThreadEmulation.h"
		using namespace ThreadEmulation;  // for Sleep()
	#endif
#endif


SwitchEntry::SwitchEntry(const char *pzKey)
{
	strKey = pzKey;
	pConnectHandle = 0;

	bHandoff = 1;
	fdServer = -1;
	fdClient = -1;
	bResponse = 0;

	struct tm *newtime;
#ifdef WINCE
	unsigned long ltime;
#else
	long ltime;
#endif
	time((time_t *)&ltime);
	newtime = gmtime((time_t *)&ltime);
	strftime(pzCreateTime, 128, "Date: %a, %d %b %Y %H:%M:%S GMT\r\n", newtime);

	gthread_mutex_init(&lock,0);
	gthread_cond_init(&cond,0);
	gthread_mutex_lock(&lock);
}
SwitchEntry::~SwitchEntry()
{
	gthread_mutex_destroy(&lock);
	gthread_cond_destroy(&cond);
}


SwitchBoard::SwitchBoard()
{
	m_bServerIsShuttingDown = 0;
	m_bTraceConnection = 1;
	m_fnLog = 0;
	gthread_mutex_init(&m_cs, NULL);
	gthread_mutex_init(&m_csServers, NULL);
}

SwitchBoard::~SwitchBoard()
{
	gthread_mutex_destroy(&m_cs);
	gthread_mutex_destroy(&m_csServers);
}

void SwitchBoard::ViewCurrentState()
{
	gthread_mutex_lock(&m_cs);

	InfoLog(14, "Waiting for:");
	GBTreeIterator it(&m_hshState,2);
	while (it())
	{
		SwitchEntry *pSE = (SwitchEntry *)it++;
		if (pSE)
		{
			GString str;
			str.Format("%s -- %s",(const char *)pSE->strKey, pSE->pzCreateTime);
			InfoLog(15, str);
		}
		else
		{
			// InfoLog(16,"Error - A (null) Switch Entry exists"); // this never happens
		}
	}

	InfoLog(17,"Waiting to Service:");
	GBTreeIterator it2(&m_hshPending,2);
	while (it2())
	{
		SwitchEntry *pSE = (SwitchEntry *)it2++;
		if (pSE)
		{
			GString str;
			str.Format("%s -- %s",(const char *)pSE->strKey, pSE->pzCreateTime);
			InfoLog(18,str);
		}
		else
		{
			// InfoLog(19,"Error - A (null) Switch Entry exists"); // this never happens
		}
	}
	gthread_mutex_unlock(&m_cs);
}

		
// WaitForResponse() is client step 4 in the complete transaction message flow that is as follows
// -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//					Client																Server
// -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// 1. The Client asked for permission to PUT in WaitForConnection()			1. Server looks for clients HandOffConnection() (A. gets an HTTP 200's 'not here' or B. gets put in hshPending)
// 2. SwitchBoard sends client HTTP 200 an "OK" to send					
// 3. The Client POSTS to the switchboard - finds the server in hshPending.	2. The Server gets the clients POST converted to an HTTP 200.
// 4. The Client waits in hshPending with WaitForResponse()					3. The Server POSTS a response to the switchboard for the message is just received, finds client in hshPending
// 5. Switchboard converts Servers POST(Server step 3) into an HTTP 200 (response to client step 3)
//																			4. Switchboard sends an 200 back to the server.
void SwitchBoard::WaitForResponse (int fd, const char *pzCertifiedMsgResponse)
{

	if (m_bTraceConnection)
	{
		GString strMsg("Client waiting for response txn:");
		strMsg << pzCertifiedMsgResponse;
		InfoLog(20,strMsg);
	}

	// upper-case the key parts
	char *key = (char *)pzCertifiedMsgResponse;
	int len = (int)strlen(key);
	for (int i = 0; i < len; i++)
		key[i] = toupper(key[i]);



	gthread_mutex_lock(&m_cs);
	SwitchEntry *pSENew = new SwitchEntry(key);
	pSENew->fdServer = fd;
	pSENew->bHandoff = 0;
	pSENew->bResponse = 1;
	pSENew->strRandomData = pzCertifiedMsgResponse;
	while(m_hshPending.remove(key)); // unlikely chance of duplicates, but we'll eliminate it
	m_hshPending.insert(key, (void *)pSENew);
	gthread_mutex_unlock(&m_cs);
}


// fd is an inbound connection that was accepted() on the HTTP port (maybe 80)
// It might have looked like this if it is checking for 3 different connection names.
//
//		GET /PublicPath/DesignerXML.com&&DesignerXSL.com&&TransactXML.com%23456234645 HTTP/1.?
//		Connection: Keep-Alive
//
// HandOffConnection() returns true if handoff happens otherwise false
int SwitchBoard::HandOffConnection(int fd, const char *keyin, void *pConnectHandle/* = 0*/)
{
	if (m_bTraceConnection)
	{
		GString strTrace;
		strTrace << "HandOffConnection() to clients looking for[" << keyin << "]";
		InfoLog(111,strTrace);
	}

	// proxy caching delimiter is % 
	// if the connection is something like MyConnect%12345, remove the % and following data.
	// It was added so that a proxy-firewall would not cache the request regardless of settings.
	char key[256];
	key[255]=0;
	char *pzKeyTerm = (char *)strpbrk(keyin,"%");
	if (pzKeyTerm)
	{
		*pzKeyTerm = 0;
		strncpy(key,keyin,255);
		*pzKeyTerm = '%';
	}
	else
	{
		strncpy(key,keyin,255);
	}

	// upper-case the key parts
	int len = (int)strlen(key);
	for (int i = 0; i < len; i++)
		key[i] = toupper(key[i]);


	GStringList lstKeys("&&",key);
	GStringIterator itKeys(&lstKeys);


	//	todo: remove this loop, and replace it with a WaitForMultipleObjects where each key
	//  is in a structure with a synchronization object that may be signaled
	//
	//  todo: If the process that connected to us is ended, we don't know.
	//	SO_ERROR gives no indication, and even getpeername() refuses to return WSAENOTCONN under Windows anyhow
	//  so we need to atleast allow superseded requests to cancel prior requests.

	for (int i2=0;i2<60;i2++)
	{


	gthread_mutex_lock(&m_cs);
	SwitchEntry *pSE;
	while (itKeys())
	{
		const char *keyPart = itKeys++;
		pSE = (SwitchEntry *)m_hshState.search(keyPart);
		if (pSE)
		{
			SwitchEntry *pSENew = 0;
			m_hshState.remove(keyPart);
			if (m_bTraceConnection)
			{
				GString strLog;
				strLog.Format("Picking up Connection [%s] on SwitchBoardServer",keyPart);
				InfoLog(20,strLog);
			}
			pSE->fdServer = fd;
			if (!pSE->bHandoff)
			{
				pSENew = new SwitchEntry(keyPart);
				pSENew->fdServer = fd;
				pSENew->bHandoff = pSE->bHandoff;
				pSENew->strRandomData = pSE->strRandomData;
				pSENew->pConnectHandle = pConnectHandle;

				while(m_hshPending.remove(keyPart));
				m_hshPending.insert(keyPart, (void *)pSENew);
			}
			gthread_mutex_unlock(&m_cs);
			gthread_cond_signal(  &pSE->cond );
			if ( pSENew )
				gthread_cond_signal( &pSENew->cond );

			// the caller must now send an HTTP header and an "OK"
			return 1;
		}
	}
	gthread_mutex_unlock(&m_cs);
	

	
	itKeys.reset();
#ifdef _WIN32
	Sleep(2000);
#else
	PortableSleep(2,0);
#endif

	if(m_bServerIsShuttingDown)
		break;
	} // end of 'to be removed'
	
	
	// no pickup was found....
	if (m_bTraceConnection)
	{
		//44=No Picking up for [%s] on SwitchBoardServer
		GString strLog;
		strLog.Format("No Picking up for [%s] on SwitchBoardServer", key);
		InfoLog(21,strLog);
	}

	return 0;
}

	
// looks in the m_hshPending for the specified connection
SwitchEntry *SwitchBoard::Pending (const char *keyin, int bRemove)
{
	// upper-case the key parts
	char *key = (char *)keyin;
	int len = (int)strlen(key);
	for (int i = 0; i < len; i++)
		key[i] = toupper(key[i]);


	// see if he's already here.......
	gthread_mutex_lock(&m_cs);
	SwitchEntry *pSE = (SwitchEntry *)m_hshPending.search(key);
	if (bRemove)
	{
		m_hshPending.remove(key);
	}
	gthread_mutex_unlock(&m_cs);
	return pSE;
}




// This is where the sender (the client) waits for the receiver (the server) to give us permission to send(put) the message.
// 
// The client sent something like this to the HTTP port:
//
// GET /PublicPath/put/DOMAIN%nnnnnnnnnn HTTP/1.1
//
int SwitchBoard::WaitForConnection (const char *keyin, GString &strKeyOut, const char *pzRandomData)
{
	// if some revolving data was tacked on the end of the request
	// so that a proxy server would not cache the request.... remove it.
	char *pc = (char *)strpbrk(keyin,"%");
	if (pc)
		*pc = 0;

	strKeyOut = keyin;// member selected in group poll requests.
	
	// upper-case the key parts
	char *key = (char *)keyin;
	int len = (int)strlen(key);
	for (int i = 0; i < len; i++)
		key[i] = toupper(key[i]);

/*
	// !!!
	// first check to see if there is a server waiting
	gthread_mutex_lock(&m_csServers);
	SwitchEntry *pseReady = (SwitchEntry *)m_hshServers.removeGet(key);
	gthread_mutex_unlock(&m_csServers);
	if (pseReady)
	{
		handoffconnection
	}

*/	




	SwitchEntry *pse = new SwitchEntry(key);
	pse->bHandoff = 0;
	pse->strRandomData = pzRandomData;

	// 2 minute wait - much can be done with adjusting wait times to tailor a server for high volume vs 'instant connect'
	gtimespec ts;
	ts.tv_sec=time(NULL) + 120;	// todo: [Switchboard] "WaitForConnection"
	ts.tv_nsec=0;

	gthread_mutex_lock(&m_cs);
	while(m_hshState.remove(key));	// no duplicates, newer supersedes older
	m_hshState.insert(key, (void *)pse);
	gthread_mutex_unlock(&m_cs);

	// wait for a condition signal(connection picked up by another thread) or timeout
	gthread_cond_timedwait(&pse->cond, &pse->lock, &ts);

	// if we timed out 
	if (time(0) >= ts.tv_sec)
	{
		// requested connection is not here, tell sender to try later
		delete pse;
		return 0;
	}
	else
	{
		// A "server" polled in to accept this client request.
		// the "server" entered the switchboard at HandOffConnection()
		// it flagged this client as "ready to be serviced" by placing it in the m_hshPending.
		// Now the sender(the client) should POST the message/file.
		return 1;
	}
}
	 	

// a poll-in to Pickup a waiting connection.
// this is an example of a multi key poll-in (&& separated):
// --------------------------------------------------------
// GET /PublicPath/DesignerXSL.com&&DOMAIN.com&&UBTInc.com HTTP/1.1
// Connection: Keep-Alive

int SwitchBoard::ConnectToServer (const char *keyin, void **ppNonSocket/* = 0*/)
{
	char *key = (char *)keyin;

	// upper-case the key parts
	int len = (int)strlen(key);
	for (int i = 0; i < len; i++)
		key[i] = toupper(key[i]);
	GStringList lstKeys("&&",key);


	// see if he's already here.......
	gthread_mutex_lock(&m_cs);
	GStringIterator itKeys(&lstKeys);
	while (itKeys())
	{
		SwitchEntry *pSE = (SwitchEntry *)m_hshPending.search(itKeys++);
		if (pSE)
		{
			gthread_mutex_unlock(&m_cs);
			if (ppNonSocket)
			{
				*ppNonSocket = pSE->pConnectHandle;
				return -2;
			}
			return pSE->fdServer;
		}
	}
	gthread_mutex_unlock(&m_cs);


	
	// otherwise wait for him for a while......
	gtimespec ts;
	ts.tv_sec=time(NULL) + 90; // 1.5 minute wait
	ts.tv_nsec=0;

	SwitchEntry se(key);

	// insert the pending connection into the wait hash
	GList lstTemp;
	gthread_mutex_lock(&m_cs);
	GStringIterator itKeys2(&lstKeys);
	while (itKeys2())
	{
		const char *pzKey2 = itKeys2++;
		while(m_hshState.remove(pzKey2));
		m_hshState.insert(pzKey2, (void *)&se);
	}
	gthread_mutex_unlock(&m_cs);




	if (m_bTraceConnection)
	{
		//6=Waiting for a [%s] server to pick up this connection
		GString strLog;
		strLog.Format("Waiting for a [%s] server to pick up this connection", key);
		InfoLog(22,strLog);
	}

	// wait for a condition signal(connection picked up by another thread) or timeout
	gthread_cond_timedwait(&se.cond, &se.lock, &ts); 

	// if we timed out 
	if (time(0) >= ts.tv_sec)
	{
		// nobody picked up this connection
		if (m_bTraceConnection)	
		{
			//7=Done Waiting ..... No connection 
			InfoLog(23,"Done Waiting ..... No connection");
		}
		gthread_mutex_lock(&m_cs);
		GStringIterator itKeys3(&lstKeys);
		while (itKeys3())
		{
			m_hshState.remove(itKeys3++);
		}
		gthread_mutex_unlock(&m_cs);
		return -1; // there is no "posted listener" for key
	}
	
	// otherwise the connection is now being serviced by another thread
	gthread_mutex_lock(&m_cs);
	GStringIterator itKeys4(&lstKeys);
	while (itKeys4())
	{
		m_hshState.remove(itKeys4++);
	}
	gthread_mutex_unlock(&m_cs);

	if (m_bTraceConnection)
	{
		//8=Done Waiting ..... Connecting
		InfoLog(24,"Done Waiting ..... Connecting");
	}

	// caller should now send the "OK" message
	return se.fdServer;
}
