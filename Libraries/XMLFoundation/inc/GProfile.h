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

#ifndef __PROFILE_H__
#define __PROFILE_H__

// GProfile uses 
//	GString, GStringList

// Comments start with ';'
// Section names are enclosed in []
// All section values are name=value pairs
// All white space is ignored
//
// Example:
//		[Settings]
//			Cache=true
//			Errors=/errors/english/errors.txt
//			Rules=/rules/

#include "GList.h"
#include "GString.h"
#include "GStringList.h"
#include "GBTree.h"


class GProfile
{
	typedef int(*fnChangeNotify)(const char *,const char *,const char *);
	GBTree	*m_pTreeNotify;
	int ChangeNotify(const char *pzSection, const char *pzEntry, const char *pzValue);
	long SetConfig(const char *szSection, const char *szEntry, const char *nValue, short bSetDefault);
	GStringList lstChangeNotifications;

public:
	// RegisterChangeNotification usage:
	// add this global function in your code:
	// --------------------------------------
	//int GProfileMonitor(const char *pzSection, const char *pzEntry, const char *pzNewValue)
	//{
	//	 if (strcmp(pzEntry,"SysValue") == 0) // should be case insensitive
	//		 g_nSysValue = atoi(pzNewValue);
	//}
	// --------------------------------------
	// and register the callback like this:
	// myProfile.RegisterChangeNotification("MySection","SysValue",GProfileMonitor);
	// --------------------------------------
	// WARNING:Never assign a monitored value from this callback or you will recurse infinitely.
	//
	// If you supply pzSection and pzEntry, your notification will be called only when
	// that value is changed, if you pass NULL for pzEntry, then you will get all changes
	// to the pzSection.
	void RegisterChangeNotification(const char *pzSection, const char *pzEntry, fnChangeNotify fn);
	void UnRegisterChangeNotification(const char *pzSection, const char *pzEntry);
	
	// returns a list of "Section\tName" strings identifying all registered Change Notifications
	GStringList *ListChangeNotifications(){return &lstChangeNotifications;}

	// 	AttachNotifications() and DetatchNotifications() affect the entries created by RegisterChangeNotification()

	//  This can be used when rebuilding the entire profile for example:
	//
	//	GString strINIFile;
	// 	GetProfile().WriteCurrentConfig(&strINIFile);
	//  EditInWordPad(strINIFile);
	//	GProfile *pNew = new GProfile((const char *)strINIFile, strINIFile.Length());
	//  pNew->AttachNotifications(GetProfile().DetatchNotifications());
	//	delete SetProfile(pNew);
	//  pNew->ExecuteAllNotifications();
	GBTree *DetatchNotifications();
	GBTree *AttachNotifications(GBTree *pTree);
	void ExecuteAllNotifications();

	typedef struct
	{
		GString m_strName;
		GString m_strValue;
	} NameValuePair;

	typedef struct
	{
		GString		m_strName;
		GList		m_lstNVP;
	} Section;

protected:

	GString m_strEnvOverride;
	GString m_strBaseFileName; // no path

	GList m_lstSections;

	Section *FindSection(const char *szSection);
	NameValuePair *FindKey(const char *szKey, Section *pSection);

	GString m_strFile;
	short m_bCached;

	void Destroy();
	void GetLine(GString &strLine, const char *szBuffer, __int64 *nIdx, __int64 dwSize);
	void Load();

#ifdef _WIN32
	void ThrowLastError(const char *pzFile);
#endif
	void ProfileParse(const char *szBuffer, __int64 dwSize);
	long WriteCurrentConfigHelper(const char *pzPathAndFileName, GString *pDest, const char *pzSection = 0);

public:
	const char *LastLoadedConfigFile(){return m_strFile;}

	// The section/entry will be created when it does not already exist
	// The existing value will be repalced with the supplied value when it does exist
	long SetConfig(const char *szSection, const char *szEntry, const char *pzValue);
	long SetConfig(const char *szSection, const char *szEntry, int nValue);
	long SetConfig(const char *szSection, const char *szEntry, long lValue);		// short bSetDefault = 0
	long SetConfig(const char *szSection, const char *szEntry, __int64 lValue);
	long SetConfigBinary(const char *szSection, const char *szEntry, unsigned char *lValue, int nValueLength);
	long SetConfigCipher(const char *szSection, const char *szEntry, const char *pzPassword, const char *lValue, int nValueLength);

	// if the section and entry exist, the value will remain unchanged.
	// otherwise it will be created and given the default value specified
	long SetConfigDefault(const char *szSection, const char *szEntry, const char *pzValue);
	long SetConfigDefault(const char *szSection, const char *szEntry, int nValue);
	long SetConfigDefault(const char *szSection, const char *szEntry, long lValue);
	long SetConfigDefault(const char *szSection, const char *szEntry, __int64 lValue);
	long SetConfigBinaryDefault(const char *szSection, const char *szEntry, unsigned char *lValue, int nValueLength);
	long SetConfigCipherDefault(const char *szSection, const char *szEntry, const char *pzPassword, const char *lValue, int nValueLength);

	// SetString() has been depreciated - use SetConfig()



	// Writes memory config to a given destination.
	// An existing file will be overwritten, GString will be appended to.
	// Returns the number of bytes written to the destination on success or 0 for failure.
	long WriteCurrentConfig(const char *pzPathAndFileName);
	long WriteCurrentConfig(GString *pzDestStr);
	// just like WriteCurrentConfig(), but only serialize a single [section]
	long WriteCurrentConfigSection(GString *pzDestStr, const char *pzSection);


	// function retrieves the names of all sections
	void  GetSectionNames(GStringList *lpList);

	// function retrieves all the keys and values for the specified section 
	// returns NULL if specified section doesn't exist, use this code to iterate a section:
	///////////////////////////////////////////////////////////////////////////////////
	//	GListIterator itNVP(GetProfile().GetSection("MySection"));
	//	while (itNVP())
	//	{
	//		GProfile::NameValuePair *pNVP = (GProfile::NameValuePair *)itNVP++;
	//		// pNVP->m_strName
	//		// pNVP->m_strValue
	//	}
	const GList *GetSection(const char *szSectionName);

	// returns the number of entries for a given section
	int GetSectionEntryCount(const char *szSectionName);

	// RemoveSection() and AddSection() do as their names suggest affecting an entire [Section] with all entries in it
	GProfile::Section *RemoveSection(const char *szSection);
	void AddSection(GProfile::Section *pS, int bIssueChangeNotification = 1);

	// deletes an entry under the specified [Section]
	int RemoveEntry(const char *szSection, const char *szEntry);

	// the string length of the value. 0 is empty or non existing
	int ValueLength(const char *szSection, const char *szEntry);

	// returns 1 if the section (and entry) exists, otherwise 0
	int DoesExist(const char *szSectionName, const char *pzEntry);
	int DoesExist(const char *szSectionName);
	

	////////////////////////////////////////////////////////////////
	// same as the Get*'s (below) but no exception will be thrown since 
	// a default value is returned when queried value does not exist
	////////////////////////////////////////////////////////////////
	short GetBoolOrDefault(const char *szSectionName, const char *szKey, short bDefault);
	__int64 GetInt64OrDefault(const char *szSectionName, const char *szKey, __int64 nDefault);
	long  GetLongOrDefault(const char *szSectionName, const char *szKey, long lDefault);
	int	  GetIntOrDefault(const char *szSectionName, const char *szKey, int nDefault);
	const char *GetStringOrDefault(const char *szSectionName, const char *szKey, const char *pzDefault);
	// same as GetStringOrDefault but result will always end with a slash
	const char *GetPathOrDefault(const char *szSectionName, const char *szKey, const char *pzDefault);
	

	// GetBinary() will only properly convert to binary if the value was created with SetConfigBinary()
	unsigned char *GetBinary(const char *szSectionName, const char *szKey, GString &strDest);
	// GetCiphered() will only properly decipher if the value was created with SetConfigCipher()
	const char *GetCiphered(const char *szSectionName, const char *szKey, const char *pzPassword, GString &strDest);



	////////////////////////////////////////////////////////////////
	// The following methods return a specified data type or 
	// WARNING: these throw a GException if bThrowNotFound = true
	// This is handy when loading large blocks of required settings, 
	// just try--catch the whole block.  
	// Unlike the Windows GetProfileInt(), this allows you to know if the setting was available.
	// This allows you to determine the difference between "0" and "Does not exist"
	////////////////////////////////////////////////////////////////
	// function retrieves a boolean from the specified section
	short GetBoolean(const char *szSectionName, const char *szKey, short bThrowNotFound = true );
	short GetBool(const char *szSectionName, const char *szKey, short bThrowNotFound = true);
	__int64 GetInt64(const char *szSectionName, const char *szKey, short bThrowNotFound = true);
	long  GetLong(const char *szSectionName, const char *szKey, short bThrowNotFound = true);
	int	  GetInt(const char *szSectionName, const char *szKey, short bThrowNotFound = true);
	const char *GetString(const char *szSectionName, const char *szKey, short bThrowNotFound = true);
	// same as GetString() but ensures a trailing \ in NT and a / in UNIX
	const char *GetPath(const char *szSectionName, const char *szKey, short bThrowNotFound = true);

	

	// base file name like "txml.txt" - and - ENV_VAR_NAME
	// for use in a statement like this: 
	// SetProfile(new GProfile("5Loaves.txt","FIVE_LOAVES_CONFIG"));
	GProfile(const char *pzFileName, const char *pzEnvOverride );

	// load the profile configuration file yourself, 
	// and create this object "with no disk config file"
	GProfile(const char *szConfigData, __int64 dwSize);
	
	// load from file, no environment override allowed
	GProfile(const char *pzFilePathAndName);

	// create an empty profile.
	// you will have to add values using OverRideConfig()
	GProfile();


	~GProfile();
};
/*
class NameValuePairIterator
{
	GList::Node *pDataNode;
	GList *pTList;
public:
	GList::Node *pCurrentNode;    // used to return reference to data

	NameValuePairIterator(const GList *pList);
	~NameValuePairIterator() {};
	int operator ! () const;
	const GProfile::NameValuePair *operator ++ (int);
};
*/

GProfile &GetProfile();
GProfile *SetProfile(GProfile *pApplicationProfile);


#endif // __PROFILE_H__
