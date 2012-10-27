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
#include "IniSection.h"

namespace Niflib
{
   class NiTextKeyExtraData;
}

// NIF Importer
class NifImporter : public BaseImporter//, public IniFileSection
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
   int vertexColorMode;
   int useNiftoolsShader;
   bool mergeNonAccum;
   bool enableLights;
   bool enableCameras;
   bool importUPB;
   bool doNotReuseExistingBones;

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
   bool ignoreRootNode;
   bool autoDetect;
   stringlist rotate90Degrees;
   bool supportPrnStrings;

   // Animation related Settings
   bool replaceTCBRotationWithBezier;
   bool enableAnimations;
   bool requireMultipleKeys;
   bool applyOverallTransformToSkinAndBones;
   bool clearAnimation;
   bool addNoteTracks;
   bool addTimeTags;

   // Collision settings
   float bhkScaleFactor;

   bool weldVertices;
   float weldVertexThresh;

   bool dummyBonesAsLines;

   vector<Niflib::NiObjectRef> blocks;
   vector<Niflib::NiNodeRef> nodes;
   map<string,int> ctrlCount; // counter for number of controllers referencing a node

   typedef map<Niflib::NiObjectNETRef, INode*> NodeToNodeMap;
   typedef map<string, INode*, ltstr> NameToNodeMap;
   NodeToNodeMap nodeMap;
   NameToNodeMap nodeNameMap;


   NifImporter(const TCHAR *Name,ImpInterface *I,Interface *GI, BOOL SuppressPrompts);
   virtual void Initialize();
   virtual void ReadBlocks();
   void BuildNodes();

   // Ini File related routines
   virtual void LoadIniSettings();
   virtual void SaveIniSettings();

   void ApplyAppSettings();

   bool HasSkeleton();
   bool IsBiped();
   void ImportBones(vector<Niflib::NiNodeRef>& bones);
   void ImportBones(Niflib::NiNodeRef blocks, bool recurse = true);
   void ImportBipeds(vector<Niflib::NiNodeRef>& blocks);
   void AlignBiped(IBipMaster* master, Niflib::NiNodeRef block);
   bool ImportMeshes(Niflib::NiNodeRef block);
   string FindImage(const string& name);

   bool ImportUPB(INode *node, Niflib::NiNodeRef block);

   void SetTriangles(Mesh& mesh, const vector<Niflib::Triangle>& v);
   void SetNormals(Mesh& mesh, const vector<Niflib::Triangle>& t, const vector<Niflib::Vector3>& v);

   bool ImportMesh(Niflib::NiTriShapeRef triShape);
   bool ImportMesh(Niflib::NiTriStripsRef triStrips);
   bool ImportMultipleGeometry(Niflib::NiNodeRef parent, vector<Niflib::NiTriBasedGeomRef>& glist);
   StdMat2 *ImportMaterialAndTextures(ImpNode *node, Niflib::NiAVObjectRef avObject);
   bool ImportMaterialAndTextures(ImpNode *node, vector<Niflib::NiTriBasedGeomRef>& glist);
   bool ImportNiftoolsShader(ImpNode *node, Niflib::NiAVObjectRef avObject, StdMat2 *m);
   bool ImportTransform(ImpNode *node, Niflib::NiAVObjectRef avObject);
   bool ImportMesh(ImpNode *node, TriObject *o, Niflib::NiTriBasedGeomRef triGeom, Niflib::NiTriBasedGeomDataRef triGeomData, vector<Niflib::Triangle>& tris);
   bool ImportVertexColor(INode *tnode, TriObject *o, vector<Niflib::Triangle>& tris, vector<Niflib::Color4> cv, int cv_offset=0);
   bool ImportSkin(ImpNode *node, Niflib::NiTriBasedGeomRef triGeom, int v_start=0);
   Texmap* CreateTexture(Niflib::TexDesc& desc);
   Texmap* CreateTexture(Niflib::NiTexturePropertyRef desc);
	Texmap* CreateTexture(const string& name);
   Texmap* CreateNormalBump(LPCTSTR name, Texmap* nmap);
   Texmap* CreateMask(LPCTSTR name, Texmap* nmap, Texmap* mask);
   
   INode *CreateBone(const string& name, Point3 startPos, Point3 endPos, Point3 zAxis);
   INode *CreateHelper(const string& name, Point3 startPos);
   INode *CreateCamera(const string& name);

   INode *CreateImportNode(const char *name, Object *obj, INode* parent);

   bool ImportLights(Niflib::NiNodeRef node);
   bool ImportLights(vector<Niflib::NiLightRef> lights);

   // Primary Collision entry point.  Tests for bhk objects
   bool ImportCollision(Niflib::NiNodeRef node);

   void RegisterNode(Niflib::NiObjectNETRef node, INode* inode);
   INode *FindNode(Niflib::NiObjectNETRef node);

   INode *GetNode(Niflib::NiNodeRef node);
   INode *GetNode(Niflib::NiObjectNETRef obj);

   void RegisterNode(const string& name, INode* inode);
   INode *GetNode(const string& name);
   INode *GetNode(const TSTR& name);

   string GetSkeleton(AppSettings *appSettings);

   bool ShowDialog();
   virtual bool DoImport();

   // Animation Helpers
   bool ImportAnimation();
   void ClearAnimation();
   void ClearAnimation(INode *node);
   bool AddNoteTracks(float time, string name, string target, Niflib::Ref<Niflib::NiTextKeyExtraData> textKeyData, bool loop);

   void WeldVertices(Mesh& mesh);

   bool IsSkyrim() const;
	bool IsFallout3() const;
	bool IsOblivion() const;
	bool IsMorrowind() const;

   protected: 
      NifImporter();
};

#endif
