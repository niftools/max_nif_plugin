//**********************************************************************
//
// FILE: BaseImporter.h
//
// CREATED BY: tazpn (Theo)
//
// HISTORY:
//	 Copyright (c) 2006, All Rights Reserved.
//**********************************************************************/
#ifndef NIFVERSION_H
#define NIFVERSION_H

#include "config.h"

//
// Define the major, minor, build, patch version numbers
//
#define VERSION_MAJOR_INT  3
#define VERSION_MINOR_INT  7
#define VERSION_BUILD_INT  2

#ifndef NIFTOOLS_REVISION_INT
#define VERSION_PATCH_INT  0
#else
#define  VERSION_PATCH_INT  NIFTOOLS_REVISION_INT
#endif

#ifndef VERSION_STRING
#ifndef NIFTOOLS_REVISION
#define VERSION_STRING "3, 7, 2, 0"
#else
#define VERSION_STRING "3, 7, 2, " NIFTOOLS_REVISION
#endif
#endif

//#define DEF_VERSION_STRING(a,b,c,d) "##a, ##b , ##c , ##d"
//#define VERSION_STRING DEF_VERSION_STRING(a,b,c,d)

#define COPYRIGHTYEARINT 2012
#define COPYRIGHTYEAR "2012"

#endif
