# Microsoft Developer Studio Project File - Name="NoLibraryLink" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=NoLibraryLink - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "NoLibraryLink.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "NoLibraryLink.mak" CFG="NoLibraryLink - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "NoLibraryLink - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "NoLibraryLink - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "NoLibraryLink - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "../../../libraries/XMLFoundation/inc" /I "../../../libraries/XMLFoundation/inc/Win32" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386

!ELSEIF  "$(CFG)" == "NoLibraryLink - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../../libraries/XMLFoundation/inc" /I "../../../libraries/XMLFoundation/inc/Win32" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FR /FD /GZ /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "NoLibraryLink - Win32 Release"
# Name "NoLibraryLink - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\NoLibraryLink.cpp

!IF  "$(CFG)" == "NoLibraryLink - Win32 Release"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "NoLibraryLink - Win32 Debug"

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Group "XMLFoundation"

# PROP Default_Filter ""
# Begin Group "headers"

# PROP Default_Filter ""
# Begin Group "Win32Pthreads"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\Win32\config.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\Win32\implement.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\Win32\need_errno.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\Win32\pthread.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\Win32\sched.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\Win32\semaphore.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\AbstractionsGeneric.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\AbstractionsMFC.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\AbstractionsRougeWave.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\AbstractionsSTD.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\AttributeList.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\Base64.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\BaseInterface.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\BOLRelation.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\BZip.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\CacheManager.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\DynamicLibrary.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\DynamicXMLObject.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\ExceptionHandler.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\FactoryManager.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\FileDataSource.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\FrameworkAuditLog.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\garray.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\GBTree.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\GDirectory.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\GException.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\GHash.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\GHTTPMultiPartPOST.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\GList.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\GlobalInclude.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\GPerformanceTimer.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\GProfile.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\GSocketHelpers.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\GStack.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\GString.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\GStringList.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\GStringStack.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\GZip.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\IntegrationBase.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\IntegrationLanguages.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\InterfaceParser.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\ListAbstraction.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\LocalSocketHTTPDataSource.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\MemberDescriptor.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\MemberHandler.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\mhash_md5.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\ObjCacheQuery.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\ObjectDataHandler.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\ObjectPointer.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\ObjQuery.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\ObjQueryParameter.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\ObjResultSet.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\PasswordHelper.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\PluginBuilder.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\PluginBuilderLowLevelStatic.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\ProcedureCall.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\RelationshipWrapper.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\Schema.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\sha256.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\SocketHTTPDataSource.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\StackFrameCheck.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\StringAbstraction.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\SwitchBoard.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\TwoFish.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\WinINetHTTPDataSource.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\xmlAttribute.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\xmlDataSource.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\xmlDefines.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\xmlElement.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\xmlElementTree.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\xmlException.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\XMLFoundation.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\xmlLex.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\xmlObject.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\xmlObjectFactory.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\xmlObjectFramework.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\XMLObjectLoader.h
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\inc\XMLProcedureDescriptor.h
# End Source File
# End Group
# Begin Group "source"

# PROP Default_Filter ""
# Begin Group "utils"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\src\Utils\Base64.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\src\Utils\BZip.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\src\Utils\GArray.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\src\Utils\GBTree.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\src\Utils\GDirectory.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\src\Utils\GException.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\src\Utils\GHash.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\src\Utils\GHTTPMultiPartPOST.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\src\Utils\GList.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\src\Utils\GMemory.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\src\Utils\GPerformanceTimer.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\src\Utils\GProfile.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\src\Utils\GSocketHelpers.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\src\Utils\GStack.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\src\Utils\GString.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\src\Utils\GString0.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\src\Utils\GString32.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\src\Utils\GStringList.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\src\Utils\GThread.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\src\Utils\GZip.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\src\Utils\PluginBuilderLowLevelStatic.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\src\Utils\SHA256.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\src\Utils\TwoFish.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\src\AttributeList.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\src\CacheManager.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\src\DynamicLibrary.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\src\FactoryManager.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\src\FileDataSource.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\src\FrameworkAuditLog.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\src\IntegrationBase.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\src\IntegrationLanguages.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\src\InterfaceParser.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\src\MemberDescriptor.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\src\ObjQueryParameter.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\src\ProcedureCall.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\src\Schema.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\src\SocketHTTPDataSource.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\src\StackFrameCheck.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\src\SwitchBoard.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\src\xmlAttribute.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\src\xmlDataSource.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\src\xmlElement.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\src\xmlElementTree.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\src\XMLException.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\src\xmlLex.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\src\xmlObject.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\src\xmlObjectFactory.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\Libraries\XMLFoundation\src\XMLProcedureDescriptor.cpp
# End Source File
# End Group
# End Group
# End Target
# End Project
