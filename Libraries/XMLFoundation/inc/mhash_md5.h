// This file contains 3 implementations of MD5 found on the internet.

//#define USE_IMPL_1
//#define USE_IMPL_2
#define USE_IMPL_3


#ifndef MHASH_MD5_H
#define MHASH_MD5_H



#ifdef USE_IMPL_1

/* typedef a 32 bit type */
#ifdef _WIN32
	typedef __int32 UINT4;
#elif _LINUX
	#include <linux/types.h>
	typedef __u32 UINT4;
#elif __sun
	#include <sys/types.h>
	typedef uint32_t UINT4;
#elif _HPUX
	#include <sys/_inttypes.h>
	typedef uint32_t UINT4;
#else
	--ERROR--
#endif


/* Data structure for MD5 (Message Digest) computation */
typedef struct {
  UINT4 i[2];                   /* number of _bits_ handled mod 2^64 */
  UINT4 buf[4];                                    /* scratch buffer */
  unsigned char in[64];                              /* input buffer */
  unsigned char digest[16];     /* actual digest after MD5Final call */
} MD5_CTX;

void MD5Init (MD5_CTX *mdContext);
void MD5Update (MD5_CTX *mdContext, unsigned char *inBuf, unsigned int inLen);
void MD5Final (MD5_CTX *mdContext);
void Hash(void* pzData, int nDataLength, unsigned char *pDest, int nBittage);

#endif // USE_IMPL_1




#ifdef USE_IMPL_2

typedef unsigned char *POINTER;
typedef unsigned char word8;

#ifdef _WIN32
	typedef __int32 word32;
#elif _LINUX
	#include <linux/types.h>
	typedef __u32 word32;
#elif __sun
	#include <sys/types.h>
	typedef uint32_t word32;
#elif _HPUX
	#include <sys/_inttypes.h>
	typedef uint32_t word32;
#else
	--ERROR--
#endif




/*
   MD5.H - header file for MD5C.C
 */

/*
   Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
   rights reserved.

   License to copy and use this software is granted provided that it
   is identified as the "RSA Data Security, Inc. MD5 Message-Digest
   Algorithm" in all material mentioning or referencing this software
   or this function.

   License is also granted to make and use derivative works provided
   that such works are identified as "derived from the RSA Data
   Security, Inc. MD5 Message-Digest Algorithm" in all material
   mentioning or referencing the derived work.

   RSA Data Security, Inc. makes no representations concerning either
   the merchantability of this software or the suitability of this
   software for any particular purpose. It is provided "as is"
   without express or implied warranty of any kind.
   These notices must be retained in any copies of any part of this
   documentation and/or software.
 */


/*
   MD5 context. 
 */
typedef struct {
	word32 state[4];			/*
								   state (ABCD) 
								 */
	word32 count[2];			/*
								   number of bits, modulo 2^64 (lsb first) 
								 */
	word8 buffer[64];			/*
								   input buffer 
								 */
} MD5_CTX;


void MD5Init (MD5_CTX *);
void MD5Update (MD5_CTX *, const unsigned char *, unsigned int);
void MD5Final (MD5_CTX *, unsigned char[16]);
void Hash(void* pzData, int nDataLength, unsigned char *pDest, int nBittage);

#endif // USE_IMPL_2


#ifdef USE_IMPL_3

#define MD5_DIGESTSIZE 16

/* UINT4 defines a four byte word */
typedef unsigned int UINT4;


/* MD5 context. */
typedef struct {
    UINT4 state[4];		/* state (ABCD) */
    UINT4 count[2];		/* number of bits, modulo 2^64 (lsb first) */
    unsigned char buffer[64];	/* input buffer */
} AP_MD5_CTX;

/*
 * Define the Magic String prefix that identifies a password as being
 * hashed using our algorithm.
 */
#define AP_MD5PW_ID "$xml1$"
#define AP_MD5PW_IDLEN 6

void ap_MD5Init(AP_MD5_CTX *context);
void ap_MD5Update(AP_MD5_CTX *context, const unsigned char *input,
			      unsigned int inputLen);
void ap_MD5Final(unsigned char digest[MD5_DIGESTSIZE],
			     AP_MD5_CTX *context);
void ap_MD5Encode(const unsigned char *password,
			      const unsigned char *salt,
			      char *result, unsigned int nbytes);
void ap_to64(char *s, unsigned long v, int n);
void Hash(void* pzData, int nDataLength, unsigned char *pDest, int nBittage);



#endif //IMPL_3



#endif // #ifndef MHASH_MD5_H
