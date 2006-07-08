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

const float FramesPerSecond = 30.0f;
const float FramesIncrement = 1.0f/30.0f;

const int TicksPerFrame = GetTicksPerFrame();

inline TimeValue TimeToFrame(float t) {
   return TimeValue(t * FramesPerSecond * TicksPerFrame);
}

typedef Key<float> FloatKey;
typedef Key<Quaternion> QuatKey;
typedef Key<Vector3> Vector3Key;


template<typename T, typename U>
inline T MapKey(U& key, float time);


template <typename T, typename U>
inline T& InitLinKey(T& rKey, U& key, float time)
{
   rKey.time = TimeToFrame(time + key.time);
   rKey.flags = 0;
   return rKey;
}
template<>
inline ILinFloatKey MapKey<ILinFloatKey,FloatKey>(FloatKey& key, float time)
{
   ILinFloatKey rKey;
   rKey.val = key.data;
   return InitLinKey(rKey, key, time);
}

template<>
inline ILinRotKey MapKey<ILinRotKey, QuatKey>(QuatKey& key, float time)
{
   ILinRotKey rKey;
   rKey.val = TOQUAT(key.data, true);
   return InitLinKey(rKey, key, time);
}

template<>
inline ILinScaleKey MapKey<ILinScaleKey, FloatKey>(FloatKey& key, float time)
{
   ILinScaleKey rKey;
   rKey.val.s.Set(key.data, key.data, key.data);
   rKey.val.q.Identity();
   return InitLinKey(rKey, key, time);
}

template<>
inline ILinPoint3Key MapKey<ILinPoint3Key, Vector3Key>(Vector3Key& key, float time)
{
   ILinPoint3Key rKey;
   rKey.val = TOPOINT3(key.data);
   return InitLinKey(rKey, key, time);
}


template <typename T, typename U>
inline T& InitBezKey(T& rKey, U& key, float time)
{
   rKey.time = TimeToFrame(time + key.time);
   rKey.flags = 0;
   SetInTanType(rKey.flags,BEZKEY_FLAT);
   SetOutTanType(rKey.flags,BEZKEY_FLAT);
   return rKey;
}
template<>
inline IBezFloatKey MapKey<IBezFloatKey, FloatKey>(FloatKey& key, float time)
{
   IBezFloatKey rKey;
   rKey.val = key.data;
   rKey.intan = key.forward_tangent;
   rKey.outtan = key.backward_tangent;
   return InitBezKey(rKey, key, time);
}

template<>
inline IBezPoint3Key MapKey<IBezPoint3Key, Vector3Key>(Vector3Key& key, float time)
{
   IBezPoint3Key rKey;
   rKey.val = TOPOINT3(key.data);
   rKey.intan = TOPOINT3(key.forward_tangent);
   rKey.outtan = TOPOINT3(key.backward_tangent);
   return InitBezKey(rKey, key, time);
}


template<>
inline IBezQuatKey MapKey<IBezQuatKey, QuatKey>(QuatKey& key, float time)
{
   IBezQuatKey rKey;
   rKey.val = TOQUAT(key.data, true);
   return InitBezKey(rKey, key, time);
}

template<>
inline IBezScaleKey MapKey<IBezScaleKey, FloatKey>(FloatKey& key, float time)
{
   IBezScaleKey rKey;
   rKey.val.s.Set(key.data, key.data, key.data);
   rKey.val.q.Identity();
   return InitBezKey(rKey, key, time);
}

template <typename T, typename U>
inline T& InitTCBKey(T& rKey, U& key, float time)
{
   rKey.time = TimeToFrame(time + key.time);
   rKey.tens = key.tension;
   rKey.cont = key.continuity;
   rKey.bias = key.bias;
   rKey.easeIn = 0.0f;
   rKey.easeOut = 0.0f;
   return rKey;
}

template<>
inline ITCBFloatKey MapKey<ITCBFloatKey, FloatKey>(FloatKey& key, float time)
{
   ITCBFloatKey rKey;
   rKey.val = key.data;
   return InitTCBKey(rKey, key, time);
}

template<>
inline ITCBRotKey MapKey<ITCBRotKey, QuatKey>(QuatKey& key, float time)
{
   ITCBRotKey rKey;
   rKey.val = TOQUAT(key.data, true);
   InitTCBKey(rKey, key, time);
   rKey.flags = TCBKEY_QUATVALID;
   return rKey;
}

template<>
inline ITCBPoint3Key MapKey<ITCBPoint3Key, Vector3Key>(Vector3Key& key, float time)
{
   ITCBPoint3Key rKey;
   rKey.val = TOPOINT3(key.data);
   return InitTCBKey(rKey, key, time);
}

template<>
inline ITCBScaleKey MapKey<ITCBScaleKey, FloatKey>(FloatKey& key, float time)
{
   ITCBScaleKey rKey;
   rKey.val.s.Set(key.data, key.data, key.data);
   rKey.val.q.Identity();
   return InitTCBKey(rKey, key, time);
}

template<typename T, typename U>
inline void SetKeys(Control *subCtrl, vector<U>& keys, float time)
{
   if (subCtrl && !keys.empty()){
      if (IKeyControl *ikeys = GetKeyControlInterface(subCtrl)){
         ikeys->SetNumKeys(keys.size());
         for (int i=0,n=keys.size(); i<n; ++i) {
            ikeys->SetKey(i, &MapKey<T>(keys[i], time));
         }
      }
   }
}

enum V3T
{
   V3T_X, V3T_Y, V3T_Z
};
inline float GetValue(Vector3& value, V3T type)
{
   switch (type) {
   case V3T_X: return value.x;
   case V3T_Y: return value.y;
   case V3T_Z: return value.z;
   }
   return 0.0f;
}

FloatKey& CopyKey( FloatKey& dst, Vector3Key& src, V3T type)
{
   dst.time = src.time;
   dst.bias = src.bias;
   dst.continuity = src.continuity;
   dst.tension = src.tension;
   dst.backward_tangent = GetValue(src.backward_tangent, type);
   dst.forward_tangent = GetValue(src.forward_tangent, type);
   dst.data = GetValue(src.data, type);
   return dst;
}

void SplitKeys(vector<Vector3Key>&keys, vector<FloatKey>&xkeys, vector<FloatKey>&ykeys, vector<FloatKey>&zkeys)
{
   int n = keys.size();
   xkeys.resize(n), ykeys.resize(n), zkeys.resize(n);
   for (int i=0,n=keys.size(); i<n; ++i) {
      Vector3Key& key = keys[i];
      CopyKey(xkeys[i], key, V3T_X), CopyKey(ykeys[i], key, V3T_Y), CopyKey(zkeys[i], key, V3T_Z);
   }
}

typedef Key<string> KeyTextValue;

struct AnimationImport
{
   AnimationImport(NifImporter& parent) : ni(parent) {}

   NifImporter &ni;

   vector<KeyTextValue> BuildKeyValues(NiObjectNETRef nref);
   bool AddValues(NiObjectNETRef nref);
   bool AddValues(vector<NiObjectNETRef>& nodes);

   bool AddValues(Control *c, NiKeyframeDataRef data, float time);

   Control* MakePosition(Control *tmCont, Class_ID clsid);
   Control* MakePositionXYZ(Control *tmCont, Class_ID clsid);
   Control* MakeRotation(Control *tmCont, Class_ID rotClsid, Class_ID rollClsid);
   Control* MakeScale(Control *tmCont, Class_ID clsid);

   Control* GetTMController(const string& name);
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

bool KFMImporter::ImportAnimation()
{
   bool ok = false;
   int curFrame = 0;
   // Read Kf files

   AnimationImport ai(*this);

   float time = FramesIncrement;
   for(vector<NiControllerSequenceRef>::iterator itr = kf.begin(); itr != kf.end(); ++itr){

      float minTime = 1e+35f;
      float maxTime = 0.0f;

      NiControllerSequenceRef cntr = (*itr);
      vector<ControllerLink> links = cntr->GetControllerData();
      for (vector<ControllerLink>::iterator lnk=links.begin(); lnk != links.end(); ++lnk){

         string name = (*lnk).targetName;
         if (name.empty()) {
            NiStringPaletteRef strings = lnk->stringPalette;
            name = strings->GetSubStr((*lnk).nodeNameOffset);
         }
         if (name.empty())
            continue;

         Control *c = ai.GetTMController(name);
         if (NULL == c)
            continue;

         float start = cntr->GetStartTime();
         float stop = cntr->GetStopTime();
         float total = (stop - start);
         if ((*lnk).interpolator){
            if (NiTransformInterpolatorRef interp = (*lnk).interpolator) {
               if (NiTransformDataRef data = interp->GetData()){
                  if (ai.AddValues(c, data, time)) {
                     minTime = min(minTime, start);
                     maxTime = max(maxTime, stop);
                     ok = true;
                  }
               }
            } else if (NiBSplineCompTransformInterpolatorRef interp = (*lnk).interpolator) {
               int npoints = total * 60.0f;

               NiKeyframeDataRef data = CreateBlock("NiKeyframeData");
               data->SetRotateType(QUADRATIC_KEY);
               data->SetTranslateType(QUADRATIC_KEY);
               data->SetScaleType(QUADRATIC_KEY);
               data->SetTranslateKeys( interp->SampleTranslateKeys(npoints, 4) );
               data->SetQuatRotateKeys( interp->SampleQuatRotateKeys(npoints, 4) );
               data->SetScaleKeys( interp->SampleScaleKeys(npoints, 4) );

               if (ai.AddValues(c, data, time)) {
                  minTime = min(minTime, start);
                  maxTime = max(maxTime, stop);
                  ok = true;
               }
            }
         } else if ((*lnk).controller) {
            if (NiTransformControllerRef tc = DynamicCast<NiTransformController>((*lnk).controller)) {
               if (NiTransformInterpolatorRef interp = tc->GetInterpolator()) {
                  if (NiTransformDataRef data = interp->GetData()){
                     if (ai.AddValues(c, data, time)) {
                        minTime = min(minTime, start);
                        maxTime = max(maxTime, stop);
                        ok = true;
                     }
                  }
               }
            } else if (NiKeyframeControllerRef kfc = DynamicCast<NiKeyframeController>((*lnk).controller)) {
               if (NiKeyframeDataRef data = kfc->GetData()) {
                  if (ai.AddValues(c, data, time)) {
                     minTime = min(minTime, start);
                     maxTime = max(maxTime, stop);
                     ok = true;
                  }
               }
            }
         }
      }
      if (maxTime > minTime && maxTime > 0.0f)
         time += (maxTime-minTime) + FramesIncrement;
   }
   return ok;
}

Control *AnimationImport::GetTMController(const string& name)
{
   INode *n = ni.gi->GetINodeByName(name.c_str());
   if (NULL == n)
      return NULL;

   Control *c = n->GetTMController();
   if (NULL == c)
      return NULL;

   // ignore bipeds for now.
   if ( (c->ClassID() == BIPSLAVE_CONTROL_CLASS_ID) 
      ||(c->ClassID() == BIPBODY_CONTROL_CLASS_ID) 
      ||(c->ClassID() == FOOTPRINT_CLASS_ID))
      return NULL;

   return c;
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
         SetKeys<ILinFloatKey, FloatKey>(subCtrl->GetXController(), xkeys, time);
         SetKeys<ILinFloatKey, FloatKey>(subCtrl->GetYController(), ykeys, time);
         SetKeys<ILinFloatKey, FloatKey>(subCtrl->GetZController(), zkeys, time);
      }
      break;

   case QUADRATIC_KEY:
   case XYZ_ROTATION_KEY:
      if (Control *subCtrl = MakePositionXYZ(c, Class_ID(HYBRIDINTERP_FLOAT_CLASS_ID,0))) {
         vector<FloatKey> xkeys, ykeys, zkeys;
         SplitKeys(posKeys, xkeys, ykeys, zkeys);
         SetKeys<IBezFloatKey, FloatKey>(subCtrl->GetXController(), xkeys, time);
         SetKeys<IBezFloatKey, FloatKey>(subCtrl->GetYController(), ykeys, time);
         SetKeys<IBezFloatKey, FloatKey>(subCtrl->GetZController(), zkeys, time);
      }
      break;

   case TBC_KEY:
      if (Control *subCtrl = MakePositionXYZ(c, Class_ID(TCBINTERP_FLOAT_CLASS_ID,0))) {
         vector<FloatKey> xkeys, ykeys, zkeys;
         SplitKeys(posKeys, xkeys, ykeys, zkeys);
         SetKeys<ITCBFloatKey, FloatKey>(subCtrl->GetXController(), xkeys, time);
         SetKeys<ITCBFloatKey, FloatKey>(subCtrl->GetYController(), ykeys, time);
         SetKeys<ITCBFloatKey, FloatKey>(subCtrl->GetZController(), zkeys, time);
      }
      break;
   }

   // Handle Rotation
   switch (data->GetRotateType())
   {
   case LINEAR_KEY:
      if (Control *subCtrl = MakeRotation(c, Class_ID(LININTERP_ROTATION_CLASS_ID,0), Class_ID(LININTERP_FLOAT_CLASS_ID,0))) {
         SetKeys<ILinRotKey, QuatKey>(subCtrl, quatKeys, time);
      }
      break;

   case QUADRATIC_KEY:
      if (Control *subCtrl = MakeRotation(c, Class_ID(HYBRIDINTERP_ROTATION_CLASS_ID,0), Class_ID(HYBRIDINTERP_FLOAT_CLASS_ID,0))) {
         SetKeys<IBezQuatKey, QuatKey>(subCtrl, quatKeys, time);
      }
      break;

   case XYZ_ROTATION_KEY:
      if (Control *subCtrl = MakeRotation(c, Class_ID(EULER_CONTROL_CLASS_ID,0), Class_ID(HYBRIDINTERP_FLOAT_CLASS_ID,0))) {
         SetKeys<IBezFloatKey, FloatKey>(subCtrl->GetXController(), xKeys, time);
         SetKeys<IBezFloatKey, FloatKey>(subCtrl->GetYController(), yKeys, time);
         SetKeys<IBezFloatKey, FloatKey>(subCtrl->GetZController(), zKeys, time);
      }
      break;

   case TBC_KEY:
      if (ni.replaceTCBRotationWithBezier) {
         // TCB simply is not working for me.  Better off with Bezier as a workaround
         if (Control *subCtrl = MakeRotation(c, Class_ID(HYBRIDINTERP_ROTATION_CLASS_ID,0), Class_ID(HYBRIDINTERP_FLOAT_CLASS_ID,0))) {
            SetKeys<IBezQuatKey, QuatKey>(subCtrl, quatKeys, time);
         }
      } else {
         if (Control *subCtrl = MakeRotation(c, Class_ID(TCBINTERP_ROTATION_CLASS_ID,0), Class_ID(TCBINTERP_FLOAT_CLASS_ID,0))) {
            SetKeys<ITCBRotKey, QuatKey>(subCtrl, quatKeys, time);
         }
      }
      break;
   }
   // Handle Scale
   switch (data->GetScaleType())
   {
   case LINEAR_KEY:
      if (Control *subCtrl = MakeScale(c, Class_ID(LININTERP_SCALE_CLASS_ID,0))) {
         SetKeys<ILinScaleKey, FloatKey>(subCtrl, sclKeys, time);
      }
      break;
   case QUADRATIC_KEY:
   case XYZ_ROTATION_KEY:
      if (Control *subCtrl = MakeScale(c, Class_ID(HYBRIDINTERP_SCALE_CLASS_ID,0))) {
         SetKeys<IBezScaleKey, FloatKey>(subCtrl, sclKeys, time);
      }
      break;
   case TBC_KEY:
      if (Control *subCtrl = MakeScale(c, Class_ID(TCBINTERP_SCALE_CLASS_ID,0))) {
         SetKeys<ITCBScaleKey, FloatKey>(subCtrl, sclKeys, time);
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
