// --------------------------------------------------------------------------
//						United Business Technologies
//			  Copyright (c) 2000 - 2014  All Rights Reserved.
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

// GProfile stores application configuration settings.  GProfile is much like the Windows Registry in design.
// The Windows Registry is rightly called a database, It has it's own format.  
// GProfile has can use either INI or XML format.
//
// From 2000 up utill 2014 GProfile used only the .INI file format which is VERY easy to hand edit,
// and it is VERY easy to read.  In both categories, Readability and Hand-editability INI wins over XML.
//
///////////////////////////////////////////////////////////////////
// This is the exact technical definition of INI format:
///////////////////////////////////////////////////////////////////
// Comments start with ';'
// Section names are enclosed in []
// All section values are "name=value"pairs
// All white space is ignored
//
///////////////////////////////////////////////////////////////////
// This is INI data
///////////////////////////////////////////////////////////////////
//
//									;a brief note about INI
//		[Settings]
//			Cache=true
//			Errors=/errors/english/errors.txt
//			Rules=/rules/
//
///////////////////////////////////////////////////////////////////
// There are several ways to model this same data in XML 
// This is one way:
///////////////////////////////////////////////////////////////////
//
//		<configuration>
//			<section name="Settings">
//				<setting name="Cache">True</setting>
//				<setting name="Errors">/errors/english/errors.txt</setting>
//				<setting name="Rules">/rules/</setting>
//			</section>
//		<configuration>
//
//
///////////////////////////////////////////////////////////////////
// This is another way. This is the way GProfile does it.<----------------------------------
///////////////////////////////////////////////////////////////////
//
//		<configuration>
//			<section name="Settings">
//				<setting name="Cache" value="True"/>
//				<setting name="Errors" value="/errors/english/errors.txt"/>
//				<setting name="Rules" value="/rules/"/>
//			</section>
//		<configuration>
//
//
// When you consider hand entered values, in a .txt file as the basis of your
// application configuration system - then INI format is by far the best solution
// due to the simplicity designed for hand edit cut and paste.  INI does not 
// require paired quotes around anything.  A frequently hand-edited config file
// is far more likely to experience a parsing error using XML which must also be
// properly escaped &amp; well formed.  A parsing error could == system failure.
//
// BUT - many or MOST applications are NOT HAND EDITED, the config file is generated.
// As you will see in the GProfile implementation, it is easier - and far less lines
// of code to use XML as the data storage format. GProfile calls FromXML() and skips 
// the hand written INI parsing code(mostly handled by GString) ProfileParse() and GetLine()
//
// Not only do many applications generate the file, they encrypt it - so all that
// concern about hand editing is Null and Void if the storage is encrypted and only machine generated.
// In that case, XML provides a more 'open' solution with respect to integration.
// In my own applications, they integrate tightly via GProfile.  One app can
// set a GProfile value that triggers another app to act on the changed setting.
// Since GProfile provides this kind of integration, the additional support of XML is justified.
// 
//
#include "GStringList.h"
#include "GBTree.h"


#include "xmlObject.h"
class GProfileEntry : public XMLObject
{
public:
	GString m_strName;
	GString m_strValue;
	virtual void MapXMLTagsToMembers();
	DECLARE_FACTORY(GProfileEntry, setting)
};


class GProfileSection : public XMLObject
{
public:
	GString		m_strName;
	GList		m_lstNVP;
	virtual void MapXMLTagsToMembers();
	DECLARE_FACTORY(GProfileSection, section)
};


class GProfile : public XMLObject
{
	typedef int(*fnChangeNotify)(const char *,const char *,const char *);
	GBTree	*m_pTreeNotify;
	int ChangeNotify(const char *pzSection, const char *pzEntry, const char *pzValue);
	long SetConfig(const char *szSection, const char *szEntry, const char *nValue, short bSetDefault);
	GStringList lstChangeNotifications;
	bool m_bIsXML; // the storage format is either INI or XML
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

protected:
	GList m_lstSections;

	GProfileSection *FindSection(const char *szSection);
	GProfileEntry *FindKey(const char *szKey, GProfileSection *pSection);

	GString m_strFile;
	short m_bCached;

	void Destroy();
	void GetLine(GString &strLine, const char *szBuffer, __int64 *nIdx, __int64 dwSize);
	void Load();

#ifdef _WIN32
	void ThrowLastError(const char *pzFile);
#endif
	void ProfileParse(const char *szBuffer, __int64 dwSize);
	long WriteCurrentConfigHelper(const char *pzPathAndFileName, GString *pDest, const char *pzSection = 0, bool bWriteXML = 0);

public:
	const char *LastLoadedConfigFile(){return m_strFile;}

	// The section/entry will be created when it does not already exist
	// The existing value will be repalced with the supplied value when it does exist
	long SetConfig(const char *szSection, const char *szEntry, const char *pzValue);
	long SetConfig(const char *szSection, const char *szEntry, int nValue);
	long SetConfig(const char *szSection, const char *szEntry, long lValue);		
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
	long WriteCurrentConfig(const char *pzPathAndFileName, bool bWriteXML = 0);
	long WriteCurrentConfig(GString *pzDestStr, bool bWriteXML = 0);
	// just like WriteCurrentConfig(), but only serialize a single [section]
	long WriteCurrentConfigSection(GString *pzDestStr, const char *pzSection, bool bWriteXML = 0);


	// function retrieves the names of all sections
	void  GetSectionNames(GStringList *lpList);

	// function retrieves all the keys and values for the specified section 
	// returns NULL if specified section doesn't exist, use this code to iterate a section:
	///////////////////////////////////////////////////////////////////////////////////
	//	GListIterator itNVP(GetProfile().GetSection("MySection"));
	//	while (itNVP())
	//	{
	//		GProfileEntry *pNVP = (GProfileEntry *)itNVP++;
	//		// pNVP->m_strName
	//		// pNVP->m_strValue
	//	}
	const GList *GetSection(const char *szSectionName);

	// returns the number of entries for a given section
	int GetSectionEntryCount(const char *szSectionName);

	// RemoveSection() and AddSection() do as their names suggest affecting an entire [Section] with all entries in it
	GProfileSection *RemoveSection(const char *szSection);
	void AddSection(GProfileSection *pS, int bIssueChangeNotification = 1);

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
	// The following methods return a specified data type 
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

	

	// load the profile configuration file yourself, 
	// and create this object "with no disk config file"
	GProfile(const char *szConfigData, __int64 dwSize, bool bIsXML);
	
	// load from file, no environment override allowed
	GProfile(const char *pzFilePathAndName, bool bIsXML);

	// create an empty profile.
	// you will have to add values using OverRideConfig()
	GProfile();
	~GProfile();

	virtual void MapXMLTagsToMembers();
	DECLARE_FACTORY(GProfile, configuration)
};
/*
class GProfileEntryIterator
{
	GList::Node *pDataNode;
	GList *pTList;
public:
	GList::Node *pCurrentNode;    // used to return reference to data

	GProfileEntryIterator(const GList *pList);
	~GProfileEntryIterator() {};
	int operator ! () const;
	const GProfileEntry *operator ++ (int);
};
*/

GProfile &GetProfile();
GProfile *SetProfile(GProfile *pApplicationProfile);


#endif // __PROFILE_H__
