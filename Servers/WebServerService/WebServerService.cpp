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
#include <Windows.h>



#define dimof(A)  (sizeof(A) / sizeof(A[0]))
HANDLE g_hIOCP = NULL;
enum COMPKEY 
{ 
   CK_SERVICECONTROL,   
   CK_PIPE              
};


#define _REENTRANT
#include <stdio.h>
#include <sys/types.h>
#include <string.h>

#include "GThread.h"


#define MAX_SOCKET_CHUNK	65536

#include <conio.h>
#include <winsock.h>


#if defined(_MSC_VER)
#ifdef _WIN32
#pragma comment(lib,    "../../Libraries/openssl/bin-win32/libeay32.lib")
#endif
#ifdef _WIN64
#pragma comment(lib,    "../../Libraries/openssl/bin-win64/libeay32.lib")
#endif
#endif

#include "../Core/ServerCore.cpp"


GString strServerName("5LoavesWebServer");
GString strServerDescription("5Loaves HTTP service");
GString strServerPassword;
char pzServer[256];

char *pzBoundStartupConfig = 
"[System]\r\n"
"Pool=20\r\n"
"ProxyPool=0\r\n"
"\r\n"
"[HTTP]\r\n"
"Enable=yes\r\n"
"Index=Index.html\r\n"
"Home=%s\r\n"
"Port=%s\r\n";



void WINAPI TimeServiceHandler(DWORD fdwControl) 
{
   // The Handler thread is very simple and executes very quickly because
   // it just passes the control code off to the ServiceMain thread.
   PostQueuedCompletionStatus(g_hIOCP, fdwControl, CK_SERVICECONTROL, NULL);
}


//////////////////////////////////////////////////////////////////////////////



#define SERVICE_CONTROL_RUN            0x00000000


DWORD dwSrvCtrlToPend[256] = {   // 255 is max user-defined code
   /* 0: SERVICE_CONTROL_RUN         */ SERVICE_START_PENDING, 
   /* 1: SERVICE_CONTROL_STOP        */ SERVICE_STOP_PENDING,
   /* 2: SERVICE_CONTROL_PAUSE       */ SERVICE_PAUSE_PENDING,
   /* 3: SERVICE_CONTROL_CONTINUE    */ SERVICE_CONTINUE_PENDING,
   /* 4: SERVICE_CONTROL_INTERROGATE */ 0, 
   /* 5: SERVICE_CONTROL_SHUTDOWN    */ SERVICE_STOP_PENDING,
   /* 6 - 255: User-defined codes    */ 0
};


DWORD dwSrvPendToState[] = { 
   /* 0: Undefined                */ 0,
   /* 1: SERVICE_STOPPED          */ 0,
   /* 2: SERVICE_START_PENDING    */ SERVICE_RUNNING,
   /* 3: SERVICE_STOP_PENDING     */ SERVICE_STOPPED, 
   /* 4: SERVICE_RUNNING          */ 0,
   /* 5: SERVICE_CONTINUE_PENDING */ SERVICE_RUNNING,
   /* 6: SERVICE_PAUSE_PENDING    */ SERVICE_PAUSED,
   /* 7: SERVICE_PAUSED           */ 0
};

static char *pPassPart = "This";
GString strPassPart("Is");
void MakePassword( GString strPassword )
{
	char *pPassPart = "One";
	int c1 ='N';
	int c2 ='o';
	int c3 ='t';

	c1 = 50 + 40; // 90 is the ASCII value of W
	c2 = 97;      // 97 is the ASCII value of a
	c3 = (50 * 2) + (7 * 3); // 121 is the ASCII value of y (thats 2 50's and 3 7's)

	// the password is "ThisIsOneWay"
	strPassword << pPassPart << strPassPart << pPassPart
		        << (char)c1 << (char)c2 << (char)c3;

	// there are many other ways to safely 'embed' your password
	// into this application so that a Hex editor cannot 'see it'.
	
}

const char *GetThisEXEName()
{
	static GString strThisEXE;
	if (strThisEXE.Length())
		return strThisEXE;

	// get the path and file name of this .exe
	// the user may have renamed the exe 
	char pzName[512];
	memset(pzName,0,512);
	GetModuleFileName(NULL,pzName,512);

	// pick the file name off the end of the path
	int nLen = strlen(pzName);
	while (1)
	{
		nLen--;
		if (pzName[nLen] == '\\')
		{
			strThisEXE = &pzName[nLen+1];
			break;
		}
		if (nLen < 1)
			break;
	}
	// remove the extension, normally ".exe"
	strThisEXE.TrimRightBytes(4);

	return strThisEXE;
}

void WINAPI TimeServiceMain(DWORD dwArgc, LPTSTR *lpszArgv) 
{
#ifdef _WIN64
   unsigned __int64 dwCompKey  = CK_SERVICECONTROL;
#else
   DWORD dwCompKey  = CK_SERVICECONTROL;
#endif
   DWORD fdwControl = SERVICE_CONTROL_RUN;
   DWORD dwBytesTransferred;
   OVERLAPPED *po;
   SERVICE_STATUS ss;
   SERVICE_STATUS_HANDLE hSS;
   BOOL bPasswordCompileEmbedded = 0;

// We must get the private password for connecting to this machine.
// The password can be compiled into the server any of the following ways......


//  1 way - the easiest way, just uncomment and put any value between the quotes
//  then remove the code between --Load External Password Begin & End-- below
//  ***** example of very simple integrated password ********
//  GString strPassword("Password");
//	GString strPort("80");
//	GString strRoot;
//	bPasswordCompileEmbedded = 1;


//  Another way - a most secure way to embed the password.
//  ***** example of integrated password ********
//  GString strPassword;
//	MakePassword(strPassword);    // Go read the MakePassword() routine above.
//	GString strPort("80");
//	GString strRoot;
//	bPasswordCompileEmbedded = 1;


//              OR

// The password can be retrieved from an external disk location.
// Passwords can be obtained from disk in two different ways.
// Either in a registry key matching the (probably renamed) 
// executable file that is this server, OR by loading a 
// predefined file name at a predefined location. 
// This file must be present while starting the server, but can be
// removed once this process has fully started.


//  --Load External Password Begin--
GString strPassword;
GString strPort;
GString strRoot;

if (!bPasswordCompileEmbedded)
{
	GString strThisEXEName(GetThisEXEName());
	GString strStartupKey;

	// use our runtime image name as the registry key
	char buf[512];
	long lSize = sizeof(buf);
	if (RegQueryValue(HKEY_CLASSES_ROOT,(const char *)strThisEXEName,buf,&lSize) == ERROR_SUCCESS)
	{
		// uudecode the startup key
		BUFFER b;
		BufferInit(&b);
		unsigned int nDecoded;
		uudecode(buf, &b, &nDecoded, false);
		strStartupKey.write((const char *)b.pBuf,nDecoded);
		BufferTerminate(&b);
	}

	GString strStartupData;
	if (strStartupKey.Length())
	{
		// look for a file in the root of the file system (c:\)
		// with the same name as this .exe
		GString strFile("c:\\");
		strFile += strThisEXEName;


		// load the crypted disk file into clear text memory
		char *pDest;
		int nDestLen;
		GString strErrorOut;
		if (FileDecryptToMemory(strStartupKey, strFile, &pDest, &nDestLen, strErrorOut))
		{
			// parse into the profile data structures
			pDest[7 + nDestLen] = 0; // null terminate it
			strStartupData.write(&pDest[7], nDestLen + 1); // and cap the GString 
		}
		else
		{
			// if the file was not in the root of the file system
			// see if there is an environment setting directing
			// this server to look for the file in another location
			// The variable name is dynamic, matching this .exe name
			// and the environment variable value must be a fully
			// qualified path and file name to the startup file.
			if (getenv(strThisEXEName))
			{
				if (FileDecryptToMemory(strStartupKey, getenv(strThisEXEName), &pDest, &nDestLen, strErrorOut))
				{
					// parse into the profile data structures
					strStartupData.write(&pDest[7], nDestLen-7);
				}
			}
		}

		// parse stored settings in startup file to startup variables
		if (strStartupData.Length())
		{
			GStringList lstOptions("&&",strStartupData);
			GStringIterator it(&lstOptions);

			if (it()) strPassword = it++;
			if (it()) strRoot = it++; // currently not used
			if (it()) strPort = it++;
		}
	}
}

//  --Load External Password End--

	GString strFile;
	GString strConfigFileDefault;
	GString strErrorOut;
	int bSetStartupFile = 0;

#ifdef _WIN32
	SetPriorityClass(GetCurrentProcess(),HIGH_PRIORITY_CLASS); // = 1 point above normal
#endif


   // Create the completion port and save its handle in a global
   // variable so that the Handler function can access it.
   g_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, CK_PIPE, 0);

   // Give SCM the address of this service's Handler
   // NOTE: hSS does not have to be closed.
   hSS = RegisterServiceCtrlHandler((const char *)strServerName, TimeServiceHandler);

   // Do what the service should do.
   // Initialize the members that never change
   ss.dwServiceType = SERVICE_WIN32_OWN_PROCESS; 
   ss.dwControlsAccepted = SERVICE_ACCEPT_STOP | 
      SERVICE_ACCEPT_PAUSE_CONTINUE | SERVICE_ACCEPT_SHUTDOWN;

   do {
      switch (dwCompKey) {
      case CK_SERVICECONTROL:
         // We got a new control code
         ss.dwWin32ExitCode = NO_ERROR; 
         ss.dwServiceSpecificExitCode = 0; 
         ss.dwCheckPoint = 0; 
         ss.dwWaitHint = 0;

         if (fdwControl == SERVICE_CONTROL_INTERROGATE) {
            SetServiceStatus(hSS, &ss);
            break;
         }

         // Determine which PENDING state to return
         if (dwSrvCtrlToPend[fdwControl] != 0) {
            ss.dwCurrentState = dwSrvCtrlToPend[fdwControl]; 
            ss.dwCheckPoint = 0;
            ss.dwWaitHint = 500;   // half a second
            SetServiceStatus(hSS, &ss);
         }

         switch (fdwControl) {
            case SERVICE_CONTROL_RUN:
            case SERVICE_CONTROL_CONTINUE:
				try
				{
					if (strRoot.Length() && strPort.Length())
					{
						GString strCfgData;
						strCfgData.Format(pzBoundStartupConfig,(const char *)strRoot,(const char *)strPort);
						SetProfile(new GProfile((const char *)strCfgData, (int)strCfgData.Length(), 0));

						if (!server_start())
						{
							ss.dwCurrentState = SERVICE_STOPPED; 
							ss.dwCheckPoint = ss.dwWaitHint = 0;
							SetServiceStatus(hSS, &ss);
							break;
						}
					}
					else
					{
						// No password compiled in - and no valid startup file found
						for(int i=0; i<3;i++)
						{
							// three beeps
							MessageBeep(0);
							Sleep(1000);
						}
						ss.dwCurrentState = SERVICE_STOPPED; 
						ss.dwCheckPoint = ss.dwWaitHint = 0;
						SetServiceStatus(hSS, &ss);
						break;
					}
				}
				catch ( GException &)
				{
					ss.dwCurrentState = SERVICE_STOPPED; 
					ss.dwCheckPoint = ss.dwWaitHint = 0;
					SetServiceStatus(hSS, &ss);
					break;
				}



				if (dwSrvPendToState[ss.dwCurrentState] != 0) 
				{
					ss.dwCurrentState = dwSrvPendToState[ss.dwCurrentState]; 
					ss.dwCheckPoint = ss.dwWaitHint = 0;
					SetServiceStatus(hSS, &ss);
				}
				break;

            case SERVICE_CONTROL_PAUSE:
            case SERVICE_CONTROL_STOP:
            case SERVICE_CONTROL_SHUTDOWN:
                server_stop();
				if (dwSrvPendToState[ss.dwCurrentState] != 0) 
				{
					ss.dwCurrentState = dwSrvPendToState[ss.dwCurrentState]; 
					ss.dwCheckPoint = ss.dwWaitHint = 0;
					SetServiceStatus(hSS, &ss);
				}
				break;
         }

         // Determine which complete state to return
         break;

      }

      if (ss.dwCurrentState != SERVICE_STOPPED) {
         // Sleep until a control code comes in or a client connects
         GetQueuedCompletionStatus(g_hIOCP, &dwBytesTransferred,
            &dwCompKey, &po, INFINITE);
         fdwControl = dwBytesTransferred;
      }
   } while (ss.dwCurrentState != SERVICE_STOPPED);

   // Cleanup and stop this service
   CloseHandle(g_hIOCP);   
}


typedef BOOL (CALLBACK* ChangeServiceConfig2Type)(SC_HANDLE,  DWORD ,  LPVOID);
void InstallService() 
{
   TCHAR szModulePathname[_MAX_PATH];
   SC_HANDLE hService;

   // Open the SCM on this machine.
   SC_HANDLE hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);

   // Get our full pathname
   GetModuleFileName(NULL, szModulePathname, dimof(szModulePathname));

   // Add this service to the SCM's database.
   hService = CreateService(hSCM, (const char *)strServerName, (const char *)strServerName, 0,
      SERVICE_WIN32_OWN_PROCESS, SERVICE_DEMAND_START, SERVICE_ERROR_IGNORE, 
      szModulePathname, NULL, NULL, NULL, NULL, NULL);
   CloseServiceHandle(hService);


	//  Set the service description in 2K & XP 
	//  --------------------------------------
    HINSTANCE dllHandle = LoadLibrary("advapi32");
	ChangeServiceConfig2Type fn = 0;;
	fn = (ChangeServiceConfig2Type)GetProcAddress(dllHandle,"ChangeServiceConfig2A");
	if (fn) 
	{
		hService = OpenService(hSCM,(const char *)strServerName,SERVICE_CHANGE_CONFIG);
		SERVICE_DESCRIPTION sd;
		sd.lpDescription=(char *)(const char *)strServerDescription;
		fn(hService, SERVICE_CONFIG_DESCRIPTION, &sd);
		CloseServiceHandle(hService);
	}


   // Close the service and the SCM
   CloseServiceHandle(hSCM);
}


//////////////////////////////////////////////////////////////////////////////


void RemoveService() {
   // Open the SCM on this machine.
   SC_HANDLE hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);

   // Open this service for DELETE access
   SC_HANDLE hService = OpenService(hSCM, (const char *)strServerName, DELETE);

   // Remove this service from the SCM's database.
   DeleteService(hService);

   // Close the service and the SCM
   CloseServiceHandle(hService);
   CloseServiceHandle(hSCM);
}



void ModifyStartupFile(GString &strStartupKey,GString &strServerPassword,GString &strRoot,GString &strPort)
{
	///////////////////////////////////////////////////////////////////
	// load the following values fron the startup file:
	//////////////////////////////////////////////////////////////////
	GString strCurrentPassword;
	GString strCurrentRoot;
	GString strCurrentPort;
	
	GString strThisEXEName(GetThisEXEName());
	GString strStartupData;
	if (strStartupKey.Length())
	{
		// look for a file in the root of the file system (c:\)
		// with the same name as this .exe
		GString strFile("c:\\");
		strFile += strThisEXEName;


		// load the crypted disk file into clear text memory
		char *pDest;
		int nDestLen;
		GString strErrorOut;
		if (FileDecryptToMemory(strStartupKey, strFile, &pDest, &nDestLen, strErrorOut))
		{
			// parse into the profile data structures
			pDest[7 + nDestLen] = 0; // null terminate it
			strStartupData.write(&pDest[7], nDestLen + 1); // and cap the GString 
		}
		else
		{
			// if the file was not in the root of the file system
			// see if there is an environment setting directing
			// this server to look for the file in another location
			// The variable name is dynamic, matching this .exe name
			// and the environment variable value must be a fully
			// qualified path and file name to the startup file.
			if (getenv(strThisEXEName))
			{
				if (FileDecryptToMemory(strStartupKey, getenv(strThisEXEName), &pDest, &nDestLen, strErrorOut))
				{
					// parse into the profile data structures
					strStartupData.write(&pDest[7], nDestLen-7);
				}
			}
		}

		// parse stored settings in startup file to startup variables
		if (strStartupData.Length())
		{
			GStringList lstOptions("&&",strStartupData);
			GStringIterator it(&lstOptions);
			
			if (it()) strCurrentPassword = it++;
			if (it()) strCurrentRoot = it++;
			if (it()) strCurrentPort = it++;
		}
	}
	//////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////


	// new values over write the old ones
	if (strServerPassword.Length())
		strCurrentPassword = strServerPassword;

	if (strPort.Length())
		strCurrentPort = strPort;

	if (strRoot.Length() && strRoot.CompareNoCase("none") != 0)
		strCurrentRoot = strRoot;
	else if (strRoot.CompareNoCase("none") == 0)
	{
		strCurrentRoot = "";
	}
	


	// use root of file system to store the startup file 
	// unless specified otherwise by environment setting
	GString strOutFile("c:\\");
	strOutFile += strThisEXEName;
	if (getenv(strThisEXEName))
	{
		strOutFile += getenv(strThisEXEName);
	}
	
	
	// create the new startup file
	GString strTempFile;
	strTempFile << strServerPassword << "&&" << strRoot << "&&" << strPort;
	strTempFile.ToFile("tempfile");
	GString strErrorOut;
	unlink(strOutFile);
	FileEncrypt(strStartupKey, "tempfile", strOutFile, strErrorOut);
	unlink("tempfile");
}



//////////////////////////////////////////////////////////////////////////////

int WINAPI WinMain(HINSTANCE hinst, HINSTANCE hinstExePrev, 
   LPSTR pszCmdLine, int nCmdShow) {


	// ***** To debug this application  ****
	// set fDebug to 1, then recompile (in debug mode)
	// and run the program in a debugger like any windows .exe
	// The exact same code executes as if the user clicked the 
	// 'start' button in the NT services manager.
	int fDebug = 0;


   int nArgc = __argc;
#ifdef UNICODE
   LPCTSTR *ppArgv = (LPCTSTR*) CommandLineToArgvW(GetCommandLine(), &nArgc);
#else
   LPCTSTR *ppArgv = (LPCTSTR*) __argv;
#endif

   BOOL fStartService = (nArgc < 2);
   int i;

   int bInstall = 0;
   int bRemove  = 0;
   int bChange  = 0;
   GString  strPort("10888");

   GString strBoot;
   GString strRoot("NotUsed");
   for (i = 1; i < nArgc; i++) 
   {
      if ((ppArgv[i][0] == __TEXT('-')) || (ppArgv[i][0] == __TEXT('/'))) 
	  {
         // Command line switch
         if (lstrcmpi(&ppArgv[i][1], __TEXT("install")) == 0) 
            bInstall = 1;

         if (lstrcmpi(&ppArgv[i][1], __TEXT("remove"))  == 0)
            bRemove = 1;

         if (lstrcmpi(&ppArgv[i][1], __TEXT("change"))  == 0)
            bChange = 1;
		 

         GString strTemp(&ppArgv[i][1],strlen("desc:"));
		 if (strTemp.CompareNoCase("desc:") == 0)
		 {
            strServerDescription = &ppArgv[i][1+strlen("desc:")];
		 }

         GString strTemp2(&ppArgv[i][1],strlen("name:"));
		 if (strTemp2.CompareNoCase("name:") == 0)
		 {
            strServerName = &ppArgv[i][1+strlen("name:")];
		 }

         GString strTemp3(&ppArgv[i][1],strlen("pass:"));
		 if (strTemp3.CompareNoCase("pass:") == 0)
		 {
            strServerPassword = &ppArgv[i][1+strlen("pass:")];
		 }

         GString strTemp4(&ppArgv[i][1],strlen("boot:"));
		 if (strTemp4.CompareNoCase("boot:") == 0)
		 {
            strBoot = &ppArgv[i][1+strlen("boot:")];
		 }

         GString strTemp5(&ppArgv[i][1],strlen("port:"));
		 if (strTemp5.CompareNoCase("port:") == 0)
		 {
            strPort = &ppArgv[i][1+strlen("port:")];
		 }

         GString strTemp6(&ppArgv[i][1],strlen("root:"));
		 if (strTemp6.CompareNoCase("root:") == 0)
		 {
            strRoot = &ppArgv[i][1+strlen("root:")];
		 }
      }
   }

	if (bChange)
	{
		ModifyStartupFile(strBoot,strServerPassword,strRoot,strPort);
	}

	if (bInstall)
	{
		GString strThisEXEName(GetThisEXEName());


		// uuencode strBoot 
		BUFFER b;
		BufferInit(&b);
		uuencode((unsigned char *)(const char *)strBoot, (int)strBoot.Length(), &b);
		GString strEncodedBoot((char *)b.pBuf, b.cLen);
		BufferTerminate(&b);

		// open the registry, save the coded boot key, close the registry
		HKEY hk;
		if (RegCreateKey(HKEY_CLASSES_ROOT,(const char *)strThisEXEName,&hk) == ERROR_SUCCESS)
		{
			RegSetValue(hk,NULL,REG_SZ,(const char *)strEncodedBoot,0);
			RegCloseKey(hk);
		}

		// use root of file system to store the startup file 
		// unless specified otherwise by environment setting
		GString strOutFile("c:\\");
		strOutFile += strThisEXEName;
		if (getenv(strThisEXEName))
		{
			strOutFile += getenv(strThisEXEName);
		}
		
		
		// create the startup file
		GString strTempFile;
		strTempFile << strServerPassword << "&&" << strRoot << "&&" << strPort;
		strTempFile.ToFile("tempfile");
		GString strErrorOut;
		FileEncrypt(strBoot, "tempfile", strOutFile, strErrorOut);
		unlink("tempfile");


		InstallService();
	}
	if (bRemove)
	{
		RemoveService();

		GString strThisEXEName(GetThisEXEName());

		// remove the registry entry
		RegDeleteKey(HKEY_CLASSES_ROOT,(const char *)strThisEXEName);

		// remove the startup file
		GString strOutFile("c:\\");
		strOutFile += strThisEXEName;
		if (getenv(strThisEXEName))
		{
			strOutFile += getenv(strThisEXEName);
		}
		unlink(strOutFile);

	}

   strcpy(pzServer,strServerName);


   if (fDebug) 
   {
      // Running as EXE not as service, just run the service for debugging
	  TimeServiceMain(0, NULL);
   }

   if (fStartService) {
      SERVICE_TABLE_ENTRY ServiceTable[] = {
         { pzServer, TimeServiceMain },
         { NULL,        NULL }   // End of list
      };
      StartServiceCtrlDispatcher(ServiceTable);
   }

   return(0);
}


//////////////////////////////// End Of File /////////////////////////////////
