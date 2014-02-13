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
// --------------------------------------------------------------------------

#include <Windows.h>
//////////////////////////////////////////////////////////////////////////////
#define dimof(A)  (sizeof(A) / sizeof(A[0]))
#ifdef ___XFER
char g_szAppName[] = "Xfer";
#else
char g_szAppName[] = "5Loaves";
#endif
HANDLE g_hIOCP = NULL;
// The completion port wakes for 1 of 2 reasons:
enum COMPKEY { 
   CK_SERVICECONTROL,   // A service control code
   CK_PIPE              // A client connects to our pipe
};
//////////////////////////////////////////////////////////////////////////////


#define _REENTRANT
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include "GThread.h"


// 64 kb max socket read
#define MAX_SOCKET_CHUNK	65536

#include <conio.h>
#include <winsock.h>


//#define ADD_REMOTE_WS
#include "../Core/ServerCore.cpp"

SERVICE_STATUS ss;
SERVICE_STATUS_HANDLE hSS;


GString g_strPassword;
GString g_strFinalRunTimePassword;

#ifdef ___XFER
GString strServerName("Xfer");
GString strServerDescription("Xfer Protocol");
#else
GString strServerName("5Loaves");
GString strServerDescription("FiveLoaves");
#endif

char pzServer[256];


void WINAPI TimeServiceHandler(DWORD fdwControl) {
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


// 
BOOL ThreadInteract(GString *pstrMessage)
{
  HDESK   hdeskCurrent;
  HDESK   hdesk;
  HWINSTA hwinstaCurrent;
  HWINSTA hwinsta;

  // 
  // Save the current Window station
  // 
  hwinstaCurrent = GetProcessWindowStation();
  if (hwinstaCurrent == NULL)
     return FALSE;

  // 
  // Save the current desktop
  // 
  hdeskCurrent = GetThreadDesktop(GetCurrentThreadId());
  if (hdeskCurrent == NULL)
     return FALSE;

  // 
  // Obtain a handle to WinSta0 - service must be running
  // in the LocalSystem account
  // 
  hwinsta = OpenWindowStation("winsta0", FALSE,
                              WINSTA_ACCESSCLIPBOARD   |
                              WINSTA_ACCESSGLOBALATOMS |
                              WINSTA_CREATEDESKTOP     |
                              WINSTA_ENUMDESKTOPS      |
                              WINSTA_ENUMERATE         |
                              WINSTA_EXITWINDOWS       |
                              WINSTA_READATTRIBUTES    |
                              WINSTA_READSCREEN        |
                              WINSTA_WRITEATTRIBUTES);
  if (hwinsta == NULL)
     return FALSE;

  // 
  // Set the windowstation to be winsta0
  // 
  if (!SetProcessWindowStation(hwinsta))
     return FALSE;

  // 
  // Get the default desktop on winsta0
  // 
  hdesk = OpenDesktop("default", 0, FALSE,
                        DESKTOP_CREATEMENU |
				        DESKTOP_CREATEWINDOW |
                        DESKTOP_ENUMERATE    |
                        DESKTOP_HOOKCONTROL  |
                        DESKTOP_JOURNALPLAYBACK |
                        DESKTOP_JOURNALRECORD |
                        DESKTOP_READOBJECTS |
                        DESKTOP_SWITCHDESKTOP |
                        DESKTOP_WRITEOBJECTS);
   if (hdesk == NULL)
           return FALSE;

   // 
   // Set the desktop to be "default"
   // 
   if (!SetThreadDesktop(hdesk))
           return FALSE;


   
   MessageBox(GetDesktopWindow(), (const char *)*pstrMessage, "5Loaves Service Message", MB_OK);
   delete pstrMessage;
   

	

   // 
   // Reset the Window station and desktop
   // 
   if (!SetProcessWindowStation(hwinstaCurrent))
           return FALSE;

   if (!SetThreadDesktop(hdeskCurrent))
      return FALSE;

   // 
   // Close the windowstation and desktop handles
   // 
   if (!CloseWindowStation(hwinsta))
      return FALSE;

   if (!CloseDesktop(hdesk))
           return FALSE;

      return TRUE;
} 










int g_ServiceisRunning = 1;
BOOL ThreadConsole(void)
{
	//  Shared service objects(syncronization, mmfile, named objects) need a NULL DACL
	PSECURITY_DESCRIPTOR    pSD;
	SECURITY_ATTRIBUTES sa;
	pSD = (PSECURITY_DESCRIPTOR) LocalAlloc( LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
	if (!InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION))
	{
		unsigned long iID;
		GString *pG = new GString("Failed InitializeSecurityDescriptor");
		CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)ThreadInteract,NULL,0,&iID);
		return 1000;
	}
	if (!SetSecurityDescriptorDacl(pSD, TRUE, (PACL) NULL, FALSE)) 	// Add a NULL DACL to the security descriptor..
	{
		unsigned long iID;
		GString *pG = new GString("Failed SetSecurityDescriptorDacl");
		CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)ThreadInteract,NULL,0,&iID);
		return 1001;
	}
	sa.nLength              = sizeof(sa);
	sa.bInheritHandle       = TRUE;
	sa.lpSecurityDescriptor = pSD; 

	
	//  Shared Process Memory
	HANDLE hFileMapping = CreateFileMapping(INVALID_HANDLE_VALUE, &sa, PAGE_READWRITE, 0, 65535, "Console_Command" );
    if ( hFileMapping == 0 )
    {
		unsigned long iID;
		GString *pG = new GString("Failed CreateFileMapping");
		CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)ThreadInteract,NULL,0,&iID);
		return 1002;
    }
    char *pMemFile = (char *)MapViewOfFile( hFileMapping, FILE_MAP_WRITE, 0, 0, 0);
    if ( pMemFile == 0 )
    {
		unsigned long iID;
		GString *pG = new GString("Failed MapViewOfFile");
		CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)ThreadInteract,NULL,0,&iID);
        CloseHandle(hFileMapping);
        return 1003;
    }




	while(g_ServiceisRunning)
	{
		const char *pzData = pMemFile;
		if (memcmp(pzData,">>",2) == 0)
		{
			pzData += 2;
			GString strCiphered(pzData);
			GString strDeCiphered;
			DeHexUUCipher(&strCiphered, &strDeCiphered, g_strFinalRunTimePassword);
			pzData = strDeCiphered;
			char *pSep = (char *)strpbrk(pzData,"|");
			if (pSep)
			{
				*pSep = 0;
				int nStrLen = strlen(pzData);
				for(int i=0; i<nStrLen;i++)
				{
					if(!isdigit(pzData[i]))
					{
						memcpy(pMemFile,"<<Bad Format",12);
						continue;
					}
				}
				pzData = pSep + 1;
			}


			GString strCmdIn(pzData);

			pzData += strCmdIn.Length() + 1;
			GString strResponse;

			if (strCmdIn.CompareNoCase("at") == 0 )
			{
				showActiveThreads(&strResponse);
			}
			else if (strCmdIn.CompareNoCase("hp") == 0 ) // Halt port
			{
				GString strAll("all");
				if (strAll.CompareNoCase(pzData) == 0)
				{
					stopListeners();
					strResponse << "Done.";
				}
				else
				{
					int nRet = stopListeners( atoi(pzData) );
					if(nRet == 0)
					{
						strResponse << "Not stopped.  You can only stop a defined port.\nUse the 'vp' command to view defined ports.";
					}
					else
					{
						strResponse << "Done.";
					}
				}
			}
			else if (strCmdIn.CompareNoCase("vp") == 0 ) // view defined ports
			{
				viewPorts(&strResponse);
			}
			else if (strCmdIn.CompareNoCase("rp") == 0 ) // resume port
			{
				GString strAll("all");
				if (strAll.CompareNoCase(pzData) == 0)
				{
					startListeners(3);
					strResponse << "Done.  use 'vp' command to see results.";
				}
				else
				{
					int nRet = startListeners(3, atoi(pzData) );
					if (nRet == 0)
					{
						strResponse << "Not started.  You can only start a defined port.\nUse the 'vp' command to view defined ports.";
					}
					else
					{
						strResponse << "Done.  use 'vp' command to see results.";
					}
				}
			}
			else if (strCmdIn.CompareNoCase("sl") == 0 ) // Stop Ststem Log
			{
				g_NoLog = 1;
				strResponse << "System logging stopped.";
			}
			else if (strCmdIn.CompareNoCase("rl") == 0 ) // Resume Ststem Log
			{
				g_NoLog = 0;
				strResponse << "System logging resumed.";
			}
			else if (strCmdIn.CompareNoCase("Kill") == 0 ) 
			{
				int nTid = atoi(&pzData[2]);
				int nRet = KillTid (nTid);
				if (nRet == 0)
				{
					strResponse << "Done.  The a new thread now owns that tid";
				}
				else if (nRet == 1)
				{
					strResponse << "Invalid ThreadID:" << nTid;
				}
				else
				{
					strResponse << "Thread not killed, reason:" << nRet;
				}
			}
			else if (strCmdIn.CompareNoCase("mc") == 0 ) // MasterConfig
			{
				const char *pzSection = pzData;
				int nLen = strlen(pzSection);
				if (nLen)
				{
					const char *pzEntry = &pzSection[nLen + 1];
					nLen = strlen(pzEntry);
					if (nLen)
					{
						const char *pzValue = &pzEntry[nLen + 1];
						if(*pzValue)
						{
							GetProfile().SetConfig(pzSection, pzEntry, pzValue);
							strResponse << "New entry value set.[" << pzSection <<"][" << pzEntry << "][" << pzValue << "]";
						}
						else
						{
							strResponse << "Value NOT set.  required 3 arguments[Section,Entry, and Value] not supplied.";
						}
					}
					else
					{
						strResponse << "Value NOT set.  required arguments[Section,Entry, and Value] not supplied.";
					}
				}
				else
				{
					strResponse << "Value NOT set.  required arguments[Section,Entry, and Value] not supplied.";
				}
			}
			else if (strCmdIn.CompareNoCase("stop") == 0 ) 
			{
				stopListeners(); // no new transactions
				
				if (pzData[0] != 'I')
				{
					showActiveThreads(&strResponse);
				}
				else
				{
	                server_stop();

					ss.dwCurrentState = SERVICE_STOPPED; 
					ss.dwCheckPoint = ss.dwWaitHint = 0;
					SetServiceStatus(hSS, &ss);
					CloseHandle(g_hIOCP);

					strResponse << "Shut Down";
					g_ServiceisRunning = 0; // end this thread too
				}
			}
			else
			{
				strResponse << "Unsupported command [" << strCmdIn << "]";
			}



			GString strIPCMessage;
			strIPCMessage << rand() << rand() << "|";
			strIPCMessage.write(strResponse,strResponse.Length()+1);
			strIPCMessage << rand() << rand();

			GString strIPCData;
			EnHexUUCipher(&strIPCMessage, &strIPCData, g_strFinalRunTimePassword);

			memcpy(pMemFile,"<<",2);
			memcpy(&pMemFile[2],(const char *)strIPCData,(int)strIPCData.Length()+1);
		}
		else
		{
			Sleep(250);
		}
	}

    if ( pMemFile )
        UnmapViewOfFile( pMemFile );

    if ( hFileMapping )
        CloseHandle( hFileMapping );

	return 0;
}
////////////////////////////////////////////////////////////////








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

	// There are even ways to keep binary debuggers from finding
	// the call to MakePassword() and FileDecryptToMemory() so 
	// that the memory location of the assembled password is not so  
	// easy to find for the brief time that it exists.
}


// Start up from the encrypted config file if possible, otherwise look for 
// 5Loaves.txt for an unciphered startup file, first in the current folder
// then in the root of the filesystem at last resort.
void WINAPI FiveLoavesServiceMain(DWORD dwArgc, LPTSTR *lpszArgv) 
{
#ifdef _WIN64
	unsigned __int64 dwCompKey  = CK_SERVICECONTROL;
#else
	DWORD dwCompKey  = CK_SERVICECONTROL;
#endif
	DWORD fdwControl = SERVICE_CONTROL_RUN;
	DWORD dwBytesTransferred;
	OVERLAPPED *po;

	SetProcessShutdownParameters(0x100,SHUTDOWN_NORETRY);
	
//  Attempt to get the password for decrypting the startup 
//  arguments that are stored in an external disk file.
	GString strPassword;

//  If the password was passed in as the service was started
//  from the command line, use that.
	if (g_strPassword.Length())
	{
		strPassword = g_strPassword;
	}
//  OR if the password was supplied by the service control manager	
	else if (dwArgc == 2)
	{
		strPassword = lpszArgv[1];
	}

//  OR just uncomment and put any value between the quotes
//  to 'hardcode' a password into your custom build of this service
//  ***** example of very simple integrated password ********
//	strPassword = "MyPassword";


//  OR Another more secure way to embed the password.
//  ***** example of integrated password ********
//	MakePassword(strPassword);



	// If no password was obtained, set a default.
	if (strPassword.IsEmpty())
		strPassword = "Password";

	// used by the "console"
	g_strFinalRunTimePassword = strPassword;

	GString strFile;
	GString strErrorOut;
	int bSetStartupFile = 0;


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

		 // get Dynamic Environment Alternate Startup File
		 char pzName[512];
		 memset(pzName,0,512);
		 GetModuleFileName(NULL,pzName,512);
		 GString strSameDirStartupFile(pzName);
		 strSameDirStartupFile.TrimRightBytes(4);
		 GString strDEASF(GDirectory::LastLeaf(strSameDirStartupFile));
		 if (strDEASF.CompareNoCase("5LoavesSvc") == 0)
		 {
			strDEASF << (long)GetTickCount() << (long)GetCurrentThread();
		 }

         switch (fdwControl) {
            case SERVICE_CONTROL_RUN:
            case SERVICE_CONTROL_CONTINUE:
				try
				{
					// look for a ciphered startup file first
					
					// if a (unique name) environment assigned location is set, use that
					if (getenv(strDEASF))
					{
						strFile = getenv(strDEASF);
					}
					else
					{
						// if a startup file equal to the name of this executable 
						// name (minus the extension) in the same directory, use that
						FILE *fpTst = fopen(strSameDirStartupFile,"r");
						if (fpTst)
						{
							fclose(fpTst);
							strFile = strSameDirStartupFile;
						}
						else // otherwise try the default root location
						{	 // if it's not there we'll look for an unciphered startup file.
							strFile = "c:\\5LoavesStartup.bin";
						}
					}



					// decrypt disk file into memory
					char *pDest;
					int nDestLen;
					try
					{
						if (FileDecryptToMemory(strPassword, strFile, &pDest, &nDestLen, strErrorOut))
						{
							// parse into the profile data structures
							SetProfile(new GProfile(&pDest[7], nDestLen));
							delete pDest;
							bSetStartupFile = 1;
						}
					}
					catch ( GException &)
					{
					}
					
					// OK, give up looking for a ciphered startup file
					// look for a clear text startup file
					if (!bSetStartupFile)
					{
						// lop off the file name (all bytes trailing the last slash of path )
						strSameDirStartupFile = 
						strSameDirStartupFile.Left(strSameDirStartupFile.ReverseFind('\\') + 1);
						// then tack on the name of the text startup file
						strSameDirStartupFile += "5Loaves.txt";
						// if the file exists and is readable by opening it.
						FILE *fp = fopen(strSameDirStartupFile,"r");
						if (fp)
						{
							fclose(fp); // it is
							strFile = strSameDirStartupFile;
						}
						else
						{
							// Use the default
							strFile = "c:\\5Loaves.txt";
						}
						
						
						
						// load the disk file into memory
						GString strCfgData;
						strCfgData.FromFile(strFile,0);
						if (strCfgData.Length())
						{
							SetProfile(new GProfile((const char *)strCfgData, (int)strCfgData.Length()));
							bSetStartupFile = 1;
						}
					}

					if (GetProfile().GetBool("Svc5Loaves","HighPriority",0))
						SetPriorityClass(GetCurrentProcess(),HIGH_PRIORITY_CLASS); 

	
					GString strStartupMessage;
					strStartupMessage << "Windows Service using [" << strFile << "]";

					if (!bSetStartupFile)
					{
						// the startup file could not be found.
						unsigned long iID;
						GString *pG = new GString("Could not find startup file (5Loaves.txt or 5LoavesStartup.bin) or password to decipher 5LoavesStartup.bin is invalid.");
						CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)ThreadInteract,NULL,0,&iID);


						ss.dwCurrentState = SERVICE_STOPPED; 
						ss.dwCheckPoint = ss.dwWaitHint = 0;
						SetServiceStatus(hSS, &ss);
						break;
					}
					else if (!server_start(strStartupMessage))
					{
						ss.dwCurrentState = SERVICE_STOPPED; 
						ss.dwCheckPoint = ss.dwWaitHint = 0;
						SetServiceStatus(hSS, &ss);
						break;
					}
					else
					{
						// server is running
						unsigned long iID;
						CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)ThreadConsole,NULL,0,&iID);
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
         GetQueuedCompletionStatus(g_hIOCP, &dwBytesTransferred, &dwCompKey, &po, INFINITE);
         fdwControl = dwBytesTransferred;
      }
   } while (ss.dwCurrentState != SERVICE_STOPPED);

   // Cleanup and stop this service
   CloseHandle(g_hIOCP);   
}


//////////////////////////////////////////////////////////////////////////////

typedef BOOL (CALLBACK* ChangeServiceConfig2Type)(SC_HANDLE,  DWORD ,  LPVOID);

void InstallService() {
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
	if (fn) // always NULL
	{
		hService = OpenService(hSCM,(const char *)strServerName,SERVICE_CHANGE_CONFIG);
		SERVICE_DESCRIPTION sd;
		sd.lpDescription=(char *)(const char *)strServerDescription;
		fn(hService, SERVICE_CONFIG_DESCRIPTION, &sd);
		CloseServiceHandle(hService);
	}



   // Close the service and the SCM
   CloseServiceHandle(hService);
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


//////////////////////////////////////////////////////////////////////////////

int WINAPI WinMain(HINSTANCE hinst, HINSTANCE hinstExePrev, 
   LPSTR pszCmdLine, int nCmdShow) {

	//
	// ***** To debug this application  ****
	// set fDebug to 1, then recompile
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
   int bRun  = 0;
   for (i = 1; i < nArgc; i++) {
      if ((ppArgv[i][0] == __TEXT('-')) || (ppArgv[i][0] == __TEXT('/'))) {
         // Command line switch
         if (lstrcmpi(&ppArgv[i][1], __TEXT("install")) == 0) 
            bInstall = 1;

         if (lstrcmpi(&ppArgv[i][1], __TEXT("remove"))  == 0)
            bRemove = 1;

         if (lstrcmpi(&ppArgv[i][1], __TEXT("run"))  == 0)
			 bRun = 1;

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
            g_strPassword = &ppArgv[i][1+strlen("pass:")];
		 }
      }
   }
	if (bInstall)
		InstallService();
	if (bRemove)
		RemoveService();

#ifdef UNICODE
   HeapFree(GetProcessHeap(), 0, (PVOID) ppArgv);
#endif

   if (fDebug) 
   {
      // Running as EXE not as service, just run the service for debugging
	  FiveLoavesServiceMain(0, NULL);
   }
   if (bRun) 
   {
		fStartService = 1;
   }

   strcpy(pzServer,strServerName);
   if (fStartService) 
   {
      SERVICE_TABLE_ENTRY ServiceTable[] = {
         { pzServer, FiveLoavesServiceMain },
         { NULL,        NULL }   // End of list
   };
   StartServiceCtrlDispatcher(ServiceTable);
   
   }
   return(0);
}


//////////////////////////////// End Of File /////////////////////////////////



