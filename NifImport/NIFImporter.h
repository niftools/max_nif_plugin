/**********************************************************************
*<
FILE: NifImporter.h

DESCRIPTION:	NIF Importer 

CREATED BY: tazpn (Theo)

HISTORY:

*>	Copyright (c) 2006, All Rights Reserved.
**********************************************************************/

#ifndef __NIFIMPORTER_H__
#define __NIFIMPORTER_H__

#include "BaseImporter.h"
// NIF Importer
class NifImporter : public BaseImporter
{
public:
   // Ini settings
   bool showTextures; // show textures in viewport
   bool removeIllegalFaces;
   bool removeDegenerateFaces;
   bool enableAutoSmooth;
   float autoSmoothAngle;
   bool flipUVTextures;
   bool enableSkinSupport;
   bool goToSkeletonBindPosition;
   bool enableCollision;

   // Biped/Bones related settings
   bool importBones;
   string skeleton;
   float bipedHeight;
   string skeletonCheck;
   float bipedAngle;
   float bipedAnkleAttach;
   bool bipedTrianglePelvis;
   bool importSkeleton;
   bool useBiped;
   bool hasSkeleton;
   bool isBiped;
   bool removeUnusedImportedBones;
   bool forceRotation;
   bool browseForSkeleton;
   string defaultSkeletonName;
   float minBoneWidth;
   float maxBoneWidth;
   float boneWidthToLengthRatio;
   bool createNubsForBones;
   stringlist dummyNodeMatches;
   bool convertBillboardsToDummyNodes;
   bool uncontrolledDummies;

   // Animation related Settings
   bool replaceTCBRotationWithBezier;
   bool enableAnimations;
   bool requireMultipleKeys;
   bool applyOverallTransformToSkinAndBones;

   // Collision settings
   float bhkScaleFactor;

   vector<Niflib::NiObjectRef> blocks;
   vector<Niflib::NiNodeRef> nodes;
   map<string,int> ctrlCount; // counter for number of controllers referencing a node

   NifImporter(const TCHAR *Name,ImpInterface *I,Interface *GI, BOOL SuppressPrompts);
   virtual void Initialize();
   virtual void ReadBlocks();
   void BuildNodes();

   // Ini File related routines
   void LoadIniSettings();
   void SaveIniSettings();

   bool HasSkeleton();
   bool IsBiped();
   void ImportBones(vector<Niflib::NiNodeRef>& bones);
   void ImportBones(Niflib::NiNodeRef blocks, bool recurse = true);
   void ImportBipeds(vector<Niflib::NiNodeRef>& blocks);
   void AlignBiped(IBipMaster* master, Niflib::NiNodeRef block);
   bool ImportMeshes(Niflib::NiNodeRef block);
   string FindImage(const string& name);

   bool ImportUPB(INode *node, Niflib::NiNodeRef block);

   void SetTrangles(Mesh& mesh, vector<Niflib::Triangle>& v, bool hasTexture);
   bool ImportMesh(Niflib::NiTriShapeRef triShape);
   bool ImportMesh(Niflib::NiTriStripsRef triStrips);
   bool ImportMaterialAndTextures(ImpNode *node, Niflib::NiAVObjectRef avObject);
   bool ImportTransform(ImpNode *node, Niflib::NiAVObjectRef avObject);
   bool ImportMesh(ImpNode *node, TriObject *o, Niflib::NiTriBasedGeomRef triGeom, Niflib::NiTriBasedGeomDataRef triGeomData, bool hasTexture);

   bool ImportSkin(ImpNode *node, Niflib::NiTriBasedGeomRef triGeom);
   Texmap* CreateTexture(Niflib::TexDesc& desc);
   INode *CreateBone(const string& name, Point3 startPos, Point3 endPos, Point3 zAxis);
   INode *CreateHelper(const string& name, Point3 startPos);

   // Primary Collision entry point.  Tests for bhk objects
   bool ImportCollision(Niflib::NiNodeRef node);

   INode *GetNode(Niflib::NiNodeRef node);

   virtual bool DoImport();

   // Animation Helpers
   bool ImportAnimation();

   protected: 
      NifImporter();
};

#endif
