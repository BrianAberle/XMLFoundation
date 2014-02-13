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

#include "GStringList.h"
#include "GString.h"
#include "GListNodeCache.h"

#include <string.h> // for: strlen(), strstr()

const char *GStringList::PeekLast(__int64 *pnLen/* = 0*/)
{
	m_strSerializeDest = "";
	if (pnLen)
	{
		*pnLen = 0;
	}

	GString *pG = (GString *)GList::Last();
	if (pG)
	{
		m_strSerializeDest = (const char *)*pG;
		if (pnLen)
		{
			*pnLen = pG->Length();
		}
	}
	return m_strSerializeDest;
}

const char *GStringList::PeekFirst(__int64 *pnLen/* = 0*/)
{
	m_strSerializeDest = "";
	if (pnLen)
	{
		*pnLen = 0;
	}

	GString *pG = (GString *)GList::First();
	if (pG)
	{
		m_strSerializeDest = (const char *)*pG;
		if (pnLen)
		{
			*pnLen = pG->Length();
		}
	}
	return m_strSerializeDest;
}

const char *GStringList::RemoveFirst(__int64 *pnLen/* = 0*/)
{
	m_strSerializeDest = "";
	if (pnLen)
	{
		*pnLen = 0;
	}

	GString *pG = (GString *)GList::First();
	if (pG)
	{
		GList::RemoveFirst();
		m_strSerializeDest = (const char *)*pG;
		if (pnLen)
		{
			*pnLen = pG->Length();
		}
		delete pG;
	}
	return m_strSerializeDest;
}

const char *GStringList::RemoveLast(__int64 *pnLen/* = 0*/)
{
	m_strSerializeDest = "";
	if (pnLen)
	{
		*pnLen = 0;
	}

	GString *pG = (GString *)GList::RemoveLast();
	if (pG)
	{
		m_strSerializeDest = (const char *)*pG;
		if (pnLen)
		{
			*pnLen = pG->Length();
		}
		delete pG;
	}
	return m_strSerializeDest;
}

void GStringList::AppendList(const GStringList *pListToCopy)
{
	if (pListToCopy)
	{
		GStringIterator it( pListToCopy );
		while (it())
		{
			AddLast((const char *)it++);
		}
	}
}



GString *GStringList::AddFirst(const char *szString, __int64 nStringSize/* = -1*/)
{
	GString *pstrString;
	if (nStringSize > -1)
		pstrString = new GString(szString,nStringSize);
	else
		pstrString = new GString(szString);
	GList::AddHead(pstrString);
	return pstrString;
}

GString *GStringList::AddFirst(unsigned long N)
{
	GString *pstrString = new GString;
	*pstrString << N;
	GList::AddHead(pstrString);
	return pstrString;
}

GString *GStringList::AddLast(unsigned long N)
{
	GString *pstrString = new GString;
	*pstrString << N;
	GList::AddLast(pstrString);
	return pstrString;
}

GString *GStringList::AddFirst(long N)
{
	GString *pstrString = new GString;
	*pstrString << N;
	GList::AddHead(pstrString);
	return pstrString;
}

GString *GStringList::AddLast(long N)
{
	GString *pstrString = new GString;
	*pstrString << N;
	GList::AddLast(pstrString);
	return pstrString;
}

GString *GStringList::AddFirst(unsigned int N)
{
	GString *pstrString = new GString;
	*pstrString << N;
	GList::AddHead(pstrString);
	return pstrString;
}

GString *GStringList::AddLast(unsigned int N)
{
	GString *pstrString = new GString;
	*pstrString << N;
	GList::AddLast(pstrString);
	return pstrString;
}


GString *GStringList::AddFirst(int N)
{
	GString *pstrString = new GString;
	*pstrString << N;
	GList::AddHead(pstrString);
	return pstrString;
}

GString *GStringList::AddLast(int N)
{
	GString *pstrString = new GString;
	*pstrString << N;
	GList::AddLast(pstrString);
	return pstrString;
}

GString *GStringList::AddFirst(unsigned short N)
{
	GString *pstrString = new GString;
	*pstrString << N;
	GList::AddHead(pstrString);
	return pstrString;
}
GString *GStringList::AddLast(unsigned short N)
{
	GString *pstrString = new GString;
	*pstrString << N;
	GList::AddLast(pstrString);
	return pstrString;
}
GString *GStringList::AddFirst(short N)
{
	GString *pstrString = new GString;
	*pstrString << N;
	GList::AddHead(pstrString);
	return pstrString;
}
GString *GStringList::AddLast(short N)
{
	GString *pstrString = new GString;
	*pstrString << N;
	GList::AddLast(pstrString);
	return pstrString;
}
GString *GStringList::AddFirst(float N)
{
	GString *pstrString = new GString;
	*pstrString << N;
	GList::AddHead(pstrString);
	return pstrString;
}
GString *GStringList::AddLast(float N)
{
	GString *pstrString = new GString;
	*pstrString << N;
	GList::AddLast(pstrString);
	return pstrString;
}
GString *GStringList::AddFirst(double N)
{
	GString *pstrString = new GString;
	*pstrString << N;
	GList::AddHead(pstrString);
	return pstrString;
}
GString *GStringList::AddLast(double N)
{
	GString *pstrString = new GString;
	*pstrString << N;
	GList::AddLast(pstrString);
	return pstrString;
}
GString *GStringList::AddFirst(long double N)
{
	GString *pstrString = new GString;
	*pstrString << N;
	GList::AddHead(pstrString);
	return pstrString;
}
GString *GStringList::AddLast(long double N)
{
	GString *pstrString = new GString;
	*pstrString << N;
	GList::AddLast(pstrString);
	return pstrString;
}



GString *GStringList::AddFirstUserConstructed(GString *pstrString)
{
	GList::AddHead(pstrString);
	return pstrString;
}

GString *GStringList::AddLastUserConstructed(GString *pstrString)
{
	GList::AddLast(pstrString);
	return pstrString;
}


GString *GStringList::AddLast(const char *szString, __int64 nStringSize /*= -1*/)
{
	GString *pstrString;
	if (nStringSize > -1)
		pstrString = new GString(szString,nStringSize);
	else
		pstrString = new GString(szString);
	GList::AddLast(pstrString);
	return pstrString;
}

GStringList::GStringList(const GStringList &src)
{
	GStringIterator it(&src);
	while (it())
		AddLast(it++);
}

GStringList::GStringList()
{
}

void GStringList::operator+=(const GStringList &src)
{
	GStringIterator it(&src);
	while (it())
		AddLast(it++);
}

void GStringList::operator+=(const char *szSrc)
{
	AddLast(szSrc);
}

void GStringList::RemoveAll()
{
	int n = Size();
	for(int i=0; i<n;i++)
		RemoveLast();
}


GList::Node* GlobalNodeCache(GList::Node*pNode, int nNewNode = -1, int *nStateQuery = 0);
GStringList::~GStringList()
{
	int nGlobalStateOnMyStack = 0;

	if ( GlobalNodeCache(0,0,&nGlobalStateOnMyStack) != (GList::Node *)21 ) 
	{
		// note: pNodeCache is NOT global
		// if we have any nodes in the GLOBAL NodeCache, we we are finished.
		// return 0
	}

	while (FirstNode)
	{
		if (CurrentNode == FirstNode) 
			CurrentNode = FirstNode->NextNode;
		if (LastNode == FirstNode) 
			LastNode = 0;
		Node *Save = FirstNode->NextNode;
		delete (GString *)FirstNode->Data;

		if (pNodeCache) pNodeCache->Put(FirstNode); else ::delete FirstNode;

		FirstNode = Save;
		iSize--;
	}
}

void GStringIterator::reset()
{
	pDataNode = (GStringList::Node *)((GStringList *)pTList)->FirstNode;
}

GStringIterator::GStringIterator(const GStringList *pList, int IterateFirstToLast/* = 1*/)
{
	pTList = (GStringList *)pList;
	pDataNode = 0;
	if (IterateFirstToLast) 
	{
		pDataNode = (GStringList::Node *)((GStringList *)pList)->FirstNode;
	}
	else 
	{
		pDataNode = ((GStringList *)pList)->LastNode;
	}
}

const char *GStringIterator::operator ++ (int)
{
	GString *pRet = (GString *)pDataNode->Data;
	pCurrentNode = pDataNode;
	pDataNode = pDataNode->NextNode;
	return pRet->StrVal();
}

const char *GStringIterator::operator -- (int)
{
	GString *pRet = (GString *)pDataNode->Data;
	pCurrentNode = pDataNode;
	pDataNode = pDataNode->PrevNode;
	return pRet->StrVal();
}

GString *GStringIterator::Next()	// same as operator++, only it returns the GString object *
{
	GString *pRet = (GString *)pDataNode->Data;
	pCurrentNode = pDataNode;
	pDataNode = pDataNode->NextNode;
	return pRet;
}

GString *GStringIterator::Prev()	// same as operator--, only it returns the GString object *
{
	GString *pRet = (GString *)pDataNode->Data;
	pCurrentNode = pDataNode;
	pDataNode = pDataNode->PrevNode;
	return pRet;
}

int GStringIterator::operator ()  (void) const
{
	return pDataNode != 0;
}



// replace the string value at the given index with the new value, return the old value
GString *GStringList::SetAt(int nIdx, const char *pzNewValue)
{
	static GString strLastValue;
	if (nIdx < GList::Size())
	{
		GList::First();
		int nCur = 0;
		while(1)
		{
			if (nCur == nIdx)
			{
				GString *pG = (GString *)GList::Current();
				strLastValue = *pG;
				*pG = pzNewValue;
				return &strLastValue;
			}
			nCur++;
		}
	}
	else
		return 0;
}

GString *GStringList::GetStrAt(__int64 nIdx)
{
	static GString strBadIndex;
	if (nIdx < GList::Size())
	{
		void *p = GList::First();
		__int64 nCur = 0;
		while(1)
		{
			if (nCur == nIdx)
			{
				return (GString *)GList::Current();
			}
			nCur++;
			p = GList::Next();
		}
	}
	else
		return &strBadIndex;
}

const char *GStringList::GetAt(__int64 nIdx)
{
	if (nIdx > -1)
	{
		GStringIterator it(this);
		__int64 i = 0;
		while(it())
		{
			if (i++ == nIdx)
			{
				return it++;
			}
			it++;
		}
	}
	return 0;
}


const char *GStringList::operator[](__int64 nIdx)
{
	if (nIdx > -1)
	{
		GStringIterator it(this);
		__int64 i = 0;
		while(it())
		{
			if (i++ == nIdx)
			{
				return it++;
			}
			it++;
		}
	}
	return 0;
}
const char *GStringList::operator[](int nIdx)
{
	if (nIdx > -1)
	{
		GStringIterator it(this);
		int i = 0;
		while(it())
		{
			if (i++ == nIdx)
			{
				return it++;
			}
			it++;
		}
	}
	return 0;
}

int GStringList::ToFileAppend(const char *pzDelimiter, const char *pzPathAndFileName, int nStartIndex/* = 0*/, int nItemCount/* = -1*/)
{
	Serialize(pzDelimiter, nStartIndex,nItemCount);
	return m_strSerializeDest.ToFileAppend(pzPathAndFileName,0);
}

void GStringList::SerializeTo(GString &strDest, const char *pzDelimiter, int nStartIndex/* = 0*/, int nItemCount /*= -1*/)
{
	strDest = "";
	GStringIterator it(this);
	int nCount = 0;
	int nCurIndex = 0;
	nItemCount = (nItemCount == -1) ? Size() : nItemCount;
	while (it())
	{
		if (nCurIndex >= nStartIndex)
		{
			if (nCount)
				strDest += pzDelimiter;
			nCount++;
			strDest += it++;
			if (--nItemCount < 1)
				break;
		}
		else
		{
			it++;
		}
		nCurIndex++;
	}
}

const char *GStringList::Serialize(const char *pzDelimiter, int nStartIndex/* = 0*/, int nItemCount /* = -1*/)
{
	m_strSerializeDest = "";
	SerializeTo(m_strSerializeDest, pzDelimiter, nStartIndex, nItemCount);
	return m_strSerializeDest;
}

GStringList::GStringList(const char *pzDelimiter, const char *pzSource, int nItemCount/*  = -1*/)
{
	DeSerialize(pzDelimiter, pzSource, nItemCount);
}

int GStringList::FromFile(const char *pzDelimiter, const char *pzPathAndFileName, int nItemCount /* = -1*/)
{
	GString strData;
	if (strData.FromFile(pzPathAndFileName,0))
	{
		DeSerialize(pzDelimiter, strData, nItemCount);
		return 1;
	}
	return 0;
}

void GStringList::DeSerialize(const char *pzDelimiter, const char *pzSource, int nItemCount/*  = -1*/)
{
	RemoveAll();
	DeSerializeAppend(pzDelimiter, pzSource, nItemCount);
}


int GStringList::FromFileAppend(const char *pzDelimiter, const char *pzPathAndFileName, int nItemCount/* = -1*/)
{
	GString strTemp;
	if (strTemp.FromFile(pzPathAndFileName,0))
	{
		DeSerializeAppend(pzDelimiter, strTemp, nItemCount);
		return 1;
	}
	return 0;
}

void GStringList::DeSerializeAppend(const char *pzDelimiter, const char *pzSource, int nItemCount/*  = -1*/)
{
	if (!pzDelimiter || !pzSource || !pzDelimiter[0] || !pzSource[0])
		return;

	
	__int64 nSourceLen = strlen(pzSource);

	__int64 nDelimiterLen = strlen(pzDelimiter);
	
	if (!nSourceLen)
		return;	

	int nAdded = 0;
	char *beg = (char *)pzSource; 
	char *del = strstr(beg,pzDelimiter);
	while(1)
	{
		if ( !del )
		{
			// there is only one entry in the list 
			if (nItemCount > -1 && nAdded == nItemCount)
				break;
			AddLast(beg);
			nAdded++;
			break;
		}

		if (nItemCount > -1 && nAdded == nItemCount)
			break;

		// add this entry
		AddLast(beg, del - beg);
		nAdded++;

		// advance to the next string
		beg = del + nDelimiterLen;
		
		// advance to the next delimiter, break if none
		del = strstr(beg,pzDelimiter);
		if ( !del )
		{
			if (nItemCount > -1 && nAdded == nItemCount)
				break;
			AddLast(beg);
			nAdded++;
			break;
		}
	}
}

// removes the first occurance of pzMatch from the list
// or every occurance if bRemoveAllOccurances = 1
// if bMatchCase = 1 only ExAcT Case Matches are removed
// returns number of items removed.
__int64 GStringList::Remove(const char *pzMatch, int bMatchCase, int bRemoveAllOccurances)
{
	GString *p = (GString *)First();
	__int64 nItemsRemoved = 0;
	while(p)
	{
		if (bMatchCase)
		{
			if (p->Compare(pzMatch) == 0)
			{
				nItemsRemoved++;
				RemoveCurrent();
				delete p;
				if (!bRemoveAllOccurances)
					break;
			}
		}
		else
		{
			if (p->CompareNoCase(pzMatch) == 0)
			{
				nItemsRemoved++;
				RemoveCurrent();
				delete p;
				if (!bRemoveAllOccurances)
					break;
			}
		}
		p = (GString *)Next();
	}
	return nItemsRemoved;
}

// returns -1 if not found otherwise the Index
__int64 GStringList::Find(const char *pzFindWhat, int bMatchCase/* = 1*/)
{
	GString *p = (GString *)First();
	__int64 nIndex = -1;
	while(p)
	{
		nIndex++;
		if (bMatchCase)
		{
			if (p->Compare(pzFindWhat) == 0)
			{
				return nIndex;
			}
		}
		else
		{
			if (p->CompareNoCase(pzFindWhat) == 0)
			{
				return nIndex;
			}
		}
		p = (GString *)Next();
	}
	return -1; // not found
}

// returns -1 if not found otherwise the Index
GString * GStringList::FindStringContaining(char chWhatToFind, __int64 *pnFoundIndex, __int64 nStartingIndex/* = 0*/)
{
	GString *p = GetStrAt(nStartingIndex);
	while(p)
	{
		if (p->Find(chWhatToFind) != -1)
		{
			if (pnFoundIndex)
				*pnFoundIndex = nStartingIndex;
			return p;
		}

		nStartingIndex++;
		p = (GString *)Next();
	}
	if (pnFoundIndex)
		*pnFoundIndex = -1;
	return 0; // not found
}

GString * GStringList::FindStringContaining(const char *pzWhatToFind, __int64 *pnFoundIndex, int bMatchCase/* = 1*/, __int64 nStartingIndex/* = 0*/)
{
	GString *p = GetStrAt(nStartingIndex);
	while(p)
	{
		if (bMatchCase)
		{
			if (p->Find(pzWhatToFind) != -1)
			{
				if (pnFoundIndex)
					*pnFoundIndex = nStartingIndex;
				return p;
			}
		}
		else
		{
			if (p->FindCaseInsensitive(pzWhatToFind) == 0)
			{
				if (pnFoundIndex)
					*pnFoundIndex = nStartingIndex;
				return p;
			}
		}
		nStartingIndex++;
		p = (GString *)Next();
	}

	if (pnFoundIndex)
		*pnFoundIndex = -1;
	return 0; // not found
}
