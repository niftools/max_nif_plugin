#include "pch.h"

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
	if ((coll && !mExportCollision) ||
		(node->IsHidden() && !mExportHidden && !coll) ||
		(mSelectedOnly && !node->Selected()))
		return Skip;

	NiNodeRef newParent;
	TimeValue t = 0;
	ObjectState os = node->EvalWorldState(t); 
	if (!coll && os.obj && os.obj->SuperClassID()==GEOMOBJECT_CLASS_ID)
	{
/*		newParent = DynamicCast<NiNode>(CreateBlock("NiNode"));
		parent->AddChild(DynamicCast<NiAVObject>(newParent));

		Matrix33 rot;
		Vector3 trans;
		nodeTransform(rot, trans, node, t);

		newParent->SetLocalRotation(rot);
		newParent->SetLocalTranslation(trans);
		string name = (char*)node->GetName();
		newParent->SetName(name);
*/
		newParent = makeNode(parent, node);

		Result result;
		result = exportMesh(newParent, node, t);
		if (result != Ok)
			return result;

	} else
	if (node->IsGroupHead())
	{
		newParent = makeNode(parent, node);
/*		newParent = DynamicCast<NiNode>(CreateBlock("NiNode"));
		Matrix33 rot;
		Vector3 trans;
		nodeTransform(rot, trans, node, t);
		newParent->SetLocalRotation(rot);
		newParent->SetLocalTranslation(trans);
		string name = (char*)node->GetName();
		newParent->SetName(name);

		parent->AddChild(DynamicCast<NiAVObject>(newParent));
*/
	} else
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

	TriObject *tri = (TriObject *)os.obj->ConvertToType(t, Class_ID(TRIOBJ_CLASS_ID, 0));
	if (!tri)
		return Error;

	Mesh *mesh = &tri->GetMesh();
	mesh->buildNormals();

	Result result = Ok;
	while (1)
	{
		FaceGroups grps;
		if (!splitMesh(node, mesh, grps, t))
		{
			result = Error;
			break;
		}

		FaceGroups::iterator grp;
		for (grp=grps.begin(); grp!=grps.end(); ++grp)
		{
			if (!makeMesh(ninode, getMaterial(node, grp->first), grp->second))
			{
				result = Error;
				break;
			}
		}

		break;
	}

	if (tri != os.obj)
		tri->DeleteMe();

	return result;
}

bool Exporter::makeMesh(NiNodeRef &parent, Mtl *mtl, FaceGroup &grp)
{
	NiTriBasedGeomRef shape;
	NiTriBasedGeomDataRef data;

	if (mTriStrips)
	{
		NiTriStripsRef stripsShape = DynamicCast<NiTriStrips>(CreateBlock("NiTriStrips"));
		shape = DynamicCast<NiTriBasedGeom>(stripsShape);

		TriStrips strips;
		strippify(strips, grp);
		NiTriStripsDataRef stripData = makeTriStripsData(strips);
		data = DynamicCast<NiTriBasedGeomData>(stripData);

	} else
	{
		NiTriShapeRef tris = DynamicCast<NiTriShape>(CreateBlock("NiTriShape"));
		NiTriShapeDataRef triData = DynamicCast<NiTriShapeData>(CreateBlock("NiTriShapeData"));
		data = DynamicCast<NiTriBasedGeomData>(triData);
		shape = DynamicCast<NiTriBasedGeom>(tris);

		triData->SetTriangles(grp.faces);
	}

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
	makeTexture(av, mtl);

	parent->AddChild(DynamicCast<NiAVObject>(shape));

	return true;
}

int Exporter::addVertex(FaceGroup &grp, int face, int vi, Mesh *mesh, const Matrix3 &texm)
{
	Point3 pt = mesh->verts[ mesh->faces[ face ].v[ vi ] ];
	Point3 norm = getVertexNormal(mesh, face, mesh->getRVertPtr(mesh->faces[ face ].v[ vi ]));

	Point3 uv;
	if (mesh->tvFace)
		uv = mesh->tVerts[ mesh->tvFace[ face ].t[ vi ]] * texm;

	VertColor col;
	if (mVertexColors && mesh->vertCol)
		col = mesh->vertCol[ mesh->vcFace[ face ].t[ vi ] ];

	for (int i=0; i<grp.verts.size(); i++)
	{
		if (equal(grp.verts[i], pt, mWeldThresh) &&
			equal(grp.vnorms[i], norm, 0))
		{
			if (mesh->tvFace && (grp.uvs[i].u!=uv.x || grp.uvs[i].v!=uv.y))
				continue;

			if (mVertexColors && mesh->vertCol &&
				(grp.vcolors[i].r!=col.x ||
				 grp.vcolors[i].g!=col.y ||
				 grp.vcolors[i].b!=col.z))
				continue;

			return i;
		}
	}
	
	grp.verts.push_back(Vector3(pt.x, pt.y, pt.z));
	grp.vnorms.push_back(Vector3(norm.x, norm.y, norm.z));

	if (mesh->tvFace)
		grp.uvs.push_back(TexCoord(uv.x, uv.y));

	if (mVertexColors && mesh->vertCol)
		grp.vcolors.push_back(Color4(col.x, col.y, col.z, 1));

	return grp.verts.size()-1;
}

void Exporter::addFace(FaceGroup &grp, int face, const int vi[3], Mesh *mesh, const Matrix3 &texm)
{
	Triangle tri;
	for (int i=0; i<3; i++)
		tri[i] = addVertex(grp, face, vi[i], mesh, texm);

	grp.faces.push_back(tri);
}

bool Exporter::splitMesh(INode *node, Mesh *mesh, FaceGroups &grps, TimeValue t)
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

		addFace(grps[mtlID], i, vi, mesh, texm);
	}

	return true;
}
