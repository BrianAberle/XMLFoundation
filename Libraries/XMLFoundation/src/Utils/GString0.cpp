// --------------------------------------------------------------------------
//						United Business Technologies
//			  Copyright (c) 2014  All Rights Reserved.
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

#include "GString0.h"
#include "GString32.h"

#include "GException.h"
#include "GDirectory.h" // only for 1 method, CreatePath()
#include "GZip.h"		// for Compress() / DeCompress()
#include "TwoFish.h"	// for Cipher() / DeCipher()
#include "GProfile.h"	// for Cipher() / DeCipher()


#include <string.h> // for: memcpy(), strlen(), strcmp(), memmove(), strchr(), 
					//      strstr(), strncmp()
#include <ctype.h>  // for: isdigit()
#include <stdlib.h> // for: atof(), atoi(), atol(), abs(), and strtol()
#include <stdio.h>  // for: sprintf(),  vsprintf(), FILE, fopen(), fwrite(), 
					//      fclose(), fseek(), fread()
#include <stdarg.h> // for: va_start(), va_end 

#include <wchar.h> // for: wcslen() in linux



unsigned short Ghtons(unsigned short x);
unsigned long Ghtonl(unsigned long x);
unsigned short Gntohs(unsigned short x);
unsigned long Gntohl(unsigned long x);
bool IsNaN(const char *szN,  char decimal_separator, char grouping_separator,  char minus_sign);
int CountOf(const char *szN, char zero_digit);
long round(const char *value);
#ifdef _UNICODE
	wchar_t *DLAsciiToUnicode(const char *a, char *b); 
	unsigned int Gstrlen( const wchar_t *str );
#endif




#define CommonConstruct(nInitialSize)				\
	_str = 0;										\
	_len = 0;										\
    _pWideStr = 0;									\
	_growby = -1;									\
	_strIsOnHeap = 0;								\
	_max = nInitialSize;							\
	if (nInitialSize == GSTRING_INITIAL_SIZE_0)	\
		_str = _initialbuf;							\
	else											\
	{												\
		_initialbuf[0] = 0;							\
		_str = new char[nInitialSize];				\
		_strIsOnHeap = 1;							\
	}												\
	if (!_str)										\
		throw GException("String", 0);				\
	_str[_len] = 0;



void GString0::Attach(const char *pzSource, __int64 nSrcLen, __int64 nMax, int nOwn)
{
	if (_strIsOnHeap == 1)
		delete[] _str;

	_str = (char *)pzSource;
	_len = nSrcLen;
	_max = nMax;
	_strIsOnHeap = (nOwn) ? 1 : 2;
}

void GString0::WriteOn(const char *src,__int64 n)
{
	if (src)
	{
		_len = 0;
		while (n>0)
		{
			_str[_len] = *src; // allows nulls in string
			_len++;
			src++;
			if (_len >= _max)
				resize();
			n--;
		}
		if (_len >= _max)
			resize();
		_str[_len] = 0;
	}
}

__int64 GString0::SetFromUpToFirstOf(const char *pSrc, const char *pzUpToTerm)
{
	char *p = (char *)strpbrk(pSrc,pzUpToTerm);
	if (p)
	{
		WriteOn(pSrc, p - pSrc);
		return p - pSrc;
	}
	return -1;
}

__int64 GString0::AppendFromUpToFirstOf(const char *pSrc, const char *pzUpToTerm)
{
	char *p = (char *)strpbrk(pSrc,pzUpToTerm);
	if (p)
	{
		write(pSrc, p - pSrc);
		return p - pSrc;
	}
	return -1;
}

__int64 GString0::AppendFromUpTo(const char *pSrc, const char *pzUpToTerm)
{
	char *p = (char *)strstr(pSrc,pzUpToTerm);
	if (p)
	{
		write(pSrc, p - pSrc);
		return p - pSrc;
	}
	return -1;
}
__int64 GString0::SetFromUpTo(const char *pSrc, const char *pzUpToTerm)
{
	char *p = (char *)strstr(pSrc,pzUpToTerm);
	if (p)
	{
		WriteOn(pSrc, p - pSrc);
		return p - pSrc;
	}
	return -1;
}


void GString0::AppendEscapeXMLReserved(const char *Src, __int64 nLen/* = -1*/, int nSerializeFlags)
{
	const unsigned char *src = (const unsigned char *)Src;
	
	while (*src && (nLen != 0))
	{
		if (nLen != -1) // -1 means to append until a null in src
			nLen--;

		switch (*src)
		{
			case 13 :
				this->write("&#",2);
				(*this) += (unsigned int)(unsigned char)*src;
				(*this) += ';';
				break;
			case '<' :
				this->write("&lt;",4);
				break;
			case '>' :
				this->write("&gt;",4);
				break;
			case '&' :
				this->write("&amp;",5);
				break;
			default :

				// Note: UNICODE - NEL (133) and LSEP (8232)  will need to be escaped.

			if( ( nSerializeFlags & INTERNAL_ATTRIBUTE_ESCAPE ) != 0 )
			{
				// Note: NL(10) and TAB(9) MUST be escaped in attribute values, they CAN be escaped in element values
				// Likewise with quote(s) which only MUST be escaped when it differs from the outer quotes in an attribute
				if (*src == 9 || *src == 10)
				{
					this->write("&#",2);
					(*this) += (unsigned int)(unsigned char)*src;
					(*this) += ';';
					break;
				}

				// escape double quotes, the attribute value is wrapped in single quotes
				if ( *src == '"' )
				{
					this->write("&#",2);
					(*this) += (unsigned int)(unsigned char)*src;
					(*this) += ';';
					break;
				}
			}


			// http://www.w3.org/TR/2006/REC-xml11-20060816/  says, "If a document is well-formed or valid XML 1.0, and provided 
			// it does not contain any  control characters in the range [#x7F-#x9F] other than as character escapes, 
			// it may be made well-formed or  valid XML 1.1 respectively simply by changing the version number."
				
			// Note: x7F = 127  and   x9F = 159 
			if (  (nSerializeFlags & CONTROL_CHAR_ESCAPE) != 0     &&     (*src > 126) && (*src < 160)      )
			{
				this->write("&#",2);
				(*this) += (unsigned int)(unsigned char)*src;
				(*this) += ';';
				break;
			}


			// Note: chars 1-31 (except CR, TAB, NL) CANNOT occur in XML 1.0.  They CAN occur in XML 1.1 and must be escaped.
			if (  (nSerializeFlags & NO_0_31_CHAR_ESCAPE ) == 0    &&	 (*src < 32)					    )
			{
				this->write("&#",2);
				(*this) += (unsigned int)(unsigned char)*src;
				(*this) += ';';
				break;
			}


			// most cases, append the normal(unescaped) char in [*src] to this GString  (inline operator << code)
			if (_len >= _max)
				resize();
			_str[_len] = *src;
			_len++;

			if (_len >= _max)
				resize();
			_str[_len] = 0;
			break;
		
		}
		src++;
	}
}


void GString0::EscapeXMLReserved()
{
	GString str_xml( Length() + 1024 );
	const unsigned char *src = (const unsigned char *)(const char *)(*this);

	while (*src)
	{
		switch (*src)
		{
			case '<' :
			case '>' :
			case '&' :
			case '"' :
			case '\'' :
				str_xml << "&#" << (unsigned int)*src << ';';				
				break;
			default :
				if ((*src > 127) || (*src < 32))
				{
					str_xml << "&#" << (unsigned int)((unsigned char)*src) << ';';				
				}
				else
				{
					str_xml << *src;
				}
				break;
		}
		src++;
	}

	(*this) = str_xml;
}


void GString0::SetPreAlloc(__int64 l)
{
	if (l <= _max)
		return; // no shrinking, only growing


	_max = l;
	char *nstr = new char[_max];
	
	// not enough memory for resize
	if (!nstr)
		throw GException("String", 0);


	nstr[0] = 0;
	if (_str)
	{
		memcpy(nstr, _str, _len);
		if (_strIsOnHeap == 1)
			delete [] _str;
	}

	nstr[_len] = 0;


	_str = nstr;
	_strIsOnHeap = 1;
	_initialbuf[0] = 0;
}

void GString0::resize()
{
	// Double the size of the buffer.  After much study and research this 'doubling'
	// of the allocation space had proved to be a good generalized guess that works 
	// for most situations.  Using _growby as a default, is a bad idea, it was like 
	// that once.  The GrowBySize was replaced with this doubling - and it has held
	// true for several applications as a good default that can handle big exceptions
	// efficiently, and even allow you to double your way up to Gigabytes - with no 
	// specified prealloc - and it wont punish you like a typical block grow would 
	// for a big exception.
	if (_growby < 1)
	{
		_max <<= 1;
	}
	else
	{
		// and with the year 2014, came full control - this is NOT the default.
		// Sometimes it is better to preallocate 99% of the buffer size then creep 
		// up to value needed, adding one block of [_growby]
		_max += _growby;
	}

	char *nstr = new char[_max];
	
	// not enough memory for resize
	if (!nstr)
		 throw GException("String", 0);

	nstr[0] = 0;
	

	if (_str)
	{
		memcpy(nstr, _str, _len);
		if (_strIsOnHeap == 1)
			delete [] _str;
	}

	_str = nstr;
	_strIsOnHeap = 1;
	_initialbuf[0] = 0;
}

// construct a GString attached to existing memory
GString0::GString0(const char *src, __int64 nCnt, __int64 nMax, int nOwn)
{
	_initialbuf[0] = 0;
	_str = (char *)src;
	_len = nCnt;
	_max = nMax;
	_strIsOnHeap = (nOwn) ? 1 : 2; // 2 is on heap - unowned
    _pWideStr = 0;

	if (_max > _len)
		_str[_len] = 0;
}


GString0::GString0(__int64 nInitialSize,int nGrowByAllocationSize)
{
	CommonConstruct(nInitialSize);
	_growby = nGrowByAllocationSize;
}

// constructs a copy of the source string 
GString0::GString0(const GString &src )
{
	__int64 l = (src._len > GSTRING_INITIAL_SIZE_0) ? src._len + GSTRING_INITIAL_SIZE_0 : GSTRING_INITIAL_SIZE_0;
	CommonConstruct( l );

	_len = ___min(_max, src._len);
	memcpy(_str, src._str, _len);
	_str[_len] = 0;
}

GString0::GString0(const GString &src, __int64 nCnt)
{
	__int64 l = (src._len > GSTRING_INITIAL_SIZE_0) ? src._len + GSTRING_INITIAL_SIZE_0 : GSTRING_INITIAL_SIZE_0;
	CommonConstruct(l);

	_len = ___min(_max, ___min(src._len, nCnt));
	memcpy(_str, src._str, _len);
	_str[_len] = 0;
}

// constructs a copy of the character string
GString0::GString0(const char *src)
{
	__int64 srcLen = (src) ? strlen(src) : GSTRING_INITIAL_SIZE_0;
	__int64 nInitialSize = (srcLen > GSTRING_INITIAL_SIZE_0) ? srcLen + GSTRING_INITIAL_SIZE_0 : GSTRING_INITIAL_SIZE_0;

	CommonConstruct(nInitialSize);

	while (src && *src)
	{
		_str[_len] = *src;
		_len++;
		src++;
		if (_len >= _max)
			resize();
	}
	if (_len >= _max)
		resize();
	_str[_len] = 0;
}

GString0::GString0(const wchar_t *src)
{
	__int64 srcLen = (src) ? wcslen(src) : GSTRING_INITIAL_SIZE_0;
	__int64 nInitialSize = (srcLen > GSTRING_INITIAL_SIZE_0) ? srcLen + GSTRING_INITIAL_SIZE_0 : GSTRING_INITIAL_SIZE_0;

	CommonConstruct(nInitialSize);

	if (src)
	{
		for(__int64 i=0;i<srcLen; i++)
		{
			*this << (char)src[i];
		}
	}
}


GString0::GString0(const char *src, __int64 nCnt)
{
	__int64 nInitialSize = (nCnt > GSTRING_INITIAL_SIZE_0) ? nCnt + GSTRING_INITIAL_SIZE_0 : GSTRING_INITIAL_SIZE_0;

	CommonConstruct(nInitialSize);

	_len = 0;
	if (src)
	{
		__int64 i;
		_len = ___min(_max - 1, nCnt);
		for (i = 0; i < _len; i++)
			_str[i] = src[i]; // allows nulls in string
		_len = i;
	}

	_str[_len] = 0;
}

// constructs a string with ch repeated nCnt times
GString0::GString0(char ch, short nCnt)
{
	short nInitialSize = (nCnt > GSTRING_INITIAL_SIZE_0) ? nCnt + GSTRING_INITIAL_SIZE_0 : GSTRING_INITIAL_SIZE_0;
	CommonConstruct(nInitialSize);

	_len = ___min(_max - 1, nCnt);
	int i;
	for (i = 0; i < _len; i++)
		_str[i] = ch;

	_len = i;
	_str[_len] = 0;
}

GString0::~GString0()
{
	if (_strIsOnHeap == 1)
		delete [] _str;
    if (_pWideStr)
	{
	    free(_pWideStr);
	}
}
void GString0::Reset()
{
	if (_strIsOnHeap == 1)
		delete [] _str;
	CommonConstruct( GSTRING_INITIAL_SIZE_0 );
    if (_pWideStr)
	{
	    free(_pWideStr);
		_pWideStr = 0;
	}
}


GString0 & GString0::operator=(char _p)
{
	if (!_max)
	{
		_max = 2;
		resize();
	}

	_len = 0;
	_str[_len] = _p;
	_len++;
	_str[_len] = 0;

	return *this;
}

GString0 & GString0::operator=(unsigned __int64 _p)
{
	char  szBuffer[33];

	Xi64toa ( _p,szBuffer);

	__int64 nLen = strlen(szBuffer);
	for (_len = 0; _len < nLen; _len++)
	{
		if (_len >= _max)
			resize();
		_str[_len] = szBuffer[_len];
	}

	if (_len >= _max)
		resize();
	_str[_len] = 0;

	return *this;
}

GString0 & GString0::operator=(__int64 _p)
{
	char  szBuffer[33];

	Xi64toa ( _p,szBuffer,10);

	__int64 nLen = strlen(szBuffer);
	for (_len = 0; _len < nLen; _len++)
	{
		if (_len >= _max)
			resize();
		_str[_len] = szBuffer[_len];
	}

	if (_len >= _max)
		resize();
	_str[_len] = 0;

	return *this;
}

GString0 & GString0::operator=(const char * _p)
{
	_len = 0;
	while (_p && *_p)
	{
		if (_len >= _max)
			resize();
		_str[_len] = *_p;
		_p++;
		_len++;
	}

	if (_len >= _max)
		resize();
	_str[_len] = 0;

	return *this;
}

GString0 & GString0::operator=(const signed char * _p)
{
	*this = (const char *)_p;

	return *this;
}

GString0 & GString0::operator=(unsigned char _p)
{
	*this = (char)_p;

	return *this;
}

GString0 & GString0::operator=(signed char _p)
{
	*this = (char)_p;

	return *this;
}

GString0 & GString0::operator=(short _p)
{
	char  szBuffer[25];
	sprintf(szBuffer, "%hi", _p);

	__int64 nLen = strlen(szBuffer);
	for (_len = 0; _len < nLen; _len++)
	{
		if (_len >= _max)
			resize();
		_str[_len] = szBuffer[_len];
	}

	if (_len >= _max)
		resize();
	_str[_len] = 0;

	return *this;
}

GString0 & GString0::operator=(unsigned short _p)
{
	char  szBuffer[25];
	sprintf(szBuffer, "%hu", _p);

	__int64 nLen = strlen(szBuffer);
	for (_len = 0; _len < nLen; _len++)
	{
		if (_len >= _max)
			resize();
		_str[_len] = szBuffer[_len];
	}

	if (_len >= _max)
		resize();
	_str[_len] = 0;

	return *this;
}

GString0 & GString0::operator=(int _p)
{
	char  szBuffer[25];
	sprintf(szBuffer, "%i", _p);

	__int64 nLen = strlen(szBuffer);
	for (_len = 0; _len < nLen; _len++)
	{
		if (_len >= _max)
			resize();
		_str[_len] = szBuffer[_len];
	}

	if (_len >= _max)
		resize();
	_str[_len] = 0;

	return *this;
}

GString0 & GString0::operator=(unsigned int _p)
{
	char  szBuffer[25];
	sprintf(szBuffer, "%u", _p);

	__int64 nLen = strlen(szBuffer);
	for (_len = 0; _len < nLen; _len++)
	{
		if (_len >= _max)
			resize();
		_str[_len] = szBuffer[_len];
	}

	if (_len >= _max)
		resize();
	_str[_len] = 0;

	return *this;
}

GString0 & GString0::operator=(long _p)
{
	char  szBuffer[25];
	sprintf(szBuffer, "%li", _p);

	__int64 nLen = strlen(szBuffer);
	for (_len = 0; _len < nLen; _len++)
	{
		if (_len >= _max)
			resize();
		_str[_len] = szBuffer[_len];
	}

	if (_len >= _max)
		resize();
	_str[_len] = 0;

	return *this;
}

GString0 & GString0::operator=(unsigned long _p)
{
	char  szBuffer[25];
	sprintf(szBuffer, "%lu", _p);

	__int64 nLen = strlen(szBuffer);
	for (_len = 0; _len < nLen; _len++)
	{
		if (_len >= _max)
			resize();
		_str[_len] = szBuffer[_len];
	}

	if (_len >= _max)
		resize();
	_str[_len] = 0;

	return *this;
}

GString0 & GString0::operator=(float _p)
{
	char  szBuffer[50];
	sprintf(szBuffer, GString::g_FloatFmt._str, _p);
	__int64 nLen = strlen(szBuffer);

	for (_len = 0; _len < nLen; _len++)
	{
		if (_len >= _max)
			resize();
		_str[_len] = szBuffer[_len];
	}

	if (_len >= _max)
		resize();
	_str[_len] = 0;

	return *this;
}



GString0 & GString0::operator=(double _p)
{
	char  szBuffer[50];
	sprintf(szBuffer, "%.6g", _p);
	__int64 nLen = strlen(szBuffer);

	for (_len = 0; _len < nLen; _len++)
	{
		if (_len >= _max)
			resize();
		_str[_len] = szBuffer[_len];
	}

	if (_len >= _max)
		resize();
	_str[_len] = 0;

	return *this;
}

GString0 & GString0::operator=(long double _p)
{
	char  szBuffer[50];
	sprintf(szBuffer, "%.6Lg", _p);
	__int64 nLen = strlen(szBuffer);

	for (_len = 0; _len < nLen; _len++)
	{
		if (_len >= _max)
			resize();
		_str[_len] = szBuffer[_len];
	}

	if (_len >= _max)
		resize();
	_str[_len] = 0;

	return *this;
}

GString0 & GString0::operator=(const GString & _p)
{

	for (_len = 0; _len < _p._len; _len++)
	{
		if (_len >= _max)
			resize();
		_str[_len] = _p._str[_len];
	}
	if (_len >= _max)
		resize();
	_str[_len] = 0;

	return *this;
}
GString0 & GString0::operator=(const GString0 & _p)
{

	for (_len = 0; _len < _p._len; _len++)
	{
		if (_len >= _max)
			resize();
		_str[_len] = _p._str[_len];
	}
	if (_len >= _max)
		resize();
	_str[_len] = 0;

	return *this;
}

GString0 & GString0::operator=(const GString32 & _p)
{

	for (_len = 0; _len < _p._len; _len++)
	{
		if (_len >= _max)
			resize();
		_str[_len] = _p._str[_len];
	}
	if (_len >= _max)
		resize();
	_str[_len] = 0;

	return *this;
}




GString0 & GString0::operator+=(unsigned __int64 _p)
{
	char  szBuffer[33];

	Xi64toa ( _p,szBuffer);

	int nLen = strlen(szBuffer);
	for (int i = 0; i < nLen; i++, _len++)
	{
		if (_len >= _max)
			resize();
		_str[_len] = szBuffer[i];
	}

	if (_len >= _max)
		resize();
	_str[_len] = 0;

	return *this;
}



GString0 & GString0::operator+=(__int64 _p)
{
	char  szBuffer[33];

	Xi64toa ( _p,szBuffer,10);

	int nLen = strlen(szBuffer);
	for (int i = 0; i < nLen; i++, _len++)
	{
		if (_len >= _max)
			resize();
		_str[_len] = szBuffer[i];
	}

	if (_len >= _max)
		resize();
	_str[_len] = 0;

	return *this;
}



GString0 & GString0::operator+=(const signed char * _p)
{
	while (_p && *_p)
	{
		if (_len >= _max)
			resize();
		_str[_len] = *_p;
		_p++;
		_len++;
	}


	if (_len >= _max)
		resize();
	_str[_len] = 0;

	return *this;
}

GString0 & GString0::operator+=(const char * _p)
{
	while (_p && *_p)
	{
		if (_len >= _max)
			resize();
		_str[_len] = *_p;
		_p++;
		_len++;
	}

	if (_len >= _max)
		resize();
	_str[_len] = 0;

	return *this;
}


void GString0::write(const char *p,__int64 nSize)
{
	// this is a tough call because either way can be more optimal under certain conditions
	// A.)alloc what we need in a single pass.  Imagine you created a GString then write()
	// 10Meg into it, this is what you would want.  1 alloc, no more than you need.
	// SetPreAlloc(_len + nSize + 1);

	// however write is rarely used like that, in ostream it's an append operation and
	// one append is often followed by another so it's wise to allocate more than you need
	while (_len + nSize + 1 >= _max)
		resize();

	// so the caller needs to be wise about using this, call SetPreAlloc() yourself for large writes.

	memcpy(&_str[_len],p,nSize);
	_len += nSize;
	_str[_len] = 0;
}

GString0 & GString0::operator+=(char _p)
{
	if (_p)
	{
		if (_len >= _max)
			resize();
		_str[_len] = _p;
		_len++;

		if (_len >= _max)
			resize();
		_str[_len] = 0;
	}

	return *this;
}

GString0 & GString0::operator+=(unsigned char _p)
{
	if (_p)
	{
		if (_len >= _max)
			resize();
		_str[_len] = _p;
		_len++;

		if (_len >= _max)
			resize();
		_str[_len] = 0;
	}

	return *this;
}

GString0 & GString0::operator+=(signed char _p)
{
	if (_p)
	{
		if (_len >= _max)
			resize();
		_str[_len] = _p;
		_len++;

		if (_len >= _max)
			resize();
		_str[_len] = 0;
	}

	return *this;
}

GString0 & GString0::operator+=(short _p)
{
	char  szBuffer[25];
	sprintf(szBuffer, "%hi", _p);

	int nLen = strlen(szBuffer);
	for (int i = 0; i < nLen; i++, _len++)
	{
		if (_len >= _max)
			resize();
		_str[_len] = szBuffer[i];
	}

	if (_len >= _max)
		resize();
	_str[_len] = 0;

	return *this;
}

GString0 & GString0::operator+=(unsigned short _p)
{
	char  szBuffer[25];
	sprintf(szBuffer, "%hu", _p);

	int nLen = strlen(szBuffer);
	for (int i = 0; i < nLen; i++, _len++)
	{
		if (_len >= _max)
			resize();
		_str[_len] = szBuffer[i];
	}

	if (_len >= _max)
		resize();
	_str[_len] = 0;

	return *this;
}

GString0 & GString0::operator+=(int _p)
{
	char  szBuffer[25];
	sprintf(szBuffer, "%i", _p);

	int nLen = strlen(szBuffer);
	for (int i = 0; i < nLen; i++, _len++)
	{
		if (_len >= _max)
			resize();
		_str[_len] = szBuffer[i];
	}

	if (_len >= _max)
		resize();
	_str[_len] = 0;

	return *this;
}

GString0 & GString0::operator+=(unsigned int _p)
{
	char  szBuffer[25];
	sprintf(szBuffer, "%u", _p);

	int nLen = strlen(szBuffer);
	for (int i = 0; i < nLen; i++, _len++)
	{
		if (_len >= _max)
			resize();
		_str[_len] = szBuffer[i];
	}

	if (_len >= _max)
		resize();
	_str[_len] = 0;

	return *this;
}

GString0 & GString0::operator+=(long _p)
{
	char  szBuffer[25];
	sprintf(szBuffer, "%ld", _p);

	int nLen = strlen(szBuffer);
	for (int i = 0; i < nLen; i++, _len++)
	{
		if (_len >= _max)
			resize();
		_str[_len] = szBuffer[i];
	}

	if (_len >= _max)
		resize();
	_str[_len] = 0;

	return *this;
}

GString0 & GString0::operator+=(unsigned long _p)
{
	char  szBuffer[25];
	sprintf(szBuffer, "%lu", _p);
	int nLen = strlen(szBuffer);

	for (int i = 0; i < nLen; i++, _len++)
	{
		if (_len >= _max)
			resize();
		_str[_len] = szBuffer[i];
	}

	if (_len >= _max)
		resize();
	_str[_len] = 0;



	return *this;
}

GString0 & GString0::operator+=(float _p)
{
	char  szBuffer[50];
	sprintf(szBuffer, GString::g_FloatFmt._str, _p);
	int nLen = strlen(szBuffer);


	for (int i = 0; i < nLen; i++, _len++)
	{
		if (_len >= _max)
			resize();
		_str[_len] = szBuffer[i];
	}

	if (_len >= _max)
		resize();
	_str[_len] = 0;


	return *this;
}

GString0 & GString0::operator+=(double _p)
{
	// For a 64-bit IEEE double - max chars is 1079
	char  szBuffer[1080];
	sprintf(szBuffer, "%lf", _p);

	int nLen = strlen(szBuffer);


	for (int i = 0; i < nLen; i++, _len++)
	{
		if (_len >= _max)
			resize();
		_str[_len] = szBuffer[i];
	}

	if (_len >= _max)
		resize();
	_str[_len] = 0;


	return *this;
}

GString0 & GString0::operator+=(long double _p)
{
	char  szBuffer[50];
	sprintf(szBuffer, "%Lf", _p);
	int nLen = strlen(szBuffer);


	for (int i = 0; i < nLen; i++, _len++)
	{
		if (_len >= _max)
			resize();
		_str[_len] = szBuffer[i];
	}

	if (_len >= _max)
		resize();
	_str[_len] = 0;


	return *this;
}

GString0 & GString0::operator+=(const GString & _p)
{
	while (_len + _p._len + 1 >= _max)
		resize();


	memcpy(&_str[_len],_p._str,_p._len);
	_len += _p._len;
	_str[_len] = 0;
	return *this;
}
GString0 & GString0::operator+=(const GString0 & _p)
{
	while (_len + _p._len + 1 >= _max)
		resize();


	memcpy(&_str[_len],_p._str,_p._len);
	_len += _p._len;
	_str[_len] = 0;
	return *this;
}
GString0 & GString0::operator+=(const GString32 & _p)
{
	while (_len + _p._len + 1 >= _max)
		resize();


	memcpy(&_str[_len],_p._str,_p._len);
	_len += _p._len;
	_str[_len] = 0;
	return *this;
}

GString0 & GString0::operator<<(__int64 _p)
{
	char  szBuffer[33];

	Xi64toa ( _p,szBuffer,10);

	int nLen = strlen(szBuffer);
	for (int i = 0; i < nLen; i++, _len++)
	{
		if (_len >= _max)
			resize();
		_str[_len] = szBuffer[i];
	}

	if (_len >= _max)
		resize();
	_str[_len] = 0;

	return *this;
}

GString0 & GString0::operator<<(unsigned __int64 _p)
{
	char  szBuffer[33];

	Xi64toa ( _p,szBuffer);

	int nLen = strlen(szBuffer);
	for (int i = 0; i < nLen; i++, _len++)
	{
		if (_len >= _max)
			resize();
		_str[_len] = szBuffer[i];
	}

	if (_len >= _max)
		resize();
	_str[_len] = 0;

	return *this;
}

GString0 & GString0::operator<<(const char * _p)
{
	if (_p)
	{
		while (*_p)
		{
			if (_len >= _max)
				resize();
			_str[_len] = *_p;
			_p++;
			_len++;
		}
	}

	if (_len >= _max)
		resize();
	_str[_len] = 0;

	return *this;
}

GString0 & GString0::operator<<(const signed char * _p)
{
	return *this += _p;
}

GString0 & GString0::operator<<(char _p)
{
	if (_p)
	{
		if (_len >= _max)
			resize();
		_str[_len] = _p;
		_len++;

		if (_len >= _max)
			resize();
		_str[_len] = 0;
	}
	return *this;
}

GString0 & GString0::operator<<(unsigned char _p)
{
	if (_p)
	{
		if (_len >= _max)
			resize();
		_str[_len] = _p;
		_len++;

		if (_len >= _max)
			resize();
		_str[_len] = 0;
	}
	return *this;
}

GString0 & GString0::operator<<(signed char _p)
{
	if (_p)
	{
		if (_len >= _max)
			resize();
		_str[_len] = _p;
		_len++;

		if (_len >= _max)
			resize();
		_str[_len] = 0;
	}
	return *this;
}

GString0 & GString0::operator<<(short _p)
{
	char  szBuffer[25];
	sprintf(szBuffer, "%hi", _p);

	int nLen = strlen(szBuffer);
	for (int i = 0; i < nLen; i++, _len++)
	{
		if (_len >= _max)
			resize();
		_str[_len] = szBuffer[i];
	}

	if (_len >= _max)
		resize();
	_str[_len] = 0;

	return *this;
}

GString0 & GString0::operator<<(unsigned short _p)
{
	char  szBuffer[25];
	sprintf(szBuffer, "%hu", _p);

	int nLen = strlen(szBuffer);
	for (int i = 0; i < nLen; i++, _len++)
	{
		if (_len >= _max)
			resize();
		_str[_len] = szBuffer[i];
	}

	if (_len >= _max)
		resize();
	_str[_len] = 0;

	return *this;
}

GString0 & GString0::operator<<(int _p)
{
	char  szBuffer[25];
	sprintf(szBuffer, "%i", _p);

	int nLen = strlen(szBuffer);
	for (int i = 0; i < nLen; i++, _len++)
	{
		if (_len >= _max)
			resize();
		_str[_len] = szBuffer[i];
	}

	if (_len >= _max)
		resize();
	_str[_len] = 0;

	return *this;
}

GString0 & GString0::operator<<(unsigned int _p)
{
	char  szBuffer[25];
	sprintf(szBuffer, "%u", _p);

	int nLen = strlen(szBuffer);
	for (int i = 0; i < nLen; i++, _len++)
	{
		if (_len >= _max)
			resize();
		_str[_len] = szBuffer[i];
	}

	if (_len >= _max)
		resize();
	_str[_len] = 0;

	return *this;
}

GString0 & GString0::operator<<(long _p)
{
	char  szBuffer[25];
	sprintf(szBuffer, "%ld", _p);

	int nLen = strlen(szBuffer);
	for (int i = 0; i < nLen; i++, _len++)
	{
		if (_len >= _max)
			resize();
		_str[_len] = szBuffer[i];
	}

	if (_len >= _max)
		resize();
	_str[_len] = 0;

	return *this;
}

GString0 & GString0::operator<<(unsigned long _p)
{
	char  szBuffer[25];
	sprintf(szBuffer, "%lu", _p);
	int nLen = strlen(szBuffer);

	for (int i = 0; i < nLen; i++, _len++)
	{
		if (_len >= _max)
			resize();
		_str[_len] = szBuffer[i];
	}

	if (_len >= _max)
		resize();
	_str[_len] = 0;

	return *this;
}

GString0 & GString0::operator<<(float _p)
{
	char  szBuffer[50];
	sprintf(szBuffer, GString::g_FloatFmt._str, _p);
	int nLen = strlen(szBuffer);


	for (int i = 0; i < nLen; i++, _len++)
	{
		if (_len >= _max)
			resize();
		_str[_len] = szBuffer[i];
	}

	if (_len >= _max)
		resize();
	_str[_len] = 0;

	return *this;
}

GString0 & GString0::operator<<(double _p)
{
	// For a 64-bit IEEE double - max chars is 1079
	char  szBuffer[1080];
	sprintf(szBuffer, "%lf", _p);
	int nLen = strlen(szBuffer);

	for (int i = 0; i < nLen; i++, _len++)
	{
		if (_len >= _max)
			resize();
		_str[_len] = szBuffer[i];
	}

	if (_len >= _max)
		resize();
	_str[_len] = 0;
	return *this;
}

GString0 & GString0::operator<<(long double _p)
{
	char  szBuffer[50];
	sprintf(szBuffer, "%Lf", _p);
	int nLen = strlen(szBuffer);


	for (int i = 0; i < nLen; i++, _len++)
	{
		if (_len >= _max)
			resize();
		_str[_len] = szBuffer[i];
	}

	if (_len >= _max)
		resize();
	_str[_len] = 0;
	return *this;
}


GString0 & GString0::operator<<(const GString & _p)
{
	while (_len + _p._len + 1 >= _max)
		resize();


	memcpy(&_str[_len],_p._str,_p._len);
	_len += _p._len;
	_str[_len] = 0;
	return *this;
}

GString0 & GString0::operator<<(const GString0 & _p)
{
	while (_len + _p._len + 1 >= _max)
		resize();


	memcpy(&_str[_len],_p._str,_p._len);
	_len += _p._len;
	_str[_len] = 0;
	return *this;
}

GString0 & GString0::operator<<(const GString32 & _p)
{
	while (_len + _p._len + 1 >= _max)
		resize();


	memcpy(&_str[_len],_p._str,_p._len);
	_len += _p._len;
	_str[_len] = 0;
	return *this;
}


GString operator+(GString0 &_p1, GString0 &_p2)
{
	GString strRet(_p1._len + _p2._len + 1);
	__int64 i;
	for (i = 0; i < _p1._len; strRet._len++, i++)
		strRet._str[strRet._len] = _p1._str[i];

	for (i = 0; i < _p2._len; strRet._len++, i++)
		strRet._str[strRet._len] = _p2._str[i];

	strRet._str[strRet._len] = 0;

	return strRet;
}

GString operator+(GString0 &_p1, const char *_p2)
{
	GString strRet;

	for (__int64 i = 0; i < _p1._len; strRet._len++, i++)
	{
		if (strRet._len >= strRet._max)
			strRet.resize();
		strRet._str[strRet._len] = _p1._str[i];
	}

	while (_p2 && *_p2)
	{
		if (strRet._len >= strRet._max)
			strRet.resize();
		strRet._str[strRet._len] = *_p2;
		_p2++;
		strRet._len++;
	}

	if (strRet._len >= strRet._max)
		strRet.resize();
	strRet._str[strRet._len] = 0;

	return strRet;
}

GString operator+(const char *_p1, GString0 &_p2)
{
	GString strRet;

	while (_p1 && *_p1)
	{
		if (strRet._len >= strRet._max)
			strRet.resize();
		strRet._str[strRet._len] = *_p1;
		_p1++;
		strRet._len++;
	}

	for (__int64 i = 0; i < _p2._len; strRet._len++, i++)
	{
		if (strRet._len >= strRet._max)
			strRet.resize();
		strRet._str[strRet._len] = _p2._str[i];
	}

	if (strRet._len >= strRet._max)
		strRet.resize();
	strRet._str[strRet._len] = 0;

	return strRet;
}

GString operator+(GString0 &_p1, const signed char *_p2)
{
	GString strRet;

	for (__int64 i = 0; i < _p1._len; strRet._len++, i++)
	{
		if (strRet._len >= strRet._max)
			strRet.resize();
		strRet._str[strRet._len] = _p1._str[i];
	}

	while (_p2 && *_p2)
	{
		if (strRet._len >= strRet._max)
			strRet.resize();
		strRet._str[strRet._len] = *_p2;
		_p2++;
		strRet._len++;
	}

	if (strRet._len >= strRet._max)
		strRet.resize();
	strRet._str[strRet._len] = 0;

	return strRet;
}

GString operator+(const signed char *_p1, GString0 &_p2)
{
	GString strRet;

	while (_p1 && *_p1)
	{
		if (strRet._len >= strRet._max)
			strRet.resize();
		strRet._str[strRet._len] = *_p1;
		_p1++;
		strRet._len++;
	}

	for (__int64 i = 0; i < _p2._len; strRet._len++, i++)
	{
		if (strRet._len >= strRet._max)
			strRet.resize();
		strRet._str[strRet._len] = _p2._str[i];
	}

	if (strRet._len >= strRet._max)
		strRet.resize();
	strRet._str[strRet._len] = 0;

	return strRet;
}

GString operator+(GString0 &_p1, char _p2)
{
	GString strRet(_p1._len + 2);

	for (__int64 i = 0; i < _p1._len; strRet._len++, i++)
		strRet._str[strRet._len] = _p1._str[i];

	strRet._str[strRet._len] = _p2;
	strRet._len++;
	strRet._str[strRet._len] = 0;

	return strRet;
}

GString operator+(char _p1, GString0 &_p2)
{
	GString strRet(_p2._len + 2);

	strRet._str[strRet._len] = _p1;
	strRet._len++;

	for (__int64 i = 0; i < _p2._len; strRet._len++, i++)
		strRet._str[strRet._len] = _p2._str[i];

	strRet._str[strRet._len] = 0;

	return strRet;
}

GString operator+(GString0 &_p1, unsigned char _p2)
{
	GString strRet(_p1._len + 2);

	for (__int64 i = 0; i < _p1._len; strRet._len++, i++)
		strRet._str[strRet._len] = _p1._str[i];

	strRet._str[strRet._len] = _p2;
	strRet._len++;
	strRet._str[strRet._len] = 0;

	return strRet;
}

GString operator+(unsigned char _p1, GString0 &_p2)
{
	GString strRet(_p2._len + 2);

	strRet._str[strRet._len] = _p1;
	strRet._len++;

	for (__int64 i = 0; i < _p2._len; strRet._len++, i++)
		strRet._str[strRet._len] = _p2._str[i];

	strRet._str[strRet._len] = 0;

	return strRet;
}

GString operator+(GString0 &_p1, signed char _p2)
{
	GString strRet(_p1._len + 2);

	for (__int64 i = 0; i < _p1._len; strRet._len++, i++)
		strRet._str[strRet._len] = _p1._str[i];

	strRet._str[strRet._len] = _p2;
	strRet._len++;
	strRet._str[strRet._len] = 0;

	return strRet;
}

GString operator+(signed char _p1, GString0 &_p2)
{
	GString strRet(_p2._len + 2);

	strRet._str[strRet._len] = _p1;
	strRet._len++;

	for (__int64 i = 0; i < _p2._len; strRet._len++, i++)
		strRet._str[strRet._len] = _p2._str[i];

	strRet._str[strRet._len] = 0;

	return strRet;
}

//ostream& operator<<(ostream &os, GString &s)
//{
//	os << s._str;
//	return os;
//}

int GString0::operator >  (const GString &s) const
{
	return strcmp(_str, s._str) > 0;
}

int GString0::operator >= (const GString &s) const
{
	return strcmp(_str, s._str) >= 0;
}

int GString0::operator == (const GString &s) const
{
	return strcmp(_str, s._str) == 0;
}

int GString0::operator <  (const GString &s) const
{
	return strcmp(_str, s._str) < 0;
}

int GString0::operator <= (const GString &s) const
{
	return strcmp(_str, s._str) <= 0;
}

int GString0::operator != (const GString &s) const
{
	return strcmp(_str, s._str) != 0;
}



int GString0::operator >  (const GString0 &s) const
{
	return strcmp(_str, s._str) > 0;
}

int GString0::operator >= (const GString0 &s) const
{
	return strcmp(_str, s._str) >= 0;
}

int GString0::operator == (const GString0 &s) const
{
	return strcmp(_str, s._str) == 0;
}

int GString0::operator <  (const GString0 &s) const
{
	return strcmp(_str, s._str) < 0;
}

int GString0::operator <= (const GString0 &s) const
{
	return strcmp(_str, s._str) <= 0;
}

int GString0::operator != (const GString0 &s) const
{
	return strcmp(_str, s._str) != 0;
}





int GString0::operator >  (const GString32 &s) const
{
	return strcmp(_str, s._str) > 0;
}

int GString0::operator >= (const GString32 &s) const
{
	return strcmp(_str, s._str) >= 0;
}

int GString0::operator == (const GString32 &s) const
{
	return strcmp(_str, s._str) == 0;
}

int GString0::operator <  (const GString32 &s) const
{
	return strcmp(_str, s._str) < 0;
}

int GString0::operator <= (const GString32 &s) const
{
	return strcmp(_str, s._str) <= 0;
}

int GString0::operator != (const GString32 &s) const
{
	return strcmp(_str, s._str) != 0;
}




int GString0::operator >  (const char *s) const
{
	return strcmp(_str, s) > 0;
}

int GString0::operator >= (const char *s) const
{
	return strcmp(_str, s) >= 0;
}

int GString0::operator == (const char *s) const
{
	return strcmp(_str, s) == 0;
}

int GString0::operator <  (const char *s) const
{
	return strcmp(_str, s) < 0;
}

int GString0::operator <= (const char *s) const
{
	return strcmp(_str, s) <= 0;
}

int GString0::operator != (const char *s) const
{
	return strcmp(_str, s) != 0;
}

int GString0::Compare(const char *s) const
{
	return strcmp(_str, s);
}
int GString0::Compare(const char *s, int n) const
{
	return memcmp(_str, s, n);
}

int GString0::Compare(GString &s) const
{
	return strcmp(_str, s._str);
}

int GString0::CompareNoCase(const char *s, __int64 n) const
{
	__int64 i = 0;  // start at the beginning

	// loop through the two strings comparing case insensitively
	while ((toupper(_str[i]) == toupper(s[i])) &&  // the two strings are equal
		   (_str[i] != '\0') && i < n)             // the first hasn't ended
		i++;

	if (i == n)		// the strings were equal for the first n bytes
		return 0;

	// the difference between the characters that ended it is
	// indicative of the direction of the comparison.  if this
	// is negative, the first was before the second.  if it is
	// positive, the first was after the second.  if it is zero,
	// the two are equal (and the != '\0' condition stopped the
	// loop such that the two were of equal length).
	return (short)toupper(_str[i]) - (short)toupper(s[i]);
}


int GString0::CompareNoCase(const char *s) const
{
	__int64 i = 0;  // start at the beginning

	// loop through the two strings comparing case insensitively
	while ((toupper(_str[i]) == toupper(s[i])) &&  // the two strings are equal
		   (_str[i] != '\0'))                      // the first hasn't ended
		i++;

	// the difference between the characters that ended it is
	// indicative of the direction of the comparison.  if this
	// is negative, the first was before the second.  if it is
	// positive, the first was after the second.  if it is zero,
	// the two are equal (and the != '\0' condition stopped the
	// loop such that the two were of equal length).
	return (short)toupper(_str[i]) - (short)toupper(s[i]);
}

int GString0::CompareNoCase(const GString &s) const
{
	__int64 i = 0;  // start at the beginning

	// loop through the two strings comparing case insensitively
	while ((toupper(_str[i]) == toupper(s._str[i])) &&  // the two strings are equal
		   (_str[i] != '\0'))							// the first hasn't ended
		i++;

	// the difference between the characters that ended it is
	// indicative of the direction of the comparison.  if this
	// is negative, the first was before the second.  if it is
	// positive, the first was after the second.  if it is zero,
	// the two are equal (and the != '\0' condition stopped the
	// loop such that the two were of equal length).
	return (short)toupper(_str[i]) - (short)toupper(s._str[i]);
}

int GString0::CompareSub(const char *s, __int64 i, __int64 z) const
{
	char ch = 0;
	if (z != -1 && z <= _len)
	{
		ch = _str[z];
		_str[z] = 0;
	}
	__int64 nRet = strcmp(&_str[i], s);
	if (z != -1 && z <= _len)
	{
		_str[z] = ch;
	}
	return nRet;
}


int GString0::CompareSubNoCase(const char *s, __int64 i, __int64 z) const
{
	__int64 j = 0;
	// loop through the two strings comparing case insensitively
	while ((toupper(_str[i]) == toupper(s[j])) &&  // the two strings are equal
		(_str[i] != '\0'))							// the first hasn't ended
	{                      
		i++;
		j++;
		if (z > 0 && i == z)
			return 0;
	}

	// the difference between the characters that ended it is
	// indicative of the direction of the comparison.  if this
	// is negative, the first was before the second.  if it is
	// positive, the first was after the second.  if it is zero,
	// the two are equal (and the != '\0' condition stopped the
	// loop such that the two were of equal length).
	return (short)toupper(_str[i]) - (short)toupper(s[j]);
}

int GString0::CompareToFile(const char *pzFileAndPath, int nMatchCase)
{
	GString strTemp;
	strTemp.FromFile(pzFileAndPath,0);
	if (nMatchCase == 1)
	{
		return Compare(strTemp); // nCase = 0 is exact case match

	}
	return CompareNoCase(strTemp);
}



char& GString0::operator[](__int64 nIdx)
{
	// check for subscript out of range
	if (nIdx > _len)
		throw GException("String", 1);

	return _str[nIdx];
}


char& GString0::operator[](int nIdx)
{
	// check for subscript out of range
	if (nIdx > _len)
		throw GException("String", 1);

	return _str[nIdx];
}


char GString0::GetAt(__int64 nIdx) const
{
	// check for subscript out of range
	if (nIdx > _len)
		throw GException("String", 1);

	return _str[nIdx];
}

void GString0::SetAt(__int64 nIdx, char ch)
{
	// check for subscript out of range
	// note: if you are wanting to set a null one byte past _len and your nIdx is safely less than _max use SetLength()
	if (nIdx >= _len)
		throw GException("String", 1);

	_str[nIdx] = ch;
}

void GString0::PadLeft(__int64 nCnt, char ch /* = ' ' */)
{
	if (nCnt > _len)
		Prepend(nCnt - _len, ch);
}

void GString0::Prepend(__int64 nCnt, char ch /* = ' ' */)
{
	while (nCnt + _len + 1 >= _max)
		resize();

	memmove(&_str[nCnt], _str, _len);
	_len += nCnt;

	for (__int64 i = 0; i < nCnt; i++)
		_str[i] = ch;

	_str[_len] = 0;
}

// Copy nBytes from pSrc or up to the NULL in pSrc if nBytes = -1 onto the beginning of 'this'
void GString0::Prepend(const char *pSrc, int nBytes)
{
	Insert(0,pSrc,nBytes);
}
// Copy strSource onto the beginning of 'this'
void GString0::Prepend(GString &strSource)
{
	Insert(0,strSource._str,strSource._len);
}
// Copy the character value from native integers onto the beginning of 'this'
void GString0::Prepend(__int64 n)
{
	GString strTemp;
	strTemp << n;
	Insert(0,strTemp._str,strTemp._len);
}


void GString0::TrimLeftBytes(__int64 i)
{
	if (i != 0)
	{
		_len -= i;
		memmove(_str, &_str[i], _len + 1);
	}
}

void GString0::TrimLeft(char ch /* = ' ' */, short nCnt /* = -1 */)
{
	int i = 0;
	while ((i < _len) && (_str[i] == ch) && (nCnt != 0))
	{
		i++;
		nCnt--;
	}

	if (i != 0)
	{
		_len -= i;
		memmove(_str, &_str[i], _len + 1);
	}
}

#define isWS(ch) ( ch == 0x20 || ch == 0x09 || ch == 0x0D || ch == 0x0A)
void GString0::TrimLeftWS()
{
	__int64 i = 0;
	while ((i < _len) && (isWS(_str[i])))
		i++;

	if (i != 0)
	{
		_len -= i;
		memmove(_str, &_str[i], _len + 1);
	}
}

void GString0::PadRight(__int64 nCnt, char ch /* = ' ' */)
{
	if (nCnt > _len)
		Append(nCnt - _len, ch);
}

void GString0::Append(__int64 nCnt, char ch /* = ' ' */)
{
	for (__int64 i = 0; i < nCnt; i++, _len++)
	{
		if (_len >= _max)
			resize();
		_str[_len] = ch;
	}

	if (_len >= _max)
		resize();
	_str[_len] = 0;
}

void GString0::TrimRightBytes(__int64 nCnt)
{
	if (_len >= nCnt)
		_len -= nCnt;
	_str[_len] = 0;
}

void GString0::TrimRight(char ch /* = ' ' */, short nCnt /* = -1 */)
{
	while ((_len) && (_str[_len - 1] == ch) && (nCnt != 0))
	{
		_len--;
		nCnt--;
	}

	_str[_len] = 0;
}

GString GString0::Left (__int64 nCnt) const
{
	if (nCnt > _len)
		nCnt = _len;

	return GString(_str, nCnt);
}

GString GString0::Mid  (__int64 nFirst) const
{
	if (nFirst > _len)
		nFirst = _len;

	return GString(&_str[nFirst]);
}

GString GString0::Mid  (__int64 nFirst, __int64 nCnt) const
{
	if (nFirst > _len)
		nFirst = _len;

	return GString(&_str[nFirst], nCnt);
}

GString GString0::Right(__int64 nCnt) const
{
	if (nCnt > _len)
		nCnt = _len + 1;

	__int64 nFirst = _len - nCnt;
	return GString(&_str[nFirst]);
}

void GString0::TrimRightWS()
{
	while ((_len) && (isWS(_str[_len - 1])))
		_len--;

	_str[_len] = 0;
}

void GString0::MakeUpper(__int64 nBeginIndex/*=0*/)
{
	for (__int64 i = nBeginIndex; i < _len; i++)
		_str[i] = toupper(_str[i]);
}

void GString0::MakeLower(__int64 nBeginIndex/*=0*/)
{
	for (__int64 i = nBeginIndex; i < _len; i++)
		_str[i] = tolower(_str[i]);
}

void GString0::MakeReverse()
{
	for (__int64 a = 0, b = _len - 1; a < b; a++, b--)
	{
		char c = _str[b];
		_str[b] = _str[a];
		_str[a] = c;
	}
}

__int64 GString0::FindCaseInsensitive( const char * lpszSub, __int64 nStart/* = 0*/ ) const
{
	if (nStart < 0 || nStart > _len)
		return -1;

	char *p = stristr(&_str[nStart], lpszSub);
	if (p)
		return p - _str;
	return -1;
}

// s="aaaBBBccc"    s2 = s.FindStringBetween("aaa","ccc")     s2 == "bbb"
GString GString0::FindStringBetween(const char *pSearchForBegin, const char *pSearchForEnd) const
{
	__int64 nIndex = FindCaseInsensitive( pSearchForBegin ); 
	if (nIndex > -1)
	{
		const char *pStart = &_str[nIndex + strlen(pSearchForBegin)];
		__int64 nIndex = FindCaseInsensitive( pSearchForEnd );
		if (nIndex > -1)
			return GString(pStart,nIndex);
		return GString(pStart);
	}
	return GString();
}

const char *GString0::FindStringAfter(const char *pSearchFor) const
{
	__int64 nIndex = FindCaseInsensitive( pSearchFor ); 
	if (nIndex > -1)
	{
		return &_str[nIndex + strlen(pSearchFor)];
	}
	return 0;
}

__int64 GString0::FindOneOf(const char *pzCharsToSearchFor) const
{
	char *p = strpbrk(_str, pzCharsToSearchFor);
	return (p == 0) ? -1 : p - _str;
}

// replace all occurrences of each character in [pzCharSet] with [chReplaceWith]
void GString0::ReplaceChars(const char *pzCharSet, char chReplaceWith)
{
	char *p = strpbrk(_str, pzCharSet);
	while(p)
	{
		*p = chReplaceWith;
		p = strpbrk(p, pzCharSet);
	}
}


__int64 GString0::FindBinary(GString &strToFind, __int64 nStart/* = 0*/)
{
	__int64 nIndex = -1;

	char *pzToFind = strToFind.Buf();
	__int64 nToFindLen = strToFind.Length();

	while(_len - nStart > 0)
	{
		const char *lpsz = (const char *)memchr(_str + nStart, pzToFind[0], _len - nStart );
		if (lpsz == 0)
			break;
		nStart = (int)(lpsz - _str);
		
		if (nStart + nToFindLen > _len)
			break;

		if ( memcmp(&_str[nStart],pzToFind,nToFindLen) == 0)
		{
			nIndex = nStart;
			break; // found it
		}
		nStart++;
	}
	return nIndex;

}

__int64 GString0::Find( char ch, __int64 nStart ) const
{
	if (nStart >= _len)
		return -1;

	// find first single character
	const char *lpsz = strchr(_str + nStart, (int)ch);

	// return -1 if not found and index otherwise
	return (lpsz == NULL) ? -1 : (lpsz - _str);
}

__int64 GString0::FindNth( char ch, int Nth, __int64 nStart/* = 0 */) const
{
	__int64 nRet = -1;
	for(int i=0; i<Nth; i++)
	{
		nRet = Find(ch,nStart);
		if (nRet == -1)
			break;
		nStart = nRet + 1;
	}
	return nRet;
}

__int64 GString0::ReverseFindOneOf( const char *pstr, __int64 nStart  ) const
{
	if (nStart > _len || nStart < 0)
		return -1;
	
	__int64 n = strlen(pstr);
	for(__int64 i=nStart;i>0;i-- )
	{
		for(__int64 j=0;j<n;j++)
		{
			if (_str[i] == pstr[j])
				return i;
		}
	}
	return -1;
}
__int64 GString0::ReverseFindOneOf( const char *pstr ) const
{
	return ReverseFindOneOf( pstr, _len );
}

__int64 GString0::ReverseFind( char ch ) const
{
	return ReverseFind( ch, _len );
}

__int64 GString0::ReverseFind( char ch, __int64 nStart ) const
{
	if (nStart > _len || nStart < 0)
		return -1;
	
	for(__int64 i=nStart;i>0;i-- )
	{
		if (_str[i] == ch)
			return i;
	}
	return -1;
}

int GString0::RemoveLast ( char ch )
{
	__int64 nIndex = ReverseFind( ch );
	if (nIndex != -1)
	{
		Remove( nIndex, 1 );
		return 1;
	}
	return 0;
}

int GString0::RemoveLast ( const char *pStr, int bMatchCase /*= 0*/)
{
	__int64 nIndex = ReverseFind( pStr, _len, bMatchCase );
	if (nIndex != -1)
	{
		__int64 nstrLen = strlen(pStr);
		Remove( nIndex, nstrLen );
		return 1;
	}
	return 0;
}

__int64 GString0::ReverseFind( const char *pch, __int64 nStart/* = -1*/, int bMatchCase/* = 0*/ ) const
{
	if (nStart == -1)
		nStart = _len;

	if (nStart > _len || nStart < 0)
		return -1;
	
	for(__int64 i=nStart;i>0;i-- )
	{
		__int64 nRet;
		if (bMatchCase)
			nRet = FindCaseInsensitive( pch, i );
		else
			nRet = Find( pch, i );
		if (nRet != -1)
			return nRet;
	}
	return -1;
}

__int64 GString0::ReverseFindNth( const char *pstr, int Nth ) const
{
	__int64 nRet = -1;
	__int64 nStart = _len-1;
	const char *pLastFound = 0;
	
	int nFound = 0;

	while(nStart > -1)
	{
		const char *lpsz = strstr(_str + nStart, pstr);
		if (lpsz)
		{
			if (pLastFound != lpsz)
			{
				nFound++;
				if (nFound == Nth)
					return (int)(lpsz - _str);
				pLastFound = lpsz;
			}
		}
		nStart--;
	}
	return nRet;
}



__int64 GString0::Find( const char * lpszSub, __int64 nStart ) const
{
	if (nStart > _len)
		return -1;

	// find first matching substring
	const char *lpsz = strstr(_str + nStart, lpszSub);

	// return -1 for not found, distance from beginning otherwise
	return (lpsz == NULL) ? -1 : (lpsz - _str);
}

__int64 GString0::FindNth( const char *pstr, int Nth, __int64 nStart/* = 0*/ ) const
{
	__int64 nRet = -1;
	__int64 strLen = strlen(pstr);
	for(int i=0; i<Nth; i++)
	{
		nRet = Find(pstr,nStart);
		if (nRet == -1)
			break;
		nStart = nRet + strLen;
	}
	return nRet;
}


void GString0::OverWrite(const char *pzNew, __int64 nIndex/*=0*/, __int64 nLength/* = -1*/)
{
	if (nLength == -1)
		nLength = strlen(pzNew);

	while (nIndex + nLength + 1 >= _max)
		resize();


	memcpy(&_str[nIndex],pzNew,nLength);

	if (nLength + nIndex > _len)
	{
		_len = nLength + nIndex;
		_str[_len] = 0;
	}
}


// search 'this' string for [pzMatch], and insert [pzInsertThis] before [pzMatch] for InsertBefore() or after [pzMatch] for InsertAfter()
// if bMatchCase is set to 1, then ONLY ExAcT case matches for [pzMatch] will be used.
// returns the index that [pzInsertThis] was inserted or -1 if [pzMatch] was not found.
__int64 GString0::InsertBefore( const char *pzMatch, const char *pzInsertThis, int bMatchCase /*= 0*/)
{
	__int64 nIndex;
	if (bMatchCase)
	{
		nIndex = Find( pzMatch );
	}
	else
	{
		nIndex = FindCaseInsensitive( pzMatch );
	}
	if (nIndex != -1)
		Insert( nIndex, pzInsertThis);

	return nIndex;
}

__int64 GString0::InsertAfter( const char *pzMatch, const char *pzInsertThis, int bMatchCase /*= 0*/)
{
	__int64 nIndex;
	if (bMatchCase)
	{
		nIndex = Find( pzMatch );
	}
	else
	{
		nIndex = FindCaseInsensitive( pzMatch );
	}
	
	__int64 n = 0;
	if (nIndex != -1)
	{
		n = strlen(pzMatch);
		Insert( nIndex + n, pzInsertThis );
	}

	return nIndex + n;
}


void GString0::Insert( __int64 nIndex, char ch )
{
	// subscript out of range
	if (nIndex > _len)
		throw GException("String", 1);

	SetPreAlloc(_len + 2);

	memmove(&_str[nIndex + 1], &_str[nIndex], _len - nIndex);
	
	_len++;

	_str[nIndex] = ch;
	_str[_len] = 0;
}

void GString0::Insert( __int64 nIndex, const char *pstr, __int64 nStrLen/* -1 */ )
{
	if (!pstr)
		return;

	// subscript out of range
	if (nIndex > _len)
		throw GException("String", 1);

	__int64 nCnt = (nStrLen == -1) ? strlen(pstr) : nStrLen;

	SetPreAlloc(_len + nCnt + 1);

	memmove(&_str[nIndex + nCnt], &_str[nIndex], _len - nIndex);
	_len += nCnt;

	memcpy(&_str[nIndex],pstr,nCnt);

	_str[_len] = 0;
}

void GString0::MergeMask(const char *szSrc, const char *szMask)
{
	Empty();

	if ((szSrc != 0) && (*szSrc != 0))
	{
		while ((*szSrc != 0) || (*szMask != 0))
		{
			switch (*szMask)
			{
				// copy a single character from the 
				case '_' :
					if (*szSrc)
					{
						*this += *szSrc;
						szSrc++;
					}
					szMask++;
					break;
				// copy the remaining characters from szSrc
				case '*' :
					*this += szSrc;
					while (*szSrc)
						szSrc++;
					szMask++;
					break;
				// all other characters are mask chars
				// with the exception of '\' which is
				// used to escape '_', '*' and '\'
				default  :
					if (*szMask != 0)
					{
						if (*szMask == '\\')
							// skip the escape character
							szMask++;
						if (*szMask != 0)
						{
							*this += *szMask;
							szMask++;
						}
					}
					break;
			}

			// throw away the remaining characters in the source string
			if (*szMask == 0)
			break;
		}
	}
}

void GString0::FormatNumber(const char *szFormat, 
						   char decimal_separator,
						   char grouping_separator,
						   char minus_sign,
						   const char *NaN,
						   char zero_digit,
						   char digit,
						   char pattern_separator,
						   char percent,
						   char permille)
{
	if (szFormat && *szFormat)
	{
		// make sure that the string is a number {0..9, '.', '-'}
		// if the string contains a character not in the number
		// subset then set the value of the string to NaN.
		const char *szValue = _str;
		if (IsNaN(szValue, '.', ',', '-'))
			*this = NaN;
		else
		{
			// it's a number, get the whole part and the fraction part
			__int64 nIdx = Find('.');
			GString strWhole;
			strWhole = (nIdx == -1) ? _str : (const char *)Left(nIdx);
			GString strFraction('0', (short)1);
			nIdx = Find('.') + 1;
			if (nIdx > 0)
				strFraction = Mid(nIdx);
			bool bIsNeg = (Find(minus_sign) != -1);

			long nWhole = abs(atol((const char *)strWhole));
			long nFract = abs(atol((const char *)strFraction));

			// check for % and ?
			if (percent == szFormat[0])
			{
				double d = atof(_str);
				d *= 100;
				GString strTemp;
				strTemp.Format("%f", d);
				nIdx = strTemp.Find('.');
				strFraction = (nIdx == -1) ? strTemp._str : (const char *)strTemp.Left(nIdx);
				nWhole = atol((const char *)strFraction);
				nFract = 0;
			}
			if (permille == szFormat[0])
			{
				double d = atof(_str);
				d *= 1000;
				GString strTemp;
				strTemp.Format("%f", d);
				nIdx = strTemp.Find('.');
				strFraction = (nIdx == -1) ? strTemp._str : (const char *)strTemp.Left(nIdx);
				nWhole = atol((const char *)strFraction);
				nFract = 0;
			}

			// if the number is negative, get the negative pattern out of the format.
			// if a negative pattern doesn't exist, the minus_sign will be prepended
			// to the positive pattern.
			GString strFormat(szFormat);
			nIdx = strFormat.Find(pattern_separator);
			if (bIsNeg)
			{
				if (nIdx != -1)
					strFormat = strFormat.Mid(nIdx + 1);
				else
					strFormat.Format("%c%s", minus_sign, (const char *)strFormat);
			}
			else
			{
				if (nIdx != -1)
					strFormat = strFormat.Left(nIdx);
			}

			GString strFormatWhole(strFormat);
			GString strFormatDecimal('#', (short)1);

			// determine the number of digits per group
			__int64 nGroup = 0;
			nIdx = strFormat.Find(',');
			if (nIdx != -1)
			{
				nIdx++;
				__int64 nNext = strFormat.Find(',', nIdx);
				if (nNext == -1)
					nNext = strFormat.Find('.', nIdx);
				if (nNext == -1)
					nNext = strFormat.Length();
				nGroup = (nNext - nIdx);
			}

			// determine the number of decimals to display
			__int64 nDecimals = 0;
			nIdx = strFormat.Find('.');
			if ((nIdx != -1) && 
				(percent != szFormat[0]) &&
				(permille != szFormat[0]))
			{
				if (nGroup)
					strFormatWhole = strFormat.Mid(nIdx - nGroup, nGroup);
				else
					strFormatWhole = strFormat.Left(nIdx);
				nIdx++;
				strFormatDecimal = strFormat.Mid(nIdx);
				nDecimals = (strFormat.Length() - nIdx);
			}

			// Format the whole part of the number
			int nCount = CountOf((const char *)strFormatWhole, zero_digit);
			strWhole.Format("%0ld", (int)nWhole);
			if (strWhole.Length() < nCount)
			{
				GString temp(zero_digit, (short)(nCount - (short)strWhole.Length()));
				strWhole.Format("%s%s", (const char *)temp, (const char *)strWhole);
			}

			Empty();

			// add all prefix characters
			nIdx = 0;
			const char *szP = (const char *)strFormat;
			while (*szP)
			{
				if (*szP == digit ||
					*szP == zero_digit ||
					*szP == decimal_separator ||
					*szP == grouping_separator ||
					*szP == percent ||
					*szP == permille)
					break;

				szP++;
				nIdx++;
			}
			strFormat = strFormat.Left(nIdx);

			strFormat.MakeReverse();

			__int64 i, j;
			for (i = 0, j = strWhole.Length() - 1; j >= 0; j--, i++)
			{
				if ((nGroup) && (i == nGroup))
				{
					*this += grouping_separator;
					i = 0;
				}

				*this += strWhole.GetAt(j);
			}
			*this += strFormat;

			MakeReverse();

			if (nDecimals)
			{
				*this += decimal_separator;

				strFraction.Format("%ld", (int)nFract);
				const char *szF1 = (const char *)strFormatDecimal;
				const char *szF2 = (const char *)strFraction;
				i = 0;
				while (*szF1)
				{
					if (*szF2)
						*this += *szF2;
					else if (*szF1 == zero_digit)
						*this += zero_digit;
					else if (*szF1 != digit) // add all sufix characters
						*this += *szF1;
					if (*szF2)
						szF2++;
					if (*szF1)
						szF1++;
				}
			}

			if (percent == szFormat[0])
				*this += percent;
			if (permille == szFormat[0])
				*this += permille;
		}
	}
}

void GString0::FormatBinary(const GString &strBinary, int bIncludeAscii/*=1*/)
{
	FormatBinary((unsigned char *)(const char *)strBinary, strBinary.Length(), bIncludeAscii);
}

void GString0::FormatBinary(unsigned char *pData, __int64 nBytes, int bIncludeAscii/*=1*/)
{
	if (!pData || nBytes < 1)
		return;

	// prealloc estimate
	if (_max < nBytes * 5)
	{
		_max = nBytes * 3; // it will be doubled during resize
		resize();
	}
	
	GString strHexByte;
	GString strASCIIBytes;
	__int64 nByteIndex = 0;
	while(nBytes > 0)
	{
		strHexByte.Format("%02X ",(int)(unsigned char)pData[nByteIndex]);
		(*this) += strHexByte;
		if (isprint(pData[nByteIndex]))
		{
			strASCIIBytes += pData[nByteIndex];
		}
		else
		{
			strASCIIBytes += ".";
		}
		nByteIndex++;
		nBytes--;

		if ( bIncludeAscii )
		{
			if ( !(nByteIndex % 25) )
			{
				(*this)  += "     ";
				(*this)  += strASCIIBytes;
				strASCIIBytes = "";
#ifdef _WIN32
				(*this)  += "\r\n";
#else
				(*this)  += "\n";
#endif
			}
		}

	}
	if ( bIncludeAscii )
	{
		__int64 nSpaces = ((25 - strASCIIBytes.Length()) * 3) + 5;
		(*this).Append(nSpaces,' ');
		(*this)  += strASCIIBytes;
	}
}



#define FORCE_ANSI      0x10000
#define FORCE_UNICODE   0x20000
#define FORCE_INT64     0x40000

void GString0::FormatV(const char *lpszFormat, va_list argList)
{
try
{
#ifdef _LINUX64	
	va_list argListSave;		
	va_copy(argListSave, argList); 
#else
	va_list argListSave = argList;
#endif


	////////////////////////////////////////////////////////////////////////
	// calculate the destination size in this first for() loop
	__int64 nMaxLen = 8;	// 8 is a minimum.
	////////////////////////////////////////////////////////////////////////
	for (const char *lpsz = lpszFormat; *lpsz != '\0'; lpsz++)
	{
		// handle '%' character, but watch out for '%%'
		if (*lpsz != '%' || *(lpsz = lpsz + 1) == '%')
		{
			nMaxLen += 1;
			continue;
		}

		__int64 nItemLen = 0;

		// handle '%' character with format
		int nWidth = 0;
		for (; *lpsz != '\0'; lpsz++)
		{
			// check for valid flags
			if (*lpsz == '#')
				nMaxLen += 2;   // for '0x'
			else if (*lpsz == '*')
				nWidth = va_arg(argList, int);
			else if (*lpsz == '-' || *lpsz == '+' || *lpsz == '0' || *lpsz == ' ');
			else // hit non-flag character
				break;
		}
		// get width and skip it
		if (nWidth == 0)
		{
			// width indicated by
			nWidth = atoi(lpsz);
			for (; *lpsz != '\0' && isdigit(*lpsz); lpsz++);
		}

		int nPrecision = 0;
		if (*lpsz == '.')
		{
			// skip past '.' separator (width.precision)
			lpsz++;

			// get precision and skip it
			if (*lpsz == '*')
			{
				nPrecision = va_arg(argList, int);
				lpsz++;
			}
			else
			{
				nPrecision = atoi(lpsz);
				for (; *lpsz != '\0' && isdigit(*lpsz); lpsz++);
			}
		}

		// should be on type modifier or specifier
		int nModifier = 0;
		switch (*lpsz)
		{
		// modifiers that affect size
		case 'h':
			nModifier = FORCE_ANSI;
			lpsz++;
			break;
		case 'l':
			nModifier = FORCE_UNICODE;
			lpsz++;
			break;

		// modifiers that do not affect size
		case 'F':
		case 'N':
		case 'L':
			lpsz++;
			break;
		}

		// now should be on specifier
		switch (*lpsz | nModifier)
		{
		// single characters
		case 'c':
		case 'C':
			nItemLen = 2;
#ifdef _WIN32 
			va_arg(argList, char);
#else
			// gcc says: "`char' is promoted to `int' when passed through ...
			// so we pass 'int' not 'char' to va_arg"
			va_arg(argList, int);
#endif
			break;

		// strings
		case 's':
			{
				const char *pstrNextArg = va_arg(argList, const char *);
				if (pstrNextArg == NULL)
				   nItemLen = 6;  // "(null)"
				else
				{
				   nItemLen = strlen(pstrNextArg);
				   nItemLen = ___max(1, nItemLen);
				}
			}
			break;

		case 's'|FORCE_ANSI:
		case 'S'|FORCE_ANSI:
			{
				const char * pstrNextArg = va_arg(argList, const char *);
				if (pstrNextArg == NULL)
				   nItemLen = 6; // "(null)"
				else
				{
				   nItemLen = strlen(pstrNextArg);
				   nItemLen = ___max(1, nItemLen);
				}
			}
			break;
		}
		// adjust nItemLen for strings
		if (nItemLen != 0)
		{
			if (nPrecision != 0)
				nItemLen = ___min(nItemLen, nPrecision);
			nItemLen = ___max(nItemLen, nWidth);
		}
		else
		{
			switch (*lpsz)
			{
			case 'I': // look for I64
				if (strncmp(lpsz, "I64", 3) == 0)
				{
					va_arg(argList, __int64);
					lpsz += 2;
					nItemLen = 32;
				}
				break;

			// integers
			case 'd':
			case 'i':
			case 'u':
			case 'x':
			case 'X':
			case 'o':
				if (nModifier & FORCE_INT64)
					va_arg(argList, __int64);
				else
					va_arg(argList, int);

				nItemLen = 32;
				nItemLen = ___max(nItemLen, nWidth+nPrecision);
				break;

			case 'e':
			case 'g':
			case 'G':
				va_arg(argList, double);
				nItemLen = 128;
				nItemLen = ___max(nItemLen, nWidth+nPrecision);
				break;

			case 'f':
				{
					double f;
					// 312 == strlen("-1+(309 zeroes).")
					// 309 zeroes == max precision of a double
					// 6 == adjustment in case precision is not specified,
					//   which means that the precision defaults to 6
					char *pszTemp = new char[___max(nWidth, 312+nPrecision+6)];

					f = va_arg(argList, double);
					sprintf( pszTemp,  "%*.*f", nWidth, nPrecision+6, f );
					nItemLen = strlen(pszTemp);

					delete [] pszTemp;
				}
				break;

			case 'p':
				va_arg(argList, void*);
				nItemLen = 32;
				nItemLen = ___max(nItemLen, nWidth+nPrecision);
				break;

			// no output
			case 'n':
				va_arg(argList, int*);
				break;
			}
		}

		// adjust nMaxLen for output nItemLen
		nMaxLen += nItemLen;
	}
	
	////////////////////////////////////////////////////////////////////////
	// Now we know how big it will be, resize 'this' GString to have guaranteed space
	////////////////////////////////////////////////////////////////////////
	if ( nMaxLen + 1 > _max )
	{
		if (_strIsOnHeap == 1)
			delete[] _str;
		_max = nMaxLen + 1;
		_str = new char[_max];
		_strIsOnHeap = 1;
		_initialbuf[0] = 0;
	}

	_len = vsprintf(_str, lpszFormat, argListSave);
	va_end(argListSave);

	if (_len < 0)
	{
		_len = 0;
		_str[_len] = 0;
	}
}catch(...)	{}
}


GProfile &GetErrorProfile(); // GException.cpp

// Load an error from the active ErrorProfile() owned by GException.cpp
void GString0::LoadResource(const char* szSystem, int error, ...)
{
	GProfile &ErrorProfile = GetErrorProfile();
//	int nsubSystem = atoi(ErrorProfile.GetString(szSystem, "SubSystem"));

	GString strKey;
	strKey.Format("%ld", (int)error);
	strKey = ErrorProfile.GetString(szSystem, (const char *)strKey);

	va_list argList;
	va_start(argList, error);
	FormatV((const char *)strKey, argList);
	va_end(argList);

}

void GString0::FormatAppend(const char *lpszFormat, ...)
{
	GString str;
	va_list argList;
	va_start(argList, lpszFormat);
	str.FormatV(lpszFormat, argList);
	va_end(argList);

	*this << str;
}

void GString0::FormatPrepend(const char *lpszFormat, ...)
{
	GString str;
	va_list argList;
	va_start(argList, lpszFormat);
	str.FormatV(lpszFormat, argList);
	va_end(argList);

	Insert(0,str);
}

void GString0::Format(const char *lpszFormat, ...)
{
	va_list argList;
	va_start(argList, lpszFormat);
	FormatV(lpszFormat, argList);
	va_end(argList);
}

bool GString0::ToFileAppend(const char* pzFileName, bool bThrowOnFail /*= 1*/)
{
	int nTry = 0;
RETRY:
	
	// c runtime fopen() throws exception when filename == "", so manage it here
	if (!pzFileName || pzFileName[0] == 0)
	{
		if (bThrowOnFail)
			throw GException("String", 3, pzFileName);
		return 0;
	}

	FILE *fp = fopen(pzFileName,"ab"); 
	if (fp)
	{
		fwrite(_str,1,_len,fp); 
		fclose(fp);
	}
	else
	{
		if (nTry++ < 1)
		{
			// try to create the path if it was not there.
			GDirectory::CreatePath(pzFileName,1);
			goto RETRY;
		}

		// the file could not be opened for writing
		if (bThrowOnFail)
			throw GException("String", 3, pzFileName);
		
		// fail
		return 0;
	}
	// success
	return 1;
}


bool GString0::ToFile(const char* pzFileName, bool bThrowOnFail/* = 1*/)
{
	// c runtime fopen() throws exception when filename == "", so manage it here
	if (!pzFileName || pzFileName[0] == 0)
	{
		if (bThrowOnFail)
			throw GException("String", 3, pzFileName);
		return 0;		
	}
	
	int nTry = 0;
RETRY:
	FILE *fp = fopen(pzFileName,"wb");
	if (fp)
	{
		fwrite(_str,1,_len,fp);
		fclose(fp);
	}
	else
	{
		if (nTry++ < 1)
		{
			// try to create the path if it was not there.
			GDirectory::CreatePath(pzFileName,1);
			goto RETRY;
		}

		// the file could not be opened for writing
		if (bThrowOnFail)
			throw GException("String", 3, pzFileName);
		
		// fail
		return 0;
	}
	// success
	return 1;
}


bool GString0::FromFileAppend(const char* pzFileName, bool bThrowOnFail)
{
	FILE *fp = fopen(pzFileName,"rb");
	if (fp)
	{
		// get the size of the file
		fseek(fp,0,SEEK_END);
		__int64 lBytes = ftell(fp);
		fseek(fp,0,SEEK_SET);

		SetPreAlloc(_len+lBytes+1); 

		 fread(&_str[_len],sizeof(char),lBytes,fp);
		_len += lBytes;
		_str[_len] = 0;

		fclose(fp);
	}
	else  
	{
		// the file could not be opened.
		if (bThrowOnFail)
			throw GException("String", 2, pzFileName);
		
		// fail
		return 0;
	}
	// success
	return 1;
}

bool GString0::FromFile(const char* pzFileName, bool bThrowOnFail/* = 1*/)
{
	FILE *fp = 0;
	if (pzFileName && pzFileName[0]) // in NT "" will cause a debug break.  Since the server can run live in a debugger we never want an unexpected break
		fp = fopen(pzFileName,"rb");
	if (fp)
	{
		// get the size of the file
		fseek(fp,0,SEEK_END);
		__int64 lBytes = ftell(fp);
		fseek(fp,0,SEEK_SET);

		if (lBytes + 1 > _max) // only grow if we have to
		{
			if (_strIsOnHeap == 1)
				delete [] _str;

			// pre-alloc the GString
			_str = new char[lBytes + 1];
			_strIsOnHeap = 1;
			_initialbuf[0] = 0;
			_max = lBytes + 1;
		}



		fread(_str,sizeof(char),lBytes,fp);
		_len = lBytes;
		_str[_len] = 0;
		fclose(fp);
	}
	else  
	{
		// the file could not be opened.
		if (bThrowOnFail)
			throw GException("String", 2, pzFileName);
		
		// fail
		return 0;
	}
	// success
	return 1;
}


void GString0::ReplaceCaseInsensitive( const char * szWhat, const char *szRep, __int64 nStart/* = 0*/, int nFirstOnly/* = 0*/  )
{
	if ((!szWhat) || (!szRep))
		return;

	__int64 nPos = FindCaseInsensitive(szWhat, nStart);
	__int64 nLen = strlen(szRep);

	while (nPos != -1)
	{
		Remove(nPos, strlen(szWhat));
		if (nLen)
			Insert(nPos, szRep);
		
		if (nFirstOnly)
			break;

		nPos += nLen;

		nPos = FindCaseInsensitive(szWhat, nPos);
	}
}


void GString0::Replace( const char * szWhat, const char *szRep, int nFirstOnly/* = 0*/ )
{
	if ((!szWhat) || (!szRep))
		return;

	__int64 nPos = Find(szWhat);
	__int64 nLen = strlen(szRep);

	while (nPos != -1)
	{
		Remove(nPos, strlen(szWhat));
		if (nLen)
			Insert(nPos, szRep, nLen);

		if (nFirstOnly)
			break;

		nPos += nLen;

		nPos = Find(szWhat, nPos);
	}
}

void GString0::Replace( char chWhat, char chRep, int nFirstOnly/* = 0*/ )
{
	for(__int64 i=0;i < _len; i++)
	{
		if (_str[i] == chWhat)
		{
			_str[i] = chRep;
			if (nFirstOnly)
				break;
		}
	}
}

void GString0::Replace( const char * szWhat, char chRep, int nFirstOnly/* = 0*/ )
{
	char szRep[2];
	szRep[0] = chRep;
	szRep[1] = 0;
	Replace( (const char *)szWhat, (const char *)szRep, nFirstOnly );
}

void GString0::Replace( char chWhat, const char *szRep, int nFirstOnly/* = 0*/ )
{
	if (chWhat)
	{
		char szWhat[2];
		szWhat[0] = chWhat;
		szWhat[1] = 0;

		Replace( (const char *)szWhat, (const char *)szRep, nFirstOnly );
	}
}

void GString0::Remove( __int64 nStart, __int64 nLen )
{
	// subscript out of range
	if (nStart + nLen > _len)
		throw GException("String", 1);

	memmove(&_str[nStart], &_str[nStart + nLen], _len - (nStart + nLen));
	_len -= nLen;
	_str[_len] = 0;
}

__int64 GString0::RemoveAll( char ch )
{
	__int64 nLenCopy = _len;
	__int64 nRemoved = 0;
	for(__int64 i=0; i<nLenCopy; i++)
	{
		if (_str[i] == ch)
		{
			Remove( i, 1 );
			i--;
			nLenCopy--;
			nRemoved++;
		}
	}
	return nRemoved;
}

__int64 GString0::RemoveAll( const char *pStr, int bMatchCase /*= 0*/ )
{
	__int64 nRemoved = 0;

	__int64 nstrLen = strlen(pStr);
	
	__int64 nIndex = (bMatchCase) ? Find( pStr, 0 ) : FindCaseInsensitive(pStr, 0);

	while(nIndex != -1)
	{
		Remove( nIndex, nstrLen );
		nRemoved++;
		nIndex = (bMatchCase) ? Find( pStr, nIndex ) : FindCaseInsensitive(pStr, nIndex);
	}
	return nRemoved;
}

int GString0::RemoveFirst ( char ch )
{
	__int64 nLenCopy = _len;
	int nRemoved = 0;
	for(__int64 i=0; i<nLenCopy; i++)
	{
		if (_str[i] == ch)
		{
			Remove( i, 1 );
			i--;
			nLenCopy--;
			nRemoved++; // sets nRemoved to 1
			break;	
		}
	}
	return nRemoved;
}

int GString0::RemoveFirst ( const char *pStr, int bMatchCase /*= 0*/ )
{
	int nRemoved = 0;

	__int64 nstrLen = strlen(pStr);
	
	__int64 nIndex = (bMatchCase) ? Find( pStr, 0 ) : FindCaseInsensitive(pStr, 0);

	if(nIndex != -1)
	{
		Remove( nIndex, nstrLen );
		nRemoved++; // sets nRemoved to 1
	}
	return nRemoved;
}


void GString0::StripQuotes()
{
	if ((_len >= 2) && (_str[0] == _str[_len - 1]))
	{
		if ((_str[0] == '\'') || (_str[0] == '"'))
		{
			memmove(&_str[0], &_str[1], _len - 2);
			_len -= 2;
			_str[_len] = 0;
		}
	}
}


void GString0::NormalizeSpace()
{
	__int64 nBegWS = -1;
	__int64 nEndWS = -1;

	for (__int64 i = 0; i < _len; i++)
	{
		if (isWS(_str[i]))
		{
			nEndWS = i;
			if (nBegWS == -1)
				nBegWS = i;
		}
		else if ((nBegWS != -1) && 
				 (nEndWS != -1))
		{
			nEndWS++;
			Remove(nBegWS, nEndWS - nBegWS);
			Insert(nBegWS, ' ');
			i = nBegWS;
			nBegWS = -1;
			nEndWS = -1;
		}
		else
		{
			nBegWS = -1;
			nEndWS = -1;
		}
	}

	if ((nBegWS != -1) && 
		(nEndWS != -1))
	{
			nEndWS++;
			Remove(nBegWS, nEndWS - nBegWS);
			Insert(nBegWS, ' ');
	}

	TrimLeftWS();
	TrimRightWS();
}

void GString0::Translate(const char *szConvert, const char *szTo)
{
	if ((szConvert) && (szTo))
	{
		__int64 nTranslate = ___min(strlen(szConvert), strlen(szTo));
		for (__int64 i = 0; i < _len; i++)
		{
			for (__int64 j = 0; j < nTranslate; j++)
			{
				if (_str[i] == szConvert[j])
					_str[i] =  szTo[j];
			}
		}
	}
}


// replace each char in pzReplaceChars with %nn where
// nn is a two byte hex value of the byte that was replaced.
// example: GString s("ABC");  
//			s.EscapeWithHex("XYZB");
//			s == "A%42C"
//	42 is hex for 66 that is the ASCII of a B
//
// example: GString s("A\nC");  
//			s.EscapeWithHex("\r\n\t");
//			s == "A%0AC"
//
const char *GString0::EscapeWithHex(const char *pzEscapeChars)
{
	GString strDestrination;
	GString strEscapeChars(pzEscapeChars);
	char *pSrc = (char *)(const char *)(*this);
	__int64 nSourceBytes = strlen(pSrc);
	
	for(__int64 i=0;i < nSourceBytes; i++)
	{
		// escape special chars
		if ( strEscapeChars.Find(pSrc[i]) > -1 )
		{
			char buf[20];
			sprintf(buf,"%%%02X",pSrc[i]);
			strDestrination += buf;
		}
		else 
		{
			strDestrination += pSrc[i];
		}
	}
	*this = strDestrination;
	return *this;
}

// reverse of GString0::EscapeWithHex()
void GString0::UnEscapeHexed()
{
	const char *pSource = *this;
	char *pDest = new char [strlen(pSource) + 1];

	char *pszQuery = (char *)pSource;
	char *t = pDest;
	while (*pszQuery)
	{
		switch (*pszQuery)
		{
		case '%' :
			{
				pszQuery++;
				char chTemp = pszQuery[2];
				pszQuery[2] = 0;
				char *pStart = pszQuery;
				char *pEnd;
				*t = (char)strtol(pStart, &pEnd, 16);
				pszQuery[2] = chTemp;
				pszQuery = pEnd;
				t++;
				continue;
			}
			break;
		default :
			*t = *pszQuery;
			break;
		}
		pszQuery++;
		t++;
	}
	*t = 0;
	*this = pDest;
	delete[] pDest;
}


void GString0::UnEscapeURL()
{
	const char *pSource = *this;
	char *pDest = new char [strlen(pSource) + 1];

	char *pszQuery = (char *)pSource;
	char *t = pDest;
	while (*pszQuery)
	{
		switch (*pszQuery)
		{
		case '+' :
			*t = ' ';
			break;

		case '%' :
			{
				pszQuery++;
				char chTemp = pszQuery[2];
				pszQuery[2] = 0;
				char *pStart = pszQuery;
				char *pEnd;
				*t = (char)strtol(pStart, &pEnd, 16);
				pszQuery[2] = chTemp;
				pszQuery = pEnd;
				t++;
				continue;
			}
			break;
		default :
			*t = *pszQuery;
			break;
		}
		pszQuery++;
		t++;
	}
	*t = 0;
	*this = pDest;
	delete[] pDest;

}

void GString0 ::Inc(__int64 nAmount/* = 1*/)
{
	__int64 n = Xatoi64(StrVal());
	n += nAmount;
	*this = n;
}

void GString0 ::Dec(__int64 nAmount/* = 1*/)
{
	__int64 n = Xatoi64(StrVal());
	n -= nAmount;
	*this = n;
}
void GString0 ::Inc(const char *pzAmount)
{
	__int64 n = Xatoi64(StrVal());
	__int64 n2 = Xatoi64(pzAmount);
	n += n2;
	*this = n;
}

void GString0 ::Dec(const char *pzAmount)
{
	__int64 n = Xatoi64(StrVal());
	__int64 n2 = Xatoi64(pzAmount);
	n -= n2;
	*this = n;
}


// convert 'this' to a user friendly byte notation with 2 digit unrounded percision
//-----------------------------------------------------------------------------------
// 999		=  999b		1000000		=	1.0Mb	1000000000000	=1.0Tb
// 1000		= 1.0kb		10000000	=	 10Mb	9500000000000	=9.5Tb
// 10000	=  10kb		999999999	=	999Mb	999999999999999	=999Tb
// 999999	= 999kb		1000000000	=	1.0Gb
const char *GString0::AbbreviateNumeric()
{
	__int64 N = Xatoi64(_str);
	if (N < 1000)
	{
		Empty(); 
		*this << N;
	}
	else if (N < 10000)			// 1.0KB - 9.9KB
	{
		Insert(1,'.');
		SetLength(3);
		*this << "Kb";
	}
	else if (N < 100000)		// 10KB - 99KB
	{
		SetLength(2);
		*this << "Kb";
	}
	else if (N < 1000000)		// 100KB - 999KB
	{
		SetLength(3);
		*this << "Kb";
	}
	else if (N < 10000000)		// 1.0Mb - 9.9Mb
	{
		Insert(1,'.');
		SetLength(3);
		*this << "Mb";
	}
	else if (N < 100000000)		// 10Mb - 99Mb
	{
		SetLength(2);
		*this << "Mb";
	}
	else if (N < 1000000000)	// 100Mb - 999Mb
	{
		SetLength(3);
		*this << "Mb";
	}

	// under gcc 4 this code won't compile (July 2, 2009)
	// the compile error is "integer constant too large for long type"
	// N is an __int64, and I double checked that __int64 is defined as 'long long' in Linux
	// I even tried to cast the constant to a "long long" and I get the same error.
//#ifndef _LINUX


	else if (N < 10000000000)	// 1.0Gb - 9.9Gb
	{
		Insert(1,'.');
		SetLength(3);
		*this << "Gb";
	}
	else if (N < 100000000000)	// 10Gb - 99Gb
	{
		SetLength(2);
		*this << "Gb";
	}
	else if (N < 1000000000000)	// 100Gb - 999Gb
	{
		SetLength(3);
		*this << "Gb";
	}
	else if (N < 10000000000000)// 1.0Tb - 9.9Tb
	{
		Insert(1,'.');
		SetLength(3);
		*this << "Tb";
	}
	else if (N < 100000000000000)// 10Tb - 99Tb
	{
		SetLength(2);
		*this << "Tb";
	}
	else if (N < 1000000000000000)// 100Tb - 999Tb
	{
		SetLength(3);
		*this << "Tb";
	}

//#endif
	
	return _str;
}

const char *GString0::CommaNumeric()
{

	GString strReverse;
	int nNum = 1;
	for(__int64 idx=_len-1; idx>-1; idx--)
	{
		strReverse += _str[idx];
		if (nNum++ == 3)
		{
			if(idx)
				strReverse += ',';
			nNum = 1;
		}
	}
	strReverse.MakeReverse();
	*this = strReverse;
	return _str;
}

void GString0::AppendPackedInteger(unsigned short n)
{
	unsigned short s = Ghtons(n); // packet length in network byte order
	write((const char *)&s,sizeof(short));
}

void GString0::AppendPackedInteger(unsigned long n)
{
	unsigned long l = Ghtonl(n); // packet length in network byte order
	write((const char *)&l,sizeof(long));
}

void GString0::AppendPackedInteger(unsigned __int64 n)
{
	static short int word = 0x0001;
	static char *byte = (char *) &word;
	if (!byte[0]) // BigEndian
		write((const char *)&n,sizeof(n));
	else
	{
		unsigned __int64 n2 = (((unsigned __int64)Ghtonl((unsigned long)n)) << 32) + Ghtonl((unsigned long)(n >> 32));
		write((const char *)&n2,sizeof(n2));
	}
}
unsigned short GString0::GetPackedShort(__int64 index)
{
	unsigned short theNumber = 0;				// storage on platform architecture byte boundry
	memcpy(&theNumber,&_str[index],2);			// storage in network byte order moved to architecture byte boundry storage 
	return  Gntohs( theNumber );					// convert to host byte order
}


unsigned long GString0::GetPackedLong(__int64 index)
{
	// note:
	// return Gntohl( &_str[index] ) does not work on all platforms

	unsigned long theNumber = 0;				// storage on platform architecture byte boundry
	memcpy(&theNumber,&_str[index],4);			// storage in network byte order moved to architecture byte boundry storage 
	return  Gntohl( theNumber );					// convert to host byte order
}
unsigned __int64 GString0::GetPacked64(__int64 index)
{
	static short int word = 0x0001; // being static, it's only initialized the first time this is called
	static char *byte = (char *) &word;

	unsigned __int64 n;
	memcpy(&n,&_str[index],8);	

	if (!byte[0]) // BigEndian		
		return n;
	else
		return (((unsigned __int64)Gntohl((unsigned long)n)) << 32) + Gntohl( (unsigned long)(n >> 32) );
}



bool GString0::Cipher( GString &strKey, GString *pSrc )
{
	// returns 1 on success, 0 on Fail with description in  strErrorOut
	GString strErrorOut;
	Empty();
	SetPreAlloc( pSrc->Length() + 64 );
	
	// convert to GString without making a copy
	GString gStr(this->_str, this->_len, this->_max, 0);
	bool bRet = EncryptMemoryToMemory(strKey, pSrc->_str, pSrc->_len, gStr, strErrorOut);
	*this = gStr;
	return bRet;
}

// This Cipher() assumes 'this' contains the unciphered source
// unciphered data in 'this' will be replaced with the ciphered version of that data.
bool GString0::Cipher( GString &strKey ) 
{
	GString strTemp(*this);
	return Cipher( strKey, &strTemp); 
}
bool GString0::Cipher( const char *pzKey )
{
	GString strTemp(*this);
	GString strKey(pzKey);
	return Cipher( strKey, &strTemp); 
}


bool GString0::DeCipher( GString &strKey, GString *pSrc )
{
	GString strErrorOut;
	SetPreAlloc( pSrc->Length() + 64 );
	// convert to GString without making a copy
	GString gStr(this->_str, this->_len, this->_max, 0);
	bool bRet = DecryptMemoryToMemory(strKey, pSrc->_str, pSrc->_len, gStr, strErrorOut);
	*this = gStr;
	return bRet;

}

// This DeCipher() assumes 'this' contains the ciphered source - possibly loaded with FromFile()
//  ciphered data in 'this' will be replaced with the unciphered version of that data.
bool GString0::DeCipher( GString &strKey ) 
{ 
	GString strSrc(*this);
	return DeCipher(strKey,&strSrc);
}
bool GString0::DeCipher( const char *pzKey )
{
	GString strSrc(*this);
	GString strKey(pzKey);
	return DeCipher( strKey, &strSrc ); 
}


bool GString0::DeCompress( const char *pFileName, __int64 nDecompressedSize)
{
	GString strG;
	if ( strG.FromFile(pFileName,0) )
	{
		Empty();
		return DeCompress( &strG,nDecompressedSize );
	}
	return 0;
}

// this DeCompress() assumes that the current contents of 'this' are the compressed data, perhaps assigned by FromFile()
// the compressed source will be replaced with the uncompressed form of the data in 'this'
bool GString0::DeCompress(__int64 nDecompressedSize) 
{
	GString strSrc(*this);
	Empty();
	return DeCompress( &strSrc, nDecompressedSize );
}


// pSrc is the compressed data in a GString
// 'this' contains the uncompressed results
bool GString0::DeCompress( GString *pSrc, __int64 nDestSize)
{
	Empty();
	unsigned int nUnZippedLength = (unsigned int)nDestSize;

	// since XML compresses very well - and the destination was not supplied 
	// start with a destination buffer 21 times bigger than the compressed size and we will make it bigger if need be.
	if (nDestSize == -1)
		nUnZippedLength = (unsigned int)pSrc->GetLength() * 21;

	SetPreAlloc( nUnZippedLength );

	int cRet = GZipBuffToBuffDecompress
							   ( this->_str, // destination
								 &nUnZippedLength, // decompressed length
								 pSrc->_str, // source
								 (unsigned int)pSrc->_len); // source len
	
	if (cRet > -1)
	{
		SetLength(cRet);
		return 1;
	}
	else
	{
		return 0; // "Compress failed. Code: " << cRet;
	}
}


// pSrc is the uncompressed GString
// 'this' contains the results
bool GString0::Compress( GString *pSrc )
{
	unsigned int nZippedLength = (unsigned int)pSrc->GetLength();

	
	// This sets the max size of compressed data, it is possible for compressed data to expand a little 
	// under worst case scenarios( which is data that is already compressed ). 
	// The actual formula for buffer expansion is = (n * .01) + 600, so our destination memory allocation will be 
	// a little bigger than the memory of what we are intending to compress.
	// Only when the pSrc contains compressed data(of various formats) can the data actually expand.

	SetPreAlloc( nZippedLength + (nZippedLength * .01) + 600 );

	int cRet = GZipBuffToBuffCompress(
									this->_str, // destination
								   &nZippedLength,	
								   pSrc->_str, // source
								   (unsigned int)pSrc->_len);
	if (cRet != 0)
	{
		return 0; // "Compress failed. Code: " << cRet;
	}
	else
	{
		SetLength(nZippedLength);
		return 1;
	}
}

// this Compress() assumes that the current contents of 'this' are the uncompressed data
// the source will be replaced with the compressed form of the data in 'this'
bool GString0::Compress( ) 
{
	GString strSrc(*this);
	return Compress (&strSrc);
}


// =============================================== END


#ifdef _UNICODE

#include <windows.h>

wchar_t *GString0::Unicode()
{

    if (_pWideStr)
	{
	    free(_pWideStr);
		_pWideStr = 0;
	}
    
    // Covert _str to Unicode
    int len = MultiByteToWideChar(CP_ACP, 0, _str, _len+1, NULL, 0) ;
    (wchar_t *)_pWideStr = (wchar_t*)malloc(sizeof(wchar_t) * len);
	
    MultiByteToWideChar(CP_ACP, 0, _str, -1, _pWideStr, _len);
	
    // it gets cleaned up either on object destruction or the next time this method is called
    return _pWideStr;
}

GString0::operator wchar_t * () 
{

    if (_pWideStr)
	{
	    free(_pWideStr);
		_pWideStr = 0;
	}
    
    // Covert _str to Unicode
    int len = MultiByteToWideChar(CP_ACP, 0, _str, _len, NULL, 0) ;

    int nAllocLen = (sizeof(wchar_t) * len);
	_pWideStr = (wchar_t*)malloc(nAllocLen+2);
	memset(_pWideStr,0,nAllocLen+2);
	
	MultiByteToWideChar(CP_ACP, 0, _str, _len, _pWideStr, nAllocLen);
	
    // it gets cleaned up either on object destruction or the next time this method is called
    return _pWideStr;
}


GString0 & GString0::operator+=(const wchar_t * unicodestr)
{
	int a = Gstrlen(unicodestr)+1;
	char *_p = new char[a];
	::WideCharToMultiByte( CP_ACP, 0, unicodestr, -1, _p, a, NULL, NULL);

	
	while (_p && *_p)
	{
		if (_len >= _max)
			resize();
		_str[_len] = *_p;
		_p++;
		_len++;
	}


	if (_len >= _max)
		resize();
	_str[_len] = 0;

	return *this;
}
GString0 & GString0::operator<<(const wchar_t * unicodestr)
{
	return *this += unicodestr;
}

#endif

//__int64 Gstrlen( const char *str )
//{
//	return strlen(str);
//}
