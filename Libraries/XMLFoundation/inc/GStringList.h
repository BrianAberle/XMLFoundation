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

#ifndef _GSTRING_LIST_H__
#define _GSTRING_LIST_H__

#include "GList.h"
#include "GString.h"

class GStringList : public GList
{
	GString m_strSerializeDest;
protected:

	friend class GStringIterator; // Generic String List Iterator Class

public:

	///////////////////////////////////////////////////////////////////
	// Serialize the list.
	// examples if list contains 10 items: A,B,C,D,E,F,G,H,I, and J
	///////////////////////////////////////////////////////////////////
	// GStringList lst("|","A|B|C|D|E|F|G|H|I|J");
	// GString rslt;
	// rslt = lst.Serialize(":");		 // "A:B:C:D:E:F:G:H:I:J"
	// rslt = lst.Serialize(":", 5);	 // "F:G:H:I:J"
	// rslt = lst.Serialize(":", 0, 5);  // "A:B:C:D:E"
	// rslt = lst.Serialize(":", 2, 5);  // "C:D:E:F:G"
	// rslt = lst.Serialize("---", 2, 5);// "C---D---E---F---G"
	const char *Serialize(const char *pzDelimiter, int nStartIndex = 0, int nItemCount = -1);
	// same as Serialize, but you supply the destination, that you may have PreAlloc()'d so it will be faster.
	void SerializeTo(GString &strDest, const char *pzDelimiter, int nStartIndex = 0, int nItemCount = -1);

	// set the items in this list from the serialized data.  This can be done in the constructor too.
	void DeSerialize(const char *pzDelimiter, const char *pzSource, int nItemCount = -1);


	// returns 1 for success, 0 if faild to write
	int ToFileAppend(const char *pzDelimiter, const char *pzPathAndFileName, int nStartIndex = 0, int nItemCount = -1);

	// returns 1 for success, 0 if disk read failed
	int FromFile(const char *pzDelimiter, const char *pzPathAndFileName, int nItemCount = -1);

	// append the serialized data to this list
	void DeSerializeAppend(const char *pzDelimiter, const char *pzSource, int nItemCount = -1);
	// returns 1 for success, or 0 if disk read failed
	int FromFileAppend(const char *pzDelimiter, const char *pzPathAndFileName, int nItemCount = -1);

	// to append one GStringList to 'this'
	void AppendList(const GStringList *);

	// number of strings in the list
	__int64 Size()	const	{return GList::Size();}

	// the data supplied is copied into a new GString and added 
	// to the list in either the first or last position
	GString *AddFirst(const char *szString, __int64 nStringSize = -1);
	GString *AddLast(const char *szString, __int64 nStringSize = -1);
	

	// the supplied numeric data type stored converted and stored in ASCII
	GString *AddFirst(unsigned long N);
	GString *AddLast(unsigned long N);
	GString *AddFirst(long N);
	GString *AddLast(long N);
	GString *AddFirst(unsigned int N);
	GString *AddLast(unsigned int N);
	GString *AddFirst(int N);
	GString *AddLast(int N);
	GString *AddFirst(unsigned short N);
	GString *AddLast(unsigned short N);
	GString *AddFirst(short N);
	GString *AddLast(short N);
	GString *AddFirst(float N);
	GString *AddLast(float N);
	GString *AddFirst(double N);
	GString *AddLast(double N);
	GString *AddFirst(long double N);
	GString *AddLast(long double N);
	

	// The supplied GString * MUST BE ON THE HEAP.
	// The GStringList will 'own' the GString and IT WILL DELETE IT when the list goes out of scope.
	// This allows you to create your GString in any manner before putting it into this list.
	// YOU MUST NOT DELETE pstrString, since this GStringList will delete it.
	GString *AddLastUserConstructed(GString *pstrString);
	GString *AddFirstUserConstructed(GString *pstrString);

	
	// supply a pointer to an Integer if you also want the length of result
	const char *PeekLast(__int64 *pnLen = 0);
	const char *RemoveLast(__int64 *pnLen = 0);
	const char *PeekFirst(__int64 *pnLen = 0);
	const char *RemoveFirst(__int64 *pnLen = 0);
	
	
	void RemoveAll();
	
	// removes the first occurance of pzMatch from the list
	// or every occurance if bRemoveAllOccurances = 1
	// if bMatchCase = 1 only ExAcT Case Matches are removed
	// returns number of items removed.
	__int64 Remove(const char *pzMatch, int bMatchCase = 0, int bRemoveAllOccurances = 1);

	GStringList();
	GStringList(const GStringList &);
	// see notes for Serialize() method to use this 'ctor
	GStringList(const char *pzDelimiter, const char *pzSource, int nItemCount = -1);

	~GStringList();

	// returns -1 if not found otherwise the Index, only finds ExAcT matches unless bMatchCase = 0
	__int64 Find(const char *pzFindWhat, int bMatchCase = 1);
	
	// searches for a substring in each string entry. 
	// pnFoundIndex will be set to -1 upon return if no string in this list contained 
	//		chWhatToFind  otherwise pnFoundIndex will contain the Index of the string in this
	// if pnFoundIndex != -1, the GString object is returned - otherwise if pnFoundIndex == -1 NULL is returned
	GString *FindStringContaining(char chWhatToFind, __int64 *pnFoundIndex = 0, __int64 nSearchStartingIndex = 0);
	GString *FindStringContaining(const char *pzWhatToFind, __int64 *pnFoundIndex = 0, int bMatchCase = 1, __int64 nSearchStartingIndex = 0);


	// 0 based index = 0 is the first string in the list, 1 is the second...
	// the [] and GetAt() are the same.
	const char *operator[](int nIdx);		// iterator based - read only thread safe
	const char *operator[](__int64 nIdx);	// iterator based - read only thread safe

	const char *GetAt(__int64 nIdx);			// iterator based - read only thread safe
	GString *GetStrAt(__int64 nIdx);			// normal list iteration

	// replace the string value at the given nIdx(0 is first, 1 is second...) with pzNewValue, 
	// return the old value in a temporary GString, YOU must MUTEX this call if you have 
	// multiple threads that use this same GStringList and may call this method simultaneously.
	GString *SetAt(int nIdx, const char *pzNewValue);


	void operator+=(const GStringList &);
	void operator+=(const char *);
};

class GStringIterator
{
	GStringList::Node *pDataNode;
	GStringList *pTList;
public:
	GStringList::Node *pCurrentNode;    // used to return reference to data

	GStringIterator(const GStringList *pList, int IterateFirstToLast = 1);
	~GStringIterator() {};
	int operator () () const;
	void reset();
	const char * operator ++ (int);
	const char * operator -- (int);
	GString * Next();	// same as operator++, only it returns the GString object *
	GString * Prev();	// same as operator--, only it returns the GString object *
};

#endif //_GSTRING_LIST_H__
