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
#ifndef _SWITCHBOARD_HH__
#define _SWITCHBOARD_HH__

#include "GString.h"
#include "GBTree.h"
#include "GThread.h"

// contains the attributes of two connections that will 'meet' at 'this' server
class SwitchEntry
{
public:
	int bHandoff;
	int fdClient;
	int fdServer;
	int bResponse;
	void *pConnectHandle;

	char pzCreateTime[128];
	GString strRandomData;
	GString strKey;

	gthread_mutex_t lock;
	gthread_cond_t	cond;
	SwitchEntry(const char *pzKey);
	~SwitchEntry();
};



class SwitchBoard
{
	GBTree m_hshPending;// connections here waiting to be serviced
	GBTree m_hshState;  // connections are waiting to send
	
	gthread_mutex_t m_csServers;
	GBTree m_hshServers;	// connections are waiting to recv

	
	// todo: this one mutex serves all operation on both hashes.
	// switch on the first byte of the hash key to determine which
	// hash to go to (A-J=1  K-R=2   T-Z=3) to balance the load.
	// Give each hash it's own mutex to reduce exclusion lock wait time by
	// a factor of 1/3.
	gthread_mutex_t m_cs;

	typedef void(*fnInfoLog)(int, GString &);
	fnInfoLog m_fnLog;

	// set to any non zero value to terminate all pending connections and prepare for process exit
	int m_bServerIsShuttingDown;


public:
	void ShutDown(){m_bServerIsShuttingDown = 1;}
	void Startup(){m_bServerIsShuttingDown = 0;}

	// set to 0 to turn tracing off, any other value to turn tracing on
	int m_bTraceConnection;

	SwitchBoard();
	~SwitchBoard();
	void ViewCurrentState();

	void SetInfoLog( void (*pfn) (int, GString &) )
	{
		m_fnLog = pfn;
	}
	void InfoLog( int nID, const char *pzMsg )
	{
		if (m_fnLog)
		{
			GString str(pzMsg);
			m_fnLog(nID,str);
		}
	}
	void InfoLog( int nID, GString &str )
	{
		if (m_fnLog)
		{
			m_fnLog(nID,str);
		}
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
	void WaitForResponse (int fd, const char *pzCertifiedMsgResponse);


	// fd is an inbound connection that was accepted() on the HTTP port (maybe 80)
	// It might have looked like this if it is checking for 3 different connection names.
	//
	//		GET /PublicPath/DesignerXML.com&&DesignerXSL.com&&TransactXML.com%23456234645 HTTP/1.?
	//		Connection: Keep-Alive
	//
	// HandOffConnection() returns true if handoff happens otherwise false
	int HandOffConnection(int fd, const char *keyin, void *pConnectHandle = 0);
	
	// looks in the m_hshPending for the specified connection
	SwitchEntry *Pending (const char *keyin, int bRemove);


	// This is where the sender (the client) waits for the receiver (the server) to give us permission to send(put) the message.
	// 
	// The client set something like this to the HTTP port:
	//
	// GET /PublicPath/put/DOMAIN%nnnnnnnnnn HTTP/1.1
	//
	int WaitForConnection (const char *keyin, GString &strKeyOut, const char *pzRandomData);
	 	

	// a poll-in to Pickup a waiting connection.
	// this is an example of a multi key poll-in (&& separated):
	// --------------------------------------------------------
	// GET /PublicPath/DesignerXSL.com&&DOMAIN.com&&UBTInc.com HTTP/1.1
	// Connection: Keep-Alive
	int ConnectToServer (const char *keyin, void **ppNonSocket = 0);

};


#endif // _SWITCHBOARD_HH__