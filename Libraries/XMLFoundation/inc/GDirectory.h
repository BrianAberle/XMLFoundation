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

#ifndef _DIRECTORY_H__
#define _DIRECTORY_H__

#include "GString.h"
#include "GStringList.h"

class GDirectory : public GStringList
{
protected:
	void Init(const char *szPath, int nMode, int bGetSizes, int nLabelDirs); 

public:
	// gather all files in the file system under the specified pzDir.
	// strDirs and strFiles are optional. (one, the other, or both) neither won't do anything.
	static void RecurseFolder(const char *pzDir, GStringList *strDirs, GStringList *strFiles, int bDeep = 1);
	
	// return the last leaf from a fully qualified path (file or directory)
	// given "c:\my\path\myfile.ext" return will be "myfile.ext"
	// given "/my/path" or "/my/path/" return will be "path"
	//
	// chSlash is generally left to default (auto-detect), in UNIX this is a valid
	// directory "/user/a\b\c" where "a\b\c" is a single directory under user.
	// left to auto-detect "c" will be returned when in fact "a\b\c" is the LastLeaf.
	static const char *LastLeaf(const char *pzFullPath, char chSlash = 0);

	// given "c:\my\path\" or "c:\my\path\myFile.ext" or "/my/path" or "/my/path/myfile.ext"
	// the following folders will be made: 
	// c:\my			or		/my
	// c:\my\path		or		/my/path
	// bPathHasFileName should be 1 or 0, 1 if the pzPathOnlyOrPathAndFileName is "/My/path/MyFile.txt"
	//									  0 if the pzPathOnlyOrPathAndFileName is "/My/path"
	static void CreatePath(const char *pzPathOnlyOrPathAndFileName, int bPathHasFileName);

	// returns 1 if the argument is a directory, 0 if it's a file or a bad path.
	static int IsDirectory(const char *szDirPath);

	// same as rmdir() but it removes the directory, all the files in it, and all the folder under it
	// returns 1 if it worked, otherwise 0 and some indication why in strError
	static int RmDirDeep(const char *pzDirectory, GString &strError);

	// recurse the stats on a folder
	static int DirStats(const char *pzDirectory, int bDeep, __int64 *pnTotalBytesAllFiles, __int64 *pnTotalFileCount, __int64 *pnTotalFolderCount);

	GDirectory(const char *szPath);
	
	// nMode = 1 files, 2 dirs, 3 both
	// nGetAttributes = 0 none, 1 file size, 2 file time, 3 both
	// nLabelDirs only applies if nMode = 2 or 3, if so and nLabelDirs is true each directory will be prefixed with "[dir] "

	GDirectory(const char *szPath, int nMode, int nGetAttributes = 0, int nLabelDirs = 1);
	~GDirectory();
};

#endif //_DIRECTORY_H__