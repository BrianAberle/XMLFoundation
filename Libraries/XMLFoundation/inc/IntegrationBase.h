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
#ifndef _INTEGRATION_BASE_H__
#define _INTEGRATION_BASE_H__

#include "GString.h"
#include "GStringList.h"

#ifndef _BASE__IFACE_H__
#include "BaseInterface.h"
#endif


class InterfaceInstance : public BaseInterface
{
	GString m_strDriver; // name passed in on construction
	GString m_InvokeArgDelimit; // delimiter used to marshal argument data
	
	GString m_strResults;	// default dynamic result buffer
	GString *m_pActiveResultGString;  // pointer to m_strResults by default;
	
	int m_bInUse;		// 1 when issued, 0 while idle in cache
	int	m_bLockLib;
	int m_nThreadOperationMode;

	void *m_dllHandle;
	void *m_iHandle;
	typedef char *(*fnGetDriverName)(void);
	typedef char *(*fnDriverOperation)(void);

	typedef void *(*fnConstruct)(const char *,const char *);
	typedef char *(*fnGetComponents)(void *);
	typedef char *(*fnGetInterfaces)(void *,const char *);
	typedef char *(*fnGetMethods)(void *,const char *,const char *);
	typedef char *(*fnGetMethodParams)(void *,const char *,const char *,const char *);
	typedef int   (*fnIsAvailable)(void *,const char *,const char *,const char *);
	typedef char *(*fnInvoke)(void *,  const char *,const char *,const char *,const char *);
	typedef int   (*fnSetOption)(void *,const char *,void *);
	typedef char *(*fnGetOption)(void *,const char *);
	typedef void  (*fnDestruct)(void *);

	

	

	fnConstruct				m_fnConstruct;
	fnGetComponents			m_fnGetComponents;
	fnGetInterfaces			m_fnGetInterfaces;
	fnGetMethods			m_fnGetMethods;
	fnGetMethodParams		m_fnGetMethodParams;
	fnIsAvailable			m_fnIsAvailable;
	fnSetOption				m_fnSetOption;
	fnGetOption				m_fnGetOption;
	fnInvoke				m_fnInvoke;
	fnDestruct				m_fnDestruct;
	fnDriverOperation		m_fnDriverOperation;

	void Construct();

public:
	InterfaceInstance( const InterfaceInstance &cpyFrm );
	InterfaceInstance( const char *pzDriver);
	~InterfaceInstance();
	GStringList* GetComponents();
	GStringList* GetInterfaces(const char *pzObject);
	GStringList* GetMethods(const char *pzObject, const char *pzInterface);
	GStringList* GetMethodParams(const char *pzObject, const char *pzInterface, const char *pzMethodName);
	int IsAvailable(const char *pzObject, const char *pzInterface, const char *pzMethodName);
	
	// argument data must not contain the data sequence "¿&~&¿" unless you first change
	// driver markup format by calling SetDriverOption("Delimit","YourOwnMarkup!")
	const char * Invoke(  const char *pzObject, const char *pzInterface, const char *pzMethodName, GStringIterator &varArgs);


	// InvokeEx() allows the caller to prepare the argument data and manage the memory.
	// This allows the implementation to read(and write) the argument data directly from 
	// the callers heap or stack, useful for large arguments as memory buffers are not 
	// copied before calling the entension, C++ extensions will receive direct memory 
	// address, Interpretors (java.perl,python........) recieve the memory in the most 
	// optimal way and may work in direct or copied memory depending on implementation.
	// To pass a single argument - "Hello World", pArgData is the address to memory containing
	// the "H", and pzArgSizes is the data length in string format, so "11" or "12" if you 
	// count the null.  The length is "info only" since 
	// 
	// To pass 2 arguments - They must reside in contigous memory, for example to pass
	// "Luck is for the hopeless¿13 don't scare me¿" where each statement is it's own
	// argument, and ¿ are NULLs then pzArgSizes = "25|18" - each argument length pipe delimited.
	// while pArgData is the address of the byte containing "L".
	// and so on for more than 2 arguments where (25|18|13|777) describes a 4 argument call. 
	// 
	// Data may be binary as well, and nulls may be embedded in the argument values
	// if you are using a C++ driver - otherwise it depends on each driver implementation.
	
	// The data returned may have nulls or be binary as well, and *pnOutResultSize will be
	// set to the total length of the return value.
	const char * InvokeEx(const char *pzObject, const char *pzInterface, const char *pzMethodName, const char *pArgData, const char *pzArgSizes, int *pnOutResultSize);


	int SetOption(const char *pzOption, void *pzValue);
	void *GetOption(const char *pzOption);

	// return detailed information about the last method described by GetMethodParams()
	GStringList* GetMethodParamTypes();
	const char * GetMethodReturnType();

	int GetThreadMode() {return m_nThreadOperationMode;}
	const char *GetDriverName() {return m_strDriver;}
	int InUse(){return m_bInUse;}
	void InUse(int nVal){m_bInUse = nVal;}

};




/*  Usage Sample walking object interfaces

#include "IntegrationCOM.h"
#include "IntegrationJava.h"
#include "IntegrationTXML.h"
#include "IntegrationPerl.h"

//	COMInterface CI;
//	JavaInterface CI;
//	PerlInterface CI;
	TXMLInterface CI;
	GStringIterator ObjectList( CI.GetComponents() );
	while (!ObjectList)
	{
		GString *strObject = ObjectList++;
		cout << "Object: " << (const char *)*strObject << "\n" << flush;
		GStringIterator InterfaceList( CI.GetInterfaces( *strObject ) );
		while (!InterfaceList)
		{
			GString *strInterface = InterfaceList++;
			cout << "     Interface: " << (const char *)*strInterface << "\n" << flush;
			GStringIterator MethodList( CI.GetMethods( *strObject, *strInterface) );
			while (!MethodList)
			{
				GString *strMethod = MethodList++;
				cout << "          " << (const char *)*strMethod << "(" << flush;
				GStringIterator ParamNameList( CI.GetMethodParams(*strObject, *strInterface, *strMethod) );
				GStringIterator ParamTypeList (CI.GetMethodParamTypes());
				int i = 0;
				while (!ParamNameList)
				{
					if (i > 0)
					{
						cout << " , ";
					}
					i++;
					GString *strParamName = ParamNameList++;
					GString *strParamType = ParamTypeList++;
					cout << (const char *)*strParamType << " " << (const char *)*strParamName << flush;
				}

				cout << ")\n" << flush;
			}
		}
	}
*/




#endif
