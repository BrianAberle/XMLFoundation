// --------------------------------------------------------------------------
//						United Business Technologies
//			  Copyright (c) 2000 - 2010  All Rights Reserved.
//
// Source in this file is released to the public under the following license:
// --------------------------------------------------------------------------
// This toolkit may be used free of charge for any purpose including corporate
// and academic use.  For profit, and Non-Profit uses are permitted.
//
// This source code and any work derived from this source code must retain 
// this copyright at the top of each source file.
// --------------------------------------------------------------------------
// WinCERuntimeC.h
#ifndef _CE_C_RUNTIME_
#define _CE_C_RUNTIME_

void * _beginthreadex(void* security, unsigned stack_size, unsigned (__stdcall* start_address)(void*),  void* arglist, unsigned initflag, unsigned* thrdaddr);

extern int errno;
int rmdir(const char *filename);
int unlink(const char *filename);
int mkdir( const char* dirname );
int stat(const char* filename, struct stat *buffer);
int access( const char *path, int mode );
char* getenv(const char* varname);
int rename(const char *oldfile, const char *newfile);
long getTimeMS();

#include <time.h> // for struct tm definition and time_t definition


//size_t strftime(char *s, size_t maxsize, const char* format, const struct tm *timeptr);

// the following are decalred below the definition of time_t
//time_t time(time_t *timer);
//struct tm *gmtime(const time_t *timer);
//struct tm *localtime(const time_t *timer);
//time_t mktime(struct tm *tmbuff);


#define TM_YEAR_BASE	1900

// Portions were developed by UBT
// Portions were also taken from code with the following copyright.

/*
 * $Id: wce_stat.h,v 1.2 2006/04/09 16:48:18 mloskot Exp $
 *
 * sys/stat.h - data returned by the stat() function
 *
 * Created by Mateusz Loskot (mateusz@loskot.net)
 *
 * Copyright (c) 2006 Taxus SI Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom 
 * the Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH
 * THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * MIT License:
 * http://opensource.org/licenses/mit-license.php
 *
 * Contact:
 * Taxus SI Ltd.
 * http://www.taxussi.com.pl
 *
 */
#ifndef WCEEX_STAT_H
#define WCEEX_STAT_H 1

#if !defined(_WIN32_WCE)
# error "Only Winddows CE target is supported!"
#endif


//#include <wce_types.h>
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef WCEEX_SYS_TYPES_H
#define WCEEX_SYS_TYPES_H 1

#if !defined(_WIN32_WCE)
# error "Only Winddows CE target is supported!"
#endif

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */



#ifndef _SIZE_T_DEFINED
typedef unsigned int    size_t;     /* Used for sizes of objects. */
# define _SIZE_T_DEFINED
#endif


#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* #ifndef WCEEX_SYS_TYPES_H */

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#ifndef _STAT_T_DEFINED
struct stat
{
	/* Drive number of the disk containing the file (same as st_rdev). */
	unsigned int st_dev;

	/*
     * Number of the information node (the inode) for the file (UNIX-specific).
     * On UNIX file systems, the inode describes the file date and time stamps, permissions, and content.
     * When files are hard-linked to one another, they share the same inode. 
     * The inode, and therefore st_ino, has no meaning in the FAT, HPFS, or NTFS file systems.
     */
	unsigned short st_ino;

	/*
     * Bit mask for file-mode information. The _S_IFDIR bit is set if path specifies a directory;
	 * the _S_IFREG bit is set if path specifies an ordinary file or a device. 
	 * User read/write bits are set according to the file's permission mode;
     * user execute bits are set according to the filename extension.
     */
    unsigned short st_mode;

	/* Always 1 on non-NTFS file systems. */
	short st_nlink;

	/*
     * Numeric identifier of user who owns file (UNIX-specific).
     * This field will always be zero on Windows NT systems.
	 * A redirected file is classified as a Windows NT file.
     */
	short st_uid;
	
    /*
     * Numeric identifier of group that owns file (UNIX-specific).
     * This field will always be zero on Windows NT systems.
     * A redirected file is classified as a Windows NT file
     */
	short st_gid;

	/* Drive number of the disk containing the file (same as st_dev) */
	unsigned int st_rdev;

	long st_size;    /* Size of the file in bytes */
	time_t st_atime; /* Time of last access of file */
	time_t st_mtime; /* Time of last modification of file */
	time_t st_ctime; /* Time of creation of file */
};
# define _STAT_T_DEFINED
#endif

/* Encoding of the file mode. */
#define __S_IFMT       0170000         /* These bits determine file type. */

/* File types. */
#define S_IFDIR        0040000         /* Directory. */
#define S_IFCHR        0020000         /* Character device. */
#define S_IFREG        0100000         /* Regular file. */
#define S_IFIFO        0010000         /* FIFO. */

/* Permission bits */
#define	S_ISUID	         04000	       /* Set user ID on execution.  */
#define	S_ISGID	         02000	       /* Set group ID on execution.  */ 
#define S_IREAD        0000400         /* Read permission, owner */
#define S_IWRITE       0000200         /* Write permission, owner */
#define S_IEXEC        0000100         /* Execute/search permission, owner */

/* Macros to test file types masks.*/

#define	__S_ISTYPE(mode, mask)	(((mode) & __S_IFMT) == (mask))

#define	S_ISDIR(mode)	 __S_ISTYPE((mode), S_IFDIR)    /* Test for a directory. */
#define	S_ISCHR(mode)	 __S_ISTYPE((mode), S_IFCHR)    /* Test for a character special file. */
#define	S_ISREG(mode)	 __S_ISTYPE((mode), S_IFREG)    /* Test for a regular file. */
#define S_ISFIFO(mode)	 __S_ISTYPE((mode), S_IFIFO)    /* Test for a pipe or FIFO special file. */


/*
 * File functions declarations.
 */

int wceex_stat(const char *filename, struct stat *buf);
/* XXX - mloskot - int wceex_fstat(int filedes, struct stat *buf); */

int wceex_mkdir(const char *filename);
int wceex_rmdir(const char *filename);


time_t mktime(struct tm *tmbuff);
//time_t time(time_t *timer);
//struct tm * gmtime(const long *timer);
//struct tm *localtime(const long *timer);


#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* #ifndef WCEEX_STAT_H */
#endif // _CE_C_RUNTIME_