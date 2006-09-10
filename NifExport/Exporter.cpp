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
int Exporter::mNifUserVersion = 0;
bool Exporter::mSkeletonOnly=false;
bool Exporter::mExportCameras=false;
bool Exporter::mGenerateBoneCollision=false;

Exporter::Exporter(Interface *i, AppSettings *appSettings)
   : mI(i), mAppSettings(appSettings)
{
}


Exporter::Result Exporter::doExport(NiNodeRef &root, INode *node)
{
   //root->SetName("Scene Root");

   int nifVersion = GetVersion(Exporter::mNifVersion);
   mIsBethesda = (nifVersion == VER_20_0_0_5 || nifVersion == VER_20_0_0_4) && (Exporter::mNifUserVersion == 11);

   CalcBoundingBox(node, mBoundingBox);

   if (mSkeletonOnly && mIsBethesda)
   {
      BSBoundRef bsb = CreateNiObject<BSBound>();
      bsb->SetName("BBX");    
      bsb->SetCenter( TOVECTOR3(mBoundingBox.Center()) );
      bsb->SetDimensions( TOVECTOR3(mBoundingBox.Width() / 2.0f) );
      root->AddExtraData(DynamicCast<NiExtraData>(bsb));

      BSXFlagsRef bsx = CreateNiObject<BSXFlags>();
      bsx->SetName("BSX");
      bsx->SetFlags( 0x00000007 );
      root->AddExtraData(DynamicCast<NiExtraData>(bsx));
   }
   else if (mExportCollision && mIsBethesda)
   {
	   BSXFlagsRef bsx = CreateNiObject<BSXFlags>();
	   bsx->SetName("BSX");
	   bsx->SetFlags( 0x00000002 );
      root->AddExtraData(DynamicCast<NiExtraData>(bsx));
   }

   bool ok = exportUPB(root, node);
   //if (!ok && Exporter::mExportCollision)
   //{
   //   NiStringExtraDataRef strings = DynamicCast<NiStringExtraData>(CreateBlock("NiStringExtraData"));	
   //   strings->SetName("UPB");
   //   strings->SetData("Ellasticity = 0.300000\r\nFriction = 0.300000\r\nUnyielding = 0\r\nProxy_Geometry = <None>\r\nUse_Display_Proxy = 0\r\nDisplay_Children = 1\r\nDisable_Collisions = 0\r\nInactive = 0\r\nDisplay_Proxy = <None>\r\nMass = 0.000000\r\nSimulation_Geometry = 2\r\nCollision_Groups = 589825\r\n");
   //   root->AddExtraData(DynamicCast<NiExtraData>(strings));
   //}

	mNiRoot = root;
	
	Result result = exportNodes(root, node);
   if (result != Ok)
      return result;
		
	if (mExportCollision)
	{
		result = exportCollision(root, node);
		if (result != Ok)
			return result;
	}
   // handle post export callbacks (like skin)
   for (CallbackList::iterator cb = mPostExportCallbacks.begin(); cb != mPostExportCallbacks.end(); cb = mPostExportCallbacks.erase(cb))
   {
      (*cb)->execute();
      delete (*cb);
   }
   // Remove unreferenced Bones
   if (mRemoveUnreferencedBones)
      removeUnreferencedBones(mNiRoot);
   if (mSortNodesToEnd)
      sortNodes(mNiRoot);

	return Ok;
}

// Primary recursive decent routine
Exporter::Result Exporter::exportNodes(NiNodeRef &parent, INode *node)
{
   bool coll = npIsCollision(node);
   if (coll ||	(node->IsHidden() && !mExportHidden && !coll) || (mSelectedOnly && !node->Selected()))
      return Skip;

   bool local = !mFlattenHierarchy;
   NiNodeRef nodeParent = mFlattenHierarchy ? mNiRoot : parent;

   NiNodeRef newParent;
   TimeValue t = 0;
   ObjectState os = node->EvalWorldState(t); 

   // Always skip bones and bipeds
   SClass_ID scid = node->SuperClassID();
   Class_ID ncid = node->ClassID();
   TSTR nodeName = node->GetName();
   TSTR nodeClass; node->GetClassName(nodeClass);

   // For some unusual reason, bones named Helper are converted to Meshes and 
   //   lose their Bone properties except a new node named Bone seem to show up
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
         newParent = (mExportExtraNodes) ? makeNode(nodeParent, node, local) : nodeParent;

         Result result;
         result = exportMesh(newParent, node, t);
         if (result != Ok)
            return result;
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
   else if (isMeshGroup(node) && local) // only create node if local
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
