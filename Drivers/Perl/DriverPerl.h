
// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the DRIVERPERL_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// DRIVERPERL_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#ifdef DRIVERPERL_EXPORTS
#define DRIVERPERL_API __declspec(dllexport)
#else
#define DRIVERPERL_API __declspec(dllimport)
#endif

// This class is exported from the DriverPerl.dll
class DRIVERPERL_API CDriverPerl {
public:
	CDriverPerl(void);
	// TODO: add your methods here.
};

extern DRIVERPERL_API int nDriverPerl;

DRIVERPERL_API int fnDriverPerl(void);

