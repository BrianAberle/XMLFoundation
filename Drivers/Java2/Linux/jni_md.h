/*
 * @(#)src/contract/jvm/pfm/jni_md.h, xx130, xx130, 20000522 1.3.2.1
 * ===========================================================================
 * Licensed Materials - Property of IBM
 * IBM Java(tm)2 SDK, Standard Edition, v 1.2
 *
 * (C) Copyright IBM Corp. 1999 All Rights Reserved.
 * Copyright 1996-1998 by Sun Microsystems Inc.,
 * ===========================================================================
 */
#ifndef _JAVASOFT_JNI_MD_H_
#define _JAVASOFT_JNI_MD_H_

#define JNIEXPORT_PROTOTYPE                                           /*ibm.54*/
#define JNIEXPORT
#define JNIIMPORT
#define JNICALL

#ifdef _LP64 /* 64-bit Solaris */
typedef int jint;
#else
typedef long jint;
#endif
typedef long long jlong;
typedef signed char jbyte;
typedef jint jsize;                                                 /*ibm@9237*/

#endif /* !_JAVASOFT_JNI_MD_H_ */
