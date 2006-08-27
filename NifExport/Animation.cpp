/**********************************************************************
*<
FILE: ImportAnimation.cpp

DESCRIPTION:	Animation Import Routines

CREATED BY: tazpn (Theo)

HISTORY: 

*>	Copyright (c) 2006, All Rights Reserved.
**********************************************************************/
#include "pch.h"
#include "NifExport.h"
#include "AnimKey.h"

#include <obj/NiControllerSequence.h>
#include <obj/NiInterpolator.h>
#include <obj/NiTransformInterpolator.h>
#include <obj/NiTransformData.h>
#include <obj/NiTimeController.h>
#include <obj/NiTransformController.h>
#include <obj/NiTextKeyExtraData.h>
#include <obj/NiKeyframeController.h>
#include <obj/NiKeyframeData.h>
#include <obj/NiStringPalette.h>
#include <obj/NiBSplineCompTransformInterpolator.h>
using namespace Niflib;

const Class_ID IPOS_CONTROL_CLASS_ID = Class_ID(0x118f7e02,0xffee238a);
enum {
   IPOS_X_REF	=	0,
   IPOS_Y_REF	=	1,
   IPOS_Z_REF	=	2,
   IPOS_W_REF	=	3,
};

struct AnimationExport
{
   AnimationExport(Exporter& parent) : ne(parent) {}

   bool doExport();
   bool exportController(INode *node, NiControllerSequenceRef seq);
   Control *GetTMController(INode* node);

   Exporter &ne;
};

bool Exporter::doAnimExport()
{
   AnimationExport animExporter(*this);
   return animExporter.doExport();

   return true;
}
//NiControllerSequenceRef makeSequence(TimeValue start, TimeValue end);

bool AnimationExport::doExport()
{
   INode *root = ne.mI->GetRootNode();
   NiControllerSequenceRef seq = new NiControllerSequence();
   seq->SetStartTime(0.0f);
   seq->SetStopTime(0.0f);
   seq->SetFrequency(1.0f);
   seq->SetCycleType( NiControllerSequence::CYCLE_CLAMP );
   seq->SetTargetName("Bip01");
   
   return true;
}


Control *AnimationExport::GetTMController(INode *n)
{
   Control *c = n->GetTMController();
   if (NULL == c)
      return NULL;

#ifdef USE_BIPED
   // ignore bipeds for now.
   if ( (c->ClassID() == BIPSLAVE_CONTROL_CLASS_ID) 
      ||(c->ClassID() == BIPBODY_CONTROL_CLASS_ID) 
      ||(c->ClassID() == FOOTPRINT_CLASS_ID))
      return NULL;
#endif

   return c;
}


bool AnimationExport::exportController(INode *node, NiControllerSequenceRef seq)
{
   bool ok = true;
   int nKeys = node->NumKeys();
   if (nKeys >= 0)
   {
      float timeOffset = 0.0f;
      NiKeyframeDataRef data = new NiKeyframeData();
      if (Control *tmCont = GetTMController(node))
      {
         if (Control *c = tmCont->GetPositionController()) 
         {
            // separate xyz
            if (c->ClassID() == IPOS_CONTROL_CLASS_ID) 
            { 
               KeyType kType = QUADRATIC_KEY;
               vector<FloatKey> xkeys, ykeys, zkeys;

               if (Control *x = c->GetXController()){
                  if (x->ClassID() == Class_ID(LININTERP_FLOAT_CLASS_ID,0)) {
                     kType = LINEAR_KEY;
                     GetKeys<FloatKey, ILinFloatKey>(x, xkeys, timeOffset);
                  } else if (x->ClassID() == Class_ID(HYBRIDINTERP_FLOAT_CLASS_ID,0)) {
                     kType = QUADRATIC_KEY;
                     GetKeys<FloatKey, IBezFloatKey>(x, xkeys, timeOffset);
                  } else if (x->ClassID() == Class_ID(TCBINTERP_FLOAT_CLASS_ID,0)) {
                     kType = TBC_KEY;
                     GetKeys<FloatKey, ITCBFloatKey>(x, xkeys, timeOffset);
                  } else {
                     kType = QUADRATIC_KEY;
                     GetKeys<FloatKey, IBezFloatKey>(x, xkeys, timeOffset);
                  }
               }
               if (Control *y = c->GetYController()){
                  if (y->ClassID() == Class_ID(LININTERP_FLOAT_CLASS_ID,0)) {
                     GetKeys<FloatKey, ILinFloatKey>(y, ykeys, timeOffset);
                  } else if (y->ClassID() == Class_ID(HYBRIDINTERP_FLOAT_CLASS_ID,0)) {
                     GetKeys<FloatKey, IBezFloatKey>(y, ykeys, timeOffset);
                  } else if (y->ClassID() == Class_ID(TCBINTERP_FLOAT_CLASS_ID,0)) {
                     GetKeys<FloatKey, ITCBFloatKey>(y, ykeys, timeOffset);
                  } else {
                     GetKeys<FloatKey, IBezFloatKey>(y, ykeys, timeOffset);
                  }
               }
               if (Control *z = c->GetZController()){
                  if (z->ClassID() == Class_ID(LININTERP_FLOAT_CLASS_ID,0)) {
                     GetKeys<FloatKey, ILinFloatKey>(z, zkeys, timeOffset);
                  } else if (z->ClassID() == Class_ID(HYBRIDINTERP_FLOAT_CLASS_ID,0)) {
                     GetKeys<FloatKey, IBezFloatKey>(z, zkeys, timeOffset);
                  } else if (z->ClassID() == Class_ID(TCBINTERP_FLOAT_CLASS_ID,0)) {
                     GetKeys<FloatKey, ITCBFloatKey>(z, zkeys, timeOffset);
                  } else {
                     GetKeys<FloatKey, IBezFloatKey>(z, zkeys, timeOffset);
                  }
               }
               vector<Vector3Key> keys;
               JoinKeys(keys, xkeys, ykeys, zkeys);
               data->SetTranslateType(kType);
               data->SetTranslateKeys(keys);

            } else {
               //if (c->ClassID() == Class_ID(LININTERP_FLOAT_CLASS_ID,0)) {
               //   data->SetRotateType(LINEAR_KEY);
               //   GetKeys<FloatKey, ILinFloatKey>(z, zkeys, timeOffset);
               //} else if (c->ClassID() == Class_ID(HYBRIDINTERP_FLOAT_CLASS_ID,0)) {
               //   data->SetZRotateType(QUADRATIC_KEY);
               //   GetKeys<FloatKey, IBezFloatKey>(z, zkeys, timeOffset);
               //} else if (c->ClassID() == Class_ID(TCBINTERP_FLOAT_CLASS_ID,0)) {
               //   data->SetZRotateType(TBC_KEY);
               //   GetKeys<FloatKey, ITCBFloatKey>(z, zkeys, timeOffset);
               //} else {
               //   data->SetZRotateType(QUADRATIC_KEY);
               //   GetKeys<FloatKey, IBezFloatKey>(z, zkeys, timeOffset);
               //}
            }
         }

         //// rotations
         //if (Control *c = tmCont->GetPositionController()) 
         //{
         //   // separate xyz
         //   if (c->ClassID() == IPOS_CONTROL_CLASS_ID) 
         //   { 
         //      if (Control *x = c->GetXController()){
         //         if (x->ClassID() == Class_ID(LININTERP_FLOAT_CLASS_ID,0)) {
         //            data->SetXRotateType(LINEAR_KEY);
         //            GetKeys<FloatKey, ILinFloatKey>(x, xkeys, timeOffset);
         //         } else if (x->ClassID() == Class_ID(HYBRIDINTERP_FLOAT_CLASS_ID,0)) {
         //            data->SetXRotateType(QUADRATIC_KEY);
         //            GetKeys<FloatKey, IBezFloatKey>(x, xkeys, timeOffset);
         //         } else if (x->ClassID() == Class_ID(TCBINTERP_FLOAT_CLASS_ID,0)) {
         //            data->SetXRotateType(TBC_KEY);
         //            GetKeys<FloatKey, ITCBFloatKey>(x, xkeys, timeOffset);
         //         } else {
         //            data->SetXRotateType(QUADRATIC_KEY);
         //            GetKeys<FloatKey, IBezFloatKey>(x, xkeys, timeOffset);
         //         }
         //         data->SetXRotateKeys(xkeys);
         //      }
         //      if (Control *y = c->GetYController()){
         //         KeyType kType = LINEAR_KEY;
         //         vector<FloatKey> ykeys;
         //         if (y->ClassID() == Class_ID(LININTERP_FLOAT_CLASS_ID,0)) {
         //            data->SetYRotateType(LINEAR_KEY);
         //            GetKeys<FloatKey, ILinFloatKey>(y, ykeys, timeOffset);
         //         } else if (y->ClassID() == Class_ID(HYBRIDINTERP_FLOAT_CLASS_ID,0)) {
         //            data->SetYRotateType(QUADRATIC_KEY);
         //            GetKeys<FloatKey, IBezFloatKey>(y, ykeys, timeOffset);
         //         } else if (y->ClassID() == Class_ID(TCBINTERP_FLOAT_CLASS_ID,0)) {
         //            data->SetYRotateType(TBC_KEY);
         //            GetKeys<FloatKey, ITCBFloatKey>(y, ykeys, timeOffset);
         //         } else {
         //            data->SetYRotateType(QUADRATIC_KEY);
         //            GetKeys<FloatKey, IBezFloatKey>(y, ykeys, timeOffset);
         //         }
         //         data->SetYRotateKeys(ykeys);
         //      }
         //      if (Control *z = c->GetZController()){
         //         KeyType kType = LINEAR_KEY;
         //         vector<FloatKey> zkeys;
         //         if (z->ClassID() == Class_ID(LININTERP_FLOAT_CLASS_ID,0)) {
         //            data->SetZRotateType(LINEAR_KEY);
         //            GetKeys<FloatKey, ILinFloatKey>(z, zkeys, timeOffset);
         //         } else if (z->ClassID() == Class_ID(HYBRIDINTERP_FLOAT_CLASS_ID,0)) {
         //            data->SetZRotateType(QUADRATIC_KEY);
         //            GetKeys<FloatKey, IBezFloatKey>(z, zkeys, timeOffset);
         //         } else if (z->ClassID() == Class_ID(TCBINTERP_FLOAT_CLASS_ID,0)) {
         //            data->SetZRotateType(TBC_KEY);
         //            GetKeys<FloatKey, ITCBFloatKey>(z, zkeys, timeOffset);
         //         } else {
         //            data->SetZRotateType(QUADRATIC_KEY);
         //            GetKeys<FloatKey, IBezFloatKey>(z, zkeys, timeOffset);
         //         }
         //         data->SetZRotateKeys(zkeys);
         //      }
         //   } else {
         //      if (c->ClassID() == Class_ID(LININTERP_FLOAT_CLASS_ID,0)) {
         //         data->SetRotateType(LINEAR_KEY);
         //         GetKeys<FloatKey, ILinFloatKey>(z, zkeys, timeOffset);
         //      } else if (c->ClassID() == Class_ID(HYBRIDINTERP_FLOAT_CLASS_ID,0)) {
         //         data->SetZRotateType(QUADRATIC_KEY);
         //         GetKeys<FloatKey, IBezFloatKey>(z, zkeys, timeOffset);
         //      } else if (c->ClassID() == Class_ID(TCBINTERP_FLOAT_CLASS_ID,0)) {
         //         data->SetZRotateType(TBC_KEY);
         //         GetKeys<FloatKey, ITCBFloatKey>(z, zkeys, timeOffset);
         //      } else {
         //         data->SetZRotateType(QUADRATIC_KEY);
         //         GetKeys<FloatKey, IBezFloatKey>(z, zkeys, timeOffset);
         //      }
         //   }
         //}

      }
      //Class_ID c->ClassID()
      //if (Control *subCtrl = MakePositionXYZ(c, Class_ID(LININTERP_FLOAT_CLASS_ID,0))) {
      
      //NiKeyframeDataRef data = new NiKeyframeData();
      //NiKeyframeDataRef data = CreateBlock("NiKeyframeData");
      //data->SetRotateType(QUADRATIC_KEY);
      //data->SetTranslateType(QUADRATIC_KEY);
      //data->SetScaleType(QUADRATIC_KEY);
      //data->SetTranslateKeys( interp->SampleTranslateKeys(npoints, 4) );
      //data->SetQuatRotateKeys( interp->SampleQuatRotateKeys(npoints, 4) );
      //data->SetScaleKeys( interp->SampleScaleKeys(npoints, 4) );
   }
   for (int i=0, n=node->NumberOfChildren(); ok && i<n; ++i)
   {
      INode *child = node->GetChildNode(i);
      ok |= exportController(child, seq);
   }
   return ok;
}