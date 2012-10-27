#pragma warning( disable:4800 )

#include <stdio.h>
#include <tchar.h>
#include <iomanip>
#include <cmath>
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <exception>
#include <stdexcept>
#include <algorithm>
#include <functional>
#include <numeric>
#include <vector>
#include <list>
#include <map>
#include <deque>
#include <sstream>

// _WIN32 will detect windows on most compilers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <math.h>
#include <io.h>
#include <string.h>
#include <direct.h>
#include <commdlg.h>

#ifndef ASSERT
#ifdef _DEBUG
#include <crtdbg.h>
#define ASSERT _ASSERTE
#else
#define ASSERT(exp)
#endif
#endif

#include <Max.h>
#include "MAX_MemDirect.h"
#include <istdplug.h>
#include <iparamb2.h>
#include <iparamm2.h>
#ifdef USE_BIPED
#  include <cs/BipedApi.h>
#endif
#include <plugapi.h>
#include <triobj.h> 
#include <bitmap.h>
#include <modstack.h>
#include <iskin.h>
#include <strclass.h>
#include "objectParams.h"

#undef ALPHA_NONE
#undef DECAY_NONE

#include "niflib.h"
#include "obj/NiObject.h"
#include "obj/NiNode.h"
#include "obj/NiTriShape.h"
#include "obj/NiTriShapeData.h"
#include "obj/NiTriStrips.h"
#include "obj/NiTriStripsData.h"
#include "obj/NiMaterialProperty.h"
#include "obj/NiTexturingProperty.h"
#include "obj/NiSourceTexture.h"
#include "obj/NiExtraData.h"
#include "obj/BSBound.h"
#include "obj/NiSkinData.h"
#include "obj/NiSkinInstance.h"
#include "obj/NiSkinPartition.h"
#include "obj/NiLight.h"
#include "obj/bhkCollisionObject.h"
#include "obj/bhkRigidBody.h"
#include "obj/bhkNiTriStripsShape.h"
#include "obj/bhkBoxShape.h"
#include "obj/bhkSphereShape.h"
#include "obj/bhkCapsuleShape.h"
#include "obj/NiMaterialProperty.h"
#include "obj/NiTexturingProperty.h"
#include "obj/NiSourceTexture.h"
#include "obj/BsxFlags.h"
#include "obj/NiStringExtraData.h"
#include "obj/bhkRigidBodyT.h"

#include "niutils.h"
#include "AppSettings.h"

// undef macros for tristripper
#undef max
#undef min
#include "NvTriStrip/NvTriStrip.h"
#include "TriStripper/tri_stripper.h"

#include "NifPlugins.h"
