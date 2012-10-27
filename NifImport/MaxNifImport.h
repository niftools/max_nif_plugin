/**********************************************************************
 *<
	FILE: MaxNifImport.h

	DESCRIPTION:	Includes for Plugins

	CREATED BY: tazpn (Theo)

	HISTORY:

 *>	Copyright (c) 2006, All Rights Reserved.
 **********************************************************************/

#ifndef __MaxNifImport__H
#define __MaxNifImport__H

#include <math.h>
#include <io.h>
#include <sstream>

#include <Max.h>
#include "resource.h"
#include <istdplug.h>
#include <iparamb2.h>
#include <iparamm2.h>
#ifdef USE_BIPED
#  include <cs/BipedApi.h>
#endif
#include <meshadj.h>
//#include <scenetraversal.h> 
#include <plugapi.h>
#include <maxtypes.h>
#include <triobj.h> 
#include <stdmat.h>
#include <simpobj.h>
#include <bitmap.h>
#include <modstack.h>
#include <iskin.h>
#include <strclass.h>
#include "objectParams.h"
#undef ALPHA_NONE

#include <math.h>
#include <string.h>

#include "niflib.h"
#include "obj\NiObject.h"
#include "obj\NiNode.h"
#include "obj\NiTriShape.h"
#include "obj\NiTriShapeData.h"
#include "obj\NiTriStrips.h"
#include "obj\NiTriStripsData.h"
#include "obj\NiMaterialProperty.h"
#include "obj\NiTexturingProperty.h"
#include "obj\NiSourceTexture.h"
#include "obj\NiExtraData.h"
#include "obj\BSBound.h"
#include "obj\NiSkinData.h"
#include "obj\NiSkinInstance.h"
#include "obj\NiSkinPartition.h"
#include "obj\NiLight.h"
#include "obj\NiTextureProperty.h"

#include "niutils.h"
#include "AppSettings.h"

#ifndef ASSERT
#ifdef _DEBUG
#include <crtdbg.h>
#define ASSERT _ASSERTE
#else
#define ASSERT(exp)
#endif
#endif

#include <direct.h>
#include <commdlg.h>

#include "NifImporter.h"

extern TCHAR *GetString(int id);
extern TSTR shortDescription;
extern HINSTANCE hInstance;

#endif
