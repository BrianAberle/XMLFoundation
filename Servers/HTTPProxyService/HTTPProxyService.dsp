# Microsoft Developer Studio Project File - Name="HTTPProxyService" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=HTTPProxyService - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "HTTPProxyService.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "HTTPProxyService.mak" CFG="HTTPProxyService - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "HTTPProxyService - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "HTTPProxyService - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "HTTPProxyService - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "../../Libraries/XMLFoundation/inc" /I "../../Libraries/XMLFoundation/inc/Win32" /D "_STATIC_LINK_LIB" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 netapi32.lib Advapi32.lib user32.lib wsock32.lib ../../Libraries/Release/XMLFoundation.lib gdi32.lib /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "HTTPProxyService - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "HTTPProxyService___Win32_Debug"
# PROP BASE Intermediate_Dir "HTTPProxyService___Win32_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "HTTPProxyService___Win32_Debug"
# PROP Intermediate_Dir "HTTPProxyService___Win32_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /Gm /GX /ZI /Od /I "../vncapi" /I "../../../../Apache\xml-xerces\c\src" /I "../../Library\inc" /I "../../Library\inc\Win32" /I "../../library/inc/server" /D "_INTEL" /D "__NOTRACE_STACK__" /D "_STATIC_LINK_LIB" /D "_NO_EXCEPT" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /YX /FD /c
# ADD CPP /nologo /MD /W3 /Gm /GX /ZI /Od /I "../../Libraries/XMLFoundation/inc" /I "../../Libraries/XMLFoundation/inc/Win32" /D "__NOTRACE_STACK__" /D "_STATIC_LINK_LIB" /D "_NO_EXCEPT" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 netapi32.lib Advapi32.lib user32.lib wsock32.lib ../../Library/Build/DebugDLL/XMLFoundation.lib gdi32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 netapi32.lib Advapi32.lib user32.lib wsock32.lib ../../Libraries/Debug/XMLFoundation.lib gdi32.lib /nologo /subsystem:windows /debug /machine:I386 /out:"Debug/HTTPProxyService.exe" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "HTTPProxyService - Win32 Release"
# Name "HTTPProxyService - Win32 Debug"
# Begin Source File

SOURCE=.\HTTPProxyService.cpp
# End Source File
# End Target
# End Project
