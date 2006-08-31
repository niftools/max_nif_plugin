/**********************************************************************
*<
FILE: ImportAnimation.cpp

DESCRIPTION:	Animation Import Routines

CREATED BY: tazpn (Theo)

HISTORY: 

*>	Copyright (c) 2006, All Rights Reserved.
**********************************************************************/
#include "stdafx.h"
#include "MaxNifImport.h"
#include "NIFImporter.h"
#include "KFMImporter.h"
#include "KFImporter.h"
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

#include "AnimKey.h"

struct AnimationImport
{
   AnimationImport(NifImporter& parent) : ni(parent) {}

   NifImporter &ni;

   vector<KeyTextValue> BuildKeyValues(NiObjectNETRef nref);
   bool AddValues(NiObjectNETRef nref);
   bool AddValues(vector<NiObjectNETRef>& nodes);

   bool AddValues(Control *c, NiKeyframeDataRef data, float time);
   bool AddBiped(Control *c, NiKeyframeDataRef data, float time);

   Control* MakePosition(Control *tmCont, Class_ID clsid);
   Control* MakePositionXYZ(Control *tmCont, Class_ID clsid);
   Control* MakeRotation(Control *tmCont, Class_ID rotClsid, Class_ID rollClsid);
   Control* MakeScale(Control *tmCont, Class_ID clsid);

   Control* GetTMController(const string& name);
   Matrix3 GetTM(const string& name, TimeValue t = 0);

   bool GetTransformData(ControllerLink& lnk, string name, NiKeyframeDataRef& outData, Point3& p, Quat& q, float& s);
};

bool NifImporter::ImportAnimation()
{
   if (!enableAnimations)
      return false;

   if (nodes.empty())
      return false;

   NiNodeRef rootNode = root;
   AnimationImport ai(*this);
   return ai.AddValues(DynamicCast<NiObjectNET>(rootNode->GetChildren()));
}

static vector<ControllerLink>::iterator FindLink(string name, vector<ControllerLink>& links)
{
   for (vector<ControllerLink>::iterator lnk=links.begin(); lnk != links.end(); )
   {
      string target = (*lnk).targetName;
      if (target.empty()) {
         NiStringPaletteRef strings = lnk->stringPalette;
         target = strings->GetSubStr((*lnk).nodeNameOffset);
      }
      if (target == name) {
         return lnk;
      }
   }
   return links.end();
}

static void ClearAnimation(Control *c)
{
   if (c != NULL)
   {
      if (c->IsColorController())
         return;

      if (IKeyControl *ikeys = GetKeyControlInterface(c)){
         ikeys->SetNumKeys(0);
      }
      if (Control *sc = c->GetWController()) { 
         if (sc != c) ClearAnimation(sc); 
      }
      if (Control *sc = c->GetXController()) { 
         if (sc != c) ClearAnimation(sc); 
      }
      if (Control *sc = c->GetYController()) { 
         if (sc != c) ClearAnimation(sc); 
      }
      if (Control *sc = c->GetZController()) { 
         if (sc != c) ClearAnimation(sc); 
      }
      if (Control *sc = c->GetRotationController()) { 
         if (sc != c) ClearAnimation(sc); 
      }
      if (Control *sc = c->GetPositionController()) { 
         if (sc != c) ClearAnimation(sc); 
      }
      if (Control *sc = c->GetScaleController()) { 
         if (sc != c) ClearAnimation(sc); 
      }
   }
}

void NifImporter::ClearAnimation(INode *node)
{
   if (node != NULL)
   {
      node->DeleteKeys(TRACK_DOALL);
      ::ClearAnimation(node->GetTMController());
      for (int i=0, n=node->NumberOfChildren(); i<n; ++i){
         ClearAnimation(node->GetChildNode(i));
      }
   }
}

bool KFMImporter::ImportAnimation()
{
   bool ok = false;
   int curFrame = 0;
   // Read Kf files

   AnimationImport ai(*this);

   float time = 0.0f;
   for(vector<NiControllerSequenceRef>::iterator itr = kf.begin(); itr != kf.end(); ++itr){

      float minTime = 1e+35f;
      float maxTime = 0.0f;

      NiControllerSequenceRef cntr = (*itr);
      vector<ControllerLink> links = cntr->GetControllerData();

      for (vector<ControllerLink>::iterator lnk=links.begin(); lnk != links.end(); ++lnk)
      {
         string name = (*lnk).targetName;
         if (name.empty()) {
            NiStringPaletteRef strings = lnk->stringPalette;
            name = strings->GetSubStr((*lnk).nodeNameOffset);
         }
         if (name.empty())
            continue;

         // I realize this is not the best way to do this but it works for some files
         if (mergeNonAccum && wildmatch("* NonAccum", name)) 
         {
            name = name.substr(0, name.length() - 9);
         }

         Control *c = ai.GetTMController(name);
         if (NULL == c)
            continue;

         INode *n = gi->GetINodeByName(name.c_str());

         float start = cntr->GetStartTime();
         float stop = cntr->GetStopTime();
         float total = (stop - start);

         NiKeyframeDataRef data;
         Point3 p; Quat q; float s;
         if (ai.GetTransformData(*lnk, name, data, p, q, s)) {
            PosRotScaleNode(n, p, q, s, prsDefault, 0);
            if (ai.AddValues(c, data, time)) {
               minTime = min(minTime, start);
               maxTime = max(maxTime, stop);
               ok = true;
            }
         }
      }
      if (maxTime > minTime && maxTime > 0.0f)
         time += (maxTime-minTime) + FramesIncrement;
   }

   if (time != 0.0f)
   {
      Interval range(0, TimeToFrame(time));
      gi->SetAnimRange(range);
   }

   return ok;
}

bool AnimationImport::GetTransformData(ControllerLink& lnk, string name, NiKeyframeDataRef& outData, Point3& p, Quat& q, float& s)
{
   Control *c = GetTMController(name);
   if (NULL == c)
      return false;

   if (lnk.interpolator){
      if (NiTransformInterpolatorRef interp = lnk.interpolator) {
         if (NiTransformDataRef data = interp->GetData()){
            outData = StaticCast<NiKeyframeData>(data);

            // Set initial conditions
            p = TOPOINT3(interp->GetTranslation());
            q = TOQUAT(interp->GetRotation(), true);
            s = interp->GetScale();
            return true;
         }
      } else if (NiBSplineCompTransformInterpolatorRef interp = lnk.interpolator) {
         int npoints = interp->GetNumControlPt();

         if (npoints > 3)
         {
            NiKeyframeDataRef data = CreateBlock("NiKeyframeData");
            data->SetRotateType(QUADRATIC_KEY);
            data->SetTranslateType(QUADRATIC_KEY);
            data->SetScaleType(QUADRATIC_KEY);
            data->SetTranslateKeys( interp->SampleTranslateKeys(npoints, 3) );
            data->SetQuatRotateKeys( interp->SampleQuatRotateKeys(npoints, 3) );
            data->SetScaleKeys( interp->SampleScaleKeys(npoints, 3) );

            outData = data;

            p = TOPOINT3(interp->GetTranslation());
            q = TOQUAT(interp->GetRotation(), true);
            s = interp->GetScale();
            return true;
         }
      }
   } else if (lnk.controller) {
      if (NiTransformControllerRef tc = DynamicCast<NiTransformController>(lnk.controller)) {
         if (NiTransformInterpolatorRef interp = tc->GetInterpolator()) {
            if (NiTransformDataRef data = interp->GetData()){
               outData = StaticCast<NiKeyframeData>(data);

               p = TOPOINT3(interp->GetTranslation());
               q = TOQUAT(interp->GetRotation(), true);
               s = interp->GetScale();
               return true;
            }
         }
      } else if (NiKeyframeControllerRef kfc = DynamicCast<NiKeyframeController>(lnk.controller)) {
         if (NiKeyframeDataRef data = kfc->GetData()) {
            outData = data;
            // values will not be used in transforms
            p = Point3(FloatNegINF, FloatNegINF, FloatNegINF);
            q = Quat(FloatNegINF, FloatNegINF, FloatNegINF, FloatNegINF);
            s = FloatNegINF;
            return true;
         }
      }
   }
   return false;
}


Control *AnimationImport::GetTMController(const string& name)
{
   INode *n = ni.gi->GetINodeByName(name.c_str());
   if (NULL == n)
      return NULL;

   Control *c = n->GetTMController();
   if (NULL == c)
      return NULL;

   return c;
}

Matrix3 AnimationImport::GetTM(const string& name, TimeValue t)
{
   INode *n = ni.gi->GetINodeByName(name.c_str());
   if (NULL == n)
      return Matrix3(true);

   return n->GetObjTMAfterWSM(t);
}

vector<KeyTextValue> AnimationImport::BuildKeyValues(NiObjectNETRef nref)
{
   if (NiTextKeyExtraDataRef keydata = SelectFirstObjectOfType<NiTextKeyExtraData>(nref->GetExtraData()))
      return keydata->GetKeys();
   return vector<KeyTextValue>();
}

bool AnimationImport::AddValues(vector<NiObjectNETRef>& nodes)
{
   bool ok = false;
   for (vector<NiObjectNETRef>::iterator itr = nodes.begin(), end = nodes.end(); itr != end; ++itr)
   {
      ok |= AddValues(*itr);
      NiNodeRef ninode = (*itr);
      if (ninode)
         ok |= AddValues(DynamicCast<NiObjectNET>(ninode->GetChildren()));
   }
   return ok;
}

bool AnimationImport::AddValues(NiObjectNETRef nref)
{
   Control *c = GetTMController(nref->GetName().c_str());
   if (NULL == c)
      return false;

   float time = FramesIncrement;
   list< NiTimeControllerRef > clist = nref->GetControllers();
   if (NiTransformControllerRef tc = SelectFirstObjectOfType<NiTransformController>(clist)) {
      if (NiTransformInterpolatorRef interp = tc->GetInterpolator()) {
         if (NiTransformDataRef data = interp->GetData()) {
            return AddValues(c, data, time);
         }
      }
   } else if (NiKeyframeControllerRef kf = SelectFirstObjectOfType<NiKeyframeController>(clist)) {
      if (NiKeyframeDataRef kfData = kf->GetData()) {
         return AddValues(c, kfData, time);
      }
   }
   return false;
}

bool AnimationImport::AddValues(Control *c, NiKeyframeDataRef data, float time)
{
#ifdef USE_BIPED
   // Bipeds are special.  And will crash if you dont treat them with care
   if ( (c->ClassID() == BIPSLAVE_CONTROL_CLASS_ID) 
      ||(c->ClassID() == BIPBODY_CONTROL_CLASS_ID) 
      ||(c->ClassID() == FOOTPRINT_CLASS_ID))
   {
      return AddBiped(c, data, time);
   }
#endif

   vector<Vector3Key> posKeys = data->GetTranslateKeys();
   vector<QuatKey> quatKeys = data->GetQuatRotateKeys();
   vector<FloatKey> sclKeys = data->GetScaleKeys();
   vector<FloatKey> xKeys = data->GetXRotateKeys();
   vector<FloatKey> yKeys = data->GetYRotateKeys();
   vector<FloatKey> zKeys = data->GetZRotateKeys();

   // Require more than one key to import (to avoid zero frame positioning used in mw and daoc
   if (ni.requireMultipleKeys && 
      !( posKeys.size() > 1
      || quatKeys.size() > 1
      || sclKeys.size() > 1
      || xKeys.size() > 1
      || yKeys.size() > 1
      || zKeys.size() > 1
      ))
   {
      return false;
   }

   // Handle Translation
   switch (data->GetTranslateType())
   {
   case LINEAR_KEY:
      if (Control *subCtrl = MakePositionXYZ(c, Class_ID(LININTERP_FLOAT_CLASS_ID,0))) {
         vector<FloatKey> xkeys, ykeys, zkeys;
         SplitKeys(posKeys, xkeys, ykeys, zkeys);
         MergeKeys<ILinFloatKey, FloatKey>(subCtrl->GetXController(), xkeys, time);
         MergeKeys<ILinFloatKey, FloatKey>(subCtrl->GetYController(), ykeys, time);
         MergeKeys<ILinFloatKey, FloatKey>(subCtrl->GetZController(), zkeys, time);
      }
      break;

   case QUADRATIC_KEY:
   case XYZ_ROTATION_KEY:
      if (Control *subCtrl = MakePositionXYZ(c, Class_ID(HYBRIDINTERP_FLOAT_CLASS_ID,0))) {
         vector<FloatKey> xkeys, ykeys, zkeys;
         SplitKeys(posKeys, xkeys, ykeys, zkeys);
         MergeKeys<IBezFloatKey, FloatKey>(subCtrl->GetXController(), xkeys, time);
         MergeKeys<IBezFloatKey, FloatKey>(subCtrl->GetYController(), ykeys, time);
         MergeKeys<IBezFloatKey, FloatKey>(subCtrl->GetZController(), zkeys, time);
      }
      break;

   case TBC_KEY:
      if (Control *subCtrl = MakePositionXYZ(c, Class_ID(TCBINTERP_FLOAT_CLASS_ID,0))) {
         vector<FloatKey> xkeys, ykeys, zkeys;
         SplitKeys(posKeys, xkeys, ykeys, zkeys);
         MergeKeys<ITCBFloatKey, FloatKey>(subCtrl->GetXController(), xkeys, time);
         MergeKeys<ITCBFloatKey, FloatKey>(subCtrl->GetYController(), ykeys, time);
         MergeKeys<ITCBFloatKey, FloatKey>(subCtrl->GetZController(), zkeys, time);
      }
      break;
   }

   // Handle Rotation
   switch (data->GetRotateType())
   {
   case LINEAR_KEY:
      if (Control *subCtrl = MakeRotation(c, Class_ID(LININTERP_ROTATION_CLASS_ID,0), Class_ID(LININTERP_FLOAT_CLASS_ID,0))) {
         MergeKeys<ILinRotKey, QuatKey>(subCtrl, quatKeys, time);
      }
      break;

   case QUADRATIC_KEY:
      if (Control *subCtrl = MakeRotation(c, Class_ID(HYBRIDINTERP_ROTATION_CLASS_ID,0), Class_ID(HYBRIDINTERP_FLOAT_CLASS_ID,0))) {
         MergeKeys<IBezQuatKey, QuatKey>(subCtrl, quatKeys, time);
      }
      break;

   case XYZ_ROTATION_KEY:
      if (Control *subCtrl = MakeRotation(c, Class_ID(EULER_CONTROL_CLASS_ID,0), Class_ID(HYBRIDINTERP_FLOAT_CLASS_ID,0))) {
         MergeKeys<IBezFloatKey, FloatKey>(subCtrl->GetXController(), xKeys, time);
         MergeKeys<IBezFloatKey, FloatKey>(subCtrl->GetYController(), yKeys, time);
         MergeKeys<IBezFloatKey, FloatKey>(subCtrl->GetZController(), zKeys, time);
      }
      break;

   case TBC_KEY:
      if (ni.replaceTCBRotationWithBezier) {
         // TCB simply is not working for me.  Better off with Bezier as a workaround
         if (Control *subCtrl = MakeRotation(c, Class_ID(HYBRIDINTERP_ROTATION_CLASS_ID,0), Class_ID(HYBRIDINTERP_FLOAT_CLASS_ID,0))) {
            MergeKeys<IBezQuatKey, QuatKey>(subCtrl, quatKeys, time);
         }
      } else {
         if (Control *subCtrl = MakeRotation(c, Class_ID(TCBINTERP_ROTATION_CLASS_ID,0), Class_ID(TCBINTERP_FLOAT_CLASS_ID,0))) {
            MergeKeys<ITCBRotKey, QuatKey>(subCtrl, quatKeys, time);
         }
      }
      break;
   }
   // Handle Scale
   switch (data->GetScaleType())
   {
   case LINEAR_KEY:
      if (Control *subCtrl = MakeScale(c, Class_ID(LININTERP_SCALE_CLASS_ID,0))) {
         MergeKeys<ILinScaleKey, FloatKey>(subCtrl, sclKeys, time);
      }
      break;
   case QUADRATIC_KEY:
   case XYZ_ROTATION_KEY:
      if (Control *subCtrl = MakeScale(c, Class_ID(HYBRIDINTERP_SCALE_CLASS_ID,0))) {
         MergeKeys<IBezScaleKey, FloatKey>(subCtrl, sclKeys, time);
      }
      break;
   case TBC_KEY:
      if (Control *subCtrl = MakeScale(c, Class_ID(TCBINTERP_SCALE_CLASS_ID,0))) {
         MergeKeys<ITCBScaleKey, FloatKey>(subCtrl, sclKeys, time);
      }
      break;
   }
   return true;
}

Control* AnimationImport::MakeRotation(Control *tmCont, Class_ID rotClsid, Class_ID rollClsid)
{
   Interface *ip = ni.gi;
   if (Control *c = tmCont->GetRotationController()) {
      if (c->ClassID()!=rotClsid) {
         if (Control *tmpCtrl = (Control*)ip->CreateInstance(CTRL_ROTATION_CLASS_ID, rotClsid)) {
            if (!tmCont->SetRotationController(tmpCtrl))
               tmpCtrl->DeleteThis();
            else
               c = tmpCtrl;
         }
      }
      if (Control *r = tmCont->GetRollController()) {
         if (r->ClassID()!=rollClsid) {
            if (Control *r = (Control*)ip->CreateInstance(CTRL_FLOAT_CLASS_ID,rollClsid))
               if (!tmCont->SetRollController(r))
                  r->DeleteThis();
         }
      }
      return c;
   }
   return NULL;
}

Control* AnimationImport::MakeScale(Control *tmCont, Class_ID clsid)
{
   Interface *ip = ni.gi;
   if (Control *c = tmCont->GetScaleController()) {
      if (c->ClassID()!=clsid) {
         if (Control *tmpCtrl = (Control*)ip->CreateInstance(CTRL_SCALE_CLASS_ID, clsid)){
            if (!tmCont->SetScaleController(tmpCtrl))
               tmpCtrl->DeleteThis();
            else
               c = tmpCtrl;
         }
      }
      return c;
   }
   return NULL;
}

Control* AnimationImport::MakePosition(Control *tmCont, Class_ID clsid)
{
   Interface *ip = ni.gi;
   if (Control *c = tmCont->GetPositionController()) {
      if (c->ClassID()!=clsid) {
         if (Control *tmpCtrl = (Control*)ip->CreateInstance(CTRL_POSITION_CLASS_ID, clsid)){
            if (!tmCont->SetPositionController(tmpCtrl))
               tmpCtrl->DeleteThis();
            else
               c = tmpCtrl;
         }
      }
      return c;
   }
   return NULL;
}

Control* AnimationImport::MakePositionXYZ(Control *tmCont, Class_ID clsid)
{
   Interface *ip = ni.gi;
   // First make the controller and XYZ Independent position controller, then fix individuals
   if (Control *c = MakePosition(tmCont, IPOS_CONTROL_CLASS_ID)){
      if (Control *x = c->GetXController()){
         if (x->ClassID()!= clsid) {
            if (Control *tmp = (Control*)ip->CreateInstance(CTRL_FLOAT_CLASS_ID, clsid))
               c->SetReference(IPOS_X_REF, tmp);
         }
      }
      if (Control *y = c->GetYController()){
         if (y->ClassID()!= clsid) {
            if (Control *tmp = (Control*)ip->CreateInstance(CTRL_FLOAT_CLASS_ID, clsid))
               c->SetReference(IPOS_Y_REF, tmp);
         }
      }
      if (Control *z = c->GetZController()){
         if (z->ClassID()!= clsid) {
            if (Control *tmp = (Control*)ip->CreateInstance(CTRL_FLOAT_CLASS_ID, clsid))
               c->SetReference(IPOS_Z_REF, tmp);
         }
      }
      return c;
   }
   return NULL;
}


#ifdef USE_BIPED
bool AnimationImport::AddBiped(Control *c, NiKeyframeDataRef data, float time)
{
   return false;
}
#endif