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
#ifndef _GHTTP_MULTI_PART_H__
#define _GHTTP_MULTI_PART_H__

#include "GString.h"
#include "GBTree.h"


class DataController
{
public:
	DataController(){};

	virtual int GetTotalBytesIn() = 0;
	virtual int GetExpectedDataSize() = 0;
	virtual char *GetBuffer1() = 0;
	virtual void Disconnect() = 0;

	// returns 1 on success, -1 on error, 0 when no more data, pnMoreBytes will be size of the the data found
	virtual int NextDataChunk(int *pnDataSize, char *pDest, int nMax, int nSecondsTimeout, int nMicrosecondsTimeout) = 0;
	virtual int NextDataChunk(int *pnMoreBytes) = 0;

	~DataController(){};
};


class FormPart
{
public:
	GString strDestFileName;
	GString strSourceFileName;
	GString strPartName;
	GString strHeader;
	GString *strData;
	int m_bOwns;
	FormPart(int nEstimatedSize) {strData = new GString(nEstimatedSize); m_bOwns = 1; };
	FormPart(GString *pDest) {strData = pDest; m_bOwns = 0; };
	~FormPart() {if (m_bOwns == 1) delete strData; };
};

class CMultiPartForm
{
	DataController *m_pPIC;
	char *m_pzContent;
	__int64 m_pzContentIndex;
	__int64 m_nBytesInBuffer;
	GString m_strBoundary;
	GString m_strEndBoundary;
	__int64 m_nRemainingData;
	int m_bFinished;
	GBTree m_Parts;
	int m_bOwnMemory;
	int m_nLastError;
	GString m_strLastError;
public:
	~CMultiPartForm();
	
	CMultiPartForm(DataController* pPIC, const char *pzHeader, const char *pzContent);
	void MapArgument(const char *pzArgName, GString *pstrDest);
	void MapArgumentToFile(const char *pzArgName, const char *pzPathAndFileName);
	FormPart *AddArgument(const char *pzArgName, int nEstimatedSize = 256);
	const char *GetLastError(int *pnErrorCode = 0);
	int GetAll();
	int ReadNext(FormPart **ppPart);
};


#endif //_GHTTP_MULTI_PART_H__

