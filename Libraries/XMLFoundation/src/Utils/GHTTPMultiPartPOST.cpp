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
#include "GHTTPMultiPartPOST.h"

#include <string.h>			// for memcmp()
#ifdef _WIN32
	#include <stdio.h>		// for unlink()
#else
	#include <unistd.h> // for unlink() 
#endif 


CMultiPartForm::~CMultiPartForm()
{
	if (m_nLastError != 0)
	{
		// if m_nLastError != 0, then we didn't finish reading the POST data so the browser is all queued up 
		// and waiting to send more, and won't even consider looking for a response to the POST until it's
		// finished sending it.  This can only happen 2 ways.  1) either we failed to write to our local disk
		// and stopped reading the POST data because there was nowhere to put it, or 2) someone POSTed us garbage.
		// Either way, disconnect is appropriate.
		m_pPIC->Disconnect();
	}
	if (m_bOwnMemory)
	{
		delete m_pzContent;
		m_bOwnMemory = 0;
	}
	GBTreeIterator it(&m_Parts);
	while(it())
	{
		delete (FormPart *)it++;
	}
}
	
CMultiPartForm::CMultiPartForm(DataController* pPIC, const char *pzHeader, const char *pzContent)
{
	m_pzContent = (char *)pzContent;
	m_pPIC = pPIC;
	char *p = stristr(pzHeader,"boundary=");
	if (p)
	{
		m_strBoundary.SetFromUpTo(p+9,"\r\n");
		m_strEndBoundary << "\r\n--" << m_strBoundary;
	}
	m_nBytesInBuffer = m_pPIC->GetTotalBytesIn();
	m_nRemainingData = m_pPIC->GetExpectedDataSize() - m_nBytesInBuffer;
	m_pzContentIndex = 0;
	m_bFinished = 0;
	m_bOwnMemory = 0;
	m_nLastError = 0;
}

void CMultiPartForm::MapArgument(const char *pzArgName, GString *pstrDest)
{
	FormPart *p = new FormPart(pstrDest);
	p->strPartName = pzArgName;
	m_Parts.insert(pzArgName,p);
}

void CMultiPartForm::MapArgumentToFile(const char *pzArgName, const char *pzPathAndFileName)
{
	FormPart *p = new FormPart(32767);
	p->strDestFileName = pzPathAndFileName;
	p->strPartName = pzArgName;
	m_Parts.insert(pzArgName,p);
}


FormPart *CMultiPartForm::AddArgument(const char *pzArgName, int nEstimatedSize/* = 256*/)
{
	FormPart *p = new FormPart(nEstimatedSize);
	p->strPartName = pzArgName;
	m_Parts.insert(pzArgName,p);
	return p;
}
const char *CMultiPartForm::GetLastError(int *pnErrorCode/* = 0*/)
{
	if (pnErrorCode)
		*pnErrorCode = m_nLastError;
	return m_strLastError;
}
int CMultiPartForm::GetAll()
{
	while(ReadNext(0));
	return (m_bFinished) ? 1 : 0; // 1 = success, 0 = error
}

int CMultiPartForm::ReadNext(FormPart **ppPart)
{
	int nReadMore = 0;
RETRY:
	int nSuccess = 0;
	FormPart *pFormPart = 0;
	__int64 bIsDiskFile = 0;
	char *pStart = &m_pzContent[m_pzContentIndex];

	if (memcmp(&pStart[2], m_strBoundary.Buf(), (unsigned int)m_strBoundary.Length()) == 0)
	{
		GString strName;
		GString *pHeader = new GString;
		__int64 nBoundryLen = m_strBoundary.Length();
		if ( pHeader->SetFromUpTo(pStart + nBoundryLen, "\r\n\r\n") == -1)
		{
			if (pStart[nBoundryLen+2] == '-' && pStart[nBoundryLen+3] == '-')
			{
				// we successfully read all the way to the termination boundry marker
				m_bFinished = 1;
				return 0;
			}
			else
			{
				// it is possible that the last network read split the multipart subheader
				// otherwise this is not valid HTTP
				nReadMore++;
			}
		}
		else
		{
			char *p = stristr(pHeader->Buf(),"name=");
			if (p)
			{
				char *pQuote = strpbrk(p,"\"");
				if (pQuote)
				{
					strName.SetFromUpTo(pQuote+1,"\"");
				}
			}
		}

		if (strName.Length())
		{
			pFormPart = (FormPart *)m_Parts.search(strName);
			if (!pFormPart)
			{
				pFormPart = AddArgument(strName);
			}
			char *p = stristr(pHeader->Buf(),"filename=");
			if (p)
			{
				char *pQuote = strpbrk(p,"\"");
				if (pQuote)
				{
					pFormPart->strSourceFileName.SetFromUpTo(pQuote+1,"\"");
				}
			}


			__int64 nOffset = m_strBoundary.Length() + 2 + pHeader->Length() + 2;
			bIsDiskFile = pFormPart->strDestFileName.Length();
			if (bIsDiskFile)
			{
#ifdef _WIN32
				_unlink(pFormPart->strDestFileName);  // if the destination file existed we would otherwise append to it without this line of code - that could be desired behavior in rare cases
#else
				unlink(pFormPart->strDestFileName);  // if the destination file existed we would otherwise append to it without this line of code - that could be desired behavior in rare cases
#endif
			}

			__int64 nLen = m_strEndBoundary.Length();
			while(1)
			{
				if ( memcmp(&pStart[nOffset],m_strEndBoundary, (unsigned int)nLen) == 0)
				{
					// finished.
					m_pzContentIndex += ( nOffset );
					m_pzContentIndex += 2;
					nSuccess = 1;
					break;
				}
				else
				{
					pFormPart->strData->write( &pStart[nOffset++], 1 );
				}


				if (nOffset + m_pzContentIndex  == m_nBytesInBuffer)
				{
					// we could not set it, get more data if there is more
					if ( m_nRemainingData > 0 )
					{
						if (bIsDiskFile)
						{
							if ( !pFormPart->strData->ToFileAppend( pFormPart->strDestFileName, 0) )
							{
								m_strLastError = "Failed to write to file: ";
								m_strLastError += pFormPart->strDestFileName;
								m_nLastError = 1000;
								break;
							}
							else
							{
								*pFormPart->strData = ""; // reset the GString to 0 size, retain the memory allocation
							}
						}
						int nTemp;
						int nRslt = m_pPIC->NextDataChunk(&nTemp);
						m_nBytesInBuffer = nTemp;
						if (nRslt == -1)
						{
							// the connection has been broken
							m_strLastError = "Connection to client has been broken or it timed out";
							m_nLastError = 1001;
							break;
						}
						else if (nRslt == 1)
						{
							m_nRemainingData -= m_nBytesInBuffer;
							pStart = m_pPIC->GetBuffer1();
							pStart[m_nBytesInBuffer] = 0;
							if (m_bOwnMemory)
							{
								delete m_pzContent;
								m_bOwnMemory = 0;
							}
							m_pzContent = pStart;
							nOffset = 0;
							m_pzContentIndex = 0;
							continue;
						}
						else if (nRslt == 0)
						{
							// there is no more data here yet
						}
					}
					break;
				}
			}
		}
	}
	else
	{
		nReadMore++;
	}

	// if m_nBytesInBuffer - m_pzContentIndex is > 512, there is no chance that we split the subheader
	// between network reads, rather this means that the HTTP is malformed
	if (nReadMore == 1 && ((m_nBytesInBuffer - m_pzContentIndex) < 512)  )
	{
		// The following comment is risky code, because it intentionally writes past the end of the system buffer.
		// It's safe as long as MAX_RAW_CHUNK is 16384 or bigger because it uses the grow space
		// defined as (MAX_RAW_CHUNK / 32), but if a server is built with small buffers, this would be bad
		//
		// int nDataSize = 0;
		// NextDataChunk(&nDataSize, &m_pzContent[m_nBytesInBuffer], 512, 60, 0);
		// m_nBytesInBuffer += nDataSize;
		//
		// so we'll do it the way it does not create a kernel build dependancy
		// this copies the tail of the last network read into our memory and sets all indexes relative to the 
		// new memory and now we also have to be sure to delete it, but that's not so simple because it may need to
		// exist until the next multi-part section gets parsed, so the delete has to exist in 3 places, 
		// search this file for "m_bOwnMemory" to see all three places, that could be removed if we depended on the kernel build settings.
		__int64 nBufferTailSize = m_nBytesInBuffer - m_pzContentIndex;
		char *pBuf = new char[ (int)nBufferTailSize + 512 ];
		memcpy(pBuf,&m_pzContent[m_pzContentIndex], (int)nBufferTailSize);
		int nDataSize = 0;
		m_pPIC->NextDataChunk(&nDataSize, &pBuf[nBufferTailSize], 511, 60, 0); // 511 save room for the null
		m_nBytesInBuffer = nDataSize + nBufferTailSize;
		pBuf[m_nBytesInBuffer] = 0;
		if (m_bOwnMemory)
		{
			delete m_pzContent;
		}
		m_bOwnMemory = 1;
		m_pzContent = pBuf;
		m_pzContentIndex = 0;


		// now, we need to try it again
		goto RETRY;
	}
	if (nReadMore > 1)
	{
		m_strLastError = "The format of the HTTP multipart is invalid";
		m_nLastError = 1002;
	}
	
	if (nSuccess)
	{
		if (ppPart)
			*ppPart = pFormPart;

		if (bIsDiskFile)
		{
			if ( !pFormPart->strData->ToFileAppend( pFormPart->strDestFileName, 0) )
			{
				// error writing to disk
			}
			else
			{
				pFormPart->strData->Reset();
			}
		}

		m_strLastError = "The command was successful";
		m_nLastError = 0;
		return 1; // success
	}
	return 0; 
}


