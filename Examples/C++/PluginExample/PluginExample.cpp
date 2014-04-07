#include "stdafx.h"
#include "PluginExample.h"
#include "XMLFoundation.h"
#include "PluginBuilder.h" 

// note: it is not necessary to link to the XMLFoundation to create a plugin, it is not necessary to link to anything.
// Only 1 test case in this plugin uses any support from the XMLFoundation, that is Test 7.


// A debugging note:
// ------------------
// sometimes setting soft breakpoints in DLL's can be difficult, this is a very simple way to achieve
// the same, use a hard breakpoint. This will prompt you to debug when the code is executed:
//
//	_asm{int 3}
//
// The only thing to beware of, is that you do not accidently leave this compiled into the code that 
// you deliver to the end user, they won't know how to attach a debugger.




//############################################################################
// -- To run this sample code, use the following 5loaves.txt configuration
//############################################################################
/*

[System]
Pool=20 
ProxyPool=0 

[HTTP]
Enable=yes
EnableServerExtensions=yes
ServerPlugin1=/Test2WWWPage|CStdCall|PluginExample.dll|PluginExample|Test2
ServerPlugin2=/Test3WWWPage|CStdCall|PluginExample.dll|PluginExample|Test3
ServerPlugin3=/Test4WWWPage|CStdCall|PluginExample.dll|PluginExample|Test4
ServerPlugin4=/Test5WWWPage|CStdCall|PluginExample.dll|PluginExample|Test5
ServerPlugin5=/Test7WWWPage|CStdCall|PluginExample.dll|PluginExample|Test7
ServerPlugin6=/Page1|CStdCall|PluginExample.dll|PluginExample|Page1
ServerPlugin7=/Page2|CStdCall|PluginExample.dll|PluginExample|Page2
ServerPlugin8=/Page3|CStdCall|PluginExample.dll|PluginExample|Page3

[TXML]
Drivers=C:\Users\Brian\Desktop\XMLFoundation\Drivers\Debug

[CStdCall]
Path=C:\Users\Brian\Desktop\XMLFoundation\Examples\C++\HTTP.Xfer.Messaging-PluginExample

*/




// This is what we expose from this plugin, every public handler MUST be registered here.
extern "C" __declspec(dllexport) char *ExposedMethods() 
{ 
	static char pzInterface[] = 

	  // Define some static HTML pages.  These could also be served up as .html files 
	  // from the home directory configured under the [HTTP] section - but as static bound
	  // files it requires less to configure and run this example.
      "Page1&Header&char *&Content&char  *!"			  // http://127.0.0.1/Page1
      "Page2&Header&char *&Content&char  *!"			  // http://127.0.0.1/Page2
      "Page3&Header&char *&Content&char  *!"			  // http://127.0.0.1/Page3

      "Test2&One&char *&Two&char  *&Three&char *!"   // process POST from Page1
      "Test3&Header&char *&Content&char  *!"			  // process POST from Page2
      "Test4&Header&char *&Content&char  *!"			  // http://127.0.0.1/Test4WWWPage
      "Test5&Header&char *&Content&char  *!"			  // http://127.0.0.1/test5WWWPage
      "Test7&Header&char *&Content&char  *!"			  // process POST from Page3
      "TestXferPlugin&Data&char *&&"
      "CustomMessagingHandler&Data&char *!"; // <-- semi-colon here
			 
     return pzInterface; 
} 




/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////// This implements hard bound HTML pages  //////////////////////////
/////////////////////////  Essentially 0 argument function calls   //////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
// :
//
//                                   http://127.0.0.1/Page1
//
//
//  This page posts back to [Test2WWWPage] - another plugin defined in this source file
extern "C" __declspec(dllexport) void Page1(void *pHandle, DriverExec Exec,
                                     const char *pzHTTPHeader,	// string
									 const char *pzContent)		// string
{
	PlugInController PIC(pHandle, Exec);
	PIC << "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\">";
	PIC << "<HTML><HEAD><TITLE>United Business Technologies</TITLE>";
	PIC << "<BODY>";
	PIC << "<FORM method=post Action=\"/Test2WWWPage\">";
	PIC << "<P><br>Enter some text<INPUT name=One>";
	PIC << "<P><br>Enter more text<INPUT name=Two>";
	PIC << "<P><br>and a third time<INPUT name=Three>";
	PIC << "<INPUT type=submit value=Submit name=B1>";
	PIC << "<INPUT type=reset value=Reset name=B2></P></FORM></BODY></HTML>";
}
//                                   http://127.0.0.1/Page2
//
//
//  This page posts back to [Test3WWWPage] - another plugin defined in this source file
extern "C" __declspec(dllexport) void Page2(void *pHandle, DriverExec Exec,
                                     const char *pzHTTPHeader,	// string
									 const char *pzContent)		// string
{
	PlugInController PIC(pHandle, Exec);


	PIC << "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\">";
	PIC << "<HTML><HEAD><TITLE>United Business Technologies</TITLE>";
	PIC << "<BODY>";
	PIC << "<FORM method=post Action=\"/Test3WWWPage\">";
	PIC << "<P><br>Enter some text<INPUT name=One>";
	PIC << "<P><br>Enter more text<INPUT name=Two>";
	PIC << "<P><br>and a third time<INPUT name=Three>";
	PIC << "<INPUT type=submit value=Submit name=B1>";
	PIC << "<INPUT type=reset value=Reset name=B2></P></FORM></BODY></HTML>";;
}

//                                   http://127.0.0.1/Page3
//
//
//  This page posts back to [Test7WWWPage] - another plugin defined in this source file
extern "C" __declspec(dllexport) void Page3(void *pHandle, DriverExec Exec,
											const char *pzHTTPHeader,	// string
											const char *pzContent)		// string
{
	PlugInController PIC(pHandle, Exec);

	PIC << "<HTML><HEAD><TITLE>Send File</TITLE><BODY>";
	PIC << "<FORM id=Upload name=Upload method=post Action=\"/Test7WWWPage\" encType=multipart/form-data>";
	PIC << "Select a file to send<INPUT id=Location name=City Value=\"Antioch\" Type=Hidden>";
	PIC << "<INPUT id=UploadFile style=\"width: 364; height: 23\" type=file name=UploadFile>";
	PIC << "<br><br>Add Text to send<INPUT id=Location name=Street Value=\"Send some text between the 2 files\" size=\"51\">";
	PIC << "<INPUT id=Location name=Country Type=Hidden> ";
	PIC << "<INPUT id=Location name=Signature Value=\"777\" Type=Hidden> <br> <br>  &nbsp;Add another file";
	PIC << "<INPUT id=UploadFile2 style=\"width: 372; height: 23\" type=file name=UploadFile2>";
	PIC << "<br><p>There is code between the lines.</p> <DIV style=\"WIDTH: 500px\" align=right> <SPAN class=LittleText></SPAN>";
	PIC << "<INPUT type=submit value=Submit name=Submit></DIV>";
	PIC << "</FORM><p>&nbsp;</p></BODY></HTML>";

}









//////////////////////////////////////////////////////////////////////////////////
// Test2 - Called by 5Loaves HTTP server
//////////////////////////////////////////////////////////////////////////////////
//
// Arguments are Mapped to this HTML:
//
//      <!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">
//      <HTML><HEAD><TITLE>United Business Technologies</TITLE>
//      <BODY>
//      <FORM method=post Action="/Test2WWWPage">
//      <P><INPUT name=One>
//      <P><INPUT name=Two>
//      <P><INPUT name=Three>
//      <INPUT type=submit value=Submit name=B1>
//      <INPUT type=reset value=Reset name=B2></P></FORM></BODY></HTML>
//
// which can stored as file "test2.html" in the HTTP servers home directory
// and accessed from your browser at ----->  http://127.0.0.1/test2.html
//
// ----------------------------------------------------------------------------
//									AND		
// ----------------------------------------------------------------------------
//
// it may also be accessed through the static bound page ----> http://127.0.0.1/Page1
//
// 
// ----------------------------------------------------------------------------
//									AND		
// ----------------------------------------------------------------------------
// 
//  This same method may be invoked from the browser by typing a URL like this:
//
//				http://127.0.0.1/test2WWWPage&aaa&bbb&ccc
//
//	so you can see the exact same handler mapped to both a GET and a POST:
//
//
// Note the user defined delimiter prescribed in the first byte - the "&"
// that is used to delimit the remaining arguments aaa,bbb,and ccc.
// this url is valid too:
//
//				http://127.0.0.1/test2WWWPage~777~777~777
//
//
// to escape the delimiter, use the %hex value of the ASCII code, for example
// a & is an ASCII 38 or 26 in hex, so that this URL:
//
//				http://127.0.0.1/test2WWWPage&7%267%267&Big&Wave
//
// produces this result.
// 
//				"5Loaves Invoked me with [7&7&7] and [Big] and [Wave]!"
//
//
extern "C" __declspec(dllexport) void Test2(void *pHandle, DriverExec Exec,
										    const char *One,	// string
											const char *Two,	// string
											const char *Three)	// string

{ 
	PlugInController PIC(pHandle, Exec);
	
	PIC.AppendResults("5Loaves HTTP Server Invoked me with [");
	PIC.AppendResults(One);
	PIC.AppendResults("] and [");
	PIC.AppendResults(Two);
	PIC.AppendResults("] and [");
	PIC.AppendResults(Three);
	PIC.AppendResults("]!");
} 





//
// Like Test2(), Test3() accepts input from a form POST or a URL GET.
// Unlike Test2(), Test3() does not extract any arguments so ......
//
// this URL: http://127.0.0.1/test3WWWPageCompleteControlForThePlugin
//
// returns the HTTP header and posted data back to the browser
//
// and this URL:http://127.0.0.1/test3WWWPageComplete%20Control%20For%20The%20Plugin
//
// also returns the HTTP header and posted data back to the browsewr
// 
// also you may load the static page that POSTS back to this handler: 
// http://127.0.0.1/Page2
//                     
//
//
//  ----------------------NOTES ABOUT   pzHTTPHeader --------------------------
// the first argument (pzHTTPHeader) will almost never be used since it contains
// no data, only processing instructions - considering you are calling
// a user defined method, the processing instructions will most likely be known
// without looking at the HTTP headers.  The header value from the form POST is:
//
//		HTTP/1.1
//		Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, application/vnd.ms-excel, application/msword, application/vnd.ms-powerpoint
//		Referer: http://127.0.0.1/test2
//		Accept-Language: en-us
//		Content-Type: application/x-www-form-urlencoded
//		Accept-Encoding: gzip, deflate
//		User-Agent: Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.0)
//		Host: 127.0.0.1
//		Content-Length: 39
//		Connection: Keep-Alive
//		Cache-Control: no-cache
//
// 
// and the value from URL GET    http://127.0.0.1/test3WWWPageCompleteControlForThePlugin
// 
//
//		CompleteControlForThePlugin HTTP/1.1
//		Accept: */*
//		Accept-Language: en-us
//		Accept-Encoding: gzip, deflate
//		If-Modified-Since: Mon, 09 Feb 2004 17:45:54 GMT
//		User-Agent: Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.0)
//		Host: 127.0.0.1
//		Connection: Keep-Alive
//
// ---------------------------------------------------------------------
//	NOTE:the pzHTTPHeader is (\r\n\r\n) terminated NOT NULL TERMINATED!
// ---------------------------------------------------------------------
extern "C" __declspec(dllexport) void Test3(void *pHandle, DriverExec Exec,
                                     const char *pzHTTPHeader,	// string
									 const char *pzContent)		// string
{
	PlugInController PIC(pHandle, Exec);

	PIC << pzHTTPHeader;
}




//
// Test4() shows how to retrieve transactional information like the IP address
// as reported by the machine that invoked this method.
//
// invoke this function with this URL:     http://127.0.0.1/Test4WWWPage


#include <stdio.h>
// to understand the internal operation of plugins, press f5 repeatedly in the browser
// to see the thread assignment, and keep alive functionality.
extern "C" __declspec(dllexport) void Test4(void *pHandle, DriverExec Exec,
                                     const char *pzHTTPHeader,	// string
									 const char *pzContent)		// string
{
	PlugInController PIC(pHandle, Exec);


	PIC.AppendResults("<html><head><title>This is Test 4</title></head><body>");
	PIC.AppendResults("<p>Method was invoked by machine IP:");
	PIC.AppendResults(   PIC.GetCallerIPAddress()   );
	PIC.AppendResults("</p><p>On Thread");
	
	char buf[64];
	sprintf(buf, "%d", (int)PIC.GetServerThreadID() );
	PIC.AppendResults( buf );

	PIC.AppendResults("</p><p>Socket file descriptor ");
	sprintf(buf, "%d", (int)PIC.GetServerSocketHandle() );
	PIC.AppendResults( buf );


	PIC.AppendResults("</p><p>Remaining Keep Alives ");
	sprintf(buf, "%d", (int)PIC.GetKeepAliveCount() );
	PIC.AppendResults( buf );

	// uncomment this, to see how eternal keep alives affect transaction
	// PIC.GetKeepAliveCount(1);

	// -----------------------------------------------------------------
	// these are two 640000 byte buffers than can be used for any purpose by the plugin
	// using them rather than allocating memory is very wise, it is much faster than an allocation and does not fragment memory.
	// GetBuffer1() is inter-transactional within the same connection.
	// GetBuffer2() is not inter-transactional

	//  you could reset GetBuffer1 each time like this:
	// memset(PIC.GetBuffer1(),0, PIC.GetBufferSize() ); 

	// or build upon it's value, note:  it is not bound to a method, it is bound to the thread and the connection so 
	// various methods can share the same memory.  Naturally if the connection is broken the contents will be reset.
	// this means one method in this plugin, may set some inter-transactional state in GetBuffer1, and another method
	// will have that information available to it, and there is no need for an application programmer to lookup
	// transactional state in many cases - if the client supports keep alive connections.  most do.
	strcat(PIC.GetBuffer1(),"X");
	// notice we add one X each time, and the inter-transactional state grows until the keep alive max has been reached
	// and if you set PIC.GetKeepAliveCount(1), it grows for ever, and if you hit F5 in your browser 64000 times
	// you would append right off the end of the system buffer, so stop before then.

	// the other buffer, GetBuffer2(),  has HTTP headers and POST data, after you have used them, feel free to reuse the memory.
	memset(PIC.GetBuffer2(), 0, PIC.GetBufferSize() ); 



	PIC.AppendResults( "<p>Inter-Transactional buffer:" );
	PIC.AppendResults( PIC.GetBuffer1() );
	PIC.AppendResults( "</p>" );
	// -----------------------------------------------------------------



	PIC.AppendResults( "</p></body></html>" );
}



//
// Call Test5 from the browser like this: http://127.0.0.1/test5WWWPage
//
// this shows how you can build the HTTP headers manually
// use the Processor Instruction
extern "C" __declspec(dllexport) void Test5(void *pHandle, DriverExec Exec,
                                     const char *pzHTTPHeader,	// string
									 const char *pzContent)		// string
{
	PlugInController PIC(pHandle, Exec);

	PIC << "HTTP/1.1 200 OK\r\n";
	PIC << "Date: Sat, 7 Jul 2007 07:07:07 GMT\r\n";
	PIC << "Server: My Own Server\r\n";
	PIC << "Connection: Keep-Alive\r\n";
	PIC << "Keep-Alive: timeout=777, max=77\r\n";
	PIC << "Last-modified: Sat, 7 Jul 2007 07:07:07 GMT\r\n";
	PIC << "Content-type: text/html\r\n";
	PIC << "Content-length: 4\r\n\r\n";
	PIC << "Test";
	PIC.SetProcessInstruction("HasHeaders");// <---- instructs the HTTP Server not to create the HTTP headers
}



//
//
//
//
//
//
extern "C" __declspec(dllexport) void Test7(void *pHandle, DriverExec Exec,
                                     const char *pzHTTPHeader,	// string
									 const char *pzContent)		// string
{
	PlugInController PIC(pHandle, Exec);

	char *p1 = PIC.GetBuffer1();
	char *p2 = PIC.GetBuffer2();

//  uncomment this to see what the browser is sending
//	PIC << pzHTTPHeader ;
//	return;

	// CMultiPartForm makes it very easy to extract the individual data segments from HTTP multipart messages
	// there are several usage examples that follow
	GMultiPartForm p(&PIC, pzHTTPHeader, pzContent);


/*

	// usage sample 1, put the form parts into our own GStrings
	// if you are expecting something large you should PreAlloc the GString accordingly as is done with strFile1
	GString strCity, strStreet, strCountry, strSignature, strFile2;	
	GString strFile1(500000); // pre-allocate 500,000 bytes
	p.MapArgument("City"	,&strCity);
	p.MapArgument("Street"	,&strStreet);
	p.MapArgument("Country"	,&strCountry);
	p.MapArgument("Signature",&strSignature);
	p.MapArgument("UploadFile", &strFile1);
	p.MapArgument("UploadFile2",&strFile2);
	if ( p.GetAll() )
	{
		// the data has been put where it was mapped, the files have both been placed in GStrings
		PIC << "RIGHT CLICK IN THE BROWSER AND 'View Source'\r\n";
		PIC << "City=[" <<  strCity << "]\r\nStreet=[" << strStreet << "]\r\nCountry=[" << strCountry << "]\r\nSignature=[" << strSignature << "]\r\n";
		PIC << "UploadFile(" << strFile1.Length() << ") bytes:";
		PIC.AppendResults( strFile1, strFile1.Length() );
		PIC << "\r\n\r\n\r\n **************************************************************\r\nUploadFile2(" << strFile2.Length() << ") bytes:";
		PIC.AppendResults( strFile2, strFile2.Length() );
	}
	else
	{
		printf( p.GetLastError() );
	}


*/


/*

	// usage sample 2, put the small data in GStrings and the large data directly on disk
	// for large files, or when supporting many sessions it is wise not to hold large files in memory.

	GString strCity, strStreet, strCountry, strSignature;
	p.MapArgument("City"	,&strCity);
	p.MapArgument("Street"	,&strStreet);
	p.MapArgument("Country"	,&strCountry);
	p.MapArgument("Signature",&strSignature);
	p.MapArgumentToFile("UploadFile", "d:\\File1.bin");
	p.MapArgumentToFile("UploadFile2","d:\\File2.bin");
	if ( p.GetAll() )
	{
		// the short data has been put where it was mapped, the files have both been written to disk.
		// If the files could be very large, it would wise to use this style of receiving them because they 
		// are not loaded into memory, rather they are incrementally written to disk as the data is received.

		PIC << "RIGHT CLICK IN THE BROWSER AND 'View Source'\r\n";
		PIC << "City=[" <<  strCity << "]\r\nStreet=[" << strStreet << "]\r\nCountry=[" << strCountry << "]\r\nSignature=[" << strSignature << "]\r\n";
		PIC << "The files were written to D:\\ as [File1.bin] and [File2.bin] \r\n";
	}
	else
	{
		printf( p.GetLastError() );
	}

*/

/*

	// Usage sample 3, dynamically extract each piece of data, 
	// try posting small .txt files to this example then right click the browser and "view source" to see the results
	FormPart *pFormPart = 0;
	while( p.ReadNext(&pFormPart) )
	{
		PIC << "[" << pFormPart->strPartName << "]=[" << *pFormPart->strData << "]\r\n\r\n";
	}

*/



	// usage sample 4, dynamically extract each piece of data, 
	// suppose you could not know the disk file name and location until you had obtained the username, 
	// or in this example the city, this example does not hold the file in memory it puts it directly to disk.
	// Note:obviously in this example "City" must exist before "UploadFile2" in the HTML form
	FormPart *pFormPart = 0;
	while( p.ReadNext(&pFormPart) )
	{
		if (pFormPart->strPartName.CompareNoCase("City") == 0)
		{
			// use the value of City to define the filename
			GString strDestFile("d:\\");
			strDestFile << *pFormPart->strData;
			p.MapArgumentToFile("UploadFile2", strDestFile); // creates D:\\Denver
		}
	}
	PIC << "done";
}




//
//	This sample runs through the Xfer protocol, not the HTTP server
//
//
// The system buffers default to 64k, so if you pass less than 64k in data to a plugin, it will all be available
// in pData, you can call "map(1)" to see the size of the data that was passed into this function.  You can call
// "siz" to see the size of all the data that this method should expect to receive.  If "siz" > "map(1)", you will
// need to call "nxt" to get the next chunk, that will be available in GetBuffer2()
//
// Plugins that use the Xfer protocol are very different than the ones that pass through the HTTP protocol.
// The handler code is very similar, but the service provided by the protocol is very different.  HTTP does not do 
// much underneath the plugin but read the data off the TCP buffer and hand it to the plugin, Xfer does a lot.
// Xfer supports connection routes and proxies so if you consider the plugin as function foo(), you can 
// execute:      www.MyComany.com|MyMachine-->foo()
// where www.MyComany.com is the only machine that has a public IP, and MyMachine is reachable by www.MyComany.com
// there is NO limit to the depth of a connection route and it supports reverse connects where MyMachine polls out
// to a public IP looking for someone who wants to execute foo().  You can't do that with IIOP, DCOM only Xfer
//
// There's a lot of security underneath this too.  Every call is guaranteed to be from the user it should be from.
// This is unlike an HTTP plugin that uses anonymous connections.  The data posted to this Xfer plugin was also
// encrypted and compressed during network transport.  It may have been certified with a transactional sequencer also.
//
// All of this underneath, and it's FAST.  Very fast.
//
extern "C" __declspec(dllexport) void TestXferPlugin(void *pHandle, DriverExec Exec,const char *pData)
{

	
	PlugInController PIC(pHandle, Exec);

	// printf(PIC.GetUserName());

	int nArg1Size = PIC.GetArgumentSize();


	int nDataSize = PIC.GetExpectedDataSize();
	FILE *fp = fopen("copyOfFilePosted.cpy","wb");
	if (fp)
	{
		fwrite(pData,1,nArg1Size,fp);
		int nBytesIn = nArg1Size;
		while (nBytesIn < nDataSize)
		{
			// get the next chunk of the data
			int nDataSize = 0;
			int nRslt = PIC.NextDataChunk(&nDataSize);

			if (nRslt == -1)
			{
				// the connection has been broken
				break;
			}
			else if (nRslt == 1)
			{
				nBytesIn += nDataSize;
				fwrite(PIC.GetBuffer1(),1, nDataSize ,fp);
			}
			else if (nRslt == 0)
			{
				// there is no more data here yet
			}
		}
		fclose(fp);
	}


	fp = fopen("copyOfFilePosted.cpy","rb");
	if (fp)
	{
		// get the size of the file
		fseek(fp,0,SEEK_END);
		long lBytes = ftell(fp);
		fseek(fp,0,SEEK_SET);

		if (lBytes != nDataSize)
		{
			PIC.AppendResults("Error writing the data sent");
			fclose(fp);
			return;
		}
		
		// This can be any size. - multiples of 8192 are most efficient
		char pDataBlock[16384];

		// the first call to SendDataChunk() must contain the size of the data to be sent
		sprintf(pDataBlock,"OK %d:",(int)lBytes);
		int nHeaderOffset = strlen(pDataBlock);
		int nBytesToSend = lBytes;



		while(nBytesToSend > 0)
		{
			int nReadSize = (nBytesToSend > sizeof(pDataBlock) ) ? sizeof(pDataBlock) : nBytesToSend;
			if (nReadSize + nHeaderOffset > sizeof(pDataBlock))
			{
				nReadSize -= nHeaderOffset;
			}
			fread(&pDataBlock[nHeaderOffset],sizeof(char),nReadSize,fp);
			int nRslt = PIC.SendDataChunk( pDataBlock , nReadSize + nHeaderOffset );
			nBytesToSend -= nReadSize;
			nHeaderOffset = 0; // always 0 after the first block
		}
		fclose(fp);
	}
}



//
// This is a handler for the messaging protocol in the 5Loaves engine.
//
// 5loaves will poll out to a switchboard server looking for client that want to send a messages.
// That message can then be mapped to a custom handler.
// This handler is almost identical to the handlers for HTTP and Xfer, with a few notable differences:
//
// First, GetBuffer2() will return NULL, and GetBuffer1()'s length is GetBufferSize() NOT 65536
// Second, PIC.GetExpectedDataSize() will equal PIC.GetTotalBytesIn() when there is no more data to read.
//         prior to those two valued being equal, PIC.GetExpectedDataSize() is only an estimate.
// Third, MessageArgs() returns details about the message, and that does not apply to HTTP or Xfer plugins
//		  MessageArgs(0) = Account name
//		  MessageArgs(1) = User name      ---same as---    PIC.GetUserName()
//		  MessageArgs(2) = Message Title
//		  MessageArgs(3) = Size | HTTP Multipart boundary
//		  MessageArgs(4) = Certification Data

/*
[TXML]
Drivers=D:\SRC\UBT\XMLFoundation\Bin\Drivers

[CStdCall]
Path=D:\MySharedLibs

[Messaging]
Enable=yes
AcceptFrom=UBTsAccountForYou
DefaultSwitchBoardServer=192.168.1.3
DefaultSwitchBoardPort=81
UseBrowserProxy=no
BrowserProxy=dilbert
BrowserProxyPort=8080
LocalNetworkAuthenticate=user:password
UsePluginHandler=yes
PluginHandler=CStdCall::PluginExample.dll::PluginExample::CustomMessagingHandler


[MsgFrom-UBTsAccountForYou]
Enable=yes
CheckAtSwitchBoard=yes
Name=/PublicPath/UBTsAccountForYou
DiskLocation=c:\5LMessages\UBTsAccountForYou
LetSenderPlaceFile=No
PollIntervalSeconds=20
*/

extern "C" __declspec(dllexport) void CustomMessagingHandler(void *pHandle, DriverExec Exec,const char *pData)
{

	PlugInController PIC(pHandle, Exec);

	int nArg1Size = PIC.GetArgumentSize();

	FILE *fp = fopen("copyOfMessage.cpy","wb");
	if (fp)
	{
		fwrite(pData,1,nArg1Size,fp);
		int nBytesIn = nArg1Size;
		while ( 1 )
		{
			if ( PIC.GetExpectedDataSize() == PIC.GetTotalBytesIn() )
				break;

			// get the next chunk of the data
			int nDataSize = 0;
			int nRslt = PIC.NextDataChunk(&nDataSize);
			if (nRslt == -1)
			{
				// the connection has been broken
				break;
			}
			else if (nRslt == 1)
			{
				nBytesIn += nDataSize;
				fwrite(PIC.GetBuffer1(),1, nDataSize ,fp);
			}
			else if (nRslt == 0)
			{
				// there is no more data here yet
			}
		}
		fclose(fp);
	}

	PIC << "Your message was received in full and processed by a custom message handler.";
}


//////////////////////////////////////////////////////////////////////////////////////////////


BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
    }
    return TRUE;
}
