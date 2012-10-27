/**********************************************************************
*<
FILE: AnimKey.h

DESCRIPTION:	Animation Key Import Routines

CREATED BY: tazpn (Theo)

HISTORY: 

*>	Copyright (c) 2006, All Rights Reserved.
**********************************************************************/
// Max Headers
#include <Max.h>
#include <tab.h>

// Niflib Headers
#include <niflib.h>
#include <Key.h>
using namespace Niflib;

#if VERSION_3DSMAX <= ((5000<<16)+(15<<8)+0) // Version 5
class AnyKey
{
public:
   Tab<char> data;
   AnyKey(int size = 128) { data.SetCount(size); } // 128 is default from IKeyControl::GetKeySize()
   void SetSize(int size) { data.SetCount(size); }
   operator IKey*() { return (IKey*)data.Addr(0); }
};
#endif

const float FramesPerSecond = 30.0f;
const float FramesIncrement = 1.0f/30.0f;
const int TicksPerFrame = GetTicksPerFrame();

inline TimeValue TimeToFrame(float t) {
   return TimeValue(t * FramesPerSecond * TicksPerFrame);
}
inline float FrameToTime(TimeValue t) {
   return float(t) / (FramesPerSecond * TicksPerFrame);
}

typedef Key<float> FloatKey;
typedef Key<Quaternion> QuatKey;
typedef Key<Vector3> Vector3Key;
typedef Key<string> StringKey;

template<typename T, typename U> T MapKey(U& key, float time);

// Specialized Linear Mappings
template<> ILinFloatKey MapKey<ILinFloatKey,FloatKey>(FloatKey& key, float time);
template<> ILinRotKey MapKey<ILinRotKey, QuatKey>(QuatKey& key, float time);
template<> ILinScaleKey MapKey<ILinScaleKey, FloatKey>(FloatKey& key, float time);
template<> ILinPoint3Key MapKey<ILinPoint3Key, Vector3Key>(Vector3Key& key, float time);

template<> FloatKey MapKey<FloatKey, ILinFloatKey>(ILinFloatKey& key, float time);
template<> QuatKey MapKey<QuatKey, ILinRotKey>(ILinRotKey& key, float time);
template<> FloatKey MapKey<FloatKey, ILinScaleKey>(ILinScaleKey& key, float time);
template<> Vector3Key MapKey<Vector3Key, ILinPoint3Key>(ILinPoint3Key& key, float time);

// Specialized Bezier/Hybrid mappings
template<> IBezFloatKey MapKey<IBezFloatKey, FloatKey>(FloatKey& key, float time);
template<> IBezPoint3Key MapKey<IBezPoint3Key, Vector3Key>(Vector3Key& key, float time);
template<> IBezQuatKey MapKey<IBezQuatKey, QuatKey>(QuatKey& key, float time);
template<> IBezScaleKey MapKey<IBezScaleKey, FloatKey>(FloatKey& key, float time);

template<> FloatKey MapKey<FloatKey, IBezFloatKey>(IBezFloatKey& key, float time);
template<> QuatKey MapKey<QuatKey, IBezQuatKey>(IBezQuatKey& key, float time);
template<> FloatKey MapKey<FloatKey, IBezScaleKey>(IBezScaleKey& key, float time);
template<> Vector3Key MapKey<Vector3Key, IBezPoint3Key>(IBezPoint3Key& key, float time);

// Specialized TCB mappings
template<> ITCBFloatKey MapKey<ITCBFloatKey, FloatKey>(FloatKey& key, float time);
template<> ITCBRotKey MapKey<ITCBRotKey, QuatKey>(QuatKey& key, float time);
template<> ITCBPoint3Key MapKey<ITCBPoint3Key, Vector3Key>(Vector3Key& key, float time);
template<> ITCBScaleKey MapKey<ITCBScaleKey, FloatKey>(FloatKey& key, float time);

template<> FloatKey MapKey<FloatKey, ITCBFloatKey>(ITCBFloatKey& key, float time);
template<> QuatKey MapKey<QuatKey, ITCBRotKey>(ITCBRotKey& key, float time);
template<> FloatKey MapKey<FloatKey, ITCBScaleKey>(ITCBScaleKey& key, float time);
template<> Vector3Key MapKey<Vector3Key, ITCBPoint3Key>(ITCBPoint3Key& key, float time);

// Interpolated Keys
template<typename T> T InterpKey(Control *subCtrl, TimeValue time, float timeOff);
template<> FloatKey InterpKey<FloatKey>(Control *subCtrl, TimeValue time, float timeOff);
template<> QuatKey InterpKey<QuatKey>(Control *subCtrl, TimeValue time, float timeOff);
template<> Vector3Key InterpKey<Vector3Key>(Control *subCtrl, TimeValue time, float timeOff);

template<typename T> void MergeKey(T& lhs, T& rhs) {
   lhs.val += rhs.val;
}

//template<> void MergeKey<ILinFloatKey>(ILinFloatKey& lhs, ILinFloatKey& rhs);
template<> void MergeKey<ILinRotKey>(ILinRotKey& lhs, ILinRotKey& rhs);
//template<> void MergeKey<ILinScaleKey>(ILinScaleKey& lhs, ILinScaleKey& rhs);
//template<> void MergeKey<ILinPoint3Key>(ILinPoint3Key& lhs, ILinPoint3Key& rhs);

//template<> void MergeKey<IBezFloatKey>(IBezFloatKey& lhs, IBezFloatKey& rhs);
template<> void MergeKey<IBezQuatKey>(IBezQuatKey& lhs, IBezQuatKey& rhs);
//template<> void MergeKey<IBezPoint3Key>(IBezPoint3Key& lhs, IBezPoint3Key& rhs);
//template<> void MergeKey<IBezScaleKey>(IBezScaleKey& lhs, IBezScaleKey& rhs);

//template<> void MergeKey<ITCBFloatKey>(ITCBFloatKey& lhs, ITCBFloatKey& rhs);
template<> void MergeKey<ITCBRotKey>(ITCBRotKey& lhs, ITCBRotKey& rhs);
//template<> void MergeKey<ITCBPoint3Key>(ITCBPoint3Key& lhs, ITCBPoint3Key& rhs);
//template<> void MergeKey<ITCBScaleKey>(ITCBScaleKey& lhs, ITCBScaleKey& rhs);

template<typename T> void MergeKey(IKeyControl *keys, T& key)
{
   for (int i=0, n=keys->GetNumKeys(); i<n; ++i)
   {
      AnyKey buf; T *tmp = reinterpret_cast<T*>((IKey*)buf);
      keys->GetKey(i, tmp);
      if (tmp->time == key.time) {
         MergeKey<T>(*(T*)tmp, key);
         keys->SetKey(i, tmp);
         return;
      }
   }
   keys->AppendKey(&key);
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
         ikeys->SortKeys();
      }
   }
}

template<typename T, typename U>
inline int GetKeys(Control *subCtrl, vector<T>& keys, float time)
{
   if (IKeyControl *ikeys = GetKeyControlInterface(subCtrl)){
      int n = ikeys->GetNumKeys();
      keys.reserve(n);
      for (int i=0; i<n; ++i){
         AnyKey buf; U *key = reinterpret_cast<U*>((IKey*)buf);
         ikeys->GetKey(i, key);
         keys.push_back( MapKey<T>(*key, time) );
      }
      return keys.size();
   }
   return 0;
}

template<typename T, typename U>
inline int GetKeys(Control *subCtrl, vector<T>& keys, Interval range)
{
   if (IKeyControl *ikeys = GetKeyControlInterface(subCtrl)){
      float timeOffset = -FrameToTime(range.Start());
      int n = ikeys->GetNumKeys();
      keys.reserve(n);
      bool hasStart = false, hasEnd = false;
      for (int i=0; i<n; ++i){
         AnyKey buf; U *key = reinterpret_cast<U*>((IKey*)buf);
         ikeys->GetKey(i, key);
         if (range.InInterval(key->time)) {
            hasStart |= (range.Start() == key->time);
            hasEnd   |= (range.End() == key->time);
            keys.push_back( MapKey<T>(*key, timeOffset) );
         }
      }
      if (keys.size() > 0) {
         if (!hasStart) keys.insert(keys.begin(), InterpKey<T>(subCtrl, range.Start(), timeOffset) );
         if (!hasEnd) keys.push_back(InterpKey<T>(subCtrl, range.End(), timeOffset) );
      }
      return keys.size();
   }
   return 0;
}

template<typename T, typename U>
inline void MergeKeys(Control *subCtrl, vector<U>& keys, float time)
{
   if (subCtrl && !keys.empty()){
      if (IKeyControl *ikeys = GetKeyControlInterface(subCtrl)){
         if (ikeys->GetNumKeys() == 0){
            SetKeys<T,U>(subCtrl, keys, time);
         } else {
            for (int i=0,n=keys.size(); i<n; ++i) {
               MergeKey(ikeys, MapKey<T>(keys[i], time));
            }
            ikeys->SortKeys();
         }        
      }
   }
}

enum V3T
{
   V3T_X, V3T_Y, V3T_Z
};
extern float GetValue(Vector3& value, V3T type);
extern FloatKey& CopyKey( FloatKey& dst, Vector3Key& src, V3T type);
extern void SplitKeys(vector<Vector3Key>&keys, vector<FloatKey>&xkeys, vector<FloatKey>&ykeys, vector<FloatKey>&zkeys);
extern void JoinKeys(vector<Vector3Key>&keys, vector<FloatKey>&xkeys, vector<FloatKey>&ykeys, vector<FloatKey>&zkeys);

typedef Key<string> KeyTextValue;

bool GetTranslationKeys(Control *c, vector<Vector3Key> keys, const vector<float>& times, float timeOffset=0.0f);

extern void ScaleKeys(vector<FloatKey>&keys, float mult);
extern void ScaleKey(FloatKey& key, float mult);
