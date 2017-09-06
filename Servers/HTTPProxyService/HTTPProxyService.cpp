// --------------------------------------------------------------------------
//						United Business Technologies
//			  Copyright (c) 2014   All Rights Reserved.
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


GString strServerName("5Loaves HTTPProxy"); // 5Loaves sorts toward the top of the services list so you see it on the first page in the services list, or as the first service
GString strServerDescription("Share your Internet connection via HTTP Proxy");
GString strServerPassword;
char pzServer[256];

char *pzBoundStartupConfig = 
"[System]\r\n"
"Pool=200\r\n"
"ProxyPool=200\r\n"
"LogFile=C:\\HTTPProxyLog.txt\r\n"

"[Trace]\r\n"
"Enable=yes\r\n"					
"HTTPProxyTrace=0\r\n"
"HTTPProxyReturnTrace=0\r\n"
"ConnectTrace=1\r\n"

"[HTTPProxy]\r\n"
"Enable=yes\r\n"
"HTTPProxyAgent=5LoavesHTTPProxy\r\n"
"BlockAds=1\r\n"			// most effiecient use of bandwidth on your LAN
"RequireAuth=0\r\n"
"Port=%s\r\n"				// variable #1, defaults to 10888
"RestrictIPs=1\r\n"			// this is not an open relay
"RestrictBeginMatch=%s\r\n" // variable #2 blocks open relay to specified subnet  defaults to "192.168"
"LogEnabled=0\r\n"
"LogFileName=\r\n"			// if LogEnabled is true set LogFileName=c:\MyLogFile.txt 
"Timeout=300\r\n"
"BlockedDomains=\r\n";		// the bad guys in a comma seperated list

/*
[HTTPProxy]
Enable=1
Port=8080
HTTPProxyAgent=InternetVia5Loaves
RequireAuth=1
Timeout=300
BlockedDomains=
LogEnabled=0
RestrictIPs=1
Only443or80=1
NegotiateBasic=1
BlockHTTPProxyAds=1
EnableOutboundConnectCache=0
NegotiateNTLMv2=1
LogHTTPTxnFiles=0
BlockGoogle=1
RestrictBeginMatch=10.0.;192.168.
LogFileName=C:\Users\theboss\Desktop\HTTPProxyLog.txt
*/




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

	GString strPort("10888");
	GString strRestrictBeginMatch("192.168;10.0.");

	GString strFile;
	GString strConfigFileDefault;
	GString strErrorOut;

   // Create the completion port and save its handle in a global variable so that the Handler function can access it.
   g_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, CK_PIPE, 0);

   // Give SCM the address of this service's Handler        NOTE: hSS does not have to be closed.
   hSS = RegisterServiceCtrlHandler((const char *)strServerName, TimeServiceHandler);

   // Do what the service should do.    Initialize the members that never change
   ss.dwServiceType = SERVICE_WIN32_OWN_PROCESS; 
   ss.dwControlsAccepted = SERVICE_ACCEPT_STOP |  SERVICE_ACCEPT_PAUSE_CONTINUE | SERVICE_ACCEPT_SHUTDOWN;

   do {
      switch (dwCompKey) {
      case CK_SERVICECONTROL:
         // We got a new control code
         ss.dwWin32ExitCode = NO_ERROR; 
         ss.dwServiceSpecificExitCode = 0; 
         ss.dwCheckPoint = 0; 
         ss.dwWaitHint = 0;

         if (fdwControl == SERVICE_CONTROL_INTERROGATE) 
		 {
            SetServiceStatus(hSS, &ss);
            break;
         }

         // Determine which PENDING state to return
         if (dwSrvCtrlToPend[fdwControl] != 0) 
		 {
            ss.dwCurrentState = dwSrvCtrlToPend[fdwControl]; 
            ss.dwCheckPoint = 0;
            ss.dwWaitHint = 500;   // half a second
            SetServiceStatus(hSS, &ss);
         }

         switch (fdwControl) 
		 {
            case SERVICE_CONTROL_RUN:
            case SERVICE_CONTROL_CONTINUE:
				try
				{
					// use our runtime image name as the registry key to obtain user setting that override the default settings
					GString strThisEXEName(GetThisEXEName());
					GString strCustomUserStartup;
					char buf[512];
					long lSize = sizeof(buf);
					if (RegQueryValue(HKEY_CLASSES_ROOT,(const char *)strThisEXEName,buf,&lSize) == ERROR_SUCCESS)
					{
						// uudecode the key value into strCustomUserStartup
						BUFFER b;
						BufferInit(&b);
						unsigned int nDecoded;
						uudecode(buf, &b, &nDecoded, false);
						strCustomUserStartup.write((const char *)b.pBuf,nDecoded);
						BufferTerminate(&b);
					}
					
					// make sure the startup data looks right
					if ( strCustomUserStartup.Length() && strCustomUserStartup.FindCaseInsensitive("port=") != -1 )
					{
						SetProfile(new GProfile((const char *)strCustomUserStartup, (int)strCustomUserStartup.Length(), 0));

						if (!server_start())
						{
							// failed to start with Custom startup settings, try defaults
						}
					}


					if ( strPort.Length() )
					{
						GString strCfgData;
						strCfgData.Format(pzBoundStartupConfig,(const char *)strPort,(const char *)strRestrictBeginMatch);
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
						// Missing startup arguments
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
				catch ( GException & )
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
   hService = CreateService(hSCM, (const char *)strServerName, (const char *)strServerName, 0, SERVICE_WIN32_OWN_PROCESS, SERVICE_DEMAND_START, SERVICE_ERROR_IGNORE,   szModulePathname, NULL, NULL, NULL, NULL, NULL);
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


	// now set it to auto-start
	// ------------------------------------------
	SC_HANDLE schService = OpenService( hSCM, strServerName, SERVICE_ALL_ACCESS); 
	if (schService)
	{
		if (! ChangeServiceConfig( 
				schService,        // handle of service 
				SERVICE_NO_CHANGE, // service type: no change 
				SERVICE_AUTO_START, // change service start type (SERVICE_AUTO_START = 2)
				SERVICE_NO_CHANGE, // error control: no change 
				NULL,			   // null is 'no change' or new binary path
				NULL,              // load order group: no change 
				NULL,              // tag ID: no change 
				NULL,              // dependencies: no change 
				NULL,              // null is 'no change' or new NT account name
				NULL,              // null is 'no change' or new password
				NULL) )            // display name: no change
		{
			// strResult << "Failed to set service config";
		}
		CloseServiceHandle(schService); 
	}
	CloseServiceHandle(hSCM);



	// Finally, - open the firewall
	// netsh advfirewall firewall add rule name="Xfer" dir=in action=allow program="C:\Users\The one\Desktop\Xfer Pro\Windows Service\X64.exe" enable=yes
	// ------------------------------------------
	GString strShellOpenFirewall("advfirewall firewall add rule name=\"");
	strShellOpenFirewall << strServerName << "\" dir=in action=allow program=\"" << szModulePathname << "\" enable=yes";
	
	GString strWindowsSystemNetShell(4096);					// pre-alloc a 4k GString
	GetSystemDirectory(strWindowsSystemNetShell._str,4096);	// Put GetSystemDirectory() output in our GString
	strWindowsSystemNetShell.SetLength(strlen(strWindowsSystemNetShell._str)); // now, tell GString the Length since we used it's (should be) private parts
	strWindowsSystemNetShell << "\\netsh.exe";
	
	GString strCommand( strWindowsSystemNetShell );
	SHELLEXECUTEINFO lpExecInfo;
	memset(&lpExecInfo, 0, sizeof(SHELLEXECUTEINFO));
	lpExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	lpExecInfo.lpFile = strCommand; 
	lpExecInfo.lpParameters = strShellOpenFirewall;
	lpExecInfo.lpDirectory = "";
	lpExecInfo.lpVerb = "open";
	lpExecInfo.nShow = SW_SHOW;
	lpExecInfo.fMask = 0;
	lpExecInfo.hwnd = GetDesktopWindow();
	ShellExecuteEx(&lpExecInfo); // note  Shell32.lib contains the ShellExecuteEx() implementation, 
}
//////////////////////////////////////////////////////////////////////////////
//	Needed for ShellExecuteEx()
	#pragma comment(lib, "Shell32.lib")	
//////////////////////////////////////////////////////////////////////////////

void StartService()
{
	SC_HANDLE schSCManager = OpenSCManager(NULL,SERVICES_ACTIVE_DATABASE,SC_MANAGER_CONNECT/*SC_MANAGER_ALL_ACCESS*/);
	SC_HANDLE schService = OpenService( schSCManager, strServerName, SERVICE_START); 
	if (!schService)
	{
		MessageBox(GetDesktopWindow(), "You must Run As Administrator to start services", "Service Message", MB_OK);
		CloseServiceHandle(schSCManager);
	}
	else
	{
		int nRet = StartService(schService,0,NULL);
		CloseServiceHandle(schService); 
		CloseServiceHandle(schSCManager);
	}
}
void StopService()
{
	SC_HANDLE schSCManager = OpenSCManager(NULL,SERVICES_ACTIVE_DATABASE,SC_MANAGER_CONNECT/*SC_MANAGER_ALL_ACCESS*/);
	SC_HANDLE schService = OpenService( schSCManager, strServerName, SERVICE_STOP/*SERVICE_ALL_ACCESS*/);
	if (!schService)
	{
		CloseServiceHandle(schSCManager);
	}
	else
	{
		SERVICE_STATUS sts;
		int nRet = ControlService(schService, SERVICE_CONTROL_STOP, &sts);
		CloseServiceHandle(schService); 
		CloseServiceHandle(schSCManager);
	}
}
void SetServiceAutoStartup(bool bAutoStart)
{
	// Open the SCM on this machine.
	SC_HANDLE hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS/*SC_MANAGER_CREATE_SERVICE*/);

	SC_HANDLE schService = OpenService( hSCM, strServerName, SERVICE_ALL_ACCESS); 
	if (schService)
	{
		if (! ChangeServiceConfig( 
				schService,        // handle of service 
				SERVICE_NO_CHANGE, // service type: no change 
				(bAutoStart) ? SERVICE_AUTO_START : SERVICE_DEMAND_START, // change service start type (SERVICE_AUTO_START = 2, SERVICE_DEMAND_START = 3)
				SERVICE_NO_CHANGE, // error control: no change 
				NULL,			   // null is 'no change' or new binary path
				NULL,              // load order group: no change 
				NULL,              // tag ID: no change 
				NULL,              // dependencies: no change 
				NULL,              // null is 'no change' or new NT account name
				NULL,              // null is 'no change' or new password
				NULL) )            // display name: no change
		{
			// strResult << "Failed to set service config";
		}
		CloseServiceHandle(schService); 
	}
	CloseServiceHandle(hSCM);
}



//////////////////////////////////////////////////////////////////////////////


void RemoveService() 
{
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





//////////////////////////////////////////////////////////////////////////////

int WINAPI WinMain(HINSTANCE hinst, HINSTANCE hinstExePrev, LPSTR pszCmdLine, int nCmdShow) 
{
	// ***** To debug this application  ****
	// set bDebug to 1, then recompile (in debug mode) and run the program in a debugger like any windows .exe
	// The exact same code executes as if the user clicked the 'start' button in the NT services manager.
	int bDebug = 0;


   int nArgc = __argc;
#ifdef UNICODE
   LPCTSTR *ppArgv = (LPCTSTR*) CommandLineToArgvW(GetCommandLine(), &nArgc);
#else
   LPCTSTR *ppArgv = (LPCTSTR*) __argv;
#endif

	BOOL bStartService = (nArgc < 2);
	int i;

	int bInstall = 0;
	int bRemove  = 0;
	int bChange  = 0;
	int bRun	= 0;
	int bStart	= 0;
	int bStop	= 0;
	int bAuto  = 0;
	int bManual  = 0;


   GString strBoot;
   GString  strPort("10888");
   GString strRestrictBeginMatch("192.168");

	for ( i = 1; i < nArgc; i++ ) 
	{
     // Command line switch
		if (lstrcmpi(&ppArgv[i][0], __TEXT("install")) == 0) 
			bInstall = 1;
		if (lstrcmpi(&ppArgv[i][0], __TEXT("remove"))  == 0)
			bRemove = 1;
		if (lstrcmpi(&ppArgv[i][0], __TEXT("run"))  == 0)
			bRun = 1;
		if (lstrcmpi(&ppArgv[i][0], __TEXT("start"))  == 0)
			bStart = 1;
		if (lstrcmpi(&ppArgv[i][0], __TEXT("stop"))  == 0)
			bStop = 1;
		if (lstrcmpi(&ppArgv[i][0], __TEXT("auto"))  == 0)
			bAuto = 1;
		if (lstrcmpi(&ppArgv[i][0], __TEXT("manual"))  == 0)
			bManual = 1;



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

		GString strTemp6(&ppArgv[i][1],strlen("allowsubnet:"));
		if (strTemp6.CompareNoCase("allowsubnet:") == 0)
		{
			strRestrictBeginMatch = &ppArgv[i][1+strlen("allowsubnet:")];
		}
   }

	if (bChange)
	{
		// change the startup settings - or uninstall and reinstall
	}
	if (bRun) // run will run the service in the current process 
	{
		bStartService = 1;
	}
	if (bStart) // start will use SCM to start a new process as a service
	{
		StartService();
		return 0;
	}
	if (bStop) // start will use SCM to start a new process as a service
	{
		StopService();
		return 0;
	}
	if (bManual)
	{
		SetServiceAutoStartup(0);
		return 0;
	}
	if (bAuto)
	{
		SetServiceAutoStartup(1);
		return 0;
	}

	if (bInstall)
	{
		GString strThisEXEName(GetThisEXEName());

		GString strBoot;
		strBoot.Format(pzBoundStartupConfig,(const char *)strPort,(const char *)strRestrictBeginMatch);


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

		InstallService();
	}
	if (bRemove)
	{
		RemoveService();

		GString strThisEXEName(GetThisEXEName());

		// remove the registry entry
		RegDeleteKey(HKEY_CLASSES_ROOT,(const char *)strThisEXEName);

	}

   strcpy(pzServer,strServerName);


   if (bDebug) 
   {
      // Running as EXE not as service, just run the service for debugging
	  TimeServiceMain(0, NULL);
   }

   if (bStartService) 
   {
      SERVICE_TABLE_ENTRY ServiceTable[] = {
         { pzServer, TimeServiceMain },
         { NULL,        NULL }   // End of list
      };
      StartServiceCtrlDispatcher(ServiceTable);
   }

   return(0);
}


//////////////////////////////// End Of File /////////////////////////////////
