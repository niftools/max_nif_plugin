/**********************************************************************
*<
FILE: ImportAnimation.cpp

DESCRIPTION:	Animation Import Routines

CREATED BY: tazpn (Theo)

HISTORY: 

*>	Copyright (c) 2006, All Rights Reserved.
**********************************************************************/
#include "pch.h"
#if VERSION_3DSMAX >= ((7000<<16)+(15<<8)+0) // Version 7+
#include <IFrameTagManager.h>
#endif
#include <notetrck.h>
#include <set>
#include "NifExport.h"
#include "AnimKey.h"
#ifdef USE_BIPED
#  include <cs/BipedApi.h>
#endif
#include <iiksys.h>

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
#include <obj/NiBSplineTransformInterpolator.h>
#include <obj/NiDefaultAVObjectPalette.h>
#include <obj/NiMultiTargetTransformController.h>
#include <obj/NiGeomMorpherController.h>
#include <obj/NiMorphData.h>
#include <obj/NiBSplineCompFloatInterpolator.h>
#include <obj/NiFloatInterpolator.h>
#include <obj/NiFloatData.h>
using namespace Niflib;

const Class_ID IPOS_CONTROL_CLASS_ID = Class_ID(0x118f7e02,0xffee238a);

enum {
   IPOS_X_REF	=	0,
   IPOS_Y_REF	=	1,
   IPOS_Z_REF	=	2,
   IPOS_W_REF	=	3,
};

static void GetTimeRange(Control *c, Interval& range);
static Interval GetTimeRange(INode *node);

struct AnimationExport
{
   AnimationExport(Exporter& parent) : ne(parent) { }

   INode * findTrackedNode(INode *root);

   bool doExport(NiControllerSequenceRef seq);
   bool doExport(NiControllerManagerRef ctrl, INode *node);
   bool exportController(INode *node, Exporter::AccumType accumType);
   Control *GetTMController(INode* node);
   NiTimeControllerRef exportController(INode *node, Interval range, bool setTM );

   bool SampleAnimation( INode * node, Interval &range, PosRotScale prs, NiKeyframeDataRef data );
   bool GetTextKeys(INode *node, vector<StringKey>& textKeys);
   bool splitAccum(NiTransformDataRef base, NiTransformDataRef accum, Exporter::AccumType accumType);
   void GetTimeRange(Control *c, Interval& range);
   Interval GetTimeRange(INode *node);

   Exporter &ne;
   Interval range;
   NiControllerSequenceRef seq;

   set<NiAVObjectRef> objRefs;
   map<NiControllerSequenceRef, Interval> ranges;
};

float QuatDot(const Quaternion& q, const Quaternion&p)
{
   return q.w*p.w + q.x*p.x + q.y*p.y + q.z*p.z;
}

bool GetTextKeys(INode *node, vector<StringKey>& textKeys, Interval range)
{
   // Populate Text keys and Sequence information from note tracks
   if (Exporter::mUseTimeTags) {
#if VERSION_3DSMAX >= ((7000<<16)+(15<<8)+0) // Version 7+
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
#endif
   } else {
      for (int i=0, n=node->NumNoteTracks(); i<n; ++i) {
         if ( NoteTrack *nt = node->GetNoteTrack(i) ) {
            if ( nt->ClassID() == Class_ID(NOTETRACK_CLASS_ID,0) ) {
               DefNoteTrack *defNT = (DefNoteTrack *)nt;
               if ( defNT->NumKeys() > 0 ) {
                  bool stop = false;
                  for (int j=0, m=defNT->keys.Count(); j<m && !stop; ++j) {
                     NoteKey* key = defNT->keys[j];

                     stringlist args = TokenizeCommandLine(key->note, true);
                     if (args.empty()) continue;

                     bool isStart = false, isEnd = false;
                     if ( stricmp(args[0].c_str(), "start") == 0) {
                        isStart = true;
                     } else if ( args.size() > 1 && stricmp(args[1].c_str(), "start") == 0 ) {
                        isStart = true;
                     }else if ( stricmp(args[0].c_str(), "end") == 0) {
                        isEnd = true;
                        stop = true;
                     } else if ( args.size() > 1 && stricmp(args[1].c_str(), "stop") == 0 ) {
                        isEnd = true;
                     }

                     if (range.Empty()) {
                        range.SetInstant(key->time);
                     } else {
                        if (key->time < range.Start())
                           range.SetStart(key->time);
                        if (key->time > range.End())
                           range.SetEnd(key->time);
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


// Callback interface to register a Skin after entire structure is built due to 
//    constraints in the nif library
struct SampledAnimationCallback : public Exporter::NiCallback
{  
   Exporter *     owner;
   // Common Data
   INode*         maxNode;
   NiNodeRef      nifNode;
   Interval       maxRange;

   // Either keyframe or transform must be present and will have the time frame
   NiKeyframeControllerRef keyframeController;
   NiTransformControllerRef transformController;
   NiTransformInterpolatorRef transformInterp;
   vector<Vector3Key> posKeys;
   vector<QuatKey> rotKeys;
   vector<float> scaleKeys;

   SampledAnimationCallback(Exporter *Owner) : owner(Owner) {}
   virtual ~SampledAnimationCallback() {}
   virtual Exporter::Result execute();
};


void Exporter::InitializeTimeController(NiTimeControllerRef ctrl, NiNodeRef parent)
{
   ctrl->SetFrequency(1.0f);
   ctrl->SetStartTime(FloatINF);
   ctrl->SetStopTime(FloatNegINF);
   ctrl->SetPhase(0.0f);
   ctrl->SetFlags(0x0C);
   //ctrl->SetTarget( parent );
   parent->AddController(DynamicCast<NiTimeController>(ctrl));
}

NiNodeRef Exporter::createAccumNode(NiNodeRef parent, INode *node)
{
   //
   //Tweak by veryxRV
   //
   NiNodeRef accumNode;
   bool isTracked = isNodeTracked(node);
   if (!Exporter::mAllowAccum || (!isTracked && !isSkeletonRoot(node)))
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
         ctrl->SetExtraTargets( DynamicCast<NiAVObject>(children) );
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
                     // Versions less than 20.0.0.4 will always export
                     if (wildmatch("*start*", key->note) ) {
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

Exporter::Result Exporter::scanForAnimation(INode *node)
{
	if (NULL == node) 
		return Exporter::Skip;

   Interval nodeRange = GetTimeRange(node);
   if (!nodeRange.Empty())
   {
      if (globalRange.Empty())
         globalRange = nodeRange;
      if (  globalRange.Empty() || nodeRange.Start() < globalRange.Start() )
         globalRange.SetStart(nodeRange.Start());
      if (  globalRange.Empty() || nodeRange.End() > globalRange.End() )
         globalRange.SetEnd(nodeRange.End());
   }

	// Ideally check for Morph: targets
#if VERSION_3DSMAX >= ((8000<<16)+(15<<8)+0) // Version 8+
	if (Modifier * mod = GetMorpherModifier(node)){
		int idx = -1;
		for (int i=1; i<=100; ++i) {
			if (MorpherIsActive(mod, i) && MorpherHasData(mod, i)) {
				TSTR str = MorpherGetName(mod, i);
				int nodes = MorpherNumProgMorphs(mod, i);
				for (int j=1; j<=nodes; j++)
				{
					if (INode *morph = MorpherGetProgMorph(mod, i, j))
					{
						markAsHandled(morph);
					}
				}
			}
		}
	}
#endif

   for (int i=0; i<node->NumberOfChildren(); i++) {
      scanForAnimation(node->GetChildNode(i));
   }
	return Exporter::Ok;
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
#if VERSION_3DSMAX > ((5000<<16)+(15<<8)+0) // Version 5
      if (Control *sc = c->GetWController()) { 
         if (sc != c && HasKeys(sc)) 
            return true;
      }
#endif
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

   Exporter::AccumType accumType = Exporter::AT_NONE;

   // Populate Text keys and Sequence information from note tracks
   if (Exporter::mUseTimeTags) {
#if VERSION_3DSMAX >= ((7000<<16)+(15<<8)+0) // Version 7
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
#endif
   } else {
      for (int i=0, n=node->NumNoteTracks(); i<n; ++i) {
         if ( NoteTrack *nt = node->GetNoteTrack(i) ) {
            if ( nt->ClassID() == Class_ID(NOTETRACK_CLASS_ID,0) ) {
               DefNoteTrack *defNT = (DefNoteTrack *)nt;
               if ( defNT->NumKeys() > 0 ) {
                  bool stop = false;
                  for (int j=0, m=defNT->keys.Count(); j<m && !stop; ++j) {
                     NoteKey* key = defNT->keys[j];

                     stringlist args = TokenizeCommandLine(key->note, true);
                     if (args.empty()) continue;

                     bool isStart = false, isEnd = false;
                     if ( stricmp(args[0].c_str(), "start") == 0) {
                        isStart = true;
                     } else if ( args.size() > 1 && stricmp(args[1].c_str(), "start") == 0 ) {
                        isStart = true;
                        const char *s = args[0].c_str();
                        const char *p = strchr(s, ':');
                        string name(p == NULL ? s : s, p-s);
                        seq->SetName( name );
                     }else if ( stricmp(args[0].c_str(), "end") == 0) {
                        isEnd = true;
                     } else if ( args.size() > 1 && stricmp(args[1].c_str(), "stop") == 0 ) {
                        isEnd = true;
                     }

                     if (isStart) {
                        seq->SetStartTime(0.0f);
                        range.SetStart( key->time );
                        for (stringlist::iterator itr = args.begin(); itr != args.end(); ++itr) {
                           if (strmatch("-name", *itr)) {
                              if (++itr == args.end()) break;
                              seq->SetName(*itr);
                           } else if (strmatch("-loop", *itr)) {
                              seq->SetCycleType(CYCLE_LOOP);
						         } else if (strmatch("-at", *itr)) {
							         if (++itr == args.end()) break;
							         string type = (*itr);
							         if (strmatch(type, "none")) {
								         accumType = Exporter::AT_NONE;
							         } else {
								         for (size_t j=0; j<type.size(); ++j) {
									         if (tolower(type[j]) == 'x')
										         accumType = Exporter::AccumType(accumType | Exporter::AT_X);
									         else if (tolower(type[j]) == 'y')
										         accumType = Exporter::AccumType(accumType | Exporter::AT_Y);
									         else if (tolower(type[j]) == 'z')
										         accumType = Exporter::AccumType(accumType | Exporter::AT_Z);
								         }
							         }
						         }
                        }
                     } else if ( isEnd ) {
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
   // keys without explicit start/stop will 
   if (!textKeys.empty() && seq->GetStartTime() == FloatINF)
   {
      seq->SetStartTime(0.0f);
      seq->SetStopTime(textKeys.back().time);
   }

   textKeyData->SetKeys(textKeys);

   // Now let the fun begin.

   return exportController(node, accumType);
}

bool AnimationExport::doExport(NiControllerManagerRef mgr, INode *node)
{
   int start = 0;
   NiDefaultAVObjectPaletteRef objPal = new NiDefaultAVObjectPalette();
   mgr->SetObjectPalette(objPal);


   vector<NiControllerSequenceRef> seqs;
   vector<StringKey> textKeys;
   NiControllerSequenceRef curSeq;
   Exporter::AccumType accumType = Exporter::AT_FORCE;

   // Populate Text keys and Sequence information from note tracks
   if (Exporter::mUseTimeTags) {
#if VERSION_3DSMAX >= ((7000<<16)+(15<<8)+0) // Version 7
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
#endif
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

                     stringlist args = TokenizeCommandLine(key->note, true);
                     if (args.empty()) continue;

                     bool isStart = false, isEnd = false;
                     string startName;
                     if ( stricmp(args[0].c_str(), "start") == 0) {
                        isStart = true;
                     } else if ( args.size() > 1 && stricmp(args[1].c_str(), "start") == 0 ) {
                        isStart = true;
                        const char *s = args[0].c_str();
                        const char *p = strchr(s, ':');
                        startName = string(p == NULL ? s : s, p-s-1);
                     }else if ( stricmp(args[0].c_str(), "end") == 0) {
                        isEnd = true;
                     } else if ( args.size() > 1 && stricmp(args[1].c_str(), "stop") == 0 ) {
                        isEnd = true;
                     }

                     if (isStart) {
                        textKeys.clear();

                        curSeq = new NiControllerSequence();
                        curSeq->SetStartTime(FloatINF);
                        curSeq->SetStopTime(FloatINF);
                        curSeq->SetFrequency(1.0f);
                        curSeq->SetCycleType( CYCLE_CLAMP );
                        curSeq->SetTargetName( node->GetName() );
                        curSeq->SetName(startName);
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
                           } else if (strmatch("-at", *itr)) {
                              if (++itr == args.end()) break;
                              string type = (*itr);
                              if (strmatch(type, "none")) {
                                 accumType = Exporter::AT_NONE;
                              } else {
                                 for (size_t j=0; j<type.size(); ++j) {
                                    if (tolower(type[j]) == 'x')
                                       accumType = Exporter::AccumType( accumType | Exporter::AT_X );
                                    else if (tolower(type[j]) == 'y')
                                       accumType = Exporter::AccumType( accumType | Exporter::AT_Y );
                                    else if (tolower(type[j]) == 'z')
                                       accumType = Exporter::AccumType( accumType | Exporter::AT_Z );
                                 }
                              }
                           }
                        }
                     }

                     StringKey strkey;
                     strkey.time = FrameToTime( Interval(range.Start(), key->time).Duration()-1 );
                     strkey.data = key->note;
                     textKeys.push_back(strkey);

                     if ( isEnd ) {
                        range.SetEnd( key->time );

                        // add accumulated text keys to sequence
                        if (curSeq != NULL) {
                           curSeq->SetStopTime( FrameToTime( range.Duration()-1 ) );

                           this->ranges[curSeq] = range;

                           NiTextKeyExtraDataRef textKeyData = new NiTextKeyExtraData();
                           curSeq->SetTextKey(textKeyData);
                           curSeq->SetTextKeys(textKeyData);
                           curSeq->SetTextKeysName( curSeq->GetTargetName() );
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
      bool ok = exportController(node, accumType);
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
   return c;
}

void Exporter::GetTimeRange(Control *c, Interval& range)
{
   //if ( Exporter::mExportType == Exporter::NIF_WO_ANIM ) 
   //   return;

	//Class_ID cID = tmCont->ClassID();
	//if ( (cID == BIPSLAVE_CONTROL_CLASS_ID) 
	//	|| (cID == BIPBODY_CONTROL_CLASS_ID) 
	//	|| (cID == FOOTPRINT_CLASS_ID) )
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
         if (range.Empty())
            range = globalRange;
         if (  !globalRange.Empty() && range.Start() > globalRange.Start() )
            range.SetStart(globalRange.Start());
         if (  !globalRange.Empty() && range.End() < globalRange.End() )
            range.SetEnd(globalRange.End());

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
#if VERSION_3DSMAX >= ((5000<<16)+(9<<8)+0) // Version 5+
   if (range.Empty())
   {
      if ( IListControl* listc = GetIListControlInterface(c) )
      {
         if ( listc->GetListCount() > 0 )
         {
            if (range.Empty())
               range = globalRange;
            if (  !globalRange.Empty() && range.Start() > globalRange.Start() )
               range.SetStart(globalRange.Start());
            if (  !globalRange.Empty() && range.End() < globalRange.End() )
               range.SetEnd(globalRange.End());
         }
      }
   }
#endif   
}

Interval Exporter::GetTimeRange(INode *node)
{
   Interval range;
   range.SetEmpty();
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
   return range;
}

void AnimationExport::GetTimeRange(Control *c, Interval& range)
{
   ne.GetTimeRange(c, range);
}

Interval AnimationExport::GetTimeRange(INode *node)
{
   Interval range = ne.GetTimeRange(node);
   if (range.Empty())
   {
      // Allow specific types of controllers to use the global range
      if (Control *tmCont = GetTMController(node))
      {
         Class_ID cID = tmCont->ClassID();
         if (  cID == BIPSLAVE_CONTROL_CLASS_ID
            || cID == BIPBODY_CONTROL_CLASS_ID
            || cID == IKCONTROL_CLASS_ID 
            || cID == IKCHAINCONTROL_CLASS_ID 
            )
         {
            range = ne.globalRange;
         }
      }
   }

   return range;
}


NiTimeControllerRef Exporter::CreateController(INode *node, Interval range)
{
   AnimationExport ae(*this);
   if (Exporter::mExportType == Exporter::NIF_WO_KF && isNodeTracked(node)) {
	   NiNodeRef ninode = getNode(node);
	   vector<StringKey> textKeys;
	   if (GetTextKeys(node, textKeys, range)) {
		   NiTextKeyExtraDataRef textKeyData = new NiTextKeyExtraData();
		   ninode->AddExtraData(StaticCast<NiExtraData>(textKeyData), Exporter::mNifVersionInt);
		   textKeyData->SetKeys(textKeys);
	   }
   }
   if ( NiTimeControllerRef tc = ae.exportController(node, range, false) ) {
      return tc;
   }
   return NiTimeControllerRef();
}

NiTimeControllerRef AnimationExport::exportController(INode *node, Interval range, bool setTM )
{
   bool skip = false;
   NiTimeControllerRef timeControl;
   if (range.Empty())
   {
      range = GetTimeRange(node);
      if (range.Empty())
         return timeControl;
   }

   ne.ProgressUpdate(Exporter::Animation, FormatText("'%s' Animation", node->GetName()));

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
         //Vector3 trans(FloatNegINF, FloatNegINF, FloatNegINF);
         //Quaternion rot(FloatNegINF, FloatNegINF, FloatNegINF, FloatNegINF);
         float scale = FloatNegINF;
         Vector3 trans = TOVECTOR3(tm.GetTrans());
         Quaternion rot = TOQUAT( Quat(tm), true );

         NiNodeRef ninode = ne.getNode(node);
         if (setTM) {
            trans = TOVECTOR3(tm.GetTrans());
            rot = TOQUAT( Quat(tm), true );
         }
         string name = ninode->GetName();

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
            NiTransformDataRef tdata = new NiTransformData();
            data = StaticCast<NiKeyframeData>(tdata);
            control->SetInterpolator(StaticCast<NiInterpolator>(interp));

            interp->SetTranslation(trans);
            interp->SetScale(scale);
            interp->SetRotation(rot);
            interp->SetData(tdata);

            timeControl = StaticCast<NiTimeController>(control);
         }
         timeControl->SetStartTime( 0.0f );
         timeControl->SetStopTime( FrameToTime( range.Duration()-1 ) );

         Class_ID cID = tmCont->ClassID();
         if (range.Empty() || (range.Start() == range.End()))
            return timeControl;

#ifdef USE_BIPED
         if (cID == BIPSLAVE_CONTROL_CLASS_ID) 
         {
            bool doprint = false;
            TSTR name = node->NodeName();
            if (name == TSTR("Bip01 R Clavicle"))
            {
               name = name;
               doprint = true;
            }
            // query MAX for the number of keyframes
            int iNumKeys = tmCont->NumKeys();

            vector<Vector3Key> posKeys;
            vector<QuatKey> rotKeys;
            TimeValue interval = (range.Duration()) / TicksPerFrame;

            Quaternion prevq;
            for (TimeValue t = range.Start(); t <= range.End(); t += interval)
            {
               //TimeValue t = tmCont->GetKeyTime(i);
               Matrix3 tm = ne.getNodeTransform(node, t, true);
               Vector3Key pk;
               QuatKey qk;
               qk.time = pk.time = FrameToTime(t);
               pk.data = TOVECTOR3(tm.GetTrans());
               qk.data = TOQUAT( Quat(tm), true );

               if (t != range.Start())
               {
                  if ( QuatDot(qk.data, prevq) < 0.0f )
                     qk.data.Set(-qk.data.w, -qk.data.x, -qk.data.y, -qk.data.z);
               }
               prevq = qk.data;

               posKeys.push_back( pk );
               rotKeys.push_back( qk );
            }

            // Dont really know what else to use since I cant get anything but the raw data.
            data->SetTranslateType(LINEAR_KEY);
            data->SetTranslateKeys(posKeys);
            data->SetRotateType(LINEAR_KEY);
            data->SetQuatRotateKeys(rotKeys);
            //data->SetScaleKeys();
            if (iNumKeys != 0) { // if no changes set the base transform
               keepData = true;
            }
         }
         else if (cID == BIPBODY_CONTROL_CLASS_ID) 
         {
            Animatable* vert = tmCont->SubAnim(VERTICAL_SUBANIM);
            Animatable* horiz = tmCont->SubAnim(HORIZONTAL_SUBANIM);
            Animatable* rot = tmCont->SubAnim(ROTATION_SUBANIM);
            int iVertKeys = vert->NumKeys();
            int iHorizKeys = horiz->NumKeys();
            int iRotKeys = rot->NumKeys();

            // merge vertical and horizontal. rotation stands alone
            vector<Vector3Key> posKeys;
            vector<QuatKey> rotKeys;
            set<TimeValue> times;
            for (int i = 0; i < iVertKeys; i++)
               times.insert(vert->GetKeyTime(i));
            for (int i = 0; i < iHorizKeys; i++)
               times.insert(horiz->GetKeyTime(i));
            for (set<TimeValue>::iterator itr = times.begin(); itr != times.end(); ++itr)
            {
               TimeValue t = *itr;
               Matrix3 tm = ne.getNodeTransform(node, t, true);
               Vector3Key p;
               p.time = FrameToTime(t+range.Start());
               p.data = TOVECTOR3(tm.GetTrans());
               posKeys.push_back( p );
            }
            Quaternion prevq;
            for (int i = 0; i < iRotKeys; i++)
            {
               TimeValue t = rot->GetKeyTime(i);
               Matrix3 tm = ne.getNodeTransform(node, t, true);

               Point3 p, s; Quat q;
               DecomposeMatrix(tm, p, q, s);

               QuatKey qk;
               qk.time = FrameToTime(t+range.Start());
               qk.data = TOQUAT( Quat(tm), true );

               if (t != range.Start())
               {
                  if ( QuatDot(qk.data, prevq) < 0.0f )
                     qk.data.Set(-qk.data.w, -qk.data.x, -qk.data.y, -qk.data.z);
               }
               prevq = qk.data;

               rotKeys.push_back( qk );
            }
            // Dont really know what else to use since I cant get anything but the raw data.
            data->SetTranslateType(LINEAR_KEY);
            data->SetTranslateKeys(posKeys);
            data->SetRotateType(LINEAR_KEY);
            data->SetQuatRotateKeys(rotKeys);
            if (posKeys.size() != 0 || rotKeys.size() != 0) { // if no changes set the base transform
               keepData = true;
            }

         }
#endif
         else if (cID == IKCONTROL_CLASS_ID || cID == IKCHAINCONTROL_CLASS_ID )
         {
            keepData |= SampleAnimation(node, range, prsDefault, data);
         }

         //if (validity.InInterval(range))
         //{
         //   // Valid for entire interval.  i.e. no changes
         //   interp->SetTranslation( TOVECTOR3(tm.GetTrans()) );
         //   interp->SetScale( Average(GetScale(tm)) );
         //   interp->SetRotation( TOQUAT( Quat(tm) ) );
         //   seq->AddInterpolator(StaticCast<NiSingleInterpController>(control));
         //}
         else
         {
            if (Control *c = tmCont->GetPositionController()) 
            {
               int nkeys = 0;


#if VERSION_3DSMAX >= ((5000<<16)+(9<<8)+0) // Version 5+
               if ( IListControl* listc = GetIListControlInterface(c) )
               {
                  keepData = SampleAnimation(node, range, prsPos, data);
               }
#endif
               // separate xyz
               if (keepData)
               {
                  // already handled,  just skip below
               }
               else if (c->ClassID() == IPOS_CONTROL_CLASS_ID) 
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
                     keepData = SampleAnimation(node, range, prsPos, data);
                  }
                  if (!keys.empty())
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
                  // XYZ can have adverse affects if no keys are present
                  if (nkeys == 0)
                     data->SetRotateType(QUADRATIC_KEY);
               } else if (c->ClassID() == Class_ID(TCBINTERP_ROTATION_CLASS_ID,0)) {
                  vector<QuatKey> keys;
                  data->SetRotateType(TBC_KEY);
                  nkeys += GetKeys<QuatKey, ITCBRotKey>(c, keys, range);
                  data->SetQuatRotateKeys(keys);
               } else {
                  keepData = SampleAnimation(node, range, prsRot, data);
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

                  // I think max has a bug in the Bezier Scale control w.r.t. the keycontroller.
                  float timeOffset = -FrameToTime(range.Start());
                  for (int i=0, n=keys.size(); i<n; ++i) {
                     FloatKey k = InterpKey<FloatKey>(c, TimeToFrame(keys[i].time-timeOffset), timeOffset);
                     keys[i].data = k.data;
                  }

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
   return timeControl;
}

//////////////////////////////////////////////////////////////////////////
// Delayed Animation sampling
Exporter::Result SampledAnimationCallback::execute()
{
   return Exporter::Skip;
}

bool AnimationExport::splitAccum(NiTransformDataRef base, NiTransformDataRef accum, Exporter::AccumType accumType)
{
	vector<Vector3Key> baseTrans = base->GetTranslateKeys();
	vector<Vector3Key> accmTrans; accmTrans.resize( baseTrans.size() );
	accum->SetTranslateType( base->GetTranslateType() );

	for (int i=0, n=baseTrans.size(); i<n; ++i) {
		accmTrans[i] = baseTrans[i];
		accmTrans[i].data = Vector3(0.0f, 0.0f, 0.0f);
		if (accumType & Exporter::AT_X) {
			swap(accmTrans[i].data.x, baseTrans[i].data.x);
		}
		if (accumType & Exporter::AT_Y) {
			swap(accmTrans[i].data.y, baseTrans[i].data.y);
		}
		if (accumType & Exporter::AT_Z) {
			swap(accmTrans[i].data.z, baseTrans[i].data.z);
		}
	}
	base->SetTranslateKeys(accmTrans);
	accum->SetTranslateKeys(baseTrans);

	// Rotate
	accum->SetRotateType( base->GetRotateType() );
	accum->SetQuatRotateKeys( base->GetQuatRotateKeys() );
	base->SetQuatRotateKeys( vector<QuatKey>() );

	accum->SetXRotateType( base->GetXRotateType() );
	accum->SetXRotateKeys( base->GetXRotateKeys() );
	base->SetXRotateKeys( vector<FloatKey>() );

	accum->SetYRotateType( base->GetYRotateType() );
	accum->SetYRotateKeys( base->GetYRotateKeys() );
	base->SetYRotateKeys( vector<FloatKey>() );

	accum->SetZRotateType( base->GetZRotateType() );
	accum->SetZRotateKeys( base->GetZRotateKeys() );
	base->SetZRotateKeys( vector<FloatKey>() );

	// Scale
	accum->SetScaleType( base->GetScaleType() );
	accum->SetScaleKeys( base->GetScaleKeys() );
	base->SetScaleKeys( vector<FloatKey>() );

	return true;
}

bool AnimationExport::exportController(INode *node, Exporter::AccumType accumType)
{
   bool ok = true;

   bool keepTM = false;
   if (seq->GetTargetName() == node->GetName()) {
      keepTM = true;
   }

   NiTimeControllerRef control = exportController( node, range, keepTM );
   if (control != NULL)
   {
      NiSingleInterpControllerRef interpControl = DynamicCast<NiSingleInterpController>(control);
      if (interpControl) 
      {
         if (Exporter::mNifVersionInt >= VER_10_2_0_0)
         {
            // Get Priority from node
            float priority;
            npGetProp(node, NP_ANM_PRI, priority, Exporter::mDefaultPriority);
            seq->AddInterpolator(StaticCast<NiSingleInterpController>(control), (Niflib::byte)priority);
         }
         else
         {
            seq->AddController(node->GetName(), control);
            if (NiKeyframeControllerRef ctrl = DynamicCast<NiKeyframeController>(control))
            {
               CycleType ct = seq->GetCycleType();
               ctrl->SetFlags( (ctrl->GetFlags() & ~0x7) | (1 << seq->GetCycleType()) );
            }
         }

         // Handle NonAccum 
         if (Exporter::mAllowAccum && accumType != Exporter::AT_NONE)
         {
            NiNodeRef accnode = ne.getNode( FormatString("%s NonAccum", node->GetName()) );
            objRefs.insert( StaticCast<NiAVObject>(accnode) );

            if (Exporter::mNifVersionInt >= VER_10_2_0_0)
            {
               NiTransformInterpolatorRef interp = DynamicCast<NiTransformInterpolator>(interpControl->GetInterpolator());
               NiTransformControllerRef acccontrol = new NiTransformController();
               NiTransformInterpolatorRef accinterp = new NiTransformInterpolator();
               accnode->AddController(StaticCast<NiTimeController>(acccontrol));
               acccontrol->SetInterpolator(StaticCast<NiInterpolator>(accinterp));

               accinterp->SetTranslation( Vector3(0.0f, 0.0f, 0.0f) );
               accinterp->SetScale( 1.0f );
               accinterp->SetRotation( Quaternion(1.0f, 0.0f, 0.0f, 0.0f) );

               if (interp != NULL) {
                  NiTransformDataRef accumData = new NiTransformData();
                  // Transfer entire data to accum node
                  if ((accumType & Exporter::AT_XYZ) == Exporter::AT_XYZ) {
                     accinterp->SetData( interp->GetData() );
                     interp->SetData( accumData );
                  } else {
                     accinterp->SetData( accumData );
                     splitAccum(interp->GetData(), accumData, accumType);
                  }
               }
               seq->AddInterpolator(StaticCast<NiSingleInterpController>(acccontrol), (Niflib::byte)Exporter::mDefaultPriority);

               accnode->RemoveController(acccontrol);
            }
         }

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
      ok |= exportController(child, Exporter::AT_NONE);
   }
   return ok;
}

bool AnimationExport::SampleAnimation( INode * node, Interval &range, PosRotScale prs, NiKeyframeDataRef data)
{
   bool keepData = false;

   vector<Vector3Key> posKeys;
   vector<QuatKey> rotKeys;
   TimeValue interval = (range.Duration()) / TicksPerFrame;
   Quaternion prevq;
   for (TimeValue t = range.Start(); t <= range.End(); t += interval)
   {
      Matrix3 tm = ne.getNodeTransform(node, t, true);
      Vector3Key p;
      QuatKey q;
      q.time = p.time = FrameToTime(t);
      p.data = TOVECTOR3(tm.GetTrans());
      q.data = TOQUAT( Quat(tm), true );

      if (t != range.Start()){
         if ( QuatDot(q.data, prevq) < 0.0f )
            q.data.Set(-q.data.w, -q.data.x, -q.data.y, -q.data.z);
      }
      prevq = q.data;

      posKeys.push_back( p );
      rotKeys.push_back( q );
   }

   // Dont really know what else to use since I cant get anything but the raw data.
   if (prs & prsPos && !posKeys.empty())
   {
      data->SetTranslateType(LINEAR_KEY);
      data->SetTranslateKeys(posKeys);
      keepData = true;
   }
   if (prs & prsRot && !rotKeys.empty())
   {
      data->SetRotateType(LINEAR_KEY);
      data->SetQuatRotateKeys(rotKeys);
      keepData = true;
   }
   if (prs & prsScale)
   {
      //data->SetScaleKeys();
   }
   return keepData;
}
Exporter::Result Exporter::exportGeomMorpherControl(Modifier* mod, vector<Vector3>& baseVerts, vector<int>& baseVertIdx, NiObjectNETRef owner)
{
	// Check for morphs
	//if ( mExportType != NIF_WO_ANIM ) 
	if (mod != NULL)
	{
		if (mod->IsEnabled()) {
			
			NiGeomMorpherControllerRef ctrl = new NiGeomMorpherController();
			NiMorphDataRef data = new NiMorphData();
			vector<NiInterpolatorRef> interpolators;
			vector<int> indices;
			for (int i=1; i<100; ++i) {
				if (MorpherHasData(mod, i) && MorpherIsActive(mod, i)) {
					indices.push_back(i);
				}
			}
			data->SetMorphCount( indices.size() + 1);
			data->SetFrameName(0, string("Base"));
			data->SetMorphKeyType(0, LINEAR_KEY);

			Interval range; range.SetEmpty();
			
			int nbaseverts = baseVerts.size();
			data->SetVertexCount( nbaseverts );
			data->SetMorphVerts(0, baseVerts);
			if (NiFloatInterpolatorRef interp = new NiFloatInterpolator())
			{
				NiFloatDataRef fdata = new NiFloatData();
				vector<FloatKey> keys;
				keys.resize(2);
				data->SetMorphKeyType(0, LINEAR_KEY);
				data->SetMorphKeys(0, keys);
				fdata->SetKeyType(LINEAR_KEY);
				fdata->SetKeys(keys);
				interp->SetFloatValue(FloatNegINF);
				interp->SetData(fdata);
				interpolators.push_back(interp);
			}

			for (size_t i=0; i<indices.size(); ++i) {
				int idx = indices[i];
				if (IParamBlock* pblock = (IParamBlock*)mod->GetReference(idx)) {
					if (Control *c = pblock->GetController(0)) {
						GetTimeRange(c, range);
					}
				}
			}

			//// Root text keys ???
			//NiNodeRef ninode = this->mNiRoot;
			//vector<StringKey> textKeys;
			//if (GetTextKeys(mI->GetRootNode(), textKeys, range)) {
			//	NiTextKeyExtraDataRef textKeyData = new NiTextKeyExtraData();
			//	ninode->AddExtraData(StaticCast<NiExtraData>(textKeyData), Exporter::mNifVersionInt);
			//	textKeyData->SetKeys(textKeys);
			//}

			for (size_t i=0; i<indices.size(); ++i) {
				int idx = indices[i];

				IParamBlock* pblock = (IParamBlock*)mod->GetReference(idx);
				TSTR name = MorpherGetName(mod, idx);
				data->SetFrameName(i+1, string(name));

				KeyType keyType = LINEAR_KEY;
				vector<FloatKey> keys;
				if (Control *c = pblock->GetController(0)) {
					if (c->ClassID() == Class_ID(LININTERP_FLOAT_CLASS_ID,0)) {
						GetKeys<FloatKey, ILinFloatKey>(c, keys, range);
						keyType = LINEAR_KEY;
					} else if (c->ClassID() == Class_ID(HYBRIDINTERP_FLOAT_CLASS_ID,0)) {
						GetKeys<FloatKey, IBezFloatKey>(c, keys, range);
						keyType = QUADRATIC_KEY;
					} else if (c->ClassID() == Class_ID(TCBINTERP_FLOAT_CLASS_ID,0)) {
						GetKeys<FloatKey, ITCBFloatKey>(c, keys, range);
						keyType = TBC_KEY;
					} else {
						GetKeys<FloatKey, IBezFloatKey>(c, keys, range);
						keyType = QUADRATIC_KEY;
					}
				}
				ScaleKeys(keys, 1.0f/100.0f);
				if (NiFloatInterpolatorRef interp = new NiFloatInterpolator())
				{
					NiFloatDataRef fdata = new NiFloatData();
					data->SetMorphKeyType(i+1, keyType);
					data->SetMorphKeys(i+1, keys);
					fdata->SetKeyType(keyType);
					fdata->SetKeys(keys);
					interp->SetFloatValue(FloatNegINF);
					interp->SetData(fdata);
					interpolators.push_back(interp);
				}

				vector<Vector3> morphMaxVerts, verts;
				MorpherGetMorphVerts(mod, idx, morphMaxVerts);
				// Here's the deal with morphs and sub-meshes. The morpher mod contains
				// in each stage ALL of the vertices of the original mesh before splitting.
				// It also contains only the verts as listed in Max; additional verts created
				// here in the plugin are not included. Additionally the verts are ordered by their
				// index in Max; e.g. index 0 has the coordinates of vertex #1 as shown in the Select
				// Poly modifier for that mesh in Max. The baseVerts set may include those
				// dupe verts and also may not include some of the verts in the morpher mod.
				// Thi is where the vertex index list comes in. For each baseVert in the list,
				// the corresponding Max index is gotten; that integer is then used to index into
				// the morph verts array.
				for (size_t j = 0; j < baseVerts.size(); j++)
				{
					int vertMaxIdx = baseVertIdx[j];
					Vector3 morphPoint = morphMaxVerts[vertMaxIdx] - baseVerts[j];
					verts.push_back(morphPoint);
				}
				data->SetMorphVerts(i+1, verts);
			}
			ctrl->SetData(data);
			if (Exporter::mNifVersionInt >= VER_10_1_0_106)
				ctrl->SetInterpolators(interpolators);
			ctrl->SetFlags( 0x000C );
			ctrl->SetFrequency(1.0f);
			ctrl->SetPhase(0.0f);
			ctrl->SetStartTime( 0.0f );
			ctrl->SetStopTime( FrameToTime( range.Duration()-1 ) );
			owner->AddController(ctrl);

			// Update model bounds for target of the controller
			ctrl->UpdateModelBound();
		}
	}
	return Exporter::Ok;
}

