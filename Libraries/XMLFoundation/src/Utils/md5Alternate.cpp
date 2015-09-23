// MD5.CC - source code for the C++/object oriented translation and 
//          modification of MD5.

// Translation and modification (c) 1995 by Mordechai T. Abzug 

// This translation/ modification is provided "as is," without express or 
// implied warranty of any kind.

// The translator/ modifier does not claim (1) that MD5 will do what you think 
// it does; (2) that this translation/ modification is accurate; or (3) that 
// this software is "merchantible."  (Language for this disclaimer partially 
// copied from the disclaimer below).

/* based on:

   MD5C.C - RSA Data Security, Inc., MD5 message-digest algorithm
   MDDRIVER.C - test driver for MD2, MD4 and MD5


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








#include <assert.h>
#include <string.h>
#include <iostream>
using namespace std;

#pragma warning (disable:4996)

// MD5 simple initialization method

MD5Alt::MD5Alt(){

  init();

}




// MD5 block update operation. Continues an MD5 message-digest
// operation, processing another message block, and updating the
// context.

void MD5Alt::update (uint1 *input, uint4 input_length) {

  uint4 input_index, buffer_index;
  uint4 buffer_space;                // how much space is left in buffer

  if (finalized){  // so we can't update!
    cerr << "MD5Alt::update:  Can't update a finalized digest!" << endl;
    return;
  }

  // Compute number of bytes mod 64
  buffer_index = (unsigned int)((count[0] >> 3) & 0x3F);

  // Update number of bits
  if (  (count[0] += ((uint4) input_length << 3))<((uint4) input_length << 3) )
    count[1]++;

  count[1] += ((uint4)input_length >> 29);


  buffer_space = 64 - buffer_index;  // how much space is left in buffer

  // Transform as many times as possible.
  if (input_length >= buffer_space) { // ie. we have enough to fill the buffer
    // fill the rest of the buffer and transform
    memcpy (buffer + buffer_index, input, buffer_space);
    transform (buffer);

    // now, transform each 64-byte piece of the input, bypassing the buffer
    for (input_index = buffer_space; input_index + 63 < input_length; 
	 input_index += 64)
      transform (input+input_index);

    buffer_index = 0;  // so we can buffer remaining
  }
  else
    input_index=0;     // so we can buffer the whole input


  // and here we do the buffering:
  memcpy(buffer+buffer_index, input+input_index, input_length-input_index);
}



// MD5 update for files.
// Like above, except that it works on files (and uses above as a primitive.)

void MD5Alt::update(FILE *file){

  unsigned char buffer[1024];
  int len;

  while ((len=fread(buffer, 1, 1024, file))!=0)
    update(buffer, len);

  fclose (file);

}






// MD5 update for istreams.
// Like update for files; see above.

void MD5Alt::update(istream& stream){

  unsigned char buffer[1024];
  int len;

  while (stream.good()){
    stream.read((char *)buffer, 1024); // note that return value of read is unusable.
    len=(int) stream.gcount();
    update(buffer, len);
  }

}






// MD5 update for ifstreams.
// Like update for files; see above.

void MD5Alt::update(ifstream& stream){

  unsigned char buffer[1024];
  int len;

  while (stream.good()){
    stream.read((char *) buffer, 1024); // note that return value of read is unusable.
    len=(int) stream.gcount();
    update(buffer, len);
  }

}






// MD5 finalization. Ends an MD5 message-digest operation, writing the
// the message digest and zeroizing the context.


void MD5Alt::finalize (){

  unsigned char bits[8];
  unsigned int index, padLen;
  static uint1 PADDING[64]={
    0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };

  if (finalized){
    cerr << "MD5Alt::finalize:  Already finalized this digest!" << endl;
    return;
  }

  // Save number of bits
  encode (bits, count, 8);

  // Pad out to 56 mod 64.
  index = (uint4) ((count[0] >> 3) & 0x3f);
  padLen = (index < 56) ? (56 - index) : (120 - index);
  update (PADDING, padLen);

  // Append length (before padding)
  update (bits, 8);

  // Store state in digest
  encode (digest, state, 16);

  // Zeroize sensitive information
  memset (buffer, 0, sizeof(buffer));

  finalized=1;

}




MD5Alt::MD5Alt(FILE *file){

  init();  // must be called be all constructors
  update(file);
  finalize ();
}




MD5Alt::MD5Alt(istream& stream){

  init();  // must called by all constructors
  update (stream);
  finalize();
}



MD5Alt::MD5Alt(ifstream& stream){

  init();  // must called by all constructors
  update (stream);
  finalize();
}



unsigned char *MD5Alt::raw_digest(){

  uint1 *s = new uint1[16];

  if (!finalized){
    cerr << "MD5Alt::raw_digest:  Can't get digest if you haven't "<<
      "finalized the digest!" <<endl;
    return ( (unsigned char*) "");
  }

  memcpy(s, digest, 16);
  return s;
}



char *MD5Alt::hex_digest(){

  int i;
  char *s= new char[33];

  ::memset(s, 0, 33);	
  if (!finalized){
    cerr << "MD5Alt::hex_digest:  Can't get digest if you haven't "<<
      "finalized the digest!" <<endl;
    return s;
  }

  for (i=0; i<16; i++)
    sprintf(s+i*2, "%02x", digest[i]);

  s[32]='\0';

  return s;
}


#include "md5Alternate.h"
static MD5Alt *pGGGGG;

ostream& operator<<(ostream &stream, MD5Alt *context){

  stream << context->hex_digest();
  return stream;
}




// PRIVATE METHODS:



void MD5Alt::init(){
  finalized=0;  // we just started!

  // Nothing counted, so count=0
  count[0] = 0;
  count[1] = 0;

  // Load magic initialization constants.
  state[0] = 0x67452301;
  state[1] = 0xefcdab89;
  state[2] = 0x98badcfe;
  state[3] = 0x10325476;
}



// Constants for MD5Transform routine.
// Although we could use C++ style constants, defines are actually better,
// since they let us easily evade scope clashes.

#define S11 7
#define S12 12
#define S13 17
#define S14 22
#define S21 5
#define S22 9
#define S23 14
#define S24 20
#define S31 4
#define S32 11
#define S33 16
#define S34 23
#define S41 6
#define S42 10
#define S43 15
#define S44 21




// MD5 basic transformation. Transforms state based on block.
void MD5Alt::transform (uint1 block[64]){

  uint4 a = state[0], b = state[1], c = state[2], d = state[3], x[16];

  decode (x, block, 64);

  assert(!finalized);  // not just a user error, since the method is private

  /* Round 1 */
  FFX (a, b, c, d, x[ 0], S11, 0xd76aa478); /* 1 */
  FFX (d, a, b, c, x[ 1], S12, 0xe8c7b756); /* 2 */
  FFX (c, d, a, b, x[ 2], S13, 0x242070db); /* 3 */
  FFX (b, c, d, a, x[ 3], S14, 0xc1bdceee); /* 4 */
  FFX (a, b, c, d, x[ 4], S11, 0xf57c0faf); /* 5 */
  FFX (d, a, b, c, x[ 5], S12, 0x4787c62a); /* 6 */
  FFX (c, d, a, b, x[ 6], S13, 0xa8304613); /* 7 */
  FFX (b, c, d, a, x[ 7], S14, 0xfd469501); /* 8 */
  FFX (a, b, c, d, x[ 8], S11, 0x698098d8); /* 9 */
  FFX (d, a, b, c, x[ 9], S12, 0x8b44f7af); /* 10 */
  FFX (c, d, a, b, x[10], S13, 0xffff5bb1); /* 11 */
  FFX (b, c, d, a, x[11], S14, 0x895cd7be); /* 12 */
  FFX (a, b, c, d, x[12], S11, 0x6b901122); /* 13 */
  FFX (d, a, b, c, x[13], S12, 0xfd987193); /* 14 */
  FFX (c, d, a, b, x[14], S13, 0xa679438e); /* 15 */
  FFX (b, c, d, a, x[15], S14, 0x49b40821); /* 16 */

 /* Round 2 */
  GGX (a, b, c, d, x[ 1], S21, 0xf61e2562); /* 17 */
  GGX (d, a, b, c, x[ 6], S22, 0xc040b340); /* 18 */
  GGX (c, d, a, b, x[11], S23, 0x265e5a51); /* 19 */
  GGX (b, c, d, a, x[ 0], S24, 0xe9b6c7aa); /* 20 */
  GGX (a, b, c, d, x[ 5], S21, 0xd62f105d); /* 21 */
  GGX (d, a, b, c, x[10], S22,  0x2441453); /* 22 */
  GGX (c, d, a, b, x[15], S23, 0xd8a1e681); /* 23 */
  GGX (b, c, d, a, x[ 4], S24, 0xe7d3fbc8); /* 24 */
  GGX (a, b, c, d, x[ 9], S21, 0x21e1cde6); /* 25 */
  GGX (d, a, b, c, x[14], S22, 0xc33707d6); /* 26 */
  GGX (c, d, a, b, x[ 3], S23, 0xf4d50d87); /* 27 */
  GGX (b, c, d, a, x[ 8], S24, 0x455a14ed); /* 28 */
  GGX (a, b, c, d, x[13], S21, 0xa9e3e905); /* 29 */
  GGX (d, a, b, c, x[ 2], S22, 0xfcefa3f8); /* 30 */
  GGX (c, d, a, b, x[ 7], S23, 0x676f02d9); /* 31 */
  GGX (b, c, d, a, x[12], S24, 0x8d2a4c8a); /* 32 */

  /* Round 3 */
  HHX (a, b, c, d, x[ 5], S31, 0xfffa3942); /* 33 */
  HHX (d, a, b, c, x[ 8], S32, 0x8771f681); /* 34 */
  HHX (c, d, a, b, x[11], S33, 0x6d9d6122); /* 35 */
  HHX (b, c, d, a, x[14], S34, 0xfde5380c); /* 36 */
  HHX (a, b, c, d, x[ 1], S31, 0xa4beea44); /* 37 */
  HHX (d, a, b, c, x[ 4], S32, 0x4bdecfa9); /* 38 */
  HHX (c, d, a, b, x[ 7], S33, 0xf6bb4b60); /* 39 */
  HHX (b, c, d, a, x[10], S34, 0xbebfbc70); /* 40 */
  HHX (a, b, c, d, x[13], S31, 0x289b7ec6); /* 41 */
  HHX (d, a, b, c, x[ 0], S32, 0xeaa127fa); /* 42 */
  HHX (c, d, a, b, x[ 3], S33, 0xd4ef3085); /* 43 */
  HHX (b, c, d, a, x[ 6], S34,  0x4881d05); /* 44 */
  HHX (a, b, c, d, x[ 9], S31, 0xd9d4d039); /* 45 */
  HHX (d, a, b, c, x[12], S32, 0xe6db99e5); /* 46 */
  HHX (c, d, a, b, x[15], S33, 0x1fa27cf8); /* 47 */
  HHX (b, c, d, a, x[ 2], S34, 0xc4ac5665); /* 48 */

  /* Round 4 */
  IIX (a, b, c, d, x[ 0], S41, 0xf4292244); /* 49 */
  IIX (d, a, b, c, x[ 7], S42, 0x432aff97); /* 50 */
  IIX (c, d, a, b, x[14], S43, 0xab9423a7); /* 51 */
  IIX (b, c, d, a, x[ 5], S44, 0xfc93a039); /* 52 */
  IIX (a, b, c, d, x[12], S41, 0x655b59c3); /* 53 */
  IIX (d, a, b, c, x[ 3], S42, 0x8f0ccc92); /* 54 */
  IIX (c, d, a, b, x[10], S43, 0xffeff47d); /* 55 */
  IIX (b, c, d, a, x[ 1], S44, 0x85845dd1); /* 56 */
  IIX (a, b, c, d, x[ 8], S41, 0x6fa87e4f); /* 57 */
  IIX (d, a, b, c, x[15], S42, 0xfe2ce6e0); /* 58 */
  IIX (c, d, a, b, x[ 6], S43, 0xa3014314); /* 59 */
  IIX (b, c, d, a, x[13], S44, 0x4e0811a1); /* 60 */
  IIX (a, b, c, d, x[ 4], S41, 0xf7537e82); /* 61 */
  IIX (d, a, b, c, x[11], S42, 0xbd3af235); /* 62 */
  IIX (c, d, a, b, x[ 2], S43, 0x2ad7d2bb); /* 63 */
  IIX (b, c, d, a, x[ 9], S44, 0xeb86d391); /* 64 */

  state[0] += a;
  state[1] += b;
  state[2] += c;
  state[3] += d;

  // Zeroize sensitive information.
  memset ( (uint1 *) x, 0, sizeof(x));

}



// Encodes input (UINT4) into output (unsigned char). Assumes len is
// a multiple of 4.
void MD5Alt::encode (uint1 *output, uint4 *input, uint4 len) {

  unsigned int i, j;

  for (i = 0, j = 0; j < len; i++, j += 4) {
    output[j]   = (uint1)  (input[i] & 0xff);
    output[j+1] = (uint1) ((input[i] >> 8) & 0xff);
    output[j+2] = (uint1) ((input[i] >> 16) & 0xff);
    output[j+3] = (uint1) ((input[i] >> 24) & 0xff);
  }
}




// Decodes input (unsigned char) into output (UINT4). Assumes len is
// a multiple of 4.
void MD5Alt::decode (uint4 *output, uint1 *input, uint4 len){

  unsigned int i, j;

  for (i = 0, j = 0; j < len; i++, j += 4)
    output[i] = ((uint4)input[j]) | (((uint4)input[j+1]) << 8) |
      (((uint4)input[j+2]) << 16) | (((uint4)input[j+3]) << 24);
}





// Note: Replace "for loop" with standard memcpy if possible.
void MD5Alt::memcpy (uint1 *output, uint1 *input, uint4 len){

  unsigned int i;

  for (i = 0; i < len; i++)
    output[i] = input[i];
}



// Note: Replace "for loop" with standard memset if possible.
void MD5Alt::memset (uint1 *output, uint1 value, uint4 len){

  unsigned int i;

  for (i = 0; i < len; i++)
    output[i] = value;
}



// ROTATE_LEFT rotates x left n bits.

inline unsigned int MD5Alt::rotate_left  (uint4 x, uint4 n){
  return (x << n) | (x >> (32-n))  ;
}




// F, G, H and I are basic MD5 functions.

inline unsigned int MD5Alt::FX            (uint4 x, uint4 y, uint4 z){
  return (x & y) | (~x & z);
}

inline unsigned int MD5Alt::GX            (uint4 x, uint4 y, uint4 z){
  return (x & z) | (y & ~z);
}

inline unsigned int MD5Alt::HX            (uint4 x, uint4 y, uint4 z){
  return x ^ y ^ z;
}

inline unsigned int MD5Alt::IX            (uint4 x, uint4 y, uint4 z){
  return y ^ (x | ~z);
}



// FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4.
// Rotation is separate from addition to prevent recomputation.


inline void MD5Alt::FFX(uint4& a, uint4 b, uint4 c, uint4 d, uint4 x,     uint4  s, uint4 ac){
 a += FX(b, c, d) + x + ac;
 a = rotate_left (a, s) +b;
}

inline void MD5Alt::GGX(uint4& a, uint4 b, uint4 c, uint4 d, uint4 x,     uint4 s, uint4 ac){
 a += GX(b, c, d) + x + ac;
 a = rotate_left (a, s) +b;
}

inline void MD5Alt::HHX(uint4& a, uint4 b, uint4 c, uint4 d, uint4 x,     uint4 s, uint4 ac){
 a += HX(b, c, d) + x + ac;
 a = rotate_left (a, s) +b;
}

inline void MD5Alt::IIX(uint4& a, uint4 b, uint4 c, uint4 d, uint4 x,      uint4 s, uint4 ac){
 a += IX(b, c, d) + x + ac;
 a = rotate_left (a, s) +b;
}

