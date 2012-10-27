#pragma warning( disable:4800 )

#include "resource.h"

#include <deque>
#include <algorithm>
#include <functional>
#include <string>
#include <sstream>
#include <set>

#include "Max.h"
#include "resource.h"
#include "istdplug.h"
#include "iparamb2.h"
#include "iparamm2.h"
#include "objectParams.h"
#undef ALPHA_NONE

#ifndef BIPSLAVE_CONTROL_CLASS_ID
#define BIPSLAVE_CONTROL_CLASS_ID Class_ID(0x9154,0)
#endif
// this is the class for the center of mass, biped root controller ("Bip01")
#ifndef BIPBODY_CONTROL_CLASS_ID
#define BIPBODY_CONTROL_CLASS_ID  Class_ID(0x9156,0) 
#endif
// this is the class for the biped footstep controller ("Bip01 Footsteps")
#ifndef FOOTPRINT_CLASS_ID
#define FOOTPRINT_CLASS_ID Class_ID(0x3011,0)        
#endif
// this is the class for biped geometric objects
#ifdef SKELOBJ_CLASS_ID
#define SKELOBJ_CLASS_ID Class_ID(0x9125, 0)
#endif
// this is the class for the biped master controller
#ifdef BIPED_CLASS_ID
#define BIPED_CLASS_ID Class_ID(0x9155, 0)
#endif

// niflib/Ref.h' header guard caused havok!
// near & far 
//#include "niflib/pch.h"
#include "niflib.h"
#include "obj/NiNode.h"
#include "obj/NiTriStrips.h"
#include "obj/NiTriStripsData.h"
#include "obj/NiTriShape.h"
#include "obj/NiTriShapeData.h"
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

// undef macros for tristripper
//#undef max
//#undef min
#include "../NvTriStrip/NvTriStrip.h"
//#include "TriStripper/detail/types.h"
//#include "TriStripper/detail/policy.h"
//#include "TriStripper/detail/cache_simulator.h"
//#include "TriStripper/tri_stripper.h"

#include "NifPlugins.h"
#include "Exporter.h"
#include "NifExport.h"

#if VERSION_3DSMAX >= (14000<<16) // Version 14 (2012)
#define SDK_RESERVED_METHOD(a)
#endif

#include "niutils.h"
#include <shlwapi.h>
