#ifndef _SHA_256_H__
#define _SHA_256_H__

#include "xmlDefines.h" // for __int64
#ifdef _IOS
	#include <inttypes.h>
	#ifndef __int64
		#define	__int64 __int64_t
	#endif
	#define LTC_NO_ROLC
#endif

//
// Sample use:
// unsigned char digest[32];
// SHA256Hash("Hello World",strlen("Hello World"),digest);
//
void SHA256Hash(unsigned char *pzData, __int64 nDataLength, unsigned char *pDigestDest);



//
// Sample use:
// unsigned char digest[32];
// if ( SHA256Hash("c:\\TheFile.777 ", digest)) 
// {
//   The file was found and the digest was created
// }
int SHA256FileHash(const char*pzFileName, unsigned char *pDigestDest, __int64 nBytesToHash = 0);

//
// Sample use to get a digest by passing the data in blocks
//
//	unsigned char digest[32];
//	hash_state md;
//	sha256_init(&md);
//  while( !done )
//		sha256_process(&md, (unsigned char *)pDataChunk, nChunkSize );
//	sha256_done(&md, digest);



struct sha256_state {
    __int64 length;
    unsigned long state[8], curlen;
    unsigned char buf[64];
};

typedef union Hash_state {
    char dummy[1];
    struct sha256_state sha256;
    void *data;
} hash_state;

int sha256_init(hash_state * md);
int sha256_process(hash_state * md, const unsigned char *in, __int64 inlen);
int sha256_done(hash_state * md, unsigned char *hash);


#endif // _SHA_256_H__