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
#ifdef USE_BIPED
#include <cs/BipedApi.h>
#endif
#include <obj/NiTriBasedGeom.h>
#include <obj/NiTriBasedGeomData.h>
#include <obj/NiTimeController.h>
#include <obj/NiMultiTargetTransformController.h>
#include <obj/NiStringExtraData.h>
#include <obj/NiBillboardNode.h>
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
   for (unsigned int i = 0; i < blocks.size(); ++i) {
      NiNodeRef node = blocks[i];
      if ( node != NULL && node->IsSkeletonRoot() ) {
         node->GoToSkeletonBindPosition();
      }
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

static void BuildControllerRefList(NiNodeRef node, map<string,int>& ctrlCount)
{
   list<NiTimeControllerRef> ctrls = node->GetControllers();
   for (list<NiTimeControllerRef>::iterator itr = ctrls.begin(), end = ctrls.end(); itr != end; ++itr) {
      list<NiNodeRef> nlist = DynamicCast<NiNode>((*itr)->GetRefs());

      // Append extra targets.  Goes away if GetRefs eventually returns the extra targets
      if (NiMultiTargetTransformControllerRef multiCtrl = DynamicCast<NiMultiTargetTransformController>(*itr)) {
         vector<NiNodeRef> extra = DynamicCast<NiNode>( multiCtrl->GetExtraTargets() );
         nlist.insert(nlist.end(), extra.begin(), extra.end());
      }

      for (list<NiNodeRef>::iterator nitr = nlist.begin(); nitr != nlist.end(); ++nitr){
         string name = (*nitr)->GetName();
         map<string,int>::iterator citr = ctrlCount.find(name);
         if (citr != ctrlCount.end())
            ++(*citr).second;
         else
            ctrlCount[name] = 1;
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
   if (HasSkeleton()){
      NiNodeRef rootNode = root;
      if (rootNode){
         list<NiExtraDataRef> extraData = rootNode->GetExtraData();
         if (!extraData.empty()) {
            if ( BSXFlagsRef flags = SelectFirstObjectOfType<BSXFlags>(extraData) ) {
               return (flags->GetData() & 0x4);
            }
         }
      }
   }
   return false;
}

void NifImporter::ImportBipeds(vector<NiNodeRef>& nodes)
{
#ifdef USE_BIPED
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
         IBipMaster* master = NULL;
         if (Max8CreateNewBiped) {
            master = Max8CreateNewBiped(height, angle, wpos, arms, triPelvis, 
               nnecklinks, nspinelinks, nleglinks, ntaillinks, npony1links, npony2links, 
               numfingers, nfinglinks, numtoes, ntoelinks, bipedAnkleAttach, prop1exists, 
               prop2exists, prop3exists, forearmTwistLinks, upperarmTwistLinks, thighTwistLinks,
               calfTwistLinks, horseTwistLinks);
         } else if (Max7CreateNewBiped) {
            master = Max7CreateNewBiped(height, angle, wpos, arms, triPelvis, 
               nnecklinks, nspinelinks, nleglinks, ntaillinks, npony1links, npony2links, 
               numfingers, nfinglinks, numtoes, ntoelinks, bipedAnkleAttach, prop1exists, 
               prop2exists, prop3exists, forearmTwistLinks);
         }

         if (master)
         {
            master->SetRootName(const_cast<TCHAR*>(bipname.c_str()));

            //if (INode *rootNode = gi->GetINodeByName(bipname.c_str())) {
            //   if (Control *c = rootNode->GetTMController()) {
            //      if (IBipMaster8 *master8 = GetBipMaster8Interface(c)) {
            //         bool ok = master8->SetEulerActive(KEY_LARM, EULERTYPE_XYZ);
            //         ok = ok;
            //      }
            //   }
            //}
            master->BeginModes(BMODE_FIGURE, FALSE);
            master->SetTrianglePelvis(FALSE);
            master->SetDisplaySettings(BDISP_BONES);
            LPCTSTR bipname = master->GetRootName();

            // Rename twists, if necessary for Oblivion
            RenameNode(gi, FormatText("%s L ForeTwist", bipname), FormatText("%s L ForearmTwist", bipname));
            RenameNode(gi, FormatText("%s R ForeTwist", bipname), FormatText("%s R ForearmTwist", bipname));
            RenameNode(gi, FormatText("%s R LUpArmTwist", bipname), FormatText("%s L UpperArmTwist", bipname));
            RenameNode(gi, FormatText("%s R LUpArmTwist", bipname), FormatText("%s R UpperArmTwist", bipname));

            if (NiNodeRef nifBip = FindNodeByName(nodes, bipname))
            {
               AlignBiped(master, nifBip);
            }
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
#endif
}

static float CalcLength(NiNodeRef node, vector<NiAVObjectRef>& children)
{
   bool hasChildren = !children.empty();
   float len = 0.0f;
   if (hasChildren) {
      for (vector<NiAVObjectRef>::iterator itr=children.begin(), end = children.end(); itr != end; ++itr) {
         len += GetObjectLength(*itr);
      }
      len /= float(children.size());
   }
   else
   {
      len = node->GetLocalTranslation().Magnitude();
   }
   return len;
}


static float CalcLength(INode *bone)
{
   int n = bone->NumberOfChildren();
   float len = 0.0f;
   if (n > 0) 
   {
      Matrix3 m = bone->GetNodeTM(0);
      Point3 p = m.GetTrans();
      for (int i = 0; i<n; i++)
      {
         INode *child = bone->GetChildNode(i);
         Matrix3 cm = child->GetObjectTM(0);
         Point3 cp = cm.GetTrans();

         float clen = Length(p-cp);
         len += clen;
      }
      len /= float(n);
   }
   else
   {
      len = Length(GetLocalTM(bone).GetTrans());
   }
   return len;
}

static bool HasBipedPosDOF(LPCTSTR name)
{
   // Check for specific nodes to ignore.  
   //   These are nodes which have a full translation DOF and 
   //   there for do not effect the scale value we are toying with
   return (  wildcmpi("Bip?? ? Clavicle", name) 
      || wildcmpi("Bip?? ? Toe?", name) 
      || wildcmpi("Bip?? ? Finger?", name) 
      || wildcmpi("Bip?? *Twist*", name) 
      );
}

static float CalcScale(INode *bone, NiNodeRef node, vector<NiNodeRef>& children)
{
   int n = bone->NumberOfChildren();
   if (n > 0) 
   {
      float len1 = 0.0f;
      float len2 = 0.0f;
      Matrix3 m = bone->GetNodeTM(0);
      Matrix3 m2 = TOMATRIX3(node->GetWorldTransform());
      for (int i = 0; i<n; i++)
      {
         INode *child = bone->GetChildNode(i);
         LPCTSTR name = child->GetName();
         if (HasBipedPosDOF(name))
            continue;

         Matrix3 cm = child->GetObjectTM(0);
         len1 += Length(m.GetTrans()-cm.GetTrans());

         if (NiNodeRef child2 = FindNodeByName(children, string(child->GetName()))){
            Matrix3 cm2 = TOMATRIX3(child2->GetWorldTransform());
            len2 += Length(m2.GetTrans()-cm2.GetTrans());
         }
      }
      return (len2 != 0.0f && len1 != 0.0f) ? (len2/len1) : 1.0f;
   }
   return 1.0f;
}

#ifdef USE_BIPED
void PosRotScaleBiped(IBipMaster* master, INode *n, Point3 p, Quat& q, float s, PosRotScale prs, TimeValue t = 0)
{
   if (prs & prsScale)
      master->SetBipedScale(TRUE, ScaleValue(Point3(s,s,s)), t, n);
   if (prs & prsRot)
      master->SetBipedRot(q, t, n, FALSE);
   if (prs & prsPos)
      master->SetBipedPos(p, t, n, FALSE);
}

AngAxis CalcAngAxis(Point3 vs, Point3 vf)
{
   Point3 cross = CrossProd(vs, vf);
   // Test for colinear
   if (cross.x < 0.01 && cross.y < 0.01 && cross.z < 0.01)
      return AngAxis(Matrix3(TRUE));
   float dot = DotProd(vs, vf);
   return AngAxis( cross, acos( dot ) );
}

Matrix3 GenerateRotMatrix(AngAxis a)
{
   Matrix3 m(TRUE);
   float u = a.axis.x;
   float v = a.axis.y;
   float w = a.axis.z;
   float rcos = cos(a.angle);
   float rsin = sin(a.angle);
   m.GetRow(0)[0] =      rcos + u*u*(1-rcos);
   m.GetRow(1)[0] =  w * rsin + v*u*(1-rcos);
   m.GetRow(2)[0] = -v * rsin + w*u*(1-rcos);
   m.GetRow(0)[1] = -w * rsin + u*v*(1-rcos);
   m.GetRow(1)[1] =      rcos + v*v*(1-rcos);
   m.GetRow(2)[1] =  u * rsin + w*v*(1-rcos);
   m.GetRow(0)[2] =  v * rsin + u*w*(1-rcos);
   m.GetRow(1)[2] = -u * rsin + v*w*(1-rcos);
   m.GetRow(2)[2] =      rcos + w*w*(1-rcos);
   return m;
}

static AngAxis CalcTransform(INode *bone, NiNodeRef node, vector<NiNodeRef>& children)
{
   Matrix3 mr(TRUE);
   int n = bone->NumberOfChildren();
   if (n > 0) 
   {
      int c = 0;
      Point3 vs(0.0f, 0.0f, 0.0f), vf(0.0f, 0.0f, 0.0f);
      Matrix3 m = bone->GetNodeTM(0);
      Matrix3 m2 = TOMATRIX3(node->GetWorldTransform());
      for (int i = 0; i<n; i++)
      {
         INode *child = bone->GetChildNode(i);
         LPCTSTR name = child->GetName();
         if (HasBipedPosDOF(name))
            continue;

         Matrix3 cm = child->GetObjectTM(0);
         vs += (m.GetTrans()-cm.GetTrans());

         if (NiNodeRef child2 = FindNodeByName(children, string(child->GetName()))){
            Matrix3 cm2 = TOMATRIX3(child2->GetWorldTransform());
            vf += (m2.GetTrans()-cm2.GetTrans());
         }
         ++c;
      }
      vs = FNormalize(vs); 
      vf = FNormalize(vf);
      Point3 cross = CrossProd(vs, vf);
      if (fabs(cross.x) < 0.01 && fabs(cross.y) < 0.01 && fabs(cross.z) < 0.01)
         return AngAxis(Point3(0.0f, 0.0f, 0.0f), 0.0f);
      float dot = DotProd(vs, vf);
      return AngAxis( cross, acos( dot ) );
   }
   return mr;
}
#endif



void NifImporter::AlignBiped(IBipMaster* master, NiNodeRef node)
{
#ifdef USE_BIPED
   NiNodeRef parent = node->GetParent();
   string name = node->GetName();
   vector<NiAVObjectRef> children = node->GetChildren();
   vector<NiNodeRef> childNodes = DynamicCast<NiNode>(children);

   TSTR s1 = FormatText("Processing %s:", name.c_str());
   TSTR s2 = FormatText("Processing %s:", name.c_str());
   INode *bone = GetNode(node);
   if (bone != NULL) 
   {
      if (uncontrolledDummies)
         BuildControllerRefList(node, ctrlCount);

      Matrix44 m4 = node->GetWorldTransform();
      Vector3 pos; Matrix33 rot; float scale;
      m4.Decompose(pos, rot, scale);
      Matrix3 m = TOMATRIX3(m4);
      Point3 p = m.GetTrans();
      Quat q(m);

      s1 += FormatText(" ( %s)", PrintMatrix3(m).data());
      if (strmatch(name, master->GetRootName()))
      {
         // Align COM
         //PosRotScaleNode(bone, p, q, 1.0f, prsPos);
         PosRotScaleBiped(master, bone, p, q, 1.0f, prsPos);
      }
      else if (INode *pnode = bone->GetParentNode())
      {
         // Reparent if necessary
         if (!strmatch(parent->GetName(), pnode->GetName())) {
            if (pnode = FindNode(parent)) {
               bone->Detach(0);
               pnode->AttachChild(bone);
            }
         }

         // Hack to scale the object until it fits
         for (int i=0; i<10; ++i) {
            float s = CalcScale(bone, node, childNodes);
            if (fabs(s-1.0f) < (FLT_EPSILON*100.0f))
               break;
            s1 += FormatText(" (%g)", s);
            master->SetBipedScale(TRUE, ScaleValue(Point3(s,s,s)), 0, bone);
         }
         PosRotScale prs = prsDefault;
         PosRotScaleBiped(master, bone, p, q, scale, prs);

         // Rotation with Clavicle is useless in Figure Mode using the standard interface
         //   I was tring unsuccessfully to correct for it
         //if (wildcmpi("Bip?? ? Clavicle", name.c_str())) {
         //   AngAxis a1 = CalcTransform(bone, node, childNodes);
         //   Matrix3 tm1 = GenerateRotMatrix(a1);
         //   Quat nq = TransformQuat(tm1, q);
         //   PosRotScaleNode(bone, p, nq, scale, prsRot);
         //}
      }
      s2 += FormatText(" ( %s)", PrintMatrix3(bone->GetNodeTM(0)).data());
   }
   else
   {
      ImportBones(node, false);
   }
   for (char *p = s1; *p != 0; ++p) if (isspace(*p)) *p = ' ';
   for (char *p = s2; *p != 0; ++p) if (isspace(*p)) *p = ' ';
   OutputDebugString(s1 + "\n");
   OutputDebugString(s2 + "\n");

   for (vector<NiNodeRef>::iterator itr = childNodes.begin(), end = childNodes.end(); itr != end; ++itr){
      AlignBiped(master, *itr);
   }
#endif
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
#if VERSION_3DSMAX > ((6000<<16)+(15<<8)+0) // Version 6
            n->BoneAsLine(1);
            n->ShowBone(2);
#else
            //n->BoneAsLine(1);
            n->ShowBone(1);
#endif
         }
         return result.n;
      }
      fpBones->ReleaseInterface();
   }
   return NULL;
}

INode *NifImporter::CreateHelper(const string& name, Point3 startPos)
{
   if (DummyObject *ob = (DummyObject *)gi->CreateInstance(HELPER_CLASS_ID,Class_ID(DUMMY_CLASS_ID,0))) {
      const float DUMSZ = 1.0f;
      ob->SetBox(Box3(Point3(-DUMSZ,-DUMSZ,-DUMSZ),Point3(DUMSZ,DUMSZ,DUMSZ)));

      if (INode *n = CreateImportNode(name.c_str(), ob, NULL)) {
         n->SetWireColor( RGB(192,192,192) );

         Quat q; q.Identity();
         PosRotScaleNode(n, startPos, q, 1.0f, prsPos);

#if VERSION_3DSMAX > ((6000<<16)+(15<<8)+0) // Version 6
         n->BoneAsLine(dummyBonesAsLines ? 1 : 0);
         n->ShowBone(2);
#else
         //n->BoneAsLine(1);
         n->ShowBone(1);
#endif

         return n;
      }
   }
   //if (Object *ob = (Object *)gi->CreateInstance(HELPER_CLASS_ID,Class_ID(BONE_CLASS_ID,0))) {
   //   if (INode *n = gi->CreateObjectNode(ob)) {
   //      n->SetName(const_cast<TCHAR*>(name.c_str()));
   //      Quat q; q.Identity();
   //      PosRotScaleNode(n, startPos, q, 1.0f, prsPos);
   //      return n;
   //   }
   //}
   return  NULL;
}

INode *NifImporter::CreateCamera(const string& name)
{
   if (GenCamera *ob = (GenCamera *)gi->CreateInstance(CAMERA_CLASS_ID, Class_ID(SIMPLE_CAM_CLASS_ID,0))) {
      ob->Enable(1);
      ob->NewCamera(0);
      ob->SetFOV(0, TORAD(75.0f));
      if (INode *n = gi->CreateObjectNode(ob)) {
         //if (INode *n = CreateImportNode(name.c_str(), ob, NULL)) {
         n->Hide(TRUE);
         n->BoneAsLine(1);
         return n;
      }
   }
   return NULL;
}

void NifImporter::ImportBones(vector<NiNodeRef>& bones)
{
   for (vector<NiNodeRef>::iterator itr = bones.begin(), end = bones.end(); itr != end; ++itr){
      ImportBones(*itr);
   }
}


static bool HasControllerRef(map<string,int>& ctrlCount, const string& name)
{
   return (ctrlCount.find(name) != ctrlCount.end());
}

static bool HasUserPropBuffer(NiNodeRef node)
{
   if (node) {
      if (NiStringExtraDataRef data = SelectFirstObjectOfType<NiStringExtraData>(node->GetExtraData())){
         if (strmatch(data->GetName(), "UserPropBuffer"))
            return true;
      }
   }
   return false;
}

extern Point3 TOEULER(const Matrix3 &m);
inline Point3 TORAD(const Point3& p){
   return Point3(TORAD(p[0]), TORAD(p[1]), TORAD(p[2]));
}

void NifImporter::ImportBones(NiNodeRef node, bool recurse)
{
   try 
   {
      if (uncontrolledDummies)
         BuildControllerRefList(node, ctrlCount);

      string name = node->GetName();

      vector<NiAVObjectRef> children = node->GetChildren();
      vector<NiNodeRef> childNodes = DynamicCast<NiNode>(children);

      NiAVObject::CollisionType cType = node->GetCollisionMode();
      if (children.empty() && name == "Bounding Box")
         return;

      // Do all node manipulations here
      NiNodeRef parent = node->GetParent();
      string parentname = (parent ? parent->GetName() : "");
      Matrix44 m4 = node->GetWorldTransform();

      // Check for Prn strings and change parent if necessary
      if (supportPrnStrings) {
         list<NiStringExtraDataRef> strings = DynamicCast<NiStringExtraData>(node->GetExtraData());
         for (list<NiStringExtraDataRef>::iterator itr = strings.begin(); itr != strings.end(); ++itr){
            if (strmatch((*itr)->GetName(), "Prn")) {
               parentname = (*itr)->GetData();
               if (INode *pn = GetNode(parentname)){
                  // Apparently Heads tend to need to be rotated 90 degrees on import for 
                  if (!rotate90Degrees.empty() && wildmatch(rotate90Degrees, parentname)) {
                     m4 *= TOMATRIX4(RotateYMatrix(TORAD(90)));
                  }
                  m4 *= TOMATRIX4(pn->GetObjTMAfterWSM(0, NULL));
               }
            }
         }
      }

      float len = node->GetLocalTranslation().Magnitude();

      // Remove NonAccum nodes and merge into primary bone
      if (mergeNonAccum && wildmatch("* NonAccum", name) && parent)
      {
         string realname = name.substr(0, name.length() - 9);
         if (strmatch(realname, parent->GetName()))
         {
            Matrix44 tm = parent->GetLocalTransform() * node->GetLocalTransform();
            name = realname;
            len += tm.GetTranslation().Magnitude();
            parent = parent->GetParent();
         }
      }

      PosRotScale prs = prsDefault;
      Vector3 pos; Matrix33 rot; float scale;
      m4.Decompose(pos, rot, scale);

      Matrix3 im = TOMATRIX3(m4);
      Point3 p = im.GetTrans();
      Quat q(im);
      //q.Normalize();
      Vector3 ppos;
      Point3 zAxis(0,0,0);
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
         ppos = pos + Vector3(len/3.0f, 0.0f, 0.0f);
      }
      Point3 pp(ppos.x, ppos.y, ppos.z);

      Point3 qp = TORAD(TOEULER(im));


      INode *bone = NULL;
      if (!doNotReuseExistingBones) // Games like BC3 reuse the same bone names
      {
         bone = FindNode(node);
         if (bone == NULL) 
            bone = GetNode(name);
      }
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
         bool isDummy = ( (uncontrolledDummies && !HasControllerRef(ctrlCount, name))
            || (!dummyNodeMatches.empty() && wildmatch(dummyNodeMatches, name))
            || (convertBillboardsToDummyNodes && node->IsDerivedType(NiBillboardNode::TYPE))
            );
         if (wildmatch("Camera*", name)) {
            if (enableCameras) {
               if (bone = CreateCamera(name)) {
                  PosRotScaleNode(bone, p, q, scale, prs);
                  bone->Hide(node->GetVisibility() ? FALSE : TRUE);
               }
            }
         }else if (isDummy || createNubsForBones) {
            bone = CreateHelper(name, p);
            PosRotScaleNode(bone, p, q, scale, prs);
            //bone->Hide(node->GetVisibility() ? FALSE : TRUE);
         } else if (bone = CreateBone(name, p, pp, zAxis)) {
            PosRotScaleNode(bone, p, q, scale, prs);
            bone->Hide(node->GetVisibility() ? FALSE : TRUE);
         }
         if (bone)
         {
            if (!parentname.empty())
            {
               if (mergeNonAccum && wildmatch("* NonAccum", parentname)) {
                  parentname = parentname.substr(0, parentname.length() - 9);
               }
               if (INode *pn = GetNode(parentname))
                  pn->AttachChild(bone, 1);
            }
            RegisterNode(node, bone);
         }
      }
      // Import UPB
      if (bone) ImportUPB(bone, node);

      // Import Havok Collision Data surrounding node,  
      //   unfortunately this causes double import of collision so I'm disabling it for now.
      if (enableCollision && node->GetParent()) {
         ImportCollision(node);
      }

      if (bone && recurse)
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

bool NifImporter::ImportUPB(INode *node, Niflib::NiNodeRef block)
{
   if (!importUPB)
      return false;

   bool ok = false;
   if (node && block)
   {
      list<NiStringExtraDataRef> strings = DynamicCast<NiStringExtraData>(block->GetExtraData());
      for (list<NiStringExtraDataRef>::iterator itr = strings.begin(); itr != strings.end(); ++itr){
         if (strmatch((*itr)->GetName(), "UserPropBuffer") || strmatch((*itr)->GetName(), "UPB")) {
            char buffer[1048];
            istringstream istr((*itr)->GetData(), ios_base::out);
            while (!istr.eof()) {
               char *line = buffer;
               buffer[0] = 0;
               istr.getline(buffer, _countof(buffer)-1);
               if (LPTSTR equals = _tcschr(line, TEXT('='))){
                  *equals++ = 0;
                  Trim(line), Trim(equals);
                  if (line[0] && equals[0]){
                     node->SetUserPropString(TSTR(line), TSTR(equals));
                     ok |= true;
                  }
               } else {
                  Trim(line);
                  int len = strlen(line);
                  // Handle bethesda special values?
                  if (len > 0 && line[len-1] == '#'){
                     TSTR buf, value;
                     node->GetUserPropBuffer(buf);
                     value.append(line).append("\r\n").append(buf);
                     if (wildmatch("BSBoneLOD#*", value))
                        value[0] = 'N', value[1] = 'i'; // Use NIBoneLOD to be compatible with Civ4 code
                     node->SetUserPropBuffer(value);
                  }
               }
            }
         }
      }
   }
   return ok;
}