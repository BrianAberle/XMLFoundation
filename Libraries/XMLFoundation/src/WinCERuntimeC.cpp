// About 80% of the source in this file is based on a work titled WCELIBCEX and published under this license:


#include "WinCERuntimeC.h"
#include <stdlib.h>
#include <windows.h>

int errno = 0;



/*
WCELIBCEX - Windows CE C Library Extensions

The source code of the WCELIBCEX library is
licensed under MIT License:

http://opensource.org/licenses/mit-license.php

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation 
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom 
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH
THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/



/*
 * $Id: wce_rmdir.c,v 1.3 2006/04/09 16:48:18 mloskot Exp $
 *
 * Defines rmdir() function.
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


//#include <time.h>
time_t wceex_local_to_time_r(int year, int mon, int day, int hour, int min, int sec)
{
    struct tm tmbuff;// = { 0 };

    tmbuff.tm_year = year;
    tmbuff.tm_mon = mon;
    tmbuff.tm_mday = day;
    tmbuff.tm_hour = hour;
    tmbuff.tm_min = min;
    tmbuff.tm_sec = sec;
    tmbuff.tm_isdst = 0;
    tmbuff.tm_wday = 0;
    tmbuff.tm_yday = 0;

    /* Convert tm struct to time_tUTC */
    return mktime(&tmbuff);
}

time_t wceex_local_to_time(const SYSTEMTIME *st)
{
    if (st == NULL)
    {
        return (time_t)-1;
    }
 
    return wceex_local_to_time_r(st->wYear - TM_YEAR_BASE,
                               st->wMonth - 1,
                               st->wDay, st->wHour,
                               st->wMinute,
                               st->wSecond);
}




/*******************************************************************************
* wceex_rmdir - Remove empty directory from filesystem.
*
* Description:
*
*   The rmdir() function shall remove a directory whose name is given by path.
*   The directory shall be removed only if it is an empty directory.
*   Internally, mkdir() function wraps RemoveDirectory call from 
*   Windows CE API.
*
* Return:
*
*   Upon successful completion, rmdir() shall return 0.
*   Otherwise, -1 shall be returned. If -1 is returned, the named directory
*   shall not be changed.
*
*   XXX - mloskot - errno is not set - todo.
*       
* Reference:
*
*   IEEE 1003.1, 2004 Edition
*
*******************************************************************************/
int rmdir(const char *filename)
{
    int res;    
    size_t len;
    wchar_t *widestr;

    /* Covert filename buffer to Unicode. */
	len = MultiByteToWideChar (CP_ACP, 0, filename, -1, NULL, 0) ;
	widestr = (wchar_t*)malloc(sizeof(wchar_t) * len);
	MultiByteToWideChar( CP_ACP, 0, filename, -1, widestr, len);
	
	/* Delete file using Win32 CE API call */
	res = RemoveDirectory(widestr);
	
	/* Free wide-char string */
	free(widestr);

    /* XXX - Consider following recommendations: */
    /* XXX - mloskot - update the st_ctime and st_mtime fields of the parent directory. */
    /* XXX - mloskot - set errno to [EEXIST] or [ENOTEMPTY] if function failed. */

    if (res)
	    return 0; /* success */
    else
        return -1;

}


int unlink(const char *filename)
{
    int res;
    int len;
    wchar_t* pWideStr;

    /* Covert filename buffer to Unicode. */
    len = MultiByteToWideChar(CP_ACP, 0, filename, -1, NULL, 0) ;
    pWideStr = (wchar_t*)malloc(sizeof(wchar_t) * len);
	
    MultiByteToWideChar(CP_ACP, 0, filename, -1, pWideStr, len);
	
    /* Delete file using Win32 CE API call */
    res = DeleteFile(pWideStr);
	
    /* Free wide-char string */
    free(pWideStr);

    if (res)
        return 0; /* success */
    else
    {
        errno = GetLastError();
        return -1;
    }

}

int mkdir( const char* filename )
{
    int res;    
    size_t len;
    wchar_t *widestr;

    /* Covert filename buffer to Unicode. */
    len = MultiByteToWideChar (CP_ACP, 0, filename, -1, NULL, 0) ;
	widestr  = (wchar_t*)malloc(sizeof(wchar_t) * len);

    MultiByteToWideChar( CP_ACP, 0, filename, -1, widestr, len);
	
    /* Delete file using Win32 CE API call */
    res = CreateDirectory(widestr, NULL);
	
    /* Free wide-char string */
    free(widestr);

    if (res)
	    return 0; /* success */
    else
    {
        errno = GetLastError();
        return -1;
    }
}


int stat(const char* filename, struct stat *buffer)
{
    HANDLE findhandle;
    WIN32_FIND_DATA findbuf;
    wchar_t pathWCE[MAX_PATH];

    //Don't allow wildcards to be interpreted by system
    if(strpbrk(filename, "?*"))
        //if(wcspbrk(path, L"?*"))
    {
        //errno = ENOENT;
        return(-1);
    }

    //search file/dir
    mbstowcs(pathWCE, filename, strlen(filename) + 1);
    findhandle = FindFirstFile(pathWCE, &findbuf);
    if(findhandle == INVALID_HANDLE_VALUE)
    {
        //is root
        if(_stricmp(filename, ".\\")==0)
        {
            findbuf.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;

            //dummy values
            findbuf.nFileSizeHigh = 0;
            findbuf.nFileSizeLow = 0;
            findbuf.cFileName[0] = '\0';

			buffer->st_mtime = wceex_local_to_time_r(1980 - TM_YEAR_BASE, 0, 1, 0, 0, 0);
            buffer->st_atime = buffer->st_mtime;
            buffer->st_ctime = buffer->st_mtime;
        }

        //treat as an error
        else
        {
            //errno = ENOENT;
            return(-1);
        }
    }
    else
    {
        /* File is found*/

        SYSTEMTIME SystemTime;
        FILETIME LocalFTime;

        //Time of last modification
        if(!FileTimeToLocalFileTime( &findbuf.ftLastWriteTime, &LocalFTime) ||
            !FileTimeToSystemTime(&LocalFTime, &SystemTime))
        {
            //errno = ::GetLastError();
            FindClose( findhandle );
            return( -1 );
        }

        buffer->st_mtime = wceex_local_to_time(&SystemTime);

        //Time od last access of file
        if(findbuf.ftLastAccessTime.dwLowDateTime || findbuf.ftLastAccessTime.dwHighDateTime)
        {
            if(!FileTimeToLocalFileTime(&findbuf.ftLastAccessTime, &LocalFTime) ||
                !FileTimeToSystemTime(&LocalFTime, &SystemTime))
            {
                //errno = ::GetLastError();
                FindClose( findhandle );
                return( -1 );
            }
            buffer->st_atime = wceex_local_to_time(&SystemTime);
        }
        else
        {
            buffer->st_atime = buffer->st_mtime;
        }


        //Time of creation of file
        if(findbuf.ftCreationTime.dwLowDateTime || findbuf.ftCreationTime.dwHighDateTime)
        {
            if(!FileTimeToLocalFileTime(&findbuf.ftCreationTime, &LocalFTime) ||
                !FileTimeToSystemTime(&LocalFTime, &SystemTime))
            {
                //errno = ::GetLastError();
                FindClose( findhandle );
                return( -1 );
            }
            buffer->st_ctime = wceex_local_to_time(&SystemTime);
        }
        else
        {
            buffer->st_ctime = buffer->st_mtime;
        }

        //close handle
        FindClose(findhandle);
    }

    //file mode
//    buffer->st_mode = __wceex_get_file_mode(filename, findbuf.dwFileAttributes);

    //file size
    buffer->st_size = findbuf.nFileSizeLow;

    //drive letter 0
    buffer->st_rdev = buffer->st_dev = 0;

    //set the common fields
    buffer->st_gid = 0;
    buffer->st_ino = 0;
    buffer->st_uid = 0;

    //1 dla nlink
    buffer->st_nlink = 1;


    return 0;
}

#define ENOENT          2
#define EACCES          13

int wceex_waccess( const wchar_t *path, int mode )
{
    SHFILEINFO fi;
    if( !SHGetFileInfo( path, 0, &fi, sizeof(fi), SHGFI_ATTRIBUTES ) )
    {
        errno = ENOENT;
        return -1;
    }
    // existence ?
    if( mode == 0 )
        return 0;
    // write permission ?
    if( mode & 2 )
    {
        if( fi.dwAttributes & SFGAO_READONLY )
        {
            errno = EACCES;
            return -1;
        }
    }
    return 0;
}

/*******************************************************************************
* wceex_access - Determine file-access permission
*
* Description:
*
* Return: 
*   0 if the file has the given mode
*   –1 if the named file does not exist or is not accessible in the given mode 
*   and errno set as :
*       EACCES file's permission setting does not allow specified access
*       ENOENT filename or path not found
*       
* Reference:
* 
*******************************************************************************/

int access( const char *path, int mode )
{
    wchar_t wpath[_MAX_PATH];
	if( !MultiByteToWideChar( CP_ACP, 0, path, -1, wpath, _MAX_PATH ) )
    {
        errno = ENOENT;
        return -1;
    }
    return wceex_waccess( wpath, mode );
}

char* getenv(const char* varname)
{
    return NULL;
}



// Windows CE doesn't define _beginthreadex 
 
 struct ThreadProxyData 
 { 
	typedef unsigned (__stdcall* func)(void*); 
	func start_address_; 
	void* arglist_; 
	ThreadProxyData(func start_address,void* arglist) : start_address_(start_address), arglist_(arglist) {} 
 }; 
 
 DWORD WINAPI ThreadProxy(LPVOID args) 
 { 
	ThreadProxyData* data=reinterpret_cast<ThreadProxyData*>(args); 
	DWORD ret=data->start_address_(data->arglist_); 
	delete data; 
	return ret; 
 } 
 
 
 void *_beginthreadex(void* security, unsigned stack_size, unsigned (__stdcall* start_address)(void*),  void* arglist, unsigned initflag, unsigned* thrdaddr) 
 { 
	DWORD threadID; 
	HANDLE hthread=CreateThread(static_cast<LPSECURITY_ATTRIBUTES>(security),stack_size,ThreadProxy, 
	new ThreadProxyData(start_address,arglist),initflag,&threadID); 
	if (hthread!=0) 
		*thrdaddr=threadID; 
	return hthread; 
 }

 int rename(const char *oldfile, const char *newfile)
 {
    int res;    
    size_t lenold;
    size_t lennew;
    wchar_t *wsold;
    wchar_t *wsnew;
    
    /* Covert filename buffer to Unicode. */

    /* Old filename */
    lenold = MultiByteToWideChar (CP_ACP, 0, oldfile, -1, NULL, 0) ;
    wsold = (wchar_t*)malloc(sizeof(wchar_t) * lenold);
    MultiByteToWideChar( CP_ACP, 0, oldfile, -1, wsold, lenold);
    
    /* New filename */
    lennew = MultiByteToWideChar (CP_ACP, 0, newfile, -1, NULL, 0) ;
    wsnew = (wchar_t*)malloc(sizeof(wchar_t) * lennew);
    MultiByteToWideChar(CP_ACP, 0, newfile, -1, wsnew, lennew);

    /* Delete file using Win32 CE API call */
    res = MoveFile(wsold, wsnew);
    
    /* Free wide-char string */
    free(wsold);
    free(wsnew);
    
    if (res)
        return 0; /* success */
    else
        return -1;

}

#define MONTHS_NUMBER 12
#define EPOCH_YEAR 1970
#define IS_LEAP_YEAR(year) (((year) % 4) == 0 && (((year) % 100) != 0 || ((year) % 400) == 0))
static const int MONTHDAYS[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

static time_t __wceex_mktime_internal(struct tm *tmbuff, time_t _loctime_offset)
{
    time_t tres;
    int doy;
    int i;

    /* We do allow some ill-formed dates, but we don't do anything special
    with them and our callers really shouldn't pass them to us.  Do
    explicitly disallow the ones that would cause invalid array accesses
    or other algorithm problems. */
    if (tmbuff->tm_mon < 0 || tmbuff->tm_mon > 11 || tmbuff->tm_year < (EPOCH_YEAR - TM_YEAR_BASE))
    {
        return (time_t) -1;
    }

    /* Convert calender time to a time_t value. */
    tres = 0;

    /* Sum total amount of days from the Epoch with respect to leap years. */
    for (i = EPOCH_YEAR; i < tmbuff->tm_year + TM_YEAR_BASE; i++)
    {
        tres += 365 + IS_LEAP_YEAR(i);
    }

    /* Add days of months before current month. */
    doy = 0;
    for (i = 0; i < tmbuff->tm_mon; i++)
    {
        doy += MONTHDAYS[i];
    }
    tres += doy;
    
    /* Day of year */
    tmbuff->tm_yday = doy + tmbuff->tm_mday;

    if (tmbuff->tm_mon > 1 && IS_LEAP_YEAR(tmbuff->tm_year + TM_YEAR_BASE))
    {
        tres++;
    }
    
    /* Add days of current month and convert to total to hours. */
    tres = 24 * (tres + tmbuff->tm_mday - 1) + tmbuff->tm_hour;

    /* Add minutes part and convert total to minutes. */
    tres = 60 * tres + tmbuff->tm_min;

    /* Add seconds part and convert total to seconds. */
    tres = 60 * tres + tmbuff->tm_sec;
    
    /* For offset > 0 adjust time value for timezone
    given as local to UTC time difference in seconds). */
    tres += _loctime_offset;
    
    return tres;
}



 time_t wceex_gmmktime(struct tm *tmbuff)
{
    return __wceex_mktime_internal(tmbuff, 0);
}

time_t time(time_t *timer)
//long time(long *timer)
{
    time_t t;
    struct tm tmbuff;
    SYSTEMTIME st;

    /* Retrive current system date time as UTC */
    GetSystemTime(&st);

    /* Build tm struct based on SYSTEMTIME values */

    /* Date values */
    tmbuff.tm_year = st.wYear - TM_YEAR_BASE;
    tmbuff.tm_mon = st.wMonth - 1;      /* wMonth value 1-12 */
    tmbuff.tm_mday = st.wDay;

    /* Time values */
    tmbuff.tm_hour = st.wHour;
    tmbuff.tm_min = st.wMinute;
    tmbuff.tm_sec = st.wSecond;
    tmbuff.tm_isdst = 0;    /* Always 0 for UTC time. */
    tmbuff.tm_wday = st.wDayOfWeek;
    tmbuff.tm_yday = 0;     /* Value is set by wceex_gmmktime */

    /* Convert tm struct to time_tUTC */
    t = wceex_gmmktime(&tmbuff);

    /* Assign time value. */
    if (timer != NULL)
    {
        *timer = t;
    }

	return t;
}


#define TM_THURSDAY	4
#define MINS_PER_HOUR	60
#define HOURS_PER_DAY	24
#define MONS_PER_YEAR	12
#define DAYS_PER_NYEAR	365
#define DAYS_PER_LYEAR	366
#define SECS_PER_MIN	60
#define SECS_PER_HOUR	(SECS_PER_MIN * MINS_PER_HOUR)
#define SECS_PER_DAY	((long) SECS_PER_HOUR * HOURS_PER_DAY)
#define EPOCH_WDAY	TM_THURSDAY
#define DAYS_PER_WEEK	7
#define isleap(y) (((y) % 4) == 0 && ((y) % 100) != 0 || ((y) % 400) == 0)

 static int	mon_lengths[2][MONS_PER_YEAR] =
{
    31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31,
    31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

static int	year_lengths[2] = { DAYS_PER_NYEAR, DAYS_PER_LYEAR };


struct tm * __wceex_offtime(const time_t *timer, long tzoffset)
{
    register struct tm *tmp;
    register long		days;
    register long		rem;
    register int		y;
    register int		yleap;
    register int       *ip;
    static struct tm    tm;

    tmp = &tm;
    days = *timer / SECS_PER_DAY;
    rem = *timer % SECS_PER_DAY;
    rem += tzoffset;
    while (rem < 0)
    {
        rem += SECS_PER_DAY;
        --days;
    }
    
    while (rem >= SECS_PER_DAY)
    {
        rem -= SECS_PER_DAY;
        ++days;
    }
    tmp->tm_hour = (int) (rem / SECS_PER_HOUR);

    rem = rem % SECS_PER_HOUR;
    tmp->tm_min = (int) (rem / SECS_PER_MIN);
    tmp->tm_sec = (int) (rem % SECS_PER_MIN);
    tmp->tm_wday = (int) ((EPOCH_WDAY + days) % DAYS_PER_WEEK);
    
    if (tmp->tm_wday < 0)
        tmp->tm_wday += DAYS_PER_WEEK;
    
    y = EPOCH_YEAR;
    
    if (days >= 0)
    {
        for ( ; ; )
        {
            yleap = isleap(y);
            if (days < (long) year_lengths[yleap])
                break;

            ++y;
            days = days - (long) year_lengths[yleap];
        }
    }
    else
    {
        do
        {
            --y;
            yleap = isleap(y);
            days = days + (long) year_lengths[yleap];
        } while (days < 0);
    }

    tmp->tm_year = y - TM_YEAR_BASE;
    tmp->tm_yday = (int) days;
    ip = mon_lengths[yleap];

    for (tmp->tm_mon = 0; days >= (long) ip[tmp->tm_mon]; ++(tmp->tm_mon))
    {
        days = days - (long) ip[tmp->tm_mon];
    }

    tmp->tm_mday = (int) (days + 1);
    tmp->tm_isdst = 0;

    return tmp;
}


struct tm * gmtime(const long *timer)
{
    register struct tm *tmp;

    tmp = __wceex_offtime(timer, 0L);

    return tmp;
}


struct tm * localtime(const long *timer)
{
    register struct tm *tmp;

    long tzoffset;
    TIME_ZONE_INFORMATION tzi;
    
    // Retrive timezone offset in seconds
    tzoffset = 0;
	if (GetTimeZoneInformation(&tzi) != 0xFFFFFFFF)
	{
		tzoffset += (tzi.Bias * 60);
		if (tzi.StandardDate.wMonth != 0)
        {
			tzoffset += (tzi.StandardBias * 60);
        }
	}
    
    tzoffset *= -1;
    tmp = __wceex_offtime(timer, tzoffset);

    return tmp;
}

time_t mktime(struct tm *tmbuff)
{
    time_t offset;
    TIME_ZONE_INFORMATION tzi;
    
    offset = 0;

    // Retrive timezone offset in seconds
	if (GetTimeZoneInformation(&tzi) != 0xFFFFFFFF)
	{
		offset += (tzi.Bias * 60);
		if (tzi.StandardDate.wMonth != 0)
        {
			offset += (tzi.StandardBias * 60);
        }
	}

    return __wceex_mktime_internal(tmbuff, offset);
}

long getTimeMS()
{
	return GetTickCount();
}




///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//#include <time.h>
#include <string.h>
#include <locale.h>
#define _ANSI_ARGS_(x)	x

#define TM_YEAR_BASE   1900
#define IsLeapYear(x)   ((x % 4 == 0) && (x % 100 != 0 || x % 400 == 0))

typedef struct {
    const char *abday[7];
    const char *day[7];
    const char *abmon[12];
    const char *mon[12];
    const char *am_pm[2];
    const char *d_t_fmt;
    const char *d_fmt;
    const char *t_fmt;
    const char *t_fmt_ampm;
} _TimeLocale;

/*
 * This is the C locale default.  On Windows, if we wanted to make this
 * localized, we would use GetLocaleInfo to get the correct values.
 * It may be acceptable to do localization of month/day names, as the
 * numerical values would be considered the locale-independent versions.
 */
static const _TimeLocale _DefaultTimeLocale = 
{
    {
	"Sun","Mon","Tue","Wed","Thu","Fri","Sat",
    },
    {
	"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday",
	"Friday", "Saturday"
    },
    {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    },
    {
	"January", "February", "March", "April", "May", "June", "July",
	"August", "September", "October", "November", "December"
    },
    {
	"AM", "PM"
    },
    "%a %b %d %H:%M:%S %Y",
    "%m/%d/%y",
    "%H:%M:%S",
    "%I:%M:%S %p"
};

static const _TimeLocale *_CurrentTimeLocale = &_DefaultTimeLocale;

static size_t gsize;
static char *pt;
static int _add(unsigned short *str);
static int		 _add _ANSI_ARGS_((const char* str));
static int		_conv _ANSI_ARGS_((int n, int digits, int pad));
static int		_secs _ANSI_ARGS_((const struct tm *t));
static size_t		_fmt _ANSI_ARGS_((const char *format,  const struct tm *t));


size_t strftime(char *s, size_t maxsize, const char *format, const struct tm *t)
{
    if (format[0] == '%' && format[1] == 'Q') {
	/* Format as a stardate */
	sprintf(s, "Stardate %2d%03d.%01d",
		(((t->tm_year + TM_YEAR_BASE) + 377) - 2323),
		(((t->tm_yday + 1) * 1000) /
			(365 + IsLeapYear((t->tm_year + TM_YEAR_BASE)))),
		(((t->tm_hour * 60) + t->tm_min)/144));
	return(strlen(s));
    }

    pt = s;
    if ((gsize = maxsize) < 1)
	return(0);
    if (_fmt(format, t)) {
	*pt = '\0';
	return(maxsize - gsize);
    }
    return(0);
}

#define SUN_WEEK(t)	(((t)->tm_yday + 7 - \
				((t)->tm_wday)) / 7)
#define MON_WEEK(t)	(((t)->tm_yday + 7 - \
				((t)->tm_wday ? (t)->tm_wday - 1 : 6)) / 7)

static size_t _fmt(const char *format, const struct tm *t)
{
#ifdef WIN32
#define BUF_SIZ 256
    TCHAR buf[BUF_SIZ];
    SYSTEMTIME syst = {
	t->tm_year + 1900,
	t->tm_mon + 1,
	t->tm_wday,
	t->tm_mday,
	t->tm_hour,
	t->tm_min,
	t->tm_sec,
	0,
    };
#endif
    for (; *format; ++format) {
	if (*format == '%') {
	    ++format;
	    if (*format == 'E') {
				/* Alternate Era */
		++format;
	    } else if (*format == 'O') {
				/* Alternate numeric symbols */
		++format;
	    }
	    switch(*format) {
		case '\0':
		    --format;
		    break;
		case 'A':
		    if (t->tm_wday < 0 || t->tm_wday > 6)
			return(0);
		    if (!_add(_CurrentTimeLocale->day[t->tm_wday]))
			return(0);
		    continue;
		case 'a':
		    if (t->tm_wday < 0 || t->tm_wday > 6)
			return(0);
		    if (!_add(_CurrentTimeLocale->abday[t->tm_wday]))
			return(0);
		    continue;
		case 'B':
		    if (t->tm_mon < 0 || t->tm_mon > 11)
			return(0);
		    if (!_add(_CurrentTimeLocale->mon[t->tm_mon]))
			return(0);
		    continue;
		case 'b':
		case 'h':
		    if (t->tm_mon < 0 || t->tm_mon > 11)
			return(0);
		    if (!_add(_CurrentTimeLocale->abmon[t->tm_mon]))
			return(0);
		    continue;
		case 'C':
		    if (!_conv((t->tm_year + TM_YEAR_BASE) / 100,
			    2, '0'))
			return(0);
		    continue;
		case 'D':
		    if (!_fmt("%m/%d/%y", t))
			return(0);
		    continue;
		case 'd':
		    if (!_conv(t->tm_mday, 2, '0'))
			return(0);
		    continue;
		case 'e':
		    if (!_conv(t->tm_mday, 2, ' '))
			return(0);
		    continue;
		case 'H':
		    if (!_conv(t->tm_hour, 2, '0'))
			return(0);
		    continue;
		case 'I':
		    if (!_conv(t->tm_hour % 12 ?
			    t->tm_hour % 12 : 12, 2, '0'))
			return(0);
		    continue;
		case 'j':
		    if (!_conv(t->tm_yday + 1, 3, '0'))
			return(0);
		    continue;
		case 'k':
		    if (!_conv(t->tm_hour, 2, ' '))
			return(0);
		    continue;
		case 'l':
		    if (!_conv(t->tm_hour % 12 ?
			    t->tm_hour % 12: 12, 2, ' '))
			return(0);
		    continue;
		case 'M':
		    if (!_conv(t->tm_min, 2, '0'))
			return(0);
		    continue;
		case 'm':
		    if (!_conv(t->tm_mon + 1, 2, '0'))
			return(0);
		    continue;
		case 'n':
		    if (!_add("\n"))
			return(0);
		    continue;
		case 'p':
		    if (!_add(_CurrentTimeLocale->am_pm[t->tm_hour >= 12]))
			return(0);
		    continue;
		case 'R':
		    if (!_fmt("%H:%M", t))
			return(0);
		    continue;
		case 'r':
		    if (!_fmt(_CurrentTimeLocale->t_fmt_ampm, t))
			return(0);
		    continue;
		case 'S':
		    if (!_conv(t->tm_sec, 2, '0'))
			return(0);
		    continue;
		case 's':
		    if (!_secs(t))
			return(0);
		    continue;
		case 'T':
		    if (!_fmt("%H:%M:%S", t))
			return(0);
		    continue;
		case 't':
		    if (!_add("\t"))
			return(0);
		    continue;
		case 'U':
		    if (!_conv(SUN_WEEK(t), 2, '0'))
			return(0);
		    continue;
		case 'u':
		    if (!_conv(t->tm_wday ? t->tm_wday : 7, 1, '0'))
			return(0);
		    continue;
		case 'V':
		{
				/* ISO 8601 Week Of Year:
				   If the week (Monday - Sunday) containing
				   January 1 has four or more days in the new 
				   year, then it is week 1; otherwise it is 
				   week 53 of the previous year and the next
				   week is week one. */
				 
		    int week = MON_WEEK(t);

		    int days = (((t)->tm_yday + 7 - \
			    ((t)->tm_wday ? (t)->tm_wday - 1 : 6)) % 7);


		    if (days >= 4) {
			week++;
		    } else if (week == 0) {
			week = 53;
		    }

		    if (!_conv(week, 2, '0'))
			return(0);
		    continue;
		}
		case 'W':
		    if (!_conv(MON_WEEK(t), 2, '0'))
			return(0);
		    continue;
		case 'w':
		    if (!_conv(t->tm_wday, 1, '0'))
			return(0);
		    continue;
#ifdef WIN32
		/*
		 * To properly handle the localized time routines on Windows,
		 * we must make use of the special localized calls.
		 */
		case 'c':
		    if (!GetDateFormat(LOCALE_USER_DEFAULT, DATE_LONGDATE,
			    &syst, NULL, buf, BUF_SIZ) || !_add(buf)
			    || !_add(" ")) {
			return(0);
		    }
		    /*
		     * %c is created with LONGDATE + " " + TIME on Windows,
		     * so continue to %X case here.
		     */
		case 'X':
		    if (!GetTimeFormat(LOCALE_USER_DEFAULT, 0,
			    &syst, NULL, buf, BUF_SIZ) || !_add(buf)) {
			return(0);
		    }
		    continue;
		case 'x':
		    if (!GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE,
			    &syst, NULL, buf, BUF_SIZ) || !_add(buf)) {
			return(0);
		    }
		    continue;
#else
		case 'c':
		    if (!_fmt(_CurrentTimeLocale->d_t_fmt, t))
			return(0);
		    continue;
		case 'x':
		    if (!_fmt(_CurrentTimeLocale->d_fmt, t))
			return(0);
		    continue;
		case 'X':
		    if (!_fmt(_CurrentTimeLocale->t_fmt, t))
			return(0);
		    continue;
#endif
		case 'y':
		    if (!_conv((t->tm_year + TM_YEAR_BASE) % 100,
			    2, '0'))
			return(0);
		    continue;
		case 'Y':
		    if (!_conv((t->tm_year + TM_YEAR_BASE), 4, '0'))
			return(0);
		    continue;
		case 'Z': 
			{
		    char *name = "GMT";
//		    char *name = (isGMT ? "GMT" : TclpGetTZName(t->tm_isdst));
		    if (name && !_add(name)) 
			{
				return 0;
		    }
		    continue;
		}
		case '%':
		    /*
		     * X311J/88-090 (4.12.3.5): if conversion char is
		     * undefined, behavior is undefined.  Print out the
		     * character itself as printf(3) does.
		     */
		default:
		    break;
	    }
	}
	if (!gsize--)
	    return(0);
	*pt++ = *format;
    }
    return(gsize);
}

static int _secs(const struct tm *t)
{
    static char buf[15];
    register time_t s;
    register char *p;
    struct tm tmp;

    /* Make a copy, mktime(3) modifies the tm struct. */
    tmp = *t;
    s = mktime(&tmp);
    for (p = buf + sizeof(buf) - 2; s > 0 && p > buf; s /= 10)
	*p-- = (char)(s % 10 + '0');
    return(_add(++p));
}
static int _conv(int n, int digits, int pad)
{
    static char buf[10];
    register char *p;

    for (p = buf + sizeof(buf) - 2; n > 0 && p > buf; n /= 10, --digits)
	*p-- = (char)(n % 10 + '0');
    while (p > buf && digits-- > 0)
	*p-- = (char) pad;
    return(_add(++p));
}

static int _add(unsigned short *str)
{
    for (;; ++pt, --gsize) 
	{
	if (!gsize)
	    return(0);
	if (!(*pt = (char)*str++))
	    return(1);
    }
}

static int _add(const char *str)
{
    for (;; ++pt, --gsize) 
	{
	if (!gsize)
	    return(0);
	if (!(*pt = *str++))
	    return(1);
    }
}




