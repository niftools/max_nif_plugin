/**********************************************************************
*<
FILE: ImportAnimation.cpp

DESCRIPTION:	Animation Import Routines

CREATED BY: tazpn (Theo)

HISTORY: 

*>	Copyright (c) 2006, All Rights Reserved.
**********************************************************************/
#include "stdafx.h"
#if VERSION_3DSMAX >= ((7000<<16)+(15<<8)+0) // Version 7
#  include <IFrameTagManager.h>
#endif
#if VERSION_3DSMAX < (14000<<16) // Version 14 (2012)
#include "maxscrpt/Strings.h"
#else
#include "maxscript/maxscript.h"
#endif
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
#include <obj/NiBSplineTransformInterpolator.h>
#include <obj/NiGeomMorpherController.h>
#include <obj/NiMorphData.h>
#include <obj/NiBSplineCompFloatInterpolator.h>
#include <obj/NiFloatInterpolator.h>
#include <obj/NiFloatData.h>
#include "niutils.h"
using namespace Niflib;

const Class_ID IPOS_CONTROL_CLASS_ID = Class_ID(0x118f7e02,0xffee238a);
enum {
   IPOS_X_REF	=	0,
   IPOS_Y_REF	=	1,
   IPOS_Z_REF	=	2,
   IPOS_W_REF	=	3,
};


const KeyType UNKNOWN_KEY = (KeyType)0;

#if VERSION_3DSMAX > ((5000<<16)+(15<<8)+0) // Version 6+
void* operator new(size_t size, NoteKey* stub )
{ return MAX_new(size); }

void operator delete(void* memblock, NoteKey* stub )
{ return MAX_delete(memblock); }
#else
void* operator new(size_t size, NoteKey* stub )
{ 
   void * (__cdecl *pfmalloc)(__in size_t _Size) = 0;
   if (HMODULE hMod = GetModuleHandle("msvcrt.dll")) 
      *(FARPROC*)pfmalloc = GetProcAddress(hMod, "malloc");
   if (pfmalloc == 0) pfmalloc = malloc;
   return pfmalloc(size); 
}

void operator delete(void* memblock, NoteKey* stub )
{ 
   void (__cdecl *pffree)(void *memblock) = 0;
   if (HMODULE hMod = GetModuleHandle("msvcrt.dll")) 
      *(FARPROC*)pffree = GetProcAddress(hMod, "free");
   if (pffree == 0) pffree = free;
   return pffree(memblock); 
}

#endif

struct AnimationImport
{
   AnimationImport(NifImporter& parent) : ni(parent) {}

   NifImporter &ni;

   vector<KeyTextValue> BuildKeyValues(NiObjectNETRef nref);
   bool AddValues(NiObjectNETRef nref);
   bool AddValues(vector<NiObjectNETRef>& nodes);

   bool AddValues(Control *c, NiKeyframeDataRef data, float time);
   bool AddBiped(Control *c, NiKeyframeDataRef data, float time);

   bool AddValues(NiInterpolatorRef interp, IParamBlock* pblock, float time);

   Control* MakePosition(Control *tmCont, Class_ID clsid);
   Control* MakePositionXYZ(Control *tmCont, Class_ID clsid);
   Control* MakeRotation(Control *tmCont, Class_ID rotClsid, Class_ID rollClsid);
   Control* MakeScale(Control *tmCont, Class_ID clsid);
   Control* MakeFloat(IParamBlock* pblock, int idx, Class_ID clsid);

   
   Control* GetTMController(const string& name);
   Control* GetTMController(NiObjectNETRef node);
   Matrix3 GetTM(const string& name, TimeValue t = 0);

   bool GetTransformData(ControllerLink& lnk, string name, NiKeyframeDataRef& outData, Point3& p, Quat& q, float& s);

   bool ImportGeoMorph(INode *n, NiGeomMorpherControllerRef ctrl, float time);
   INode* CreateGeoMesh(const vector<Vector3>& verts, const vector<Triangle>& tris, Matrix3& tm, INode *parent);
};

bool NifImporter::ImportAnimation()
{
   if (!enableAnimations)
      return false;

   if (nodes.empty())
      return false;

   AnimationImport ai(*this);
   vector<NiObjectNETRef> roots;
   roots.push_back(root);
   return ai.AddValues(roots);
   //NiNodeRef rootNode = root;
   //return ai.AddValues(DynamicCast<NiObjectNET>(rootNode->GetChildren()));
}

static vector<ControllerLink>::iterator FindLink(string name, vector<ControllerLink>& links)
{
   for (vector<ControllerLink>::iterator lnk=links.begin(); lnk != links.end(); )
   {
      string target = (*lnk).targetName;
      if (target.empty() && (*lnk).nodeNameOffset >= 0) {
         NiStringPaletteRef strings = lnk->stringPalette;
         target = strings->GetSubStr((*lnk).nodeNameOffset);
      }
	  if (target.empty()) {
		  name = (*lnk).nodeName;
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
#if VERSION_3DSMAX > ((5000<<16)+(15<<8)+0) // Version 5
      if (Control *sc = c->GetWController()) { 
         if (sc != c) ClearAnimation(sc); 
      }
#endif
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
	  if (node->UserPropExists(NP_ANM_PRI)) {
		node->SetUserPropString(NP_ANM_PRI, "");
	  }
   }
}
void NifImporter::ClearAnimation()
{
   if (clearAnimation)
   {
#if VERSION_3DSMAX >= ((7000<<16)+(15<<8)+0) // Version 7
      if (IFrameTagManager *tagMgr = (IFrameTagManager*)GetCOREInterface(FRAMETAGMANAGER_INTERFACE)) {

         int n = tagMgr->GetTagCount();
         for (int i=n-1; i>=0; --i){
            tagMgr->DeleteTag( tagMgr->GetTagID(i) );
         }
      }
#endif
      ClearAnimation(gi->GetRootNode());
   }
}

static void GetTimeRange(Control *c, Interval& range)
{
   if (c->IsKeyable())
   {
      int iNumKeys = c->NumKeys();
      for (int i = 0; i < iNumKeys; i++) {
         TimeValue t = c->GetKeyTime(i);
         if (range.Empty()) {
            range.SetInstant(t);
         } else {
            if (t < range.Start())
               range.SetStart(t);
            if (t > range.End())
               range.SetEnd(t);
         }
      }
      if (range.Empty())
      {
         if (IKeyControl *ikeys = GetKeyControlInterface(c)){
            int n = ikeys->GetNumKeys();
            for (int i=0; i<n; ++i){
               AnyKey buf; IKey *key = (IKey*)buf;
               ikeys->GetKey(i, key);
               if (range.Empty()) {
                  range.SetInstant(key->time);
               } else {
                  if (key->time < range.Start())
                     range.SetStart(key->time);
                  if (key->time > range.End())
                     range.SetEnd(key->time);
               }
            }
         }
      }
   }
   if (range.Empty())
   {
      if (c->IsAnimated())
      {
         //Tab<TimeValue> times;
         //if ( c->GetKeyTimes( times,FOREVER, KEYAT_POSITION ) ) {
         //   for (int i=0; i<times.Count(); ++i){
         //      TimeValue time = times[i];
         //      if (range.Empty()) {
         //         range.SetInstant(time);
         //      } else {
         //         if (time < range.Start())
         //            range.SetStart(time);
         //         if (time > range.End())
         //            range.SetEnd(time);
         //      }
         //   }
         //}
         //if ( c->GetKeyTimes( times,FOREVER, KEYAT_ROTATION ) ) {
         //   for (int i=0; i<times.Count(); ++i){
         //      TimeValue time = times[i];
         //      if (range.Empty()) {
         //         range.SetInstant(time);
         //      } else {
         //         if (time < range.Start())
         //            range.SetStart(time);
         //         if (time > range.End())
         //            range.SetEnd(time);
         //      }
         //   }
         //}
      }
   }
}

static void GetTimeRange(INode *node, Interval& range, bool recursive = true)
{
   int nTracks = node->NumNoteTracks();

   // Populate Text keys and Sequence information from note tracks
   for (int i=0; i<nTracks; ++i) {
      if ( NoteTrack *nt = node->GetNoteTrack(i) ) {
         if ( nt->ClassID() == Class_ID(NOTETRACK_CLASS_ID,0) ) {
            DefNoteTrack *defNT = (DefNoteTrack *)nt;
            if ( defNT->NumKeys() > 0 ) {
               for (int j=0, m=defNT->keys.Count(); j<m; ++j) {
                  NoteKey* key = defNT->keys[j];
                  if (range.Empty()) {
                     range.SetInstant(key->time);
                  } else {
                     if (key->time < range.Start())
                        range.SetStart(key->time);
                     if (key->time > range.End())
                        range.SetEnd(key->time);
                  }
               }
            }
         }
      }
   }
   if (Control* c = node->GetTMController())
   {
      GetTimeRange(c, range);

#if VERSION_3DSMAX > ((5000<<16)+(15<<8)+0) // Version 5
      if (Control *sc = c->GetWController()) { 
         GetTimeRange(sc, range);
         if (sc != c) GetTimeRange(sc, range);
      }
#endif
      if (Control *sc = c->GetXController()) { 
         if (sc != c) GetTimeRange(sc, range);
      }
      if (Control *sc = c->GetYController()) { 
         if (sc != c) GetTimeRange(sc, range);
      }
      if (Control *sc = c->GetZController()) { 
         if (sc != c) GetTimeRange(sc, range);
      }
      if (Control *sc = c->GetRotationController()) { 
         if (sc != c) GetTimeRange(sc, range);
      }
      if (Control *sc = c->GetPositionController()) { 
         if (sc != c) GetTimeRange(sc, range);
      }
      if (Control *sc = c->GetScaleController()) { 
         if (sc != c) GetTimeRange(sc, range);
      }
   }
   if (recursive) {
      for (int i=0; i<node->NumberOfChildren(); i++) 
         GetTimeRange(node->GetChildNode(i), range, true);
   }
}

#if 0
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
#endif

bool NifImporter::AddNoteTracks(float time, string name, string target, NiTextKeyExtraDataRef textKeyData, bool loop)
{
   if (textKeyData == NULL)
      return false;

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

      if (addNoteTracks /*&& (wildmatch("start*", textKeys.front().data))*/ ) {
         if ( INode *n = GetNode(target) ) {
//#if VERSION_3DSMAX > ((5000<<16)+(15<<8)+0) // Version 6+
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
				  bool hasAt = false;
                  for (stringlist::iterator itr = args.begin(); itr != args.end(); ++itr) {
                     if (strmatch("-name", *itr)) {
                        if (++itr == args.end()) break;                       
                        hasName = true;
                     } else if (strmatch("-loop", *itr)) {
                        hasLoop = true;
					 } else if (strmatch("-at", *itr)) {
						 if (++itr == args.end()) break;
						 hasAt = true;
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
				  if (!hasAt) {
					  args.push_back("-at");
					  args.push_back("y");
				  }

                  string line = JoinCommandLine(args);
                  NoteKey *key = new NoteKey(t, line.c_str(), 0);
                  nt->keys.Append(1, &key);
               } else {
                  NoteKey *key = new NoteKey(t, (*itr).data.c_str(), 0);
                  nt->keys.Append(1, &key);
               }
            }

#else // Version 5
            extern void CallMaxscript(const TCHAR *s);
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
                  bool hasLoop = loop;
				  bool hasAt = false;
                  for (stringlist::iterator itr = args.begin(); itr != args.end(); ++itr) {
                     if (strmatch("-name", *itr)) {
                        if (++itr == args.end()) break;                       
                        hasName = true;
                     } else if (strmatch("-loop", *itr)) {
                        hasLoop = true;
					 } else if (strmatch("-at", *itr)) {
						 if (++itr == args.end()) break;
						 hasAt = true;
					 }
                  }
                  if (!hasName) {
                     string name = FormatString("EMPTY_SEQUENCE_AT_%df", int(t * FramesPerSecond / TicksPerFrame) );
                     args.push_back("-name");
                     args.push_back(name);
                  }
                  if (!hasLoop) {
                     args.push_back("-loop");
                  }
				  if (!hasAt) {
					  args.push_back("-at");
					  args.push_back("y");
				  }
                  string line = JoinCommandLine(args);
                  script += FormatText("addNoteKey nt (%d/ticksPerFrame) \"%s\"\n", t, line.c_str());
               } else {
                  script += FormatText("addNoteKey nt (%d/ticksPerFrame) \"%s\"\n", t, (*itr).data.c_str());
               }
            }
            CallMaxscript(script);
            //ExecuteMAXScriptScript(script, TRUE, NULL);
#endif
         }
      }

      if (addTimeTags) {
#if VERSION_3DSMAX >= ((7000<<16)+(15<<8)+0) // Version 7
         if (IFrameTagManager *tagMgr = (IFrameTagManager*)GetCOREInterface(FRAMETAGMANAGER_INTERFACE)) {
            for (vector<StringKey>::iterator itr=textKeys.begin(); itr != textKeys.end(); ++itr) {
               tagMgr->CreateNewTag(const_cast<TCHAR*>((*itr).data.c_str()), TimeToFrame(time + (*itr).time), 0, FALSE);
            }
         }
#endif
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
   // Locate the end of the previous animation and start importing there
   if (!clearAnimation) {
      float curTime = FrameToTime(gi->GetTime() + TicksPerFrame);
      Interval globalRange; globalRange.SetInstant(0);
      GetTimeRange( gi->GetRootNode(), globalRange, true );
      if (globalRange.Duration() != 0)
         globalRange.SetEnd( globalRange.End() + TicksPerFrame );
      float endTime = FrameToTime( globalRange.End() );
      time = max(curTime, endTime);
      gi->SetTime( TimeToFrame(time) );
   }

   for(vector<NiControllerSequenceRef>::iterator itr = kf.begin(); itr != kf.end(); ++itr){

      float minTime = 1e+35f;
      float maxTime = 0.0f;

      NiControllerSequenceRef cntr = (*itr);
      float start = cntr->GetStartTime();
      float stop = cntr->GetStopTime();

      vector<ControllerLink> links = cntr->GetControllerData();

      NiTextKeyExtraDataRef textKeyData = cntr->GetTextKeys();
      if (textKeyData == NULL)
         textKeyData = cntr->GetTextKeyExtraData();

      string textName = cntr->GetTextKeysName();
      if (textName.empty())
         textName = cntr->GetTargetName();

      AddNoteTracks( time, cntr->GetName(), textName, textKeyData, (CYCLE_LOOP == cntr->GetCycleType()) );

      for (vector<ControllerLink>::iterator lnk=links.begin(); lnk != links.end(); ++lnk)
      {
         string name = (*lnk).targetName;
         if (name.empty() && (*lnk).nodeNameOffset >= 0) {
            NiStringPaletteRef strings = lnk->stringPalette;
            name = strings->GetSubStr((*lnk).nodeNameOffset);
         }
         if (name.empty()) {
            name = (*lnk).nodeName;
         }
         if (name.empty())
            continue;

         // I realize this is not the best way to do this but it works for some files
         if (mergeNonAccum && wildmatch("* NonAccum", name)) 
         {
            name = name.substr(0, name.length() - 9);
         }

         string type = (*lnk).controllerType;
         if (type.empty()) {
            NiStringPaletteRef strings = lnk->stringPalette;
            type = strings->GetSubStr((*lnk).controllerTypeOffset);
         }
         if (type.empty()) {
            if ((*lnk).controller != NULL) {
               type = (*lnk).controller->GetType().GetTypeName();
            }
         }
         if (type.empty())
            continue;

         if (strmatch(type, "NiTransformController"))
         {
            Control *c = ai.GetTMController(name);
            if (NULL == c)
               continue;

            INode *n = GetNode(name);

            if ((*lnk).priority != 0.0f) {
               npSetProp(n, NP_ANM_PRI, (*lnk).priority);
            }

            NiKeyframeDataRef data;
            Point3 p; Quat q; float s;
            if (ai.GetTransformData(*lnk, name, data, p, q, s)) {
               PosRotScaleNode(n, p, q, s, prsDefault, TimeToFrame(time));
               if (ai.AddValues(c, data, time)) {
                  minTime = min(minTime, start);
                  maxTime = max(maxTime, stop);
                  ok = true;
               }
            }
         }
         else if (strmatch(type, "NiGeomMorpherController"))
         {
            string var2 = (*lnk).variable2;
            if (var2.empty()) {
               if (NiStringPaletteRef strings = lnk->stringPalette)
                  var2 = strings->GetSubStr((*lnk).variable2Offset);
            }
            if (!var2.empty())
            {
               if (INode *n = GetNode(name))
               {
                  if (Modifier* mod = GetMorpherModifier(n))
                  {
                     int idx = -1;
                     for (int i=1; i<=100; ++i) {
                        if (strmatch(var2, MorpherGetName(mod, i))) {
                           idx = i;
                           break;
                        }
                     }
                     if (idx != -1)
                     {
                        if (ai.AddValues(lnk->interpolator, (IParamBlock*)mod->GetReference(idx), time)) {
                           minTime = min(minTime, start);
                           maxTime = max(maxTime, stop);
                           ok = true;
                        }
                     }
                  }
               }
            }
         }
         else if (strmatch(type, "NiKeyframeController"))
         {
            Control *c = ai.GetTMController(name);
            if (NULL == c)
               continue;

            if (NiKeyframeControllerRef controller = (*lnk).controller)
            {
               start = controller->GetStartTime();
               stop = controller->GetStopTime();
            }

            INode *n = GetNode(name);

            if ((*lnk).priority != 0.0f) {
               npSetProp(n, NP_ANM_PRI, (*lnk).priority);
            }

            NiKeyframeDataRef data;
            Point3 p; Quat q; float s;
            if (ai.GetTransformData(*lnk, name, data, p, q, s)) {
               PosRotScaleNode(n, p, q, s, prsDefault, TimeToFrame(time));
               if (ai.AddValues(c, data, time)) {
                  minTime = min(minTime, start);
                  maxTime = max(maxTime, stop);
                  ok = true;
               }
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
      } else if (NiBSplineTransformInterpolatorRef interp = lnk.interpolator) {
         int npoints = interp->GetNumControlPoints();

         if (npoints > 3)
         {
            NiKeyframeDataRef data = CreateNiObject<NiKeyframeData>();
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
   INode *n = ni.GetNode(name);
   if (NULL == n)
      return NULL;

   Control *c = n->GetTMController();
   if (NULL == c)
      return NULL;

   return c;
}

Control *AnimationImport::GetTMController(NiObjectNETRef obj)
{
	if (obj->IsDerivedType(NiNode::TYPE))
	{
		NiNodeRef node = StaticCast<NiNode>(obj);
		if (INode *n = ni.GetNode(node)) {
			return n->GetTMController();
		}
	}
	return GetTMController(obj->GetName().c_str());
}

Matrix3 AnimationImport::GetTM(const string& name, TimeValue t)
{
   INode *n = ni.GetNode(name);
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
   if (NiTextKeyExtraDataRef keydata = SelectFirstObjectOfType<NiTextKeyExtraData>(nref->GetExtraData())) {
      ni.AddNoteTracks(0.0f, string(), nref->GetName(), keydata, false);
   }

   bool ok = false;
   float time = 0.0f;
   list< NiTimeControllerRef > clist = nref->GetControllers();
   if (NiTransformControllerRef tc = SelectFirstObjectOfType<NiTransformController>(clist)) {
      if (NiTransformInterpolatorRef interp = tc->GetInterpolator()) {
         if (NiTransformDataRef data = interp->GetData()) {
			 if (Control *c = GetTMController(nref))
	            ok |= AddValues(c, data, time);
         }
      }
   } else if (NiKeyframeControllerRef kf = SelectFirstObjectOfType<NiKeyframeController>(clist)) {
      if (NiKeyframeDataRef kfData = kf->GetData()) {
		  if (Control *c = GetTMController(nref))
	         ok |= AddValues(c, kfData, time);
      }
   }
   if (NiGeomMorpherControllerRef gmc = SelectFirstObjectOfType<NiGeomMorpherController>(clist)) {
	   if (INode *n = ni.GetNode(nref)) {
		   ok |= ImportGeoMorph(n, gmc, time);
	   }
   }

   return ok;
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
Control* AnimationImport::MakeFloat(IParamBlock* pblock, int idx, Class_ID clsid)
{
	Interface *ip = ni.gi;
	if (Control *c = pblock->GetController(idx)) {
		if (c->ClassID()!=clsid) {
			if (Control *tmpCtrl = (Control*)ip->CreateInstance(CTRL_SCALE_CLASS_ID, clsid)){
				pblock->SetController(idx, tmpCtrl);
				if (pblock->GetController(idx) != tmpCtrl)
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
               c->ReplaceReference(IPOS_X_REF, tmp);
         }
      }
      if (Control *y = c->GetYController()){
         if (y->ClassID()!= clsid) {
            if (Control *tmp = (Control*)ip->CreateInstance(CTRL_FLOAT_CLASS_ID, clsid))
               c->ReplaceReference(IPOS_Y_REF, tmp);
         }
      }
      if (Control *z = c->GetZController()){
         if (z->ClassID()!= clsid) {
            if (Control *tmp = (Control*)ip->CreateInstance(CTRL_FLOAT_CLASS_ID, clsid))
               c->ReplaceReference(IPOS_Z_REF, tmp);
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

bool AnimationImport::ImportGeoMorph(INode *n, NiGeomMorpherControllerRef ctrl, float time)
{
	NiObjectNETRef target = ctrl->GetTarget();
	if (!target->IsDerivedType(NiTriBasedGeom::TYPE))
		return false;
	NiTriBasedGeomRef parentGeom = StaticCast<NiTriBasedGeom>(target);
	NiMorphDataRef data = ctrl->GetData();
	if (data == NULL)
		return false;
	vector<NiInterpolatorRef> interpolators = ctrl->GetInterpolators();
	unsigned int nmorphs = data->GetMorphCount();
	if (ni.nifVersion >= VER_10_1_0_106)
	{
		if ((interpolators.size() < nmorphs) || nmorphs == 0)
			return false;
	}
	else
	{
		for (unsigned int i=0; i<nmorphs; i++)
		{
			if (NiFloatInterpolatorRef interp = new NiFloatInterpolator())
			{
				NiFloatDataRef fdata = new NiFloatData();
				fdata->SetKeyType( data->GetMorphKeyType(i) );
				fdata->SetKeys( data->GetMorphKeys(i) );
				interp->SetFloatValue(FloatNegINF);
				interp->SetData(fdata);
				interpolators.push_back(interp);
			}
		}
	}
	NiGeometryDataRef geoData = parentGeom->GetData();
	int nBaseVerts = geoData->GetVertexCount();
	vector<Triangle> tris;
	vector<Vector3> baseVerts;
	if (geoData->IsDerivedType(NiTriShapeData::TYPE)) {
		NiTriShapeDataRef triShapeData = StaticCast<NiTriShapeData>(geoData);
		if (triShapeData == NULL)
			return false;
		tris = triShapeData->GetTriangles();
		baseVerts = triShapeData->GetVertices();
	} else if (geoData->IsDerivedType(NiTriStripsData::TYPE)) {
		NiTriStripsDataRef triStripData = StaticCast<NiTriStripsData>(geoData);
		if (triStripData == NULL)
			return false;
		tris = triStripData->GetTriangles();
		baseVerts = triStripData->GetVertices();
	} else {
		return false;
	}
	Matrix3 tm = TOMATRIX3(parentGeom->GetLocalTransform());
	tm.Invert();
	//tm.NoRot(); tm.NoTrans(); // Leave scale.  Parent to owner mesh

	Modifier *mod = CreateMorpherModifier(n);
	n->EvalWorldState(0, TRUE);

	// Create meshes for morph
	for (unsigned int i=1; i<nmorphs; ++i) // Skip first morph as its the baseline
	{
		string frameName = (ni.nifVersion >= VER_10_1_0_106) ? data->GetFrameName(i) : FormatString("Frame #%d", i);
		vector<Vector3> verts = data->GetMorphVerts(i);
		if (verts.size() != nBaseVerts)
			continue;

		for (int j=0; j<nBaseVerts; ++j)
			verts[j] += baseVerts[j];

		TSTR name(frameName.c_str());
		INode *geoNode = CreateGeoMesh(verts, tris, tm, n);
		geoNode->SetName(FormatText("Morph: %s", name));

		MorpherBuildFromNode(mod, i+1, geoNode);
		MorpherSetName(mod, i+1, name);
		n->EvalWorldState(0, TRUE);

		AddValues(interpolators[i], (IParamBlock*)mod->GetReference(i+1), time);
	}
	return false;
}

bool AnimationImport::AddValues(NiInterpolatorRef interp, IParamBlock* pblock, float time)
{
	bool retval = false;
	// Handle Translation
	//
	if (interp == NULL)
		return false;

	KeyType keyType = UNKNOWN_KEY;
	vector<FloatKey> keys;
	if (interp->IsDerivedType(NiFloatInterpolator::TYPE)) {
		if (NiFloatDataRef data = StaticCast<NiFloatInterpolator>(interp)->GetData())
		{
			keyType = data->GetKeyType();
			keys = data->GetKeys();
		}
	} else if (interp->IsDerivedType(NiBSplineCompFloatInterpolator::TYPE)) {
		NiBSplineCompFloatInterpolatorRef bsfi = StaticCast<NiBSplineCompFloatInterpolator>(interp);
		int npoints = bsfi->GetNumControlPoints();
		if (npoints > 3) {
			keyType = QUADRATIC_KEY;
			keys = bsfi->SampleKeys(npoints, 3);
		}			
	}
	if (keyType != UNKNOWN_KEY)
	{
		ScaleKeys(keys, 100.0f);
		switch (keyType)
		{
		case LINEAR_KEY:
			if (Control *subCtrl = MakeFloat(pblock, 0, Class_ID(LININTERP_FLOAT_CLASS_ID,0))) {
				MergeKeys<ILinFloatKey, FloatKey>(subCtrl, keys, time);
				retval |= true;
			}
			break;
		case QUADRATIC_KEY:
		case XYZ_ROTATION_KEY:
			if (Control *subCtrl = MakeFloat(pblock, 0, Class_ID(HYBRIDINTERP_FLOAT_CLASS_ID,0))) {
				MergeKeys<IBezFloatKey, FloatKey>(subCtrl, keys, time);
				retval |= true;
			}
			break;
		case TBC_KEY:
			if (Control *subCtrl = MakeFloat(pblock, 0, Class_ID(TCBINTERP_FLOAT_CLASS_ID,0))) {
				MergeKeys<ITCBFloatKey, FloatKey>(subCtrl, keys, time);
				retval |= true;
			}
			break;
		}
	}
	return retval;
}

INode *AnimationImport::CreateGeoMesh(
	const vector<Vector3>& verts, 
	const vector<Triangle>& tris,
	Matrix3& tm,
	INode *parent
	)
{
	INode *returnNode = NULL;
	if ( ImpNode *node = ni.i->CreateNode() )
	{
		TriObject *triObject = CreateNewTriObject();
		node->Reference(triObject);

		Mesh& mesh = triObject->GetMesh();
		INode *tnode = node->GetINode();

		// Vertex info
		{
			int nVertices = verts.size();
			mesh.setNumVerts(nVertices);
			for (int i=0; i < nVertices; ++i){
				const Vector3& v = verts[i];
				mesh.verts[i].Set(v.x, v.y, v.z);
			}
		}

		// Triangles and texture vertices
		ni.SetTriangles(mesh, tris);

		//MNMesh mn(mesh);
		//mn.OutToTri(mesh);
		mesh.checkNormals(TRUE);

		// Wireframe Red color
		StdMat2 *collMat = NewDefaultStdMat();
		collMat->SetDiffuse(Color(0.0f, 1.0f, 0.0f), 0);
		collMat->SetWire(TRUE);
		collMat->SetFaceted(TRUE);
		ni.gi->GetMaterialLibrary().Add(collMat);
		tnode->SetMtl(collMat);
		tnode->SetRenderable(FALSE);
		tnode->SetPrimaryVisibility(FALSE);
		tnode->SetSecondaryVisibility(FALSE);
		tnode->SetWireColor( RGB(0,255,0) );
		tnode->SetUserPropBool("np_ignore", TRUE);

		returnNode = node->GetINode();
		
		//PosRotScaleNode(returnNode, tm, prsDefault, 0);
		if (parent != NULL)
			parent->AttachChild(tnode, 1);
	}
	return returnNode;
}

