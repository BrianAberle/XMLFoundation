// --------------------------------------------------------------------------
//						United Business Technologies
//			  Copyright (c) 2000 - 2010  All Rights Reserved.
//
// Source in this file is released to the public under the following license:
// --------------------------------------------------------------------------
// This toolkit may be used free of charge for any purpose including corporate
// and academic use.  For profit, and Non-Profit uses are permitted.
//
// This source code and any work derived from this source code must retain 
// this copyright at the top of each source file.
// --------------------------------------------------------------------------
//
// IntegrationJava.cpp
//

#include "IntegrationJava.h"
#include "GException.h"
#include "GDirectory.h"

//requires "XMLFoundation\Library\inc\Win32" in the build path
#include "jni.h"

// entry points into Java Runtime Binaries
// been using.....

#ifdef _WIN32
typedef jint (*P_JNI_GetDefaultJavaVMInitArgs)(void *args);
typedef jint (*P_JNI_CreateJavaVM)(JavaVM **pvm, JNIEnv ** penv, void *args);
#else
typedef void (*fnJNI_GetDefaultJavaVMInitArgs)( JDK1_1InitArgs* );
typedef int (*fnJNI_CreateJavaVM)(JavaVM **pvm, void **penv, void *args); 
#endif



#include <string.h>
#include <stdlib.h>

void GetJavaClassMethods(const char *pzClassFile, GStringList *pDest);
void GetJavaMethodParams(const char *pzClassFile, const char *pzMethodName, GStringList *pDest);
char* buildArgs(JNIEnv *, jobjectArray, jint, jvalue*, GStringIterator &varArgs);







class JClass {
public:
	static jobjectArray getMethods(JNIEnv*, jclass);
	static const char* toString(JNIEnv*, jclass);
	static char* typeToSig(JNIEnv*, const char*, int*);
};

class Method {
public:
	static jobjectArray getParameterTypes(JNIEnv* env, jobject);

	static const char* getName(JNIEnv*, jobject);
	static int getModifiers(JNIEnv*, jobject); 
	static jclass getReturnType(JNIEnv*, jobject); 
	static const char* getDescription(JNIEnv* env, jobject method);
};

class Modifier {
public:
	static jboolean isStatic(JNIEnv*, int);
};



#ifdef __cplusplus
 extern "C" {
#endif

#ifdef WIN32
 typedef __int64 	EIGHTBYTES;
#else
 typedef long long	EIGHTBYTES;
#endif

typedef EIGHTBYTES (JNICALL *CSMA)(JNIEnv*, jclass, jmethodID, jvalue*);
typedef EIGHTBYTES (JNICALL *CMA )(JNIEnv*, jobject,jmethodID, jvalue*);


#ifdef __cplusplus
 }
#endif






// Class.getMethods

jobjectArray 
JClass::getMethods(JNIEnv* env, jclass clzObject) {
	// Input jclass argument is, e.g. 'Main' class
	// getMethods is a method in Class class
	jclass classClz = env->GetObjectClass(clzObject);
	jmethodID mid = env->GetMethodID(classClz,
				"getMethods",
				"()[Ljava/lang/reflect/Method;");
    return (jobjectArray) env->CallObjectMethod(clzObject, mid, NULL);
}

// Class.toString

const char* 
JClass::toString(JNIEnv* env, jclass clazz) {
	jboolean isCopy;
	jclass classClz = env->GetObjectClass(clazz);
	jmethodID mid = env->GetMethodID(classClz, "toString", "()Ljava/lang/String;");
	jstring str = (jstring) env->CallObjectMethod(clazz, mid, NULL);
	return env->GetStringUTFChars(str, &isCopy);
}
// Input
//   typeName - The value returned by classToString. It is
//		char* representation of type of a Class
// Return
//	The proper JNI signature code for input type
//	If typeIndex is not NULL, an index of type is returned
//  for use with function table
//
// Any 'class' type is assumed to be a java.lang.String


char*
JClass::typeToSig(JNIEnv* env, const char* typeName, int* typeIndex) {

	char * retVal = NULL;
	int index;
	
	if (strncmp(typeName, "class", 5) == 0) {
		retVal = "Ljava/lang/String;";
		index = 0;
	}
	if (strcmp(typeName, "boolean") == 0) {
		retVal = "Z";
		index = 1;
	}
	if (strcmp(typeName, "byte") == 0) {
		retVal = "B";
		index = 2;
	}
	if (strcmp(typeName, "char") == 0) {
		retVal = "C";
		index = 3;
	}
	if (strcmp(typeName, "short") == 0) {
		retVal = "S";
		index = 4;
	}
	if (strcmp(typeName, "int") == 0) {
		retVal = "I";
		index = 5;
	}
	if (strcmp(typeName, "long") == 0) {
		retVal = "J";
		index = 6;
	}
	if (strcmp(typeName, "float") == 0) {
		retVal = "F";
		index = 7;
	}
	if (strcmp(typeName, "double") == 0) {
		retVal = "D";
		index = 8;
	}
	if (strcmp(typeName, "void") == 0) {
		retVal = "V";
		index = 9;
	}
	if (typeIndex) 
		*typeIndex = index;
	return retVal;
}


// Method.getParameterTypes

jobjectArray Method::getParameterTypes(JNIEnv* env, jobject method) {
	jclass clazz = env->GetObjectClass(method);
	jmethodID mid = env->GetMethodID(clazz,
				"getParameterTypes",
				"()[Ljava/lang/Class;");
	return (jobjectArray) env->CallObjectMethod(method, mid, NULL);
}


const char* 
Method::getDescription(JNIEnv* env, jobject method) {
	jboolean isCopy;
	jclass clazz = env->GetObjectClass(method);
	jmethodID mid = env->GetMethodID(clazz,
				"toString",
				"()Ljava/lang/String;");
	jstring name = (jstring) env->CallObjectMethod(method, mid, NULL);
	return env->GetStringUTFChars(name, &isCopy);
}



// Method.getName
// A char* is returned. May as well do the conversion
// in this function.

const char* 
Method::getName(JNIEnv* env, jobject method) {
	jboolean isCopy;
	jclass clazz = env->GetObjectClass(method);
	jmethodID mid = env->GetMethodID(clazz,
				"getName",
				"()Ljava/lang/String;");
	jstring name = (jstring) env->CallObjectMethod(method, mid, NULL);
	return env->GetStringUTFChars(name, &isCopy);
}
// Method.getModifiers

int
Method::getModifiers(JNIEnv* env, jobject method) {
	jclass clazz = env->GetObjectClass(method);
	jmethodID mid = env->GetMethodID(clazz, "getModifiers", "()I");
	return env->CallIntMethod(method, mid, NULL);
}

// Method.getReturnType

jclass 
Method::getReturnType(JNIEnv* env, jobject method) {
	jclass clazz = env->GetObjectClass(method);
	jmethodID mid = env->GetMethodID(clazz,
				"getReturnType",
				"()Ljava/lang/Class;");
	return (jclass) env->CallObjectMethod(method, mid, NULL);
}

// Modifier reflection support
//
// Modifier.isStatic

jboolean
Modifier::isStatic(JNIEnv* env, int mods) {
	jclass clazz = env->FindClass("java/lang/reflect/Modifier");
	jmethodID mid = env->GetStaticMethodID(clazz, "isStatic", "(I)Z");
	return env->CallStaticBooleanMethod(clazz, mid, mods);
}








// Input:
//	params - An object array of Class object describing
//	a parameter list to a method.
//
//	nParams - size of object array
//
//	argValues - A jvalue array, allocated by caller, populated
//	by buildArgs
// 
//
// Returns:
//	The signature that corresponds to the params in object
//  array. User must free the char* that gets returned.
//
//	Upon return buildArgs has populated the argValues array
//	with the properly converted values.

char*
buildArgs(JNIEnv *env, 
		jobjectArray params, jint nParams,
		jvalue* argValues, GStringIterator &varArgs ) {

	char* typeSig = (char*) malloc(sizeof(char) * 128);
	int strCnt = 0;
	int reAllocCnt = 2;
	int i;
	char* val;

	if (typeSig == NULL)
		return NULL;

	memset((void*) typeSig, 0, sizeof(char) * 128);

	for (i = 0; i < nParams; i++) {
		char *pzCurrentArg = (char *)varArgs++;

		jclass pClz = (jclass) env->GetObjectArrayElement(params, i);
		const char* tName = JClass::toString(env, pClz);
		
		if (strCnt > 100) {
			typeSig = (char*) realloc((void*)typeSig, reAllocCnt * 128);
			reAllocCnt++;
			if (typeSig == NULL) {
				return NULL;
			}
		}
	
		if (strncmp(tName, "class", 5) == 0) {
			argValues[i].l = (jobject) env->NewStringUTF(pzCurrentArg);
			strcat(typeSig, 
				val = JClass::typeToSig(env, tName, NULL));
		}
		if (strcmp(tName, "boolean") == 0) {
			char* b = pzCurrentArg;
			if (strcmp(b, "true") == 0 ||
				strcmp(b, "TRUE") == 0 ||
				strcmp(b, "True") == 0 ||
				atoi(b) > 0)
				argValues[i].z = JNI_TRUE;
			else
				argValues[i].z = JNI_FALSE;

			strcat(typeSig, 
				val = JClass::typeToSig(env, tName, NULL));
		}
		if (strcmp(tName, "byte") == 0) {
			argValues[i].b = (jbyte) atoi(pzCurrentArg);
			strcat(typeSig, 
				val = JClass::typeToSig(env, tName, NULL));
		}
		if (strcmp(tName, "char") == 0) {
			argValues[i].c = (jchar) atoi(pzCurrentArg);
			strcat(typeSig, 
				val = JClass::typeToSig(env, tName, NULL));
		}
		if (strcmp(tName, "short") == 0) {
			argValues[i].s = (jshort) atoi(pzCurrentArg);
			strcat(typeSig, 
				val = JClass::typeToSig(env, tName, NULL));
		}
		if (strcmp(tName, "int") == 0) {
			argValues[i].i = (jint) atoi(pzCurrentArg);
			strcat(typeSig, 
				val = JClass::typeToSig(env, tName, NULL));
		}
		if (strcmp(tName, "long") == 0) {
			argValues[i].j = (jlong) atol(pzCurrentArg);
			strcat(typeSig, 
				val = JClass::typeToSig(env, tName, NULL));
		}
		if (strcmp(tName, "float") == 0) {
			argValues[i].f = (jfloat) atof(pzCurrentArg);
			strcat(typeSig, 
				val = JClass::typeToSig(env, tName, NULL));
		}
		if (strcmp(tName, "double") == 0) {
			argValues[i].d = (jdouble) atof(pzCurrentArg);
			strcat(typeSig, 
				val = JClass::typeToSig(env, tName, NULL));
		}
		strCnt += strlen(val);
	}
	return typeSig;
}


// Dots are changed to / character.
// Input arg is both changed and returned
//
char*
dotsToSlashes(char* dotPath) {
        char *p = dotPath;
        while (*p) {
                if (*p == '.')
                        *p = '/';
                p++;
        }
        return dotPath;
}


JavaVM * JavaInterface::m_jvm = 0;

#ifdef _WIN32
	#define PATH_SEPARATOR ';'
#else 
	#define PATH_SEPARATOR ':'
#endif



#include "ExceptionHandler.h"
#include "DynamicLibrary.h"

JavaInterface::JavaInterface( const char *pzComponentPath, const char *pzComponentSettings)
	:	BaseInterface(pzComponentPath, pzComponentSettings)
{
	if (m_jvm)
		return;
	JNIEnv *env;

#ifdef _WIN32
	MS_JDK1_1InitArgs vm_args;
	JNI_GetDefaultJavaVMInitArgs(&vm_args);
	vm_args.nVersion = 0x00010001;
	vm_args.pcszClasspath = (char *)(const char *)m_strComponentPath;
	int nRet = JNI_CreateJavaVM(&m_jvm, &env, &vm_args);
	if (nRet < 0) 
	{
		throw GException("JavaIntegration", 1,nRet);
	}
#else
#ifdef _DYNAMIC_JAVA_LINK_
	JavaVMInitArgs vm_args;
	void *dllHandle = _OPENLIB("libjvm.so");
	if (dllHandle)
	{
	   JavaVMOption options[3];
	   GString strTemp;
	   strTemp.Format("-Djava.class.path=%s",(const char *)m_strComponentPath);
	   options[0].optionString = "-Djava.compiler=NONE"; // disable JIT
	   options[1].optionString = (char *)(const char *)strTemp;
	   vm_args.options = options;
	   vm_args.version = JNI_VERSION_1_2;
	   vm_args.nOptions = 2;
	   vm_args.ignoreUnrecognized = JNI_FALSE;

		void *pfn2 = 0;
		ExceptionHandlerScope duration;
		XML_TRY
		{
			pfn2 = (void *)_PROCADDRESS(dllHandle, "JNI_CreateJavaVM");
		}
		XML_CATCH(ex)
		{
			throw GException("JavaIntegration", 6, getenv("LD_LIBRARY_PATH"));
		}
		if (pfn2)
		{
			cout << "Creating VM\n";
			int nRet = ((fnJNI_CreateJavaVM)pfn2)(&m_jvm, (void **) &env, &vm_args);
			if (nRet < 0) 
			{
				throw GException("JavaIntegration", 1,nRet);
			}
			else
			{
				cout << "Created Java Interpreter\n";
			}
		}
	}
	else
	{
		throw GException("JavaIntegration", 5, getenv("LD_LIBRARY_PATH"));
	}
#else
   JavaVMInitArgs vm_args;
   jint ret;
   
   JavaVMOption options[3];
   GString strTemp;
   strTemp.Format("-Djava.class.path=%s",(const char *)m_strComponentPath);
   options[0].optionString = "-Djava.compiler=NONE"; // disable JIT
   options[1].optionString = (char *)(const char *)strTemp;
    
   vm_args.options = options;
   vm_args.version = JNI_VERSION_1_2;
   vm_args.nOptions = 2;
   vm_args.ignoreUnrecognized = JNI_FALSE;

	ret = JNI_CreateJavaVM(&m_jvm, (void **)&env, &vm_args);
	if (ret < 0) 
	{
		GString strErr;
		strErr.Format("JNI_CreateJavaVM() failed. Error code[%ld]\nThis may be caused by a wrong version of java or incorrect LD_LIBRARY_PATH", ret);
		throw GException (-140, (const char *)strErr);
	}
#endif
#endif



/*
	XML_TRY
	{
		ret = JNI_CreateJavaVM(&m_jvm, (void **)&env, &vm_args);
	}
	XML_CATCH(ex)
	{
		GString strErr;  
		strErr.Format("JNI_CreateJavaVM() failed with error[%d:%s]\nThis may be caused by a wrong version of java or incorrect LD_LIBRARY_PATH", ex.getErrorCode(), ex.getDescription());
		throw XMLException(-140,(const char *)strErr);
	}
	if (ret < 0) 
	{
		GString strErr;
		strErr.Format("JNI_CreateJavaVM() failed. Error code[%ld]\nThis may be caused by a wrong version of java or incorrect LD_LIBRARY_PATH", ret);
		throw XMLException(-140,(const char *)strErr);
	}
*/

}

JavaInterface::~JavaInterface()
{
	// should be done only once at application shutdown
	// m_jvm->DestroyJavaVM();
}

// returns class files (maybe JAR and Applet files in the future)
GStringList* JavaInterface::GetComponents()
{
	m_pObjectList.RemoveAll();
	GDirectory dir(m_strComponentPath);
	GStringIterator it(&dir);
	while (it())
		m_pObjectList.AddLast( it++ );

	return &m_pObjectList;
}

GStringList *JavaInterface::GetInterfaces(const char *pzJavaObjectFile)
{
	m_pInterfaceList.RemoveAll();
	GString GStr;
	GStr = pzJavaObjectFile;

	// assumes pzJavaObjectFile is a *.class file, in this case * is the Interface
	if ( (GStr.Right(6)).CompareNoCase(".class") == 0)
	{
		m_pInterfaceList.AddLast((const char *)(GStr.Left(GStr.Length() - 6)));
	}
	else
	{
		// if the pzJavaObjectFile is a jar or applet open it and query for interfaces
		m_pInterfaceList.AddLast("Unsupported Object Type!");
	}
	return &m_pInterfaceList;
}


// Ask the VM to describe an object
GStringList *JavaInterface::GetMethods(const char *pzClassFile, const char *pzClass)
{
	m_pMethodList.RemoveAll();
	JNIEnv *env;
#ifdef _WIN32
	MS_JDK1_1AttachArgs args;
#else
	JavaVMAttachArgs args;
	args.version= JNI_VERSION_1_2;
	args.name="user";
	args.group=NULL;
#endif
	jclass clazz;

#ifdef _WIN32
	if (m_jvm->AttachCurrentThread(&env, &args) < 0) 
#else
	if (m_jvm->AttachCurrentThread((void **)&env, &args) < 0) 
#endif
	{
		//JavaVM - Can't attach current thread to VM
		throw GException("JavaIntegration", 2);
	}
	clazz = env->FindClass(dotsToSlashes((char *)pzClass));
	if (clazz == 0) 
	{
		GString strErr;
		strErr.Format("JavaVM - Can't locate the [%s] class.", pzClass);
		m_jvm->DetachCurrentThread();
		return &m_pMethodList; // just return an empty list.
	}
	jobject methodObj = NULL;
   
	// Get an arrary of all the methods available in the .class specified
	jobjectArray clazzMethods = JClass::getMethods(env, clazz);
	int nMethods = (clazzMethods) ? env->GetArrayLength(clazzMethods) : 0;

	for (int j = 0; j < nMethods; j++) 
	{
		jobject method = env->GetObjectArrayElement(clazzMethods, j);
		m_pMethodList.AddLast( Method::getName(env, method) );
	}
	m_jvm->DetachCurrentThread();
	return &m_pMethodList;
}

// Ask the VM to describe a Method
GStringList *JavaInterface::GetMethodParams(const char *pzClassFile, const char *pzClass, const char *pzMethodName)
{
	m_pParamNameList.RemoveAll();
	m_pParamTypeList.RemoveAll();

	jclass clazz;
	JNIEnv *env;
#ifdef _WIN32
	MS_JDK1_1AttachArgs args;
#else
	JavaVMAttachArgs args;
	args.version= JNI_VERSION_1_2;
	args.name="user";
	args.group=NULL;
#endif

#ifdef _WIN32
	if (m_jvm->AttachCurrentThread(&env, &args) < 0) 
#else
	if (m_jvm->AttachCurrentThread((void **)&env, &args) < 0) 
#endif
	{
		//JavaVM - Can't attach current thread to VM
		throw GException("JavaIntegration", 2);
	}

	clazz = env->FindClass(dotsToSlashes((char *)pzClass));

	if (clazz == 0) 
	{
		//JavaVM - Can't locate the [%s] class.
		throw GException("JavaIntegration", 3, (char *)pzClass);
	}
	jobject methodObj = NULL;
   
	// Get an arrary of all the methods available in the .class specified
	jobjectArray clazzMethods = JClass::getMethods(env, clazz);
	const char* methodToInvoke = pzMethodName;
	int nMethods = env->GetArrayLength(clazzMethods);

	for (int j = 0; j < nMethods; j++) 
	{
		jobject method = env->GetObjectArrayElement(clazzMethods, j);
		if (strcmp(methodToInvoke, Method::getName(env, method)) == 0) 
		{
			methodObj = method;
			break;
		}
	}
	if (methodObj == NULL) 
	{
		m_jvm->DetachCurrentThread();
		throw GException("JavaIntegration", 4, methodToInvoke);
	}

	// a string like "public static void MYClass.MyMethod(int,float)"
	const char *pzMethodSignature = Method::getDescription(env, methodObj);


	// describe the params
	jobjectArray mParams = Method::getParameterTypes(env, methodObj);
	int mParamsCnt = env->GetArrayLength(mParams);
	jvalue * argVals = (jvalue *) malloc(sizeof(jvalue) * mParamsCnt);

	// As far as I know there is no way to retrieve the actual parameter
	// names (through Reflection Classes anyhow), Uses: Param1 Param2 ...
	char pzParamBuf[32];
	for (int i = 0; i < mParamsCnt; i++) 
	{
		jclass pClz = (jclass) env->GetObjectArrayElement(mParams, i);
		const char* tName = JClass::toString(env, pClz);
		m_pParamTypeList.AddLast( tName );
		sprintf(pzParamBuf,"Param%d",i+1);
		m_pParamNameList.AddLast( pzParamBuf );
	}
   jclass mReturn = Method::getReturnType(env, methodObj);
   m_strReturnType = JClass::toString(env, mReturn);

   m_jvm->DetachCurrentThread();
   return &m_pParamNameList;
}

GStringList* JavaInterface::GetMethodParamTypes()
{
	return &m_pParamTypeList;
}

const char *JavaInterface::GetMethodReturnType()
{
	return m_strReturnType;
}


int JavaInterface::IsAvailable(const char *pzClassFile, const char *pzClass, const char *pzMethodName)
{
	GStringList *pList = GetMethods(pzClassFile, pzClass);
	GStringIterator it( pList );
	GString strMatch(pzMethodName);
	while (it())
	{
		if (strMatch.Compare(it++) == 0)
		{
			return 1;
		}
	}
	return 0;
}


const char * JavaInterface::Invoke(const char *pzClassFile, const char *pzClass, const char *pzMethodName, GStringIterator &varArgs)
{
	GString strErr;
	jclass clazz;
	JNIEnv *env;
#ifdef _WIN32
	MS_JDK1_1AttachArgs args;
#else
	JavaVMAttachArgs args;
	args.version= JNI_VERSION_1_2;
	args.name="user";
	args.group=NULL;
#endif

#ifdef _WIN32
	if (m_jvm->AttachCurrentThread(&env, &args) < 0) 
#else
	if (m_jvm->AttachCurrentThread((void **)&env, &args) < 0) 
#endif
	{
		//JavaVM - Can't attach current thread to VM
		throw GException("JavaIntegration", 2);
	}

	clazz = env->FindClass(dotsToSlashes((char *)pzClass));

	if (clazz == 0) 
	{
		m_jvm->DetachCurrentThread();
		throw GException("JavaIntegration", 3, (char *)pzClass);
	}
	jobject methodObj = NULL;
   
	// Get an arrary of all the methods available in the .class specified
	jobjectArray clazzMethods = JClass::getMethods(env, clazz);
	const char* methodToInvoke = pzMethodName;
	int nMethods = env->GetArrayLength(clazzMethods);

	for (int j = 0; j < nMethods; j++) 
	{
		jobject method = env->GetObjectArrayElement(clazzMethods, j);
		if (strcmp(methodToInvoke, Method::getName(env, method)) == 0) 
		{
			methodObj = method;
			break;
		}
	}
	if (methodObj == NULL) 
	{
		throw GException("JavaIntegration", 4, methodToInvoke);
	}

	// a string like "public static void MYClass.MyMethod(int,float)"
	const char *pzMethodSignature = Method::getDescription(env, methodObj);


	// describe the params
	jobjectArray mParams = Method::getParameterTypes(env, methodObj);
	int mParamsCnt = env->GetArrayLength(mParams);
	jvalue * argVals = (jvalue *) malloc(sizeof(jvalue) * mParamsCnt);

	// As far as I know there is no way to retrieve the actual parameter
	// names (through Reflection Classes anyhow), Uses: Param1 Param2 ...
	char pzParamBuf[32];
	for (int i = 0; i < mParamsCnt; i++) 
	{
		jclass pClz = (jclass) env->GetObjectArrayElement(mParams, i);
		const char* tName = JClass::toString(env, pClz);
		m_pParamTypeList.AddLast( tName );
		sprintf(pzParamBuf,"Param%d",i+1);
		m_pParamNameList.AddLast( pzParamBuf );
	}


//	bind params
	const char * paramSig = buildArgs(env, mParams, mParamsCnt, argVals, varArgs );

	int mIndex;
	jclass mReturn = Method::getReturnType(env, methodObj);
	const char* clType = JClass::toString(env, mReturn);
	const char* retSig = JClass::typeToSig(env, clType, &mIndex);

	// ( paramSig ) retSig + newline
	int sigSize = strlen(paramSig) + strlen(retSig) + 3;
	char* methodSig = (char*) malloc(sigSize);
	sprintf(methodSig, "(%s)%s", paramSig, retSig);
	free((void*)paramSig);

	// clear out the return value left over from the last call
	m_strInvokeReturnValue.Empty();

	int mods = Method::getModifiers(env, methodObj);
	jmethodID mid;
	if (Modifier::isStatic(env, mods)) 
	{
		mid = env->GetStaticMethodID(clazz, methodToInvoke, methodSig);
		if (mid == 0) 
		{
			m_jvm->DetachCurrentThread();
			throw GException("JavaIntegration", 4, methodToInvoke);
		}

		switch(mIndex)
		{
		case 0: //env->CallStaticObjectMethodA( clazz, mid, argVals);
			{
			jboolean isCopy;
			jstring name = (jstring) env->CallStaticObjectMethodA(clazz, mid, argVals);
			m_strInvokeReturnValue << (const char *)env->GetStringUTFChars(name, &isCopy);
			}
			break;

		case 1: m_strInvokeReturnValue << env->CallStaticBooleanMethodA( clazz, mid, argVals); break;
		case 2: m_strInvokeReturnValue << env->CallStaticByteMethodA(clazz, mid, argVals);break;
		case 3: m_strInvokeReturnValue << env->CallStaticCharMethodA(clazz, mid, argVals);break;
		case 4: m_strInvokeReturnValue << env->CallStaticShortMethodA(clazz, mid, argVals);break;
		case 5: m_strInvokeReturnValue << env->CallStaticIntMethodA(clazz, mid, argVals);break;
		case 6: m_strInvokeReturnValue << env->CallStaticLongMethodA(clazz, mid, argVals);break;
		case 7: m_strInvokeReturnValue << env->CallStaticFloatMethodA(clazz, mid, argVals);break;
		case 8: m_strInvokeReturnValue << env->CallStaticDoubleMethodA(clazz, mid, argVals);break;
		case 9: env->CallStaticVoidMethodA(clazz, mid, argVals);break;
		} 
	}
	else 
	{
		mid = env->GetMethodID(clazz, "<init>", "()V");
		jobject obj = env->NewObject(clazz, mid, NULL);
		mid = env->GetMethodID(clazz, methodToInvoke, methodSig);
		if (mid == 0) 
		{
			m_jvm->DetachCurrentThread();
			throw GException("JavaIntegration", 4, methodToInvoke);
		}
		switch(mIndex)
	   {
		case 0: // better be a string object.
			{
			jboolean isCopy;
			jstring name = (jstring) env->CallObjectMethodA(obj, mid, argVals);
			m_strInvokeReturnValue << (const char *)env->GetStringUTFChars(name, &isCopy);
			}
			break;
		case 1: m_strInvokeReturnValue << env->CallBooleanMethodA(obj, mid, argVals);break;
		case 2: m_strInvokeReturnValue << env->CallByteMethodA(obj, mid, argVals);break;
		case 3: m_strInvokeReturnValue << env->CallCharMethodA(obj, mid, argVals);break;
		case 4: m_strInvokeReturnValue << env->CallShortMethodA(obj, mid, argVals);break;
		case 5: m_strInvokeReturnValue << env->CallIntMethodA(obj, mid, argVals); break;
		case 6: m_strInvokeReturnValue << env->CallLongMethodA(obj, mid, argVals);break;
		case 7: m_strInvokeReturnValue << env->CallFloatMethodA(obj, mid, argVals);break;
		case 8: m_strInvokeReturnValue << env->CallDoubleMethodA(obj, mid, argVals);break;
		case 9: env->CallVoidMethodA(obj, mid, argVals);break;
	   }

   }
   m_jvm->DetachCurrentThread();

  return m_strInvokeReturnValue;
}

