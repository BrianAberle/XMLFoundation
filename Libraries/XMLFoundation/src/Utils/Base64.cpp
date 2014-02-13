//	No copyright.  This source was found in Academic postings.
//	Author unknown.

#include "GlobalInclude.h"
static char SOURCE_FILE[] = __FILE__;

#include "Base64.h"
#include <memory.h> // for : memcpy()
#include <string.h> // needed for memcpy() definition on Android build

bool BufferInit( BUFFER *pB ) 
{ 
    pB->pBuf = 0; 
    pB->cLen = 0; 
 
    return true; 
} 
 
void BufferTerminate( BUFFER *pB ) 
{ 
    if ( pB->pBuf != 0 ) 
    { 
        delete [] pB->pBuf; 
        pB->pBuf = 0; 
        pB->cLen = 0; 
    } 
} 
 
unsigned char *BufferQueryPtr( BUFFER * pB ) 
{ 
    return pB->pBuf; 
} 
 
bool BufferResize( BUFFER *pB, unsigned int cNewL ) 
{ 
    unsigned char *pN; 
    if ( cNewL > (unsigned int)pB->cLen ) 
    { 
        pN = new unsigned char[cNewL]; 
        if ( pB->pBuf ) 
        { 
            memcpy( pN, pB->pBuf, pB->cLen ); 
            delete [] pB->pBuf; 
        } 
        pB->pBuf = pN; 
        pB->cLen = cNewL; 
    } 
 
    return true; 
} 

const int pr2six[256] =
{ 
	64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64, 
	64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,62,64,64,64,63, 
	52,53,54,55,56,57,58,59,60,61,64,64,64,64,64,64,64,0,1,2,3,4,5,6,7,8,9, 
	10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,64,64,64,64,64,64,26,27, 
	28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51, 
	64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64, 
	64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64, 
	64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64, 
	64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64, 
	64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64, 
	64,64,64,64,64,64,64,64,64,64,64,64,64 
}; 
 
char six2pr[64] = 
{ 
	'A','B','C','D','E','F','G','H','I','J','K','L','M', 
	'N','O','P','Q','R','S','T','U','V','W','X','Y','Z', 
	'a','b','c','d','e','f','g','h','i','j','k','l','m', 
	'n','o','p','q','r','s','t','u','v','w','x','y','z', 
	'0','1','2','3','4','5','6','7','8','9','+','/' 
}; 
 
/*
	Routine Description: 

		uudecode a string of data 

	Arguments: 

		bufcoded            pointer to uuencoded data 
		pbuffdecoded        pointer to output BUFFER structure 
		pcbDecoded          number of decode bytes 

	Return Value: 

		Returns TRUE is successful; otherwise false is returned. 
*/ 
bool uudecode(char *bufcoded, 
			  BUFFER *pbuffdecoded, 
			  unsigned int *pcbDecoded, 
			  bool bTrunc) 
{ 
	*pcbDecoded = 0;

	int nbytesdecoded; 
	char *bufin = bufcoded; 
	unsigned char *bufout; 
	int nprbytes; 

	/* Strip leading whitespace. */ 
	while(*bufcoded == ' ' || *bufcoded == '\t') 
		bufcoded++; 

	/* 
		Figure out how many characters are in the input buffer. 
		If this would decode into more bytes than would fit into 
		the output buffer, adjust the number of input bytes downwards. 
	*/ 
	bufin = bufcoded; 
	while(pr2six[*(bufin++)] <= 63); 
	nprbytes = bufin - bufcoded - 1; 
	nbytesdecoded = ((nprbytes+3)/4) * 3; 

	if ( !BufferResize( pbuffdecoded, nbytesdecoded + 4 )) 
		return false; 

	bufout = (unsigned char *) BufferQueryPtr(pbuffdecoded); 

	bufin = bufcoded; 

	while (nprbytes > 0) 
	{ 
		*(bufout++) = 
			(unsigned char) (pr2six[*bufin] << 2 | pr2six[bufin[1]] >> 4); 
		(*pcbDecoded)++;

		*(bufout++) = 
			(unsigned char) (pr2six[bufin[1]] << 4 | pr2six[bufin[2]] >> 2); 
		(*pcbDecoded)++;
		
		*(bufout++) = 
			(unsigned char) (pr2six[bufin[2]] << 6 | pr2six[bufin[3]]); 
		(*pcbDecoded)++;

		bufin += 4; 
		nprbytes -= 4; 
	} 

	if (nprbytes & 03) 
	{ 
		if (pr2six[bufin[-2]] > 63) 
		{
			nbytesdecoded -= 2; 
			(*pcbDecoded) -= 2;
		}
		else 
		{
			nbytesdecoded -= 1; 
			(*pcbDecoded) -= 1;
		}
	} 

	int nReminder = (*pcbDecoded % 4);
	switch (nReminder)
	{
		case 1 :
//		case 3 :
			if (bTrunc != false)
				(*pcbDecoded)--;
	}

	((char *)BufferQueryPtr(pbuffdecoded))[nbytesdecoded] = '\0'; 

	return true; 
} 
 
/*
	Routine Description: 

		uuencode a string of data 

	Arguments: 

		bufin           pointer to data to encode 
		nbytes          number of bytes to encode 
		pbuffEncoded    pointer to output BUFFER structure 

	Return Value: 

		Returns TRUE is successful; otherwise false is returned. 
*/ 
bool uuencode(unsigned char *bufin, unsigned int nbytes, BUFFER *pbuffEncoded) 
{ 
	unsigned char *outptr; 
	unsigned int i; 

	/*  Resize the buffer to 133% of the incoming data */
	if ( !BufferResize( pbuffEncoded, nbytes + ((nbytes + 3) / 3) + 4)) 
		return false; 

	(*pbuffEncoded).cLen = 0;

	outptr = (unsigned char *) BufferQueryPtr(pbuffEncoded); 

	for (i = 0; i < nbytes; i += 3) 
	{ 
		*(outptr++) = six2pr[*bufin >> 2];            /* c1 */ 
		(*pbuffEncoded).cLen++;
		*(outptr++) = six2pr[((*bufin << 4) & 060) | ((bufin[1] >> 4) & 017)]; /*c2*/ 
		(*pbuffEncoded).cLen++;
		*(outptr++) = six2pr[((bufin[1] << 2) & 074) | ((bufin[2] >> 6) & 03)];/*c3*/ 
		(*pbuffEncoded).cLen++;
		*(outptr++) = six2pr[bufin[2] & 077];         /* c4 */ 
		(*pbuffEncoded).cLen++;

		bufin += 3; 
	} 

	/* 
		If nbytes was not a multiple of 3, then we have encoded too 
		many characters.  Adjust appropriately. 
	*/ 
	if (i == nbytes+1) 
	{ 
		/* There were only 2 bytes in that last group */ 
		outptr[-1] = '='; 
		(*pbuffEncoded).cLen++;
	} 
	else if (i == nbytes+2) 
	{ 
		/* There was only 1 byte in that last group */ 
		outptr[-1] = '='; 
		(*pbuffEncoded).cLen++;
		outptr[-2] = '='; 
		(*pbuffEncoded).cLen++;
	} 

	*outptr = '\0'; 

	return true; 
} 
