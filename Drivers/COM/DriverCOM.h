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

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the DRIVERCOM_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// DRIVERCOM_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#ifdef DRIVERCOM_EXPORTS
#define DRIVERCOM_API __declspec(dllexport)
#else
#define DRIVERCOM_API __declspec(dllimport)
#endif

// This class is exported from the DriverCOM.dll
class DRIVERCOM_API CDriverCOM {
public:
	CDriverCOM(void);
	// TODO: add your methods here.
};

extern DRIVERCOM_API int nDriverCOM;

DRIVERCOM_API int fnDriverCOM(void);

