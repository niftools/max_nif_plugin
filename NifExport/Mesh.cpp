#include "pch.h"

Exporter::Result Exporter::exportMesh(NiNodeRef &parent, INode *node, TimeValue t)
{	
	ObjectState os = node->EvalWorldState(t);
	if (!os.obj || os.obj->SuperClassID()!=GEOMOBJECT_CLASS_ID)
		return Error;
		
	BOOL alloc;
	Object *obj = os.obj;
	if (!obj->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID, 0))) 
		return Error;

	TriObject *tri = (TriObject *) obj->ConvertToType(t, Class_ID(TRIOBJ_CLASS_ID, 0));
	if (!tri)
		return Error;

	if (obj != tri) 
		alloc = TRUE;
	else
		alloc = FALSE;

	Mesh *mesh = &tri->GetMesh();
	mesh->buildNormals();
/*
	Matrix3 pm = node->GetParentTM(t);
	pm.Invert();
	Matrix3 tm = node->GetObjTMAfterWSM(t) * pm;
	Matrix33 rot;
	convertMatrix(rot, tm);
	Vector3 trans(tm.GetTrans().x, tm.GetTrans().y, tm.GetTrans().z);
*/
	Matrix33 rot;
	Vector3 trans;
	nodeTransform(rot, trans, node, t);

	NiNodeRef ninode = DynamicCast<NiNode>(CreateBlock("NiNode"));
	ninode->SetLocalRotation(rot);
	ninode->SetLocalTranslation(trans);
	string name = (char*)node->GetName();
	ninode->SetName(name);
	parent->AddChild(DynamicCast<NiAVObject>(ninode));

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

	if (alloc)
		delete tri;

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
	data->SetUVSetCount(1);
	data->SetUVSet(0, grp.uvs);

	if (grp.vcolors.size() > 0)
		data->SetVertexColors(grp.vcolors);

	shape->SetData(data);

	NiAVObjectRef av(DynamicCast<NiAVObject>(shape));
	makeMaterial(av, mtl);
	makeTexture(av, mtl);

	parent->AddChild(DynamicCast<NiAVObject>(shape));

	return true;
}

int Exporter::addVertex(FaceGroup &grp, int face, int vi, Mesh *mesh)
{
	Point3 pt = mesh->verts[ mesh->faces[ face ].v[ vi ] ];
	Point3 uv = mesh->tVerts[ mesh->tvFace[ face ].t[ vi ]];
	Point3 norm = getVertexNormal(mesh, face, mesh->getRVertPtr(mesh->faces[ face ].v[ vi ]));

	VertColor col;
	if (mVertexColors && mesh->vertCol)
		col = mesh->vertCol[ mesh->vcFace[ face ].t[ vi ] ];

	for (int i=0; i<grp.verts.size(); i++)
	{
		if (equal(grp.verts[i], pt, mWeldThresh) &&
			grp.uvs[i].u==uv.x && grp.uvs[i].v==uv.y &&
			equal(grp.vnorms[i], norm, 0))
		{
			if (mVertexColors && mesh->vertCol &&
				(grp.vcolors[i].r!=col.x ||
				 grp.vcolors[i].g!=col.y ||
				 grp.vcolors[i].b!=col.z))
				continue;

			return i;
		}
	}
	
	grp.verts.push_back(Vector3(pt.x, pt.y, pt.z));
	grp.uvs.push_back(TexCoord(uv.x, uv.y));
	grp.vnorms.push_back(Vector3(norm.x, norm.y, norm.z));

	if (mVertexColors && mesh->vertCol)
		grp.vcolors.push_back(Color4(col.x, col.y, col.z, 1));

	return grp.verts.size()-1;
}

void Exporter::addFace(FaceGroup &grp, int face, const int vi[3], Mesh *mesh)
{
	Triangle tri;
	for (int i=0; i<3; i++)
	{
/*		Point3 tv = mesh->verts[ mesh->faces[ face ].v[ vi[i] ] ]; // * tm;
		tri[i] = addVertex(grp, 
			               tv, 
						   mesh->tVerts[ mesh->tvFace[ face ].t[ vi[i] ]], 
						   getVertexNormal(mesh, face, mesh->getRVertPtr(mesh->faces[ face ].v[ vi[i] ])));
*/
		tri[i] = addVertex(grp, face, vi[i], mesh);
	}

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

	int i, numSubMtls = nodeMtl?nodeMtl->NumSubMtls():0;
	for (i=0; i<mesh->getNumFaces(); i++) 
	{
		int mtlID = (numSubMtls!=0) ? (mesh->faces[i].getMatID() % numSubMtls) : 0;
		addFace(grps[mtlID], i, vi, mesh);
	}

	return true;
}
