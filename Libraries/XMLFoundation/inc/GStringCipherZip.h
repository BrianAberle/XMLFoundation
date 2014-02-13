#include "GString.h"

#include "GZip.h"
#include "TwoFish.h"



// For simplifying build dependencies, Zip and Cipher were not added to the 'GString' but to this derivation of it.
class GStringCipherZip  : public GString
{
public:
	GStringCipherZip() 	{ 	}
	
	// pSrc contains the unciphered data
	// strKey is the cipher password
	// 'this' contains the ciphered results when Cipher() returns 1
	bool Cipher( GString &strKey, GString *pSrc )
	{
		// returns 1 on success, 0 on Fail with description in  strErrorOut
		GString strErrorOut;
		SetPreAlloc( pSrc->Length() + 64 );
		return EncryptMemoryToMemory(strKey, pSrc->_str, pSrc->_len, *this, strErrorOut);
	}

	// This Cipher() assumes 'this' contains the unciphered source
	// unciphered data in 'this' will be replaced with the ciphered version of that data.
//	 bool Cipher( GString &strKey )

	// pSrc contains the ciphered data
	// strKey is the cipher password
	// 'this' contains the unciphered results when Cipher() returns 1
	bool DeCipher( GString &strKey, GString *pSrc )
	{
		GString strErrorOut;
		SetPreAlloc( pSrc->Length() + 64 );
		return DecryptMemoryToMemory(strKey, pSrc->_str, pSrc->_len, *this, strErrorOut);
	}


	// This DeCipher() assumes 'this' contains the ciphered source - possibly loaded with FromFile()
	//  ciphered data in 'this' will be replaced with the unciphered version of that data.
//	bool DeCipher( GString &strKey )
	

	bool DeCompress( const char *pFileName, __int64 nDecompressedSize = -1)
	{
		GString strG;
		if ( strG.FromFile(pFileName,0) )
		{
			Empty();
			return DeCompress( &strG,nDecompressedSize );
		}
		return 0;
	}

	// pSrc is the compressed data in a GString
	// 'this' contains the uncompressed results
	bool DeCompress( GString *pSrc, __int64 nDestSize = -1)
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

	// this DeCompress() assumes that the current contents of 'this' are the compressed data, perhaps assigned by FromFile()
	// the compressed source will be replaced with the uncompressed form of the data in 'this'
//	bool DeCompress()

	// pSrc is the uncompressed GString
	// 'this' contains the results
	bool Compress( GString *pSrc )
	{
		unsigned int nZippedLength = (unsigned int)pSrc->GetLength();
		SetPreAlloc( nZippedLength );

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
//	bool Compress( )

};
