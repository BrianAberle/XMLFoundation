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
#include "GlobalInclude.h"
static char SOURCE_FILE[] = __FILE__;

#include "MemberDescriptor.h"
#include "xmlDefines.h"
#include "xmlObject.h"
#include "FrameworkAuditLog.h"
#include "StackFrameCheck.h"
#include "ListAbstraction.h"
#include "StringAbstraction.h"
#include "MemberHandler.h"
#include "ObjectDataHandler.h"
#include "GException.h"
#include "GArray.h"

#include <stdio.h>  // for: sprintf(), sscanf()
#include <ctype.h> // for: isdigit()
#include <stdlib.h> // for __atoi64 in Windows
#include <string.h>			// for memcpy() (needed in Linux)


class TranslationLists
{
public:
	GStringList List0;
	GStringList List1;
	GStringList inRaw;
	GArray inTranslated;
	int nTranslationFlags;

	GStringList outRaw;
	GArray outTranslated;

	TranslationLists(const char *pCommaSeparated0List, const char *pCommaSeparated1List, MemberDescriptor::DataTypes DataType, int nFlags) : 
	  List0(",",pCommaSeparated0List) , List1(",",pCommaSeparated1List), inTranslated(32,32), outTranslated(32,32)
	  {
		
		// do NOT do any of this for MemberDescriptor::Bit
		if (DataType == MemberDescriptor::UserString			|| 
			DataType == MemberDescriptor::UserStringCollection	||  
			DataType == MemberDescriptor::UserIntegerArray		||  
			DataType == MemberDescriptor::Str					||
			DataType == MemberDescriptor::Int)	
		{
			nTranslationFlags = nFlags;
			GStringIterator it0(&List0);
			while(it0())
			{
				GString *pG = it0.Next();
				// every string must have an '=' in it (or the entry is ignored)
				int eqIndex = pG->Find('=');
				if (eqIndex > -1)
				{
					// left of the = in the first list, right of the = in the array
					inRaw.AddLastUserConstructed(new GString(pG->Buf(),eqIndex));
					inTranslated.AddElement(new GString(pG->StartingAt(eqIndex+1)));
				}
			}
			
			GStringIterator it1(&List1);
			while(it1())
			{
				GString *pG = it1.Next();
				// every string must have an '=' in it (or the entry is ignored)
				int eqIndex = pG->Find('=');
				if (eqIndex > -1)
				{
					// left of the = in the first list, right of the = in the 2nd list
					outRaw.AddLastUserConstructed(new GString(pG->Buf(),eqIndex));
					outTranslated.AddElement(new GString(pG->StartingAt(eqIndex+1)));

				}
			}
		}
	  };

};


MemberDescriptor::~MemberDescriptor()
{
	if (IsSubObject() && DataType != Root)
		g_ActiveObjectMap.RemoveActiveObjectMap(this);

	if (m_pAttributes)
		delete m_pAttributes;

	if (m_MoreData.pTranslationLists)
		delete m_MoreData.pTranslationLists;
};

void MemberDescriptor::SetAsTempObject(XMLObject *pO)
{
	pO->ModifyObjectBehavior(IS_TEMP_OBJECT);
}





#define WriteMemberBeginTag(xml,nTabs,nFlags)											\
{																						\
	if ( (  nFlags & NO_WHITESPACE ) == 0)												\
	{																					\
		TabifyXML(xml,nTabs);															\
	}																					\
	xml << '<' << strTagName;															\
	if (m_pAttributes)																	\
	{																					\
		GListIterator Iter(&m_pAttributes->m_attrList);									\
		while ( Iter() )																\
		{																				\
			XMLAttributeList::XMLAttributeNameValuePair *temp =							\
				(XMLAttributeList::XMLAttributeNameValuePair *)Iter++;					\
			xml << " ";																	\
			xml << temp->strAttrName;													\
			xml << '=' << '"';															\
			xml	<< temp->strAttrValue;													\
			xml << '"';																	\
		}																				\
	}																					\
	xml << '>';																			\
}


#define WriteMemberEndTag(xml)			xml << "</" << strTagName << '>';

#define WriteMemberEndTagShort(xml)		xml.Insert(xml.Length()-1,"/");


const char *MemberDescriptor::TranslateXMLValueToMemberValue(const char *pzData)
{
	// look for an exact match, unless the XLATE_CASE_INSENSITIVE_IN bit is set
	__int64 nFindIndex = m_MoreData.pTranslationLists->inRaw.Find(pzData,  !(m_MoreData.pTranslationLists->nTranslationFlags & XLATE_CASE_INSENSITIVE_IN) );
	if (nFindIndex != -1)
	{
		// return it if we found one
		// no stack call is made for a lookup. This is a direct memory address calculated at compile time.  We do NOT make a method call to get the data.
		// Its a FAST indexed lookup.
		return ((GString *)(m_MoreData.pTranslationLists->inTranslated.m_arrObjects[nFindIndex].m_pValue))->_str;
	}
	else // look for wildcards
	{
		__int64 nSearchStart = 0;

		// the loop walks through multiple wildcard values if the pzTranslationMap contains > than 1 '*' in the string
		while( !(m_MoreData.pTranslationLists->nTranslationFlags & XLATE_NO_IN_WILDCARDS) ) 
		{
			GString *p = m_MoreData.pTranslationLists->inRaw.FindStringContaining('*',&nFindIndex, nSearchStart);
			if (nFindIndex != -1)
			{
				GString strTemp(p->Buf(),p->Find('*'));
				if (strTemp.CompareNoCase(pzData,strTemp.Length()) == 0)
				{
					return ((GString *)(m_MoreData.pTranslationLists->inTranslated.m_arrObjects[nFindIndex].m_pValue))->_str;
				}
				nSearchStart = nFindIndex + 1;
			}
			else
			{
				if (nSearchStart == 0)
				{
					// we know that this translation list does not contain any wildcard searches - so that information is cached and we never need to check again
					m_MoreData.pTranslationLists->nTranslationFlags |= XLATE_NO_IN_WILDCARDS;
				}
				break;
			}
		}
	}
	nFindIndex = m_MoreData.pTranslationLists->inRaw.Find("XML_DEFAULT",1);
	if (nFindIndex != -1)
	{
		return ((GString *)(m_MoreData.pTranslationLists->inTranslated.m_arrObjects[nFindIndex].m_pValue))->_str;
	}
	return pzData; // no translation
}

const char *MemberDescriptor::TranslateMemberValueToXMLValue(__int64 nIntegers, GString *strDest)
{
	*strDest << nIntegers;
	return TranslateMemberValueToXMLValue(strDest->Buf());

}

const char *MemberDescriptor::TranslateMemberValueToXMLValue(const char *src)
{
	__int64 nFindIndex = m_MoreData.pTranslationLists->outRaw.Find(src, !(m_MoreData.pTranslationLists->nTranslationFlags & XLATE_CASE_INSENSITIVE_OUT) );
	if (nFindIndex != -1)
	{
		return ((GString *)(m_MoreData.pTranslationLists->outTranslated.m_arrObjects[nFindIndex].m_pValue))->_str;
	}
	else // look for wildcards
	{
		__int64 nSearchStart = 0;
		while(1)
		{
			GString *p = m_MoreData.pTranslationLists->outRaw.FindStringContaining('*',&nFindIndex, nSearchStart);
			if (nFindIndex != -1)
			{
				GString strTemp(p->Buf(),p->Find('*'));
				if (strTemp.CompareNoCase(src,strTemp.Length()) == 0)
				{
					return ((GString *)(m_MoreData.pTranslationLists->outTranslated.m_arrObjects[nFindIndex].m_pValue))->_str;
				}
				nSearchStart = nFindIndex + 1;
			}
			else
			{
				break;
			}
		}
	}
	nFindIndex = m_MoreData.pTranslationLists->outRaw.Find("XML_DEFAULT",0);
	if (nFindIndex != -1)
	{
		return ((GString *)(m_MoreData.pTranslationLists->outTranslated.m_arrObjects[nFindIndex].m_pValue))->_str;

	}
	return src; // no translation
}

void MemberDescriptor::MemberToXML(GString& xml, int nTabs, StackFrameCheck *pStack,int nSerializeFlags)
{
	if ( DataType == ManagedByDerivedClass )																
	{																										
		bool bDirty = ( nSerializeFlags & INCLUDE_ALL_CACHED_MEMBERS ) != 0;									
		if (!bDirty)																						
		{																									
			bDirty = m_DataAbstractor.pMemberHandler->IsMemberDirty((const char *)strTagName);				
		}																									
		if (bDirty)																							
		{																									
			if (m_DataAbstractor.pMemberHandler->IsMemberObject((const char *)strTagName))					
			{																								
				// We must delegate to the derived class since we don't know if
				// the object is a collection, or if there is any special processing
				m_DataAbstractor.pMemberHandler->StreamObject((const char *)strTagName,xml,nTabs,!pStack->isNestedStackFrame()); 
			}																								
			else																							
			{
				// add the value of the member variable
				WriteMemberBeginTag(xml,nTabs,nSerializeFlags);
				m_DataAbstractor.pMemberHandler->GetMember( (const char *)strTagName, xml );
				WriteMemberEndTag(xml);
			}																								
		}																									
	}																										
	else if ( IsDirty() || DataType == XMLObj || DataType == XMLObjList || DataType == XMLObjPtr			
						|| ((( nSerializeFlags & INCLUDE_ALL_CACHED_MEMBERS ) != 0) && IsCached() ) ||			
						((nSerializeFlags & FULL_SERIALIZE) != 0)										
			)																								
	{																										
		if (!IsSerializable())																				
		{																									
			return;																							
		}																									
																											
		if(DataType == EntityReference)																		
		{																									
			TabifyXML(xml,nTabs);																			
			xml << (const char *)strTagName;																
		}																									
		else if(DataType == UserString)																		
		{
			if (   (nSerializeFlags & NO_EMPTY_STRINGS) != 0  && m_DataAbstractor.pStringHandler->isEmpty(m_Member.pUserString) && !IsDirty() )
			{
				// if (NO empty strings) AND (string is empty)  AND (value of ""(empty) is not an assigned/changed/new value)
				// then skip the serialization of this member
			}
			else
			{
				WriteMemberBeginTag( xml,nTabs,nSerializeFlags );
				char const *src = m_DataAbstractor.pStringHandler->data(m_Member.pUserString);

				// if there is no TranslationList we avoid the stack call to TranslateMemberValueToXMLValue().
				// WriteMemberBeginTag() and WriteMemberEndTag() are MACROS - to avoid stack calls - thats why this is so FAST.
				if (m_MoreData.pTranslationLists) 
				{
					src = TranslateMemberValueToXMLValue(src);
				}

				xml.AppendEscapeXMLReserved(src,-1,nSerializeFlags);

				if ( ( ((nSerializeFlags & EXCLUDE_SHORT_TERMINATION) != 0) && !src[0]) || src[0])
				{
					WriteMemberEndTag(xml);
				}
				else
				{
					WriteMemberEndTagShort(xml);
				}

			}
		}																									
		else if(DataType == Int)																			
		{																									
			WriteMemberBeginTag(xml,nTabs,nSerializeFlags);
			
			// if there is no TranslationList we avoid the stack call to TranslateMemberValueToXMLValue().
			// WriteMemberBeginTag() and WriteMemberEndTag() are MACROS - to avoid stack calls - thats why this is so FAST.
			if (m_MoreData.pTranslationLists) 
			{
				GString strDest;
				xml.AppendEscapeXMLReserved(TranslateMemberValueToXMLValue(*m_Member.pInt,&strDest),-1,nSerializeFlags);
			}
			else
			{
				xml << *m_Member.pInt;
			}
			//WriteMemberEndTag(xml);
			xml << "</" << strTagName << '>';
		}
		else if (DataType == Str)																			
		{																									
			if (   (nSerializeFlags & NO_EMPTY_STRINGS) != 0  && !m_Member.pNativeStr[0] && !IsDirty() )
			{
				// if (NO empty strings) AND (string is empty)  AND (value of ""(empty) is not an assigned/changed/new value)
				// then skip the serialization of this member
			}
			else
			{
				WriteMemberBeginTag(xml,nTabs,nSerializeFlags);

				const char *pzOutputValue = m_Member.pNativeStr;
				// if there is no TranslationList we avoid the stack call to TranslateMemberValueToXMLValue().
				// WriteMemberBeginTag() and WriteMemberEndTag() are MACROS - to avoid stack calls - thats why this is so FAST.
				if (m_MoreData.pTranslationLists) 
				{
					pzOutputValue = TranslateMemberValueToXMLValue(m_Member.pNativeStr);
				}
				xml.AppendEscapeXMLReserved(pzOutputValue,-1,nSerializeFlags);
				if ( ( ((nSerializeFlags & EXCLUDE_SHORT_TERMINATION) != 0) && !pzOutputValue[0]) || pzOutputValue[0])
				{
					WriteMemberEndTag(xml);
				}
				else
				{
					WriteMemberEndTagShort(xml);
				}
			}

		}																									
		else if (DataType == Bool)																			
		{																									
			WriteMemberBeginTag(xml,nTabs,nSerializeFlags);
			if (*m_Member.pBool)
			{
				if (m_DataAbstractor.nBoolReadability == 0)
				{
					xml << "Yes";
				}
				else if (m_DataAbstractor.nBoolReadability == 1)
				{
					xml << "True";
				}
				else if (m_DataAbstractor.nBoolReadability == 2)
				{
					xml << "On";
				}
				else // m_DataAbstractor.nBoolReadability == 3
				{
					xml << "1";
				}
			}
			else
			{
				if (m_DataAbstractor.nBoolReadability == 0)
				{
					xml << "No";
				}
				else if (m_DataAbstractor.nBoolReadability == 1)
				{
					xml << "False";
				}
				else if (m_DataAbstractor.nBoolReadability == 2)
				{
					xml << "Off";
				}
				else // m_DataAbstractor.nBoolReadability == 3
				{
					xml << "0";
				}
			}
			WriteMemberEndTag(xml);
		}																									
		else if(DataType == Bit)
		{
			WriteMemberBeginTag(xml,nTabs,nSerializeFlags);
			
			if ( (  *m_Member.pBits & m_DataAbstractor.nBitMask ) == 0)
			{
				xml << m_MoreData.pTranslationLists->List0.PeekFirst();
			}
			else
			{
				xml << m_MoreData.pTranslationLists->List1.PeekFirst();
			}

			WriteMemberEndTag(xml);
		}
		else if(DataType == Byte)
		{																									
			WriteMemberBeginTag(xml,nTabs,nSerializeFlags);
			xml.AppendEscapeXMLReserved(m_Member.pByte, 1, nSerializeFlags);// note: 1 byte NOT null terminated
			WriteMemberEndTag(xml);
		}
		else if(DataType == Short)																			
		{																									
			WriteMemberBeginTag(xml,nTabs,nSerializeFlags);
			if (m_MoreData.pTranslationLists) 
			{
				GString strDest;
				xml.AppendEscapeXMLReserved(TranslateMemberValueToXMLValue(*m_Member.pShort,&strDest),-1,nSerializeFlags);
			}
			else
			{
				xml << *m_Member.pShort;
			}
			WriteMemberEndTag(xml);
		}
		else if(DataType == Long)
		{																									
			WriteMemberBeginTag(xml,nTabs,nSerializeFlags);

			if (m_MoreData.pTranslationLists) 
			{
				GString strDest;
				xml.AppendEscapeXMLReserved(TranslateMemberValueToXMLValue(*m_Member.pLong,&strDest),-1,nSerializeFlags);
			}
			else
			{
				xml << *m_Member.pLong;
			}

			WriteMemberEndTag(xml);
		}
		else if(DataType == Int64)																			
		{																									
			WriteMemberBeginTag(xml,nTabs,nSerializeFlags);

			if (m_MoreData.pTranslationLists) 
			{
				GString strDest;
				xml.AppendEscapeXMLReserved(TranslateMemberValueToXMLValue(*m_Member.pInt64,&strDest),-1,nSerializeFlags);
			}
			else
			{
				xml << *m_Member.pInt64;
			}

			WriteMemberEndTag(xml);
		}																									
		else if(DataType == Double)
		{																									
			WriteMemberBeginTag(xml,nTabs,nSerializeFlags);
			// For a 64-bit IEEE double - max chars is 1079
			char  szBuffer[1080];
			sprintf(szBuffer, "%lf", *m_Member.pDouble);
			xml << szBuffer;
			WriteMemberEndTag(xml);
		}																									
		else if(DataType == UserStringCollection)															
		{
			if (   (nSerializeFlags & NO_EMPTY_STRINGS_IN_STRINGLIST) != 0  && m_DataAbstractor.pStringHandler->isEmpty(m_Member.pUserString) && !IsDirty() )
			{
				// if (NO empty strings) AND (string is empty)  AND (value of ""(empty) is not an assigned/changed/new value)
				// then skip the serialization of this member
			}
			else
			{
				userStringListIterator iter;																	
				const char *pzEntry = m_DataAbstractor.pStrListHandler->getFirst(m_Member.pUserStringCollection,&iter);	
				while (pzEntry)																					
				{																								
					WriteMemberBeginTag(xml,nTabs,nSerializeFlags);
					
					if (m_MoreData.pTranslationLists) 
					{
						pzEntry = TranslateMemberValueToXMLValue(pzEntry);
					}
					
					xml.AppendEscapeXMLReserved(pzEntry,-1,nSerializeFlags); 

					if ( (((nSerializeFlags & EXCLUDE_SHORT_TERMINATION) != 0) && !pzEntry[0]) || pzEntry[0])
					{
						WriteMemberEndTag(xml);
					}
					else
					{
						WriteMemberEndTagShort(xml);
					}
					pzEntry = m_DataAbstractor.pStrListHandler->getNext(m_Member.pUserStringCollection,iter);	
				}
			}
		}																									
		else if (DataType == UserIntegerArray)																
		{																									
			int nIndex = 0;																					
			int nIsValidIndex = 0;																			
			int nValue = m_DataAbstractor.pIntArrayHandler->getAt(m_Member.pUserIntegerArray,nIndex,&nIsValidIndex);	
			while(nIsValidIndex)																			
			{																								
				WriteMemberBeginTag(xml,nTabs,nSerializeFlags);

				if (m_MoreData.pTranslationLists) 
				{
					GString strDest;
					xml.AppendEscapeXMLReserved(TranslateMemberValueToXMLValue(nValue,&strDest),-1,nSerializeFlags);
				}
				else
				{
					xml << nValue;
				}

				WriteMemberEndTag(xml);
				nIndex++;																					
				nValue = m_DataAbstractor.pIntArrayHandler->getAt(m_Member.pUserIntegerArray,nIndex,&nIsValidIndex);
			}																								
		}																									
		else if( (	( DataType == XMLObjPtr && *(m_Member.ppObject) ) ||									
					( DataType == XMLObj ) ||																
					( DataType == XMLObjList ) ||															
					( DataType == KeyedDataStructure )														
				 )	&& !pStack->isNestedStackFrame()														
			   )																							
		{																									
			// nObjectCount is always 1 if the DataType == XMLObj or ObjPointer
			// it may be 0 or more if the DataType == XMLObjList
			xmlObjectIterator iter = 0;
			XMLObject *pObj = GetFirstContainedObject(&iter);												
			while (pObj)																					
			{																								
				// only check if we know we want to recursivly serialize
				if ( (nSerializeFlags & FULL_SERIALIZE)
					// don't bother walking the tree if we already know
					// we must include this member in the XMLstream
					&& !( pObj->getMemberStateSummary(DATA_DIRTY) ||										
					(( nSerializeFlags & INCLUDE_ALL_CACHED_MEMBERS ) && pObj->getMemberStateSummary(DATA_CACHED) )) )	
				{																							
					// If the object is not dirty already recurse it's children
					m_memberStateFlags |= pObj->CalculateObjectState();
				}
				
				if ( pObj->getMemberStateSummary(DATA_DIRTY) ||
					((( nSerializeFlags & INCLUDE_ALL_CACHED_MEMBERS ) != 0) && pObj->getMemberStateSummary(DATA_CACHED) ) || 
					((nSerializeFlags & FULL_SERIALIZE) != 0) ||
					((nSerializeFlags & RECURSE_OBJECTS_DEEP) != 0)
				   )
				{																							
					if ( (  nSerializeFlags & USE_OBJECT_MARKERS ) != 0)
					{																						
						// If the object has no OID, we cannot treat it as a real object.
						// It is merely a structure embedded in an object.  Since the
						// structure (a user defined data type, not an object) is contained	
						// in an object we treat this as any other member by deeply
						// recursing it's value into the XML stream.  This case also applies
						// to RelationshipWrappers that are really structs containing
						// pointers to objects.
						if (!pObj->getOID())
						{																					
							pObj->ToXML( &xml, nTabs , (const char *)strTagName, nSerializeFlags, pStack );
						}																					
						else																				
						{																					
							// If we are not deeply recursing nested objects write
							// out just an "Object Marker" enough to re-associate
							// the objects later.
							pObj->WriteObjectBeginTag( xml, nTabs, 0, nSerializeFlags );
							pObj->WriteObjectEndTag( xml, nTabs, 0, nSerializeFlags );
						}																					
					}																						
					else if ( DataType == XMLObjList )														
					{																						
						pObj->ToXML( &xml, nTabs, 0, nSerializeFlags, pStack );								
					}																						
					else																					
					{																						
						// write full object and all contained data
						pObj->ToXML( &xml, nTabs , (const char *)strTagName, nSerializeFlags, pStack );
					}																						
				}																							
				pObj = GetNextContainedObject(iter);														
			}																								
		}																									
	}
}


const char *MemberDescriptor::GetTypeName(GString &strTypeDescription)
{
	if ( DataType == ManagedByDerivedClass )
	{
		strTypeDescription = "ManagedByDerivedClass";
	}
	if(DataType == UserString)
	{
		strTypeDescription = "string";
	}
	else if (DataType == Str)
	{
		strTypeDescription << "char [" << m_DataAbstractor.nBufferMaxSize << "]";;
	}
	else if (DataType == Bool)
	{
		strTypeDescription = "bool";
	}
	else if(DataType == Bit)
	{
		strTypeDescription = "bit";
	}
	else if(DataType == Byte)
	{
		strTypeDescription = "char";
	}
	else if(DataType == Short)
	{
		strTypeDescription = "short";
	}
	else if(DataType == Int)
	{
		strTypeDescription = "int";
	}
	else if(DataType == Long)
	{
		strTypeDescription = "long";
	}
	else if(DataType == XMLObj)
	{
		strTypeDescription = "XMLObject";
	}
	else if(DataType == XMLObjPtr)
	{
		strTypeDescription = "XMLObject *";
	}
	else if(DataType == XMLObjList)
	{
		strTypeDescription = "List<XMLObject *>";
	}
	else if(DataType == KeyedDataStructure )
	{
		// could be binary tree, hash table, sorted array, etc..
		strTypeDescription = "KeyedDataStructure";
	}
	else
	{
		strTypeDescription = "Unknown";
	}

	return strTypeDescription;
}


void MemberDescriptor::SetDirty( int bDirty /* = 1*/ )
{
	if (bDirty)
	{
		m_memberStateFlags |= DATA_DIRTY;
		if (m_ParentObject)
			m_ParentObject->setMemberStateSummary(DATA_DIRTY);
	}
	else
	{
		m_memberStateFlags ^= DATA_DIRTY;
	}
}


//*********************************************************
// Description:
//		When the XML Parser sets the data, it is setting the initial value
//		so it is not considered dirty, however when an application developer
//		sets the state of a member variable, it is now dirty.
// 
//*********************************************************
void MemberDescriptor::Set(const char * pzData, __int64 nDataLen, unsigned int nFlags)
{
	if ( !( nFlags & FACTORY_FLAG_SET_DIRTY ) && IsDirty() )
	{
	// When bSetDirty = false, this means the member is being set by the 
	// DataSource.  The state becomes CACHED, and NOT_NULL when the DataSource
	// sets the value of a member.  However if the member is already DIRTY
	// when the DataSource attempts to set the value this means the Application has
	// explicitly defined the value for this member.  Possibly we intentionally
	// want the value to differ from that of the DataSource (in preperation
	// for a database update) or the Object has state from an alternate source
	// with higher precedence.
	if (m_ParentObject)
		m_ParentObject->ObjectMessage(MSG_XML_ASSIGN, strTagName, pzData, nDataLen, 0);
		return;	
	}

	// if we didnt know the pzData length - then find out what it is.
	if (nDataLen < 0)
		nDataLen = strlen(pzData);

//  This was removed because when a member is set by the XML parser it is Cached
//  then when a new stream  of data comes in we need to pick up the changes by default
//	if ( !( nFlags & FACTORY_FLAG_UPDATE_CLEAN_MEMBERS ) && IsCached() )
//	{
//		// If we're not FACTORY_FLAG_UPDATE_CLEAN_MEMBERS (because the TimeStamp matched
//		// a previously cached instance of this object) and we already have 
//		// the data there is nothing to do.
//		return;
//	}

	static GString strYes("yes");
	static GString strTrue("true");
	static GString strOn("on");

	switch(DataType)
	{
	case ManagedByDerivedClass:
			m_DataAbstractor.pMemberHandler->SetMember((const char *)strTagName,pzData);
		break;
	case Bool:
			if (strYes.CompareNoCase(pzData) == 0 || strTrue.CompareNoCase(pzData) == 0 || strOn.CompareNoCase(pzData) == 0)
			{
				*m_Member.pBool = true; // "True", "Yes", or "On" mapped to a bool is true
			}
			else
			{
				*m_Member.pBool = false;
			}
			if (m_ParentObject && nDataLen > 1)
				m_ParentObject->ObjectMessage(MSG_DATA_TRUNCATE, strTagName, pzData, nDataLen, 0);
		break;
	case Bit:
			if (m_MoreData.pTranslationLists->List1.Find(pzData,0) != -1) // search list of 1 values - case insensitive
			{
				*m_Member.pBits |= m_DataAbstractor.nBitMask; // turn bit on
			}
			else if (m_MoreData.pTranslationLists->List0.Find(pzData,0) != -1) // search list of 0 values - case insensitive
			{
				*m_Member.pBits &= ~m_DataAbstractor.nBitMask; // turn bit off
			}
			else
			{
				if (m_ParentObject && nDataLen > 1)
					m_ParentObject->ObjectMessage(MSG_UNEXPECTED_VALUE, strTagName, pzData, nDataLen, 0);
			}
		break;
	case Byte:
			*m_Member.pByte = pzData[0];
			if (m_ParentObject && nDataLen > 1)
				m_ParentObject->ObjectMessage(MSG_DATA_TRUNCATE, strTagName, pzData, nDataLen, 0);
		break;
	case Short:
		if (m_MoreData.pTranslationLists)
		{ 
			if (!pzData || pzData[0] == 0) // XML_EMPTY_VALUE
			{
				int nFindIndex = m_MoreData.pTranslationLists->inRaw.Find("XML_EMPTY_VALUE",1);
				if (nFindIndex != -1)
				{
					pzData = ((GString *)(m_MoreData.pTranslationLists->inTranslated.m_arrObjects[nFindIndex].m_pValue))->_str;

					int i;
					sscanf (pzData, "%d",  &i); //%d writes on 4 bytes, so temp i is used. I dont know the %? for a short
					*m_Member.pShort = (short)i; // the unwanted high order bytes are lost in the conversion
					break;
				}
			}
			else
			{
				pzData = TranslateXMLValueToMemberValue(pzData);
			}
		}
		if (!isdigit(pzData[0]) && pzData[0] != '-') // look for a number, or a negative number
		{
			*m_Member.pShort = 0;  // any other non-numeric defaults to 0
			if (m_ParentObject)
				m_ParentObject->ObjectMessage(MSG_NON_NUMERIC, strTagName, pzData, nDataLen, 0);
		}
		else
		{
			int i;
			sscanf (pzData, "%d",  &i); //%d writes on 4 bytes, so temp i is used. I dont know the %? for a short
			*m_Member.pShort = (short)i; // the unwanted high order bytes are lost in the conversion
		}
		break;
	case Int:
		if (m_MoreData.pTranslationLists)
		{ 
			if (!pzData || pzData[0] == 0) // XML_EMPTY_VALUE
			{
				int nFindIndex = m_MoreData.pTranslationLists->inRaw.Find("XML_EMPTY_VALUE",1);
				if (nFindIndex != -1)
				{
					pzData = ((GString *)(m_MoreData.pTranslationLists->inTranslated.m_arrObjects[nFindIndex].m_pValue))->_str;

					*m_Member.pInt = atoi(pzData);
					break;
				}
			}
			else
			{
				pzData = TranslateXMLValueToMemberValue(pzData);
			}
		}
		if (!isdigit(pzData[0]) && pzData[0] != '-') // the [&& pzData[0] != '-'] is added for negative number support
		{
			*m_Member.pInt = 0;  // any other non-numeric defaults to 0
			if (m_ParentObject)
				m_ParentObject->ObjectMessage(MSG_NON_NUMERIC, strTagName, pzData, nDataLen, 0);
		}
		else
		{
			*m_Member.pInt = atoi(pzData); // faster than the previous code:  sscanf (pzData, "%d",  m_Member.pInt); 
		}
		break;
	case Long:
		if (m_MoreData.pTranslationLists)
		{ 
			if (!pzData || pzData[0] == 0) // XML_EMPTY_VALUE
			{
				int nFindIndex = m_MoreData.pTranslationLists->inRaw.Find("XML_EMPTY_VALUE",1);
				if (nFindIndex != -1)
				{
					pzData = ((GString *)(m_MoreData.pTranslationLists->inTranslated.m_arrObjects[nFindIndex].m_pValue))->_str;
					sscanf (pzData, "%ld",  m_Member.pLong); 
					break;
				}
			}
			else
			{
				pzData = TranslateXMLValueToMemberValue(pzData);
			}
		}
		if (!isdigit(pzData[0]) && pzData[0] != '-' )
		{
			*m_Member.pLong = 0;  // any other non-numeric defaults to 0
			if (m_ParentObject)
				m_ParentObject->ObjectMessage(MSG_NON_NUMERIC, strTagName, pzData, nDataLen, 0);
		}
		else
		{
			sscanf (pzData, "%ld",  m_Member.pLong); 
		}
		break;
	case Int64 :
		if (m_MoreData.pTranslationLists)
		{ 
			if (!pzData || pzData[0] == 0) // XML_EMPTY_VALUE
			{
				int nFindIndex = m_MoreData.pTranslationLists->inRaw.Find("XML_EMPTY_VALUE",1);
				if (nFindIndex != -1)
				{
					pzData = ((GString *)(m_MoreData.pTranslationLists->inTranslated.m_arrObjects[nFindIndex].m_pValue))->_str;
					*m_Member.pInt64 = Xatoi64( pzData );
					break;
				}
			}
			else
			{
				pzData = TranslateXMLValueToMemberValue(pzData);
			}
		}
		if (!isdigit(pzData[0]) && pzData[0] != '-' )
		{
			*m_Member.pInt64 = 0;  // any other non-numeric defaults to 0
			if (m_ParentObject)
				m_ParentObject->ObjectMessage(MSG_NON_NUMERIC, strTagName, pzData, nDataLen, 0);
		}
		else
		{
			*m_Member.pInt64 = Xatoi64( pzData );
		}
		break;
	case Double :
		if (m_MoreData.pTranslationLists)
		{ 
			if (!pzData || pzData[0] == 0) // XML_EMPTY_VALUE
			{
				int nFindIndex = m_MoreData.pTranslationLists->inRaw.Find("XML_EMPTY_VALUE",1);
				if (nFindIndex != -1)
				{
					pzData = ((GString *)(m_MoreData.pTranslationLists->inTranslated.m_arrObjects[nFindIndex].m_pValue))->_str;
					*m_Member.pDouble = atof( pzData );
					break;
				}
			}
			else
			{
				pzData = TranslateXMLValueToMemberValue(pzData);
			}
		}

		if (!isdigit(pzData[0]) && pzData[0] != '-' )
		{
			*m_Member.pDouble = 0.0;  // any other non-numeric defaults to 0
			if (m_ParentObject)
				m_ParentObject->ObjectMessage(MSG_NON_NUMERIC, strTagName, pzData, nDataLen, 0);
		}
		else
		{			
			*m_Member.pDouble = atof( pzData );
		}
		break;
	case UserStringCollection:
		if ( pzData && pzData[0] )
		{
			if (!m_MoreData.pTranslationLists)
			{
				m_DataAbstractor.pStrListHandler->append(m_Member.pUserStringCollection,pzData);
			}
			else 
			{
				m_DataAbstractor.pStrListHandler->append(m_Member.pUserStringCollection,TranslateXMLValueToMemberValue(pzData));
			}
		}
		else
		{
			if (m_MoreData.pTranslationLists) 
			{
				int nFindIndex = m_MoreData.pTranslationLists->inRaw.Find("XML_EMPTY_VALUE",1);
				if (nFindIndex != -1)
				{
					pzData = ((GString *)(m_MoreData.pTranslationLists->inTranslated.m_arrObjects[nFindIndex].m_pValue))->_str;
					m_DataAbstractor.pStrListHandler->append(m_Member.pUserStringCollection,pzData);
				}
			}
			else
			{
				m_DataAbstractor.pStrListHandler->append(m_Member.pUserStringCollection,"");
				if (m_ParentObject)
					m_ParentObject->ObjectMessage(MSG_EMPTY_STRING, strTagName, pzData, nDataLen, 0);
			}
		}
		break;
	case UserIntegerArray:
		if (m_MoreData.pTranslationLists)
		{ 
			if (!pzData || pzData[0] == 0) // XML_EMPTY_VALUE
			{
				int nFindIndex = m_MoreData.pTranslationLists->inRaw.Find("XML_EMPTY_VALUE",1);
				if (nFindIndex != -1)
				{
					pzData = ((GString *)(m_MoreData.pTranslationLists->inTranslated.m_arrObjects[nFindIndex].m_pValue))->_str;
					m_DataAbstractor.pIntArrayHandler->append(m_Member.pUserIntegerArray, atoi(pzData) );
					break;
				}
			}
			else
			{
				pzData = TranslateXMLValueToMemberValue(pzData);
			}
		}
		if (!isdigit(pzData[0]) && pzData[0] != '-') 
		{
			// except for negative numbers - any other non-numeric defaults to 0
			m_DataAbstractor.pIntArrayHandler->append(m_Member.pUserIntegerArray, 0 );
			if (m_ParentObject)
				m_ParentObject->ObjectMessage(MSG_NON_NUMERIC, strTagName, pzData, nDataLen, 0);
		}
		else
		{
			m_DataAbstractor.pIntArrayHandler->append(m_Member.pUserIntegerArray,atoi(pzData));
		}

		break;
	case UserString:
		if ( pzData && pzData[0] )
		{
			if (!m_MoreData.pTranslationLists)
			{
				m_DataAbstractor.pStringHandler->assign(m_Member.pUserString,pzData);
			}
			else 
			{
				m_DataAbstractor.pStringHandler->assign(m_Member.pUserString,TranslateXMLValueToMemberValue(pzData));
			}
		}
		else
		{
			if (m_MoreData.pTranslationLists) 
			{
				int nFindIndex = m_MoreData.pTranslationLists->inRaw.Find("XML_EMPTY_VALUE",1);
				if (nFindIndex != -1)
				{
					pzData = ((GString *)(m_MoreData.pTranslationLists->inTranslated.m_arrObjects[nFindIndex].m_pValue))->_str;
					m_DataAbstractor.pStringHandler->assign(m_Member.pUserString,pzData);
				}
			}
			else
			{
				m_DataAbstractor.pStringHandler->assign(m_Member.pUserString,"");
				if (m_ParentObject)
					m_ParentObject->ObjectMessage(MSG_EMPTY_STRING, strTagName, pzData, nDataLen, 0);
			}
		}
		break;
	case Str:
		if ( pzData && pzData[0] )
		{
			if (!m_MoreData.pTranslationLists)
			{
				if (nDataLen > m_DataAbstractor.nBufferMaxSize)
				{
					memcpy(m_Member.pNativeStr,pzData,m_DataAbstractor.nBufferMaxSize);
					m_Member.pNativeStr[m_DataAbstractor.nBufferMaxSize-1] = 0;
					m_ParentObject->ObjectMessage(MSG_DATA_TRUNCATE, strTagName, pzData, nDataLen, 0);
				}
				else
				{
					memcpy(m_Member.pNativeStr,pzData,nDataLen);
					m_Member.pNativeStr[nDataLen] = 0;
				}
			}
			else
			{
				const char *pzXlated = TranslateXMLValueToMemberValue(pzData);
				int nXlatedLen = strlen(pzXlated);
				if (nXlatedLen > m_DataAbstractor.nBufferMaxSize)
				{
					memcpy(m_Member.pNativeStr,pzXlated,m_DataAbstractor.nBufferMaxSize);
					m_Member.pNativeStr[m_DataAbstractor.nBufferMaxSize-1] = 0;
					m_ParentObject->ObjectMessage(MSG_DATA_TRUNCATE, strTagName, pzXlated, nXlatedLen, (void *)pzData);
				}
				else
				{
					memcpy(m_Member.pNativeStr,pzXlated,nXlatedLen);
					m_Member.pNativeStr[nXlatedLen] = 0;
				}
			}
		}
		else
		{
			if (m_MoreData.pTranslationLists) 
			{
				int nFindIndex = m_MoreData.pTranslationLists->inRaw.Find("XML_EMPTY_VALUE",1);
				if (nFindIndex != -1)
				{
					pzData = ((GString *)(m_MoreData.pTranslationLists->inTranslated.m_arrObjects[nFindIndex].m_pValue))->_str;
					m_DataAbstractor.pStringHandler->assign(m_Member.pUserString,pzData);
				}
			}
			else
			{
				m_DataAbstractor.pStringHandler->assign(m_Member.pUserString,"");
				if (m_ParentObject)
					m_ParentObject->ObjectMessage(MSG_EMPTY_STRING, strTagName, pzData, nDataLen, 0);
			}
		}
		break;
	case XMLObjList:
		{
			XMLObject *pX = m_DataAbstractor.pListHandler->getLast( m_Member.pXMLObjList );
			if (pX)
			{
				if ( (pX->m_pDataHandler) && (*pzData) )
				{
					pX->m_pDataHandler->SetObjectValue(pzData,nDataLen,3);
				}
			}
		}
		break;

	case XMLObj:
		if ( (m_Member.pObject)->m_pDataHandler )
		{
			(m_Member.pObject)->m_pDataHandler->SetObjectValue(pzData,nDataLen,3);
		}
		break;
	}
	
	// there is a m_ParentObject for all but 'Root'
	if (m_ParentObject)
	{
		if( nFlags & FACTORY_FLAG_SET_DIRTY )
		{
			// set by the user of the business object 
			m_memberStateFlags |= DATA_DIRTY;
			m_ParentObject->setObjectDirty();
		}
		else
		{
			// set by the Object Factory from a DataSource update
			m_memberStateFlags |= DATA_CACHED;
			m_ParentObject->setMemberStateSummary(DATA_CACHED);
		}
	}

	// If the data is ever assigned a value then it becomes (not null)
	// This allows access via a Get() method, to throw an exception
	// when uninitialized data is accessed.
	m_memberStateFlags |= DATA_NOT_NULL;
	m_memberStateFlags &= ~DATA_NULL;

}


void MemberDescriptor::Set(const char * pzData, __int64 nDataLen, 
								bool bSetDirty, // bSetDirty = 0 when set by the XMLObjectFactory, 1 when set by application
								bool bUpdateCleanMembers)
{
	
	unsigned int nFlags = 0;
	if (bSetDirty)
		nFlags |= FACTORY_FLAG_SET_DIRTY;
	if (bUpdateCleanMembers)
		nFlags |= FACTORY_FLAG_UPDATE_CLEAN_MEMBERS;
	Set(pzData,nDataLen,nFlags);
	return;
}

void MemberDescriptor::AddAttribute( const char * pzName, int nValue )
{
	char pzTempBuf[20];
	sprintf(pzTempBuf,"%d",(int)nValue);
	AddAttribute( pzName, pzTempBuf );
}

void MemberDescriptor::AddAttribute( const char * pzName, const char * pzValue )
{
	if (!m_pAttributes)
	{
		m_pAttributes = new XMLAttributeList;
	}
	m_pAttributes->AddAttribute( pzName, pzValue );
}

const char *MemberDescriptor::FindAttribute( const char *pzAttName )
{
	if (!m_pAttributes)
	{
		return 0;
	}
	return m_pAttributes->FindAttribute( pzAttName );
}

void *MemberDescriptor::GetItemData()
{
	return m_pUserData;
}

void  MemberDescriptor::SetItemData(void *p)
{
	m_pUserData = p;
}

void MemberDescriptor::Init(XMLObject *pParent)
{ 
	m_pAttributes = 0;
	m_ParentObject = pParent;
	m_memberStateFlags = ( DATA_NULL | DATA_SERIALIZE ); 
	Left = 0; 
	Right = 0; 
	m_pfnFactoryCreate = 0; 
	m_DataAbstractor.pListHandler = 0;
	m_pUserData = 0;
	m_bFirstMap = 1;
	m_MoreData.pTranslationLists = 0;
}

MemberDescriptor::MemberDescriptor(XMLObject *pParent, const char *pzTag, 
								   void *pKeyedDSA,KeyedDataStructureAbstraction *pHandler)
{
	Init(pParent); 
	strTagName = pzTag; 
	m_Member.pKeyedDSA = pKeyedDSA;
	DataType = KeyedDataStructure;
	m_DataAbstractor.pKeyDSA= pHandler;
}


MemberDescriptor::MemberDescriptor (XMLObject *pParent, const char *pzTag,MemberHandler *pHandle)
{ 
	Init(pParent); 
	strTagName = pzTag; 
	m_Member.pObject = pParent;
	DataType = ManagedByDerivedClass;
	m_DataAbstractor.pMemberHandler = pHandle;
}


MemberDescriptor::MemberDescriptor 
	(XMLObject *pParent, const char *pzTag, ObjectFactory pFactory, const char * pzClassType)
{ 
	Init(pParent); 
	strTagName = pzTag; 
	m_pfnFactoryCreate = pFactory; 
	DataType = Root; 
	m_Member.pClassName = pzClassType;

}

MemberDescriptor::MemberDescriptor(XMLObject *pParent, const char *pzTag, char *pVal, int nMaxSize,const char *pzTranslationMapIn,const char *pzTranslationMapOut,int nTranslationFlags)
{
	Init(pParent); 
	strTagName = pzTag; 
	m_Member.pNativeStr = pVal;
	DataType = Str;
	m_DataAbstractor.nBufferMaxSize = nMaxSize;
	if (pzTranslationMapIn || pzTranslationMapOut)
	{
		m_MoreData.pTranslationLists = new TranslationLists(pzTranslationMapIn,pzTranslationMapOut,Str,nTranslationFlags);
	}
}

MemberDescriptor::MemberDescriptor(XMLObject *pParent, const char *pzTag, char *pVal, char ForNameManglingOnly)
{
	Init(pParent); 
	strTagName = pzTag; 
	m_Member.pByte = pVal;
	DataType = Byte;
}

MemberDescriptor::MemberDescriptor(XMLObject *pParent, const char *pzTag, short *pVal, const char *pzTranslationMapIn, const char *pzTranslationMapOut,int nTranslationFlags)
{
	Init(pParent); 
	strTagName = pzTag; 
	m_Member.pShort = pVal;
	DataType = Short;
	if (pzTranslationMapIn || pzTranslationMapOut)
	{
		m_MoreData.pTranslationLists = new TranslationLists(pzTranslationMapIn,pzTranslationMapOut,Short,nTranslationFlags);
	}
}

#ifndef _NO_XMLF_BOOL
MemberDescriptor::MemberDescriptor(XMLObject *pParent, const char *pzTag, bool *pVal, int nBoolReadability)
{
	Init(pParent); 
	strTagName = pzTag; 
	m_Member.pBool = pVal;
	DataType = Bool;
	m_DataAbstractor.nBoolReadability = nBoolReadability; // defaults to "True/False"
}
#endif // _NO_XMLF_BOOL


MemberDescriptor::MemberDescriptor(XMLObject *pParent, const char *pzTag, unsigned char *pVal, unsigned __int64 nBitMask, const char *pzCommaSeparated1Values, const char *pzCommaSeparated0Values)
{
	Init(pParent); 
	strTagName = pzTag; 
	m_Member.p8Bits = pVal;
	DataType = Bit;
	m_DataAbstractor.nBitMask = nBitMask;
	m_MoreData.pTranslationLists = new TranslationLists(pzCommaSeparated1Values,pzCommaSeparated0Values,Bit,0);
}
MemberDescriptor::MemberDescriptor(XMLObject *pParent, const char *pzTag, unsigned short *pVal, unsigned __int64 nBitMask, const char *pzCommaSeparated1Values, const char *pzCommaSeparated0Values)
{
	Init(pParent); 
	strTagName = pzTag; 
	m_Member.p16Bits = pVal;
	DataType = Bit;
	m_DataAbstractor.nBitMask = nBitMask;
	m_MoreData.pTranslationLists = new TranslationLists(pzCommaSeparated1Values,pzCommaSeparated0Values,Bit,0);
}
MemberDescriptor::MemberDescriptor(XMLObject *pParent, const char *pzTag, unsigned int *pVal, unsigned __int64 nBitMask, const char *pzCommaSeparated1Values, const char *pzCommaSeparated0Values)
{
	Init(pParent); 
	strTagName = pzTag; 
	m_Member.p32Bits = pVal;
	DataType = Bit;
	m_DataAbstractor.nBitMask = nBitMask;
	m_MoreData.pTranslationLists = new TranslationLists(pzCommaSeparated1Values,pzCommaSeparated0Values,Bit,0);
}
MemberDescriptor::MemberDescriptor(XMLObject *pParent, const char *pzTag, unsigned __int64 *pVal, unsigned __int64 nBitMask, const char *pzCommaSeparated1Values, const char *pzCommaSeparated0Values)
{
	Init(pParent); 
	strTagName = pzTag; 
	m_Member.pBits = pVal;
	DataType = Bit;
	m_DataAbstractor.nBitMask = nBitMask;
	m_MoreData.pTranslationLists = new TranslationLists(pzCommaSeparated1Values,pzCommaSeparated0Values,Bit,0);
}


MemberDescriptor::MemberDescriptor (XMLObject *pParent, const char *pzTag, long *pVal, const char *pzTranslationMapIn, const char *pzTranslationMapOut,int nTranslationFlags)
{
	Init(pParent); 
	strTagName = pzTag; 
	m_Member.pLong = pVal;
	DataType = Long;
	if (pzTranslationMapIn || pzTranslationMapOut)
	{
		m_MoreData.pTranslationLists = new TranslationLists(pzTranslationMapIn,pzTranslationMapOut,Long,nTranslationFlags);
	}
}

MemberDescriptor::MemberDescriptor (XMLObject *pParent, const char *pzTag, int *pVal, const char *pzTranslationMapIn, const char *pzTranslationMapOut,int nTranslationFlags)
{ 
	Init(pParent); 
	strTagName = pzTag; 
	m_Member.pInt = pVal;
	DataType = Int;

	if (pzTranslationMapIn || pzTranslationMapOut)
	{
		m_MoreData.pTranslationLists = new TranslationLists(pzTranslationMapIn,pzTranslationMapOut,Int,nTranslationFlags);
	}
}


MemberDescriptor::MemberDescriptor (XMLObject *pParent, const char *pzTag, __int64 *pVal, const char *pzTranslationMapIn, const char *pzTranslationMapOut,int nTranslationFlags)
{ 
	Init(pParent); 
	strTagName = pzTag; 
	m_Member.pInt64 = pVal;
	DataType = Int64;
	if (pzTranslationMapIn || pzTranslationMapOut)
	{
		m_MoreData.pTranslationLists = new TranslationLists(pzTranslationMapIn,pzTranslationMapOut,Int64,nTranslationFlags);
	}
}

MemberDescriptor::MemberDescriptor (XMLObject *pParent, const char *pzTag, double *pVal, const char *pzTranslationMapIn, const char *pzTranslationMapOut,int nTranslationFlags)
{ 
	Init(pParent); 
	strTagName = pzTag; 
	m_Member.pDouble = pVal;
	DataType = Double;
	if (pzTranslationMapIn || pzTranslationMapOut)
	{
		m_MoreData.pTranslationLists = new TranslationLists(pzTranslationMapIn,pzTranslationMapOut,Double,nTranslationFlags);
	}
}

MemberDescriptor::MemberDescriptor(XMLObject *pParent, const char *pzEntityName)
{ 
	Init(pParent); 
	strTagName.Format("&%s;", pzEntityName); 
	DataType = EntityReference;
}

MemberDescriptor::MemberDescriptor(XMLObject *pParent, const char *pzTag, void *pVal, StringAbstraction *pHandler,const char *pzTranslationMapIn,const char *pzTranslationMapOut,int nTranslationFlags)
{ 
	Init(pParent); 
	strTagName = pzTag; 
	m_Member.pUserString = pVal;
	DataType = UserString;
	m_DataAbstractor.pStringHandler = pHandler;
	if (pzTranslationMapIn || pzTranslationMapOut)
	{
		m_MoreData.pTranslationLists = new TranslationLists(pzTranslationMapIn,pzTranslationMapOut,UserString,nTranslationFlags);
	}
}

// Construct a string collection mapping entry
MemberDescriptor::MemberDescriptor(XMLObject *pParent, const char *pzTag, void *pStringCollection, StringCollectionAbstraction *pHandler,const char *pzTranslationMapIn,const char *pzTranslationMapOut,int nTranslationFlags)
{
	Init(pParent); 
	strTagName = pzTag; 
	m_Member.pUserStringCollection = pStringCollection;
	DataType = UserStringCollection;
	m_DataAbstractor.pStrListHandler = pHandler;
	if (pzTranslationMapIn || pzTranslationMapOut)
	{
		m_MoreData.pTranslationLists = new TranslationLists(pzTranslationMapIn,pzTranslationMapOut,UserStringCollection,nTranslationFlags);
	}
}

// Construct an integer array mapping entry
MemberDescriptor::MemberDescriptor(XMLObject *pParent, const char *pzTag, void *pIntegerArray, IntegerArrayAbstraction *pHandler,const char *pzTranslationMapIn,const char *pzTranslationMapOut,int nTranslationFlags)
{
	Init(pParent); 
	strTagName = pzTag; 
	m_Member.pUserIntegerArray = pIntegerArray;
	DataType = UserIntegerArray;
	m_DataAbstractor.pIntArrayHandler = pHandler;
	if (pzTranslationMapIn || pzTranslationMapOut)
	{
		m_MoreData.pTranslationLists = new TranslationLists(pzTranslationMapIn,pzTranslationMapOut,UserIntegerArray,nTranslationFlags);
	}
}


MemberDescriptor::MemberDescriptor(XMLObject *pParent, const char *pzTag, XMLObject *pObj)
{ 
	Init(pParent); 
	strTagName = pzTag; 
	m_Member.pObject = pObj; 
	DataType = XMLObj;
}

MemberDescriptor::MemberDescriptor(XMLObject *pParent, const char *pzTag, XMLObject **pObj, ObjectFactory pFactory)
{ 
	Init(pParent); 
	strTagName = pzTag; 
	m_Member.ppObject = pObj; 
	m_pfnFactoryCreate = pFactory; 
	DataType = XMLObjPtr;
}

MemberDescriptor::MemberDescriptor(XMLObject *pParent, const char *pzTag, void *pList, ListAbstraction *pHandler, ObjectFactory pFactory)
{
	Init(pParent); 
	strTagName = pzTag; 
	m_pfnFactoryCreate = pFactory; 
	m_Member.pXMLObjList = pList; 
	DataType = XMLObjList;
	m_DataAbstractor.pListHandler = pHandler;
}


__int64 MemberDescriptor::GetObjectContainmentCount()
{
	__int64 nContainmentCount = 0;
	if ( DataType == XMLObj )
	{
		nContainmentCount = 1;
	}
	else if ( DataType == XMLObjPtr )
	{
		// if we've allocated the object pointer, consider it an instance
		if ( *(m_Member.ppObject) )
		{
			nContainmentCount = 1;
		}
	}
	else if ( DataType == XMLObjList )
	{
		nContainmentCount = m_DataAbstractor.pListHandler->itemCount(m_Member.pXMLObjList);
	}
	else if (DataType == KeyedDataStructure )
	{
		m_DataAbstractor.pKeyDSA->itemCount(m_Member.pKeyedDSA);
	}
	return nContainmentCount;
}

// Only release an iterator if you have not walked the entire list.
// Calling GetNextContainedObject() after the last item in a list 
// will return null and delete the iterator.
void MemberDescriptor::ReleaseObjectIterator( xmlObjectIterator iter )
{
	if ( DataType == XMLObjList && iter )
	{
		m_DataAbstractor.pListHandler->releaseIterator(iter);
	}
	else if (DataType == KeyedDataStructure )
	{
		m_DataAbstractor.pKeyDSA->releaseIterator(iter);
	}
}

void MemberDescriptor::DeferredDestruction()
{
	if ( DataType == XMLObjList )
	{
		m_DataAbstractor.pListHandler->destroy( m_Member.pXMLObjList );
	}
	else if (DataType == KeyedDataStructure )
	{
		m_DataAbstractor.pKeyDSA->destroy( m_Member.pKeyedDSA );
	}
}


void MemberDescriptor::RemoveLastContainedObject( xmlObjectIterator iter, GString &strTag, XMLObject *pAddressOfInvalidMemory )
{
	if ( DataType == XMLObjPtr )
	{
		// Null out what the ObjectPtr<> was pointing to.
		*(m_Member.ppObject) = 0;
	}
	else if ( DataType == XMLObjList )
	{
		try
		{
			m_DataAbstractor.pListHandler->removeObject( m_Member.pXMLObjList, iter );
		}	
		catch(...)
		{
			GString strError;
			strError.Format("The list mapped to tag %s has an invalid list handler.", (const char *)strTagName);
			TRACE_ERROR((const char *)strError);
		}
	}
	else if (DataType == KeyedDataStructure )
	{
		// search the structure for key [strtag], remove any reference to object [pAddressOfInvalidMemory]
		m_DataAbstractor.pKeyDSA->remove( m_Member.pKeyedDSA, iter, strTag, pAddressOfInvalidMemory);
	}
}


int MemberDescriptor::ObjectExists( XMLObject *pO )
{
	xmlObjectIterator iter = 0;
	XMLObject *pObj = GetFirstContainedObject(&iter);
	while (pObj)
	{
		if (pObj == pO)
		{
			ReleaseObjectIterator(iter);
			return 1;
		}
		pObj = GetNextContainedObject(iter);
	}
	return 0;
}


int MemberDescriptor::AddContainedObject( XMLObject *pO )
{
	int nRet = 1; // success
	if ( DataType == XMLObjList )
	{
		m_DataAbstractor.pListHandler->append( m_Member.pXMLObjList, pO );
	}
	else if ( DataType == KeyedDataStructure )
	{
		if (!pO->getOID())
		{
			// This happens if you map an object to a keyed data structure without specifying an OID
			throw GException("XML Object Factory", 3, pO->GetObjectType(), pO->GetObjectTag());
		}
		m_DataAbstractor.pKeyDSA->insert(m_Member.pKeyedDSA,pO,pO->getOID());
	}
	else if ( DataType == XMLObjPtr )
	{
		*(m_Member.ppObject) = pO;
	}
	else
	{
		// cannot Add an object to this member type.
		nRet = 0; 
	}
	
	if (nRet && GetObjectContainmentCount() == 1)
	{
		// add this map to the 'active system maps'
		g_ActiveObjectMap.AddActiveObjectMap(this);
	}

	return nRet;
}

XMLObject *MemberDescriptor::GetKeyedContainedObject( const char *key )
{
	XMLObject *pReturnObject = 0;
	if (DataType == KeyedDataStructure )
	{
		pReturnObject = m_DataAbstractor.pKeyDSA->search( m_Member.pKeyedDSA, key);
	}
	return pReturnObject;
}

XMLObject *MemberDescriptor::GetFirstContainedObject( xmlObjectIterator *iter )
{
	XMLObject *pReturnObject = 0;

	if ( DataType == XMLObj )
	{
		pReturnObject = m_Member.pObject;
	}
	else if ( DataType == XMLObjPtr )
	{
		pReturnObject = *(m_Member.ppObject);
	}
	else if ( DataType == XMLObjList )
	{
		try
		{
			pReturnObject = m_DataAbstractor.pListHandler->getFirst( m_Member.pXMLObjList, iter );
		}	
		catch(...)
		{
			GString strError;
			strError.Format("The list mapped to tag %s has an invalid list handler.", (const char *)strTagName);
			TRACE_ERROR((const char *)strError);
			pReturnObject = 0;
		}
	}
	else if (DataType == KeyedDataStructure )
	{
		pReturnObject = m_DataAbstractor.pKeyDSA->getFirst( m_Member.pKeyedDSA, iter );
	}
	if (pReturnObject)
	{
		try
		{
			// A useful development time object reference validation.  777 is good.
			// any other value indicates corrupt or unowned memory at this location.
			// Possible if the implementation object was deleted in the application 
			// without sync'ing the ref count.
			// Also use XMLObject::Dump() to check that Ref Counts are correct.
			if (pReturnObject == (void *)-1 || pReturnObject->m_ValidObjectBeg != 777 )
			{
				return 0; // lower code cut intentionally.

				// This indicates that the return object is no longer valid.
				// The object has been deleted but some data structure kept
				// an invalid pointer to it.  This can happen when Objects with
				// Interfaces (CORBA,COM,RPC) decrement to destroy the object.
				// Then leave a reference to it in a list, hash, or ObjPointer.
				GString strTypeDescription;
				GetTypeName(strTypeDescription); // if "this" is garbage that line can crash.
				const char *pzOwningObject = (m_ParentObject) ? m_ParentObject->GetObjectType() : "Global"; 

				// Object [O] conains an invalid Obj Ref in a [List] mapped to tag [MyXMLTag]
				throw GException("XML Object", 2, pzOwningObject, 
														(const char *)strTypeDescription, 
														(const char *)strTagName);
			}
		}catch(...) // 2009
		{
			return 0; 
		}
	}
	return pReturnObject;
}

XMLObject *MemberDescriptor::GetNextContainedObject( xmlObjectIterator iter )
{
	XMLObject *pReturnObject = 0;
	if ( DataType == XMLObjList )
	{
		pReturnObject = m_DataAbstractor.pListHandler->getNext( m_Member.pXMLObjList, iter );
	}
	else if (DataType == KeyedDataStructure )
	{
		pReturnObject = m_DataAbstractor.pKeyDSA->getNext( m_Member.pKeyedDSA, iter );
	}
	return pReturnObject;
}



// GetMemberValue() returns the value for all simple datatypes.
// For complex types (list<void>, list<object>, object&, and UserCollections )
// NULL is returned, since there is no simple value.
const char *MemberDescriptor::GetMemberValue(GString &strValue)
{
	bool bRet = false;
	if ( DataType == ManagedByDerivedClass )
	{
		try
		{
			m_DataAbstractor.pMemberHandler->GetMember( (const char *)strTagName, strValue/*oStr*/ );
			bRet = true;
		}
		catch(...)
		{
			GString strError;
			strError.Format("The custom handler for %s failed.", (const char *)strTagName);
			TRACE_ERROR((const char *)strError);
			bRet = false;
		}
	}
	else if(DataType == UserString)
	{
		try
		{
			strValue = m_DataAbstractor.pStringHandler->data(m_Member.pUserString);
			bRet = true;
		}
		catch(...)
		{
			GString strError;
			strError.Format("The string mapped to tag %s has an invalid string handler.", (const char *)strTagName);
			TRACE_ERROR((const char *)strError);
			bRet = false;
		}
	}
	else if (DataType == Str)
	{
		strValue = m_Member.pNativeStr;
		bRet = true;
	}
	else if (DataType == Bool)
	{
		// m_DataAbstractor.nBoolReadability =   0=Yes/No       1=True/False       2=On/Off		3=1/0
		if (*m_Member.pBool)
		{
			if (m_DataAbstractor.nBoolReadability == 0)
			{
				strValue = "Yes";
			}
			else if (m_DataAbstractor.nBoolReadability == 1)
			{
				strValue = "True";
			}
			else if (m_DataAbstractor.nBoolReadability == 2)
			{
				strValue = "On";
			}
			else // m_DataAbstractor.nBoolReadability == 3
			{
				strValue = "1";
			}
		}
		else
		{
			if (m_DataAbstractor.nBoolReadability == 0)
			{
				strValue = "No";
			}
			else if (m_DataAbstractor.nBoolReadability == 1)
			{
				strValue = "False";
			}
			else if (m_DataAbstractor.nBoolReadability == 2)
			{
				strValue = "Off";
			}
			else // m_DataAbstractor.nBoolReadability == 3
			{
				strValue = "0";
			}
		}
		bRet = true;
	}
	else if(DataType == Bit)
	{
		strValue = "";
		bRet = true;
	}
	else if(DataType == Byte)
	{
		strValue.Format("%c",*m_Member.pByte);
		bRet = true;
	}
	else if(DataType == Short)
	{
		strValue.Format("%d",*m_Member.pShort);
		bRet = true;
	}
	else if(DataType == Int)
	{
		strValue.Format("%d",*m_Member.pInt);
		bRet = true;
	}
	else if(DataType == Long)
	{
		strValue.Format("%ld",*m_Member.pLong);
		bRet = true;
	}
	else if(DataType == Int64)
	{
		strValue << *m_Member.pInt64;
		bRet = true;
	}
	else if(DataType == Double)
	{
		strValue << *m_Member.pDouble;
		bRet = true;
	}
//	else DataType is one of:
//		( XMLObjList || XMLObj || Root || XMLObjPtr || UserStringCollection || UserIntegerArray)
	if (bRet)
	{
		return strValue;
	}
	return NULL;
}
