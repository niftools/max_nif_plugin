/**********************************************************************
*<
FILE: ImportMeshAndSkin.cpp

DESCRIPTION: Mesh and Skin Import routines

CREATED BY: tazpn (Theo)

HISTORY: 

*>	Copyright (c) 2006, All Rights Reserved.
**********************************************************************/
#include "stdafx.h"
#include "MaxNifImport.h"

using namespace Niflib;

// Locate a TriObject in an Object if it exists
TriObject* GetTriObject(Object *o)
{
   if (o && o->CanConvertToType(triObjectClassID))
      return (TriObject *)o->ConvertToType(0, triObjectClassID);
   while (o->SuperClassID() == GEN_DERIVOB_CLASS_ID && o)
   {
      IDerivedObject* dobj = (IDerivedObject *)(o);
      o = dobj->GetObjRef();
      if (o && o->CanConvertToType(triObjectClassID))
         return (TriObject *)o->ConvertToType(0, triObjectClassID);
   }
   return NULL;
}

// Get or Create the Skin Modifier
Modifier *GetSkin(INode *node)
{
   Object* pObj = node->GetObjectRef();
   if (!pObj) return NULL;
   while (pObj->SuperClassID() == GEN_DERIVOB_CLASS_ID)
   {
      IDerivedObject* pDerObj = (IDerivedObject *)(pObj);
      int Idx = 0;
      while (Idx < pDerObj->NumModifiers())
      {
         // Get the modifier. 
         Modifier* mod = pDerObj->GetModifier(Idx);
         if (mod->ClassID() == SKIN_CLASSID)
         {
            // is this the correct Physique Modifier based on index?
            return mod;
         }
         Idx++;
      }
      pObj = pDerObj->GetObjRef();
   }

   IDerivedObject *dobj = CreateDerivedObject(node->GetObjectRef());

   //create a skin modifier and add it
   Modifier *skinMod = (Modifier*) CreateInstance(OSM_CLASS_ID, SKIN_CLASSID);
   dobj->SetAFlag(A_LOCK_TARGET);
   dobj->AddModifier(skinMod);
   dobj->ClearAFlag(A_LOCK_TARGET);
   node->SetObjectRef(dobj);
   return skinMod;
}

bool NifImporter::ImportTransform(ImpNode *node, NiAVObjectRef avObject)
{
   node->SetTransform(0,TOMATRIX3(avObject->GetWorldTransform()));   
   return true;
}


bool NifImporter::ImportMeshes(NiNodeRef node)
{
   bool ok = true;
   try 
   {
      vector<NiTriShapeRef> trinodes = DynamicCast<NiTriShape>(node->GetChildren());
      for (vector<NiTriShapeRef>::iterator itr = trinodes.begin(), end = trinodes.end(); itr != end; ++itr){
         ok |= ImportMesh(*itr);
      }
      vector<NiTriStripsRef> tristrips = DynamicCast<NiTriStrips>(node->GetChildren());
      for (vector<NiTriStripsRef>::iterator itr = tristrips.begin(), end = tristrips.end(); itr != end; ++itr){
         ok |= ImportMesh(*itr);
      }
      vector<NiNodeRef> nodes = DynamicCast<NiNode>(node->GetChildren());
      for (vector<NiNodeRef>::iterator itr = nodes.begin(), end = nodes.end(); itr != end; ++itr){
         ok |= ImportMeshes(*itr);
      }
   }
   catch( exception & e ) 
   {
      e=e;
      ok = false;
   }
   catch( ... ) 
   {
      ok = false;
   }
   return ok;
}

bool NifImporter::ImportMesh(ImpNode *node, TriObject *o, NiTriBasedGeomRef triGeom, NiTriBasedGeomDataRef triGeomData, bool hasTexture)
{
   Mesh& mesh = o->GetMesh();

   Matrix44 baseTM = (importBones) ? triGeom->GetLocalTransform() : triGeom->GetWorldTransform();
   node->SetTransform(0,TOMATRIX3(baseTM));  

   // Vertex info
   {
      int nVertices = triGeomData->GetVertexCount();
      vector<Vector3> vertices = triGeomData->GetVertices();
      mesh.setNumVerts(nVertices);
      for (int i=0; i < nVertices; ++i){
         Vector3 &v = vertices[i];
         mesh.verts[i].Set(v.x, v.y, v.z);
      }
   }
   // uv texture info
   {
      int nUVSet = triGeomData->GetUVSetCount();
      int n = 0;
      for (int j=0; j<nUVSet; j++){
         vector<TexCoord> texCoords = triGeomData->GetUVSet(j);
         n = texCoords.size();
         mesh.setNumTVerts(n, FALSE);
         for (int i=0; i<n; ++i) {
            TexCoord& texCoord = texCoords[i];
            mesh.tVerts[i].Set(texCoord.u, (flipUVTextures) ? -texCoord.v : texCoord.v, 0);
         }
      }
   }
   if (removeDegenerateFaces)
      mesh.RemoveDegenerateFaces();
   if (removeIllegalFaces)
      mesh.RemoveIllegalFaces();
   if (enableAutoSmooth)
      mesh.AutoSmooth(TORAD(autoSmoothAngle), FALSE, FALSE);

   // Normals
   {
      mesh.checkNormals(TRUE);
      vector<Vector3> n = triGeomData->GetNormals();
      for (int i=0; i<n.size(); i++){
         Vector3 v = n[i];
         mesh.setNormal(i, Point3(v.x, v.y, v.z));
      }
   }
   // vertex coloring
   {
      bool hasAlpha = false;
      vector<Color4> cv = triGeomData->GetColors();
      mesh.setNumVertCol(cv.size());
      for (int i=0; i<cv.size(); i++){
         Color4& c = cv[i];
         mesh.vertCol[i].Set(c.r, c.g, c.b);
         hasAlpha |= (c.a != 0.0);
      }
   }
   return true;
}

void NifImporter::SetTrangles(Mesh& mesh, vector<Triangle>& v, bool hasTexture)
{
   int n = v.size();
   mesh.setNumFaces(n);
   //if (hasTexture)
      mesh.setNumTVFaces(n);
   for (int i=0; i<n; ++i) {
      Triangle& t = v[i];
      Face& f = mesh.faces[i];
      f.setVerts(t.v1, t.v2, t.v3);
      f.Show();
      f.setEdgeVisFlags(EDGE_VIS, EDGE_VIS, EDGE_VIS);
      //if (hasTexture) {
         TVFace& tf = mesh.tvFace[i];
         tf.setTVerts(t.v1, t.v2, t.v3);
      //}
   }
}

bool NifImporter::ImportMesh(NiTriShapeRef triShape)
{
   bool ok = true;

   ImpNode *node = i->CreateNode();
   if(!node) return false;
   TriObject *triObject = CreateNewTriObject();
   node->Reference(triObject);
   string name = triShape->GetName();
   node->SetName(name.c_str());

   INode *inode = node->GetINode();

   // Texture
   bool hasTexture = ImportMaterialAndTextures(node, triShape);
   Mesh& mesh = triObject->GetMesh();
   NiTriShapeDataRef triShapeData = DynamicCast<NiTriShapeData>(triShape->GetData());
   if (triShapeData == NULL)
      return false;

   ok |= ImportMesh(node, triObject, triShape, triShapeData, hasTexture);

   // Triangles and texture vertices
   if (ok)
   {
      vector<Triangle> v = triShapeData->GetTriangles();
      SetTrangles(mesh, v, hasTexture);
   }
   if (enableSkinSupport)
      ImportSkin(node, triShape);

   i->AddNodeToScene(node);   

   // attach child
   if (INode *parent = GetNode(triShape->GetParent()))
      parent->AttachChild(inode, 1);

   inode->Hide(triShape->GetHidden() ? TRUE : FALSE);
   
   if (enableAutoSmooth){
      if (TriObject *tri = GetTriObject(inode->GetObjectRef())){
         tri->GetMesh().AutoSmooth(TORAD(autoSmoothAngle), FALSE, FALSE);
      }
   }

   return ok;
}

bool NifImporter::ImportMesh(NiTriStripsRef triStrips)
{
   bool ok = true;

   ImpNode *node = i->CreateNode();
   if(!node) return false;
   INode *inode = node->GetINode();
   TriObject *triObject = CreateNewTriObject();
   node->Reference(triObject);
   string name = triStrips->GetName();
   node->SetName(name.c_str());

   // Texture
   bool hasTexture = ImportMaterialAndTextures(node, triStrips);

   Mesh& mesh = triObject->GetMesh();
   NiTriStripsDataRef triStripsData = DynamicCast<NiTriStripsData>(triStrips->GetData());
   if (triStripsData == NULL)
      return false;

   ok |= ImportMesh(node, triObject, triStrips, triStripsData, hasTexture);

   // Triangles and texture vertices
   if (ok)
   {
      vector<Triangle> v = triStripsData->GetTriangles();
      SetTrangles(mesh, v, hasTexture);
   }
   if (enableSkinSupport)
      ImportSkin(node, triStrips);

   i->AddNodeToScene(node);   

   // attach child
   if (INode *parent = GetNode(triStrips->GetParent())) {
      parent->AttachChild(inode, 0);
   }

   inode->Hide(triStrips->GetHidden() ? TRUE : FALSE);

   // apply autosmooth after object creation for it to take hold
   if (enableAutoSmooth){
      if (TriObject *tri = GetTriObject(inode->GetObjectRef())){
         tri->GetMesh().AutoSmooth(TORAD(autoSmoothAngle), FALSE, FALSE);
      }
   }

   return ok;
}

struct VertexHolder
{
   VertexHolder() : vertIndex(0), count(0) {}

   int vertIndex;
   int count;
   Tab<INode*> boneNodeList;
   Tab<float> weights;
};

bool NifImporter::ImportSkin(ImpNode *node, NiTriBasedGeomRef triGeom)
{
   bool ok = true;
   NiSkinInstanceRef nifSkin = triGeom->GetSkinInstance();
   if (!nifSkin) 
      return false;

   INode *tnode = node->GetINode();

   NiSkinDataRef data = nifSkin->GetSkinData();
   NiSkinPartitionRef part = nifSkin->GetSkinPartition();

   vector<NiNodeRef> nifBones = nifSkin->GetBones();

   //create a skin modifier and add it
   Modifier *skinMod = GetSkin(tnode);
   TriObject *triObject = GetTriObject(tnode->GetObjectRef());
   Mesh& m = triObject->GetMesh();

   //get the skin interface
   if (ISkin *skin = (ISkin *) skinMod->GetInterface(I_SKIN)){
      ISkinImportData* iskinImport = (ISkinImportData*) skinMod->GetInterface(I_SKINIMPORTDATA);

      Matrix3 m3 = TOMATRIX3(data->GetOverallTransform());
      Matrix3 im3 = Inverse(m3);
      iskinImport->SetSkinTm(tnode, m3, m3); // ???
      // Create Bone List
      Tab<INode*> bones;
      for (size_t i=0; i<nifBones.size(); ++i){
         NiNodeRef bone = nifBones[i];
         Matrix3 b3 = TOMATRIX3(data->GetBoneTransform(i));
         Matrix3 ib3 = Inverse(b3);

         string name = bone->GetName();
         if (INode *boneRef = gi->GetINodeByName(name.c_str())) {
            bones.Append(1, &boneRef);
            iskinImport->AddBoneEx(boneRef, TRUE);

            // Set Bone Transform
            Matrix3 tm = ib3;
            if (applyOverallTransformToSkinAndBones)
               ib3 *= im3;
            iskinImport->SetBoneTm(boneRef, ib3, ib3);
         }
      }
      if (bones.Count() != data->GetBoneCount())
         return false;

      ObjectState os = tnode->EvalWorldState(0);

      // Need to get a list of bones and weights for each vertex.
      vector<VertexHolder> vertexHolders;
      vertexHolders.resize(m.numVerts);
      for (int i=0, n=data->GetBoneCount();i<n; ++i){
         if (INode *boneRef = bones[i]){
            vector<SkinWeight> weights = data->GetBoneWeights(i);
            for (vector<SkinWeight>::iterator itr=weights.begin(), end=weights.end(); itr != end; ++itr){
               VertexHolder& h = vertexHolders[itr->index];
               h.vertIndex = itr->index;
               ++h.count;
               h.weights.Append(1, &itr->weight);
               h.boneNodeList.Append(1, &boneRef);
            }
         }
      }

      // Assign the weights 
      for (vector<VertexHolder>::iterator itr=vertexHolders.begin(), end=vertexHolders.end(); itr != end; ++itr){
         VertexHolder& h = (*itr);
         if (h.count){
            float sum = 0.0f;
            for (int i=0; i<h.count; ++i)
               sum += h.weights[i];
            ASSERT(fabs(sum-1.0) < 0.001);
            BOOL add = iskinImport->AddWeights(tnode, h.vertIndex, h.boneNodeList, h.weights);
            add = add;
         }
      }
      // This is a kludge to get skin transforms to update and avoid jumping around after modifying the transforms.
      //   Initially they show up incorrectly but magically fix up if you go to the modifier roll up.
      //   There is still an outstanding issue with skeleton and GetObjectTMBeforeWSM.
      skinMod->DisableModInViews();
      skinMod->EnableModInViews();
   }
   return ok;
}
