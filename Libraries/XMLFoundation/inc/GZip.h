// nLevel
// ----------------------
// Z_NO_COMPRESSION		 0
// Z_BEST_SPEED			 1
// Z_BEST_COMPRESSION	 9
// Z_DEFAULT_COMPRESSION -1

// nStrategy
// ----------------------
// Z_FILTERED            1
// Z_HUFFMAN_ONLY        2
// Z_RLE                 3
// Z_FIXED               4
// Z_DEFAULT_STRATEGY    0

int GZipBuffToBuffCompress( 
      char*         pDestBuffer, 
      unsigned int* destLen,
      char*         pSource, 
      unsigned int  nSourceLen,
      int           nLevel = -1, 
      int           nStrategy = 0 
   );


int GZipBuffToBuffDecompress ( 
      char*         dest, 
      unsigned int* destLen,
      char*         source, 
      unsigned int  sourceLen
   );

