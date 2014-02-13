# Microsoft Developer Studio Project File - Name="DriverPython" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=DriverPython - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "DriverPython.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "DriverPython.mak" CFG="DriverPython - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "DriverPython - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "DriverPython - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/UBT/XMLFoundation/Tools/Drivers/Python", OJSAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "DriverPython - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DRIVERPython_EXPORTS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "./Python26/Include" /I "./Python26/PC" /I "../../libraries/XMLFoundation/inc" /I "../../libraries/XMLServer/inc" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DRIVERPython_EXPORTS" /FD /I /Python-2.1/PCbuild/Include" " /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 ./Python26/libs/python26.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /out:"../Binary/Python.dll"

!ELSEIF  "$(CFG)" == "DriverPython - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DRIVERPython_EXPORTS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "./Python31/Include" /I "./Python31/PC" /I "../../libraries/XMLFoundation/inc" /I "../../libraries/XMLServer/inc" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DRIVERPython_EXPORTS" /FR /FD /I /Python-2.1/PCbuild/Include" /GZ " /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ./Python31/libs/python31.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"../Binary/Python.dll" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "DriverPython - Win32 Release"
# Name "DriverPython - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\Libraries\XMLFoundation\src\Utils\Base64.cpp
# End Source File
# Begin Source File

SOURCE=.\DriverPython.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Libraries\XMLFoundation\src\Utils\GBTree.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Libraries\XMLFoundation\src\Utils\GDirectory.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Libraries\XMLFoundation\src\Utils\GException.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Libraries\XMLFoundation\src\Utils\GList.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Libraries\XMLFoundation\src\Utils\GProfile.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Libraries\XMLFoundation\src\Utils\GString.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Libraries\XMLFoundation\src\Utils\GStringList.cpp
# End Source File
# Begin Source File

SOURCE=.\IntegrationPython.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Libraries\XMLFoundation\src\InterfaceParser.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Libraries\XMLFoundation\src\Utils\SHA256.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Libraries\XMLFoundation\src\Utils\TwoFish.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\DriverPython.h
# End Source File
# Begin Source File

SOURCE=.\IntegrationPython.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
