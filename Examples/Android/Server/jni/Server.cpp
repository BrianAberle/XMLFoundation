#include <string.h>
#include <stdlib.h>			// for atol(), strtol(), srand(), rand(), atoi()
#include <stdio.h>

#include <jni.h>
#include "GString.h"
#include "GProfile.h"
#include "TwoFish.h"


#include <pthread.h>		// POSIX Threads Library
#include <sched.h>			// POSIX Threads Library

int g_isRunning = 0;
JavaVM* g_javaVM = NULL;
jclass g_activityClass;
jobject g_activityObj;

void JavaInfoLog(int nMsgID, GString &strSrc)
{
    JNIEnv *env;
	if (g_javaVM)
	{
		g_javaVM->AttachCurrentThread(&env, NULL);

		jmethodID messageMe = env->GetMethodID(g_activityClass, "messageMe", "(Ljava/lang/String;)V");
		if (messageMe)
		{
			// Construct a String
			jstring jstr = env->NewStringUTF( strSrc.Buf() );
			env->CallVoidMethod(g_activityObj, messageMe, jstr);
		}
	}
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


extern void SetServerCoreInfoLog( void (*pfn) (int, GString &) ); // in ServerCore.cpp


GProfile *g_SavedProfile = 0;
extern "C" JNIEXPORT jstring JNICALL Java_com_Server_app_Server_serverInteract  (JNIEnv *env, jobject obj, jint nOperation, jstring jsArg1, jstring jsArg2)
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
		GStringList list("&&",str1);  // example arg1 is a list of strings
		GStringIterator it(&list);
		const char *p1 = it++;
		const char *p2 = it++;
		const char *p3 = it++;
		const char *p4 = it++;
		const char *p5 = it++;
		strResultForJava.Format(pzBoundStartupConfig,p1,p2,p3,p4,p5);
	}
	if (nOperation == 2) // do your own thing
	{
		strResultForJava = "nOperation 2 was called with [";
		strResultForJava << str1 << "] and [" << str2 << "]";
	}


	env->ReleaseStringUTFChars(jsArg1, str1);
	env->ReleaseStringUTFChars(jsArg2, str2);
	return env->NewStringUTF(strResultForJava.Buf());
	
}

extern int server_start(const char *pzStartupMessage = 0); // in ServerCore.cpp
void viewPorts(GString *pG = 0);
void showActiveThreads(GString *pG = 0);
void pingPools();

extern "C" JNIEXPORT jstring JNICALL Java_com_server_app_Server_stringFromJNI (JNIEnv *env, jobject)
{
    return env->NewStringUTF("Hello World: From C++;");
}

extern "C" JNIEXPORT jint JNICALL Java_com_server_app_Server_serverStart  (JNIEnv *env, jobject obj, jstring s)
{
	int nRet = 0;
	env->GetJavaVM(&g_javaVM);
	jclass cls = env->GetObjectClass(obj);
	g_activityClass = (jclass) env->NewGlobalRef(cls);
	g_activityObj = env->NewGlobalRef(obj);

	if (!g_isRunning)
	{
		g_isRunning = 1;

		SetServerCoreInfoLog( JavaInfoLog );

		const char *str = env->GetStringUTFChars(s,0);
		GString g("Starting Server");
		JavaInfoLog(777, g);

		
		GStringList list("&&",str);
		GStringIterator it(&list);
		const char *p1 = it++;
		const char *p2 = it++;
		const char *p3 = it++;
		const char *p4 = it++;
		const char *p5 = it++;

		GString strCfgData;
		strCfgData.Format(pzBoundStartupConfig,p1,p2,p3,p4,p5);

		SetProfile(new GProfile((const char *)strCfgData, (int)strCfgData.Length(), 0));

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


