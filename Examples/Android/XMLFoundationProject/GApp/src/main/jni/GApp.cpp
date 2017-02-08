#include "XMLFoundation.cpp"

// The next includes are redundant (since we included the cpp's above)  if we had built the .so with ndk-build and linked to it
// as was typical in Eclipse projects and even in lesser integrated Android Studio projects - in those cases we would NEED the
// includes - but in this file they are redundant, the .h files may appear as many times as they are needed in your application.
#include "GString.h"
#include "GProfile.h"
#include "TwoFish.h"
#include "CSmtp.h"



#ifndef _G_APP_SERVER_INCLUDED_
#define _G_APP_SERVER_INCLUDED_

int g_isRunning = 0;
//JavaVM* g_javaVM = NULL;
extern JavaVM* g_javaVM; // in JavaXMLFoundation.cpp
extern GString g_JNIFoundationClass;
extern void setFoundationJNIClass(const char *pzClass); // sets g_JNIFoundationClass
extern void JavaShellExec(GString &strCommand, GString &strResult); // in JavaFoundation.cpp


jclass g_activityClass = 0;
jobject g_activityObj = 0;

void JavaInfoLog(int nMsgID, GString &strSrc)
{
	if (strSrc.IsEmpty())
		return;
    JNIEnv *env = 0;
	if (g_javaVM) {
		g_javaVM->AttachCurrentThread(&env, NULL);
		if (env && g_activityClass) {
			// Get the instance of [public class GAppGlobal], then find the [void messageMe(String)]
			jclass java_class = env->FindClass("a777/root/GApp/GAppGlobal");
			if (java_class) {
				jmethodID messageMe = env->GetStaticMethodID(java_class, "messageMe", "(Ljava/lang/String;)V");
				if (messageMe) {
					// Construct a JNI String from the GString - then pass it to the java code in the 3rd arg to CallStaticObjectMethod()
					jstring jstr = env->NewStringUTF(strSrc.Buf());
//					env->CallStaticVoidMethod((jobject)g_activityObj, messageMe, jstr);
				}
			}
		}
	}
}
void JavaInfoLog(int nMsgID, const char *pzSrc)
{
	GString strSrc(pzSrc);
	JavaInfoLog(nMsgID,strSrc);
}



void JavaKill(GString &strPID, GString &strResult)
{
	JNIEnv *env;
	if (g_javaVM)
	{
		g_javaVM->AttachCurrentThread(&env, NULL);
		jclass java_class = env->FindClass("a777/root/GApp/GAppGlobal");
		if (java_class) {
			jmethodID shellexec = env->GetMethodID(java_class, "Kill", "(Ljava/lang/String;)Ljava/lang/String;");
			if (shellexec) {
				// Construct a String
				jstring jstr = env->NewStringUTF(strPID.Buf());
				jstring strJava = (jstring) env->CallObjectMethod(g_activityObj, shellexec, jstr);
				const char *strCpp = env->GetStringUTFChars(strJava, 0);
				strResult = strCpp;
				env->ReleaseStringUTFChars(strJava, strCpp);
			}
		}
	}
}



extern void SetServerCoreInfoLog( void (*pfn) (int, GString &) ); // in ServerCore.cpp


GProfile *g_SavedProfile = 0;
int testAudioCapture();

extern "C" JNIEXPORT jstring JNICALL Java_a777_root_GApp_GAppGlobal_serverInteract  (JNIEnv *env, jobject obj, jint nOperation, jstring jsArg1, jstring jsArg2)
{
	int nRet = 0;
	env->GetJavaVM(&g_javaVM);
	jclass cls = env->GetObjectClass(obj);
	g_activityClass = (jclass) env->NewGlobalRef(cls);
	g_activityObj = env->NewGlobalRef(obj);

	const char *str1 = env->GetStringUTFChars(jsArg1,0);
	const char *str2 = env->GetStringUTFChars(jsArg2,0);

	GString strResultForJava;
	if (nOperation == 1) // in your java app you will set 1 to dispatch here
	{
		GString strRunningProcessData;
		GetProcessList( &strRunningProcessData );
		GStringList lstProcess("\n",strRunningProcessData); // each row divided by "\n"
		GStringIterator itP(&lstProcess);
		while(itP()) {
			GProcessListRow *pRow = new GProcessListRow(itP++);
			if (!strResultForJava.IsEmpty()) {
				strResultForJava << "\n";
			}
			strResultForJava << pRow->strName << "=" << pRow->strPID;
		}
	}
	if (nOperation == 2) // do your own thing
	{
		// This would route port 80 to port 8888 (the Proxy server is on port 8080, HTTP on 8888)
		// Android iptables is here: https://github.com/android/platform_external_iptables
		// then we could run the HTTP server on port 80 like on a normal platform.  This just normalizes the abnormalities in Android.
		// Android has the VNC port (5900) locked up the same way.
		// iptables -A PREROUTING -t nat -i eth0 -p tcp --dport 80 -j REDIRECT --to-port 8888

		GString strCommand("netstat -nlp");// listening ports and their respective pids
		JavaShellExec(strCommand, strResultForJava); // executes any command that the ADB Debugger can.
	}
	if (nOperation == 3)
	{
		GString strCommand("netstat -nlp | grep :80");
		JavaShellExec(strCommand, strResultForJava); // executes any command that the ADB Debugger can.
	}
	if (nOperation == 4)
	{
		strResultForJava = "nOperation 4 was called with [";
		strResultForJava << str1 << "] and [" << str2 << "]";
	}
	if (nOperation == 5)
	{
		strResultForJava = "nOperation 5 was called with [";
		strResultForJava << str1 << "] and [" << str2 << "]";
	}



	env->ReleaseStringUTFChars(jsArg1, str1);
	env->ReleaseStringUTFChars(jsArg2, str2);
	return env->NewStringUTF(strResultForJava.Buf());
	
}

extern int server_start(const char *pzStartupMessage/* = 0*/); // in ServerCore.cpp
void viewPorts(GString *pG/* = 0*/);
void showActiveThreads(GString *pG/* = 0*/);
void pingPools();


#include "GProcess.h" //for InternalIPs()
extern "C" JNIEXPORT jstring JNICALL Java_a777_root_GApp_GAppGlobal_getThisIP (JNIEnv *env, jobject obj)
{
	env->GetJavaVM(&g_javaVM);
	jclass cls = env->GetObjectClass(obj);
	g_activityClass = (jclass) env->NewGlobalRef(cls);
	g_activityObj = env->NewGlobalRef(obj);

	GStringList lstBoundIPAddresses;
	InternalIPs(&lstBoundIPAddresses);
	GString strResult = lstBoundIPAddresses.Serialize("-");

	return env->NewStringUTF(strResult);
}
extern "C" JNIEXPORT jstring JNICALL Java_a777_root_GApp_GAppGlobal_getThisHost (JNIEnv *env, jobject)
{
	return env->NewStringUTF(g_strThisHostName);
}

extern "C" JNIEXPORT jstring JNICALL Java_a777_root_GApp_GAppGlobal_getWANIP (JNIEnv *env, jobject)
{
	GString strIP, strError;
	ExternalIP(&strIP, &strError);
    return env->NewStringUTF(strIP);
}
extern "C" JNIEXPORT jstring JNICALL Java_a777_root_GApp_GAppGlobal_loadFile (JNIEnv *env, jobject, jstring jstrFile)
{
	GString strG;
	const char *str1 = env->GetStringUTFChars(jstrFile, 0);
	try {
//		unlink(str1);  // if you change the defaults in GAppGlobal.java they will not be used while the config file exists - this will remove it
//      otherwise load the stored settings that override defaults.
		strG.FromFile(str1);
	}
	catch(...){}
	env->ReleaseStringUTFChars(jstrFile, str1);

	return env->NewStringUTF(strG._str);
}


extern "C" JNIEXPORT jstring JNICALL Java_a777_root_GApp_GAppGlobal_configLoad (JNIEnv *env, jobject, jstring strFile) {

	const char *pzName = env->GetStringUTFChars(strFile, 0);
	GString gstrFileName(pzName);
	env->ReleaseStringUTFChars(strFile, pzName);

	GString strINIData;
	strINIData.FromFile(gstrFileName, 0);
	SetProfile(new GProfile((const char *) strINIData, (int) strINIData.Length(), 0));

	return env->NewStringUTF(strINIData._str);
}

extern "C" JNIEXPORT jint JNICALL Java_a777_root_GApp_GAppGlobal_configStore (JNIEnv *env, jobject, jstring strFile) {

	const char *pzName = env->GetStringUTFChars(strFile, 0);
	GString gstrFileName( pzName );
	env->ReleaseStringUTFChars(strFile, pzName);

	GetProfile().WriteCurrentConfig(gstrFileName,0);
	return 1;
}

extern "C" JNIEXPORT jint JNICALL Java_a777_root_GApp_GAppGlobal_saveFile (JNIEnv *env, jobject, jstring strFile, jstring strFileData)
{
	const char *pzData = env->GetStringUTFChars(strFileData, 0);
	GString gstrFileData( pzData );
	env->ReleaseStringUTFChars(strFileData, pzData);

	const char *pzName = env->GetStringUTFChars(strFile, 0);
	GString gstrFileName( pzName );
	env->ReleaseStringUTFChars(strFile, pzName);

	try {
		gstrFileData.ToFile(gstrFileName , true);
	}
	catch(...)
	{return 0;}
	return 1;
}

extern "C" JNIEXPORT jstring JNICALL Java_a777_root_GApp_GAppGlobal_configGet  (JNIEnv *env, jobject obj, jstring jsSection, jstring jsEntry){

	const char *pzSection = env->GetStringUTFChars(jsSection,0);
	GString strSection( pzSection );
	env->ReleaseStringUTFChars(jsSection, pzSection);

	const char *pzEntry = env->GetStringUTFChars(jsEntry,0);
	GString strEntry( pzEntry );
	env->ReleaseStringUTFChars(jsEntry, pzEntry);

	const char *pzValue = GetProfile().GetStringOrDefault(strSection,strEntry,"");

	GString g;
	g << "[" << strSection << "]" << strEntry << "=" << pzValue;
	JavaInfoLog(777, g);


	return env->NewStringUTF(  pzValue  );

}

void logit2(const char *pzLog){
	GString g(pzLog);
	g.ToFileAppend(GetProfile().GetStringOrDefault("System","LogFile",""),0);
}

extern "C" JNIEXPORT jint JNICALL Java_a777_root_GApp_GAppGlobal_emailSend (JNIEnv *env, jobject, jstring jsServer, jstring jsServerPort, jstring jsProtocol,
																  jstring jsUser, jstring jsPass, jstring jsSenderAlias, jstring jsSenderEmail,
																  jstring jsEmailSubject, jstring jsEmailRecipient, jstring jsEmailMessage){

//	logit2("**--inside Java_a777_root_GApp_GAppGlobal_emailSend()\n");

//  Convert all 10 arguments to GStrings
	const char *pzServer = env->GetStringUTFChars(jsServer,0);
	GString strServer( pzServer );
	env->ReleaseStringUTFChars(jsServer, pzServer );

	const char *pzServerPort = env->GetStringUTFChars(jsServerPort,0);
	GString strServerPort( pzServerPort );
	env->ReleaseStringUTFChars(jsServerPort, pzServerPort );

	const char *pzProtocol = env->GetStringUTFChars(jsProtocol,0);
	GString strProtocol( pzProtocol );
	env->ReleaseStringUTFChars(jsProtocol, pzProtocol );

	const char *pzUser = env->GetStringUTFChars(jsUser,0);
	GString strUser( pzUser );
	env->ReleaseStringUTFChars(jsUser, pzUser );

	const char *pzPass = env->GetStringUTFChars(jsPass,0);
	GString strPass( pzPass );
	env->ReleaseStringUTFChars(jsPass, pzPass );

	const char *pzSenderAlias = env->GetStringUTFChars(jsSenderAlias,0);
	GString strSenderAlias( pzSenderAlias );
	env->ReleaseStringUTFChars(jsSenderAlias, pzSenderAlias );

	const char *pzSenderEmail = env->GetStringUTFChars(jsSenderEmail,0);
	GString strSenderEmail( pzSenderEmail );
	env->ReleaseStringUTFChars(jsSenderEmail, pzSenderEmail );

	const char *pzEmailSubject = env->GetStringUTFChars(jsEmailSubject,0);
	GString strEmailSubject( pzEmailSubject );
	env->ReleaseStringUTFChars(jsEmailSubject, pzEmailSubject );

	const char *pzEmailRecipient = env->GetStringUTFChars(jsEmailRecipient,0);
	GString strEmailRecipient( pzEmailRecipient );
	env->ReleaseStringUTFChars(jsEmailRecipient, pzEmailRecipient );

	const char *pzEmailMessage = env->GetStringUTFChars(jsEmailMessage,0);
	GString strEmailMessage( pzEmailMessage );
	env->ReleaseStringUTFChars(jsEmailMessage, pzEmailMessage );




	CSmtp mail;
	mail.SetSMTPServer(strServer,strServerPort.Int());
	if (strProtocol == "TLS")
		mail.SetSecurityType(USE_TLS);
	if (strProtocol == "SSL")
		mail.SetSecurityType(USE_SSL);

	mail.SetLogin(strUser);
	mail.SetPassword(strPass);

	mail.SetSenderName(strSenderAlias);

	// When I set the sender email to: DTrump@whitehouse.gov and Google SMTP servers replace it with the sending account email.
	// Google SMTP servers always overwrite this value,
	// but it should be known that SMTP allows any value here - so unless an email uses digital signatures or cryptography  you dont know who its really from.
	mail.SetSenderMail(strSenderEmail);
	mail.SetReplyTo(strSenderEmail); // this Reply to does not have to match Sender
	//mail.SetReplyTo("No-reply@nowhere.com");
	mail.SetSubject(strEmailSubject);


	GStringList lst(";",strEmailRecipient); // each recipient divided by ";"
	GStringIterator it(&lst);
	while(it()) {
		mail.AddRecipient(it++);
	}



	mail.SetXPriority(XPRIORITY_NORMAL);
	mail.SetXMailer("Professional (v7.77) Pro");


	GStringList lst2("\n",strEmailMessage); // each line divided by "\n"
	GStringIterator it2(&lst2);
	while(it2()) {
		mail.AddMsgLine(it2++);
	}


	//mail.AddAttachment("../test1.jpg");
	//mail.AddAttachment("c:\\test2.exe");
	//mail.AddAttachment("c:\\test3.txt");
	mail.Send();

	return 1; // note, this code will compile without this line, if that happens the app will build and at runtime crash after sending the email 
}

extern "C" JNIEXPORT jint JNICALL Java_a777_root_GApp_GAppGlobal_configSet (JNIEnv *env, jobject, jstring jsSection, jstring jsEntry, jstring jsValue){

	const char *pzSection = env->GetStringUTFChars(jsSection,0);
	GString strSection( pzSection );
	env->ReleaseStringUTFChars(jsSection, pzSection);

	const char *pzEntry = env->GetStringUTFChars(jsEntry,0);
	GString strEntry( pzEntry );
	env->ReleaseStringUTFChars(jsEntry, pzEntry);

	const char *pzValue = env->GetStringUTFChars(jsValue,0);
	GString strValue( pzValue );
	env->ReleaseStringUTFChars(jsValue, pzValue);

	GetProfile().SetConfig(strSection,strEntry,strValue);

	return 1;// it always works, if its not there it gets added
}

extern "C" JNIEXPORT jint JNICALL Java_a777_root_GApp_GAppGlobal_configSetINIData  (JNIEnv *env, jobject obj, jstring sINIData)
{

	const char *pzINI = env->GetStringUTFChars(sINIData,0);
	GString strINIData( pzINI );
	env->ReleaseStringUTFChars(sINIData, pzINI);

	SetProfile( new GProfile((const char *)strINIData, (int)strINIData.Length(), 0) );
	return 1;
}

extern "C" JNIEXPORT jint JNICALL Java_a777_root_GApp_GAppGlobal_lineCount  (JNIEnv *env, jobject obj, jstring s) {

	const char *pz = env->GetStringUTFChars(s,0);
	GString str( pz );
	env->ReleaseStringUTFChars(s, pz);
	return str.OccurrenceCount('\n');
}

extern "C" JNIEXPORT void JNICALL Java_a777_root_GApp_GAppGlobal_serverStop  (JNIEnv *env, jobject obj)
{
//	server_stop();  // this is the proper shutdown(if we were to recycle this process),
//  but this is faster and works and ends the process.
	g_isRunning = 0;
	exit(0);
}


extern "C" JNIEXPORT jint JNICALL Java_a777_root_GApp_GAppGlobal_serverStart  (JNIEnv *env, jobject obj, jstring s)
{
	int nRet = 0;
	env->GetJavaVM(&g_javaVM);
	jclass cls = env->GetObjectClass(obj);
	g_activityClass = (jclass) env->NewGlobalRef(cls);
	g_activityObj = env->NewGlobalRef(obj);

	if (!g_isRunning)
	{
		g_isRunning = 1;

		setFoundationJNIClass("a777/root/GApp/GAppGlobal");
		SetServerCoreInfoLog( JavaInfoLog );

		const char *str = env->GetStringUTFChars(s,0);
		SetProfile(new GProfile((const char *)str, (int)strlen(str), 0));

		nRet = server_start("-- Android Server --");
		env->ReleaseStringUTFChars(s, str);

	}
	else
	{
		GString g("Server is already running");
		JavaInfoLog(777, g);
	}
	return nRet;
}


#endif // _G_APP_SERVER_INCLUDED_
