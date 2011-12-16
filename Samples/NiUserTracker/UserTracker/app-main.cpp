/*
 *  app-main.cpp
 *  UserTracker
 *
 *  Created by James Walker on 5/10/2011.
 *  Copyright 2011 ScriptPerfection Enterprises, Inc. All rights reserved.
 *
 */

/****************************************************************************
*                                                                           *
*  OpenNI 1.1 Alpha                                                         *
*  Copyright (C) 2011 PrimeSense Ltd.                                       *
*                                                                           *
*  This file is part of OpenNI.                                             *
*                                                                           *
*  OpenNI is free software: you can redistribute it and/or modify           *
*  it under the terms of the GNU Lesser General Public License as published *
*  by the Free Software Foundation, either version 3 of the License, or     *
*  (at your option) any later version.                                      *
*                                                                           *
*  OpenNI is distributed in the hope that it will be useful,                *
*  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the             *
*  GNU Lesser General Public License for more details.                      *
*                                                                           *
*  You should have received a copy of the GNU Lesser General Public License *
*  along with OpenNI. If not, see <http://www.gnu.org/licenses/>.           *
*                                                                           *
****************************************************************************/
//---------------------------------------------------------------------------
// Includes
//---------------------------------------------------------------------------
#include <XnOpenNI.h>
#include <XnCodecIDs.h>
#include <XnCppWrapper.h>

#import "SceneDrawer.h"
#import "RegisterPrivateModule.h"
#import "InitLogDestination.h"
#include <XnVSessionManager.h>
#include <XnVCircleDetector.h>

#import <CoreFoundation/CoreFoundation.h>

//---------------------------------------------------------------------------
// Globals
//---------------------------------------------------------------------------
xn::Context g_Context;
xn::ScriptNode g_scriptNode;
xn::DepthGenerator g_DepthGenerator;
xn::UserGenerator g_UserGenerator;
xn::IRGenerator g_image;
xn::GestureGenerator g_GestureGenerator;
XnVSessionManager * g_pSessionManager = NULL;
XnVCircleDetector*  g_pCircle = NULL;

XnBool g_bNeedPose = FALSE;
XnChar g_strPose[20] = "";
XnBool g_bDrawBackground = TRUE;
XnBool g_bDrawPixels = TRUE;
XnBool g_bDrawSkeleton = TRUE;
XnBool g_bPrintID = TRUE;
XnBool g_bPrintState = TRUE;

#include <GLUT/glut.h>


#define GL_WIN_SIZE_X 720
#define GL_WIN_SIZE_Y 480

XnBool g_bPause = false;
XnBool g_bRecord = false;

XnBool g_bQuit = false;

//---------------------------------------------------------------------------
// Code
//---------------------------------------------------------------------------

void CleanupExit()
{
	g_scriptNode.Release();
	g_DepthGenerator.Release();
	g_UserGenerator.Release();
	g_Context.Release();

	exit (1);
}

// Callback: New user was detected
void XN_CALLBACK_TYPE User_NewUser(xn::UserGenerator& generator, XnUserID nId, void* pCookie)
{
	printf("New User %d\n", nId);
	// New user found
	if (g_bNeedPose)
	{
		g_UserGenerator.GetPoseDetectionCap().StartPoseDetection(g_strPose, nId);
	}
	else
	{
		g_UserGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);
	}
}
// Callback: An existing user was lost
void XN_CALLBACK_TYPE User_LostUser(xn::UserGenerator& generator, XnUserID nId, void* pCookie)
{
	printf("Lost user %d\n", nId);
}
// Callback: Detected a pose
void XN_CALLBACK_TYPE UserPose_PoseDetected(xn::PoseDetectionCapability& capability, const XnChar* strPose, XnUserID nId, void* pCookie)
{
	printf("Pose %s detected for user %d\n", strPose, nId);
	g_UserGenerator.GetPoseDetectionCap().StopPoseDetection(nId);
	g_UserGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);
}
// Callback: Started calibration
void XN_CALLBACK_TYPE UserCalibration_CalibrationStart(xn::SkeletonCapability& capability, XnUserID nId, void* pCookie)
{
	printf("Calibration started for user %d\n", nId);
}
// Callback: Finished calibration
void XN_CALLBACK_TYPE UserCalibration_CalibrationEnd(xn::SkeletonCapability& capability,
			XnUserID nId, XnCalibrationStatus calibrationError, void* pCookie)
{
	if (calibrationError == XN_CALIBRATION_STATUS_OK)
	{
		// Calibration succeeded
		printf("Calibration complete, start tracking user %d\n", nId);
		g_UserGenerator.GetSkeletonCap().StartTracking(nId);
	}
	else
	{
		// Calibration failed
		printf("Calibration failed for user %d\n", nId);
		if (g_bNeedPose)
		{
			g_UserGenerator.GetPoseDetectionCap().StartPoseDetection(g_strPose, nId);
		}
		else
		{
			g_UserGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);
		}
	}
}

#define XN_CALIBRATION_FILE_NAME "UserCalibration.bin"

// Save calibration to file
void SaveCalibration()
{
	XnUserID aUserIDs[20] = {0};
	XnUInt16 nUsers = 20;
	g_UserGenerator.GetUsers(aUserIDs, nUsers);
	for (int i = 0; i < nUsers; ++i)
	{
		// Find a user who is already calibrated
		if (g_UserGenerator.GetSkeletonCap().IsCalibrated(aUserIDs[i]))
		{
			// Save user's calibration to file
			g_UserGenerator.GetSkeletonCap().SaveCalibrationDataToFile(aUserIDs[i], XN_CALIBRATION_FILE_NAME);
			break;
		}
	}
}
// Load calibration from file
void LoadCalibration()
{
	XnUserID aUserIDs[20] = {0};
	XnUInt16 nUsers = 20;
	g_UserGenerator.GetUsers(aUserIDs, nUsers);
	for (int i = 0; i < nUsers; ++i)
	{
		// Find a user who isn't calibrated or currently in pose
		if (g_UserGenerator.GetSkeletonCap().IsCalibrated(aUserIDs[i])) continue;
		if (g_UserGenerator.GetSkeletonCap().IsCalibrating(aUserIDs[i])) continue;

		// Load user's calibration from file
		XnStatus rc = g_UserGenerator.GetSkeletonCap().LoadCalibrationDataFromFile(aUserIDs[i], XN_CALIBRATION_FILE_NAME);
		if (rc == XN_STATUS_OK)
		{
			// Make sure state is coherent
			g_UserGenerator.GetPoseDetectionCap().StopPoseDetection(aUserIDs[i]);
			g_UserGenerator.GetSkeletonCap().StartTracking(aUserIDs[i]);
		}
		break;
	}
}

// this function is called each frame
void glutDisplay (void)
{

	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Setup the OpenGL viewpoint
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();

	xn::SceneMetaData sceneMD;
	xn::DepthMetaData depthMD;
	xn::IRMetaData imageMD;
	
	g_DepthGenerator.GetMetaData(depthMD);
	glOrtho(0, depthMD.XRes(), depthMD.YRes(), 0, -1.0, 1.0);

	glDisable(GL_TEXTURE_2D);

	if (!g_bPause)
	{
		// Read next available data
		g_Context.WaitOneUpdateAll(g_DepthGenerator);

		// Process the data
		g_pSessionManager->Update(&g_Context);
	}

		// Process the data
		g_DepthGenerator.GetMetaData(depthMD);
		g_UserGenerator.GetUserPixels(0, sceneMD);
		g_image.GetMetaData(imageMD);
		
		//DrawDepthMap(depthMD, sceneMD);
		//DrawImageMap( imageMD, sceneMD );
		DrawIRMap( imageMD );

	glutSwapBuffers();
}

void glutIdle (void)
{
	if (g_bQuit) {
		CleanupExit();
	}

	// Display the frame
	glutPostRedisplay();
}

void glutKeyboard (unsigned char key, int x, int y)
{
	switch (key)
	{
	case 27:
		CleanupExit();
	case 'b':
		// Draw background?
		g_bDrawBackground = !g_bDrawBackground;
		break;
	case 'x':
		// Draw pixels at all?
		g_bDrawPixels = !g_bDrawPixels;
		break;
	case 's':
		// Draw Skeleton?
		g_bDrawSkeleton = !g_bDrawSkeleton;
		break;
	case 'i':
		// Print label?
		g_bPrintID = !g_bPrintID;
		break;
	case 'l':
		// Print ID & state as label, or only ID?
		g_bPrintState = !g_bPrintState;
		break;
	case'p':
		g_bPause = !g_bPause;
		break;
	case 'S':
		SaveCalibration();
		break;
	case 'L':
		LoadCalibration();
		break;
	}
}
void glInit (int * pargc, char ** argv)
{
	glutInit(pargc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(GL_WIN_SIZE_X, GL_WIN_SIZE_Y);
	glutCreateWindow ("Prime Sense User Tracker Viewer");
	//glutFullScreen();
	glutSetCursor(GLUT_CURSOR_NONE);

	glutKeyboardFunc(glutKeyboard);
	glutDisplayFunc(glutDisplay);
	glutIdleFunc(glutIdle);

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);

	glEnableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
}

static void ReportFailure( XnStatus inErrCode, const char* inMessage )
{
	printf( "%s failed: %s\n", inMessage, xnGetStatusString(inErrCode) );
}


#define CHECK_RC(nRetVal, what)										\
	if (nRetVal != XN_STATUS_OK)									\
	{																\
		ReportFailure( nRetVal, what );								\
		return nRetVal;												\
	}


static void RegisterModules()
{
	RegisterPrivateModule( "XnDeviceSensorV2KM" );	// Required
	RegisterPrivateModule( "XnVFeatures_1_5_0" );	// Required to create User node
	RegisterPrivateModule( "XnVNite_1_5_0" );	// Required to create Script node
	RegisterPrivateModule( "XnVHandGenerator_1_5_0" );
	
	// Experimental things that might not be needed.
	// OpenNI
	//RegisterPrivateModule( "nimMockNodes" );
	//RegisterPrivateModule( "nimCodecs" );
	//RegisterPrivateModule( "nimRecorder" );
	// NITE
	RegisterPrivateModule( "XnVCNITE_1_5_0" );
	// Sensor
	//RegisterPrivateModule( "XnDeviceFile" );
	//RegisterPrivateModule( "XnCore" );
	//RegisterPrivateModule( "XnDDK" );
}

/*static void AddLicense()
{
	XnLicense theLicense =
	{
		"PrimeSense",
		"0KOIk2JeIBYClPWVnMoRKn5cdY4="
	};
	g_Context.AddLicense( theLicense );
}*/

typedef char char32[32];

void XN_CALLBACK_TYPE SessionStart(const XnPoint3D& ptFocus, void *pUserCxt)
{
	//SetVisualFeedbackFrame(true, 0.1, 1.0, 0.1);
	fprintf( stderr, "SessionStart\n");
}

void XN_CALLBACK_TYPE SessionEnd(void *pUserCxt)
{
	//SetVisualFeedbackFrame(false, 0, 0, 0);
	fprintf( stderr, "SessionEnd\n");
}

void XN_CALLBACK_TYPE CircleCB(XnFloat fTimes, XnBool bConfident, const XnVCircle* pCircle, void* pUserCxt)
{
	fprintf( stderr, "CircleCB\n");
}

void XN_CALLBACK_TYPE NoCircleCB(XnFloat fLastValue, XnVCircleDetector::XnVNoCircleReason reason, void * pUserCxt)
{
	fprintf( stderr, "NoCircleCB\n");
}

void XN_CALLBACK_TYPE Circle_PrimaryCreate(const XnVHandPointContext *cxt, const XnPoint3D& ptFocus, void * pUserCxt)
{
	fprintf( stderr, "Circle_PrimaryCreate\n");
}

void XN_CALLBACK_TYPE Circle_PrimaryDestroy(XnUInt32 nID, void * pUserCxt)
{
	fprintf( stderr, "Circle_PrimaryDestroy\n");
}

void GestureRecognizedProc( xn::GestureGenerator& generator, const XnChar* strGesture,
	const XnPoint3D* pIDPosition, const XnPoint3D* pEndPosition, void* pCookie )
{
	fprintf( stderr, "Gesture '%s' Recognized\n", (const char*)strGesture );
}


void GestureProgressProc( xn::GestureGenerator& generator, const XnChar* strGesture,
	const XnPoint3D* pPosition, XnFloat fProgress, void* pCookie )
{
	fprintf( stderr, "Gesture '%s' in progress, %f\n", (const char*)strGesture,
		fProgress );
}

int main(int argc, char **argv)
{
	XnStatus nRetVal = XN_STATUS_OK;
	
	InitLogDestination();
	
	RegisterModules();

	char configPath[ 1024 ];
	CFBundleRef mainBundle = CFBundleGetMainBundle();
	CFURLRef dataURL = CFBundleCopyResourceURL( mainBundle,
		CFSTR("SamplesConfig"), CFSTR("xml"), NULL );
	if (dataURL != NULL)
	{
		CFURLGetFileSystemRepresentation( dataURL, true, (UInt8*)configPath,
			sizeof(configPath) );
		CFRelease( dataURL );
	}
	else
	{
		return 1;
	}
	
	{
		xn::EnumerationErrors errors;
		nRetVal = g_Context.InitFromXmlFile( configPath, g_scriptNode, &errors );
		if (nRetVal == XN_STATUS_NO_NODE_PRESENT)
		{
			XnChar strError[1024];
			errors.ToString(strError, 1024);
			printf("%s\n", strError);
			return (nRetVal);
		}
		else if (nRetVal != XN_STATUS_OK)
		{
			printf("Open failed: %s\n", xnGetStatusString(nRetVal));
			return (nRetVal);
		}
	}

	//AddLicense();

#if 0
	// Create and initialize point tracker
	g_pSessionManager = new XnVSessionManager();
	nRetVal = g_pSessionManager->Initialize(&g_Context, "Wave", "RaiseHand");
	if (nRetVal != XN_STATUS_OK)
	{
		printf("Couldn't initialize the Session Manager: %s\n", xnGetStatusString(nRetVal));
		CleanupExit();
	}
#endif
	
#if 0
	g_pSessionManager->RegisterSession(NULL, &SessionStart, &SessionEnd);

	// init and register circle control
	g_pCircle = new XnVCircleDetector;
	g_pCircle->RegisterCircle(NULL, &CircleCB);
	g_pCircle->RegisterNoCircle(NULL, &NoCircleCB);
	g_pCircle->RegisterPrimaryPointCreate(NULL, &Circle_PrimaryCreate);
	g_pCircle->RegisterPrimaryPointDestroy(NULL, &Circle_PrimaryDestroy);
	g_pSessionManager->AddListener(g_pCircle);
#endif

	nRetVal = g_Context.FindExistingNode(XN_NODE_TYPE_DEPTH, g_DepthGenerator);
	CHECK_RC(nRetVal, "Find depth generator");
	nRetVal = g_Context.FindExistingNode(XN_NODE_TYPE_USER, g_UserGenerator);
	if (nRetVal != XN_STATUS_OK)
	{
		nRetVal = g_UserGenerator.Create(g_Context);
		CHECK_RC(nRetVal, "Find user generator");
	}
	//nRetVal = g_Context.FindExistingNode(XN_NODE_TYPE_IMAGE, g_image);
	nRetVal = g_Context.FindExistingNode(XN_NODE_TYPE_IR, g_image);
	if (nRetVal != XN_STATUS_OK)
	{
		nRetVal = g_image.Create(g_Context);
		CHECK_RC(nRetVal, "Find image generator");
	}

	// Get a gesture generator
	nRetVal = g_Context.FindExistingNode(XN_NODE_TYPE_GESTURE, g_GestureGenerator);
	if (nRetVal != XN_STATUS_OK)
	{
		nRetVal = g_GestureGenerator.Create(g_Context);
		CHECK_RC(nRetVal, "Find gesture generator");
	}
	
	// Find out what gestures are available
	{
		char32 gests[20];
		XnUInt16 numGest = sizeof(gests);
		char* namePtrs[20];
		for (int i = 0; i < 20; ++i)
		{
			namePtrs[i] = (char*)gests[i];
		}
		nRetVal = g_GestureGenerator.EnumerateAllGestures( namePtrs, 32, numGest );
		CHECK_RC(nRetVal, "Find gesture count");
		printf("%d gestures\n", (int)numGest );
	}
	
	// Register callbacks for gestures
	XnCallbackHandle hGestureCallback;
	g_GestureGenerator.RegisterGestureCallbacks( GestureRecognizedProc,
		GestureProgressProc, NULL, hGestureCallback );

	XnCallbackHandle hUserCallbacks, hCalibrationStartCallback,
		hiCalibrationEndCallback, hPoseDetectedCallback;
	if (!g_UserGenerator.IsCapabilitySupported(XN_CAPABILITY_SKELETON))
	{
		printf("Supplied user generator doesn't support skeleton\n");
		return 1;
	}
	g_UserGenerator.RegisterUserCallbacks(User_NewUser, User_LostUser, NULL, hUserCallbacks);
	g_UserGenerator.GetSkeletonCap().RegisterToCalibrationStart( UserCalibration_CalibrationStart, NULL, hCalibrationStartCallback );
	g_UserGenerator.GetSkeletonCap().RegisterToCalibrationComplete( UserCalibration_CalibrationEnd, NULL, hiCalibrationEndCallback );

	if (g_UserGenerator.GetSkeletonCap().NeedPoseForCalibration())
	{
		g_bNeedPose = TRUE;
		if (!g_UserGenerator.IsCapabilitySupported(XN_CAPABILITY_POSE_DETECTION))
		{
			printf("Pose required, but not supported\n");
			return 1;
		}
		g_UserGenerator.GetPoseDetectionCap().RegisterToPoseDetected( UserPose_PoseDetected, NULL, hPoseDetectedCallback );
		g_UserGenerator.GetSkeletonCap().GetCalibrationPose(g_strPose);
	}

	g_UserGenerator.GetSkeletonCap().SetSkeletonProfile(XN_SKEL_PROFILE_ALL);
	
	// JWW: Not sure what this does, but I copied it from NISimpleViewer, and
	// it helps align the scene analysis labels with the image.
	//g_DepthGenerator.GetAlternativeViewPointCap().SetViewPoint(g_image);
	//g_image.GetAlternativeViewPointCap().SetViewPoint(g_DepthGenerator);

	nRetVal = g_Context.StartGeneratingAll();
	CHECK_RC(nRetVal, "StartGenerating");

	glInit(&argc, argv);
	glutMainLoop();
}
