
// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the DRIVERPython_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// DRIVERPython_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#ifdef DRIVERPython_EXPORTS
#define DRIVERPython_API __declspec(dllexport)
#else
#define DRIVERPython_API __declspec(dllimport)
#endif

// This class is exported from the DriverPython.dll
class DRIVERPython_API CDriverPython {
public:
	CDriverPython(void);
	// TODO: add your methods here.
};

extern DRIVERPython_API int nDriverPython;

DRIVERPython_API int fnDriverPython(void);

