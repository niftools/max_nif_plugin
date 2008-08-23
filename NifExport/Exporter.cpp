#include "pch.h"
#include "AppSettings.h"
#include "niutils.h"

#include "obj/BSXFlags.h"
#include "obj/BSBound.h"

int Exporter::mVersion=013;
bool Exporter::mSelectedOnly=false;
bool Exporter::mTriStrips=true;
bool Exporter::mExportHidden=false;
bool Exporter::mExportFurn=true;
bool Exporter::mExportLights=false;
bool Exporter::mVertexColors=true;
float Exporter::mWeldThresh=0.1f;
string Exporter::mTexPrefix="textures";
bool Exporter::mExportCollision=true;
bool Exporter::mRemapIndices=true;
bool Exporter::mUseRegistry=false;
bool Exporter::mExportExtraNodes=false;
bool Exporter::mExportSkin=false;
bool Exporter::mUserPropBuffer=false;
bool Exporter::mFlattenHierarchy=false;
bool Exporter::mRemoveUnreferencedBones=false;
bool Exporter::mSortNodesToEnd=false;
string Exporter::mGameName = "User";
string Exporter::mNifVersion = "20.0.0.5";
int Exporter::mNifVersionInt = VER_20_0_0_5;
int Exporter::mNifUserVersion = 0;
bool Exporter::mSkeletonOnly=false;
bool Exporter::mExportCameras=false;
bool Exporter::mGenerateBoneCollision=false;
bool Exporter::mExportTransforms=true;
float Exporter::mDefaultPriority=0.0f;
Exporter::ExportType Exporter::mExportType = NIF_WO_ANIM;
bool Exporter::mMultiplePartitions=false;
int Exporter::mBonesPerVertex = 4;
int Exporter::mBonesPerPartition = 20;
bool Exporter::mUseTimeTags = false;
bool Exporter::mAutoDetect = true;
bool Exporter::mAllowAccum = true;
string Exporter::mCreatorName;
bool Exporter::mCollapseTransforms = false;
bool Exporter::mZeroTransforms = false;
bool Exporter::mFixNormals = false;
bool Exporter::mTangentAndBinormalExtraData = false;
bool Exporter::mSupportPrnStrings = false;
stringlist Exporter::mRotate90Degrees;
bool Exporter::mSuppressPrompts = false;
bool Exporter::mUseAlternateStripper = false;
float Exporter::bhkScaleFactor = 7.0f;
int Exporter::mTangentAndBinormalMethod = 0;

static bool IsNodeOrParentSelected(INode *node) {
   if (node == NULL)
      return false;
   if (node->Selected())
      return true;
   return IsNodeOrParentSelected(node->GetParentNode());
}

Exporter::Exporter(Interface *i, AppSettings *appSettings)
   : mI(i), mAppSettings(appSettings), mSceneCollisionNode(NULL)
{
   memset(progressCounters, 0, sizeof(progressCounters));
   memset(progressMax, 0, sizeof(progressMax));
}

Exporter::Result Exporter::doExport(NiNodeRef &root, INode *node)
{
   root->SetName("Scene Root");

   int nifVersion = ParseVersionString(Exporter::mNifVersion);
   mIsBethesda = (nifVersion == VER_20_0_0_5 || nifVersion == VER_20_0_0_4) && (Exporter::mNifUserVersion == 11);

   if (mUseTimeTags && nifVersion >= VER_20_0_0_4) {
      throw runtime_error("Time tag sequences are not supported for version 20.0.0.4 or higher.");
   }

   if (!Exporter::mSelectedOnly)
   {
      CalcBoundingBox(node, mBoundingBox);

      if (mIsBethesda)
      {
         if (mSkeletonOnly)
         {
            BSBoundRef bsb = CreateNiObject<BSBound>();
            bsb->SetName("BBX");    
            bsb->SetCenter( TOVECTOR3(mBoundingBox.Center()) );
            bsb->SetDimensions( TOVECTOR3(mBoundingBox.Width() / 2.0f) );
            root->AddExtraData(DynamicCast<NiExtraData>(bsb));

            BSXFlagsRef bsx = CreateNiObject<BSXFlags>();
            bsx->SetName("BSX");
			bsx->SetData( 0x00000007 );
            root->AddExtraData(DynamicCast<NiExtraData>(bsx));
         }
         else if (mExportType != NIF_WO_ANIM)
         {
            BSXFlagsRef bsx = CreateNiObject<BSXFlags>();
            bsx->SetName("BSX");
            bsx->SetData( 0x00000003 );
            root->AddExtraData(DynamicCast<NiExtraData>(bsx));
         }
         else if (mExportCollision)
         {
            BSXFlagsRef bsx = CreateNiObject<BSXFlags>();
            bsx->SetName("BSX");
            bsx->SetData( 0x00000002 );
            root->AddExtraData(DynamicCast<NiExtraData>(bsx));
         }
      }
      exportUPB(root, node);
   }

   // Always Scan for Collision Nodes first
   scanForIgnore(node);
   scanForCollision(node);
   scanForAnimation(node);

   mNiRoot = root;
   if (mSelectedOnly) {
      int count = 0;
      int n = mI->GetSelNodeCount();
      vector<INode*> selectedRoots;
      for (int i=0; i<n; ++i) {
         INode * selNode = mI->GetSelNode(i);
         if (!IsNodeOrParentSelected(selNode->GetParentNode())) {
            selectedRoots.push_back(selNode);
            count += countNodes(selNode);
         }
      }
      if (selectedRoots.size() == 0) {
         throw runtime_error("No Nodes have been selected for Export.");
      }

      progressMax[Geometry] = progressMax[Skin] = count;
      if (mExportCollision)
         progressMax[Collision] = progressMax[Geometry];
      if (mExportType == NIF_WO_ANIM)
         progressMax[Animation] = progressMax[Geometry];
     
      for (int i=0; i<selectedRoots.size(); i++){
         Result result = exportNodes(root, selectedRoots[i]);
         if (result != Ok && result != Skip)
            return result;
         if (mExportCollision) {
            result = exportCollision(root, selectedRoots[i]);
            if (result != Ok)
               return result;
         }
      }
      // Always Zero out root transforms
      vector<NiAVObjectRef> children = root->GetChildren();
      for (int i=0; i<children.size(); ++i){
         children[i]->SetLocalTransform(Matrix44::IDENTITY);
      }

      // Fix Used Nodes that where never properly initialized.  Happens normally during select export
      for (NodeMap::iterator itr = mNameMap.begin(); itr != mNameMap.end(); ++itr) {
         NiNodeRef bone = (*itr).second;
         if (bone->GetParent() == NULL) {
            if (INode* boneNode = mI->GetINodeByName((*itr).first.c_str())) {
               makeNode(root, boneNode, false);
            }
         }
      }

      // Special case when exporting a single branch, use first child as scene root
      if (selectedRoots.size() == 1 ) {
         vector<NiNodeRef> childnodes = DynamicCast<NiNode>(root->GetChildren());
         if (childnodes.size() == 1) {
            NiNodeRef child = childnodes[0];
            root->RemoveChild(child);
            root = child;
            mNiRoot = root;
            exportPrn(root, selectedRoots[0]);
         }
      }

   } else {
      // Estimate progress bar
      int count = countNodes(node);
      progressMax[Geometry] = progressMax[Skin] = count;
      if (mExportCollision)
         progressMax[Collision] = progressMax[Geometry];
      if (mExportType == NIF_WO_ANIM)
         progressMax[Animation] = progressMax[Geometry];

      // Normal export
      Result result = exportNodes(root, node);
      if (result != Ok)
         return result;

      // Fix Used Nodes that where never properly initialized.  Happens normally during select export
      for (NodeMap::iterator itr = mNameMap.begin(); itr != mNameMap.end(); ++itr) {
         NiNodeRef bone = (*itr).second;
         if (bone->GetParent() == NULL) {
            if (INode* boneNode = mI->GetINodeByName((*itr).first.c_str())) {
               makeNode(root, boneNode, false);
            }
         }
      }

      if (mExportCollision) {
         result = exportCollision(root, node);
         if (result != Ok)
            return result;
      }
   }

   // handle post export callbacks (like skin)
   progressMax[Skin] = mPostExportCallbacks.size();
   for (CallbackList::iterator cb = mPostExportCallbacks.begin(); cb != mPostExportCallbacks.end(); cb = mPostExportCallbacks.erase(cb)) {
      ProgressUpdate(Skin, NULL);
      (*cb)->execute();
      delete (*cb);
   }
   // Remove unreferenced Bones
   if (mRemoveUnreferencedBones)
      removeUnreferencedBones(mNiRoot);
   if (mSortNodesToEnd)
      sortNodes(mNiRoot);
   ApplyAllSkinOffsets(StaticCast<NiAVObject>(mNiRoot));
   root = mNiRoot;
	return Ok;
}

// Primary recursive decent routine
Exporter::Result Exporter::exportNodes(NiNodeRef &parent, INode *node)
{
   TSTR nodeName = node->GetName();
   //bool coll = npIsCollision(node);
   bool coll = isCollision(node);

   ProgressUpdate(Geometry, FormatText("'%s' Geometry", nodeName.data()));

   // Abort if is a collision node or is hidden and we are not exporting hidden
   if (coll ||	(node->IsHidden() && !mExportHidden) || isHandled(node))
      return Skip;

   bool local = !mFlattenHierarchy;
   NiNodeRef nodeParent = mFlattenHierarchy ? mNiRoot : parent;

   NiNodeRef newParent;
   TimeValue t = 0;
   ObjectState os = node->EvalWorldState(t); 

   // Always skip bones and bipeds
   SClass_ID scid = node->SuperClassID();
   Class_ID ncid = node->ClassID();
   TSTR nodeClass; node->GetClassName(nodeClass);
   if (node->IsBoneShowing())
      newParent = exportBone(nodeParent, node);
   else if (os.obj && os.obj->SuperClassID()==GEOMOBJECT_CLASS_ID)
   {
      TSTR objClass;
      os.obj->GetClassName(objClass);
      SClass_ID oscid = os.obj->SuperClassID();
      Class_ID oncid = os.obj->ClassID();
      if (  os.obj 
         && (  os.obj->ClassID() == BONE_OBJ_CLASSID 
            || os.obj->ClassID() == Class_ID(BONE_CLASS_ID,0)
            || os.obj->ClassID() == Class_ID(0x00009125,0) /* Biped Twist Helpers */
            )
         ) 
      {
         newParent = exportBone(nodeParent, node);
      } 
      else if (!mSkeletonOnly)
      {
         if (mExportType != NIF_WO_ANIM && isNodeTracked(node)) {
            // Create Node + Accum if has Start Track
            newParent = createAccumNode( makeNode(nodeParent, node, local) , node);
         } else if ( mExportExtraNodes || (mExportType != NIF_WO_ANIM && isNodeKeyed(node) ) ) {
            // Create node if using Extra Nodes or if exporting with anim and node has key values
            newParent = makeNode(nodeParent, node, local);
         } else {
            // Else don't create a node
            newParent = nodeParent;
         }
         // No need to export meshes when NIF is not exported.
         if (mExportType != SINGLE_KF_WO_NIF && mExportType != MULTI_KF_WO_NIF)
         {
            Result result = exportMesh(newParent, node, t);
            if (result != Ok)
               return result;
         }
      }
   }
   else if (mExportCameras && os.obj && os.obj->SuperClassID()==CAMERA_CLASS_ID)
   {
      newParent = makeNode(nodeParent, node, local);
   }
   else if (mExportLights && os.obj && os.obj->SuperClassID()==LIGHT_CLASS_ID)
   {
      return exportLight(nodeParent, node, (GenLight*)os.obj);
   }
   else if (isMeshGroup(node) && local && !mSkeletonOnly) // only create node if local
   {
      newParent = makeNode(parent, node, local);
   } 
   else
      newParent = parent;

   for (int i=0; i<node->NumberOfChildren(); i++) 
   {
      Result result = exportNodes(newParent, node->GetChildNode(i));
      if (result!=Ok && result!=Skip)
         return result;
   }
   return Ok;
}

void Exporter::ProgressUpdate(ProgressSection section, const TCHAR *s)
{
   if (mSuppressPrompts)
      return;

   if (mI->GetCancel()){
      throw CancelExporterException();
   }
   int total = 1;
   int percent = 1;
   for (int i=0; i< int(ProgressSectionCount); i++){
      total += progressMax[i];
      if (i < section) {
         percent += progressMax[i];
      } else if (i == section ) {
         percent += (++progressCounters[i]);
      }
   }
   mI->ProgressUpdate( (percent * 100) / total , s == NULL ? TRUE : FALSE,  const_cast<TCHAR*>(s));
}
