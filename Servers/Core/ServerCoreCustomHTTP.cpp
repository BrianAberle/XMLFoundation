//
// Server Core HTTP Extension Example for when SERVERCORE_CUSTOM_HTTP is #defined prior to #include ServerCore.cpp
//

// There are many variables available to the current scope.  A few of exceptional interest are:
// [sockBuffer] the raw network data directly in the TCP kernel buffer
// [nContentLen] the Content-Length as reported from the HTTP header
// [nHTTPHeaderLength] byte count of the HTTP header
// [nHTTPContentAcquired] a POST may come in several network reads, this is the bytes of content not including HTTP header recv'd in the first read
// [nBytes] bytes in [sockBuffer] (note there may be more in transit if Content-Length > nBytes )
// [strConnectedIPAddress] the ip address of the client
// [td] a structure, td stands for "Thread Data"
// [td->sockfd] the socket handle.
// [td->nThreadID] a unique servicing thread number that you will need if you were to log status messages because you may 
//                 have the identical logic executing near simultaneously on another thread - you will need to distinguish 
//                 and this number is easier to look at(1 to the number in your pool) vs. looking at a thread handle.
// [td->buf1&2] You ought to take advantage of the memory management infrastructure provided in ServerCore because two 
//            rather large memory buffers have been allocated and are reserved for your use.  Never free() or delete() them.
//            Both buffers were allocated prior to the connect() and rapid memory allocations in high transaction volumne
//			  system will slow you down however if you use the thread "heaps" as they were designed to be used, you will find
//            that even when linking with excellent heap managers as provided by www.microquill.com that normally shave
//            valuable time from transaction execution - it will not help you here because runtime allocations are
//            nearly nonexisting so it's already as fast as it can possibly be.  
//            Both buffers are MAX_DATA_CHUNK bytes long.  MAX_DATA_CHUNK is slightly over 64kb.
//			  also an interesting fact is that td->Buf1 == sockBuffer
//			  To use the thread memory buffers you may want to attach the memory to a GString like this.
//            GString str1(td->Buf1, 0, MAX_DATA_CHUNK, 0); // create an empty GString attached to a preallocated 64k buffer
//            GString str2(td->Buf2, 0, MAX_DATA_CHUNK, 0); // create an empty GString attached to a preallocated 64k buffer
//			  Use str1 and str2 to contain the request, and to build your response in and your server will be very fast.
// Finally, Note that the 2 buffers are actually 1 buffer divided in half, the lower address is safe for 128k, or MAX_DATA_CHUNK * 2

g_TotalHits++; // this is not required, it's just interesting and you can see it with the shell command: hits
// Note: hold F5 in the browser for a while then look at the hit count.  
// Implement the identical application in IIS or Apache. Race them. Publish the results.

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
	// You might call each 'web page' a 'function' or transaction in your own code.
	// Here I will keep it inline for example simplicity.
	GString Page1("/Page1.html");
	GString Page2("/Page2.html");
	GString Page3("/Page3.html");
	GString Page4("/Page4.html");

	if (Page1.CompareNoCase(strRequest) == 0) // check the first few bytes of strRequest to see if they match a command we know
	{
		// send back an HTML page with a basic HTTP POST form.  Notice we set the page title to be [g_MyExampleGlobalString]
		// that variable is always up-to-date with the GProfile setting mapped to it.  In most applications you dont need to
		// bother with adding an entry in ServerCoreCustomProfileSetNotify.cpp such as been added so that [g_MyExampleGlobalString]
		// is always up-to-date WITHOUT adding this this code here:
		//
		// g_MyExampleGlobalString = GetProfile().GetStringOrDefault("CustomHTTP","ExampleString",0);
		//
		// That would keep [g_MyExampleGlobalString] up-to-date as well, and is an acceptable simpler solution
		// if the highest possible performance is not a requirement for your implementation, because if you have
		// many variables that you constantly query the most recent value for, it can become an issue - therefore
		// If you register a 'trigger' in GProfile, it will update your variable only when changed and you can always
		// access it directly and have the current value ready to use.  And inversely, for rarely checked values
		// simply query the profile and dont bother setting up an entry in ServerCoreCustomProfileSetNotify.cpp
		// Likewise, if you assign [g_MyExampleGlobalString] only 1 time in ServerCoreCustomServerStart.cpp and do 
		// NOT allow the value to change (like the ThreadPool sizes) then set the value globally 1 time at startup.
		GString strHTMLPage; 
		strHTMLPage << "<HTML><HEAD><TITLE>" << g_MyExampleGlobalString << "</TITLE><BODY>"
		"<FORM method=post Action=\"/MyLowLevelBasicPostHandler\">"
		"<P><br>Enter some text<INPUT name=One>"
		"<P><br>Enter more text<INPUT name=Two>"
		"<P><br>and a third time<INPUT name=Three>"
		"<INPUT type=submit value=Submit name=B1>"
		"<INPUT type=reset value=Reset name=B2></P></FORM></BODY></HTML>";

		HTTPSend(td->sockfd, strHTMLPage, strHTMLPage.Length(), "text/html");
		goto KEEP_ALIVE;
	} 
	else if (Page2.CompareNoCase(strRequest) == 0)
	{
		// For the sake of example we can return the entire HTTP request that are on the network buffer
		// so that you can see how the HTTP headers and data are sent over the wire and browser or 
		// HTTP gateway data in the headers.  In your custom server you can act on any of this information as well.
		
		// Sending the response back through HTTPSend() will build the HTTP headers for us wrapping the HTTP headers 
		// in our TCP buffer as the data, the length of which will be the Content-Length in the message we generate
		GString strHTMLPage("FYI: When your browser asked for Page2.html, this is what it sent over the wire:\r\n\r\n");
		strHTMLPage.Write(sockBuffer, nBytes);

		HTTPSend(td->sockfd, strHTMLPage, strHTMLPage.Length(), "text/plain");
		goto KEEP_ALIVE;
	}
	else if (Page3.CompareNoCase(strRequest) == 0)
	{
		// multi-part POST example
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
		goto KEEP_ALIVE;
	}
	else if (Page4.CompareNoCase(strRequest) == 0)
	{
		// multi-part POST example identical to Page 3 except that [MyLowLevelMultiPartPostHandler] is [foo]
		const char *pzHTMLPage = 
		"<HTML><HEAD><TITLE>Send File</TITLE><BODY>"
		"<FORM id=Upload name=Upload method=post Action=\"/foo\" encType=multipart/form-data>"
		" Select VERY SMALL .TXT file<INPUT id=Location name=City Value=\"Antioch\" Type=Hidden>"
		"<INPUT id=UploadFile style=\"width: 364; height: 23\" type=file name=UploadFile>"
		"<br><br>Add Text to send<INPUT id=Location name=Street Value=\"Send some text between the 2 files\" size=\"51\">"
		"<INPUT id=Location name=Country Type=Hidden> "
		"<INPUT id=Location name=Signature Value=\"777\" Type=Hidden> <br> <br>  &nbsp;Another SMALL TXT file"
		"<INPUT id=UploadFile2 style=\"width: 372; height: 23\" type=file name=UploadFile2>"
		"<br><p>There is code between the lines.</p> <DIV style=\"WIDTH: 500px\" align=right> <SPAN class=LittleText></SPAN>"
		"<INPUT type=submit value=Submit name=Submit></DIV>"
		"</FORM><p>&nbsp;</p></BODY></HTML>";
	
		HTTPSend(td->sockfd, pzHTMLPage, strlen(pzHTMLPage), "text/html");
		goto KEEP_ALIVE;
	}
	else // they asked for a page we dont know about
	{
		// we could close this connection and return this thread to the pool
		// so that all system resources are immediately returned to the pool awaiting the next connection.
		// You may optionally set the nCloseCode to any value that you like, and the value you set will be
		// present in connection trace logs when [Trace]ConnectTrace=1.  The nCloseCode value will have no 
		// effect on server bahavior.
		// nCloseCode =7000; 
		// goto CLOSE_CONNECT;

		// you also have the option to allow the logic flow to return naturally (without a goto)
		// but ONLY if you did not handle this transaction.  If you generated a response with HTTPSend() 
		// then you MUST return with a goto, otherwise the standard HTTP handler will also generate a response
		// Suppose in this example, we dont want the HTTP server to ever serve any file from disk
		GString strError("HTTP/1.1 404 Not Found\r\n");
		GString strHTML;
		strHTML.Format("<HTML><HEAD><TITLE>404 Not Found</TITLE></HEAD><BODY><H1>404 Not Found</H1>The requested URL <code>%s</code> was not found on this server.</BODY></HTML>", strRequest );
		SendHTTPNoFileResponse(td->sockfd, strError, strHTML);
		nCloseCode =7000; // this will show up in the logging
		goto CLOSE_CONNECT;  // You cant hook IIS or Apache this low, This core is hooked by the throat and ass.
	}


	// alternative to HTTPSend() we could setup the HTTP header ourselves and call nonblocksend()
	//
	// int nonblocksend(int fd,const char *pData,int nDataLen);
	//
	// HTTP commands are "transactional" unlike a protocol like VNC or Telnet that is "conversational".  
	// Each GET/POST gets a response and that is one complete transaction.  It may be the end of 
	// the connection too unless the HTTP Server uses "HTTP Keep Alives" described in the HTTP 
	// Specification.  This 'goto KEEP_ALIVE' will cause this thread to expect more transactions from 
	// the same connection so this thread will not return to the thread pool until it gets the next 
	// transaction or times out (120 seconds is default).  It should also be noted that connections 
	// are commonly broken by clients,proxies,and even ISP's after a complete HTTP transaction so you 
	// should not assume that the next time this client sends you a transaction it will be on this thread, 
	// but if it is on this thread then it will service the connection a faster because TCP did not 
	// need to re accept() the connection like it would if you 'goto CLOSE_CONNECT', so the behavior 
	// should be identical no matter where you goto  KEEP_ALIVE or  CLOSE_CONNECT if the protocol is HTTP.  
	// This handler will work for ANY protocol so don't think like you are in a box or something just 
	// because of the file name.

}
// Begin the main logic branch for your HTTP POST handler
else if (memcmp(sockBuffer,"POST /",6) == 0)
{
    GString strHTTPReturn(td->Buf2, 0, MAX_DATA_CHUNK, 0); // create an empty GString attached to a preallocated 64k buffer

	GString strRequest;
	strRequest.SetFromUpTo(&sockBuffer[5]," ");
	
	// search this file for MyLowLevelBasicPostHandler, it is defined in the HTML above
	if (strRequest.Compare("/MyLowLevelBasicPostHandler") == 0)  
	{
		// likewise the variable names One,Two,and Three were defined in the HTML above
		char *pzContentData = &sockBuffer[nHTTPHeaderLength];
		char *p1 = strstr(pzContentData,"One=");
		GString one;
		one.SetFromUpTo(p1+strlen("One="),"&"); // p1 + 4 bytes for strlen("One=")
		
		char *p2 = strstr(pzContentData,"Two=");
		GString two;
		two.SetFromUpTo(p2+strlen("Two="),"&"); // p1 + 4 bytes for "Two="

		char *p3 = strstr(pzContentData,"Three=");
		GString three;
		three.SetFromUpTo(p3+strlen("Three="),"&"); // p1 + 6 bytes for	strlen("Three=")

		strHTTPReturn << "call MyLowLevelBasicPostHandler( " << one << ", " << two << ", " << three << " )\n";

		// printf on the server what we are about to send to the web browser
		// this is where generally you write your own MyLowLevelBasicPostHandler(one,two,three);
		printf(strHTTPReturn);	

		HTTPSend(td->sockfd, strHTTPReturn, strHTTPReturn.Length(), "text/plain");
		
		goto KEEP_ALIVE;  

	}
	// This concludes all the code required for a Basic POST handler
	// If you set [nBasicPOST] to 0 above, recompile then you will see the following code work


	////////////////////////////////////////////////////////////////////////////////////////////
	
	if (strRequest.Compare("/MyLowLevelMultiPartPostHandler") == 0)
	{
		// Here is a very handy class that does all of the work for you with multi-part-forms
		LowLevelStaticController LLSC(nBytes, nContentLen-nHTTPContentAcquired, td->sockfd, sockBuffer);
		GMultiPartForm p(&LLSC, sockBuffer, sockBuffer + nHTTPHeaderLength);
		

		// usage sample 1, put the HTTP Multi-Form parts into GStrings  (Example 2 puts them in files)
		// if you are expecting something large you should PreAlloc the GString accordingly
		GString strCity, strStreet, strCountry, strSignature;
		GString strFile1(500000); // You should preallocate for a file size you expect
		GString strFile2(500000); 
		
		// the variable names City, Street, Country, Signature, UploadFile, and UploadFile2 are defined in the HTML above
		p.MapArgument("City"	,&strCity);
		p.MapArgument("Street"	,&strStreet);
		p.MapArgument("Country"	,&strCountry);
		p.MapArgument("Signature",&strSignature);

		// The POSTed files can be mapped either way - to GStrings or to files
		p.MapArgument("UploadFile", &strFile1);
		p.MapArgument("UploadFile2",&strFile2);
		//p.MapArgumentToFile("UploadFile", "c:\\log\\File1.txt");
		//p.MapArgumentToFile("UploadFile2","c:\\log\\File2.txt");

		if ( p.GetAll() ) // this gets everything
		{
			// the data has been put where it was mapped, the files have both been placed in GStrings

			strHTTPReturn << "City=[" <<  strCity << "]\r\nStreet=[" << strStreet << "]\r\nCountry=[" << strCountry << "]\r\nSignature=[" << strSignature << "]\r\n";
			strHTTPReturn << "UploadFile Bytes(" << strFile1.Length() << ")\r\n";
			strHTTPReturn << "UploadFile2 Bytes(" << strFile2.Length() << ")\r\n";
			
			HTTPSend(td->sockfd, strHTTPReturn, strHTTPReturn.Length(), "text/plain");
			goto KEEP_ALIVE;

		}
		else
		{
			 printf( p.GetLastError() );
			 goto CLOSE_CONNECT;
		}
	}
	else if (strRequest.Compare("/foo") == 0)
	{
		// Here is a very handy class that does all of the work for you with multi-part-forms
		LowLevelStaticController LLSC(nBytes, nContentLen-nHTTPContentAcquired, td->sockfd, sockBuffer);
		GMultiPartForm p(&LLSC, sockBuffer, sockBuffer + nHTTPHeaderLength);

		// Another usage sample: dynamically extract each piece of data, 
		// try posting small .txt files to this example 
		FormPart *pFormPart = 0;
		while( p.ReadNext(&pFormPart) )
		{
			strHTTPReturn << "[" << pFormPart->strPartName << "]=[" << *pFormPart->strData << "]\r\n\r\n";
		}
		HTTPSend(td->sockfd, strHTTPReturn, strHTTPReturn.Length(), "text/plain");
		goto KEEP_ALIVE;
	}
	else
	{
		// if we don't know what this is we should close the connection
		goto CLOSE_CONNECT;
	}

	////////////////////////////////////////////////////////////////////////////////////////////

/*
	// usage sample 3, dynamically extract each piece of data, 
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
