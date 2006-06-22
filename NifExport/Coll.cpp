#include "pch.h"

int Exporter::addVertex(vector<Vector3> &verts, vector<Vector3> &vnorms, const Point3 &pt, const Point3 &norm)
{
	for (int i=0; i<verts.size(); i++)
	{
		if (equal(verts[i], pt, mWeldThresh) &&
			equal(vnorms[i], norm, 0))
			return i;
	}
	
	verts.push_back(Vector3(pt.x, pt.y, pt.z));
	vnorms.push_back(Vector3(norm.x, norm.y, norm.z));

	return verts.size()-1;
}

void Exporter::addFace(Triangles &tris, vector<Vector3> &verts, vector<Vector3> &vnorms, 
			 int face, const int vi[3], Mesh *mesh)
{
	Triangle tri;
	for (int i=0; i<3; i++)
	{
		tri[i] = addVertex(verts, vnorms,
			               mesh->verts[ mesh->faces[ face ].v[ vi[i] ] ], 
						   getVertexNormal(mesh, face, mesh->getRVertPtr(mesh->faces[ face ].v[ vi[i] ])));
	}
	tris.push_back(tri);
}

bool Exporter::makeCollisionHierarchy(NiNodeRef &parent, INode *node, TimeValue t)
{	
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

	ObjectState os = node->EvalWorldState(t);
	if (!os.obj || os.obj->SuperClassID()!=GEOMOBJECT_CLASS_ID)
		return Error;
		
	Object *obj = os.obj;
	if (!obj->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID, 0))) 
		return Error;

	TriObject *tri = (TriObject *)obj->ConvertToType(t, Class_ID(TRIOBJ_CLASS_ID, 0));
	if (!tri)
		return false;

	Mesh *mesh = &tri->GetMesh();
	mesh->buildNormals();

	int lyr, mtl;
	if (!getHvkLayer(node, lyr))
		lyr = NP_DEFAULT_HVK_LAYER;

	if (!getHvkMaterial(node, mtl))
		mtl = NP_DEFAULT_HVK_LAYER;

	// setup shape data
	vector<Vector3> verts;
	vector<Vector3> vnorms;
	Triangles		tris;

	for (int i=0; i<mesh->getNumFaces(); i++)
		addFace(tris, verts, vnorms, i, vi, mesh);

	NiTriStripsDataRef data = makeTriStripsData(tris);
	data->SetVertices(verts);
	data->SetNormals(vnorms);

	// setup shape
	bhkNiTriStripsShapeRef shape = DynamicCast<bhkNiTriStripsShape>(CreateBlock("bhkNiTriStripsShape"));
	shape->SetNumStripsData(1);
	shape->SetStripsData(0, data);
	shape->SetMaterial(mtl);

	array<float, 2> unknownFloats1;
	uint i1 = 0x3DCCCCCD;
	uint i2 = 0x004ABE60;
	unknownFloats1[0] = *((float*)&i1);
	unknownFloats1[1] = *((float*)&i2);
	shape->SetUnknownFloats1(unknownFloats1);

	array<float, 3> unknownFloats2;
	unknownFloats2[0] = 1;
	unknownFloats2[1] = 1;
	unknownFloats2[2] = 1;
	shape->SetUnknownFloats2(unknownFloats2);

	array<uint, 5> unknownInts1;
	unknownInts1[4] = 1;
	shape->SetUnknownInts1(unknownInts1);

	vector<uint> unknownInts3;
	unknownInts3.resize(1);
	shape->SetUnknownInts3(unknownInts3);

	// setup collision object
	bhkCollisionObjectRef co = DynamicCast<bhkCollisionObject>(CreateBlock("bhkCollisionObject"));

	// setup body
	bhkRigidBodyTRef body = DynamicCast<bhkRigidBodyT>(CreateBlock("bhkRigidBodyT"));

	Vector3 trans;
	QuaternionXYZW q;
	nodeTransform(q, trans, node, t, false);
	body->SetRotation(q);
	body->SetTranslation(Vector3(trans.x/7, trans.y/7, trans.z/7));


	body->SetLayer(lyr);
	body->SetLayerCopy(lyr);

	// link
	parent->SetCollisionObject(DynamicCast<NiCollisionObject>(co));
	co->SetParent(parent);
	co->SetBody(DynamicCast<NiObject>(body));
	body->SetShape(DynamicCast<bhkShape>(shape));

	if (obj != tri)
		tri->DeleteMe();

	return true;
}
