/*
 *  InitLogDestination.cpp
 *  UserTracker
 *
 *  Created by James Walker on 5/13/2011.
 *  Copyright 2011 ScriptPerfection Enterprises, Inc. All rights reserved.
 *
 */

#import "InitLogDestination.h"

#import <CoreFoundation/CoreFoundation.h>
#import <CoreServices/CoreServices.h>
#import <ApplicationServices/ApplicationServices.h>

#import <XnLog.h>

#import <sys/syslimits.h>

void	InitLogDestination()
{
	FSRef logDirRef;
	OSStatus err = FSFindFolder( kUserDomain, kLogsFolderType, kCreateFolder,
		&logDirRef );
	if (err == noErr)
	{
		CFURLRef logDirURL = CFURLCreateFromFSRef( NULL, &logDirRef );
		
		ProcessSerialNumber myPSN =
		{
			kNoProcess,
			kCurrentProcess
		};
		CFStringRef processName = NULL;
		CopyProcessName( &myPSN, &processName );
		
		// Although I am creating the name of a directory, I pass 'false' for
		// the isDirectory parameter here, because xnLogSetOutputFolder appends
		// a directory separator to the path you pass in.
		CFURLRef myLogsURL = CFURLCreateCopyAppendingPathComponent( NULL,
			logDirURL, processName, false );
		
		char fullPath[ PATH_MAX ];
		CFURLGetFileSystemRepresentation( myLogsURL, true, (UInt8*)fullPath,
			sizeof(fullPath) );
		
		printf( "Log path '%s' length %d\n", fullPath, (int)strlen( fullPath ) );
		xnLogSetOutputFolder( fullPath );
		
		// Clean up
		CFRelease( logDirURL );
		CFRelease( processName );
		CFRelease( myLogsURL );
	}
}
