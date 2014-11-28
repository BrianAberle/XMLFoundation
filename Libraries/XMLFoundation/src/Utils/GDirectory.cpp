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
#include "GlobalInclude.h"
static char SOURCE_FILE[] = __FILE__;

#ifdef WINCE
	#include  "WinCERuntimeC.h"
#endif
#include "GDirectory.h"
#include "GString.h"
#include "GException.h"
#include <string.h>			// for: strlen() strpbrk()

#ifndef __WINPHONE
 #ifndef ETIMEDOUT
  #define ETIMEDOUT               WSAETIMEDOUT
 #endif
#endif


#if defined _WIN32
	#include <wtypes.h>		
	#include <tchar.h>
	// FindFirstFile & FindNextFile
	#include "Winbase.h"
	#include <windows.h>
	#ifndef WINCE
		#include <sys/stat.h>
		#include <io.h>
		#include <direct.h> // for: mkdir
	#endif
	#ifdef __WINPHONE
		#include <errno.h>
	#endif
#else
	// linux, Solaris, HPUX, AIX
	#include <unistd.h>
	#include <dirent.h>
	#include <sys/stat.h>
	#include <errno.h>
#endif

#if defined _LINUX 
	// opendir & readdir & closedir
	#include <getopt.h>
	#include <pwd.h>
#elif defined __sun || defined _HPUX
	// opendir & readdir & closedir
	#include <sys/types.h>
#endif

#ifdef WINCE

#define TIME_ZONE_ID_INVALID 0xFFFFFFFF

static int _DayLightCompareDate( const LPSYSTEMTIME date, const LPSYSTEMTIME compareDate)
{
    int limit_day;

    if (compareDate->wYear != 0)
    {
        if (date->wMonth < compareDate->wMonth)
            return -1; /* We are in a year before the date limit. */

        if (date->wMonth > compareDate->wMonth)
            return 1; /* We are in a year after the date limit. */
    }

    if (date->wMonth < compareDate->wMonth)
        return -1; /* We are in a month before the date limit. */

    if (date->wMonth > compareDate->wMonth)
        return 1; /* We are in a month after the date limit. */

    if (compareDate->wDayOfWeek <= 6)
    {
       SYSTEMTIME tmp;
       FILETIME tmp_ft;

       /* compareDate->wDay is interpreted as number of the week in the month. */
       /* 5 means: the last week in the month */
       int weekofmonth = compareDate->wDay;

         /* calculate day of week for the first day in the month */
        memcpy(&tmp, date, sizeof(SYSTEMTIME));
        tmp.wDay = 1;
        tmp.wDayOfWeek = -1;

        if (weekofmonth == 5)
       {
             /* Go to the beginning of the next month. */
            if (++tmp.wMonth > 12)
            {
                tmp.wMonth = 1;
                ++tmp.wYear;
            }
        }

        if (!SystemTimeToFileTime(&tmp, &tmp_ft))
            return -2;

        if (weekofmonth == 5)
        {
          __int64 t, one_day;

          t = tmp_ft.dwHighDateTime;
          t <<= 32;
          t += (UINT)tmp_ft.dwLowDateTime;

          /* substract one day */
          one_day = 24*60*60;
          one_day *= 10000000;
          t -= one_day;

          tmp_ft.dwLowDateTime  = (UINT)t;
          tmp_ft.dwHighDateTime = (UINT)(t >> 32);
        }

        if (!FileTimeToSystemTime(&tmp_ft, &tmp))
            return -2;

       if (weekofmonth == 5)
       {
          /* calculate the last matching day of the week in this month */
          int dif = tmp.wDayOfWeek - compareDate->wDayOfWeek;
          if (dif < 0)
             dif += 7;

          limit_day = tmp.wDay - dif;
       }
       else
       {
         /* calulcate the matching day of the week in the given week */
          int dif = compareDate->wDayOfWeek - tmp.wDayOfWeek;
          if (dif < 0)
             dif += 7;

          limit_day = tmp.wDay + 7*weekofmonth + dif;
       }
    }
    else
    {
      limit_day = compareDate->wDay;
   }

    if (date->wDay < limit_day)
        return -1;

    if (date->wDay > limit_day)
        return 1;

    return 0;   /* date is equal to the date limit. */
}



static BOOL _GetTimezoneBias( const LPTIME_ZONE_INFORMATION lpTimeZoneInformation,  LPSYSTEMTIME  lpSystemTime, LONG* pBias)
{
    int ret;
    BOOL beforedaylightsaving, afterdaylightsaving;
    BOOL daylightsaving = FALSE;
    LONG bias = lpTimeZoneInformation->Bias;

    if (lpTimeZoneInformation->DaylightDate.wMonth != 0)
    {
        if (lpTimeZoneInformation->StandardDate.wMonth == 0 ||
            lpTimeZoneInformation->StandardDate.wDay<1 ||
            lpTimeZoneInformation->StandardDate.wDay>5 ||
            lpTimeZoneInformation->DaylightDate.wDay<1 ||
            lpTimeZoneInformation->DaylightDate.wDay>5)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

         /* check for daylight saving */
       ret = _DayLightCompareDate(lpSystemTime, &lpTimeZoneInformation->StandardDate);
       if (ret == -2)
          return -1;

        beforedaylightsaving = ret < 0;

       _DayLightCompareDate(lpSystemTime, &lpTimeZoneInformation->DaylightDate);
       if (ret == -2)
          return -1;

       afterdaylightsaving = ret >= 0;

        if (!beforedaylightsaving && !afterdaylightsaving)
            daylightsaving = TRUE;
    }

   if (daylightsaving)
        bias += lpTimeZoneInformation->DaylightBias;
    else if (lpTimeZoneInformation->StandardDate.wMonth != 0)
        bias += lpTimeZoneInformation->StandardBias;

    *pBias = bias;

    return TRUE;
}



BOOL WINAPI SystemTimeToTzSpecificLocalTime(   LPTIME_ZONE_INFORMATION lpTimeZoneInformation, LPSYSTEMTIME lpUniversalTime, LPSYSTEMTIME lpLocalTime)
{
    FILETIME ft;
    LONG lBias;
    __int64 t, bias;
    TIME_ZONE_INFORMATION tzinfo;

    if (lpTimeZoneInformation != NULL)
    {
        memcpy(&tzinfo, lpTimeZoneInformation, sizeof(TIME_ZONE_INFORMATION));
    }
    else
    {
        if (GetTimeZoneInformation(&tzinfo) == TIME_ZONE_ID_INVALID)
            return FALSE;
    }

    if (!SystemTimeToFileTime(lpUniversalTime, &ft))
        return FALSE;

    t = ft.dwHighDateTime;
    t <<= 32;
    t += (UINT)ft.dwLowDateTime;

    if (!_GetTimezoneBias(&tzinfo, lpUniversalTime, &lBias))
        return FALSE;

    bias = lBias * 600000000; /* 60 seconds per minute, 100000 [100-nanoseconds-ticks] per second */
    t += bias;

    ft.dwLowDateTime  = (UINT)t;
    ft.dwHighDateTime = (UINT)(t >> 32);

    return FileTimeToSystemTime(&ft, lpLocalTime);
}
#endif


// return the last leaf from a fully qualified path (file or directory)
const char *GDirectory::LastLeaf(const char *pzFullPath, char chSlash/*= 0*/)
{
	static GString strReturnValue;
	strReturnValue = "";
	if (pzFullPath && pzFullPath[0])
	{
		strReturnValue = pzFullPath;
		int nLen = (int)strlen(pzFullPath);
		if (chSlash)
		{
			if (pzFullPath[nLen - 1] == chSlash)
				nLen--;

		}
		else if ( pzFullPath[nLen - 1] == '\\' ||  pzFullPath[nLen - 1] == '/')
		{
			nLen--; // if the input value is "/etc/bin/" start searching behind the last '/'
					// so that the return leaf value is "bin"
		}

		strReturnValue = pzFullPath;
		for(int i = nLen-1; i > -1; i-- )
		{
			if (chSlash)
			{
				if (pzFullPath[i] == chSlash)
				{
					strReturnValue = &pzFullPath[i+1];
					break;
				}
			}
			else if (pzFullPath[i] == '\\' || pzFullPath[i] == '/')	
			{
				strReturnValue = &pzFullPath[i+1];
				break;
			}
		}
	}
	return strReturnValue;
}
/*
#ifdef _WIN32
BOOL GetFileSizeExWIN32(HANDLE hFile, PLARGE_INTEGER lpFileSize)
{
	typedef BOOL (WINAPI* lpfnGetFileSizeEx) (HANDLE hFile, PLARGE_INTEGER lpFileSize);
	BOOL bRet = FALSE;
	static HMODULE hModule = ::LoadLibraryA("kernel32.DLL");
	if(NULL != hModule)
	{
		static lpfnGetFileSizeEx lpfn  = (lpfnGetFileSizeEx)GetProcAddress(hModule, _T("GetFileSizeEx"));
		if( NULL != lpfn )
		{
			bRet = lpfn(hFile, lpFileSize);
		}
//		::FreeLibrary(hModule);
	}
	return bRet;
}
#endif
*/
__int64 GDirectory::FileSize(const char *pzFile)
{
#ifdef _WIN32
		WIN32_FIND_DATA find;
		GString strSearch( pzFile );

	// In VC 6.0 and older we must use FindFirstFile, not FindFirstFileEx
	#if defined(_MSC_VER) && _MSC_VER <= 1200 
			HANDLE hFindFile = FindFirstFile(strSearch, &find);
		   __int64 nSize = ((__int64)find.nFileSizeHigh << 32 | find.nFileSizeLow);
			FindClose(hFindFile);
			return nSize;
	#else
		// __WINPHONE does not support the older interface, so the newer interface is used in all other _WIN32 builds
		HANDLE hFind = FindFirstFileEx(strSearch, FindExInfoStandard, &find, FindExSearchNameMatch, NULL, 0);
		__int64 nSize = ((__int64)find.nFileSizeHigh << 32 | find.nFileSizeLow);
		FindClose(hFind);
		return nSize;
	#endif


#else
		struct stat sstruct;
		stat(pzFile, &sstruct);
		return sstruct.st_size;
#endif

}

int GDirectory::DirStats(const char *pzDirectory, int bDeep, __int64 *pnTotalBytesAllFiles, __int64 *pnTotalFileCount, __int64 *pnTotalFolderCount)
{
	// recurse into and gather all the files and directories
	GStringList lstDirs;
	GStringList lstFiles;
	GDirectory::RecurseFolder(pzDirectory,&lstDirs,&lstFiles,bDeep);
	GStringIterator it(&lstFiles);
	while (it())
	{
		const char *pF = it++;

		(*pnTotalFileCount)++;

#ifdef _WIN32
		WIN32_FIND_DATA find;
		GString strSearch( pF );

	// In VC 6.0 and older we must use FindFirstFile, not FindFirstFileEx
	#if defined(_MSC_VER) && _MSC_VER <= 1200 
			HANDLE hFindFile = FindFirstFile(strSearch, &find);
		   __int64 nSize = ((__int64)find.nFileSizeHigh << 32 | find.nFileSizeLow);
			FindClose(hFindFile);
			(*pnTotalBytesAllFiles) += nSize;

	#else
		// __WINPHONE does not support the older interface, so the newer interface is used in all other _WIN32 builds
		HANDLE hFind = FindFirstFileEx(strSearch, FindExInfoStandard, &find, FindExSearchNameMatch, NULL, 0);
		__int64 nSize = ((__int64)find.nFileSizeHigh << 32 | find.nFileSizeLow);
		(*pnTotalBytesAllFiles) += nSize;
		FindClose(hFind);
	#endif


#else
		struct stat sstruct;
		stat(pF, &sstruct);
		(*pnTotalBytesAllFiles) += sstruct.st_size;
#endif

	}

	GStringIterator it2(&lstDirs);
	while (it2())
	{
		const char *pD = it2++;
		(*pnTotalFolderCount)++;
		if (bDeep)
		{
			DirStats(pD, 1, pnTotalBytesAllFiles, pnTotalFileCount, pnTotalFolderCount);
		}
	}
	return 1;
}

int GDirectory::RmDirDeep(const char *pzDirectory, GString &strError)
{
	// recurse into and gather all the files and directories
	GStringList lstDirs;
	GStringList lstFiles;
	GDirectory::RecurseFolder(pzDirectory,&lstDirs,&lstFiles);
	int nFailedFileCount = 0;
	int nFailedDirCount = 0;

	// remove all those files
	GStringIterator it(&lstFiles);
	while (it())
	{
		const char *pF = it++;
		if ( unlink( pF ) != 0)	// under all but WinCE - retry on a fail
		{
			// try to grant write permissions before re-try.
#ifndef WINCE
#ifdef _WIN32
			_chmod( pF, _S_IWRITE );
#else
			chmod(	pF, 777 );     
#endif
			if ( unlink( pF ) == 0) // if it worked
			{
				continue;
			}

			
			if (!nFailedFileCount)
				strError = "Error:Failed to delete file(s):\n" ;
			nFailedFileCount++;
			strError += pF;
			strError += "\n";
#endif // if not WinCE
		}
	}


	// then remove the directories
	while (lstDirs.Size())
	{
		const char *pD = lstDirs.RemoveLast();
		if ( rmdir( pD ) != 0)
		{
			// optionally, try to grant write permissions before re-try.
#ifndef WINCE
#ifdef _WIN32
			_chmod( pD, _S_IWRITE );
#else
			chmod(	pD, 777);  
#endif
			if ( rmdir( pD ) == 0) // if it worked
			{
				continue;
			}
			else
			{
				RmDirDeep(pD, strError);
				if ( rmdir( pD ) == 0) // if it worked
				{
					continue;
				}
			}
			if (!nFailedDirCount)
			{
				strError += "Error:Failed to remove folder(s):\n";
			}
			nFailedDirCount++;
			strError += pD;
			strError += "\n";
#endif // if not WinCE
		}
	}


	// if everything was deleted, this will now work.
	if (rmdir(pzDirectory) == 0)
	{
		return 1; // everything was deleted
	}
	
	return 0;// something was locked, or no permissions
}



// given "/usr/bob/file.ext"  create [/usr] and [/usr/bob] if necessary
// if modifications are made here see also: "static void CreatePath" in GString.cpp
void GDirectory::CreatePath(const char *pzPathOnlyOrPathAndFileName, int bPathHasFileName)
{
	GString strLocal(pzPathOnlyOrPathAndFileName);
	char *pzFileAndPath = strLocal.Buf();

	if (!pzFileAndPath || !pzFileAndPath[0])
	{
		return;
	}
	int nLen = (int)strlen(pzFileAndPath);
	for(int i=1;i<nLen+1;i++)
	{
		if (pzFileAndPath[i] == '\\' || pzFileAndPath[i] == '/' || pzFileAndPath[i] == 0)
		{
			if ( bPathHasFileName && pzFileAndPath[i] == 0)
			{
				// if the path includes a filename, we're done.
				break; 
			}

			char ch = pzFileAndPath[i];
			pzFileAndPath[i] = 0;

			int nAttempts = 0;
RETRY_MKDIR:
			nAttempts++;

#ifdef _WIN32
			if (pzFileAndPath[i - 1] != ':')
			{
				int nRslt = mkdir(pzFileAndPath);

				if (nRslt != 0 && nAttempts < 5 && errno == 2)
				{
					goto RETRY_MKDIR;
				}
			}
#else
			mkdir(pzFileAndPath, 777);
#endif			

			pzFileAndPath[i] = ch;
		}
	}
}

// returns 1 if the argument is a directory, 0 if it's a file or a bad path.
int GDirectory::IsDirectory(const char *szPath)
{
	char *szDirPath = (char *)szPath; // cast off the const
	int nDirPathLen = (int)strlen(szDirPath);
	int nRetVal = 0;
	int chSlash = 0;
	if (szDirPath[nDirPathLen-1] == '\\')
	{
		chSlash = 1;
		szDirPath[nDirPathLen-1] = 0;
	}
	else if (szDirPath[nDirPathLen-1] == '/')
	{
		chSlash = 2;
		szDirPath[nDirPathLen-1] = 0;
	}

#ifdef _WIN32


	WIN32_FIND_DATA w32fd;
	GString strDirPath(szDirPath);

	// In VC 6.0 and older we must use FindFirstFile, not FindFirstFileEx
	#if defined(_MSC_VER) && _MSC_VER <= 1200 
	  HANDLE hFind = FindFirstFile( strDirPath, &w32fd );
	#else
	  HANDLE hFind = FindFirstFileEx(strDirPath, FindExInfoStandard, &w32fd, FindExSearchNameMatch, NULL, 0);
	#endif
	if (hFind != INVALID_HANDLE_VALUE) 
	{
		if( w32fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
			nRetVal = 1;
		FindClose( hFind );
	}
#else
	struct stat sstruct;
	int result = stat(szDirPath, &sstruct);
	if (result == 0)
	{
//  note: this code works in Win32 also, however not in Windows CE, so it was changed for a "one windows" solution
//		if ( (sstruct.st_mode & S_IFDIR)  ) // this no longer works under Vista w/o admin
//		{
//			return 1;
//		}
		if ( S_ISDIR(sstruct.st_mode)  )
		{
			nRetVal = 1;
		}
	}
#endif

	// put it back the way it was - it's a const
	if (chSlash == 1)
	{
		strcat(szDirPath,"\\");
	}
	else if (chSlash == 2)
	{
		strcat(szDirPath,"/");
	}

	return nRetVal;
}


void GDirectory::RecurseFolder(const char *pzDir, GStringList *strDirs, GStringList *strFiles, int bDeep/*=1*/)
{
#ifdef _WIN32
	char chSlash = '\\';
#else
	char chSlash = '/';
#endif
	static GString strDot("[dir] .");
	static GString strDotDot("[dir] ..");
	try
	{
		// Sample listing 2 files + 1 directory = "file1.txt*[dir] Temp*file2.txt"
		GString strResults;
		GDirectory dir(pzDir, 2); // directories only
		GStringIterator it(&dir);
		while (it())
		{
			// pzResult will look something like "[dir] SubDir"
			const char *pzResult = it++; 
			if (strDot.Compare(pzResult) != 0 && strDotDot.Compare(pzResult) != 0)
			{
				// pzDir may be "/myPath" to begin with
				GString strFullPath(pzDir);
				if ( strFullPath.GetAt(strFullPath.GetLength() - 1) != '\\' && 
					 strFullPath.GetAt(strFullPath.GetLength() - 1) != '/')
				{
					// pzDir will now end with a slash if it did not already.
					// like "/myPath/" or "c:\myPath\"
					strFullPath += chSlash;
				}

				// add the file name to the complete path we're building
				strFullPath += &pzResult[6]; // skip the "[dir] ", add a string like "SubDir"

				if(strDirs)
				{
					strDirs->AddLast(strFullPath);
				}

				// now add the final slash for a string like this "/myPath/SubDir/"
				strFullPath += chSlash;

				// go down into that directory now.
				if (bDeep)
					RecurseFolder(strFullPath, strDirs, strFiles);
			}
		}

		if(strFiles)
		{
			GDirectory files(pzDir, 1); // files only
			GStringIterator it2(&files);
			while (it2())
			{
				// pzDir may be "/myPath" to begin with
				GString strFullPath(pzDir);
				if ( strFullPath.GetAt(strFullPath.GetLength() - 1) != '\\' && 
					 strFullPath.GetAt(strFullPath.GetLength() - 1) != '/')
				{
					// strFullPath will now end with a slash like "/myPath/" 
					strFullPath += chSlash;
				}


				const char *pzFile = it2++;
				strFullPath += pzFile;
				strFiles->AddLast((const char *)strFullPath);
			}
		}
	}
	catch( GException & )
	{
		// ignore the directory we can't access 
		// rErr.GetDescription();
	}
}


void GDirectory::Init(const char *szDirPath, int nMode, int nGetAttributes, int nLabelDirs)
{
	static GString dotdot("..");
	static GString dot(".");
	
	bool bIncludeSubDirs = (nMode == 2 || nMode == 3) ? 1 : 0;
	bool bIncludeFiles = (nMode == 1 || nMode == 3) ? 1 : 0;
	
	
	GString strPathWithTrailingSlash(szDirPath);
	GString strPathWithNoTrailingSlash(szDirPath);

	// if szDirPath ends with a slash
	if ( strPathWithNoTrailingSlash.Right(1) == "/" ||
		 strPathWithNoTrailingSlash.Right(1) == "\\" )
	{
		// if the path is "/" leave it alone
		if (strPathWithNoTrailingSlash.Length() > 1)
			strPathWithNoTrailingSlash = strPathWithNoTrailingSlash.Left(strPathWithNoTrailingSlash.Length() - 1);
	}
	else
	{
#ifdef _WIN32
			strPathWithTrailingSlash += "\\";
#else 
			strPathWithTrailingSlash += "/";
#endif
	}


#ifdef _WIN32
	if( access( (const char *)strPathWithNoTrailingSlash, 0 ) )
	{
		throw GException("GDirectoryListing",0,(const char *)strPathWithNoTrailingSlash, errno);
	}
#else
	if( access( (const char *)strPathWithNoTrailingSlash, F_OK ) )
	{
		throw GException("GDirectoryListing",0,(const char *)strPathWithNoTrailingSlash, errno);
	}
#endif
	

#if defined _WIN32

	// FindFirstFile & FindNextFile
	WIN32_FIND_DATA find;
	BOOL fRet = TRUE;
	GString strSearch( strPathWithTrailingSlash );
	strSearch += "*.*";
	// In VC 6.0 and older we must use FindFirstFile, not FindFirstFileEx
	#if defined(_MSC_VER) && _MSC_VER <= 1200 
	  HANDLE hFindFile = FindFirstFile(strSearch, &find);
	#else
	  HANDLE hFindFile = FindFirstFileEx(strSearch, FindExInfoStandard, &find, FindExSearchNameMatch, NULL, 0);
	#endif
	
	
	GString strTemp;
	while (hFindFile != (HANDLE)-1 && fRet == TRUE) 
	{
		if ( (find.cFileName[0] == '.' && find.cFileName[1] == 0)  || 
			 (find.cFileName[0] == '.' && find.cFileName[1] == '.' && find.cFileName[2] == 0))
		{
			fRet = FindNextFile(hFindFile, &find);
			continue;
		}

		strTemp = strPathWithTrailingSlash;
		strTemp += find.cFileName;
		{
			if( !(find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
			{
				// Add the file
				if (bIncludeFiles)
				{
					GString strFormatted;

					if (nGetAttributes == 1 || nGetAttributes == 3)
					{

				      __int64 nSize = ((__int64)find.nFileSizeHigh << 32 | find.nFileSizeLow);

						strFormatted << nSize;
						strFormatted.PadLeft(11,'0');
						strFormatted.CommaNumeric();
						strFormatted << "  |  ";

					}
					if (nGetAttributes == 2 || nGetAttributes == 3)
					{
						SYSTEMTIME st;
						SYSTEMTIME st2;
						FILETIME lft;
						if (FileTimeToSystemTime(&lft, &st))
						{
							if (SystemTimeToTzSpecificLocalTime(0,&st,&st2))
							{
								strFormatted.Format("%d.%02d.%02d:%02d:%02d", st2.wYear, st2.wMonth, st2.wDay, st2.wHour, st2.wMinute);
								strFormatted << "  |  ";

							}
						}
					}
					try
					{
						if (nGetAttributes == 0)
						{
#ifdef _UNICODE
							GString s;
							s << find.cFileName;
							AddLast( s );
#else
							AddLast( find.cFileName );
#endif
						}
						else
						{
							strFormatted << find.cFileName;
							AddLast( strFormatted );
						}
					}
					catch(...)
					{
						throw GException("GDirectoryListing",0,"Error adding file to result set");
					}
				}
			}
			else if (bIncludeSubDirs)
			{
				GString strFileName( LastLeaf( (char *)(const char *)strTemp,'\\') );
				if ( ( dotdot.Compare(strFileName) != 0 ) && ( dot.Compare(strFileName) != 0 ))
				{
					try
					{
						GString strFormattedDir;
						if (nLabelDirs)
						{
							strFormattedDir.Format("[dir] %s", LastLeaf( (char *)(const char *)strFileName,'\\') );
							AddLast((const char *)strFormattedDir);
						}
						else
						{
							AddLast(LastLeaf( (char *)(const char *)strFileName));
						}
					}
					catch(...)
					{
						throw GException("GDirectoryListing",0,"Error adding folder to result set");
					}
				}
			}
		}
		fRet = FindNextFile(hFindFile, &find);
	}
	FindClose(hFindFile);

#elif defined _LINUX || defined __sun || defined _HPUX || defined _IOS

	DIR *d;
	if (!(d = opendir( (const char *)strPathWithTrailingSlash )))
	{
		// 27=Directory [%s] does not exist or cannot be accessed.
		throw GException("GDirectoryListing",0,(const char *)strPathWithNoTrailingSlash, errno);
	}

	struct dirent *dstruct;

	while ((dstruct = readdir(d)))
	{
		struct stat sstruct;

		GString strTemp( strPathWithTrailingSlash );
		strTemp += dstruct->d_name;

		int result = stat((const char *)strTemp, &sstruct);
		if (result == 0)
		{
			if ( !S_ISDIR(sstruct.st_mode)  )
			{
				if (bIncludeFiles)
				{
					// Add files
					GString strFormatted;
					if (nGetAttributes == 1 || nGetAttributes == 3)
					{
						strFormatted << sstruct.st_size;
						strFormatted.PadLeft(11,'0');
						strFormatted.CommaNumeric();
//						strFormatted << "--?--";
						strFormatted << "  |  ";
					}
					if (nGetAttributes == 2 || nGetAttributes == 3)
					{
						// use sstruct.st_mtime
//						{
//							strFormatted.Format("%d.%02d.%02d:%02d:%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute);
//							strFormatted << "--?--";
//						}
					}
					strFormatted << (const char *)dstruct->d_name;
					AddLast( strFormatted );
				}
			}
			else if (bIncludeSubDirs)
			{
				GString strFileName( LastLeaf( (char *)(const char *)strTemp,'/') );
				if ( ( dotdot.Compare(strFileName) != 0 ) && ( dot.Compare(strFileName) != 0 ))
				{
					// Add Directories
					GString strFormattedDir;
					strFormattedDir.Format("[dir] %s", LastLeaf((char *)(const char *)strFileName,'/') );
					AddLast((const char *)strFormattedDir);
				}
			}
		}
	}

	closedir(d);

#endif
}


GDirectory::GDirectory(const char *szPath)
{
	Init(szPath, 1, 0, 1);
}

// nMode = 1 files, 2 dirs, 3 both
// nGetAttributes = 1 file size, 2 file time
GDirectory::GDirectory(const char *szPath, int nMode, int nGetAttributes/* = 0*/, int nLabelDirs/* = 1*/)
{
	Init(szPath, nMode, nGetAttributes, nLabelDirs);
}

GDirectory::~GDirectory()
{
}



