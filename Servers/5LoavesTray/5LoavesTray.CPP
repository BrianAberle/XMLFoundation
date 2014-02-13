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
#include "stdafx.h"				// this app
#include "5LoavesTray.h"		// this app
#include "mainfrm.h"			// this app
#include "AboutBox.h"			// this app
#include "StartupArgs.h"		// this app

#include <winsock.h>

#include "../Core/ServerCore.cpp"

C5LoavesTrayApp theApp;


BEGIN_MESSAGE_MAP(C5LoavesTrayApp, CWinApp)
	//{{AFX_MSG_MAP(C5LoavesTrayApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


void C5LoavesTrayApp::StopServer()
{
	try
	{
		server_stop();
	}
	catch(...)
	{
	}
}

BOOL C5LoavesTrayApp::InitInstance()
{
	try
	{
	SetProcessShutdownParameters(0x100,SHUTDOWN_NORETRY);
	GString strPassword(m_lpCmdLine);
	if (!strPassword.Length())
	{
		char pzName[512];
		memset(pzName,0,512);
		GetModuleFileName(NULL,pzName,512);
		GString strSameDirStartupFile(pzName);
		strSameDirStartupFile.TrimRightBytes(4);

		GString strFile("c:\\5LoavesStartup.bin");
		FILE *fpTst = fopen(strSameDirStartupFile,"r");
		if (fpTst)
		{
			fclose(fpTst);
			strFile = strSameDirStartupFile;
		}

		// decrypt disk file into memory
		int bSetStartupFile = 0;
		char *pDest;
		int nDestLen;
		try
		{
			GString strErrorOut;
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
			// load the disk file into memory
			GString strCfgData;
			strCfgData.FromFile("c:\\5Loaves.txt",0);
			if (strCfgData.Length())
			{
				strFile = "c:\\5Loaves.txt";
				SetProfile(new GProfile((const char *)strCfgData, (int)strCfgData.Length()));
				bSetStartupFile = 1;
			}
		}
		GString strStartupMessage;
		strStartupMessage << "Windows System Tray App using [" << strFile << "]";
		if (bSetStartupFile)
		{
			if (server_start(strStartupMessage))
			{
				// Create main frame window 
				CMainFrame* pMainFrame = new CMainFrame;
				if (!pMainFrame->LoadFrame(IDR_MAINFRAME))
					return FALSE;
				pMainFrame->ShowWindow(SW_HIDE);
				pMainFrame->UpdateWindow();
				m_pMainWnd = pMainFrame;
				return TRUE;
			}
		}
	}
	}
	catch(GException &ge)
	{
		AfxMessageBox(ge.GetDescription());
	}
	catch(...)
	{
		AfxMessageBox("Unknown Error");
	}
	return FALSE;
}

void C5LoavesTrayApp::OnAppAbout()
{
	CAboutBox dlg;
	dlg.DoModal();
}