#pragma warning( disable:4800 )

#include "resource.h"

#include "Max.h"
#include "resource.h"
#include "istdplug.h"
#include "iparamb2.h"
#include "iparamm2.h"


// niflib/Ref.h' header guard caused havok!
// near & far 
#include "niflib/pch.h"
#include "niflib/obj/NiTriStrips.h"
#include "niflib/obj/NiTriStripsData.h"
#include "niflib/obj/NiTriShape.h"
#include "niflib/obj/NiTriShapeData.h"
#include "niflib/obj/bhkCollisionObject.h"
#include "niflib/obj/bhkRigidBodyT.h"
#include "niflib/obj/bhkNiTriStripsShape.h"
#include "niflib/obj/NiMaterialProperty.h"
#include "niflib/obj/NiTexturingProperty.h"
#include "niflib/obj/NiSourceTexture.h"
#include "niflib/obj/BsxFlags.h"
#include "niflib/obj/NiStringExtraData.h"

#include "NvTriStrip/NvTriStrip.h"
#include <deque>
#include "TriStripper/tri_stripper.h"

#include "../NifPlugins.h"
#include "Exporter.h"
#include "NifExport.h"
