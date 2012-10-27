/**********************************************************************
*<
FILE: AnimKey.h

DESCRIPTION:	Animation Key Import Routines

CREATED BY: tazpn (Theo)

HISTORY: 

*>	Copyright (c) 2006, All Rights Reserved.
**********************************************************************/
#include "pch.h"
// Max Headers
#include <Max.h>
#include <control.h>
// Niflib Headers
#include <niflib.h>
#include <Key.h>

#include "AnimKey.h"
#include "niutils.h"


const Class_ID IPOS_CONTROL_CLASS_ID = Class_ID(0x118f7e02,0xffee238a);
enum {
   IPOS_X_REF	=	0,
   IPOS_Y_REF	=	1,
   IPOS_Z_REF	=	2,
   IPOS_W_REF	=	3,
};

template <typename T, typename U>
T& InitLinKey(T& rKey, U& key, float time)
{
   rKey.time = TimeToFrame(time + key.time);
   rKey.flags = 0;
   return rKey;
}

template <typename T, typename U>
T& InitFromLinKey(T& rKey, U& key, float time)
{
   rKey.time = time + FrameToTime(key.time);
   return rKey;
}

template <typename T, typename U>
T& InitBezKey(T& rKey, U& key, float time)
{
   rKey.time = TimeToFrame(time + key.time);
   rKey.flags = 0;
#if VERSION_3DSMAX >= ((5000<<16)+(15<<8)+0) // Version 5+
   SetInTanType(rKey.flags,BEZKEY_FLAT);
   SetOutTanType(rKey.flags,BEZKEY_FLAT);
#endif
   return rKey;
}


template <typename T, typename U>
T& InitFromBezKey(T& rKey, U& key, float time)
{
   rKey.time = time + FrameToTime(key.time);
   return rKey;
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

template <typename T, typename U>
inline T& InitFromTCBKey(T& rKey, U& key, float time)
{
   rKey.time = time + FrameToTime(key.time);
   rKey.tension = key.tens;
   rKey.continuity = key.cont;
   rKey.bias = key.bias;
   return rKey;
}

// Specialized Linear Mappings

template<>
ILinFloatKey MapKey<ILinFloatKey,FloatKey>(FloatKey& key, float time)
{
   ILinFloatKey rKey;
   rKey.val = key.data;
   return InitLinKey(rKey, key, time);
}

template<>
ILinRotKey MapKey<ILinRotKey, QuatKey>(QuatKey& key, float time)
{
   ILinRotKey rKey;
   rKey.val = TOQUAT(key.data, true);
   return InitLinKey(rKey, key, time);
}

template<>
ILinScaleKey MapKey<ILinScaleKey, FloatKey>(FloatKey& key, float time)
{
   ILinScaleKey rKey;
   rKey.val.s.Set(key.data, key.data, key.data);
   rKey.val.q.Identity();
   return InitLinKey(rKey, key, time);
}

template<>
ILinPoint3Key MapKey<ILinPoint3Key, Vector3Key>(Vector3Key& key, float time)
{
   ILinPoint3Key rKey;
   rKey.val = TOPOINT3(key.data);
   return InitLinKey(rKey, key, time);
}


// Specialized Linear Mappings

template<>
FloatKey MapKey<FloatKey, ILinFloatKey>(ILinFloatKey& key, float time)
{
   FloatKey rKey;
   rKey.data = key.val;
   return InitFromLinKey(rKey, key, time);
}

template<>
Vector3Key MapKey<Vector3Key, ILinPoint3Key>(ILinPoint3Key& key, float time)
{
   Vector3Key rKey;
   rKey.data = TOVECTOR3(key.val);
   return InitFromLinKey(rKey, key, time);
}

template<>
QuatKey MapKey<QuatKey, ILinRotKey>(ILinRotKey& key, float time)
{
   QuatKey rKey;
   rKey.data = TOQUAT(key.val, true);
   return InitFromLinKey(rKey, key, time);
}

template<>
FloatKey MapKey<FloatKey, ILinScaleKey>(ILinScaleKey& key, float time)
{
   FloatKey rKey;
   rKey.data = (key.val[0] + key.val[1] + key.val[2]) / 3.0f;
   return InitFromLinKey(rKey, key, time);
}


// Specialized Bezier/Hybrid mappings

template<>
IBezFloatKey MapKey<IBezFloatKey, FloatKey>(FloatKey& key, float time)
{
   IBezFloatKey rKey;
   rKey.val = key.data;
   rKey.intan = key.forward_tangent;
   rKey.outtan = key.backward_tangent;
   return InitBezKey(rKey, key, time);
}

template<>
IBezPoint3Key MapKey<IBezPoint3Key, Vector3Key>(Vector3Key& key, float time)
{
   IBezPoint3Key rKey;
   rKey.val = TOPOINT3(key.data);
   rKey.intan = TOPOINT3(key.forward_tangent);
   rKey.outtan = TOPOINT3(key.backward_tangent);
   return InitBezKey(rKey, key, time);
}

template<>
IBezQuatKey MapKey<IBezQuatKey, QuatKey>(QuatKey& key, float time)
{
   IBezQuatKey rKey;
   rKey.val = TOQUAT(key.data, true);
   return InitBezKey(rKey, key, time);
}

template<>
IBezScaleKey MapKey<IBezScaleKey, FloatKey>(FloatKey& key, float time)
{
   IBezScaleKey rKey;
   rKey.val.s.Set(key.data, key.data, key.data);
   rKey.val.q.Identity();
   return InitBezKey(rKey, key, time);
}

// Specialized Bezier/Hybrid Mappings

template<>
FloatKey MapKey<FloatKey, IBezFloatKey>(IBezFloatKey& key, float time)
{
   FloatKey rKey;
   rKey.data = key.val;
   rKey.forward_tangent = key.intan;
   rKey.backward_tangent = key.outtan;
   return InitFromBezKey(rKey, key, time);
}

template<>
Vector3Key MapKey<Vector3Key, IBezPoint3Key>(IBezPoint3Key& key, float time)
{
   Vector3Key rKey;
   rKey.data = TOVECTOR3(key.val);
   rKey.forward_tangent = TOVECTOR3(key.intan);
   rKey.backward_tangent = TOVECTOR3(key.outtan);
   return InitFromBezKey(rKey, key, time);
}

template<>
QuatKey MapKey<QuatKey, IBezQuatKey>(IBezQuatKey& key, float time)
{
   QuatKey rKey;
   rKey.data = TOQUAT(key.val, true);
   //rKey.forward_tangent = TOQUAT(key.intan, true);
   //rKey.backward_tangent = TOQUAT(key.outtan, true);
   return InitFromBezKey(rKey, key, time);
}

template<>
FloatKey MapKey<FloatKey, IBezScaleKey>(IBezScaleKey& key, float time)
{
   FloatKey rKey;
   rKey.data = Average(key.val.s);
   rKey.forward_tangent = Average(key.intan);
   rKey.backward_tangent = Average(key.outtan);
   return InitFromBezKey(rKey, key, time);
}


// Specialized TCB Mappings

template<>
ITCBFloatKey MapKey<ITCBFloatKey, FloatKey>(FloatKey& key, float time)
{
   ITCBFloatKey rKey;
   rKey.val = key.data;
   return InitTCBKey(rKey, key, time);
}

template<>
ITCBRotKey MapKey<ITCBRotKey, QuatKey>(QuatKey& key, float time)
{
   ITCBRotKey rKey;
   rKey.val = TOQUAT(key.data, true);
   InitTCBKey(rKey, key, time);
   rKey.flags = TCBKEY_QUATVALID;
   return rKey;
}

template<>
ITCBPoint3Key MapKey<ITCBPoint3Key, Vector3Key>(Vector3Key& key, float time)
{
   ITCBPoint3Key rKey;
   rKey.val = TOPOINT3(key.data);
   return InitTCBKey(rKey, key, time);
}

template<>
ITCBScaleKey MapKey<ITCBScaleKey, FloatKey>(FloatKey& key, float time)
{
   ITCBScaleKey rKey;
   rKey.val.s.Set(key.data, key.data, key.data);
   rKey.val.q.Identity();
   return InitTCBKey(rKey, key, time);
}

template<>
FloatKey MapKey<FloatKey, ITCBFloatKey>(ITCBFloatKey& key, float time)
{
   FloatKey rKey;
   rKey.data = key.val;
   return InitFromTCBKey(rKey, key, time);
}

template<>
Vector3Key MapKey<Vector3Key, ITCBPoint3Key>(ITCBPoint3Key& key, float time)
{
   Vector3Key rKey;
   rKey.data = TOVECTOR3(key.val);
   return InitFromTCBKey(rKey, key, time);
}

template<>
QuatKey MapKey<QuatKey, ITCBRotKey>(ITCBRotKey& key, float time)
{
   QuatKey rKey;
   rKey.data = TOQUAT(key.val, true);
   return InitFromTCBKey(rKey, key, time);
}

template<>
FloatKey MapKey<FloatKey, ITCBScaleKey>(ITCBScaleKey& key, float time)
{
   FloatKey rKey;
   rKey.data = Average(key.val.s);
   return InitFromTCBKey(rKey, key, time);
}

// Merge Keys

template<> void MergeKey<ILinRotKey>(ILinRotKey& lhs, ILinRotKey& rhs) {
   lhs.val *= rhs.val;
}

template<> void MergeKey<IBezQuatKey>(IBezQuatKey& lhs, IBezQuatKey& rhs) {
   lhs.val *= rhs.val;
}

template<> void MergeKey<ITCBRotKey>(ITCBRotKey& lhs, ITCBRotKey& rhs) {
   lhs.val = Quat(lhs.val) * Quat(rhs.val);
}


template<> FloatKey InterpKey<FloatKey>(Control *subCtrl, TimeValue time, float timeOff) {
   FloatKey rKey;
   memset(&rKey, 0, sizeof(rKey));
   rKey.time = timeOff + FrameToTime(time);
   Interval valid; valid.SetEmpty();
   if (subCtrl->SuperClassID() == SClass_ID(CTRL_SCALE_CLASS_ID) ) {
      ScaleValue s;
      subCtrl->GetValue(time, &s, valid, CTRL_ABSOLUTE);
      rKey.data = Average(s.s);
   } else {
      subCtrl->GetValue(time, &rKey.data, valid, CTRL_ABSOLUTE);
   }
   return rKey;
}

template<> QuatKey InterpKey<QuatKey>(Control *subCtrl, TimeValue time, float timeOff) {
   QuatKey rKey;
   memset(&rKey, 0, sizeof(rKey));
   rKey.time = timeOff + FrameToTime(time);
   Interval valid; valid.SetEmpty();
   Quat q;
   subCtrl->GetValue(time, &q, valid, CTRL_ABSOLUTE);
   rKey.data = TOQUAT(q, true);
   return rKey;
}

template<> Vector3Key InterpKey<Vector3Key>(Control *subCtrl, TimeValue time, float timeOff) {
   Vector3Key rKey;
   memset(&rKey, 0, sizeof(rKey));
   rKey.time = timeOff + FrameToTime(time);
   Interval valid; valid.SetEmpty();
   Point3 p;
   subCtrl->GetValue(time, &p, valid, CTRL_ABSOLUTE);
   rKey.data = TOVECTOR3(p);
   return rKey;
}

float GetValue(Vector3& value, V3T type)
{
   switch (type) {
   case V3T_X: return value.x;
   case V3T_Y: return value.y;
   case V3T_Z: return value.z;
   }
   return 0.0f;
}

void SetValue(Vector3& v, float value, V3T type)
{
   switch (type) {
   case V3T_X: v.x = value; break;
   case V3T_Y: v.y = value; break;
   case V3T_Z: v.z = value; break;
   }
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

Vector3Key& CopyKey( Vector3Key& dst, FloatKey& src, V3T type)
{
   dst.time = src.time;
   dst.bias = src.bias;
   dst.continuity = src.continuity;
   dst.tension = src.tension;
   SetValue(dst.backward_tangent, src.backward_tangent, type);
   SetValue(dst.forward_tangent, src.forward_tangent, type);
   SetValue(dst.data, src.data, type);
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

void JoinKeys(vector<Vector3Key>&keys, vector<FloatKey>&xkeys, vector<FloatKey>&ykeys, vector<FloatKey>&zkeys)
{
   int n = xkeys.size();
   keys.resize(n);
   for (int i=0; i<n; ++i) {
      Vector3Key& key = keys[i];
      CopyKey(key, xkeys[i], V3T_X), CopyKey(key, ykeys[i], V3T_Y), CopyKey(key, zkeys[i], V3T_Z);
   }
}

bool GetTranslationKeys(Control *c, vector<Vector3Key> keys, const vector<float>& times, float timeOffset)
{
   // separate xyz
   if (c->ClassID() == IPOS_CONTROL_CLASS_ID) 
   { 
      KeyType kType = QUADRATIC_KEY;
      vector<FloatKey> xkeys, ykeys, zkeys;

      if (Control *x = c->GetXController()){
         if (x->ClassID() == Class_ID(LININTERP_FLOAT_CLASS_ID,0)) {
            kType = LINEAR_KEY;
            GetKeys<FloatKey, ILinFloatKey>(x, xkeys, /*times,*/ timeOffset);
         } else if (x->ClassID() == Class_ID(HYBRIDINTERP_FLOAT_CLASS_ID,0)) {
            kType = QUADRATIC_KEY;
            GetKeys<FloatKey, IBezFloatKey>(x, xkeys, /*times,*/ timeOffset);
         } else if (x->ClassID() == Class_ID(TCBINTERP_FLOAT_CLASS_ID,0)) {
            kType = TBC_KEY;
            GetKeys<FloatKey, ITCBFloatKey>(x, xkeys, /*times,*/ timeOffset);
         } else {
            kType = QUADRATIC_KEY;
            GetKeys<FloatKey, IBezFloatKey>(x, xkeys, /*times,*/ timeOffset);
         }
      }
      if (Control *y = c->GetYController()){
         if (y->ClassID() == Class_ID(LININTERP_FLOAT_CLASS_ID,0)) {
            GetKeys<FloatKey, ILinFloatKey>(y, ykeys, /*times,*/ timeOffset);
         } else if (y->ClassID() == Class_ID(HYBRIDINTERP_FLOAT_CLASS_ID,0)) {
            GetKeys<FloatKey, IBezFloatKey>(y, ykeys, /*times,*/ timeOffset);
         } else if (y->ClassID() == Class_ID(TCBINTERP_FLOAT_CLASS_ID,0)) {
            GetKeys<FloatKey, ITCBFloatKey>(y, ykeys, /*times,*/ timeOffset);
         } else {
            GetKeys<FloatKey, IBezFloatKey>(y, ykeys, /*times,*/ timeOffset);
         }
      }
      if (Control *z = c->GetZController()){
         if (z->ClassID() == Class_ID(LININTERP_FLOAT_CLASS_ID,0)) {
            GetKeys<FloatKey, ILinFloatKey>(z, zkeys, /*times,*/ timeOffset);
         } else if (z->ClassID() == Class_ID(HYBRIDINTERP_FLOAT_CLASS_ID,0)) {
            GetKeys<FloatKey, IBezFloatKey>(z, zkeys, /*times,*/ timeOffset);
         } else if (z->ClassID() == Class_ID(TCBINTERP_FLOAT_CLASS_ID,0)) {
            GetKeys<FloatKey, ITCBFloatKey>(z, zkeys, /*times,*/ timeOffset);
         } else {
            GetKeys<FloatKey, IBezFloatKey>(z, zkeys, /*times,*/ timeOffset);
         }
      }
      JoinKeys(keys, xkeys, ykeys, zkeys);
      return true;
   }
   return false;
}

void ScaleKey(FloatKey& key, float mult) {
	key.data *= mult;
}

void ScaleKeys(vector<FloatKey>&keys, float mult) {
	for (int i=0, n = keys.size(); i<n; ++i) {
		ScaleKey(keys[i], mult);
	}
}
