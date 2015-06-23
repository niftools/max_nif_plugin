// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
#include <io.h>
#include <Shlwapi.h>
#include <string>
#include <sstream>
#include <set>
#include <ShellAPI.h>

#ifndef ASSERT
#ifdef _DEBUG
#include <crtdbg.h>
#define ASSERT _ASSERTE
#else
#define ASSERT(exp)
#endif
#endif

#include <notify.h>
#include <plugapi.h>
#include <max.h>
#include <plugapi.h>
#include <utilapi.h>
#include <simpmod.h>
#include <simpobj.h>
#include <iparamb2.h>
#include <notetrck.h>
#include <modstack.h>
#include <iskin.h>
#include <stdmat.h>
#include <shaders.h>
#include <iiksys.h>
#include <meshadj.h>
#include <meshdelta.h>
#include <IFrameTagManager.h>
#include <macrorec.h>
#include <gport.h>
#include <dummy.h>
#include <particle.h>
#include <CS/BIPEXP.H>
#include <maxscript\maxscript.h>
#include <maxscript\compiler\parser.h>
#include <maxscript\maxwrapper\mxsobjects.h>
#include <maxscript\foundation\numbers.h>
#include <maxscript\foundation\colors.h>
#if VERSION_3DSMAX > (( 5000 << 16 ) + ( 15 << 8 ) + 0 ) // Version 5
	#include <MeshNormalSpec.h>
#endif

#include "nif_versions.h"
#include "niflib.h"
#include "ObjectRegistry.h"
#include "Key.h"
#include "Inertia.h"
#include "nifqhull.h"
#include "obj/NiNode.h"
#include "obj/BSBound.h"
#include "obj/BSXFlags.h"
#include "obj/NiStringExtraData.h"
#include "obj/NiTimeController.h"
#include "obj/NiControllerManager.h"
#include "obj/NiControllerSequence.h"
#include "obj/NiGeometry.h"
#include "obj/NiTransformController.h"
#include "obj/NiTransformData.h"
#include "obj/NiTransformInterpolator.h"
#include "obj/NiBSBoneLODController.h"
#include "obj/NiMultiTargetTransformController.h"
#include "obj/NiKeyframeData.h"
#include "obj/NiTriBasedGeom.h"
#include "obj/NiTriBasedGeomData.h"
#include "obj/NiTriShape.h"
#include "obj/NiTriShapeData.h"
#include "obj/NiTriStrips.h"
#include "obj/NiTriStripsData.h"
#include "obj/NiMaterialProperty.h"
#include "obj/NiWireframeProperty.h"
#include "obj/NiStencilProperty.h"
#include "obj/NiShadeProperty.h"
#include "obj/NiVertexColorProperty.h"
#include "obj/NiSpecularProperty.h"
#include "obj/NiDitherProperty.h"
#include "obj/NiAlphaProperty.h"
#include "obj/NiTextKeyExtraData.h"
#include "obj/NiDefaultAVObjectPalette.h"
#include "obj/NiSingleInterpController.h"
#include "obj/NiTextureProperty.h"
#include "obj/NiBillboardNode.h"
#include "obj/NiTexturingProperty.h"
#include "obj/SkyShaderProperty.h"
#include "obj/TileShaderProperty.h"
#include "obj/TallGrassShaderProperty.h"
#include "obj/Lighting30ShaderProperty.h"
#include "obj/NiSourceTexture.h"
#include "obj/NiImage.h"
#include "obj/NiSkinData.h"
#include "obj/NiSkinPartition.h"
#include "obj/NiParticleSystem.h"
#include "obj/NiPSysEmitter.h"
#include "obj/NiPSysMeshEmitter.h"
#include "obj/NiPSysGravityModifier.h"
#include "obj/NiPSysData.h"
#include "obj/NiPSysEmitterCtlr.h"
#include "obj/NiFloatInterpolator.h"
#include "obj/NiFloatData.h"
#include "obj/BSLODTriShape.h"
#include "obj/NiSwitchNode.h"

#include "obj/BSShaderProperty.h"
#include "obj/BSEffectShaderProperty.h"
#include "obj/BSLightingShaderProperty.h"
#include "obj/BSShaderTextureSet.h"
#include "obj/BSDismemberSkinInstance.h"
#include "obj/BSShaderNoLightingProperty.h"
#include "obj/BSShaderPPLightingProperty.h"
#include "obj/BSLightingShaderPropertyFloatController.h"
#include "obj/BSLightingShaderPropertyColorController.h"
#include "obj/BSEffectShaderPropertyFloatController.h"
#include "obj/BSEffectShaderPropertyColorController.h"

#include "obj/bhkBlendController.h"
#include "obj/bhkShape.h"
#include "obj/bhkCapsuleShape.h"
#include "obj/bhkSphereShape.h"
#include "obj/bhkBlendCollisionObject.h"
#include "obj/bhkRigidBody.h"
#include "obj/bhkRigidBodyT.h"
#include "obj/bhkBoxShape.h"
#include "obj/bhkTransformShape.h"
#include "obj/bhkConvexVerticesShape.h"
#include "obj/bhkNiTriStripsShape.h"
#include "obj/bhkPackedNiTriStripsShape.h"
#include "obj/hkPackedNiTriStripsData.h"
#include "obj/bhkMoppBvTreeShape.h"
#include "obj/bhkListShape.h"
#include "obj/bhkCompressedMeshShape.h"
#include "obj/bhkCompressedMeshShapeData.h"

#include "gen/QuaternionXYZW.h"
#include "gen/TexDesc.h"

#include "resource.h"

#include "NifPlugins.h"
#include "objectparams.h"
#include "niutils.h"
#include "AppSettings.h"
#include "Exporter.h"
#include "NifExport.h"
#include "AnimKey.h"
#include "iNifProps.h"
#include "NifGui.h"
#include "NifStrings.h"
#include "bhkHelperFuncs.h"
#include "bhkHelperInterface.h"
#include "bhkRigidBodyInterface.h"
#include "FurnitureMarkers.h"
#include "NifFurniture.h"
#include "shaderutil.h"
#include "BaseImporter.h"
#include "NifImporter.h"
#include "NodeMeshes.h"