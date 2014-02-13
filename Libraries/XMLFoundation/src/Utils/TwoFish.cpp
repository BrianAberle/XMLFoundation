#include "GlobalInclude.h"
static char SOURCE_FILE[] = __FILE__;

#include "TwoFish.h"
#include "GDirectory.h"
#include "SHA256.h"
#include "GStringList.h"
 #include <string.h> // for strlen() and  memcpy()

#ifndef _WIN32
 #include <stdlib.h> // for rand()
#endif




// this source file contains both the Optimized and Reference implementations.
// if OPTIMIZED_TWOFISH is defined, the Optimized version will be used.
#define OPTIMIZED_TWOFISH

#ifdef _ANDROID
#define FishLittleEndian 1
#endif



#ifdef OPTIMIZED_TWOFISH
/***************************************************************************
	TWOFISH2.C	-- Optimized C API calls for TWOFISH AES submission
	// Cipher Block Chaining
	// Electronic Codebook

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
		*	Optimized version
		*	Tab size is set to 4 characters in this file

***************************************************************************/
#include	<memory.h>
#include	<assert.h>

//#include	"table.h"
/***************************************************************************
	TABLE.H	-- Tables, macros, constants for Twofish S-boxes and MDS matrix

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
		*	These definitions should be used in optimized and unoptimized
			versions to insure consistency.

***************************************************************************/

/* for computing subkeys */
#define	SK_STEP			0x02020202u
#define	SK_BUMP			0x01010101u
#define	SK_ROTL			9

/* Reed-Solomon code parameters: (12,8) reversible code
	g(x) = x**4 + (a + 1/a) x**3 + a x**2 + (a + 1/a) x + 1
   where a = primitive root of field generator 0x14D */
#define	RS_GF_FDBK		0x14D		/* field generator */
#define	RS_rem(x)		\
	{ BYTE  b  = (BYTE) (x >> 24);											 \
	  DWORD g2 = ((b << 1) ^ ((b & 0x80) ? RS_GF_FDBK : 0 )) & 0xFF;		 \
	  DWORD g3 = ((b >> 1) & 0x7F) ^ ((b & 1) ? RS_GF_FDBK >> 1 : 0 ) ^ g2 ; \
	  x = (x << 8) ^ (g3 << 24) ^ (g2 << 16) ^ (g3 << 8) ^ b;				 \
	}

/*	Macros for the MDS matrix
*	The MDS matrix is (using primitive polynomial 169):
*      01  EF  5B  5B
*      5B  EF  EF  01
*      EF  5B  01  EF
*      EF  01  EF  5B
*----------------------------------------------------------------
* More statistical properties of this matrix (from MDS.EXE output):
*
* Min Hamming weight (one byte difference) =  8. Max=26.  Total =  1020.
* Prob[8]:      7    23    42    20    52    95    88    94   121   128    91
*             102    76    41    24     8     4     1     3     0     0     0
* Runs[8]:      2     4     5     6     7     8     9    11
* MSBs[8]:      1     4    15     8    18    38    40    43
* HW= 8: 05040705 0A080E0A 14101C14 28203828 50407050 01499101 A080E0A0 
* HW= 9: 04050707 080A0E0E 10141C1C 20283838 40507070 80A0E0E0 C6432020 07070504 
*        0E0E0A08 1C1C1410 38382820 70705040 E0E0A080 202043C6 05070407 0A0E080E 
*        141C101C 28382038 50704070 A0E080E0 4320C620 02924B02 089A4508 
* Min Hamming weight (two byte difference) =  3. Max=28.  Total = 390150.
* Prob[3]:      7    18    55   149   270   914  2185  5761 11363 20719 32079
*           43492 51612 53851 52098 42015 31117 20854 11538  6223  2492  1033
* MDS OK, ROR:   6+  7+  8+  9+ 10+ 11+ 12+ 13+ 14+ 15+ 16+
*               17+ 18+ 19+ 20+ 21+ 22+ 23+ 24+ 25+ 26+
*/
#define	MDS_GF_FDBK		0x169	/* primitive polynomial for GF(256)*/
#define	LFSR1(x) ( ((x) >> 1)  ^ (((x) & 0x01) ?   MDS_GF_FDBK/2 : 0))
#define	LFSR2(x) ( ((x) >> 2)  ^ (((x) & 0x02) ?   MDS_GF_FDBK/2 : 0)  \
							   ^ (((x) & 0x01) ?   MDS_GF_FDBK/4 : 0))

#define	Mx_1(x) ((DWORD)  (x))		/* force result to dword so << will work */
#define	Mx_X(x) ((DWORD) ((x) ^            LFSR2(x)))	/* 5B */
#define	Mx_Y(x) ((DWORD) ((x) ^ LFSR1(x) ^ LFSR2(x)))	/* EF */

#define	M00		Mul_1
#define	M01		Mul_Y
#define	M02		Mul_X
#define	M03		Mul_X

#define	M10		Mul_X
#define	M11		Mul_Y
#define	M12		Mul_Y
#define	M13		Mul_1

#define	M20		Mul_Y
#define	M21		Mul_X
#define	M22		Mul_1
#define	M23		Mul_Y

#define	M30		Mul_Y
#define	M31		Mul_1
#define	M32		Mul_Y
#define	M33		Mul_X

#define	Mul_1	Mx_1
#define	Mul_X	Mx_X
#define	Mul_Y	Mx_Y

/*	Define the fixed p0/p1 permutations used in keyed S-box lookup.  
	By changing the following constant definitions for P_ij, the S-boxes will
	automatically get changed in all the Twofish source code. Note that P_i0 is
	the "outermost" 8x8 permutation applied.  See the f32() function to see
	how these constants are to be  used.
*/
#define	P_00	1					/* "outermost" permutation */
#define	P_01	0
#define	P_02	0
#define	P_03	(P_01^1)			/* "extend" to larger key sizes */
#define	P_04	1

#define	P_10	0
#define	P_11	0
#define	P_12	1
#define	P_13	(P_11^1)
#define	P_14	0

#define	P_20	1
#define	P_21	1
#define	P_22	0
#define	P_23	(P_21^1)
#define	P_24	0

#define	P_30	0
#define	P_31	1
#define	P_32	1
#define	P_33	(P_31^1)
#define	P_34	1

#define	p8(N)	P8x8[P_##N]			/* some syntax shorthand */

/* fixed 8x8 permutation S-boxes */

/***********************************************************************
*  07:07:14  05/30/98  [4x4]  TestCnt=256. keySize=128. CRC=4BD14D9E.
* maxKeyed:  dpMax = 18. lpMax =100. fixPt =  8. skXor =  0. skDup =  6. 
* log2(dpMax[ 6..18])=   --- 15.42  1.33  0.89  4.05  7.98 12.05
* log2(lpMax[ 7..12])=  9.32  1.01  1.16  4.23  8.02 12.45
* log2(fixPt[ 0.. 8])=  1.44  1.44  2.44  4.06  6.01  8.21 11.07 14.09 17.00
* log2(skXor[ 0.. 0])
* log2(skDup[ 0.. 6])=   ---  2.37  0.44  3.94  8.36 13.04 17.99
***********************************************************************/
BYTE P8x8[2][256]=
	{
/*  p0:   */
/*  dpMax      = 10.  lpMax      = 64.  cycleCnt=   1  1  1  0.         */
/* 817D6F320B59ECA4.ECB81235F4A6709D.BA5E6D90C8F32471.D7F4126E9B3085CA. */
/* Karnaugh maps:
*  0111 0001 0011 1010. 0001 1001 1100 1111. 1001 1110 0011 1110. 1101 0101 1111 1001. 
*  0101 1111 1100 0100. 1011 0101 0010 0000. 0101 1000 1100 0101. 1000 0111 0011 0010. 
*  0000 1001 1110 1101. 1011 1000 1010 0011. 0011 1001 0101 0000. 0100 0010 0101 1011. 
*  0111 0100 0001 0110. 1000 1011 1110 1001. 0011 0011 1001 1101. 1101 0101 0000 1100. 
*/
	{
	0xA9, 0x67, 0xB3, 0xE8, 0x04, 0xFD, 0xA3, 0x76, 
	0x9A, 0x92, 0x80, 0x78, 0xE4, 0xDD, 0xD1, 0x38, 
	0x0D, 0xC6, 0x35, 0x98, 0x18, 0xF7, 0xEC, 0x6C, 
	0x43, 0x75, 0x37, 0x26, 0xFA, 0x13, 0x94, 0x48, 
	0xF2, 0xD0, 0x8B, 0x30, 0x84, 0x54, 0xDF, 0x23, 
	0x19, 0x5B, 0x3D, 0x59, 0xF3, 0xAE, 0xA2, 0x82, 
	0x63, 0x01, 0x83, 0x2E, 0xD9, 0x51, 0x9B, 0x7C, 
	0xA6, 0xEB, 0xA5, 0xBE, 0x16, 0x0C, 0xE3, 0x61, 
	0xC0, 0x8C, 0x3A, 0xF5, 0x73, 0x2C, 0x25, 0x0B, 
	0xBB, 0x4E, 0x89, 0x6B, 0x53, 0x6A, 0xB4, 0xF1, 
	0xE1, 0xE6, 0xBD, 0x45, 0xE2, 0xF4, 0xB6, 0x66, 
	0xCC, 0x95, 0x03, 0x56, 0xD4, 0x1C, 0x1E, 0xD7, 
	0xFB, 0xC3, 0x8E, 0xB5, 0xE9, 0xCF, 0xBF, 0xBA, 
	0xEA, 0x77, 0x39, 0xAF, 0x33, 0xC9, 0x62, 0x71, 
	0x81, 0x79, 0x09, 0xAD, 0x24, 0xCD, 0xF9, 0xD8, 
	0xE5, 0xC5, 0xB9, 0x4D, 0x44, 0x08, 0x86, 0xE7, 
	0xA1, 0x1D, 0xAA, 0xED, 0x06, 0x70, 0xB2, 0xD2, 
	0x41, 0x7B, 0xA0, 0x11, 0x31, 0xC2, 0x27, 0x90, 
	0x20, 0xF6, 0x60, 0xFF, 0x96, 0x5C, 0xB1, 0xAB, 
	0x9E, 0x9C, 0x52, 0x1B, 0x5F, 0x93, 0x0A, 0xEF, 
	0x91, 0x85, 0x49, 0xEE, 0x2D, 0x4F, 0x8F, 0x3B, 
	0x47, 0x87, 0x6D, 0x46, 0xD6, 0x3E, 0x69, 0x64, 
	0x2A, 0xCE, 0xCB, 0x2F, 0xFC, 0x97, 0x05, 0x7A, 
	0xAC, 0x7F, 0xD5, 0x1A, 0x4B, 0x0E, 0xA7, 0x5A, 
	0x28, 0x14, 0x3F, 0x29, 0x88, 0x3C, 0x4C, 0x02, 
	0xB8, 0xDA, 0xB0, 0x17, 0x55, 0x1F, 0x8A, 0x7D, 
	0x57, 0xC7, 0x8D, 0x74, 0xB7, 0xC4, 0x9F, 0x72, 
	0x7E, 0x15, 0x22, 0x12, 0x58, 0x07, 0x99, 0x34, 
	0x6E, 0x50, 0xDE, 0x68, 0x65, 0xBC, 0xDB, 0xF8, 
	0xC8, 0xA8, 0x2B, 0x40, 0xDC, 0xFE, 0x32, 0xA4, 
	0xCA, 0x10, 0x21, 0xF0, 0xD3, 0x5D, 0x0F, 0x00, 
	0x6F, 0x9D, 0x36, 0x42, 0x4A, 0x5E, 0xC1, 0xE0
	},
/*  p1:   */
/*  dpMax      = 10.  lpMax      = 64.  cycleCnt=   2  0  0  1.         */
/* 28BDF76E31940AC5.1E2B4C376DA5F908.4C75169A0ED82B3F.B951C3DE647F208A. */
/* Karnaugh maps:
*  0011 1001 0010 0111. 1010 0111 0100 0110. 0011 0001 1111 0100. 1111 1000 0001 1100. 
*  1100 1111 1111 1010. 0011 0011 1110 0100. 1001 0110 0100 0011. 0101 0110 1011 1011. 
*  0010 0100 0011 0101. 1100 1000 1000 1110. 0111 1111 0010 0110. 0000 1010 0000 0011. 
*  1101 1000 0010 0001. 0110 1001 1110 0101. 0001 0100 0101 0111. 0011 1011 1111 0010. 
*/
	{
	0x75, 0xF3, 0xC6, 0xF4, 0xDB, 0x7B, 0xFB, 0xC8, 
	0x4A, 0xD3, 0xE6, 0x6B, 0x45, 0x7D, 0xE8, 0x4B, 
	0xD6, 0x32, 0xD8, 0xFD, 0x37, 0x71, 0xF1, 0xE1, 
	0x30, 0x0F, 0xF8, 0x1B, 0x87, 0xFA, 0x06, 0x3F, 
	0x5E, 0xBA, 0xAE, 0x5B, 0x8A, 0x00, 0xBC, 0x9D, 
	0x6D, 0xC1, 0xB1, 0x0E, 0x80, 0x5D, 0xD2, 0xD5, 
	0xA0, 0x84, 0x07, 0x14, 0xB5, 0x90, 0x2C, 0xA3, 
	0xB2, 0x73, 0x4C, 0x54, 0x92, 0x74, 0x36, 0x51, 
	0x38, 0xB0, 0xBD, 0x5A, 0xFC, 0x60, 0x62, 0x96, 
	0x6C, 0x42, 0xF7, 0x10, 0x7C, 0x28, 0x27, 0x8C, 
	0x13, 0x95, 0x9C, 0xC7, 0x24, 0x46, 0x3B, 0x70, 
	0xCA, 0xE3, 0x85, 0xCB, 0x11, 0xD0, 0x93, 0xB8, 
	0xA6, 0x83, 0x20, 0xFF, 0x9F, 0x77, 0xC3, 0xCC, 
	0x03, 0x6F, 0x08, 0xBF, 0x40, 0xE7, 0x2B, 0xE2, 
	0x79, 0x0C, 0xAA, 0x82, 0x41, 0x3A, 0xEA, 0xB9, 
	0xE4, 0x9A, 0xA4, 0x97, 0x7E, 0xDA, 0x7A, 0x17, 
	0x66, 0x94, 0xA1, 0x1D, 0x3D, 0xF0, 0xDE, 0xB3, 
	0x0B, 0x72, 0xA7, 0x1C, 0xEF, 0xD1, 0x53, 0x3E, 
	0x8F, 0x33, 0x26, 0x5F, 0xEC, 0x76, 0x2A, 0x49, 
	0x81, 0x88, 0xEE, 0x21, 0xC4, 0x1A, 0xEB, 0xD9, 
	0xC5, 0x39, 0x99, 0xCD, 0xAD, 0x31, 0x8B, 0x01, 
	0x18, 0x23, 0xDD, 0x1F, 0x4E, 0x2D, 0xF9, 0x48, 
	0x4F, 0xF2, 0x65, 0x8E, 0x78, 0x5C, 0x58, 0x19, 
	0x8D, 0xE5, 0x98, 0x57, 0x67, 0x7F, 0x05, 0x64, 
	0xAF, 0x63, 0xB6, 0xFE, 0xF5, 0xB7, 0x3C, 0xA5, 
	0xCE, 0xE9, 0x68, 0x44, 0xE0, 0x4D, 0x43, 0x69, 
	0x29, 0x2E, 0xAC, 0x15, 0x59, 0xA8, 0x0A, 0x9E, 
	0x6E, 0x47, 0xDF, 0x34, 0x35, 0x6A, 0xCF, 0xDC, 
	0x22, 0xC9, 0xC0, 0x9B, 0x89, 0xD4, 0xED, 0xAB, 
	0x12, 0xA2, 0x0D, 0x52, 0xBB, 0x02, 0x2F, 0xA9, 
	0xD7, 0x61, 0x1E, 0xB4, 0x50, 0x04, 0xF6, 0xC2, 
	0x16, 0x25, 0x86, 0x56, 0x55, 0x09, 0xBE, 0x91
	}
	};


#if   defined(min_key)  && !defined(MIN_KEY)
#define	MIN_KEY		1			/* toupper() */
#elif defined(part_key) && !defined(PART_KEY)
#define	PART_KEY	1
#elif defined(zero_key) && !defined(ZERO_KEY)
#define	ZERO_KEY	1
#endif


#ifdef USE_ASM
extern	int	useAsm;				/* ok to use ASM code? */

typedef	int cdecl CipherProc
   (cipherInstance *cipher, keyInstance *key,BYTE *input,int inputLen,BYTE *outBuffer);
typedef int	cdecl KeySetupProc(keyInstance *key);

extern CipherProc	*blockEncrypt_86;	/* ptr to ASM functions */
extern CipherProc	*blockDecrypt_86;
extern KeySetupProc	*reKey_86;
extern DWORD		cdecl TwofishAsmCodeSize(void);
#endif

/*
+*****************************************************************************
*			Constants/Macros/Tables
-****************************************************************************/

#define		TFCONST					/* help syntax from C++, NOP here */

TFCONST		fullSbox MDStab;		/* not actually const.  Initialized ONE time */
int			needToBuildMDS=1;		/* is MDStab initialized yet? */

#define		BIG_TAB		0

#if BIG_TAB
BYTE		bigTab[4][256][256];	/* pre-computed S-box */
#endif

/* number of rounds for various key sizes:  128, 192, 256 */
/* (ignored for now in optimized code!) */
TFCONST int	numRounds[4]= {0,ROUNDS_128,ROUNDS_192,ROUNDS_256};

#if REENTRANT
#define		_sBox_	 key->sBox8x32
#else
static		fullSbox _sBox_;		/* permuted MDStab based on keys */
#endif
#define _sBox8_(N) (((BYTE *) _sBox_) + (N)*256)

/*------- see what level of S-box precomputation we need to do -----*/
#if   defined(ZERO_KEY)
#define	MOD_STRING	"(Zero S-box keying)"
#define	Fe32_128(x,R)	\
	(	MDStab[0][p8(01)[p8(02)[_b(x,R  )]^b0(SKEY[1])]^b0(SKEY[0])] ^	\
		MDStab[1][p8(11)[p8(12)[_b(x,R+1)]^b1(SKEY[1])]^b1(SKEY[0])] ^	\
		MDStab[2][p8(21)[p8(22)[_b(x,R+2)]^b2(SKEY[1])]^b2(SKEY[0])] ^	\
		MDStab[3][p8(31)[p8(32)[_b(x,R+3)]^b3(SKEY[1])]^b3(SKEY[0])] )
#define	Fe32_192(x,R)	\
	(	MDStab[0][p8(01)[p8(02)[p8(03)[_b(x,R  )]^b0(SKEY[2])]^b0(SKEY[1])]^b0(SKEY[0])] ^ \
		MDStab[1][p8(11)[p8(12)[p8(13)[_b(x,R+1)]^b1(SKEY[2])]^b1(SKEY[1])]^b1(SKEY[0])] ^ \
		MDStab[2][p8(21)[p8(22)[p8(23)[_b(x,R+2)]^b2(SKEY[2])]^b2(SKEY[1])]^b2(SKEY[0])] ^ \
		MDStab[3][p8(31)[p8(32)[p8(33)[_b(x,R+3)]^b3(SKEY[2])]^b3(SKEY[1])]^b3(SKEY[0])] )
#define	Fe32_256(x,R)	\
	(	MDStab[0][p8(01)[p8(02)[p8(03)[p8(04)[_b(x,R  )]^b0(SKEY[3])]^b0(SKEY[2])]^b0(SKEY[1])]^b0(SKEY[0])] ^ \
		MDStab[1][p8(11)[p8(12)[p8(13)[p8(14)[_b(x,R+1)]^b1(SKEY[3])]^b1(SKEY[2])]^b1(SKEY[1])]^b1(SKEY[0])] ^ \
		MDStab[2][p8(21)[p8(22)[p8(23)[p8(24)[_b(x,R+2)]^b2(SKEY[3])]^b2(SKEY[2])]^b2(SKEY[1])]^b2(SKEY[0])] ^ \
		MDStab[3][p8(31)[p8(32)[p8(33)[p8(34)[_b(x,R+3)]^b3(SKEY[3])]^b3(SKEY[2])]^b3(SKEY[1])]^b3(SKEY[0])] )

#define	GetSboxKey	DWORD SKEY[4];	/* local copy */ \
					memcpy(SKEY,key->sboxKeys,sizeof(SKEY));
/*----------------------------------------------------------------*/
#elif defined(MIN_KEY)
#define	MOD_STRING	"(Minimal keying)"
#define	Fe32_(x,R)(MDStab[0][p8(01)[_sBox8_(0)[_b(x,R  )]] ^ b0(SKEY0)] ^ \
				   MDStab[1][p8(11)[_sBox8_(1)[_b(x,R+1)]] ^ b1(SKEY0)] ^ \
				   MDStab[2][p8(21)[_sBox8_(2)[_b(x,R+2)]] ^ b2(SKEY0)] ^ \
				   MDStab[3][p8(31)[_sBox8_(3)[_b(x,R+3)]] ^ b3(SKEY0)])
#define sbSet(N,i,J,v) { _sBox8_(N)[i+J] = v; }
#define	GetSboxKey	DWORD SKEY0	= key->sboxKeys[0]		/* local copy */
/*----------------------------------------------------------------*/
#elif defined(PART_KEY)	
#define	MOD_STRING	"(Partial keying)"
#define	Fe32_(x,R)(MDStab[0][_sBox8_(0)[_b(x,R  )]] ^ \
				   MDStab[1][_sBox8_(1)[_b(x,R+1)]] ^ \
				   MDStab[2][_sBox8_(2)[_b(x,R+2)]] ^ \
				   MDStab[3][_sBox8_(3)[_b(x,R+3)]])
#define sbSet(N,i,J,v) { _sBox8_(N)[i+J] = v; }
#define	GetSboxKey	
/*----------------------------------------------------------------*/
#else	/* default is FULL_KEY */
#ifndef FULL_KEY
#define	FULL_KEY	1
#endif
#if BIG_TAB
#define	TAB_STR		" (Big table)"
#else
#define	TAB_STR
#endif
#ifdef COMPILE_KEY
#define	MOD_STRING	"(Compiled subkeys)" TAB_STR
#else
#define	MOD_STRING	"(Full keying)" TAB_STR
#endif
/* Fe32_ does a full S-box + MDS lookup.  Need to #define _sBox_ before use.
   Note that we "interleave" 0,1, and 2,3 to avoid cache bank collisions
   in optimized assembly language.
*/
#define	Fe32_(x,R) (_sBox_[0][2*_b(x,R  )] ^ _sBox_[0][2*_b(x,R+1)+1] ^	\
				    _sBox_[2][2*_b(x,R+2)] ^ _sBox_[2][2*_b(x,R+3)+1])
		/* set a single S-box value, given the input byte */
#define sbSet(N,i,J,v) { _sBox_[N&2][2*i+(N&1)+2*J]=MDStab[N][v]; }
#define	GetSboxKey	
#endif

const char *moduleDescription	= "Optimized C ";
const char *modeString			= MOD_STRING;


/* macro(s) for debugging help */
#define		CHECK_TABLE		0		/* nonzero --> compare against "slow" table */
#define		VALIDATE_PARMS	0		/* disable for full speed */

//#include	"debug.h"				/* debug display macros */

/* end of debug macros */

#ifdef GetCodeSize
extern DWORD Here(DWORD x);			/* return caller's address! */
DWORD TwofishCodeStart(void) { return Here(0); }
#endif

/*
+*****************************************************************************
*
* Function Name:	TableOp
*
* Function:			Handle table use checking
*
* Arguments:		op	=	what to do	(see TAB_* defns in AES.H)
*
* Return:			TRUE --> done (for TAB_QUERY)		
*
* Notes: This routine is for use in generating the tables KAT file.
*		 For this optimized version, we don't actually track table usage,
*		 since it would make the macros incredibly ugly.  Instead we just
*		 run for a fixed number of queries and then say we're done.
*
-****************************************************************************/
int TableOp(int op)
	{
	static int queryCnt=0;

	switch (op)
		{
		case TAB_DISABLE:
			break;
		case TAB_ENABLE:
			break;
		case TAB_RESET:
			queryCnt=0;
			break;
		case TAB_QUERY:
			queryCnt++;
			if (queryCnt < TAB_MIN_QUERY)
				return FALSE;
		}
	return TRUE;
	}


/*
+*****************************************************************************
*
* Function Name:	ParseHexDword
*
* Function:			Parse ASCII hex nibbles and fill in key/iv dwords
*
* Arguments:		bit			=	# bits to read
*					srcTxt		=	ASCII source
*					d			=	ptr to dwords to fill in
*					dstTxt		=	where to make a copy of ASCII source
*									(NULL ok)
*
* Return:			Zero if no error.  Nonzero --> invalid hex or length
*
* Notes:  Note that the parameter d is a DWORD array, not a byte array.
*	This routine is coded to work both for little-endian and big-endian
*	architectures.  The character stream is interpreted as a LITTLE-ENDIAN
*	byte stream, since that is how the Pentium works, but the conversion
*	happens automatically below. 
*
-****************************************************************************/
int ParseHexDword(int bits,TFCONST char *srcTxt,DWORD *d,char *dstTxt)
	{
	int i;
	char c;
	DWORD b;

	union	/* make sure FishLittleEndian is defined correctly */
		{
		BYTE  b[4];
		DWORD d[1];
		} v;
	v.d[0]=1;
	if (v.b[0 ^ ADDR_XOR] != 1)
		return BAD_ENDIAN;		/* make sure compile-time switch is set ok */

#if VALIDATE_PARMS
  #if ALIGN32
	if (((int)d) & 3)
		return BAD_ALIGN32;	
  #endif
#endif

	for (i=0;i*32<bits;i++)
		d[i]=0;					/* first, zero the field */

	for (i=0;i*4<bits;i++)		/* parse one nibble at a time */
		{						/* case out the hexadecimal characters */
		c=srcTxt[i];
		if (dstTxt) dstTxt[i]=c;
		if ((c >= '0') && (c <= '9'))
			b=c-'0';
		else if ((c >= 'a') && (c <= 'f'))
			b=c-'a'+10;
		else if ((c >= 'A') && (c <= 'F'))
			b=c-'A'+10;
		else
			return BAD_KEY_MAT;	/* invalid hex character */
		/* works for big and little endian! */
		d[i/8] |= b << (4*((i^1)&7));		
		}

	return 0;					/* no error */
	}


#if CHECK_TABLE
/*
+*****************************************************************************
*
* Function Name:	f32
*
* Function:			Run four bytes through keyed S-boxes and apply MDS matrix
*
* Arguments:		x			=	input to f function
*					k32			=	pointer to key dwords
*					keyLen		=	total key length (k32 --> keyLey/2 bits)
*
* Return:			The output of the keyed permutation applied to x.
*
* Notes:
*	This function is a keyed 32-bit permutation.  It is the major building
*	block for the Twofish round function, including the four keyed 8x8 
*	permutations and the 4x4 MDS matrix multiply.  This function is used
*	both for generating round subkeys and within the round function on the
*	block being encrypted.  
*
*	This version is fairly slow and pedagogical, although a smartcard would
*	probably perform the operation exactly this way in firmware.   For
*	ultimate performance, the entire operation can be completed with four
*	lookups into four 256x32-bit tables, with three dword xors.
*
*	The MDS matrix is defined in TABLE.H.  To multiply by Mij, just use the
*	macro Mij(x).
*
-****************************************************************************/
DWORD f32(DWORD x,TFCONST DWORD *k32,int keyLen)
	{
	BYTE  b[4];
	
	/* Run each byte thru 8x8 S-boxes, xoring with key byte at each stage. */
	/* Note that each byte goes through a different combination of S-boxes.*/

	*((DWORD *)b) = Bswap(x);	/* make b[0] = LSB, b[3] = MSB */
	switch (((keyLen + 63)/64) & 3)
		{
		case 0:		/* 256 bits of key */
			b[0] = p8(04)[b[0]] ^ b0(k32[3]);
			b[1] = p8(14)[b[1]] ^ b1(k32[3]);
			b[2] = p8(24)[b[2]] ^ b2(k32[3]);
			b[3] = p8(34)[b[3]] ^ b3(k32[3]);
			/* fall thru, having pre-processed b[0]..b[3] with k32[3] */
		case 3:		/* 192 bits of key */
			b[0] = p8(03)[b[0]] ^ b0(k32[2]);
			b[1] = p8(13)[b[1]] ^ b1(k32[2]);
			b[2] = p8(23)[b[2]] ^ b2(k32[2]);
			b[3] = p8(33)[b[3]] ^ b3(k32[2]);
			/* fall thru, having pre-processed b[0]..b[3] with k32[2] */
		case 2:		/* 128 bits of key */
			b[0] = p8(00)[p8(01)[p8(02)[b[0]] ^ b0(k32[1])] ^ b0(k32[0])];
			b[1] = p8(10)[p8(11)[p8(12)[b[1]] ^ b1(k32[1])] ^ b1(k32[0])];
			b[2] = p8(20)[p8(21)[p8(22)[b[2]] ^ b2(k32[1])] ^ b2(k32[0])];
			b[3] = p8(30)[p8(31)[p8(32)[b[3]] ^ b3(k32[1])] ^ b3(k32[0])];
		}

	/* Now perform the MDS matrix multiply inline. */
	return	((M00(b[0]) ^ M01(b[1]) ^ M02(b[2]) ^ M03(b[3]))	  ) ^
			((M10(b[0]) ^ M11(b[1]) ^ M12(b[2]) ^ M13(b[3])) <<  8) ^
			((M20(b[0]) ^ M21(b[1]) ^ M22(b[2]) ^ M23(b[3])) << 16) ^
			((M30(b[0]) ^ M31(b[1]) ^ M32(b[2]) ^ M33(b[3])) << 24) ;
	}
#endif	/* CHECK_TABLE */


/*
+*****************************************************************************
*
* Function Name:	RS_MDS_encode
*
* Function:			Use (12,8) Reed-Solomon code over GF(256) to produce
*					a key S-box dword from two key material dwords.
*
* Arguments:		k0	=	1st dword
*					k1	=	2nd dword
*
* Return:			Remainder polynomial generated using RS code
*
* Notes:
*	Since this computation is done only once per reKey per 64 bits of key,
*	the performance impact of this routine is imperceptible. The RS code
*	chosen has "simple" coefficients to allow smartcard/hardware implementation
*	without lookup tables.
*
-****************************************************************************/
DWORD RS_MDS_Encode(DWORD k0,DWORD k1)
	{
	int i,j;
	DWORD r;

	for (i=r=0;i<2;i++)
		{
		r ^= (i) ? k0 : k1;			/* merge in 32 more key bits */
		for (j=0;j<4;j++)			/* shift one byte at a time */
			RS_rem(r);				
		}
	return r;
	}


/*
+*****************************************************************************
*
* Function Name:	BuildMDS
*
* Function:			Initialize the MDStab array
*
* Arguments:		None.
*
* Return:			None.
*
* Notes:
*	Here we precompute all the fixed MDS table.  This only needs to be done
*	one time at initialization, after which the table is "TFCONST".
*
-****************************************************************************/
void BuildMDS(void)
	{
	int i;
	DWORD d;
	BYTE m1[2],mX[2],mY[4];

	for (i=0;i<256;i++)
		{
		m1[0]=P8x8[0][i];		/* compute all the matrix elements */
		mX[0]=(BYTE) Mul_X(m1[0]);
		mY[0]=(BYTE) Mul_Y(m1[0]);

		m1[1]=P8x8[1][i];
		mX[1]=(BYTE) Mul_X(m1[1]);
		mY[1]=(BYTE) Mul_Y(m1[1]);

#undef	Mul_1					/* change what the pre-processor does with Mij */
#undef	Mul_X
#undef	Mul_Y
#define	Mul_1	m1				/* It will now access m01[], m5B[], and mEF[] */
#define	Mul_X	mX				
#define	Mul_Y	mY

#define	SetMDS(N)					\
		b0(d) = M0##N[P_##N##0];	\
		b1(d) = M1##N[P_##N##0];	\
		b2(d) = M2##N[P_##N##0];	\
		b3(d) = M3##N[P_##N##0];	\
		MDStab[N][i] = d;

		SetMDS(0);				/* fill in the matrix with elements computed above */
		SetMDS(1);
		SetMDS(2);
		SetMDS(3);
		}
#undef	Mul_1
#undef	Mul_X
#undef	Mul_Y
#define	Mul_1	Mx_1			/* re-enable true multiply */
#define	Mul_X	Mx_X
#define	Mul_Y	Mx_Y
	
#if BIG_TAB
	{
	int j,k;
	BYTE *q0,*q1;

	for (i=0;i<4;i++)
		{
		switch (i)
			{
			case 0:	q0=p8(01); q1=p8(02);	break;
			case 1:	q0=p8(11); q1=p8(12);	break;
			case 2:	q0=p8(21); q1=p8(22);	break;
			case 3:	q0=p8(31); q1=p8(32);	break;
			}
		for (j=0;j<256;j++)
			for (k=0;k<256;k++)
				bigTab[i][j][k]=q0[q1[k]^j];
		}
	}
#endif

	needToBuildMDS=0;			/* NEVER modify the table again! */
	}

/*
+*****************************************************************************
*
* Function Name:	ReverseRoundSubkeys
*
* Function:			Reverse order of round subkeys to switch between encrypt/decrypt
*
* Arguments:		key		=	ptr to keyInstance to be reversed
*					newDir	=	new direction value
*
* Return:			None.
*
* Notes:
*	This optimization allows both blockEncrypt and blockDecrypt to use the same
*	"fallthru" switch statement based on the number of rounds.
*	Note that key->numRounds must be even and >= 2 here.
*
-****************************************************************************/
void ReverseRoundSubkeys(keyInstance *key,BYTE newDir)
	{
	DWORD t0,t1;
	register DWORD *r0=key->subKeys+ROUND_SUBKEYS;
	register DWORD *r1=r0 + 2*key->numRounds - 2;

	for (;r0 < r1;r0+=2,r1-=2)
		{
		t0=r0[0];			/* swap the order */
		t1=r0[1];
		r0[0]=r1[0];		/* but keep relative order within pairs */
		r0[1]=r1[1];
		r1[0]=t0;
		r1[1]=t1;
		}

	key->direction=newDir;
	}

/*
+*****************************************************************************
*
* Function Name:	Xor256
*
* Function:			Copy an 8-bit permutation (256 bytes), xoring with a byte
*
* Arguments:		dst		=	where to put result
*					src		=	where to get data (can be same asa dst)
*					b		=	byte to xor
*
* Return:			None
*
* Notes:
* 	BorlandC's optimization is terrible!  When we put the code inline,
*	it generates fairly good code in the *following* segment (not in the Xor256
*	code itself).  If the call is made, the code following the call is awful!
*	The penalty is nearly 50%!  So we take the code size hit for inlining for
*	Borland, while Microsoft happily works with a call.
*
-****************************************************************************/
#if defined(__BORLANDC__)	/* do it inline */
#define Xor32(dst,src,i) { ((DWORD *)dst)[i] = ((DWORD *)src)[i] ^ tmpX; } 
#define	Xor256(dst,src,b)				\
	{									\
	register DWORD tmpX=0x01010101u * b;\
	for (i=0;i<64;i+=4)					\
		{ Xor32(dst,src,i  ); Xor32(dst,src,i+1); Xor32(dst,src,i+2); Xor32(dst,src,i+3); }	\
	}
#else						/* do it as a function call */
void Xor256(void *dst,void *src,BYTE b)
	{
	register DWORD	x=b*0x01010101u;	/* replicate byte to all four bytes */
	register DWORD *d=(DWORD *)dst;
	register DWORD *s=(DWORD *)src;
#define X_8(N)	{ d[N]=s[N] ^ x; d[N+1]=s[N+1] ^ x; }
#define X_32(N)	{ X_8(N); X_8(N+2); X_8(N+4); X_8(N+6); }
	X_32(0 ); X_32( 8); X_32(16); X_32(24);	/* all inline */
	d+=32;	/* keep offsets small! */
	s+=32;
	X_32(0 ); X_32( 8); X_32(16); X_32(24);	/* all inline */
	}
#endif

/*
+*****************************************************************************
*
* Function Name:	reKey
*
* Function:			Initialize the Twofish key schedule from key32
*
* Arguments:		key			=	ptr to keyInstance to be initialized
*
* Return:			TRUE on success
*
* Notes:
*	Here we precompute all the round subkeys, although that is not actually
*	required.  For example, on a smartcard, the round subkeys can 
*	be generated on-the-fly	using f32()
*
-****************************************************************************/
int reKey(keyInstance *key)
	{
	int		i,j,k64Cnt,keyLen;
	int		subkeyCnt;
	DWORD	A=0,B=0,q;
	DWORD	sKey[MAX_KEY_BITS/64],k32e[MAX_KEY_BITS/64],k32o[MAX_KEY_BITS/64];
	BYTE	L0[256],L1[256];	/* small local 8-bit permutations */

#if VALIDATE_PARMS
  #if ALIGN32
	if (((int)key) & 3)
		return BAD_ALIGN32;
	if ((key->keyLen % 64) || (key->keyLen < MIN_KEY_BITS))
		return BAD_KEY_INSTANCE;
  #endif
#endif

	if (needToBuildMDS)			/* do this one time only */
		BuildMDS();

#define	F32(res,x,k32)	\
	{															\
	DWORD t=x;													\
	switch (k64Cnt & 3)											\
	    {														\
		case 0:  /* same as 4 */								\
					b0(t)   = p8(04)[b0(t)] ^ b0(k32[3]);		\
					b1(t)   = p8(14)[b1(t)] ^ b1(k32[3]);		\
					b2(t)   = p8(24)[b2(t)] ^ b2(k32[3]);		\
					b3(t)   = p8(34)[b3(t)] ^ b3(k32[3]);		\
				 /* fall thru, having pre-processed t */		\
		case 3:		b0(t)   = p8(03)[b0(t)] ^ b0(k32[2]);		\
					b1(t)   = p8(13)[b1(t)] ^ b1(k32[2]);		\
					b2(t)   = p8(23)[b2(t)] ^ b2(k32[2]);		\
					b3(t)   = p8(33)[b3(t)] ^ b3(k32[2]);		\
				 /* fall thru, having pre-processed t */		\
		case 2:	 /* 128-bit keys (optimize for this case) */	\
			res=	MDStab[0][p8(01)[p8(02)[b0(t)] ^ b0(k32[1])] ^ b0(k32[0])] ^	\
					MDStab[1][p8(11)[p8(12)[b1(t)] ^ b1(k32[1])] ^ b1(k32[0])] ^	\
					MDStab[2][p8(21)[p8(22)[b2(t)] ^ b2(k32[1])] ^ b2(k32[0])] ^	\
					MDStab[3][p8(31)[p8(32)[b3(t)] ^ b3(k32[1])] ^ b3(k32[0])] ;	\
		}														\
	}


#if !CHECK_TABLE
#if defined(USE_ASM)				/* only do this if not using assember */
if (!(useAsm & 4))
#endif
#endif
	{
	subkeyCnt = ROUND_SUBKEYS + 2*key->numRounds;
	keyLen=key->keyLen;
	k64Cnt=(keyLen+63)/64;			/* number of 64-bit key words */
	for (i=0,j=k64Cnt-1;i<k64Cnt;i++,j--)
		{							/* split into even/odd key dwords */
		k32e[i]=key->key32[2*i  ];
		k32o[i]=key->key32[2*i+1];
		/* compute S-box keys using (12,8) Reed-Solomon code over GF(256) */
		sKey[j]=key->sboxKeys[j]=RS_MDS_Encode(k32e[i],k32o[i]);	/* reverse order */
		}
	}

#ifdef USE_ASM
if (useAsm & 4)
	{
	#if defined(COMPILE_KEY) && defined(USE_ASM)
		key->keySig		= VALID_SIG;			/* show that we are initialized */
		key->codeSize	= sizeof(key->compiledCode);	/* set size */
	#endif
	reKey_86(key);
	}
else
#endif
	{
	for (i=q=0;i<subkeyCnt/2;i++,q+=SK_STEP)	
		{							/* compute round subkeys for PHT */
		F32(A,q        ,k32e);		/* A uses even key dwords */
		F32(B,q+SK_BUMP,k32o);		/* B uses odd  key dwords */
		B = ROL(B,8);
		key->subKeys[2*i  ] = A+B;	/* combine with a PHT */
		B = A + 2*B;
		key->subKeys[2*i+1] = ROL(B,SK_ROTL);
		}
#if !defined(ZERO_KEY)
	switch (keyLen)	/* case out key length for speed in generating S-boxes */
		{
		case 128:
		#if defined(FULL_KEY) || defined(PART_KEY)
#if BIG_TAB
			#define	one128(N,J)	sbSet(N,i,J,L0[i+J])
			#define	sb128(N) {						\
				BYTE *qq=bigTab[N][b##N(sKey[1])];	\
				Xor256(L0,qq,b##N(sKey[0]));		\
				for (i=0;i<256;i+=2) { one128(N,0); one128(N,1); } }
#else
			#define	one128(N,J)	sbSet(N,i,J,p8(N##1)[L0[i+J]]^k0)
			#define	sb128(N) {					\
				Xor256(L0,p8(N##2),b##N(sKey[1]));	\
				{ register DWORD k0=b##N(sKey[0]);	\
				for (i=0;i<256;i+=2) { one128(N,0); one128(N,1); } } }
#endif
		#elif defined(MIN_KEY)
			#define	sb128(N) Xor256(_sBox8_(N),p8(N##2),b##N(sKey[1]))
		#endif
			sb128(0); sb128(1); sb128(2); sb128(3);
			break;
		case 192:
		#if defined(FULL_KEY) || defined(PART_KEY)
			#define one192(N,J) sbSet(N,i,J,p8(N##1)[p8(N##2)[L0[i+J]]^k1]^k0)
			#define	sb192(N) {						\
				Xor256(L0,p8(N##3),b##N(sKey[2]));	\
				{ register DWORD k0=b##N(sKey[0]);	\
				  register DWORD k1=b##N(sKey[1]);	\
				  for (i=0;i<256;i+=2) { one192(N,0); one192(N,1); } } }
		#elif defined(MIN_KEY)
			#define one192(N,J) sbSet(N,i,J,p8(N##2)[L0[i+J]]^k1)
			#define	sb192(N) {						\
				Xor256(L0,p8(N##3),b##N(sKey[2]));	\
				{ register DWORD k1=b##N(sKey[1]);	\
				  for (i=0;i<256;i+=2) { one192(N,0); one192(N,1); } } }
		#endif
			sb192(0); sb192(1); sb192(2); sb192(3);
			break;
		case 256:
		#if defined(FULL_KEY) || defined(PART_KEY)
			#define one256(N,J) sbSet(N,i,J,p8(N##1)[p8(N##2)[L0[i+J]]^k1]^k0)
			#define	sb256(N) {										\
				Xor256(L1,p8(N##4),b##N(sKey[3]));					\
				for (i=0;i<256;i+=2) {L0[i  ]=p8(N##3)[L1[i]];		\
									  L0[i+1]=p8(N##3)[L1[i+1]]; }	\
				Xor256(L0,L0,b##N(sKey[2]));						\
				{ register DWORD k0=b##N(sKey[0]);					\
				  register DWORD k1=b##N(sKey[1]);					\
				  for (i=0;i<256;i+=2) { one256(N,0); one256(N,1); } } }
		#elif defined(MIN_KEY)
			#define one256(N,J) sbSet(N,i,J,p8(N##2)[L0[i+J]]^k1)
			#define	sb256(N) {										\
				Xor256(L1,p8(N##4),b##N(sKey[3]));					\
				for (i=0;i<256;i+=2) {L0[i  ]=p8(N##3)[L1[i]];		\
									  L0[i+1]=p8(N##3)[L1[i+1]]; }	\
				Xor256(L0,L0,b##N(sKey[2]));						\
				{ register DWORD k1=b##N(sKey[1]);					\
				  for (i=0;i<256;i+=2) { one256(N,0); one256(N,1); } } }
		#endif
			sb256(0); sb256(1);	sb256(2); sb256(3);
			break;
		}
#endif
	}

#if CHECK_TABLE						/* sanity check  vs. pedagogical code*/
	{
	GetSboxKey;
	for (i=0;i<subkeyCnt/2;i++)
		{
		A = f32(i*SK_STEP        ,k32e,keyLen);	/* A uses even key dwords */
		B = f32(i*SK_STEP+SK_BUMP,k32o,keyLen);	/* B uses odd  key dwords */
		B = ROL(B,8);
		assert(key->subKeys[2*i  ] == A+  B);
		assert(key->subKeys[2*i+1] == ROL(A+2*B,SK_ROTL));
		}
  #if !defined(ZERO_KEY)			/* any S-boxes to check? */
	for (i=q=0;i<256;i++,q+=0x01010101)
		assert(f32(q,key->sboxKeys,keyLen) == Fe32_(q,0));
  #endif
	}
#endif /* CHECK_TABLE */

	//DebugDumpKey(key);

	if (key->direction == DIR_ENCRYPT)	
		ReverseRoundSubkeys(key,DIR_ENCRYPT);	/* reverse the round subkey order */

	return TRUE;
	}
/*
+*****************************************************************************
*
* Function Name:	makeKey
*
* Function:			Initialize the Twofish key schedule
*
* Arguments:		key			=	ptr to keyInstance to be initialized
*					direction	=	DIR_ENCRYPT or DIR_DECRYPT
*					keyLen		=	# bits of key text at *keyMaterial
*					keyMaterial	=	ptr to hex ASCII chars representing key bits
*
* Return:			TRUE on success
*					else error code (e.g., BAD_KEY_DIR)
*
* Notes:	This parses the key bits from keyMaterial.  Zeroes out unused key bits
*
-****************************************************************************/
int makeKey(keyInstance *key, BYTE direction, int keyLen,char *keyMaterial)
	{
#if VALIDATE_PARMS				/* first, sanity check on parameters */
	if (key == NULL)			
		return BAD_KEY_INSTANCE;/* must have a keyInstance to initialize */
	if ((direction != DIR_ENCRYPT) && (direction != DIR_DECRYPT))
		return BAD_KEY_DIR;		/* must have valid direction */
	if ((keyLen > MAX_KEY_BITS) || (keyLen < 8) || (keyLen & 0x3F))
		return BAD_KEY_MAT;		/* length must be valid */
	key->keySig = VALID_SIG;	/* show that we are initialized */
  #if ALIGN32
	if ((((int)key) & 3) || (((int)key->key32) & 3))
		return BAD_ALIGN32;
  #endif
#endif

	key->direction	= direction;/* set our cipher direction */
	key->keyLen		= (keyLen+63) & ~63;		/* round up to multiple of 64 */
	key->numRounds	= numRounds[(keyLen-1)/64];
	memset(key->key32,0,sizeof(key->key32));	/* zero unused bits */
	key->keyMaterial[MAX_KEY_SIZE]=0;	/* terminate ASCII string */

	if ((keyMaterial == NULL) || (keyMaterial[0]==0))
		return TRUE;			/* allow a "dummy" call */
		
	if (ParseHexDword(keyLen,keyMaterial,key->key32,key->keyMaterial))
		return BAD_KEY_MAT;	

	return reKey(key);			/* generate round subkeys */
	}


/*
+*****************************************************************************
*
* Function Name:	cipherInit
*
* Function:			Initialize the Twofish cipher in a given mode
*
* Arguments:		cipher		=	ptr to cipherInstance to be initialized
*					mode		=	MODE_ECB, MODE_CBC, or MODE_CFB1
*					IV			=	ptr to hex ASCII test representing IV bytes
*
* Return:			TRUE on success
*					else error code (e.g., BAD_CIPHER_MODE)
*
-****************************************************************************/
int cipherInit(cipherInstance *cipher, BYTE mode,char *IV)
	{
	int i;
#if VALIDATE_PARMS				/* first, sanity check on parameters */
	if (cipher == NULL)			
		return BAD_PARAMS;		/* must have a cipherInstance to initialize */
	if ((mode != MODE_ECB) && (mode != MODE_CBC) && (mode != MODE_CFB1))
		return BAD_CIPHER_MODE;	/* must have valid cipher mode */
	cipher->cipherSig	=	VALID_SIG;
  #if ALIGN32
	if ((((int)cipher) & 3) || (((int)cipher->IV) & 3) || (((int)cipher->iv32) & 3))
		return BAD_ALIGN32;
  #endif
#endif

	if ((mode != MODE_ECB) && (IV))	/* parse the IV */
		{
		if (ParseHexDword(BLOCK_SIZE,IV,cipher->iv32,NULL))
			return BAD_IV_MAT;
		for (i=0;i<BLOCK_SIZE/32;i++)	/* make byte-oriented copy for CFB1 */
			((DWORD *)cipher->IV)[i] = Bswap(cipher->iv32[i]);
		}

	cipher->mode		=	mode;

	return TRUE;
	}

/*
+*****************************************************************************
*
* Function Name:	blockEncrypt
*
* Function:			Encrypt block(s) of data using Twofish
*
* Arguments:		cipher		=	ptr to already initialized cipherInstance
*					key			=	ptr to already initialized keyInstance
*					input		=	ptr to data blocks to be encrypted
*					inputLen	=	# bits to encrypt (multiple of blockSize)
*					outBuffer	=	ptr to where to put encrypted blocks
*
* Return:			# bits ciphered (>= 0)
*					else error code (e.g., BAD_CIPHER_STATE, BAD_KEY_MATERIAL)
*
* Notes: The only supported block size for ECB/CBC modes is BLOCK_SIZE bits.
*		 If inputLen is not a multiple of BLOCK_SIZE bits in those modes,
*		 an error BAD_INPUT_LEN is returned.  In CFB1 mode, all block 
*		 sizes can be supported.
*
-****************************************************************************/
int BlockEncrypt(cipherInstance *cipher, keyInstance *key,BYTE *input,
				int inputLen, BYTE *outBuffer)
	{
	int   i,n;						/* loop counters */
	DWORD x[BLOCK_SIZE/32];			/* block being encrypted */
	DWORD t0,t1;					/* temp variables */
	int	  rounds=key->numRounds;	/* number of rounds */
	BYTE  bit,bit0,ctBit,carry;		/* temps for CFB */

	/* make local copies of things for faster access */
	int	  mode = cipher->mode;
	DWORD sk[TOTAL_SUBKEYS];
	DWORD IV[BLOCK_SIZE/32];

	GetSboxKey;

#if VALIDATE_PARMS
	if ((cipher == NULL) || (cipher->cipherSig != VALID_SIG))
		return BAD_CIPHER_STATE;
	if ((key == NULL) || (key->keySig != VALID_SIG))
		return BAD_KEY_INSTANCE;
	if ((rounds < 2) || (rounds > MAX_ROUNDS) || (rounds&1))
		return BAD_KEY_INSTANCE;
	if ((mode != MODE_CFB1) && (inputLen % BLOCK_SIZE))
		return BAD_INPUT_LEN;
  #if ALIGN32
	if ( (((int)cipher) & 3) || (((int)key      ) & 3) ||
		 (((int)input ) & 3) || (((int)outBuffer) & 3))
		return BAD_ALIGN32;
  #endif
#endif

	if (mode == MODE_CFB1)
		{	/* use recursion here to handle CFB, one block at a time */
		cipher->mode = MODE_ECB;	/* do encryption in ECB */
		for (n=0;n<inputLen;n++)
			{
			BlockEncrypt(cipher,key,cipher->IV,BLOCK_SIZE,(BYTE *)x);
			bit0  = 0x80 >> (n & 7);/* which bit position in byte */
			ctBit = (input[n/8] & bit0) ^ ((((BYTE *) x)[0] & 0x80) >> (n&7));
			outBuffer[n/8] = (outBuffer[n/8] & ~ bit0) | ctBit;
			carry = ctBit >> (7 - (n&7));
			for (i=BLOCK_SIZE/8-1;i>=0;i--)
				{
				bit = cipher->IV[i] >> 7;	/* save next "carry" from shift */
				cipher->IV[i] = (cipher->IV[i] << 1) ^ carry;
				carry = bit;
				}
			}
		cipher->mode = MODE_CFB1;	/* restore mode for next time */
		return inputLen;
		}

	/* here for ECB, CBC modes */
	if (key->direction != DIR_ENCRYPT)
		ReverseRoundSubkeys(key,DIR_ENCRYPT);	/* reverse the round subkey order */

#ifdef USE_ASM
	if ((useAsm & 1) && (inputLen))
  #ifdef COMPILE_KEY
		if (key->keySig == VALID_SIG)
			return ((CipherProc *)(key->encryptFuncPtr))(cipher,key,input,inputLen,outBuffer);
  #else	
		return (*blockEncrypt_86)(cipher,key,input,inputLen,outBuffer);
  #endif
#endif
	/* make local copy of subkeys for speed */
	memcpy(sk,key->subKeys,sizeof(DWORD)*(ROUND_SUBKEYS+2*rounds));
	if (mode == MODE_CBC)
		BlockCopy(IV,cipher->iv32)
	else
		IV[0]=IV[1]=IV[2]=IV[3]=0;

	for (n=0;n<inputLen;n+=BLOCK_SIZE,input+=BLOCK_SIZE/8,outBuffer+=BLOCK_SIZE/8)
		{
#ifdef DEBUG
		// DebugDump(input,"\n",-1,0,0,0,1);
		//if (cipher->mode == MODE_CBC)
		//	DebugDump(cipher->iv32,"",IV_ROUND,0,0,0,0);
#endif
#define	LoadBlockE(N)  x[N]=Bswap(((DWORD *)input)[N]) ^ sk[INPUT_WHITEN+N] ^ IV[N]
		LoadBlockE(0);	LoadBlockE(1);	LoadBlockE(2);	LoadBlockE(3);
//		DebugDump(x,"",0,0,0,0,0);
#define	EncryptRound(K,R,id)	\
			t0	   = Fe32##id(x[K  ],0);					\
			t1	   = Fe32##id(x[K^1],3);					\
			x[K^3] = ROL(x[K^3],1);							\
			x[K^2]^= t0 +   t1 + sk[ROUND_SUBKEYS+2*(R)  ];	\
			x[K^3]^= t0 + 2*t1 + sk[ROUND_SUBKEYS+2*(R)+1];	\
			x[K^2] = ROR(x[K^2],1);							\

#define		Encrypt2(R,id)	{ EncryptRound(0,R+1,id); EncryptRound(2,R,id); }

//			DebugDump(x,"",rounds-(R),0,0,1,0);

#if defined(ZERO_KEY)
		switch (key->keyLen)
			{
			case 128:
				for (i=rounds-2;i>=0;i-=2)
					Encrypt2(i,_128);
				break;
			case 192:
				for (i=rounds-2;i>=0;i-=2)
					Encrypt2(i,_192);
				break;
			case 256:
				for (i=rounds-2;i>=0;i-=2)
					Encrypt2(i,_256);
				break;
			}
#else
		Encrypt2(14,_);
		Encrypt2(12,_);
		Encrypt2(10,_);
		Encrypt2( 8,_);
		Encrypt2( 6,_);
		Encrypt2( 4,_);
		Encrypt2( 2,_);
		Encrypt2( 0,_);
#endif

		/* need to do (or undo, depending on your point of view) final swap */
#if FishLittleEndian
#define	StoreBlockE(N)	((DWORD *)outBuffer)[N]=x[N^2] ^ sk[OUTPUT_WHITEN+N]
#else
#define	StoreBlockE(N)	{ t0=x[N^2] ^ sk[OUTPUT_WHITEN+N]; ((DWORD *)outBuffer)[N]=Bswap(t0); }
#endif
		StoreBlockE(0);	StoreBlockE(1);	StoreBlockE(2);	StoreBlockE(3);
		if (mode == MODE_CBC)
			{
			IV[0]=Bswap(((DWORD *)outBuffer)[0]);
			IV[1]=Bswap(((DWORD *)outBuffer)[1]);
			IV[2]=Bswap(((DWORD *)outBuffer)[2]);
			IV[3]=Bswap(((DWORD *)outBuffer)[3]);
			}
#ifdef DEBUG
//		DebugDump(outBuffer,"",rounds+1,0,0,0,1);
//		if (cipher->mode == MODE_CBC)
//			DebugDump(cipher->iv32,"",IV_ROUND,0,0,0,0);
#endif
		}

	if (mode == MODE_CBC)
		BlockCopy(cipher->iv32,IV);

	return inputLen;
	}

/*
+*****************************************************************************
*
* Function Name:	blockDecrypt
*
* Function:			Decrypt block(s) of data using Twofish
*
* Arguments:		cipher		=	ptr to already initialized cipherInstance
*					key			=	ptr to already initialized keyInstance
*					input		=	ptr to data blocks to be decrypted
*					inputLen	=	# bits to encrypt (multiple of blockSize)
*					outBuffer	=	ptr to where to put decrypted blocks
*
* Return:			# bits ciphered (>= 0)
*					else error code (e.g., BAD_CIPHER_STATE, BAD_KEY_MATERIAL)
*
* Notes: The only supported block size for ECB/CBC modes is BLOCK_SIZE bits.
*		 If inputLen is not a multiple of BLOCK_SIZE bits in those modes,
*		 an error BAD_INPUT_LEN is returned.  In CFB1 mode, all block 
*		 sizes can be supported.
*
-****************************************************************************/
int BlockDecrypt(cipherInstance *cipher, keyInstance *key, BYTE *input,
				int inputLen, BYTE *outBuffer)
	{
	int   i,n;						/* loop counters */
	DWORD x[BLOCK_SIZE/32];			/* block being encrypted */
	DWORD t0,t1;					/* temp variables */
	int	  rounds=key->numRounds;	/* number of rounds */
	BYTE  bit,bit0,ctBit,carry;		/* temps for CFB */

	/* make local copies of things for faster access */
	int	  mode = cipher->mode;
	DWORD sk[TOTAL_SUBKEYS];
	DWORD IV[BLOCK_SIZE/32];

	GetSboxKey;

#if VALIDATE_PARMS
	if ((cipher == NULL) || (cipher->cipherSig != VALID_SIG))
		return BAD_CIPHER_STATE;
	if ((key == NULL) || (key->keySig != VALID_SIG))
		return BAD_KEY_INSTANCE;
	if ((rounds < 2) || (rounds > MAX_ROUNDS) || (rounds&1))
		return BAD_KEY_INSTANCE;
	if ((cipher->mode != MODE_CFB1) && (inputLen % BLOCK_SIZE))
		return BAD_INPUT_LEN;
  #if ALIGN32
	if ( (((int)cipher) & 3) || (((int)key      ) & 3) ||
		 (((int)input)  & 3) || (((int)outBuffer) & 3))
		return BAD_ALIGN32;
  #endif
#endif

	if (cipher->mode == MODE_CFB1)
		{	/* use blockEncrypt here to handle CFB, one block at a time */
		cipher->mode = MODE_ECB;	/* do encryption in ECB */
		for (n=0;n<inputLen;n++)
			{
			BlockEncrypt(cipher,key,cipher->IV,BLOCK_SIZE,(BYTE *)x);
			bit0  = 0x80 >> (n & 7);
			ctBit = input[n/8] & bit0;
			outBuffer[n/8] = (outBuffer[n/8] & ~ bit0) |
							 (ctBit ^ ((((BYTE *) x)[0] & 0x80) >> (n&7)));
			carry = ctBit >> (7 - (n&7));
			for (i=BLOCK_SIZE/8-1;i>=0;i--)
				{
				bit = cipher->IV[i] >> 7;	/* save next "carry" from shift */
				cipher->IV[i] = (cipher->IV[i] << 1) ^ carry;
				carry = bit;
				}
			}
		cipher->mode = MODE_CFB1;	/* restore mode for next time */
		return inputLen;
		}

	/* here for ECB, CBC modes */
	if (key->direction != DIR_DECRYPT)
		ReverseRoundSubkeys(key,DIR_DECRYPT);	/* reverse the round subkey order */
#ifdef USE_ASM
	if ((useAsm & 2) && (inputLen))
  #ifdef COMPILE_KEY
		if (key->keySig == VALID_SIG)
			return ((CipherProc *)(key->decryptFuncPtr))(cipher,key,input,inputLen,outBuffer);
  #else	
		return (*blockDecrypt_86)(cipher,key,input,inputLen,outBuffer);
  #endif
#endif
	/* make local copy of subkeys for speed */
	memcpy(sk,key->subKeys,sizeof(DWORD)*(ROUND_SUBKEYS+2*rounds));
	if (mode == MODE_CBC)
		BlockCopy(IV,cipher->iv32)
	else
		IV[0]=IV[1]=IV[2]=IV[3]=0;

	for (n=0;n<inputLen;n+=BLOCK_SIZE,input+=BLOCK_SIZE/8,outBuffer+=BLOCK_SIZE/8)
		{
//		DebugDump(input,"\n",rounds+1,0,0,0,1);
#define LoadBlockD(N) x[N^2]=Bswap(((DWORD *)input)[N]) ^ sk[OUTPUT_WHITEN+N]
		LoadBlockD(0);	LoadBlockD(1);	LoadBlockD(2);	LoadBlockD(3);

#define	DecryptRound(K,R,id)								\
			t0	   = Fe32##id(x[K  ],0);					\
			t1	   = Fe32##id(x[K^1],3);					\
			x[K^2] = ROL (x[K^2],1);						\
			x[K^2]^= t0 +   t1 + sk[ROUND_SUBKEYS+2*(R)  ];	\
			x[K^3]^= t0 + 2*t1 + sk[ROUND_SUBKEYS+2*(R)+1];	\
			x[K^3] = ROR (x[K^3],1);						\

#define		Decrypt2(R,id)	{ DecryptRound(2,R+1,id); DecryptRound(0,R,id); }

#if defined(ZERO_KEY)
		switch (key->keyLen)
			{
			case 128:
				for (i=rounds-2;i>=0;i-=2)
					Decrypt2(i,_128);
				break;
			case 192:
				for (i=rounds-2;i>=0;i-=2)
					Decrypt2(i,_192);
				break;
			case 256:
				for (i=rounds-2;i>=0;i-=2)
					Decrypt2(i,_256);
				break;
			}
#else
		{
		Decrypt2(14,_);
		Decrypt2(12,_);
		Decrypt2(10,_);
		Decrypt2( 8,_);
		Decrypt2( 6,_);
		Decrypt2( 4,_);
		Decrypt2( 2,_);
		Decrypt2( 0,_);
		}
#endif
		if (cipher->mode == MODE_ECB)
			{
#if FishLittleEndian
#define	StoreBlockD(N)	((DWORD *)outBuffer)[N] = x[N] ^ sk[INPUT_WHITEN+N]
#else
#define	StoreBlockD(N)	{ t0=x[N]^sk[INPUT_WHITEN+N]; ((DWORD *)outBuffer)[N] = Bswap(t0); }
#endif
			StoreBlockD(0);	StoreBlockD(1);	StoreBlockD(2);	StoreBlockD(3);
#undef  StoreBlockD
			continue;
			}
		else
			{
#define	StoreBlockD(N)	x[N]   ^= sk[INPUT_WHITEN+N] ^ IV[N];	\
						IV[N]   = Bswap(((DWORD *)input)[N]);	\
						((DWORD *)outBuffer)[N] = Bswap(x[N]);
			StoreBlockD(0);	StoreBlockD(1);	StoreBlockD(2);	StoreBlockD(3);
#undef  StoreBlockD
			}
		}
	if (mode == MODE_CBC)	/* restore iv32 to cipher */
		BlockCopy(cipher->iv32,IV)

	return inputLen;
	}

#ifdef GetCodeSize
DWORD TwofishCodeSize(void)
	{
	DWORD x= Here(0);
#ifdef USE_ASM
	if (useAsm & 3)
		return TwofishAsmCodeSize();
#endif
	return x - TwofishCodeStart();
	};
#endif


#else // NOT OPTIMIZED_TWOFISH
#define		TFCONST					/* help syntax from C++, NOP here */
/***************************************************************************
	TWOFISH.C	-- C API calls for TWOFISH AES submission

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
		*	Pedagogical version (non-optimized)
		*	Tab size is set to 4 characters in this file

***************************************************************************/

//#include	"aes.h"
//#include	"table.h"
/***************************************************************************
	TABLE.H	-- Tables, macros, constants for Twofish S-boxes and MDS matrix

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
		*	These definitions should be used in optimized and unoptimized
			versions to insure consistency.

***************************************************************************/

/* for computing subkeys */
#define	SK_STEP			0x02020202u
#define	SK_BUMP			0x01010101u
#define	SK_ROTL			9

/* Reed-Solomon code parameters: (12,8) reversible code
	g(x) = x**4 + (a + 1/a) x**3 + a x**2 + (a + 1/a) x + 1
   where a = primitive root of field generator 0x14D */
#define	RS_GF_FDBK		0x14D		/* field generator */
#define	RS_rem(x)		\
	{ BYTE  b  = (BYTE) (x >> 24);											 \
	  DWORD g2 = ((b << 1) ^ ((b & 0x80) ? RS_GF_FDBK : 0 )) & 0xFF;		 \
	  DWORD g3 = ((b >> 1) & 0x7F) ^ ((b & 1) ? RS_GF_FDBK >> 1 : 0 ) ^ g2 ; \
	  x = (x << 8) ^ (g3 << 24) ^ (g2 << 16) ^ (g3 << 8) ^ b;				 \
	}

/*	Macros for the MDS matrix
*	The MDS matrix is (using primitive polynomial 169):
*      01  EF  5B  5B
*      5B  EF  EF  01
*      EF  5B  01  EF
*      EF  01  EF  5B
*----------------------------------------------------------------
* More statistical properties of this matrix (from MDS.EXE output):
*
* Min Hamming weight (one byte difference) =  8. Max=26.  Total =  1020.
* Prob[8]:      7    23    42    20    52    95    88    94   121   128    91
*             102    76    41    24     8     4     1     3     0     0     0
* Runs[8]:      2     4     5     6     7     8     9    11
* MSBs[8]:      1     4    15     8    18    38    40    43
* HW= 8: 05040705 0A080E0A 14101C14 28203828 50407050 01499101 A080E0A0 
* HW= 9: 04050707 080A0E0E 10141C1C 20283838 40507070 80A0E0E0 C6432020 07070504 
*        0E0E0A08 1C1C1410 38382820 70705040 E0E0A080 202043C6 05070407 0A0E080E 
*        141C101C 28382038 50704070 A0E080E0 4320C620 02924B02 089A4508 
* Min Hamming weight (two byte difference) =  3. Max=28.  Total = 390150.
* Prob[3]:      7    18    55   149   270   914  2185  5761 11363 20719 32079
*           43492 51612 53851 52098 42015 31117 20854 11538  6223  2492  1033
* MDS OK, ROR:   6+  7+  8+  9+ 10+ 11+ 12+ 13+ 14+ 15+ 16+
*               17+ 18+ 19+ 20+ 21+ 22+ 23+ 24+ 25+ 26+
*/
#define	MDS_GF_FDBK		0x169	/* primitive polynomial for GF(256)*/
#define	LFSR1(x) ( ((x) >> 1)  ^ (((x) & 0x01) ?   MDS_GF_FDBK/2 : 0))
#define	LFSR2(x) ( ((x) >> 2)  ^ (((x) & 0x02) ?   MDS_GF_FDBK/2 : 0)  \
							   ^ (((x) & 0x01) ?   MDS_GF_FDBK/4 : 0))

#define	Mx_1(x) ((DWORD)  (x))		/* force result to dword so << will work */
#define	Mx_X(x) ((DWORD) ((x) ^            LFSR2(x)))	/* 5B */
#define	Mx_Y(x) ((DWORD) ((x) ^ LFSR1(x) ^ LFSR2(x)))	/* EF */

#define	M00		Mul_1
#define	M01		Mul_Y
#define	M02		Mul_X
#define	M03		Mul_X

#define	M10		Mul_X
#define	M11		Mul_Y
#define	M12		Mul_Y
#define	M13		Mul_1

#define	M20		Mul_Y
#define	M21		Mul_X
#define	M22		Mul_1
#define	M23		Mul_Y

#define	M30		Mul_Y
#define	M31		Mul_1
#define	M32		Mul_Y
#define	M33		Mul_X

#define	Mul_1	Mx_1
#define	Mul_X	Mx_X
#define	Mul_Y	Mx_Y

/*	Define the fixed p0/p1 permutations used in keyed S-box lookup.  
	By changing the following constant definitions for P_ij, the S-boxes will
	automatically get changed in all the Twofish source code. Note that P_i0 is
	the "outermost" 8x8 permutation applied.  See the f32() function to see
	how these constants are to be  used.
*/
#define	P_00	1					/* "outermost" permutation */
#define	P_01	0
#define	P_02	0
#define	P_03	(P_01^1)			/* "extend" to larger key sizes */
#define	P_04	1

#define	P_10	0
#define	P_11	0
#define	P_12	1
#define	P_13	(P_11^1)
#define	P_14	0

#define	P_20	1
#define	P_21	1
#define	P_22	0
#define	P_23	(P_21^1)
#define	P_24	0

#define	P_30	0
#define	P_31	1
#define	P_32	1
#define	P_33	(P_31^1)
#define	P_34	1

#define	p8(N)	P8x8[P_##N]			

/* fixed 8x8 permutation S-boxes */

/***********************************************************************
*  07:07:14  05/30/98  [4x4]  TestCnt=256. keySize=128. CRC=4BD14D9E.
* maxKeyed:  dpMax = 18. lpMax =100. fixPt =  8. skXor =  0. skDup =  6. 
* log2(dpMax[ 6..18])=   --- 15.42  1.33  0.89  4.05  7.98 12.05
* log2(lpMax[ 7..12])=  9.32  1.01  1.16  4.23  8.02 12.45
* log2(fixPt[ 0.. 8])=  1.44  1.44  2.44  4.06  6.01  8.21 11.07 14.09 17.00
* log2(skXor[ 0.. 0])
* log2(skDup[ 0.. 6])=   ---  2.37  0.44  3.94  8.36 13.04 17.99
***********************************************************************/
TFCONST BYTE P8x8[2][256]=
	{
/*  p0:   */
/*  dpMax      = 10.  lpMax      = 64.  cycleCnt=   1  1  1  0.         */
/* 817D6F320B59ECA4.ECB81235F4A6709D.BA5E6D90C8F32471.D7F4126E9B3085CA. */
/* Karnaugh maps:
*  0111 0001 0011 1010. 0001 1001 1100 1111. 1001 1110 0011 1110. 1101 0101 1111 1001. 
*  0101 1111 1100 0100. 1011 0101 0010 0000. 0101 1000 1100 0101. 1000 0111 0011 0010. 
*  0000 1001 1110 1101. 1011 1000 1010 0011. 0011 1001 0101 0000. 0100 0010 0101 1011. 
*  0111 0100 0001 0110. 1000 1011 1110 1001. 0011 0011 1001 1101. 1101 0101 0000 1100. 
*/
	{
	0xA9, 0x67, 0xB3, 0xE8, 0x04, 0xFD, 0xA3, 0x76, 
	0x9A, 0x92, 0x80, 0x78, 0xE4, 0xDD, 0xD1, 0x38, 
	0x0D, 0xC6, 0x35, 0x98, 0x18, 0xF7, 0xEC, 0x6C, 
	0x43, 0x75, 0x37, 0x26, 0xFA, 0x13, 0x94, 0x48, 
	0xF2, 0xD0, 0x8B, 0x30, 0x84, 0x54, 0xDF, 0x23, 
	0x19, 0x5B, 0x3D, 0x59, 0xF3, 0xAE, 0xA2, 0x82, 
	0x63, 0x01, 0x83, 0x2E, 0xD9, 0x51, 0x9B, 0x7C, 
	0xA6, 0xEB, 0xA5, 0xBE, 0x16, 0x0C, 0xE3, 0x61, 
	0xC0, 0x8C, 0x3A, 0xF5, 0x73, 0x2C, 0x25, 0x0B, 
	0xBB, 0x4E, 0x89, 0x6B, 0x53, 0x6A, 0xB4, 0xF1, 
	0xE1, 0xE6, 0xBD, 0x45, 0xE2, 0xF4, 0xB6, 0x66, 
	0xCC, 0x95, 0x03, 0x56, 0xD4, 0x1C, 0x1E, 0xD7, 
	0xFB, 0xC3, 0x8E, 0xB5, 0xE9, 0xCF, 0xBF, 0xBA, 
	0xEA, 0x77, 0x39, 0xAF, 0x33, 0xC9, 0x62, 0x71, 
	0x81, 0x79, 0x09, 0xAD, 0x24, 0xCD, 0xF9, 0xD8, 
	0xE5, 0xC5, 0xB9, 0x4D, 0x44, 0x08, 0x86, 0xE7, 
	0xA1, 0x1D, 0xAA, 0xED, 0x06, 0x70, 0xB2, 0xD2, 
	0x41, 0x7B, 0xA0, 0x11, 0x31, 0xC2, 0x27, 0x90, 
	0x20, 0xF6, 0x60, 0xFF, 0x96, 0x5C, 0xB1, 0xAB, 
	0x9E, 0x9C, 0x52, 0x1B, 0x5F, 0x93, 0x0A, 0xEF, 
	0x91, 0x85, 0x49, 0xEE, 0x2D, 0x4F, 0x8F, 0x3B, 
	0x47, 0x87, 0x6D, 0x46, 0xD6, 0x3E, 0x69, 0x64, 
	0x2A, 0xCE, 0xCB, 0x2F, 0xFC, 0x97, 0x05, 0x7A, 
	0xAC, 0x7F, 0xD5, 0x1A, 0x4B, 0x0E, 0xA7, 0x5A, 
	0x28, 0x14, 0x3F, 0x29, 0x88, 0x3C, 0x4C, 0x02, 
	0xB8, 0xDA, 0xB0, 0x17, 0x55, 0x1F, 0x8A, 0x7D, 
	0x57, 0xC7, 0x8D, 0x74, 0xB7, 0xC4, 0x9F, 0x72, 
	0x7E, 0x15, 0x22, 0x12, 0x58, 0x07, 0x99, 0x34, 
	0x6E, 0x50, 0xDE, 0x68, 0x65, 0xBC, 0xDB, 0xF8, 
	0xC8, 0xA8, 0x2B, 0x40, 0xDC, 0xFE, 0x32, 0xA4, 
	0xCA, 0x10, 0x21, 0xF0, 0xD3, 0x5D, 0x0F, 0x00, 
	0x6F, 0x9D, 0x36, 0x42, 0x4A, 0x5E, 0xC1, 0xE0
	},
/*  p1:   */
/*  dpMax      = 10.  lpMax      = 64.  cycleCnt=   2  0  0  1.         */
/* 28BDF76E31940AC5.1E2B4C376DA5F908.4C75169A0ED82B3F.B951C3DE647F208A. */
/* Karnaugh maps:
*  0011 1001 0010 0111. 1010 0111 0100 0110. 0011 0001 1111 0100. 1111 1000 0001 1100. 
*  1100 1111 1111 1010. 0011 0011 1110 0100. 1001 0110 0100 0011. 0101 0110 1011 1011. 
*  0010 0100 0011 0101. 1100 1000 1000 1110. 0111 1111 0010 0110. 0000 1010 0000 0011. 
*  1101 1000 0010 0001. 0110 1001 1110 0101. 0001 0100 0101 0111. 0011 1011 1111 0010. 
*/
	{
	0x75, 0xF3, 0xC6, 0xF4, 0xDB, 0x7B, 0xFB, 0xC8, 
	0x4A, 0xD3, 0xE6, 0x6B, 0x45, 0x7D, 0xE8, 0x4B, 
	0xD6, 0x32, 0xD8, 0xFD, 0x37, 0x71, 0xF1, 0xE1, 
	0x30, 0x0F, 0xF8, 0x1B, 0x87, 0xFA, 0x06, 0x3F, 
	0x5E, 0xBA, 0xAE, 0x5B, 0x8A, 0x00, 0xBC, 0x9D, 
	0x6D, 0xC1, 0xB1, 0x0E, 0x80, 0x5D, 0xD2, 0xD5, 
	0xA0, 0x84, 0x07, 0x14, 0xB5, 0x90, 0x2C, 0xA3, 
	0xB2, 0x73, 0x4C, 0x54, 0x92, 0x74, 0x36, 0x51, 
	0x38, 0xB0, 0xBD, 0x5A, 0xFC, 0x60, 0x62, 0x96, 
	0x6C, 0x42, 0xF7, 0x10, 0x7C, 0x28, 0x27, 0x8C, 
	0x13, 0x95, 0x9C, 0xC7, 0x24, 0x46, 0x3B, 0x70, 
	0xCA, 0xE3, 0x85, 0xCB, 0x11, 0xD0, 0x93, 0xB8, 
	0xA6, 0x83, 0x20, 0xFF, 0x9F, 0x77, 0xC3, 0xCC, 
	0x03, 0x6F, 0x08, 0xBF, 0x40, 0xE7, 0x2B, 0xE2, 
	0x79, 0x0C, 0xAA, 0x82, 0x41, 0x3A, 0xEA, 0xB9, 
	0xE4, 0x9A, 0xA4, 0x97, 0x7E, 0xDA, 0x7A, 0x17, 
	0x66, 0x94, 0xA1, 0x1D, 0x3D, 0xF0, 0xDE, 0xB3, 
	0x0B, 0x72, 0xA7, 0x1C, 0xEF, 0xD1, 0x53, 0x3E, 
	0x8F, 0x33, 0x26, 0x5F, 0xEC, 0x76, 0x2A, 0x49, 
	0x81, 0x88, 0xEE, 0x21, 0xC4, 0x1A, 0xEB, 0xD9, 
	0xC5, 0x39, 0x99, 0xCD, 0xAD, 0x31, 0x8B, 0x01, 
	0x18, 0x23, 0xDD, 0x1F, 0x4E, 0x2D, 0xF9, 0x48, 
	0x4F, 0xF2, 0x65, 0x8E, 0x78, 0x5C, 0x58, 0x19, 
	0x8D, 0xE5, 0x98, 0x57, 0x67, 0x7F, 0x05, 0x64, 
	0xAF, 0x63, 0xB6, 0xFE, 0xF5, 0xB7, 0x3C, 0xA5, 
	0xCE, 0xE9, 0x68, 0x44, 0xE0, 0x4D, 0x43, 0x69, 
	0x29, 0x2E, 0xAC, 0x15, 0x59, 0xA8, 0x0A, 0x9E, 
	0x6E, 0x47, 0xDF, 0x34, 0x35, 0x6A, 0xCF, 0xDC, 
	0x22, 0xC9, 0xC0, 0x9B, 0x89, 0xD4, 0xED, 0xAB, 
	0x12, 0xA2, 0x0D, 0x52, 0xBB, 0x02, 0x2F, 0xA9, 
	0xD7, 0x61, 0x1E, 0xB4, 0x50, 0x04, 0xF6, 0xC2, 
	0x16, 0x25, 0x86, 0x56, 0x55, 0x09, 0xBE, 0x91
	}
	};

/*
+*****************************************************************************
*			Constants/Macros/Tables
-****************************************************************************/

#define		VALIDATE_PARMS	1		/* nonzero --> check all parameters */
#define		FEISTEL			0		/* nonzero --> use Feistel version (slow) */

int  tabEnable=0;					/* are we gathering stats? */
BYTE tabUsed[256];					/* one bit per table */

#if FEISTEL
TFCONST		char *moduleDescription="Pedagogical C code (Feistel)";
#else
TFCONST		char *moduleDescription="Pedagogical C code";
#endif
TFCONST		char *modeString = "";

#define	P0_USED		0x01
#define	P1_USED		0x02
#define	B0_USED		0x04
#define	B1_USED		0x08
#define	B2_USED		0x10
#define	B3_USED		0x20
#define	ALL_USED	0x3F

/* number of rounds for various key sizes: 128, 192, 256 */
int			numRounds[4]= {0,ROUNDS_128,ROUNDS_192,ROUNDS_256};

#ifndef	DEBUG
#ifdef GetCodeSize
#define	DEBUG	1					/* force debug */
#endif
#endif
//#include	"debug.h"				/* debug display macros */

#ifdef GetCodeSize
extern DWORD Here(DWORD x);			/* return caller's address! */
DWORD TwofishCodeStart(void) { return Here(0); };
#endif

/*
+*****************************************************************************
*
* Function Name:	TableOp
*
* Function:			Handle table use checking
*
* Arguments:		op	=	what to do	(see TAB_* defns in AES.H)
*
* Return:			TRUE --> done (for TAB_QUERY)		
*
* Notes: This routine is for use in generating the tables KAT file.
*
-****************************************************************************/
int TableOp(int op)
	{
	static int queryCnt=0;
	int i;
	switch (op)
		{
		case TAB_DISABLE:
			tabEnable=0;
			break;
		case TAB_ENABLE:
			tabEnable=1;
			break;
		case TAB_RESET:
			queryCnt=0;
			for (i=0;i<256;i++)
				tabUsed[i]=0;
			break;
		case TAB_QUERY:
			queryCnt++;
			for (i=0;i<256;i++)
				if (tabUsed[i] != ALL_USED)
					return FALSE;
			if (queryCnt < TAB_MIN_QUERY)	/* do a certain minimum number */
				return FALSE;
			break;
		}
	return TRUE;
	}


/*
+*****************************************************************************
*
* Function Name:	ParseHexDword
*
* Function:			Parse ASCII hex nibbles and fill in key/iv dwords
*
* Arguments:		bit			=	# bits to read
*					srcTxt		=	ASCII source
*					d			=	ptr to dwords to fill in
*					dstTxt		=	where to make a copy of ASCII source
*									(NULL ok)
*
* Return:			Zero if no error.  Nonzero --> invalid hex or length
*
* Notes:  Note that the parameter d is a DWORD array, not a byte array.
*	This routine is coded to work both for little-endian and big-endian
*	architectures.  The character stream is interpreted as a LITTLE-ENDIAN
*	byte stream, since that is how the Pentium works, but the conversion
*	happens automatically below. 
*
-****************************************************************************/
int ParseHexDword(int bits,TFCONST char *srcTxt,DWORD *d,char *dstTxt)
	{
	int i;
	DWORD b;
	char c;
#if ALIGN32
	char alignDummy[3];	/* keep dword alignment */
#endif

	union	/* make sure FishLittleEndian is defined correctly */
		{
		BYTE  b[4];
		DWORD d[1];
		} v;
	v.d[0]=1;
	if (v.b[0 ^ ADDR_XOR] != 1)	/* sanity check on compile-time switch */
		return BAD_ENDIAN;

#if VALIDATE_PARMS
  #if ALIGN32
	if (((int)d) & 3)
		return BAD_ALIGN32;
  #endif
#endif

	for (i=0;i*32<bits;i++)
		d[i]=0;					/* first, zero the field */

	for (i=0;i*4<bits;i++)		/* parse one nibble at a time */
		{						/* case out the hexadecimal characters */
		c=srcTxt[i];
		if (dstTxt) dstTxt[i]=c;
		if ((c >= '0') && (c <= '9'))
			b=c-'0';
		else if ((c >= 'a') && (c <= 'f'))
			b=c-'a'+10;
		else if ((c >= 'A') && (c <= 'F'))
			b=c-'A'+10;
		else
			return BAD_KEY_MAT;	/* invalid hex character */
		/* works for big and little endian! */
		d[i/8] |= b << (4*((i^1)&7));		
		}

	return 0;					/* no error */
	}


/*
+*****************************************************************************
*
* Function Name:	f32
*
* Function:			Run four bytes through keyed S-boxes and apply MDS matrix
*
* Arguments:		x			=	input to f function
*					k32			=	pointer to key dwords
*					keyLen		=	total key length (k32 --> keyLey/2 bits)
*
* Return:			The output of the keyed permutation applied to x.
*
* Notes:
*	This function is a keyed 32-bit permutation.  It is the major building
*	block for the Twofish round function, including the four keyed 8x8 
*	permutations and the 4x4 MDS matrix multiply.  This function is used
*	both for generating round subkeys and within the round function on the
*	block being encrypted.  
*
*	This version is fairly slow and pedagogical, although a smartcard would
*	probably perform the operation exactly this way in firmware.   For
*	ultimate performance, the entire operation can be completed with four
*	lookups into four 256x32-bit tables, with three dword xors.
*
*	The MDS matrix is defined in TABLE.H.  To multiply by Mij, just use the
*	macro Mij(x).
*
-****************************************************************************/
DWORD f32(DWORD x,TFCONST DWORD *k32,int keyLen)
	{
	BYTE  b[4];
	
	/* Run each byte thru 8x8 S-boxes, xoring with key byte at each stage. */
	/* Note that each byte goes through a different combination of S-boxes.*/

	*((DWORD *)b) = Bswap(x);	/* make b[0] = LSB, b[3] = MSB */
	switch (((keyLen + 63)/64) & 3)
		{
		case 0:		/* 256 bits of key */
			b[0] = p8(04)[b[0]] ^ b0(k32[3]);
			b[1] = p8(14)[b[1]] ^ b1(k32[3]);
			b[2] = p8(24)[b[2]] ^ b2(k32[3]);
			b[3] = p8(34)[b[3]] ^ b3(k32[3]);
			/* fall thru, having pre-processed b[0]..b[3] with k32[3] */
		case 3:		/* 192 bits of key */
			b[0] = p8(03)[b[0]] ^ b0(k32[2]);
			b[1] = p8(13)[b[1]] ^ b1(k32[2]);
			b[2] = p8(23)[b[2]] ^ b2(k32[2]);
			b[3] = p8(33)[b[3]] ^ b3(k32[2]);
			/* fall thru, having pre-processed b[0]..b[3] with k32[2] */
		case 2:		/* 128 bits of key */
			b[0] = p8(00)[p8(01)[p8(02)[b[0]] ^ b0(k32[1])] ^ b0(k32[0])];
			b[1] = p8(10)[p8(11)[p8(12)[b[1]] ^ b1(k32[1])] ^ b1(k32[0])];
			b[2] = p8(20)[p8(21)[p8(22)[b[2]] ^ b2(k32[1])] ^ b2(k32[0])];
			b[3] = p8(30)[p8(31)[p8(32)[b[3]] ^ b3(k32[1])] ^ b3(k32[0])];
		}

	if (tabEnable)
		{	/* we could give a "tighter" bound, but this works acceptably well */
		tabUsed[b0(x)] |= (P_00 == 0) ? P0_USED : P1_USED;
		tabUsed[b1(x)] |= (P_10 == 0) ? P0_USED : P1_USED;
		tabUsed[b2(x)] |= (P_20 == 0) ? P0_USED : P1_USED;
		tabUsed[b3(x)] |= (P_30 == 0) ? P0_USED : P1_USED;

		tabUsed[b[0] ] |= B0_USED;
		tabUsed[b[1] ] |= B1_USED;
		tabUsed[b[2] ] |= B2_USED;
		tabUsed[b[3] ] |= B3_USED;
		}

	/* Now perform the MDS matrix multiply inline. */
	return	((M00(b[0]) ^ M01(b[1]) ^ M02(b[2]) ^ M03(b[3]))	  ) ^
			((M10(b[0]) ^ M11(b[1]) ^ M12(b[2]) ^ M13(b[3])) <<  8) ^
			((M20(b[0]) ^ M21(b[1]) ^ M22(b[2]) ^ M23(b[3])) << 16) ^
			((M30(b[0]) ^ M31(b[1]) ^ M32(b[2]) ^ M33(b[3])) << 24) ;
	}

/*
+*****************************************************************************
*
* Function Name:	RS_MDS_Encode
*
* Function:			Use (12,8) Reed-Solomon code over GF(256) to produce
*					a key S-box dword from two key material dwords.
*
* Arguments:		k0	=	1st dword
*					k1	=	2nd dword
*
* Return:			Remainder polynomial generated using RS code
*
* Notes:
*	Since this computation is done only once per reKey per 64 bits of key,
*	the performance impact of this routine is imperceptible. The RS code
*	chosen has "simple" coefficients to allow smartcard/hardware implementation
*	without lookup tables.
*
-****************************************************************************/
DWORD RS_MDS_Encode(DWORD k0,DWORD k1)
	{
	int i,j;
	DWORD r;

	for (i=r=0;i<2;i++)
		{
		r ^= (i) ? k0 : k1;			/* merge in 32 more key bits */
		for (j=0;j<4;j++)			/* shift one byte at a time */
			RS_rem(r);				
		}
	return r;
	}

/*
+*****************************************************************************
*
* Function Name:	reKey
*
* Function:			Initialize the Twofish key schedule from key32
*
* Arguments:		key			=	ptr to keyInstance to be initialized
*
* Return:			TRUE on success
*
* Notes:
*	Here we precompute all the round subkeys, although that is not actually
*	required.  For example, on a smartcard, the round subkeys can 
*	be generated on-the-fly	using f32()
*
-****************************************************************************/
int reKey(keyInstance *key)
	{
	int		i,k64Cnt;
	int		keyLen	  = key->keyLen;
	int		subkeyCnt = ROUND_SUBKEYS + 2*key->numRounds;
	DWORD	A,B;
	DWORD	k32e[MAX_KEY_BITS/64],k32o[MAX_KEY_BITS/64]; /* even/odd key dwords */

#if VALIDATE_PARMS
  #if ALIGN32
	if ((((int)key) & 3) || (((int)key->key32) & 3))
		return BAD_ALIGN32;
  #endif
	if ((key->keyLen % 64) || (key->keyLen < MIN_KEY_BITS))
		return BAD_KEY_INSTANCE;
	if (subkeyCnt > TOTAL_SUBKEYS)
		return BAD_KEY_INSTANCE;
#endif

	k64Cnt=(keyLen+63)/64;		/* round up to next multiple of 64 bits */
	for (i=0;i<k64Cnt;i++)
		{						/* split into even/odd key dwords */
		k32e[i]=key->key32[2*i  ];
		k32o[i]=key->key32[2*i+1];
		/* compute S-box keys using (12,8) Reed-Solomon code over GF(256) */
		key->sboxKeys[k64Cnt-1-i]=RS_MDS_Encode(k32e[i],k32o[i]); /* reverse order */
		}

	for (i=0;i<subkeyCnt/2;i++)					/* compute round subkeys for PHT */
		{
		A = f32(i*SK_STEP        ,k32e,keyLen);	/* A uses even key dwords */
		B = f32(i*SK_STEP+SK_BUMP,k32o,keyLen);	/* B uses odd  key dwords */
		B = ROL(B,8);
		key->subKeys[2*i  ] = A+  B;			/* combine with a PHT */
		key->subKeys[2*i+1] = ROL(A+2*B,SK_ROTL);
		}

//	DebugDumpKey(key);

	return TRUE;
	}
/*
+*****************************************************************************
*
* Function Name:	makeKey
*
* Function:			Initialize the Twofish key schedule
*
* Arguments:		key			=	ptr to keyInstance to be initialized
*					direction	=	DIR_ENCRYPT or DIR_DECRYPT
*					keyLen		=	# bits of key text at *keyMaterial
*					keyMaterial	=	ptr to hex ASCII chars representing key bits
*
* Return:			TRUE on success
*					else error code (e.g., BAD_KEY_DIR)
*
* Notes:
*	This parses the key bits from keyMaterial.  No crypto stuff happens here.
*	The function reKey() is called to actually build the key schedule after
*	the keyMaterial has been parsed.
*
-****************************************************************************/
int makeKey(keyInstance *key, BYTE direction, int keyLen,char *keyMaterial)
	{
	int i;

#if VALIDATE_PARMS				/* first, sanity check on parameters */
	if (key == NULL)			
		return BAD_KEY_INSTANCE;/* must have a keyInstance to initialize */
	if ((direction != DIR_ENCRYPT) && (direction != DIR_DECRYPT))
		return BAD_KEY_DIR;		/* must have valid direction */
	if ((keyLen > MAX_KEY_BITS) || (keyLen < 8))	
		return BAD_KEY_MAT;		/* length must be valid */
	key->keySig = VALID_SIG;	/* show that we are initialized */
  #if ALIGN32
	if ((((int)key) & 3) || (((int)key->key32) & 3))
		return BAD_ALIGN32;
  #endif
#endif

	key->direction	= direction;	/* set our cipher direction */
	key->keyLen		= (keyLen+63) & ~63;		/* round up to multiple of 64 */
	key->numRounds	= numRounds[(keyLen-1)/64];
	for (i=0;i<MAX_KEY_BITS/32;i++)	/* zero unused bits */
		   key->key32[i]=0;
	key->keyMaterial[MAX_KEY_SIZE]=0;	/* terminate ASCII string */

	if ((keyMaterial == NULL) || (keyMaterial[0]==0))
		return TRUE;			/* allow a "dummy" call */
		
	if (ParseHexDword(keyLen,keyMaterial,key->key32,key->keyMaterial))
		return BAD_KEY_MAT;	

	return reKey(key);			/* generate round subkeys */
	}


/*
+*****************************************************************************
*
* Function Name:	cipherInit
*
* Function:			Initialize the Twofish cipher in a given mode
*
* Arguments:		cipher		=	ptr to cipherInstance to be initialized
*					mode		=	MODE_ECB, MODE_CBC, or MODE_CFB1
*					IV			=	ptr to hex ASCII test representing IV bytes
*
* Return:			TRUE on success
*					else error code (e.g., BAD_CIPHER_MODE)
*
-****************************************************************************/
int cipherInit(cipherInstance *cipher, BYTE mode, char *IV)
	{
	int i;
#if VALIDATE_PARMS				/* first, sanity check on parameters */
	if (cipher == NULL)			
		return BAD_PARAMS;		/* must have a cipherInstance to initialize */
	if ((mode != MODE_ECB) && (mode != MODE_CBC) && (mode != MODE_CFB1))
		return BAD_CIPHER_MODE;	/* must have valid cipher mode */
	cipher->cipherSig	=	VALID_SIG;
  #if ALIGN32
	if ((((int)cipher) & 3) || (((int)cipher->IV) & 3) || (((int)cipher->iv32) & 3))
		return BAD_ALIGN32;
  #endif
#endif

	if ((mode != MODE_ECB) && (IV))	/* parse the IV */
		{
		if (ParseHexDword(BLOCK_SIZE,IV,cipher->iv32,NULL))
			return BAD_IV_MAT;
		for (i=0;i<BLOCK_SIZE/32;i++)	/* make byte-oriented copy for CFB1 */
			((DWORD *)cipher->IV)[i] = Bswap(cipher->iv32[i]);
		}

	cipher->mode		=	mode;

	return TRUE;
	}

/*
+*****************************************************************************
*
* Function Name:	blockEncrypt
*
* Function:			Encrypt block(s) of data using Twofish
*
* Arguments:		cipher		=	ptr to already initialized cipherInstance
*					key			=	ptr to already initialized keyInstance
*					input		=	ptr to data blocks to be encrypted
*					inputLen	=	# bits to encrypt (multiple of blockSize)
*					outBuffer	=	ptr to where to put encrypted blocks
*
* Return:			# bits ciphered (>= 0)
*					else error code (e.g., BAD_CIPHER_STATE, BAD_KEY_MATERIAL)
*
* Notes: The only supported block size for ECB/CBC modes is BLOCK_SIZE bits.
*		 If inputLen is not a multiple of BLOCK_SIZE bits in those modes,
*		 an error BAD_INPUT_LEN is returned.  In CFB1 mode, all block 
*		 sizes can be supported.
*
-****************************************************************************/
int BlockEncrypt(cipherInstance *cipher, keyInstance *key,BYTE *input,
				int inputLen, BYTE *outBuffer)
	{
	int   i,n,r;					/* loop variables */
	DWORD x[BLOCK_SIZE/32];			/* block being encrypted */
	DWORD t0,t1,tmp;				/* temp variables */
	int	  rounds=key->numRounds;	/* number of rounds */
	BYTE  bit,ctBit,carry;			/* temps for CFB */
#if ALIGN32
	BYTE alignDummy;				/* keep 32-bit variable alignment on stack */
#endif

#if VALIDATE_PARMS
	if ((cipher == NULL) || (cipher->cipherSig != VALID_SIG))
		return BAD_CIPHER_STATE;
	if ((key == NULL) || (key->keySig != VALID_SIG))
		return BAD_KEY_INSTANCE;
	if ((rounds < 2) || (rounds > MAX_ROUNDS) || (rounds&1))
		return BAD_KEY_INSTANCE;
	if ((cipher->mode != MODE_CFB1) && (inputLen % BLOCK_SIZE))
		return BAD_INPUT_LEN;
  #if ALIGN32
	if ( (((int)cipher) & 3) || (((int)key      ) & 3) ||
		 (((int)input ) & 3) || (((int)outBuffer) & 3))
		return BAD_ALIGN32;
  #endif
#endif

	if (cipher->mode == MODE_CFB1)
		{	/* use recursion here to handle CFB, one block at a time */
		cipher->mode = MODE_ECB;	/* do encryption in ECB */
		for (n=0;n<inputLen;n++)
			{
			BlockEncrypt(cipher,key,cipher->IV,BLOCK_SIZE,(BYTE *)x);
			bit	  = 0x80 >> (n & 7);/* which bit position in byte */
			ctBit = (input[n/8] & bit) ^ ((((BYTE *) x)[0] & 0x80) >> (n&7));
			outBuffer[n/8] = (outBuffer[n/8] & ~ bit) | ctBit;
			carry = ctBit >> (7 - (n&7));
			for (i=BLOCK_SIZE/8-1;i>=0;i--)
				{
				bit = cipher->IV[i] >> 7;	/* save next "carry" from shift */
				cipher->IV[i] = (cipher->IV[i] << 1) ^ carry;
				carry = bit;
				}
			}
		cipher->mode = MODE_CFB1;	/* restore mode for next time */
		return inputLen;
		}

	/* here for ECB, CBC modes */
	for (n=0;n<inputLen;n+=BLOCK_SIZE,input+=BLOCK_SIZE/8,outBuffer+=BLOCK_SIZE/8)
		{
#ifdef DEBUG
		DebugDump(input,"\n",-1,0,0,0,1);
		if (cipher->mode == MODE_CBC)
			DebugDump(cipher->iv32,"",IV_ROUND,0,0,0,0);
#endif
		for (i=0;i<BLOCK_SIZE/32;i++)	/* copy in the block, add whitening */
			{
			x[i]=Bswap(((DWORD *)input)[i]) ^ key->subKeys[INPUT_WHITEN+i];
			if (cipher->mode == MODE_CBC)
				x[i] ^= cipher->iv32[i];
			}

//		DebugDump(x,"",0,0,0,0,0);
		for (r=0;r<rounds;r++)			/* main Twofish encryption loop */
			{	
#if FEISTEL
			t0	 = f32(ROR(x[0],  (r+1)/2),key->sboxKeys,key->keyLen);
			t1	 = f32(ROL(x[1],8+(r+1)/2),key->sboxKeys,key->keyLen);
										/* PHT, round keys */
			x[2]^= ROL(t0 +   t1 + key->subKeys[ROUND_SUBKEYS+2*r  ], r    /2);
			x[3]^= ROR(t0 + 2*t1 + key->subKeys[ROUND_SUBKEYS+2*r+1],(r+2) /2);

			DebugDump(x,"",r+1,2*(r&1),1,1,0);
#else
			t0	 = f32(    x[0]   ,key->sboxKeys,key->keyLen);
			t1	 = f32(ROL(x[1],8),key->sboxKeys,key->keyLen);

			x[3] = ROL(x[3],1);
			x[2]^= t0 +   t1 + key->subKeys[ROUND_SUBKEYS+2*r  ]; /* PHT, round keys */
			x[3]^= t0 + 2*t1 + key->subKeys[ROUND_SUBKEYS+2*r+1];
			x[2] = ROR(x[2],1);

//			DebugDump(x,"",r+1,2*(r&1),0,1,0);/* make format compatible with optimized code */
#endif
			if (r < rounds-1)						/* swap for next round */
				{
				tmp = x[0]; x[0]= x[2]; x[2] = tmp;
				tmp = x[1]; x[1]= x[3]; x[3] = tmp;
				}
			}
#if FEISTEL
		x[0] = ROR(x[0],8);                     /* "final permutation" */
		x[1] = ROL(x[1],8);
		x[2] = ROR(x[2],8);
		x[3] = ROL(x[3],8);
#endif
		for (i=0;i<BLOCK_SIZE/32;i++)	/* copy out, with whitening */
			{
			((DWORD *)outBuffer)[i] = Bswap(x[i] ^ key->subKeys[OUTPUT_WHITEN+i]);
			if (cipher->mode == MODE_CBC)
				cipher->iv32[i] = Bswap(((DWORD *)outBuffer)[i]);
			}
#ifdef DEBUG
		DebugDump(outBuffer,"",rounds+1,0,0,0,1);
		if (cipher->mode == MODE_CBC)
			DebugDump(cipher->iv32,"",IV_ROUND,0,0,0,0);
#endif
		}

	return inputLen;
	}

/*
+*****************************************************************************
*
* Function Name:	blockDecrypt
*
* Function:			Decrypt block(s) of data using Twofish
*
* Arguments:		cipher		=	ptr to already initialized cipherInstance
*					key			=	ptr to already initialized keyInstance
*					input		=	ptr to data blocks to be decrypted
*					inputLen	=	# bits to encrypt (multiple of blockSize)
*					outBuffer	=	ptr to where to put decrypted blocks
*
* Return:			# bits ciphered (>= 0)
*					else error code (e.g., BAD_CIPHER_STATE, BAD_KEY_MATERIAL)
*
* Notes: The only supported block size for ECB/CBC modes is BLOCK_SIZE bits.
*		 If inputLen is not a multiple of BLOCK_SIZE bits in those modes,
*		 an error BAD_INPUT_LEN is returned.  In CFB1 mode, all block 
*		 sizes can be supported.
*
-****************************************************************************/
int BlockDecrypt(cipherInstance *cipher, keyInstance *key,BYTE *input,
				int inputLen, BYTE *outBuffer)
	{
	int   i,n,r;					/* loop counters */
	DWORD x[BLOCK_SIZE/32];			/* block being encrypted */
	DWORD t0,t1;					/* temp variables */
	int	  rounds=key->numRounds;	/* number of rounds */
	BYTE  bit,ctBit,carry;			/* temps for CFB */
#if ALIGN32
	BYTE alignDummy;				/* keep 32-bit variable alignment on stack */
#endif

#if VALIDATE_PARMS
	if ((cipher == NULL) || (cipher->cipherSig != VALID_SIG))
		return BAD_CIPHER_STATE;
	if ((key == NULL) || (key->keySig != VALID_SIG))
		return BAD_KEY_INSTANCE;
	if ((rounds < 2) || (rounds > MAX_ROUNDS) || (rounds&1))
		return BAD_KEY_INSTANCE;
	if ((cipher->mode != MODE_CFB1) && (inputLen % BLOCK_SIZE))
		return BAD_INPUT_LEN;
  #if ALIGN32
	if ( (((int)cipher) & 3) || (((int)key      ) & 3) ||
		 (((int)input)  & 3) || (((int)outBuffer) & 3))
		return BAD_ALIGN32;
  #endif
#endif

	if (cipher->mode == MODE_CFB1)
		{	/* use blockEncrypt here to handle CFB, one block at a time */
		cipher->mode = MODE_ECB;	/* do encryption in ECB */
		for (n=0;n<inputLen;n++)
			{
			BlockEncrypt(cipher,key,cipher->IV,BLOCK_SIZE,(BYTE *)x);
			bit	  = 0x80 >> (n & 7);
			ctBit = input[n/8] & bit;
			outBuffer[n/8] = (outBuffer[n/8] & ~ bit) |
							 (ctBit ^ ((((BYTE *) x)[0] & 0x80) >> (n&7)));
			carry = ctBit >> (7 - (n&7));
			for (i=BLOCK_SIZE/8-1;i>=0;i--)
				{
				bit = cipher->IV[i] >> 7;	/* save next "carry" from shift */
				cipher->IV[i] = (cipher->IV[i] << 1) ^ carry;
				carry = bit;
				}
			}
		cipher->mode = MODE_CFB1;	/* restore mode for next time */
		return inputLen;
		}

	/* here for ECB, CBC modes */
	for (n=0;n<inputLen;n+=BLOCK_SIZE,input+=BLOCK_SIZE/8,outBuffer+=BLOCK_SIZE/8)
		{
//		DebugDump(input,"\n",rounds+1,0,0,0,1);

		for (i=0;i<BLOCK_SIZE/32;i++)	/* copy in the block, add whitening */
			x[i]=Bswap(((DWORD *)input)[i]) ^ key->subKeys[OUTPUT_WHITEN+i];

		for (r=rounds-1;r>=0;r--)			/* main Twofish decryption loop */
			{
			t0	 = f32(    x[0]   ,key->sboxKeys,key->keyLen);
			t1	 = f32(ROL(x[1],8),key->sboxKeys,key->keyLen);

//			DebugDump(x,"",r+1,2*(r&1),0,1,0);/* make format compatible with optimized code */
			x[2] = ROL(x[2],1);
			x[2]^= t0 +   t1 + key->subKeys[ROUND_SUBKEYS+2*r  ]; /* PHT, round keys */
			x[3]^= t0 + 2*t1 + key->subKeys[ROUND_SUBKEYS+2*r+1];
			x[3] = ROR(x[3],1);

			if (r)									/* unswap, except for last round */
				{
				t0   = x[0]; x[0]= x[2]; x[2] = t0;	
				t1   = x[1]; x[1]= x[3]; x[3] = t1;
				}
			}
//		DebugDump(x,"",0,0,0,0,0);/* make final output match encrypt initial output */

		for (i=0;i<BLOCK_SIZE/32;i++)	/* copy out, with whitening */
			{
			x[i] ^= key->subKeys[INPUT_WHITEN+i];
			if (cipher->mode == MODE_CBC)
				{
				x[i] ^= cipher->iv32[i];
				cipher->iv32[i] = Bswap(((DWORD *)input)[i]);
				}
			((DWORD *)outBuffer)[i] = Bswap(x[i]);
			}
//		DebugDump(outBuffer,"",-1,0,0,0,1);
		}

	return inputLen;
	}


#ifdef GetCodeSize
DWORD TwofishCodeSize(void) { return Here(0)-TwofishCodeStart(); };
#endif

#endif // OPTIMIZED_TWOFISH










int Twofish(int Operation, //1=Encrypt otherwise Decrypt
			int mode, // MODE_ECB | MODE_CBC
			int keySize, // keySize must be 128, 192, or 256 
			void*pKey, // 128 bits(16 bytes) or twice that.
			char *pzDataIn, // raw data or crypted input 
			int DataInSize, // length of pzDataIn 
			char *cryptDest) // Destination
{							


	if (DataInSize == 0)
		return 0;
	keyInstance    ki;			// key information, including tables 
	cipherInstance ci;			// keeps mode (ECB, CBC) and IV 
	BYTE iv[BLOCK_SIZE/8];
	int  i;//byteCnt;

	if (makeKey(&ki,DIR_ENCRYPT,keySize,NULL) != TRUE)
		return 1;				// 'dummy' setup for a 128-bit key 
	if (cipherInit(&ci,mode,NULL) != TRUE)
		return 1;				// 'dummy' setup for cipher 
	
	memcpy(&ki.key32,pKey,keySize/8);

	reKey(&ki);					// run the key schedule 

	if (mode != MODE_ECB)		// set up random iv (if needed)
		{
		for (i=0;i<sizeof(iv);i++)
			iv[i]=(BYTE) rand();
		memcpy(ci.iv32,iv,sizeof(ci.iv32));	// copy the IV to ci 
		}

	if (Operation == 1)
	{
		if (BlockEncrypt(&ci,&ki, (unsigned char *)pzDataIn,DataInSize*8,(unsigned char *)cryptDest) != DataInSize*8)
			return 1;
	}
	else
	{
		// decrypt the bytes 
		if (mode != MODE_ECB)		// first re-init the IV (if needed) 
			memcpy(ci.iv32,iv,sizeof(ci.iv32));
		if (BlockDecrypt(&ci,&ki,(unsigned char *)pzDataIn,DataInSize*8,(unsigned char *)cryptDest) != DataInSize*8)
			return 1;				
	}

	return 0;					
}


// returns 1 on success, 0 on Fail with description in  strErrorOut
// pDest will be allocated upon success - YOU must clean up, upon failure you do not cleanup.
// pDest will always start with these 7 bytes: 5Loaves, the nDestLen will 
// be set to the length of the data following the first 7 bytes.
int FileDecryptToMemory(GString &strKey, const char *strInFile, char **pDest, int *nDestLen, GString &strErrorOut)
{
	*nDestLen = 0;
	*pDest = 0;
	FILE *fp = fopen((const char *)strInFile,"rb");
	if (fp)
	{
		// get the size of the file
		fseek(fp,0,SEEK_END);
		long lFileBytes = ftell(fp);
		fseek(fp,0,SEEK_SET);

		char *pzIn = new char [lFileBytes + 64];
		*pDest = new char [lFileBytes + 64];
		if (pzIn && *pDest)
		{
			fread(pzIn,1,lFileBytes,fp);
			fclose(fp);
			if (memcmp(pzIn,"CipherBy5Loaves2Fish:", 21) != 0)
			{
				// 1=[%s] is not encrypyed - cannot decrypt.
				strErrorOut.Format("Unexpected Format in file[%s]",(const char *)strInFile);
				delete pzIn;
				delete *pDest;
				return 0;
			}

			unsigned char pKey[32];
			SHA256Hash((unsigned char *)strKey.Buf(), (int)strKey.Length(), pKey);

			char *pCipherStart = &pzIn[21]; // this is where the ciphered data starts
#ifdef WINCE
			memmove(pzIn,&pzIn[21],lFileBytes-21);
			pCipherStart = pzIn;
#endif

			int nReturn = Twofish(0, //1=Encrypt otherwise Decrypt
						MODE_ECB, // MODE_ECB | MODE_CBC
						256, // keySize must be 128, 192, or 256 
						pKey, // 32 bytes (256 bits)
						pCipherStart, // raw data or crypted input 
						lFileBytes-21, // length of pzDataIn 
						*pDest); // Destination
			if (!nReturn)
			{
				if (memcmp(*pDest,"5Loaves",7) == 0)
				{
					unsigned char chPad = (*pDest)[lFileBytes - 21 - 16];
					*nDestLen = lFileBytes - 21 - 7 - chPad - 16;
				}
				else
				{
					//2=Incorrect decrypt key for file [%s].
					strErrorOut.Format("Incorrect decrypt key for file [%s]", (const char *)strInFile);
				}

			}
			else
			{
				//3=Failed to decrypt file [%s] code [%d].
				strErrorOut.Format("Failed to decrypt file [%s] code [%d]", (const char *)strInFile, (int)nReturn);
			}
			delete pzIn;
		}
		else
		{
			//4=Not enough memory to decrypt file [%s].
			strErrorOut.Format("Not enough memory to decrypt file [%s]", (const char *)strInFile);
		}
	}
	else
	{
		//5=Failed to open or access[%s] - cannot decrypt.
		strErrorOut.Format("Failed to open or access[%s] - cannot decrypt", (const char *)strInFile);
	}
	if (strErrorOut.Length())
	{
		delete *pDest;
		return 0;
	}
	return 1;
}



// returns 1 on success, 0 on Fail with description in  strErrorOut
int FileDecrypt(GString &strKey, const char *strInFile, const char *strOutFile, GString &strErrorOut)
{
	char *pDest;
	int nDestLen;
	
	if (FileDecryptToMemory(strKey, strInFile, &pDest, &nDestLen, strErrorOut))
	{
		FILE *fpOut = fopen((const char *)strOutFile,"wb");
		if (fpOut)
		{
			fwrite(&pDest[7],1, nDestLen,fpOut);
			fclose(fpOut);
			delete pDest;
			return 1;
		}
		else
		{
			// "Failed to create output file
			strErrorOut.Format("Failed to create output file[%s]", (const char *)strOutFile);
			delete pDest;
			return 0;
		}
	}	
	if (strErrorOut.Length())
		return 0;
	return 1;
}



// returns 1 on success, 0 on Fail with description in  strErrorOut
int FileEncrypt(GString &strKey, const char *strInFile, const char *strOutFile, GString &strErrorOut)
{

	FILE *fp = fopen((const char *)strInFile,"rb");
	if (fp)
	{
		// get the size of the file
		fseek(fp,0,SEEK_END);
		long lFileBytes = ftell(fp);
		fseek(fp,0,SEEK_SET);

		// bytes required to pad the last block to a 128 bit boundry
		unsigned char nPad = (16 - (lFileBytes + 7) % 16);
		nPad = (nPad == 16) ? 0 : nPad;


		char *pInBuf = new char [lFileBytes + 7 + nPad + 16];
		char *pDestBuf = new char [lFileBytes + 64];
		memcpy(pDestBuf,"CipherBy5Loaves2Fish:",21);
		if (pInBuf && pDestBuf)
		{
			memcpy(pInBuf,"5Loaves",7);
			fread(&pInBuf[7],1,lFileBytes,fp);
			fclose(fp);
			
			// if this file is already encrypted do not double encrypt it
			if (memcmp(&pInBuf[7],"CipherBy5Loaves2Fish:",21) != 0)
			{
				unsigned char pKey[32];
				SHA256Hash((unsigned char *)strKey.Buf(), (int)strKey.Length(), pKey);


				pInBuf[7 + lFileBytes + nPad] = nPad;
				
				char *pCipherDest = &pDestBuf[21]; // this is where we ultimately want it
#ifdef WINCE
				pCipherDest = new char [lFileBytes + 64]; // but put it here first if we must
#endif

				int nReturn = Twofish(1, //1=Encrypt otherwise Decrypt
							MODE_ECB, // MODE_ECB | MODE_CBC
							256, // keySize must be 128, 192, or 256 
							pKey, // 32 bytes (256 bits)
							pInBuf, // raw data or crypted input 
							lFileBytes + 7 + nPad + 16, // number of bytes to encrypt 
							pCipherDest); // Destination - past "CipherBy5Loaves2Fish:"
				if (!nReturn)
				{
#ifdef WINCE
					int nCipherDataSize = 7+             +lFileBytes	 	+ nPad + 16;
					memmove(&pDestBuf[21],pCipherDest,nCipherDataSize);
					delete pCipherDest;
#endif
					
					FILE *fpOut = fopen((const char *)strOutFile,"wb");
					if (fpOut)
					{
						//				   clear-header	    crypted header  file data    	  pad    pad-count block
						fwrite(pDestBuf,1, 21+				7+             +lFileBytes	 	+ nPad + 16,		fpOut);
						fclose(fpOut);
					}
					else
					{
						//6=Failed to create Cipher output file [%s].
						strErrorOut.Format("Failed to create Cipher output file [%s]", (const char *)strOutFile);
					}

				}
				else
				{
					//7=Failed to decrypt file [%s] code [%d].
					strErrorOut.Format("Failed to decrypt file [%s] code [%d]", (const char *)strInFile, (int)nReturn);
				}
			}
			
			delete pInBuf;
			delete pDestBuf;
		}
		else
		{
			//8=Not enough memory to encrypt file [%s].
			strErrorOut.Format("Not enough memory to encrypt file [%s]", (const char *)strInFile);
		}
	}
	else
	{
		//9=Failed to open or access[%s] - cannot encrypt.
		strErrorOut.Format("Failed to open or access[%s] - cannot encrypt", (const char *)strInFile);
		
	}

	if (strErrorOut.Length())
		return 0;
	return 1;
}

int CipherDir(int bIsEncrypt, GString &strPass, const char *pzDirectory, int bRecurseDeep, GString &strErrorOut)
{
	if (bRecurseDeep)
	{
		GStringList strlistFiles;
		GDirectory::RecurseFolder(pzDirectory, 0/*no dirs*/, &strlistFiles);

		GStringIterator it(&strlistFiles);
		while (it())
		{
			const char *pFile = it++;
			// returns 1 on success, 0 on Fail with description in  strErrorOut
			if (bIsEncrypt)
			{
				if (!FileEncrypt(strPass, pFile, pFile, strErrorOut))
				{
					return 0; // fail
				}
			}
			else
			{
				if (!FileDecrypt(strPass, pFile, pFile, strErrorOut))
				{
					return 0; // fail
				}
			}
		}
		return 1; // done
	}
	else
	{
		GDirectory dir(pzDirectory, 1); // nMode = 1 files, 2 dirs, 3 both
		GStringIterator it(&dir);
		while (it())
		{
			const char *pFile = it++;
			// returns 1 on success, 0 on Fail with description in  strErrorOut
			if (bIsEncrypt)
			{
				if (!FileEncrypt(strPass, pFile, pFile, strErrorOut))
				{
					return 0; // fail
				}
			}
			else
			{
				if (!FileDecrypt(strPass, pFile, pFile, strErrorOut))
				{
					return 0; // fail
				}
			}
		}
		return 1; // done
	}
}



int EncryptMemoryToFile(GString &strKey, const char *pIn,long pInLen,const char *strOutFile, GString &strErrorOut)
{
	long lFileBytes = pInLen;

	// bytes required to pad the last block to a 128 bit boundry
	unsigned char nPad = (16 - (lFileBytes + 7) % 16);
	nPad = (nPad == 16) ? 0 : nPad;


	char *pInBuf = new char [lFileBytes + 7 + nPad + 16];
	char *pDestBuf = new char [lFileBytes + 64];
	memcpy(pDestBuf,"CipherBy5Loaves2Fish:",21);
	if (pInBuf && pDestBuf)
	{
		memcpy(pInBuf,"5Loaves",7);
		memcpy(&pInBuf[7],pIn,lFileBytes);
		
		// if this file is already encrypted do not double encrypt it
		if (memcmp(&pInBuf[7],"CipherBy5Loaves2Fish:",21) != 0)
		{
			unsigned char pKey[32];
			SHA256Hash((unsigned char *)strKey.Buf(), (int)strKey.Length(), pKey);

			pInBuf[7 + lFileBytes + nPad] = nPad;

			int nReturn = Twofish(1, //1=Encrypt otherwise Decrypt
						MODE_ECB, // MODE_ECB | MODE_CBC
						256, // keySize must be 128, 192, or 256 
						pKey, // 32 bytes (256 bits)
						pInBuf, // raw data or crypted input 
						lFileBytes + 7 + nPad + 16, // number of bytes to encrypt 
						&pDestBuf[21]); // Destination - past "CipherBy5Loaves2Fish:"
			if (!nReturn)
			{
				FILE *fpOut = fopen((const char *)strOutFile,"wb");
				if (fpOut)
				{
					//				   clear-header	    crypted header  file data    	  pad    pad-count block
					fwrite(pDestBuf,1, 21+				7+             +lFileBytes	 	+ nPad + 16,		fpOut);
					fclose(fpOut);
				}
				else
				{
					//6=Failed to create Cipher output file [%s].
					strErrorOut.Format("Failed to create Cipher output file [%s]", (const char *)strOutFile);
				}

			}
			else
			{
				//7=Failed to encrypt file [%s] code [%d].
				strErrorOut.Format("Failed to encrypt file [%s] code [%d]", (const char *)strOutFile, (int)nReturn);
			}
		}
		
		delete pInBuf;
		delete pDestBuf;
	}
	else
	{
		//8=Not enough memory to encrypt file [%s].
		strErrorOut.Format("Not enough memory to encrypt file [%s]", (const char *)strOutFile);
	}

	if (strErrorOut.Length())
		return 0;
	return 1;
}


bool EncryptMemoryToMemory(GString &strKey, const char *pIn,__int64 pInLen,GString &strDest, GString &strErrorOut)
{
	__int64 lFileBytes = pInLen;

	// bytes required to pad the last block to a 128 bit boundry
	unsigned char nPad = (16 - (lFileBytes + 7) % 16);
	nPad = (nPad == 16) ? 0 : nPad;


	char *pInBuf = new char [lFileBytes + 7 + nPad + 16];
	char *pDestBuf = new char [lFileBytes + 64];
	if (pInBuf && pDestBuf)
	{
		memcpy(pDestBuf,"CipherBy5Loaves2Fish:",21);
		memcpy(pInBuf,"5Loaves",7);
		memcpy(&pInBuf[7],pIn,lFileBytes);
		
		// if this file is already encrypted do not double encrypt it
		if (memcmp(&pInBuf[7],"CipherBy5Loaves2Fish:",21) != 0)
		{
			unsigned char pKey[32];
			SHA256Hash((unsigned char *)strKey.Buf(), (int)strKey.Length(), pKey);

			pInBuf[7 + lFileBytes + nPad] = nPad;

			int nReturn = Twofish(1, //1=Encrypt otherwise Decrypt
						MODE_ECB, // MODE_ECB | MODE_CBC
						256, // keySize must be 128, 192, or 256 
						pKey, // 32 bytes (256 bits)
						pInBuf, // raw data or crypted input 
						lFileBytes + 7 + nPad + 16, // number of bytes to encrypt 
						&pDestBuf[21]); // Destination - past "CipherBy5Loaves2Fish:"
			if (!nReturn)
			{
				strDest.write(&pDestBuf[21], 7+             +lFileBytes	 	+ nPad + 16);
			}
			else
			{
				//7=Failed to encrypt file [%s] code [%d].
				strErrorOut.Format("Failed to encrypt Memory code [%d]", (int)nReturn);
			}
		}
		
		delete pInBuf;
		delete pDestBuf;
	}
	else
	{
		//8=Not enough memory to encrypt file [%s].
		strErrorOut = "Not enough memory to decrypt";
	}

	if (strErrorOut.Length())
		return 0;
	return 1;
}


// returns 1 on success, 0 on Fail with description in  strErrorOut
bool DecryptMemoryToMemory(GString &strKey, const char *pIn,__int64 pInLen, GString &strDest,  GString &strErrorOut)
{
	strDest.SetPreAlloc(pInLen + 64);

	unsigned char pKey[32];
	SHA256Hash((unsigned char *)strKey.Buf(), (int)strKey.Length(), pKey);

	int nReturn = Twofish(0, //1=Encrypt otherwise Decrypt
				MODE_ECB, // MODE_ECB | MODE_CBC
				256, // keySize must be 128, 192, or 256 
				pKey, // 32 bytes (256 bits)
				(char *)pIn, // raw data or crypted input 
				pInLen, // length of pzDataIn 
				strDest.Buf()); // Destination
	if (!nReturn)
	{
		if (memcmp(strDest.Buf(),"5Loaves",7) == 0)
		{
			strDest.SetLength(pInLen);
			unsigned char chPad = strDest.Buf()[pInLen - 16];
			int nLen = pInLen - 7 - chPad - 16;
			strDest = strDest.Mid(7,nLen);
		}
		else
		{
			//2=Incorrect decrypt key for file [%s].
			return 0;
		}
	}
	else
	{
		//3=Failed to decrypt file [%s] code [%d].
		// strErrorOut.LoadResource(g_pzCryptErrorSection, 3, "Memory", nReturn);
		return 0;
	}


	if (strErrorOut.Length())
		return 0;
	return 1;
}

int CipherData::blockEncrypt(BYTE *input,int inputLen, BYTE *outBuffer)
{
	if (!bIsInitted)
		return 0;
	return ::BlockEncrypt(&ci,&ki,input,inputLen,outBuffer);
}

int CipherData::blockDecrypt(BYTE *input,int inputLen, BYTE *outBuffer)
{
	if (!bIsInitted)
		return 0;
	return ::BlockDecrypt(&ci,&ki,input,inputLen,outBuffer);
}

CipherData::CipherData(const char *pKey, int nDIR)
{
	memset(&ki,0,sizeof(keyInstance));
	memset(&ci,0,sizeof(cipherInstance));

	CipherKeyBits = 256;
	pzCipherPass = (pKey) ? pKey : "";

	if (cipherInit(&ci,MODE_ECB,NULL) != TRUE)
	{
		bIsInitted = 0;
		return; // failed
	}

	// create a digest onto the memory at: (unsigned char *)ki.key32
	SHA256Hash((unsigned char *)pzCipherPass, (int)strlen(pzCipherPass), (unsigned char *)ki.key32);

	//	makeKey(&ki,nDIR,CipherKeyBits,0);
	//	makeKey() is from the TwoFish sample implementation code, 
	//  I intend to replicate and simplify this call here:
	ki.keySig		= 0x48534946;			// initialization signature ('FISH')
	ki.direction	= nDIR;					// DIR_ENCRYPT or DIR_DECRYPT
	ki.keyLen		= CipherKeyBits;	// 128 or 256 - always 256 here.
	ki.numRounds	= 16;


	// build the key schedule
	reKey(&ki);
	bIsInitted = 1;
}
