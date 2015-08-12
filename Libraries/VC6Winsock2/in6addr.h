// --UBT note: based on file dated 9/14/2014 found in [C:\Program Files (x86)\Windows Kits\8.1\Include\shared], equal in structure to previoius file dates
/*++

Copyright (c) Microsoft Corporation

Module Name:

    in6addr.h

Environment:

    user mode or kernel mode

--*/

#ifndef s6_addr
#pragma once

//
// IPv6 Internet address (RFC 2553)
// This is an 'on-wire' format structure.
//
typedef struct in6_addr {
    union {
        UCHAR       Byte[16];
        USHORT      Word[8];
    } u;
} IN6_ADDR, *PIN6_ADDR, FAR *LPIN6_ADDR;

#define in_addr6 in6_addr

//
// Defines to match RFC 2553.
//
#define _S6_un      u
#define _S6_u8      Byte
#define s6_addr     _S6_un._S6_u8

//
// Defines for our implementation.
//
#define s6_bytes    u.Byte
#define s6_words    u.Word

#endif
