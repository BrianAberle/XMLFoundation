#define _LIBRARY_IN_1_FILE

#ifdef _ANDROID
	#define _NO_SYS_BTIME_
#endif

#ifndef _INCLUDED_XMLFOUNDATION_INLINE_STATIC_LIBRARY_
#define _INCLUDED_XMLFOUNDATION_INLINE_STATIC_LIBRARY_

#include "ServerCore.cpp"

#include "Utils/GSocketHelpers.cpp"
#include "Utils/GArray.cpp"
#include "Utils/CSmtp.cpp"
#include "Utils/GBTree.cpp"
#include "Utils/GException.cpp"
#include "Utils/GHash.cpp"
#include "Utils/GList.cpp"
#include "Utils/GProfile.cpp"
#include "Utils/GProcess.cpp"
#include "Utils/GStack.cpp"
#include "Utils/GString.cpp"
#include "Utils/GString0.cpp"
#include "Utils/GString32.cpp"
#include "Utils/GStringList.cpp"
#include "Utils/GDirectory.cpp"
#include "Utils/GPerformanceTimer.cpp"
#include "Utils/TwoFish.cpp"
#include "Utils/SHA256.cpp"
#include "Utils/BZip.cpp"
#include "Utils/GZip.cpp"
#include "Utils/Base64.cpp"
#include "Utils/PluginBuilderLowLevelStatic.cpp"
#include "Utils/GHTTPMultiPartPOST.cpp"
#include "AttributeList.cpp"
#include "CacheManager.cpp"
#include "FactoryManager.cpp"
#include "FrameworkAuditLog.cpp"
#include "MemberDescriptor.cpp"
#include "FileDataSource.cpp"
#include "ObjQueryParameter.cpp"
#include "ProcedureCall.cpp"
#include "Schema.cpp"
#include "SocketHTTPDataSource.cpp"
#include "StackFrameCheck.cpp"
#include "xmlAttribute.cpp"
#include "xmlDataSource.cpp"
#include "xmlElement.cpp"
#include "xmlElementTree.cpp"
#include "XMLException.cpp"
#include "xmlLex.cpp"
#include "xmlObject.cpp"
#include "xmlObjectFactory.cpp"
#include "XMLProcedureDescriptor.cpp"
#include "SwitchBoard.cpp"
#include "DynamicLibrary.cpp"
#include "IntegrationBase.cpp"
#include "IntegrationLanguages.cpp"



#ifdef _ANDROID
    #ifdef _NO_JAVA_DEPENDENCIES
        #ifdef _USE_SHELL_STUB
            void JavaShellExec(GString &strCommand, GString &strResult)
            {
                //stub out for dynamic lib builds
            }
        #endif
    #else
        // this can be included on ANY platform - not just Android - however it requires that <jni.h> be in path.
        #include "JavaFoundation.cpp"
        extern void JavaShellExec(GString &strCommand, GString &strResult); // in JavaFoundation.cpp
    #endif
#endif


#endif //_INCLUDED_XMLFOUNDATION_INLINE_STATIC_LIBRARY_