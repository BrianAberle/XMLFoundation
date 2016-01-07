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

#include <XMLFoundation.h>

#include "CSmtp.h"
#include "GSocketHelpers.h"
#include "GProcess.h"


int main(int argc, char* argv[])
{

	GString strNetworkConnections(32767);
	GetNetworkConnections(strNetworkConnections,NET_FLAG_REDUCE_INFO|NET_FLAG_NO_UDP);

	GString strWanIP, strNoWanIPError; 
	ExternalIP(&strWanIP, &strNoWanIPError);

	CSmtp mail;
	GString strMailServer("smtp.gmail.com");


	// ------- GMail TLS --------
	// Note about GMail - login, then under "My Account" go to "Sign-In & Security" and set "Allow Less Secure Apps": to ON.  
	// This enables TLS and non-google apps(not insecure apps).  AOL, HotMail, and Yahoo enable the SMTP over TLS relay for
	// the paid email account services with no ads and higher mail/data limits.
	//
	mail.SetSMTPServer(strMailServer,587);
	mail.SetSecurityType(USE_TLS);

#include <"Do.not.compile">  // add your own Gmail account in the next two lines..... then delete this line   (and set the recipient)

	// Note about GMail - login, then under "My Account" go to "Sign-In & Security" and set "Allow Less Secure Apps": to ON.  
//	mail.SetLogin("MyOwnGmailAddress@gmail.com");
//	mail.SetPassword("MyOwnPassword");


	mail.SetSenderName("My Application");
	mail.SetSenderMail("No-reply@nowhere.com");
	mail.SetReplyTo("No-reply@nowhere.com");
	
	GString strSubject(g_strThisHostName);
	strSubject << " Stats";
	mail.SetSubject(strSubject);

	mail.AddRecipient("somebody@yahoo.com");   //<---------------------------------------------------------- Who to send the email to
//	mail.AddRecipient("AnotherRecipient@gmail.com");


  	mail.SetXPriority(XPRIORITY_NORMAL);
  	mail.SetXMailer("Professional (v7.77) Pro");
  	

	mail.AddMsgLine("----------------------------Wan IP----------------------------------------");
	mail.AddMsgLine(strWanIP);

	mail.AddMsgLine("----------------------Network Interfaces----------------------------------");
	GString strThisHost("Host:");
	strThisHost << g_strThisHostName;
	mail.AddMsgLine(strThisHost);
	GStringList lstBoundIPAddresses;
	InternalIPs(&lstBoundIPAddresses);
	GStringIterator it2(&lstBoundIPAddresses);
	while(it2())
	{
		mail.AddMsgLine(it2++);
	}

	mail.AddMsgLine("----------------------Network Connections----------------------------------");
	GStringList l("\n",strNetworkConnections); // each row divided by "\n"
	GStringIterator it(&l);
	while(it())
	{
		mail.AddMsgLine(it++);
	}


	mail.AddMsgLine("------------------------Processes-------------------------------------------");
	GString strRunningProcessData;
	GetProcessList( &strRunningProcessData );
	GStringList lstProcess("\n",strRunningProcessData); // each row divided by "\n"
	GStringIterator itP(&lstProcess);
	while(itP())
	{
		//mail.AddMsgLine(itP++);								// write it out -  raw
		GProcessListRow *pRow = new GProcessListRow(itP++);		// or use the already written code to parse process information

		GString strProcess;										// write onluy the data we want into this GString 
		strProcess << "pid:" << pRow->strPID << "   " << pRow->strName  << "     [" << pRow->strBinaryPath << "]";
		mail.AddMsgLine(strProcess);							// then write it out - formatted.
	}
	
  	//mail.AddAttachment("../test1.jpg");
  	//mail.AddAttachment("c:\\test2.exe");
	//mail.AddAttachment("c:\\test3.txt");
	mail.Send();



	return 0;
}


