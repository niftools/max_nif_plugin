#pragma warning( disable:4800 )

#include "resource.h"

#include <deque>
#include <algorithm>
#include <functional>
#include <string>
#include <sstream>

#include "Max.h"
#include "resource.h"
#include "istdplug.h"
#include "iparamb2.h"
#include "iparamm2.h"
#include "objectParams.h"
#undef ALPHA_NONE

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

#include "niutils.h"
#include <shlwapi.h>
