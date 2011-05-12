/*
 *  RegisterPrivateModule.cpp
 *  UserTracker
 *
 *  Created by James Walker on 5/10/2011.
 *  Copyright 2011 ScriptPerfection Enterprises, Inc. All rights reserved.
 *
 */

#import "RegisterPrivateModule.h"

#import <CoreFoundation/CoreFoundation.h>

#import <XnModuleInterface.h>
#import <XnUtils.h>

#import <dlfcn.h>
#import <cstring>

void	RegisterPrivateModule( const char* inNameBase )
{
	CFBundleRef mainBundle = CFBundleGetMainBundle();
	CFURLRef frameworksURL = CFBundleCopyPrivateFrameworksURL( mainBundle );
	CFStringRef libName = CFStringCreateWithFormat( NULL, NULL,
		CFSTR("lib%s.dylib"), inNameBase );
	CFURLRef libURL = CFURLCreateCopyAppendingPathComponent( NULL, frameworksURL,
		libName, false );
	CFRelease( libName );
	CFRelease( frameworksURL );
	
	char fullPath[ 1024 ];
	CFURLGetFileSystemRepresentation( libURL, true, (UInt8*)fullPath, sizeof(fullPath) );
	CFRelease( libURL );

	void* libHandle = dlopen( fullPath, RTLD_LAZY );
	
	if (libHandle != NULL)
	{
		XnOpenNIModuleInterface* interfaceRec = new XnOpenNIModuleInterface;
		// this pointer will leak, but there are only a few
		
		interfaceRec->pLoadFunc = (XnModuleLoadPtr) dlsym( libHandle,
				XN_STRINGIFY(XN_MODULE_LOAD) );
		interfaceRec->pUnloadFunc = (XnModuleUnloadPtr) dlsym( libHandle,
				XN_STRINGIFY(XN_MODULE_UNLOAD) );
		interfaceRec->pGetCountFunc = (XnModuleGetExportedNodesCountPtr) dlsym( libHandle,
				XN_STRINGIFY(XN_MODULE_GET_EXPORTED_NODES_COUNT) );
		interfaceRec->pGetEntryPointsFunc = (XnModuleGetExportedNodesEntryPointsPtr) dlsym( libHandle,
				XN_STRINGIFY(XN_MODULE_GET_EXPORTED_NODES_ENTRY_POINTS) );
		interfaceRec->pGetVersionFunc = (XnModuleGetOpenNIVersionPtr) dlsym( libHandle,
				XN_STRINGIFY(XN_MODULE_GET_OPEN_NI_VERSION) );

		const char* configDirPath = "";
		
		if (NULL != std::strstr( inNameBase, "Features" ))
		{
			CFURLRef dataURL = CFBundleCopyResourceURL( mainBundle,
				CFSTR("s"), CFSTR("dat"), CFSTR("Features") );
			if (dataURL != NULL)
			{
				CFURLRef dirURL = CFURLCreateCopyDeletingLastPathComponent( NULL,
					dataURL );
				CFRelease( dataURL );
				CFURLGetFileSystemRepresentation( dirURL, true, (UInt8*)fullPath,
					sizeof(fullPath) );
				CFRelease( dirURL );
				
				// Copy the string into heap storage, which again will leak.  Sigh.
				// But xnRegisterModuleWithOpenNI just passes pointers around.
				int len = std::strlen( fullPath );
				configDirPath = new char[len+1];
				std::strcpy( (char*)configDirPath, fullPath );
			}
		}
		
		xnRegisterModuleWithOpenNI( interfaceRec, configDirPath, inNameBase );
	}
}
