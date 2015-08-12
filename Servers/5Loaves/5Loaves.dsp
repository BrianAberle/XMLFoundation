# Microsoft Developer Studio Project File - Name="5Loaves" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=5Loaves - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "5Loaves.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "5Loaves.mak" CFG="5Loaves - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "5Loaves - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "5Loaves - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "5Loaves - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /Gf /Gy /I "../../Libraries/XMLFoundation/inc" /I "../../Libraries/XMLFoundation/inc/Win32" /I "../../Libraries/XFer/inc" /D "WIN32" /FR /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 advapi32.lib wsock32.lib user32.lib ../../Libraries/Release/XMLFoundation.lib kernel32.lib user32.lib gdi32.lib /nologo /version:777.777 /subsystem:console /map /machine:I386 /libpath:"../../library/buildwin32/release_mt" /libpath:"../../Servers/CommonSource/JavaNT/"

!ELSEIF  "$(CFG)" == "5Loaves - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "5Loaves___Win32_Debug"
# PROP BASE Intermediate_Dir "5Loaves___Win32_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /GX /Zi /Od /I "../vncapi" /I "../../Library\inc" /I "../../Library\inc\Win32" /D "_NO_EXCEPT" /D "_INTEL" /D "__NOTRACE_STACK__" /D "_STATIC_LINK_LIB" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FR /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /GX /Zi /Od /I "../../Drivers/Perl/ActiveStatePerl5.10/lib/CORE" /I "../../Libraries/XMLFoundation/inc" /I "../../Libraries/XMLFoundation/inc/Win32" /I "../../Libraries/XFer/inc" /D "_NO_EXCEPT" /D "__NOTRACE_STACK__" /D "_STATIC_LINK_LIB" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FR /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 netapi32.lib Advapi32.lib user32.lib ../../Library/Build/Debug/XMLFoundation.lib kernel32.lib user32.lib gdi32.lib /nologo /subsystem:console /profile /debug /machine:I386 /nodefaultlib:"libcmtd" /libpath:"../../library/buildwin32/debug_mt" /libpath:"../../Servers/CommonSource/JavaNT/"
# ADD LINK32 wsock32.lib netapi32.lib Advapi32.lib user32.lib ../../Libraries/Debug/XMLFoundation.lib kernel32.lib user32.lib gdi32.lib /nologo /subsystem:console /profile /debug /machine:I386 /nodefaultlib:"libcmtd" /libpath:"../../library/buildwin32/debug_mt" /libpath:"../../Servers/CommonSource/JavaNT/"

!ENDIF 

# Begin Target

# Name "5Loaves - Win32 Release"
# Name "5Loaves - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\Console.cpp
# End Source File
# Begin Source File

SOURCE=..\Core\ServerCore.cpp
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\Core\ServerCoreCustomGlobalInternals.cpp
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\Core\ServerCoreCustomGlobals.cpp
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\Core\ServerCoreCustomHTTP.cpp
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\Core\ServerCoreCustomProfileInit.cpp
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\Core\ServerCoreCustomProfileSetNotify.cpp
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\Core\ServerCoreCustomProfileUpdate.cpp
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\Core\ServerCoreCustomServerStart.cpp
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\Core\ServerCoreCustomServerStop.cpp
# PROP Exclude_From_Build 1
# End Source File
# End Group
# End Target
# End Project
