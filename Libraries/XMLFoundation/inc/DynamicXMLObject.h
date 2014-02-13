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

#ifndef _DYN_XML_OBJ_HH__
#define _DYN_XML_OBJ_HH__

#include "xmlObject.h"
#include "GString.h"
#include "GList.h"
#include "GStringList.h"
#include "MemberHandler.h" 
#include "ObjectDataHandler.h"
#include "MemberDescriptor.h"
#include "CacheManager.h"


// for describing native (Java,VB,Perl,Python...) members
class NativeMemberDescriptor
{
public:
	int Duplicates;
	int Source;	  // 1 element, 2 attribute
	int DataType; // an implementation specific datatype enumeration
	GString strName;
	GString strTag;
	GString strWrapper;
	GString strObjectType; // if DataType is 'Object' or type of 'Object Container'
	GString strContainerType; // if DataType is of type 'Object Container'
};


class DynamicXMLObject :  public  XMLObject,
						  public  XMLObjectDataHandler,
						  private MemberHandler
{
	bool m_bIsCacheDisabled;
	DynamicXMLObject *m_pLastIssuedObject;
	GStringList m_lstKeyParts;
	void *m_pUserObject; // handle to VB/Java object
	static int nInstanceCount;
	long m_lAutoSync;
protected:
	GString m_strObjectValue;
	GString m_strThisObjectXMLTag;
	GString m_strThisObjectType;
	GList   *m_plstSubObjects;		// DynamicXMLObject's (data of members or sub-objects)
	GList	m_lstSubUserObjs;		// Active User Object Reference list

	GBTree   m_hshMemberDescriptors; // NativeMemberDescriptor's
public:
	void setAutoSync(long nAutoSync){m_lAutoSync = nAutoSync;}
	long useAutoSync(){return m_lAutoSync;}
	
	void setUserLanguageObject(void *p) {m_pUserObject =p;}
	void *getUserLanguageObject() {return m_pUserObject;}
	void addSubUserLanguageObject(void *p)
	{
		GListIterator Iter(&m_lstSubUserObjs);
		bool bInList = false;
		while(Iter())
		{
			if (p == Iter++)
			{
				bInList = true;
				break;
			}
		}
		if (!bInList)
			m_lstSubUserObjs.AddLast(p);
	}
	GList *getSubUserLanguageObjects()
	{
		return &m_lstSubUserObjs;
	}
	void getSubUserLanguageObjects(DynamicXMLObject *pNewOwner)
	{
		GListIterator Iter(&m_lstSubUserObjs);
		while(Iter())
		{
			pNewOwner->addSubUserLanguageObject(Iter++);
		}
	}

	// free memory storage for all simple members, and objects.
	void ClearPreviousMembers(DynamicXMLObject *pUpdateFrom = 0)
	{
		if (m_plstSubObjects)
		{
			// iterate what we are about to 'detach' storing sub-object refs
			GListIterator Iter(m_plstSubObjects);
			while ( Iter() )
			{
				DynamicXMLObject *pDXO= (DynamicXMLObject*)Iter++;
//				pDXO->getSubUserLanguageObjects(&m_lstSubUserObjs);
				pDXO->getSubUserLanguageObjects(this);

				if (!cacheManager.isForeign(pDXO))
				{
					delete pDXO; // simple member (not an object or collection)
				}
			}
			m_plstSubObjects->RemoveAll();
			RemoveAllAttributes();
			UnMapMembers();
		}

		if (pUpdateFrom)
		{
			delete m_plstSubObjects;
			m_plstSubObjects = pUpdateFrom->m_plstSubObjects;
			pUpdateFrom->m_plstSubObjects = 0;
		}
	}
	GStringList *GetKeyPartsList(){return &m_lstKeyParts;}
	bool IsCacheDisabled(){return m_bIsCacheDisabled;}
	void DisableCache() { m_bIsCacheDisabled = true; }
	void AddMemberDescriptor(NativeMemberDescriptor *p){m_hshMemberDescriptors.insert(p->strName,p);}
	NativeMemberDescriptor *FindMemberDescriptor(const char *pzVarName){return (NativeMemberDescriptor *)m_hshMemberDescriptors.search(pzVarName);}
	GBTree *GetMemberDescriptors(){return &m_hshMemberDescriptors;}
	void FreeMemberDescriptors()
	{
		if (!m_hshMemberDescriptors.isEmpty())
		{
			GBTreeIterator it(&m_hshMemberDescriptors);
			while( it() )
			{
				delete (NativeMemberDescriptor *)it++;
			}
			m_hshMemberDescriptors.clear();
		}
	}
	void SetObjectType(const char *s)	{m_strThisObjectType = s;}
	const char *GetObjectTag()		{return m_strThisObjectXMLTag;}
	const char *GetObjectType()		{return m_strThisObjectType;}
	const char *GetVirtualTag()		{return m_strThisObjectXMLTag;}
	const char *GetVirtualType()	{return m_strThisObjectType;}
protected:
	// called by the object factory for each Element encountered under 'this'
	MemberDescriptor *GetEntry( const char * szTagToFind )
	{
		MemberDescriptor *pMD = ::new MemberDescriptor (this, szTagToFind,(MemberHandler *)this);
		RegisterMember(pMD);
		return pMD;
	}


public:	
	GList *GetSubObjectList(){return m_plstSubObjects;}
	
	DynamicXMLObject* NewSubObject( const char *pzTag )
	{
		DynamicXMLObject *d = new DynamicXMLObject(pzTag);
		if (!m_plstSubObjects)
			m_plstSubObjects = new GList;
		m_plstSubObjects->AddLast(d);
		return d;
	}
	DynamicXMLObject* GetSubObject( const char *pzTag, int nIndex = 0 )
	{
		int nFoundIndex = -1;
		GListIterator Iter(m_plstSubObjects);
		while ( Iter() )
		{
			DynamicXMLObject *d = (DynamicXMLObject*)Iter++;
			if ( d->IsXMLTag(pzTag) )
			{
				nFoundIndex++;
				if (nFoundIndex == nIndex)
					return d;
			}
		}
		return 0;
	}
	~DynamicXMLObject( )
	{
		nInstanceCount--;
//		printf("--DXOInstanceCount=%d [%s%s][%d]\n",nInstanceCount, (const char *)m_strThisObjectXMLTag, (const char *)m_strThisObjectType, this);

		ClearPreviousMembers();

		FreeMemberDescriptors();
		if (m_plstSubObjects)
			delete m_plstSubObjects;
	}
	DynamicXMLObject( const char *pzTag )
	{
		// register the XMLObjectDataHandler Interface
		m_pLastIssuedObject = 0;
		setObjectDataHandler(this);
		m_strThisObjectXMLTag = pzTag;
		m_bIsCacheDisabled = false;
		m_strThisObjectType = "NativeObject";
		m_plstSubObjects = new GList;
		m_pUserObject = 0;
		m_lAutoSync = 0;
		nInstanceCount++;
//		printf("++DXOInstanceCount=%d\n",nInstanceCount);
	}	
	bool IsXMLTag(const char *pzTag)
	{
		int n = m_strThisObjectXMLTag.CompareNoCase(pzTag);
		return (n == 0) ? true : false;
	}
	void FromXML(const char *pzXML)
	{
		XMLObject::FromXML(pzXML);
	}
	
	// XMLObjectDataHandler Interface
	virtual void SetObjectValue(const char *strSource, __int64 nLength, int nType){ m_strObjectValue = strSource;}
	const char * GetObjectValue(){ return m_strObjectValue;}
	virtual void AppendObjectValue(GString& xml){if (m_strObjectValue.Length()) { xml << (const char *)m_strObjectValue;} }


	// Member Handler Interface - called by the XML factory process
	virtual bool IsNull(const char *pzTag) {return false;}
	virtual bool IsCached(const char *pzTag){return true;}
	virtual bool IsMemberObject(const char *pzTag){return true;}
	virtual bool IsMemberDirty(const char *pzTag){return true;}
	virtual bool SetMember(const char *pzTag, const char *pzValue)
	{
		if (m_pLastIssuedObject && m_pLastIssuedObject->IsXMLTag(pzTag))
		{
			m_pLastIssuedObject->SetObjectValue(pzValue, -1, 0);
		}
		else
		{
			SetMemberVar( pzTag, pzValue );
		}
		return true;
	}

	// called directly by the ObjectFactory to get a new Object based on the supplied tag.
	virtual XMLObject *GetObject(const char *pzTag, 
								 const char *pzOID,
								 const char *pzUpdateTime,
								 int *bDidCreate)
	{
		*bDidCreate = 0;

		DynamicXMLObject *d = new DynamicXMLObject(pzTag);
		m_plstSubObjects->AddLast(d);
		m_pLastIssuedObject = d;
		return d;
	}

	void AddSubObject( DynamicXMLObject *d ) {m_plstSubObjects->AddLast(d);}

	void SetMemberVar( const char *pzTag, const char *pzValue, int nUpdate = 1 )
	{
		GListIterator Iter(m_plstSubObjects);
		
		// this searches the list first before adding the new item
		// it may be unnecessary.
		int nFound = 0;
		if (nUpdate)
		{
			while ( Iter() )
			{
				DynamicXMLObject *d = (DynamicXMLObject*)Iter++;
				if (d->IsXMLTag(pzTag))
				{
					nFound = 1;
					d->SetObjectValue(pzValue,-1,0);
					break;
				}
			}
		}
		if (!nFound)
		{
			DynamicXMLObject *d = new DynamicXMLObject(pzTag);
			d->SetObjectValue(pzValue, -1, 0);
			m_plstSubObjects->AddLast(d);
		}
	}


	const char *GetMember( const char *pzTag )
	{
		GListIterator Iter(m_plstSubObjects);
		while ( Iter() )
		{
			DynamicXMLObject *d = (DynamicXMLObject*)Iter++;
			if (d->IsXMLTag(pzTag))
			{
				return d->GetObjectValue();
			}
		}
		return 0;
	}

	virtual bool GetMember( const char *pzTag,
							GString &strAppendValueDestination)
	{
		GListIterator Iter(m_plstSubObjects);
		while ( Iter() )
		{
			DynamicXMLObject *d = (DynamicXMLObject*)Iter++;
			if (d->IsXMLTag(pzTag))
			{
				strAppendValueDestination << d->GetObjectValue();
			}
		}
		return true;
	}


	// custom object streaming for you to handle serialization of 
	// complex objects. This member will only be called if you 
	// returned true from IsMemberObject().
	virtual void StreamObject(	const char *pzTag, 
								GString& xml, 
								int nDebugBeautifyTabs, 
								bool bForceOutput)
	{
		if (m_strObjectValue.Length())
			xml << m_strObjectValue;
		
		GListIterator Iter(m_plstSubObjects);
		while ( Iter() )
		{
			DynamicXMLObject *d = (DynamicXMLObject*)Iter++;
			if (d->IsXMLTag(pzTag))
			{
				d->ToXML(&xml,nDebugBeautifyTabs);
				break;
			}
		}
	}

	const char *getXML()
	{
		static GString xml;
		xml = "";
		theXML(xml, 0);
		return xml;
	}

	void theXML(GString &xml, int nTabs)
	{
		MemberDescriptor::TabifyXML(xml,nTabs);

		xml << "<" << m_strThisObjectXMLTag;

		if (m_pAttributes)
		{
			GListIterator Iter(&m_pAttributes->m_attrList);
			while ( Iter() )
			{
				XMLAttributeList::XMLAttributeNameValuePair *temp =
					(XMLAttributeList::XMLAttributeNameValuePair *)Iter++;
				xml << " " << temp->strAttrName << "=\"" << temp->strAttrValue << "\"";	
			}
		}
		xml << '>';
		xml << m_strObjectValue;

		GListIterator Iter(m_plstSubObjects);
		while ( Iter() )
		{
			DynamicXMLObject *d = (DynamicXMLObject*)Iter++;
			d->theXML(xml,nTabs+1);
		}
		if (m_plstSubObjects && m_plstSubObjects->Size())
			MemberDescriptor::TabifyXML(xml,nTabs);
		xml << "</" << m_strThisObjectXMLTag << '>';
	}

	// custom attribute management
	virtual bool SetAttribute(const char *pzAttrib, const char *pzVal)
	{
		return true;	
	}
};

#endif //_DYN_XML_OBJ_HH__