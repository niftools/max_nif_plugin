#include "pch.h"
#include "niutils.h"
#include "iskin.h"
#include "MeshNormalSpec.h"
#ifdef USE_BIPED
#  include <cs/BipedApi.h>
#endif
#include "obj/NiSkinInstance.h"
#include "obj/NiSkinData.h"
#include "obj/NiSkinPartition.h"
/*

void FPUtility::GetAlphaVal(void)
{
	if(ip->GetSelNodeCount()<1)return;
	INode *node = ip->GetSelNode(0);
	if(!node)return;
 	ObjectState os = node->EvalWorldState(0);
	Object *obj = os.obj;
	BOOL delMesh = false;

	if (obj && obj->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID, 0))) { 
		TriObject * tri = NULL;
		tri = (TriObject *) obj->ConvertToType(0, Class_ID(TRIOBJ_CLASS_ID, 0));
		if (obj != tri) 
			delMesh = true; // we own the copy
		if (tri) 
		{
			Mesh * mesh = &(tri->GetMesh());
			MeshDelta md(*mesh);
			BOOL support = mesh->mapSupport(MAP_ALPHA);
			if(support)
			{
				UVVert *alpha = mesh->mapVerts(MAP_ALPHA);
				for(int i=0;igetNumVerts();i++)
				{
					float a = alpha[i].x;
				}
			}

		}

		if (delMesh)
			delete tri;
	}
}
				
*/


Exporter::Result Exporter::exportMeshes(NiNodeRef &parent, INode *node)
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
   if (node->IsBoneShowing() || strmatch(nodeName, "Bone"))
      newParent = makeNode(nodeParent, node, local);
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
         newParent = makeNode(nodeParent, node, local);
      } 
      else 
      {
         newParent = (mExportExtraNodes) ? makeNode(nodeParent, node, local) : nodeParent;

         Result result;
         result = exportMesh(newParent, node, t);
         if (result != Ok)
            return result;
      }
	} 
   else if (isMeshGroup(node) && local) // only create node if local
	{
		newParent = makeNode(parent, node, local);

	} 
   else
		newParent = parent;

	for (int i=0; i<node->NumberOfChildren(); i++) 
	{
		Result result = exportMeshes(newParent, node->GetChildNode(i));
		if (result!=Ok && result!=Skip)
			return result;
	}

	return Ok;
}


Exporter::Result Exporter::exportMesh(NiNodeRef &ninode, INode *node, TimeValue t)
{	
	ObjectState os = node->EvalWorldState(t);

   bool local = !mFlattenHierarchy;

	TriObject *tri = (TriObject *)os.obj->ConvertToType(t, Class_ID(TRIOBJ_CLASS_ID, 0));
	if (!tri)
		return Error;

	Mesh *mesh = &tri->GetMesh();

   // Note that calling setVCDisplayData will clear things like normals so we set this up first
   vector<Color4> vertColors;
   if (mVertexColors)
   {
      bool hasvc = false;
      if (mesh->mapSupport(MAP_ALPHA))
      {
         mesh->setVCDisplayData(MAP_ALPHA);
         int n = mesh->getNumVertCol();
         if (n > vertColors.size())
            vertColors.assign(n, Color4(1.0f, 1.0f, 1.0f, 1.0f));
         VertColor *vertCol = mesh->vertColArray;
         if (vertCol) {
            for (int i=0; i<n; ++i) {
               VertColor c = vertCol[ i ];
               float a = (c.x + c.y + c.z) / 3.0f;
               vertColors[i].a = a;
               hasvc |= (a != 1.0f);
            }
         }
      }
      if (mesh->mapSupport(0))
      {
         mesh->setVCDisplayData(0);
         VertColor *vertCol = mesh->vertColArray;
         int n = mesh->getNumVertCol();
         if (n > vertColors.size())
            vertColors.assign(n, Color4(1.0f, 1.0f, 1.0f, 1.0f));
         if (vertCol) {
            for (int i=0; i<n; ++i) {
               VertColor col = vertCol[ i ];
               vertColors[i] = Color4(col.x, col.y, col.z, vertColors[i].a);
               hasvc |= (col.x != 1.0f || col.y != 1.0f || col.z != 1.0f);
            }
         }
      }
      if (!hasvc) vertColors.clear();
   }
   
   MeshNormalSpec *specNorms = mesh->GetSpecifiedNormals ();
   if (NULL != specNorms) {
      specNorms->CheckNormals();
   } else {
      mesh->checkNormals(TRUE);
   }

	Result result = Ok;
	while (1)
	{
		FaceGroups grps;
		if (!splitMesh(node, mesh, grps, t, vertColors))
		{
			result = Error;
			break;
		}
      bool exportStrips = mTriStrips;

      Matrix44 tm = Matrix44::IDENTITY;
      if (!mExportExtraNodes) {
         Matrix33 rot; Vector3 trans;
         nodeTransform(rot, trans, node, t, local);
         tm = Matrix44(trans, rot, 1.0f);
      }

      TSTR basename = node->NodeName();
      TSTR format = (!basename.isNull() && grps.size() > 1) ? "%s:%d" : "%s";

      int i=1;
		FaceGroups::iterator grp;
		for (grp=grps.begin(); grp!=grps.end(); ++grp, ++i)
		{
         string name = FormatString(basename, basename.data(), i);
         NiTriBasedGeomRef shape = makeMesh(ninode, getMaterial(node, grp->first), grp->second, exportStrips);
         if (shape == NULL)
			{
				result = Error;
				break;
			}

         shape->SetName(name);
         shape->SetLocalTransform(tm);
         makeSkin(shape, node, grp->second, t);
		}

		break;
	}

	if (tri != os.obj)
		tri->DeleteMe();

	return result;
}

NiTriBasedGeomRef Exporter::makeMesh(NiNodeRef &parent, Mtl *mtl, FaceGroup &grp, bool exportStrips)
{
	NiTriBasedGeomRef shape;
	NiTriBasedGeomDataRef data;

	if (exportStrips) {
      shape = new NiTriStrips();
      data = new NiTriStripsData();
	} else {
      shape = new NiTriShape();
      data = new NiTriShapeData();
	}

   data->SetTriangles(grp.faces);
	data->SetVertices(grp.verts);
	data->SetNormals(grp.vnorms);

	if (grp.uvs.size() > 0)
	{
		data->SetUVSetCount(1);
		data->SetUVSet(0, grp.uvs);
	}

	if (grp.vcolors.size() > 0)
		data->SetVertexColors(grp.vcolors);

	shape->SetData(data);

	NiAVObjectRef av(DynamicCast<NiAVObject>(shape));
	makeMaterial(av, mtl);

	parent->AddChild(DynamicCast<NiAVObject>(shape));

	return shape;
}

int Exporter::addVertex(FaceGroup &grp, int face, int vi, Mesh *mesh, const Matrix3 &texm, vector<Color4>& vertColors)
{
   int vidx = mesh->faces[ face ].v[ vi ];
	Point3 pt = mesh->verts[ vidx ];
   Point3 norm;

   MeshNormalSpec *specNorms = mesh->GetSpecifiedNormals ();
   if (NULL != specNorms)
      norm = specNorms->GetNormal(face, vi);
   else
      norm = getVertexNormal(mesh, face, mesh->getRVertPtr(vidx));

	Point3 uv;
   if (mesh->tVerts && mesh->tvFace) {
		uv = mesh->tVerts[ mesh->tvFace[ face ].t[ vi ]] * texm;
      uv.y += 1.0f;
   }

   Color4 col(1.0f, 1.0f, 1.0f);
   if (mVertexColors && !vertColors.empty()){
      TVFace *vcFace = mesh->vcFaceData ? mesh->vcFaceData : mesh->vcFace;
      if (vcFace) {
         int vidx = vcFace[ face ].t[ vi ];
         col = vertColors[vidx];
      }
   }

	for (int i=0; i<grp.verts.size(); i++)
	{
		if (equal(grp.verts[i], pt, mWeldThresh) &&
			equal(grp.vnorms[i], norm, 0))
		{
			if (mesh->tVerts && mesh->tvFace && (grp.uvs[i].u!=uv.x || grp.uvs[i].v!=uv.y))
				continue;

			if (mVertexColors && !vertColors.empty() &&
				(grp.vcolors[i].r!=col.r ||
				 grp.vcolors[i].g!=col.g ||
				 grp.vcolors[i].b!=col.b))
				continue;

			return i;
		}
	}
	
   grp.vidx.push_back(vidx);
	grp.verts.push_back(Vector3(pt.x, pt.y, pt.z));
	grp.vnorms.push_back(Vector3(norm.x, norm.y, norm.z));

	if (mesh->tvFace)
		grp.uvs.push_back(TexCoord(uv.x, uv.y));

   if (mVertexColors && !vertColors.empty()){
		grp.vcolors.push_back(col);
   }

	return grp.verts.size()-1;
}

void Exporter::addFace(FaceGroup &grp, int face, const int vi[3], Mesh *mesh, const Matrix3 &texm, vector<Color4>& vertColors)
{
	Triangle tri;
	for (int i=0; i<3; i++)
		tri[i] = addVertex(grp, face, vi[i], mesh, texm, vertColors);

	grp.faces.push_back(tri);
}

bool Exporter::splitMesh(INode *node, Mesh *mesh, FaceGroups &grps, TimeValue t, vector<Color4>& vertColors)
{
	Mtl* nodeMtl = node->GetMtl();
	Matrix3 tm = node->GetObjTMAfterWSM(t);

	// Order of the vertices. Get 'em counter clockwise if the objects is
	// negatively scaled.
	int vi[3];
	if (TMNegParity(tm)) 
	{
		vi[0] = 2;
		vi[1] = 1;
		vi[2] = 0;
	} else 
	{
		vi[0] = 0;
		vi[1] = 1;
		vi[2] = 2;
	}

	Matrix3 flip;
	flip.IdentityMatrix();
	flip.Scale(Point3(1, -1, 1));

	int i, numSubMtls = nodeMtl?nodeMtl->NumSubMtls():0;
	for (i=0; i<mesh->getNumFaces(); i++) 
	{
		int mtlID = (numSubMtls!=0) ? (mesh->faces[i].getMatID() % numSubMtls) : 0;

		Matrix3 texm;
		getTextureMatrix(texm, getMaterial(node, mtlID));

		texm *= flip;

		addFace(grps[mtlID], i, vi, mesh, texm, vertColors);
	}

	return true;
}

// Callback interface to register a Skin after entire structure is built due to contraints
//   in the nif library
struct SkinInstance : public Exporter::NiCallback
{  
   typedef vector<SkinWeight> SkinWeightList;
   typedef vector<SkinWeightList> BoneWeightList;

   Exporter *owner;
   // Common Data
   NiTriBasedGeomRef shape;
   vector<NiNodeRef> boneList;

   // Bone to weight map
   BoneWeightList boneWeights;

   SkinInstance(Exporter *Owner) : owner(Owner) {}
   virtual ~SkinInstance() {}
   virtual Exporter::Result execute();
};


bool Exporter::makeSkin(NiTriBasedGeomRef shape, INode *node, FaceGroup &grp, TimeValue t)
{
   if (!mExportSkin)
      return false;

   if (grp.verts.empty())
      return false;

   //get the skin modifier
   Modifier *mod = GetSkin(node);
   if (!mod)
      return false;

   ISkin *skin = (ISkin *) mod->GetInterface(I_SKIN);
   if (!skin)
      return false;

   ISkinContextData *skinData = skin->GetContextInterface(node);
   if (!skinData)
      return false;

   if (grp.strips.empty())
      strippify(grp);

   // Create new call back to finish export
   SkinInstance* si = new SkinInstance(this);
   mPostExportCallbacks.push_back(si);

   si->shape = shape;

   // Get bone references (may not actually exist in proper structure at this time)
   int totalBones = skin->GetNumBones();
   si->boneWeights.resize(totalBones);
   si->boneList.resize(totalBones);
   for (int i=0; i<totalBones; ++i) {
      string name = skin->GetBone(i)->GetName();
      si->boneList[i] = getNode(name);
   }

   vector<int>& vidx = grp.vidx;
   int nv = vidx.size();
   for (int i=0; i<nv; ++i)
   {
      int vi = vidx[i];
      int nbones = skinData->GetNumAssignedBones(vi);
      for (int j=0; j<nbones; ++j)
      {
         SkinWeight sw;
         sw.index = i;
         sw.weight = skinData->GetBoneWeight(vi,j);
         int boneIndex = skinData->GetAssignedBone(vi,j);

         SkinInstance::SkinWeightList& weights = si->boneWeights[boneIndex];
         weights.push_back(sw);
      }         
   }

   // remove unused bones
   vector<NiNodeRef>::iterator bitr = si->boneList.begin();
   SkinInstance::BoneWeightList::iterator switr = si->boneWeights.begin();
   for (int i=0; i<totalBones; ++i) {
      vector<SkinWeight> &weights = (*switr);
      if (weights.empty())
      {
         bitr = si->boneList.erase(bitr);
         switr = si->boneWeights.erase(switr);
      }
      else
      {
         ++bitr, ++switr;
      }      
   }

   return true;
}

Exporter::Result SkinInstance::execute()
{
   shape->BindSkin(boneList);
   uint bone = 0;
   for (BoneWeightList::iterator bitr = boneWeights.begin(); bitr != boneWeights.end(); ++bitr, ++bone) {
      shape->SetBoneWeights(bone, (*bitr));
   }
   shape->GenHardwareSkinInfo();
   return Exporter::Ok;
}
