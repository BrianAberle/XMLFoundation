//#define ___XFER				http://1drv.ms/1fo7Asp
//#define ___XFER_SRC
//#define ___XFER_EVAL
// --------------------------------------------------------------------------
//						United Business Technologies
//			  Copyright (c) 2000 - 2014  All Rights Reserved.
//
// Source in this file is released to the public under the following license:
// --------------------------------------------------------------------------
// This toolkit may be used free of charge for any purpose including corporate
// and academic use.  For profit, and Non-Profit uses are permitted.
//
// This source code and any work derived from this source code must retain 
// this copyright at the top of each source file.
// --------------------------------------------------------------------------
int g_paused;
int g_doneCount;


#ifndef _REENTRANT
#define _REENTRANT
#endif

#include <stdio.h>
#include <sys/types.h>

#include "GThread.h"
#include "XMLFoundation.h"
#include "GException.h"



#ifdef _WIN32
	#include <io.h>		// for _chmod
	#include <conio.h>
	#include <winsock.h>
	#include <fcntl.h>
	#include <errno.h>
#else 
	#include <sys/ioctl.h>
	#include <termio.h>		// for keyboard poll
	#include <netinet/in.h>
	#include <sys/socket.h>
	#include <sys/time.h>
	#include <fcntl.h>
	#include <errno.h>
	#include <signal.h>
	#include <unistd.h> // for close()
#endif
#ifdef __sun
	#include <sched.h>	// for sched_yield() also must link -lposix4
#endif


#ifdef _AIX
#include <sys/select.h>
#endif

// The core server
#define __CUSTOM_CORE__				// for all ServerCore.cpp hooks to be enabled
// #define SERVERCORE_CUSTOM_HTTP	// for only the most common hook - ServerCoreCustomHTTP.cpp
#include "../Core/ServerCore.cpp"


#ifdef ___XFER
	char g_szAppName[] = "Xfer";
	#include "../../Libraries/Xfer/Init/XferInit.cpp"
	#define __REQUIRE_CIPHERED_STARTUP_FILE
#else
	char g_szAppName[] = "CustomHTTPService";
#endif


GString g_strFinalRunTimePassword;

// Terminal IO Tested in : linux, Solaris, AIX, and HPUX 
#ifndef _WIN32

void SetTermIO(int nNew, termio *original_io, int bEcho)
{
   static struct termio current_io;
   int result;
	if (nNew)
	{
		result = ioctl (0, TCGETA, original_io);
	}
	else
	{
		ioctl (0, TCSETA, original_io);
		return; // success
	}

   if (result == -1)
   {
		return; // failed
   }
   memcpy (&current_io, original_io, sizeof (struct termio));
   current_io.c_cc[VMIN]  = 1;    // wait for one character 
   current_io.c_cc[VTIME] = 0;    // and don't wait to return it 
   current_io.c_lflag &= ~ICANON; // unbuffered input 

	if (!bEcho)
		current_io.c_lflag &= ~ECHO;   // turn off local display 

   // set new input mode 
   result = ioctl (0, TCSETA, &current_io);
   if (result == -1)
   {
      return; // failed
   }
}

int PollKeyBoard ()
{
   fd_set rfds;
   struct timeval tv;
   FD_ZERO (&rfds);
   FD_SET (0, &rfds);
   tv.tv_sec  = 0;
   tv.tv_usec = 0;
   select (1, &rfds, NULL, NULL, &tv);
   if (FD_ISSET(0, &rfds))
      return 1;
   return 0;
}
#endif

class TermIOObject 
{
public:
#ifndef _WIN32
	struct termio original_io;
	TermIOObject(int bEcho)
	{
		SetTermIO(1, &original_io, bEcho);
	}
	~TermIOObject()
	{
		SetTermIO(0, &original_io, 0);
	}
#else
	TermIOObject(int bEcho){}
#endif
};

GString g_ConsoleLock;
char *GetUserCommand(int bEcho = 1)
{
	if (g_ConsoleLock.Length())
	{
		printf("\nConsole Locked.  Enter password:");
	}
	TermIOObject t(bEcho);
	static unsigned char commandBuffer[512];
	int nCmdBufIndex;
	unsigned char *pCmd;

NEXT_COMMAND:	
	nCmdBufIndex = 0;

	while(1)
	{
#ifndef _WIN32
		PollKeyBoard();				// wait for data to hit stdin
		int nCh = fgetc (stdin);	// get data from stdin
#else
		int nCh = getchar();		
#endif
		if (nCh == -1)
		{
			// this happens in Linux when compile flags are wrong
			// it also happens when the process is running in the background and not at a shell
			return 0;
		}
		else // deal with the next byte from stdin
		{
			switch (nCh)
			{
			case 10: 
				commandBuffer[nCmdBufIndex++] = 0;
				nCmdBufIndex = 0;
				pCmd = (commandBuffer[0] == 10) ? &commandBuffer[1] : commandBuffer;
				if (g_ConsoleLock.Length())
				{
					if (g_ConsoleLock.Compare((const char *)pCmd) == 0)
					{
						printf("Unlocked.  Continue...\n");
						g_ConsoleLock = "";
						goto NEXT_COMMAND;
					}
					else
					{
						printf("Bad Password.  Try again.\n");
						goto NEXT_COMMAND;
					}
				}
				return (char *)pCmd; // exit the while(1)

			default:
				if (nCmdBufIndex < 500)
				{
					commandBuffer[nCmdBufIndex++] = (unsigned char)nCh;
				}
				else // this command is invalid
				{
					commandBuffer[0] = 0;
					return (char *)commandBuffer;
				}
			}
		}
	}
}


int GetUserBoolean()
{
	GString input(GetUserCommand());
	if (input.CompareNoCase("yes") == 0 || 
		input.CompareNoCase("true") == 0 || 
		input.CompareNoCase("on") == 0 ||
		input.CompareNoCase("1") == 0 ||
		input.CompareNoCase("y") == 0)
	{
		return 1;
	}
	return 0;
}

#ifdef ___XFER
	#include "../../Libraries/Xfer/Core/XferConsoleCommand.cpp"
#else
	// add your own additional hooks in this file - for easy source versioning
	#include "../Core/ConsoleCommandsCustom.cpp"
#endif 

int g_isRunning = 1;
void ConsoleCommand(char *pzCommand)
{
	GString strCmdIn(pzCommand);

	if (strCmdIn.CompareNoCase("si") == 0)
	{
#ifdef _WIN32
		GString strServiceCommand;

		printf("This command sends commands to the %s Windows Service\n",g_szAppName);
		printf("running on THIS machine. Commands: at,hp,rp,vp,sl,rl,kill,mc\n");
		printf("work like direct console commands, +additional commands:\n");
		printf("'stop' stops the service, arg1[I] for Immediate shutdown.\n");
		printf("Enter a command, empty to exit.\n");
		char *pzCmd = GetUserCommand();
		if (!pzCmd || !pzCmd[0])
			return;
		int nArgCount = 1;
		strServiceCommand << pzCmd;
		char chNull = 0;
		while(1)
		{
			printf("Enter argument %d for this command, empty if it has no argument %d\n",nArgCount,nArgCount);
			nArgCount++;
			char *pzArg = GetUserCommand();
			if(pzArg && pzArg[0])
			{
				strServiceCommand.write(&chNull,1); // null
				strServiceCommand.write(pzArg,strlen(pzArg)); // null
			}
			else
			{
				break;
			}
		}
		strServiceCommand.write(&chNull,1); // double null terminate the end
		strServiceCommand.write(&chNull,1); // double null terminate the end

		HANDLE hFileMapping = OpenFileMapping(FILE_MAP_ALL_ACCESS,TRUE,"Console_Command");
		if (hFileMapping == 0)
		{
			
			printf("Service is not running:%d\n",GetLastError());
		}
		else
		{
			char *p = (PCHAR)MapViewOfFile( hFileMapping, FILE_MAP_WRITE, 0, 0, 0);
			if (p)
			{
				#ifdef ___XFER
					GString strPasswordOnStack(&g_XKey.pzFiller2[41],79);
				#else
					GString strPasswordOnStack(g_strFinalRunTimePassword);
				#endif
				
				GString strIPCMessage;
				strIPCMessage << rand() << rand() << "|";
				strIPCMessage.write(strServiceCommand,strServiceCommand.Length());
				strIPCMessage << rand() << rand();

				GString strIPCData;						
				EnHexUUCipher(&strIPCMessage, &strIPCData, strPasswordOnStack);
										

				memcpy(p,">>",2);
				memcpy(&p[2],(const char *)strIPCData,strIPCData.Length()+1);
				while (memcmp(p,">>",2) == 0)
				{
					Sleep(100);
				}

				GString strCiphered(&p[2]);
				GString strDeCiphered;
				DeHexUUCipher(&strCiphered, &strDeCiphered, strPasswordOnStack);
				const char *pzData = strDeCiphered;
				char *pSep = (char *)strpbrk(pzData,"|");
				if (pSep)
				{
					printf(pSep+1);
					printf("\n");
				}
		        UnmapViewOfFile( p );
			}
	        CloseHandle( hFileMapping );
		}
#else
	printf("This command only applies to the Windows OS\n");
#endif
	}


	else if (strCmdIn.CompareNoCase("?") == 0 ||
		strCmdIn.CompareNoCase("help") == 0)
	{
		printf("Commands:(type command for more information)\n");
		printf("Exit                 Hits                  LockConsole(lc)\n");
		printf("Encrypt              Decrypt               EncryptDir\n");
		printf("DecryptDir           MasterConfig(mc)      Stop/Resume Log(sl/rl)\n");
		printf("ViewPorts(vp)        HaltPort(hp)          ResumePort(rp)\n");
		printf("WriteConfig(wc)      ServiceInteract(si)   Register\n");
		printf("ActiveThreads(at)    Kill                  End\n");
		printf("QueryMasterConfig(qm)\n");
// You can see how easy the console is to customize.  The console is a complete
// command line shell implementation that works on many platforms.  Xfer uses it.
#ifdef ___XFER
		printf("-------------------Xfer Commands-----------------\n");
		printf("             MKD  DIR  ALL  DEL  GET  PUT  REN\n");
		printf("             PRP  ENC  DEC  STS  PLS  KIL\n");
#endif
		printf("-----------------------Debug Commands---------------------\n");
		printf("PingPool(pp)      Check\n");
		printf("HTTPBalanceTrace      Mem\n");
	}
	else if (strCmdIn.CompareNoCase("stop") == 0 ||
		strCmdIn.CompareNoCase("quit") == 0 ||
		strCmdIn.CompareNoCase("exit") == 0)
	{
		printf("Stopping and releasing all ports.\n");
		stopListeners();
		printf("All ports released, no new transactions are being accepted.\n");
		
		
		printf("System Shutdown in progress.");
		g_isRunning = 0;
	}
	else if (strCmdIn.CompareNoCase("at") == 0 )
	{
		printf("%d Threads/Transactions currently in progress\n",g_nClientthreadsInUse);
		if (g_nClientthreadsInUse)
		{
			showActiveThreads();
		}
	}
	else if (strCmdIn.CompareNoCase("hp") == 0 ) // Halt port
	{
		printf("Enter socket port to release and stop servicing(for example: 10777). 'all' for all ports, Empty to do nothing.\n");
		printf("When a port has been released, no new transactions/connections are started but existing ones will continue.\n");
		char *pzPort = GetUserCommand();
		if (pzPort && pzPort[0])
		{
			GString strAll("all");
			if (strAll.CompareNoCase(pzPort) == 0)
			{
				stopListeners();
			}
			else
			{
				int nRet = stopListeners(atoi(pzPort));
				if(nRet == 0)
				{
					printf("Not stopped.  You can only stop a defined port.\nUse the 'vp' command to view defined ports.");
				}
			}
		}
	}
	else if (strCmdIn.CompareNoCase("vp") == 0 ) // view defined ports
	{
		viewPorts();
	}
	else if (strCmdIn.CompareNoCase("rp") == 0 ) // resume port
	{
		printf("Enter socket port to resume or 'all'. Empty to do nothing.\n");
		char *pzPort = GetUserCommand();
		if (pzPort && pzPort[0])
		{
			GString strAll("all");
			if (strAll.CompareNoCase(pzPort) == 0)
			{
				startListeners(3);
			}
			else
			{
				int nRet = startListeners(3, atoi(pzPort));
				if (nRet == 0)
				{
					printf("Not started.  You can only start a defined port.\nUse the 'vp' command to view defined ports.");
				}
			}
		}
	}


	else if (strCmdIn.CompareNoCase("Register") == 0 ) // Register
	{
		printf("This command allows you to enable premium features in %s\n",g_szAppName);
		printf("Enter the feature you want to enable:\n");

		char *pzFeature = GetUserCommand();
		if (pzFeature && pzFeature[0])
		{
			char *pBuf = new char[8192];
			char *pResult = 0;
			int nRet = 0 ;//EnableFeature(pzFeature, pBuf, &pResult);  // this is under development
			if (nRet)
			{
				if (pResult[0] == 0)
				{
					printf("Failed to enable feature.  Did you spell it properly?\n");
				}
				else
				{
					printf("Copy this to your startup file:\n\n");
					printf(pResult);
					printf("\n\n");
				}
			}
			else
			{
				printf("Failed to enable feature.\n");
			}
			delete pBuf;
		}
	}
	else if (strCmdIn.CompareNoCase("sl") == 0 ) // Stop System Log
	{
		g_NoLog = 1;
		printf("System logging stopped.\n");
	}
	else if (strCmdIn.CompareNoCase("rl") == 0 ) // Resume System Log
	{
		g_NoLog = 0;
		printf("System logging resumed.\n");
	}
	else if (strCmdIn.CompareNoCase("lc") == 0 ) // LockConsole
	{
		printf("This command disables this command interface.\n");
		printf("Warning:This lock does not yet apply on Windows due to the doskey feature.\n");
		printf("Enter the password:  (enter to exit command)\n");
		char *pzPass = GetUserCommand();
		if (pzPass && pzPass[0])
		{
			g_ConsoleLock = pzPass;
		}
	}
	else if (strCmdIn.CompareNoCase("Kill") == 0 ) 
	{
		printf("I wish there wasn't killing.  In the next world there won't be.\n");
		printf("Normally there is a way around killing.  Suppose you have a \n");
		printf("[Tunnel1N] section with a high timeout, higher than the client\n");
		printf("application.  This would be an error in your system configuration.\n\n");
		printf("This subtle error would normally be unproblematic.\n");
		printf("Suppose one day the network was up and down a lot, so the client\n");
		printf("application issued repeated sessions, while the old sessions\n");
		printf("sit waiting for an inactivity timeout.  Connection buildup is bad,\n");
		printf("eventually it may lead to failure.  Failure is unacceptable.\n\n");

		printf("Killing is bad, but in the case stated it is a perfectly safe solution.\n");
		printf("Killing is not always safe,  Memory can be temporarily 'lost',\n");
		printf("'partial transactional residue' aka corruption can happen to disk.\n");
		printf("Either may lead to localized or system wide failure.  Unacceptable.\n");
		printf("If the victim thread is running a server plug-in, any ill effect\n");
		printf("of a kill is determined by the plugin itself.  The core is unaffected.\n\n");

		printf("Note: Kill, kills threads in this process, KIL kills remote processes.\n\n");
		printf("Note: END ends jobs, normally.\n\n");


		printf("Enter the Thread ID(tid) to kill, empty to exit this command:\n");
		GString strTid = GetUserCommand();
		int ntidToKill = atoi(strTid);
		if (ntidToKill > 0)
		{
			int nRet = KillTid (ntidToKill);
			if (nRet == 0)
			{
				printf("Done.  The a new thread now owns that tid.\n");
			}
			else if (nRet == 1)
			{
				printf("Invalid ThreadID(tid).\n");
			}
			else
			{
				printf("Thread not killed, reason(%d).\n",nRet);
			}
		}
	}
	else if (strCmdIn.CompareNoCase("End") == 0 ) 
	{
		printf("END ends jobs.  This closes the connection no matter what the state or\n");
		printf("progress of the current transaction or download.  There is an error in\n");
		printf("Internet Explorer v6.0.2800.1106, so that if you a file download gets\n"); 
		printf("'ended', IE will write the file to disk and finish the download with\n"); 
		printf(" no error, even though the file it has written is not complete and will\n");
		printf("be invalid.  IE should not write the file since they know the expected size.\n\n");
	
		printf("Enter the Thread ID(tid) to end, empty to exit this command:\n");
		GString strTid = GetUserCommand();
		int ntidToKill = atoi(strTid);
		if (ntidToKill > 0)
		{
			int nRet = KillTid (ntidToKill,1);
			if (nRet == 0)
			{
				printf("Done.\n");
			}
			else if (nRet == 1)
			{
				printf("Invalid ThreadID(tid).\n");
			}
			else
			{
				printf("Thread not ended, reason(%d).\n",nRet);
			}
		}
	}
	else if (strCmdIn.CompareNoCase("qm") == 0 ) // QueryMasterConfig
	{
		printf("This command allows you to view any setting in the Config file\n");
		printf("Enter a section without the [] (enter to exit command):\n");
		GString strSection(GetUserCommand());
		if (strSection.Length())
		{
			printf("Enter a config entry (left of the =):\n");
			char *pzEntry = GetUserCommand();
			if (pzEntry && pzEntry[0])
			{
				static GString strNull("(null)");
				const char *pzVal = GetProfile().GetString(strSection, pzEntry, 0);
				if (!pzVal)
					pzVal = strNull;

				GString str;
				str.Format("[%s]\n%s=%s\n",(const char *)strSection,pzEntry,pzVal);
				printf((const char *)str);
			}
		}
	}
	else if (strCmdIn.CompareNoCase("mc") == 0 ) // MasterConfig
	{
		printf("This command allows you to change any setting in the Config \n");
		printf("Not all commands apply while the server is already running.\n");
		printf("For example changing the thread pool size will have no effect\n");
		printf("because that value is only meaningful during server startup.\n");
		printf("Enter a section without the [] (enter to exit command):\n");
		char *pzSection = GetUserCommand();
		if (pzSection && pzSection[0])
		{
			GString strSection(pzSection);
			printf("Enter a config entry (left of the =):\n");
			char *pzEntry = GetUserCommand();
			if (pzEntry && pzEntry[0])
			{
				GString strEntry(pzEntry);
				printf("Enter the new value:\n");
				char *pzValue = GetUserCommand();
				if (pzValue && pzValue[0])
				{
#ifdef ___XFER_EVAL
					// This command allows you to set ANY value for ANY entry, 
					// You can even add Xfer users with this command, or change their password - (you cant evaluate that)
					printf("This feature is removed from the evaluation version.\n");
#else
					GetProfile().SetConfig(strSection, strEntry, pzValue);
					printf("New value has been set.\n");
#endif
				}
			}
		}
	}
	
	else if (strCmdIn.CompareNoCase("mem") == 0 )
	{
#ifdef _WIN32
		
		// Fill available memory
		MEMORYSTATUS MemStat;
		MemStat.dwLength = sizeof(MEMORYSTATUS);
		GlobalMemoryStatus(&MemStat);
		printf("Physical   %d/%d\n",MemStat.dwAvailPhys / 1024L,MemStat.dwTotalPhys / 1024L);
		printf("Virtual    %d/%d   At:",MemStat.dwAvailVirtual / 1024L,MemStat.dwTotalVirtual / 1024L);
		
		// current time
		char pzTime[128];
		struct tm *newtime;
		time_t ltime;
		time(&ltime);
		newtime = gmtime(&ltime);
		strftime(pzTime, 128, "Date: %a, %d %b %Y %H:%M:%S", newtime);
		printf(pzTime);
#else
		printf("MEM not yet available on this platform\n");
#endif	

	}
	else if (strCmdIn.CompareNoCase("Hits") == 0 )
	{
		GString strStats;
		strStats +=	"Total Hit Count:";
		strStats += g_TotalHits;

		strStats += "\nMost threads used: ";
		strStats += g_ThreadUsageMax;

		strStats += "\nThreads in use now: ";
		strStats += g_nClientthreadsInUse;


		// current time
		char pzTime[128];
		struct tm *newtime;
		time_t ltime;
		time(&ltime);
		newtime = gmtime(&ltime);
		strftime(pzTime, 128, "\nDate: %a, %d %b %Y %H:%M:%S", newtime);
		strStats += pzTime;


		printf((const char *)strStats);
	}
	else if (strCmdIn.CompareNoCase("HTTPBalanceTrace") == 0 )
	{
		g_TraceHTTPBalance = !g_TraceHTTPBalance;
		printf("HTTP Balance Tracing is now:");
		if (g_TraceHTTPBalance)
			printf("ON\n");
		else
			printf("OFF\n");
	}
	else if (strCmdIn.CompareNoCase("check") == 0 )
	{
		// add this command to test a new platform port
		server_build_check();
	}
	else if (strCmdIn.CompareNoCase("PingPool") == 0 || strCmdIn.CompareNoCase("pp") == 0)
	{
		pingPools();
	}

	// You can see how easy the console is to customize.  The console is a complete
	// command line shell implementation that works on many platforms.  Xfer uses it.
#ifdef ___XFER
	#include "../../Libraries/Xfer/Core/XferConsoleCommandHook.cpp"
#else
	// add your own additional hooks in this file - for easy source versioning
	#include "../Core/ConsoleCommandsCustomHook.cpp"
#endif 


	////////////////////////////////////////////////////////////////////////////////////////////////
	else if (strCmdIn.CompareNoCase("Decrypt") == 0 )
	{
		GString strKey; GString strInFile; GString strOutFile; GString strErrorOut;

		printf("Enter the complete path to the crypted file [enter to cancel]\n");
		printf("Examples [c:\\CustomHTTPServiceConfig.xbin] or [/XferConfig.xbin]\n");
		strInFile = GetUserCommand();
		if (strInFile.Length())
		{
			printf("Enter the decrypt key: [enter to cancel] \n");
			strKey = GetUserCommand(0);
			if (strKey.Length())
			{
				printf("Enter the complete path to the destination file \n");
				printf("Examples [c:\\Path\\File.txt] or [/Path/File.txt]\n");
				strOutFile = GetUserCommand();
				if (strInFile.Length())
				{
					if (!FileDecrypt(strKey, strInFile, strOutFile, strErrorOut))
					{
						printf("Failed to decrypt [%s] to [%s]\n%s",(const char *)strInFile, (const char *)strOutFile, (const char *)strErrorOut);
					}
					else
					{
						printf("OK");
					}
				}
			}
		}
	}
	else if (strCmdIn.CompareNoCase("Encrypt") == 0 )
	{
		GString strKey; GString strInFile; GString strOutFile; GString strErrorOut;

		printf("Enter the complete path to the clear text file [enter to cancel]\n");
		printf("Examples [c:\\CustomHTTPServiceConfig.xbin] or [/XferConfig.xbin]\n");
		strInFile = GetUserCommand();
		if (strInFile.Length())
		{
			printf("Enter the encrypt key:\n");
			strKey = GetUserCommand(0);
			if (strKey.Length())
			{
				printf("Enter the complete path to the destination file \n");
				printf("Examples [c:\\CustomHTTPServiceConfig.xbin] or [/CustomHTTPServiceConfig.xbin]\n");
				strOutFile = GetUserCommand();
				if (strOutFile.Length())
				{
					if (!FileEncrypt(strKey, strInFile, strOutFile, strErrorOut))
					{
						printf("%s\n",(const char *)strErrorOut);
					}
					else
					{
						printf("OK");
					}
				}
			}
		}
	}
	else if (strCmdIn.CompareNoCase("EncryptDir") == 0  || strCmdIn.CompareNoCase("DecryptDir") == 0)
	{
		GString strOperation("decrypt");
		if (strCmdIn.CompareNoCase("EncryptDir") == 0)
			strOperation = "encrypt";

		GString strDirectory; GString strErrorOut; GString strKey;
		printf("Enter the complete path to directory you want to %s.\n",(const char *)strOperation);
		printf("Examples [c:\\wwwHome\\] or [/WWWHome/]       [enter to cancel]\n");
		strDirectory = GetUserCommand();

		if (strDirectory.Length())
		{
			printf("Enter the %s key:\n",(const char *)strOperation);
			strKey = GetUserCommand(0);
			if (strKey.Length())
			{

				printf("Do you want to %s any files found in sub directories?\n", (const char *)strOperation);
				int nRecurseDeep = GetUserBoolean();

				if (!CipherDir(strCmdIn.CompareNoCase("DecryptDir"), strKey, strDirectory, nRecurseDeep, strErrorOut))
				{
					printf("%s\n",(const char *)strErrorOut);
				}
				else
				{
					printf("OK");
				}
			}
		}
	}
	else
	{
		if (strCmdIn.Length())
		{
			printf("command [");
			printf((const char *)strCmdIn);
			printf("] is invalid");
		}
	}
}


void server_build_check();

int main(int argc, char * argv[])
{
// After a new platform port, enable the following code, and compare it to the output of a good build.
// This will test all the algorithms to be sure they compiled correctly.  The byte ordering is sometimes
// backwards (or forwards) which causes TwoFish to cipher backwards (or forwards). An executable compiled
// backwards works great with all other backwards builds ciphering and deciphering between them.  
// We all need to order the integer the same only for the sake of TCP/IP and protocols. Therefore "network byte ordering" exists.
// and likewise for a block cipher based protocol, so - After a new platform port - run server_build_check() 
// to be sure your bits are in order.
// --------------------------------
//	g_LogCache = 0;
//	g_LogStdOut = 1;
//	server_build_check();
//	return 0;
// --------------------------------
#ifdef ___XFER
	if (!XferProcessInit())
	{
		return 0;
	}
#endif


#ifndef _WIN32
	struct termio original_io;
	SetTermIO(1, &original_io, 1);
#endif



	// check for a ciphered startup file

	
	// Look in current directory for "CustomHTTPServiceConfig.xbin", XferConfig.xbin, or MyAppConfig.xbin
	GString strCipheredConfigFile;
	int bUseCipheredStartup = 1;
	bool bHasSetStartupFile = 0;
	strCipheredConfigFile << g_szAppName << "Config.xbin";


	struct stat sstruct;
	int result = stat(strCipheredConfigFile, &sstruct);
	if (result != 0)
	{
		// look in the current directory
		int result = stat(strCipheredConfigFile, &sstruct);
		if (result != 0)
		{
			// also try one folder below the current directory - mostly for the sake of a debugger
			strCipheredConfigFile = "../";
			strCipheredConfigFile << g_szAppName << "Config.xbin";
			int result = stat(strCipheredConfigFile, &sstruct);
			if (result != 0)
			{
				bUseCipheredStartup = 0; // the ciphered file was not found
			}
		}
	}
	if (bUseCipheredStartup)
	{
		GString strPassword;

		// get the password from stdin, turn off echo where possible
		if (strPassword.IsEmpty())
		{
			printf( "Enter an Over-ride password OR press enter:\n" );
			strPassword = GetUserCommand(0);
		}


		if (!strPassword.Length())
		{
			// add your own code here to embed a passward used between your application and the config file if it must be ciphered.
			// strPassword << "my hard coded password";
			#ifdef ___XFER
				strPassword.Write(&g_XKey.pzFiller2[41],79);
			#endif
		}
		if (!strPassword.Length())
		{
			#ifdef __REQUIRE_CIPHERED_STARTUP_FILE
				printf("Password is required\n");
				return -1;
			#else
			#endif
		}

		// open the ciphered file, read it into [strCfgData] decipher it into [strDest] and set the GProfile from the [strDest] value
		try
		{
			GString strCfgData;
			strCfgData.FromFile(strCipheredConfigFile);
			GString strDest(8192);
			GString strErrorOut;
			if (DecryptMemoryToMemory(strPassword, strCfgData,strCfgData.GetLength(), strDest,  strErrorOut))
			{
				// delete the default (and empty) GProfile object and create a new one
				delete SetProfile(new GProfile((const char *)strDest, strDest.Length(), 0));
				bHasSetStartupFile = 1;
			}
			else
			{
				printf("%s",strErrorOut._str);
			}
		}
		catch ( GException & )
		{
		}

	}
	
	if (!bHasSetStartupFile)
	{
#ifdef __REQUIRE_CIPHERED_STARTUP_FILE
		printf("Could not find %sConfig.xbin\n",g_szAppName);
		return -1;
#else
		g_strFinalRunTimePassword = "Password"; // no password
		if (argc > 1)
		{
			delete SetProfile(new GProfile(argv[1],0));
		}
		else
		{
			///////////////////////////////////////////////////////////////////
			// 3/19/2014 - when bUseXML is set to 1 it switches the application config file to XML
			// Notice that it is also passed into GProfile() as the 3rd argument to let it know to expect XML
			// ------------------------
			bool bUseXML = 0; // change it to 1 for an XML based application config file - 
			//  -- Read the comment at the top of GProfile.cpp --
			///////////////////////////////////////////////////////////////////

			char *pExt = (bUseXML) ? ".xml" : ".txt";
			GString strConfigFile(g_szAppName);
			strConfigFile << pExt;

			GString strConfigData;
			if (!strConfigData.FromFile( strConfigFile, 0 )) // look in the current directory
			{
				strConfigFile = "../"; // over-write the value 
				strConfigFile << g_szAppName << pExt; // append file name and .ext
				if (!strConfigData.FromFile( strConfigFile )) // look back one directory
				{
					printf("Could not find %s.%s\n",g_szAppName,pExt);
				}
			}
			strCipheredConfigFile = strConfigFile; // store the startup file we used
			delete SetProfile(new GProfile(strConfigData._str, strConfigData._len, bUseXML ));
		}
#endif
	}

	// 3/19/2014  Converting INI to XML  = set bUseXML to 0, then uncomment this code
	//           ------------------------
	// GString s;
	// GetProfile().WriteCurrentConfig(&s,1);
	// s.ToFile("c:\\log\\config.xml");

	// 3/20/2014 Converting XML to INI	 = set bUseXML = 1;, then uncomment this code
	//           ------------------------
	// GString s;
	// GetProfile().WriteCurrentConfig(&s,0);
	// s.ToFile("c:\\log\\config.txt");

	GString strStartupMessage;
	strStartupMessage << "Using [";
	if (bUseCipheredStartup && strCipheredConfigFile.Length())
		strStartupMessage << strCipheredConfigFile;
	else
		strStartupMessage << strCipheredConfigFile;
	strStartupMessage << "]";

	// build the shell prompt like this:
	//	char *   = "\nXfer>";
	//	char *   = "\nCustomHTTPService>";
	//	char *   = "\nMyShell>";
	GString strPrompt("\n");
	strPrompt << g_szAppName << ">";

	ExceptionHandlerScope durationOfprocess;
NEVER_DIE:
	XML_TRY			
	{
		try
		{
			if (!server_start(strStartupMessage))
				return 0;
			printf(" Server Started.\n");

			char *pzFirstCommandRead = GetUserCommand();
			
			printf( strPrompt._str );
			
			while( g_isRunning )
			{
				char *pzCommand = GetUserCommand();
				if (pzCommand)
				{
					ConsoleCommand(pzCommand);
					printf( strPrompt._str );
					fflush(stdout);
				}
				else
				{
					// this can happen on some linux when compiling without the -maccumulate-outgoing-args option in gcc
					
					// it also happens when the process is launched through a linux System V script as a background process
					// argc will be 2 in this case because the startup file was passed to the process from the shell script

					if (argc == 1) // if startup arguments are passed in then do not display the warning or non functioning prompt
					{
						printf("\nCommand line input has been disabled\n");
						printf( strPrompt._str );

					}
					else // since we have no console prompt, write a pidfile 
					{
						#ifndef _WIN32
							GString strUnixPIDFile("var/run/");
							strUnixPIDFile << g_szAppName << ".pid";
							strUnixPIDFile.MakeLower();  

							const char *pzPIDFile = GetProfile().GetStringOrDefault("System", "PidFile", strUnixPIDFile );

						    GString strPIDFile;
							strPIDFile << (long) getpid();
							unlink(pzPIDFile);
							strPIDFile.ToFile(pzPIDFile);
						#endif
					}

					// 10 second Sleep
					gtimespec ts;
					ts.tv_sec=time(NULL) + 10;
					ts.tv_nsec=0;
					gthread_cond_t _TimeoutCond;
					gthread_mutex_t _TimeoutLock;
					gthread_mutex_init(&_TimeoutLock,0);
					gthread_cond_init(&_TimeoutCond,0);
					gthread_mutex_lock(&_TimeoutLock);
					gthread_cond_timedwait(&_TimeoutCond, &_TimeoutLock, &ts); 
				}
			}
		}	
		catch ( GException &e)
		{
			if (e.GetError() == 2) // the config file could not be found
			{
			// todo: try to find the file then 
			// goto NEVER_DIE;
			}
			
			// otherwise print the "Cant find config file" error, and exit this application.
			printf( e.GetDescription() );
			return 0;
			
		}

		catch(...)
		{
			printf("Error C runtime exception.\n");
		}
	}
	XML_CATCH(ex)
	{
		printf("XML_CATCH=%s\n",(const char *)ex.GetDescription());
	}


	if( g_isRunning )
		goto NEVER_DIE;

#ifndef _WIN32
	SetTermIO(0,&original_io,1);
#endif
	server_stop();
	return 0;
}

//////////////////////////////// End Of File /////////////////////////////////