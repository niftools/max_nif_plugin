/**********************************************************************
*<
FILE: ImportAnimation.cpp

DESCRIPTION:	Animation Import Routines

CREATED BY: tazpn (Theo)

HISTORY: 

*>	Copyright (c) 2006, All Rights Reserved.
**********************************************************************/
#include "stdafx.h"
#include <IFrameTagManager.h>
#include <notetrck.h>
#include "MaxNifImport.h"
#include "NIFImporter.h"
#include "KFMImporter.h"
#include "KFImporter.h"
#include "AnimKey.h"
#include "NifPlugins.h"
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

void* operator new(size_t size, NoteKey* stub )
{ return MAX_new(size); }

void operator delete(void* memblock, NoteKey* stub )
{ return MAX_delete(memblock); }


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
      if (node->HasNoteTracks()){
         for (int i = node->NumNoteTracks()-1; i>=0; --i ){
            if (NoteTrack *nt = node->GetNoteTrack(i))
               node->DeleteNoteTrack(nt, TRUE);
         }
      }
      node->DeleteKeys(TRACK_DOALL);
      ::ClearAnimation(node->GetTMController());
      for (int i=0, n=node->NumberOfChildren(); i<n; ++i){
         ClearAnimation(node->GetChildNode(i));
      }

      // Clear animation priority
      node->SetUserPropFloat(NP_ANM_PRI, 0.0f);
   }
}
void NifImporter::ClearAnimation()
{
   if (clearAnimation)
   {
      if (IFrameTagManager *tagMgr = (IFrameTagManager*)GetCOREInterface(FRAMETAGMANAGER_INTERFACE)) {

         int n = tagMgr->GetTagCount();
         for (int i=n-1; i>=0; --i){
            tagMgr->DeleteTag( tagMgr->GetTagID(i) );
         }
      }
      ClearAnimation(gi->GetRootNode());
   }
}

FPValue GetScriptedProperty( FPValue& thing, TCHAR* propName ) {
   init_thread_locals();
   push_alloc_frame();
   two_value_locals( thingValue, propNameValue );
   save_current_frames();
   trace_back_active = FALSE;

   FPValue retVal( TYPE_INT, 0 );
   BOOL isUndefined = ((thing.type==TYPE_VALUE) && (thing.v==&undefined));
   if( (thing.i!=0) && (!isUndefined) ) try { //Safe handling for NULL
      vl.thingValue = InterfaceFunction::FPValue_to_val( thing );
      vl.propNameValue = Name::intern( propName );
      vl.thingValue = vl.thingValue->get_property( &vl.propNameValue, 1 );
      vl.thingValue->to_fpvalue( retVal );
   }
   catch ( ... ) {
      clear_error_source_data();
      restore_current_frames();
      MAXScript_signals = 0;
      if (progress_bar_up)
         MAXScript_interface->ProgressEnd(), progress_bar_up = FALSE;
   }
   pop_value_locals();
   pop_alloc_frame();
   return retVal;
}

Value* GetFunction( Value* thing, TCHAR* funcName ) {
   init_thread_locals();
   push_alloc_frame();
   one_value_local( funcNameValue );
   save_current_frames();
   trace_back_active = FALSE;

   Value* retval = NULL;
   if( (thing!=0) ) try { //Safe handling for NULL
      vl.funcNameValue = Name::intern( funcName );
      retval = thing->get_property( &vl.funcNameValue, 1 );
   }
   catch ( ... ) {
      clear_error_source_data();
      restore_current_frames();
      MAXScript_signals = 0;
      if (progress_bar_up)
         MAXScript_interface->ProgressEnd(), progress_bar_up = FALSE;
   }
   pop_value_locals();
   pop_alloc_frame();
   return retval;
}

static FPValue myAddNewNoteKey(Value* noteTrack, int frame)
{
   // Magic initialization stuff for maxscript.
   static bool script_initialized = false;
   if (!script_initialized) {
      init_MAXScript();
      script_initialized = TRUE;
   }
   init_thread_locals();
   push_alloc_frame();
   five_value_locals(name, fn, track, frame, result);
   save_current_frames();
   trace_back_active = FALSE;

   FPValue retVal( TYPE_INT, 0 );
   try	{
      // Create the name of the maxscript function we want.
      // and look it up in the global names
      vl.name = Name::intern(_T("addNewNoteKey"));
      vl.fn = globals->get(vl.name);

      // For some reason we get a global thunk back, so lets
      // check the cell which should point to the function.
      // Just in case if it points to another global thunk
      // try it again.
      while (vl.fn != NULL && is_globalthunk(vl.fn))
         vl.fn = static_cast<GlobalThunk*>(vl.fn)->cell;
      while (vl.fn != NULL && is_constglobalthunk(vl.fn))
         vl.fn = static_cast<ConstGlobalThunk*>(vl.fn)->cell;

      // Now we should have a MAXScriptFunction, which we can
      // call to do the actual conversion. If we didn't
      // get a MAXScriptFunction, we can't convert.
      if (vl.fn != NULL && vl.fn->_is_function()) {
         Value* args[4];

         // Ok. convertToArchMat takes one parameter, the material
         // and an optional keyword paramter, replace, which tells
         // convertToArchMat whether to replace all reference to
         // the old material by the new one.
         args[0] = vl.track = noteTrack;	// The original material
         args[1] = Integer::intern(frame);
         args[2] = &keyarg_marker;						// Separates keyword params from mandatory

         // Call the funtion and save the result.
         vl.result = vl.fn->apply(args, 2);

         // If the result isn't NULL, try to convert it to a material.
         // If the convesion fails, an exception will be thrown.
         if (vl.result != NULL)
            vl.result->to_fpvalue(retVal);
      }
   } catch (...) {
      clear_error_source_data();
      restore_current_frames();
      MAXScript_signals = 0;
      if (progress_bar_up)
         MAXScript_interface->ProgressEnd(), progress_bar_up = FALSE;
   }

   // Magic Max Script stuff to clear the frame and locals.
   pop_value_locals();
   pop_alloc_frame();

   return retVal;
}

bool NifImporter::AddNoteTracks(float time, string name, string target, NiTextKeyExtraDataRef textKeyData, bool loop)
{
   vector<StringKey> textKeys = textKeyData->GetKeys();
   if (!textKeys.empty()) {

      Interval range = gi->GetAnimRange();
      for (vector<StringKey>::iterator itr=textKeys.begin(); itr != textKeys.end(); ++itr) {
         TimeValue t = TimeToFrame(time + (*itr).time);
         if (t < range.Start())
            range.SetStart(t);
         if (t > range.End())
            range.SetEnd(t);
      }
      gi->SetAnimRange(range);

      if (addNoteTracks && (wildmatch("start*", textKeys.front().data)) ) {
         if ( INode *n = gi->GetINodeByName(target.c_str()) ) {
#if 1
            DefNoteTrack* nt = (DefNoteTrack*)NewDefaultNoteTrack();
            n->AddNoteTrack(nt);

            for (vector<StringKey>::iterator itr=textKeys.begin(); itr != textKeys.end(); ++itr) {
               TimeValue t = TimeToFrame(time + (*itr).time);

               if (wildmatch("start*", (*itr).data)){
                  stringlist args = TokenizeCommandLine((*itr).data.c_str(), true);
                  if (args.empty()) continue;
                  bool hasName = false;
                  bool hasLoop = false;
                  for (stringlist::iterator itr = args.begin(); itr != args.end(); ++itr) {
                     if (strmatch("-name", *itr)) {
                        if (++itr == args.end()) break;                       
                        hasName = true;
                     } else if (strmatch("-loop", *itr)) {
                        hasLoop = true;
                     }
                  }
                  if (!hasName) {
                     if (name.empty())
                        name = FormatString("EMPTY_SEQUENCE_AT_%df", int(t * FramesPerSecond / TicksPerFrame) );
                     args.push_back("-name");
                     args.push_back(name);
                  }
                  if (!hasLoop && loop) {
                     args.push_back("-loop");
                  }

                  string line = JoinCommandLine(args);
                  NoteKey *key = new NoteKey(t, line.c_str(), 0);
                  nt->keys.Append(1, &key);
               } else {
                  NoteKey *key = new NoteKey(t, (*itr).data.c_str(), 0);
                  nt->keys.Append(1, &key);
               }
            }

#else
            TSTR script;
            script += 
               "fn getActorManager obj = (\n"
               "   local nt = undefined\n"
               "   n = numNoteTracks obj\n"
               "   for i = 1 to n do (\n"
               "      local nt = getNoteTrack obj i\n"
               "      if (nt.name == \"ActorManager\") then ( return nt )\n"
               "   )\n"
               "   nt = notetrack \"ActorManager\"\n"
               "   addNoteTrack obj nt\n"
               "   return nt\n"
               ")\n"
               "fn addNoteKey nt frame tag = (\n"
               "   local Key = addNewNoteKey nt.keys frame\n"
               "   Key.value = tag\n"
               ")\n";

            script += FormatText("nt = getActorManager $'%s'\n", target.c_str());

            for (vector<StringKey>::iterator itr=textKeys.begin(); itr != textKeys.end(); ++itr) {
               TimeValue t = TimeToFrame(time + (*itr).time);

               if (wildmatch("start*", (*itr).data)){
                  stringlist args = TokenizeCommandLine((*itr).data.c_str(), true);
                  if (args.empty()) continue;
                  bool hasName = false;
                  bool hasLoop = false;
                  CycleType ct = cntr->GetCycleType();
                  for (stringlist::iterator itr = args.begin(); itr != args.end(); ++itr) {
                     if (strmatch("-name", *itr)) {
                        if (++itr == args.end()) break;                       
                        hasName = true;
                     } else if (strmatch("-loop", *itr)) {
                        hasLoop = true;
                     }
                  }
                  if (!hasName) {
                     string name = cntr->GetName();
                     if (name.empty())
                        name = FormatString("EMPTY_SEQUENCE_AT_%df", int(t * FramesPerSecond / TicksPerFrame) );
                     args.push_back("-name");
                     args.push_back(name);
                  }
                  if (!hasLoop && ct == CYCLE_LOOP) {
                     args.push_back("-loop");
                  }

                  string line = JoinCommandLine(args);
                  script += FormatText("addNoteKey nt (%d/ticksPerFrame) \"%s\"\n", t, line.c_str());
               } else {
                  script += FormatText("addNoteKey nt (%d/ticksPerFrame) \"%s\"\n", t, (*itr).data.c_str());
               }

               //NoteKey *key = new NoteKey(TimeToFrame(time + (*itr).time), (*itr).data.c_str(), 0);
               //nt->keys.Append(1, &key);
            }
            ExecuteMAXScriptScript(script, TRUE, NULL);
#endif
         }
      }

      if (addTimeTags) {
         if (IFrameTagManager *tagMgr = (IFrameTagManager*)GetCOREInterface(FRAMETAGMANAGER_INTERFACE)) {
            for (vector<StringKey>::iterator itr=textKeys.begin(); itr != textKeys.end(); ++itr) {
               tagMgr->CreateNewTag(const_cast<TCHAR*>((*itr).data.c_str()), TimeToFrame(time + (*itr).time), 0, FALSE);
            }
         }
      }
      return true;
   }
   return false;
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
      float start = cntr->GetStartTime();
      float stop = cntr->GetStopTime();
      float total = (stop - start);

      vector<ControllerLink> links = cntr->GetControllerData();

      NiTextKeyExtraDataRef textKeyData = cntr->GetTextKeyExtraData();
      AddNoteTracks( time, cntr->GetName(), cntr->GetTargetName(), textKeyData, (CYCLE_LOOP == cntr->GetCycleType()) );

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

         if ((*lnk).priority_ != 0.0f) {
            npSetProp(n, NP_ANM_PRI, (*lnk).priority_);
         }

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

   if (NiTextKeyExtraDataRef keydata = SelectFirstObjectOfType<NiTextKeyExtraData>(nref->GetExtraData())) {
      ni.AddNoteTracks(0.0f, string(), nref->GetName(), keydata, false);
   }

   float time = 0.0f;
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