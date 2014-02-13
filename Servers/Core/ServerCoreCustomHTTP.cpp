//
// Server Core HTTP Extension Example for when SERVERCORE_CUSTOM_HTTP is #defined prior to #include ServerCore.cpp
//

// There are many variables available to the current scope.  A few of exceptional interest are:
// [sockBuffer] the raw network data directly in the TCP kernel buffer
// [nContentLen] the Content-Length from the HTTP header
// [nHTTPHeaderLength] byte count of HTTP headers
// [nHTTPContentAcquired] a POST may come in several network reads, this is the bytes of content not including HTTP header recv'd in the first read
// [nBytes] bytes in [sockBuffer] (note there may be more in transit if Content-Length > nBytes )
// [strConnectedIPAddress] the ip address of the client
// [td] a structure, td stands for "Thread Data"
// [td->sockfd] the socket handle.
// [td->nThreadID] a unique servicing thread number that you will need if you were to log status points because you may 
//                 have the identical logic executing near simultaneously on another thread - you will need to distinguish 
//                 and this number is easier to look at(1 to the number in your pool) vs. looking at a thread handle.
// [td->buf1&2] You ought to take advantage of the memory management infrastructure provided in ServerCore because two 
//            rather large memory buffers have been allocated and are reserved for your use.  Never free() or delete() them.
//            Both buffers were allocated prior to the connect() and rapid memory allocations in high transaction volumne
//			  system will slow you down however if you use the thread heaps as they were designed to be used, you will find
//            that even when linking with excellent heap managers as provided by www.microquill.com that normally shave
//            valuable time from transaction execution - it will not help you here because runtime allocations are
//            nearly nonexisting so it's already as fast as it can possibly be.  
//            Both buffers are MAX_DATA_CHUNK bytes long.  MAX_DATA_CHUNK is slightly over 64kb.
//			  also an interesting fact is that td->Buf1 == sockBuffer
//			  To use the thread memory buffers you may want to attach the memory to a GString like this.
//            GString str1(td->Buf1, 0, MAX_DATA_CHUNK, 0); // create an empty GString attached to a preallocated 64k buffer
//            GString str2(td->Buf2, 0, MAX_DATA_CHUNK, 0); // create an empty GString attached to a preallocated 64k buffer
//			  Use str1 and str2 to contain the request, and to build your response in and your server will be very fast.

if (memcmp(sockBuffer,"GET",3) == 0)
{
	GString strRequest;


	// extract the Transaction 'Starting From' the 4th byte 'UpTo'() the first space
	// we can extract the transaction like this safely because HTTP proper does not allow spaces in the URL
	// Proper HTTP will look like this:GET /Index.html HTTP/1.1
	strRequest.SetFromUpTo(&sockBuffer[4]," ");

	// now you have the request in the variable [strRequest] and there was 
	// very little logic overhead between you and the actual TCP connect()

	// Be aware that the URL is still encoded at this level. 
	// http://127.0.0.1/Spaces%20Look%20Like%20This HTTP/1.1
	// RFC 2396  states that these  "-" | "_" | "." | "!" | "~" | "%" |  "*" | "'" | "(" | ")" be encoded in URL's
	// unEscapeData() can quickly fix the data in the memory buffer that it already resides in when (pSource == pDest)
	// int newLen = unEscapeData(&sockBuffer[4],&sockBuffer[4]);
	// The same can be achieved through GString since this example has already got it in a GString
	strRequest.UnEscapeURL();


	//
	// You need to generate a response for yourself based on the request.  This is a good place to dispatch your 
	// call to your own function.  You might return HTML, or XML or a file or anything that your client is expecting.
	//
	// I would expect to see some sort of logic like this in a typical custom server, but this example is not going to be typical.
	/*
	GString cmd1("/Page1.html");
	GString cmd2("<GetInvoice")
	GString cmd3("GetRealtimeStats")
	GString cmd4("AdvanceMotor7")

	if (cmd1.CompareNocase(strRequest) == 0) // check the first few bytes of strRequest to see if they match a command we know
	{
		BuildDynamicWebPage(strRequest)    // strRequest may be "/Page1.html?arg1="Antioch"&arg2="777"
	} 
	else if (cmd2.CompareNocase(strRequest) == 0)
	{
		GenerateInvoice(strRequest)        // strRequest may be an XML request "<GetInvoice id='777'> ...
	}
	else if (cmd3.CompareNocase(strRequest) == 0)
	{
		GetBankAccountValue(strRequest)    // strRequest may be a request for state information in any type of device or system
	}
	else if (cmd4.CompareNocase(strRequest) == 0)
	{
		PositionRobotics(strRequest)       // custom
	}

	// note: If you add the functions to MYNewSourceFile.cpp then you should add MYNewSourceFile.cpp to your 
	// project and #include "MYNewSourceFile.h" prior to the #include "ServerCore.cpp" in your code.
	// Do not #include "MYNewSourceFile.h" into this source file - the compiler most likely won't let you.

	*/
	


	// For the sake of example we can return the entire HTTP request that we read in from the network.
	// so that you can see how the HTTP headers and data are sent over the wire and the information added by the browser.
	// Sending the response back through HTTPSend() will build the HTTP headers for us setting the Content-Length
	//
//	HTTPSend(td->sockfd, sockBuffer, nBytes, "text/plain");
//	goto KEEP_ALIVE;

	// You should enable the above 2 lines of code to see the HTTP headers (recompile then watch it work)
	// After you do that, you should set nBasicPOST=0 (recompile then watch it work)

	
	// that will send back an HTML form that will POST some data back to this example.
	int nBasicPOST = 1;
	if (nBasicPOST)
	{
		const char *pzHTMLPage = 
		"<HTML><HEAD><TITLE>United Business Technologies</TITLE>"
		"<BODY>"
		"<FORM method=post Action=\"/MyLowLevelBasicPostHandler\">"
		"<P><br>Enter some text<INPUT name=One>"
		"<P><br>Enter more text<INPUT name=Two>"
		"<P><br>and a third time<INPUT name=Three>"
		"<INPUT type=submit value=Submit name=B1>"
		"<INPUT type=reset value=Reset name=B2></P></FORM></BODY></HTML>";

		HTTPSend(td->sockfd, pzHTMLPage, strlen(pzHTMLPage), "text/html");
	}
	else // multi-part POST example
	{
		const char *pzHTMLPage = 
		"<HTML><HEAD><TITLE>Send File</TITLE><BODY>"
		"<FORM id=Upload name=Upload method=post Action=\"/MyLowLevelMultiPartPostHandler\" encType=multipart/form-data>"
		"Select a file to send<INPUT id=Location name=City Value=\"Antioch\" Type=Hidden>"
		"<INPUT id=UploadFile style=\"width: 364; height: 23\" type=file name=UploadFile>"
		"<br><br>Add Text to send<INPUT id=Location name=Street Value=\"Send some text between the 2 files\" size=\"51\">"
		"<INPUT id=Location name=Country Type=Hidden> "
		"<INPUT id=Location name=Signature Value=\"777\" Type=Hidden> <br> <br>  &nbsp;Add another file"
		"<INPUT id=UploadFile2 style=\"width: 372; height: 23\" type=file name=UploadFile2>"
		"<br><p>There is code between the lines.</p> <DIV style=\"WIDTH: 500px\" align=right> <SPAN class=LittleText></SPAN>"
		"<INPUT type=submit value=Submit name=Submit></DIV>"
		"</FORM><p>&nbsp;</p></BODY></HTML>";
	
		HTTPSend(td->sockfd, pzHTMLPage, strlen(pzHTMLPage), "text/html");
	}







	// alternatively we could setup the HTTP header ourselves and call nonblocksend()
	// int nonblocksend(int fd,const char *pData,int nDataLen)


	// lastly manage this connection and thread.  HTTP commands are "transactional" unlike
	// a protocol like SMTP that is "conversational".  Each GET/POST gets a response and that is one
	// complete transaction.  It may well be the end of the connection too unless the HTTP Server
	// uses "HTTP Keep Alives" described in the HTTP Specification.  This 'goto' will cause this 
	// thread to expect more transactions from the same connection so this thread will not return
	// to the thread pool until it gets the next transaction or times out (120 seconds is default).
	// it should also be noted that connections are commonly broken by clients,proxies,and even ISP's
	// after a complete HTTP transaction so you should not assume that the next time this client
	// sends you a transaction it will be on this thread, but if it is on this thread then it serviced
	// the connection a little faster because TCP did not need to re accept() the connection like it
	// would if you went to CLOSE_CONNECT, so the behavior should be identical no matter where you
	// goto  KEEP_ALIVE or  CLOSE_CONNECT) -->IF<-- the protocol is HTTP.  This handler will work 
	// for ANY protocol so don't think like you are in a box or something.
	goto KEEP_ALIVE;

	// alternatively we could close this connection and return this thread to the pool
	// so that all system resources are immediately returned to the pool awaiting the next connection.
	// You may optionally set the nCloseCode to any value that you like, and the value you set will be
	// present in connection trace logs when [Trace]ConnectTrace=1.  The nCloseCode value will have no 
	// effect on server bahavior.
	// nCloseCode =7000; 
	// goto CLOSE_CONNECT;


	// you also have the option to allow the logic flow to return naturally (without a goto)
	// but ONLY if you did not handle this transaction.  In this example we generated a response
	// with HTTPSend() so we MUST return with a goto, otherwise the standard HTTP handler will also
	// generate a response that the client would not expect.

}
else if (memcmp(sockBuffer,"POST /",6) == 0)
{
    GString str2(td->Buf2, 0, MAX_DATA_CHUNK, 0); // create an empty GString attached to a preallocated 64k buffer

	GString strRequest;
	strRequest.SetFromUpTo(&sockBuffer[5]," ");
	if (strRequest.Compare("/MyLowLevelBasicPostHandler") == 0)
	{
		char *pzContentData = &sockBuffer[nHTTPHeaderLength];
		char *p1 = strstr(pzContentData,"One=");
		GString one;
		one.SetFromUpTo(p1+strlen("One="),"&"); // p1 + 4 bytes for "One="
		
		char *p2 = strstr(pzContentData,"Two=");
		GString two;
		two.SetFromUpTo(p2+strlen("Two="),"&"); // p1 + 4 bytes for "Two="

		char *p3 = strstr(pzContentData,"Three=");
		GString three;
		three.SetFromUpTo(p3+strlen("Three="),"&"); // p1 + 6 bytes for "Three="

		str2 << "One=[" << one << "]     Two=[" << two << "]    Three=[" << three << "]";
		HTTPSend(td->sockfd, str2, str2.Length(), "text/plain");
		
		goto KEEP_ALIVE;

	}
	// This concludes all the code required for a Basic POST handler
	// If you set [nBasicPOST] to 0 above, recompile then you will see the following code work


	////////////////////////////////////////////////////////////////////////////////////////////
	
	// [strRequest] must be "/MyLowLevelMultiPartPostHandler"
	if (strRequest.Compare("/MyLowLevelMultiPartPostHandler") != 0)
	{
		// if we don't know what this is we should close the connection
		goto CLOSE_CONNECT;
	}

	////////////////////////////////////////////////////////////////////////////////////////////

	LowLevelStaticController LLSC(nBytes, nContentLen-nHTTPContentAcquired, td->sockfd, sockBuffer);
	CMultiPartForm p(&LLSC, sockBuffer, sockBuffer + nHTTPHeaderLength);
	


	// usage sample 1, put the form parts into our own GStrings
	// if you are expecting something large you should PreAlloc the GString accordingly as is done with strFile1
	GString strCity, strStreet, strCountry, strSignature, strFile2;	
	GString strFile1(500000); // preallocate 500,000 bytes
	p.MapArgument("City"	,&strCity);
	p.MapArgument("Street"	,&strStreet);
	p.MapArgument("Country"	,&strCountry);
	p.MapArgument("Signature",&strSignature);
	p.MapArgument("UploadFile", &strFile1);
	p.MapArgument("UploadFile2",&strFile2);
	if ( p.GetAll() )
	{
		// the data has been put where it was mapped, the files have both been placed in GStrings


		str2 << "RIGHT CLICK IN THE BROWSER AND 'View Source'\r\n";
		str2 << "City=[" <<  strCity << "]\r\nStreet=[" << strStreet << "]\r\nCountry=[" << strCountry << "]\r\nSignature=[" << strSignature << "]\r\n";
		str2 << "UploadFile(" << strFile1.Length() << ")\r\n";
		str2 << "UploadFile2(" << strFile2.Length() << ")\r\n";
		
		HTTPSend(td->sockfd, str2, str2.Length(), "text/html");
		goto KEEP_ALIVE;

	}
	else
	{
		// printf( p.GetLastError() );
		 goto CLOSE_CONNECT;
	}


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

		str2 << "RIGHT CLICK IN THE BROWSER AND 'View Source'\r\n";
		str2 << "City=[" <<  strCity << "]\r\nStreet=[" << strStreet << "]\r\nCountry=[" << strCountry << "]\r\nSignature=[" << strSignature << "]\r\n";
		str2 << "The files were written to D:\\ as [File1.bin] and [File2.bin] \r\n";
		HTTPSend(td->sockfd, str2, str2.Length(), "text/html");
		goto KEEP_ALIVE;
	}
	else
	{
		//printf( p.GetLastError() );
		goto CLOSE_CONNECT;
	}

*/

/*

	// Usage sample 3, dynamically extract each piece of data, 
	// try posting small .txt files to this example then right click the browser and "view source" to see the results
	FormPart *pFormPart = 0;
	while( p.ReadNext(&pFormPart) )
	{
		str2 << "[" << pFormPart->strPartName << "]=[" << *pFormPart->strData << "]\r\n\r\n";
	}
	HTTPSend(td->sockfd, str2, str2.Length(), "text/html");
	goto KEEP_ALIVE;

*/

/*

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
			p.MapArgumentToFile("UploadFile2", strDestFile); // copies the second file to D:\\Antioch
		}
	}

*/

}
