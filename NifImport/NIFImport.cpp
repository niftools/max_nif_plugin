/**********************************************************************
*<
FILE: ImporterCore.cpp

DESCRIPTION:	Core Import helper routines

CREATED BY: tazpn (Theo)

HISTORY: 

*>	Copyright (c) 2006, All Rights Reserved.
**********************************************************************/
#include "stdafx.h"
#include "MaxNifImport.h"

using namespace Niflib;

// Define the standard section names used in the ini file
LPCTSTR NifImportSection = TEXT("MaxNifImport");
LPCTSTR SystemSection = TEXT("System");
LPCTSTR BipedImportSection = TEXT("BipedImport");
LPCTSTR AnimImportSection = TEXT("AnimationImport");
LPCTSTR CollisionSection = TEXT("Collision");

class IBipMaster;
IBipMaster * (_cdecl * Max8CreateNewBiped)(float,float,class Point3 const &,int,int,int,int,int,int,int,int,int,int,int,int,float,int,int,int,int,int,int,int,int) = 0;
IBipMaster * (_cdecl * Max7CreateNewBiped)(float,float,class Point3 const &,int,int,int,int,int,int,int,int,int,int,int,int,float,int,int,int,int) = 0;

NifImporter::NifImporter(const TCHAR *Name,ImpInterface *I,Interface *GI, BOOL SuppressPrompts)
   : BaseImporter()
{
   BaseInit(Name,I,GI,SuppressPrompts);
}

NifImporter::NifImporter()
{
}

INode* NifImporter::CreateImportNode(const char *name, Object *obj, INode* parent)
{
#if USE_IMPORTNODE
	ImpNode* impNode = i->CreateNode();
	impNode->Reference(obj);
	if (INode *n = impNode->GetINode()) {
		n->SetName(const_cast<TCHAR*>(name));
		n->SetObjectRef(obj);
		i->AddNodeToScene(impNode);
		this->RegisterNode(name, n);
		if (parent)
		{
			parent->AttachChild(impNode->GetINode());

			ASSERT(parent == n->GetParentNode());

		}
	}
	return impNode->GetINode();
#else
	if ( INode* n = gi->CreateObjectNode(obj) )
	{
		n->SetName(const_cast<TCHAR*>(name));
		this->RegisterNode(name, n);
		if (parent)
		{
			parent->AttachChild(n);
			ASSERT(parent == n->GetParentNode());
		}
		return n;
	}
	return NULL;
	
#endif
}

void NifImporter::ReadBlocks()
{
   //blocks = ReadNifList( name );
	Niflib::NifInfo info;
	int opts;
	root = ReadNifTree(name, &info);
	nifVersion = info.version;
	userVersion = info.userVersion;
	BuildNodes();
}


static void BuildNodes(NiNodeRef object, vector<NiNodeRef>& nodes)
{
   if (!object)
      return;
   nodes.push_back(object);
   vector<NiNodeRef> links = DynamicCast<NiNode>(object->GetChildren());
   for (vector<NiNodeRef>::iterator itr = links.begin(), end = links.end(); itr != end; ++itr)
      BuildNodes(*itr, nodes);
}

void NifImporter::BuildNodes()
{
   ::BuildNodes(root, nodes);
   std::sort(nodes.begin(), nodes.end(), NodeEquivalence());
}

void NifImporter::Initialize()
{
   // Apply post processing checks after reading blocks
   if (isValid()){
      if (goToSkeletonBindPosition && !nodes.empty() && importBones)
         GoToSkeletonBindPosition(nodes);

      // Only support biped if CreateNewBiped can be found.
      useBiped &= (Max8CreateNewBiped != NULL || Max7CreateNewBiped != NULL);

      hasSkeleton = HasSkeleton();
      isBiped = IsBiped();
      skeleton = GetSkeleton(appSettings);
      importSkeleton = (appSettings != NULL) ? appSettings->useSkeleton : false;
      importSkeleton &= hasSkeleton;
	  importSkeleton &= !isBiped;

      // Guess that the skeleton is the same one in the current directory
      if (importSkeleton && !defaultSkeletonName.empty()) {
         TCHAR buffer[MAX_PATH];
         GetFullPathName(name.c_str(), _countof(buffer), buffer, NULL);
         PathRemoveFileSpec(buffer);
         PathAddBackslash(buffer);
         PathAppend(buffer, defaultSkeletonName.c_str());
         if (-1 != _taccess(buffer, 0))
            skeleton = buffer;
      }
   }
}

string NifImporter::GetSkeleton(AppSettings *appSettings)
{
   string skeleton = (appSettings != NULL) ? appSettings->Skeleton : "";
   // Guess that the skeleton is the same one in the current directory
   if (importSkeleton && !defaultSkeletonName.empty()) {
      TCHAR buffer[MAX_PATH];
      GetFullPathName(name.c_str(), _countof(buffer), buffer, NULL);
      PathRemoveFileSpec(buffer);
      PathAddBackslash(buffer);
      PathAppend(buffer, defaultSkeletonName.c_str());
      if (-1 != _taccess(buffer, 0))
         skeleton = buffer;
   }
   return skeleton;
}

void NifImporter::LoadIniSettings()
{
   TCHAR iniName[MAX_PATH];
   GetIniFileName(iniName);
   this->iniFileName = iniName;
   iniFileValid = (-1 != _access(iniName, 0));

   // Locate which application to use. If Auto, find first app where this file appears in the root path list
   appSettings = NULL;
   string curapp = GetIniValue<string>(NifImportSection, "CurrentApp", "AUTO");
   if (0 == _tcsicmp(curapp.c_str(), "AUTO")) {
      autoDetect = true;
      // Scan Root paths
      bool versionmatch = false;
      int version = GetNifVersion(this->name);
      for (AppSettingsMap::iterator itr = TheAppSettings.begin(), end = TheAppSettings.end(); itr != end; ++itr){
         if ((*itr).IsFileInRootPaths(this->name)) {
            appSettings = &(*itr);
            break;
         } else if ( !versionmatch && ParseVersionString((*itr).NiVersion) == version ) {
            // Version matching is an ok fit but we want the other if possible. And we want the first match if possible.
            appSettings = &(*itr);
            versionmatch = true;
         }
      }
   } else {
      autoDetect = false;
      appSettings = FindAppSetting(curapp);
   }
   if (appSettings == NULL && !TheAppSettings.empty()){
      appSettings = &TheAppSettings.front();
   }

   // General System level
   useBiped = GetIniValue(NifImportSection, "UseBiped", false);
   skeletonCheck = GetIniValue<string>(NifImportSection, "SkeletonCheck", "Bip*");
   showTextures = GetIniValue(NifImportSection, "ShowTextures", true);
   removeIllegalFaces = GetIniValue(NifImportSection, "RemoveIllegalFaces", true);
   removeDegenerateFaces = GetIniValue(NifImportSection, "RemoveDegenerateFaces", true);
   enableAutoSmooth = GetIniValue(NifImportSection, "EnableAutoSmooth", true);
   autoSmoothAngle = GetIniValue(NifImportSection, "AutoSmoothAngle", 30.0f);
   flipUVTextures = GetIniValue(NifImportSection, "FlipUVTextures", true);
   enableSkinSupport = GetIniValue(NifImportSection, "EnableSkinSupport", true);
   enableCollision = GetIniValue(NifImportSection, "EnableCollision", true);
   enableLights = GetIniValue(NifImportSection, "Lights", false);
   enableCameras = GetIniValue(NifImportSection, "Cameras", false);
   vertexColorMode = GetIniValue<int>(NifImportSection, "VertexColorMode", 1);
   useNiftoolsShader = GetIniValue<int>(NifImportSection, "UseNiftoolsShader", 1);
   mergeNonAccum = GetIniValue(NifImportSection, "MergeNonAccum", true);
   importUPB = GetIniValue(NifImportSection, "ImportUPB", true);
   ignoreRootNode = GetIniValue(NifImportSection, "IgnoreRootNode", true);
   weldVertices = GetIniValue(NifImportSection, "WeldVertices", false);
   weldVertexThresh = GetIniValue(NifImportSection, "WeldVertexThresh", 0.01f);
   dummyBonesAsLines = GetIniValue(NifImportSection, "DummyBonesAsLines", false);
   

   // Biped
   importBones = GetIniValue(BipedImportSection, "ImportBones", true);
   bipedHeight = GetIniValue(BipedImportSection, "BipedHeight", 131.90f);
   bipedAngle = GetIniValue(BipedImportSection, "BipedAngle", 90.0f);
   bipedAnkleAttach = GetIniValue(BipedImportSection, "BipedAnkleAttach", 0.2f);
   bipedTrianglePelvis = GetIniValue(BipedImportSection, "BipedTrianglePelvis", false);
   removeUnusedImportedBones = GetIniValue(BipedImportSection, "RemoveUnusedImportedBones", false);
   forceRotation = GetIniValue(BipedImportSection, "ForceRotation", true);
   browseForSkeleton = GetIniValue(BipedImportSection, "BrowseForSkeleton", true);
   defaultSkeletonName = GetIniValue<string>(BipedImportSection, "DefaultSkeletonName", "Skeleton.Nif");
   minBoneWidth = GetIniValue(BipedImportSection, "MinBoneWidth", 0.5f);
   maxBoneWidth = GetIniValue(BipedImportSection, "MaxBoneWidth", 3.0f);
   boneWidthToLengthRatio = GetIniValue(BipedImportSection, "BoneWidthToLengthRatio", 0.25f);
   createNubsForBones = GetIniValue(BipedImportSection, "CreateNubsForBones", true);
   dummyNodeMatches = TokenizeString(GetIniValue<string>(BipedImportSection, "DummyNodeMatches", "").c_str(), ";");
   convertBillboardsToDummyNodes = GetIniValue(BipedImportSection, "ConvertBillboardsToDummyNodes", true);
   uncontrolledDummies = GetIniValue(BipedImportSection, "UncontrolledDummies", true);

   // Animation
   replaceTCBRotationWithBezier = GetIniValue(AnimImportSection, "ReplaceTCBRotationWithBezier", true);
   enableAnimations = GetIniValue(AnimImportSection, "EnableAnimations", true);
   requireMultipleKeys = GetIniValue(AnimImportSection, "RequireMultipleKeys", true);
   applyOverallTransformToSkinAndBones = GetIniValue(AnimImportSection, "ApplyOverallTransformToSkinAndBones", true);
   clearAnimation = GetIniValue(AnimImportSection, "ClearAnimation", true);
   addNoteTracks = GetIniValue(AnimImportSection, "AddNoteTracks", true);
   addTimeTags = GetIniValue(AnimImportSection, "AddTimeTags", true);

   rotate90Degrees = TokenizeString(GetIniValue<string>(NifImportSection, "Rotate90Degrees", "").c_str(), ";");

   // Collision
   bhkScaleFactor = GetIniValue<float>(CollisionSection, "bhkScaleFactor", 7.0f);
   ApplyAppSettings();
}
void NifImporter::ApplyAppSettings()
{
   goToSkeletonBindPosition = false;
   // Override specific settings
   if (appSettings) {
      if (appSettings->disableCreateNubsForBones)
         createNubsForBones = false;
      goToSkeletonBindPosition = appSettings->goToSkeletonBindPosition;
      if (!appSettings->dummyNodeMatches.empty())
         dummyNodeMatches = appSettings->dummyNodeMatches;
      if (appSettings->applyOverallTransformToSkinAndBones != -1)
         applyOverallTransformToSkinAndBones = appSettings->applyOverallTransformToSkinAndBones ? true : false;
      if (!appSettings->rotate90Degrees.empty())
         rotate90Degrees = appSettings->rotate90Degrees;
      supportPrnStrings = appSettings->supportPrnStrings;

	  doNotReuseExistingBones = appSettings->doNotReuseExistingBones;

	  if (!appSettings->skeletonCheck.empty())
		skeletonCheck = appSettings->skeletonCheck;
   }
}

void NifImporter::SaveIniSettings()
{
   SetIniValue(NifImportSection, "UseBiped", useBiped);
   SetIniValue(NifImportSection, "EnableSkinSupport", enableSkinSupport);
   SetIniValue(NifImportSection, "VertexColorMode", vertexColorMode);
   SetIniValue(NifImportSection, "EnableCollision", enableCollision);
   SetIniValue(NifImportSection, "Lights", enableLights);
   SetIniValue(NifImportSection, "Cameras", enableCameras);
   
   //SetIniValue(NifImportSection, "EnableFurniture", enableAnimations);

   SetIniValue(NifImportSection, "FlipUVTextures", flipUVTextures);
   SetIniValue(NifImportSection, "ShowTextures", showTextures);
   SetIniValue(NifImportSection, "EnableAutoSmooth", enableAutoSmooth);
   SetIniValue(NifImportSection, "RemoveIllegalFaces", removeIllegalFaces);
   SetIniValue(NifImportSection, "RemoveDegenerateFaces", removeDegenerateFaces);
   SetIniValue(NifImportSection, "ImportUPB", importUPB);
   SetIniValue(NifImportSection, "IgnoreRootNode", ignoreRootNode);

   SetIniValue(BipedImportSection, "ImportBones", importBones);
   SetIniValue(BipedImportSection, "RemoveUnusedImportedBones", removeUnusedImportedBones);  

   SetIniValue(AnimImportSection, "EnableAnimations", enableAnimations);
   SetIniValue(AnimImportSection, "ClearAnimation", clearAnimation);
   SetIniValue(AnimImportSection, "AddNoteTracks", addNoteTracks);
   SetIniValue(AnimImportSection, "AddTimeTags", addTimeTags);
   SetIniValue(NifImportSection, "WeldVertices", weldVertices);
   SetIniValue(NifImportSection, "WeldVertexThresh", weldVertexThresh);
   SetIniValue(NifImportSection, "DummyBonesAsLines", dummyBonesAsLines);

   SetIniValue<string>(NifImportSection, "CurrentApp", autoDetect ? "AUTO" : appSettings->Name );
}

void NifImporter::RegisterNode(Niflib::NiObjectNETRef node, INode* inode)
{
	nodeMap[node] = inode;
}

INode* NifImporter::FindNode(Niflib::NiObjectNETRef node)
{
	// may want to make this a map if its hit a lot
	if (NULL == node) return NULL;

	NodeToNodeMap::iterator itr = nodeMap.find(node);
	if (itr != nodeMap.end())
		return (*itr).second;

	//return gi->GetINodeByName(node->GetName().c_str());
	return GetNode(node->GetName());
}

INode *NifImporter::GetNode(Niflib::NiNodeRef node)
{
	return FindNode(node);
}

INode *NifImporter::GetNode(Niflib::NiObjectNETRef obj)
{
	if (obj->IsDerivedType(NiNode::TYPE)) {
		NiNodeRef node = StaticCast<NiNode>(obj);
		if (INode *n = GetNode(node)) {
			return n;
		}
	}
	//return gi->GetINodeByName(obj->GetName().c_str());
	return GetNode(obj->GetName());
}

void NifImporter::RegisterNode(const string& name, INode* inode){
	nodeNameMap[name] = inode;
}
INode *NifImporter::GetNode(const string& name){
	
	NameToNodeMap::iterator itr = nodeNameMap.find(name);
	if (itr != nodeNameMap.end())
		return (*itr).second;

	INode *node  = gi->GetINodeByName(name.c_str());
	if (node != NULL) {
		nodeNameMap[name] = node;
	}
	return node;
}

INode *NifImporter::GetNode(const TSTR& name){
	return GetNode( string(name.data()) );
}

bool NifImporter::DoImport()
{
   bool ok = true;
   if (!suppressPrompts)
   {
      if (!ShowDialog())
         return true;

      ApplyAppSettings();
      SaveIniSettings();
   }

   vector<string> importedBones;
   if (!isBiped && importSkeleton && importBones)
   {
      if (importSkeleton && !skeleton.empty()) {
         try
         {
            NifImporter skelImport(skeleton.c_str(), i, gi, suppressPrompts);
            if (skelImport.isValid())
            {
               // Enable Skeleton specific items
               skelImport.isBiped = true;
               skelImport.importBones = true;
               // Disable undesirable skeleton items
               skelImport.enableCollision = false;
               skelImport.enableAnimations = false;
               skelImport.suppressPrompts = true;
               skelImport.DoImport();
               if (!skelImport.useBiped && removeUnusedImportedBones)
                  importedBones = GetNamesOfNodes(skelImport.nodes);
            }
         }
         catch (RuntimeError &error)
         {
            // ignore import errors and continue
         }
      }
   } else if (hasSkeleton && useBiped && importBones) {
      ImportBipeds(nodes);
   }

   if (isValid()) {

      if (root->IsDerivedType(NiNode::TYPE))
      {
         NiNodeRef rootNode = root;

		 if (importBones) {
			 if (ignoreRootNode || strmatch(rootNode->GetName(), "Scene Root")) {
				 RegisterNode(root, gi->GetRootNode());
				 ImportBones(DynamicCast<NiNode>(rootNode->GetChildren()));
			 } else {
				 ImportBones(rootNode);
			 }
		 }


         if (enableLights){
            ok = ImportLights(rootNode);
         }

         ok = ImportMeshes(rootNode);

		 // Import Havok Collision Data surrounding node
		 if (enableCollision) {
			 ImportCollision(rootNode);
		 }

         if (importSkeleton && removeUnusedImportedBones){
            vector<string> importedNodes = GetNamesOfNodes(nodes);
            sort(importedBones.begin(), importedBones.end());
            sort(importedNodes.begin(), importedNodes.end());
            vector<string> results;
            results.resize(importedBones.size());
            vector<string>::iterator end = set_difference ( 
               importedBones.begin(), importedBones.end(),
               importedNodes.begin(), importedNodes.end(), results.begin());
            for (vector<string>::iterator itr = results.begin(); itr != end; ++itr){
               if (INode *node = gi->GetINodeByName((*itr).c_str())){
				   gi->DeleteNode(node, FALSE);
               }
            }
         }
      }
      else if (root->IsDerivedType(NiTriShape::TYPE))
      {
         ok |= ImportMesh(NiTriShapeRef(root));
      }
      else if (root->IsDerivedType(NiTriStrips::TYPE))
      {
         ok |= ImportMesh(NiTriStripsRef(root));
      }
   }

   ClearAnimation();
   ImportAnimation();
   return true;
}
bool NifImporter::IsSkyrim() const {
	return (nifVersion == 0x14020007 && userVersion == 12);
}
bool NifImporter::IsFallout3() const {
	return (nifVersion == 0x14020007 && userVersion == 11);
}
bool NifImporter::IsOblivion() const {
	return ((nifVersion == 0x14000004 || nifVersion == 0x14000005) && (userVersion == 11 || userVersion == 10));
}
bool NifImporter::IsMorrowind() const {
	return ((nifVersion == 0x04000002) && (userVersion == 11 || userVersion == 10) );
}
