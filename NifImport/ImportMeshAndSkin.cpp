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
#include "istdplug.h"
#if VERSION_3DSMAX > ((5000<<16)+(15<<8)+0) // Version 5
#  include "MeshNormalSpec.h"
#endif

using namespace Niflib;

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

bool NifImporter::ImportMesh(ImpNode *node, TriObject *o, NiTriBasedGeomRef triGeom, NiTriBasedGeomDataRef triGeomData, vector<Triangle>& tris)
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
      int n = 0, j = 0;
      //for (int j=0; j<nUVSet; j++){
      if (nUVSet > 0) {
         vector<TexCoord> texCoords = triGeomData->GetUVSet(j);
         n = texCoords.size();
         mesh.setNumTVerts(n, FALSE);
         for (int i=0; i<n; ++i) {
            TexCoord& texCoord = texCoords[i];
            mesh.tVerts[i].Set(texCoord.u, (flipUVTextures) ? 1.0f-texCoord.v : texCoord.v, 0);
         }
      }
   }
   // Triangles and texture vertices
   SetTrangles(mesh, tris);

   // Normals
   {
      mesh.checkNormals(TRUE);
      vector<Vector3> n = triGeomData->GetNormals();
      if (n.size() > 0)
      {
         bool needNormals = false;
         for (int i=0; i<n.size(); i++){
            Vector3 v = n[i];
            Point3 norm(v.x, v.y, v.z);
            if (norm != mesh.getNormal(i)) {
               needNormals = true;
               break;
            }
         }
         if (needNormals)
         {
#if VERSION_3DSMAX > ((5000<<16)+(15<<8)+0) // Version 5
            mesh.SpecifyNormals();
            MeshNormalSpec *specNorms = mesh.GetSpecifiedNormals ();
            if (NULL != specNorms)
            {
               specNorms->ClearAndFree();
               specNorms->SetNumFaces(tris.size());
               specNorms->SetNumNormals(n.size());

               Point3* norms = specNorms->GetNormalArray();
               for (int i=0; i<n.size(); i++){
                  Vector3 v = n[i];
                  norms[i] = Point3(v.x, v.y, v.z);
               }
               MeshNormalFace* pFaces = specNorms->GetFaceArray();
               for (int i=0; i<tris.size(); i++){
                  Triangle& tri = tris[i];
                  pFaces[i].SpecifyNormalID(0, tri.v1);
                  pFaces[i].SpecifyNormalID(1, tri.v2);
                  pFaces[i].SpecifyNormalID(2, tri.v3);
               }
               specNorms->SetAllExplicit(true);
               specNorms->CheckNormals();
            }
#endif
         }
      }
   }

   ImportVertexColor(node, o, triGeom, triGeomData, tris);

   ImportMaterialAndTextures(node, triGeom);

   if (removeDegenerateFaces)
      mesh.RemoveDegenerateFaces();
   if (removeIllegalFaces)
      mesh.RemoveIllegalFaces();
   if (enableAutoSmooth)
      mesh.AutoSmooth(TORAD(autoSmoothAngle), FALSE, FALSE);

   if (enableSkinSupport)
      ImportSkin(node, triGeom);

   i->AddNodeToScene(node);   

   INode *inode = node->GetINode();
   inode->EvalWorldState(0);

   // attach child
   if (INode *parent = GetNode(triGeom->GetParent()))
      parent->AttachChild(inode, 1);

   inode->Hide(triGeom->GetHidden() ? TRUE : FALSE);

   if (enableAutoSmooth){
      mesh.AutoSmooth(TORAD(autoSmoothAngle), FALSE, FALSE);
   }

   return true;
}

void NifImporter::SetTrangles(Mesh& mesh, vector<Triangle>& v)
{
   int n = v.size();
   mesh.setNumFaces(n);
   mesh.setNumTVFaces(n);
   for (int i=0; i<n; ++i) {
      Triangle& t = v[i];
      Face& f = mesh.faces[i];
      f.setVerts(t.v1, t.v2, t.v3);
      f.Show();
      f.setEdgeVisFlags(EDGE_VIS, EDGE_VIS, EDGE_VIS);
      TVFace& tf = mesh.tvFace[i];
      tf.setTVerts(t.v1, t.v2, t.v3);
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
   Mesh& mesh = triObject->GetMesh();
   NiTriShapeDataRef triShapeData = DynamicCast<NiTriShapeData>(triShape->GetData());
   if (triShapeData == NULL)
      return false;

   vector<Triangle> tris = triShapeData->GetTriangles();
   ok |= ImportMesh(node, triObject, triShape, triShapeData, tris);
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
   Mesh& mesh = triObject->GetMesh();
   NiTriStripsDataRef triStripsData = DynamicCast<NiTriStripsData>(triStrips->GetData());
   if (triStripsData == NULL)
      return false;

   vector<Triangle> tris = triStripsData->GetTriangles();
   ok |= ImportMesh(node, triObject, triStrips, triStripsData, tris);
   return ok;
}

// vertex coloring
bool NifImporter::ImportVertexColor(ImpNode *node, TriObject *o, Niflib::NiTriBasedGeomRef triGeom, Niflib::NiTriBasedGeomDataRef triGeomData, vector<Triangle>& tris)
{
   bool hasAlpha = false;
   bool hasColor = false;
   Mesh& mesh = o->GetMesh();

   if (vertexColorMode == 1) // Bake into mesh (no Modifier)
   {
      vector<Color4> cv = triGeomData->GetColors();
      int n = cv.size();
      if (n > 0)
      {
         INode *tnode = node->GetINode();

         vector<TVFace> vcFace;
         int nt = tris.size();
         vcFace.resize(nt);
         mesh.setNumVCFaces(nt);
         for (int i=0; i<nt; ++i) {
            Triangle& t = tris[i];
            TVFace& vcf = vcFace[i];
            vcf.setTVerts(t.v1, t.v2, t.v3);
            mesh.vcFace[i].setTVerts(t.v1, t.v2, t.v3);
         }

         vector<VertColor> vertColors, vertAlpha;
         vertColors.resize(n);
         vertAlpha.resize(n);
         mesh.setNumVertCol(cv.size());
         
         for (int i=0; i<n; i++){
            Color4& c = cv[i];
            hasColor |= (c.r != 1.0f && c.g != 1.0f && c.b != 1.0f);
            vertColors[i] = Color(c.r,c.g,c.b);

            hasAlpha |= (c.a != 1.0f);
            vertAlpha[i] = Color(c.a,c.a,c.a);
         }

         // Add the Vertex Paint Alpha modifier now
         if (hasAlpha)
         {
            mesh.setMapSupport(MAP_ALPHA, TRUE);
            mesh.setNumMapVerts(MAP_ALPHA, n, FALSE);
            mesh.setNumMapFaces(MAP_ALPHA, nt, FALSE);
            mesh.setVCDisplayData(MAP_ALPHA, 0, 0);
            for (int i=0; i<nt; ++i)
               mesh.vcFaceData[i] = vcFace[i];
            for (int i=0; i<n; ++i)
               mesh.vertColArray[i] = vertAlpha[i];
         }
         // Add the Vertex Paint Color modifier now
         if (hasAlpha || hasColor)
         {
            mesh.setMapSupport(0, TRUE);
            mesh.setNumMapVerts(0, n, TRUE);
            mesh.setNumMapFaces(0, nt, FALSE);
            mesh.setVCDisplayData(0, NULL, NULL);
            for (int i=0; i<nt; ++i)
               mesh.vcFaceData[i] = vcFace[i];
            for (int i=0; i<n; ++i)
               mesh.vertColArray[i] = vertColors[i];
         }
      }
   }
   else if (vertexColorMode == 2)
   {
#if VERSION_3DSMAX > ((5000<<16)+(15<<8)+0) // Version 5
      vector<Color4> cv = triGeomData->GetColors();
      int n = cv.size();
      if (n > 0)
      {

         INode *tnode = node->GetINode();

         vector<Color> colorMap, alphaMap;
         IVertexPaint::VertColorTab vertColors, vertAlpha;
         vertColors.SetCount(n, TRUE);
         vertAlpha.SetCount(n, TRUE);
         colorMap.resize(n);
         alphaMap.resize(n);
         mesh.setNumVertCol(cv.size());

         for (int i=0; i<n; i++){
            Color4& c = cv[i];
            mesh.vertCol[i].Set(c.r, c.g, c.b);

            hasColor |= (c.r != 1.0f && c.g != 1.0f && c.b != 1.0f);
            colorMap[i] = Color(c.r,c.g,c.b);
            vertColors[i] = &colorMap[i];

            hasAlpha |= (c.a != 1.0f);
            alphaMap[i] = Color(c.a,c.a,c.a);
            vertAlpha[i] = &alphaMap[i];
         }
         // Civ4 assumes that vcFace is filled in even if only alpha is given via color modifier
         if (hasColor || hasAlpha)
         {
            int n = tris.size();
            mesh.setNumVCFaces(n);
            for (int i=0; i<n; ++i) {
               Triangle& t = tris[i];
               TVFace& vcf = mesh.vcFace[i];
               vcf.setTVerts(t.v1, t.v2, t.v3);
            }
         }
         // Add the Vertex Paint Color modifier now
         if (hasColor)
         {
            IDerivedObject *dobj = CreateDerivedObject(tnode->GetObjectRef());
            Modifier * mod = (Modifier*)CreateInstance(OSM_CLASS_ID, PAINTLAYERMOD_CLASS_ID);
            dobj->AddModifier(mod);
            tnode->SetObjectRef(dobj);
            IVertexPaint* ivertexPaint = (IVertexPaint*)mod->GetInterface(IVERTEXPAINT_INTERFACE_ID);
            ObjectState os = tnode->EvalWorldState(0);
            IAssignVertexColors::Options o;
            ivertexPaint->GetOptions(o);
            o.mapChannel = 0;
            o.mixVertColors = true;
            ivertexPaint->SetOptions(o);
            ivertexPaint->SetColors(tnode, vertColors);
            //mod->DisableModInViews();
            //mod->EnableModInViews();
         }
         // Add the Vertex Paint Alpha modifier now
         if (hasAlpha)
         {
            IDerivedObject *dobj = CreateDerivedObject(tnode->GetObjectRef());
            Modifier * mod = (Modifier*)CreateInstance(OSM_CLASS_ID, PAINTLAYERMOD_CLASS_ID);
            dobj->AddModifier(mod);
            tnode->SetObjectRef(dobj);
            IVertexPaint* ivertexPaint = (IVertexPaint*)mod->GetInterface(IVERTEXPAINT_INTERFACE_ID);
            ObjectState os = tnode->EvalWorldState(0);
            IAssignVertexColors::Options o;
            ivertexPaint->GetOptions(o);
            o.mapChannel = -2;
            o.mixVertColors = true;
            ivertexPaint->SetOptions(o);
            ivertexPaint->SetColors(tnode, vertAlpha);
            //mod->DisableModInViews();
            //mod->EnableModInViews();
         }
      }
#endif
   }
   return (hasAlpha || hasColor);
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
   Modifier *skinMod = GetOrCreateSkin(tnode);
   TriObject *triObject = GetTriObject(tnode->GetObjectRef());
   Mesh& m = triObject->GetMesh();

   //get the skin interface
   if (ISkin *skin = (ISkin *) skinMod->GetInterface(I_SKIN)){
      ISkinImportData* iskinImport = (ISkinImportData*) skinMod->GetInterface(I_SKINIMPORTDATA);

      // Set the num weights to 4.  Yes its in the nif but Shon doesn't like to expose those values 
      //   and the value always seems to be 4 anyway.  I'd also this be more dynamic than hard coded numbers
      //   but I cant figure out the correct values to pass the scripting engine from here so I'm giving up.
      int numWeightsPerVertex = 4;
      IParamBlock2 *params = skinMod->GetParamBlockByID(2/*advanced*/);
      params->SetValue(0x7/*bone_Limit*/, 0, numWeightsPerVertex);

      // Can get some truly bizarre animations without this in MAX with Civ4 Leaderheads
#if VERSION_3DSMAX > ((5000<<16)+(15<<8)+0) // Version 6+
      BOOL ignore = TRUE;
      params->SetValue(0xE/*ignoreBoneScale*/, 0, ignore);
#endif

      //RefTargetHandle advanced = skinMod->GetReference(3);
      //setMAXScriptValue(advanced, "bone_Limit", 0, numWeightsPerVertex);

      Matrix3 geom = TOMATRIX3(triGeom->GetLocalTransform());
      Matrix3 m3 = TOMATRIX3(data->GetOverallTransform());
      Matrix3 im3 = Inverse(m3);
      Matrix3 nm3 = im3 * geom;
      iskinImport->SetSkinTm(tnode, nm3, nm3); // ???
      // Create Bone List
      Tab<INode*> bones;
      for (size_t i=0; i<nifBones.size(); ++i){
         NiNodeRef bone = nifBones[i];

         string name = bone->GetName();
         if (INode *boneRef = gi->GetINodeByName(name.c_str())) {
            bones.Append(1, &boneRef);
            iskinImport->AddBoneEx(boneRef, TRUE);

            //// Set Bone Transform
            Matrix3 b3 = TOMATRIX3(data->GetBoneTransform(i));
            Matrix3 ib3 = Inverse(b3);
            ib3 *= geom;
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
