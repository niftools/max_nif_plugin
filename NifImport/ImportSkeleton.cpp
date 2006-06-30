/**********************************************************************
*<
FILE: ImportSkeleton.cpp

DESCRIPTION:	Skeleton import routines

CREATED BY: tazpn (Theo)

HISTORY: 

*>	Copyright (c) 2006, All Rights Reserved.
**********************************************************************/
#include "stdafx.h"
#include "MaxNifImport.h"
#include <obj/NiTriBasedGeom.h>
#include <obj/NiTriBasedGeomData.h>
#include <obj/NiTimeController.h>
#include <float.h>
#include <dummy.h>

using namespace Niflib;

struct NiNodeNameEquivalence : public NumericStringEquivalence
{
   bool operator()(const NiNodeRef& n1, const NiNodeRef& n2) const { 
      return NumericStringEquivalence::operator()(n1->GetName(), n2->GetName());
   }
};

void GoToSkeletonBindPosition(vector<NiNodeRef>& blocks)
{
   //Send all skeleton roots to bind position
   for (uint i = 0; i < blocks.size(); ++i) {
      NiNodeRef node = blocks[i];
      if ( node != NULL && node->IsSkeletonRoot() ) {
         node->GoToSkeletonBindPosition();
      }
   }
}

bool NifImporter::HasSkeleton()
{
   if (!skeletonCheck.empty()){
      vector<NiNodeRef> bipedRoots = SelectNodesByName(nodes, skeletonCheck.c_str());
      return !bipedRoots.empty();
   }
   return false;
}


bool NifImporter::IsBiped()
{
   if (hasSkeleton){
      list<NiExtraDataRef> extraData = nodes[0]->GetExtraData();
      if (!extraData.empty()) {
         return ( DynamicCast<BSBound>(extraData).size() != 0 );
      }
   }
   return false;
}

void NifImporter::ImportBipeds(vector<NiNodeRef>& nodes)
{
   IBipMaster* master = NULL;
   try 
   {
      vector<NiNodeRef> bipedRoots = SelectNodesByName(nodes, "Bip??");
      std::stable_sort(bipedRoots.begin(), bipedRoots.end(), NiNodeNameEquivalence());
      for (vector<NiNodeRef>::iterator bipedItr = bipedRoots.begin(); bipedItr != bipedRoots.end(); ++bipedItr)
      {
         string bipname = (*bipedItr)->GetName();
         string match = bipname + "*";
         vector<NiNodeRef> bipedNodes = SelectNodesByName(nodes, match.c_str());

         float height = this->bipedHeight;
#if USE_CUSTOM_BSBOUND
         list<NiExtraDataRef> extraData = nodes[0]->GetExtraData();
         if (!extraData.empty()) {
            BSBoundRef bound = SelectFirst<BSBound>(extraData);
            if (bound) {
               array<float,6> floats = bound->GetUnknownFloats();
               height = floats[2] * 2.0f;
            }
         }
#endif
         float angle = TORAD(bipedAngle);
         Point3 wpos(0.0f,0.0f,0.0f);
         BOOL arms = (CountNodesByName(bipedNodes, FormatText("%s L UpperArm", bipname.c_str())) > 0) ? TRUE : FALSE;
         BOOL triPelvis = bipedTrianglePelvis ? TRUE : FALSE;
         int nnecklinks=CountNodesByName(bipedNodes, FormatText("%s Neck*", bipname.c_str()));
         int nspinelinks=CountNodesByName(bipedNodes, FormatText("%s Spine*", bipname.c_str()));
         int nleglinks = 3 + CountNodesByName(bipedNodes, FormatText("%s L HorseLink", bipname.c_str()));
         int ntaillinks = CountNodesByName(bipedNodes, FormatText("%s Tail*", bipname.c_str()));
         int npony1links = CountNodesByName(bipedNodes, FormatText("%s Ponytail1*", bipname.c_str()));
         int npony2links = CountNodesByName(bipedNodes, FormatText("%s Ponytail2*", bipname.c_str()));
         int numfingers = CountNodesByName(bipedNodes, FormatText("%s L Finger?", bipname.c_str()));
         int nfinglinks = CountNodesByName(bipedNodes, FormatText("%s L Finger0*", bipname.c_str()));
         int numtoes = CountNodesByName(bipedNodes, FormatText("%s L Toe?", bipname.c_str()));
         int ntoelinks = CountNodesByName(bipedNodes, FormatText("%s L Toe0*", bipname.c_str()));
         BOOL prop1exists = CountNodesByName(bipedNodes, FormatText("%s Prop1", bipname.c_str())) ? TRUE : FALSE;
         BOOL prop2exists = CountNodesByName(bipedNodes, FormatText("%s Prop2", bipname.c_str())) ? TRUE : FALSE;
         BOOL prop3exists = CountNodesByName(bipedNodes, FormatText("%s Prop3", bipname.c_str())) ? TRUE : FALSE;
         int forearmTwistLinks = CountNodesByName(bipedNodes, FormatText("%s L Fore*Twist*", bipname.c_str()));
         int upperarmTwistLinks = CountNodesByName(bipedNodes, FormatText("%s L Up*Twist*", bipname.c_str()));
         int thighTwistLinks = CountNodesByName(bipedNodes, FormatText("%s L Thigh*Twist*", bipname.c_str()));
         int calfTwistLinks = CountNodesByName(bipedNodes, FormatText("%s L Calf*Twist*", bipname.c_str()));
         int horseTwistLinks = CountNodesByName(bipedNodes, FormatText("%s L Horse*Twist*", bipname.c_str()));

         NiNodeRef root = nodes[0];
         IBipMaster* master = Max8CreateNewBiped(height, angle, wpos, arms, triPelvis, 
            nnecklinks, nspinelinks, nleglinks, ntaillinks, npony1links, npony2links, 
            numfingers, nfinglinks, numtoes, ntoelinks, bipedAnkleAttach, prop1exists, 
            prop2exists, prop3exists, forearmTwistLinks, upperarmTwistLinks, thighTwistLinks,
            calfTwistLinks, horseTwistLinks);
         master->SetRootName(const_cast<TCHAR*>(bipname.c_str()));
         if (master)
         {
            master->BeginModes(BMODE_FIGURE, FALSE);
            master->SetTrianglePelvis(FALSE);
            master->SetDisplaySettings(BDISP_BONES);
            LPCTSTR bipname = master->GetRootName();

            // Rename twists, if necessary
            RenameNode(gi, FormatText("%s L ForeTwist", bipname), FormatText("%s L ForearmTwist", bipname));
            RenameNode(gi, FormatText("%s R ForeTwist", bipname), FormatText("%s R ForearmTwist", bipname));
            RenameNode(gi, FormatText("%s R LUpArmTwist", bipname), FormatText("%s L UpperArmTwist", bipname));
            RenameNode(gi, FormatText("%s R LUpArmTwist", bipname), FormatText("%s R UpperArmTwist", bipname));

            NiNodeRef nifBip = FindNodeByName(nodes, bipname);
            //AlignBiped(master, nifBip);
            ScaleBiped(master, nifBip, true);
            PositionBiped(master, nifBip, true);
            RotateBiped(master, nifBip, true);
         }
      }
   }
   catch( exception & e ) 
   {
      e=e;
   }
   catch( ... ) 
   {
   }
   if (master)
      master->EndModes(BMODE_FIGURE, TRUE);
}

void NifImporter::AlignBiped(IBipMaster* master, NiNodeRef block)
{
   string name = block->GetName();
   Matrix44 wt = block->GetWorldTransform();
   Matrix44 lt = block->GetLocalTransform();

   Vector3 pos; Matrix33 rot; float scale;
   wt.Decompose(pos, rot, scale);

   INode *node = gi->GetINodeByName(name.c_str());
   if (node != NULL) 
   {
      Matrix3 m3 = node->GetNodeTM(TimeValue(), NULL); // local translation m
      master->SetBipedPos(Point3(pos.x, pos.y, pos.z), 0, node, TRUE);

      Matrix3 m(rot.rows[0].data, rot.rows[1].data, rot.rows[2].data, Point3(0,0,0));
      Matrix3 im = Inverse(m);

      Point3 p; Quat q; Point3 s;
      DecomposeMatrix(im, p, q, s);
      master->SetBipedRot(q, 0, node, TRUE);
   }
   else
   {

   }
   vector<NiNodeRef> nodes = DynamicCast<NiNode>(block->GetChildren());
   for (vector<NiNodeRef>::iterator itr = nodes.begin(), end = nodes.end(); itr != end; ++itr){
      AlignBiped(master, *itr);
   }
}

void NifImporter::PositionBiped(IBipMaster* master, NiNodeRef block, bool Recurse)
{
   string name = block->GetName();

   Matrix44 wt = block->GetWorldTransform();
   Matrix44 lt = block->GetLocalTransform();

   Vector3 pos; Matrix33 rot; float scale;
   wt.Decompose(pos, rot, scale);

   INode *node = gi->GetINodeByName(name.c_str());
   if (node != NULL) 
   {
      Matrix3 m3 = node->GetNodeTM(TimeValue(), NULL); // local translation m
      master->SetBipedPos(Point3(pos.x, pos.y, pos.z), 0, node, TRUE);

      Matrix3 m(rot.rows[0].data, rot.rows[1].data, rot.rows[2].data, Point3(0,0,0));
      Matrix3 im = Inverse(m);
      Point3 p; Quat q; Point3 s;
      DecomposeMatrix(im, p, q, s);
      master->SetBipedRot(q, 0, node, TRUE);
   }
   else
   {

   }
   if (Recurse)
   {
      vector<NiNodeRef> nodes = DynamicCast<NiNode>(block->GetChildren());
      for (vector<NiNodeRef>::iterator itr = nodes.begin(), end = nodes.end(); itr != end; ++itr){
         PositionBiped(master, *itr, Recurse);
      }
   }
}

void NifImporter::RotateBiped(IBipMaster* master, NiNodeRef block, bool Recurse)
{
   string name = block->GetName();

   Matrix44 wt = block->GetWorldTransform();
   Matrix44 lt = block->GetLocalTransform();

   Vector3 pos; Matrix33 rot; float scale;
   wt.Decompose(pos, rot, scale);

   INode *node = gi->GetINodeByName(name.c_str());
   if (node != NULL) 
   {
      Matrix3 m3 = node->GetNodeTM(TimeValue(), NULL); // local translation m
      master->SetBipedPos(Point3(pos.x, pos.y, pos.z), 0, node, TRUE);

      Matrix3 m(rot.rows[0].data, rot.rows[1].data, rot.rows[2].data, Point3(0,0,0));
      Matrix3 im = Inverse(m);

      Point3 p; Quat q; Point3 s;
      DecomposeMatrix(im, p, q, s);
      master->SetBipedRot(q, 0, node, TRUE);
   }
   else
   {

   }
   if (Recurse)
   {
      vector<NiNodeRef> nodes = DynamicCast<NiNode>(block->GetChildren());
      for (vector<NiNodeRef>::iterator itr = nodes.begin(), end = nodes.end(); itr != end; ++itr){
         RotateBiped(master, *itr, Recurse);
      }
   }
}

void NifImporter::ScaleBiped(IBipMaster* master, NiNodeRef block, bool Recurse)
{
   string name = block->GetName();

   Matrix44 wt = block->GetWorldTransform();
   Matrix44 lt = block->GetLocalTransform();

   Vector3 pos; Matrix33 rot; float scale;
   wt.Decompose(pos, rot, scale);

   INode *node = gi->GetINodeByName(name.c_str());
   if (node != NULL) 
   {
      Matrix3 m3 = node->GetNodeTM(TimeValue(), NULL); // local translation m
      master->SetBipedPos(Point3(pos.x, pos.y, pos.z), 0, node, TRUE);

      Matrix3 m(rot.rows[0].data, rot.rows[1].data, rot.rows[2].data, Point3(0,0,0));
      Matrix3 im = Inverse(m);

      Point3 p; Quat q; Point3 s;
      DecomposeMatrix(im, p, q, s);
      master->SetBipedRot(q, 0, node, TRUE);
   }
   else
   {

   }
   if (Recurse)
   {
      vector<NiNodeRef> nodes = DynamicCast<NiNode>(block->GetChildren());
      for (vector<NiNodeRef>::iterator itr = nodes.begin(), end = nodes.end(); itr != end; ++itr){
         ScaleBiped(master, *itr, Recurse);
      }
   }
}


INode *NifImporter::CreateBone(const string& name, Point3 startPos, Point3 endPos, Point3 zAxis)
{
   if (FPInterface * fpBones = GetCOREInterface(Interface_ID(0x438aff72, 0xef9675ac)))
   {
      FunctionID createBoneID = fpBones->FindFn(TEXT("createBone"));
      FPValue result;
      FPParams params (3, TYPE_POINT3, &startPos, TYPE_POINT3, &endPos, TYPE_POINT3, &zAxis);     
      FPStatus status = fpBones->Invoke(createBoneID, result, &params);
      if (status == FPS_OK && result.type == TYPE_INODE)
      {
         if (INode *n = result.n)
         {
            n->SetName(const_cast<TCHAR*>(name.c_str()));
            float len = Length(endPos-startPos);
            float width = max(minBoneWidth, min(maxBoneWidth, len * boneWidthToLengthRatio));
            if (Object* o = n->GetObjectRef())
            {
               setMAXScriptValue(o->GetReference(0), "width", 0, width);
               setMAXScriptValue(o->GetReference(0), "height", 0, width);
            }
            n->ShowBone(1);
         }
         return result.n;
      }
      fpBones->ReleaseInterface();
   }
   return NULL;
}

INode *NifImporter::CreateHelper(const string& name, Point3 startPos)
{
   //POINTHELP_CLASS_ID
   if (DummyObject *ob = (DummyObject *)gi->CreateInstance(HELPER_CLASS_ID,Class_ID(DUMMY_CLASS_ID,0))) {
      const float DUMSZ = 1.0f;
      ob->SetBox(Box3(Point3(-DUMSZ,-DUMSZ,-DUMSZ),Point3(DUMSZ,DUMSZ,DUMSZ)));
      if (INode *n = gi->CreateObjectNode(ob)) {
         n->SetName(const_cast<TCHAR*>(name.c_str()));
         Quat q; q.Identity();
         PosRotScaleNode(n, startPos, q, 1.0f, prsPos);
         return n;
      }
   }
   return  NULL;
}

void NifImporter::ImportBones(vector<NiNodeRef>& bones)
{
   for (vector<NiNodeRef>::iterator itr = bones.begin(), end = bones.end(); itr != end; ++itr){
      ImportBones(*itr);
   }
}

float GetObjectLength(NiAVObjectRef obj)
{
   float clen = obj->GetLocalTranslation().Magnitude();
   if (clen < (FLT_EPSILON*10)) {
      if (NiTriBasedGeomRef geom = DynamicCast<NiTriBasedGeom>(obj)) {
         if (NiTriBasedGeomDataRef data = geom->GetData()) {
            clen = data->GetRadius() * 2.0f;
         }
      }
   }
   return clen;
}

void NifImporter::ImportBones(NiNodeRef node)
{
   try 
   {
      string name = node->GetName();
      vector<NiAVObjectRef> children = node->GetChildren();
      vector<NiNodeRef> childNodes = DynamicCast<NiNode>(children);

      NiAVObject::CollisionType cType = node->GetCollision();
      if (cType == NiAVObject::ctBoundingBox && children.empty() && name=="Bounding Box")
         return;

      NiNodeRef parent = node->GetParent();

      PosRotScale prs = prsDefault;
      Matrix44 m4 = node->GetWorldTransform();
      Vector3 pos; Matrix33 rot; float scale;
      m4.Decompose(pos, rot, scale);

      Matrix3 im = TOMATRIX3(m4);
      Point3 p = im.GetTrans();
      Quat q(im);
      //q.Normalize();
      Vector3 ppos;
      Point3 zAxis(0,1,0);
      bool hasChildren = !children.empty();
      if (hasChildren) {
         float len = 0.0f;
         for (vector<NiAVObjectRef>::iterator itr=children.begin(), end = children.end(); itr != end; ++itr) {
            len += GetObjectLength(*itr);
         }
         len /= float(children.size());
         ppos = pos + Vector3(len, 0.0f, 0.0f); // just really need magnitude as rotation will take care of positioning
      }
      else if (parent)
      {
         float len = node->GetLocalTranslation().Magnitude();
         ppos = pos + Vector3(len/3.0f, 0.0f, 0.0f);
      }
      Point3 pp(ppos.x, ppos.y, ppos.z);

      INode *bone = gi->GetINodeByName(name.c_str());
      if (bone)
      {
         // Is there a better way of "Affect Pivot Only" behaviors?
         INode *pinode = bone->GetParentNode();
         if (pinode)
            bone->Detach(0,1);
         PosRotScaleNode(bone, p, q, scale, prs);
         if (pinode)
            pinode->AttachChild(bone, 1);
      }
      else
      {
         list<NiTimeControllerRef> ctrls = node->GetControllers();
         if (parent == NULL || parent->GetParent() == NULL || ctrls.empty())
         {
            bone = CreateHelper(name, p);
            if (ctrls.empty())
               bone->Hide(TRUE);
         }
         else if (bone = CreateBone(name, p, pp, zAxis))
         {
            PosRotScaleNode(bone, p, q, scale, prs);
            if (createNubsForBones && childNodes.empty()){
               if (INode *helper = CreateHelper(string().assign(name).append(" Nub"), pp)){
                  helper->Hide(TRUE);
                  bone->AttachChild(helper, 1);
               }
            }
         }
         if (bone)
         {
            if (parent)
            {
               if (INode *pn = gi->GetINodeByName(parent->GetName().c_str()))
                  pn->AttachChild(bone, 1);
            }
            bone->Hide(node->GetHidden() ? TRUE : FALSE);
         }
      }
      if (bone)
      {
         ImportBones(childNodes);
      }
   }
   catch( exception & e ) 
   {
      e=e;
   }
   catch( ... ) 
   {
   }
}