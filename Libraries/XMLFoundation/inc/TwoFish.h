#ifndef _TWO_FISH_H__
#define _TWO_FISH_H__

#ifdef _WIN32
#define _INTEL
#endif

#ifdef _ANDROID
#define _INTEL // ARM - Motorolla actually, but the _INTEL define does what we need
#endif

#ifdef _IOS
#define _INTEL // ARM/i386 but the _INTEL define does what we need
#endif

/* aes.h */

/* ---------- See examples at end of this file for typical usage -------- */

/* AES Cipher header file for ANSI C Submissions
	Lawrence E. Bassham III
	Computer Security Division
	National Institute of Standards and Technology

	This sample is to assist implementers developing to the
Cryptographic API Profile for AES Candidate Algorithm Submissions.
Please consult this document as a cross-reference.
	
	ANY CHANGES, WHERE APPROPRIATE, TO INFORMATION PROVIDED IN THIS FILE
MUST BE DOCUMENTED. CHANGES ARE ONLY APPROPRIATE WHERE SPECIFIED WITH
THE STRING "CHANGE POSSIBLE". FUNCTION CALLS AND THEIR PARAMETERS
CANNOT BE CHANGED. STRUCTURES CAN BE ALTERED TO ALLOW IMPLEMENTERS TO
INCLUDE IMPLEMENTATION SPECIFIC INFORMATION.
*/

/* Includes:
	Standard include files
*/

#include	<stdio.h>
#include	"GString.h"
//#include	"platform.h"			/* platform-specific defines */
/***************************************************************************
	PLATFORM.H	-- Platform-specific defines for TWOFISH code

	Submitters:
		Bruce Schneier, Counterpane Systems
		Doug Whiting,	Hi/fn
		John Kelsey,	Counterpane Systems
		Chris Hall,		Counterpane Systems
		David Wagner,	UC Berkeley
			
	Code Author:		Doug Whiting,	Hi/fn
		
	Version  1.00		April 1998
		
	Copyright 1998, Hi/fn and Counterpane Systems.  All rights reserved.
		
	Notes:
		*	Tab size is set to 4 characters in this file

***************************************************************************/

/* use intrinsic rotate if possible */
#define	ROL(x,n) (((x) << ((n) & 0x1F)) | ((x) >> (32-((n) & 0x1F))))
#define	ROR(x,n) (((x) >> ((n) & 0x1F)) | ((x) << (32-((n) & 0x1F))))

#if (0) && defined(__BORLANDC__) && (__BORLANDC__ >= 0x462)
#error "!!!This does not work for some reason!!!"
#include	<stdlib.h>					/* get prototype for _lrotl() , _lrotr() */
#pragma inline __lrotl__
#pragma inline __lrotr__
#undef	ROL								/* get rid of inefficient definitions */
#undef	ROR
#define	ROL(x,n)	__lrotl__(x,n)		/* use compiler intrinsic rotations */
#define	ROR(x,n)	__lrotr__(x,n)
#endif

#ifdef _MSC_VER
#include	<stdlib.h>					/* get prototypes for rotation functions */
#undef	ROL
#undef	ROR
#pragma intrinsic(_lrotl,_lrotr)		/* use intrinsic compiler rotations */
#define	ROL(x,n)	_lrotl(x,n)			
#define	ROR(x,n)	_lrotr(x,n)
#endif

#ifndef _INTEL
#ifdef	__BORLANDC__
#define	_INTEL					300		/* make sure this is defined for Intel CPUs */
#endif
#endif

#ifdef _INTEL 
#define		FishLittleEndian		1		/* e.g., 1 for Pentium, 0 for 68K */
#define		ALIGN32				0		/* need dword alignment? (no for Pentium) */
#else	/* non-Intel platforms */
#define		FishLittleEndian		0		/* (assume big endian */
#define		ALIGN32				1		/* (assume need alignment for non-Intel) */
#endif


#if FishLittleEndian
#define		Bswap(x)			(x)		/* NOP for little-endian machines */
#define		ADDR_XOR			0		/* NOP for little-endian machines */
#else
#define		Bswap(x)			((ROR(x,8) & 0xFF00FF00) | (ROL(x,8) & 0x00FF00FF))
#define		ADDR_XOR			3		/* convert byte address in dword */
#endif

/*	Macros for extracting bytes from dwords (correct for endianness) */
#define	_b(x,N)	(((BYTE *)&x)[((N) & 3) ^ ADDR_XOR]) /* pick bytes out of a dword */

#define		b0(x)			_b(x,0)		/* extract LSB of DWORD */
#define		b1(x)			_b(x,1)
#define		b2(x)			_b(x,2)
#define		b3(x)			_b(x,3)		/* extract MSB of DWORD */






/*	Defines:
		Add any additional defines you need
*/

#define 	DIR_ENCRYPT 	0 		/* Are we encrpyting? */
#define 	DIR_DECRYPT 	1 		/* Are we decrpyting? */
#define 	MODE_ECB 		1 		/* Are we ciphering in ECB mode? */
#define 	MODE_CBC 		2 		/* Are we ciphering in CBC mode? */
#define 	MODE_CFB1 		3 		/* Are we ciphering in 1-bit CFB mode? */

#define 	TRUE 			1
#define 	FALSE 			0

#define 	BAD_KEY_DIR 		-1	/* Key direction is invalid (unknown value) */
#define 	BAD_KEY_MAT 		-2	/* Key material not of correct length */
#define 	BAD_KEY_INSTANCE 	-3	/* Key passed is not valid */
#define 	BAD_CIPHER_MODE 	-4 	/* Params struct passed to cipherInit invalid */
#define 	BAD_CIPHER_STATE 	-5 	/* Cipher in wrong state (e.g., not initialized) */

/* CHANGE POSSIBLE: inclusion of algorithm specific defines */
/* TWOFISH specific definitions */
#define		MAX_KEY_SIZE		64	/* # of ASCII chars needed to represent a key */
#define		MAX_IV_SIZE			16	/* # of bytes needed to represent an IV */
#define		BAD_INPUT_LEN		-6	/* inputLen not a multiple of block size */
#define		BAD_PARAMS			-7	/* invalid parameters */
#define		BAD_IV_MAT			-8	/* invalid IV text */
#define		BAD_ENDIAN			-9	/* incorrect endianness define */
#define		BAD_ALIGN32			-10	/* incorrect 32-bit alignment */

#define		BLOCK_SIZE			128	/* number of bits per block */
#define		MAX_ROUNDS			 16	/* max # rounds (for allocating subkey array) */
#define		ROUNDS_128			 16	/* default number of rounds for 128-bit keys*/
#define		ROUNDS_192			 16	/* default number of rounds for 192-bit keys*/
#define		ROUNDS_256			 16	/* default number of rounds for 256-bit keys*/
#define		MAX_KEY_BITS		256	/* max number of bits of key */
#define		MIN_KEY_BITS		128	/* min number of bits of key (zero pad) */
#define		VALID_SIG	 0x48534946	/* initialization signature ('FISH') */
#define		MCT_OUTER			400	/* MCT outer loop */
#define		MCT_INNER		  10000	/* MCT inner loop */
#define		REENTRANT			  1	/* nonzero forces reentrant code (slightly slower) */

#define		INPUT_WHITEN		0	/* subkey array indices */
#define		OUTPUT_WHITEN		( INPUT_WHITEN + BLOCK_SIZE/32)
#define		ROUND_SUBKEYS		(OUTPUT_WHITEN + BLOCK_SIZE/32)	/* use 2 * (# rounds) */
#define		TOTAL_SUBKEYS		(ROUND_SUBKEYS + 2*MAX_ROUNDS)

/* Typedefs:
	Typedef'ed data storage elements. Add any algorithm specific
	parameters at the bottom of the structs as appropriate.
*/

typedef unsigned char BYTE;

// 32-bit unsigned quantity 
#ifdef _WIN32
//	typedef __int32 DWORD;
//	typedef	unsigned long DWORD;
	typedef unsigned long DWORD;		
#elif _LINUX
	#include <linux/types.h>
	typedef __u32 DWORD;
#elif __sun
	#include <sys/types.h>
	typedef uint32_t DWORD;
#elif _HPUX
	#include <sys/_inttypes.h>
	typedef uint32_t DWORD;
#elif _IOS
	#include <inttypes.h>
	typedef uint32_t DWORD;
#endif


typedef DWORD fullSbox[4][256];

/* The structure for key information */
typedef struct 
	{
	BYTE direction;					/* Key used for encrypting or decrypting? */
#if ALIGN32
	BYTE dummyAlign[3];				/* keep 32-bit alignment */
#endif
	int  keyLen;					/* Length of the key */
	char keyMaterial[MAX_KEY_SIZE+4];/* Raw key data in ASCII */

	/* Twofish-specific parameters: */
	DWORD keySig;					/* set to VALID_SIG by makeKey() */
	int	  numRounds;				/* number of rounds in cipher */
	DWORD key32[MAX_KEY_BITS/32];	/* actual key bits, in dwords */
	DWORD sboxKeys[MAX_KEY_BITS/64];/* key bits used for S-boxes */
	DWORD subKeys[TOTAL_SUBKEYS];	/* round subkeys, input/output whitening bits */
#if REENTRANT
	fullSbox sBox8x32;				/* fully expanded S-box */
  #if defined(COMPILE_KEY) && defined(USE_ASM)
#undef	VALID_SIG
#define	VALID_SIG	 0x504D4F43		/* 'COMP':  C is compiled with -DCOMPILE_KEY */
	DWORD cSig1;					/* set after first "compile" (zero at "init") */
	void *encryptFuncPtr;			/* ptr to asm encrypt function */
	void *decryptFuncPtr;			/* ptr to asm decrypt function */
	DWORD codeSize;					/* size of compiledCode */
	DWORD cSig2;					/* set after first "compile" */
	BYTE  compiledCode[5000];		/* make room for the code itself */
  #endif
#endif
	} keyInstance;

/* The structure for cipher information */
typedef struct 
	{
	BYTE  mode;						/* MODE_ECB, MODE_CBC, or MODE_CFB1 */
#if ALIGN32
	BYTE dummyAlign[3];				/* keep 32-bit alignment */
#endif
	BYTE  IV[MAX_IV_SIZE];			/* CFB1 iv bytes  (CBC uses iv32) */

	/* Twofish-specific parameters: */
	DWORD cipherSig;				/* set to VALID_SIG by cipherInit() */
	DWORD iv32[BLOCK_SIZE/32];		/* CBC IV bytes arranged as dwords */
	} cipherInstance;


/* Function protoypes */
int makeKey(keyInstance *key, BYTE direction, int keyLen, char *keyMaterial);

int cipherInit(cipherInstance *cipher, BYTE mode, char *IV);

int BlockEncrypt(cipherInstance *cipher, keyInstance *key, BYTE *input,	int inputLen, BYTE *outBuffer);

int BlockDecrypt(cipherInstance *cipher, keyInstance *key, BYTE *input,	int inputLen, BYTE *outBuffer);

int	reKey(keyInstance *key);	/* do key schedule using modified key.keyDwords */

int Twofish(int Operation, //1=Encrypt otherwise Decrypt
			int mode, // MODE_ECB | MODE_CBC
			int keySize, // keySize must be 128, 192, or 256 
			void*pKey, // 128 bits(16 bytes) or twice that.
			char *pzDataIn, // raw data or crypted input 
			int DataInSize, // length of pzDataIn 
			char *cryptDest); // Destination

// returns 1 on success, 0 on Fail with description in  strErrorOut
int CipherDir(int bIsEncrypt, GString &strPass, const char *pzDirectory, int bRecurseDeep, GString &strErrorOut);
int FileEncrypt(GString &strKey, const char *strInFile, const char *strOutFile, GString &strErrorOut);
int FileDecrypt(GString &strKey, const char *strInFile, const char *strOutFile, GString &strErrorOut);
int FileDecryptToMemory(GString &strKey, const char *strInFile, char **pDest, int *nDestLen, GString &strErrorOut);
int EncryptMemoryToFile(GString &strKey, const char *pIn,long pInLen,const char *strOutFile, GString &strErrorOut);


// these two are 'better' than the above
// returns 1 on success, 0 on Fail with description in  strErrorOut
bool EncryptMemoryToMemory(GString &strKey, const char *pIn,__int64 pInLen,GString &strDest, GString &strErrorOut);
bool DecryptMemoryToMemory(GString &strKey, const char *pIn,__int64 pInLen, GString &strDest, GString &strErrorOut);



/* API to check table usage, for use in ECB_TBL KAT */
#define		TAB_DISABLE			0
#define		TAB_ENABLE			1
#define		TAB_RESET			2
#define		TAB_QUERY			3
#define		TAB_MIN_QUERY		50

int TableOp(int op);



#if BLOCK_SIZE == 128			/* optimize block copies */
#define		Copy1(d,s,N)	((DWORD *)(d))[N] = ((DWORD *)(s))[N]
#define		BlockCopy(d,s)	{ Copy1(d,s,0);Copy1(d,s,1);Copy1(d,s,2);Copy1(d,s,3); }
#else
#define		BlockCopy(d,s)	{ memcpy(d,s,BLOCK_SIZE/8); }
#endif



class CipherData
{
public:
	int CipherKeyBits;
	const char *pzCipherPass;
	keyInstance    ki;		//  key information, including tables
	cipherInstance ci;		// keeps mode (ECB, CBC) and IV */
	int bIsInitted;

	CipherData(const char *pKey, int nDIR);
	int blockEncrypt(BYTE *input,int inputLen, BYTE *outBuffer);
	int blockDecrypt(BYTE *input,int inputLen, BYTE *outBuffer);
};




#endif // _TWO_FISH_H__