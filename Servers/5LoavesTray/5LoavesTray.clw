; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CAboutBox
LastTemplate=CDialog
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "5loavestray.h"
LastPage=0

ClassCount=5
Class1=C5LoavesTrayApp
Class2=CAboutBox
Class3=CMainFrame
Class4=CStartupArgs
Class5=CUBTHyperlink

ResourceCount=3
Resource1=IDR_TRAYICON
Resource2=IDR_MAINFRAME
Resource3=IDD_ABOUTBOX

[CLS:C5LoavesTrayApp]
Type=0
BaseClass=CWinApp
HeaderFile=5LoavesTray.h
ImplementationFile=5LoavesTray.CPP

[CLS:CAboutBox]
Type=0
BaseClass=CDialog
HeaderFile=AboutBox.h
ImplementationFile=AboutBox.cpp
LastObject=IDC_WWW

[CLS:CMainFrame]
Type=0
BaseClass=CFrameWnd
HeaderFile=MAINFRM.H
ImplementationFile=MAINFRM.CPP

[CLS:CStartupArgs]
Type=0
BaseClass=CDialog
HeaderFile=StartupArgs.h
ImplementationFile=StartupArgs.cpp

[CLS:CUBTHyperlink]
Type=0
BaseClass=CStatic
HeaderFile=UBTHyperlink.h
ImplementationFile=UBTHyperlink.cpp

[DLG:IDD_ABOUTBOX]
Type=1
Class=CAboutBox
ControlCount=4
Control1=IDOK,button,1342373889
Control2=IDC_STATIC,static,1342308352
Control3=IDC_STATIC,static,1342177283
Control4=IDC_STATIC,static,1342308352

[DLG:IDD_DLG_STARTUP_PASS]
Type=1
Class=CStartupArgs

[MNU:IDR_MAINFRAME]
Type=1
Class=?
Command1=ID_APP_ABOUT
Command2=ID_APP_EXIT
CommandCount=2

[MNU:IDR_TRAYICON]
Type=1
Class=?
Command1=ID_APP_SUSPEND
Command2=ID_APP_ABOUT
CommandCount=2

