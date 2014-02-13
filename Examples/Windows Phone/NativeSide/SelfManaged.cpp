#include "pch.h"
#include "SelfManaged.h"
#include <string.h> // for strcpy()

using namespace NativeSide;
using namespace Platform;

IMPLEMENT_FACTORY(MyCustomObject, Thing)

//
// This is the XML we'll process.
//
char pzXML[] = 
"<Thing Color='Red'>"
	"<String>Owners Word</String>"
	"<Number>777</Number>"
	"<Bool>True</Bool>"
	"<FixedBuffer>native</FixedBuffer>"
	"<Wrapper>"
	     "<StringList>one</StringList>"
	     "<StringList>two</StringList>"
	"</Wrapper>"
	"<Seven77thBit>on</Seven77thBit>"
	"<OnItsSide8>white</OnItsSide8>"
	"<Fifty5>yes</Fifty5>"
"</Thing>";


SelfManaged::SelfManaged(IMyType^ foo)
{
	m_MyManagedObject = foo;
	m_isRunning = 0;
}


GString GStringFromManagedString(Platform::String^ strSource)
{
	return GString(strSource->Data());
}

Platform::String^ MakeCppCXString(GString &strSource)
{
	int nStrLen = (int)strSource.GetLength();
	int nLen = MultiByteToWideChar(CP_ACP, 0, strSource.Buf(),	nStrLen, NULL, NULL);
	wchar_t *ptemp = new wchar_t[nLen+2];
	MultiByteToWideChar(CP_ACP, 0, strSource.Buf(), nStrLen, ptemp, nLen);
	Platform::String^ CppCXString = ref new String(ptemp, (unsigned int)strSource.Length());
	delete ptemp;
	return CppCXString;
}



Platform::String^ SelfManaged::NativeStartHere0()
{
	m_MyManagedObject->MyMethod();

	MyCustomObject O;
	O.FromXMLX(pzXML);

	// look at the Object "O".
	GString strDebug;
	strDebug << "Yo! Check out O:" << O.m_strString << "[" << O.m_nInteger << "][" << O.m_strColor << "]" << O.m_szNative << "\n\n\n";

	// print out the value of each individual bit in m_bits
	// Change "<OnItsSide8>" from "white" to "balck" in pzXML and watch how the bits change
	for(int i=0;i<8;i++)
		strDebug.FormatAppend("bit%d=%d\n",i+1,((O.m_bits&(1i64<<i))!=0));  // :)
//		this is the output:
//			bit1=0
//			bit2=0
//			bit3=0
//			bit4=0
//			bit5=0
//			bit6=0
//			bit7=1
//			bit8=1

	// check the value of the 55th and 60th bits - (only if sizeof(m_bits) is 8 bytes which it is an __int64)
	if ( sizeof(O.m_bits) == 8 )
	{
		strDebug.FormatAppend("bit%d=%d\n",54+1,((O.m_bits&(1i64<<54))!=0)); // check the 55th bit
		strDebug.FormatAppend("bit%d=%d\n",59+1,((O.m_bits&(1i64<<59))!=0)); // check the 60th bit
	}
	
	// set some data 
	O.m_strString = "Root was here";
	O.m_nInteger = 21;
	strcpy_s(O.m_szNative,"so native");
	O.m_bBool = !O.m_bBool;

	// turn the 8th bit off - changes the value to "Black" in the XML - remove the following line and it stays "White" in the output XML
	O.m_bits &= ~(1<<7);

	// add any encoding tags or doctype you need - if you need them other wise skip the next two lines
	GString strXMLStreamDestinationBuffer = "<?xml version=\"1.0\" standAlone='yes'?>\n";
	strXMLStreamDestinationBuffer << "<!DOCTYPE totallyCustom SYSTEM \"http://www.IBM.com/example.dtd\">";
	O.ToXML( &strXMLStreamDestinationBuffer);
/*  
	// this is the xml in strXMLStreamDestinationBuffer 
	<?xml version="1.0" standAlone='yes'?>
	<!DOCTYPE totallyCustom SYSTEM "http://www.IBM.com/example.dtd">
	<Thing Color="Red">
			<Wrapper>
					<StringList>one</StringList>
					<StringList>two</StringList>
			</Wrapper>
			<Number>21</Number>
			<String>Root was here</String>
			<Bool>False</Bool>
			<FixedBuffer>so native</FixedBuffer>
			<Seven77thBit>True</Seven77thBit>
			<OnItsSide8>Black</OnItsSide8>
	</Thing>
*/	

	strDebug << strXMLStreamDestinationBuffer;
	return MakeCppCXString(strDebug);
}

GString g_WindowsPhoneInfoLog;
void WinPhoneInfoLog(int nMsgID, GString &strSrc)
{
	g_WindowsPhoneInfoLog << "[" << nMsgID << "]:" << strSrc << "\n";
}
Platform::String^ SelfManaged::GetServerLogData()
{
	Platform::String^ strResult;
	strResult = MakeCppCXString(g_WindowsPhoneInfoLog);
	g_WindowsPhoneInfoLog.Empty();
	return strResult;
}

char *pzBoundStartupConfig = 
"[System]\r\n"
"Pool=3\r\n"
"ProxyPool=2\r\n"
"LogFile=%s\r\n"								// 1. Log File (/sdcard/Download/ServerLog.txt)
"LogCache=0\r\n"
"\r\n"
"[HTTP]\r\n"
"Enable=%s\r\n"									// 2. Enable HTTP
"Index=%s\r\n"									// 3. HTTP Index.html
"Home=%s/\r\n"									// 4. HTTP Home Dir
"Port=8080\r\n"
"\r\n"
"[Trace]\r\n"
"ConnectTrace=%s\r\n"							// 5. Connect Trace
"ThreadTrace=1\r\n"
"TransferSizes=0\r\n"
"SocketHandles=0\r\n"
"\r\n";

#include "../../../Servers/Core/ServerCore.cpp"

// in mainpage.xaml.cs this value is passed in: "ServerLog.txt&&Yes&&Index.html&&c:\\user\\desktop&&Yes"
Platform::String^ SelfManaged::StartServerCore(Platform::String^ strStartupArgs)
{
	if (!m_isRunning)
	{
		m_isRunning = 1;
		SetServerCoreInfoLog( WinPhoneInfoLog );

		GString g("Starting Server");
		WinPhoneInfoLog(777, g);

		
		GString str(strStartupArgs->Data());
		GStringList list("&&",str);
		GStringIterator it(&list);
		const char *p1 = it++;
		const char *p2 = it++;
		const char *p3 = it++;
		const char *p4 = it++;
		const char *p5 = it++;

		GString strCfgData;
		strCfgData.Format(pzBoundStartupConfig,p1,p2,p3,p4,p5);

		SetProfile(new GProfile((const char *)strCfgData, (int)strCfgData.Length()));
		int nRet = server_start("-- Windows Phone Server --");
		return MakeCppCXString(g_WindowsPhoneInfoLog);
	}

	GString g("Server is already running");
	WinPhoneInfoLog(777, g);
	return MakeCppCXString(g_WindowsPhoneInfoLog);
}

Platform::String^ SelfManaged::Test()
{
	SetServerCoreInfoLog( WinPhoneInfoLog ); // do this incase it has not been done already

	//	server_stop();
	server_build_check();
	return MakeCppCXString(g_WindowsPhoneInfoLog);
}

