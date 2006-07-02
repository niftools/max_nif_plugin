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

class IBipMaster;
IBipMaster * (_cdecl * Max8CreateNewBiped)(float,float,class Point3 const &,int,int,int,int,int,int,int,int,int,int,int,int,float,int,int,int,int,int,int,int,int) = 0;

NifImporter::NifImporter(const TCHAR *Name,ImpInterface *I,Interface *GI, BOOL SuppressPrompts)
   : BaseImporter()
{
   BaseInit(Name,I,GI,SuppressPrompts);
}

NifImporter::NifImporter()
{
}

void NifImporter::ReadBlocks()
{
   //blocks = ReadNifList( name );
   root = ReadNifTree(name);
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
      useBiped &= (Max8CreateNewBiped != NULL);

      hasSkeleton = HasSkeleton();
      isBiped = IsBiped();
      skeleton = (appSettings != NULL) ? appSettings->Skeleton : "";
      importSkeleton = (appSettings != NULL) ? appSettings->useSkeleton : false;
      importSkeleton &= hasSkeleton;

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

void NifImporter::LoadIniSettings()
{
   TCHAR iniName[MAX_PATH];
   LPCTSTR pluginDir = gi->GetDir(APP_PLUGCFG_DIR);
   PathCombine(iniName, pluginDir, "MaxNifTools.ini");
   this->iniFileName = iniName;
   iniFileValid = (-1 != _access(iniName, 0));

   // Locate which application to use. If Auto, find first app where this file appears in the root path list
   appSettings = NULL;
   string curapp = GetIniValue<string>(NifImportSection, "CurrentApp", "AUTO");
   if (0 == _tcsicmp(curapp.c_str(), "AUTO")) {
      // Scan Root paths
      for (AppSettingsMap::iterator itr = TheAppSettings.begin(), end = TheAppSettings.end(); itr != end; ++itr){
         if ((*itr).IsFileInRootPaths(this->name)) {
            appSettings = &(*itr);
            break;
         }
      }
   } else {
      appSettings = FindAppSetting(curapp);
   }
   if (appSettings == NULL && !TheAppSettings.empty()){
      appSettings = &TheAppSettings.front();
   }

   useBiped = GetIniValue<bool>(NifImportSection, "UseBiped", false);
   skeletonCheck = GetIniValue<string>(NifImportSection, "SkeletonCheck", "Bip*");
   showTextures = GetIniValue<bool>(NifImportSection, "ShowTextures", true);

   removeIllegalFaces = GetIniValue<bool>(NifImportSection, "RemoveIllegalFaces", true);
   removeDegenerateFaces = GetIniValue<bool>(NifImportSection, "RemoveDegenerateFaces", true);
   enableAutoSmooth = GetIniValue<bool>(NifImportSection, "EnableAutoSmooth", true);
   autoSmoothAngle = GetIniValue<float>(NifImportSection, "AutoSmoothAngle", 30.0f);
   flipUVTextures = GetIniValue<bool>(NifImportSection, "FlipUVTextures", true);
   enableSkinSupport = GetIniValue<bool>(NifImportSection, "EnableSkinSupport", true);

   importBones = GetIniValue<bool>(BipedImportSection, "ImportBones", true);
   bipedHeight = GetIniValue<float>(BipedImportSection, "BipedHeight", 131.90f);
   bipedAngle = GetIniValue<float>(BipedImportSection, "BipedAngle", 90.0f);
   bipedAnkleAttach = GetIniValue<float>(BipedImportSection, "BipedAnkleAttach", 0.2f);
   bipedTrianglePelvis = GetIniValue<bool>(BipedImportSection, "BipedTrianglePelvis", false);
   removeUnusedImportedBones = GetIniValue<bool>(BipedImportSection, "RemoveUnusedImportedBones", false);
   forceRotation = GetIniValue<bool>(BipedImportSection, "ForceRotation", true);
   browseForSkeleton = GetIniValue<bool>(BipedImportSection, "BrowseForSkeleton", true);
   defaultSkeletonName = GetIniValue<string>(BipedImportSection, "DefaultSkeletonName", "Skeleton.Nif");
   minBoneWidth = GetIniValue<float>(BipedImportSection, "MinBoneWidth", 0.5f);
   maxBoneWidth = GetIniValue<float>(BipedImportSection, "MaxBoneWidth", 3.0f);
   boneWidthToLengthRatio = GetIniValue<float>(BipedImportSection, "BoneWidthToLengthRatio", 0.25f);
   createNubsForBones = GetIniValue<bool>(BipedImportSection, "CreateNubsForBones", true);
   dummyNodeMatches = TokenizeString(GetIniValue<string>(BipedImportSection, "DummyNodeMatches", "").c_str(), ";");
   convertBillboardsToDummyNodes = GetIniValue<bool>(BipedImportSection, "ConvertBillboardsToDummyNodes", true);
   uncontrolledDummies = GetIniValue<bool>(BipedImportSection, "UncontrolledDummies", true);

   replaceTCBRotationWithBezier = GetIniValue<bool>(AnimImportSection, "ReplaceTCBRotationWithBezier", true);
   enableAnimations = GetIniValue<bool>(AnimImportSection, "EnableAnimations", true);
   requireMultipleKeys = GetIniValue<bool>(AnimImportSection, "RequireMultipleKeys", true);
   applyOverallTransformToSkinAndBones = GetIniValue<bool>(AnimImportSection, "ApplyOverallTransformToSkinAndBones", true);

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
   }
}

void NifImporter::SaveIniSettings()
{
   SetIniValue<bool>(NifImportSection, "UseBiped", useBiped);
   SetIniValue<string>(NifImportSection, "Skeleton", skeleton);
   SetIniValue<string>(NifImportSection, "SkeletonCheck", skeletonCheck);

   SetIniValue<float>(BipedImportSection, "BipedHeight", bipedHeight);
   SetIniValue<float>(BipedImportSection, "BipedAngle", bipedAngle);
   SetIniValue<float>(BipedImportSection, "BipedAnkleAttach", bipedAnkleAttach);
   SetIniValue<bool>(BipedImportSection, "BipedTrianglePelvis", bipedTrianglePelvis);
}

INode *NifImporter::GetNode(Niflib::NiNodeRef node)
{
   // may want to make this a map if its hit a lot
   if (NULL == node) return NULL;
   return gi->GetINodeByName(node->GetName().c_str());
}

bool NifImporter::DoImport()
{
   bool ok = true;
   NiNodeRef rootNode = root;

   vector<string> importedBones;
   if (!isBiped && importSkeleton && importBones)
   {
      if (browseForSkeleton)
      {
         TCHAR filter[64], *pfilter=filter;
         pfilter = _tcscpy(filter, shortDescription.c_str());
         pfilter = _tcscat(pfilter, " (*.NIF)");
         pfilter += strlen(pfilter);
         *pfilter++ = '\0';
         _tcscpy(pfilter, "*.NIF");
         pfilter += strlen(pfilter);
         *pfilter++ = '\0';
         *pfilter++ = '\0';

         TCHAR filename[MAX_PATH];
         GetFullPathName(skeleton.c_str(), _countof(filename), filename, NULL);

         OPENFILENAME ofn;
         memset(&ofn, 0, sizeof(ofn));
         ofn.lStructSize = sizeof(ofn);
         ofn.hwndOwner = gi->GetMAXHWnd();
         ofn.lpstrFilter = filter;
         ofn.lpstrFile = filename;
         ofn.nMaxFile = _countof(filename);
         ofn.lpstrTitle = TEXT("Browse for Skeleton NIF...");
         ofn.lpstrDefExt = TEXT("NIF");
         ofn.Flags = OFN_HIDEREADONLY|OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_NOCHANGEDIR|OFN_PATHMUSTEXIST;
         importSkeleton = GetOpenFileName(&ofn) ? true : false;
         if (importSkeleton) {
            skeleton = filename;
         }
      }
      if (importSkeleton && !skeleton.empty()) {
         NifImporter skelImport(skeleton.c_str(), i, gi, suppressPrompts);
         if (skelImport.isValid())
         {
            skelImport.isBiped = true;
            skelImport.importBones = true;
            skelImport.DoImport();
            if (!skelImport.useBiped && removeUnusedImportedBones)
               importedBones = GetNamesOfNodes(skelImport.nodes);
         }
      }
   } else if (hasSkeleton && useBiped && importBones) {
      ImportBipeds(nodes);
   }

   if (isValid()) {
      if (importBones) {
         if (strmatch(rootNode->GetName(), "Scene Root"))
            ImportBones(DynamicCast<NiNode>(rootNode->GetChildren()));
         else
            ImportBones(rootNode);
      }

      ok = ImportMeshes(rootNode);

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
               node->Delete(0, TRUE);
            }
         }
      }
   }


   // Kick of animation import
   ImportAnimation();
   return true;
}