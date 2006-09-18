/**********************************************************************
*<
FILE: ImportAnimation.cpp

DESCRIPTION:	Animation Import Routines

CREATED BY: tazpn (Theo)

HISTORY: 

*>	Copyright (c) 2006, All Rights Reserved.
**********************************************************************/
#include "pch.h"
#include <IFrameTagManager.h>
#include <notetrck.h>
#include <set>
#include "NifExport.h"
#include "AnimKey.h"

#include <obj/NiControllerSequence.h>
#include <obj/NiControllerManager.h>
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
#include <obj/NiDefaultAVObjectPalette.h>
#include <obj/NiMultiTargetTransformController.h>
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

   INode * findTrackedNode(INode *root);

   bool doExport(NiControllerSequenceRef seq);
   bool doExport(NiControllerManagerRef ctrl, INode *node);
   bool exportController(INode *node);
   Control *GetTMController(INode* node);
   NiTimeControllerRef exportController(INode *node, Interval range, bool setTM );
   bool GetTextKeys(INode *node, vector<StringKey>& textKeys);

   Exporter &ne;
   Interval range;
   NiControllerSequenceRef seq;

   set<NiAVObjectRef> objRefs;
   map<NiControllerSequenceRef, Interval> ranges;
};

bool GetTextKeys(INode *node, vector<StringKey>& textKeys, Interval range)
{
   // Populate Text keys and Sequence information from note tracks
   if (Exporter::mUseTimeTags) {
      if (IFrameTagManager *tagMgr = (IFrameTagManager*)GetCOREInterface(FRAMETAGMANAGER_INTERFACE)) {
         int n = tagMgr->GetTagCount();
         for (int i = 0; i<n; i++) {
            UINT id = tagMgr->GetTagID(i);
            TimeValue t = tagMgr->GetTimeByID(id, FALSE);
            TSTR name = tagMgr->GetNameByID(id);

            StringKey strkey;
            strkey.time = FrameToTime( Interval(range.Start(), t).Duration()-1 );
            strkey.data = name;
            textKeys.push_back(strkey);
         }
      }
   } else {
      for (int i=0, n=node->NumNoteTracks(); i<n; ++i) {
         if ( NoteTrack *nt = node->GetNoteTrack(i) ) {
            if ( nt->ClassID() == Class_ID(NOTETRACK_CLASS_ID,0) ) {
               DefNoteTrack *defNT = (DefNoteTrack *)nt;
               if ( defNT->NumKeys() > 0 ) {
                  bool stop = false;
                  for (int j=0, m=defNT->keys.Count(); j<m && !stop; ++j) {
                     NoteKey* key = defNT->keys[j];

                     if (wildmatch("start*", key->note)) {
                        stringlist args = TokenizeCommandLine(key->note, true);
                        if (args.empty()) continue;

                        range.SetStart( key->time );
                        for (stringlist::iterator itr = args.begin(); itr != args.end(); ++itr) {
                           if (strmatch("-name", *itr)) {
                              if (++itr == args.end()) break;
                           }
                        }
                     } else if ( wildmatch("end*", key->note) ) {
                        range.SetEnd( key->time );
                        stop = true;
                     }
                     StringKey strkey;
                     strkey.time = FrameToTime( Interval(range.Start(), key->time).Duration()-1 );
                     strkey.data = key->note;
                     textKeys.push_back(strkey);
                  }
               }
            }
         }
      }
   }
   return !textKeys.empty();
}

void Exporter::InitializeTimeController(NiTimeControllerRef ctrl, NiNodeRef parent)
{
   ctrl->SetFrequency(1.0f);
   ctrl->SetStartTime(FloatINF);
   ctrl->SetStopTime(FloatNegINF);
   ctrl->SetPhase(0.0f);
   ctrl->SetFlags(0x0C);
   ctrl->SetTarget( parent );
   parent->AddController(DynamicCast<NiTimeController>(ctrl));
}

NiNodeRef Exporter::createAccumNode(NiNodeRef parent, INode *node)
{
   NiNodeRef accumNode;
   bool isTracked = isNodeTracked(node);
   if (!Exporter::mAllowAccum || !isTracked)
   {
      accumNode = parent;
   }
   else
   {
      accumNode = getNode( FormatString("%s NonAccum", node->GetName()) );
      accumNode->SetLocalTransform(Matrix44::IDENTITY);
      parent->AddChild(DynamicCast<NiAVObject>(accumNode));
   }

   // add multi target controller to parent if exporting with animation
   if ( mExportType == NIF_WO_KF ){

      if ( Exporter::mAllowAccum && isTracked ) {
         // transfer controllers to accum
         list<NiTimeControllerRef> ctlrs = parent->GetControllers();
         for (list<NiTimeControllerRef>::iterator it = ctlrs.begin(); it != ctlrs.end(); ++it) {
            parent->RemoveController(*it);
            accumNode->AddController(*it);
         }
      }

   } else if ( Exporter::mExportType != Exporter::NIF_WO_ANIM ) 
   {
      // NiMultiTargetTransformController causes crashes in old formats
      if (Exporter::mNifVersionInt >= VER_10_0_1_0)
      {
         NiMultiTargetTransformControllerRef ctrl = new NiMultiTargetTransformController();
         vector<NiNodeRef> children;
         getChildNodes(node, children);
         ctrl->SetExtraTargets(children);
         Exporter::InitializeTimeController(ctrl, parent);
      }
      NiControllerManagerRef mgr = new NiControllerManager();
      Exporter::InitializeTimeController(mgr, parent);

      // Export Animation now
      doAnimExport(mgr, node);
   }
   return accumNode;
}
Exporter::Result Exporter::doAnimExport(NiControllerSequenceRef root)
{
   AnimationExport animExporter(*this);
   return animExporter.doExport(root) ? Exporter::Ok : Exporter::Abort ;
}

Exporter::Result Exporter::doAnimExport(NiControllerManagerRef mgr, INode *node)
{
   AnimationExport animExporter(*this);
   return animExporter.doExport(mgr, node) ? Exporter::Ok : Exporter::Abort ;
}

bool Exporter::isNodeTracked(INode *node)
{
   if (Exporter::mUseTimeTags) {
      // Assume only one top level node has animation
      if (mI->GetRootNode() == node->GetParentNode() && isNodeKeyed(node)) {
         return true;
      }
   }
   else if (node->HasNoteTracks()) {
      for (int i=0, n=node->NumNoteTracks(); i<n; ++i) {
         if ( NoteTrack *nt = node->GetNoteTrack(i) ) {
            if ( nt->ClassID() == Class_ID(NOTETRACK_CLASS_ID,0) ) {
               DefNoteTrack *defNT = (DefNoteTrack *)nt;
               if ( defNT->NumKeys() > 0 ) {
                  for (int j=0, m=defNT->keys.Count(); j<m; ++j) {
                     NoteKey* key = defNT->keys[j];
                     if (wildmatch("start*", key->note)) {
                        return true;
                     }
                  }
               }
            }
         }
      }
   }
   return false;
}


static bool HasKeys(Control *c)
{
   bool rv = false;
   if (c != NULL)
   {
      if (c->IsColorController())
         return false;

      if (IKeyControl *ikeys = GetKeyControlInterface(c)){
         if (ikeys->GetNumKeys() > 0)
            return true;
      }
      if (Control *sc = c->GetWController()) { 
         if (sc != c && HasKeys(sc)) 
            return true;
      }
      if (Control *sc = c->GetXController()) { 
         if (sc != c && HasKeys(sc)) 
            return true;
      }
      if (Control *sc = c->GetYController()) { 
         if (sc != c && HasKeys(sc)) 
            return true;
      }
      if (Control *sc = c->GetZController()) { 
         if (sc != c && HasKeys(sc)) 
            return true;
      }
      if (Control *sc = c->GetRotationController()) { 
         if (sc != c && HasKeys(sc)) 
            return true;
      }
      if (Control *sc = c->GetPositionController()) { 
         if (sc != c && HasKeys(sc)) 
            return true;
      }
      if (Control *sc = c->GetScaleController()) { 
         if (sc != c && HasKeys(sc)) 
            return true;
      }
   }
   return false;
}

bool Exporter::isNodeKeyed(INode *node) {
   if (node->HasNoteTracks()) {
      return true;
   }
   if (node->NumKeys() > 0) {
      return true;
   }
   if (Control *tmCont = node->GetTMController()) {
      if (HasKeys(tmCont))
         return true;
   }
   return false;
}


INode * AnimationExport::findTrackedNode(INode *node)
{
   if (ne.isNodeTracked(node))
      return node;

   // locate START in note track before assuming all is well
   for (int i=0; i < node->NumberOfChildren(); ++i ){
      if ( INode *root = findTrackedNode( node->GetChildNode(i) ) ) {
         return root;
      }
   }
   return NULL;
}

bool AnimationExport::doExport(NiControllerSequenceRef seq)
{
   INode *node = findTrackedNode(ne.mI->GetRootNode());
   if (node == NULL)
      throw runtime_error("No Actor Roots have been selected in the Animation Manager. Cannot continue.");

   this->seq = seq;

   vector<StringKey> textKeys;

   this->range.SetInstant(0);

   seq->SetStartTime(FloatINF);
   seq->SetStopTime(FloatINF);
   seq->SetFrequency(1.0f);
   seq->SetCycleType( CYCLE_CLAMP );
   seq->SetTargetName( node->GetName() );

   NiTextKeyExtraDataRef textKeyData = new NiTextKeyExtraData();
   seq->SetTextKey(textKeyData);

   // Populate Text keys and Sequence information from note tracks
   if (Exporter::mUseTimeTags) {
      if (IFrameTagManager *tagMgr = (IFrameTagManager*)GetCOREInterface(FRAMETAGMANAGER_INTERFACE)) {
         int n = tagMgr->GetTagCount();
         for (int i = 0; i<n; i++) {
            UINT id = tagMgr->GetTagID(i);
            TimeValue t = tagMgr->GetTimeByID(id, FALSE);
            TSTR name = tagMgr->GetNameByID(id);

            StringKey strkey;
            strkey.time = FrameToTime( Interval(range.Start(), t).Duration()-1 );
            strkey.data = name;
            textKeys.push_back(strkey);
         }
      }
   } else {
      for (int i=0, n=node->NumNoteTracks(); i<n; ++i) {
         if ( NoteTrack *nt = node->GetNoteTrack(i) ) {
            if ( nt->ClassID() == Class_ID(NOTETRACK_CLASS_ID,0) ) {
               DefNoteTrack *defNT = (DefNoteTrack *)nt;
               if ( defNT->NumKeys() > 0 ) {
                  bool stop = false;
                  for (int j=0, m=defNT->keys.Count(); j<m && !stop; ++j) {
                     NoteKey* key = defNT->keys[j];

                     if (wildmatch("start*", key->note)) {
                        stringlist args = TokenizeCommandLine(key->note, true);
                        if (args.empty()) continue;

                        seq->SetStartTime(0.0f);
                        range.SetStart( key->time );
                        for (stringlist::iterator itr = args.begin(); itr != args.end(); ++itr) {
                           if (strmatch("-name", *itr)) {
                              if (++itr == args.end()) break;
                              seq->SetName(*itr);
                           } else if (strmatch("-loop", *itr)) {
                              seq->SetCycleType(CYCLE_LOOP);
                           }
                        }
                     } else if ( wildmatch("end*", key->note) ) {
                        range.SetEnd( key->time );
                        seq->SetStopTime( FrameToTime( range.Duration()-1 ) );
                        stop = true;
                     }
                     StringKey strkey;
                     strkey.time = FrameToTime( Interval(range.Start(), key->time).Duration()-1 );
                     strkey.data = key->note;
                     textKeys.push_back(strkey);
                  }
               }
            }
         }
      }
   }
   textKeyData->SetKeys(textKeys);

   // Now let the fun begin.

   bool ok = exportController(node);

   // Handle NonAccum
   if (ok && Exporter::mAllowAccum)
   {
      NiNodeRef ninode = new NiNode();
      ninode->SetName(FormatString("%s NonAccum", node->GetName()));

      if (Exporter::mNifVersionInt >= VER_10_2_0_0)
      {
         NiTransformControllerRef control = new NiTransformController();
         NiTransformInterpolatorRef interp = new NiTransformInterpolator();
         ninode->AddController(StaticCast<NiTimeController>(control));
         control->SetInterpolator(StaticCast<NiInterpolator>(interp));

         interp->SetTranslation( Vector3(0.0f, 0.0f, 0.0f) );
         interp->SetScale( 1.0f );
         interp->SetRotation( Quaternion(1.0f, 0.0f, 0.0f, 0.0f) );
         seq->AddInterpolator(StaticCast<NiSingleInterpolatorController>(control), Exporter::mDefaultPriority);
      }
   }
   return ok;
}

bool AnimationExport::doExport(NiControllerManagerRef mgr, INode *node)
{
   int start = 0;
   NiDefaultAVObjectPaletteRef objPal = new NiDefaultAVObjectPalette();
   mgr->SetObjectPalette(objPal);


   vector<NiControllerSequenceRef> seqs;
   vector<StringKey> textKeys;
   NiControllerSequenceRef curSeq;

   // Populate Text keys and Sequence information from note tracks
   if (Exporter::mUseTimeTags) {
      if (IFrameTagManager *tagMgr = (IFrameTagManager*)GetCOREInterface(FRAMETAGMANAGER_INTERFACE)) {

         curSeq = new NiControllerSequence();
         curSeq->SetStartTime(FloatINF);
         curSeq->SetStopTime(FloatINF);
         curSeq->SetFrequency(1.0f);
         curSeq->SetCycleType( CYCLE_CLAMP );
         curSeq->SetTargetName( node->GetName() );
         seqs.push_back(curSeq);
         this->range.SetInstant(0);
         curSeq->SetStartTime(0.0f);

         int n = tagMgr->GetTagCount();
         for (int i = 0; i<n; i++) {
            UINT id = tagMgr->GetTagID(i);
            TimeValue t = tagMgr->GetTimeByID(id, FALSE);
            TSTR name = tagMgr->GetNameByID(id);

            if (t < range.Start())
               range.SetStart(t);

            if (t > range.End())
               range.SetEnd( t );

            StringKey strkey;
            strkey.time = FrameToTime( Interval(range.Start(), t).Duration()-1 );
            strkey.data = name;
            textKeys.push_back(strkey);
         }
         NiTextKeyExtraDataRef textKeyData = new NiTextKeyExtraData();
         curSeq->SetTextKey(textKeyData);
         textKeyData->SetKeys(textKeys);

         curSeq->SetStopTime( FrameToTime( range.Duration()-1 ) );
         this->ranges[curSeq] = range;
         curSeq = NULL;
      }

   } else {

      int nTracks = node->NumNoteTracks();

      // Populate Text keys and Sequence information from note tracks
      for (int i=0; i<nTracks; ++i) {
         if ( NoteTrack *nt = node->GetNoteTrack(i) ) {
            if ( nt->ClassID() == Class_ID(NOTETRACK_CLASS_ID,0) ) {
               DefNoteTrack *defNT = (DefNoteTrack *)nt;
               if ( defNT->NumKeys() > 0 ) {
                  for (int j=0, m=defNT->keys.Count(); j<m; ++j) {
                     NoteKey* key = defNT->keys[j];

                     if (wildmatch("start*", key->note)) {
                        stringlist args = TokenizeCommandLine(key->note, true);
                        textKeys.clear();

                        curSeq = new NiControllerSequence();
                        curSeq->SetStartTime(FloatINF);
                        curSeq->SetStopTime(FloatINF);
                        curSeq->SetFrequency(1.0f);
                        curSeq->SetCycleType( CYCLE_CLAMP );
                        curSeq->SetTargetName( node->GetName() );
                        seqs.push_back(curSeq);
                        this->range.SetInstant(0);

                        curSeq->SetStartTime(0.0f);
                        range.SetStart( key->time );
                        for (stringlist::iterator itr = args.begin(); itr != args.end(); ++itr) {
                           if (strmatch("-name", *itr)) {
                              if (++itr == args.end()) break;
                              curSeq->SetName(*itr);
                           } else if (strmatch("-loop", *itr)) {
                              curSeq->SetCycleType(CYCLE_LOOP);
                           }
                        }
                     }

                     StringKey strkey;
                     strkey.time = FrameToTime( Interval(range.Start(), key->time).Duration()-1 );
                     strkey.data = key->note;
                     textKeys.push_back(strkey);

                     if ( wildmatch("end*", key->note) ) {
                        range.SetEnd( key->time );

                        // add accumulated text keys to sequence
                        if (curSeq != NULL) {
                           curSeq->SetStopTime( FrameToTime( range.Duration()-1 ) );

                           this->ranges[curSeq] = range;

                           NiTextKeyExtraDataRef textKeyData = new NiTextKeyExtraData();
                           curSeq->SetTextKey(textKeyData);
                           textKeyData->SetKeys(textKeys);
                           textKeys.clear();
                           curSeq = NULL;
                        }
                     }
                  }
               }
            }
         }
      }
   }

   for (vector<NiControllerSequenceRef>::iterator itr = seqs.begin(); itr != seqs.end(); ++itr)
   {     
      // Hold temporary value
      this->seq = (*itr);
      
      //this->range.SetStart( TimeToFrame(seq->GetStartTime()) );
      //this->range.SetEnd( TimeToFrame(seq->GetStopTime()) );

      this->range = this->ranges[this->seq];

      // Now let the fun begin.
      bool ok = exportController(node);

      // Handle NonAccum
      if (ok && Exporter::mAllowAccum)
      {
         NiNodeRef ninode = ne.getNode( FormatString("%s NonAccum", node->GetName()) );
         objRefs.insert( StaticCast<NiAVObject>(ninode) );

         if (Exporter::mNifVersionInt >= VER_10_2_0_0)
         {
            NiTransformControllerRef control = new NiTransformController();
            NiTransformInterpolatorRef interp = new NiTransformInterpolator();
            ninode->AddController(StaticCast<NiTimeController>(control));
            control->SetInterpolator(StaticCast<NiInterpolator>(interp));

            interp->SetTranslation( Vector3(0.0f, 0.0f, 0.0f) );
            interp->SetScale( 1.0f );
            interp->SetRotation( Quaternion(1.0f, 0.0f, 0.0f, 0.0f) );
            seq->AddInterpolator(StaticCast<NiSingleInterpolatorController>(control), Exporter::mDefaultPriority);

            // now remove temporary controller
            ninode->RemoveController(StaticCast<NiTimeController>(control));
         }
      }
   }

   // Set objects with animation
   vector<NiAVObjectRef> objs;
   objs.insert(objs.end(), objRefs.begin(), objRefs.end());
   objPal->SetObjs(objs);

   mgr->SetControllerSequences(seqs);

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

NiTimeControllerRef Exporter::CreateController(INode *node, Interval range)
{
   AnimationExport ae(*this);
   if ( NiTimeControllerRef tc = ae.exportController(node, range, false) ) {
      if (Exporter::mExportType == Exporter::NIF_WO_KF && isNodeTracked(node)) {
         NiNodeRef ninode = getNode(node->GetName());
         vector<StringKey> textKeys;
         if (GetTextKeys(node, textKeys, range)) {
            NiTextKeyExtraDataRef textKeyData = new NiTextKeyExtraData();
            ninode->AddExtraData(StaticCast<NiExtraData>(textKeyData), Exporter::mNifVersionInt);
            textKeyData->SetKeys(textKeys);
         }
      }
      return tc;
   }
   return NiTimeControllerRef();
}

NiTimeControllerRef AnimationExport::exportController(INode *node, Interval range, bool setTM )
{
   bool skip = false;
   NiTimeControllerRef timeControl;

   // Primary recursive decent routine

   ObjectState os = node->EvalWorldState(range.Start()); 

   if (!Exporter::mExportCameras && os.obj && os.obj->SuperClassID()==CAMERA_CLASS_ID)
   {
      skip = true;
   }
   else if (!Exporter::mExportLights && os.obj && os.obj->SuperClassID()==LIGHT_CLASS_ID)
   {
      skip = true;
   }

   if (!skip && Exporter::mExportTransforms)
   {
      float timeOffset = -FrameToTime(range.Start());
      if (Control *tmCont = GetTMController(node))
      {
         Interval validity; validity.SetEmpty();
         Matrix3 tm = node->GetObjTMAfterWSM(range.Start());
         if (INode *parent = node->GetParentNode()) {
            Matrix3 pm = Inverse(parent->GetObjTMAfterWSM(range.Start()));
            tm *= pm;
         }

         bool keepData = false;

         // Set default transform to NaN except for root node
         Vector3 trans(FloatNegINF, FloatNegINF, FloatNegINF);
         Quaternion rot(FloatNegINF, FloatNegINF, FloatNegINF, FloatNegINF);
         float scale = FloatNegINF;
         //Vector3 trans = TOVECTOR3(tm.GetTrans());
         //Quaternion rot = TOQUAT( Quat(tm), true );

         NiNodeRef ninode = ne.getNode( node->GetName() );
         if (setTM) {
            trans = TOVECTOR3(tm.GetTrans());
            rot = TOQUAT( Quat(tm), true );
         }

         NiKeyframeDataRef data;

         if (Exporter::mNifVersionInt < VER_10_2_0_0)
         {
            NiKeyframeControllerRef control = new NiKeyframeController();
            Exporter::InitializeTimeController(control, ninode);
            data = new NiKeyframeData();
            control->SetData(data);
            timeControl = StaticCast<NiTimeController>(control);
         }
         else
         {
            NiTransformControllerRef control = new NiTransformController();
            Exporter::InitializeTimeController(control, ninode);

            NiTransformInterpolatorRef interp = new NiTransformInterpolator();
            data = new NiTransformData();
            control->SetInterpolator(StaticCast<NiInterpolator>(interp));

            interp->SetTranslation(trans);
            interp->SetScale(scale);
            interp->SetRotation(rot);
            interp->SetData(data);

            timeControl = StaticCast<NiTimeController>(control);
         }
         timeControl->SetStartTime( 0.0f );
         timeControl->SetStopTime( FrameToTime( range.Duration()-1 ) );

         //if (validity.InInterval(range))
         //{
         //   // Valid for entire interval.  i.e. no changes
         //   interp->SetTranslation( TOVECTOR3(tm.GetTrans()) );
         //   interp->SetScale( Average(GetScale(tm)) );
         //   interp->SetRotation( TOQUAT( Quat(tm) ) );
         //   seq->AddInterpolator(StaticCast<NiSingleInterpolatorController>(control));
         //}
         //else
         {
            if (Control *c = tmCont->GetPositionController()) 
            {
               int nkeys = 0;
               // separate xyz
               if (c->ClassID() == IPOS_CONTROL_CLASS_ID) 
               { 
                  KeyType kType = QUADRATIC_KEY;
                  vector<FloatKey> xkeys, ykeys, zkeys;
                  if (Control *x = c->GetXController()){
                     if (x->ClassID() == Class_ID(LININTERP_FLOAT_CLASS_ID,0)) {
                        kType = LINEAR_KEY;
                        nkeys += GetKeys<FloatKey, ILinFloatKey>(x, xkeys, range);
                     } else if (x->ClassID() == Class_ID(HYBRIDINTERP_FLOAT_CLASS_ID,0)) {
                        kType = QUADRATIC_KEY;
                        nkeys += GetKeys<FloatKey, IBezFloatKey>(x, xkeys, range);
                     } else if (x->ClassID() == Class_ID(TCBINTERP_FLOAT_CLASS_ID,0)) {
                        kType = TBC_KEY;
                        nkeys += GetKeys<FloatKey, ITCBFloatKey>(x, xkeys, range);
                     } else {
                        kType = QUADRATIC_KEY;
                        nkeys += GetKeys<FloatKey, IBezFloatKey>(x, xkeys, range);
                     }
                  }
                  if (Control *y = c->GetYController()){
                     if (y->ClassID() == Class_ID(LININTERP_FLOAT_CLASS_ID,0)) {
                        nkeys += GetKeys<FloatKey, ILinFloatKey>(y, ykeys, range);
                     } else if (y->ClassID() == Class_ID(HYBRIDINTERP_FLOAT_CLASS_ID,0)) {
                        nkeys += GetKeys<FloatKey, IBezFloatKey>(y, ykeys, range);
                     } else if (y->ClassID() == Class_ID(TCBINTERP_FLOAT_CLASS_ID,0)) {
                        nkeys += GetKeys<FloatKey, ITCBFloatKey>(y, ykeys, range);
                     } else {
                        nkeys += GetKeys<FloatKey, IBezFloatKey>(y, ykeys, range);
                     }
                  }
                  if (Control *z = c->GetZController()){
                     if (z->ClassID() == Class_ID(LININTERP_FLOAT_CLASS_ID,0)) {
                        nkeys += GetKeys<FloatKey, ILinFloatKey>(z, zkeys, range);
                     } else if (z->ClassID() == Class_ID(HYBRIDINTERP_FLOAT_CLASS_ID,0)) {
                        nkeys += GetKeys<FloatKey, IBezFloatKey>(z, zkeys, range);
                     } else if (z->ClassID() == Class_ID(TCBINTERP_FLOAT_CLASS_ID,0)) {
                        nkeys += GetKeys<FloatKey, ITCBFloatKey>(z, zkeys, range);
                     } else {
                        nkeys += GetKeys<FloatKey, IBezFloatKey>(z, zkeys, range);
                     }
                  }
                  vector<Vector3Key> keys;
                  JoinKeys(keys, xkeys, ykeys, zkeys);
                  data->SetTranslateType(kType);
                  data->SetTranslateKeys(keys);
               } else {
                  vector<Vector3Key> keys;
                  if (c->ClassID() == Class_ID(LININTERP_FLOAT_CLASS_ID,0)) {
                     data->SetTranslateType(LINEAR_KEY);
                     nkeys += GetKeys<Vector3Key, ILinPoint3Key>(c, keys, range);
                  } else if (c->ClassID() == Class_ID(HYBRIDINTERP_FLOAT_CLASS_ID,0)) {
                     data->SetTranslateType(QUADRATIC_KEY);
                     nkeys += GetKeys<Vector3Key, IBezPoint3Key>(c, keys, range);
                  } else if (c->ClassID() == Class_ID(TCBINTERP_FLOAT_CLASS_ID,0)) {
                     data->SetTranslateType(TBC_KEY);
                     nkeys += GetKeys<Vector3Key, ITCBPoint3Key>(c, keys, range);
                  } else {
                     data->SetTranslateType(QUADRATIC_KEY);
                     nkeys += GetKeys<Vector3Key, IBezPoint3Key>(c, keys, range);
                  }
                  data->SetTranslateKeys(keys);
               }
               if (nkeys != 0) { // if no changes set the base transform
                  keepData = true;
                  //trans = TOVECTOR3(tm.GetTrans());
               }
            }

            // Rotations
            if (Control *c = tmCont->GetRotationController()) 
            {
               int nkeys = 0;
               if (c->ClassID() == Class_ID(LININTERP_ROTATION_CLASS_ID,0)) {
                  vector<QuatKey> keys;
                  data->SetRotateType(LINEAR_KEY);
                  nkeys += GetKeys<QuatKey, ILinRotKey>(c, keys, range);
                  data->SetQuatRotateKeys(keys);
               } else if (c->ClassID() == Class_ID(HYBRIDINTERP_ROTATION_CLASS_ID,0)) {
                  vector<QuatKey> keys;
                  data->SetRotateType(QUADRATIC_KEY);
                  nkeys += GetKeys<QuatKey, IBezQuatKey>(c, keys, range);
                  data->SetQuatRotateKeys(keys);
               } else if (c->ClassID() == Class_ID(EULER_CONTROL_CLASS_ID,0)) {
                  data->SetRotateType(XYZ_ROTATION_KEY);
                  if (Control *x = c->GetXController()){
                     vector<FloatKey> keys;
                     if (x->ClassID() == Class_ID(LININTERP_FLOAT_CLASS_ID,0)) {
                        nkeys += GetKeys<FloatKey, ILinFloatKey>(x, keys, range);
                        data->SetXRotateType(LINEAR_KEY);
                     } else if (x->ClassID() == Class_ID(HYBRIDINTERP_FLOAT_CLASS_ID,0)) {
                        nkeys += GetKeys<FloatKey, IBezFloatKey>(x, keys, range);
                        data->SetXRotateType(QUADRATIC_KEY);
                     } else if (x->ClassID() == Class_ID(TCBINTERP_FLOAT_CLASS_ID,0)) {
                        nkeys += GetKeys<FloatKey, ITCBFloatKey>(x, keys, range);
                        data->SetXRotateType(TBC_KEY);
                     } else {
                        nkeys += GetKeys<FloatKey, IBezFloatKey>(x, keys, range);
                        data->SetXRotateType(QUADRATIC_KEY);
                     }
                     data->SetXRotateKeys(keys);
                  }
                  if (Control *y = c->GetYController()) {
                     vector<FloatKey> keys;
                     if (y->ClassID() == Class_ID(LININTERP_FLOAT_CLASS_ID,0)) {
                        nkeys += GetKeys<FloatKey, ILinFloatKey>(y, keys, range);
                        data->SetYRotateType(LINEAR_KEY);
                     } else if (y->ClassID() == Class_ID(HYBRIDINTERP_FLOAT_CLASS_ID,0)) {
                        nkeys += GetKeys<FloatKey, IBezFloatKey>(y, keys, range);
                        data->SetYRotateType(QUADRATIC_KEY);
                     } else if (y->ClassID() == Class_ID(TCBINTERP_FLOAT_CLASS_ID,0)) {
                        nkeys += GetKeys<FloatKey, ITCBFloatKey>(y, keys, range);
                        data->SetYRotateType(TBC_KEY);
                     } else {
                        nkeys += GetKeys<FloatKey, IBezFloatKey>(y, keys, range);
                        data->SetYRotateType(QUADRATIC_KEY);
                     }
                     data->SetYRotateKeys(keys);
                  }
                  if (Control *z = c->GetZController()) {
                     vector<FloatKey> keys;
                     if (z->ClassID() == Class_ID(LININTERP_FLOAT_CLASS_ID,0)) {
                        nkeys += GetKeys<FloatKey, ILinFloatKey>(z, keys, range);
                        data->SetZRotateType(LINEAR_KEY);
                     } else if (z->ClassID() == Class_ID(HYBRIDINTERP_FLOAT_CLASS_ID,0)) {
                        nkeys += GetKeys<FloatKey, IBezFloatKey>(z, keys, range);
                        data->SetZRotateType(QUADRATIC_KEY);
                     } else if (z->ClassID() == Class_ID(TCBINTERP_FLOAT_CLASS_ID,0)) {
                        nkeys += GetKeys<FloatKey, ITCBFloatKey>(z, keys, range);
                        data->SetZRotateType(TBC_KEY);
                     } else {
                        nkeys += GetKeys<FloatKey, IBezFloatKey>(z, keys, range);
                        data->SetZRotateType(QUADRATIC_KEY);
                     }
                     data->SetZRotateKeys(keys);
                  }
               } else if (c->ClassID() == Class_ID(TCBINTERP_ROTATION_CLASS_ID,0)) {
                  vector<QuatKey> keys;
                  data->SetRotateType(TBC_KEY);
                  nkeys += GetKeys<QuatKey, ITCBRotKey>(c, keys, range);
                  data->SetQuatRotateKeys(keys);
               }
               if (nkeys != 0) { // if no changes set the base transform
                  keepData = true;
                  //rot = TOQUAT( Quat(tm) );
               }
            }

            // Scale
            if (Control *c = tmCont->GetScaleController()) 
            {
               int nkeys = 0;
               if (c->ClassID() == Class_ID(LININTERP_SCALE_CLASS_ID,0)) {
                  vector<FloatKey> keys;
                  data->SetScaleType(LINEAR_KEY);
                  nkeys += GetKeys<FloatKey, ILinFloatKey>(c, keys, range);
                  data->SetScaleKeys(keys);
               } else if (c->ClassID() == Class_ID(HYBRIDINTERP_SCALE_CLASS_ID,0)) {
                  vector<FloatKey> keys;
                  data->SetScaleType(QUADRATIC_KEY);
                  nkeys += GetKeys<FloatKey, IBezFloatKey>(c, keys, range);
                  data->SetScaleKeys(keys);
               } else if (c->ClassID() == Class_ID(TCBINTERP_SCALE_CLASS_ID,0)) {
                  vector<FloatKey> keys;
                  data->SetScaleType(TBC_KEY);
                  nkeys += GetKeys<FloatKey, ITCBFloatKey>(c, keys, range);
                  data->SetScaleKeys(keys);
               }
               if (nkeys != 0) { // if no changes set the base transform
                  keepData = true;
                  //scale = Average(GetScale(tm));
               }           
            }

            // only add transform data object if data actually is present
            if (!keepData) {
               ninode->RemoveController(timeControl);
               timeControl = NULL;
            } else {
               objRefs.insert( StaticCast<NiAVObject>(ninode) );
            }
         }
      }
   }
   return timeControl;
}

bool AnimationExport::exportController(INode *node)
{
   bool ok = true;

   bool keepTM = false;
   if (seq->GetTargetName() == node->GetName()) {
      keepTM = true;
   }

   NiTimeControllerRef control = exportController( node, range, keepTM );
   if (control != NULL)
   {
      NiSingleInterpolatorControllerRef interpControl = DynamicCast<NiSingleInterpolatorController>(control);
      if (interpControl) {
         // Get Priority from node
         float priority;
         npGetProp(node, NP_ANM_PRI, priority, Exporter::mDefaultPriority);
         seq->AddInterpolator(StaticCast<NiSingleInterpolatorController>(control), priority);
      }
      else 
      {
         seq->AddController(control);
      }

      NiObjectNETRef target = control->GetTarget();
      // now remove temporary controller
      target->RemoveController(StaticCast<NiTimeController>(control));
   }

   for (int i=0, n=node->NumberOfChildren(); ok && i<n; ++i)
   {
      INode *child = node->GetChildNode(i);
      ok |= exportController(child);
   }
   return ok;
}