// --------------------------------------------------------------------------
//						United Business Technologies
//			  Copyright (c) 2000 - 2017  All Rights Reserved.
//
// Source in this file is released to the public under the following license:
// --------------------------------------------------------------------------
// This toolkit may be used free of charge for any purpose including corporate
// and academic use.  For profit, and Non-Profit uses are permitted.
//
// This source code and any work derived from this source code must retain 
// this copyright at the top of each source file.
// --------------------------------------------------------------------------
#include "GlobalInclude.h"


// must always be defined for external building to know this file and it's definitions has been included
#define __SERVER_CORE	

///////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef __NO_OPENSSL		// prevent the openssl calls and library link requirements by defining __NO_OPENSSL prior to including ServerCore.cpp
	#define __OPENSSL
#endif
///////////////////////////////////////////////////////////////////////////////////////////////////////

#include "XMLFoundation.h"	// lists, sorts, xmlparser, object factory, cipher, compression, and other app framework tools

//#define _XMLF_JIT_    // comment out this line to allow JIT Debugging(just In Time) to be hooked by your debugger


#ifdef _WIN32 // Windows 32 bit, 64 bit, Windows Mobile, and Windows Phone all define _WIN32
	#ifdef WINCE
		#include "WinCERuntimeC.h" // Adds common C Runtime functions to Windows CE.
		#include <sys/types.h>
	#elif __WINPHONE
		#include <time.h>			// for tm struct and time()	
		#include <Ws2tcpip.h>		
		#include <errno.h>
		#include <direct.h>			// for mkdir()
	#else // 32 and 64 bit and also older MSC compilers

		#include <winsock.h>		// added August 2015 for ntohl() and many more needed definitions now that <windows.h> is not yet included due to COM_NO_WINDOWS_H, RPC_NO_WINDOWS_H, and WIN32_LEAN_AND_MEAN
		#define socklen_t int
		#include <time.h>			// for tm struct and time()	
		#include <io.h>				// for _chmod() in _WIN32
		#include <process.h>		// for _spawnv()
		#include <direct.h>			// for mkdir()
		#include <errno.h>
		#if defined(_MSC_VER) && _MSC_VER <= 1200 
			#ifndef ETIMEDOUT
			  #define ETIMEDOUT               WSAETIMEDOUT
			#endif
		#endif
	#endif
#else // Linux & Solaris & HPUX & AIX & Android & iOS
	#include <wchar.h>      // for wcslen()
	#include <arpa/inet.h>	// for inet_addr()
	#include <netdb.h>		// for gethostbyname()
	#include <unistd.h>		// for gethostname(), execv()
#endif

#ifdef _ANDROID
	#include <errno.h>		// for definition of EBUSY (16) Note: Needed ONLY for Android port
	// Android NDK pthreads does not have a pthread_cancel() so ServerCore's calls to gthread_cancel 
	// are #defined to pthread_cancel and resolved by this function here for the Andriod build
	int pthread_cancel(pthread_t pthread_id)
	{
		pthread_kill(pthread_id, SIGUSR1);
		return 0;
	}
#endif

// common to all platforms
#include <ctype.h>		// for isprint()
#include <stdlib.h>		// for atol(), strtol(), srand(), rand(), atoi()
#include <string.h>		// for strcmp(), memcpy(), memset(), strlen(), strpbrk()
#include <sys/stat.h>	// for stat struct, stat(), and file stats flags
 
#ifdef _AIX
 #include <sys/select.h>		// for nonblocked IO event select()
#endif


#include "IntegrationLanguages.h" // integration to Perl, Python, Java, C++, VB, COM, CORBA, and TransactXML extensions
GList g_lstActivePlugins;


#ifdef __OPENSSL
	#include <openssl/md4.h>
	#include <openssl/md5.h>
	#include <openssl/hmac.h>
	#include <openssl/des.h>
	#include <openssl/opensslv.h>

	// you can define NO_PRAGMA_CRYPTO_LINK then add libeay32.lib to the linker input libraries at any path you prefer
	#ifndef NO_PRAGMA_CRYPTO_LINK
		#ifdef _WIN32
			#ifdef _WIN64
				// You can always hardcode the openssl libs with a full path like this:
				//#pragma comment(lib,    "c:/XMLFoundation/Libraries/openssl/bin-win64/libeay32.lib")	


			#elif __WINPHONE
				#pragma comment(lib, "../../../Libraries/openssl/bin-winphone/libeay32.lib") // the extra .. needs to be there for the WindowsPhone example
			#else
                // this preprocessor condition is for 32 bit Windows builds

				//  openssl uses _iob
				//	https://social.msdn.microsoft.com/Forums/vstudio/en-US/4a1c9610-fa41-45f6-ad39-c9f6795be6f2/msvcrt-iob-disappeared?forum=vclanguage#page:2
				//
				#if defined(_MSC_VER) && _MSC_VER > 1200 

					// unless _NO_IOB__ is defined,     _iob is added.
					// on August 22, 2017 using the latest openssl obtained from [https://github.com/Microsoft/openssl/] along with the latest version of VS2017 causes these unresolved items that are resolved here.
					// This is true building for 32 bit targets only, the 64 bit Windows Runtime with VS2017 on does have these symbols (as of August 22, 2017)
					#ifndef _NO_IOB__
						FILE* __cdecl __iob_func(void)
						{
							return stdout;
						}
						extern "C" { FILE _iob[3] = { __iob_func()[0], __iob_func()[1], __iob_func()[2] }; }
						#define _NO_IOB__

						// likewise with the __iob issues resolved above - _fprintf went missing - resolved here under its own preprocessor condition so that it could be isolated out independant of _iob should you be linking with a c-runtime that has it
						#ifndef _NO_FPRINTF__
							extern "C" int __cdecl _fprintf(FILE* file, const char *format, ...)
							{
								va_list _ArgList;
								__crt_va_start(_ArgList, format);
								int _Result = fprintf(file, format, _ArgList);
								__crt_va_end(_ArgList);
								return _Result;
							}

							// __vsnprintf is referenced in function _OPENSSL_showfatal and does not exist in the VS2017 C Runtime
							int (WINAPIV * __vsnprintf)(char *, size_t, const char*, va_list) = _vsnprintf;
							#define _NO_FPRINTF__
						#endif			

					#endif
				#else
					// this preprocessor condition is a 32 bit windows build - VC6 where relative linking was also moved to the application layer.

					// #pragma comment(lib,       "../../Libraries/openssl/bin-win32/libeay32.lib")
					// #pragma comment(lib,    "../../../Libraries/openssl/bin-win32/libeay32.lib")


				#endif
			#endif
		#endif
	#endif
#else
	// __OPENSSL is not defined.
	#include "md4.h"  // not actually used unless HTTP Proxy Authentication is "Negotiate" and that currently cant happen unless __OPENSSL is defined for the HMAC interfaces
	#include "md5.h"  // not actually used unless HTTP Proxy Authentication is "Negotiate" and that currently cant happen unless __OPENSSL is defined for the HMAC interfaces
#endif



#ifdef ___XFER
	#include "../../Libraries/Xfer/inc/XferCommand.h"		// Xfer Protocol
	#include "../../Libraries/Xfer/inc/XferProtocol.h"		// Xfer Protocol
	#include "../../Libraries/Xfer/inc/XferDispatch.h"		// Xfer Protocol
	#include "../../Libraries/Xfer/inc/XferSwitchBoard.h"	// Xfer Protocol
	// Note: you must also link to "../../Libraries/Release/Xfer.lib" or "../../Libraries/Debug/Xfer.lib" or you can include the source like this:
	#ifdef ___XFER_SRC
		#include "../../Libraries/Xfer/src/XferCommand.cpp"		// Xfer Protocol
		#include "../../Libraries/Xfer/src/XferProtocol.cpp"	// Xfer Protocol
		#include "../../Libraries/Xfer/src/XferDispatch.cpp"	// Xfer Protocol
		#include "../../Libraries/Xfer/src/XferSwitchBoard.cpp"	// Xfer Protocol
	#else
		#pragma comment(lib, "../../Libraries/Xfer.lib")	
	#endif
#else
#endif //___XFER

#include "SwitchBoard.h"		
#include "GSocketHelpers.h" // nonblocksend(), nonblockrecvAny(), readableTimeout() writableTimeout() ntoh64(), SOCK_ERR
#include "GPerformanceTimer.h" // only used for getTimeMS() definition
#include "PluginBuilderLowLevelStatic.h"

#ifdef ___TXML
 #include "ServerGlobal.cpp"	// UBT Server extension for TransactXML
#else
 #define OBJECT_PRECACHE
 #define SERVER_MEMORY_EXTENSION
 #define SERVER_MEMORY_CLEANUP
 #define SERVER_MEMORY_RESET
 #define PROPIGATE_SERVER_EXCEPTION
#endif

#ifndef _NO_GZIP_
 #include "GZip.h"
#endif


// MAX_RAW_CHUNK can be any size you choose, but realistically it should not be less than 8192 in most cases, and it will reject big packets from the standard build, that may be desirable in some cases.
// This thread buffer doubles as the kernel TCP buffer. Each thread will allocate 2 buffers of this size.  The value should align on a page boundary, since the waste will be multiplied relative to the number of threads if it does not.
#define MAX_RAW_CHUNK		65536					// 64k max size of raw packet data, this is 8X larger than the OS kernel default in NT, cut it in half if you don't have memory to burn and the performance loss will be minimal.
// note: Xfer does assume that MAX_RAW_CHUNK > 2048, it can be smaller - but you must search for "1024 is not magic" and adjust it accordingly if you run very small packets.  Other than that no assumptions are made in the code.
#define MAX_DATA_CHUNK		MAX_RAW_CHUNK + 2048	// max size of compressed/ciphered data packet, it is possible for compressed data to expand a little under worst case scenerios
// The actual formula for buffer expansion is = (n * .01) + 600, so this size is safe and reserves working space that is used to prefix data such as HTTP headers, 256 bytes is the most Xfer ever currently uses.
// **** Note: Make thise values match XFER_RAW_CHUNK, and XFER_DATA_CHUNK respectively


// Data conversion attributes. 
#ifndef _ATTRIBS_DEFINED
#define _ATTRIBS_DEFINED
#define ATTRIB_SYNCRONIZED				0x0001
#define ATTRIB_COMPRESSED_GZIP			0x0002
#define ATTRIB_COMPRESSED_BZIP			0x0004
#define ATTRIB_CRYPTED_2FISH			0x0008
#define ATTRIB_UNUSED_BIT5_RESERVED		0x0010	// search [bValidDataFlags] in XferDispatch.cpp
#define ATTRIB_UNUSED_BIT6_RESERVED		0x0020	// for an explanation of what the reserved bits are for.
#define ATTRIB_UNUSED_BIT7_RESERVED		0x0040
#define ATTRIB_UNUSED_BIT8_RESERVED		0x0080
#define ATTRIB_UNUSED_BIT9				0x0100
#define ATTRIB_UNUSED_BIT10				0x0200
#define ATTRIB_UNUSED_BIT11				0x0400
#define ATTRIB_UNUSED_BIT12				0x0800
#define ATTRIB_UNUSED_BIT13				0x1000
#define ATTRIB_UNUSED_BIT14				0x2000
#define ATTRIB_UNUSED_BIT15				0x4000
#define ATTRIB_UNUSED_BIT16				0x8000
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////
// Turn off Nagle Algorithm.  This is a major item to note with respect to performance within Internet protocols.  
// The Nagle Algorithm is all about improving performance of TCP applications via a generalized buffering algorithm.
// ServerCore uses it's own buffering system.  There are two "Large" system buffers.  Go ask Nagle.  Mine are big.
// Large is relative to year and device.  Nagle didnt have 2 64k system buffers.  I buffer better than Nagle, 
// especially considering that all of the protocols I have implemented via ServerCore 'hooks' (designed after ServerCore)
// use the reserved system buffers within the protocol design.  Nagle generally improves TCP application performance
// however Nagle can't help this server - it only slows it down and wastes memory.  Search for "Turn off Nagle Algorithm"
#ifndef TCP_NODELAY 
	#define TCP_NODELAY 1   
#endif
///////////////////////////////////////////////////////////////////////////////////////////////////


// These are 3 Major Logic threads that a connection uses.  Needless to say - they use thread synchronization
void *clientThread(void *);
void *ProxyHelperThread(void *);
void *listenThread(void *);


// The FilePoster example uses this, this functioning "switchboard" implementation has changed little since since 2002.
SwitchBoard			g_SwitchBoard;

// global variables - (not user settings)
int g_bEnableXferSwitchBoard = 0;
GString g_strMessagingSwitchPath;
int g_bEnableMessagingSwitchboard = 0; 
gthread_mutex_t PoolExpansionLock;
GArray *g_PoolClientThread;
GArray *g_PoolProxyHelperThread;
int g_ServerIsShuttingDown = 0;
int g_ThreadPoolSize = 0;
int g_ProxyPoolSize = 0;



#define ARRAY_OF_OUTSTANDING_CHALLENGES						32 // 32 is the number of concurrent challenge requests supoported - the number can be any value
unsigned __int64 g_arrChallenges[ARRAY_OF_OUTSTANDING_CHALLENGES]; 
gthread_mutex_t g_csMtxChallengeList;



///////////////////////////////////////////////////////////////////////////////////////////////////////
// Build flags - LOGFILE_CACHED_WRITE
// If you are targeting a phone/tablet/embedded system use a smaller value
// If you are targeting a Linux/Windows/Unix server use > value
#ifndef LOGFILE_CACHED_WRITE
	#define LOGFILE_CACHED_WRITE	128000
//	#define LOGFILE_CACHED_WRITE	32000
#endif

#ifndef PRIMARY_POOL_SIZE
	#define PRIMARY_POOL_SIZE	    250
#endif
#ifndef PROXY_POOL_SIZE
	#define PROXY_POOL_SIZE			250
	// if you only want the HTTP server and not the HTTP Proxy you can set
	// #define PROXY_POOL_SIZE			0
#endif


int g_ThreadIDCount = 0;
int g_TraceIn = 0;
int g_TraceOut = 0;
int g_TraceSocketHandles = 0;
int g_TraceHTTPBalance = 0;
int g_nRrandSeed = 777;		
int g_DataTrace = 0;
int g_ThreadPing = 0;
int g_PingListenThread = 0;
int g_TraceConnection = 0;
int g_HTTPHeaderTrace = 0;
int g_TraceHTTPFiles = 0;

int g_TraceThread = 0;
int g_LogStdOut = 0;
int g_listenThreadsRunning = 0;
int g_bPartialHeaderWait = 1;
int g_nPartialHeaderWaitSec = 5;	// 5:		normally HTTP headers arrive in a single read from the network, if a header is partially received allow N seconds for the rest of the header.  This guards against a certain type denial of service attack.
int g_nProxyReadWait = 75000;		// 75000:	in nanoseconds, time to block on IO reads in the proxy thread
int g_nProxyIdleReadWait = 1000000; // 1000000:	idle connection should block longer on IO reads
int g_nKernelSocketRcvBuf = 32768;	// 32768:	4 * default(8192 in 2K/XP/Vista)  If we see limits reached on page locks (ERROR_INSUFFICIENT_RESOURCES or WSAENOBUFS)  reduce the size
int g_nListenQueue = 64;			// 64:		Connections in this queue should not exist for more than a few microseconds.
int g_nProxyIdleAt = 20;			// 20:		idle proxy enters a lower processor time slice after [g_nProxyIdleAt] network reads at[g_nProxyReadWait] of no network activity.  Data on the network automatically re-elevates the time slice.
int g_nMaxHTTPHeaderSize = 4096;	// 4096:	4096 is plenty unless you are doing something custom
int g_StopDoubleBuffer = 1;			// 1:		when on(1) this sets SO_SND_BUF to 0, preventing double buffering since this server goes to great lengths to provide optimal buffering, when off(0) the TCP Nagle algorithm will queue up small data writes - but normally there are'nt any.  Some protocols will need Nagle, and they should turn it on themselves.
int g_bDisableLog = 0;
int g_NoLog = 0;
int g_nBlockedPortPenalty = 777;
int g_bUseHTTPKeepAlives = 1;
int g_nMaxConnections = 500; // max connections from a single IP or sub-net connecting to this server
int g_nTimeLimit = 0;
int g_LogCache = 1; 
int g_nKeepAliveTimeout = 300;  // the timeout= in the Keep-Alive of the HTTP header  ("HTTP","KeepAliveTimeOut")
unsigned int g_nMaxKeepAlives = 150;		// the max= in the Keep-Alive of the HTTP header  ("HTTP","KeepAliveMax")
int g_nServerShutdownWait = 3;
//////////////////////////////////////////////////////
// Defaults for the HTTPProxy, implemented as "Protocol 11"
bool g_bNeedHTTPProxyAuth = 1;
int g_nHTTPProxyTimeout = 300; 
GString g_strHTTPProxyAgentName;		 // defaults to "5Loaves"
bool g_bHTTPProxyEnabled = 1;
GString g_strHTTPProxyBlocked;
GString g_strHTTPProxyCacheBlocked;
GString g_strHTTPProxyNotBlocked;
bool g_bHTTPProxyRestrictToSubnet = 1;	 // g_pstrHTTPProxyRestrictList will be in effect when true
GStringList *g_pstrHTTPProxyRestrictList;// LAN subnet prefix, semicolon seperated list. like 192.168;10.10.1  block all but connections from 10.10.1.* and 192.168.*
bool g_bEnforceNTLMv2Timestamp = 1;		 // A Windows CE device may not have a real-time clock, therefore NTLMv2 from those devices has no timestamp.
int  g_nAuthTimeoutSeconds = 7200;		 // 7200 seconds is 2 hours.  NTLM authenticates connections for this amount of time.
bool g_bTraceHTTPProxy = 0;
bool g_bTraceHTTPProxyReturn = 0;
bool g_bTraceHTTPProxyBinary = 1;
bool g_bTraceHTTPProxyFiles = 0;		 // enable to write "atomic" http transaction files, 1 per HTTP transaction kind.
bool g_bHTTPProxyLogExtra = 1;
bool g_bBlockGoogleTracking = 1;		 // enable this for better performance or privacy or to help save the earth.  Yahoo and Bing use HBase which uses less electricity than BigTable.  Google should be forced by law to upgrade for the sake of planet earth.  Until then, then helps.
bool g_bOnly443or80 = 1;				 // by default, this HTTP Proxy only allows port 80 and 443.
bool g_bBlockHTTPProxyAds = 1;			 // Ads make the Internet difficult to use for new users, so this will help make the Internet a better place that is less confusing to surf.  It birthed in Acedemia and .edu will see it through.  However we know ads are getting smarter - they can be in video and custom websites.
bool g_bEnableOutboundConnectCache = 0;  // testing  -  an enterprise mid-tier or ISP server WILL want this, a phone or tablet MIGHT want this.
bool g_bNegotiateBasic = 0;				 // for a more secure server, set to false and only allow more secure options.  Modern browsers like IE, Firefox, and Safari support NTLM. 
bool g_bNegotiateNTLMv2 = 1;			 // only matters if __OPENSSL is defined, then it enables/disables the NTLM Negotiate forcing Basic Authentication
//////////////////////////////////////////////////////
GString g_strHTTPHomeDirectory; 
GString g_strLogFile;
GString g_strListeningOnPorts;

GHash g_OutboundConnectCache;
gthread_mutex_t g_csMtxConnectCache;


// stats for clientthread(), there are [g_ThreadPoolSize] of them
long g_nClientthreadsInUse = 0;;
long g_nClientthreadsPeakUse = 0;

// stats for ProxyThread(), there are [g_ProxyPoolSize] of them
long g_nProxyThreadsInUse = 0;;
long g_nProxyThreadsPeakUse = 0;

time_t g_ServerStartTime;

// more statistic counters
__int64 g_TotalHits = 0;
__int64 g_Total404s = 0;
__int64 g_TotalTooBusy = 0;
__int64 g_PreZipBytes = 0;
__int64 g_PostZipBytes = 0;

#if defined(_WIN32) && !defined(_WIN64)
	long g_TotalConnections = 0;
	long g_ActiveConnections = 0;
#else
	volatile __int64 g_TotalConnections = 0;
	volatile __int64 g_ActiveConnections = 0;
#endif

int g_nLockThreadBuffers = 1;

#ifndef _WIN32
char g_chSlash = '/';
#else
char g_chSlash = '\\';
#endif


// defined in in GString.cpp, K & R forgot this one
char * stristr(const char * str1, const char * str2); 
static const char *g_pzCoreErrorSection = "ServerCore";
unsigned int g_RandomNumbers[8];


typedef void(*CoreInfoLog )(int, GString &);
CoreInfoLog g_CoreLog = 0;
void SetServerCoreInfoLog( void (*pfn) (int, GString &) )
{
	g_CoreLog = pfn;
}


// http://pgl.yoyo.org/adservers/serverlist.php?skip[]=101com.com
const char *g_pAds = "101order.com,123found.com,180hits.de,180searchassistant.com,1x1rank.com,207.net,247media.com,24log.com,24log.de,24pm-affiliation.com,2mdn.net,2o7.net,360yield.com,4affiliate.net,4d5.net,50websads.com,518ad.com,51yes.com,600z.com,777partner.com,777seo.com,77tracking.com,7adpower.com,7bpeople.com,7search.com,99count.com,a-ads.com,a-counter.kiev.ua,a.0day.kiev.ua,a.collective-media.net,a.consumer.net,a.mktw.net,a.sakh.com,a.ucoz.net,a.ucoz.ru,a.wowinterface.com,a.xanga.com,a32.g.a.yimg.com,aaddzz.com,abacho.net,abc-ads.com,abz.com,ac.rnm.ca,accounts.pkr.com,acsseo.com,actionsplash.com,actualdeals.com,ad-flow.com,ad-images.suntimes.com,ad-pay.de,ad-rotator.com,ad-server.gulasidorna.se,ad-serverparc.nl,ad-souk.com,ad-space.net,ad-tech.com,ad-universe.com,ad-up.com,ad.100.tbn.ru,ad.103092804.com,ad.37.com,ad.71i.de,ad.980x.com,ad.a8.net,ad.abcnews.com,ad.abctv.com,ad.about.com,ad.aboutit.de,ad.aboutwebservices.com,ad.abum.com,ad.allstar.cz,ad.altervista.org,ad.amgdgt.com,ad.anuntis.com,"
"ad.auditude.com,ad.bigpoint.net,ad.bizo.com,ad.bnmla.com,ad.bondage.com,ad.caradisiac.com,ad.centrum.cz,ad.cgi.cz,ad.chip.de,ad.choiceradio.com,ad.clix.pt,ad.cooks.com,ad.crwdcntrl.net,ad.digitallook.com,ad.doctissimo.fr,ad.domainfactory.de,ad.e-kolay.net,ad.eurosport.com,ad.f1cd.ru,ad.foxnetworks.com,ad.freecity.de,ad.gate24.ch,ad.gdmexchange.com,ad.globe7.com,ad.grafika.cz,ad.harmony-central.com,ad.hbv.de,ad.hodomobile.com,ad.httpool.com,ad.hyena.cz,ad.iinfo.cz,ad.ilove.ch,ad.infoseek.com,ad.investopedia.com,ad.ir.ru,ad.jamba.net,ad.jamster.co.uk,ad.jetsoftware.com,ad.keenspace.com,ad.leadbolt.net,ad.liveinternet.ru,ad.lupa.cz,ad.m5prod.net,ad.media-servers.netad.mediastorm.hu,ad.mgd.de,ad.moscowtimes.ru,ad.musicmatch.com,ad.nachtagenten.de,ad.netdialogs.de,ad.nozonedata.com,ad.nttnavi.co.jp,ad.nwt.cz,ad.onad.eu,ad.pandora.tv,ad.pbs.bb.ru,ad.playground.ru,ad.preferances.com,ad.profiwin.de,ad.prv.pl,ad.rambler.ru,ad.reunion.com,ad.scanmedios.com,ad.sensismediasmart.com.au,ad.seznam.cz,ad.simgames.net,"
"ad.slutload.com,ad.smartclip.net,ad.tbn.ru,ad.technoratimedia.com,ad.thewheelof.com,ad.top50.to,ad.tv2.no,ad.tweakpc.de,ad.twitchguru.com,ad.usatoday.com,ad.virtual-nights.com,ad.watch.impress.co.jp,ad.wavu.hu,ad.way.cz,ad.weatherbug.com,ad.wsod.com,ad.wz.cz,ad.yadro.ru,ad.yourmedia.com,ad.zanox.com,ad0.bigmir.net,ad01.mediacorpsingapore.com,ad1.emediate.dk,ad1.emule-project.org,ad1.kde.cz,ad1.pamedia.com.au,ad2.bb.ru,ad2.doublepimp.com,ad2.iinfo.cz,ad2.ip.ro,ad2.linxcz.cz,ad2.lupa.cz,ad2.pamedia.com.au,ad2flash.com,ad2games.com,ad3.iinfo.cz,ad3.pamedia.com.au,ad4game.com,adaction.de,adadvisor.net,adaos-ads.net,adap.tv,adapt.tv,adbanner.ro,adbard.net,adblade.com,adboost.de.vu,adboost.net,adbooth.net,adbot.com,adbrite.com,adbroker.de,adbunker.com,adbureau.net,adbutler.com,adbutler.de,adbuyer.com,adbuyer3.lycos.com,adcash.com,adcast.deviantart.com,adcell.de,adcenter.mdf.se,adcenter.net,adcentriconline.com,adcept.net,adclick.com,adclient.rottentomatoes.com,adclient.uimserv.net,adclient1.tucows.com,"
"adcloud.net,adcomplete.com,adconion.com,adcontent.gamespy.com,adcycle.com,add.newmedia.cz,addealing.com,addesktop.com,addfreestats.com,addme.com,adecn.com,ademails.com,adengage.com,adexpose.com,adext.inkclub.com,adf.ly,adfair.se,adfarm.mediaplex.com,adflight.com,adfly.com,adforce.com,adform.com,adgardener.com,adgoto.com,adgridwork.com,adhese.be,adhese.com,adhostcenter.com,adi.mainichi.co.jp,adimage.asiaone.com.sg,adimage.guardian.co.uk,adimages.been.com,adimages.carsoup.com,adimages.go.com,adimages.homestore.com,adimages.sanomawsoy.fi,adimg.cnet.com,adimg.com.com,adimg.uimserv.net,adimg1.chosun.com,adimgs.sapo.pt,adimpact.com,adincl.gopher.com,adinjector.net,adinterax.com,adisfy.com,adition.com,adition.de,adition.net,adjix.com,adjug.com,adjuggler.com,adjuggler.yourdictionary.com,adjustnetwork.com,adk2.com,adland.ru,adlantic.nl,adlegend.com,adlink.de,adlog.com.com,adloox.com,adlooxtracking.com,adm.ad.asap-asp.net,admagnet.net,admailtiser.com,adman.gr,adman.in.gr,adman.otenet.gr,admanagement.ch,"
"admanager.btopenworld.com,,admanager.carsoup.com,admarketplace.net,admarvel.com,admax.nexage.com,admedia.com,admedia.ro,admeld.com,admeta.com,admex.com,adminde.ivwbox.de,adminder.com,adminshop.com,admob.com,admonitor.com,admotion.com.ar,adnet-media.net,adnet.asahi.com,adnet.biz,adnet.de,adnet.ru,adnet.worldreviewer.com,adnetinteractive.com,adnetwork.net,adnews.maddog2000.de,adocean.pl,adonspot.com,adoperator.com,adorigin.com,adpepper.dk,adpepper.nl,adperium.com,adpick.switchboard.com,adpost.com,adprofile.net,adprojekt.pl,adq.nextag.com,adreactor.com,adremedy.com,adreporting.com,adres.internet.com,adrevolver.com,adriver.ru,adrolays.de,adroll.com,adrotate.de,adrotator.net,adrotator.se,ads-click.com,ads.128b.com,ads.4tube.com,ads.5ci.lt,ads.abovetopsecret.com,ads.aceweb.net,ads.activestate.com,ads.adfox.ru,ads.administrator.de,ads.adshareware.net,ads.adultfriendfinder.com,ads.adultking.com,ads.adultswim.com,ads.advance.net,ads.adverline.com,ads.affiliates.match.com,ads.ak.facebook.com,"
"ads.ak.facebook.com.edgesuite.net,ads.allvatar.com,ads.alt.com,ads.alwayson-network.com,ads.amdmb.com,ads.amigos.com,ads.anm.co.uk,ads.aol.co.uk,ads.aol.com,ads.apn.co.nz,ads.appsgeyser.com,ads.as4x.tmcs.net,ads.as4x.tmcs.ticketmaster.com,ads.asia1.com.sg,ads.asiafriendfinder.com,ads.ask.com,ads.aspalliance.com,ads.avazu.net,ads.batpmturner.com,ads.beenetworks.net,ads.belointeractive.com,ads.berlinonline.de,ads.betanews.com,ads.betfair.com,ads.betfair.com.au,ads.bigchurch.com,ads.bigfoot.com,ads.billiton.de,ads.bing.com,ads.bittorrent.com,ads.blog.com,ads.bloomberg.com,ads.bluelithium.com,ads.bluemountain.com,ads.bluesq.com,ads.bonniercorp.com,ads.boylesports.com,ads.brabys.com,ads.brain.pk,ads.brazzers.com,ads.bumq.com,ads.businessweek.com,ads.canalblog.com,ads.canoe.ca,ads.carocean.co.uk,ads.casinocity.com,ads.cbc.ca,ads.cc,ads.cc-dt.com,ads.cdfreaks.com,ads.centraliprom.com,ads.cgnetworks.com,ads.channel4.com,ads.cimedia.com,ads.clearchannel.com,ads.cnn.com,ads.co.com,ads.collegclub.com,"
"ads.com.com,ads.comicbookresources.com,ads.contactmusic.com,,ads.crakmedia.com,ads.creative-serving.com,ads.creativematch.com,ads.cricbuzz.com,ads.cyberfight.ru,ads.cybersales.cz,ads.dada.it,ads.datinggold.com,ads.datingyes.com,ads.dazoot.ro,ads.deltha.hu,ads.dennisnet.co.uk,ads.desmoinesregister.com,ads.detelefoongids.nl,ads.deviantart.com,ads.digital-digest.com,ads.digitalmedianet.com,ads.digitalpoint.com,ads.directionsmag.com,ads.discovery.com,ads.domeus.com,ads.eagletribune.com,ads.easy-forex.com,ads.eatinparis.com,ads.economist.com,ads.edbindex.dk,ads.einmedia.com,ads.electrocelt.com,ads.elitetrader.com,ads.emirates.net.ae,ads.epltalk.com,ads.escalatemedia.com,ads.esmas.com,ads.eu.msn.com,ads.exactdrive.com,ads.exhedra.com,ads.expat-blog.biz,ads.expedia.com,ads.ezboard.com,ads.factorymedia.com,ads.fairfax.com.au,ads.faxo.com,ads.ferianc.com,ads.filmup.com,ads.financialcontent.com,ads.flooble.com,ads.fool.com,ads.footymad.net,ads.forbes.com,ads.forbes.net,ads.forium.de,ads.fortunecity.com,"
"ads.fotosidan.se,ads.foxkidseurope.net,ads.foxnetworks.com,ads.foxnews.com,ads.freecity.de,ads.freeze.com,ads.friendfinder.com,ads.ft.com,ads.futurenet.com,ads.gamecity.net,ads.gameforgeads.de,ads.gamershell.com,ads.gamespyid.com,ads.gamigo.de,ads.gaming-universe.de,ads.gawker.com,ads.geekswithblogs.net,ads.glispa.com,ads.globeandmail.com,ads.gmodules.com,ads.godlikeproductions.com,ads.good.is,ads.goyk.com,ads.gplusmedia.com,ads.gradfinder.com,ads.grindinggears.com,ads.groundspeak.com,ads.gsm-exchange.com,ads.gsmexchange.com,ads.guardian.co.uk,ads.guardianunlimited.co.uk,ads.guru3d.com,ads.h1351109.stratoserver.net,ads.hardwaresecrets.com,ads.harpers.org,ads.hbv.de,ads.hearstmags.com,ads.heartlight.org,ads.hideyourarms.com,ads.hollywood.com,ads.horsehero.com,ads.horyzon-media.com,ads.iafrica.com,ads.ibest.com.br,ads.ibryte.com,ads.icq.com,ads.iforex.com,ads.ign.com,ads.img.co.za,ads.imgur.com,ads.incgamers.com,ads.indiatimes.com,ads.infi.net,ads.internic.co.il,ads.ipowerweb.com,"
"ads.isoftmarketing.com,ads.itv.com,ads.iwon.com,ads.jewishfriendfinder.com,ads.jiwire.com,,ads.jobsite.co.uk,ads.jpost.com,ads.jubii.dk,ads.justhungry.com,ads.kaktuz.net,ads.kelbymediagroup.com,ads.kinobox.cz,ads.kinxxx.com,ads.komli.com,ads.kompass.com,ads.krawall.de,ads.lesbianpersonals.com,ads.linuxfoundation.org,ads.linuxjournal.com,ads.linuxsecurity.com,ads.livenation.com,ads.localnow.com,ads.lvz-online.de,ads.mambocommunities.com,ads.mariuana.it,ads.massinfra.nl,ads.mcafee.com,ads.mediaodyssey.com,ads.mediaturf.net,ads.medienhaus.de,ads.mgnetwork.com,ads.mmania.com,ads.moceanads.com,ads.motor-forum.nl,ads.motormedia.nl,ads.movieflix.com,ads.msn.com,ads.multimania.lycos.fr,ads.nationalgeographic.com,ads.ncm.com,ads.netclusive.de,ads.netmechanic.com,ads.networksolutions.com,ads.newdream.net,ads.newgrounds.com,ads.newmedia.cz,ads.newsint.co.uk,ads.newsquest.co.uk,ads.newtention.net,ads.nigella.com,ads.ninemsn.com.au,ads.nj.com,ads.nola.com,ads.nordichardware.com,ads.nordichardware.se,ads.nwsource.com,"
"ads.nyi.net,ads.nytimes.com,ads.nyx.cz,ads.nzcity.co.nz,ads.o2.pl,ads.oddschecker.com,ads.okcimg.com,ads.ole.com,ads.olivebrandresponse.com,ads.oneplace.com,ads.ookla.com,ads.optusnet.com.au,ads.outpersonals.com,ads.overclockers.at,ads.p161.net,ads.passion.com,ads.pennet.com,ads.penny-arcade.com,ds.pheedo.com,ads.phpclasses.org,ads.pickmeup-ltd.com,ads.pkr.com,ads.planet.nl,ads.pni.com,ads.pof.com,ads.powweb.com,ads.primissima.it,ads.prisacom.com,ads.program3.com,ads.psd2html.com,ads.pushplay.com,ads.quaylemedia.com,ads.quoka.de,ads.rcs.it,ads.realmedia.de,ads.recoletos.es,ads.rediff.com,ads.redlightcenter.com,ads.redtube.com,ads.resoom.de,ads.returnpath.net,ads.rottentomatoes.com,ads.rpgdot.com,ads.s3.sitepoint.com,ads.satyamonline.com,ads.savannahnow.com,ads.scifi.com,ads.seniorfriendfinder.com,ads.sexinyourcity.com,ads.shizmoo.com,ads.shopstyle.com,ads.sift.co.uk,ads.silverdisc.co.uk,ads.skins.be,ads.slim.com,ads.smartclick.com,ads.smartshopping.co.uk,ads.soft32.com,ads.space.com,ads.spoonfeduk.com,"
"ads.sprotiv.org,ads.sptimes.com,ads.stackoverflow.com,ads.stationplay.com,ads.stratics.com,ads.struq.com,ads.sun.com,ads.supplyframe.com,ads.switchboard.com,ads.t-online.de,ads.tahono.com,ads.techtv.com,ads.techweb.com,ads.telegraph.co.uk,ads.theglobeandmail.com,ads.themovienation.com,ads.thestar.com,ads.thewebfreaks.com,ads.timeout.com,ads.tjwi.info,ads.tmcs.net,ads.top500.org,ads.totallyfreestuff.com,ads.townhall.com,ads.trinitymirror.co.uk,ads.tripod.com,ads.tripod.lycos.co.uk,ads.tripod.lycos.de,ads.tripod.lycos.es,ads.tripod.lycos.it,ads.tripod.lycos.nl,ads.tripod.spray.se,ads.tso.dennisnet.co.uk,ads.tweetmeme.com,ads.uknetguide.co.uk,ads.ultimate-guitar.com,ads.uncrate.com,ads.undertone.com,ads.unixathome.org,ads.uploading.com,ads.usatoday.com,ads.v3.com,ads.verticalresponse.com,ads.vgchartz.com,ads.videosz.com,ads.viewx.co.uk,ads.virtual-nights.com,ads.virtualcountries.com,ads.vnumedia.com,ads.weather.ca,ads.web.aol.com,ads.web.cs.com,ads.web.de,ads.webmasterpoint.org,ads.websiteservices.com,"
"ads.whi.co.nz,ads.whoishostingthis.com,ads.wiezoekje.nl,ads.wikia.nocookie.net,ads.wineenthusiast.com,ads.wunderground.com,ads.wwe.biz,ads.xhamster.com,ads.xtra.co.nz,ads.y-0.net,ads.yimg.com,ads.yldmgrimg.net,ads.yourfreedvds.com,ads.youtube.com,ads.zdnet.com,ads.ztod.com,ads03.redtube.com,ads1.canoe.ca,ads1.mediacapital.pt,ads1.msn.com,ads1.rne.com,ads1.theglobeandmail.com,ads1.virtual-nights.com,ads10.speedbit.com,ads180.com,ads2.brazzers.com,ads2.clearchannel.com,ads2.collegclub.com,ads2.collegeclub.com,ads2.exhedra.com,ads2.gamecity.net,ads2.jubii.dk,ads2.net-communities.co.uk,ads2.oneplace.com,ads2.rne.com,ads2.virtual-nights.com,ads2.xnet.cz,ads2004.treiberupdate.de,ads3.gamecity.net,ads3.virtual-nights.com,ads4.clearchannel.com,ads4.gamecity.net,ads4.virtual-nights.com,ads4homes.com,ads5.canoe.ca,ads5.virtual-nights.com,ads6.gamecity.net,ads7.gamecity.net,ads8.com,adsatt.abc.starwave.com,Adsatt.ABCNews.starwave.com,adsatt.espn.go.com,adsatt.espn.starwave.com,Adsatt.go.starwave.com,adscale.de,"
"adscholar.com,adscpm.com,adsdaq.com,adsdk.com,adsend.de,adserv.evo-x.de,adserv.gamezone.de,,adserv.iafrica.com,adserv.qconline.com,adserv.quality-channel.de,adserver-live.yoc.mobi,adserver.3digit.de,adserver.43plc.com,adserver.71i.de,adserver.adultfriendfinder.com,adserver.aidameter.com,adserver.aol.fr,adserver.barrapunto.com,adserver.beggarspromo.com,adserver.betandwin.de,adserver.bing.com,adserver.bizhat.com,adserver.bluewin.ch,adserver.break-even.it,adserver.cams.com,adserver.clashmusic.com,adserver.com,adserver.designertoday.com,adserver.digitoday.com,adserver.dotcommedia.de,adserver.finditquick.com,adserver.flossiemediagroup.com,adserver.freecity.de,adserver.freenet.de,adserver.friendfinder.com,adserver.gb2.motorpresse.de,adserver.gb3.motorpresse.de,adserver.hardsextube.com,adserver.hardwareanalysis.com,adserver.html.it,adserver.ig.com.br,adserver.irishwebmasterforum.com,adserver.itsfogo.com,adserver.janes.com,adserver.kyoceramita-europe.com,adserver.libero.it,adserver.motorpresse.de,"
"adserver.n9nedegrees.com,adserver.news.com.au,adserver.ngz-network.de,adserver.nydailynews.com,adserver.o2.pl,adserver.oddschecker.com,adserver.pl,adserver.plhb.com,adserver.portalofevil.com,adserver.portugalmail.net,adserver.portugalmail.pt,adserver.quizdingo.com,adserver.realhomesex.net,adserver.sanomawsoy.fi,adserver.sciflicks.com,adserver.sharewareonline.com,adserver.snowball.com,adserver.spankaway.com,adserver.startnow.com,adserver.theonering.net,adserver.twitpic.com,adserver.viagogo.com,adserver.virginmedia.com,adserver.yahoo.com,adserver01.de,adserver1-images.backbeatmedia.com,adserver1.backbeatmedia.com,adserver1.mindshare.de,adserver1.mokono.com,adserver1.ogilvy-interactive.de,adserver1.shareconnector.com,adserver1.w00tmedia.net,adserver2.mindshare.de,adserver2.popdata.de,adserverplus.com,adserversolutions.com,adserveuk.com,adserving.eleven-agency.com,adservinginternational.com,adsfac.eu,adsfac.net,adsfac.us,adshost1.com,adshuffle.com,adside.com,adskape.ru,adsklick.de,adsmarket.com,"
"adsmart.co.uk,adsmart.com,adsmart.net,adsmusic.com,adsoftware.com,adsoldier.com,adsonar.com,adspace.ro,adspeed.net,,adspirit.de,adsponse.de,adsremote.scrippsnetworks.com,adsrevenue.net,adsrv.deviantart.com,adsrv.iol.co.za,adsstat.com,adstat.4u.pl,adstest.weather.com,adsymptotic.com,adsynergy.com,adsys.townnews.com,adsystem.simplemachines.org,adsystem.tt-forums.net,adtech.de,adtechus.com,adtegrity.net,adthis.com,adtiger.de,adtoll.com,adtology.com,adtology3.com,adtoma.com,adtrade.net,adtrading.de,adtrak.net,adtriplex.com,adult-adv.com,adultadvertising.com,adv-adserver.com,adv-banner.libero.it,adv.cooperhosting.net,adv.freeonline.it,adv.hwupgrade.it,adv.livedoor.com,adv.webmd.com,adv.wp.pl,adv.yo.cz,adv08.edintorni.net,advariant.com,adventory.com,advert.bayarea.com,advert.dyna.ultraweb.hu,advertarium.com.ua,adverticum.com,adverticum.net,adverticus.de,advertise.com,advertiseireland.com,advertisespace.com,advertising.com,advertising.guildlaunch.net,advertisingbanners.com,advertisingbox.com,"
"advertmarket.com,advertmedia.de,advertpro.sitepoint.com,advertpro.ya.com,adverts.carltononline.com,adverts.digitalspy.co.uk,advertserve.com,advertstream.com,advertwizard.com,advideo.uimserv.net,adview.ppro.de,advisormedia.cz,adviva.com,adviva.net,advnt.com,advprotraffic.com,adwareremovergold.com,adwhirl.com,adwitserver.com,adworldnetwork.com,adworx.at,adworx.be,adworx.nl,adx.allstar.cz,adx.arip.co.th,adx.atnext.com,adx.gainesvillesun.com,adxpansion.com,adxpose.com,adxvalue.com,adyea.com,adzerk.net,adzerk.s3.amazonaws.com,adzones.com,af-ad.co.uk,afbtracking09.com,affbuzzads.com,affili.net,affiliate.1800flowers.com,affiliate.7host.com,affiliate.doubleyourdating.com,affiliate.dtiserv.com,affiliate.gamestop.com,affiliate.kitapyurdu.com,affiliate.mercola.com,affiliate.mogs.com,affiliate.offgamers.com,affiliate.travelnow.com,affiliate.viator.com,affiliatefuel.com,affiliatefuture.com,affiliates.allposters.com,affiliates.babylon.com,affiliates.devilfishpartners.com,affiliates.digitalriver.com,"
"affiliates.globat.com,affiliates.ige.com,affiliates.internationaljock.com,affiliates.jlist.com,affiliates.quintura.com,affiliates.streamray.com,,affiliates.thinkhost.net,affiliates.thrixxx.com,affiliates.ultrahosting.com,affiliatetracking.com,affiliatetracking.net,affiliatewindow.com,affiliation-france.com,ah-ha.com,aidu-ads.de,aim4media.com,aistat.net,ajouter.net,alclick.com,alenty.com,alexa-sitestats.s3.amazonaws.com,all4spy.com,alladvantage.com,allosponsor.com,amazingcounters.com,amung.us,an.tacoda.net,anahtars.com,analytics.live.com,analytics.yahoo.com,anm.intelli-direct.com,annonser.dagbladet.no,apdsrv.com,apex-ad.com,api.intensifier.de,apture.com,arc1.msn.com,arcadebanners.com,ard.xxxblackbook.com,as.webmd.com,as1.advfn.com,as2.advfn.com,as5000.com,assets.loomia.com,assets1.exgfnetwork.com,assoc-amazon.com,at-adserver.alltop.com,atdmt.com,athena-ads.wikia.com,atopsites.com,atwola.com,auctionads.com,auctionads.net,audience2media.com,audit.median.hu,audit.webinform.hu,auto-bannertausch.de,"
"autohits.dk,avenuea.com,avpa.javalobby.org,avres.net,avsads.com,awempire.com,awin1.com,aylarl.com,azfront.com,b-1st.com,ba.afl.rakuten.co.jp,babs.tv2.dk,backbeatmedia.com,banik.redigy.cz,banner-exchange-24.de,banner.ad.nu,banner.alphacool.de,banner.ambercoastcasino.com,banner.blogranking.net,banner.buempliz-online.ch,banner.casino.net,banner.casinodelrio.com,banner.cotedazurpalace.com,banner.coza.com,banner.cz,banner.easyspace.com,banner.elisa.net,banner.eurogrand.com,banner.featuredusers.com,banner.getgo.de,banner.goldenpalace.com,banner.img.co.za,banner.inyourpocket.com,banner.jobsahead.com,banner.joylandcasino.com,banner.kiev.ua,banner.linux.se,banner.media-system.de,banner.mindshare.de,banner.nixnet.czbanner.noblepoker.com,banner.northsky.com,banner.orb.net,banner.penguin.cz,banner.prestigecasino.com,banner.rbc.ru,banner.relcom.ru,banner.supereva.it,banner.tanto.de,banner.titan-dsl.de,banner.vadian.net,banner.webmersion.com,banner.wirenode.com,bannerads.de,bannerads.zwire.com,bannerbank.ru,"
"mbannerboxes.com,bannercommunity.de,bannerconnect.com,bannerconnect.net,bannerexchange.cjb.net,bannergrabber.internet.gr,bannerhost.com,bannerimage.com,,bannerlandia.com.ar,bannermall.com,bannermarkt.nl,bannerpower.com,banners.adultfriendfinder.com,banners.amigos.com,banners.apnuk.com,banners.asiafriendfinder.com,banners.audioholics.com,banners.babylon-x.com,banners.bol.com.br,banners.cams.com,banners.clubseventeen.com,banners.czi.cz,banners.dine.com,banners.direction-x.com,banners.directnic.com,banners.easydns.com,banners.ebay.com,banners.expressindia.com,banners.freett.com,banners.friendfinder.com,banners.getiton.com,banners.iq.pl,banners.isoftmarketing.com,banners.lifeserv.com,banners.linkbuddies.com,banners.passion.com,banners.resultonline.com,banners.sexsearch.com,banners.sugeridos.com,banners.sys-con.com,banners.thomsonlocal.com,banners.videosz.com,banners.virtuagirlhd.com,banners.wunderground.com,bannerserver.com,bannersgomlm.com,bannershotlink.perfectgonzo.com,bannersng.yell.com,"
"bannerspace.com,bannerswap.com,bannertesting.com,bannery.cz,bannieres.acces-contenu.com,bans.adserver.co.il,bans.bride.ru,barnesandnoble.bfast.com,baypops.com,bbelements.com,bbn.img.com.ua,begun.ru,belstat.com,belstat.nl,berp.com,best-pr.info,best-top.ro,bestsearch.net,bidbuddy.com,bidclix.com,bidclix.net,bidvertiser.com,bigads.guj.de,bigbangmedia.com,bigclicks.com,billboard.cz,bitads.net,bizad.nikkeibp.co.jp,bizban.net,bizrate.com,blast4traffic.com,blingbucks.com,blogads.com,blogcounter.de,blogherads.com,blogrush.com,blogtoplist.se,blogtopsites.com,blueadvertise.com,bluehornet.com,bluekai.com,bluelithium.com,bluewhaleweb.com,bm.annonce.cz,bn.bfast.com,boersego-ads.de,boldchat.com,boom.ro,boost-my-pr.de,box.anchorfree.net,bpath.com,braincash.com,brandreachsys.com,bravenet.com,bridgetrack.com,british-banners.com,bs.yandex.ru,budsinc.com,bullseye.backbeatmedia.com,buyhitscheap.com,buysellads.com,bvalphaserver.com,bwp.download.com,c.bigmir.net,c.compete.com,c1.nowlinux.com,campaign.bharatmatrimony.com,"
"canadatop.com,caniamedia.com,carbonads.com,carbonads.net,casalemedia.com,casalmedia.com,cash4banner.de,cash4members.com,cash4popup.de,cashcrate.com,cashengines.com,cashfiesta.com,cashlayer.com,cashpartner.com,casinogames.com,casinopays.com,casinorewards.com,casinotraffic.com,casinotreasure.com,cbanners.virtuagirlhd.com,cben1.net,cbmall.com,cbx.net,cecash.com,ceskydomov.alias.ngs.modry.cz,cetrk.com,cgicounter.puretec.de,ch.questionmarket.com,channelintelligence.com,chart.dk,chartbeat.com,chartbeat.net,checkm8.com,checkstat.nl,chestionar.ro,chitika.net,ciaoclick.com,cibleclick.com,cityads.telus.net,cj.com,cjbmanagement.com,cjlog.com,claria.com,class-act-clicks.com,click-fr.com,click.absoluteagency.com,click.fool.com,click.kmindex.ru,click2freemoney.com,click2paid.com,clickability.com,clickadz.com,clickagents.com,clickbank.com,clickbank.net,clickbooth.com,clickboothlnk.com,clickbrokers.com,clickchatsold.com,clickcompare.co.uk,clickdensity.com,clickedyclick.com,clickforwebmasters.com,"
"clickhereforcellphones.com,clickhouse.com,clickhype.com,clicklink.jp,clickmedia.ro,clickosmedia.com,clicks.equantum.com,clicks.mods.de,clicks.stripsaver.com,clickserve.cc-dt.com,clicksor.com,clicktag.de,clickthrucash.com,clickthruserver.com,clickthrutraffic.com,clicktorrent.info,clicktrace.info,clicktrack.ziyu.net,clicktracks.com,clicktrade.com,clickxchange.com,clickz.com,clicmanager.fr,clictrafic.com,clients.tbo.com,clixgalore.com,clkads.com,cluster.adultworld.com,clustrmaps.com,cmpstar.com,cnomy.com,cnt.one.ru,cnt.spbland.ru,cnt1.pocitadlo.cz,code-server.biz,colonize.com,comclick.com,commindo-media-ressourcen.de,commissionmonster.com,compactbanner.com,comprabanner.it,connextra.com,contaxe.de,content.ad,contextweb.com,conversionruler.com,cookies.cmpnet.com,coremetrics.com,count.rbc.ru,count.rin.ru,count.west263.com,count6.rbc.ru,counted.com,counter.avtoindex.com,counter.bloke.com,counter.cgiworld.net,counter.cnw.cz,counter.cz,counter.dreamhost.com,counter.fateback.com,counter.mirohost.net,"
"counter.mojgorod.ru,counter.nowlinux.com,counter.rambler.ru,counter.search.bg,counter.sparklit.com,counter.times.lv,counter.yadro.ru,counters.honesty.com,counting.kmindex.ru,counts.tucows.com,coupling-media.de,cpalead.com,cpays.com,cpmstar.com,cpxadroit.com,cpxinteractive.com,cqcounter.com,crakmedia.com,craktraffic.com,crawlability.com,crazypopups.com,creafi-online-media.com,creative.ak.facebook.com,creative.whi.co.nz,creatives.as4x.tmcs.net,creatives.livejasmin.com,crispads.com,criteo.com,crowdgravity.com,crtv.mate1.com,crwdcntrl.net,ctnetwork.hu,cubics.com,customad.cnn.com,cyberbounty.com,cybermonitor.com,d1.openx.org,danban.com,dapper.net,datashreddergold.com,dbbsrv.com,dc-storm.com,de17a.com,dealdotcom.com,debtbusterloans.com,decknetwork.net,default-homepage-network.com,deloo.de,demandbase.com,depilflash.tv,di1.shopping.com,dialerporn.com,didtheyreadit.com,digits.com,direct-xxx-access.com,directaclick.com,directivepub.com,directleads.com,directorym.com,directtrack.com,discountclick.com,"
"displayadsmedia.com,displaypagerank.com,dist.belnk.com,dmtracker.com,dmtracking.alibaba.com,dmtracking2.alibaba.com,dnads.directnic.com,doclix.com,domaining.in,domainsponsor.com,domainsteam.de,donorschoose.org,doubleclick.com,doubleclick.de,doubleclick.net,drumcash.com,dynamic.fmpub.net,e-adimages.scrippsnetworks.com,e-bannerx.com,e-debtconsolidation.com,e-m.fr,e-planning.net,e.kde.cz,eadexchange.com,easyad.info,easyhits4u.com,ebuzzing.com,ecircle-ag.com,ecoupons.com,edgeio.com,effectivemeasure.net,eiv.baidu.com,elitedollars.com,elitetoplist.com,emarketer.com,emediate.dk,emediate.eu,emonitor.takeit.cz,engine.awaps.net,engine.espace.netavenir.com,enginenetwork.com,enoratraffic.com,enquisite.com,entercasino.com,entrecard.s3.amazonaws.com,epiccash.com,ero-advertising.com,esellerate.net,estat.com,etargetnet.com,ethicalads.net,etracker.de,eu-adcenter.net,eu1.madsone.com,eur.a1.yimg.com,eurekster.com,euro-linkindex.de,euroclick.com,european-toplist.de,europeanbanners.com,europebanner.net,euroranking.de,"
"euros4click.de,eusta.de,evergage.com,evidencecleanergold.com,ewebcounter.com,exchange-it.com,exchange.bg,exchangead.com,exchangeclicksonline.com,exit76.com,exitexchange.com,exitfuel.com,exoclick.com,exogripper.com,experteerads.com,exponential.com,express-submit.de,extads.web.de,extractorandburner.com,extreme-dm.com,eyeblaster.com,eyewonder.com,ezula.com,f5biz.com,fast-adv.it,fastclick.com,fastclick.com.edgesuite.net,fc.webmasterpro.de,feedbackresearch.com,feedjit.com,ffxcam.fairfax.com.au,filitrac.com,fimc.net,fimserve.com,findcommerce.com,findyourcasino.com,fineclicks.com,first.nova.cz,firstlightera.com,flashtalking.com,fleshlightcash.com,flexbanner.com,flowgo.com,flurry.com,fonecta.leiki.com,foo.cosmocode.de,forex-affiliate.net,forrestersurveys.com,fpctraffic.com,fpctraffic2.com,fragmentserv.iac-online.de,free-banners.com,freebanner.com,freelogs.com,freeonlineusers.com,freepay.com,freestat.pl,freestats.com,freestats.tv,freewebcounter.com,funklicks.com,funpageexchange.com,fusionads.net,fusionquest.com,"
"fxclix.com,fxstyle.net,galaxien.com,game-advertising-online.com,gamehouse.com,gamesites100.net,gamesites200.com,gamesitestop100.com,gator.com,gbanners.hornymatches.com,gemius.pl,geo.digitalpoint.com,geoadserving.coffeetree.info,geobanner.adultfriendfinder.com,geovisite.com,german-linkindex.de,getclicky.com,getfreecounter.com,globalismedia.com,globaltakeoff.net,globaltrack.com,globe7.com,gmads.net,go-clicks.de,go-rank.de,goingplatinum.com,gold.weborama.fr,goldstats.com,google-analytics.com,googleadservices.com,googlesyndication.com,gostats.com,gp.dejanews.com,gpr.hu,grafstat.ro,grapeshot.co.uk,gtop.ro,gtop100.com,gtsads.com,harrenmedia.com,harrenmedianetwork.com,havamedia.net,heias.com,hentaicounter.com,herbalaffiliateprogram.com,hexusads.fluent.ltd.uk,heyos.com,hidden.gogoceleb.com,hightrafficads.com,histats.com,hit-parade.com,hit-ranking.de,hit.bg,hit.ua,hit.webcentre.lycos.co.uk,hitbox.com,hitcents.com,hitexchange.net,hitfarm.com,hitiz.com,hitlist.ru,hitlounge.com,hitometer.com,hits.europuls.eu,"
"hits.informer.com,hits.puls.lv,hits4me.com,hits4pay.com,hitslink.com,hittail.com,hollandbusinessadvertising.nl,homepageking.de,hostedads.realitykings.com,hotkeys.com,hotlog.ru,hotrank.com.tw,httpool.com,hurricanedigitalmedia.com,hydramedia.com,hyperbanner.net,hypertracker.com,i-clicks.net,i.xx.openx.com,i1img.com,i1media.no,ia.iinfo.cz,iad.anm.co.uk,iadnet.com,iconadserver.com,icptrack.com,idcounter.com,identads.com,idot.cz,idregie.com,ientrymail.com,iesnare.com,ifa.tube8live.com,ilbanner.com,ilead.itrack.it,iliillliO00OO0.321.cn,ilovecheating.com,image.ugo.com,imageads.canoe.ca,imagecash.net,images-pw.secureserver.net,images.v3.com,imarketservices.com,img.prohardver.hu,imgpromo.easyrencontre.com,imonitor.nethost.cz,impact.as,imprese.cz,impressionmedia.cz,impressionz.co.uk,imrworldwide.com,inboxdollars.com,inc.com,incentaclick.com,indexstats.com,indieclick.com,industrybrains.com,inetlog.ru,infinite-ads.com,infinityads.com,infolinks.com,information.com,infra-ad.com,initgroup.com,inringtone.com,"
"insightexpress.com,insightexpressai.com,inspectorclick.com,instantmadness.com,intelliads.com,intellitxt.com,interactive.forthnet.gr,interclick.com,intergi.com,internetfuel.com,interreklame.de,interstat.hu,ip.ro,ip193.cn,iperceptions.com,ipro.com,ireklama.cz,itbannerexchange.com,itfarm.com,itop.cz,its-that-easy.com,itsptp.com,jbeet.cjt1.net,jcount.com,jedonkey.cjt1.net,jinkads.de,jkazaa.cjt1.net,jnova.cjt1.net,joetec.net,jokedollars.com,js.users.51.la,juicyads.com,jumptap.com,justrelevant.com,justwebads.com,k.iinfo.cz,kanoodle.com,keymedia.hu,kissmetrics.com,kliks.nl,kompasads.com,kontera.com,kt-g.de,ktu.sv2.biz,lakequincy.com,layer-ad.de,layer-ads.de,lbn.ru,lct.salesforce.com,leadboltads.net,leadclick.com,leadingedgecash.com,levelrate.de,lfstmedia.com,liftdna.com,ligatus.com,ligatus.de,lightningcast.net,lightspeedcash.com,link-booster.de,link4ads.com,linkadd.de,linkbucks.net,linkbuddies.com,linkexchange.com,linkexchange.ru,linkprice.com,linkrain.com,linkreferral.com,links-ranking.de,"
"linkshighway.com,linkshighway.net,linkstorms.com,linkswaper.com,linktarget.com,liquidad.narrowcastmedia.com,liveintent.com,liveinternet.ru,liverail.com,loading321.com,log.btopenworld.com,logua.com,lop.com,lucidmedia.com,lv.boost-my-pr.com,lynxtrack.com,lzjl.com,m.webtrends.com,m1.webstats4u.com,m4n.nl,madclient.uimserv.net,madisonavenue.com,mads.cnet.com,mads.gamespot.com,madvertise.de,mailusnow123.com,malware-scan.com,marchex.com,marketing.888.com,marketing.nyi.net,marketing.osijek031.com,marketingsolutions.yahoo.com,maroonspider.com,mas.sector.sk,mastermind.com,matchcraft.com,max.i12.de,maximumcash.com,mbn.com.ua,mbs.megaroticlive.com,mbuyu.nl,mdotm.com,measuremap.com,media-adrunner.mycomputer.com,media-servers.net,media.ftv-publicite.fr,media.funpic.de,media1first.com,media6degrees.com,mediaarea.eu,mediacharger.com,mediadvertising.ro,mediageneral.com,mediamayhemcorp.com,mediamgr.ugo.com,mediaplazza.com,mediaplex.com,mediascale.de,mediatext.com,mediax.angloinfo.com,mediaz.angloinfo.com,medleyads.com,"
"megacash.de,megago.com,megastats.com,megawerbung.de,memorix.sdv.fr,metaffiliation.com,metanetwork.com,methodcash.com,metrics.windowsitpro.com,mgid.com,miarroba.com,microstatic.pl,microticker.com,midnightclicking.com,misstrends.com,miva.com,mixpanel.com,mixtraffic.com,mlm.de,mmismm.com,moatads.com,mobclix.com,mocean.mobi,modemspeedbooster.com,moneyexpert.com,monsterpops.com,mopub.com,mr-rank.de,mrskincash.com,mtree.com,muk.247realmedia.com,musiccounter.ru,muwmedia.com,myaffiliateprogram.com,mybloglog.com,mycounter.ua,mypagerank.net,mypagerank.ru,mypowermall.com,myrating.ljseek.com,mystat-in.net,mystat.pl,mytop-in.net,n69.com,naiadsystems.com,naj.sk,namimedia.com,nastydollars.com,navigator.io,navrcholu.cz,nbjmp.com,ndparking.com,nedstat.com,nedstat.nl,nedstatbasic.net,nedstatpro.net,neocounter.neoworx-blog-tools.net,neocounter.net,neoffic.com,net-filter.com,netaffiliation.com,netagent.cz,netclickstats.com,netcommunities.com,netdirect.nl,netflame.cc,netincap.com,netpool.netbookia.net,netshelter.net,"
"network.business.com,network.realmedia.com,neudesicmediagroup.com,newads.bangbros.com,newbie.com,newnet.qsrch.com,newnudecash.com,newopenx.detik.com,newt1.adultadworld.com,newt1.adultworld.com,newtopsites.com,ng3.ads.warnerbros.com,ngadcenter.net,ngs.impress.co.jp,nitroclicks.com,novem.pl,numax.nu-1.com,nuseek.com,oas-central.realmedia.com,oas.benchmark.fr,oas.foxnews.com,oas.repubblica.it,oas.roanoke.com,oas.salon.com,oas.toronto.com,oas.uniontrib.com,oas.villagevoice.com,oas.wwwheise.de,oascentral.businessweek.com,oascentral.chicagobusiness.com,oascentral.fortunecity.com,oascentral.ggl.com,oascentral.register.com,oewa.at,oewabox.at,offerforge.com,offermatica.com,oinadserver.com,olivebrandresponse.com,omniture.com,onclickads.com,onclickads.net,oneandonlynetwork.com,onenetworkdirect.com,onestat.com,onestatfree.com,onewaylinkexchange.net,online-metrix.net,onlinecash.com,onlinecashmethod.com,onlinerewardcenter.com,openad.tf1.fr,openad.travelnow.com,openads.friendfinder.com,openads.org,openclick.com,"
"openx.angelsgroup.org.uk,openx.blindferret.com,opienetwork.com,optimost.com,optmd.com,ota.cartrawler.com,otto-images.developershed.com,overture.com,owebmoney.ru,oxado.com,oxcash.com,oxen.hillcountrytexas.com,pagead.l.google.com,pagerank-estate-spb.ru,pagerank-ranking.com,pagerank-ranking.de,pagerank-server7.de,pagerank-submitter.com,pagerank-submitter.de,pagerank-suchmaschine.de,pagerank-united.de,pagerank4u.eu,pagerank4you.com,pageranktop.com,partage-facile.com,partner-ads.com,partner.gonamic.de,partner.pelikan.cz,partner.topcities.com,partnerad.l.google.com,partnercash.de,partners.priceline.com,passion-4.net,passport.baidu.com,pay-ads.com,pay-to-promote.com,paycounter.com,paydayasap.com,paypopup.com,payserve.com,pbnet.ru,pcash.imlive.com,pcekspert.com,peep-auktion.de,peer39.com,pennyweb.com,pepperjamnetwork.com,percentmobile.com,perf.weborama.fr,perfiliate.com,performancing.com,performancingads.com,pgmediaserve.com,pgpartner.com,pheedo.com,phoenix-adrunner.mycomputer.com,phpadsnew.new.natuurpark.nl,"
"phpmyvisites.net,picadmedia.com,pillscash.com,pimproll.com,planetactive.com,play4traffic.com,playhaven.com,plugrush.com,pointroll.com,pokerstrategy.com,pop-under.ru,pops.freeze.com,popub.com,popunder.ru,popup.msn.com,popupmoney.com,popupnation.com,popups.infostart.com,popupsponsor.com,popuptraffic.com,porntrack.com,postmasterbannernet.com,postrelease.com,potenza.cz,pr-star.de,pr-ten.de,pr5dir.com,praddpro.de,prchecker.info,precisioncounter.com,predictad.com,premium-offers.com,primaryads.com,primetime.net,privatecash.com,pro-advertising.com,pro.i-doctor.co.kr,proext.com,profero.com,professorbanner.com,projectwonderful.com,promo.badoink.com,promo.ulust.com,promo1.webcams.nl,promobenef.com,promos.fling.com,promote.pair.com,promotion-campaigns.com,promozia.de,pronetadvertising.com,propellerads.com,proranktracker.com,proton-tm.com,protraffic.com,provexia.com,prsitecheck.com,psstt.com,pub.chez.com,pub.club-internet.fr,pub.hardware.fr,pub.opt.pf,pubdirecte.com,publicidad.elmundo.es,pubmatic.com,"
"pubs.lemonde.fr,pulse360.com,q.azcentral.com,qctop.com,qnsr.com,quantcast.com,quantserve.com,quarterserver.de,questaffiliates.net,quigo.com,quinst.com,quisma.com,rad.msn.com,radarurl.com,radiate.com,rampidads.com,rank-master.com,rank-master.de,rank8.de,rankchamp.de,ranking-charts.de,ranking-hits.de,ranking-id.de,ranking-links.de,ranking-liste.de,ranking-street.de,rankingchart.de,rankingscout.com,rankingscout.org,rankyou.com,rapidcounter.com,rate.ru,ratings.lycos.com,rb1.design.ru,reachjunction.com,readserver.net,realads.realmedia.com,realcastmedia.com,realclix.com,realmedia-a800.d4p.net,realtechnetwork.com,realteencash.com,realtracker.com,reduxmedia.com,reduxmediagroup.com,reedbusiness.com,reefaquarium.biz,referralware.com,regnow.com,reinvigorate.net,reklam.rfsl.se,reklama.mironet.cz,reklama.reflektor.cz,reklamcsere.hu,reklame.unwired-i.net,reklamer.com.ua,relevanz10.de,relmaxtop.com,remotead.cnet.com,republika.onet.pl,retaildirect.realmedia.com,retargeter.com,revenue.net,revenuedirect.com,"
"revenuepilot.com,revsci.net,revstats.com,rewardster.com,richmails.com,richmedia.yimg.com,richwebmaster.com,rightstats.com,rl.auto.ru,rlcdn.com,rle.ru,rmads.msn.com,rmedia.boston.com,roar.com,robotreplay.com,roia.biz,rok.com.com,rose.ixbt.com,rotabanner.com,roxr.net,rpxnow.com,rtbpop.com,rts.doublepimp.com,ru-traffic.com,ru4.com,rubiconproject.com,s2d6.com,sageanalyst.net,sbx.pagesjaunes.fr,scambiobanner.aruba.it,scanscout.com,scopelight.com,scorecardresearch.com,scratch2cash.com,scripte-monster.de,searchfeast.com,searchmarketing.com,searchramp.com,secure.webconnect.net,sedoparking.com,sedotracker.com,seeq.com,sensismediasmart.com.au,seo4india.com,serv0.com,servedbyadbutler.com,servedbyopenx.com,servethis.com,service1.adten.de,services.hearstmags.com,serving-sys.com,sexaddpro.de,sexcounter.com,sexinyourcity.com,sexlist.com,sextracker.com,sexystat.com,sezwho.com,shareadspace.com,shareasale.com,sharepointads.com,sher.index.hu,shinystat.com,shinystat.it,shoppingads.com,siccash.com,sidebar.angelfire.com,"
"sinoa.com,sitebrand.geeks.com,sitemerkezi.net,sitemeter.com,sitestat.com,sixsigmatraffic.com,slickaffiliate.com,sm.aport.ru,sma.punto.net,smart4ads.com,smartadserver.com,smartbase.cdnservices.com,smowtion.com,snapads.com,snoobi.com,socialspark.com,softclick.com.br,spa.snap.com,spacash.com,sparkstudios.com,specificmedia.co.uk,specificpop.com,spezialreporte.de,spinbox.techtracker.com,spinbox.versiontracker.com,sponsorads.de,sponsorpro.de,sponsors.thoughtsmedia.com,spot.fitness.com,sprinks-clicks.about.com,spylog.com,spywarelabs.com,spywarenuker.com,spywords.com,srbijacafe.org,srwww1.com,starffa.com,start.freeze.com,stat.cliche.se,stat.dealtime.com,stat.dyna.ultraweb.hu,stat.pl,stat.su,stat.tudou.com,stat.webmedia.pl,stat.zenon.net,stat24.com,stat24.meta.ua,statcounter.com,static.fmpub.net,static.itrack.it,staticads.btopenworld.com,statistik-gallup.net,statm.the-adult-company.com,stats.blogger.com,stats.cts-bv.nl,stats.directnic.com,stats.hyperinzerce.cz,stats.mirrorfootball.co.uk,stats.olark.com,"
"stats.suite101.com,stats.surfaid.ihost.com,stats.townnews.com,stats.unwired-i.net,stats.wordpress.com,stats4all.com,statxpress.com,steelhouse.com,steelhousemedia.com,subscribe.hearstmags.com,sugoicounter.com,superclix.de,superstats.com,supertop.ru,supertop100.com,suptullog.com,surfmusik-adserver.de,swordfishdc.com,t.pusk.ru,tacoda.net,tailsweep.co.uk,tailsweep.com,tailsweep.se,takru.com,tangerinenet.biz,tapad.com,targad.de,targetingnow.com,targetnet.com,targetpoint.com,tatsumi-sys.jp,tcads.net,tdsearchware.com,techclicks.net,teenrevenue.com,teknosurf3.com,teliad.de,test.com,text-link-ads.com,textad.sexsearch.com,textads.biz,textads.opera.com,textlinks.com,tfag.de,theadhost.com,theanswerto.com,thebugs.ws,thecounter.com,thefreesitez.com,therapistla.com,therichkids.com,thesearchworks.com,thruport.com,tinybar.com,tizers.net,tlvmedia.com,tntclix.co.uk,top-casting-termine.de,top-site-list.com,top.list.ru,top.mail.ru,top.one.ru,top.proext.com,top100-images.rambler.ru,top100.mafia.ru,top123.ro,top20.com,"
"top20free.com,top66.ro,top90.ro,topbarh.box.sk,topblogarea.se,topbloglists.com,topbucks.com,topforall.com,topgamesites.net,toplist.cz,toplist.pornhost.com,toplista.mw.hu,toplistcity.com,topmango.com,topmmorpgsites.com,topping.com.ua,toprebates.com,topsafelist.net,topsajt.com,topsearcher.com,topsir.com,topsite.lv,topsites.com.br,topstats.com,totemcash.com,touchclarity.com,touchclarity.natwest.com,tour.brazzers.com,tpnads.com,track.adform.net,track.anchorfree.com,track.collegehumor.com,track.gawker.com,trackalyzer.com,tracker.icerocket.com,tracking.gajmp.com,tracking.koego.com,tracking.yourfilehost.com,tracking101.com,trackingsoft.com,trackmysales.com,tradedoubler.com,traffic-exchange.com,traffic.liveuniversenetwork.com,trafficadept.com,trafficcdn.liveuniversenetwork.com,trafficfactory.biz,trafficholder.com,trafficjunky.net,trafficleader.com,trafficmasterz.net,trafficmaxx.de,trafficsecrets.com,trafficspaces.net,trafficstrategies.com,trafficswarm.com,traffictrader.net,trafficz.com,trafficz.net,"
"traffiq.com,trafic.ro,traficdublu.ro,travis.bosscasinos.com,trekblue.com,trekdata.com,trendcounter.com,tribalfusion.com,trix.net,truehits.net,truehits1.gits.net.th,truehits2.gits.net.th,tsms-ad.tsms.com,tubedspots.com,tubemogul.com,turn.com,tvmtracker.com,twittad.com,tyroo.com,uarating.com,ukbanners.com,ultramercial.com,ultsearch.com,unanimis.co.uk,untd.com,updated.com,upspiral.com,urlcash.net,us.a1.yimg.com,usapromotravel.com,usmsad.tom.com,utarget.co.uk,utils.mediageneral.net,v1.cnzz.com,validclick.com,valuead.com,valueclick.com,valueclickmedia.com,valuecommerce.com,valuesponsor.com,veille-referencement.com,ventivmedia.com,vericlick.com,vertadnet.com,veruta.com,vervewireless.com,vg.ad.asap-asp.net,vibrantmedia.com,video-stats.video.google.com,videoegg.com,view4cash.de,viewpoint.com,visistat.com,visit.webhosting.yahoo.com,visitbox.de,visitorprofiler.com,visual-pagerank.fr,visualrevenue.com,voicefive.com,vpon.com,vrs.cz,vs.tucows.com,wads.webteh.com,warlog.info,warlog.ru,wdads.sx.atl.publicus.com,"
"web-stat.com,web.informer.com,web2.deja.com,webads.co.nz,webads.nl,webangel.ru,webcash.nl,webcounter.cz,webcounter.goweb.de,webgains.com,webmaster-partnerprogramme24.de,webmasterplan.com,webmasterplan.de,weborama.fr,webpower.com,webreseau.com,webseoanalytics.com,websitepagerank.net,websponsors.com,webstat.channel4.com,webstat.com,webstat.net,webstats4u.com,webtrackerplus.com,webtraffic.se,webtraxx.de,webtrendslive.com,wegcash.com,werbung.meteoxpress.com,wetrack.it,whaleads.com,whenu.com,whispa.com,whoisonline.net,wholesaletraffic.info,widespace.com,widgetbucks.com,wikia-ads.wikia.com,window.nixnet.cz,wintricksbanner.googlepages.com,wipub.com,witch-counter.de,wlmarketing.com,wmirk.ru,wondoads.de,woopra.com,worldwide-cash.net,wtlive.com,www-banner.chat.ru,www-google-analytics.l.google.com,www.banner-link.com.br,www.dnps.com,www.kaplanindex.com,www.money4exit.de,www.photo-ads.co.uk,www1.gto-media.com,www8.glam.com,x-traceur.com,x6.yakiuchi.com,xchange.ro,xclicks.net,xertive.com,xg4ken.com,xiti.com,"
"xplusone.com,xponsor.com,xq1.net,xrea.com,xtendmedia.com,xtremetop100.com,xxxcounter.com,xzoomy.com,y.ibsys.com,yab-adimages.s3.amazonaws.com,yabuka.com,yadro.ru,yesads.com,yesadvertising.com,yieldads.com,yieldlab.net,yieldmanager.com,yieldmanager.net,yoc.mobi,yoggrt.com,yourtracking.net,z.times.lv,z5x.net,zangocash.com,zanox-affiliate.de,zanox.com,zantracker.com,zde-affinity.edgecaching.net,zedo.com,zencudo.co.uk,zenzuu.com,zeus.developershed.com,zeusclicks.com,zintext.com,zmedia.com,ad.doubleclick.net,";



GString g_strCachedLog(LOGFILE_CACHED_WRITE);


GString g_strPreLog;
gthread_mutex_t g_csMtx3;
int g_csMtxInit3 = 0;
void InfoLog(int nMsgID, GString &strSrc)
{
	if(g_CoreLog)
	{
		if (!g_csMtxInit3)
		{
			g_csMtxInit3 = 1;
			gthread_mutex_init(&g_csMtx3, NULL);
		}
		gthread_mutex_lock(&g_csMtx3);
		try
		{
			// This allows an application to trap special messages for dynamic debug breaks of special message notification handlers.
			// It also allows visual distinction between two processes - both writing to the same log file....
			strSrc.Prepend(g_strPreLog);
			g_CoreLog(nMsgID,strSrc);

		}catch(...)
		{
			// because if g_CoreLog throws for any reason the whole server would deadlock to death over logging
		}
		gthread_mutex_unlock(&g_csMtx3);
	}

	if (g_NoLog)	// if logging is off because (disk full/ disk error/ bad log path)
		return;

	if (g_bDisableLog) // if logging is off because the user has disabled it
		return;

	// Note that we dont use any logging related CPU until we know logging is enabled
	strSrc.Prepend(g_strPreLog);


	//  note: g_strLogFile may not have a value at startup( == ""), but it may get a value shortly thereafter so we should cache the log if so configured
	time_t ltime;
	time(&ltime);
	char pzTime[128];
	struct tm *newtime;
	newtime = localtime(&ltime);
	strftime(pzTime, 128, "%H:%M:%S", newtime);
	
	GString strTime;
	strTime.Format("%s[%03d] %s",pzTime,(int)nMsgID,(const char *)strSrc);

	strTime.TrimRightWS();
	strTime += "\t\r\n\r\n";  // the '\t' is to make a record delimiter that will not occur in the data.  HTTP headers contain "\r\n\r\n" and may be logged.

	
	if (!g_strLogFile.IsEmpty())
	{
		if (!g_csMtxInit3)
		{
			g_csMtxInit3 = 1;
			gthread_mutex_init(&g_csMtx3, NULL);
		}
		
		gthread_mutex_lock(&g_csMtx3);
		try
		{
			if (g_LogCache)
			{
				g_strCachedLog << strTime;

				if ( g_strCachedLog._len > LOGFILE_CACHED_WRITE )
				{
					g_strCachedLog.ToFileAppend(g_strLogFile);
					g_strCachedLog.Empty();
				}
			}
			else
			{
				strTime.ToFileAppend(g_strLogFile);
			}

		}catch(...)
		{
			if (!g_LogStdOut)
				g_NoLog = 1; // quit trying to log due to disk error
		}
		gthread_mutex_unlock(&g_csMtx3);
	}
	else
	{
		if (!g_LogStdOut)
			g_NoLog = 1; // quit trying to log because no log file name was configured
	}
		
	if (g_LogStdOut)
	{
		printf(strSrc.Buf());
		printf("\n");
	}
}
void InfoLog(int nMsgID, const char *pDataToLog)
{
	if (g_NoLog)
		return;
	GString str(pDataToLog);
	InfoLog(nMsgID, str);
}
void InfoLogFlush()
{
	if ( g_LogCache && g_strCachedLog.Length() )
	{
		gthread_mutex_lock(&g_csMtx3);
		try
		{
			g_strCachedLog.ToFileAppend(g_strLogFile);
			g_strCachedLog.Empty();
		}
		catch(...)
		{
		}
		gthread_mutex_unlock(&g_csMtx3);
	}
}
void *loggingThread(void *)
{
	while(!g_ServerIsShuttingDown)
	{
		PortableSleep(3,0);

		InfoLogFlush();
	}
	return 0;
}


void DataTrace(const char *pzTag, const char *pData, int nBytes, const char *pzFileOverride = 0)
{
	GString strLabel;
	if (memcmp(pzTag,"raw",3) == 0)
	{
		strLabel.Format("\n\n\n\n\n%d Bytes %s:\n",(int)nBytes,pzTag);
	}
	else
	{
		strLabel.Format("\n\n%d Bytes %s:\n",(int)nBytes,pzTag);
	}


	// Format it
	GString strBinary(strLabel);
	strBinary.FormatBinary((unsigned char *)pData,(int)nBytes);


	// write it to disk
	if (pzFileOverride && pzFileOverride[0])
	{
		strBinary.ToFileAppend(pzFileOverride);
	}
	else
	{
		strBinary.ToFileAppend(GetProfile().GetString("Trace","DataTraceFile",false));
	}
}







////////////////////////////////////////////////////////////////////////////////////
//
// External Connection Cache - currently used for connections to network proxy
//
////////////////////////////////////////////////////////////////////////////////////
GHash g_hshConnections;
gthread_mutex_t g_csMtxConnCache;
struct CacheConnectEntry
{
	long time;
	int fd;
	GString strServer;
};
int HasCachedConnection(int *fd, const char *strServer)
{
	gthread_mutex_lock(&g_csMtxConnCache);
	try
	{

FIND_ENTRY:
	CacheConnectEntry *pCCE = (CacheConnectEntry *)g_hshConnections.RemoveKey(strServer);
	if (pCCE)
	{
		// hold longer depending on the protocol
		// connections to the network HTTP proxy will not be kept for long.
		if ( time(0) - pCCE->time > 15 ) 
		{
			// ********* EXPIRED *********
			PortableClose(pCCE->fd,"Core1");

			goto FIND_ENTRY;
		}
		else
		{
			// ********* CACHE HIT *********
			*fd = pCCE->fd;
			delete pCCE;
		}
	}
	else
	{
		*fd = -1;
	}
	}
	catch( GException &rErr )
	{
		InfoLog(1, rErr.GetDescription());
	}
	catch(...)
	{
	}
	gthread_mutex_unlock(&g_csMtxConnCache);

	return (*fd != -1);
}
void CacheConnection(int fd, const char *strServer)
{
//  to disable the cache:
//	closesocket(fd);
//	return;


	static long lastExpCheck = 0; // needs mutexed on RISC machines

	gthread_mutex_lock(&g_csMtxConnCache);
	try
	{

	CacheConnectEntry *pCCE = new CacheConnectEntry;
	long lNow = time(0);
	pCCE->fd = fd;
	pCCE->time = lNow;
	pCCE->strServer = strServer;
	g_hshConnections.Insert(strServer, (void *)pCCE);
	if (lNow - lastExpCheck > 15)
	{
		// ********* CLEANUP ROUTINE *********
		lastExpCheck = lNow;
		// check every entry in the hash for expired connections
		GStringList lstRemoveStack;
		GHashIterator it(&g_hshConnections);
		while (it())
		{
			CacheConnectEntry *p = (CacheConnectEntry *)it++;
			if (lNow - p->time > 15)
			{
				lstRemoveStack.AddLast(p->strServer);
			}
		}
		//	********* lstRemoveStack.Size() items EXPIRED *********
		if (!lstRemoveStack.isEmpty())
		{
			GStringIterator itL(&lstRemoveStack);
			while(itL())
			{
				CacheConnectEntry *pCCE = (CacheConnectEntry *)g_hshConnections.RemoveKey(itL++);
				PortableClose(pCCE->fd,"Core2");
				delete pCCE;
			}
		}
	}
	}
	catch( GException &rErr )
	{
		InfoLog(2, rErr.GetDescription());
	}
	catch(...)
	{
	}
	gthread_mutex_unlock(&g_csMtxConnCache);
}


// Init application instance random numbers
void QuickInitRandomNumbers()
{
	// capture the server start time, used for admin statistic info.
	time(&g_ServerStartTime);

	// seed the system random number generator
	srand( g_ServerStartTime );

	// increment each by time() seeded rand() to give them a starting value
	for(int nIndex=0; nIndex < 8; nIndex++)
		g_RandomNumbers[nIndex] += rand();

#ifdef ___XFER
	memcpy(&g_nXferTxnID,g_RandomNumbers,sizeof(g_nXferTxnID));
#endif //___XFER
}
// since the psudo random seed could be a security issue, this defeats it, this is random
void InitRandomNumbers()
{
	InfoLog(3, "Random Key Init:");

	// capture the server start time, used for admin statistic info.
	time(&g_ServerStartTime);

	// seed the system random number generator
	srand( g_ServerStartTime );



	// increment each by time() seeded rand() to give them a starting value
	for(int nIndex=0; nIndex < 8; nIndex++)
		g_RandomNumbers[nIndex] += rand();


	// each number is incremented for a 'random()' amount of time. 
	// some machines are faster than others, and current system load also affects the values.
	for(int n_RandomNumberWorkerIndex=0; n_RandomNumberWorkerIndex < 8; n_RandomNumberWorkerIndex++)
	{
		unsigned long nYieldTime = 1 + (rand() % 7);
		unsigned long lStartTime = time(0);

		// round and round she goes, where she stops nobody knows.
		while (time(0) - lStartTime < nYieldTime)
		{
			g_RandomNumbers[n_RandomNumberWorkerIndex]++;
			
			// context switching randomizes the numbers better and magnifies the 
			// effects of other applications currently using system resource.
			gsched_yield(); 
		}
	}

	// now re-seed the random generator(with 8 different CPU-Cycle based seed numbers), 
	// then increment each value by rand() for each seed.
	for(int nSeedCount=0; nSeedCount<8; nSeedCount++)
	{
		srand( g_RandomNumbers[nSeedCount] );
		for(int nIndex2=0; nIndex2 < 8; nIndex2++)
			g_RandomNumbers[nIndex2] += rand();	
	}

	// finally make sure none of the numbers are == 0, it breaks other algorithms
	// that depend on them as valid seed numbers
	for(int nIndex3=0; nIndex3<8; nIndex3++)
	{
		if (g_RandomNumbers[nIndex3] == 0)
		{
			srand( time(0) );
			g_RandomNumbers[nIndex3] = rand();
			if (g_RandomNumbers[nIndex3] == 0)		
				g_RandomNumbers[nIndex3] = 777; // any non-zero number.
		}
	}

#ifdef ___XFER
	memcpy(&g_nXferTxnID,g_RandomNumbers,sizeof(g_nXferTxnID));
#endif // ___XFER
}



// Allows paths like this "/usr/path\subdir" or "c:\my/folder/file.ext"
const char *ReSlash(const char *pzInParam)
{
	char *pzIn = (char *)pzInParam; 
	int nLen = (int)strlen(pzInParam);
	for(int i=0; i<nLen; i++ )
	{
		if (pzIn[i] == '\\' || pzIn[i] == '/')
		{
			pzIn[i] = g_chSlash;
		}
	}
	return pzIn;
}



const char *GenerateCertificationData(GString &strDest)
{
	// variable length is allowed, here we always use 64 bytes
	int i;
	for(i=0; i < 64; i++)
	{
		strDest << (char)((rand() % 90) + 32);  
	}

	strDest.Replace( '/', '.' ); // no slashes allowed in random data
	strDest.Replace( '\0', '.' ); // no nulls allowed in random data

	// upper-case the key parts
	char *key = strDest.Buf();
	int len = (int)strlen(key);
	for (i = 0; i < len; i++)
		key[i] = toupper(key[i]);


	return strDest;
}



// This is a governor for the number of connections from a single 'reported' IP
// over the course of a specified time.  This feature prevents a type of
// 'denial of service' attack where the vandal steals your CPU time
// with a flood of instant 'hang-up calls', until a legitimate customer
// can no longer get through.  Each entry is in a hand-made unbalanced 
// tree to optimize insertion speed and reduce memory overhead.  
// The tree grows forever, so it may be trimmed by the FrequencyTimeRelease
// entry under the [System] section of the config file specifying the interval
// in seconds that the tree should be released.
class IPLog
{
public:
	int  nTime[64];					// an 'first in - last out' stack of connection times
	IPLog *Left;					// in a tree.
	IPLog *Right;
	char pzIP[32];					// for this IP address
	IPLog(const char *pIP)
	{
		memset(nTime,0,sizeof(nTime));
		strcpy((char *)pzIP,pIP);
		nTime[0] = time(0);
		Left = 0;
		Right = 0;
	}
};
IPLog *g_LogRoot = 0;
int g_FrequencyTimeRelease = 0;

void TimedRelease( IPLog* Root )
{
	if(Root)
	{
		if( Root->Left )
			TimedRelease( Root->Left );

		if( Root->Right )
			TimedRelease( Root->Right );

		delete Root;
	}
}

// This had to be extremely fast because the "Global refuse list" is searched for every new connection
// this custom built linked list has no unnecessary support (not doubly linked) (uses native types)
class IPQueryEntry
{
public:
	IPQueryEntry *pNext;
	unsigned long nRefuseUntil; 
	int nRefusedCount;
	GString strReason;

	unsigned long nIPBegin;
	unsigned long nIPEnd;
	IPQueryEntry(unsigned long IP)
	{
		nRefusedCount = 0;
		nRefuseUntil = 0;
		nIPBegin = IP;
		nIPEnd = IP;
		pNext=0;
	}
	IPQueryEntry(unsigned long IP1, unsigned long IP2)
	{
		nRefusedCount = 0;
		nRefuseUntil = 0;
		nIPBegin = IP1;
		nIPEnd = IP2;
		pNext=0;
	}
};
IPQueryEntry *g_FirstRefusedIP = 0;
IPQueryEntry *g_LastRefusedIP = 0;

IPQueryEntry *g_FirstAllowedIP = 0;
IPQueryEntry *g_LastAllowedIP = 0;


// return a numerical value of a "N.N.N.N" char representation ip address.
unsigned long IpToValue(const char *pzIp) 
{
#if !defined(_WIN32) && !defined(_INTEL) 
	return inet_addr(pzIp);
#else
	return ntohl(inet_addr("127.0.0.1"));
#endif
}


// dynamically flag an ip address as 'refuse service to' for some duration of seconds
void AddRefuseService(const char *pzIP, int nRefuse, const char *pzReason)
{
	IPQueryEntry *pRip = 0;
	unsigned long nNow = time(0);
	int bMadeNew = 0;

	// walk the list of refused IP's looking for an empty(expired) slot
	IPQueryEntry *pCurrRefusedIP = g_FirstRefusedIP;
	while (pCurrRefusedIP)
	{
		if (pCurrRefusedIP->nRefuseUntil < nNow && pCurrRefusedIP->nRefuseUntil != 0)
		{
			// flag it as taken so we do not collide with another thread
			pCurrRefusedIP->nRefuseUntil = nNow + 10;
			pCurrRefusedIP->nIPBegin = 0;
			pCurrRefusedIP->nIPEnd = 0;
			pCurrRefusedIP->nRefusedCount = 0;

			pRip = pCurrRefusedIP;
			break;
		}
		pCurrRefusedIP = pCurrRefusedIP->pNext;
	}

	// if there was not an expired one laying around make a new one
	if (!pRip)
	{
		pRip = new IPQueryEntry(IpToValue(pzIP));
		bMadeNew = 1;
	}
	else
	{
		pRip->nIPBegin = IpToValue(pzIP);
		pRip->nIPEnd = pRip->nIPBegin;
	}

	// set the duration of time that we reject this guy
	pRip->nRefuseUntil = nRefuse + nNow;
	pRip->strReason = pzReason;
	pRip->nRefusedCount = 0;

	// and to the list if a new structure was created
	if (bMadeNew)
	{
		if (!g_FirstRefusedIP)
		{
			g_FirstRefusedIP = pRip; // first one in the list
		}
		else
		{
			g_LastRefusedIP->pNext = pRip;
		}
		g_LastRefusedIP = pRip;
	}
}


int AddTime( IPLog** Root, const char *pzIP )
{
	int nNow = time(0);
	int nConnectLimit = GetProfile().GetInt("System","FrequencyConnectLimit",false);
	if (nConnectLimit < 1)
		return 0;
	nConnectLimit = (nConnectLimit > 32) ? 32 : nConnectLimit;
	
	// cleanup and reset the counters if it's time.
	if (g_FrequencyTimeRelease && nNow - g_FrequencyTimeRelease > GetProfile().GetInt("System","FrequencyTimeRelease",false) )
	{
		TimedRelease( g_LogRoot );
		g_LogRoot = 0;
		g_FrequencyTimeRelease = nNow;
	}


	if( *Root == 0 )    
	{

		*Root = new IPLog(pzIP);
		
		// if the global root (g_LogRoot) was just assigned - start the cleanup clock
		if (g_FrequencyTimeRelease == 0)
			g_FrequencyTimeRelease = nNow;

		return 0;
	}

	int nTreePlacementTest = strcmp((*Root)->pzIP,pzIP);
	
	
	if( nTreePlacementTest == 0 ) // if this IP has been here recently
	{
		
		// if less than N seconds have passed since this guy filled his time array.
		int N = GetProfile().GetInt("System","FrequencyTimeLimit",false);

		if (nNow - (*Root)->nTime[0] < N &&  (*Root)->nTime[nConnectLimit - 1] > 0)
		{

			//	caught a resource hog
			GString strErr;
			strErr << pzIP << ": violated connect limit of " << nConnectLimit << " over " << N << " seconds.";
			InfoLog(4,strErr);


			// if we are configured to 'shut this guy off' for a little while
			// set the time that we no longer accept connections from this 
			// resource hog - otherwise just disconnect this attempt and allow him 
			// to retry immediately.
			int nRefuseSeconds = GetProfile().GetInt("System","FrequencyViolatorRefuse",false);
			if (nRefuseSeconds)
			{
				AddRefuseService(pzIP,nRefuseSeconds,strErr);
			}

			return 1; // connections are coming too fast.
		}
		
		if ((*Root)->nTime[nConnectLimit - 1] > 0) // if the time array was full
		{
			for(int ii=0; ii<nConnectLimit; ii++ )
				(*Root)->nTime[ii] = (*Root)->nTime[ii+1]; // slide the time, bump off the oldest.
			(*Root)->nTime[nConnectLimit - 1] = nNow;
		}
		else // add this time to the end of the array.
		{
			for (int i = 1; i < nConnectLimit; i++)
			{
				if ((*Root)->nTime[i] == 0)
				{
					(*Root)->nTime[i] = nNow;
					break;
				}
			}
		}
		return 0;
	}

	if( nTreePlacementTest > 0 )
		return AddTime( &((*Root)->Left), pzIP );
	else 
		return AddTime( &((*Root)->Right), pzIP );
	return 0;
}





// For a fast implementation of connection restrictions:
// Incoming connections are hashed(based on IP address) to index 
// an integer in this array, that is incremented when a connection
// is being processed, and decremented when that thread completes.
//
// Collisions will case 2 ip's to 'double the cumulative active connection total'
// A large hash causes the collision factor to be almost non existent
// considering proxies/routers will cause the same (cumulative active connection total) 
// to all ip's on the subnet behind the proxy/router.  So connections are restricted
// to a limited number per subnet, for security that's even better than per IP.
//
//
// the hash is divided into two equal regions (increment & decrement). So :
//			hash[i] - hash[ACTIVE_CONN_HASH_SIZE / 2) + i]
//	equals the real-time current connection count for the ip(s) that hash to i.
//                              AND
//  hash[i] is the total number of connections from the IP that have been
//  counted since 'this' server has started, isn't that some interesting
//  stuff to have in memory?
//
#define ACTIVE_CONN_HASH_SIZE 200000
#define HASHLOCK_ARR_SIZE 50
long HashOfActiveConnections[ACTIVE_CONN_HASH_SIZE];
static gthread_mutex_t ArrayOfLocks[HASHLOCK_ARR_SIZE];

struct ConnHashInit
{
	ConnHashInit()
	{ 
		memset(HashOfActiveConnections,0,ACTIVE_CONN_HASH_SIZE );
		for(int i=0;i<HASHLOCK_ARR_SIZE;i++)
		{
			gthread_mutex_init(&ArrayOfLocks[i],0);
		}
	}
	~ConnHashInit()
	{ 
		for(int i=0;i<HASHLOCK_ARR_SIZE;i++)
		{
			gthread_mutex_destroy(&ArrayOfLocks[i]);
		}
	}
} g_ConnHashInit ;





// nChange of 1 increments, -1 decrements
static int ConnectionCounter ( const char *pzAddress, int nChange )
{
	const unsigned char *ip = (const unsigned char *)pzAddress;
	unsigned long   h = 0, g;
    while ( *ip )
    {
        h = ( h << 4 ) + *ip++;
        if ( (g = h & 0xF0000000) )
            h ^= g >> 24;
        h &= ~g;
    }
    int idx = h % ( ACTIVE_CONN_HASH_SIZE / 2);

	if (nChange == 1)											
	{
#ifdef _WIN32						
		InterlockedIncrement(&HashOfActiveConnections[idx]);	// this is lockless
	#ifdef _WIN64
		InterlockedIncrement64(&g_ActiveConnections);
		InterlockedIncrement64(&g_TotalConnections);
	#else
		InterlockedIncrement(&g_ActiveConnections);
		InterlockedIncrement(&g_TotalConnections);
	#endif
#else
		int nLockIdx = idx % HASHLOCK_ARR_SIZE;
		gthread_mutex_lock(&ArrayOfLocks[nLockIdx]);			// and this blocks 50 times less than a single global lock
		HashOfActiveConnections[idx]++;
		gthread_mutex_unlock(&ArrayOfLocks[nLockIdx]);
		g_ActiveConnections++;
		g_TotalConnections++;
#endif


	}
	else
	{
#ifdef _WIN32
		InterlockedIncrement(&HashOfActiveConnections[(ACTIVE_CONN_HASH_SIZE / 2) + idx]);
	#ifdef _WIN64
		InterlockedDecrement64(&g_ActiveConnections);
	#else
		InterlockedDecrement(&g_ActiveConnections);
	#endif

#else
		int nLockIdx = idx % HASHLOCK_ARR_SIZE;
		gthread_mutex_lock(&ArrayOfLocks[nLockIdx]);			
		HashOfActiveConnections[(ACTIVE_CONN_HASH_SIZE / 2) + idx]++;
		gthread_mutex_unlock(&ArrayOfLocks[nLockIdx]);
		g_ActiveConnections--;
#endif
	}
	
	return HashOfActiveConnections[idx] - HashOfActiveConnections[(ACTIVE_CONN_HASH_SIZE / 2) + idx];
}


void BuildServicePermissionList(int nType)
{
	IPQueryEntry **pFirst = 0;
	IPQueryEntry **pLast = 0;
	GStringList lst;

	GString strReason;
	if (nType == 0)
	{
		pFirst = &g_FirstRefusedIP;
		pLast = &g_LastRefusedIP;
		lst.DeSerialize(",", GetProfile().GetString("System","RefuseService",false));
		strReason = "[System] RefuseService";
	}
	else if (nType == 1)
	{
		pFirst = &g_FirstAllowedIP;
		pLast = &g_LastAllowedIP;
		lst.DeSerialize(",", GetProfile().GetString("System","GrantService",false));
		strReason = "[System] GrantService";
	}


	if (*pFirst) // the first time the list is built this will be null
	{
		// todo: delete the previous settings if we are rebuilding the list
		*pFirst = 0;
	}


	GStringIterator itRefuse(&lst);
	while (itRefuse())
	{
		IPQueryEntry *pCurrent = 0;
		const char *pzIp = itRefuse++;
		char *pDash = (char *)strpbrk(pzIp, "-");
		if (pDash)
		{
			// a range between two IP's
			*pDash = 0; 
			pCurrent = new IPQueryEntry(IpToValue(pzIp),IpToValue(pDash+1));
			*pDash = '-';
		}
		else
		{
			// a single IP
			pCurrent = new IPQueryEntry(IpToValue(pzIp));
		}
		pCurrent->strReason = strReason;
		if (!(*pFirst) )
		{
			*pFirst = pCurrent; // first one in the list
			*pLast = pCurrent;
		}
		else
		{
			(*pLast)->pNext = pCurrent;
		}
		*pLast = pCurrent;
	}
}



// QueryServicePermission() returns 1 if the ip is in the specified list
// nType  0 == "global refuse service"
// nType  1 == "global allow service"
// todo: add  more specific IP based service permissions (per protocol, and per switchboard operation)
int QueryServicePermission(unsigned long ip, int nType)
{
	// load the denied IP's from the config file on the first lookup.
	IPQueryEntry *pFirst = 0;
	if (nType == 0)
	{
		pFirst = g_FirstRefusedIP;
	}
	else if (nType == 1)
	{
		pFirst = g_FirstAllowedIP;
	}

	IPQueryEntry *pCurrRefusedIP = pFirst;
	if (pCurrRefusedIP)
	{
	#ifdef _WIN32
		ip = htonl(ip);
	#endif
		unsigned long now = time(0);
		while (pCurrRefusedIP)
		{
			// integer ip comparison
			if (ip >= pCurrRefusedIP->nIPBegin && ip <= pCurrRefusedIP->nIPEnd)
			{
				if (pCurrRefusedIP->nRefuseUntil == 0)
				{
					// this ip is indefinitely in the list.
					pCurrRefusedIP->nRefusedCount++;
					return 1; 
				}

				if ( pCurrRefusedIP->nRefuseUntil > now )
				{
					// this ip is in the list for [pCurrRefusedIP->nRefuseUntil - now] more seconds.
					pCurrRefusedIP->nRefusedCount++;
					return 1; 
				}
			}
			pCurrRefusedIP = pCurrRefusedIP->pNext;
		}
	}
	return 0; // this ip is not in the list
}


void GetBalanceTo(const char *pzBalance, GString &strDest)
{
	if (pzBalance && pzBalance[0])
	{
		int nDistributionKey = rand() % 100; // creates a random #: 0-99
		GStringList lst(",",pzBalance);

		GStringIterator itParams(&lst);
		int nDistributionChannel = 0;
		while (itParams())
		{
			nDistributionChannel += atoi(itParams++); // should add up to 100
			if (!itParams())
				break; // bad config file format
			const char *pzServer = itParams++;
			if (nDistributionChannel > nDistributionKey)
			{
				strDest = pzServer;
				break;
			}
		}
	}
}


// feel free to replace this entire table, with special file types that will be served up by this HTTP Server
const char *mimeTypeTable[] = {
   "mpeg",  "video/x-mpeg", //(*.mpeg,*.mpg,*.m1s,*.m1v,*.m1a,*.m75,*.m15,*.mp2,*.mpm,*.mpv,*.mpa)
   "mpg",  "video/x-mpeg",
   "wav",  "audio/x-wav",
   "mp3",  "audio/mpeg",
   "mid",  "audio/midi",
   "doc",  "application/msword",
   "rtf",  "application/rtf",
   "swf",  "application/x-shockwave-flash",
   "tar",  "application/x-tar",
   "gtar",  "application/x-gtar",
   "xls",  "application/excel",
   "ico",  "image/x-icon",
   "dwf",  "drawing/x-dwf",
   "avi",  "video/x-msvideo",
   "pkm",  "application/octet-stream",
   "xml",  "text/xml",
   "css",  "text/css",
   "exe",  "application/octet-stream",
   "gif",  "image/gif",
   "jpeg", "image/jpeg",
   "jpg",  "image/jpeg",
   "htm",  "text/html",
   "html", "text/html",
   "txt",  "text/plain",
   "png",  "image/png",
   "pdf",  "application/pdf",
   "js",   "application/x-javascript",
   "zip",  "application/zip",
   "gz",   "application/x-gzip",
   "jar",  "application/java-archive",
   "apk",  "application/vnd.android.package-archive",
   0
};


const char *GetProtocolName(int nProtocol)
{
	static GString strStorage;
	switch(nProtocol)
	{
		case 1: strStorage = "HTTP"; break;
		case 2: strStorage = "Proxy-Tunnel"; break;
		case 3: strStorage = "Xfer"; break;
		case 4: strStorage = "TXML"; break;
		case 5: strStorage = "TXML+HTTP"; break;
		case 6: strStorage = "Remote-WS"; break;
		case 7: strStorage = "Reserved"; break;
		case 8: strStorage = "Messaging"; break;
		case 9: strStorage = "PrivateSurf"; break;
		case 10: strStorage = "PortBlock"; break;
		case 11: strStorage = "HTTPProxy"; break;
		case 12: strStorage = "OpenSurf"; break;
		case 13: strStorage = "RemoteConsole"; break;
		default : strStorage = "Unknown"; break;
	}
	return strStorage;
}



// This (structure/class/object) contains information "about a listener"
// That is everything needed to listen on a port and apply a protocol.
// This information is passed into the thread that "does the work" but
// no thread state should ever be written into this structure because
// multiple threads servicing this port will all point to the same 
// ThreadStartupData (structure/class/object) it should be treated
// as "read-only" to the servicing thread, they are public only for
// unwanted Get/Set accessors.
class ThreadStartupData
{
	ThreadStartupData *pNext;
public:
	int nState; // -2 bind error, -1 permission error, 0 stopped, 1 ready, 2 waiting, 3 servicing
	int nProtocol; // 0 Root, 1 HTTP, 2 Proxy/Tunnel, 3 Xfer, 4 TXML, 5 TXML&HTTP, 6 Remote-WS, 7 Reserved, 8 Messaging, 9 PrivateSurf, 10 PortBlock, 11 HTTPProxy, 12 OpenSurf
	int nInstanceLimit; // 0 is no limit, 1 services a single transaction at a time.
	int nPort;
	int sockfd;	// the handle to the port listener
	int bThreadShuttingDown;

	// stats for this port reported by listening thread
	int nTooFrequent;
	int nTooMany;
	int nRefused;
	int nCount;

	char szConfigSectionName[64];
	
	// if nProtocol == 2 this will also be set
	int nProxyToPort;
	char szProxyToServer[1024];
	char szHTTPProxyFirewall[1024];
	int nProxyTimeout;
	
	bool bIsLogging;
	GString strLogFile;

	int bIsTunnel;

	// ThreadStartupData describes the static aspects of a type of connection,
	// If the connection was initiated through the (~route) mechanism, the nPort
	// will be set to -777 and pzPostConnectPathAndName will contain a value like:
	// /MySwitchPath/MyVNCSwitchName.html
	char pzPostConnectPathAndName[2048];

	CipherData *pcd;

	ThreadStartupData * Next(int bCreateIfNone = 0)
	{
		if (!pNext && bCreateIfNone)
			pNext = new ThreadStartupData;
		return pNext;
	}
	ThreadStartupData()
	{
		bIsLogging = 0;
		nTooFrequent = 0;
		nTooMany = 0;
		nRefused = 0;
		nCount = 0;
		nPort = -1;

		nInstanceLimit = 0;
		bThreadShuttingDown = 0;
		bIsTunnel = 0;
		nProtocol = 0;
		pNext = 0;
		nProxyToPort = 0;
		szProxyToServer[0] = 0;
		szHTTPProxyFirewall[0] = 0;
		szConfigSectionName[0] = 0;
		pcd = 0;
		nState = 0;
	}
};
ThreadStartupData g_TSD;


#define PROXY_STATE_READING_REQUEST		0x01
#define PROXY_STATE_READING_RESPONSE	0x02
#define PROXY_STATE_AWAITING_REQUEST	0x04
#define PROXY_STATE_AWAITING_RESPONSE	0x08


class ThreadConnectionData
{
public:
	int bHasStartedThread;


	int *pnProxyClosed;
	int *pnProxyFlags;
	ThreadStartupData *pTSD;

	int sockfd;		// the handle for 'this' accept()'ed connection on pTSD->nPort
	int *pnsockfd;	// pointer to sockfd in another ThreadData.
	
	int *pnProxyfd;	
	unsigned long *plLastActivityTime;
	int *pbHelperThreadIsRunning;
	int nParentThreadID;
	gthread_cond_t	*pcondHelper;


	char pzClientIP[64]; // machine IP or name that started this transaction

	// when this structure is used to poll for a connection
	// this variable contains the section name like "PollExternalConnect1" ... "PollExternalConnectN"
	// or "MsgFrom-User" when used for messaging
	char pzPollSectionName[64];

	// this is set when for any Xfer protocol proxy
	char pzIP[1024];	// could be a host name, make room for a big one this is a system wide limitation for a host name length

	char pzConnectRoute[1024];
	char pzRemoteKey[1024];
	int nConnectRouteSize;
	int nAction; // 1=Proxy, 2=Tunnel, 3=Destination, 4=ConvertHTTP

	GString strAction; // this is the same strAction in the ThreadData, it's context depends on the protocol and task

	// if using HTTP authentication to make a request outside the local network
	GString strLocalNetworkAuthenticate; // "user:password"


	int nPollThreadID;
	int nPollInterval;
	int nPollQueueMax;
	int nPollSrvWait;
	int bInUse;
	int nLastPollConnectStatus;
	time_t lPollResume;
	time_t lOnErrorBeginTime;
	ThreadConnectionData *pNext;
	ThreadConnectionData *pBaseTCD;// if a poll queue is 3, all 3 point to the same base for the same switchboard
	time_t lLastActivity;

	void Copy(ThreadConnectionData *pFrom)
	{
		bInUse = 0;
		bHasStartedThread = 0;
		pNext = 0;
		nPollThreadID = 0;

		pnProxyClosed = pFrom->pnProxyClosed;
		pnProxyFlags = pFrom->pnProxyFlags;
		pTSD = pFrom->pTSD;
		sockfd = pFrom->sockfd;
		pnsockfd = pFrom->pnsockfd;

		pnProxyfd = pFrom->pnProxyfd;
		plLastActivityTime = pFrom->plLastActivityTime;
		pbHelperThreadIsRunning = pFrom->pbHelperThreadIsRunning;
		nParentThreadID = pFrom->nParentThreadID;
		pcondHelper = pFrom->pcondHelper;

		strcpy(pzClientIP,pFrom->pzClientIP);
		strcpy(pzPollSectionName,pFrom->pzPollSectionName);
		strcpy(pzIP,pFrom->pzIP);
		strcpy(pzConnectRoute,pFrom->pzConnectRoute);
		strcpy(pzRemoteKey,pFrom->pzRemoteKey);

		nConnectRouteSize = pFrom->nConnectRouteSize;
		nAction = pFrom->nAction;
		strLocalNetworkAuthenticate = pFrom->strLocalNetworkAuthenticate;
		nPollInterval = pFrom->nPollInterval;
		nPollQueueMax = pFrom->nPollQueueMax;
		nPollSrvWait = pFrom->nPollSrvWait;
		nLastPollConnectStatus = pFrom->nLastPollConnectStatus;
		lPollResume = pFrom->lPollResume;
		lOnErrorBeginTime = pFrom->lOnErrorBeginTime;
	}

	void Init()
	{
		nAction = 0; // none
		pzIP[0] = 0;
		pzPollSectionName[0] = 0;
		pzConnectRoute[0] = 0;
		nConnectRouteSize = 0;
		pzRemoteKey[0] = 0;
		pzClientIP[0] = 0;
		
		nPollInterval = 15;
		nPollQueueMax = 1;
		nPollSrvWait = 300;
		
		lPollResume = 0;

		nParentThreadID = 0;
		pcondHelper = 0;
		plLastActivityTime = 0;
		pbHelperThreadIsRunning = 0;
		pnProxyClosed = 0;
		pnProxyFlags = 0;
		sockfd = 0;
		pnsockfd = 0;

		bInUse = 0;
		nLastPollConnectStatus = 0;
		pNext = 0;

		bHasStartedThread = 0;
		nPollThreadID = 0;
		pBaseTCD = 0;
		lLastActivity = 0;
	}
	
	ThreadConnectionData()
	{
		Init();
	}
};


// ThreadData's do not come and go, they are created 1 per thread and pooled with the thread.
class ThreadData
{
public:
	char *Buf1;
	char *Buf2;


	ThreadStartupData *pTSD;
	/////////////////////////////////////////////////////////
	int nProxyClosed;	 // 0 is "Not Closed" so 0 is a Running Proxy.  1 2 3 are stopping states.  777 is uninitialized.
	int *pnProxyClosed;  // points to a nProxyClosed in another ThreadData for shared memory between those 2 threads
	/////////////////////////////////////////////////////////
	int	sockfd;			 // this is the accepted fd from the client in the clientthread() - not used in the ProxyWorkerThread, set to -1
	int *pnsockfd;		 // points to a sockfd in another ThreadData.  not used in clientthread()
	/////////////////////////////////////////////////////////
	int nProxyFlags;	
	int *pnProxyFlags;
	int nProxyfd;
	/////////////////////////////////////////////////////////


	gthread_t chld_thr;

	gthread_mutex_t lock;
	gthread_cond_t	cond;

	gthread_mutex_t lockFinished;
	gthread_cond_t	condFinished;

	int nThreadIsBusy;
	int nThreadID;
	int m_bUseThreads;
	int nSequence; // nCmdSeq
	long starttime;

	__int64 nTotalBytesProxied; // clientthread sent to server				|| ProxyHelperThread sent to client
	__int64 nTotalBytesRead;    // clientthread read from browser/client	|| ProxyHelperThread read from server
	__int64 nTotalBytesSent;    // clientthread sent to client				|| 0

	int nParentThreadID;

	unsigned long *plLastActivityTime;
	int *pbHelperThreadIsRunning;

	gthread_mutex_t lockLog;
	gthread_cond_t	condLog;

	gthread_mutex_t lockHelper;
	gthread_cond_t	condHelper;
	// only set if this is a ProxyHelperThread 'attached' to the ThreadData of it's 
	// clientthread.  The following variables will actually point to the variables 
	// preceding this comment in the ThreadData of the clientthread that is proxying.
	gthread_cond_t	*pcondHelper;
	int	*pnProxyfd;
	char pzClientIP[32];
	
	// for debugging, a text description of the action of this thread.
	GString strAction;



	char pzPollSectionName[64];


	char pzIP[1024];
	
	// this space will vary per protocol, 
	// for the HTTP Proxy it is the remote connect to this form "80@www.server.com"
	// for the HTTP Server it is the Referrer
	char pzConnectRoute[1024];
	int nAction; // 1=Proxy, 2=Tunnel, 3=Destination, 4=ConvertHTTP



	ThreadData( bool bUseThreads = 1 )
	{

		Buf1 = 0;
		Buf2 = 0;
		
		nSequence = 0;
		nTotalBytesProxied = 0;
		nTotalBytesRead = 0;
		nTotalBytesSent = 0;

		nProxyFlags = 0;

		nAction = 0; // none
		pzIP[0] = 0;
		pzPollSectionName[0] = 0;
		pzConnectRoute[0] = 0;
		
		nProxyClosed = 777; // 777 is uninitialized, 0 is an open proxy
		nProxyfd = -1; // -1 is an invalid socket handle

		nParentThreadID = 0;
		pcondHelper = 0;
		pnProxyClosed = 0;
		pnProxyFlags = 0;
		plLastActivityTime = 0;
		pbHelperThreadIsRunning = 0;
		pTSD = 0;

		m_bUseThreads = bUseThreads;
		if (bUseThreads)
			nThreadID = ++g_ThreadIDCount;
		else
			nThreadID = 1;
		sockfd = 0;
		nThreadIsBusy = 0;
		if (m_bUseThreads)
		{
			gthread_mutex_init(&lock,0);
			gthread_cond_init(&cond,0);
			gthread_mutex_init(&lockHelper,0);
			gthread_cond_init(&condHelper,0);
			gthread_mutex_init(&lockLog,0);
			gthread_cond_init(&condLog,0);

			gthread_mutex_init(&lockFinished,0);
			gthread_cond_init(&condFinished,0);

			// lock the mutex so the thread awaits until woken.
			gthread_mutex_lock(&lockHelper);
			gthread_mutex_lock(&lockFinished);
			gthread_mutex_lock(&lock);
		}
	}
	~ThreadData()
	{
		if (m_bUseThreads)
		{
			gthread_mutex_destroy(&lock);
			gthread_cond_destroy(&cond);
			gthread_mutex_destroy(&lockHelper);
			gthread_cond_destroy(&condHelper);
			gthread_mutex_destroy(&lockLog);
			gthread_cond_destroy(&condLog);

			gthread_mutex_destroy(&lockFinished);
			gthread_cond_destroy(&condFinished);
		}
	}
};
int g_nProxyPoolRoundRobin = 0;
int g_nThreadPoolRoundRobin = 0;


void PingPool(GArray *pPool, int nPoolSize)
{
	g_ThreadPing = 1;
	if( nPoolSize > 1 )
	{
		for(int i=0; i < nPoolSize; i++)
		{
			ThreadData *td = (ThreadData *)(pPool->m_arrObjects[i].m_pValue);
			if (!td->nThreadIsBusy)
			{
				if (gthread_mutex_trylock( &td->lock ) == EBUSY)
				{
					continue;
				}
				gthread_mutex_unlock( &td->lock );

				// wake it up it'll call InfoLog() then go back to sleep.
				gthread_cond_signal(  &(td->cond) ); 
			}
		}
	}
	
	// give 'em plenty of time to finish before we turn off the g_ThreadPing flag.
	PortableSleep(1,0);

	g_ThreadPing = 0;
}



void DestroyThreadPool(GArray *pPool, int nPoolSize)
{
	g_ServerIsShuttingDown = 1;
	if( nPoolSize > 1 )
	{
		GList lstHungOrBusy;
		int i;
		for(i=0; i < nPoolSize; i++)
		{
			ThreadData *td = (ThreadData *)(pPool->m_arrObjects[i].m_pValue);
			if (!td->nThreadIsBusy)
			{
				if (gthread_mutex_trylock( &td->lock ) == EBUSY)
				{
					lstHungOrBusy.AddLast(td);
					continue;
				}
				gthread_mutex_unlock( &td->lock );
				// wake it up to die a natural death
				gthread_cond_signal(  &td->cond ); 
			}
			else
			{
				lstHungOrBusy.AddLast(td);
			}
		}
		// give the threads plenty of time to end normally.
		for(i=0 ;i < nPoolSize * 2; i++)
			gsched_yield();

		if (lstHungOrBusy.Size())
		{
			// we can kill them here, if needed
			InfoLog(5, "Threads still working.");
		}
	}
}


int KillTid (int ntidToKill, int bOnlyEndIt = 0)
{
try
{
	if( g_ThreadPoolSize > 1 )
	{
		// primary pool
		for(int i=0; i < g_ThreadPoolSize; i++)
		{
			ThreadData *td = (ThreadData *)(g_PoolClientThread->m_arrObjects[i].m_pValue);

			if(td->nThreadID == ntidToKill)
			{
				if (bOnlyEndIt)
				{
					if ( td->nProxyClosed == 0 )
					{
						td->nProxyClosed = 1; // 1 == Shut down in progress.
					}

					GString g("******* ADMIN MANUALLY CLOSED fd:");
					g << td->sockfd << " on tid:" << ntidToKill << " connected out to[" << td->pzConnectRoute << "] fd:" << td->nProxyfd << " Action=[" << td->strAction << "]";
					InfoLog(8567,g);


					PortableClose(td->sockfd,	 "KillTid");
					PortableClose(td->nProxyfd,"KillTid");
					td->sockfd = -1;
					td->nProxyfd = -1;
				}
				else
				{
					GString g("******* ADMIN MANUALLY KILLED tid:");
					g << td->nThreadID << " fd:" << td->sockfd << " connected out to[" << td->pzConnectRoute << "] fd:" << td->nProxyfd << " Action=[" << td->strAction << "]";
					InfoLog(8567,g);


					int nRet = gthread_cancel (td->chld_thr);
					if (nRet == 0) // if it worked
					{
						if (td->Buf1)
							delete td->Buf1; // search this file for KillTidMemoryNote
						gthread_create(&td->chld_thr,NULL, clientThread, (void *)td );
						td->nThreadIsBusy = 0;
					}

				}
				return 0;
			}
		}
	}
	if( g_ProxyPoolSize > 1 )
	{
		// proxy pool
		for(int i=0; i < g_ProxyPoolSize; i++)
		{
			ThreadData *td = (ThreadData *)(g_PoolProxyHelperThread->m_arrObjects[i].m_pValue);

			if(td->nThreadID == ntidToKill)
			{
				int nRet = gthread_cancel (td->chld_thr);
				if (nRet == 0) // if it worked
				{
					if (td->Buf1)
						delete td->Buf1; // search this file for KillTidMemoryNote
					gthread_create(&td->chld_thr,NULL, ProxyHelperThread, (void *)td );
					td->nThreadIsBusy = 0;
				}
				return 0;
			}
		}

		// invalid [ntidToKill]
		return 1;
	}
}
catch(...)
{
	return 3; // OS error
}

	// system not using threads
	return 2;
}


void CreateThreadPool(GArray **pGPool, int nPoolSize, void *(*pfnThreadProc) (void *))
{
	if( nPoolSize > 1 )
	{
		*pGPool = new GArray(nPoolSize * 2); // the pool starts at the specified size and currently cannot ever exceed twice that size

		for(int i=0; i < nPoolSize; i++)
		{
			ThreadData *td = new ThreadData;

			(*pGPool)->AddElement(td);
			// the mutex is locked so pfnThreadProc hangs shortly after creation
			gthread_create(&td->chld_thr,NULL, pfnThreadProc, (void *)td );
			gsched_yield();
		}
	}
}



void AttachToClientThreadHelper(ThreadData *pTD, ThreadConnectionData *pTCD)
{
	ThreadStartupData *pTSD = pTCD->pTSD;

//	if (!IsOpenSocket(pTCD->sockfd))
//	{
//		// break in the debugger
//	}

	pTD->sockfd = pTCD->sockfd;		// the accepted fd for clientthread, nothing for ProxyHelperThread
	pTD->pnsockfd = pTCD->pnsockfd; // nothing for clientthread, pointer to the shared client side fd in ThreadData

	pTD->pTSD = pTSD;
	pTD->strAction = pTCD->strAction;
	pTD->pcondHelper = pTCD->pcondHelper;
	pTD->pnProxyClosed = pTCD->pnProxyClosed;
	pTD->pnProxyFlags =  pTCD->pnProxyFlags;
	pTD->nParentThreadID = pTCD->nParentThreadID;
	pTD->pnProxyfd = pTCD->pnProxyfd;
	pTD->plLastActivityTime = pTCD->plLastActivityTime;
	pTD->pbHelperThreadIsRunning = pTCD->pbHelperThreadIsRunning;
	strcpy(pTD->pzClientIP,pTCD->pzClientIP);


	strcpy(pTD->pzIP,pTCD->pzIP); // for protocol 6 && 3 && 8

	strcpy(pTD->pzPollSectionName,pTCD->pzPollSectionName );

	if (pTCD->nConnectRouteSize) // protocols 6,11, and 2 use this
	{
		strcpy(pTD->pzConnectRoute,pTCD->pzConnectRoute );  
	}
	gthread_cond_signal(  &(pTD->cond) ); 
}

ThreadData *AttachToClientThread(ThreadConnectionData *pTCD, GArray *pGPool,int nPoolSize, int *pnPoolIssuedCount)
{
	// try round robin approach first, see if the thread that was woken up the longest time ago has completed it's 
	// task, and is sleeping again.  Depending on load and pool size 99+% of connections attach on Round Robin.
	// If we're handing out something like a telnet connection that may last for days - it's not so likely to 
	// hit an idle thread in the pool on the the first try - but then speed is a little less an issue anyhow.
	//
	// It's faster than using a stack(list) of available threads since no synchronizations objects need to be 
	// used to set the index or check the thread at that index doing it this way.  Even on RISC or multi CPU
	// machines this is safe because we always index at the modulus of the result.
	int n = ++(*pnPoolIssuedCount) % nPoolSize;
	ThreadData *pTD = (ThreadData *)(pGPool->m_arrObjects[n].m_pValue);


	// This Flag + Lock check guarantees proper pool checkin/checkout - either one alone is insufficient.
	// only 1 in a million (or less) will ever call gthread_mutex_trylock() and not obtain the lock since that would
	// mean that a thread was preempted exactly between the assignment of nThreadIsBusy and the gthread_cond_wait()
	// gthread_cond_wait() is the very next line of code after the assignment of nThreadIsBusy in the workerthread.
	// It does happen - very rarely.
	if (!pTD->nThreadIsBusy  &&  gthread_mutex_trylock( &pTD->lock ) != EBUSY)
	{									
		pTD->nThreadIsBusy = 1;		
		gthread_mutex_unlock( &pTD->lock );
		AttachToClientThreadHelper(pTD, pTCD);
		return pTD;
	}
	
	else // that thread is still busy, find another one.
	{
		// walk through the thread pool until we find an available thread.
		// it's not a list, it's an array so walking it is very fast.
		for(int i = 0;;i++)	
		{
			pTD = (ThreadData *)(pGPool->m_arrObjects[i].m_pValue);

			if (!pTD->nThreadIsBusy  &&  gthread_mutex_trylock( &pTD->lock ) != EBUSY)
			{
				pTD->nThreadIsBusy = 1;
				gthread_mutex_unlock( &pTD->lock );
				AttachToClientThreadHelper(pTD, pTCD);
				return pTD;
			}
			if (i == (nPoolSize - 1) ) // if we made it all the way through the pool and they are all busy
			{
				// todo: check system memory to see if auto grow is feasible

				// lock only the grow procedure, 
				// the pool is NOT locked, - other threads may still be using it during it's expansion.
				// however, no other threads may be expanding it while this thread is expanding it.
				gthread_mutex_lock(&PoolExpansionLock); 

				int bCreatedThread = 0;
				ThreadData *pNewTD = 0;
				// is is very important that the array is not reallocated here, this check ensures it never happens.
				// it also ensures that the pool does not grow forever, since it cannot exceed the pre-allocated grow space.
				if (pGPool->GetItemCount() < pGPool->GetSize())
				{
					pNewTD = new ThreadData;
					pNewTD->nThreadIsBusy = 1;
					int nReCreateRetry = 0; 

					while(!bCreatedThread && nReCreateRetry++ < 3)
					{
						if(pGPool == g_PoolClientThread) // there are two thread pools, decide which one we are dealing with
						{
							if ( gthread_create(&pNewTD->chld_thr,NULL, clientThread, (void *)pNewTD ) == 0)
							{
								bCreatedThread = 1;
							}
						}
						else
						{
							if (gthread_create(&pNewTD->chld_thr,NULL, ProxyHelperThread, (void *)pNewTD ) == 0)
							{
								bCreatedThread = 1;
							}
						}
						if (!bCreatedThread)
						{
							gsched_yield();
						}
					}

					if(bCreatedThread)
					{
if (nReCreateRetry > 1)
{
	// never happens.
	InfoLog(1234567,"^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^a retry fixed it!");
}

						while(pNewTD->nThreadIsBusy) // when it's not busy, it is ready, initialized and waiting
						{
							gsched_yield(); // it won't take more than a loop or two
						}
						// now, before we increment the pool size (making it available for other connections to use)
						// re-flag it as in use, so that nobody else can pick it up before the guy we created it for.
						pNewTD->nThreadIsBusy = 1; 

						pGPool->AddElement(pNewTD);	// now it's in the pool, but no other thread could know it's there because it's past the bounds of the known pool size
						
						if(pGPool == g_PoolClientThread) // there are two thread pools, decide which one we are dealing with
							g_ThreadPoolSize++; // now it's in the pool for everyone to see.
						else
							g_ProxyPoolSize++;  // now it's in the pool for everyone to see.
					}
					else
					{
						// we failed to create the thread - but we should have never tried if resources were low
						// so this could only happen if the thread pool was grossly mis-configured for this hardware.
						delete pNewTD; 
					}
				}

				gthread_mutex_unlock(&PoolExpansionLock);

				if (bCreatedThread)
				{
					AttachToClientThreadHelper(pNewTD, pTCD);
					return pNewTD;
				}
				else
				{

					//10=-- Server Too Busy
					GString strLog;
					strLog.LoadResource(g_pzCoreErrorSection, 10);
					InfoLog(6,strLog);

					// bad news.  The requests are coming faster than we can deal with them, and we are not configured to, or cannot, create another thread to handle this request.
					// If this happens, you need a bigger thread pool, more memory,  or a faster server. - depending on your configuration settings
					// It's also possible that we have too many idle connections, try reducing timeout values
					g_TotalTooBusy++;

					// reject the pending connection.
					PortableClose(*pTCD->pnsockfd,"Core4");
					return 0;
				}
			}
		}
	}
	return 0;
}



gthread_mutex_t g_csMtx2;
int g_csMtxInit2 = 0;
GString g_strHTTPHitLog;

void MutexLog(const char *strFileNoPath,const char *pzRequestingIP,const char *strReferer, const char *pzExt, const char *pzCommentNoCommas)
{
	if (g_strHTTPHitLog.IsEmpty())
	{
		return;
	}

	// If we have been explicitly told not to log requests for this file type - leave.
	// note: "static" means that the data is initialized only once, not every time this execution come here.
	static const char *pzNonLogTypes = GetProfile().GetString("HTTP","NonLoggingTypes",false);
	if (pzNonLogTypes)
	{
		static GStringList row(",",pzNonLogTypes);
		GStringIterator itParams(&row);
		while (itParams())
		{
			GString strNonLogType(itParams++);
			if (strNonLogType.CompareNoCase(pzExt) == 0 )
				return; // don't log this one
		}
	}


	// If we have been explicitly told not to log requests for this file - leave.
	static const char *pzNonLogFiles = GetProfile().GetString("HTTP","NonLoggingFiles",false);
	if (pzNonLogTypes)
	{
		static GStringList row2(",",pzNonLogFiles);
		GStringIterator itParams(&row2);
		while (itParams())
		{
			GString strNonLogType(itParams++);
			if (strNonLogType.CompareNoCase(strFileNoPath) == 0 )
				return; // don't log this one
		}
	}


	time_t lTime;
	time(&lTime);
	
	GString strComment(pzCommentNoCommas);
	GString strLog;
	strLog << lTime << "," << strComment << "," << strFileNoPath << "," << pzRequestingIP << ","  << strReferer << "\n";

	if (!g_csMtxInit2)
	{
		g_csMtxInit2 = 1;
		gthread_mutex_init(&g_csMtx2, NULL);
	}

	gthread_mutex_lock(&g_csMtx2);
	strLog.ToFileAppend(g_strHTTPHitLog,0);
	gthread_mutex_unlock(&g_csMtx2);
}


// When posting data through a GET, from a browser, the 
// URL Data has certain characters hex encoded (a space becomes %20 for example)
// RFC 2396  = "-" | "_" | "." | "!" | "~" | "%" |  "*" | "'" | "(" | ")"
// You may supply the same memory address from [pSource] and [pDest] to this function
int unEscapeData(const char *pSource,const char *pDest)
{
	// cast off the const and store the values
	char *pszQuery = (char *)pSource;
	char *t = (char *)pDest;
	
	// walk the pSource until we find a NULL or a space
	while (*pszQuery && *pszQuery != 32)
	{
		switch (*pszQuery)
		{
		case '+' :
			*t = ' ';
			break;
		case '%' :
			{
				pszQuery++;
				char chTemp = pszQuery[2];
				pszQuery[2] = 0;
				char *pStart = pszQuery;
				char *pEnd;
				*t = (char)strtol(pStart, &pEnd, 16);
				pszQuery[2] = chTemp;
				pszQuery = pEnd;
				t++;
				continue;
			}
			break;
		default :
			*t = *pszQuery;
			break;
		}
		pszQuery++;
		t++;
	}
	*t = 0;
	return t - pDest;
}

int DoesCacheHTTPContent(GString &strExt, long lBytes)
{
	if ( GetProfile().GetBoolOrDefault("HTTP","ContentCache",0) )
	{
		int nMax = GetProfile().GetInt("HTTP","MaxCacheFileSize",false);
		if (nMax > 1)
		{
			if (lBytes > nMax)
			{
				return 0; // too big to cache
			}
		}
		const char *pzNonCacheTypes = GetProfile().GetString("HTTP","NonCacheTypes",false);
		if (pzNonCacheTypes)
		{
			GStringList types(",",pzNonCacheTypes);
			GStringIterator itTypes(&types);
			while (itTypes())
			{
				// if this is a file type that does not get cached
				if ( strExt.CompareNoCase( itTypes++ ) == 0 )
				{
					return 0; // don't cache it
				}
			}
		}

		const char *pzOnlyCacheTypes = GetProfile().GetString("HTTP","OnlyCacheTypes",false);
		if (pzOnlyCacheTypes && pzOnlyCacheTypes[0] )
		{
			GStringList types(",",pzOnlyCacheTypes);
			GStringIterator itTypes(&types);
			while (itTypes())
			{
				// if this is a file type that does not get cached
				if ( strExt.CompareNoCase( itTypes++ ) == 0 )
				{
					return 1; //cache it
				}
			}
			return 0; //don't cache it, it's not one of the types we cache
		}
	}
	else
	{
		return 0; // caching turned off
	}
	return 1;
}


// for sending back 404's or 301's , any non-file response.
int SendHTTPNoFileResponse(int fd, GString &strError, GString &strHTML)
{

	// HTTP header server name
	const char *pWebSrvType=GetProfile().GetString("HTTP","HTTPHeaderServerName",false);
	strError += "Server: ";
	if (pWebSrvType && pWebSrvType[0])
		strError += pWebSrvType;
	else
		strError += "5Loaves";
	strError += "\r\n";

	// add the current time
	char pzTime[128];
	struct tm *newtime;
	time_t ltime;
	time(&ltime);
	newtime = gmtime(&ltime);
	strftime(pzTime, 128, "Date: %a, %d %b %Y %H:%M:%S GMT\r\n", newtime);
	strError += pzTime;

	strError += "Content-Type: text/html\r\n";

	strError += "Content-Length: ";
	strError += strHTML.Length();
	strError += "\r\n\r\n";
	strError += strHTML;
	
	return nonblocksend(fd,strError, (int)strError.Length());
}

void BuildHTTPHeader(char *header, const char *pzFileName, const char *pzConnectionType, int nIsKeepAlive, int nKeepAliveReturnValue, const char *pzETag, const char *pzExt, long lBytes, int nBeginRange, int nEndRange)
{
	memset(header,0,512);
	if (nBeginRange > -1)
		strcat(header,"HTTP/1.1 206 Partial Content\r\n");
	else
		strcat(header,"HTTP/1.1 200 OK\r\n");

	// HTTP header: "Date: Sunday Sun, 21 Oct 1995 19:38:46 GMT\n"
	char pzTime[128];
	struct tm *newtime;
	time_t ltime;
	time(&ltime);
	newtime = gmtime(&ltime);
	strftime(pzTime, 128, "Date: %a, %d %b %Y %H:%M:%S GMT\r\n", newtime);
	strcat(header,pzTime );

	// HTTP header server name
	const char *pWebSrvType=GetProfile().GetString("HTTP","HTTPHeaderServerName",false);
	strcat(header,"Server: ");
	if (pWebSrvType && pWebSrvType[0])
		strcat(header,pWebSrvType);
	else
		strcat(header,"5Loaves");
	strcat(header,"\r\n");

	
	// HTTP header connection type
	GString strBrowserCachedHeader;
	strBrowserCachedHeader += "Connection: "; // "close" or "Keep-Alive"
	strBrowserCachedHeader += pzConnectionType;
	strBrowserCachedHeader += "\r\n";
	if (nIsKeepAlive)
	{
		strBrowserCachedHeader += "Keep-Alive: timeout=";
		strBrowserCachedHeader += g_nKeepAliveTimeout;
		strBrowserCachedHeader += ", max=";
		strBrowserCachedHeader += nKeepAliveReturnValue;
		strBrowserCachedHeader +=  "\r\n";
	}
	strcat(header,(const char *)strBrowserCachedHeader);

	
	// HTTP header: "Last-modified: Sunday Sun, 21 Oct 1995 19:38:46 GMT\n"
	if (pzETag)
	{
		time_t lFileTime = atol(pzETag);
		struct tm *tmFileTime = gmtime(&lFileTime);
		strftime(pzTime, 128, "Last-modified: %a, %d %b %Y %H:%M:%S GMT\r\n", tmFileTime);
		strcat(header,pzTime );
	}
	else
	{
		strftime(pzTime, 128, "Last-modified: %a, %d %b %Y %H:%M:%S GMT\r\n", newtime);
		strcat(header,pzTime );
	}



	// HTTP header: ETag
	if (pzFileName)
	{
		struct stat ss;
		stat(pzFileName,&ss);
		GString strETag("ETag: ");
		strETag += (unsigned long)ss.st_mtime; 
		strETag += "\r\n";
		strcat(header,(const char *)strETag);
	}
	
	// HTTP header: Content-type
	int bAddedContentType = 0;
	strcat(header,"Content-type: "); 
	if (pzExt)
	{
		GString strExt(pzExt);
		for (int iMime = 0; ;iMime++)
		{
			if (mimeTypeTable[iMime])
			{
				if ( strExt.CompareNoCase( mimeTypeTable[iMime] ) == 0)
				{
					bAddedContentType = 1;
					strcat(header,mimeTypeTable[iMime + 1]); 
					strcat(header,"\r\n");
					break;
				}
				else
				{
					iMime++;
				}
			}
			else
				break;
		}
		
		// check the user defined mime types
		if (!bAddedContentType)
		{
			const GList *pMimePairs = GetProfile().GetSection("MimeTypes");
			if (pMimePairs)
			{
				GListIterator itNVP(pMimePairs);
				while (itNVP())
				{
					GProfileEntry *pNVP = (GProfileEntry *)itNVP++;

					if ( strExt.CompareNoCase( pNVP->m_strName ) == 0)
					{
						bAddedContentType = 1;
						strcat(header,pNVP->m_strValue);
						strcat(header,"\r\n");
						break;
					}
				}
			}
		}

	}
	if (!bAddedContentType)
	{
		strcat(header,"text/html\r\n");
	}


	// If the return value is only a partial range of the entire file
	if (nBeginRange > -1)
	{
		// HTTP header: Content-length
		char cl[128];
		sprintf(cl,"Content-length: %d\r\n",(int) (nEndRange + 1) - nBeginRange);
		strcat (header,cl);

		// HTTP header: Content-Range: bytes 0-9699463/9699464
		sprintf(cl,"Content-Range: bytes %d-%d/%ld\r\n\r\n",(int)nBeginRange,(int)nEndRange,(int)lBytes);
		strcat (header,cl);
	}
	else // 98% go here - return the whole file
	{
		// HTTP header: Content-length
		char cl[128];
		sprintf(cl,"Content-length: %ld\r\n\r\n",(int)lBytes);
		strcat (header,cl);
	}
}


// Return file content from an HTTP request.
int ReturnFile(ThreadData *td, const char *pzHTTPGET, int nSize, const char *pzRequestingIP, int nKeepAlive, int bisHead, char *buf1, char*buf2, __int64 *nTotalProxiedBytes)
{
	g_TotalHits++;
	
	char pzFilePartialPath[1024];		// relative path (ex. /images/pic.gif)
	GString strFileNoPath;				// no path - file name only (ex. pic.gif)
	GString strExt(" ");				// ext only(ex.  gif)
	GString strFileName(g_strHTTPHomeDirectory);		// complete path (ex. d:\home/images/pic.gif)
	GString strError(512);		// only used when sending back a 404, or 301				
	GString strHTML(512);		// only used when sending back a 404, or 301


	if ( g_strHTTPHomeDirectory.IsEmpty() ) // the HTTP server is enabled, perhaps to run the switchboard, but there are 
											// no files to serve via HTTP requests because there is no home directory
	{
		// build a 404 Error
		strError = "HTTP/1.1 403 Forbidden\r\n";
		strHTML = "<HTML><HEAD><TITLE>403 Forbidden</TITLE></HEAD><BODY><H1>403 Forbidden</H1>This server is not configured to publish web content because no home folder is specified</BODY></HTML>";
		int nRet = SendHTTPNoFileResponse(td->sockfd, strError, strHTML);
		if (nRet > 0)
			*nTotalProxiedBytes += nRet;
		return 0;
		
	}


	//
	// Get the file name
	//
	char *pSpace = strpbrk((char *)pzHTTPGET," \r\n"); // pzHTTPGET = "/images/pic.gif HTTP/1.1 ............."
	char chSave;
	if (pSpace)	
	{
		// get the file name previous to the space before HTTP/1.1
		chSave = *pSpace;				// temp null 1st byte after the file name
		*pSpace = 0;
		strncpy(pzFilePartialPath,pzHTTPGET,1023);
		pzFilePartialPath[1023] = 0;


		// When posting data through a GET, from a browser, the 
		// URL Data has certain characters hex encoded (a space becomes %20 for example)
		unEscapeData(pzFilePartialPath,pzFilePartialPath);

		
		// append the file name to the path
		strFileName += pzFilePartialPath;	

		*pSpace = chSave;				// un-null
	}
	else
	{
		// This is a GET with no HTTP headers
		GString strF(pzHTTPGET, nSize);
		strFileName += strF;
		strcpy(pzFilePartialPath,pzHTTPGET);
	}


	// Test1:
	// prevent any file access behind the root or home directory.
	// the file path may be "/images/pic.gif", may not be "../images/pic.gif"
	// Test2:
	// Disable the HTTP "Range" feature, that is primarily used so that
	// one client can use multiple threads in the server to speed up downloads
	// by fetching different ranges of the same file at the same time.
	int bRange = GetProfile().GetBoolOrDefault("HTTP","EnableRange",1);
	if (strstr(pzFilePartialPath,"..") || ( strstr(pzHTTPGET,"Range:") && !bRange ) )
	{
		// build a 404 Error
		strError = "HTTP/1.1 404 Not Found\r\n";
		strHTML.Format("<HTML><HEAD><TITLE>404 Not Found</TITLE></HEAD><BODY><H1>404 Not Found</H1>The requested URL <code>%s</code> was not found on this server.</BODY></HTML>",(const char *)pzFilePartialPath);
		int nRet = SendHTTPNoFileResponse(td->sockfd, strError, strHTML);
		if (nRet > 0)
			*nTotalProxiedBytes += nRet;
		return 0;
	}

	char *pRange = strstr((char *)pzHTTPGET,"Range:");
	int nBeginRange = -1;
	int nEndRange = -1;
	if (pRange)
	{
		char *pNum = strpbrk(pRange + 6, "=");
		if (pNum)
		{
			nBeginRange = atoi(pNum+1);
			int bZeroOnly = GetProfile().GetBoolOrDefault("HTTP","RangeZeroBeginOnly",false);
			if (bZeroOnly && ( nBeginRange != 0) )
			{
				// drop the connection (like the server is too busy to handle it)
				// for working compatibility with most file download managers that
				// will finish the download on the primary connection.
				// This prevents a download manager to workaround connection bandwidth limits.
				return 0;
			}
			pNum = strpbrk(pNum+1, "-");
			if (pNum)
			{
				if (*pNum+1 > 47 && *pNum+1 < 58)
					nEndRange = atoi(pNum+1);
			}
		}
	}


	// This HTTP server can be configured to load balance by redirecting
	// the client to another server.  Check to see if Balancing is enabled.
	// This can be changed on the fly to balance per peak times or to
	// dodge forms of denial of service attacks.
	static const char *pzBalance = GetProfile().GetString("HTTP","Balance",false);
	// issue an HTTP redirect to another server.
	GString strBalanceToServer;
	GetBalanceTo(pzBalance,strBalanceToServer);

	// as long as the config file is valid - strBalanceToServer will always have a value
	if (strBalanceToServer.Length())
	{
		if (strBalanceToServer.CompareNoCase("this") != 0)
		{
			// build the redirect location
			GString strHREF(strBalanceToServer);	// different server
			strHREF += pzFilePartialPath;			// same place.

			strError =  "HTTP/1.1 301 Moved Permanently\r\n";
			strError += "Location: ";
			strError += strHREF;
			strError += "\r\n";

			strHTML.Format("<head><title>Document Moved</title></head><body><h1>Object Moved</h1>This document may be found <a HREF=\"%s\">here</a></body>",(const char *)strHREF);
			int nRet = SendHTTPNoFileResponse(td->sockfd, strError, strHTML);
			if (nRet > 0)
				*nTotalProxiedBytes += nRet;
			return 0; // this server has finished dealing with this request
		}
		// or service this request from this machine
	}


	// a GET for the index looks like this from IE or Netscape: pzHTTPGET= "/ HTTP/1.1 ..." 
	// also if the browser gives us a path (with no file) the FileName may end with
	//   a slash - append the default index page name as specified in the config file.
	if (strFileName.GetAt(strFileName.Length()-1) == '/' )
	{
		const char *pzIndex = GetProfile().GetString("HTTP","Index",false);
		if (pzIndex)
		{
			strcat(pzFilePartialPath,pzIndex);
			strFileName += pzIndex;
		}
		else
		{
			strcat(pzFilePartialPath,"index.html");
			strFileName += "index.html";
		}
	}
	
	
	// if strFileName contains only the home-dir path... give them the homepage
	if ( strFileName.Compare( g_strHTTPHomeDirectory ) == 0 )
	{
		const char *pzIndex = GetProfile().GetString("HTTP","Index",false);
		if (pzIndex)
		{
			strcat(pzFilePartialPath,pzIndex);
			strFileName += g_chSlash;
			strFileName += pzIndex;
		}
		else
		{
			strcat(pzFilePartialPath,"index.html");
			strFileName += g_chSlash;
			strFileName += "index.html";
		}
	}


	// walk the file name backwards until the first '.' to get the extension
	int nIdx = strlen(pzFilePartialPath) - 1;
	int bHasExt = 0;
	while (nIdx > -1)
	{
		if ( pzFilePartialPath[nIdx] == '.' && bHasExt == 0)
		{
			strExt = &pzFilePartialPath[nIdx] + 1;
			bHasExt = 1;
		}
		if ( pzFilePartialPath[nIdx] == '\\' ||  pzFilePartialPath[nIdx] == '/')
		{
			strFileNoPath = &pzFilePartialPath[nIdx] + 1;
			break;
		}
		nIdx--;
	}
	

	static const char *pzIPPage = GetProfile().GetString("HTTP","ShowIPAddressPageName",false);
	if ( pzIPPage && (strFileNoPath.CompareNoCase(pzIPPage) == 0) )
	{
		GString strIP;
		strIP.Format("<head><title>Your IP Address</title></head><body>Your Address is:%s</body>",pzRequestingIP);
		int nRet = HTTPSend(td->sockfd, strIP, (int)strIP.Length());
		if (nRet > 0)
			*nTotalProxiedBytes += nRet;
		return 0;
	}


	// if the page being requested has been configured as a page that is not browser-navigatable
	// but has been requested through an automated link-walking application.
	//
	// todo: the "static" below is there so that pzNoCrawl is only initialized one time and we do not incur the
	// overhead of this call for every HTTP page request, that's good, but it's bad because we cannot add this
	// to the "Defensive configuration" settings while the server is active and under attack.
	// use an integer comparison to a value mapped to a RegisterChangeNotification(), for the best of both worlds.
	static const char *pzNoCrawl = GetProfile().GetString("HTTP","DisableCrawlersPage",false);
	if (pzNoCrawl)
	{
		if ( !(pzNoCrawl[0] == 'n' && pzNoCrawl[1] == 'o') )
		{
			GStringList lst(",",pzNoCrawl);
			GStringIterator itPages(&lst);
			while (itPages())
			{
				if (strFileNoPath.CompareNoCase(itPages++) == 0)
				{
					// disable this IP(pzRequestingIP) for a while
					int nRefuseSeconds = GetProfile().GetInt("HTTP","CrawlerPenalty",false);
					
					GString strErr("[HTTP] CrawlerPenalty:");
					strErr << strFileNoPath;
					AddRefuseService(pzRequestingIP,nRefuseSeconds,strErr);
					return 0;
				}
			}
		}
	}




	// determine (next transaction) connection attributes
	GString strConnectionType("close");
	int nIsKeepAlive = 0;
	int nKeepAliveReturnValue = 0;
	if ( g_bUseHTTPKeepAlives )
	{
		char *pCon = strstr((char *)pzHTTPGET,"Connection:");
		if (pCon)
		{
			char *pEnd = strpbrk(pCon + 12, " \r\n");
			if (pEnd)
			{
				char chSave = *pEnd;
				*pEnd = 0;
				strConnectionType = pCon + 12;
				*pEnd = chSave;
				if (strConnectionType.CompareNoCase("Keep-Alive") == 0)
				{
					nIsKeepAlive = 1;
					nKeepAliveReturnValue = --nKeepAlive;
				}
			}
		}
	}



	// extract the Referer from the HTTP headers if supplied.

	// Interesting fact: The misspelling referer originated in the original proposal by computer scientist Phillip Hallam-Baker to incorporate the field into 
	// the HTTP specification.  The misspelling was set in stone by the time of its incorporation into the standards document Request for Comments (RFC) 1945; 
	// document co-author Roy Fielding has remarked that neither "referrer" nor the misspelling "referer" were recognized by the standard Unix spell checker 
	// of the period.  "RFC" means "Request For Comments" and nobody commented on it until it was already a standard so dont blame Phillip Hallam-Baker.
	// Blame the multitude who failed to comment.
	GString strReferer(" ");
	char *pReferer = (char *)strstr(pzHTTPGET,"Referer:");
	if (pReferer)
	{
		char *pRefererEnd = strpbrk(pReferer,"\r\n");
		if (pRefererEnd)
		{
			char chSave = *pRefererEnd;
			*pRefererEnd = 0;
			strReferer = pReferer + 9; // skip 9 bytes "Referer: "
			*pRefererEnd = chSave;
		}
	}




	// replace the file name they asked for with what we want them to have
	GString strLogComment;
	if (GetProfile().GetBoolean("HTTP","EnableLinkSwap",false))
	{
		const GList *pSpecialFile = GetProfile().GetSection(strFileNoPath);
		if (pSpecialFile)
		{
			GListIterator itNVP(pSpecialFile);
			while (itNVP())
			{
				GProfileEntry *pNVP = (GProfileEntry *)itNVP++;
				GString strCopy = pNVP->m_strValue;
				char *pSep = (char *)strpbrk(strCopy, "*");
				if (pSep)
				{
					*pSep = 0;
					if (strReferer.FindCaseInsensitive(pSep+1) != -1)
					{
						if (GetProfile().GetBoolean(strFileNoPath,"Disconnect",false))
						{
							// do not give them another image, give them nothing
							return 0;
						}

						// they asked for one thing, but give them another.
						strLogComment << "[Swapped]:" << (const char *)strFileNoPath;
						strFileNoPath = (const char *)strCopy;

						int nIdx = strlen(pzFilePartialPath) - 1;
						while (nIdx > -1)
						{
							if ( pzFilePartialPath[nIdx] == '\\' ||  pzFilePartialPath[nIdx] == '/')
							{
								strcpy(&pzFilePartialPath[nIdx+1],(const char *)pNVP->m_strName);
								break;
							}
							nIdx--;
						}
					}
				}
			}
		}
	}




	// this is a real time 'info' for this thread viewable by a thread process monitor
	td->strAction = "GET ";
	td->strAction << strFileNoPath;
	if (strReferer.Length() > 2)
		td->strAction << "," << strReferer;

	 

	// If the config file specifies that this file should be served from this, or other machines
	// as opposed to exclusively 'this' machine, then obtain the comma separated list of 
	// servers that this request might balance to.
	short bEnableHTTPBalancing = GetProfile().GetBoolean("HTTP-Balance","Enable",false);
	if (bEnableHTTPBalancing)
	{
		const char *pzBalanceFile = GetProfile().GetString("HTTP-Balance",strFileNoPath,false);
		const char *pzBalanceType = GetProfile().GetString("HTTP-Balance",strExt,false);

		const char *pzBalanceRange = 0;
		if ( pzBalanceFile || pzBalanceType || pzBalanceRange )
		{
			// Decide which balancing instructions to use:
			// Exact file name is highest precedence, next to extension match,
			// followed by a balance range match.
			const char *pzUse = (pzBalanceFile) ? pzBalanceFile : pzBalanceType;
			if (!pzUse)
				pzUse = pzBalanceRange;


			GString strBalanceToServer;
			GetBalanceTo(pzUse, strBalanceToServer);
			

			// if the 'balance to' is * then this server will handle the request,
			// any value is either an IP address or server name.
			if (strBalanceToServer.Length() && (strBalanceToServer.Compare("*") != 0) )
			{
				// Log the redirection
				strLogComment << "[Moved]:" << strBalanceToServer;
				MutexLog(strFileNoPath,pzRequestingIP,strReferer,strExt,strLogComment);

				strBalanceToServer += pzFilePartialPath;

				strError =  "HTTP/1.1 301 Moved Permanently\r\n";
				strError += "Location: ";
				strError += strBalanceToServer;
				strError += "\r\n";
				
				if(g_TraceHTTPBalance)
				{
					GString strMsg("Redirected To:");
					strMsg += strBalanceToServer;
					InfoLog(7,strMsg);
				}

				strHTML = "";
				int nRet = SendHTTPNoFileResponse(td->sockfd, strError, strHTML);
				if (nRet > 0)
					*nTotalProxiedBytes += nRet;

				// return and close the session - connection not kept alive
				return 0;

			}
			if(g_TraceHTTPBalance && (strBalanceToServer.Compare("*") == 0))
			{
				GString strMsg("NOT Redirected:");
				strMsg += strFileNoPath;
				InfoLog(8,strMsg);
			}
		}
	}



	// We will serve the file from this machine, log it to the file that exclusively logs HTTP file hits
	if (!bisHead)
		MutexLog(strFileNoPath,pzRequestingIP,strReferer,strExt,strLogComment);

	// write it to debug tracing too, if so configured
	if(g_TraceHTTPFiles)
	{
		GString strLog;
		strLog << "[" << strFileNoPath << "] to [" << pzRequestingIP << "]  HTTP referrer [" << strReferer << "]";
		InfoLog(9,strLog);
	}



	// Extract the ETag from the HTTP GET command, 
	GString strETag;
	char *p = strstr((char *)pzHTTPGET,"If-None-Match: ");
	if(p)
	{
		p+=15;// move past "If-None-Match: "
		char *pEnd = strpbrk(p, " \r\n");
		if (pEnd)
		{
			char chSave = *pEnd;
			*pEnd = 0;
			strETag = p;
			*pEnd = chSave;
		}
	}
	
	// if there was an ETag in the HTTP headers
	if (strETag.Length()) 
	{
		// load the last modified time from the file on disk
		struct stat s;
		stat((const char *)strFileName,&s);
		GString strLastModTime;
		strLastModTime.Format("%ld",s.st_mtime);

		// if it matches the ETag - shoot back a 304 to have the browser use it's local cache.
		if ( strLastModTime.Compare(strETag) == 0 )
		{
			// attach GString to pre-allocated memory 
			GString strBrowserCachedHeader(buf1, 0, MAX_RAW_CHUNK, 0);

			strBrowserCachedHeader += "HTTP/1.1 304 Not Modified\r\n";
			const char *pWebSrvType=GetProfile().GetString("HTTP","HTTPHeaderServerName",false);
			strBrowserCachedHeader += "Server: ";
			if (pWebSrvType && pWebSrvType[0])
				strBrowserCachedHeader += pWebSrvType;
			else
				strBrowserCachedHeader += "5Loaves";
			strBrowserCachedHeader += "\r\n";
				

			char pzTime[128];
			struct tm *newtime;
			time_t ltime;
			time(&ltime);
			newtime = gmtime(&ltime);
			strftime(pzTime, 128, "Date: %a, %d %b %Y %H:%M:%S GMT\r\n", newtime);

			strBrowserCachedHeader += pzTime;
			
			strBrowserCachedHeader += "Connection: "; // "close" or "Keep-Alive"
			strBrowserCachedHeader += (const char *)strConnectionType;
			strBrowserCachedHeader += "\r\n";

	
			// Defensive configuration supported - not reported.
			if (nIsKeepAlive)
			{
				strBrowserCachedHeader << "Keep-Alive: timeout=" << g_nKeepAliveTimeout << ", max=" << nKeepAliveReturnValue << "\r\n";
			}

			strBrowserCachedHeader += "ETag: "; 
			strBrowserCachedHeader +=  strETag;
			strBrowserCachedHeader +=  "\r\n";
			strBrowserCachedHeader +=  "Content-Length: 0\r\n\r\n";

			if ( nonblocksend(td->sockfd,strBrowserCachedHeader,(int)strBrowserCachedHeader.Length()) == strBrowserCachedHeader.Length() )
				*nTotalProxiedBytes += (int)strBrowserCachedHeader.Length();

			return nKeepAliveReturnValue;
		}
		else
		{
			// otherwise uncache any file that has been changed since it was cached.
			char *pOldHTTPData = (char *)cacheManager.UncacheData(strFileName);
			if (pOldHTTPData)
				delete pOldHTTPData;
		}
	}


	// look in the data cache for the pre-assembled HTTP headers and unciphered data.
	long length = 0;
	unsigned char *pData = (unsigned char *)cacheManager.GetCacheData((const char *)strFileName, &length);
	if (pData)
	{
		// if they only requested part of the file with a valid Begin Range
		if (nBeginRange > -1 && nBeginRange < length) 
		{
			// size of cached data + pre-built HTTP headers
			char *pContent= strstr((char *)pData,"\r\n\r\n");
			int nHeaderLen = (pContent - (const char *)pData) + 4; 
			int nContentLen = length - nHeaderLen; // entire length of raw file data
			
			if (nBeginRange < nContentLen)
			{
				if (nEndRange > 0 && nEndRange < nContentLen)
				{
					// user set a valid end range
				}
				else // we set it for the user
				{
					nEndRange = nContentLen - 1; // a zero based index in the HTTP header
				}
				nContentLen = (nEndRange + 1) - nBeginRange; // # of bytes in file to send


				// send them back a piece of the file in the cache.  This is request for a range
				// of bytes that will be sent back out of our complete HTTP response (headers+data)
				// that was cached.
				if (nContentLen > 0)
				{
					// send the headers
					BuildHTTPHeader(buf1, strFileName, "close" , 0, 0, strETag, strExt, length - nHeaderLen, nBeginRange, nEndRange);
					int nHdLen = strlen(buf1);

					if ( nonblocksend(td->sockfd,buf1,nHdLen) == nHdLen)
					{
						*nTotalProxiedBytes += nHdLen;

						// send the data
						int nCtLen = (nEndRange + 1) - nBeginRange;
						if ( nonblocksend(td->sockfd,pContent+4+nBeginRange, nCtLen ) == nCtLen)
						{
							*nTotalProxiedBytes += nCtLen;
						}
					}
				}
			}
		}
		else // send the entire header/content chunk directly from cache-memory (98%)
		{
			if (bisHead) // HTTP HEAD command requests the headers only, no data
			{
				char *pContent= strstr((char *)pData,"\r\n\r\n");
				length = (pContent - (const char *)pData) + 4;
			}

			// send him the file he asked for
			if ( nonblocksend(td->sockfd,pData,length) == length)
			{
				*nTotalProxiedBytes += length;
			}
		}
	}
	
	// else the file was not found in the cache, go get it from disk
	else
	{
		char *buf = 0;
		long lBytes = 0;

		const char *pDiskCipherPass=GetProfile().GetString("HTTP","EncryptedHomeDirPassword",false);
		if (pDiskCipherPass && !pDiskCipherPass[0])
			pDiskCipherPass = 0;

		if (pDiskCipherPass)
		{
			// returns 1 on success, 0 on Fail with description in  strErrorOut
			// pDest will be allocated upon success.
			// pDest will always start with these 7 bytes: 5Loaves, the nDestLen will 
			// be set to the length of the data following the first 7 bytes.
			int nDestLen;
			GString strErrorOut;
			GString strPass(pDiskCipherPass);
			if (!FileDecryptToMemory(strPass, strFileName, &buf, &nDestLen, strErrorOut))
			{
				GString strErr;
				strErr.Format("\nEncryptedHomeDir file[%s]\nReports:[%s]",(const char *)strFileName,(const char *)strErrorOut);
				InfoLog(10,strErr);

				// build a 404 Error
				strError = "HTTP/1.1 404 Not Found\r\n";
				strHTML.Format("<HTML><HEAD><TITLE>404 Not Found</TITLE></HEAD><BODY><H1>404 Not Found</H1>The requested URL <code>%s</code> was not found on this server.</BODY></HTML>",(const char *)pzFilePartialPath);
				int nRet = SendHTTPNoFileResponse(td->sockfd, strError, strHTML);
				if (nRet > 0)
					*nTotalProxiedBytes += nRet;
				return 0;
			}
			lBytes = nDestLen;

		}
		else // get the file from an unencrypted disk file
		{

			
			FILE *fp = fopen((const char *)strFileName,"rb");
			if (fp)
			{
				// get the size of the file
				fseek(fp,0,SEEK_END);
				lBytes = ftell(fp);
				fseek(fp,0,SEEK_SET);

				// if the file is < 1 Meg (or we're going to cache the content) read in the whole thing
				if (lBytes < 1024000 || DoesCacheHTTPContent(strExt,lBytes) ) 
				{
					// load the file content file
					buf = new char[lBytes +1];
					if (fread(buf,sizeof(char),lBytes,fp) != (unsigned int)lBytes)
					{
						InfoLog(11,"Failed to read entire file\n");
						lBytes = 0;
					}
				}
				fclose(fp);
			}
			else // if the requested file was not found try appending
				 // a default extension if configured to do so.  For example, if 
				 // "document" was requested and we are configured to run with
				 // a default extension of ".html" - look for "document.html" on disk.
			{
				const char *pzDefaultExtensions = GetProfile().GetString("HTTP","DefaultExtensions",false);
				if (pzDefaultExtensions)
				{
					GStringList types(",",pzDefaultExtensions);
					GStringIterator itTypes(&types);
					while (itTypes())
					{
						GString strNewFileName(strFileName);
						const char *pzAppendedExtension = itTypes++;
						strNewFileName += ".";
						strNewFileName += pzAppendedExtension;

						// now try to open the file with the default extension appended to the file name
						FILE *fp = fopen((const char *)strNewFileName,"rb");
						if (fp)
						{
							
							strFileName = strNewFileName;
							strExt = pzAppendedExtension;

							// get the size of the file
							fseek(fp,0,SEEK_END);
							lBytes = ftell(fp);
							fseek(fp,0,SEEK_SET);

							if (lBytes < 1024000 || DoesCacheHTTPContent(strExt,lBytes) ) 
							{
								// load the file content
								buf = new char[lBytes +1];
								if (fread(buf,sizeof(char),lBytes,fp) != (unsigned int)lBytes)
								{
									InfoLog(12,"Failed to read entire file\n");
									lBytes = 0;
								}
							}
							fclose(fp);
							break; // out of the "while (itTypes())"
						}
					}
				}
			}
		}
		if (lBytes)
		{
			//
			// Build the HTTP Headers
			//
			
			// if they only requested part of the file (2%)
			if (nBeginRange > -1 && nBeginRange < lBytes) 
			{
				
				int nContentBytes = -1;
				if ( nEndRange > 0 && nEndRange < lBytes )
				{
					// user set a valid end range
				}
				else
				{
					// we set it for the user at the last indexable byte
					nEndRange = lBytes - 1;
				}
				nContentBytes = (nEndRange + 1) - nBeginRange;


				BuildHTTPHeader(buf1, strFileName, "close" , 0, 0, strETag, strExt, lBytes, nBeginRange, nEndRange);
				int nHeaderLen = strlen(buf1);
				
		
				// open the file(already verified as valid/accessible) and obtain the piece requested
				if (nContentBytes > 0)
				{
					// send the header
					if ( nonblocksend(td->sockfd,buf1,nHeaderLen) == nHeaderLen)
						*nTotalProxiedBytes += nHeaderLen;


					// send the data
					char *pzBlockData = buf1;
					FILE *fp = fopen((const char *)strFileName,"rb");
					fseek(fp,nBeginRange,SEEK_SET);

					if (fp)
					{
						int nBytesToSend = nContentBytes;
						while (nBytesToSend > 0)
						{
							int nChunkSize = (nBytesToSend > MAX_RAW_CHUNK) ? MAX_RAW_CHUNK : nBytesToSend;
							if (fread(pzBlockData,sizeof(char),nChunkSize,fp) != (unsigned int)nChunkSize)
							{
								InfoLog(13,"Failed to read file chunk\n");
								lBytes = 0;
								break;
							}
							if (nonblocksend(td->sockfd,pzBlockData,nChunkSize) != nChunkSize)
							{
								break;
							}
							nBytesToSend -= nChunkSize;
							*nTotalProxiedBytes += nChunkSize;
						}
						fclose(fp);
					}
				}
			}
			else // they want the whole file (98%)
			{
				BuildHTTPHeader(buf1, strFileName, strConnectionType, nIsKeepAlive, nKeepAliveReturnValue, strETag, strExt, lBytes, -1, -1);
				int nHeaderLen = strlen(buf1);

				char *pzCacheData = 0;
				// Add Headers + data then send the whole thing in one write
				if (buf) 
				{
					pzCacheData = new char[lBytes + nHeaderLen + 1];
					strcpy(pzCacheData,buf1);
					if (pDiskCipherPass) // start past the "5Loaves" tag in the deciphered data stream
						memcpy(&pzCacheData[nHeaderLen],&buf[7],lBytes);
					else
						memcpy(&pzCacheData[nHeaderLen],buf,lBytes);

					// send it back (to the browser usually)
					if (nonblocksend(td->sockfd,pzCacheData,nHeaderLen+lBytes) == nHeaderLen+lBytes)
						*nTotalProxiedBytes += nHeaderLen+lBytes;

				}
				else // chunk up the data and send it back
				{
					
					// send the (headers and) data in the first chunk
					char *pzCacheData = buf2;
					FILE *fp = fopen((const char *)strFileName,"rb");
					if (fp)
					{
						int nBytesToSend = lBytes;
						int bIsFirstChunk = 1;
						int nHeaderOffset = 0;
						while (nBytesToSend > 0)
						{
							int nChunkSize = (nBytesToSend > MAX_RAW_CHUNK) ? MAX_RAW_CHUNK : nBytesToSend;
							if(bIsFirstChunk)
							{
								bIsFirstChunk = 0;
								nChunkSize -= nHeaderLen;
								memcpy(pzCacheData,buf1,nHeaderLen);
								nHeaderOffset = nHeaderLen;
							}
							else
							{
								nHeaderOffset = 0;
							}

							if (fread(pzCacheData + nHeaderOffset,sizeof(char),nChunkSize,fp) != (unsigned int)nChunkSize)
							{
								InfoLog(14,"Failed to read file chunk\n");
								lBytes = 0;
								break;
							}
							if (nonblocksend(td->sockfd,pzCacheData,nChunkSize+nHeaderOffset) != nChunkSize+nHeaderOffset)
							{
								break;
							}
							nBytesToSend -= nChunkSize;
							*nTotalProxiedBytes += nChunkSize;
						}
						fclose(fp);
					}
				}

				// if so configured, leave this complete HTTP response (headers + data) in the cache
				if (DoesCacheHTTPContent(strExt,lBytes))
				{
					cacheManager.EnterCacheData(nHeaderLen+lBytes,pzCacheData,(const char *)strFileName);
				}
				else
				{
					// otherwise free the memory and rebuild this message next time someone asks for it.
					delete pzCacheData;

					// todo: if we know in advance that we will NOT cache this AND know in advance that 
					// our system buffer is large enough to contain the data, then use the system buffer 
					// and do not make the allocation, and obviously do not free it here. == FASTER.
				}
			}
			if (buf)
			{
				delete buf; // the 'whole file' data buffer

				// todo:avoid the allocation of buf if the system buffer can be used and make sure not to free what we did not allocate.
				// This will require a new decryption API.  If decryptions are always cached, this unnecessary allocation will only happen 
				// on the first hit from disk, if so - this is very minor.
			}
		}
		else // we failed to find the file requested.
		{
			// Rather than sputtering out a 404, we auto-redirect them to an alternate page/site.
			const char *pz404Redirect = GetProfile().GetString("HTTP","PageRedirect",false);

			// this only applies to HTML pages (not images, javascript etc)
			const char *p = strExt;
			if (  
				  pz404Redirect 
				  && strlen(p) > 2 
				  && (p[0] == 'H' || p[0] == 'h') 
				  && (p[1] == 'T' || p[1] == 't') 
				  && (p[2] == 'M' || p[2] == 'm') 
			   )
			{

				g_Total404s++;	// HTTP 404 error count
				// log the "broken link" if so configured
				const char *pzLog = GetProfile().GetString("HTTP","404Log",false);
				if (pzLog)
				{
					strError.Format("File [%s] not found\n",(const char *)strFileName);
					strError.ToFileAppend(pzLog,0);
				}

				strError =  "HTTP/1.1 301 Moved Permanently\r\n";
				strError += "Location: ";
				strError += pz404Redirect;
				strError += "\r\n";

				strHTML.Format("<head><title>Document Moved</title></head><body><h1>Object Moved</h1>This document may be found <a HREF=\"%s\">here</a></body>",pz404Redirect);
			}
			else
			{
				
				// see if strFileName is a directory.  If the user supplied a trailing slash
				// we already knew that this was a directory and never got here - but since you
				// cannot differentiate between a file and a folder(unless a trailing slash is 
				// provided) we assumed it was a file - but didn't find this file.  Now, if this
				// turns out to be a folder, append the trailing slash and redirect the browser
				// back to the same location with the the trailing slash.
				if ( GDirectory::IsDirectory(ReSlash(strFileName)) )
				{
					// in the config file is a setting like this:
					// ThisSiteURL=http://www.UnitedBusinessTechnologies.com
					const char *pz404Redirect = GetProfile().GetString("HTTP","ThisSiteURL",false);
					if (pz404Redirect)
					{
						GString strRedirectTo(pz404Redirect);
						strRedirectTo += pzFilePartialPath; // add something like "/AtHome"
						strRedirectTo += "/";

						strError =  "HTTP/1.1 301 Moved Permanently\r\n";
						strError += "Location: ";
						strError += strRedirectTo;
						strError += "\r\n";

						strHTML.Format("<head><title>Document Moved</title></head><body><h1>Object Moved</h1>This document may be found <a HREF=\"%s\">here</a></body>",(const char *)strRedirectTo);
					}
				}
				else
				{
					// build a 404 Error
					strError = "HTTP/1.1 404 Not Found\r\n";
					strHTML.Format("<HTML><HEAD><TITLE>404 Not Found</TITLE></HEAD><BODY><H1>404 Not Found</H1>The requested URL <code>%s</code> was not found on this server.</BODY></HTML>",(const char *)pzFilePartialPath);

					g_Total404s++;	// HTTP 404 error count
					// log the "broken link" if so configured
					const char *pzLog = GetProfile().GetString("HTTP","404Log",false);
					if (pzLog)
					{
						//12=HTTP Content File [%s] requested but not found
						GString strLog;
						strLog.Format("File[%s] or Method[%s] Not found",(const char *)strFileName,(const char *)strFileNoPath);
						InfoLog(15,strLog);

					}
				}
			}

			int nRet = SendHTTPNoFileResponse(td->sockfd, strError, strHTML);
			if (nRet > 0)
				*nTotalProxiedBytes += nRet;

			// Setting nKeepAliveReturnValue = 0 will close the keep alive connection when the request could not be fulfilled
			// The browser (IE anyway) always closes it - but theoretically it does not have to and other browser may not
			// nKeepAliveReturnValue = 0;
		}
	}
	return nKeepAliveReturnValue;
}




void ProxyLog(const char *pzLogFile, int nThreadID, char *pData, int nBytes, const char *prefix )
{
	if ( pzLogFile && strlen(pzLogFile))
	{
		try
		{
			char pzTime[64];
			struct tm *newtime;
			time_t ltime;
			time(&ltime);
			newtime = gmtime(&ltime);
			strftime(pzTime, 64, "%H:%M:%S", newtime);
			
			// log prefix header
			GString strPrefix("\r\n\r\n");
			strPrefix.write(pzTime,8);
			strPrefix << " " << prefix;
			strPrefix << "-";
			char pzBytes[16];
			sprintf(pzBytes,"%06d",(int)nBytes);
			strPrefix.write(pzBytes,6);
			strPrefix << ">bytes\r\n";

			// log binary
			GString strBinary(strPrefix);
			strBinary.FormatBinary((unsigned char *)pData,(int)nBytes);
			strBinary.ToFileAppend(pzLogFile);
		}
		catch(...)
		{
		}
	}
}



int PrepForServer(unsigned short sAttribs, int bIsTunnel,CipherData *pcd, char *sockBuffer,int nDataSize,char *cryptDest, int *newLength)
{
	if (nDataSize > MAX_DATA_CHUNK)
	{
		InfoLog(16,"protocol error - packet too big");
		return 0;
	}
	

	int bCipher = ( sAttribs & ATTRIB_CRYPTED_2FISH ) ? 1 : 0;
	int bZip	= ( sAttribs & ATTRIB_COMPRESSED_BZIP ) ? 1 : 0;
	int gZip	= ( sAttribs & ATTRIB_COMPRESSED_GZIP ) ? 1 : 0;


	GString strMessage;
	int nBitsCrypted;
	if (bIsTunnel)
	{
		// Compress
		if (bZip | gZip)
		{
			// in the worst case (data is already compressed) the output
			// may actually expand a little (~500 bytes on a 65535 byte buffer)
			*newLength = MAX_DATA_CHUNK;
			
			if (nDataSize > 64)
			{
				// true indicates that the packet uses compression
				cryptDest[0] = 1;

				// done
				try{

				int cRet;
				if (gZip)
				{
				  #ifndef _NO_GZIP_
					cRet = GZipBuffToBuffCompress(
										&cryptDest[1], // destination
									   (unsigned int*)newLength,	
									   sockBuffer, // source
									   nDataSize					   );
				  #endif
				}
				else // bZip
				{
					 
					cRet = BZ2_bzBuffToBuffCompress( &cryptDest[1], // destination
									   (unsigned int*)newLength,	
									   sockBuffer, // source
									   nDataSize,
									   9, //blockSize100k, 
									   0,// verbosity, 
									   30);//workFactor )
				}




				if (cRet != 0)
				{
					GString strErr("Compress failed. point 1, code:");
					strErr << cRet;
					InfoLog(17,strErr);
					*newLength = 0;
					return 0;
				}
				else
				{
					*newLength += 1;
				}
				}catch(...)
				{
					GString strErr("Compress failed. point 2");
					InfoLog(18,strErr);
					*newLength = 0;
					return 0;
				}
			}
			else
			{
				// false indicates that the packet does not use compression, it's too small to benefit
				cryptDest[0] = 0;
				memcpy(&cryptDest[1],sockBuffer,nDataSize);
				*newLength = nDataSize + 1;
			}
			
			// hang onto running total for runtime statistical info
			g_PreZipBytes += nDataSize;
			g_PostZipBytes += *newLength;

			if (bCipher) // put it where the cipher input will pick it up
			{
				memcpy(sockBuffer,cryptDest,*newLength);
				nDataSize = *newLength;
			}

			if (g_DataTrace) DataTrace("After Compress", cryptDest, *newLength);
		}
		// Encrypt
		if (bCipher)
		{
			// "Encrypted - Sent to tunnel server:

			// bytes required to pad the last block to a 128 bit boundary
			unsigned char nPad = (16 - nDataSize % 16);
			nPad = (nPad == 16) ? 0 : nPad;

			// this is the size (in bytes) to encrypt.
			*newLength = nDataSize + nPad + 16;

			// create a trailer block of 128 bits
			// write the pad size in the first trailer byte 
			sockBuffer[nDataSize + nPad] = nPad;
			// null the rest of the last block
			memset(&sockBuffer[nDataSize + nPad+1],0,15);

			try
			{
				nBitsCrypted = pcd->blockEncrypt((unsigned char *)sockBuffer,(*newLength)*8,(unsigned char *)cryptDest);
			}
			catch(...)
			{
				return 0;
			}
			if (nBitsCrypted != (*newLength)*8)
			{
				return 0;
			}
			if (g_DataTrace) DataTrace("After Encrypt", cryptDest, *newLength);
		}
	}
	else
	{
		if (bCipher)
		{
			// Decrypted - Sent to Application server

			if (nDataSize*8 % BLOCK_SIZE)
			{
				InfoLog(19,"protocol error.");
				return 0;
			}

			try
			{
				nBitsCrypted = pcd->blockDecrypt((unsigned char *)sockBuffer,nDataSize*8,(unsigned char *)cryptDest);
			}
			catch(...) // this can happen when using a 128 key to undo 256 data
			{
				return 0;
			}
			if (nBitsCrypted != nDataSize*8)
			{
				return 0;
			}

			// get the first byte of the last 128 bit block
			unsigned char chPad = (unsigned char)cryptDest[(nBitsCrypted/8) - 16];
			// ASSERTION, the byte pad count must be less than a block size
			if (chPad > 16)
			{
				InfoLog(20,"Wrong Password.");
				return 0;
			}
			// and the new size removes the last block and padding bits added to the second to last block.
			*newLength = (nBitsCrypted/8) - 16 - chPad;
			if (g_DataTrace) DataTrace("After Decrypt", cryptDest, *newLength);
		}

		// Decompress
		if (bZip || gZip) 
		{

			if (bCipher) // move cipher output to compression input
			{
				memcpy(sockBuffer,cryptDest,*newLength);
				nDataSize = *newLength;
			}

			if ( sockBuffer[0] )
			{
				try{
				*newLength = MAX_DATA_CHUNK;
				int cRet;
				if (gZip)
				{
				  #ifndef _NO_GZIP_
					cRet = GZipBuffToBuffDecompress
										   ( cryptDest, // destination
											 (unsigned int *)newLength, // decompressed length
											 &sockBuffer[1], // source
											 nDataSize - 1); // source len
				  #endif
				}
				else // bZip
				{
					cRet = BZ2_bzBuffToBuffDecompress 
										   ( cryptDest, // destination
											 (unsigned int *)newLength, // decompressed length
											 &sockBuffer[1], // source
											 nDataSize - 1, // source len
											 1,// or 0 small,
											 0);//verbosity )
				}


				if (cRet != 0)
				{
					//14=Decompress failed.  Wrong password-key attempt?
					GString strLog;
					strLog.LoadResource(g_pzCoreErrorSection, 14);
					InfoLog(21,strLog);

					*newLength = 0;
					return 0;
				}
				}catch(...)
				{
					InfoLog(22,"Decompress failed - unknown error");
					*newLength = 0;
					return 0;
				}
			}
			else
			{
				memcpy(cryptDest,&sockBuffer[1],nDataSize-1);
				*newLength = nDataSize-1;
			}

			if (g_DataTrace) DataTrace("After Decompress", cryptDest, *newLength);
		}

	}

	// for packet sync support
	if (!bCipher && (!(bZip || gZip)))
	{
		memcpy(cryptDest,sockBuffer,nDataSize);
		*newLength = nDataSize;
	}

	return 1;
}


int PrepForClient(unsigned short sAttribs, int bIsTunnel,CipherData *pcd, char *sockBuffer,int nDataSize,char *cryptDest, int *newLength)
{
	 
	int bCipher = ( sAttribs & ATTRIB_CRYPTED_2FISH ) ? 1 : 0;
	int bZip	= ( sAttribs & ATTRIB_COMPRESSED_BZIP ) ? 1 : 0;
	int gZip	= ( sAttribs & ATTRIB_COMPRESSED_GZIP ) ? 1 : 0;

	
	if (bIsTunnel)
	{
		if (bCipher)
		{
			if (g_DataTrace) DataTrace("Before Decrypt", sockBuffer, nDataSize);
			// Decrypted - Sent to Client Application:
			if (nDataSize*8 % BLOCK_SIZE)
			{
				//15=protocol error - invalid block size[%d]
				GString strLog;
				strLog.LoadResource(g_pzCoreErrorSection, 15, nDataSize);
				InfoLog(23,strLog);
				return 0;
			}

			int nBitsCrypted;
			try
			{
				nBitsCrypted = pcd->blockDecrypt((unsigned char *)sockBuffer,nDataSize*8,(unsigned char *)cryptDest);
			}
			catch(...) // this can happen when using a 128 key to undo 256 data
			{
				return 0;
			}
			if (nBitsCrypted != nDataSize*8)
			{
				//16=Wrong password-key attempt?
				GString strLog;
				strLog.LoadResource(g_pzCoreErrorSection, 16);
				InfoLog(24,strLog);
				return 0;
			}
			// get the first byte of the last 128 bit block
			unsigned char chPad = (unsigned char)cryptDest[(nBitsCrypted/8) - 16];
			// the byte pad count must be less than a block size if the decrypt was a success
			if (chPad > 16)
			{
				//16=Wrong password-key attempt?
				GString strLog;
				strLog.LoadResource(g_pzCoreErrorSection, 16);
				InfoLog(25,strLog);
				return 0;
			}

			// and the new size removes the last block and padding bits added to the second to last block.
			*newLength = (nBitsCrypted/8) - 16 - chPad;
			if (g_DataTrace) DataTrace("After Decrypt", cryptDest, *newLength);
		}

		// Decompress
		if (bZip || gZip)
		{
			if (bCipher) // move cipher output to compression input
			{
				memcpy(sockBuffer,cryptDest,*newLength);
				nDataSize = *newLength;
			}

			if ( sockBuffer[0] ) // first byte is a flag indicating if this packet is compressed
			{
				try{
				*newLength = MAX_DATA_CHUNK;
				int cRet;
				if (gZip)
				{
				  #ifndef _NO_GZIP_
					 cRet = GZipBuffToBuffDecompress
										   ( cryptDest, // destination
											 (unsigned int *)newLength, // decompressed length
											 &sockBuffer[1], // source
											 nDataSize - 1); // source len
				  #endif
				}
				else // bZip
				{
						cRet = BZ2_bzBuffToBuffDecompress 
										   ( cryptDest, // destination
											 (unsigned int *)newLength, // decompressed len
											 &sockBuffer[1], // source
											 nDataSize - 1, // source len
											 1,// or 0 small,
											 0);//verbosity )
				}
				if (cRet != 0)
				{

					//14=Decompress failed.  Wrong password-key attempt?
					GString strLog;
					strLog.LoadResource(g_pzCoreErrorSection, 14);
					InfoLog(26,strLog);

					*newLength = 0;
					return 0;
				}
				}catch(...)
				{
					InfoLog(27,"Decompress failed - unknown error");
					*newLength = 0;
					return 0;
				}
			}
			else
			{
				memcpy(cryptDest,&sockBuffer[1],nDataSize-1);
				*newLength = nDataSize-1;
			}
			if (g_DataTrace) DataTrace("Decompressed", cryptDest, *newLength);
		}
	}
	else
	{
		// Compress
		if (bZip || gZip)
		{
			if (g_DataTrace) DataTrace("Before Compress", sockBuffer, nDataSize);

			*newLength = MAX_DATA_CHUNK;
			if (nDataSize > 64)
			{
				// true indicates that the packet uses compression
				cryptDest[0] = 1;

				try{
				int cRet;
				if (gZip)
				{
				  #ifndef _NO_GZIP_
					cRet = GZipBuffToBuffCompress(
										&cryptDest[1], // destination
									   (unsigned int*)newLength,	
									   sockBuffer, // source
									   nDataSize					   );
				  #endif
				}
				else // bZip
				{
					cRet = BZ2_bzBuffToBuffCompress( &cryptDest[1],
									   (unsigned int*)newLength,
									   sockBuffer, // raw data
									   nDataSize,
									   9, //blockSize100k, 
									   0,// verbosity, 
									   30);//workFactor )
				}

				if (cRet != 0)
				{
					GString strErr("Compression failed - code:");
					strErr << cRet;
					InfoLog(28,strErr);
					*newLength = 0;
				}
				else
				{
					*newLength += 1;

				}
				}catch(...)
				{
					InfoLog(29,"Compression failed - unknown error");
					*newLength = 0;
				}
				
			}
			else
			{
				// false indicates that the packet does not use compression
				cryptDest[0] = 0;
				memcpy(&cryptDest[1],sockBuffer,nDataSize);
				*newLength = nDataSize + 1;
			}
			
			// hang onto running total for statistical info
			g_PreZipBytes += nDataSize;
			g_PostZipBytes += *newLength;

			if (bCipher) // put it where the cipher input will pick it up
			{
				memcpy(sockBuffer,cryptDest,*newLength);
				nDataSize = *newLength;
			}
			if (g_DataTrace) DataTrace("After Compress", cryptDest, *newLength);

		}
		
		// Encrypt
		if (bCipher)
		{
			// "Encrypted - Sent to Client Tunnel:";
			// bytes required to pad the last block to a 128 bit boundary
			unsigned char nPad = (16 - nDataSize % 16);
			nPad = (nPad == 16) ? 0 : nPad;

			// this is the size (in bytes) to encrypt.
			*newLength = nDataSize + nPad + 16;

			// create a trailer block of 128 bits
			// write the pad size in the first trailer byte 
			sockBuffer[nDataSize + nPad] = nPad;
			// null the rest of the last block
			memset(&sockBuffer[nDataSize + nPad+1],0,15);
			
			int nBitsCrypted;
			try
			{
				nBitsCrypted = pcd->blockEncrypt((unsigned char *)sockBuffer,(*newLength)*8,(unsigned char *)cryptDest);
			}
			catch(...)
			{
				return 0;
			}
			if (nBitsCrypted != (*newLength)*8)
			{
				return 0;
			}
			if (g_DataTrace) DataTrace("After Crypt", cryptDest, *newLength);
		}
	}
	
	// for packet sync support
	if (!bCipher && (!(bZip || gZip)))
	{
		memcpy(cryptDest,sockBuffer,nDataSize);
		*newLength = nDataSize;
	}
	return 1;
}


// after calling ReadPacketHTTP(), this code:
// int nHTTPHeaderLen = pContent - pzHTTPHeader; 
// will get you the HTTP header len IN THE FIRST packet read.

int ReadPacketHTTP(int fd, int *pnBytesInBuf, char *Buf, unsigned int *nPktIndex, unsigned long *pnContentLen = 0, char **ppContent = 0, int nTimeOutSec = 300, int nTimeOutMicroSec = 0)
{
	Buf[*pnBytesInBuf] = 0; // purely for debugging asthetics

	int nHeaderLength = 0;
	int nBytes = 0;
	char *pContent = 0;

	//
	//  if we're reading the next chunk of content then *nPktIndex equals the bytes left of Content-Length to read 
	if (*nPktIndex > 0)
	{
		// int nBufIndex = 0; // read-ahead not supported in HTTP mode, start at the beginning of Buf
		int nRemaining = *nPktIndex;
		if( nRemaining  > MAX_RAW_CHUNK)
		{
			nRemaining = MAX_RAW_CHUNK;
		}

		int rslt = readableTimeout(fd, nTimeOutSec, nTimeOutMicroSec);
		// failure during wait || time limit expired
		if (rslt == -1 || rslt == 0)
		{
			if(g_TraceConnection && rslt == 0)
			{
				GString strErr;
				strErr.Format("ReadPacketHTTP Timed out waiting for data. (%d seconds) code %d  fd:%d",(int)nTimeOutSec,(int)SOCK_ERR,(int)fd);
				InfoLog(30,strErr);
			}
			if(g_TraceConnection && rslt == -1)
			{
		int nErrorCode = 0;
		int size = sizeof(nErrorCode);
#ifdef _WIN32
		getsockopt(fd, SOL_SOCKET, SO_ERROR, (char *) &nErrorCode, &size);
#else
		getsockopt(fd, SOL_SOCKET, SO_ERROR, (char *) &nErrorCode, (socklen_t *)&size);
#endif

				GString strErr;
				strErr.Format("*******ReadPacketHTTP Errored while waiting for data from fd:%d  code:%d SO_ERROR=%d",(int)fd,(int)SOCK_ERR,(int)nErrorCode);
				InfoLog(31,strErr);
			}
			// return rslt; was replaced with return -1 in September 2014
			return -1; 
		}



		int nRcvRetryCount = 0;
SUB_RCV_RETRY:
		nBytes = recv(fd, Buf, nRemaining, 0 ); // take what ever is available and return to caller
		if (nBytes == 0)
		{
			if(g_TraceConnection)
			{
				GString strErr("ReadPacketHTTP aborted reading HTTP Header - the connection was closed by the remote side.");
				InfoLog(32,strErr);
			}

			// this most likely means that the remote side of the connection closed the socket
			if(g_TraceConnection)
			{
				GString strErr;
				strErr.Format("????ReadPacketHTTP Errored while waiting for data. Code:%d  from fd:%d",(int)SOCK_ERR,(int)fd);
				InfoLog(33,strErr);
			}
			return -1; // In September 2014, ReadPacketHTTP() added this logic to return -1 when recv() returns 0.  
					   // previously the logic fell though, and returned 1. 

		}
		if (nBytes == -1)
		{
			int nErr = SOCK_ERR;
			// 10035 = Resource temporarily unavailable
			if (nErr == 10035)
			{
				goto SUB_RCV_RETRY; 
			}
			if(nRcvRetryCount++ < 3 && nErr != 10054 && nErr != 10038) 
			{
				PortableSleep(0,150000 * nRcvRetryCount);
				goto SUB_RCV_RETRY; 
			}

			return -1; 
		}
		*nPktIndex -= nBytes;
		*pnBytesInBuf += nBytes;
		Buf[*pnBytesInBuf] = 0; // purely for debugging asthetics
		return 1; // process the data
	}


	//
	// otherwise read the primary packet (sub packets are subsequent packet data )
	//
	*pnBytesInBuf = 0; // read-ahead not supported in HTTP mode, start at the beginning of Buf
	int nRcvRetryCount = 0;
	int rslt = readableTimeout(fd, nTimeOutSec, nTimeOutMicroSec);
	// failure during wait || time limit expired
	if (rslt == -1 || rslt == 0)
	{
		if(g_TraceConnection && rslt == 0)
		{
			GString strErr;
			strErr.Format("*******ReadPacketHTTP Timed out waiting for data. (%d seconds) code %d  fd=%d",(int)nTimeOutSec,(int)SOCK_ERR,(int)fd);
			InfoLog(34,strErr);
		}
		if(g_TraceConnection && rslt == -1)
		{

			GString strErr;  //Code 10038 
			strErr.Format("*******ReadPacketHTTP Errored while waiting for data. Code:%d  from fd:%d",(int)SOCK_ERR,(int)fd);
			InfoLog(35,strErr);
		}
		// return rslt; was replaced with return -1 in September 2014
		return -1; 
	}

	// enters a byte by byte read while bReadingHTTPHeader is true.  
	int bReadingHTTPHeader = 1;


	// honest HTTP headers are not 64k, RFC 2616 for HTTP 1.1 makes no specifications for a URL length
	// Microsoft Internet Explorer as of version 7, (in July 2007) supports a maximum URL length of 2,083
	// Opera's URL limit is 4,050, and Netscape 6 is 2000, if we pull in more than 4096 of HTTP header it's probably junk.
	while(bReadingHTTPHeader && (*pnBytesInBuf < MAX_RAW_CHUNK) && (*pnBytesInBuf < g_nMaxHTTPHeaderSize) )
	{
RCV_RETRY:
		nBytes = recv(fd, &Buf[*pnBytesInBuf], 1, 0 );	// read 1 byte
		if (nBytes < 1)
		{
			if(nBytes == 0)
			{
				if(g_TraceConnection)
				{
					Buf[*pnBytesInBuf] = 0;
					GString strErr("ReadPacketHTTP Aborted reading HTTP Header - connection closed by the remote side. recv() returns 0. fd=");
					strErr << fd << " SOCK_ERR= " << SOCK_ERR << " nBytes=" << nBytes << " *pnBytesInBuf=" << *pnBytesInBuf << " Data=" << Buf;
					InfoLog(36,strErr);

				}
				return -1;
			}

			// 10035 = Resource temporarily unavailable
			if (SOCK_ERR == 10035)
			{
				goto RCV_RETRY; 
			}
			if(nRcvRetryCount++ < 3 && SOCK_ERR != 10054 && SOCK_ERR != 10038)
			{
				PortableSleep(0,150000 * nRcvRetryCount);
				goto RCV_RETRY; 
			}
			// The client aborted, or timed out, or g_bPartialHeaderWait is OFF and 0 bytes was returned during the header read.
			if(g_TraceConnection) // 10053
			{
				Buf[*pnBytesInBuf] = 0;
				GString strErr("ReadPacketHTTP - connection closed by the remote side. recv() returns -1. fd=");
				strErr << fd << " SOCK_ERR= " << SOCK_ERR << " nBytes=" << nBytes << " *pnBytesInBuf=" << *pnBytesInBuf << " Data=" << Buf;
				InfoLog(37,strErr);
			}
			return -1; 
		}
		
		// look for the "\r\n\r\n" that terminates the HTTP headers
		if (Buf[*pnBytesInBuf] == 10 && *pnBytesInBuf > 3)
		{
			if ( Buf[(*pnBytesInBuf)-1] == 13 &&  Buf[(*pnBytesInBuf)-2] == 10 &&  Buf[(*pnBytesInBuf)-3] == 13)
			{
				bReadingHTTPHeader = 0; // we got the complete header, stop reading header bytes.
				pContent = &Buf[(*pnBytesInBuf) + 1];
				*pContent = 0;
				if (ppContent)
					*ppContent = pContent;
				nHeaderLength = (pContent) - Buf;
			}
		}
		*pnBytesInBuf += nBytes; // nBytes is always 1

		// The user supplied timeout was met before entering the read of this HTTP header,
		// Between the first byte of header and the last, duration of time between each byte may not exceed N seconds.
		// Generally the entire header is on the socket buffer if any of it is there at all, but extreme conditions do exist.
		// if g_bPartialHeaderWait is true - wait for the remainder of the TCP data, otherwise the connection will get dropped.
		if (bReadingHTTPHeader && g_bPartialHeaderWait)
		{
			// todo: check the I/O stats of buffered data size in the socket queue, then this call could be avoided for each byte of header data giving us more CPU to spend elsewhere.
			rslt = readableTimeout(fd, g_nPartialHeaderWaitSec, 0);
			if (rslt == -1 || rslt == 0)
			{
				if(g_TraceConnection && rslt == 0)
				{
					GString strErr;
					strErr.Format("ReadPacketHTTP Timed out waiting for data. (%d seconds) code %d",(int)nTimeOutSec,(int)SOCK_ERR);
					InfoLog(38,strErr);
				}
				if(g_TraceConnection && rslt == -1)
				{

					GString strErr;
					strErr.Format("???k ReadPacketHTTP Errored while waiting for data. Code:%d ",(int)SOCK_ERR);
					InfoLog(39,strErr);
				}
				// return rslt; was replaced with return -1 in September 2014
				return -1; 

			}
		}
	}
	
	if (bReadingHTTPHeader == 1) // if we bailed because we hit g_nMaxHTTPHeaderSize hang up the connection
	{
		if(g_TraceConnection && rslt == -1)
		{
			GString strErr;
			strErr.Format("ReadPacketHTTP is closing the connection because the client has sent[%d] bytes of HTTP header and we only allow[%d] bytes.",(int)*pnBytesInBuf , (int)g_nMaxHTTPHeaderSize);
			InfoLog(40,strErr);
		}
		
		return -1;
	}

	// get the content length
	int nCL = 0;
	char *pCL = stristr(Buf,"Content-Length:");
	if (pCL)
	{
		pCL += 15; // first byte of content length in the HTTP header
		nCL = atoi(pCL);
	}
	if(pnContentLen)
		*pnContentLen = nCL;

	// read the packet data content too
	if (nCL)
	{
		*nPktIndex = nCL;
		int nReadSize = nCL;
		if( nReadSize + *pnBytesInBuf > MAX_RAW_CHUNK)
		{
			nReadSize = MAX_RAW_CHUNK - *pnBytesInBuf;
		}
	

		// now read the remainder of the sockets buffered data
		rslt = readableTimeout(fd, nTimeOutSec, nTimeOutMicroSec);
		// failure during wait || time limit expired waiting for content data
		if (rslt == -1 || rslt == 0)
		{
			if(g_TraceConnection && rslt == 0)
			{
				GString strErr;
				strErr.Format("ReadPacketHTTP Timed out waiting for data. (%d seconds) code %d",(int)nTimeOutSec,(int)SOCK_ERR);
				InfoLog(41,strErr);
			}
			if(g_TraceConnection && rslt == -1)
			{
				GString strErr;
				strErr.Format("???9 ReadPacketHTTP Errored while waiting for data from fd:%d Code:%d",fd,(int)SOCK_ERR);
				InfoLog(42,strErr);
			}
			// return rslt; was replaced with return -1 in September 2014
			return -1; 
		}
		nRcvRetryCount = 0;
RETRY:
		nBytes = recv(fd, &Buf[*pnBytesInBuf], nReadSize, 0 );
		// now we have the entire header + nBytes of the content length - the total of which does not exceed MAX_RAW_CHUNK

		if (nBytes < 1)
		{
			if(nBytes == 0)
			{
				if(g_TraceConnection)
				{
					GString strErr("ReadPacketHTTP Aborted reading HTTP - the connection was closed by the remote side.");
					InfoLog(43,strErr);
				}
				return -1;
			}

			// 10035 = Resource temporarily unavailable
			int nErr = SOCK_ERR;
			if (nErr == 10035)
			{
				// gsched_yield();
				goto RETRY; 
			}
			if(nRcvRetryCount++ < 3 && nErr != 10054 && nErr != 10038) 
			{
				PortableSleep(0,150000 * nRcvRetryCount);
				goto RETRY; 
			}

			// The client aborted 
			return -1; 
		}
		*nPktIndex -= nBytes;
		*pnBytesInBuf += nBytes;
	}
	else
	{
		// we got the whole HTTP message that has no content length
		*nPktIndex = 0;
	}

	if (g_DataTrace) DataTrace("raw rx", Buf, *pnBytesInBuf);

	return 1; // process the data
}



// ReadPacket() notes:  
// --------------------
// When bIsHTTP is true then:
// Protocol is expected to be standard HTTP, bPacketSync should be false.
// The message can be in command form (GET / HTTP 1.1) or use a content 
// length that may span packets, as the packets returned by this fn max at MAX_DATA_CHUNK.
// ppPkt will always == Buf in this case, and nPktIndex will be bytes remaining if the
// HTTP content-length > MAX_DATA_CHUNK it will span one packet, it(nPktIndex) must be 
// supplied in subsequent calls to get the next data packet from the current HTTP message.

// When bPacketSync is true then:  --   bIsHTTP should be false
// Protocol =  #*DATA Packet 1 ... #*DATA Packet 2 ... #*DATA Packet 3
// where # is a 16 bit numeric byte count of the DATA packet that follows, 
// and * is a 16 bit flag field of data attributes (compression and encryption attributes)
// Buf may contain 1 packet, a partial packet, or multiple packets so (ppPkt) should be
// considered the result value for the size (nPktLen), and nPktIndex must be handed back for
// subsequent packet reads.

// When bPacketSync is false and bIsHTTP is false then:
// Data is read 'raw' with no transport markup expected - reading from a localhost application
// (POP3, SMTP, Telnet, or other).  Packets may or not retain the same length as the source 
// application.  This is used to transfer raw TCP data into a tunnel, or to bounce either
// raw or ciphered data across this machine, either way the packets will be re-aligned to
// the origin packet length at the client application(browser, telnet viewer, mail reader) 
// or at the tunnel exit point if the data is ciphered.
// ------------
// Returns: 0=No data nothing to do,   1=Packet has arrived - output params are set   or   -1 network/connection error
int ReadPacket(int fd, int nTimeOutSec, int nTimeOutMicro, int *pnBytesInBuf,char *Buf,char **ppPkt,int *nPktLen, int *nPktIndex, unsigned short *psAttrFlags, unsigned short sAttribsExpected, int bPacketSync, int bIsHTTP)
{
READ_PACKET_BEGIN:
	// if the beginning of the next packet was in the last recv()
	if (*nPktIndex > -1 && !bIsHTTP)
	{
		unsigned short theNumber = 0;				// storage on platform architecture byte boundry
		memcpy(&theNumber,&Buf[*nPktIndex],2);		// storage in network byte order moved to architecture byte boundry storage 
		unsigned short nNextPktSize = ntohs( theNumber );    // convert to host byte order

		memcpy(&theNumber,&Buf[(*nPktIndex) + sizeof(short)],2); // the same thing here.
		*psAttrFlags = ntohs( theNumber );



		if (nNextPktSize < 0)
		{
			GString strLog = "*******Invalid packet size (misconfigured?)";
			InfoLog(44,strLog);
			return -1;
		}

		if (*psAttrFlags != sAttribsExpected)
		{
			// within the second block of 16 bits recv()'d, If any of the reserved protocol 
			// bits (bits 5-16) are on or all bits(1-16) are off, this is raw(non-packeted) data.
			// This machine is expecting packeted data.  
			if ( ( *psAttrFlags & ATTRIB_UNUSED_BIT9 ) || ( *psAttrFlags & ATTRIB_UNUSED_BIT10) ||
				 ( *psAttrFlags & ATTRIB_UNUSED_BIT11) || ( *psAttrFlags & ATTRIB_UNUSED_BIT12) ||
				 ( *psAttrFlags & ATTRIB_UNUSED_BIT13) || ( *psAttrFlags & ATTRIB_UNUSED_BIT14) ||
				 ( *psAttrFlags & ATTRIB_UNUSED_BIT15) || ( *psAttrFlags & ATTRIB_UNUSED_BIT16) ||
				 *psAttrFlags == 0) //there is always > 0 attributes when synchronized
			{
				// 17=Protocol error. If you are attempting to proxy or 'bounce' TCP, enable RawPacketProxy=yes in the correct subsection of the config file.
				GString strLog;
				strLog.LoadResource(g_pzCoreErrorSection, 17);
				InfoLog(45,strLog);
				return -1;
			}
		}

		int nBufBytesNeeded = *nPktIndex + nNextPktSize + (2 * sizeof(short));
		// If the next packet is the complete remainder of the recv() buffer
		if (nBufBytesNeeded == *pnBytesInBuf)
		{
			*ppPkt = &Buf[*nPktIndex + (2 * sizeof(short))]; // 4 bytes of header, then data
			*nPktLen = nNextPktSize;
			*nPktIndex = -1; // last packet
			return 1;
		}
		// if the next packet is in the recv() buffer with more data after it
		else if (nBufBytesNeeded < *pnBytesInBuf)
		{
			*ppPkt = &Buf[*nPktIndex + (2 * sizeof(short))]; // 4 bytes of header, then data
			*nPktLen = nNextPktSize;
			*nPktIndex = nBufBytesNeeded; // index to the first byte of the next packet
			return 1;
		}
		// otherwise, we need to recv() the remainder of this packet
		else 
		{
			int rslt = readableTimeout(fd, nTimeOutSec, nTimeOutMicro);
			if ( rslt > 0 )
			{
				// slide the data to the front of the buffer to make room for this packet
				if (*nPktIndex > 0)
				{
					memmove(Buf,&Buf[*nPktIndex],*pnBytesInBuf - *nPktIndex); // ANSI <string.h>
					*pnBytesInBuf = *pnBytesInBuf - *nPktIndex;
					*nPktIndex = 0;
				}

				// test the buffer for necessary free space now
				if (MAX_DATA_CHUNK - *pnBytesInBuf < 1)
				{
					// 18=ReadPacket buffer overrun.
					GString strLog;
					strLog.LoadResource(g_pzCoreErrorSection, 18);
					InfoLog(46,strLog);

					return -1;
				}


				int nReRecvRetry;
				nReRecvRetry = 0;
RE_RECV:
				// read data from the server
				int nBytes = recv(fd, &Buf[*pnBytesInBuf], MAX_DATA_CHUNK - *pnBytesInBuf, 0 );

				if (nBytes > 0)
				{
					char *pNewDataBegin = &Buf[*pnBytesInBuf];
					*pnBytesInBuf = *pnBytesInBuf + nBytes;
					if (g_DataTrace) 
					{
						DataTrace("raw rx", pNewDataBegin, nBytes);
					}
					
					goto READ_PACKET_BEGIN;
				}
				else if (nBytes == -1)
				{
					int nErr = SOCK_ERR;

					if (nErr == 10035)
					{
						gsched_yield();
						goto RE_RECV;
					}
					else if(nReRecvRetry++ < 3 && nErr != 10054 && nErr != 10038) 
					{
						PortableSleep(0,150000 * nReRecvRetry);
						goto RE_RECV;
					}

					// recv() failed (connection closed by server)
					GString strErr("*******recv() stopped.  nBytesInBuf=");
					strErr << *pnBytesInBuf << "  *nPktIndex=" << *nPktIndex << "  bIsHTTP=" << bIsHTTP << "  nBufBytesNeeded=" << nBufBytesNeeded << "  bPacketSync=" << bPacketSync;
					InfoLog(47,strErr);

					return -1;
				}
				else if (nBytes == 0)
				{
					// the connection was closed by the remote side. but it's odd we got 0 bytes rather than an error.
					GString strErr("*******recv() quit.  nBytesInBuf=");
					strErr << *pnBytesInBuf << "  *nPktIndex=" << *nPktIndex << "  bIsHTTP=" << bIsHTTP << "  nBufBytesNeeded=" << nBufBytesNeeded << "  bPacketSync=" << bPacketSync;
					InfoLog(48,strErr);

					// no retry here
					return -1;
				}

			}
			else if (rslt == -1) // rslt from readableTimeout()
			{
				GString strErr("*******readableTimeout() quit.  nBytesInBuf=");
				strErr << *pnBytesInBuf << "  *nPktIndex=" << *nPktIndex << "  bIsHTTP=" << bIsHTTP << "  nBufBytesNeeded=" << nBufBytesNeeded << "  bPacketSync=" << bPacketSync;
				InfoLog(49,strErr);

				return -1;
			}

			// no data or only a partial packet available - nothing to do because readableTimeout() timed out.
			return 0;
		}
	}
	else
	{
		int rslt = readableTimeout(fd, nTimeOutSec, nTimeOutMicro );
		
		// if there is data on the port ready to read.
		if ( rslt > 0 )
		{
			if (bIsHTTP)
			{
				if (*nPktIndex == -1)
				{
					*nPktIndex = 0; // initial value for the first packet read.
				}

				// Buf will contain the HTTP packet
				int nBytes  = 0;
				*pnBytesInBuf = nBytes; // no preread bytes exist in the destination buffer
				*ppPkt = Buf;
				
				int nRet = ReadPacketHTTP(fd, pnBytesInBuf, Buf, (unsigned int *)nPktIndex, 0, 0, nTimeOutSec, nTimeOutMicro);
				*nPktLen = *pnBytesInBuf; // HTTP reads: nPktLen always == nBytesInBuf


				return nRet; // SOCK_ERR=183, 10054, 10014
			}


			// read data from the server (tunnel server, or app server depending on bPacketSync)
			int nMaxRead = (bPacketSync) ? MAX_DATA_CHUNK : MAX_RAW_CHUNK;

			// read in some data, 4 bytes at a minimum(if bPacketSync) to obtain attributes(cipher/zip...)
			int nMinRead = (bPacketSync) ? 4 : 1;
			int nRetryOnErrCount = 0;
			int nBufi = 0;
RETRY_ON_ERR: 
			int nBytes = recv(fd, &Buf[nBufi], nMaxRead, 0 );
			if (nBytes > 0)
			{
				if (nBytes < nMinRead) // if we got 1,2 or 3 bytes but we need a min of 4
				{
					// this will most likely never happen, but in theory it could
					nMinRead -= nBytes;
					nBufi += nBytes;
					*pnBytesInBuf += nBytes;
					readableTimeout( fd, nTimeOutSec, nTimeOutMicro );
					goto RETRY_ON_ERR;
				}
				*pnBytesInBuf = nBytes;
			}
			else if (nBytes == -1)
			{
				// 10035 = Resource temporarily unavailable
				if (SOCK_ERR == 10035)
				{
					goto RETRY_ON_ERR; 
				}
				
				// this says, "if we are NOT going to retry the error"
				if (nRetryOnErrCount++ > 3 || SOCK_ERR == 10054 || SOCK_ERR == 10038 )
				{
					return -1;
				}
				else // else we will retry
				{
					PortableSleep(0,100000 * nRetryOnErrCount);
					goto RETRY_ON_ERR;
				}
			}
			else if (nBytes == 0) 
			{
				// added September 2014	to proxy the last of our recv() buffer on error				
				if (*pnBytesInBuf > 0 && !bIsHTTP && !bPacketSync) // if this is a raw data proxy
				{
					*ppPkt = Buf;
					*nPktLen = *pnBytesInBuf;
					*psAttrFlags = 0;	// no attributes - raw data
					return 1; // process the data now
				}
				int nErrorCode = 0;
				int size = sizeof(nErrorCode);
				#ifdef _WIN32
						getsockopt(fd, SOL_SOCKET, SO_ERROR, (char *) &nErrorCode, &size);
				#else
						getsockopt(fd, SOL_SOCKET, SO_ERROR, (char *) &nErrorCode, (socklen_t *)&size);
				#endif

				GString strErr("*******recv() quit.");
				strErr << "from fd:" << fd << " bytes on-hand:" << *pnBytesInBuf << "  *nPktIndex=" << *nPktIndex << "  bIsHTTP=" << bIsHTTP << "  bPacketSync=" << bPacketSync  << " SOCK_ERR=" << SOCK_ERR << "  SO_ERROR=" << nErrorCode;
				InfoLog(50,strErr);

				return -1;
			}


			// if ciphered/zipped proxy, we may have > 1 packet in Buf, or < 1 full packet - go remedy that
			if (bPacketSync) 
			{
				// a packet from the tunnel server / tunnel client has the protocol headers
				*nPktIndex = 0;
				goto READ_PACKET_BEGIN;
			}
			else // raw data proxy - we'll take whatever we got and use that.  It's usually
				 // a raw packet from an application server (pop3 server, telnet server etc)
			{
				if (g_DataTrace) DataTrace("raw rx", Buf, nBytes);
				*ppPkt = Buf;
				*nPktLen = *pnBytesInBuf;
				*psAttrFlags = 0;	// no attributes - raw data
				return 1; // process the data now
			}
		}
		else if (rslt == -1) //  -1 is an error from readableTimeout()
		{
			// added September 2014					
			if (*pnBytesInBuf > 0 && !bIsHTTP && !bPacketSync) // if this is a raw data proxy
			{
				*ppPkt = Buf;
				*nPktLen = *pnBytesInBuf;
				*psAttrFlags = 0;	// no attributes - raw data
				return 1; // process the data now
			}

			int nErrorCode = 0;
			int size = sizeof(nErrorCode);
			#ifdef _WIN32
					getsockopt(fd, SOL_SOCKET, SO_ERROR, (char *) &nErrorCode, &size);
			#else
					getsockopt(fd, SOL_SOCKET, SO_ERROR, (char *) &nErrorCode, (socklen_t *)&size);
			#endif

			GString strErr("*******readableTimeout() quit. ");
			strErr << "fd:" << fd << " Bytes=" << *pnBytesInBuf << "  *nPktIndex=" << *nPktIndex << "  bIsHTTP=" << bIsHTTP << "  bPacketSync=" << bPacketSync  << " SOCK_ERR=" << SOCK_ERR << "  SO_ERROR=" << nErrorCode;
			InfoLog(51,strErr);

			// failure during wait (connection closed by server)
			return -1;
		}

		// no data or only a partial packet available - nothing to do.
		return 0;
	}
}

 
unsigned short BuildAttributesHeader(const char *pzCfgSection, int nProtocol, int nProtocolAction, int *bReadHTTP, int nDirection)
{
	unsigned short sRet = 0;
	*bReadHTTP = 0;
	if (nProtocol == 3)
	{
		return sRet; // 0 is a raw proxy
	}
	if (nProtocol == 9)
	{
		sRet |= ATTRIB_CRYPTED_2FISH;
		sRet |= ATTRIB_COMPRESSED_GZIP;
		return sRet; 
	}


	if ( GetProfile().GetBoolean(pzCfgSection,"UseHTTP",false) )
	{
		*bReadHTTP = 1;
	}

	if ( GetProfile().GetBoolean(pzCfgSection,"CipherPass",false) )
	{
		sRet |= ATTRIB_CRYPTED_2FISH;
	}
	if ( GetProfile().GetBoolean(pzCfgSection,"CompressEnabled",false) )
	{
		sRet |= ATTRIB_COMPRESSED_GZIP;
	}
	if ( GetProfile().GetBoolean(pzCfgSection,"PacketSyncEnabled",false) )
	{
		sRet |= ATTRIB_SYNCRONIZED;
	}

	// if no attributes were specified and a raw(non-synchronized) was not specified.
	if (!sRet && !GetProfile().GetBoolean(pzCfgSection,"RawPacketProxy",false))
	{
		sRet |= ATTRIB_SYNCRONIZED;
	}
	
	// remove the ATTRIB_SYNCRONIZED for raw HTTP proxy for messaging
	if (nProtocol == 1)
	{
		sRet = 0;
	}
	
	// Dynamic HTTP Proxy
	if (nProtocol == 11)
	{
		*bReadHTTP = 1;
		sRet = 0;
	}

	// OpenSurf
	if (nProtocol == 12)
	{
		if (nDirection == 1)  // The machine that connected to this machine is sending through this machine
		{
			*bReadHTTP = 1;
			sRet |= ATTRIB_CRYPTED_2FISH;
		}
		else // the machine connected to by this machine is sending back to the initiating machine through this machine
		{
			sRet = 0; 
		}
	}
	return sRet;
}



char junkBuffer[MAX_DATA_CHUNK];
int	DoConnect(const char *pzConnectTo, int nPort, int nFromID, int nToID)
{
	if (!pzConnectTo || !pzConnectTo[0])
	{
		if(g_TraceConnection)
		{
			GString strInfo;
			strInfo.Format("*******FAILED Outbound[%d:(null)]---(tid:%d<->%d)", (int)nPort,(int)nFromID, (int)nToID);
			InfoLog(52,strInfo);
		}
		return -1;
	}
	
	// if the first byte of pzConnectTo is a '~' - make an inbound-connect.  
	// wait for 2 minutes for the named connection to be posted in (connection opened by the destination).
	if (pzConnectTo[0] == '~')
	{
		GString strInternalConnect(&pzConnectTo[1]);
		strInternalConnect.MakeUpper();
		// returns -1 or the socket handle
		return g_SwitchBoard.ConnectToServer( strInternalConnect );
	}

	int fd = -1;

	
	//
	// Note: The GHash uses a key of "Port@www.domain.com" directly to the socket handle. 
	//       When we hash a key and get a value, subtract 1 to make it a socket handle because
	//       0 is a valid socket handle which will come back as 1 if it is found and 0 if it is not.
	//       This is why we add 1 when putting it in and subtract 1 when taking it out.
	//
	if ( g_bEnableOutboundConnectCache )
	{
		GString gKey;
		gKey << nPort << "@"  << pzConnectTo;
		gKey.MakeLower();
NEXT_IN_CACHE:
		gthread_mutex_lock(&g_csMtxConnectCache);

#if defined( _LINUX64) || defined (_IOS) || defined(_WIN64)
		int fdCached = (int)(__int64)g_OutboundConnectCache.RemoveKey(gKey);
#else
		int fdCached = (int)g_OutboundConnectCache.RemoveKey(gKey);
#endif
		gthread_mutex_unlock(&g_csMtxConnectCache);
		if (fdCached)
		{
			fd = fdCached - 1; // return to the actual socket handle value which may be 0 or >

			int error = 0;
			socklen_t len = sizeof (error);
			if ( getsockopt (fd, SOL_SOCKET, SO_ERROR, (char *)&error, &len ) != 0)
			{
				PortableClose(fd,"ExpiredInCacheA");
				goto NEXT_IN_CACHE;
			}

			int rslt = readableTimeout(fd, 0, 7);
			// -1 == failure during wait   0 == the 7 microsecond time limit expired, 1 there is data
			if (rslt == -1 )
			{
				PortableClose(fd,"ExpiredInCacheB");
				goto NEXT_IN_CACHE;
			}
			if ( rslt == 1 )
			{
				// there is junk on the port that was intended for the last client that used this connection - but that client must have closed before getting this data
				int nJunk = nonblockrecvAny ( fd, junkBuffer, MAX_DATA_CHUNK, 1 ); 
				if (nJunk == -1)
				{
					PortableClose(fd,"ExpiredInCacheC");
					goto NEXT_IN_CACHE;
				}

				if (g_bHTTPProxyLogExtra)
				{
					GString g;
					junkBuffer[nJunk] = 0;
					g << "Fresh fd from Cache:" << fd << " had data in queue - throw it away[" << junkBuffer << "]";
					g.ReplaceChars("\r\n\r\n",'~');
					InfoLog(123432,g);  
				}
			}


			// getsockopt() returned 0.  According to SO_ERROR the socket is good, however it COULD still have been silently disconnected

			if(g_TraceSocketHandles || g_TraceConnection)
			{
				GString strInfo;
				strInfo.Format("CACHED Connected Out[%d@%s]  fd:%d  (tid:%d<->%d)", (int)nPort,pzConnectTo, (int)fd,(int)nFromID, (int)nToID);
				InfoLog(57,strInfo);
			}
			return fd;
		}
	}

	if(g_TraceConnection)
	{
		GString strInfo;
		strInfo.Format("Try Outbound [%d@%s]---(tid:%d<->%d)", (int)nPort,pzConnectTo, (int)nFromID, (int)nToID);
		InfoLog(53,strInfo);
	}

	// else make an outbound-connect....
	struct sockaddr_in their_addr; 
	their_addr.sin_family = AF_INET;
	their_addr.sin_port = htons(nPort);
	their_addr.sin_addr.s_addr = inet_addr (pzConnectTo);
	if (their_addr.sin_addr.s_addr == -1)
	{
		int nDNSResolutionRetries = 0;
DNSResolutionRETRY:
		// resolve a DNS server name if inet_addr() came up empty.
		struct hostent *pHE = (struct hostent *)gethostbyname(pzConnectTo);
		if (pHE == 0)
		{ 
			if (nDNSResolutionRetries++ < 3)
			{
				PortableSleep(0,250000);
				goto DNSResolutionRETRY;
			}
			// Info:Failed to resolve[%s].
			if(g_TraceConnection)
			{
				GString strInfo;
				strInfo.Format("*******FAILED to gethostbyname(%s): (tid:%d<->%d)",pzConnectTo, (int)nFromID, (int)nToID);
				InfoLog(54,strInfo);
			}

			// we failed to resolve, possibly not due to DNS error but due to the inability to obtain a network connection, likely on a mobile device that is 'sleeping' 
			goto NO_CONNECT;
		}
		memcpy((char *)&(their_addr.sin_addr), pHE->h_addr,pHE->h_length); 
	}
	memset(&(their_addr.sin_zero),0, 8);//zero the rest of the (unused) struct

	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
	{	
		if(g_TraceConnection)
		{
			GString strInfo;
			strInfo.Format("*******FAILED: Outbound[%d:%s]---(tid:%d<->%d)  NO SOCKET", (int)nPort,pzConnectTo, (int)nFromID, (int)nToID);
			InfoLog(55,strInfo);
		}
		goto NO_CONNECT;
	}
int nReConnectTries;
nReConnectTries = 0;
RE_CONNECT:
	if (connect(fd, (struct sockaddr *)&their_addr, sizeof(struct sockaddr)) != 0)
	{
		if(g_TraceConnection)
		{
			GString strInfo;
			if (nReConnectTries == 3)
				strInfo.Format("*******FAILED: Outbound[%d:%s]---(tid:%d<->%d)  NO CONNECT", (int)nPort,pzConnectTo, (int)nFromID, (int)nToID);
			else
				strInfo.Format("*******RE-TRY%d: Outbound[%d:%s]---(tid:%d<->%d)  NO CONNECT",nReConnectTries+1, (int)nPort,pzConnectTo, (int)nFromID, (int)nToID);
			InfoLog(56,strInfo);
		}

		if (nReConnectTries++ < 3)
		{
			PortableSleep(0,250000 * (nReConnectTries + 1));  // the first retry in in 1/4 of a second (at 250,000 microcesonds) and the duration to the next retry is twice that
			goto RE_CONNECT;
		}

		PortableClose(fd,"Core6");
		fd = -1;

		goto NO_CONNECT;
	}

	AddDebugSocket(fd);

	if(g_TraceSocketHandles || g_TraceConnection)
	{
		GString strInfo;
		strInfo.Format("Connected Out[%d@%s]  fd:%d  (tid:%d<->%d)", (int)nPort,pzConnectTo, (int)fd,(int)nFromID, (int)nToID);
		InfoLog(57,strInfo);
	}

NO_CONNECT:
	return fd;
}





////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////  Begin NTLM Code ////////////////////////////////////////////

// The specification is a bit scattered, and the implementation example is tightly coupled 
// with application structure - so this is largely reverse-engineered by testing with 
// the NTLMv2 implementation in Firefox, Safari, and IE.  It was better to be late than early 
// to this technology, because this is a clean implementation tested with all 3 clients.
// ----------  
// http://www.opensource.apple.com/source/CFNetwork/CFNetwork-128/HTTP/NTLM/NtlmGenerator.cpp
// http://www.innovation.ch/personal/ronald/ntlm.html
// vs
// https://developer.gnome.org/evolution-exchange/stable/ximian-connector-ntlm.html
// ----------
// see this for SPNEGO:
// see: http://tools.ietf.org/html/rfc4559    
// http://msdn.microsoft.com/en-us/library/ms995331.aspx
// ----------
// Chrome has historically been NTLM challenged with a handful of versions working with NTLMv2 and a handful that dont.
// http://code.google.com/p/chromium/issues/detail?id=14206
// http://stackoverflow.com/questions/8616818/integrated-windows-auth-ntlm-on-a-mac-using-google-chrome-or-safari
// ----------
struct TypeMessageSwitch
{
	char    protocol[8];     // 'N', 'T', 'L', 'M', 'S', 'S', 'P', '\0'
	unsigned long  type;             // 0x01
};

struct Type1Message
{
	char    protocol[8];     // 'N', 'T', 'L', 'M', 'S', 'S', 'P', '\0'
	unsigned long  type;             // 0x01
	unsigned long  flags;

	unsigned short domain_len;		 // domain string length
	unsigned short domain_len_alloc; // domain string length
    unsigned long  domain_off;       // domain string offset

	unsigned short host_len;		// host string length
    unsigned short host_len_alloc;  // host string length
    unsigned long  host_off;		// host string offset (always 0x20)

    char    Message[512];			// host string (ASCII) then domain string (ASCII) 
	Type1Message()
	{
		memset(&protocol[0],0,sizeof(Type1Message));
	}
};


void ShowAuthenticateFlags(unsigned long flags, GString &strDebugFlags)
{
	if ( (flags & 0x00000001) != 0 )
		strDebugFlags << "NegotiateUnicode  ";	// Unicode
	if ( (flags & 0x00000002) != 0 )
		strDebugFlags << "NegotiateASCII  ";	// OEM
	if ( (flags & 0x00000004) != 0 )
		strDebugFlags << "RequestTarget   ";	// If set, a TargetName field of the CHALLENGE_MESSAGE (section 2.2.1.2) MUST be supplied
	if ( (flags & 0x00000008) != 0 )
		strDebugFlags << "0x00000008     ";		// not used should be 0
	if ( (flags & 0x00000010) != 0 )
		strDebugFlags << "NegotiateSign  ";		// If set, requests session key negotiation for message signatures. If the client sends NTLMSSP_NEGOTIATE_SIGN to the server in the NEGOTIATE_MESSAGE, the server MUST return NTLMSSP_NEGOTIATE_SIGN to the client in the CHALLENGE_MESSAGE
	if ( (flags & 0x00000020) != 0 )
		strDebugFlags << "NegotiateSeal  ";		// If set, requests session key negotiation for message confidentiality. If the client sends NTLMSSP_NEGOTIATE_SEAL to the server in the NEGOTIATE_MESSAGE, the server MUST return NTLMSSP_NEGOTIATE_SEAL to the client in the CHALLENGE_MESSAGE. Clients and servers that set NTLMSSP_NEGOTIATE_SEAL SHOULD always set NTLMSSP_NEGOTIATE_56 and NTLMSSP_NEGOTIATE_128, if they are supported.
	if ( (flags & 0x00000040) != 0 )
		strDebugFlags << "NegotiateDataGram   ";// If set, requests connectionless authentication
	if ( (flags & 0x00000080) != 0 )
		strDebugFlags << "NegotiateLMKey     ";	// If set, requests LAN Manager (LM) session key computation
	if ( (flags & 0x00000100) != 0 )
		strDebugFlags << "0x00000100   ";		// not used should be 0
	if ( (flags & 0x00000200) != 0 )
		strDebugFlags << "NegotiateNTLM   ";	// If set, requests usage of the NTLM v1 session security protocol. NTLMSSP_NEGOTIATE_NTLM MUST be set in the NEGOTIATE_MESSAGE to the server and the CHALLENGE_MESSAGE to the client.
	if ( (flags & 0x00000400) != 0 )
		strDebugFlags << "0x00000400   ";		// not used should be 0
	if ( (flags & 0x00000800) != 0 )
		strDebugFlags << "0x00000800";			// If set, the connection SHOULD be anonymous
	if ( (flags & 0x00001000) != 0 )
		strDebugFlags << "NegotiateDomainSupplied   ";  // If set, the domain name is provided  
	if ( (flags & 0x00002000) != 0 )
		strDebugFlags << "NegotiateWorkstationSupplied   ";   // This flag indicates whether the Workstation field is present. If this flag is not set, the Workstation field MUST be ignored.
	if ( (flags & 0x00004000) != 0 )
		strDebugFlags << "NegotiateLocalCall   "; 
	if ( (flags & 0x00008000) != 0 )
		strDebugFlags << "NegotiateAlwaysSign   "; // If set, requests the presence of a signature block on all messages. NTLMSSP_NEGOTIATE_ALWAYS_SIGN MUST be set in the NEGOTIATE_MESSAGE to the server and the CHALLENGE_MESSAGE to the client. NTLMSSP_NEGOTIATE_ALWAYS_SIGN is overridden by NTLMSSP_NEGOTIATE_SIGN and NTLMSSP_NEGOTIATE_SEAL, if they are supported.
	if ( (flags & 0x00010000) != 0 )
		strDebugFlags << "TargetTypeDomain   "; //  If set, TargetName MUST be a domain name. The data corresponding to this flag is provided by the server in the TargetName field of the CHALLENGE_MESSAGE. If set, then NTLMSSP_TARGET_TYPE_SERVER MUST NOT be set.
	if ( (flags & 0x00020000) != 0 )
		strDebugFlags << "TargetTypeServer   "; // If set, TargetName MUST be a server name. The data corresponding to this flag is provided by the server in the TargetName field of the CHALLENGE_MESSAGE. If this bit is set, then NTLMSSP_TARGET_TYPE_DOMAIN MUST NOT be set. This flag MUST be ignored in the NEGOTIATE_MESSAGE and the AUTHENTICATE_MESSAGE.
	if ( (flags & 0x00040000) != 0 )
		strDebugFlags << "TargetTypeShare";		
	if ( (flags & 0x00080000) != 0 )
		strDebugFlags << "NegotiateNTLMv2   ";	// If set, requests usage of the NTLM v2 session security.
	if ( (flags & 0x00100000) != 0 )
		strDebugFlags << "0x00100000   ";		// If set, requests an identify level token. An alternate name for this field is NTLMSSP_NEGOTIATE_IDENTIFY
	if ( (flags & 0x00200000) != 0 )
		strDebugFlags << "0x00200000   ";		// not used must be 0     or  RequestAcceptResponse
	if ( (flags & 0x00400000) != 0 )
		strDebugFlags << "0x00400000   ";		// RequestNonNTSessionKey   - If set, requests the usage of the LMOWF
	if ( (flags & 0x00800000) != 0 )
		strDebugFlags << "NegotiateTargetInfo   "; // If set, indicates that the TargetInfo fields in the CHALLENGE_MESSAGE (section 2.2.1.2) are populated
	if ( (flags & 0x01000000) != 0 )
		strDebugFlags << "0x01000000   ";		// not used must be 0     or  Requests identify level token
	if ( (flags & 0x02000000) != 0 )
		strDebugFlags << "NegotiateVersion   "; // Version field is present. If set, requests the protocol version number. The data corresponding to this flag is provided in the Version field of the NEGOTIATE_MESSAGE, the CHALLENGE_MESSAGE, and the AUTHENTICATE_MESSAGE
	if ( (flags & 0x04000000) != 0 )
		strDebugFlags << "0x04000000   ";		// not used must be 0
	if ( (flags & 0x08000000) != 0 )
		strDebugFlags << "0x08000000   ";		// not used must be 0
	if ( (flags & 0x10000000) != 0 )
		strDebugFlags << "0x10000000   ";		// not used must be 0
	if ( (flags & 0x20000000) != 0 )
		strDebugFlags << "Negotiate128   ";		// If set, requests 128-bit session key negotiation. An alternate name for this field is NTLMSSP_NEGOTIATE_128. If the client sends NTLMSSP_NEGOTIATE_128 to the server in the NEGOTIATE_MESSAGE, the server MUST return NTLMSSP_NEGOTIATE_128 to the client in the CHALLENGE_MESSAGE only if the client sets NTLMSSP_NEGOTIATE_SEAL or NTLMSSP_NEGOTIATE_SIGN. Otherwise it is ignored. If both NTLMSSP_NEGOTIATE_56 and NTLMSSP_NEGOTIATE_128 are requested and supported by the client and server, NTLMSSP_NEGOTIATE_56 and NTLMSSP_NEGOTIATE_128 will both be returned to the client
	if ( (flags & 0x40000000) != 0 )
		strDebugFlags << "KeyExchange   ";		//	If set, requests an explicit key exchange
	if ( (flags & 0x80000000) != 0 )	
		strDebugFlags << "Negotiate56   ";		// If set, requests 56-bit encryption. If the client sends NTLMSSP_NEGOTIATE_SEAL or NTLMSSP_NEGOTIATE_SIGN with NTLMSSP_NEGOTIATE_56 to the server in the NEGOTIATE_MESSAGE, the server MUST return NTLMSSP_NEGOTIATE_56 to the client in the CHALLENGE_MESSAGE. Otherwise it is ignored. If both NTLMSSP_NEGOTIATE_56 and NTLMSSP_NEGOTIATE_128 are requested and supported by the client and server, NTLMSSP_NEGOTIATE_56 and NTLMSSP_NEGOTIATE_128 will both be returned to the client.

}



// negotiate request flags 
#define NTLM_Unicode				0x00000001
#define NTLM_ASCII					0x00000002
#define NTLM_RequestTarget			0x00000004
#define NTLM_NegotiateSign			0x00000010 
#define NTLM_NegotiateSeal			0x00000020 
#define NTLM_NegotiateNTLM			0x00000200 		
#define NTLM_DomainSupplied			0x00001000
#define NTLM_WorkstationSupplied	0x00002000
#define NTLM_NegotiateLocalCall		0x00004000
#define NTLM_AlwaysSign				0x00008000
#define NTLM_TargetTypeDomain		0x00010000
#define NTLM_TargetTypeServer		0x00020000
#define NTLM_NegotiateNTLMv2		0x00080000
#define NTLM_NegotiateTargetInfo	0x00800000
#define NTLM_NegotiateVersion		0x02000000 
#define NTLM_Negotiate56			0x80000000
#define NTLM_Negotiate128			0x20000000
#define NTLM_KeyExchange			0x40000000


// This is the reverse of this code that is implemented in the browser: 
//		http://dxr.mozilla.org/mozilla-central/source/security/manager/ssl/src/nsNTLMAuthModule.cpp
//
// the official documentation for Type2Message: http://msdn.microsoft.com/en-us/library/cc236642.aspx
struct Type2Message 
{
	char    protocol[8];     // 'N', 'T', 'L', 'M', 'S', 'S', 'P', '\0'
	unsigned long  type;

	unsigned short target_len;		 // domain string length
	unsigned short target_len_alloc; // domain string length
    unsigned long  target_off;       // target_off = &datablock[0] - &protocol[0];  // the 1st byte of datablock is target

	unsigned long  flags;       
	unsigned __int64 challenge;		 
	unsigned long  context_low;
	unsigned long  context_high;

	unsigned short datablock_len;		 
	unsigned short datablock_alloc;	
    unsigned long  datablock_off;   // &datablock[target_len] - &protocol[0];  // starts AFTER the Target
//	--------------------------------------  == 48
	char  chMajorVersion;  
	char  chMinorVersion;
	short sBuildNumber;			
	char  chNotUsed1;				// and   http://msdn.microsoft.com/en-us/library/cc236654.aspx
	char  chNotUsed2;  
	char  chNotUsed3;  
	char  chNTLMRevisionCurrent;
//	-------------------------------------   == 56

	unsigned __int64 unknown;		 //     == 64  

	char datablock [512];

	void get(GString *pArg1,GString *pArg2,GString *pArg3) // DOMAIN, SERVER, my.server.com
	{
		unsigned short *pArg1Type = (unsigned short *)(void *)(&protocol[datablock_off]);
		unsigned short *pArg1Len =  (unsigned short *)(void *)(&protocol[datablock_off + 2]);
		if (*pArg1Type > 5 || *pArg1Len > 256)
			return;
		void *pArgspaceArg1 =						  (void *)(&protocol[datablock_off + 4]);
		pArg1->FromUnicode( (wchar_t *)pArgspaceArg1, *pArg1Len / 2 );

		unsigned short *pArg2Type = (unsigned short *)(void *)(&protocol[datablock_off + 4 + *pArg1Len]);
		unsigned short *pArg2Len =  (unsigned short *)(void *)(&protocol[datablock_off + 4 + *pArg1Len + 2]);
		if (*pArg2Type > 5 || *pArg2Len > 256)
			return;
		void *pArgspaceArg2 =						  (void *)(&protocol[datablock_off + 4 + *pArg1Len + 4]);
		pArg2->FromUnicode( (wchar_t *)pArgspaceArg2, *pArg2Len / 2 );

		unsigned short *pArg3Type =  (unsigned short *)(void *)(&protocol[datablock_off + 4 + *pArg1Len + 4 + *pArg2Len]);
		unsigned short *pArg3Len =   (unsigned short *)(void *)(&protocol[datablock_off + 4 + *pArg1Len + 4 + *pArg2Len + 2]);
		if (*pArg3Type > 5 || *pArg3Len > 256 || *pArg3Len + *pArg2Len + *pArg1Len > sizeof(datablock) )
			return;
		void *pArgspaceArg3 =						  (void *)(&protocol[datablock_off + 4 + *pArg1Len + 4 + *pArg2Len + 4]);
		pArg3->FromUnicode( (wchar_t *)pArgspaceArg3, *pArg3Len / 2 );
	}

	// pzArg4 = 0 or FQDN Host Name
	// pzArg3 = 0 or DNS Domain Name
	// pzArg2 = 0 or NetBIOS Server Name
	// pzArg1 = 0 or NetBIOS Domain Name
	Type2Message(__int64 nChallenge, const wchar_t *pzTarget, const wchar_t *pzArg1, const wchar_t *pzArg2, const wchar_t *pzArg3, const wchar_t *pzArg4 )
	{
		strcpy(protocol,"NTLMSSP");
		memset(datablock,0,sizeof(datablock));
		challenge = nChallenge;
		type = 2;
		context_low = 0;
		context_high = 0;
		chNotUsed1 = 0;
		chNotUsed2 = 0;  
		chNotUsed3 = 0;  
		unknown = 0;

		// 6.2.9200 is Windows 8
		// 6.1.7600 is Windows 7
		// 6.0.6000 is Windows Vista
		chMajorVersion = 6;  
		chMinorVersion = 1;  
		sBuildNumber = 7600; //  FYI: The build number has increased by exactly 1600 each time and each number is exactly divisible by 16.  This reserves the bottom four bits of the number for custom purposes.
		chNTLMRevisionCurrent = 15 ; // 15 == 0x0F;
		//Windows Vista								6.0.6000 (08.11.2006)
		//Windows Vista, Service Pack2				6.0.6002 (04.02.2008)
		//Windows Server 2008						6.0.6001 (27.02.2008)
		//Windows 7, RTM (Release to Manufacturing)	6.1.7600.16385 (22.10.2009)
		//Windows 7									6.1.7601
		//Windows Server 2008 R2, RTM (RTM)			6.1.7600.16385 (22.10.2009)
		//Windows Server 2008 R2, SP1				6.1.7601
		//Windows Home Server 2011					6.1.8400 (05.04.2011)
		//Windows Server 2012						6.2.9200 (04.09.2012)
		//Windows 8									6.2.9200 (26.10.2012)
		//Windows Phone 8							6.2.10211 (29.10.2012)     10211 is less than 10800 which will presumably be Windows 9
		//Windows Server 2012 R2					6.3.9200 (18.10.2013)
		//Windows 8.1								6.3.9200 (18.10.2013)
		//Windows 8.1, Update 1						6.3.9600 (08.04.2014)


		// Note: Type1 flags sent in:
        // FireFox  Flags:NegotiateUnicode  NegotiateASCII  RequestTarget   NegotiateSign  NegotiateLMKey     NegotiateNTLM   NegotiateDomainSupplied   NegotiateWorkstationSupplied   NegotiateAlwaysSign   NegotiateNTLMv2   NegotiateVersion   Negotiate128   KeyExchange   Negotiate56	
        // IE       Flags:NegotiateUnicode  NegotiateASCII  RequestTarget   NegotiateSign  NegotiateLMKey     NegotiateNTLM   NegotiateDomainSupplied   NegotiateWorkstationSupplied   NegotiateAlwaysSign   NegotiateNTLMv2   NegotiateVersion   Negotiate128   KeyExchange   Negotiate56	
        // Safari   Flags:NegotiateUnicode  NegotiateASCII  RequestTarget   NegotiateSign  NegotiateLMKey     NegotiateNTLM   NegotiateAlwaysSign															 NegotiateNTLMv2   NegotiateVersion   Negotiate128   KeyExchange   Negotiate56	

		// Type2 flags with challenge sent out:
		flags =	NTLM_Unicode |	NTLM_RequestTarget | NTLM_NegotiateNTLM | NTLM_AlwaysSign | NTLM_TargetTypeServer |NTLM_NegotiateNTLMv2 | NTLM_NegotiateTargetInfo | NTLM_NegotiateVersion |  NTLM_Negotiate128 | NTLM_Negotiate56;

		target_len = target_len_alloc = wcslen(pzTarget) * 2;
		memcpy(datablock, pzTarget, target_len);
		target_off    = &datablock[0]  -		 &protocol[0];
		datablock_off = &datablock[target_len] - &protocol[0];

		int a = (pzArg1) ? wcslen(pzArg1) * 2 : 0;
		int b = (pzArg2) ? wcslen(pzArg2) * 2 : 0;
		int c = (pzArg3) ? wcslen(pzArg3) * 2 : 0;
		int d = (pzArg4) ? wcslen(pzArg4) * 2 : 0;


		datablock_len = 16 + (a + b + c + d) + 4; // two-two byte shorts occur nArgCount times.  + string lengths + 4 byte termination

		unsigned short *pArg1Type = (unsigned short *)(void *)(&protocol[datablock_off]);
		unsigned short *pArg1Len =  (unsigned short *)(void *)(&protocol[datablock_off + 2]);
		void *pArgspaceArg1 =						  (void *)(&protocol[datablock_off + 4]);
		*pArg1Type = 2; // 2 = NetBIOS Domain Name 
		*pArg1Len = a;
		memcpy(pArgspaceArg1,pzArg1,*pArg1Len);

		unsigned short *pArg2Type = (unsigned short *)(void *)(&protocol[datablock_off + 4 + *pArg1Len]);
		unsigned short *pArg2Len  = (unsigned short *)(void *)(&protocol[datablock_off + 4 + *pArg1Len + 2]);
		void *pArgspaceArg2 =						  (void *)(&protocol[datablock_off + 4 + *pArg1Len + 4]);
		*pArg2Type = 1; // 1 = NetBIOS Server Name 
		*pArg2Len = b;
		memcpy(pArgspaceArg2,pzArg2,*pArg2Len);

		unsigned short *pArg3Type = (unsigned short *)(void *)(&protocol[datablock_off + 4 + *pArg1Len + 4 + *pArg2Len]);
		unsigned short *pArg3Len =  (unsigned short *)(void *)(&protocol[datablock_off + 4 + *pArg1Len + 4 + *pArg2Len + 2]);
		void *pArgspaceArg3 =						  (void *)(&protocol[datablock_off + 4 + *pArg1Len + 4 + *pArg2Len + 4]);
		*pArg3Type = 4;// 4= DNS Domain Name 
		*pArg3Len = c;
		memcpy(pArgspaceArg3,pzArg3,*pArg3Len);

		unsigned short *pArg4Type = (unsigned short *)(void *)(&protocol[datablock_off + 4 + *pArg1Len + 4 + *pArg2Len + 4 + *pArg3Len]);
		unsigned short *pArg4Len =  (unsigned short *)(void *)(&protocol[datablock_off + 4 + *pArg1Len + 4 + *pArg2Len + 4 + *pArg3Len + 2]);
		void *pArgspaceArg4 =						  (void *)(&protocol[datablock_off + 4 + *pArg1Len + 4 + *pArg2Len + 4 + *pArg3Len + 4]);
		*pArg4Type = 3;// 3 = FQDN Host Name 
		*pArg4Len = d;
		memcpy(pArgspaceArg4,pzArg4,*pArg4Len);

		// 4 termnination bytes is included in the datablock_len calculation
		unsigned long *pTerminator =(unsigned long *)(void *)(&protocol[datablock_off + 16 + a + b + c + d]);
		*pTerminator = 0;
	}
};

struct TypeNTLMv2
{
	char ntlmv2_hash[16];

	unsigned int signature2;					 //0: 0x01010000
	unsigned int reservedZeros;					// 4: reserved, zeroes
	unsigned __int64 timestamp;					// 8: Timestamp A 64-bit unsigned integer that contains the current system time, represented as the number of ticks elapsed since midnight of January 1, 1601 (UTC).  The ANSI Date defines January 1, 1601 as day 1, and is used as the origin of COBOL integer dates. This epoch is the beginning of the previous 400-year cycle of leap years in the Gregorian calendar, which ended with the year 2000.  Initially, epoch time was measured in 60ths of a second. The origin of the term "Gregorian" is relating to Pope Gregory I OR to the plainsong chants of the Roman Catholic Church that he taught, the calendar introduced in 1582 by Pope Gregory XIII we call "Gregorian".  Glossolalia 101 : http://simple.wikipedia.org/wiki/Gregorian_chant   http://www.youtube.com/watch?v=9WgqQH5GNkA
	unsigned __int64 clientChallenge;			// 16: client challenge
	unsigned int unkZeros;						// 24: unknown, zeroes

	unsigned short name1Kind;					// 28: target info from server
	unsigned short name1Len;					// 30: target info from server
	char datablock[512];						// 30: target info from server followed by string2Kind, string2Len, and string2data so on until a 4 byte 0 terminates the pattern.
	
	int getStrings(GString &Arg1, short *p1, GString &Arg2, short *p2, GString &Arg3, short *p3,GString &Arg4, short *p4)
	{
		// if we need the values, this implementation will be just like Type2Message::get()
		return 0;
	}
};




void hmac_v2X(unsigned char *keyMD4digest16bytes, const unsigned char *pzUserAndDomainUpperCase, int nstrLength, const unsigned char *challenge, const unsigned char *blob, unsigned bloblen, unsigned char *V2Digest)
{
	try
	{
		#ifdef __OPENSSL
			HMAC_CTX ctx;
			unsigned char hash[EVP_MAX_MD_SIZE];
			unsigned int len = 0;
			
			HMAC(EVP_md5(), keyMD4digest16bytes, MD4_DIGEST_LENGTH, pzUserAndDomainUpperCase, nstrLength, hash, &len);

			// V2 = HMAC-MD5(NTLMv2hash, challenge + blob) + blob 
			HMAC_Init(&ctx, hash, len, EVP_md5());

			HMAC_Update(&ctx, challenge, 8); // 8 is the number of bytes in the challenge

			HMAC_Update(&ctx, blob, bloblen);

			HMAC_Final(&ctx, V2Digest, &len);

			HMAC_cleanup(&ctx);
		#endif
	}
	catch(...)
	{
		InfoLog(6354,"****** HMAC Failed");
	}
}


struct Type3Message
{
    char    protocol[8];     // 'N', 'T', 'L', 'M', 'S', 'S', 'P', '\0'
    unsigned long    type;            // 0x03

    unsigned short   lm_resp_len;     // @12 LanManager response length (always 0x18)
    unsigned short   lm_resp_len2;    // @14 LanManager response length (always 0x18)
    unsigned short   lm_resp_off;     // @16 LanManager response offset
    unsigned char    zero2[2];		  // @18 

    unsigned short   nt_resp_len;     // @20 NT response length    (each MD5 Hash is 16 bytes)
    unsigned short   nt_resp_len2;    // @22 NT response length 
    unsigned short   nt_resp_off;     // @24 NT response offset // 0x54 = 84
    unsigned char    zero3[2];		  // @26

    unsigned short   dom_len;         // @28 domain string length domain (server) name
    unsigned short   dom_len2;        // @30 domain string length
    unsigned short   dom_off;         // @32 domain string offset (always 0x40)
    unsigned char    zero4[2];		  // @34

    unsigned short   user_len;        // @36 username string length
    unsigned short   user_len2;       // @38 username string length
    unsigned short   user_off;        // @40 username string offset
    unsigned char    zero5[2];		  // @42

    unsigned short   host_len;        // @44 host string length
    unsigned short   host_len2;       // @46 host string length
    unsigned short   host_off;        // @48 host string offset
    unsigned char    zero6[2];		  // @50
									  
    unsigned short   sessionkey_len;  // @52: session key             session nonce =  server challenge | client nonce
    unsigned short   sessionkey_len2; // @54: session key             session nonce =  server challenge | client nonce
    unsigned short   sessionkey_off;  // @56: session key             session nonce =  server challenge | client nonce
    unsigned char    zero7[2];		  // @58

	unsigned int     flags;			  // @60: negotiated flags
	unsigned char    datablock[512];
};

////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////  End of NTLM related code ///////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////




// Convert															TO
// -------------------------------------------------------------------------------------------------------------------
// GET http://www.Domain.com/Index.html HTTP/1.0					GET Index.html HTTP/1.0
// GET http://www.Domain.com/Index HTTP/1.1							GET Index HTTP/1.1
// GET http://www.Domain.com/images/pic.jpg HTTP/1.1			GET /images/pic.jpg HTTP/1.1
// -------------------------------------------------------------------------------------------------------------------
// subsequent commands on Keep-Alive connections copy pIn to pOut as the data may look like this:
// GET /menu.js HTTP/1.0
// GET /images/pic.jpg HTTP/1.1
// GET \menu.js HTTP/7.0
// -------------------------------------------------------------------------------------------------------------------

//				>> "GET http://www.yahoo.com/ HTTP/1.1[\r][\n]"
//				>> "User-Agent: Browser[\r][\n]"
//  in			>> "Host: www.yahoo.com[\r][\n]"
//				>> "Proxy-Connection: Keep-Alive[\r][\n]"
//				>> "[\r][\n]"
//
//				<< "HTTP/1.0 407 Proxy Authentication Required[\r][\n]"
//				<< "Server: 5Loaves"
//	out			<< "Proxy-Authenticate: NTLM[\r][\n]"
//				<< "Proxy-Authenticate: Basic realm="5Loaves"[\r][\n]"
//				<< "Proxy-Connection: close[\r][\n]"
//
//				>> "GET http://www.yahoo.com/ HTTP/1.0[\r][\n]"
//				>> "User-Agent: Browser[\r][\n]"
//	in			>> "Proxy-Authorization: NTLM "[\n]"
//				>> "Host: www.yahoo.com[\r][\n]"
//				>> "Proxy-Connection: Keep-Alive[\r][\n]"
//				>> "[\r][\n]"

int ConvertHTTPProxyInternal(char *pIn, int nInLen, char *pOut, GString &strPxyConnectTo, int *pnConvertedLen, int *pbIsCONNECT)
{
	//	note: pIn must not == pOut, memory may not overlap   

	if ( memcmp(pIn,"CONNECT",7) == 0 || memcmp(pIn,"connect",7) == 0) 
	{
		GString strPxyToPort;
		char *pPort = strpbrk(pIn+8,":");
		if (pPort)
		{
			strPxyToPort.SetFromUpTo(pPort+1, " ");
			if ( g_bOnly443or80 &&  strPxyToPort.Compare("443") && strPxyToPort.Compare("80" ))
			{
				GString g("******* Blocked proxy to port[");
				g << strPxyToPort << "] for [" << pIn << "]";
				g.ReplaceChars("\r\n",'~');
				InfoLog(456,g);

				// note: This HTTP Proxy only allows port 80 and 443, removing this return removes the limitation.
				return 0; 
			}
			strPxyConnectTo = strPxyToPort;
			strPxyConnectTo << "@"; // = "443@";
		}
		else
		{
			strPxyConnectTo = "80@";
		}

		strPxyConnectTo.AppendFromUpTo(pIn+8, ":");
		*pnConvertedLen = 0;
		
		if (pbIsCONNECT)
			*pbIsCONNECT = 1;

		return 1; // Proxy conversion success
	}
	if (pbIsCONNECT)
		*pbIsCONNECT = 0;

	char *pSpace = strpbrk(pIn," "); // we want the HTTP verb left of space
	if (pSpace)
	{
		if (pSpace[1] == '/' || pSpace[1] == '\\')
		{
			memcpy(pOut,pIn,nInLen);
			*pnConvertedLen = nInLen;
			return 1; // Proxy conversion success
		}
		else if ( memcmp(pSpace+1,"http://",7) == 0  ||  memcmp(pSpace+1,"HTTP://",7) == 0)
		{
			*pnConvertedLen = pSpace-pIn+1;
			memcpy(pOut,pIn,*pnConvertedLen);  // "GET " or "POST " moved to destination


			char *pSlash = strpbrk(pSpace+8,"/"); // first / after "http://" www.xxxx.com/  <--that slash
			if (pSlash)
			{
				int nCpySize = pSlash-pIn;//  nCpySize = size of  "GET http://www.xxxx.com"
				memcpy(&pOut[*pnConvertedLen],pSlash,nInLen-nCpySize);
				*pnConvertedLen += nInLen-nCpySize;
				strPxyConnectTo.WriteOn(pSpace+8,pSlash - (pSpace+8)); // copy the server name "www.Server.com"
				
				char *pColon = strpbrk(strPxyConnectTo.Buf(),":");
				if (pColon)
				{
					// if the ConnectTo is 1.2.3.4:81 convert it to 81@1.2.3.4
					*pColon	= 0;
					GString strTemp;
					strTemp << (pColon + 1) << '@' << strPxyConnectTo.Buf();
					strPxyConnectTo = strTemp;
				}
				else
				{
					strPxyConnectTo.Prepend("80@");
				}
				return 1; // Proxy conversion success
			}
		}
		else
		{
			// I have never seen this error but i might someday
			InfoLog(58,"\r\n\r\n******************************* HTTPProxy FAILURE ***************************\r\n\r\n");
		}
	}
	return 0; // no proxy conversion
}

int ConvertHTTPProxy(char *pIn, int nInLen, char *pOut, GString &strPxyConnectTo, int *pnConvertedLen, int *pbIsCONNECT)
{
	pIn[nInLen] = 0;
	int nInHTTPHeaderLen = -1; // a POST may be followed by content so nInLen may be  > nInHTTPHeaderLen
	char *prnrn = strstr(pIn, "\r\n\r\n");
	if (prnrn)
	{
		nInHTTPHeaderLen = prnrn - pIn + 4;
	}

	int nContentLen = -1; // not all HTTP request have a Content-Length - they may be terminated with an /r/n/r/n
	char *pCL = stristr(pIn, "Content-Length: ");
	if (pCL)
	{
		nContentLen = atoi(pCL + 16); // 16 is strlen("Content-Length: ")
	}

	int nRet = ConvertHTTPProxyInternal(pIn, nInLen, pOut, strPxyConnectTo, pnConvertedLen, pbIsCONNECT);
	

	// a "CONNECT ...." command has a 0 byte *pnConvertedLen, and pbIsCONNECT will be true
	if (nRet && *pnConvertedLen)
	{
		// POST / HTTP 1.1\r\ncontent-length: 7 \r\n\r\n1234567


		// remove the "Proxy-Authorization:",
		char *p = stristr(pOut, "Proxy-Authorization:");
		if (p)
		{
			char *prn    = strstr(p, "\r\n");
			if (!prn)
			{
				InfoLog (4267,"Failed to convert invalid HTTP headers");
				return 0;
			}
			prn += 2;
			
			int nDiff = prn - p;
			if (nDiff < 0)
			{
				int i=0; i++; // debug break here
			}
			*pnConvertedLen -= prn - p;

			memmove(p,prn,(*pnConvertedLen) - (p - pOut) );
		}
		else
		{
			// the /r/n's may or may not be there - so remove them. (we will readd them)
			int nEatRNs = *pnConvertedLen - 1;
			while ( pOut[nEatRNs] == '\r' || pOut[nEatRNs] == '\n' )
			{
				nEatRNs--;
				(*pnConvertedLen)--;
			}
			memcpy(&pOut[*pnConvertedLen],"\r\n\r\n\0",5);
			*pnConvertedLen += 4; // do not count the null
		}

		// after an NTLM Negotiate, the "Host:" no longer exists in the HTTP headers from IE because it was after the Proxy-Auth
		char *pHost = stristr(pOut, "Host: ");
		if (!pHost)
		{
			// add the "Host: " to the HTTP Headers
			char *prnrn = stristr(pOut, "\r\n\r\n");
			strcpy(prnrn + 2,"Host: ");
			int nAt = strPxyConnectTo.Find('@');
			nAt++; // one byte past the @
			memcpy(prnrn + 8, strPxyConnectTo._str + nAt,  strPxyConnectTo._len - nAt);
			memcpy(prnrn + 8 + strPxyConnectTo._len - nAt,"\r\n\r\n\0",5);

			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// could call strlen() and recount the length, or do a little basic math to be faster than the iterations inside strlen() to find a null.
			*pnConvertedLen = prnrn - pOut + 12 + strPxyConnectTo._len - nAt; // 12 is len("host: ") + the "\r\n"'s, (strPxyConnectTo._len - nAt) is the actual host length
			//  or we could do it the slow way....
			// *pnConvertedLen = strlen(pOut); 
			// if (strlen(pOut) != *pnConvertedLen)
			// {
			// 	// code here is never hit - because the math is right
			// 	InfoLog (457,"\r\n\r\n ****************** Bad Math does not happen ******************************\r\n\r\n");
			// }
			//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		}
	}
	return nRet;
}




/*
This structure is stored in BaseInterface.h for maintenance reasons
because every language driver needs the definition also.
  
struct CBDataStruct
{
	int nState;			    // currently unused - may track transactional state
	int nReserved;			// currently unused - the size of this struct cannot change or it will break lots of code so maybe this will be needed someday
	void *pstrDest;		    // a GString * - may be used directly if you link with the same foundation the invoking process is built with.
	CBfn fn;				// 2nd level handler function on the calling process, 1st level is in the driver.
	char *piBuf;			// set by the invoking process, that the called method may indicate processing instructions on the results, assume 1024 bytes
	unsigned char *wkBuf;   // set by the invoking process, a region of memory as a working buffer for the called method, size is set by invoker, nWKBufferSize bytes(normally 64000).
	unsigned char *wkBuf2;  // set by the invoking process, a region of memory as a working buffer for the called method, size is set by invoker, nWKBufferSize bytes(normally 64000).
	void *pCBArg;		    // a void * user defined by the SetDriverOption("callbackArg");
	void *pDriverInterface; // pointer to instance of driver interface (for example, a CStdCallInterface, CJavaInterface, CPerlInterface)
	int nWKBufferSize;		// size of memory allocated in "wkBuf"

	union UArg
	{
		__int64 Int64;
		__int64 *PInt64;
		int Int;
		int *PInt;
		const char *CChar;
		char *Char;
		void *Void;
	};
	UArg arg[7];
};
*/







// this struct can be modified without breaking backward compatibility 
// IF any additions are added at the bottom of the structure
struct DriverCallBackArg
{
	// always valid
	int nProtocol; // 1 for HTTP, 2 for Xfer, 3 for Messaging
	int fd;
	int tid;
	int nCompleteSize;
	int nObtainedCount;
	GString strUser;
	unsigned short sDataFlags;

	int *pnKeepAlive;

	// only valid for Xfer
	CipherData *en;
	CipherData *de;
	void *XCmd; // an XferCommand *

	int nBuf1Size;
	GString strBoundary;
	GString strArgs;
};

//
// This is how plugins interface with the kernel functions through the driver that loads the plugin.
// Xfer plugins and HTTP Server plugins and Messaging plugins use this same callback handler.
// 
void DriverCallBack(const char *pzCmd, void *pCBArg)
{
	GString strOption(pzCmd);
	CBDataStruct *pcb = (CBDataStruct *)pCBArg;
	DriverCallBackArg *sysState = (DriverCallBackArg *)pcb->pCBArg;

	if (strOption.CompareNoCase("msg") == 0)
	{
		if (sysState->nProtocol == 3)
		{
			GStringList lstArgs("/",sysState->strArgs);
			GString *p = lstArgs.GetStrAt(pcb->arg[0].Int);
			strncpy(pcb->arg[1].Char, p->Buf(), pcb->arg[2].Int  );
			pcb->arg[1].Char[pcb->arg[2].Int - 1] = 0;
		}
	}
	else if (strOption.CompareNoCase("usr") == 0)
	{
		strncpy(pcb->arg[0].Char, sysState->strUser.Buf(), pcb->arg[1].Int  );
		pcb->arg[0].Char[pcb->arg[1].Int - 1] = 0;
	}
	else if (strOption.CompareNoCase("siz") == 0)  // get full size of data to expect
	{
		pcb->arg[0].Int = sysState->nCompleteSize;
	}
	else if (strOption.CompareNoCase("tot") == 0)  // get full size of data to expect
	{
		pcb->arg[0].Int = sysState->nObtainedCount;
	}
	else if (strOption.CompareNoCase("xtx") == 0)  // Transmit
	{
		if (sysState->nProtocol == 1 || sysState->nProtocol == 3)
		{
			// note: the caller MUST correctly setup the HTTP Headers in the first block sent back 
			if ( nonblocksend(sysState->fd,pcb->arg[0].CChar,pcb->arg[1].Int) == pcb->arg[1].Int)
			{
				pcb->arg[0].Int = 1;// success
			}
			else
			{
				pcb->arg[0].Int = 0;// fail
			}
		}
		else if (sysState->nProtocol == 2)
		{
		#ifdef ___XFER
			// note: this comment ONLY applies to Xfer protocol, sending from the system buffers for HTTP is always safe and always recommended.
			//
			// pcb->arg[0].CChar should be memory other than pcb->wkBuf, just to be safe
			// pcb->arg[0].CChar must never be pcb->wkBuf2
			//
			// Do NOT place data to send in the System Buffer, normally it will send fine from pcb->wkBuf, but if it spans multiple
			// packets(aka > 8192) AND the compression hits worst case scenarios(pre compressed data expands a little), then 
			// the beginning of the second block will be stepped on by end of the first, this is all our memory
			// so no error will occur, but your data will be trashed when you receive it.  Note that it will 
			// successfully decrypt and decompress on the client side because it's getting exactly what the server sent
			// but that's not what the developer of the plugin intended, so be safe and use your own memory unless you
			// know for sure that this situation could never occur in your application.
			GString strAttached(pcb->arg[0].CChar, pcb->arg[1].Int, pcb->arg[1].Int, 0); // Uses the Attach Constructor

			// note: The caller MUST correctly setup the header in the first call to "xtx" on every connection
			// each subsequent call, if any, is a continuation of the data stream initiated in the first call
			// so the beginning of the first packet must look like this: "OK nnnnnnn:DDDDD"
			// where nnnn is an alpha numeric length of the data(DDDDD) that begins after the ':'
			LocalData LD((XferCommand *)sysState->XCmd);
			LD.fd = sysState->fd;
			LD.buf1 = pcb->wkBuf;
			LD.buf2 = pcb->wkBuf2;
			LD.sDataFlags = sysState->sDataFlags;
			LD.en = sysState->en;
			pcb->arg[0].Int = LD.XCmd->SendXferResults(&LD, &strAttached);
		#endif // #ifdef ___XFER
		}

		
		// add the Process Instruction if it's not already added
		if (stristr(pcb->piBuf,"DataSent") == 0)
			strcpy(pcb->piBuf,"DataSent");
	
	}
	else if (strOption.CompareNoCase("xrx") == 0)  // Xfer Receive
	{
		if (sysState->nProtocol == 1 || sysState->nProtocol == 3)
		{
			if (pcb->arg[0].Int == -1)
			{
				pcb->arg[0].Int = nonblockrecvAny(sysState->fd,pcb->arg[1].Char,pcb->arg[2].Int, pcb->arg[3].Int, pcb->arg[4].Int);
			}
			else
			{
				pcb->arg[0].Int = nonblockrecvAny(sysState->fd,(char *)pcb->wkBuf,sysState->nBuf1Size, 60, 0);
			}
			if (pcb->arg[0].Int > 0)
			{
				sysState->nObtainedCount += pcb->arg[0].Int;
				pcb->arg[1].Int = pcb->arg[0].Int; // move the size into arg 1
				pcb->arg[0].Int = 1; // indicate success 

				if (sysState->nProtocol == 3)
				{
					//int nToWrite = pcb->arg[1].Int;
					if (sysState->nCompleteSize - sysState->nObtainedCount < 256)
					{
						// walk back from the end and stop at a null or 256
						int nEndIndex = pcb->arg[1].Int - 1;
						while(pcb->wkBuf[nEndIndex] && pcb->arg[1].Int - nEndIndex < 256)
							nEndIndex--;	

						char *pEnd = strstr(&pcb->wkBuf[nEndIndex+1],sysState->strBoundary.Buf());
						if (pEnd)
						{
							pcb->arg[1].Int = (pEnd - pcb->wkBuf) - 4; // four bytes behind the boundary marker
							sysState->nCompleteSize = sysState->nObtainedCount;
						}
					}
				}

			}
		}
		else if (sysState->nProtocol == 2)
		{
		#ifdef ___XFER
			// exec result into [0], size result into [1]
			pcb->arg[0].Int = DoXferPacketRead(sysState->fd, (char *)pcb->wkBuf2, (char *)pcb->wkBuf, &pcb->arg[1].Int, sysState->de, 60);
			if ( pcb->arg[0].Int == 1 )
			{
				sysState->nObtainedCount += pcb->arg[1].Int;
			}
		#endif // ___XFER
		}
	}
	else if (strOption.CompareNoCase("pre") == 0)  
	{
		((GString *)pcb->pstrDest)->SetPreAlloc(pcb->arg[0].Int);
	}
	else if (strOption.CompareNoCase("cat") == 0)  // concatenate data "to send"
	{
		if (pcb->arg[0].CChar)
			*((GString *)pcb->pstrDest) << pcb->arg[0].CChar;
	}
	else if (strOption.CompareNoCase("wrtSnd") == 0) 
	{
		// send it straight out of the plugin-memory
		// assumes user management of HTTP headers.
		pcb->arg[2].Int = nonblocksend(sysState->fd,pcb->arg[1].CChar,pcb->arg[0].Int);
	}
	else if (strOption.CompareNoCase("wrt") == 0)
	{
		if (pcb->arg[1].CChar)
			((GString *)pcb->pstrDest)->write(pcb->arg[1].CChar,pcb->arg[0].Int);
	}
	else if (strOption.CompareNoCase("ip") == 0)
	{
		struct sockaddr_in other_addr;
		int nSize = sizeof(sockaddr_in);
		getpeername(sysState->fd,(struct sockaddr *) &other_addr, (socklen_t *)&nSize);
		strcpy(pcb->arg[0].Char,inet_ntoa(other_addr.sin_addr));
	}
	else if (strOption.CompareNoCase("tid") == 0)
	{
		pcb->arg[0].Int = sysState->tid;
	}
	else if (strOption.CompareNoCase("keepalive") == 0)  // return connection keep alive preference
	{
		pcb->arg[0].PInt = sysState->pnKeepAlive;
	}
	else if (strOption.CompareNoCase("dis") == 0)
	{
		PortableClose(sysState->fd,"Core7");
	}
	else if (strOption.CompareNoCase("fd") == 0)  // return socket file descriptor
	{
		pcb->arg[0].Int = sysState->fd;
	}
	else if (strOption.CompareNoCase("log") == 0)  // write to system log file
	{
		InfoLog(59,pcb->arg[0].CChar);
	}
}

int Plugin(ThreadData *td, char *sockBuffer, int nBytes, int nContentLen, char *pContent, unsigned int *pnKeepAlive, char *workBuffer, __int64 *nTotalProxiedBytes, unsigned int nBytesRemaining)
{
	// Design note: This function is designed to assume that the sockBuffer value(the first read off the TCP buffers on the HTTP port)  
	// does NOT map to any plugin, it must avoid as much logic execution as possible until a plugin map has been matched
	// because this "filter" will execute for every HTTP request - we need to add as little overhead as possible.

	// This function has two main logic branches, one for POST and one for GET, as they each handle the data somewhat differently

	GListIterator itPlgIn( &g_lstActivePlugins ); // a list(plugins) of lists(each plugins arguments), they are all very small
	while(itPlgIn())
	{
		GStringIterator itPlugInArgs( (GStringList *)itPlgIn++ );		// lists are pre-constructed and pre-parsed to reduce overhead
		GString strPlugInPath( !itPlugInArgs() ? 0 : itPlugInArgs++);
		

		// check for a wildcard method match (user assigned method name by operator permission only)
		GString strMethod;
		const char *pzMethod = 0;
		if ( strPlugInPath.GetAt(strPlugInPath.Length()-1) == '*' )  // test the one bytes that indicates a wildcard
		{
			char *pSp = strpbrk(sockBuffer," ");
			if (pSp)
			{
				if ( strPlugInPath.CompareNoCase(pSp+1,strPlugInPath.Length()-1) == 0)
				{
					char *pTerm = strpbrk(pSp+strPlugInPath.Length(),"/\\ \r\n");
					if (!pTerm) pTerm = pSp+1;
					strMethod.WriteOn(pSp+2,pTerm - (pSp+2));
					pzMethod = strMethod;
					strPlugInPath   = "/";				// (this here code....
					strPlugInPath  += strMethod;		// retains path match length...
					strPlugInPath  += "/";				// so that arguments are parsed out correctly)
				}
			}
		}
		
		// this test is executed once for each configured plugin
		if ( memcmp(sockBuffer,"POST",4) == 0 && (pzMethod || strPlugInPath.CompareNoCase(&sockBuffer[5],strPlugInPath.Length()) == 0 ) )
		{
			// ok we got a match, now none of this code is overhead to standard HTTP transactions

			const char *pzLanguage = ( !itPlugInArgs() ) ?  0 : itPlugInArgs++;
			const char *pzComponent = ( !itPlugInArgs() ) ?  0 : itPlugInArgs++;
			const char *pzInterface = ( !itPlugInArgs() ) ?  0 : itPlugInArgs++;
			if (!pzMethod) pzMethod = ( !itPlugInArgs() ) ?  0 : itPlugInArgs++;


			// determine (next transaction) connection attributes
			// default to "Keep Alive, unless directed to close"
			GString strConnectionType("Keep-Alive");
			int nIsKeepAlive = 1;
			char *pCon = strstr((char *)sockBuffer,"Connection:");
			if (pCon)
			{
				char *pEnd = strpbrk(pCon + 12, " \r\n");
				if (pEnd)
				{
					char chSave = *pEnd;
					*pEnd = 0;
					strConnectionType = pCon + 12;
					*pEnd = chSave;
					if (strConnectionType.CompareNoCase("close") == 0)
					{
						nIsKeepAlive = 0;
					}
				}
			}
			if (!nIsKeepAlive) // if the requestor did not want this connection to be kept alive 
			{
				*pnKeepAlive = 0;
			}
			else
			{
				(*pnKeepAlive)--;
			}


			InterfaceInstance *pII = GetInterfaceInstance(pzLanguage);
			if (!pII)
			{
				GString strError;
				strError << pzLanguage << " is not configured correctly or is not a supported component type on this machine.";
				InfoLog(60,strError);
				*pnKeepAlive = 0;
				return 0;
			}

			const char *pDataStart = &sockBuffer[5 + strPlugInPath.Length()];


			// get the argument names, if arg1 is "Header" and arg2 is "Content", then don't parse arguments.
			// in that case, parsing is the responsibility of the plug-in.
			int bParse = 1;
			GStringList *pL = pII->GetMethodParams(pzComponent, pzInterface, pzMethod);
			int nReqArgCount = pL->Size();
			GStringIterator itArgNames(pL);
			if (itArgNames())
			{
				char *p = stristr(itArgNames++,"Header");
				if (p && itArgNames())
				{
					p = stristr(itArgNames++,"Content");
					if (p && !itArgNames())
						bParse = 0;
				}
			}
			itArgNames.reset();

			

			if (nBytesRemaining && ((nBytes + nBytesRemaining) < MAX_RAW_CHUNK ) && bParse )
			{
				// we need the rest of the POST data before we can parse out the arguments required for this method invocation
				if ( (unsigned int)nonblockrecv(td->sockfd,&sockBuffer[nBytes],nBytesRemaining) != nBytesRemaining)
				{
					*pnKeepAlive = 0; 
					return 0;
				}
				nBytesRemaining = 0;
				nBytes += nBytesRemaining;
				sockBuffer[nBytes] = 0;
			}
			else if ( nBytesRemaining && (nBytes < MAX_RAW_CHUNK ) && bParse )
			{
				// we can't fit the entire POST into the system buffer but we can hold more than we have, we'll get all we can
				// then see if we can parse out the arguments from what we have, this allows the plugin to map as much as possible
				// and callback for the remainder of the data stream that follows the mapped arguments
				if ( nonblockrecv(td->sockfd,&sockBuffer[nBytes],MAX_RAW_CHUNK - nBytes) != MAX_RAW_CHUNK - nBytes)
				{
					*pnKeepAlive = 0; 
					return 0;
				}
				nBytesRemaining -= (MAX_RAW_CHUNK - nBytes);
				nBytes += (MAX_RAW_CHUNK - nBytes);
				sockBuffer[nBytes] = 0;
			}


			// prepare the information available to the driver callback
			DriverCallBackArg prd;
			prd.XCmd = 0;
			prd.de = 0; // cipher N/A on anonymous HTTP
			prd.en = 0;
			prd.fd = td->sockfd;
			prd.nProtocol = 1;
			prd.tid = td->nThreadID;
			prd.strUser = "Anonymous HTTP";
			prd.nCompleteSize = nContentLen;
			prd.nObtainedCount = nContentLen - nBytesRemaining;
			prd.sDataFlags = 0; // N/A
			prd.nBuf1Size = MAX_DATA_CHUNK;
			

			// setup the driver options
			char pzPIBuf[1024];
			pzPIBuf[0] = 0;
			pII->SetOption("piBuf",pzPIBuf);
			pII->SetOption("callback",(void *)DriverCallBack);
			pII->SetOption("callbackArg",&prd );
			pII->SetOption("wkBuf",sockBuffer);
			pII->SetOption("wkBuf2",workBuffer);
			int n = MAX_DATA_CHUNK;
			pII->SetOption("wkBufSize",(void *)n);
			if ( GetProfile().GetBool("HTTP","UnloadPlugins",false) )
			{
				int yes = 1;
				pII->SetOption("unload",&yes); 
			}


			// get the POSTed content type 
			GString strContentType;
			char *pCT = stristr(sockBuffer,"Content-Type:");
			if (pCT)
			{
				pCT += 13;
				char *pEnd = strpbrk(pCT, "\r\n");
				char ch = *pEnd;
				*pEnd = 0;
				strContentType = pCT;
				strContentType.TrimLeftWS(); // remove leading space if there is one.
				*pEnd = ch;
			}
			// see if this content type needs to be decoded
			int bDoDecode = 0;
			if ( strContentType.CompareNoCase("application/x-www-form-urlencoded") == 0) 
			{
				bDoDecode = 1;
			}
				

			int nHeadLen = pContent - pDataStart;



			


			//
			// prepare arguments for Invoke() call
			//
			GString strArgSizes;




			// special case, pass the HTTP headers as Arg1, and the content as Arg2
			if (!bParse /*&& (nReqArgCount == 2) */) 
			{
				if (bDoDecode)
				{
					nContentLen = unEscapeData(pContent,pContent);
					pContent[nContentLen] = 0; // terminate what we just unescaped
				}
				strArgSizes << nHeadLen << "|" << nContentLen;

			}
			// parse out the HTML form data - map to method arguments
			else if (bParse)
			{
				int nCurrentIndex = 0;
				pContent[nContentLen] = 0;
				GStringList lstHTMLArgs("&",pContent); // pContent = "One=1sValue&Two=2sValue&Three=3sValue&B1=Submit" where One, Two, and Three are the input control names in the HTML
				GStringIterator itHTMLArgs(&lstHTMLArgs);
				
				GString strHTMLArgName;
				int nNewArgLen;
				while(itArgNames()) // loop through the required arguments
				{
					const char *pRegArg = itArgNames++;
					int bFound = 0;
					while(itHTMLArgs()) // loop through the supplied arguments
					{
						const char *pzHTMLArg = itHTMLArgs++;
						char *pzSep = (char *)strpbrk(pzHTMLArg, "=");
						if (pzSep)
						{
							*pzSep = 0;
							strHTMLArgName = pzHTMLArg;
							if (strHTMLArgName.CompareNoCase(pRegArg) == 0)
							{
								*pzSep = '=';
								pzSep++;
								nNewArgLen = unEscapeData(pzSep,&sockBuffer[nCurrentIndex]);
								nCurrentIndex += nNewArgLen + 1; // size plus NULL
								if (strArgSizes.Length())
									strArgSizes << "|";
								strArgSizes << (nNewArgLen+1);
								bFound = 1;
								break;
							}
							else
							{
								*pzSep = '=';
							}
						}
					}
					if (!bFound)
					{
						if (nBytesRemaining)
						{
							// Maybe we didn't find this argument because it was after an argument that came in much larger than expected.
							// In this case, either the data posted is invalid so dropping the connection is the proper thing to do.
							// It's not impossible for a name, email, phone or the like to come in at 50,000 bytes.
							// If this is legitimate data the plugin needs to handle this type of transaction by mapping directly to the 
							// HTTP Header and Content and using the argument parsing utilities to extract the arguments.  
							GString strError;
							
							strError << pzComponent << "::" << pzInterface << "::" << pzMethod << " dropped connection posting:" << nContentLen << " bytes.";
							InfoLog(61,strError);
						}
						else // this is an error in the HTML or the mapping between the plugin and the HTML is out of date
						{
							GString strInfo;
							strInfo << "ERROR: [" << pRegArg << "]" << "not found for call:" << pzComponent << "::" << pzInterface << "::" << pzMethod;
							InfoLog(62,strInfo);
						}
						*pnKeepAlive = 0; 
						return 0;
					}
					itHTMLArgs.reset();
				}
				pDataStart = sockBuffer;
			}
			else
			{
				GString strInfo;
				strInfo << "ERROR: Arg count must be 2 when using 'HTTPHeader/Content' but you have " << nReqArgCount << "---"  << pzComponent << "::" << pzInterface << "::" << pzMethod;
				InfoLog(63,strInfo);
			}

		

			int nOutResultSize = 0;
			const char *pzResults = pII->InvokeEx( pzComponent, pzInterface, pzMethod, pDataStart,strArgSizes,&nOutResultSize);


			if (stristr(pzPIBuf,"DataSent") != 0)
			{
				// the plugin sent the data directly
				return 1; // HTTP request handled.
			}
			if (nOutResultSize < 1 || stristr(pzPIBuf,"NoKeepAlive") != 0)
			{
				*pnKeepAlive = 0;
				return 1; // HTTP request handled, no results from user created handler.
			}

			// currently this is the only processing instruction available, in the future this might be a pipe separated list.
			// todo: add processing instruction support "NoKeepAlive", because by default they are all kept alive.
			if (stristr(pzPIBuf,"HasHeaders") != 0)
			{
				if (nOutResultSize)
				{
					if (nonblocksend(td->sockfd,pzResults,nOutResultSize) == nOutResultSize)
					{
						*nTotalProxiedBytes += nOutResultSize;
					}
				}
			}
			else
			{
				// todo: support multiple content types by setting "Ext", currently NULL is "text/html"
				BuildHTTPHeader(sockBuffer, 0/*FileName*/, strConnectionType, nIsKeepAlive, *pnKeepAlive, 0/*ETag*/, 0 /*Ext*/, nOutResultSize, -1, -1);
				int nHL = strlen(sockBuffer);

				if (nHL + nOutResultSize < MAX_DATA_CHUNK)
				{
					memcpy(&sockBuffer[nHL],pzResults,nOutResultSize);
					if (nonblocksend(td->sockfd,sockBuffer,nOutResultSize+nHL) == nOutResultSize+nHL)
					{
						*nTotalProxiedBytes += nOutResultSize+nHL;
					}
				}
				else
				{
					if (nonblocksend(td->sockfd,sockBuffer,nHL) == nHL)
					{
						*nTotalProxiedBytes += nHL;
						if (nonblocksend(td->sockfd,pzResults,nOutResultSize) == nOutResultSize)
						{
							*nTotalProxiedBytes += nOutResultSize;
						}
					}
				}
			}
			return 1;
		}
		else if ( memcmp(sockBuffer,"GET",3) == 0 && (pzMethod || strPlugInPath.CompareNoCase(&sockBuffer[4],strPlugInPath.Length()) == 0 ) )
		{
			// RFC 2616, "Hypertext Transfer Protocol -- HTTP/1.1," does not specify any requirement for URL length. 
			// Microsoft Internet Explorer as of version 7, (in June 2007) supports a maximum URL length of 2,083
			const char *pzLanguage = ( !itPlugInArgs() ) ?  0 : itPlugInArgs++;
			const char *pzComponent = ( !itPlugInArgs() ) ?  0 : itPlugInArgs++;
			const char *pzInterface = ( !itPlugInArgs() ) ?  0 : itPlugInArgs++;
			if (!pzMethod) pzMethod = ( !itPlugInArgs() ) ?  0 : itPlugInArgs++;


			// determine (next transaction) connection attributes
			// default to "Keep Alive, unless directed to close"
			GString strConnectionType("Keep-Alive");
			int nIsKeepAlive = 1;
			char *pCon = strstr((char *)sockBuffer,"Connection:");
			if (pCon)
			{
				char *pEnd = strpbrk(pCon + 12, " \r\n");
				if (pEnd)
				{
					char chSave = *pEnd;
					*pEnd = 0;
					strConnectionType = pCon + 12;
					*pEnd = chSave;
					if (strConnectionType.CompareNoCase("close") == 0)
					{
						nIsKeepAlive = 0;
					}
				}
			}
			if (!nIsKeepAlive) // if the requestor did not want this connection to be kept alive 
			{
				*pnKeepAlive = 0;
			}
			else
			{
				(*pnKeepAlive)--;
			}

			
			
			// prepare the information available to the driver callback			
			DriverCallBackArg prd;
			prd.XCmd = 0;
			prd.de = 0; // cipher N/A on anonymous HTTP
			prd.en = 0;
			prd.fd = td->sockfd;
			prd.nProtocol = 1;
			prd.tid = td->nThreadID;
			prd.strUser = "Anonymous HTTP";
			prd.nCompleteSize = nBytes;
			prd.nObtainedCount = nBytes;
			prd.sDataFlags = 0; // N/A
			prd.nBuf1Size = MAX_DATA_CHUNK;
			

			// setup the driver options
			InterfaceInstance *pII = GetInterfaceInstance( pzLanguage);
			if (!pII)
			{
				GString strError;
				strError << pzLanguage << " is not configured correctly or is not a supported component type on this machine.";
				InfoLog(64,strError);
			}
			pII->SetOption("callback",(void *)DriverCallBack);
			pII->SetOption("callbackArg",&prd);
			char pzPIBuf[1024];
			pzPIBuf[0] = 0;
			pII->SetOption("piBuf",pzPIBuf);
			pII->SetOption("wkBuf",workBuffer);
			pII->SetOption("wkBuf2",sockBuffer);
			int n = MAX_DATA_CHUNK;
			pII->SetOption("wkBufSize",(void *)n);
			if ( GetProfile().GetBool("HTTP","UnloadPlugins",false) )
			{
				int yes = 1;
				pII->SetOption("unload",&yes); 
			}

			const char *pDataStart = &sockBuffer[4 + strPlugInPath.Length()]; // 4 = strlen("GET ")
			nBytes -= (pDataStart - sockBuffer);

			
			// get the argument names, if arg1 is "Header" and arg2 is "Content", then don't parse arguments.
			// in that case, parsing is the responsibility of the plug-in.
			int bParse = 1;
			GStringList *pL = pII->GetMethodParams(pzComponent, pzInterface, pzMethod);
			int nReqArgCount = pL->Size();
			GStringIterator itArgNames(pL);
			if (itArgNames())
			{
				char *p = stristr(itArgNames++,"Header");
				if (p && itArgNames())
				{
					p = stristr(itArgNames++,"Content");
					if (p && !itArgNames())
						bParse = 0;
				}
			}
			itArgNames.reset();


			
			// "GET /Path/PageAAA-BBB-CCC HTTP/1.1\r\n" 
			// where "/Path/Page" is the strPlugInPath, all remaining data to the space is the argument.
			// an argument is optional, the call could be "GET /Path/Page HTTP/1.1\r\n"
			int nArgsLen = 0;
			if (*pDataStart != 32)
			{
				char *pArgEnd = (char *)strpbrk(pDataStart," \r\n");
				if (pArgEnd && *pArgEnd == 32)
				{
					nArgsLen = pArgEnd - pDataStart;
				}
				else
				{
					nArgsLen = strlen(pDataStart);
				}
			}

			
			GString strArgSizes;
			// special case, pass the HTTP headers as Arg1, and the content as Arg2
			if (!bParse && nReqArgCount == 2) 
			{
				int nNewArgsLen = nArgsLen;
				if (nArgsLen) 
				{
					if (nArgsLen + nBytes < MAX_DATA_CHUNK)
					{
						int n = (pDataStart - sockBuffer);
						// unescape and place the arguments at the end of the buffer.
						nNewArgsLen = unEscapeData(pDataStart,&sockBuffer[nBytes+n]);
						sockBuffer[nBytes+n+nNewArgsLen] = 0; // terminate what we just copied
					}
				}
				pDataStart += (nArgsLen + 1);
				nBytes -= (nArgsLen + 1);
				strArgSizes << nBytes << "|" << nNewArgsLen;
			}
			
			// parse out the arguments supplied from the browser IN EXECUTION ORDER unlike the
			// POST logic, while under the GET logic the order that arguments are supplied in MATTERS.
			else 
			{
				char pzDelimit[2];
				pzDelimit[0] = pDataStart[0]; // set the user defined delimiter byte
				pzDelimit[1] = 0;
				GStringList lstHTMLArgs(pzDelimit,&pDataStart[1]);
				
				if (nReqArgCount > lstHTMLArgs.Size())
				{
					// todo: error
				}

				GStringIterator itHTMLArgs(&lstHTMLArgs);
				int nCurrentIndex = 0;
				while(itHTMLArgs())
				{
					int nNewArgLen = unEscapeData(itHTMLArgs++,&sockBuffer[nCurrentIndex]);
					nCurrentIndex += nNewArgLen + 1; // size plus NULL
					if (strArgSizes.Length())	
						strArgSizes << "|";
					strArgSizes << (nNewArgLen+1);
				}
				pDataStart = sockBuffer;
			}

			int nOutResultSize = 0;
			GString strRslt;
			pII->SetOption("rsltGStr",&strRslt);
			const char *pzResults = pII->InvokeEx( pzComponent, pzInterface, pzMethod, pDataStart,strArgSizes,&nOutResultSize);
			if (stristr(pzPIBuf,"DataSent") != 0)
			{
				// the plugin sent the data directly
				return 1; // HTTP request handled.
			}
			if (nOutResultSize < 1 || stristr(pzPIBuf,"NoKeepAlive") != 0)
			{
				*pnKeepAlive = 0;
				return 1; // HTTP request handled, no results from user created handler.
			}

			// currently this is the only processing instruction available, in the future this will be a pipe separated list.
			// todo: add processing instruction support "NoKeepAlive"
			if (stristr(pzPIBuf,"HasHeaders") != 0)
			{
				if (nOutResultSize)
				{
					if (nonblocksend(td->sockfd,pzResults,nOutResultSize) == nOutResultSize)
					{
						*nTotalProxiedBytes += nOutResultSize;
					}
				}
			}
			else
			{
				// todo: support multiple content types by setting "Ext", currently NULL is "text/html"
				BuildHTTPHeader(sockBuffer, 0, strConnectionType, nIsKeepAlive, *pnKeepAlive, 0/*ETag*/, 0 /*Ext*/, nOutResultSize, -1, -1);
				int nHL = strlen(sockBuffer);

				if (nHL + nOutResultSize < MAX_DATA_CHUNK)
				{
					memcpy(&sockBuffer[nHL],pzResults,nOutResultSize);
					if (nonblocksend(td->sockfd,sockBuffer,nOutResultSize+nHL) == nOutResultSize+nHL)
					{
						*nTotalProxiedBytes += nOutResultSize+nHL;
					}
				}
				else
				{
					if (nonblocksend(td->sockfd,sockBuffer,nHL) == nHL)
					{
						*nTotalProxiedBytes += nHL;
						if (nonblocksend(td->sockfd,pzResults,nOutResultSize) == nOutResultSize)
						{
							*nTotalProxiedBytes += nOutResultSize;
						}
					}
				}
			}
			return 1;
		}
	}
	return 0;
}



void *ProxyHelperThread(void *arg)
{
	ThreadData *td = (ThreadData *)arg;

	CipherData *pcdToServer = 0;
	CipherData *pcdToClient = 0;

	int nBufBytes;
	int nPacketIndex;
	int nPacketLen;
	int nCloseCode;
	int nReadWait;
	int nIdleReads;
	char *pPacket;
	const int off = 0;
	int nFirstThreadUse = 1;

	GStringList lstRollOver;
	GString strMessagingSender;

	char *cryptDest = 0;
	char *sockBuffer = 0;
	if (g_nLockThreadBuffers) 
	{
		// this makes the allocation before the transaction begins, making the txn a little faster
		int n = MAX_DATA_CHUNK;
		cryptDest = new char[(n * 2) + 1024];
		sockBuffer = &cryptDest[n + 512];

		sockBuffer[0] = 0;
		cryptDest[0] = 0;
		
		// The only external reference of this memory is in KillTid(), search this file for KillTidMemoryNote
		td->Buf1 = cryptDest;
		td->Buf2 = sockBuffer;
	}

	struct sockaddr_in their_addr; 
	their_addr.sin_family = AF_INET;
	
	// seed the random number generator once on each thread with a unique seed.
	g_nRrandSeed += (777 * (g_nRrandSeed % 100));
	srand( g_nRrandSeed );

BACK_TO_THE_POOL:

	nBufBytes = 0;
	nPacketIndex = -1;
	nPacketLen = 0;
	pPacket = 0;
	nCloseCode = 2100; // errors in 21nn range are Secondary Thread errors, started by the primary thread (range [11nn - 12nn] for this connection
	nReadWait = g_nProxyReadWait;
	nIdleReads = 0;
	
	if (!nFirstThreadUse)
	{
		#ifdef _WIN32
			InterlockedDecrement(&g_nProxyThreadsInUse);
		#else
			g_nProxyThreadsInUse--;
		#endif
	}

	// ----------------------------------------------------------------------
	// wait here until the clientthread wakes up this thread.
	// ----------------------------------------------------------------------
	if ( td->m_bUseThreads )
	{
		td->nThreadIsBusy = 0;
		gthread_cond_wait(&td->cond, &td->lock); 
	}
	
	td->starttime = getTimeMS();
	if (g_ServerIsShuttingDown)
	{
		PortableClose( *(td->pnsockfd), "Core8" );

		if (g_nLockThreadBuffers == 0 && cryptDest)
		{
			delete cryptDest;
			td->Buf1 = 0;
			td->Buf2 = 0;
		}
		gthread_exit(0);
		return 0;
	}


	#ifdef _WIN32
		InterlockedIncrement(&g_nProxyThreadsInUse);
	#else
		g_nProxyThreadsInUse++;
	#endif
	if (g_nProxyThreadsInUse > g_nProxyThreadsPeakUse)
		g_nProxyThreadsPeakUse = g_nProxyThreadsInUse;

	nFirstThreadUse = 0;


	if ( g_ThreadPing )
	{
		GString strLog;
		strLog.Format("H%d",(int)td->nThreadID);
		InfoLog(65,strLog);

		goto BACK_TO_THE_POOL;
	}
	
	if (cryptDest == 0)
	{
		int n = MAX_DATA_CHUNK;
		cryptDest = new char[(n * 2) + 1024];
		sockBuffer = &cryptDest[n + 512];

		sockBuffer[0] = 0;
		cryptDest[0] = 0;
		// The only external reference of this memory is in KillTid(), search this file for KillTidMemoryNote
		td->Buf1 = cryptDest;
		td->Buf2 = sockBuffer;
	}


	// sAttribs is the 16 bit flag field indicating data attributes
	int bReadHTTP;
	unsigned short sAttribs = BuildAttributesHeader(td->pTSD->szConfigSectionName,td->pTSD->nProtocol, td->nAction, &bReadHTTP, 0);
	int bIsTunnel = td->pTSD->bIsTunnel;


	const char *pKey128 = 0;
	if ( sAttribs & ATTRIB_CRYPTED_2FISH )
	{
		pKey128 = GetProfile().GetString(td->pTSD->szConfigSectionName,"CipherPass",false);
	}
	CipherData en(pKey128,DIR_ENCRYPT);
	CipherData de(pKey128,DIR_DECRYPT);

	if ( sAttribs & ATTRIB_CRYPTED_2FISH )
	{
		if (bIsTunnel)
		{
			pcdToClient = &de;
			pcdToServer = &en;
		}
		else
		{
			pcdToClient = &en;
			pcdToServer = &de;
		}
	}
	
	// determine the destination server.
	GString strProxyToServer;
	int nProxyToPort = td->pTSD->nProxyToPort;
	if (td->pTSD->nProtocol == 3)
	{
		char *pSep = strpbrk(td->pzIP,"@");
		if ( pSep )
		{
			strProxyToServer = pSep + 1;
			*pSep = 0;
			nProxyToPort = atoi(td->pzIP);
			*pSep = '@';
		}
		else
		{
			strProxyToServer = td->pzIP;
			nProxyToPort = 10777;
		}
	}
	// standard HTTP proxy
	else if( td->pTSD->nProtocol == 11 )
	{
		char *pSep = strpbrk(td->pzConnectRoute,"@");
		if ( pSep )
		{
			*pSep = 0;
			nProxyToPort = atoi(td->pzConnectRoute);
			strProxyToServer = pSep + 1;
			*pSep = '@';
		}
		else
		{
			strProxyToServer = td->pzConnectRoute; 
			nProxyToPort = 80; 
		}

	}

	else if (td->pTSD->nProtocol == 1 ) // HTTP messaging proxy
	{
		strProxyToServer = td->pzIP; // always a "~Name"
	}
	else
	{
		nProxyToPort = td->pTSD->nProxyToPort;
		strProxyToServer = td->pTSD->szProxyToServer; // default

		if (td->pTSD->szConfigSectionName && td->pTSD->szConfigSectionName[0])
		{
			// parse the comma separated "on fail to connect - roll over list"
			const char *pzRollOver = GetProfile().GetString(td->pTSD->szConfigSectionName,"RollOver",false);
			lstRollOver.DeSerialize(",",pzRollOver);

			// See if there is a 'load balanced' connection order specified:
			//
			// [Proxy2]
			// Balance=50,www.Domain.com,25,www.UBTInc.com,25,www.5Loaves.com
			// 
			// then the "RemoteMachine" is overridden with 50% routed to www.Domain.com
			// 25% routed to www.UBTInc.com and 25% routed to www.5Loaves.com
			const char *pzBalance = GetProfile().GetString(td->pTSD->szConfigSectionName,"Balance",false);
			// strProxyToServer will be changed to the new randomly balanced location.
			GetBalanceTo(pzBalance, strProxyToServer);
		}
	}
	
	int nConnectRetries = 0;

ROLL_OVER_CONNECT:	
	if (strProxyToServer.IsEmpty())
	{
		*td->pnProxyClosed = 1;
		*(td->pnProxyfd) = -1;					
		gthread_cond_signal( td->pcondHelper ); 
		nCloseCode = 21011;
		goto THREAD_EXIT;
	}
	
	// DoConnect() returns the new socket handle.  Inside our ThreadData(td), we have a pointer to an integer inside another ThreadData on the clientthread, 
	// so we dereference it and assign its value here.  Both threads use the same handle, rather than either one getting a copy of it - so any thread can close it and set it to -1
	*(td->pnProxyfd) = DoConnect(strProxyToServer, nProxyToPort,td->nParentThreadID, td->nThreadID); // in ProxyThread

	if ( *(td->pnProxyfd) == -1)
	{
		// roll over to a backup server if configured
		if (lstRollOver.Size())
		{
			if(g_TraceConnection)
			{
				InfoLog(66,"Rollover Connection");
			}
			strProxyToServer = lstRollOver.RemoveFirst();
			goto ROLL_OVER_CONNECT;
		}
		else
		{
			if (nConnectRetries++ < 3) // this is necessary but maybe only 1 or two retries is necessary
			{
				if(g_TraceConnection)
				{
					GString strInfo;
					strInfo.Format("*******RETRY #%d: Outbound[%d:%s]---(tid:%d<->%d)",(int)nConnectRetries, (int)nProxyToPort,strProxyToServer._str, (int)td->nParentThreadID, (int)td->nThreadID);
					InfoLog(67,strInfo);
				}
				PortableSleep(0,250000);
				goto ROLL_OVER_CONNECT;
			}
		}

		*td->pnProxyClosed = 1;
		*(td->pnProxyfd) = -1;					
		gthread_cond_signal( td->pcondHelper ); 
		nCloseCode = 2101;

		goto THREAD_EXIT;
	}
	*td->pnProxyClosed = 0;

    // turn the default OS kernel buffering off - because we handle it more efficiently ourself.
	if (g_StopDoubleBuffer)
	{
		setsockopt( *(td->pnProxyfd), SOL_SOCKET, SO_SNDBUF, (char *)&off, sizeof(off));
		int one = 1;
		setsockopt( *(td->pnProxyfd), IPPROTO_TCP, TCP_NODELAY, (char *)&one, sizeof(one));
	}
	
	// Notify the base thread(clientthread) that the connection is ready
	gthread_cond_signal( td->pcondHelper ); 



	// ProxyHelperThread main loop - reading from [strProxyToServer]
	while( !(*td->pnProxyClosed) && *(td->pnProxyfd) != -1 )
	{
		// if what we are about to recv() is expected to be (bCipher || bZip || bSync)
		int bPacketSync = ( bIsTunnel && sAttribs ) ? 1 : 0;
		
		// prepare args for the call to ReadPacket()
		if (bReadHTTP) // this can be removed - producing no change in server behavior because we can proxy HTTP data without regard to its HTTP format - we treat it as if we didnt know it was HTTP
		{
			bReadHTTP = 0;  // the base thread (aka clientthread) is reading HTTP, this thread could too but does not since it's faster/more efficient to proxy data as it is available in the TCP buffers as opposed to all that HTTP parsing.
			bPacketSync = 0; 
		}
		unsigned short sPktAttributes = 0;

		int rslt = 0;
		if( td->pTSD->nProtocol == 11 ) // ReadPacket will work - this is faster
		{
			rslt = nonblockrecvAny ( *(td->pnProxyfd) , sockBuffer, MAX_DATA_CHUNK, 1 );
			pPacket = sockBuffer;
			nPacketLen = (rslt > 0) ? rslt : 0;
		}
		else
		{
			rslt = ReadPacket( *(td->pnProxyfd) ,0,nReadWait,&nBufBytes,sockBuffer,&pPacket,&nPacketLen, &nPacketIndex,&sPktAttributes, sAttribs, bPacketSync, bReadHTTP);
		}
		if (rslt > 0)
		{
			td->nTotalBytesRead += nPacketLen;
		}
		if ( *td->pnProxyClosed || (td->pTSD->nProtocol == 11 && !g_bHTTPProxyEnabled) )
		{
			// this thread was ended by clientthread() when *td->pnProxyClosed became true
			// or by stopping "protocol 11' proxy.
			nCloseCode = 2103; 
			goto THREAD_EXIT;
		}
		if (rslt == 0)
		{
			if (nIdleReads++ > g_nProxyIdleAt)
			{
				nReadWait = g_nProxyIdleReadWait; // slow this thread down until more data shows up - this needs to be researched further for greater optimization 
			}
			// no data or only a partial packet available - nothing to do.
			if (*td->plLastActivityTime)
			{
				if ( (time(0) - *td->plLastActivityTime ) > (unsigned long)td->pTSD->nProxyTimeout )
				{
					GString strInfo;
					strInfo << "*******[" << GetProtocolName( td->pTSD->nProtocol ) << "] inactivity timeout for[" << strProxyToServer << "] tids:(" << td->nThreadID << " & " << td->nParentThreadID << ") fds[" << *(td->pnsockfd) << " & " << *(td->pnProxyfd) << "] Timeout Value:" << td->pTSD->nProxyTimeout << " seconds, see config file section [" << td->pTSD->szConfigSectionName << "]";
					InfoLog(68,strInfo);

					nCloseCode = 2104; 
					goto THREAD_EXIT;
				}
			}
			else
			{
				*td->plLastActivityTime = time(0);
			}
			if ( *(td->pnsockfd) == -1 )
			{
				// if the client TCP socket has been closed, we should quit reading from the server
				nCloseCode = 2198; 
				goto THREAD_EXIT;
			}
			if ( *td->pnProxyClosed )
			{
				// if the proxy is ending for any reason, we should quit reading from the server
				nCloseCode = 2199; 
				goto THREAD_EXIT;
			}

		}
		else if (rslt == -1) // failure during wait (connection closed by server)
		{ 
			*td->pnProxyClosed = 1;
			nCloseCode = 2105; 
			goto THREAD_EXIT;
		}
		else // process this packet on the Proxy Helper thread
		{
			*td->plLastActivityTime = time(0);
			nIdleReads = 0;
			nReadWait = g_nProxyReadWait;

			char *pSendData = pPacket;
			int nSendDataLen = nPacketLen;
			if (g_DataTrace)
			{
				GString strLabel; 
				strLabel.Format("tid%d rx from s[%s]",(int)td->nThreadID,(const char *)strProxyToServer);
				DataTrace(strLabel, pSendData, nSendDataLen);
			}

			// this says, if any of the packet attribute bits are on, bCipher || bZip || bSync
			if ( sAttribs ) 
			{
				int newLength;
				if (!bIsTunnel) // if we are about to encrypt/compress data from the application to send into the tunnel
				{
					cryptDest += 4; // move the pointer ahead 4 bytes where we will put the header
				}

				// compress/crypt or decompress/decrypt depending on bIsTunnel
				if ( PrepForClient(sAttribs,bIsTunnel,pcdToClient,pSendData,nSendDataLen,cryptDest,&newLength) )
				{
					// if (sending to client tunnel) add the attribute header otherwise sending to client application 
					if (!bIsTunnel) 
					{
						cryptDest -= 4; // move the pointer back 4 bytes to make room for the header
						unsigned short sh = htons(newLength); // packet length in network byte order
						memcpy(&cryptDest[0],&sh,sizeof(short));
						sh = htons(sAttribs); // attributes in network byte order
						memcpy(&cryptDest[2],&sh,sizeof(short));
						newLength += 4; // account for the header in the packet size
					}
					pSendData = cryptDest;
					nSendDataLen = newLength;
				}
				else
				{
					*td->pnProxyClosed = 1;
					nCloseCode = 2106; 
					goto THREAD_EXIT;
				}
			}


			if (*td->pnProxyClosed)
			{
				nCloseCode = 2107; 
				goto THREAD_EXIT;
			}

			// log what we will transmit to client
			if (g_bTraceHTTPProxyReturn || g_bTraceHTTPProxyBinary)
			{
				pSendData[nSendDataLen] = 0;

				GString strG("HTTP-PROXY-RETURN(send_to_fd:");
				strG << *(td->pnsockfd) << " Action=[" << td->strAction << "] from tid:" << td->nThreadID  << " to tid:"<< td->nParentThreadID <<") Returning " << nSendDataLen << " bytes from fd:" << *(td->pnProxyfd) << " [" << nProxyToPort << "@" << strProxyToServer << "] per request[" << td->strAction << "] Data Sent[";
				strG.Write(pSendData, (nSendDataLen > 512) ? 512 : nSendDataLen );
				strG << "]";
				strG.ReplaceChars("\r\n",'~');
				if(g_bTraceHTTPProxyBinary)
				{
					strG << "\r\n";
					strG.FormatBinary( (unsigned char *)pSendData, nSendDataLen, 1, "\t\t\t\t");
				}
				InfoLog(69,strG);
			}

			//
			// in ProxyHelperThread, search for:       g_bTraceHTTPProxyFiles        to see the mirror implementation.
			//
			if ( g_bTraceHTTPProxyFiles ) 
			{
				GString strFileName(td->strAction);
				if (strFileName._len > 255)
				{
					strFileName.SetLength(255);
				}
				// none of those are allowed in a file name
				strFileName.ReplaceChars("\\/:*?\"<>|",'~');
				strFileName.Prepend(strProxyToServer);
				strFileName.Prepend("@");
				strFileName.Prepend((__int64)nProxyToPort);
				strFileName << ".IN.txt";
				
				GString strPath(g_strLogFile);				  // start with the logfile name and path
				strPath.SetLength(strPath.ReverseFindOneOf("/\\")); // cut off the file name for either a windows or unix path
				strPath << g_chSlash << "transactions" << g_chSlash;

				// make the destination directory if it does not exist
				#ifdef _WIN32
					mkdir( strPath );
				#else
					mkdir( strPath ,777 );
				#endif
				
				// concat the two strings
				strPath << strFileName;

				// attach a GString object over the send buffer
				GString g(pSendData,nSendDataLen,nSendDataLen,0);

				g.ToFileAppend(strPath);
			}
			
			////////////////////////////////////////////////////////////////////////////
			//
			//
			//							transmit to client
			//
			//
			int nActualSent = nonblocksend( *(td->pnsockfd) ,pSendData, nSendDataLen);
			//
			//
			////////////////////////////////////////////////////////////////////////////
			if (nActualSent != nSendDataLen)
			{
				// the client has disconnected
				if (g_bHTTPProxyLogExtra)
				{
					GString g;
					pSendData[nSendDataLen] = 0;
					g << "Client fd:" << *(td->pnsockfd) << " has disconnected. SOCK_ERR=" << SOCK_ERR << " The server fd:" << *(td->pnProxyfd) << " outbound to:[" << nProxyToPort << "@" <<  strProxyToServer << "] maybe can be cached,  It failed to send " << nSendDataLen << " bytes[" << pSendData << "]";
					g.ReplaceChars("\r\n",'~');
					InfoLog(21088,g);
				}
				
				// if the client disconnected and this is an entire HTTP transaction with no Content-Length 
				// OR with Content-Length and the entire content included, then we can cache this outbound connection rather than close it.
				// First, see if the port is 80, and the data starts with HTTP/1
				if (g_bEnableOutboundConnectCache && nProxyToPort == 80 && memcmp(pSendData, "HTTP/1.", 7) == 0 )
				{
					pSendData[nSendDataLen]	= 0;
					char *prnrn = (char *)strstr(pSendData, "\r\n\r\n");
					if (prnrn)
					{
						int nCL = 0;
						char *pCL = stristr(pSendData,"Content-Length:");
						if (pCL)
						{
							pCL += 15; // first byte of content length in the HTTP header
							nCL = atoi(pCL);
						}
						if ( nSendDataLen == (prnrn - pSendData) + (4 + nCL) ) // 4 is the length of "\r\n\r\n"
						{
							// cache it

							// the key is fully qualified with the port in this form "80@www.google.com"
							// since 0 is a valid socket handle, add 1 to it before we cache it, then if we find a 0 it always means "no entry exists in cache"
							GString strKey;	
							strKey << nProxyToPort << "@" << strProxyToServer;
							strKey.MakeLower();
							gthread_mutex_lock(&g_csMtxConnectCache);
							g_OutboundConnectCache.Insert(strKey, (void *)( *(td->pnProxyfd) + 1 ) );
							gthread_mutex_unlock(&g_csMtxConnectCache);

							// disconnect this socket handle from the ProxyHelperThread and return the thread to the pool
							// search this file for:   777 signifies a "detach"
							*td->pnProxyClosed = 777;
							nCloseCode = 2121;
							goto THREAD_EXIT;
						}
						else
						{
							// trash it - falls through to goto THREAD_EXIT

						}
					}
				}

				nCloseCode = 2108; 
				goto THREAD_EXIT;
			}
			td->nTotalBytesProxied += nSendDataLen;
			
			// log the send event from helper thread - sending to the "Client"
			if ( td->pTSD->bIsLogging )
			{
				GString str("Transmit to Client ");
				str << td->pzClientIP << " [" << *(td->pnsockfd) << "] per request [" << td->strAction << "]";
				ProxyLog(td->pTSD->strLogFile, td->nParentThreadID, pSendData, nSendDataLen, str);
			}
			if (g_DataTrace)
			{
				GString strLabel;
				strLabel.Format("tid%d tx to c",(int)td->nThreadID);
				DataTrace(strLabel, pSendData, nSendDataLen);
			}

			nBufBytes = 0;
			nSendDataLen = 0;
			pSendData[0] = 0;
		}
	} 
THREAD_EXIT:
	char szAction[12];
	strcpy(szAction,"Closed");
	int nWasfd = *(td->pnProxyfd); // copy nWasfd for logging

	// 777 signifies a "detach" so the connection must not be closed.   search this file for:   777 signifies a "detach"
	if ( *td->pnProxyClosed != 777 ) 
	{
		// In many cases *td->pnProxyClosed was already set to '1' a few steps earlier, do not assume it was '0' at this point
		*td->pnProxyClosed = 1; // 1 == Shut down in progress.
		
		// reset per connection
		td->nTotalBytesProxied = 0; 
		td->nTotalBytesRead = 0; 
		td->nTotalBytesSent = 0; 

		PortableClose( *(td->pnProxyfd) ,"Core9");
		*(td->pnProxyfd) = -1; // since we closed it, make the handle invalid in our shared memory
	}
	else
	{
		strcpy(szAction,"Cached");
	}

	if(g_TraceConnection && !g_ServerIsShuttingDown)
	{
		GString strTime;
		int nElapsed = getTimeMS() - td->starttime;
		if (nElapsed > 300000)
			strTime << (nElapsed / 1000) / 60 << "min"; // minutes over 5
		else if (nElapsed > 5000 && nElapsed < 300000)
			strTime << nElapsed / 1000 << "sec"; // seconds between 5 seconds and 300 seconds
		else
			strTime << nElapsed << "ms"; // milliseconds less than 5000
		
		// note: on Windows - transactions that complete in the < 50 millisecond range on slower CPU's have been seen to report an elapsed time of 0.
		// Use GPerformanceTimer in GPerformanceTimer.h if you want to count clock ticks per transaction.  
		GString strInfo;
		strInfo.Format("%s Outbound [%d:%s] (on tid:%d fd:%d  %s) code[%d:%d] bytes sent to client=%I64d",szAction,(int)nProxyToPort,(const char *)strProxyToServer,(int)td->nThreadID,(int) nWasfd  ,strTime.Buf(),(int)nCloseCode,(int)SOCK_ERR,(__int64)td->nTotalBytesProxied);
		strInfo << " (client fd:" << *(td->pnsockfd) << " tid:" << td->nParentThreadID  << ")  read from server=" <<  td->nTotalBytesRead;

		InfoLog(70,strInfo);
	}


	*td->pbHelperThreadIsRunning = 0; // helper shutdown complete.

	if (g_nLockThreadBuffers == 0 && cryptDest)
	{
		delete cryptDest;
		cryptDest = 0;
		sockBuffer = 0;
		td->Buf1 = 0;
		td->Buf2 = 0;
	}
 	else
	{
		sockBuffer[0] = 0;
		cryptDest[0] = 0;
	}

	if (g_ServerIsShuttingDown)
	{
		gthread_exit(0);
	}
	else
	{
		*td->pnProxyClosed = 2; // 2 = Closed and in pool.
		goto BACK_TO_THE_POOL;
	}
	return 0;
}





int stopListeners(int nPort = -1)
{
	// stop the listeners
	int nRet = 0;
	ThreadStartupData *pTSD = &g_TSD;
	while(pTSD)
	{
		if (nPort == -1 || nPort == pTSD->nPort)
		{
			pTSD->bThreadShuttingDown = 1;
			nRet++;
		}
		pTSD = pTSD->Next();
	}
	return nRet;
}

// nAction  1=Create socket and Bind  2=Start Listener Thread  3=Both
int startListeners(int nAction, int nPort = -1) 
{
	int nRet = 0; 

	ThreadStartupData *pTSD;
	struct sockaddr_in  serv_addr;

	// create and bind to a socket on port(s) specified that this server handles
	if (nAction == 1 || nAction == 3)
	{
		pTSD = &g_TSD;
		while(pTSD)
		{
			if (nPort == -1 || nPort == pTSD->nPort)
			{
				if (pTSD->nPort > -1)
				{
					if((pTSD->sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
					{
						pTSD->nState = -1; 
						InfoLog(71,"Can't open socket. Check permissions. ");
						return 0;
					}
					memset((char *) &serv_addr, 0, sizeof(serv_addr));
					serv_addr.sin_family = AF_INET;
					serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
					serv_addr.sin_port = htons( pTSD->nPort );
					
					if(bind(pTSD->sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
					{
						pTSD->nState = -2;
						GString strErr;																// that big arrow makes this stick out in a log file
						strErr.Format("Cannot use port[%d] (try [netstat -o] or [netstat -lp | grep %d]  ---------------------------->  N\\A[%d]",(int)pTSD->nPort, (int)pTSD->nPort,(int)pTSD->nPort);
						InfoLog(72,strErr);
					}
					else
					{
						GString strInfo;
						strInfo.Format("Listening on port[%d]",(int)pTSD->nPort);
						InfoLog(73,strInfo);
						pTSD->nState = 1; // ready
					}

					setsockopt(pTSD->sockfd,SOL_SOCKET,SO_RCVBUF,(char *)&g_nKernelSocketRcvBuf, sizeof(g_nKernelSocketRcvBuf));

					nRet++;
				}
			}
			pTSD = pTSD->Next();
		}
	}
	if (nAction == 2 || nAction == 3)
	{
		// create a listener thread for each port we handle
		pTSD = &g_TSD;
		while(pTSD)
		{
			if (nPort == -1 || nPort == pTSD->nPort)
			{
				if (pTSD->nState == 1) // we've already done a bind()
				{
					pTSD->bThreadShuttingDown = 0;
					gthread_t listen_thr;


					gthread_create(&listen_thr,	NULL, listenThread, (void *)pTSD );
					
				}
			}
			if (nAction == 2 )
			{
				nRet++;
			}

			pTSD = pTSD->Next();
		}
	}
	return nRet;
}


#ifdef ___XFER
	#include "../../Libraries/Xfer/Core/XferLocalCommands.cpp"
#endif
#ifdef __CUSTOM_CORE__
	#include "ServerCoreCustomGlobalInternals.cpp" 
#endif


class MessagingArgs
{
public:
	GString strMessagingHeader;
	GString strMessagingSender;
	GString strMessagingReceiver;
	GString strMessagingDataInfo;
	GString strMessagingTitle;
	GString strMessagingHash;
};


// returns 1 through 7  for RFC 2616 verbs that DO NOT need HTTP Proxy translation
// returns 8 through 15 for RFC 2616 verbs that need HTTP Proxy translation
// otherwise returns 0 
int isHTTPVerb(char *pSendData)
{
	// Note: this is all the verbs from RFC 2616		// is this uppercase HTTP check necessary?  ever?
	if ( memcmp(pSendData,"GET http://",	11) == 0	|| memcmp(pSendData,"GET HTTP://",		11) == 0 )
		return 8;
	if ( memcmp(pSendData,"POST http://",	12) == 0	|| memcmp(pSendData,"POST HTTP://",		12) == 0 )
		return 9;
	if ( memcmp(pSendData,"HEAD http://",	12) == 0	|| memcmp(pSendData,"HEAD HTTP://",		12) == 0 )
		return 10;
	if ( memcmp(pSendData,"OPTIONS http://",15) == 0	|| memcmp(pSendData,"OPTIONS HTTP://",	15) == 0 )
		return 11;
	if ( memcmp(pSendData,"PUT http://",	11) == 0	|| memcmp(pSendData,"PUT HTTP://",		11) == 0 )
		return 12;
	if ( memcmp(pSendData,"DELETE http://",	14) == 0	|| memcmp(pSendData,"DELETE HTTP://",	14) == 0 )
		return 13;
	if ( memcmp(pSendData,"TRACE http://",	13) == 0	|| memcmp(pSendData,"TRACE HTTP://",	13) == 0 )
		return 14;
	if ( memcmp(pSendData,"CONNECT ", 8) == 0 )
		return 15;

	// this is all the verbs from RFC 2616 (again minus CONNECT)
	if ( memcmp(pSendData,"GET ",	4) == 0 )
		return 1;
	if ( memcmp(pSendData,"POST ",	5) == 0 )
		return 2;
	if ( memcmp(pSendData,"HEAD ",	5) == 0 )
		return 3;
	if ( memcmp(pSendData,"OPTIONS ",8)== 0 )
		return 4;
	if ( memcmp(pSendData,"PUT ",	4) == 0 )
		return 5;
	if ( memcmp(pSendData,"DELETE ",7) == 0 )
		return 6;
	if ( memcmp(pSendData,"TRACE ",	6) == 0 )
		return 7;


	// Note: these are all the verbs from RFC 2518
	// PROPFIND  PROPPATCH   MKCOL  COPY  MOVE  LOCK  UNLOCK

	return 0;
}





// Labels in clientthread() are like 'functions', set the proper arguments then goto...
//----------------------------------------------------------------------------------------------------
// HTTP_PROXY_KEEP_ALIVE:
//----------------------------------------------------------------------------------------------------
// this assumes [nInitialRawSend] is != 0, it is the bytes in [sockBuffer] which contains the data
// HTTP_PROXY_CONVERT:
//----------------------------------------------------------------------------------------------------
// this assumes TCD already has the server and port set in:
//	memcpy(TCD.pzConnectRoute, ... ); // copy the null
//	TCD.nConnectRouteSize = ? ;
// this ALSO assumes any header/markup is already prepared in pInitialProxySend if nInitialRawSend is != 0			
// HTTP_PROXY_RE_CONNECT:
//----------------------------------------------------------------------------------------------------
// this assumes any header/markup is already prepared in pInitialProxySend if nInitialRawSend is != 0
// HTTP_PROXY_ALREADY_CONNECTED: 
//----------------------------------------------------------------------------------------------------
// goes to Branch 2 - reads HTTP headers, and generates an HTTP response
// KEEP_ALIVE:
//----------------------------------------------------------------------------------------------------
// HANDLER_FINISH: // legacy - may goto KEEP_ALIVE
//----------------------------------------------------------------------------------------------------
// CLOSE_CONNECT:  // closes connection
//----------------------------------------------------------------------------------------------------
// SHUTTING_DOWN:  // internal used when the process is ending
//----------------------------------------------------------------------------------------------------


// clientThread() thread services all connection types.
void *clientThread(void *arg)
{
	ThreadData *td = (ThreadData *)arg;

//	if (g_TraceThread)
//	{
//		GString strThreadTrace;
//		strThreadTrace << "++++Created new clientThread   tid:" << td->nThreadID;
//		InfoLog(111,strThreadTrace);
//	}

	unsigned long lBytesRemaining;
	char *pCommandParam;
	const char *pSent = 0;
	int nYielded = 0;
	int bHelperThreadIsRunning = 0;
	int nCloseCode = 1100; // default close code for Branch 1   error range 11nn(1st thread, 1nd branch)
	int nInitialRawSend = 0;
	char *pInitialProxySend = 0;
	const int off = 0;
	int nFirstThreadUse = 1;

		
	int bisHead = 0;  // is the HTTP HEAD verb
	unsigned int nKeepAlive = g_nMaxKeepAlives; // counts down to 0 with each kept alive transaction

	MessagingArgs MA;
	ThreadConnectionData TCD;

	

	// KillTidMemoryNote:
	// -------------------------
	// A reference to this threads memory is stored external to the thread.  ServerCore does not make other heap 
	// allocations.  If this thread was to be killed by the administrator there will be no memory loss.
	// Keep in mind that this thread services many different kinds of protocols each protocol may or may not
	// make it's own heap allocations that an untimely 'kill' would leave 'lost' until the whole process ends.
	// Future development should maintain the guarantee that no heap allocations are made at the 
	// 'system transactional level' but rather only in the protocol implementation, for example a plugin may make
	// heap allocations (on this thread) then should the thread be killed the admin may have the peace of
	// mind that any ill effect of the kill is localized to the logic of the plugin.

	char *sockBuffer = 0;
	char *cryptDest = 0;
	if (g_nLockThreadBuffers) 
	{
		// make the allocation before the transaction begins, making the txn a little faster
		int n = MAX_DATA_CHUNK;
		cryptDest = new char[(n * 2) + 1024];	// note: only 1 allocation for both buffers
		sockBuffer = &cryptDest[n + 512];

		sockBuffer[0] = 0;
		cryptDest[0] = 0;
		
		// external reference of this memory is in KillTid() and in the plugins, search this file for KillTidMemoryNote
		td->Buf1 = sockBuffer;
		td->Buf2 = cryptDest;
	}


	MessagingArgs *pMA = &MA;


	// stream read in (used for HTTP & TXML requests only)
	SERVER_MEMORY_EXTENSION;


	// seed the system random number generator on each thread with a unique number
	g_nRrandSeed += (777 * (g_nRrandSeed % 100)) ;
	srand( g_nRrandSeed );


	GString strConnectedIPAddress;


	int nBufBytes;
	int nPacketIndex;
	int nPacketLen;
	int nProxyMaxBytes;

	GString strCertifiedMsgResponse;
	GString strNoProxyResponse;
	bool bConnectionAuthorized = 0;

	char *pPacket;
	unsigned long lLastActivityTime;


	ExceptionHandlerScope durationOfThread;

MAIN_THREAD_BODY:

#ifdef _XMLF_JIT_
	XML_TRY		// note: comment this line out so the JIT debugging can record the call stack It should be in the final build
#endif
	{
		// pre-caches Transaction objects in the server build
		OBJECT_PRECACHE;

		pCommandParam = 0;
		lBytesRemaining = 0;
		pSent = 0;
		nYielded = 0;
		nBufBytes = 0;
		nPacketIndex = -1;
		nPacketLen = 0;
		pPacket = 0;
		lLastActivityTime = 0;
		bHelperThreadIsRunning = 0;
		nProxyMaxBytes = -1;
		
		if (g_TraceThread)
		{
			GString strLog;
			strLog.Format("Thread %d Waiting",(int)td->nThreadID);
			InfoLog(100,strLog);
		}

		if (!nFirstThreadUse)
		{
			#ifdef _WIN32
				InterlockedDecrement(&g_nClientthreadsInUse);
			#else
				g_nClientthreadsInUse--;
			#endif
		}
		// ----------------------------------------------------------------------
		// wait here until we accept a new connection and hand off to this thread
		// ----------------------------------------------------------------------
		if ( td->m_bUseThreads )
		{
			td->nThreadIsBusy = 0;
			gthread_cond_wait(&td->cond, &td->lock); 
		}

		// ----------------------------------------------------------------------
		// We have just begun to service a new request.  We are "On the clock" now.
		// ----------------------------------------------------------------------
		td->starttime = getTimeMS();
		if (g_ServerIsShuttingDown)
		{
			nCloseCode = 1101;
			goto SHUTTING_DOWN;
		}
		td->nSequence = 0;


	#ifdef _WIN32
		InterlockedIncrement(&g_nClientthreadsInUse);
	#else
		g_nClientthreadsInUse++;
	#endif
	if (g_nClientthreadsInUse > g_nClientthreadsPeakUse)
		g_nClientthreadsPeakUse = g_nClientthreadsInUse;

		nFirstThreadUse = 0;

		if ( g_ThreadPing )
		{
			GString strLog;
			strLog.Format("W%d",(int)td->nThreadID);
			InfoLog(101,strLog);
			goto MAIN_THREAD_BODY;
		}

		if (sockBuffer == 0)
		{
			int n = MAX_DATA_CHUNK;
			cryptDest = new char[(n * 2) + 1024];
			if (!cryptDest)
			{
				InfoLog(102,"********* Out of Memory *********");
			}
			sockBuffer = &cryptDest[n + 512];

			sockBuffer[0] = 0;
			cryptDest[0] = 0;
			// external reference of this memory is in KillTid() and in the plugins, search this file for KillTidMemoryNote
			td->Buf1 = sockBuffer;
			td->Buf2 = cryptDest;
		}


		// get the address of the connected client into strConnectedIPAddress;
		// the address as reported by the presumably unspoofed TCP/IP headers.
		strConnectedIPAddress = td->pzClientIP;


		if(g_TraceConnection)
		{
			GString strMessage;
			strMessage.Format("Inbound [%d:%s] (fd:%d tid:%d type:%s)",
				(int)td->pTSD->nPort,(const char *)strConnectedIPAddress,(int)td->sockfd, (int)td->nThreadID,GetProtocolName( td->pTSD->nProtocol ));
			InfoLog(103,strMessage);
		}
		if (g_TraceThread)
		{
			GString strLog;
			strLog.Format("Thread %d Working",(int)td->nThreadID);
			InfoLog(104,strLog);
		}



		//
	    // Turn the OS kernel buffering off - Search for "Turn off Nagle Algorithm"
		//
		if (g_StopDoubleBuffer)
		{
			setsockopt(td->sockfd, SOL_SOCKET, SO_SNDBUF, (char *)&off, sizeof(off));
			int one = 1;
			setsockopt(td->sockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&one, sizeof(one));
		}




		nProxyMaxBytes = -1;
		pInitialProxySend = 0;
		nInitialRawSend = 0;


		if (td->pTSD->nProtocol == 3) // Xfer or remote console
		{
#ifdef ___XFER
#include "../../Libraries/Xfer/Core/XferCoreIntegration.cpp"
#endif
		}


		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//		
		// Following this comment are the two main logic branches.
		// Branch 1 - forwards the request/data to another location (implements a proxy to a switchboard, the next hop, or to the final destination )
		// Branch 2 - reads HTTP headers, and generates an HTTP response (implements an an HTTP Server, or custom HTTP like server)
		//
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//  
		//										test to see if this protocol needs Branch 1
		// 
		// if ( tunnel(in)/proxy(out)  or        raw(bounce)        or   HTTP conversion bounce		or    PrivateSurf
		if (td->pTSD->nProtocol == 2  || td->pTSD->nProtocol == 3  ||   td->pTSD->nProtocol == 11   || td->pTSD->nProtocol == 9 || td->pTSD->nProtocol == 12)
		{ // Branch 1 scope is not tabbed in
			
		//////////////////////////////////////////////////////////////////////////		
		// Branch 1 - forwards the request/data to another location (switchboard, hop, or final destination)
		//////////////////////////////////////////////////////////////////////////		
		td->nProxyfd = -1;
		td->nProxyClosed = 0;	// it's not exactly open yet but it will be unless someone changes this flag to stop it
		td->pnsockfd = 0;		// on the clientthread() this is not used.

		// Tie the threadData(td) of 'this' thread to the thread that will proxy on our behalf
		TCD.pTSD = td->pTSD;
		TCD.sockfd = -1; // in TCD this -1 value can be anything. ProxyHelperThread gets it but does not use it.
		TCD.pnsockfd = &td->sockfd; // so in ProxyHelperThread td->pnsockfd points to the client socket handle
		


		TCD.pnProxyClosed = &td->nProxyClosed;
		TCD.pnProxyfd =		&td->nProxyfd;
		TCD.pnProxyFlags =	&td->nProxyFlags;

		TCD.pbHelperThreadIsRunning = &bHelperThreadIsRunning;

		TCD.pcondHelper = &td->condHelper;
		TCD.nParentThreadID = td->nThreadID;
		TCD.plLastActivityTime = &lLastActivityTime;
		strcpy(TCD.pzPollSectionName, td->pzPollSectionName);



		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// This Ciphered Tunnel Setup may be moved to an external #included hook file, until then the cipher structures are available to all protocols in Branch 1
		//
		// HOOK: Ciphered Tunnel Setup  
		//
		// the 16 bit flag field indicating data attributes
		int bReadHTTP;
		unsigned short sAttribs = BuildAttributesHeader(td->pTSD->szConfigSectionName, td->pTSD->nProtocol, TCD.nAction, &bReadHTTP, 1);
		int bIsTunnel = td->pTSD->bIsTunnel;

		const char *pKey128 = 0;
		if (  sAttribs & ATTRIB_CRYPTED_2FISH  )
		{
			pKey128 = GetProfile().GetString(td->pTSD->szConfigSectionName,"CipherPass",false);
		}
		int nDir = 0;
		if(bIsTunnel)
		{
			nDir = DIR_ENCRYPT;
		}
		else
		{
			nDir = DIR_DECRYPT;
		}
		CipherData cdToServer(pKey128,nDir);

		// assign the "pcd" so that worker thread can obtain a pointer to this 
		// key and use it rather than rebuilding the same key.  currently unused.
		TCD.pTSD->pcd = &cdToServer;	
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		

		
		////////////////////////////////////////////////////////////////////////////
		//
		//  Here we have a protocol initialized, and ready to go.  The sAttribs flag has been set by BuildAttributesHeader() and cipher keys have been prepared with the correct password.
		//
		//  Hook - protocol preprocessor   
		//
		////////////////////////////////////////////////////////////////////////////

		
		
		// HTTP Proxy Server
		int bIsCONNECT = 0;
		if ( td->pTSD->nProtocol == 11 )  
		{


			if (g_bHTTPProxyRestrictToSubnet)
			{
				bool bAllow = 0;
				GStringIterator itAllowSubNets( g_pstrHTTPProxyRestrictList );
				while ( itAllowSubNets() )
				{
					if ( strConnectedIPAddress.Find( itAllowSubNets++ ) == 0 ) // check multiple local subnets
					{
						bAllow = 1;
						break;
					}
				}
				if (!bAllow)
				{
					if ( g_bTraceHTTPProxy )
					{
						GString g("****Refused HTTP Proxy Service to:");
						g << strConnectedIPAddress <<" because the [HTTPProxy] RestrictBeginMatch=" << g_pstrHTTPProxyRestrictList->Serialize(",")  << "   Attempted:" << nInitialRawSend;
						InfoLog(10555,g);
					}
					nCloseCode = 11210; 
					goto CLOSE_CONNECT; 
				}
			}

			strNoProxyResponse =	   "HTTP/1.0 407 Proxy Authentication Required\r\n";
			strNoProxyResponse		<< "Server: " << g_strHTTPProxyAgentName << "\r\n";
			if (g_bNegotiateNTLMv2)
			{
				strNoProxyResponse  << "Proxy-Authenticate: Negotiate\r\n";
			}
			if (g_bNegotiateBasic)
			{
				strNoProxyResponse	<< "Proxy-Authenticate: Basic realm=\"" << g_strHTTPProxyAgentName << "\"\r\n";
			}
			strNoProxyResponse		<< "Connection: close\r\n"
									<< "Proxy-Connection: close\r\n\r\n";

			unsigned __int64 nChallenge;
			nChallenge = 0;

			long lDataWait;
			lDataWait = time(0);

HTTP_PROXY_KEEP_ALIVE:
			td->nSequence++;

		
			//////////////////////////////////////////////////////////
			// Begin Initial Read from the web browser or other client 
			//////////////////////////////////////////////////////////
			unsigned int nPktIndex;
			nPktIndex = 0;

			// recv() anything on the TCP buffers - Non Blocked I/O
			// return 0 when the time limit expired.
			// return -1 socket error
			// return +number of bytes read
KEEP_WAITING_FOR_DATA_FROM_SERVER:
			nInitialRawSend = nonblockrecvAny (td->sockfd, sockBuffer, MAX_DATA_CHUNK, 1 ); 
			if (nInitialRawSend == 0) 
			{
				if (time(0) - lDataWait > g_nAuthTimeoutSeconds)
				{
					if (g_bHTTPProxyLogExtra)
					{
						GString strError("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ Timed out waiting for client fd:");
						strError << td->sockfd << "Last Connect:[" << td->pzConnectRoute << "] Change g_nAuthTimeoutSeconds=" << g_nAuthTimeoutSeconds;
						InfoLog(976, strError);
					}
					// no data available within time expiration limit
					nCloseCode = 1102;
					goto CLOSE_CONNECT; 
				}
				else
				{
					// if the server closed the connection while we are waiting for data from the client
					if ( td->nProxyClosed )
					{
						// Either keep the client connection alive.....
						goto HTTP_PROXY_KEEP_ALIVE;
						//  or not?
						//nCloseCode = 1102;
						//goto CLOSE_CONNECT; 

					}
					goto KEEP_WAITING_FOR_DATA_FROM_SERVER;
				}
			}
			else if (nInitialRawSend < 1) 
			{
				// client closed connection 
				nCloseCode = 1121;
				goto CLOSE_CONNECT; 
			}

			///////////////////////////////////////////////////////////////////////////////
			// Completed Initial Read from the web browser or client, or a subsequent read
			//////////////////////////////////////////////////////////////////////////////
			else // process this packet
			{
				lDataWait = time(0);
				sockBuffer[nInitialRawSend] = 0;
				td->nTotalBytesRead += nInitialRawSend;

				pInitialProxySend = sockBuffer; // nInitialRawSend was set by ReadPacketHTTP()


				bool bIsAuthorized;
				bIsAuthorized = !g_bNeedHTTPProxyAuth;
				if (!bIsAuthorized)
				{
					bIsAuthorized = bConnectionAuthorized; // see if we already authorized this connection
				}
				
				if (!bIsAuthorized) 
				{ // this conditional scope is intentioanlly not tabbed in
		
				// look for "Authorization: Negotiate"
				char *pAuthStr;
				pAuthStr = (g_bNegotiateNTLMv2) ? stristr(pInitialProxySend,"Authorization: Negotiate ") : 0; 
				if (pAuthStr)
				{
					GString strAuthBlob;
					strAuthBlob.SetFromUpTo(pAuthStr + 25,"\r\n");  // strlen of "Authorization: Negotiate " is 25
					strAuthBlob.UUDecode();

					TypeMessageSwitch *pMsg0 = (TypeMessageSwitch *)(void *)strAuthBlob._str;
					if ( pMsg0->type == 1 )
					{

						Type1Message *pMsg1 = (Type1Message *)(void *)strAuthBlob._str;

						if (pMsg1->type != 1 || pMsg1->domain_len > 100 || pMsg1->host_len > 100 || pMsg1->domain_off > 100 || pMsg1->domain_off > 100)
						{
							// invalid message
							nCloseCode = 11333; 
							goto CLOSE_CONNECT; 

						}
						// Here is the domain,and host sent in Type1Message from the client
						GString strDomain1( strAuthBlob._str + pMsg1->domain_off, pMsg1->domain_len );
						GString strHost1  ( strAuthBlob._str + pMsg1->host_off,   pMsg1->host_len   );

						//
						// 10:11:35[123] HTTPProxy recv'd Type1Message (authentication message) from:  fd:21280  bytes:59 Domain:WORKGROUP Host:THEBOSS-PC pInitialProxySend=[GET http://www.bing.com/ HTTP/1.1~~Accept: text/html, application/xhtml+xml, */*~~Accept-Language: en-US~~User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64; Trident/7.0; rv:11.0) like Gecko~~Accept-Encoding: gzip, deflate~~Host: www.bing.com~~DNT: 1~~Proxy-Connection: Keep-Alive~~Cookie: SRCHUSR=AUTOREDIR=0&GEOVAR=&DOB=20140420; _IDP=MW=1398900583; PPLState=1; SRCHUID=V=2&GUID=3C5D2A57D3C34E90A7250D99B33B30B5~~Proxy-Authorization: Negotiate TlRMTVNTUAABAAAAl7II4gkACQAyAAAACgAKACgAAAAGAbEdAAAAD1RIRUJPU1MtUENXT1JLR1JPVVA=~~~~]	
						//
						//if ( g_bTraceHTTPProxy )
						//{
						//	GString g("HTTPProxy recv'd Type1Message (authentication message) from:");
						//	g << "  fd:" << td->sockfd << "  bytes:" << strAuthBlob._len << " Domain:" << strDomain1 << " Host:" << strHost1 << " pInitialProxySend=[" << pInitialProxySend << "]";
						//	g.ReplaceChars("\r\n",'~');
						//	InfoLog(123,g);
						//}


						GString strMessage2Challenge(256);
						////////////////////////////////////////////////////////////////////////////////////////////////
						// This server challenges the client with 8-bytes random number that we create here.
						//												
						nChallenge = (unsigned __int64)(unsigned __int64 *)(void *)&g_nClientthreadsInUse;  // guess the memory address of that variable to start with..
						nChallenge += rand() * rand() * rand() * g_RandomNumbers[rand() % 8] * g_RandomNumbers[rand() % 8] * g_RandomNumbers[rand() % 8];
						unsigned char *pnChallenge = (unsigned char *)(void *)&nChallenge;
						for(int idx = 0; idx < 8; idx++)
							if ( *( pnChallenge + idx ) == 0 )
								*( pnChallenge + idx ) = rand(); // no zero bytes
						////////////////////////////////////////////////////////////////////////////////////////////////

							
						////////////////////////////////////////////////////////////////////////////////////////////////
						// Now push the 8 byte challenge into memory that another thead can access because in some cases
						// the Type3 message that the client returns is handled by another clientthread(), therefore if the
						// key was kept here on this stack there would be no way to access it, so we push to global memory.
						gthread_mutex_lock(&g_csMtxChallengeList);
						for(int i = 0; i<ARRAY_OF_OUTSTANDING_CHALLENGES; i++)
						{
							if (g_arrChallenges[i] == 0)
							{
								g_arrChallenges[i] = nChallenge;
								break;
							}
						}
						gthread_mutex_unlock(&g_csMtxChallengeList);
						////////////////////////////////////////////////////////////////////////////////////////////////



						// The target name -- the name of the authentication target. This is sent in response to the client requesting the 
						//  target (via the [RequestTarget] flag in the Type 1 message). This can contain a domain, server, or a network share. 
						//  The target type is indicated via the [NTLM_TargetTypeServer] (otherwise NTLM_TargetTypeDomain or NTLM_TargetTypeShare) 
						//  flag in the Type2Message. The target name is Unicode, as indicated by the presence of the NTLM_Unicode flag in the Type2Message.
						GString strTarget;
	//					strTarget = g_strThisHostName;
						strTarget = strHost1;

						const wchar_t *pzTargetWide = strTarget.Unicode();

						// -------------------------------------------------------------------------------------------------
												// pzArg4 = 0 or FQDN Host Name
												// pzArg3 = 0 or DNS Domain Name
												// pzArg2 = 0 or NetBIOS Server Name
												// pzArg1 = 0 or NetBIOS Domain Name

																		// *pzArg1,		*pzArg2,	 *pzArg3,	pzArg4
						// Type2Message authMsg2(nChallenge, pzTargetWide, pzTargetWide,pzTargetWide,pzTargetWide,pzTargetWide);
						   Type2Message authMsg2(nChallenge, pzTargetWide, pzTargetWide,pzTargetWide		,0			,0);
						// Type2Message authMsg2(nChallenge, pzTargetWide, 0,			 0				    ,0			,0);
						// -------------------------------------------------------------------------------------------------
						//	write the binary of authMsg2 the into the GString
						strMessage2Challenge.Write(&authMsg2.protocol[0], authMsg2.datablock_off + authMsg2.datablock_len );

						strMessage2Challenge.UUEncode();




						GString strType2("HTTP/1.1 407 Unauthorized\r\n");
						strType2 << "Proxy-Authenticate: Negotiate " << strMessage2Challenge << "\r\n";
						strType2 << "Content-Length: 0\r\n";					
//						strType2 << "Connection: Keep-Alive\r\n";				//<-isnt it implied?  perhaps this is not necessary.
//						strType2 << "Proxy-Connection: Keep-Alive\r\n";			//<-isnt it implied?  perhaps this is not necessary.
						strType2 << "\r\n";
						

						int nRslt = nonblocksend(td->sockfd,strType2._str,strType2._len);
						if (nRslt != strType2._len)
						{
							if ( g_bTraceHTTPProxy )
							{
								
								GString g("*****Client aborted - failed to send the challenge:");
								g << "  fd:" << td->sockfd;
								InfoLog(10556,g);
							}

							nCloseCode = 11334; 
							goto CLOSE_CONNECT; 
							
						}

						goto HTTP_PROXY_KEEP_ALIVE; 

					} // end of NTLM Type 1 handler
					
					else if ( pMsg0->type == 3 )
					{
						Type3Message *p3 = (Type3Message *)(void *)strAuthBlob._str;
						if (p3->type !=3 || p3->dom_len > 100 || p3->user_len > 100 || p3->host_len > 100 || p3->dom_off > 400 || p3->user_off > 400 || p3->host_off > 400 || p3->lm_resp_len > 400 || p3->nt_resp_len > 400 || p3->lm_resp_off > 400 || p3->nt_resp_off > 400)
						{
							// error
							if ( g_bTraceHTTPProxy )
							{
								GString g("NTLM Protocol Error. Expecting Type:3,  Got Type:");
								g << p3->type << " fd:" << td->sockfd << " [" << sockBuffer << "]";
								g.ReplaceChars("\r\n",'~');
								InfoLog(1055,g);
							}
							nCloseCode = 11336; 
							goto CLOSE_CONNECT; 
						}

						GString strDomain3; // strDomain3 came in the 3rd message in Unicode, strDomain1 in the first in ASCII
						// GString strHost3;	// strHost3 is not needed
						GString strUser;
						strDomain3.FromUnicode(		(const wchar_t *)((char *)(void *)p3 + p3->dom_off),	p3->dom_len  / 2 );
						// strHost3.FromUnicode(	(const wchar_t *)((char *)(void *)p3 + p3->host_off),	p3->host_len / 2 );
						strUser.FromUnicode(		(const wchar_t *)((char *)(void *)p3 + p3->user_off),	p3->user_len / 2 );

						GString binNTResponse   (	strAuthBlob._str + p3->nt_resp_off,						p3->nt_resp_len ); // length = 260
						TypeNTLMv2 *pNTLMv2 = (TypeNTLMv2 *)(void *)binNTResponse._str;
						// GString strTargetv2; // not used. same as strDomain and strDomain3 except that strTargetv2 is lower case 
						// strTargetv2.FromUnicode(  (const wchar_t *)((char *)(void *)&pNTLMv2->datablock[0]), pNTLMv2->name1Len / 2);
						



						// There are 116444736000000000 100ns intervals between 1601 and 1970.  For more info search this file for: Timestamp A 64-bit
						__int64 NowTimestamp = ((unsigned __int64)time(0) * 10000000) + 116444736000000000;
						if (pNTLMv2->timestamp - NowTimestamp > 10000000 )
						{
							__int64 iDiff = pNTLMv2->timestamp - NowTimestamp;

							// Know This: A Windows CE device may not have a real-time clock, therefore NTLMv2 from those devices has no timestamp.							
							if ( g_bEnforceNTLMv2Timestamp )
							{
								if ( g_bTraceHTTPProxy )
								{
									GString g("*****************************Timestamp out of range from :[");
									g << strConnectedIPAddress << " on fd:" << td->sockfd << " iDiff=" << iDiff << " [" << pInitialProxySend << "]";
									g.ReplaceChars("\r\n",'~');
									InfoLog(10556,g);
								}

								nCloseCode = 1984; 
								goto CLOSE_CONNECT; 
							}
						}



					
						strUser.Prepend("User:"); // make a GProfile lookup Entry for the following layout
						const char *pzPassLookup = GetProfile().GetStringOrDefault("Xfer",strUser,"");		
						strUser.Replace("User:","Auth:",1); // make the new entry lookup key
						const char *pzAuth = GetProfile().GetStringOrDefault("Xfer",strUser,"");		
						if (pzPassLookup[0] == 0 || pzAuth[0] == 0)
						{
							// unknown user name
							if ( g_bTraceHTTPProxy )
							{
								GString g("*****************************Unknown User Name Attempted:[");
								g << strUser.StartingAt(strUser.Find(':')+1) <<  "]  from:" << strConnectedIPAddress << " on fd:" << td->sockfd << "[" << pInitialProxySend << "]";
								g.ReplaceChars("\r\n",'~');
								InfoLog(10556,g);
							}
							// send the 407
							nonblocksend(td->sockfd,strNoProxyResponse,(int)strNoProxyResponse.Length());

							goto HTTP_PROXY_KEEP_ALIVE; 
							// or close the connection like this:
							// nCloseCode = 11336; 
							// goto CLOSE_CONNECT; 
						
						}
						strUser.Remove(0,5); // removes "Auth:"



						// create the md4 hash that will become part of the data that is md5 hashed in the HMAC
						GString GPass(pzPassLookup);
						MD4_CTX passcontext;
						unsigned char passdigestMD4[16];
						MD4_Init(&passcontext);
						MD4_Update(&passcontext, GPass.Unicode(), GPass.Length()*2 );
						MD4_Final(passdigestMD4, &passcontext);

						
						GString strUserAndDomain(strUser);
						strUserAndDomain << strDomain3;
						strUserAndDomain.MakeUpper();


						for(int i = 0; i<ARRAY_OF_OUTSTANDING_CHALLENGES; i++)
						{
							// this is a lockless read from a thread on global memory (the write is locked)
							nChallenge = g_arrChallenges[i];
							if (nChallenge == 0)
							{
								continue;
							}


							unsigned char HMACDigest[16];
							unsigned char *pnChallenge = (unsigned char *)(void *)&nChallenge;
							hmac_v2X(passdigestMD4, (unsigned char *)strUserAndDomain.Unicode(), strUserAndDomain._len * 2, pnChallenge, (unsigned char *)(binNTResponse._str + 16) , binNTResponse._len - 16, HMACDigest );
							__int64 nFoundAtIndex = binNTResponse.FindBinary(&HMACDigest[0], 16, 0);
							if (nFoundAtIndex == 0)
							{
								// bingo. we hashed the same value that was sent to us, therefore this client passed the authentication process
								bIsAuthorized = 1;

								gthread_mutex_lock(&g_csMtxChallengeList);
								for(int i = 0; i<ARRAY_OF_OUTSTANDING_CHALLENGES; i++)
								{
									if (g_arrChallenges[i] == nChallenge)
									{
										g_arrChallenges[i] = 0; // this challenge is no longer outstanding
										break;
									}
								}
								gthread_mutex_unlock(&g_csMtxChallengeList);
								break;
							}
						}
						if (!bIsAuthorized)
						{
							if ( g_bTraceHTTPProxy )
							{
								GString g("XXXXXXX :  Proxy Issued a 407 due to password mismatch for [");
								g << strUser.StartingAt(strUser.Find(':')+1) <<  "]  from:" << strConnectedIPAddress << "on fd:" << td->sockfd;
								InfoLog(10556,g);
							}

							// send the 407
							nonblocksend(td->sockfd,strNoProxyResponse,(int)strNoProxyResponse.Length());

							goto HTTP_PROXY_KEEP_ALIVE;
							// or
							//nCloseCode = 11336; 
							//goto CLOSE_CONNECT; 
						}

						if ( g_bTraceHTTPProxy )
						{
							GString g("+++++++ HTTPProxy authenticated :");
							g << strUser.StartingAt(strUser.Find(':')+1) <<  "  from:" << strConnectedIPAddress << "  using NTLMv2 on fd:" << td->sockfd;
							InfoLog(10556,g);
						}

						bConnectionAuthorized = 1;

					} // end of NTLM Type 3 handler

				} // end of "Authorization: Negotiate" handler



				pAuthStr = (g_bNegotiateBasic) ? stristr(pInitialProxySend,"Proxy-Authorization: Basic ") : 0;
				if (pAuthStr)
				{
					GString strBasicAuthUser;
					GString strBasicAuthDecoded;
					strBasicAuthUser.SetFromUpTo(pAuthStr + 27,"\r\n"); // 27 is strlen("Proxy-Authorization: Basic ")
					strBasicAuthUser.UUDecode();
					if ( strBasicAuthUser.Find(":") == -1 )
					{
						// error bad auth format
						nCloseCode = 11032; 
						goto CLOSE_CONNECT; 
					}
					GString strPassword ( &strBasicAuthUser._str[strBasicAuthUser.Find(":")+1] ) ;
					strBasicAuthUser.SetLength(strBasicAuthUser.Find(":")); // now strBasicAuthUser contains only the user name
					strBasicAuthUser.Prepend("User:"); // make a GProfile lookup Entry for the following layout

					//  GProfile storage for user/password/permissions
					/////////////////////////////////////////
					//	[Xfer]													<--the section name that holds the users
					//	User:bob=aaa
					//	Auth:bob=*												<--bob's password is aaa and he's superuser so he has all permissions like me
					//	User:joe=password
					//	Auth:joe=FRLXcZf|nothing matters past the pipe			<--joe has a list of 1 byte permissions and 'c' exists so he can HTTP Proxy
					const char *pzPassLookup = GetProfile().GetStringOrDefault("Xfer",strBasicAuthUser,"");		
					if ( strPassword.Compare( pzPassLookup ) == 0 )
					{
						// the password matches, now see if the user has permission for this action
						strBasicAuthUser.Replace("User:","Auth:",1); // make the new entry looklup key
						const char *pzAuth = GetProfile().GetStringOrDefault("Xfer",strBasicAuthUser,"");		
						bIsAuthorized = (pzAuth[0] == '*'); // look for superuser access
						if ( !bIsAuthorized ) 
						{
							// if character 'c' is found before a '|', then set bIsAuthorized = true
							char *pPipe = (char *)strpbrk(pzAuth, "|");
							if (pPipe)
							{
								*pPipe = 0;
							}
							char *pFoundAuth = (char *)strpbrk(pzAuth, "c"); 
							bIsAuthorized = (pFoundAuth) ? 1 : 0; 
						}
					}
					if ( g_bTraceHTTPProxy )
					{
						if (bIsAuthorized)
						{
							GString g("+++++++ HTTPProxy authenticated :");
							g << strBasicAuthUser.StartingAt(strBasicAuthUser.Find(':')+1) <<  "  from:" << strConnectedIPAddress << "  using Basic authentication on fd:" << td->sockfd;
							InfoLog(10556,g);
						}
						else
						{
							GString g("------- HTTPProxy refused :");
							g << strBasicAuthUser.StartingAt(strBasicAuthUser.Find(':')+1) <<  "  from:" << strConnectedIPAddress << "  for a Wrong Password or No Permission on fd:" << td->sockfd;
							InfoLog(10556,g);
						}
					}
				} // end of Proxy-Authorization: Basic
				} // if (!bIsAuthorized)
				
				
				if (!bIsAuthorized)
				{
					// send the 407
					int nSent = nonblocksend(td->sockfd,strNoProxyResponse,(int)strNoProxyResponse.Length());
					if (nSent != strNoProxyResponse.Length())
					{
						if ( g_bTraceHTTPProxy )
						{
							pInitialProxySend[nInitialRawSend] = 0;
							GString strG("####### HTTP-PROXY failed to send 407 to ");
							strG << "client fd:" << td->sockfd << " from tid:" << td->nThreadID  << " SOCK_ERR = " << SOCK_ERR << ") command[" <<  pInitialProxySend << "]";
							strG.ReplaceChars("\r\n",'~');
							InfoLog(1055,strG);
						}
					}
					else
					{
						td->nTotalBytesSent += nSent;
					}

					if ( g_bTraceHTTPProxy )
					{
						pInitialProxySend[nInitialRawSend] = 0;
						GString strG("??????? HTTP-PROXY Issued a 407 Password Request to ");
						strG << "client fd:" << td->sockfd << " from tid:" << td->nThreadID  << ") allowing[";
						if (g_bNegotiateNTLMv2)
						{
							strG << "NTLMv2  ";
						}
						if (g_bNegotiateBasic)
						{
							strG<< "Basic";
						}
						strG << "] command[";
						strG.Write(pInitialProxySend, (nInitialRawSend > 512) ? 512 : nInitialRawSend );
						strG << "]";
						strG.ReplaceChars("\r\n",'~');
						InfoLog(1055,strG);
					}

					/// cannot goto HTTP_PROXY_KEEP_ALIVE here...... 

					nCloseCode = 1407;
					goto CLOSE_CONNECT;
				}

HTTP_PROXY_CONVERT:
				cryptDest[0] = 0;				 // purely for debugging asthetics
				sockBuffer[nInitialRawSend] = 0; // purely for debugging asthetics

				GString strPxyConnectTo2;


				// sockbuffer contains the pre-converted HTTP in-tact after calling ConvertHTTPProxy() we will overrwite nInitialRawSend with the length of the converted HTTP in crypDest
				int nisVerb = isHTTPVerb(sockBuffer);


				if ( ConvertHTTPProxy(sockBuffer,nInitialRawSend,cryptDest,strPxyConnectTo2, &nInitialRawSend, &bIsCONNECT) )
				{
					// strPxyConnectTo2 is in the form "443@www.google.com" or "80@www.google.com"
					td->strAction.Empty();
					//td->strAction = cryptDest;
					if (nisVerb)
					{
						td->strAction.SetFromUpTo(cryptDest,"HTTP/");
					}
					td->nSequence++;

					if (g_bHTTPProxyLogExtra)
					{
						cryptDest[nInitialRawSend] = 0;
						GString gPostConvert;
						gPostConvert << "ConvertHTTPProxy verb type:" << nisVerb << "  from fd:" << td->sockfd << " to [" << strPxyConnectTo2 << "] last connected out [" << td->pzConnectRoute << "] bytes=" << nInitialRawSend << " starts with [";
						gPostConvert.Write(cryptDest,64);
						gPostConvert << "]";
						gPostConvert.ReplaceChars("\r\n",'~');
						InfoLog(55555,gPostConvert);
					}
					pInitialProxySend = cryptDest;



					if ( strPxyConnectTo2.Compare(td->pzConnectRoute) == 0 && !td->nProxyClosed && td->nProxyfd != -1 )
					{
						// we skip the outbound connect - we are already connected
						if (g_bHTTPProxyLogExtra)
						{
							GString strMsg("HTTP_PROXY_ALREADY_CONNECTED AAA for fd:");
							strMsg << td->sockfd << " to[" << td->pzConnectRoute << "]";
							InfoLog(987,strMsg);
							bIsAuthorized = 1;
						}

						goto HTTP_PROXY_ALREADY_CONNECTED;
					}
					else
					{
						if ( g_bEnableOutboundConnectCache )
						{
							// save the open connection to [td->pzConnectRoute] 
							if( td->nProxyfd != -1 && !td->nProxyClosed )
							{
							    // the key is fully qualified with the port in this form "80@www.google.com"
								// since 0 is a valid socket handle, add 1 to it before we cache it, then if we find a 0 it always means "no entry exists in cache"
								gthread_mutex_lock(&g_csMtxConnectCache);
								g_OutboundConnectCache.Insert(td->pzConnectRoute, (void *)(td->nProxyfd+1));
								gthread_mutex_unlock(&g_csMtxConnectCache);

								// disconnect this socket handle from the ProxyHelperThread and return the thread to the pool
								// search this file for:   777 signifies a "detach"
								td->nProxyClosed = 777;
							}
						}
						else
						{
							// closes the outbound connection and returns the ProxyHelperThread to the pool
							PortableClose(td->nProxyfd,"NoCache1");
							td->nProxyClosed = 1;
							td->nProxyfd = -1;
						}

						// td->pzConnectRoute is empty on the first use, otherwise it is overwritten
						strncpy( td->pzConnectRoute, strPxyConnectTo2, sizeof(td->pzConnectRoute) - 1 );
						td->pzConnectRoute[sizeof(td->pzConnectRoute) - 1] = 0;
					}



					int nBlockConnect = 0;
					// make [td->pzConnectRoute] lower case
					for (int i = 0; i < sizeof(td->pzConnectRoute); i++)
					{
						if (td->pzConnectRoute[i] == 0)
							break;
						td->pzConnectRoute[i] = tolower(td->pzConnectRoute[i]);
					}



					// [td->pzConnectRoute] is in the form 443@www.google.com or 80@www.google.com
					char *pSep = (char *)strpbrk(td->pzConnectRoute,"@");
					pSep++; // move pointer ahead one byte past the "@"


					// strBlockTest the string to ",www.google.com" as the key to search for in the blocked or allowed array.
					GString strBlockTest;
					strBlockTest << "," << pSep; // search for ",blocked.com"


					if (g_bBlockHTTPProxyAds || g_strHTTPProxyBlocked._len)
					{
						if ( g_strHTTPProxyNotBlocked.Find(strBlockTest) != -1 )
						{
							nBlockConnect = 0;
						}
						else
						{
							if (g_bBlockHTTPProxyAds) 
							{
								if ( strstr(g_strHTTPProxyCacheBlocked._str, strBlockTest._str ) )
								{
									nBlockConnect = 3;
								}
								else if ( strstr(g_pAds, strBlockTest._str )  )
								{
									g_strHTTPProxyCacheBlocked << strBlockTest;  // perhaps this will be in a GHash someday
									nBlockConnect = 1;
								}
							}
						}
					}
					if ( g_strHTTPProxyBlocked.Find( strBlockTest ) != -1 )
					{
						g_strHTTPProxyCacheBlocked << strBlockTest;
						nBlockConnect = 2;
					}
					

					if (nBlockConnect)
					{
						if (g_bTraceHTTPProxy)
						{
							GString strG("Blocked Connect to[");
							strG << td->pzConnectRoute << "] code:" << nBlockConnect << " fd:" << td->sockfd << " [" << td->strAction << "] HTTP:" << pInitialProxySend;
							strG.ReplaceChars("\r\n",'~');
							InfoLog(777,strG);
						}


						GString strConnectResponse;
						strConnectResponse = "HTTP/1.1 402 Payment Required\r\n";  // Karma was called due.
						strConnectResponse << "Content-Length: 0\r\n";
					//  strConnectResponse << "Proxy-agent: " << g_strHTTPProxyAgentName << "\r\n";
						strConnectResponse << "Connection: keep-alive\r\n";
						strConnectResponse << "Proxy-Connection: keep-alive\r\n\r\n";
						nonblocksend(td->sockfd,strConnectResponse,(int)strConnectResponse._len);

						
						td->strAction.Empty();
						pInitialProxySend = 0; // not necessary   
						goto HTTP_PROXY_KEEP_ALIVE;
					}

					// td->strAction will be either a "CONNECT n.n.n.n" or a file name or URL to GET
					td->strAction.SetFromUpTo(pInitialProxySend,"HTTP/");

					pInitialProxySend = cryptDest;
					td->pTSD->nProxyTimeout = g_nHTTPProxyTimeout;
					memcpy(TCD.pzConnectRoute,td->pzConnectRoute,strlen(td->pzConnectRoute) + 1); // copy the null
					TCD.nConnectRouteSize = strlen(td->pzConnectRoute) + 1;
					TCD.strAction = td->strAction;

				}
				else
				{
					GString strErr;
					strErr << "\r\n\r\n*************************************** ConvertHTTPProxy Failed on fd:" << td->sockfd << " [" << sockBuffer << "]\r\n\r\n";
					InfoLog(777,strErr);

					nCloseCode = 1104; 
					goto CLOSE_CONNECT; 
				}
			}
		}

		


		//	HOOK: Main protocol pre-implementation




		//////////////////////////////////////////////////////////////////////////////////////////////////
		//
		// This is the main loop in clientthread.
		//
		//////////////////////////////////////////////////////////////////////////////////////////////////
		IOBlocking(td->sockfd, 1);   // Set non-blocking IO incase some pre-handler forgot to set it back  



//		if (!IsOpenSocket(TCD.sockfd))
//		{
//			// break in the debugger
//		}
		int bHasAwakenHelper; 
		bHasAwakenHelper = 0; // It will be true if we jump to HTTP_PROXY_ALREADY_CONNECTED

		do
		{
HTTP_PROXY_RE_CONNECT:
			////////////////////////////////////////////////////////////////////////////////////////////////
			// on the first pass through the loop start the ProxyHelperThread, make the remote connection
			// and send any initial protocol data that has been prepared in pInitialProxySend when nInitialRawSend bytes has been set to the length of pInitialProxySend.
			////////////////////////////////////////////////////////////////////////////////////////////////
			if (bHasAwakenHelper == 0) 
			{
				if(g_DataTrace)
				{
					GString strMessage("\n\nWaking Helper - Thread ID(tid):");
					strMessage << td->nThreadID;
					strMessage.ToFileAppend(GetProfile().GetString("Trace","DataTraceFile",false));
				}

				// awaken our helper thread and have it connect to the remote side of this hop or the switchboard
				AttachToClientThread(&TCD,g_PoolProxyHelperThread,g_ProxyPoolSize,&g_nProxyPoolRoundRobin);
				bHasAwakenHelper = 1;

				gtimespec ts;
				long lConnectWait;
				lConnectWait = time(0);


				// hang here until the forwardTo connection has been established.....
				ts.tv_sec=time(0) + td->pTSD->nProxyTimeout; 
				ts.tv_nsec=0;
				int nWaitRslt;
				
				nWaitRslt = gthread_cond_timedwait(&td->condHelper, &td->lockHelper, &ts);
				if ( nWaitRslt == ETIMEDOUT )  // if we timed out 
				{
					if (g_bTraceHTTPProxy)
					{
						GString strLog("*******504 Gateway Timeout on tid:");
						strLog  << td->nThreadID << " (fd:" << td->sockfd << ") awaiting connect to[" << td->pTSD->nPort << ":" << TCD.pzConnectRoute << "  Your configurable timeout value is set to:" << td->pTSD->nProxyTimeout << " in Config file section [" << td->pTSD->szConfigSectionName << "]";
						InfoLog(106,strLog);
					}

					// 504 Gateway Timeout
					GString strConnectResponse;
					strConnectResponse = "HTTP/1.1 504 Gateway Timeout\r\n";
					strConnectResponse << "Proxy-agent: " << g_strHTTPProxyAgentName << "\r\n";
					strConnectResponse << "Connection: close\r\n";
					strConnectResponse << "Proxy-Connection: keep-alive\r\n\r\n";
					nonblocksend(td->sockfd,strConnectResponse,(int)strConnectResponse._len);
					goto HTTP_PROXY_KEEP_ALIVE;

					//nCloseCode = 1109;
					//break;
				}

				if (bIsCONNECT && td->pTSD->nProtocol == 11)
				{
					if ( td->nProxyClosed )
					{
						if (g_bTraceHTTPProxy)
						{
							GString strLog("*******502 Bad Gateway on tid:");
							strLog  << td->nThreadID << " (fd:" << td->sockfd << ") awaiting connect to[" << td->pTSD->nPort << ":" << TCD.pzConnectRoute << "  Your configurable timeout value is set to:" << td->pTSD->nProxyTimeout << " in Config file section [" << td->pTSD->szConfigSectionName << "]";
							InfoLog(106,strLog);
						}
						// 502 Bad Gateway
						GString strConnectResponse;
						strConnectResponse = "HTTP/1.1 502 Bad Gateway\r\n";
						strConnectResponse << "Proxy-agent: " << g_strHTTPProxyAgentName << "\r\n";
						strConnectResponse << "Connection: close\r\n";
						strConnectResponse << "Proxy-Connection: keep-alive\r\n\r\n";
						nonblocksend(td->sockfd,strConnectResponse,(int)strConnectResponse._len);
						goto HTTP_PROXY_KEEP_ALIVE;
					}
					else
					{
						GString strConnectResponse;
						strConnectResponse = "HTTP/1.1 200 Connection established\r\n";
						strConnectResponse << "Proxy-agent: " << g_strHTTPProxyAgentName << "\r\n\r\n";
						nonblocksend(td->sockfd,strConnectResponse,(int)strConnectResponse._len);
						
						// fall through - this HTTP 200 is followed by the remote HTTP Servers response
					}
				}

				if ( td->nProxyClosed )
				{
					// cose this connection since our outbound could not be opened
					nCloseCode = 1589; 
					break;
				}

HTTP_PROXY_ALREADY_CONNECTED:


				// this assumes any header/markup is already prepared in pInitialProxySend if nInitialRawSend is != 0
				if (nInitialRawSend)
				{
					pInitialProxySend[nInitialRawSend] = 0; // for debugging/logging beautification of the data we null terminate one byte past the data we will send.
					
					GString strPrivateURL;

					if( g_bBlockGoogleTracking && strstr(td->pzConnectRoute,"google.") != 0 && memcmp(pInitialProxySend,"GET /url?",9) == 0 ) 
					{
						// hey oogle, The G, is here in the XMLFoundation.
						char *pURL = strstr(pInitialProxySend,"&url=");
						if (pURL)
						{
							pURL += 5; // 5 is strlen("&url=")
							strPrivateURL.SetFromUpTo(pURL,"&");
							strPrivateURL.UnEscapeHexed();
						}
					}


					if (strPrivateURL._len && g_bBlockGoogleTracking )
					{
						
						// this HTTP request was enroute to a remote server, but we answer on that servers behalf in this domain
						GString strConnectResponse;
						strConnectResponse = "HTTP/1.1 301 Moved Permanently\r\n";
						strConnectResponse << "Location: " << strPrivateURL << "\r\n";
						strConnectResponse << "Connection: keep-alive\r\n";
						strConnectResponse << "Proxy-Connection: keep-alive\r\n\r\n";
						if ( nonblocksend(td->sockfd,strConnectResponse,(int)strConnectResponse._len) != strConnectResponse._len)
						{
							// if we failed to send, the client closed the connection
							nCloseCode = 1599; 
							break;
						}
						else
						{
							if (g_bTraceHTTPProxy)
							{
								GString g;
								g << "Blocked URL tracking on fd:" << td->sockfd << " to[" << td->pzConnectRoute << "] for [" << strPrivateURL << "]";
								InfoLog(777,g); 
							}
							goto HTTP_PROXY_KEEP_ALIVE; 
						}
					}



					//	send the first packet of proxy
					int nResult = 0;
					if( !td->nProxyClosed )  // the other thread can cause td->nProxyClosed to become true at any time
					{
						if ( g_bTraceHTTPProxy || g_bTraceHTTPProxyBinary)
						{
							GString strG("HTTP-PROXY-OUT(from fd:");
							if ( memcmp(pInitialProxySend,td->strAction._str,(td->strAction._len >  32) ? 32 :  td->strAction._len) == 0 )
								strG << td->sockfd << " from tid:" << td->nThreadID  << " to fd:" << td->nProxyfd << " [" << td->pzConnectRoute <<"]) bytes:" << nInitialRawSend << " [";
							else
								strG << td->sockfd << " Action[" << td->strAction << "] from tid:" << td->nThreadID  << " to fd:" << td->nProxyfd << " [" << td->pzConnectRoute <<"]) bytes:" << nInitialRawSend << " [";
							strG.Write(pInitialProxySend, (nInitialRawSend > 512) ? 512 : nInitialRawSend );
							strG << "]";
							strG.ReplaceChars("\r\n",'~');
							if(g_bTraceHTTPProxyBinary)
							{
								strG << "\r\n";
								strG.FormatBinary( (unsigned char *)pInitialProxySend, nInitialRawSend, 1, "\t\t\t\t");
							}
							InfoLog(105,strG);
						}



						if ( g_bTraceHTTPProxyFiles )
						{
							GString strFileName(td->strAction);
							if (strFileName._len > 255)
							{
								strFileName.SetLength(255);
							}
							// none of those are allowed in a file name
							strFileName.ReplaceChars("\\/:*?\"<>|",'~');
							strFileName.Prepend(td->pzConnectRoute);
							strFileName << ".OUT.txt";
							
							GString strPath(g_strLogFile);				  // start with the logfile name and path
							strPath.SetLength(strPath.ReverseFindOneOf("/\\")); // cut off the file name for either a windows or unix path
							strPath << g_chSlash << "transactions" << g_chSlash;

							// make the destination directory if it does not exist
							#ifdef _WIN32
								mkdir( strPath );
							#else
								mkdir( strPath ,777 );
							#endif
							
							// concat the two strings
							strPath << strFileName;

							// attach a GString object over the send buffer
							GString g(pInitialProxySend,nInitialRawSend,nInitialRawSend,0);
							g.ToFileAppend(strPath);
						}

						nResult = nonblocksend(td->nProxyfd,pInitialProxySend,nInitialRawSend);
						pInitialProxySend[0]=0;
					}
					

					//    if 'proxy was closed' ||		'it needs to become closed'
					if (    td->nProxyClosed	||		nResult != nInitialRawSend )
					{
						// returns 1 through 7  for RFC 2616 verbs that DO NOT need HTTP Proxy translation
						// returns 8 through 15 for RFC 2616 verbs that need HTTP Proxy translation
						// 0 this is the tail-part of a transaction that has been cancelled by the server
						int nisVerb = ( atoi(td->pzConnectRoute) == 443 ) ?  0 : isHTTPVerb(pInitialProxySend);

						if (g_bHTTPProxyLogExtra)
						{
							GString g;
							if (nisVerb)
								g << "(proxyoutclosed) HTTP client keep-alive goingto HTTP_PROXY_RE_CONNECT   nResult=" << nResult << " nisVerb=" << nisVerb;
							else
								g << "(proxyoutclosed) SSL or POST Content data from client. closing with nCloseCode=1111   ";
							g << " fd:" << td->sockfd << " failed to send to nProxyfd:" << td->nProxyfd <<" [" << td->pzConnectRoute << "] td->nProxyClosed=" << td->nProxyClosed << " bytes:" << nInitialRawSend << " nResult=" << nResult << " Data on hand[" << pInitialProxySend << "]";
							g.ReplaceChars("\r\n",'~');
							InfoLog(4567,g);
						}

						// if we got in here because it failed to send (otherwise it was already closed)
						if ( nResult != nInitialRawSend )
						{
							// this causes ProxyHelperThread to free/close the socket and return to the pool
							td->nProxyClosed = 1;
						}


						if ( nisVerb )
						{
							// reconnect.....
							bHasAwakenHelper = 0;

							if (g_bHTTPProxyLogExtra)
							{
								GString g;
								g << "\r\n\r\n\t\tRECONNECT fd:" << td->sockfd << " to[" << TCD.pzConnectRoute << "] == [" << td->pzConnectRoute << "] nisVerb=" << nisVerb;
								InfoLog(34576,g);
							}


							// lock the mutex so the thread awaits until the next connect is complete.
							gthread_mutex_lock(&td->lockHelper);

							goto HTTP_PROXY_RE_CONNECT;
						}
						else
						{
							// or we close this connection since our outbound was closed and it was an SSL session
							////////////////////////////////////////////////////////////////////
							nCloseCode = 1111;  // match with SSL InfoLog comment above
							////////////////////////////////////////////////////////////////////
							break;
						}
					}
					else if (g_DataTrace)
					{
						GString strLabel;
						strLabel.Format("tid%d tx to s",(int)td->nThreadID);
						DataTrace(strLabel, pInitialProxySend, nInitialRawSend);
					}
					td->nTotalBytesProxied += nInitialRawSend;
					nInitialRawSend = 0;
				}
			}

			////////////////////////////////////////////////////////////////////////////////////////////////
			// ^^^^^^^^^^^^ End of "first pass through the main loop (started ProxyHelperThread)^^^^^^^^^^^
			////////////////////////////////////////////////////////////////////////////////////////////////


			// recv from: (bIsTunnel) ? client application : client tunnel
			// essentially---> int bPacketSync = ( (!bIsTunnel) && (bCipher || bZip || bSync) ) ? 1 : 0;
			int bPacketSync = ( (!bIsTunnel) && sAttribs ) ? 1 : 0;
			unsigned short sPktAttributes = 0;


// HOOK: Special protocol processing
/*


			// If our worker thread has been stopped
			if (td->nProxyClosed) 
			{
				if (nPxyConnectToPort == 80)
				{
					if (g_bHTTPProxyLogExtra)
					{
						GString strG;
						strG << "FYI: fd:" << td->sockfd << " being kept-alive. Failed to send to proxy fd:" << td->nProxyfd << "  [" << td->pzConnectRoute << "] td->nProxyClosed=" << td->nProxyClosed << "  SOCK_ERR=" << SOCK_ERR;
						InfoLog(777,strG);
					}
					goto HTTP_PROXY_KEEP_ALIVE;
				}
				else
				{
					// or we close this connection since our outbound was closed and it was an SSL session
					nCloseCode = 1113; 
					break;
				}
			}
*/
		
			if (td->pTSD->nProtocol == 11)
			{
				bReadHTTP = 0;
				if (!g_bHTTPProxyEnabled)
				{
					nCloseCode = 1150; 
					break;
				}
			}


			int rslt = ReadPacket(td->sockfd,1,0,&nBufBytes,sockBuffer,&pPacket,&nPacketLen, &nPacketIndex,&sPktAttributes,sAttribs,bPacketSync,bReadHTTP);
			
			// If our worker thread has been stopped, the remote side must have ended that side of the proxy
			if ( td->nProxyClosed )
			{
				if ( atoi(td->pzConnectRoute) != 443 )
				{
					if (rslt > 0)
					{
						int nVerb = isHTTPVerb(sockBuffer);
						if (nVerb == 0)
						{
							// we close this connection it looks like more POST data, but our server closed the connection
							nCloseCode = 6548; 
							break; // end this thread too
						}
						else 
						{
							if (g_bHTTPProxyLogExtra)
							{
								GString g("(WorkerProxy ended) client is being kept-alive NOT CLOSED. goingto HTTP_PROXY_CONVERT. fd:");
								g << td->sockfd << " proxy fd:" << td->nProxyfd << "  [" << td->pzConnectRoute << "]  SOCK_ERR=" << (int)SOCK_ERR << " nVerb=" << nVerb << " bReadHTTP=" << bReadHTTP << " nPacketLen=" << nPacketLen << " nPacketIndex=" << nPacketIndex << " nBufBytes=" << nBufBytes;
								InfoLog(7771,g);
							}

							nInitialRawSend = nBufBytes;
							goto HTTP_PROXY_CONVERT;
						}
					}


					if (g_bHTTPProxyLogExtra)
					{
						// in some cases we might want to close here

						GString g("(WorkerProxy ended) client is being kept-alive NOT CLOSED. goingto HTTP_PROXY_KEEP_ALIVE. fd:");
						g << td->sockfd << " proxy fd:" << td->nProxyfd << "  [" << td->pzConnectRoute << "]  SOCK_ERR=" << (int)SOCK_ERR << " bReadHTTP=" << bReadHTTP << " nPacketLen=" << nPacketLen << " nPacketIndex=" << nPacketIndex << " nBufBytes=" << nBufBytes;
						InfoLog(7771,g);
					}

					// If our worker thread has been stopped we still use this open connection to proxy the next request
					// This might be a problem, our worker proxy ended, and the client may not know, 
					// now we will go read more data from that client.  It could be the next HTTP txn, or more data for the connection that already closed.
					goto HTTP_PROXY_KEEP_ALIVE;

					//otherwise...
					//nCloseCode = 99999; 
					//break;
				}
				else // this must be SSL, and the remote side is closed so quit reading from the client
				{
					// we close this connection since our outbound was closed and it was an SSL session
					nCloseCode = 1151; 
					break; // end this thread too
				}
			}


			if (rslt == 0)
			{
				// no data or only a partial packet available - nothing to do.
				if (lLastActivityTime)
				{
					if ( (time(0) - lLastActivityTime ) > (unsigned long)td->pTSD->nProxyTimeout )
					{
						//Helper Thread Wait Expired
						nCloseCode = 1114; 
						break;
					}
				}
				else
				{
					lLastActivityTime = time(0);
				}
			}
			else if (rslt == -1) // failure during wait (connection closed by client)
			{
				// SOCK_ERR == 0
				if (g_bHTTPProxyLogExtra)
				{
					GString gZ("*******(connection closed by client) fd:");
					gZ << td->sockfd << " SOCK_ERR=" << (int)SOCK_ERR << " sAttribs=" << sAttribs << " bPacketSync=" << bPacketSync << " bReadHTTP=" << bReadHTTP << " nPacketLen=" << nPacketLen << " nPacketIndex=" << nPacketIndex << " nBufBytes=" << nBufBytes;
					InfoLog(777,gZ); 
				}
				nCloseCode = 1115; 
				break;
			}
			else // process this packet
			{
				td->nTotalBytesRead += nPacketLen;
				// keep alive on clientthread
				lLastActivityTime = time(0);


				char *pSendData = pPacket;
				int nSendDataLen = nPacketLen;
				if (g_DataTrace)
				{
					GString strLabel;
					strLabel.Format("tid%d rx from c",(int)td->nThreadID);
					DataTrace(strLabel, pSendData, nSendDataLen);
				}

				// Sending to the "Server" from the clientthread
				if ( td->pTSD->bIsLogging ) 
				{
					GString str("Transmit to Server ");
					str << TCD.pzConnectRoute << " fd[" << td->nProxyfd << "]";
					ProxyLog(td->pTSD->strLogFile, td->nParentThreadID, pSendData, nSendDataLen, str);
				}
				if (g_bHTTPProxyLogExtra)
				{
					GString strClientRead;
					strClientRead << "New HTTP request OR more data from fd:" << td->sockfd << " last connected out [" << td->pzConnectRoute << "] bytes=" << nPacketLen << " starts with[";
					strClientRead.Write(pSendData,64);
					strClientRead << "]";
					strClientRead.ReplaceChars("\r\n",'~');

					InfoLog(951,strClientRead);
				}


				// if we need to apply data attributes(Cipher || Zip || Sync)
				if ( sAttribs )
				{
					int newLength;
					if (bIsTunnel) // if we are about to encrypt/compress, make room for the packet header
					{
						cryptDest += 4; // move the pointer ahead 4 bytes
					}

					// compress/decompress/crypt/decrypt or custom manipulation
					if (PrepForServer(sAttribs,bIsTunnel,&cdToServer,pSendData,nSendDataLen,cryptDest,&newLength))
					{
						if (bIsTunnel) // if we just encrypted/compressed
						{
							cryptDest -= 4; // move the pointer back 4 bytes
							short sh = htons(newLength); // packet length in network byte order
							memcpy(&cryptDest[0],&sh,sizeof(short));
							sh = htons(sAttribs); // attributes in network byte order
							memcpy(&cryptDest[2],&sh,sizeof(short));
							newLength += 4; // account for the header in the packet size
						}
						pSendData = cryptDest;
						nSendDataLen = newLength;
					}
					else
					{
						//PrepForServer failed in Branch2
						nCloseCode = 1116; 
						break;
					}
				}


				
				////////////////////////////////////////////////////////////////////////////
				//
				// Here we have the data read in (by HTTP or by System Tunnel) from the client.
				// (client is: the application or host that made the connection to this machine)
				// It is about to be sent to the server(the machine we connected out to)
				// pSendData is set to the current data and nSendDataLen is it's length.
				//
				// Hook - Dynamic Proxy Client to Server Packet Conversion
				//
				////////////////////////////////////////////////////////////////////////////
				// if HTTP proxy
				if ( td->pTSD->nProtocol == 11)
				{
					int nVerbKind = isHTTPVerb(pSendData);
				
					if ( nVerbKind > 7 )
					{
						pSendData[nSendDataLen] = 0; // purely for debugging asthetics
						sockBuffer = pSendData;
						nInitialRawSend = nSendDataLen; 

						if (g_bHTTPProxyLogExtra)
						{
							GString g("FYI:  fd:");
							g << td->sockfd << "  goingto HTTP_PROXY_CONVERT nVerbKind=" << nVerbKind << " [" << td->pzConnectRoute << "] current nProxyfd:" << td->nProxyfd;
							InfoLog(4561,g);
						}

						goto HTTP_PROXY_CONVERT;
					}
					else // this is SSL or a following packet of a POST with Content-Length to the established connection
					{
						pInitialProxySend = pSendData;
						nInitialRawSend = nSendDataLen; 
						bIsCONNECT = 0;

						if (g_bHTTPProxyLogExtra)
						{
							GString g("FYI:  fd:");
							g << td->sockfd << "  goingto HTTP_PROXY_ALREADY_CONNECTED BBB nVerbKind=" << nVerbKind  << " current nProxyfd=" << td->nProxyfd;
							InfoLog(4562,g);
						}
						goto HTTP_PROXY_ALREADY_CONNECTED;
					}
				}

// HOOK: protocol implementation


				
//				if (nProxyMaxBytes > 0) // Protocol = 8
//				{
//					// Currently this only happens when protocol = 8, messaging is truncating the HTTP multipart trailer
//					nSendDataLen = ( (nTotalProxiedBytes + nSendDataLen) > nProxyMaxBytes) ? nProxyMaxBytes - nTotalProxiedBytes : nSendDataLen;
//					
//					// if the proxy is about to send the last chunk
//					if (nTotalProxiedBytes + nSendDataLen >= nProxyMaxBytes)  
//					{
//						td->nProxyClosed = 777; // detach the td->nProxyfd and end the ProxyHelperThread
//						while (td->nProxyClosed != 2) // it will be 2 when it's back in the pool
//						{
//							gsched_yield();
//						}
//					}
//				}



				if (nonblocksend(td->nProxyfd,pSendData, nSendDataLen) != nSendDataLen)
				{
					nCloseCode = 1120; 
					break;
				}
				td->nTotalBytesProxied += nSendDataLen;

				
				// log the send event, sending to the "server" from the clientthread
				if ( td->pTSD->bIsLogging )
				{
					GString str("Transmit to Server:");
					str << TCD.pzConnectRoute << " fd[" << td->nProxyfd << "] per request [" << td->strAction << "]";
					ProxyLog(td->pTSD->strLogFile, td->nParentThreadID, pSendData, nSendDataLen, str );
				}
				if (g_DataTrace)
				{
					GString strLabel; 
					strLabel.Format("tid%d tx to s",(int)td->nThreadID);
					DataTrace(strLabel, pSendData, nSendDataLen);
				}

				
				// this tests for the end of proxy for Protocol 8(Messaging)
//				if (nProxyMaxBytes > 0 && nTotalProxiedBytes >= nProxyMaxBytes)  
//				{
//					g_SwitchBoard.WaitForResponse(td->sockfd, strCertifiedMsgResponse);
//
//					td->sockfd = td->nProxyfd; // swap fd's - keep this one alive
//					
//					// put the thread back in the pool but DON'T CLOSE HIS FD, move it to this thread
//					goto KEEP_ALIVE;
//				}

			}

		}while( !td->nProxyClosed );


		// this is where it would fall through to anyhow.  the goto is for readability.
		goto CLOSE_CONNECT; 

		
		}// End of Branch 1	scope
		else if (td->pTSD->nProtocol == 1  ||	// HTTP Server			
				 td->pTSD->nProtocol == 4  ||	// TXML
				 td->pTSD->nProtocol == 5 )// ||	// Both TXML and HTTP
		{
		//////////////////////////////////////////////////////////////////////////		
		// Branch 2 - reads HTTP headers, and generates an HTTP response 
		//////////////////////////////////////////////////////////////////////////		
		nKeepAlive = g_nMaxKeepAlives;
		int nHeaderLength;
		int nBytes;
		int nCumulativeBytes;
		bisHead = 0;
		nCloseCode = 1200; // default close code for Branch 2   error range 12nn(1st thread, 2nd branch)
		// Set non-blocking IO 
		IOBlocking(td->sockfd, 1);



KEEP_ALIVE:
		td->nSequence++;
		nHeaderLength = 0;
		nBytes = -1;
		nCumulativeBytes = 0;

		int rslt = readableTimeout(td->sockfd, g_nKeepAliveTimeout, 0);
		// failure during wait || time limit expired
		if (rslt == -1 || rslt == 0)
		{
			//	[HTTP] Keep Alive Time Out met=(1201) or Client Disconnect=(1202)
			nCloseCode = (!rslt) ?  1201 : 1202;
			goto CLOSE_CONNECT;
		}

		// recv an HTTP command
		unsigned int nPktIndex = 0;	// you may need to use this value to fetch next block of content.
		unsigned long nContentLen = 0;
		char *pContent = 0;


		rslt = ReadPacketHTTP( td->sockfd, &nBytes, sockBuffer,&nPktIndex, &nContentLen, &pContent);
		if (rslt < 1)
		{
			// The client aborted, 1203=Timeout 1204=Socket level
			nCloseCode = (!rslt) ?  1203 : 1204;
			goto CLOSE_CONNECT;
		}
		td->nTotalBytesRead += nBytes;

		//
		// These are useful values for the ServerCoreCustomHTTP.cpp hook
		int nHTTPHeaderLength = pContent - sockBuffer;
		int nHTTPContentAcquired = nBytes - nHTTPHeaderLength;



		sockBuffer[nBytes] = 0;
		if (g_HTTPHeaderTrace)
		{
			if (sockBuffer[nBytes-4] == '\r')
			{
				sockBuffer[nBytes-4] = 0;	
				InfoLog(109,sockBuffer);
				sockBuffer[nBytes-4] = '\r';	
			}
			else
			{
				InfoLog(110,sockBuffer);
			}
		}
		if(g_TraceConnection && nKeepAlive != g_nMaxKeepAlives)
		{
			GString strInfo;
			strInfo.Format("Inbound [%d:%s] (fd:%d tid:%d seq:%d type:%s)",(int)td->pTSD->nPort,(const char *)strConnectedIPAddress,(int)td->sockfd,(int)td->nThreadID,(int)td->nSequence,GetProtocolName( td->pTSD->nProtocol ));
			InfoLog(111,strInfo);
		}


		///////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Check 1 - POST (dynamic connection for protocol 8 - Messaging) - only when the switchboard server is active
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// look for a special post to root "POST / HTTP"
		if ( g_bEnableMessagingSwitchboard && memcmp(sockBuffer,"POST / ",7) == 0 )
		{

			GString strBoundary;
			char *pBoundary = strstr(sockBuffer,"boundary=");
			if (pBoundary)
			{
				char *pEnd = strpbrk(pBoundary,"\r\n ");
				if (pEnd)
				{
					char ch = *pEnd;
					*pEnd = 0;
					strBoundary = pBoundary + 9;
					*pEnd = ch;
				}
			}

			char *pContent= strstr(sockBuffer,"\r\n\r\n");
			int nContentTermLen = 4;
			if(pContent)
			{
				pContent += nContentTermLen;
			}
			char *pContent1 = pContent;

			// if all we got was the primary header in the first read - we need more data now
			if (pContent1 - sockBuffer == nBytes)
			{
				IOBlocking(td->sockfd, 0);
				int nMoreHeader = recv(td->sockfd, &sockBuffer[nBytes], MAX_DATA_CHUNK-nBytes, 0 );
				IOBlocking(td->sockfd, 1);
				 if (nMoreHeader > 0)
				 {
					nBytes += nMoreHeader;
					td->nTotalBytesRead += nMoreHeader;
				 }
				 else
				 {
					GString str;
					str.Format("Read of more header failed %d\n",(int)SOCK_ERR);
					InfoLog(112,str);
					pContent1 = 0;
				 }
			}
				
			if (pContent1)
			{
				// find the second 'end of headers'
				nContentTermLen = 4;
				pContent = strstr(pContent1,"\r\n\r\n");
				if(pContent)
				{
					pContent += nContentTermLen;
				}
			}
			int nSecondHeaderLen = pContent - pContent1;
			
			// int nFooterLen = 2 + (int)strBoundary.Length() + 2; // "--Boundary--"

			if ( pContent && pContent[0] == 'O' && pContent[1] == 'K')
			{
				int nContentArgLength = atoi(pContent+2);
				int nLenlen = 0;
				while ( pContent[2+nLenlen] >= '0' &&  pContent[2+nLenlen] <= '9')
					nLenlen++;

				// get the 'content arguments'
				GString strContentArgs(pContent+2+nLenlen, nContentArgLength);
				GStringList lstSBAction("/",strContentArgs);
				GStringIterator itArgs(&lstSBAction);
				if (itArgs())
					pMA->strMessagingReceiver = itArgs++;
				if (itArgs())
					pMA->strMessagingSender = itArgs++;
				if (itArgs())
					pMA->strMessagingTitle = itArgs++;
				if (itArgs())
					pMA->strMessagingDataInfo = itArgs++;
				if (itArgs())
					pMA->strMessagingHash = itArgs++;


				GString strHashSource;
				SwitchEntry *pSE2 = g_SwitchBoard.Pending (pMA->strMessagingReceiver, 1);
				if (pSE2)
				{
					strHashSource = pSE2->strRandomData;
				}
				else
				{
					GString strMessage;
					strMessage << "["  << pMA->strMessagingReceiver << "] Not awaiting message.";
					InfoLog(113,strMessage);
					goto CLOSE_CONNECT;
				}
				

				// todo: If this SwitchBoard connection requires certified connections it will now be certified.
				// pMA->strMessagingHash contains a ciphered version of pSE2->strRandomData based on the 
				// password for pMA->strMessagingSender, so decipher pMA->strMessagingHash and verify that it
				// matches pSE2->strRandomData, if so - this sender is certified to be who he says he is
				
				// this is only a placeholder test
				pMA->strMessagingHash.MakeReverse();

				if (pMA->strMessagingHash.Compare(pSE2->strRandomData) == 0)
				{
					// printf("PASSED Certification test\n");
				}
				else
				{
					// printf("FAILED Certification test\n");
				}

				int fdServer = pSE2->fdServer;
				int bResponse = pSE2->bResponse;

				delete pSE2;


				GString strCertificationData;
				GenerateCertificationData(strCertificationData);


				GString strSendArgs;
				strSendArgs << pMA->strMessagingReceiver << '/' << pMA->strMessagingSender << '/' 
							<< pMA->strMessagingTitle <<  '/' << pMA->strMessagingDataInfo << '/' 
							<< strCertificationData;

				sprintf(TCD.pzIP,"~%s",(const char *)pMA->strMessagingReceiver);
				td->pTSD->nProxyTimeout = 120;

				// get the content length
				int nCL = 0;
				char *pCL = stristr(sockBuffer,"Content-Length:");
				if (pCL)
				{
					pCL += 15; // first byte of content length in the HTTP header
					nCL = atoi(pCL);
				}

				// convert the POST header to a "200 OK" header.
				pMA->strMessagingHeader = "";
				int nNewContentLen = nCL - nSecondHeaderLen - nContentArgLength + (int)strSendArgs.Length();
				MakeHTTPHeader(pMA->strMessagingHeader, nNewContentLen);

				int nHTTPHeaderLen1 = (int)pMA->strMessagingHeader.Length();  

				pMA->strMessagingHeader << "OK";
				pMA->strMessagingHeader << strSendArgs.Length(); 

				
				pMA->strMessagingHeader.write(strSendArgs, strSendArgs.Length());
				
				
				// and append any message content that came too
				char *pDataStart = pContent+2+nLenlen+nContentArgLength;
				int nAllHeadersLen = (pDataStart - sockBuffer);

				if (nBytes > nAllHeadersLen)
				{
					pMA->strMessagingHeader.write(pDataStart, nBytes - nAllHeadersLen);
				}



				pInitialProxySend = (char *)(const char *)pMA->strMessagingHeader;
				nInitialRawSend = (int)pMA->strMessagingHeader.Length();


				// #of bytes to exchange before shutting down the proxy.  The proxy will shut down prior to sending the last 100 bytes or so
				// because the last few bytes are residue from the HTTP multi-part POST that has been changed into an HTTP 200 response.
				// They were read in from the POST, and they are simply thrown away.
				nProxyMaxBytes = nHTTPHeaderLen1 + nNewContentLen;

				strCertifiedMsgResponse = strCertificationData;


				
				// fill up the first block and send it to the server
				int nPostedPassthrough = nInitialRawSend;
				memcpy(sockBuffer,pInitialProxySend,nInitialRawSend);
				int nChunkSize = (nProxyMaxBytes - nInitialRawSend < MAX_DATA_CHUNK ) ? nProxyMaxBytes - nInitialRawSend : MAX_DATA_CHUNK - nInitialRawSend;
				int nGot = 0;
				if (nChunkSize)
				{
					nGot = nonblockrecv(td->sockfd,&sockBuffer[nInitialRawSend],nChunkSize);
					if (nGot < 1)
					{
						GString strMessage("failed to read in POST1:");
						strMessage << nGot << ":" << SOCK_ERR;
						InfoLog(114,strMessage);
						goto CLOSE_CONNECT;
					}
					td->nTotalBytesRead += nGot;
					nPostedPassthrough += nGot;
				}
				if (nonblocksend(fdServer,sockBuffer,nPostedPassthrough) != nPostedPassthrough)
				{
					GString strMessage("Failed to send message to server");
					InfoLog(115,strMessage);
					goto CLOSE_CONNECT;
				}
//				td->nTotalBytesProxied += nPostedPassthrough;


				// now send anything that is remaining
				while(nPostedPassthrough < nProxyMaxBytes)
				{
					nChunkSize = (nProxyMaxBytes - nPostedPassthrough < MAX_DATA_CHUNK) ? nProxyMaxBytes - nPostedPassthrough : MAX_DATA_CHUNK;
				
					nGot = nonblockrecv(td->sockfd,sockBuffer,nChunkSize);
					if (nGot < 1)
					{
						GString strMessage("failed to read in POST2");
						InfoLog(116,strMessage);
						goto CLOSE_CONNECT;
					}
					td->nTotalBytesRead += nGot;
					if (nonblocksend(fdServer,sockBuffer,nGot) != nGot)
					{
						GString strMessage("Failed to send message to server");
						InfoLog(117,strMessage);
						goto CLOSE_CONNECT;
					}
					nPostedPassthrough += nGot;
				}

				// now do one last read from the sender and throw it away, it's the HTTP multipart footer 
				nonblockrecvAny(td->sockfd,sockBuffer,MAX_DATA_CHUNK);


				if (bResponse == 0)
				{
					// the 1st time around
					g_SwitchBoard.WaitForResponse(td->sockfd, strCertifiedMsgResponse);
				}
				else // the 2nd time around
				{
					HTTPSend(td->sockfd, "OK", 2 );
					
					PortableClose(td->sockfd,"Core11");
				}


				// we detached the sender (td->sockfd) from this thread, he is back in the switchboard now
				td->sockfd = fdServer; // now attach the receiver onto this thread
				goto KEEP_ALIVE;
			}
		}

		//
		// Extension #1 - Plugin executive loader - executes user written Java, COM, C++, Perl functions
		//
		if (g_lstActivePlugins.Size())
		{
			try // todo: embed nested OS level exception handling since we're passing control off to an external developer, see the XML_TRY macro.
			{
				// todo: pass nCloseCode - set in 1000 range
				td->strAction = "Plugin";
				if (Plugin(td, sockBuffer, nBytes, nContentLen, pContent, &nKeepAlive, cryptDest, &td->nTotalBytesProxied, nPktIndex))

				{
					goto HANDLER_FINISH;
				}
				else
				{
					td->strAction = "";
					// else give another handler a crack at it - fall through
				}
			}
			catch( GException &rErr )
			{
				// This may not be an error, for example we're mapped to a wildcard method name of
				// big*, mapped to an object that contains big1, big2 and big3 - but the HTTP request 
				// asked for BigWave.html, so log the info and allow the file handler a chance to service the request.
				GString strMsg("Plugin Message:");
				strMsg << rErr.GetDescription();
				InfoLog(118,strMsg);
			}
			catch(...)
			{
				GString strMsg("Fatal error in plugin. Debug dump follows:");
				strMsg << sockBuffer;
				strMsg << "\r\n\r\n";
				InfoLog(119,strMsg);
				nCloseCode = 1000;
				goto HANDLER_FINISH;
			}
		}

#ifdef ___XFER 
	#include "../../Libraries/Xfer/Core/XferHTTPHook.cpp"
#endif

// __CUSTOM_CORE__ includes all Core hooks,  SERVERCORE_CUSTOM_HTTP includes only this HTTP hook
// This will be the most commonly used hook into ServerCore, as it can be used to build simple HTTP Services
#if defined(__CUSTOM_CORE__) && !defined(SERVERCORE_CUSTOM_HTTP) 
	#include "ServerCoreCustomHTTP.cpp"
#endif


		//////////////////////////////////////////
		// Check 2 - GET (static file - or dynamic connection)
		//////////////////////////////////////////			
		// look for an HTTP GET, it MIGHT look something like this:
		/*
			GET /Index.html HTTP/1.1
			Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, application/vnd.ms-excel, application/msword
			Accept-Language: en-us
			Accept-Encoding: gzip, deflate
			User-Agent: Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.0)
			Host: 192.168.1.102
			Connection: Keep-Alive
		*/
		// but it could be a CGI or ISAPI call as well.
		// it could also beta GET for a connection at the switchboard

		bisHead = (memcmp(sockBuffer,"HEAD",4) == 0) ? 1 : 0;
		if ( memcmp(sockBuffer,"GET",3) == 0 || bisHead )
		{

			////////////////////////////////////////////////////////////////////////////
			//  Checks for a Messaging switchboard connection
			////////////////////////////////////////////////////////////////////////////
			// Check the path of the "file" that this HTTP get is looking for.
			// If we are looking for switchboard requests....
			// and the path of this request matches the special path that identifies this as the switchboard ---
			if ( g_bEnableMessagingSwitchboard && g_strMessagingSwitchPath.Length() )
			{
				char ch = sockBuffer[4 + g_strMessagingSwitchPath.Length()]; // save and null the first byte past the path
				sockBuffer[4 + g_strMessagingSwitchPath.Length()] = 0;
				// if the path to the "file" this HTTP GET is looking in matches
				// the "name" entry under [SwitchBoardServer], which might be a value
				// something like this "/MySwitchServerPath/"......
				if ( g_strMessagingSwitchPath.CompareNoCase(&sockBuffer[4]) == 0)
				{
					// then this is a server checking for a remote-waiting client connection on this machine.

					// restore first byte past the path since the comparison is done.
					sockBuffer[4 + g_strMessagingSwitchPath.Length()] = ch;

					// sockBuffer = "/PublicPath/put/DOMAIN 
					// being indexed at the word put;
					if (sockBuffer[4 + g_strMessagingSwitchPath.Length()] == 'p' && sockBuffer[5 + g_strMessagingSwitchPath.Length()] == 'u' && sockBuffer[6 + g_strMessagingSwitchPath.Length()] == 't')
					{
						
						// User sends "When can I put the Message?" as
						// GET /PublicPath/put/UBTsAccountForYou HTTP/1.1
						char *pCmdEnd = strstr(&sockBuffer[8 + g_strMessagingSwitchPath.Length()]," HTTP/");
						if (pCmdEnd)
							*pCmdEnd = 0;

					
						GString strRandomData;
						GenerateCertificationData(strRandomData);

						// wait for an entry in the SwitchBoard with the "DOMAIN" of 
						// 	"GET /PublicPath/put/DOMAIN HTTP/1.?" sent here on port 80.

						GString strConnectName;
						if (g_SwitchBoard.WaitForConnection (&sockBuffer[8 + g_strMessagingSwitchPath.Length()],strConnectName, strRandomData))
						{
							// found and placed in a special table of connections.
							
							// Server sends "Now", and here's some random data that you will
							// need to encrypt or hash and send with the file if you must "certify"
							// your connection before sending to this recipient.

							// make http message, send it, close connection and return to pool.
							GString strMsg;
							strMsg.Format("OK%s/%s",(const char *)strRandomData, (const char *)strConnectName);
							HTTPSend(td->sockfd, strMsg, (int)strMsg.Length() );
						}
						else
						{
							HTTPSend(td->sockfd, "NO", 2 ); // Server sends "Not yet."
						}
						
						goto KEEP_ALIVE;
					}

					char *pStart = &sockBuffer[4 + g_strMessagingSwitchPath.Length()];
					char *pEnd = strstr(pStart,"HTTP/");
					if (pEnd)
					{
						pEnd--;
						*pEnd = 0;

						// see if there is a client waiting to connect to this request.
						// Returns true if handoff happens otherwise false.
						// It is asking: "Is a [pStart] ready to send?"

						if (g_SwitchBoard.HandOffConnection (td->sockfd, pStart))
						{

							// there was a client waiting, td->sockfd is now being serviced
							// by another thread, so this thread goes back to the pool.
							// The connection does NOT get closed on this thread.
							goto SHUTTING_DOWN;
						}
						else
						{
							// no connections are waiting, close this connection then go back to the pool.
							// an HTTP response 'NO' must be sent to this client notifying him that there was no pickup.
							HTTPSend(td->sockfd, "NO", 2 );

							goto CLOSE_CONNECT;
						}
					}
				}
				else
				{
					// put [ch] back where the string was temporarily null terminated for the comparison.
					// this is not a switchboard request.
					sockBuffer[4 + g_strMessagingSwitchPath.Length()] = ch;
				}
			}
		
		
			////////////////////////////////////////////////////////////////////////////
			//  If this is not a CGI or ISAPI call then return a static file
			////////////////////////////////////////////////////////////////////////////
			// look for something like this (a CGI or ISAPI call)
			// GET /XMLServer/UBT.dll?TransactXML=......"
			// the ? must be in the first line of the HTTP header
			char *p = 0;
			char *pGETEnd = strpbrk(sockBuffer,"\r\n");
			if (pGETEnd)
			{
				char pSave = *pGETEnd;
				*pGETEnd = 0;
				p = strpbrk(sockBuffer,"?");  
				*pGETEnd = pSave;
			}
			
			// if there's no ?, this is not a CGI or ISAPI call
			// for a static document/image/javascript etc...
			if (!p) 
			{
				// If we're configured to look for HTTP GET's on this port go-on
				if ( td->pTSD->nProtocol == 1 ||	td->pTSD->nProtocol == 5 )
				{
					//pzHome = GetProfile().GetString("HTTP","Home",false);

					// get the file (from memory or disk), assemble the HTTP response, and send it back.
					nKeepAlive = ReturnFile(td, &sockBuffer[4] ,nBytes - 4,strConnectedIPAddress, nKeepAlive, bisHead, sockBuffer, cryptDest, &td->nTotalBytesProxied);
					goto HANDLER_FINISH;
				}
			}
			
			// When posting data through a GET, from a browser, the 
			// URL Data has certain characters hex encoded (a space becomes %20 for example)
			// CGI or ISAPI calls get their param values unescaped at this point.
			/*int nNewDataLen = */unEscapeData(p + 1,p + 1);
			
			goto HANDLER_FINISH;
		}
		
		//////////////////////////////////////////
		// Check 3 - default POST handler 
		//////////////////////////////////////////			
		if ( memcmp(sockBuffer,"POST",4) == 0 )
		{
			GString strPostedTo;
			char *pSep = strpbrk(&sockBuffer[5],"\r\n ");
			if (pSep)
			{
				char ch = *pSep;
				*pSep = 0;
				strPostedTo = &sockBuffer[5];
				*pSep = ch;
			}
			nKeepAlive = 0;
			GString str;
			str.Format("Posted %u bytes, ignored - no POST handler registered for [%s].\n",(int)nContentLen,(const char *)strPostedTo);
			InfoLog(120,str);
		}


HANDLER_FINISH:
	
		// there has to be some reasonable limit to prevent denial of service attack
		if (nKeepAlive > 0 && nKeepAlive != g_nMaxKeepAlives)   
		{
			goto KEEP_ALIVE;
		}

		if (nCloseCode == 1200) // unless otherwise specified, the reason we are closing this connection is:
			nCloseCode = 1277; // "1. Not using Keep alives, or 2. Max keep alives per connection reached or 3. Closed by transaction handler."

		
#ifdef ___TXML // UBT Server extension for TransactXML
	    #include "ServerDispatch.cpp"
#endif

		} // end of branch2
		
	}
#ifdef _XMLF_JIT_
	XML_CATCH(ex)
	{
		GString strLog;
		strLog.Format("XML_CATCH=%s\n",ex.GetDescription());
		InfoLog(121,strLog);

		PROPIGATE_SERVER_EXCEPTION;
	}
#endif // _XMLF_JIT_

CLOSE_CONNECT:  // this goto tag was previously named SOCKET_ERR_ABORT - it was renamed because it does not necessarily indicate error

	td->nProxyClosed = 1; // shared memory flag if a worker thread was started - stop it.


//	nCloseCode = 1110; // no alternate specified
//	nCloseCode = 1113; // end this thread too
//	nCloseCode = 1151; 


	if (td->sockfd != -1)
		PortableClose(td->sockfd,"Core12");

	// once the connection is closed it must be re-authorized
	bConnectionAuthorized = 0;
	// ----------------------------------------------------------------------------------------------------------
	// The connection is closed and the client has the response(or it's in transit).  We are "off the clock" now
	// ----------------------------------------------------------------------------------------------------------
	if (g_TraceThread)
	{
		GString strLog;
		strLog.Format("Thread %d  Service time ~:%d ms",(int)td->nThreadID, (int)getTimeMS() - td->starttime);
		InfoLog(122,strLog);
	}
	if(g_TraceConnection)
	{
		GString strTime;
		int nElapsed = getTimeMS() - td->starttime;
		if (nElapsed > 120000)
			strTime << ((getTimeMS() - td->starttime) / 1000) / 60 << "min"; // minutes over 2
		else if (nElapsed > 5000 && nElapsed < 120000)
			strTime << (getTimeMS() - td->starttime) / 1000 << "sec"; // seconds between 5 seconds and 120 seconds
		else
			strTime << (getTimeMS() - td->starttime) << "ms"; // milliseconds less than 5000

		GString strMessage;
		strMessage.Format("Closed Inbound [%d:%s] (fd:%d tid:%d  %s  seq=%d) code:[%d:%d] nProxyfd:%d  bytes out=%I64d",
			(int)td->pTSD->nPort,(const char *)strConnectedIPAddress, (int)td->sockfd, (int)td->nThreadID, strTime.Buf(), (int)td->nSequence, (int)nCloseCode, (int)SOCK_ERR, td->nProxyfd,(__int64)td->nTotalBytesProxied );
		strMessage << " bytes to client=" <<  td->nTotalBytesSent <<  "  bytes from client=" << td->nTotalBytesRead;

		InfoLog(123,strMessage);
	}
	td->nTotalBytesRead = 0;
	td->nTotalBytesProxied = 0;

SHUTTING_DOWN:

	SERVER_MEMORY_RESET;
	if (cryptDest)
	{
		if (g_nLockThreadBuffers == 0)
		{
			delete cryptDest;
			cryptDest = 0;
			sockBuffer = 0;
			td->Buf1 = 0;
			td->Buf2 = 0;
		}
		else 
		{
			sockBuffer[0] = 0;
			cryptDest[0] = 0;
		}
	}


	td->strAction = "";

	// decrement the connection resource monitor
	// clientThread() has finished handling this connection
	if (strConnectedIPAddress.Length())
		ConnectionCounter(strConnectedIPAddress,-1); 

	unsigned long timenow = time(0);
	while (bHelperThreadIsRunning)
	{
		if (time(0) - timenow > 10)
		{
			// This is a development safety net.  
			// Under no known conditions can this code be reached.
			InfoLog(124,"Thread Hung.  You need to stop then restart this application to correct this problem");
			
			// Threads should not kill other threads.  Fix the problem.
		}
		gsched_yield();
	}

	// this thread and it's helper are finished, signal the event
	gthread_cond_signal( &td->condFinished ); 

	// don't free the memory, just go back and wait for another transaction.
	if (!g_ServerIsShuttingDown && td->m_bUseThreads )
	{
		goto MAIN_THREAD_BODY;
	}

	// release any additional "per protocol" transactional memory and exit this thread.
	// delete inboundXML;
	SERVER_MEMORY_CLEANUP;
	if (td->m_bUseThreads)
	{
		gthread_exit(0);
	}
	return 0;
}




int MessageRouteInfo(int fd,char *pzConnectRoute,char *pzIP,int *nAction, int *nConnectRouteSize,char *pzPassword, char *pRouteData, int nRouteDataLen )
{
	if (nRouteDataLen < 5)
		return 0; // invalid protocol

	int nRouteDataIndex = 0;

	unsigned short theNumber = 0;					  // storage on platform architecture byte boundary
	memcpy(&theNumber,&pRouteData[nRouteDataIndex],2);// storage in network byte order moved to architecture byte boundary storage 
	short nParams = ntohs( theNumber );				  // convert to host byte order

	nRouteDataIndex = 2;
	
	memcpy(&theNumber,&pRouteData[nRouteDataIndex],2);
	short nRouteMsgSize = ntohs( theNumber );
	nRouteDataIndex += 2;

	if (nRouteDataLen < nRouteMsgSize+nRouteDataIndex)
		return 0; // invalid protocol

	GString strRoute;
	strRoute.write(&pRouteData[nRouteDataIndex],nRouteMsgSize);
	nRouteDataIndex += nRouteMsgSize;

	/////////////////////////////////////////////////////////////////////////////////////////////////
	GStringList lstDeliveryIPs("|",(const char *)strRoute );

	if (nParams == 2) // password only sent to local host - not over the network
	{
		if (nRouteDataLen < nRouteDataIndex+2)
			return 0; // invalid protocol

		unsigned short theNumber = 0;						// storage on platform architecture byte boundary
		memcpy(&theNumber,&pRouteData[nRouteDataIndex],2);	// storage in network byte order moved to architecture byte boundary storage 
		short nPassSize = ntohs( theNumber );				// convert to host byte order
		nRouteDataIndex += 2;

		if (nRouteDataLen < nRouteDataIndex+nPassSize || nPassSize > 1024)
			return 0; // invalid protocol

		
		memcpy(pzPassword, &pRouteData[nRouteDataIndex], nPassSize);

		pzPassword[nPassSize] = 0;

	}
	else if (nParams != 1)
	{
		return 0; // protocol error, only 1 or 2 params (local or not-local)
	}

	GString strFirstIP(lstDeliveryIPs.PeekFirst());
	if (strFirstIP.GetAt(0) == '~')
	{
		lstDeliveryIPs.RemoveFirst();		  //  remove the ~connectionName
		lstDeliveryIPs.AddFirst("127.0.0.1"); // 'this' will get popped off
		strFirstIP = "127.0.0.1";
	}


	// if the IP is prefixed with the port "777@ThisHost"
	char *pSep = (char *)strpbrk((const char *)strFirstIP,"@");
	const char *pMeTest = (pSep) ? pSep + 1 : strFirstIP;

	if ( IsMe( pMeTest  ) )
	{
		if ( lstDeliveryIPs.Size() > 1 )
		{
			// pop this hop
			lstDeliveryIPs.RemoveFirst();

			__int64 nipLen = 0;
			const char *ip = lstDeliveryIPs.PeekFirst(&nipLen);
			if (nipLen > 1023)
				return 0;
			strcpy(pzIP, ip);
			
			const char *pzRoute = lstDeliveryIPs.Serialize("|");
			int nRouteLen = strlen(pzRoute);
			if (nRouteLen > 1019)
				return 0; // invalid protocol

			strcpy(&pzConnectRoute[4],pzRoute);
			unsigned short sh = htons(nRouteLen); 
			memcpy(&pzConnectRoute[2],&sh,sizeof(short));
			sh = htons(1); 
			memcpy(&pzConnectRoute[0],&sh,sizeof(short));
			*nConnectRouteSize = nRouteLen + 4;

			*nAction = 2;
		}
		else
		{
			// destination (proxy-->server), decrypted from tunnel and serviced by this machine
			*nConnectRouteSize = 0;
			*nAction = 3;
			pzIP[0] = 0;
		}
	}
	else
	{
		// tunnel (client-->proxy), raw VNC into tunnel crypted and sent to proxy
		__int64 nipLen = 0;
		if (nipLen > 1023)
			return 0; // invalid protocol
		strcpy(pzIP,lstDeliveryIPs.PeekFirst());

		const char *pzRoute = lstDeliveryIPs.Serialize("|");
		int nRouteLen = strlen(pzRoute);
		if (nRouteLen > 1019)
			return 0; // invalid protocol
		strcpy(&pzConnectRoute[4],pzRoute);
		unsigned short sh = htons(nRouteLen); 
		memcpy(&pzConnectRoute[2],&sh,sizeof(short));
		sh = htons(1); 
		memcpy(&pzConnectRoute[0],&sh,sizeof(short));
		*nConnectRouteSize = nRouteLen + 4;
		
		*nAction = 1;
	}
	return 1;
}



int g_nUniqueSequence = 0; //only used by BuildConnectionPostData()
void BuildConnectionPostData(int tid, GString &strDestination, const char *pzConnectionFileName, const char *pzCfgSection, const char *pzProxyFirewall = 0,const char *pzHTTPAuth = 0, const char *pzHost = 0)
{
	
	// The strNonCacheTag is appended onto the URL for two reasons.
	// First, it guarantees that every requested URL is unique and therefore will not be cached by either a corporate HTTP proxy or by the ISP as many ISPs now manage caching for you.
	// Second, it includes this thread id, and the current Queue depth - information that the switchboard can use to detect if this connection supersedes a previous connection since TCP
	// has no way of notifying the remote side that a connection has been closed without a KeepAlive ping, that is expensive and not supported within a pure HTTP protocol.
	// %25 is a URL encoded '%'
	GString strNonCacheTag("%25");
	strNonCacheTag << tid << "/"  << g_nUniqueSequence++ << "/" <<  time(0);


	GString strAuth;
	if (pzHTTPAuth && pzHTTPAuth[0])
	{
		BUFFER b;
		BufferInit(&b);
		uuencode((unsigned char *)pzHTTPAuth, strlen(pzHTTPAuth), &b); 
		GString strEncoded((char *)b.pBuf, b.cLen);
		BufferTerminate(&b);
		strAuth.Format("Authorization: Basic %s\r\n",(const char *)strEncoded);

	}
	
	
	// get the host 
	GString strHost(pzHost);

	
	GString strProxtDestPort;
	int nProxyDestPort = GetProfile().GetInt(pzCfgSection,"Port",false);
	if (nProxyDestPort > 0)
	{
		strProxtDestPort << ":" << nProxyDestPort;
	}


	GString strProxyFirewall;
	if (pzProxyFirewall && pzProxyFirewall[0])
	{
		strProxyFirewall = pzProxyFirewall;
		strHost = pzProxyFirewall;
	}
	else
	{
		strProxyFirewall = GetProfile().GetString(pzCfgSection,"ProxyTo",false);
	}



	if ( strProxyFirewall.Length() )
	{
		// Note: This is a sample header between NS and a standard software proxy
		// ----------------------------------------------------------------------
		// GET http://www.Domain.com/Index HTTP/1.1
		// Host: www.Domain.com
		// User-Agent: Mozilla/5.0 (Windows; U; Windows NT 5.0; en-US; rv:0.9.4) Gecko/20011019 Netscape6/6.2
		// Accept: text/xml, application/xml, application/xhtml+xml, text/html;q=0.9, image/png, image/jpeg, image/gif;q=0.2, text/plain;q=0.8, text/css, */*;q=0.1
		// Accept-Language: en-us
		// Accept-Encoding: gzip, deflate, compress;q=0.9
		// Accept-Charset: ISO-8859-1, utf-8;q=0.66, *;q=0.66
		// Keep-Alive: 300
		// Proxy-Connection: keep-alive

		// Note: This is a sample header between IE and a standard software proxy
		// ----------------------------------------------------------------------
		// GET http://www.Domain.com/Index.html HTTP/1.0
		// Accept: */*
		// Accept-Language: en-us
		// User-Agent: Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.0)
		// Host: www.Domain.com
		// Proxy-Connection: Keep-Alive

		strDestination.Format(
			"GET http://%s%s%s%s HTTP/1.1\r\n"
			"Host: %s\r\n"
			"User-Agent: Xfer\r\n"
			"Keep-Alive: 300\r\n"
			"%s" // HTTP Authentication 
			"Proxy-Connection: keep-alive\r\n\r\n",(const char *)strProxyFirewall,strProxtDestPort.Buf(),pzConnectionFileName,(const char *)strNonCacheTag, (const char *)strProxyFirewall,(const char *)strAuth);
		
	}
	else
	{
		///////////////////////////////////////////////////////////////////////////////////////////////////
		//				Sample HTTP GET from Netscape 6.2 (http headers say it's still 6.1)
		// If they fix that - you'll never know what version of 6.2 you have by looking at the HTTP headers.
		///////////////////////////////////////////////////////////////////////////////////////////////////
		//GET /index.html HTTP/1.1												
		//Host: 192.168.1.104													
		//User-Agent: Mozilla/5.0 (Windows; U; Windows NT 5.0; en-US; rv:0.9.2) Gecko/20010726 Netscape6/6.1	
		//Accept: text/xml, application/xml, application/xhtml+xml, text/html;q=0.9, image/png, image/jpeg, image/gif;q=0.2, text/plain;q=0.8, text/css, */*;q=0.1 
		//Accept-Language: en-us												
		//Accept-Encoding: gzip,deflate,compress,identity						
		//Accept-Charset: ISO-8859-1, utf-8;q=0.66, *;q=0.66					
		//Keep-Alive: 300														
		//Connection: keep-alive												

		///////////////////////////////////////////////////////////////////////////////////////////////////
		//								Sample HTTP GET from IE 6.0
		///////////////////////////////////////////////////////////////////////////////////////////////////
		//GET /Index.html HTTP/1.1												
		//Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, application/vnd.ms-excel, application/msword, */*			
		//Accept-Language: en-us												
		//Accept-Encoding: gzip, deflate										
		//User-Agent: Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.0)		
		//Host: 127.0.0.1														
		//Connection: Keep-Alive												

		strDestination.Format(
			"GET %s%s HTTP/1.1\r\n"		
			"Host: %s\r\n"
			"User-Agent: Xfer\r\n"
			"Keep-Alive: 300\r\n"
			"Accept: */*\r\n"
			"%s" // HTTP Authentication 
			"Connection: keep-alive\r\n\r\n",pzConnectionFileName, (const char *)strNonCacheTag, (const char *)strHost,(const char *)strAuth);

	}
}


class MessagePollData
{
public:
	// always filled
	GString strServer; 
	int nPort;
	GString strProxyTo; 
	GString strSwitchBoardPath; 

	// if using HTTP authentication the make a request outside the lcoal network
	GString strLocalNetworkAuthenticate; // "user:password"

	// a valid value if this structure resides in a g_msgPollSenders
	GString strSender;
	GString strPassword;
	GString strUserName; 
	GString strPollPathAndName; 
	GString strSectionName; 

	// empty unless this structure reside in a g_msgPollSites
	GStringList lstPollFor;
};


// a MessagePollData per unique(Server + Port + ProxyTo)
GBTree g_msgPollSites;

// a MessagePollData per known 'send to'
GBTree g_msgPollSenders;


void *messagePollUpdateThread(void *arg)
{
	// load defaults for all locations
	GString strServer;
	int nPort;
	GString strProxyTo;
	if ( GetProfile().GetBoolean("Messaging","UseBrowserProxy",false) )
	{
		strServer = GetProfile().GetString("Messaging","BrowserProxy",false); 
		nPort = GetProfile().GetInt("Messaging","BrowserProxyPort",false);
		strProxyTo = GetProfile().GetString("Messaging","DefaultSwitchBoardServer",false); 
	}
	else
	{
		strServer = GetProfile().GetString("Messaging","DefaultSwitchBoardServer",false); 
		nPort = GetProfile().GetInt("Messaging","DefaultSwitchBoardPort",false);
	}

	GStringList lstSenders(",",GetProfile().GetString("Messaging","AcceptFrom",false)); 
	GStringIterator it(&lstSenders);
	while (it())
	{
		MessagePollData *pMPD = new MessagePollData;

		pMPD->strSender = it++;
		pMPD->strSectionName = "MsgFrom-";
		pMPD->strSectionName += pMPD->strSender;
		
		if ( !GetProfile().GetBoolean(pMPD->strSectionName,"Enable",false) || !GetProfile().GetBoolean(pMPD->strSectionName,"CheckAtSwitchBoard",false) )
		{
			delete pMPD;
			continue;
		}

	
		pMPD->strLocalNetworkAuthenticate = GetProfile().GetString("Messaging","LocalNetworkAuthenticate",false); 


		int bUseProxy = GetProfile().GetBoolean("Messaging","UseBrowserProxy",false);
		if (bUseProxy)
		{
			pMPD->strServer = strServer;
			
			GString str = GetProfile().GetString(pMPD->strSectionName,"SwitchBoardServer",false); 
			if (str.Length())
				pMPD->strProxyTo = str;
			else
				pMPD->strProxyTo = strProxyTo;


			pMPD->nPort = nPort;

		}
		else
		{
			GString str = GetProfile().GetString(pMPD->strSectionName,"SwitchBoardServer",false); 
			if (str.Length())
				pMPD->strServer = str;
			else
				pMPD->strServer = strServer;
			
			pMPD->nPort = GetProfile().GetInt(pMPD->strSectionName,"SwitchBoardPort",false);
			if (pMPD->nPort < 1)
				pMPD->nPort = nPort;
		
			pMPD->strProxyTo = "";
		}

		// load the specific recipient override settings
		pMPD->strPollPathAndName = GetProfile().GetString(pMPD->strSectionName,"Name",false); 

		pMPD->strPassword = GetProfile().GetString(pMPD->strSectionName,"Password",false); 

		// build poll location key (Server + Port + ProxyTo)
		GString strPollLocation;
		strPollLocation << pMPD->strServer << pMPD->nPort << pMPD->strProxyTo;
		
		if (pMPD->strPollPathAndName.Length())
		{
			const char *pPollName = GDirectory::LastLeaf( pMPD->strPollPathAndName );
			
			MessagePollData *pMPDList = (MessagePollData *) g_msgPollSites.search(strPollLocation);
			if (pMPDList)
			{
				
				pMPDList->lstPollFor.AddLast(pPollName);
			}
			else
			{
				pMPD->lstPollFor.AddLast(pPollName);
				
				char *pzPollPathAndName = (char *)(const char *)pMPD->strPollPathAndName;
				int i=strlen(pzPollPathAndName)-1;
				for(;pzPollPathAndName[i]; i-- )
					if (pzPollPathAndName[i] == '\\' || pzPollPathAndName[i] == '/')
						break;
				char ch = pzPollPathAndName[i+1];
				pzPollPathAndName[i+1] = 0;
				pMPD->strSwitchBoardPath = pzPollPathAndName;
				pzPollPathAndName[i+1] = ch;

				g_msgPollSites.insert(strPollLocation, (void *)pMPD);
			}
		}

		g_msgPollSenders.insert(pMPD->strSender, (void *)pMPD);
	}
return 0;
}


class MessageSendArgs
{
public:
	// address of Switchboard server UNLESS sending through a proxy-firewall
	// in that case "strServer" should contain the address of the proxy-firewall.
	GString strServer; 
	
	// port of Switchboard server (80) UNLESS sending through a proxy-firewall
	// in that case "nPort" should contain the port of the proxy-firewall. (80,81,8080,3128,???)
	int nPort;

	// EMPTY, unless (strServer & nPort) refer to a proxy-firewall.  If they do
	// then "strSwitchBoardServer" is the address of the strSwitchBoardServer that
	// "strServer" should pass the message on to.
	GString strSwitchBoardServer;


	// If the internal network is configured to prompt a browser for a user name/password
	// and it's using the "Basic" authentication of HTTP, then this value should be in the form
	// "user:password" 
	GString strBasicHTTPAuth;

	//////////////////////////////////
	// This is the data that is the content of the message, it my be supplied either as a file
	// on disk as fully qualified path and file name to send: like "c:\Path\File.doc" or "/usr/bin/file.doc"
	GString strFileToSend;
	// or you may load the message into this GString
	GString strMessageToSend;
	//////////////////////////////////


	// a value like (/PublicPath/), value is defined at the [SwitchboardServer] config file
	// and must be known by both ends polling into it.  Value must begin AND end with a '/'
	GString strPublicPathName;

	// the name that you will exchange the file under, If the 'poller' is configured
	// to poll for "/PublicPath/eSocks.com" then you will meet under the name 'eSocks.com'
	// This value should NOT contain the path (/PublicPath/) just the name.
	GString strConnectName;


	// name of the 'user' sending, "strConnectName" is like the domain or company name
	// and "strUserName" may specify the purpose of the file or decoding procedures for the
	// receiver.
	GString strUserName;
	

	// the title of the message like "file.doc" that will be written on the remote machine.
	// if the remote machine has specifically granted access for this connection then you
	// can use a value like this "\\\\CorpServer\\Dept21\\Project777\\PutThis.doc".
	// To enable unrestricted values the set "[MsgFrom-???] LetSenderPlaceFile=yes" in 
	// config file of the polling machine.
	GString strRemoteTitle;

	// after all the data has been sent from the switchboard to the receiver
	// this value is sent back not to confirm delivery but to confirm transmit.
	// This value can be used to later certify the delivery after the receiver
	// has acknowledged that the data arrived and was in the expected format
	//GString strDeliveryID;


	// Any value in strResopnse, guarantees that the message was delivered.
	GString strResponse;


	// when set to any non-zero number MessageSend() will immediately return
	// and the caller is responsible for checking the results at a later time.
	int bExecuteAsync;

	/////////////////////////////////////////////////////////////////////////////
	// the following arguments are 'out' args that are assigned by MessageSend()
	/////////////////////////////////////////////////////////////////////////////
	// updated realtime to indicate # of bytes sent
	int nTotalDataSent;
	
	// the number of bytes being sent can be watched to show progress
	int nFileBytesToSend;

	// 1 - in progress
	// 2 - done
	// any other value is an error code
	int nResult;
	GString strErrorDescription;
	MessageSendArgs()
	{
		bExecuteAsync = 0;
		nTotalDataSent = 0;
	}
};

// POSTS args->strFileToSend to the switchboard
int POSTMessage(int sockfd, MessageSendArgs	*args, GString &strData)
{
	// build the POST header

	// determine the size of the data to send
	long lBytes = (long)args->strMessageToSend.Length();
	FILE *fp = 0;
	if (args->strFileToSend.Length())
	{
		fp = fopen(args->strFileToSend,"rb");
		if (!fp)
		{
			args->strErrorDescription.Format("Failed to open local file[%s] to send.",args->strFileToSend.Buf());
			args->nResult = 8;
			return 0; 
		}

		// get the size of the file
		fseek(fp,0,SEEK_END);
		lBytes = ftell(fp);
		args->nFileBytesToSend = lBytes;
		fseek(fp,0,SEEK_SET);
	}




	GString strBoundary;
	strBoundary.Format("---------------------------%010d",time(0));

	GString strBoundaryHeader = "--";
	strBoundaryHeader += strBoundary;
	strBoundaryHeader += "\r\n";
	strBoundaryHeader += "Content-Disposition: form-data; name=\"File\"; filename=\"";
	strBoundaryHeader += time(0); strBoundaryHeader +="\"\r\n";
	strBoundaryHeader += "Content-Type: application/octet-stream\r\n\r\n";
	

	GString strBoundaryFooter;
	strBoundaryFooter.Format("\r\n--%s\r\nContent-Disposition: form-data; name=\"Submit\"\r\n\r\nSubmit\r\n--%s--\r\n",(const char *)strBoundary,(const char *)strBoundary);


	GString strDataInfo;
	strDataInfo.Format("%d|%s",(int)lBytes+strBoundaryFooter.Length(),(const char *)strBoundary);


	// todo: strData contains a value that needs to be hashed or ciphered, reversing it is not much security - but it does sufficiently test the process.
	strData.MakeReverse();


	// build the header
	GString strHeader;
	GString strSubSubHeader = args->strConnectName << '/' << args->strUserName << '/' << args->strRemoteTitle << '/' << strDataInfo << '/' << strData;
	GString strSubHeader("OK");
	strSubHeader += strSubSubHeader.Length();
	strSubHeader += strSubSubHeader;

	// file + subheader + boundary header + boundary footer (with leading -- and trailing -- (4) )
	int nContentPostLen = lBytes + (int)strSubHeader.Length() + (int)strBoundaryHeader.Length() + (int)strBoundary.Length() + 4;

	// http://www.w3.org/Protocols/rfc1341/7_2_Multipart.html

	GString strAuth;
	if (args->strBasicHTTPAuth.Length())
	{
		BUFFER b;
		BufferInit(&b);
		uuencode((unsigned char *)(const char *)args->strBasicHTTPAuth, (int)args->strBasicHTTPAuth.Length(), &b); 
		GString strEncoded((char *)b.pBuf, b.cLen);
		BufferTerminate(&b);
		strAuth.Format("Authorization: Basic %s\r\n",(const char *)strEncoded);
	}


	GString strIPHost;
	char pzHostName[512];
	gethostname(pzHostName,512); //uname() on some systems
	
	int nDNSResolutionRetries;
	nDNSResolutionRetries = 0;
DNSResolutionRETRY:
	struct hostent *pHELocal = (struct hostent *)gethostbyname((const char *)pzHostName);
	if (!pHELocal)
	{
		if (nDNSResolutionRetries++ < 5)
		{
			PortableSleep(0,250000 * nDNSResolutionRetries);
			goto DNSResolutionRETRY;
		}
		// Info:Failed to resolve[%s].
		if(g_TraceConnection)
		{
			args->strErrorDescription.Format("*******FAILED to resolve DNS for[%s]", pzHostName);
			InfoLog(300,args->strErrorDescription);
			args->nResult = 100;
			return 0; 
		}
	}

	struct sockaddr_in my_addr; 
	memcpy((char *)&(my_addr.sin_addr), pHELocal->h_addr,pHELocal->h_length); 
	memset(&(my_addr.sin_zero),0, 8);// zero the rest of the (unused) struct
	strIPHost = inet_ntoa(my_addr.sin_addr);


	if ( args->strSwitchBoardServer.Length() )
	{
		// todo: this assumes we always connect on port 80 after passing through the HTTP proxy - it should be allowed to go anywhere

		strHeader = "POST http://";
		strHeader += args->strSwitchBoardServer;
		strHeader += "/ HTTP/1.0\r\n";
		strHeader += "Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, application/vnd.ms-excel, application/msword, */*\r\n";
		strHeader += "Accept-Language: en-us\r\n";
		strHeader += "Content-Type: multipart/form-data; boundary="; strHeader += strBoundary; strHeader += "\r\n";
		strHeader += "Proxy-Connection: Keep-Alive\r\n";
		strHeader += "User-Agent: Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.0)\r\n";
		strHeader += "Host: "; strHeader += args->strSwitchBoardServer; strHeader +="\r\n";
		strHeader += "Content-Length: "; strHeader += nContentPostLen; strHeader +="\r\n";
		strHeader += strAuth;
		strHeader += "Pragma: no-cache\r\n\r\n";
	}
	else
	{
		strHeader = "POST / HTTP/1.0\r\n";
		strHeader += "Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, application/vnd.ms-excel, application/msword, */*\r\n";
		strHeader += "Accept-Language: en-us\r\n";
		strHeader += "Content-Type: multipart/form-data; boundary="; strHeader += strBoundary; strHeader += "\r\n";
		strHeader += "Proxy-Connection: Keep-Alive\r\n";
		strHeader += "Connection: Keep-Alive\r\n";
		strHeader += "User-Agent: Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.0)\r\n";
		strHeader += "Host: "; strHeader += strIPHost; strHeader +="\r\n";
		strHeader += "Content-Length: "; strHeader += nContentPostLen; strHeader +="\r\n";
		strHeader += strAuth;
		strHeader += "Pragma: no-cache\r\n\r\n";
	}
	strHeader += strBoundaryHeader;
	strHeader += strSubHeader;

	char pFileSendBuffer[8192];


	// send it all in a single send() for small messages
	if (strHeader.Length() + lBytes < 8192)
	{
		if (args->strMessageToSend.Length())
		{
			strHeader << args->strMessageToSend;
		}
		else
		{
			fread(pFileSendBuffer,sizeof(char),lBytes,fp);
			strHeader.write(pFileSendBuffer,lBytes);
		}
		
		
		strHeader += strBoundaryFooter;

		int nHeadSent = nonblocksend(sockfd, (const char *)strHeader, (int)strHeader.Length());
		if ( nHeadSent != strHeader.Length() )
		{
			args->strErrorDescription.Format("Failed to send Message code:%d:%d",(int)nHeadSent,(int)SOCK_ERR);
			args->nResult = 9;
			if (fp)
				fclose(fp);
			return 0; 
		}
	}
	else // chop up the message and send it
	{
		// now send the header
		int nHeadSent = nonblocksend(sockfd, (const char *)strHeader, (int)strHeader.Length());
		if ( nHeadSent != strHeader.Length() )
		{
			args->strErrorDescription.Format("Header send failed %d:%d \n\n",(int)nHeadSent,(int)SOCK_ERR);
			args->nResult = 10;
			fclose(fp);
			return 0; 
		}
		else
		{
			// now send the data, in this case a file. 
			int nSendStringIndex = 0;
			while(lBytes > 0)
			{
				int nChunkSize = (lBytes < 8192) ? lBytes : 8192; 

				if (args->strMessageToSend.Length())
				{
					memcpy(pFileSendBuffer,args->strMessageToSend.StartingAt(nSendStringIndex),nChunkSize);
					nSendStringIndex += nChunkSize;
				}
				else
				{
					if (fread(pFileSendBuffer,sizeof(char),nChunkSize,fp) != (unsigned int)nChunkSize)
					{
						InfoLog(301,"Failed to read from source send file\n");
						break; 
					}
				}
				lBytes -= nChunkSize;


				
				int nRslt = nonblocksend(sockfd, pFileSendBuffer, nChunkSize);
				if ( nRslt < 1 )
				{
					args->strErrorDescription.Format("Failed to send file chunk %d:%d",(int)nRslt,(int)SOCK_ERR);
					args->nResult = 11;
					break;
				}
				else if ( nRslt != nChunkSize )
				{
					// if this ever happens
					args->strErrorDescription.Format("Failed to send %d:%d:%d",(int)nRslt,(int)SOCK_ERR,(int)nChunkSize);
					args->nResult = 12;
					break;
				}
				else
				{
					// just sent [nChunkSize] bytes of file
					args->nTotalDataSent += nChunkSize;
				}
			}
			
			// send boundary footer, if that's all that's left to send
			if (lBytes == 0)
			{
				nonblocksend(sockfd, (const char *)strBoundaryFooter, (int)strBoundaryFooter.Length());
			}
			else
			{
				if (fp)
					fclose(fp);

				return 0; // error
			}
		}
	}
	if (fp)
		fclose(fp);
	
	return 1;
}

int MessagingPlugin(DriverCallBackArg *prd, const char *pzData, int nLength, char *pBuf, int nBufSize, const char *pzPluginhandler, GString *pstrReply)
{
	GStringList lstArgs("::",pzPluginhandler);
	GStringIterator it(&lstArgs);
	const char *pzLanguage = 0;
	const char *pzComponent = 0;
	const char *pzInterface = 0;
	const char *pzMethod = 0;
	if (it())
		pzLanguage = it++;
	if (it())
		pzComponent = it++;
	if (it())
		pzInterface = it++;
	if (it())
		pzMethod = it++;

	if (!pzLanguage || !pzComponent || !pzInterface || !pzMethod)
	{
		InfoLog(302,"Messaging Plugin Handler is misconfigured.  Should be... Language::Component::Interface::Method");
		return 0;
	}

	InterfaceInstance *pII = GetInterfaceInstance(pzLanguage);
	if (!pII)
	{
		GString strError;
		strError << pzLanguage << " is not configured correctly or is not a supported component type on this machine.";
		InfoLog(303,strError);
		return 0;
	}


	// setup the driver options
	char pzPIBuf[1024];
	pzPIBuf[0] = 0;
	pII->SetOption("piBuf",pzPIBuf);
	pII->SetOption("callback",(void *)DriverCallBack);
	pII->SetOption("callbackArg",prd );
	pII->SetOption("wkBuf",pBuf);
	pII->SetOption("wkBuf2",0);
	int n = nBufSize;
	pII->SetOption("wkBufSize",(void *)n);


	//
	// prepare arguments for Invoke() call
	//
	GString strArgSizes;
	strArgSizes << nLength;

	try
	{
		int nOutResultSize = 0;
		const char *pzResults = pII->InvokeEx( pzComponent, pzInterface, pzMethod, pzData,strArgSizes,&nOutResultSize);
		if (pstrReply)
			pstrReply->write(pzResults,nOutResultSize);

		if (stristr(pzPIBuf,"DataSent") != 0)
		{
			// the plugin sent the data directly
			if (pstrReply)
			{
				pstrReply->Empty();
				(*pstrReply) << "DataSent";
			}
		}
		return 1;
	}
	catch( GException &rErr )
	{
		InfoLog(304,rErr.GetDescription());
	}
	return 0;
}



int ParseMessagingResponse(int fd, int bSenderPlaceFile, const char *pzOutFile, char *pContentData, int nContentLen, char *Buf, int nBufSize, int nBytes, GString &strCertifyData, GString *pstrDest, const char *pzPluginhandler, GString *pstrReply )
{
	GString strSender;
	GString strRecvr;
	GString strDataInfo;
	GString strTitle;


	int nSubHeaderLen = atoi(&pContentData[2]); // past the "OK" is the size

	// walk past the Length, point at the first non-numerical byte 
	// counting the numerical bytes
	int nNumericals=0;
	while (pContentData[nNumericals+2] >= '0' && pContentData[nNumericals+2] <= '9')
		nNumericals++;

	// data starts past "OK" NNN and subheader
	char *pDataStart = &pContentData[2+nNumericals+nSubHeaderLen];

	int AllHeadersLen = pDataStart - Buf;

	// read in the whole sub-header if we didn't get it in the previous recv
	if (AllHeadersLen > nBytes)
	{
		// get more data
		int nMore = nonblockrecv(fd, &Buf[nBytes],nBufSize - nBytes);
		if (nMore > 0)
		{
			nBytes += nMore;
		}
		else
		{
			// printf("POLL result- Read of more header failed %d\n",SOCK_ERR);
			nBytes = -1;
		}

	}


	int nTotalMessageSize = nContentLen - 2 - nNumericals - nSubHeaderLen;
	int nDataWithHeader = nBytes - AllHeadersLen;
	int nBytesToGet = nTotalMessageSize - nDataWithHeader;




	// "OK28recvr/sender/title/data info/certify/data"
	GString strArgs(&pContentData[2+nNumericals],nSubHeaderLen);

	GStringList lstArgs("/",strArgs);
	GStringIterator itArgs(&lstArgs);
	if (itArgs())
		strRecvr = itArgs++;
	if (itArgs())
		strSender = itArgs++;
	if (itArgs())
		strTitle = itArgs++;
	if (itArgs())
		strDataInfo = itArgs++;
	if (itArgs())
		strCertifyData = itArgs++;


	GString strOutFile; // the disk location.


	// if we let the trusted sender decide where to put the file
	if (bSenderPlaceFile)
	{
		strOutFile = strTitle;  // could be "\\CorpServer\department21\project777\myfile.doc"
	}
	else // or we control where the file goes
	{
		if (pzOutFile)
		{
			// pTCD->pzPollSectionName = "MsgFrom-Domain"
			strOutFile = pzOutFile; //GetProfile().GetPath(pTCD->pzPollSectionName,"DiskLocation",false);

			// make the destination directory if it does not exist
			#ifdef _WIN32
				mkdir(ReSlash(strOutFile));
			#else
				mkdir(ReSlash(strOutFile),777);
			#endif


			// to stop access behind the configured root like "../../autoexec.bat"
			// and to keep from specifying subfolders like "/subdir/file.dat"
			// and to keep the file name valid....
			char *pBuf = (char *)(const char *)strTitle;
			char *pBadChar = strpbrk(pBuf,"\r\n\\/:<>|");
			while (pBadChar)
			{
				*pBadChar = 'X'; // modify a const
				pBadChar = strpbrk(pBuf,"\r\n\\/:<>|");
			}
			// and it cannot start with a .
			if (pBuf[0] == '.')
				pBuf[0] = 'X';

			strOutFile += strTitle;
		}
	}


	//
	// Prepare the message destination
	//

	FILE *fp = 0;
	if (strOutFile.Length() && !(pzPluginhandler && pzPluginhandler[0]) )
	{
		fp = fopen(strOutFile,"wb"); // open local file
		if (!fp)
		{
			GString strLog;
			strLog.Format("failed to open file [%s] to receive message\n",(const char *)strOutFile);
			InfoLog(305,strLog);
			return 0; // error
		}
	}



	//int nFullLen = atoi(strDataInfo);
	char *pzBoundary = (char *)strpbrk((const char *)strDataInfo,"|");
	if (pzBoundary)
		pzBoundary++;


	if (pzPluginhandler && pzPluginhandler[0])
	{
		
		int nDataLen = nDataWithHeader;
		if (nDataWithHeader)
		{
			//int nToWrite = nDataWithHeader;
			if (pzBoundary)
			{
				char *pEnd = strstr(pDataStart,pzBoundary);
				if (pEnd)
				{
					nDataLen = (pEnd - pDataStart) - 4; // four bytes behind the boundary marker
					nBytesToGet = nDataLen;
				}
			}
		}
		else
		{
			int nChunk = (nBytesToGet > nBufSize) ? nBufSize : nBytesToGet;
			nBytes = nonblockrecv(fd, Buf, nChunk);
			if (nBytes > 0)
			{
				pDataStart = Buf;
				nDataLen = nBytes;
			}
			else
			{
				GString strLog;
				strLog.Format("error reading message %d:%d\n",(int)nBytes,(int)SOCK_ERR);
				InfoLog(306,strLog);
				return 0;
			}
		}

		// prepare the information available to the driver callback
		DriverCallBackArg prd;
		prd.de = 0; // cipher N/A on anonymous HTTP
		prd.en = 0;
		prd.fd = fd;
		prd.nProtocol = 3;
		prd.tid = 7;			// N/A in Messaging
		prd.strUser = strSender;
		prd.pnKeepAlive = 0;	// N/A in Messaging
		prd.nCompleteSize = nBytesToGet;
		prd.nObtainedCount = nDataLen;
		prd.sDataFlags = 0;		// N/A in Messaging
		prd.nBuf1Size = nBufSize;
		prd.strBoundary = pzBoundary;
		prd.strArgs = strArgs;

		try 
		{
			MessagingPlugin(&prd, pDataStart, nDataLen, Buf, nBufSize, pzPluginhandler, pstrReply);
		}
		catch( GException &rErr )
		{
			GString strMsg("MessagingPlugin Message:");
			strMsg << rErr.GetDescription();
			InfoLog(307,strMsg);
		}
		catch(...)
		{
			GString strMsg("Fatal error in MessagingPlugin.");
			InfoLog(308,strMsg);
		}


		return 1;	
	}


	// if data arrived with the header in the first read, write it now.
	if (nDataWithHeader)
	{
		
		int nToWrite = nDataWithHeader;
		if (pzBoundary)
		{
			char *pEnd = strstr(pDataStart,pzBoundary);
			if (pEnd)
			{
				nToWrite = (pEnd - pDataStart) - 4; // four bytes behind the boundary marker
			}
		}
		
		// put the data somewhere
		if (fp)
		{
			fwrite(pDataStart,1,nToWrite,fp);
		}
		else if (pstrDest)
		{
			pstrDest->write(pDataStart,nToWrite);
		}
	}

	while(nBytesToGet > 0)
	{
		int nChunk = (nBytesToGet > nBufSize) ? nBufSize : nBytesToGet;
		nBytes = nonblockrecv(fd, Buf, nChunk);

		if (nBytes > 0)
		{
			
			// shorten the last packet
			int nToWrite = nBytes;
			if (pzBoundary && nBytesToGet-nBytes < 256)
			{
				// walk back from the end and stop at a null or 256
				int nEndIndex = nBytes -1;
				while(Buf[nEndIndex] && nBytes - nEndIndex < 256)
					nEndIndex--;	

				char *pEnd = strstr(&Buf[nEndIndex+1],pzBoundary);
				if (pEnd)
				{
					nToWrite = (pEnd - Buf) - 4; // four bytes behind the boundary marker
				}
			}
			
			// put the data somewhere
			if (fp)
			{
				fwrite(Buf,1,nToWrite,fp);
			}
			else if (pstrDest)
			{
				pstrDest->write(Buf,nToWrite);
			}

			nBytesToGet -= nBytes;
		}
		else
		{
			GString strLog;
			strLog.Format("error reading message %d:%d\n",(int)nBytes,(int)SOCK_ERR);
			InfoLog(309,strLog);
			break;
		}
		
	}
	if (fp)
		fclose(fp);
	return 1;
}



void *MessageSend(void *argin)
{
	MessageSendArgs	*args = (MessageSendArgs *)argin;
	if (args->bExecuteAsync)
	{
		gthread_t Async_thr;
		args->bExecuteAsync = 0;
		gthread_create(&Async_thr,	NULL, MessageSend, argin );
		return 0;
	}
	
	args->nResult = 1; // in progress

	GString strIPHost;
	char pzHostName[512];
	gethostname(pzHostName,512); //uname() on some systems

	int nDNSResolutionRetries;
	nDNSResolutionRetries = 0;
DNSResolutionRETRY:
	struct hostent *pHELocal = (struct hostent *)gethostbyname((const char *)pzHostName);
	if (!pHELocal)
	{
		if (nDNSResolutionRetries++ < 5)
		{
			PortableSleep(0,250000 * nDNSResolutionRetries);
			goto DNSResolutionRETRY;
		}
		// Info:Failed to resolve[%s].
		if(g_TraceConnection)
		{
			args->strErrorDescription.Format("*******FAILED to resolve DNS for[%s]", pzHostName);
			InfoLog(310,args->strErrorDescription);
			args->nResult = 15;
			return 0; 
		}
	}





	struct sockaddr_in my_addr; 
	memcpy((char *)&(my_addr.sin_addr), pHELocal->h_addr,pHELocal->h_length); 
	memset(&(my_addr.sin_zero),0, 8);// zero the rest of the (unused) struct
	strIPHost = inet_ntoa(my_addr.sin_addr);

	GString strConnected;
	GString strData;

	int nRetries = 0;
LOOP_IT:
	args->nTotalDataSent = 0;
	int sockfd = DoConnect(args->strServer, args->nPort,5555,5555);
	if (sockfd == -1)
	{
		args->strErrorDescription.Format("Failed to connect to %s on port %d",args->strServer.Buf(), (int)args->nPort);
		args->nResult = 3;
		return 0;
	}


	// "/PublicPath/put/Domain"
	GString strCommand;
	strCommand.Format("%sput/%s",(const char *)args->strPublicPathName,(const char *)args->strConnectName);

	GString strAuth;
	if (args->strBasicHTTPAuth.Length())
	{
		BUFFER b;
		BufferInit(&b);
		uuencode((unsigned char *)(const char *)args->strBasicHTTPAuth, (int)args->strBasicHTTPAuth.Length(), &b); 
		GString strEncoded((char *)b.pBuf, b.cLen);
		BufferTerminate(&b);
		strAuth.Format("Authorization: Basic %s\r\n",(const char *)strEncoded);
	}



	GString strConnectionName;

	// generate a "Browser Issued" GET command requesting a connection.
	if ( args->strSwitchBoardServer.Length() )
	{
		// GET http://www.Domain.com/PublicPath/put/Domain%nnnnnnnnnn HTTP/1.1
		strConnectionName.Format(
		"GET http://%s%s%%%d HTTP/1.1\r\n"
		"Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, application/vnd.ms-excel, application/msword, */*\r\n"
		"Accept-Language: en-us\r\n"
		"Accept-Encoding: gzip, deflate\r\n"
		"User-Agent: Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.0)\r\n"
		"Host: %s\r\n" 
		"%s"
		"Connection: Keep-Alive\r\n\r\n",(const char *)args->strSwitchBoardServer,(const char *)strCommand, time(0),(const char *)strAuth );
	}
	else
	{
		strConnectionName.Format(
			"GET %s%%%d HTTP/1.1\r\n"
			"Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, application/vnd.ms-excel, application/msword, */*\r\n"
			"Accept-Language: en-us\r\n"
			"Accept-Encoding: gzip, deflate\r\n"
			"User-Agent: Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.0)\r\n"
			"Host: %s\r\n"
			"%s"
			"Connection: Keep-Alive\r\n\r\n",(const char *)strCommand,time(0),(const char *)strIPHost,(const char *)strAuth);
	}


	if ( nonblocksend(sockfd, (const char *)strConnectionName, (int)strConnectionName.Length()) == -1 )
	{ 
		args->strErrorDescription.Format("nonblocksend() failed on Poll Thread code:%d",(int)SOCK_ERR);
		args->nResult = 4;
		PortableClose(sockfd,"Core13");
		return 0;
	}

	
	char Buf[8192];
	char *pContentData = Buf;
	int nContentLen = 0;
	int nBytes = nonblockrecvHTTP(sockfd, Buf, 8192, &pContentData, &nContentLen,0,90,0);

	if (nBytes > 0 && !pContentData[0] && nContentLen) 
	{
		IOBlocking(sockfd, 0);
		int nMore = recv(sockfd, pContentData,8192 - nBytes,0);
		IOBlocking(sockfd, 1);
		if (nMore > 0)
		{
			nBytes += nMore;
		}
		else
		{
			GString str;
			str.Format("Failed to read in data following header[%d]\n",(int)SOCK_ERR);
			InfoLog(311,str);

			nBytes = -1;
		}
	}


	if (nBytes != -1)
	{
		if (pContentData[0] == 'O' && pContentData[1] == 'K')
		{
			char *pzSlash = strpbrk(pContentData,"/");
			if (pzSlash)
			{
				*pzSlash = 0;
				strConnected = pzSlash + 1;
				strData = &pContentData[2];
			}
		}
		else if (pContentData[0] == 'N' && pContentData[1] == 'O')
		{
			if (nRetries++ < 5)
				goto LOOP_IT; 

			args->strErrorDescription = "No pickup at SwitchBoardServer";
			args->nResult = 13;
			PortableClose(sockfd,"Core14");

			return 0;

		}
		else
		{
			// probably an HTTP page from a proxy-firewall indicating 
			// that the server could not be connected to, or a 404 from the
			// switchboard indicating a bad switch path.
			
			char *pEnd = strpbrk(pContentData,"\r\n");
			if (pEnd)
				*pEnd = 0;
			
			// HTTP 1.1 404 Not Found
			// HTTP 1.1 401 Unauthorized
			args->strErrorDescription = pContentData;

			args->nResult = 5;
			PortableClose(sockfd,"Core15");
			return 0;
		}
	}
	else
	{
		args->strErrorDescription.Format("nonblocksend() failed on Poll Thread code:%d",(int)SOCK_ERR);
		args->nResult = 6;
		PortableClose(sockfd,"Core16");
		return 0;
	}

// ************************************************************
// * To be sure that this is truly connectionless we can 
// * hang up and reconnect here without transaction failure.
// * this code will not cause the transaction to fail. - (leave it commented out anyhow).
// ************************************************************
//
//	PortableClose(sockfd);
//	sockfd = DoConnect(args->strServer, args->nPort,4444,4444);
///////////////////////////////////////////////////////////////

	
// ************************************************************
// * Now send the POST data
// ************************************************************
	int nPOSTRetries = 0;
POSTRetry:
	int nRet = POSTMessage(sockfd, args, strData);			// POSTS args->strFileToSend
	if (nRet == 0)
	{	// failed
		PortableClose(sockfd,"Core17");

		if (nPOSTRetries++ < 2)
		{
			sockfd = DoConnect(args->strServer, args->nPort,4444,4444);
			if (sockfd == -1)
			{
				args->strErrorDescription.Format("Failed to connect to %s on port %d",args->strServer.Buf(), (int)args->nPort);
				args->nResult = 7;
				return 0;
			}
			goto POSTRetry;
		}
	}
	else
	{

	// ************************************************************
	// ****************** now read the REPLY **********************
	// ************************************************************

		nBytes = nonblockrecvHTTP(sockfd, Buf, 8000, &pContentData, &nContentLen,0,90,0);
		if (nBytes > 0)
		{
			GString strCertifyData;
			GString strDest;
			ParseMessagingResponse(sockfd, 0, 0, pContentData, nContentLen, Buf, 8192, nBytes, strCertifyData, &args->strResponse, 0 , 0);

			args->nResult = 2;
			args->strErrorDescription = "Success";
		}
		else
		{
			args->strErrorDescription = "No delivery confirmation";
			args->nResult = 50;
		}
	}


	// todo: cache sockfd with a key of  args->strServer + args->nPort
	PortableClose(sockfd,"Core18");


	return 0;
}

int g_PollNow = 0;
void PollExecSleep(int nSeconds)
{
	int nSlept = 0;
	while(nSlept++ < nSeconds)
	{
		PortableSleep(1,0);
		if (g_PollNow)
		{
			g_PollNow = 0;
			return;
		}
	}
}


int g_PollThreadID = 4999; // start poll threadID at 5000
long g_PollThreadsConnected = 0; 
// issued by SwitchBoardPoll()
void *PollExecThread(void *arg)
{
	ThreadConnectionData *pTCD = (ThreadConnectionData *)arg;

	time_t tLastFailure = 0;
	time_t tCase2Timer = 0;
	int nFailureCounter = 0;
	int nFailureCounter2 = 0;
	int nFailureCase = 0;

	pTCD->nPollThreadID = ++g_PollThreadID;


POLL_POOL:

	// if this is the 2nd, 3rd, or 4th etc in a queue that has not made a connection to the 
	// switchboard  on the 1st or has had no connection pickup on the 1st that has made it to the switchboard
	if ( pTCD->pBaseTCD != pTCD && (time(0) - pTCD->pBaseTCD->lLastActivity > 10) ) 
	{
		PortableSleep(2,0);
		goto POLL_POOL;
	}


	int bFoundConnection = 0;
	char Buf[8192];

	int nPort = pTCD->pTSD->nPort; // pTCD->pTSD->nPort will be -777 to identify this connection as having been polled rather than accept()ed
	if (nPort < 1)
	{
		nPort = GetProfile().GetInt(pTCD->pzPollSectionName,"Port",false);
		if (nPort < 1)
			nPort = 80;
		int nHTTPProxyPort = GetProfile().GetIntOrDefault(pTCD->pzPollSectionName,"HTTPProxyPort",0);
		if (nHTTPProxyPort)
		{
			nPort = nHTTPProxyPort;
		}
	}


	int fd = DoConnect(pTCD->pTSD->szProxyToServer, nPort, pTCD->nPollThreadID ,pTCD->nPollThreadID);
	if (fd == -1)
	{
		pTCD->nLastPollConnectStatus = 700;
		// failed connect falls through to close this connection, same as 'goto CLOSE_POLL;'
	}
	else
	{
		pTCD->nLastPollConnectStatus = 1;

		// set the connection attributes
		IOBlocking(fd, 1);	// Set non blocking IO

		GString strConnectionName; // contains results of BuildConnectionPostData()
		
		// generate a GET command requesting a connection.
		BuildConnectionPostData(pTCD->nPollThreadID, strConnectionName, pTCD->pTSD->pzPostConnectPathAndName, pTCD->pzPollSectionName, pTCD->pTSD->szHTTPProxyFirewall, pTCD->strLocalNetworkAuthenticate,pTCD->pTSD->szProxyToServer );

		if ( nonblocksend(fd, (const char *)strConnectionName, (int)strConnectionName.Length()) == -1 )
		{ 
			pTCD->nLastPollConnectStatus = -1;
			InfoLog(312,"nonblocksend() failed on Poll Thread");
			goto CLOSE_POLL;
		}

		Buf[0] = 0;
		char *pContentData = Buf;
		int nContentLen = 0;

#ifdef _WIN32
		InterlockedIncrement(&g_PollThreadsConnected);
#else
		g_PollThreadsConnected++;
#endif
		// note: nBytes will equal -10035 when [pTCD->nPollSrvWait] times out
		int nBytes = nonblockrecvHTTP(fd, Buf, 8192, &pContentData, &nContentLen, 0, pTCD->nPollSrvWait, 1);
#ifdef _WIN32
		InterlockedDecrement(&g_PollThreadsConnected);
#else
		g_PollThreadsConnected--;
#endif
		if (nBytes == -1)
		{
			pTCD->nLastPollConnectStatus = 701;
		}
		if (nBytes > -1)
		{
			// check the HTTP status code
			GString strStatus;
			strStatus.SetFromUpTo(&Buf[9],"\r\n");  //Gets a string like "503 Service Unavailable" or "401 Unauthorized"

			
			// 204 is not an error, it's a negative response to the poll that cam from the switchboard server - nobody is there to pick it up
			if (strStatus.CompareNoCase("204 No Content") == 0)
			{
				pTCD->nLastPollConnectStatus = 204;
				goto CLOSE_POLL;
			}
			else if (strStatus.CompareNoCase("200 OK") != 0)
			{
				//  look for a message like this: (a reply from the cisco pix firewall indicating a request with a bad password)
				//  ------------------------------
				//	HTTP/1.1 401 Unauthorized
				//	WWW-Authenticate: Basic realm="UBT Authenticate  (ID777)"
				//	Connection: close
				// 
				//  or like this: (a reply from the switchboard indicating a misconfigured connection path)
				//  ------------------------------
				//  HTTP/1.1 404 Object Not Found
				//
				//  or like this: (a reply from a microsoft proxy without an internet connection)
				//  ------------------------------
				//  HTTP/1.1 503 The service is unavailable
				//
				GString strErr("Poll Connect to [");
				strErr << pTCD->pTSD->szProxyToServer << "] failed [" << strStatus << "]";
				InfoLog(313,strErr);
				
				pTCD->nLastPollConnectStatus = atoi(strStatus);
				goto CLOSE_POLL;
			}
		}

		if (nBytes > 0 && (pTCD->pTSD->nProtocol == 3)) // File Transfer or Remote Console
		{
			GString strReplyTo;
			char *p = strstr(Buf,"XferReplyTo: ");
			if (p)
			{
				pTCD->pBaseTCD->lLastActivity = time(0); // if we have multiple threads in this poll queue - make sure the others are ready to accept connections too

				strReplyTo.SetFromUpTo(p+13,"\r\n");

				// This writes into the TCD, each polling thread has it's own TCD
				pTCD->pzClientIP[0] = '~';
				strcpy(&pTCD->pzClientIP[1],pTCD->pTSD->szProxyToServer);
				strcpy(pTCD->pzIP,strReplyTo.Buf());
				pTCD->pnsockfd = &fd;

				AttachToClientThread(pTCD,g_PoolClientThread,g_ThreadPoolSize,&g_nThreadPoolRoundRobin);


				// complete this polling, dont close the connection, it is being serviced
				// by the clientThread that it was just attached to. 
				bFoundConnection = 1;
				goto FINISH_POLL;
			}
		}

		if (nBytes > -1 && !pContentData[0])
		{
			// if all we got was header - no content, wait for some content.
			if (!pContentData[0] && nContentLen) 
			{
				int nMore = nonblockrecvAny(fd, pContentData,8192 - nBytes);
				if (nMore > 0)
				{
					nBytes += nMore;
				}
				else
				{
					nBytes = -1;
				}
			}
		}

		if (nBytes > -1)
		{
			if (pContentData[0] == 'O' && pContentData[1] == 'K')
			{
				bFoundConnection = 1;



				if (pTCD->pTSD->nProtocol == 8) // [Messaging] poll
				{
					int bSenderPlaceFile = GetProfile().GetBool(pTCD->pzPollSectionName,"LetSenderPlaceFile",false);
					GString strOutFileFolder = GetProfile().GetPath(pTCD->pzPollSectionName,"DiskLocation",false);
					
					
					GString strHandler;
					if ( GetProfile().GetBool("Messaging","UsePluginHandler",false) )
					{
						strHandler = GetProfile().GetString("Messaging","PluginHandler",false);
					}

					MessageSendArgs args;


					// extract the message content, and finish reading it in if necessary
					GString strCertifyData;
					ParseMessagingResponse(fd, bSenderPlaceFile,strOutFileFolder, pContentData, nContentLen, Buf, 8192, nBytes , strCertifyData, 0, strHandler, &args.strMessageToSend);


					// send an auto-reply, if no handler created a custom reply
					if (args.strMessageToSend.Compare("DataSent") != 0)
					{
						if (args.strMessageToSend.IsEmpty())
							args.strMessageToSend.Format("Your transmission was received in full at[%s]",g_strThisHostName.Buf());

						if ( GetProfile().GetBool("Messaging","UseBrowserProxy",0 ) )
						{
							// pTCD->pzPollSectionName is the "MsgFrom-*" for the account we are dealing with
							args.strSwitchBoardServer = GetProfile().GetString(pTCD->pzPollSectionName,"SwitchBoardServer",false);
							args.strBasicHTTPAuth = pTCD->strLocalNetworkAuthenticate;
						}
						args.strPublicPathName = GetProfile().GetString(pTCD->pzPollSectionName,"Name",false);
						args.strPublicPathName.SetLength(args.strPublicPathName.ReverseFind('/')  + 1);
						args.strConnectName = strCertifyData;
						args.strUserName = "UBT";
						args.strRemoteTitle = "Confirmation";


						int nPOSTMessageRetries = 0;
						while( !POSTMessage(fd, &args, strCertifyData) )
						{
							if (nPOSTMessageRetries++ > 1)
								break;

							// if the POST fails, try in on a new connection - it may be that a proxy server did not keep the connection alive
							PortableClose(fd,"Core19");


							fd = DoConnect(pTCD->pTSD->szProxyToServer, nPort, pTCD->nParentThreadID ,pTCD->nParentThreadID);
							IOBlocking(fd, 1);
						}
					}


					// read the response to the post, and do nothing with it.
					nonblockrecvHTTP(fd, Buf, 8000, &pContentData, &nContentLen, 0, 300, 1);
				}
				else
				{
					GString strLog;
					strLog.Format("Protocol [%d] in PollExternalConnect must be 3 or 6",(int)pTCD->pTSD->nProtocol);
					InfoLog(314,strLog);
					goto CLOSE_POLL;
				}
				
				// complete this polling, dont close the connection, it is being serviced
				// by the clientThread that it was just attached to. 
				goto FINISH_POLL;
			}
			else
			{
				// there is no client waiting for the type of connection just requested.
				goto CLOSE_POLL;
			}
		}
	}
CLOSE_POLL:
	PortableClose(fd,"Core20");

FINISH_POLL:

	
	if (bFoundConnection)
	{
		pTCD->nLastPollConnectStatus = 2; // connection accepted

		// this resets the failure throttle
		nFailureCounter = 0;
		nFailureCounter2 = 0;
		nFailureCase = 0;
		tLastFailure = 0;
	}
	else
	{
		// if the value is 200-599 it's the HTTP error code - so keep the error detail for debugging
		// the 700 range is a local defined error
		// 0 indicates 'in progress' and -1 is 'unspecified error' - we may not be set at -1 yet it is now set because it is no longer 'in progress' 
		if (pTCD->nLastPollConnectStatus < 200)	
			pTCD->nLastPollConnectStatus = -1;

		// We need to be careful not to get stuck in a tight loop here.  The polled connection failed, for one of nearly countless reasons.
		// In many cases we need to immediately try again, and in others if we do we'll immediately and indefinitely get the exact same error putting this thread in a tight loop.


		// we also get here because of reaching configured timeouts.  It could be the timeout at the switchboard, then [pTCD->nLastPollConnectStatus] equals 204
		// It could also be a timeout due to local polling timeout configuration, in that case [nBytes] will equal -10035.
		// When we pass through this failure throttle due to hitting a configured timeout, we must never slow down - because that's how we are configured.


		// CASE 1: - if we have 2 failures in less than 2 seconds - slow down
		if (nFailureCase < 2) // nFailureCase 0 is the first time on error after a success, nFailureCase 1 we've already been doing this.
		{
			if (time(0) - tLastFailure < 2) // this value 2 is has dependency logic in CASE 2, if you adjust it adjust the CASE 2 calculation also
			{
				PollExecSleep(10);// this value 10 is has dependency logic in CASE 2, if you adjust it adjust the CASE 2 calculation also
				nFailureCase = 1;
			}
			else
			{
				tLastFailure = time(0);
			}
		}


		// CASE 2: - every 15 failures check to see if we are in a CASE 1 failure loop - if so slow down even more
		else if (nFailureCounter++ > 15 || nFailureCase == 2)
		{
			if (!tCase2Timer)
			{
				tCase2Timer = time(0); // this causes a longer period for the first loop, since the application was just started the other side may be starting too and we should remain quicker to retry
			}
			if ( time(0) - tCase2Timer < 120)
			{
				// 15 immediate back to back failures from a failed connection always takes exactly 98 seconds with the CASE 1 throttling.
				// 15 HTTP 404's will take a little longer depending on network latencies so we'll say anything less than 120 seconds constitutes a tight loop of CASE 1 conditions
				// if we hit 15 errors in under 2 minutes - this problem may not ever correct itself - but it certainly could still
				PollExecSleep(60);
				nFailureCase = 2;
				nFailureCounter2++;
			}
		}

		// CASE 3: - if we've been in a back to back CASE 2 failure for an hour at 1 minute intervals - slow down even more
		else if (nFailureCounter2 > 60 )
		{
			nFailureCase = 3;
			PollExecSleep(1800); // try every 30 minutes
		}
	}
	pTCD->bInUse = 0;


goto POLL_POOL;

	gthread_exit(0);
	return 0;
}


void *SystemTrace(void *arg)
{
	int nTraceOutInterval = 60; // every minute
	time_t LastTraceTime = time(0);
	while (!g_ServerIsShuttingDown)
	{
		PortableSleep(5,0);
		if (time(0) - LastTraceTime > nTraceOutInterval)
		{
			LastTraceTime = time(0);
			
			if (g_TraceSocketHandles)
			{
				// list all of the currently open socket handles
				
				GString strOpenHandles("Currently Open Socket Handles:");
				int n = ListOpenSockets(strOpenHandles);
				if (n)
				{
					InfoLog(315,strOpenHandles);
				}
			}
		}
	}
	return 0;
}




void *SwitchBoardPoll(void *arg)
{
	// first just get the nTotalPollEntryCount by iterating the config sections
	int nTotalPollEntryCount = 0;
	int nActivePollCount = 0;
	while(1)
	{
		GString strSection;
		strSection.Format("PollExternalConnect%d",(int)nTotalPollEntryCount+1); // == "PollExternalConnect1" .. "PollExternalConnectN"
		const char *pzCountTest = GetProfile().GetString(strSection,"Server",false);
		if ( pzCountTest && pzCountTest[0] )
		{
			nTotalPollEntryCount++; // including disabled entries, they will be skipped later
		}
		else
		{
			break;
		}
	}


	// see if [Messaging] is enabled
	if ( GetProfile().GetBoolean("Messaging","Enable",false) )
	{
		messagePollUpdateThread(0);
		nTotalPollEntryCount += g_msgPollSites.getNodeCount();
	}


	// loop through the (PollExternalConnectN) config sections 
	// copying all data into thread startup structures.
	ThreadConnectionData *TCD = new ThreadConnectionData[nTotalPollEntryCount];
	int i;
	for(i=0; i<nTotalPollEntryCount - (int)g_msgPollSites.getNodeCount();i++)
	{
		sprintf(TCD[i].pzPollSectionName, "PollExternalConnect%d",(int)i+1);

		const char *pzIP = GetProfile().GetString(TCD[i].pzPollSectionName,"Server",false); 
		GString strFullPath(GetProfile().GetString(TCD[i].pzPollSectionName,"Path",false));
		GString strPollName(GetProfile().GetString(TCD[i].pzPollSectionName,"Name",false));

		// if is not properly configured, or specifically disabled
		if (!pzIP || !pzIP[0] || strFullPath.IsEmpty() || strPollName.IsEmpty() ||	!GetProfile().GetBoolean(TCD[i].pzPollSectionName,"Enable",false) )
		{
			TCD[i].sockfd = -7; // used below to skip this 'disabled' entry
			continue;
		}
		strFullPath << strPollName;

		nActivePollCount++;

		TCD[i].pTSD = new ThreadStartupData;
		TCD[i].pTSD->nProxyTimeout = 300;
		
		// 1 HTTP, 2 Proxy/Tunnel, 3 Xfer, 4 TXML, 5 TXML&HTTP, 6 Remote-WS, 7 Reserved
		TCD[i].pTSD->nProtocol = GetProfile().GetInt(TCD[i].pzPollSectionName,"Protocol",false);;

		TCD[i].nPollInterval = GetProfile().GetIntOrDefault(TCD[i].pzPollSectionName,"PollIntervalSeconds",15);
		TCD[i].nPollQueueMax = GetProfile().GetIntOrDefault(TCD[i].pzPollSectionName,"PollQueue",3);
		TCD[i].nPollSrvWait = GetProfile().GetIntOrDefault(TCD[i].pzPollSectionName,"PollSrvWait",300);
		

		GString strHTTPProxyServer = GetProfile().GetStringOrDefault(TCD[i].pzPollSectionName,"HTTPProxyServer","");
		if (strHTTPProxyServer.Length())
		{
			TCD[i].strLocalNetworkAuthenticate = GetProfile().GetStringOrDefault(TCD[i].pzPollSectionName,"HTTPBasicAuthenticate","");
			strcpy(TCD[i].pTSD->szHTTPProxyFirewall,pzIP);
			strcpy(TCD[i].pTSD->szProxyToServer,strHTTPProxyServer.Buf());
		}
		else
		{
			strcpy(TCD[i].pTSD->szProxyToServer,pzIP);
		}
		strcpy(TCD[i].pTSD->pzPostConnectPathAndName,strFullPath.Buf()); // like: /MySwitchPath/MyVNCSwitchName.html


		// any non valid port number, since this connection really comes through port 80 on another machine
		// this is not used anywhere except in debug tracing, to distinguish 'listen' and 'poll'.
		// the thread that services the request is unaware of how the connection was initiated (poll-in vs listen-in).
		TCD[i].pTSD->nPort = -777;
	}

	

	// if [Messaging] is enabled
	if ( GetProfile().GetBoolean("Messaging","Enable",false) )
	{
		// 1 = Alphabetical, 0 = Reverse Alphabetical, 2 = Insertion Order
		GBTreeIterator it(&g_msgPollSites,2);

		for (int nLoop = nTotalPollEntryCount - g_msgPollSites.getNodeCount(); nLoop < nTotalPollEntryCount; nLoop++)
		{
			// the first MessagePollData matches to the nLoop index of the TCD array
			MessagePollData *pMPD = (MessagePollData *)it++;

			GetProfile().GetString("Messaging","Name",false);  


			strcpy(TCD[nLoop].pzPollSectionName,(const char *)pMPD->strSectionName);
			TCD[nLoop].pTSD = new ThreadStartupData;
			TCD[nLoop].pTSD->nProxyTimeout = 300;
			
			// 1 HTTP, 2 Proxy/Tunnel, 3 Xfer, 4 TXML, 5 TXML&HTTP, 6 Remote-WS, 7 Reserved
			TCD[nLoop].pTSD->nProtocol = 8;
			TCD[nLoop].pTSD->nPort = pMPD->nPort;

			strcpy(TCD[nLoop].pTSD->szProxyToServer,(const char *)pMPD->strServer);
			strcpy(TCD[nLoop].pTSD->szHTTPProxyFirewall,(const char *)pMPD->strProxyTo);
			
			const char *pSerialized = pMPD->lstPollFor.Serialize("&&");
			strcpy(TCD[nLoop].pTSD->pzPostConnectPathAndName,(const char *)pMPD->strSwitchBoardPath);
			strcat(TCD[nLoop].pTSD->pzPostConnectPathAndName,pSerialized); 

			TCD[nLoop].nPollInterval = GetProfile().GetInt(TCD[nLoop].pzPollSectionName,"PollIntervalSeconds",false);
			if (TCD[nLoop].nPollInterval < 1)
				TCD[nLoop].nPollInterval = 30;

			TCD[nLoop].strLocalNetworkAuthenticate = pMPD->strLocalNetworkAuthenticate;

			
			nActivePollCount++;
		}
	}


	if (nActivePollCount == 0)
	{
		// end this thread, there is nothing to poll
		return 0;
	}


	// loop through thread startup structures, TCD[i], there is one for each [PollExternalConnectN] section.
	// Within each TCD in the array, there is a list of TCD's corresponding to each connection in the Queue
	// for this [PollExternalConnectN] section by walking TCD[i].pNext->pNext->pNext

	for(i=0; i<nTotalPollEntryCount;i++) 
	{

		if (TCD[i].sockfd == -7) // if this is not enabled, skip this one
			continue;

		for(int j=0;j < TCD[i].nPollQueueMax; j++)
		{
			// find a ThreadConnectData struct to use
			ThreadConnectionData *pUse = 0;
			if (TCD[i].bHasStartedThread)
			{
				ThreadConnectionData **pNext = &TCD[i].pNext; // check the next in the list
				while(*pNext)
				{
					pNext = &((*pNext)->pNext);
				}
				*pNext = new ThreadConnectionData; // we'll never have more than TCD[i].nPollQueueMax of these
				pUse = *pNext;
				pUse->Copy(&TCD[i]);
			}
			else
			{
				pUse = &TCD[i];
			}

			pUse->pBaseTCD = &TCD[i];

			gthread_t poll_thr;
			gthread_create(&poll_thr,	NULL, PollExecThread, (void *)pUse );
			pUse->bHasStartedThread = 1;
		}
	}
		
	return 0;
}


void *listenThread(void *arg)
{
	ThreadConnectionData TCD;
	TCD.pTSD = (ThreadStartupData *)arg;
	
//	if (g_TraceThread)
//	{
//		GString strThreadTrace;
//		strThreadTrace << "++++Created new listenThread  port:" << TCD.pTSD->nPort;
//		InfoLog(111,strThreadTrace);
//	}


	GString strConnectedIPAddress;
	g_listenThreadsRunning++;
	g_strListeningOnPorts << TCD.pTSD->nPort << "\r\n";

	int clilen;
	struct sockaddr_in cli_addr;
	int nCount = 0;
	long lLastPing = 0;
	
	listen(TCD.pTSD->sockfd, g_nListenQueue );


	// Set non-blocking IO
	IOBlocking(TCD.pTSD->sockfd, 1);


	for( ;; )
	{
		TCD.pTSD->nState = 2; // waiting
		nCount++;
		clilen = sizeof(cli_addr);

		int RetryForLinux = 0;
RETRY_FOR_LINUX:		
		int rslt = readableTimeout(TCD.pTSD->sockfd, 3/*seconds*/, 0/*microseconds*/);
		if ( rslt == -1)
		{
			if (RetryForLinux++ < 10)
			{
				gsched_yield();gsched_yield();gsched_yield();gsched_yield();gsched_yield();gsched_yield();
				goto RETRY_FOR_LINUX;
			}
			goto LISTEN_THREAD_EXIT; 
		}
		if (g_ServerIsShuttingDown || TCD.pTSD->bThreadShuttingDown)
		{
			goto LISTEN_THREAD_EXIT; 
		}
		ThreadData *pTD = 0;
		
		// if there is a connection, pick it up.
		// Otherwise check to see if the server is shutting down, of if this thread is ending.
		if ( rslt > 0 )
		{
			TCD.sockfd = accept(TCD.pTSD->sockfd, (struct sockaddr *) &cli_addr, (socklen_t *)&clilen);
			if(TCD.sockfd < 0)
			{
				// a 10038 is possible here "Connection reset by Peer"
				continue;
			}
			TCD.pTSD->nState = 3; // servicing
			TCD.pTSD->nCount++;

			AddDebugSocket(TCD.sockfd);

			if (g_TraceSocketHandles)
			{
				GString strTrace("Accepted Socket:");
				strTrace << TCD.sockfd  << "   on port:" << TCD.pTSD->nPort;
				InfoLog(316,strTrace);
			}

			// get the address of the connected client
			struct sockaddr_in other_addr;
			int nSize = sizeof(sockaddr_in);
			int nPeerRet = getpeername(TCD.sockfd,(struct sockaddr *) &other_addr, (socklen_t *)&nSize);
			if ( nPeerRet != 0 )
			{
				PortableClose(TCD.sockfd,"Core21");
				continue; 
			}

			// get his string n.n.n.n IP address
			strConnectedIPAddress = inet_ntoa(other_addr.sin_addr);
			strcpy(TCD.pzClientIP,(const char *)strConnectedIPAddress);


			//if there is a list of IP's that this system will refuse to service
			if ( g_FirstRefusedIP ) 
			{
				// if this ip IS in the list, drop him.
				if (QueryServicePermission( other_addr.sin_addr.s_addr , 0 ))
				{
					TCD.pTSD->nRefused++;
					PortableClose(TCD.sockfd,"Core22");
					GString strMessage;
					strMessage << "Refused ip [" << strConnectedIPAddress << "] found in global reject list";
					InfoLog(317, strMessage);
					continue;
				}
			}

			// if this system is configured to only allow connections from a specified IP range
			if ( g_FirstAllowedIP )
			{
				// if this ip IS NOT in the list, drop him.
				if (!QueryServicePermission( other_addr.sin_addr.s_addr , 1 ))
				{
					TCD.pTSD->nRefused++;
					PortableClose(TCD.sockfd,"Core23");
					GString strMessage;
					strMessage << "Refused ip [" << strConnectedIPAddress << "] NOT found in global permission list";
					InfoLog(318, strMessage);
					continue;
				}
			}


			// if he's connecting on a blocked port, add him to the list of IP's that this system will refuse to service then disconnect him
			if (TCD.pTSD->nProtocol == 10)
			{
				TCD.pTSD->nRefused++;
				PortableClose(TCD.sockfd,"Core24");

				// likely a bogus IP, or the IP of a probing machine.
				// note: this blocks him now on all ports because he has no business on this one
				GString strErr("[System] BlockedPorts:");
				strErr << TCD.pTSD->nPort;
				AddRefuseService(strConnectedIPAddress,g_nBlockedPortPenalty,strErr);
				continue;
			}
			
			// here the Proxy Server is being dynamically disabled, we still have the lock on the port and can dynamically reenable at any time
			if (TCD.pTSD->nProtocol == 11 && !g_bHTTPProxyEnabled)
			{
				TCD.pTSD->nRefused++;
				PortableClose(TCD.sockfd,"Core24.5");
				continue;
			}



			// if the FrequencyTimeLimit option is enabled and 
			// this guy visits too often flag him as trouble.
			if ( g_nTimeLimit )
			{
				if ( AddTime( &g_LogRoot, strConnectedIPAddress ) )
				{
					TCD.pTSD->nTooFrequent++;
					// he's hitting the server too hard, drop this connection
					PortableClose(TCD.sockfd,"Core25");
					continue;
				}
			}


			// if you do "too much"(per config file) connection traffic
			// slow down this guys transaction rate by dropping the connection.
			if ( g_nMaxConnections && ConnectionCounter(strConnectedIPAddress,1) > g_nMaxConnections ) 
			{
				TCD.pTSD->nTooMany++;

				GString strLog;
				strLog.Format("*******Machine at[%s] attempted more than %d connections, Current value:%d",(const char *)strConnectedIPAddress,(int)g_nMaxConnections, (int)ConnectionCounter(strConnectedIPAddress,1) );
				InfoLog(319,strLog);

				// we could send a "HTTP/1.1 5xx \n\n" like IIS or just quit 
				// wasting cycles on this guy by ending the connection right now.
				PortableClose(TCD.sockfd,"Core26");

				// decrement this connection
				ConnectionCounter(strConnectedIPAddress,-1);
				continue;
			}



			// *********************************************************************
			//                       service the connection
			// *********************************************************************
			pTD = AttachToClientThread(&TCD,g_PoolClientThread,g_ThreadPoolSize,&g_nThreadPoolRoundRobin);




			if (TCD.pTSD->nInstanceLimit > 0) 
			{
				gthread_cond_wait(&pTD->condFinished, &pTD->lockFinished);
			}
		}
		if (g_PingListenThread)
		{
			if (time(0) > lLastPing + 3 )
			{
				GString strLog;
				strLog.Format("%d",(int)TCD.pTSD->nPort);
				InfoLog(320,strLog);
				lLastPing = time(0);
			}
		}

		if (g_ServerIsShuttingDown || TCD.pTSD->bThreadShuttingDown)
			break;

	}
LISTEN_THREAD_EXIT:
	g_listenThreadsRunning--;


	GString strRemove;
	strRemove << TCD.pTSD->nPort << "\r\n";
	int nIndex = (int)g_strListeningOnPorts.Find(strRemove);
	g_strListeningOnPorts.Remove(nIndex,strRemove.Length());



	GString strLog;
	strLog.Format("Stopped port %d",(int)TCD.pTSD->nPort);
	InfoLog(321,strLog);

	PortableClose(TCD.pTSD->sockfd,"Core28");


	TCD.pTSD->sockfd = -1;
	TCD.pTSD->nState = 0; // stopped

	gthread_exit(0);
	return 0;
}



MessageSendArgs arg;


int server_started_port(int nPort)
{
	// loop through defined ports
	ThreadStartupData *pTSD = &g_TSD;
	while(pTSD)
	{
		if(nPort == pTSD->nPort)
		{
			if ( pTSD->nState > 0 )
			{
				return 1; // Ready, Waiting or Servicing
			}
			else
			{
				return 0; // port stopped(pTSD->nState == 0) or never started (pTSD->nState == -1)
			}
		}
		pTSD = pTSD->Next();
	}
	return -1; // port not defined
}




// WARNING:Never set a monitored value inside the GProfile from inside this callback or you may be in infinite recursion.
// This will be called automatically for GProfile entries registered with RegisterChangeNotification()
int GProfileMonitor(const char *pzSection, const char *pzEntry, const char *pzNewValue)
{
	GString strEntry(pzEntry);
	GString strSection(pzSection);

	// note string types may be directly assigned.  Boolean must go through GProfile translation to convert (yes/on/1/off/no/0)


	// [Trace]
	if (strSection.CompareNoCase("Trace") == 0)
	{

		if (strEntry.CompareNoCase("HTTPProxyTrace") == 0)
		{
			g_bTraceHTTPProxy = GetProfile().GetBoolean("Trace","HTTPProxyTrace",false);
		}
		else if (strEntry.CompareNoCase("HTTPProxyReturnTrace") == 0)
		{
			g_bTraceHTTPProxyReturn = GetProfile().GetBoolean("Trace","HTTPProxyReturnTrace",false);
		}
		else if (strEntry.CompareNoCase("HTTPProxyTraceBinary") == 0)
		{
			g_bTraceHTTPProxyBinary = GetProfile().GetBoolean("Trace","HTTPProxyTraceBinary",false);
		}
		else if (strEntry.CompareNoCase("HTTPProxyTraceExtra") == 0)
		{
			g_bHTTPProxyLogExtra = GetProfile().GetBoolean("Trace","HTTPProxyTraceExtra",false);
		}
		else if (strEntry.CompareNoCase("ConnectTrace") == 0)
		{
			g_TraceConnection = GetProfile().GetBoolean("Trace","ConnectTrace",false);
#ifdef ___XFER
			g_TraceXferConnection = g_TraceConnection;
#endif
		}
		else if (strEntry.CompareNoCase("ThreadTrace") == 0)
		{
			g_TraceThread = GetProfile().GetBoolean("Trace","ThreadTrace",false);
		}
		else if (strEntry.CompareNoCase("HTTPHeaderTrace") == 0)
		{
			g_HTTPHeaderTrace = GetProfile().GetBoolean("Trace","HTTPHeaderTrace",false);
		}
		else if (strEntry.CompareNoCase("HTTPFilesTrace") == 0)
		{
			g_TraceHTTPFiles = GetProfile().GetBoolean("Trace","HTTPFilesTrace",false);
		}
		else if (strEntry.CompareNoCase("SocketHandles") == 0)
		{
			g_TraceSocketHandles = GetProfile().GetBoolOrDefault("Trace","SocketHandles",0);
		}
		else if (strEntry.CompareNoCase("DataTraceFile") == 0)
		{
			if (strlen(pzNewValue))
				g_DataTrace = 1;
			else
				g_DataTrace = 0;
		}
		else if (strEntry.CompareNoCase("PreLog") == 0)
		{
			g_strPreLog = GetProfile().GetStringOrDefault(	"Trace","PreLog","" );
		}
		else if (strEntry.CompareNoCase("Enable") == 0)
		{
			// note the ! to flip the variable name logic
			g_NoLog = !(GetProfile().GetBoolOrDefault(	"Trace","Enable", 1 ));
		}

#ifdef ___XFER
		else if (strEntry.CompareNoCase("XferTrace") == 0)
		{
			g_TraceXfer = GetProfile().GetBoolean("Trace","XferTrace",false);
		}
		else if (strEntry.CompareNoCase("XferTraceBinary") == 0)
		{
			g_TraceXferBin = GetProfile().GetBoolean("Trace","XferTraceBinary",false);
		}
#endif // ___XFER



	} 
	// end of [Trace] - beginning of [HTTP]
	else if (strSection.CompareNoCase("HTTP") == 0)
	{
		// [HTTP]
		if (strEntry.CompareNoCase("Home") == 0)
		{
			g_strHTTPHomeDirectory = pzNewValue; 
		}
		else if (strEntry.CompareNoCase("KeepAliveMax") == 0)
		{
			g_nMaxKeepAlives = GetProfile().GetIntOrDefault(pzSection,pzEntry,150);
		}
		else if (strEntry.CompareNoCase("HitLog") == 0)
		{
			g_strHTTPHitLog = pzNewValue; 
		}
		else if (strEntry.CompareNoCase("UseKeepAlives") == 0)
		{
			g_bUseHTTPKeepAlives = GetProfile().GetBoolOrDefault(pzSection,pzEntry,1);
		}
		else if (strEntry.CompareNoCase("KeepAliveTimeOut") == 0)
		{
			g_nKeepAliveTimeout = GetProfile().GetIntOrDefault(pzSection,pzEntry,300); 
		}
	}

	// end of [HTTP] - beginning of [HTTPProxy]
	else if (strSection.CompareNoCase("HTTPProxy") == 0)
	{
		// [HTTPProxy]
		if (strEntry.CompareNoCase("HTTPProxyAgent") == 0)
		{
			g_strHTTPProxyAgentName = pzNewValue; 
		}
		else if (strEntry.CompareNoCase("Enabled") == 0)
		{
			g_bHTTPProxyEnabled = GetProfile().GetBoolean(pzSection,pzEntry,false); // converts "yes" "ON" and "true"
		}
		else if (strEntry.CompareNoCase("RequireAuth") == 0)
		{
			g_bNeedHTTPProxyAuth = GetProfile().GetBoolean(pzSection,pzEntry,false); // converts "yes" "ON" and "true"
		}
		else if (strEntry.CompareNoCase("BlockGoogle") == 0)
		{
			g_bBlockGoogleTracking = GetProfile().GetBoolean(pzSection,pzEntry,false); // converts "yes" "ON" and "true"
		}
		else if (strEntry.CompareNoCase("BlockAds") == 0)
		{
			g_bBlockHTTPProxyAds = GetProfile().GetBoolean(pzSection,pzEntry,false); // converts "yes" "ON" and "true"
		}
		else if (strEntry.CompareNoCase("LogHTTPTxnFiles") == 0)
		{
			g_bTraceHTTPProxyFiles = GetProfile().GetBoolOrDefault(pzSection,pzEntry,0);
		}
		
		else if (strEntry.CompareNoCase("Only443or80") == 0)
		{
			g_bOnly443or80 = GetProfile().GetBoolOrDefault(pzSection,pzEntry,1);
		}
		else if (strEntry.CompareNoCase("NegotiateBasic") == 0)
		{
			g_bNegotiateBasic = GetProfile().GetBoolOrDefault(pzSection,pzEntry,1);
		}
		else if (strEntry.CompareNoCase("BlockHTTPProxyAds") == 0)
		{
			g_bBlockHTTPProxyAds = GetProfile().GetBoolOrDefault(pzSection,pzEntry,1);
		}
		else if (strEntry.CompareNoCase("NegotiateNTLMv2") == 0)
		{
			g_bNegotiateNTLMv2 = GetProfile().GetBoolOrDefault(pzSection,pzEntry,1);
		}
		else if (strEntry.CompareNoCase("EnableOutboundConnectCache") == 0)
		{
			g_bEnableOutboundConnectCache = GetProfile().GetBoolOrDefault(pzSection,pzEntry,0);
		}
		
		else if (strEntry.CompareNoCase("BlockedDomains") == 0)
		{
			g_strHTTPProxyBlocked = pzNewValue;
		}
		else if (strEntry.CompareNoCase("RestrictIPs") == 0)
		{
			g_bHTTPProxyRestrictToSubnet = GetProfile().GetBoolean(pzSection,pzEntry,false); // converts "yes" "ON" and "true"
		}
		else if (strEntry.CompareNoCase("RestrictBeginMatch") == 0)
		{
			if(g_pstrHTTPProxyRestrictList)
				delete g_pstrHTTPProxyRestrictList;
			g_pstrHTTPProxyRestrictList = new GStringList("," , pzNewValue);
		}
		else if (strEntry.CompareNoCase("Timeout") == 0)
		{
			g_nHTTPProxyTimeout = atoi(pzNewValue); 
		}
		else if (strEntry.CompareNoCase("LogEnabled") == 0)
		{
			ThreadStartupData *pTSD = &g_TSD;
			while(pTSD)
			{
				if (pTSD->nProtocol != 11)
				{
					pTSD = pTSD->Next();
					continue;
				}
				pTSD->bIsLogging =  GetProfile().GetBoolean(pzSection,pzEntry,false); // converts "yes" "ON" and "true"
				break;
			}
		}
		else if (strEntry.CompareNoCase("LogFileName") == 0)
		{
			ThreadStartupData *pTSD = &g_TSD;
			while(pTSD)
			{
				if (pTSD->nProtocol != 11)
				{
					pTSD = pTSD->Next();
					continue;
				}
				pTSD->strLogFile =  pzNewValue;
				break;
			}
		}

	}

	// end of [HTTPProxy] - beginning of [System]
	else if (strSection.CompareNoCase("System") == 0)
	{
		// [System]
		if (strEntry.CompareNoCase("BlockedPortPenalty") == 0)
		{
			g_nBlockedPortPenalty = GetProfile().GetIntOrDefault("System","BlockedPortPenalty",777);
		}
		else if (strEntry.CompareNoCase("DisableLog") == 0)
		{
			g_bDisableLog = GetProfile().GetBoolOrDefault("System","DisableLog",0);
		}
		else if (strEntry.CompareNoCase("LogFile") == 0)
		{
			g_strLogFile = pzNewValue; 
			g_NoLog = 0;
		}
		else if (strEntry.CompareNoCase("LogCache") == 0)
		{
			g_LogCache = GetProfile().GetBoolOrDefault("System","LogCache",1);
			g_NoLog = 0;
		}
		else if (strEntry.CompareNoCase("RefuseService") == 0)
		{
			BuildServicePermissionList(0);
		}
		else if (strEntry.CompareNoCase("MaxOpenConnectionsPerIP") == 0)
		{
			g_nMaxConnections = GetProfile().GetIntOrDefault("System","MaxOpenConnectionsPerIP",500);
		}
		else if (strEntry.CompareNoCase("FrequencyTimeLimit") == 0)
		{
			g_nTimeLimit = GetProfile().GetIntOrDefault("System","FrequencyTimeLimit",0);
		}


	}
#ifdef ___XFER
	// end of [System] - beginning of [Xfer]
	else if (strSection.CompareNoCase("Xfer") == 0)
	{
		// Section [Xfer]
		if (strEntry.CompareNoCase("ProxyAuthSystemName") == 0)
		{
			g_strProxyAuthSysName = pzNewValue; 
		}
		else if (strEntry.CompareNoCase("RequireTxnSequence") == 0)
		{
			g_nRequireTxnSeq = GetProfile().GetBoolean("Xfer","RequireTxnSequence",false);
		}
		else if (strEntry.CompareNoCase("RequireSysId") == 0)
		{
			g_bRequireSysID = GetProfile().GetBoolean("Xfer","RequireSysId",false);
		}
		else if (strEntry.CompareNoCase("RequireProxyAuth") == 0)
		{
			g_bRequireProxyAuth = GetProfile().GetBoolean("Xfer","RequireProxyAuth",false);
		}
		else if (strEntry.CompareNoCase("TxnIDPrecision") == 0)
		{
			g_nXferTxnIDPrecision = GetProfile().GetIntOrDefault("Xfer","TxnIDPrecision",3);
		}
		else if (strEntry.CompareNoCase("RequireDataFlags") == 0)
		{
			g_bRequireFlags = GetProfile().GetIntOrDefault("Xfer","RequireDataFlags",10);
		}
		else if (strEntry.CompareNoCase("EnableSwitchBoard") == 0)
		{
			g_bEnableXferSwitchBoard = GetProfile().GetBoolOrDefault("Xfer","EnableSwitchBoard",0);
		}
		else if (strEntry.CompareNoCase("SwitchBoardWaitForServer") == 0)
		{
			g_nServerQWaitTime = GetProfile().GetIntOrDefault("Xfer","SwitchBoardWaitForServer",240);;
		}
		else if (strEntry.CompareNoCase("SwitchBoardWaitForClient") == 0)
		{
			g_nClientQWaitTime = GetProfile().GetIntOrDefault("Xfer","SwitchBoardWaitForClient",120);;
		}
		else if (strEntry.CompareNoCase("SwitchBoardMaxQ") == 0)
		{
			g_nMaxQ = GetProfile().GetIntOrDefault("Xfer","SwitchBoardMaxQ",5);;
		}
	}
#endif // #ifdef ___XFER
#ifdef __CUSTOM_CORE__
	#include "ServerCoreCustomProfileUpdate.cpp"
#endif

	return 0;
}



void AssignSystemProfileValues()
{
	// set the global values	
	g_TraceConnection = GetProfile().GetBoolOrDefault(	"Trace","ConnectTrace",0);
	g_TraceSocketHandles = GetProfile().GetBoolOrDefault("Trace","SocketHandles",0);
	g_strPreLog = GetProfile().GetStringOrDefault(		"Trace","PreLog","" );
	g_TraceThread = GetProfile().GetBoolOrDefault(		"Trace","ThreadTrace",0);
	g_HTTPHeaderTrace = GetProfile().GetBoolOrDefault(	"Trace","HTTPHeaderTrace",0);
	g_TraceHTTPFiles = GetProfile().GetBoolOrDefault(	"Trace","HTTPFilesTrace",0);
	g_DataTrace =	GetProfile().ValueLength(			"Trace","DataTraceFile");
	g_bTraceHTTPProxy = GetProfile().GetBoolOrDefault(	"Trace","HTTPProxyTrace",0);
	g_bTraceHTTPProxyReturn = GetProfile().GetBoolOrDefault("Trace","HTTPProxyReturnTrace",0);
	g_bTraceHTTPProxyBinary = GetProfile().GetBoolOrDefault("Trace","HTTPProxyTraceBinary",0);
	g_bHTTPProxyLogExtra = GetProfile().GetBoolOrDefault("Trace","HTTPProxyTraceExtra",0);


	g_NoLog = !(GetProfile().GetBoolOrDefault(			"Trace","Enable", 1 ));

	g_strHTTPHomeDirectory = GetProfile().GetStringOrDefault(	"HTTP","Home","");
	g_nMaxKeepAlives = GetProfile().GetIntOrDefault(			"HTTP","KeepAliveMax",150);
	g_strHTTPHitLog = GetProfile().GetStringOrDefault(			"HTTP","HitLog","");
	g_bUseHTTPKeepAlives = GetProfile().GetBoolOrDefault(		"HTTP","UseKeepAlives",1);
	g_nKeepAliveTimeout = GetProfile().GetIntOrDefault(			"HTTP","KeepAliveTimeOut",300); 

	g_bTraceHTTPProxyFiles = GetProfile().GetBoolOrDefault(		"HTTPProxy","LogHTTPTxnFiles",0);
	g_bNeedHTTPProxyAuth = GetProfile().GetBoolOrDefault(		"HTTPProxy","RequireAuth",0); 
	g_bBlockGoogleTracking = GetProfile().GetBoolOrDefault(		"HTTPProxy","BlockGoogle",1);
	g_bBlockHTTPProxyAds = GetProfile().GetBoolOrDefault(		"HTTPProxy","BlockAds",1);    
	g_nHTTPProxyTimeout = GetProfile().GetIntOrDefault(			"HTTPProxy","Timeout",300); 
	g_strHTTPProxyAgentName = GetProfile().GetStringOrDefault(	"HTTPProxy","HTTPProxyAgent","5Loaves" );
	g_bHTTPProxyEnabled = GetProfile().GetBoolOrDefault(		"HTTPProxy","Enable",0); 
	g_strHTTPProxyBlocked = GetProfile().GetStringOrDefault(	"HTTPProxy","BlockedDomains","" );
	g_bHTTPProxyRestrictToSubnet = GetProfile().GetBoolOrDefault("HTTPProxy","RestrictIPs",1);


	g_bOnly443or80  = GetProfile().GetBoolOrDefault(			"HTTPProxy","Only443or80", 1);
	g_bNegotiateBasic = GetProfile().GetBoolOrDefault(			"HTTPProxy","NegotiateBasic", 0);
	g_bBlockHTTPProxyAds = GetProfile().GetBoolOrDefault(		"HTTPProxy","BlockHTTPProxyAds", 1);
	g_bNegotiateNTLMv2 = GetProfile().GetBoolOrDefault(			"HTTPProxy","NegotiateNTLMv2", 1);
	g_bEnableOutboundConnectCache = GetProfile().GetBoolOrDefault("HTTPProxy","EnableOutboundConnectCache", 0);

	
	GString strTempHTTPProxyRestrictMatch = GetProfile().GetStringOrDefault("HTTPProxy","RestrictBeginMatch","" );
	if(strTempHTTPProxyRestrictMatch.IsEmpty())
	{
		GString str(g_strThisIPAddress);
		int n2ndDot = str.FindNth('.',2,0);
		if (n2ndDot > 0)
		{
			str.SetLength(n2ndDot+1);
			strTempHTTPProxyRestrictMatch = str;

			// if LAN/WAN is on a 10. we will get that address in g_strThisIPAddress, 
			// we likely still need a 192.168.* for Virtual machines and devices for our OTHER other local subnet
			strTempHTTPProxyRestrictMatch << ";192.168";

		}
	}

	if(g_pstrHTTPProxyRestrictList)
		delete g_pstrHTTPProxyRestrictList;
	g_pstrHTTPProxyRestrictList = new GStringList(";" , strTempHTTPProxyRestrictMatch);



	g_nBlockedPortPenalty = GetProfile().GetIntOrDefault("System","BlockedPortPenalty",777);;
	g_nLockThreadBuffers = GetProfile().GetBoolOrDefault ("System","LockThreadBuffers",1 );
	g_bDisableLog = GetProfile().GetBoolOrDefault(		"System","DisableLog",0);
	g_strLogFile = GetProfile().GetStringOrDefault(		"System","LogFile","");
	g_LogCache = GetProfile().GetBoolOrDefault(			"System","LogCache",1);
	g_nMaxConnections = GetProfile().GetIntOrDefault(	"System","MaxOpenConnectionsPerIP",500);


#ifdef ___XFER
	g_TraceXfer = GetProfile().GetBoolOrDefault(		"Trace","XferTrace",0);
	g_TraceXferBin = GetProfile().GetBoolOrDefault(		"Trace","XferTraceBinary",0);
	g_TraceXferConnection = g_TraceConnection;
	g_bEnableXferSwitchBoard = GetProfile().GetBoolOrDefault("Xfer","EnableSwitchBoard",0);
	g_nServerQWaitTime = GetProfile().GetIntOrDefault("Xfer","SwitchBoardWaitForServer",240);
	g_nClientQWaitTime = GetProfile().GetIntOrDefault("Xfer","SwitchBoardWaitForClient",120);
	g_nMaxQ = GetProfile().GetIntOrDefault("Xfer","SwitchBoardMaxQ",5);
	g_strProxyAuthSysName = GetProfile().GetStringOrDefault("Xfer","ProxyAuthSystemName","sys");
	g_bRequireSysID = GetProfile().GetBoolOrDefault("Xfer","RequireSysId",0);
#endif // ___XFER
	
	g_strMessagingSwitchPath = GetProfile().GetPath("SwitchBoardServer","Name",false);
	g_bEnableMessagingSwitchboard = GetProfile().GetBoolean("SwitchBoardServer","Enable",false);

	g_LogStdOut = GetProfile().GetBoolean("Trace","LogStdOut",false);
	

	g_ThreadPoolSize = GetProfile().GetIntOrDefault("System","Pool", PRIMARY_POOL_SIZE );
	g_ProxyPoolSize = GetProfile().GetIntOrDefault("System","ProxyPool", PROXY_POOL_SIZE );

	g_nTimeLimit = GetProfile().GetInt("System","FrequencyTimeLimit",false);
}

int g_HasRegisteredMonitorValues = 0; 
void RegisterRealTimeProfileMonitorValues()
{
	if (g_HasRegisteredMonitorValues)
		return;

	g_HasRegisteredMonitorValues = 1;
	// monitor for realtime changes to the global values that may be set through the GUI admin 
	// tool for realtime kernel behavior changes and also set through the MC (MasterConfig) in the server console.
	// just add and entry here, and add a handler in GProfileMonitor()
	//
	// todo: there is lots to add to this list of global variables that may be changed in a running server.
	// everything that might possibly want to be changed should be added so that a server must never be stopped
	// and started for a behavior change.  Mostly add values to change when under attack or when performance tuning.
	// here's a few: find more and make more.
	//	int g_nPartialHeaderWaitSec
	//	int g_nProxyReadWait
	//	int g_nProxyIdleReadWait
	//  int g_nMaxHTTPHeaderSize;


	GetProfile().RegisterChangeNotification("Trace", "HTTPProxyTrace",			GProfileMonitor);
	GetProfile().RegisterChangeNotification("Trace", "HTTPProxyReturnTrace",	GProfileMonitor);
	GetProfile().RegisterChangeNotification("Trace", "HTTPProxyTraceBinary",	GProfileMonitor);
	GetProfile().RegisterChangeNotification("Trace", "HTTPProxyTraceExtra",		GProfileMonitor);
	GetProfile().RegisterChangeNotification("Trace", "ConnectTrace",			GProfileMonitor);
	GetProfile().RegisterChangeNotification("Trace", "ThreadTrace",				GProfileMonitor);
	GetProfile().RegisterChangeNotification("Trace", "HTTPHeaderTrace",			GProfileMonitor);
	GetProfile().RegisterChangeNotification("Trace", "HTTPFilesTrace",			GProfileMonitor);
	GetProfile().RegisterChangeNotification("Trace", "DataTraceFile",			GProfileMonitor);
	GetProfile().RegisterChangeNotification("Trace", "SocketHandles",			GProfileMonitor);
	GetProfile().RegisterChangeNotification("Trace", "XferTrace",				GProfileMonitor);
	GetProfile().RegisterChangeNotification("Trace", "XferTraceBinary",			GProfileMonitor);
	GetProfile().RegisterChangeNotification("Trace", "PreLog",					GProfileMonitor);
	GetProfile().RegisterChangeNotification("Trace", "Enable",					GProfileMonitor);

	GetProfile().RegisterChangeNotification("HTTPProxy", "Enable",				GProfileMonitor);
	GetProfile().RegisterChangeNotification("HTTPProxy", "HTTPProxyAgent",		GProfileMonitor);
	GetProfile().RegisterChangeNotification("HTTPProxy", "RequireAuth",			GProfileMonitor);
	GetProfile().RegisterChangeNotification("HTTPProxy", "BlockGoogle",			GProfileMonitor);
	GetProfile().RegisterChangeNotification("HTTPProxy", "BlockedDomains",		GProfileMonitor);
	GetProfile().RegisterChangeNotification("HTTPProxy", "Timeout",				GProfileMonitor);
	GetProfile().RegisterChangeNotification("HTTPProxy", "LogEnabled",			GProfileMonitor);
	GetProfile().RegisterChangeNotification("HTTPProxy", "LogFileName",			GProfileMonitor);
	GetProfile().RegisterChangeNotification("HTTPProxy", "RestrictIPs",			GProfileMonitor);
	GetProfile().RegisterChangeNotification("HTTPProxy", "RestrictBeginMatch",	GProfileMonitor);
	GetProfile().RegisterChangeNotification("HTTPProxy", "LogHTTPTxnFiles",		GProfileMonitor);
	GetProfile().RegisterChangeNotification("HTTPProxy", "Only443or80",					GProfileMonitor);
	GetProfile().RegisterChangeNotification("HTTPProxy", "NegotiateBasic",				GProfileMonitor);
	GetProfile().RegisterChangeNotification("HTTPProxy", "BlockHTTPProxyAds",			GProfileMonitor);
	GetProfile().RegisterChangeNotification("HTTPProxy", "NegotiateNTLMv2",				GProfileMonitor);
	GetProfile().RegisterChangeNotification("HTTPProxy", "EnableOutboundConnectCache",	GProfileMonitor);
	
	
	GetProfile().RegisterChangeNotification("HTTP",  "Home",					GProfileMonitor);
	GetProfile().RegisterChangeNotification("HTTP",  "KeepAliveMax",			GProfileMonitor);
	GetProfile().RegisterChangeNotification("HTTP",  "HitLog",					GProfileMonitor);
	GetProfile().RegisterChangeNotification("HTTP",  "UseKeepAlives",			GProfileMonitor);
	GetProfile().RegisterChangeNotification("HTTP",  "KeepAliveTimeOut",		GProfileMonitor);

	GetProfile().RegisterChangeNotification("System","BlockedPortPenalty",		GProfileMonitor); 
	GetProfile().RegisterChangeNotification("System","DisableLog",				GProfileMonitor); 
	GetProfile().RegisterChangeNotification("System","LogFile",					GProfileMonitor); 
	GetProfile().RegisterChangeNotification("System","LogCache",				GProfileMonitor); 
	GetProfile().RegisterChangeNotification("System","RefuseService",			GProfileMonitor); 
	GetProfile().RegisterChangeNotification("System","MaxOpenConnectionsPerIP",	GProfileMonitor); 
	GetProfile().RegisterChangeNotification("System","FrequencyTimeLimit",		GProfileMonitor); 

#ifdef ___XFER
	GetProfile().RegisterChangeNotification("Xfer","TxnIDPrecision",			GProfileMonitor); 
	GetProfile().RegisterChangeNotification("Xfer","RequireDataFlags",			GProfileMonitor); 
	GetProfile().RegisterChangeNotification("Xfer","RequireTxnSequence",		GProfileMonitor); 
	GetProfile().RegisterChangeNotification("Xfer","RequireSysId",				GProfileMonitor); 
	GetProfile().RegisterChangeNotification("Xfer","RequireProxyAuth",			GProfileMonitor); 
	GetProfile().RegisterChangeNotification("Xfer","SwitchBoardPath",			GProfileMonitor); 
	GetProfile().RegisterChangeNotification("Xfer","EnableSwitchBoard",			GProfileMonitor); 
	GetProfile().RegisterChangeNotification("Xfer","SwitchBoardWaitForServer",	GProfileMonitor); 
	GetProfile().RegisterChangeNotification("Xfer","SwitchBoardWaitForClient",	GProfileMonitor); 
	GetProfile().RegisterChangeNotification("Xfer","SwitchBoardMaxQ",			GProfileMonitor); 
#endif
#ifdef __CUSTOM_CORE__
	#include "ServerCoreCustomProfileSetNotify.cpp"
#endif
}




//////////////////////////////////////////////////////////////////////////////
int server_start(const char *pzStartupMessage = 0)
{
	g_ServerIsShuttingDown = 0;
	g_SwitchBoard.Startup();
	memset(HashOfActiveConnections,0,ACTIVE_CONN_HASH_SIZE ); // reset connection counts if this is a restart.
	g_TotalConnections = 0;
	g_ActiveConnections = 0;

	// turn off stdout buffering so we never need to flush();
	#ifndef WINCE
		setbuf (stdout, NULL);
	#endif
	#ifdef _WIN32
		_gthread_processInitialize();
	#endif

	gthread_t listen_thr;
	gthread_create(&listen_thr,	NULL, loggingThread, 0 );



	g_SwitchBoard.SetInfoLog( InfoLog );

#ifdef ___XFER
	g_XferSwitchBoard.SetInfoLog( InfoLog );
	SetXferSwitchBoardConnect( SBConnect );
	SetXferWaitForReply( SBWaitForReply );
	SetXferInfoLog( InfoLog );
	// this forces logging to be on during startup regardless of logfile configuration - it only logs to memory
	g_LogCache = 1;
	int g_NoLogPush = g_NoLog;
	g_NoLog = 0;
	int g_bDisableLogPush = g_bDisableLog;
	g_bDisableLog = 0;

#endif
#ifdef __CUSTOM_CORE__
	#include "ServerCoreCustomServerStart.cpp"
#endif



//	InfoLog(777,"Log File Notes: tid = 'Thread ID'    seq = HTTP Sequence or times the connection was 'Keep-Alive' fd='File Descriptor' which is a socket() handle.  fd's in the 200 range are a ProxyHelperThread() and fd's < 100 are a clientthread()");

	GString strStartup("Server starting at:");
	strStartup << g_strThisIPAddress;
	InfoLog(322,strStartup);

	char pzTime[128];
	struct tm *newtime;
	time_t ltime;
	time(&ltime);
	newtime = gmtime(&ltime);
	strftime(pzTime, 128, "Date: %a, %d %b %Y %H:%M:%S GMT\r\n", newtime);
	InfoLog(323,pzTime);



try
{

	BuildServicePermissionList(0); // "System","RefuseService"
	BuildServicePermissionList(1); // "System","GrantService"


	AssignSystemProfileValues();
	RegisterRealTimeProfileMonitorValues();

	// one time initialization of the socket library
#ifdef _WIN32
	WSADATA wsaData;
	WSAStartup(MAKEWORD( 2, 2 ),&wsaData );
#endif

	gthread_mutex_init(&g_csMtxConnCache, NULL);
	gthread_mutex_init(&PoolExpansionLock,NULL);
	gthread_mutex_init(&g_csMtxChallengeList, NULL);
	gthread_mutex_init(&g_csMtxConnectCache,NULL);
	memset(g_arrChallenges,0,sizeof(g_arrChallenges));


	g_LogStdOut = 1; // Startup info always goes to std out and the file

	if (pzStartupMessage)
	{
		InfoLog(324,pzStartupMessage);
	}

	if ( GetProfile().GetBoolean("System","UltraHighSecure",false) )
		InitRandomNumbers();
	else
		QuickInitRandomNumbers();

	// gather info about all listen threads that will be started
	ThreadStartupData *pTSD = &g_TSD;


	if (GetProfile().GetBool("HTTP","Enable",false))
	{
		int nIndex = 1;
		GString strPluginEntry;
		while (1)
		{
			strPluginEntry << "ServerPlugin" << nIndex++;
			const char *pzVal = GetProfile().GetString("HTTP",strPluginEntry,false);
			if (pzVal)
			{
				GStringList *plstArgs = new GStringList("|",pzVal);
				g_lstActivePlugins.AddLast(plstArgs);
			}
			else
				break;
			strPluginEntry = "";
		}
	}




	// Lock any 'blocked' ports upfront
	GString strPortBlock = GetProfile().GetString("System","BlockedPorts",false);
	if ( !strPortBlock.IsEmpty() )
	{
		GStringList ports(",",strPortBlock);
		GStringIterator itPorts(&ports);
		while (itPorts())
		{
			const char *pzPort = itPorts++;
			pTSD = pTSD->Next(1);
			pTSD->nPort = atoi(pzPort);
			pTSD->nProtocol = 10;

			strcpy(pTSD->szConfigSectionName,"System");

			GString strLog;
			strLog.Format("%d   ",(int)pTSD->nPort);
		}
	}



	//
	// The HTTP/TXML service can share a port or each be on it's own port.....
	//

	// HTTP Service
	if ( GetProfile().GetBoolean("HTTP","Enable",false) )
	{
		pTSD->nPort = GetProfile().GetInt("HTTP","Port",false);
		if (!pTSD->nPort)
			pTSD->nPort = 80;
		pTSD->nProtocol = 1;
		
		strcpy(pTSD->szConfigSectionName,"HTTP");
		
		GString str;
		str.Format("%d   ",(int)pTSD->nPort);
	}
	// TXML Service
	if ( GetProfile().GetBoolean("TXML","Enable",false) )
	{
		if ( GetProfile().GetBoolean("HTTP","Enable",false) )
		{
			if (pTSD->nPort == GetProfile().GetInt("TXML","Port",false))
			{
				// Running TXML and HTTP on the same port (modify the current pTSD)
				pTSD->nProtocol = 5; 
			}
			else
			{
				// Create a new pTSD for the TXML port
				pTSD = pTSD->Next(1);
				pTSD->nPort = GetProfile().GetInt("TXML","Port",false);
				pTSD->nProtocol = 4;

				GString strLog;
				strLog.Format("%d   ",(int)pTSD->nPort);

			}
			strcpy(pTSD->szConfigSectionName,"TXML");
		}
	}

	if ( GetProfile().GetBoolean("HTTPProxy","Enable",false) )
	{
		pTSD = pTSD->Next(1);
		pTSD->nPort = GetProfile().GetInt("HTTPProxy","Port",false);
		if (pTSD->nPort < 1)
			pTSD->nPort = 8080;
		pTSD->nProtocol = 11;
		strcpy(pTSD->szConfigSectionName,"HTTPProxy");
		pTSD->nInstanceLimit = 0; 
		pTSD->nProxyTimeout = GetProfile().GetIntOrDefault("HTTPProxy","Timeout",300);
		pTSD->strLogFile =  GetProfile().GetStringOrDefault("HTTPProxy","LogFile","");
		pTSD->bIsLogging =  GetProfile().GetBoolOrDefault("HTTPProxy","LogEnabled",0);
	}

// HOOK:protocol init
#ifdef __CUSTOM_CORE__
	#include "ServerCoreCustomProtocolInit.cpp"
#endif



	// Xfer - Port 10777
	int nXferPort = 0;
	if ( GetProfile().GetBoolean("Xfer","Enable",false) )
	{
		pTSD = pTSD->Next(1);
		pTSD->nPort = GetProfile().GetInt("Xfer","Port",false);
		if (pTSD->nPort < 1)
			pTSD->nPort = 10777;
		nXferPort = pTSD->nPort;
		pTSD->nProtocol = 3;
		strcpy(pTSD->szConfigSectionName,"Xfer");
	}


	// Remote Console Service - Port 10111
	if ( GetProfile().GetBoolean("RemoteConsole","Enable",false) )
	{
		int nRemoteConsolePort = GetProfile().GetInt("RemoteConsole","Port",false);
		
		// if Xfer is running, and RemoteConsole is on the same port then don't do anything, otherwise start Remote Console on the port configured
		if (nXferPort && nRemoteConsolePort != nXferPort)
		{
			pTSD = pTSD->Next(1);
			pTSD->nPort = GetProfile().GetInt("RemoteConsole","Port",false);
			if (pTSD->nPort < 1)
				pTSD->nPort = 10111;
			strcpy(pTSD->szConfigSectionName,"RemoteConsole");
			pTSD->nProtocol = 3;
		}
	}


	// loop through the [Tunnel1]...[TunnelN] and [Proxy1]..[ProxyN] sections in the config file
	int bAdvanceTSD = 1;
	int nEnabledCount = 0;
	for(int nTunnelOrProxy = 0; nTunnelOrProxy < 2; nTunnelOrProxy++)
	{
		const char *pSection = (nTunnelOrProxy) ? "Proxy" : "Tunnel";

		// Priming Section Read
		int nEntry = 1;
		GString strNextSection;
		strNextSection.Format("%s1",pSection); // == "Proxy1" or "Tunnel1"
		const char *pzLocalPort = GetProfile().GetString(strNextSection,"LocalPort",false);
			
		while(pzLocalPort && pzLocalPort[0])
		{
			if (GetProfile().GetBoolean(strNextSection,"Enable"))
			{
				nEnabledCount++;
				if (bAdvanceTSD)
					pTSD = pTSD->Next(1);
				try // missing required profile sections throw exceptions
				{
					pTSD->nProtocol = 2;
					pTSD->nPort = atoi(pzLocalPort);
					strcpy(pTSD->szProxyToServer, GetProfile().GetString(strNextSection,"RemoteMachine") );
					pTSD->nProxyToPort = GetProfile().GetInt(strNextSection,"RemotePort"); 
					pTSD->nInstanceLimit = GetProfile().GetInt(strNextSection,"InstanceLimit",false); 
					pTSD->nProxyTimeout = GetProfile().GetInt(strNextSection,"Timeout");
					pTSD->strLogFile =  GetProfile().GetStringOrDefault(strNextSection,"LogFile","");
					pTSD->bIsLogging =  GetProfile().GetBoolOrDefault(strNextSection,"LogEnabled",0);
					pTSD->bIsTunnel = (nTunnelOrProxy) ? 0 : 1;

					// so the thread can go get more info from this section later
					strcpy(pTSD->szConfigSectionName,(const char *)strNextSection);

					bAdvanceTSD = 1;
				}
				catch( GException &rErr )
				{
					// skip this one, it's not correctly setup in the config file.
					GString strLog;
					strLog.Format("Port[%s] not started.  Configuration error[%s]",pzLocalPort,rErr.GetDescription());
					InfoLog(325,strLog)
						;
					bAdvanceTSD = 0;
				}
			}

			// Next Section Read
			nEntry++;
			strNextSection.Format("%s%d",pSection,(int)nEntry); // "ProxyN" or "TunnelN"
			pzLocalPort = GetProfile().GetString(strNextSection,"LocalPort",false);
		}
	}
	if (nEnabledCount && !g_ProxyPoolSize)
	{
		g_ProxyPoolSize = 100;
	}

	// bind to a socket on each port that this server handles
	startListeners(1);



	// create the worker threads 
	CreateThreadPool(&g_PoolClientThread, g_ThreadPoolSize,clientThread);
	g_ThreadIDCount = 2000;
	CreateThreadPool(&g_PoolProxyHelperThread, g_ProxyPoolSize,ProxyHelperThread);


	// create a listener thread for each port we handle
	startListeners(2);

	InfoLog(326,"All bound ports are now being serviced");



	gthread_t listen_thr;
	gthread_create(&listen_thr,	NULL, SwitchBoardPoll, (void *)0 );

	gthread_create(&listen_thr,	NULL, SystemTrace, (void *)0 );

#ifdef __CUSTOM_CORE__
	#include "ServerCoreCustomServerStarted.cpp"
#endif
#ifdef ___XFER
	XferStartupNotify(&g_strCachedLog);
	g_NoLog = g_NoLogPush;
	g_bDisableLog = g_bDisableLogPush;
#endif


}

catch( GException &rErr )
{
	printf( rErr.GetDescription() );
	InfoLog(327, rErr.GetDescription());
}
catch( ... )
{
	printf("Fatal error starting server.\n");
	InfoLogFlush();
	return 0;
}

	InfoLogFlush();
	return 1;
}


void viewPorts(GString *pG = 0)
{
	// defined ports
	GString strPort("All Defined Ports:");
	ThreadStartupData *pTSD = &g_TSD;
	int n = 0;
	while(pTSD)
	{
		if (pTSD->nPort == -1)
		{
			pTSD = pTSD->Next();
			continue;
		}
		strPort << pTSD->nPort << "   ";
		n++;
		pTSD = pTSD->Next();
	}
	if (!n)
		strPort << "None.";

	
	// errored ports
	pTSD = &g_TSD;
	GString str("\n\nErrored Ports:");
	n = 0;
	while(pTSD)
	{
		if (pTSD->nPort == -1)
		{
			pTSD = pTSD->Next();
			continue;
		}
		if ( pTSD->nState < 0 )
		{
			str << pTSD->nPort << "   ";
			n++;
		}
		pTSD = pTSD->Next();
	}
	if (n)
	{
		strPort << str;
	}

	// stopped ports
	pTSD = &g_TSD;
	str = "\n\nStopped Ports:";
	n = 0;
	while(pTSD)
	{
		if (pTSD->nPort == -1)
		{
			pTSD = pTSD->Next();
			continue;
		}
		if ( pTSD->nState == 0 )
		{
			str << pTSD->nPort << "   ";
			n++;
		}
		pTSD = pTSD->Next();
	}
	if (n)
	{
		strPort << str;
	}


	// active ports
	pTSD = &g_TSD;
	str ="\n\nPorts [R]eady [W]aitng and [S]ervicing:";
	n = 0;
	while(pTSD)
	{
		if (pTSD->nPort == -1)
		{
			pTSD = pTSD->Next();
			continue;
		}

		if ( pTSD->nState > 0 )
		{
			str << pTSD->nPort << "[";
			if(pTSD->nState == 1)
				str << "R";
			else if(pTSD->nState == 2)
				str << "W";
			else if(pTSD->nState == 3)
				str << "S";
			else 
				str << "?";
			str << "]   ";
			n++;
		}
		pTSD = pTSD->Next();
	}
	if (n)
	{
		strPort << str;
	}


	// display it
	int nPrevLogSetting = g_LogStdOut;
	int nPrevLogSetting2 = g_NoLog;
	g_LogStdOut = 1; 
	g_NoLog = 0; 

	InfoLog(328,strPort);
	if (pG)
		*pG << strPort;

	g_LogStdOut = nPrevLogSetting; 
	g_NoLog = nPrevLogSetting2;
}


// maybe the server was started and only 4/5 of the desired listen ports
// were available.  The server starts with a warning.  Any time in the
// future when the desired port becomes available. Calling listenerReboot()
// will close all of the active listeners then re-attempt for 5/5.
// All active transactions will complete normally.  New connections will be
// briefly paused.
void listenerReboot()
{
	stopListeners();
	
	PortableSleep(3,0);

	startListeners(3);
}



// a diagnostic to briefly wake every thread and have it print it's threadID.
// fyi: each thread is assigned a numerical ID beginning at 1  (easier to look at than real thread handles)
void pingPools()
{
	int nPrevLogSetting = g_LogStdOut;
	int nPrevLogSetting2 = g_NoLog;
	g_LogStdOut = 1; 
	g_NoLog = 0;

	InfoLog(329,"Main Pool:");
	PingPool(g_PoolClientThread,g_ThreadPoolSize);
	InfoLog(330,"Proxy Pool:");
	PingPool(g_PoolProxyHelperThread,g_ProxyPoolSize);

	g_PingListenThread = 1;
	InfoLog(331,"Listening on ports:");
	PortableSleep(3,0);
	g_PingListenThread = 0;


	g_LogStdOut = nPrevLogSetting; 
	g_NoLog = nPrevLogSetting2;
}



// Note this is how the Xfer GUI gets the data to display current thread stats in the Console.
// It reports stats from all threads and all protocols.
void showActiveThreads(GString *pG/* = 0*/)
{
	
	int nPrevLogSetting = g_LogStdOut;
	int nPrevLogSetting2 = g_NoLog;
	g_LogStdOut = 1; 
	g_NoLog = 0; 
	if( g_ThreadPoolSize > 1 )
	{
		for(int i=0; i < g_ThreadPoolSize; i++)  
		{
			ThreadData *td = (ThreadData *)(g_PoolClientThread->m_arrObjects[i].m_pValue);

			if (td->nThreadIsBusy)
			{
				// tid,protocol,bytes,action,outPort,inPort,IP,time,"Action"\r\n
				GString strAction(td->strAction);
				strAction.Replace(',','~');
				GString Msg;
				Msg << td->nThreadID << "," << td->pTSD->nProtocol << "," << td->nTotalBytesProxied
					<< "," << td->nAction << "," << td->pTSD->nProxyToPort
					<< "," << td->pTSD->nPort << "," << td->pzClientIP
					<< "," << (getTimeMS() - td->starttime) / 1000 << "," << td->nSequence << "," << strAction << "," << td->pzConnectRoute;

				if (pG)
				{
					if (pG->Length())
						*pG << "\r\n";
					*pG << Msg;
				}
				else
				{
					InfoLog(332,Msg);
				}
			}
		}
	}
	g_LogStdOut = nPrevLogSetting; 
	g_NoLog = nPrevLogSetting2;
}

#include	<memory.h>
#include	<assert.h>

// This routine was designed to verify algorithms after a new operating system port.
void server_build_check()
{
	InfoLog(333,"Starting build check");

	// view data sizes
	GString str;
	str.Format("Size of--- Short:%d   Integer:%d    Long:%d   VeryLong: %d",sizeof(short),sizeof(int),sizeof(long),sizeof(__int64));
	InfoLog(334,str);

	// view more data sizes
	GString str2;
	str2.Format("Size of--- BYTE:%d   DWORD:%d    char:%d   VeryLong: %d",sizeof(BYTE),sizeof(DWORD),sizeof(char),sizeof(__int64));
	InfoLog(335,str2);
	

	// view byte order (1-0-0-0 is Intel,  0-0-0-1 is most others)

	// thoughts from the author......
	// byte order is like the slash - some people think backwards....
	// Some even divide about it(the slash) - with religious like convictions and loyalty to their slash of choice.
	// but who will divide by 0? (that is illegal - we ALL agree - no matter which way we slash)
	// There is a time an a place for everything, search for "breakin the law" in GException.cpp. 
	// Jesus said, "I have come not to break the law, but to fulfill it"
	
	int i = 1;
	char *pi = (char *)&i;
	str = "Byte Order ";
	for(int nByteOrderIndex = 0; nByteOrderIndex < sizeof(int); nByteOrderIndex++)
	{
		str += (int)pi[nByteOrderIndex];
		str += "-";
	}
	InfoLog(336,str);




	///////////////////////////////////////// Make Test Data ///////////////////////////////////////////////
	char pData1[256];
	char pWorkBuffer[1024];
	char pDataMatch[256];
	memset(pData1,0,256);
	memset(pDataMatch,0,256);
	for(i=0; i<32; i++)
	{
		pData1[i] = 65+i;		// A thru a followed by repeating nulls
		pDataMatch[i] = 65+i;
	}

	///////////////////////////////////////// SHA256 ////////////////////////////////////////////////////
	GString strMsgHash("Testing SHA256 Hash:");
	unsigned char pDigest[32];
	SHA256Hash((unsigned char *)pData1, 32, pDigest);

	for(i=0;i<32;i++)
	{
		GString strLog;
		strLog.Format("%d-",(int)(unsigned char)pDigest[i]);
		strMsgHash << strLog;
	}
	InfoLog(337,strMsgHash);


	///////////////////////////////// TwoFish Encrypt/Decrypt  ///////////////////////////////////////
	CipherData en("This is the pass-key",DIR_ENCRYPT);
	CipherData de("This is the pass-key",DIR_DECRYPT);
	InfoLog(338,"Init Cipher Keys");
	int nBitsCrypted;
	try
	{												// uncrypted in				// crypted out
		GString strMsgCipher("Testing Encrypt:");
		nBitsCrypted = en.blockEncrypt((unsigned char *)pData1,32*8,(unsigned char *)pWorkBuffer);
		for(i=0;i<32;i++)
		{
			GString strLog;
			strLog.Format("%d-",(int)(unsigned char)pWorkBuffer[i]);
			strMsgCipher << strLog;
		}
		InfoLog(339,strMsgCipher);
	}
	catch(...)
	{
		InfoLog(340,"blockEncrypt failed.");
		return;
	}
	if (nBitsCrypted != 32*8)
	{
		InfoLog(341,"blockEncrypt failed point 2.");
		return;
	}
	try
	{
		InfoLog(342,"Testing Decrypt");
		memset(pData1,0,256);							// crypted in					 // uncrypted out	
		nBitsCrypted = de.blockDecrypt((unsigned char *)pWorkBuffer,32*8,(unsigned char *)pData1);
		
		if (memcmp(pData1,pDataMatch,256) == 0)
		{
			InfoLog(343,"Decrypted OK.");
		}
		else
		{
			InfoLog(344,"Decrypted Failed!");
		}
	}
	catch(...) 
	{
		InfoLog(345,"Decrypt Failed, point 1");
		return;
	}
	if (nBitsCrypted != 32*8)
	{
		InfoLog(346,"Decrypt Failed, point 2");
		return;
	}

	
	/////////////////////////////// Zip/UnZip ////////////////////////////////////////////////////////
	InfoLog(347,"Testing Compress");
	int nZippedLength = 256;
	int cRet;
	if (1)
	{
		cRet = GZipBuffToBuffCompress(
										pWorkBuffer, // destination
									   (unsigned int*)&nZippedLength,	
									   pData1, // source
									   256					   );
	}
	else
	{
		cRet = BZ2_bzBuffToBuffCompress( pWorkBuffer, // destination
							   (unsigned int*)&nZippedLength,	
							   pData1, // source
							   256,						// zip the nulls + the data
							   9, //blockSize100k, 
							   0,// verbosity, 
							   30);//workFactor )
	}

	if (cRet != 0)
	{
		GString strLog;
		strLog.Format("Compress failed. code[%d]",(int)cRet);
		InfoLog(348,strLog);
		return;
	}
	else
	{
		GString strLog;
		strLog.Format("Compressed 256 bytes into %d bytes OK.",(int)nZippedLength);
		InfoLog(349,strLog);
	}


	InfoLog(350,"Testing Decompress");
	int nUnZippedLength = 256;
	memset(pDataMatch,0,256);
	if (1)
	{
				cRet = GZipBuffToBuffDecompress
										   ( pDataMatch, // destination
											 (unsigned int *)&nUnZippedLength, // decompressed length
											 pWorkBuffer, // source
											 nZippedLength); // source len
	} else
	{

			cRet = BZ2_bzBuffToBuffDecompress 
										   ( pDataMatch,		// destination
											 (unsigned int *)&nUnZippedLength,
											 pWorkBuffer,		// source
											 nZippedLength,
											 1,// or 0 small,
											 0);//verbosity )
	}

	// see if the decompressed data matches the source data
	if (memcmp(pDataMatch,pData1,256) == 0)
	{
		InfoLog(351,"Decompress OK.");
	}else
	{
		InfoLog(352,"Decompress Failed!");
		return;
	}



	/////////////////////////////////// UU En/DeCode /////////////////////////////////////////////
	// uuencode into a GString
	InfoLog(353,"Testing UUEncode");
	BUFFER b;
	BufferInit(&b);
	uuencode((unsigned char *)pData1, 256, &b);
	GString strEncoded((char *)b.pBuf, b.cLen);
	BufferTerminate(&b);
	
	GString strLog;
	strLog.Format("UUEncoded=%s",(const char *)strEncoded);
	InfoLog(354,strLog);


	InfoLog(355,"Testing UUDecode");
	BUFFER b2;
	BufferInit(&b2);
	unsigned int nDecoded;
	uudecode((char *)(const char *)strEncoded, &b2, &nDecoded, false);
	if (memcmp(b2.pBuf,pData1,256) == 0 )
	{
		InfoLog(356,"UUDecoded OK");
	}
	else
	{
		InfoLog(357,"UUDecode Failed!");
		return;
	}
	BufferTerminate(&b2);

	InfoLog(358,"Done Checking Build.");
}

void server_stop()
{
	// stop receiving new connections, end the listen threads
	InfoLogFlush();
	g_ServerIsShuttingDown = 1;
	g_SwitchBoard.ShutDown();

	// This gives everything running time to respond to a pending shutdown. the number may be much lower in many cases.
	PortableSleep(g_nServerShutdownWait,0);

	// drain the pools
	InfoLog(359,"Destroying thread pools");
	DestroyThreadPool(g_PoolProxyHelperThread,g_ProxyPoolSize);
	DestroyThreadPool(g_PoolClientThread,g_ThreadPoolSize);
	g_ProxyPoolSize = 0;
	g_ThreadPoolSize = 0;

#ifdef ___XFER
	DestroyXferCommandThreadPool();
	DestroyXferProxyThreadPool();
#endif 
#ifdef __CUSTOM_CORE__
	#include "ServerCoreCustomServerStop.cpp"
#endif

	gthread_mutex_destroy(&g_csMtxConnCache);
	gthread_mutex_destroy(&PoolExpansionLock);
	gthread_mutex_destroy(&g_csMtxChallengeList);
	gthread_mutex_destroy(&g_csMtxConnectCache);

#ifdef _WIN32
	Sleep(2000);
	_gthread_processTerminate();
#else
	PortableSleep(2,0);
#endif


	InfoLog(360,"ShutDown Complete");
	InfoLogFlush();

#ifdef _WIN32
	WSACleanup();
#endif
}


void DeHexUUCipher(GString *pStrIn, GString *pStrDest, const char *pzPass)
{
	pStrIn->Replace('@','+');	// faster, constant size

	//uudecode
	BUFFER b;
	BufferInit(&b);
	unsigned int nDecoded;
	uudecode((char *)pStrIn->StrVal(), &b, &nDecoded, false);

	char *pWkBuf = new char[(int)pStrIn->Length()+16];

	//decrypt
	CipherData de(pzPass,DIR_DECRYPT);
	int nBitsCrypted;
	nBitsCrypted = de.blockDecrypt((unsigned char *)b.pBuf,nDecoded *8,(unsigned char *)pWkBuf);
	BufferTerminate(&b);


	pStrDest->WriteOn(pWkBuf,nBitsCrypted/8);
//	pStrDest->TrimRight('�');      // this was the code prior to Nov 3, 2010 replaced with...
	pStrDest->TrimRight((char)168);// For Chinese character support the � was represented in its ascii base 10 numerical equivalent.

	delete pWkBuf;
}

// cipher it, then uuencode it, then hex escape URL reserved
void EnHexUUCipher(GString *pStrIn, GString *pStrDest, const char *pzPass)
{
	// cipher it
	unsigned char nPad = (16 - ((int)pStrIn->Length()) % 16);
	nPad = (nPad == 16) ? 0 : nPad;
	// pad it
	pStrIn->PadRight(pStrIn->Length() + nPad,(char)168); // 168 is seen as'�'
	char *pWkBuf = new char[(int)pStrIn->Length()+16];

	CipherData en(pzPass,DIR_ENCRYPT);
	int nBitsCrypted = en.blockEncrypt ((unsigned char *)pStrIn->StrVal(),((int)pStrIn->Length())*8,(unsigned char *)pWkBuf);

//	pStrIn->TrimRight('�',nPad); // lop off the pad so the input is 'const' - this was the code prior to Nov 3, 2010 replaced with...
	pStrIn->TrimRight((char)168,nPad);  // For Chinese character support the � was represented in its ascii base 10 numerical equivalent.

	// uuencode it
	BUFFER b;	
	BufferInit(&b);
	uuencode((unsigned char *)pWkBuf, nBitsCrypted/8, &b);
	*pStrDest = (const char *)b.pBuf;

	BufferTerminate(&b);

	// hex escape URL reserved
	pStrDest->Replace('+','@');		// faster, constant size

	delete pWkBuf;
}



