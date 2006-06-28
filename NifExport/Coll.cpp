#include "pch.h"

static Class_ID SCUBA_CLASS_ID(0x6d3d77ac, 0x79c939a9);

enum
{
	CAPSULE_RADIUS = 0,
	CAPSULE_HEIGHT = 1,
};


/*
To mimic the "Reset Transform" and "Reset Scale" behavior, the following code snippet should help:



	Interface *ip = theResetScale.ip;
	TimeValue t = ip->GetTime();
	
	Control *tmControl = node->GetTMController();
	BOOL lookAt = tmControl->GetRollController() ? TRUE : FALSE;

	Matrix3 ntm = node->GetNodeTM(t);
	Matrix3 ptm = node->GetParentTM(t);
	Matrix3 rtm = ntm * Inverse(ptm);
	Matrix3 otm(1);
	Quat rot;

	// Grab the trans, and then set it to 0
	Point3 trans = rtm.GetTrans();
	rtm.NoTrans();
	
	// We're only doing scale - save out the 
	// rotation so we can put it back
	AffineParts parts;
	decomp_affine(rtm, &parts);
	rot = parts.q;

	// Build the offset tm
	otm.PreTranslate(node->GetObjOffsetPos()); 
	if (node->GetObjOffsetRot()!=IdentQuat()) {
	   	PreRotateMatrix(otm,node->GetObjOffsetRot());
		}

	Point3 tS(1,1,1);
	if ( node->GetObjOffsetScale().s != tS ) {
		ApplyScaling(otm,node->GetObjOffsetScale());
		}
	
	// Apply the relative tm to the offset
	otm = otm * rtm;
	decomp_affine(otm, &parts);
	node->SetObjOffsetPos(parts.t);	
	node->SetObjOffsetScale(ScaleValue(parts.k*parts.f,parts.u));

	// Now set the transform controller with a matrix 
	// that has no rotation or scale
	rtm.IdentityMatrix();
	rtm.SetTrans(trans);
	if (!lookAt) {
		PreRotateMatrix(rtm,rot);
		}

	// But first, want to keep children stationary.
	Matrix3 ttm = rtm*ptm;
	for (int i=0; iNumberOfChildren(); i++)  {
		Control *tmc  = node->GetChildNode(i)->GetTMController();
		Matrix3 oldtm = node->GetChildNode(i)->GetNodeTM(t);
		SetXFormPacket pk(oldtm,ttm);
		tmc->SetValue(t,&pk);
		}

  	SetXFormPacket pckt(rtm);
	tmControl->SetValue(t,&pckt);		



To mimic the "Align to world" behavior, the following code snippet should help:



    AffineParts parts;
    TimeValue currtime = m_pInterface->GetTime();
    Matrix3 m = pNode->GetNodeTM(currtime);
    decomp_affine(m, &parts); 
    if (rotobj) {
        // if "affect obj only" we move it simply thus:
        pNode->SetObjOffsetRot(Inverse(parts.q));
    } else {
        // otherwise, "affect pivot only" we would do:
        IdentityTM ident;
        Matrix3 wax = ident;
        wax.SetTrans(m.GetTrans());  // world aligned axis,  centered at pivot point
        pNode->Rotate(currtime, wax, Inverse(parts.q),TRUE,FALSE, PIV_PIVOT_ONLY);
    }
    m_pInterface->RedrawViews(m_pInterface->GetTime(),REDRAW_NORMAL,NULL);

*/

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
/*
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

	// setup shape data
	vector<Vector3> verts;
	vector<Vector3> vnorms;
	Triangles		tris;

	for (int i=0; i<mesh->getNumFaces(); i++)
		addFace(tris, verts, vnorms, i, vi, mesh);

	TriStrips strips;
	strippify(strips, verts, vnorms, tris);
	NiTriStripsDataRef data = makeTriStripsData(strips);
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
	body->SetMotionSystem(msys);
	body->SetQualityType(qtype);
	body->SetMass(mass);
	body->SetLinearDamping(lindamp);
	body->SetAngularDamping(angdamp);
	body->SetFriction(frict);
	body->SetRestitution(resti);
	body->SetMaxLinearVelocity(maxlinvel);
	body->SetMaxAngularVelocity(maxangvel);
	body->SetPenetrationDepth(pendepth);
	body->SetCenter(center);

	// link
	parent->SetCollisionObject(DynamicCast<NiCollisionObject>(co));
	co->SetParent(parent);
	co->SetBody(DynamicCast<NiObject>(body));
	body->SetShape(DynamicCast<bhkShape>(shape));

	if (obj != tri)
		tri->DeleteMe();

	return true;
}
*/

Exporter::Result Exporter::exportCollision(NiNodeRef &parent, INode *node)
{
	// marked as collision?
	NiNodeRef newParent;
	if (npIsCollision(node))
	{
/*		NiNodeRef n = DynamicCast<NiNode>(CreateBlock("NiNode"));
		parent->AddChild(DynamicCast<NiAVObject>(n));

		Matrix33 rot;
		Vector3 trans;
		TimeValue t = 0;
		nodeTransform(rot, trans, node, t);
		n->SetLocalRotation(rot);
		n->SetLocalTranslation(trans);
		string name = (char*)node->GetName();
		n->SetName(name);


/*		Vector3 trans;
		QuaternionXYZW q;
		TimeValue t = 0;
		nodeTransform(q, trans, node, t, false);
		body->SetRotation(q);
		body->SetTranslation(Vector3(trans.x/7, trans.y/7, trans.z/7));
*/
		newParent = makeNode(parent, node);

		bhkSphereRepShapeRef shape = makeCollisionShape(node);
		bhkRigidBodyRef body = makeCollisionBody(node);
		body->SetShape(DynamicCast<bhkShape>(shape));

		bhkCollisionObjectRef co = DynamicCast<bhkCollisionObject>(CreateBlock("bhkCollisionObject"));
		co->SetBody(DynamicCast<NiObject>(body));
		co->SetParent(newParent);

		// link
		newParent->SetCollisionObject(DynamicCast<NiCollisionObject>(co));

	} else
	if (node->IsGroupHead())
	{
		newParent = makeNode(parent, node);
	} else
		newParent = parent;

	for (int i=0; i<node->NumberOfChildren(); i++) 
	{
		Result result = exportCollision(newParent, node->GetChildNode(i));
		if (result!=Ok && result!=Skip)
			return result;
	}

	return Ok;
}


bhkRigidBodyRef Exporter::makeCollisionBody(INode *node)
{
	// get data from node
	int lyr, mtl, msys, qtype;
	float mass, lindamp, angdamp, frict, maxlinvel, maxangvel, resti, pendepth;
	Vector3 center;

	npGetProp(node, NP_HVK_LAYER, lyr, NP_DEFAULT_HVK_LAYER);
	npGetProp(node, NP_HVK_MATERIAL, mtl, NP_DEFAULT_HVK_MATERIAL);
	npGetProp(node, NP_HVK_MOTION_SYSTEM, msys, NP_DEFAULT_HVK_MOTION_SYSTEM);
	npGetProp(node, NP_HVK_QUALITY_TYPE, qtype, NP_DEFAULT_HVK_QUALITY_TYPE);
	npGetProp(node, NP_HVK_MASS, mass, NP_DEFAULT_HVK_MASS);
	npGetProp(node, NP_HVK_LINEAR_DAMPING, lindamp, NP_DEFAULT_HVK_LINEAR_DAMPING);
	npGetProp(node, NP_HVK_ANGULAR_DAMPING, angdamp, NP_DEFAULT_HVK_ANGULAR_DAMPING);
	npGetProp(node, NP_HVK_FRICTION, frict, NP_DEFAULT_HVK_FRICTION);
	npGetProp(node, NP_HVK_RESTITUTION, resti, NP_DEFAULT_HVK_RESTITUTION);
	npGetProp(node, NP_HVK_MAX_LINEAR_VELOCITY, maxlinvel, NP_DEFAULT_HVK_MAX_LINEAR_VELOCITY);
	npGetProp(node, NP_HVK_MAX_ANGULAR_VELOCITY, maxangvel, NP_DEFAULT_HVK_MAX_ANGULAR_VELOCITY);
	npGetProp(node, NP_HVK_PENETRATION_DEPTH, pendepth, NP_DEFAULT_HVK_PENETRATION_DEPTH);
	npGetProp(node, NP_HVK_CENTER, center);

	// setup body
	bhkRigidBodyRef body = DynamicCast<bhkRigidBody>(CreateBlock("bhkRigidBody"));

	body->SetLayer(lyr);
	body->SetLayerCopy(lyr);
	body->SetMotionSystem(msys);
	body->SetQualityType(qtype);
	body->SetMass(mass);
	body->SetLinearDamping(lindamp);
	body->SetAngularDamping(angdamp);
	body->SetFriction(frict);
	body->SetRestitution(resti);
	body->SetMaxLinearVelocity(maxlinvel);
	body->SetMaxAngularVelocity(maxangvel);
	body->SetPenetrationDepth(pendepth);
	body->SetCenter(center);

	return body;
}

bhkSphereRepShapeRef Exporter::makeCollisionShape(INode *node)
{
	bhkSphereRepShapeRef shape;
	
	TimeValue t = 0;
	ObjectState os = node->EvalWorldState(t); 
	if (os.obj->ClassID() == SCUBA_CLASS_ID)
		shape = makeCapsuleShape(os.obj);
	else
	if (os.obj->ClassID() == Class_ID(BOXOBJ_CLASS_ID, 0))
		shape = makeBoxShape(os.obj);
	else
	if (os.obj->ClassID() == Class_ID(SPHERE_CLASS_ID, 0))
		shape = makeSphereShape(os.obj);
	else
	if (os.obj->SuperClassID() == GEOMOBJECT_CLASS_ID)
		shape = makeTriStripsShape(node);

	if (shape)
	{
		int mtl;
		npGetProp(node, NP_HVK_MATERIAL, mtl, NP_DEFAULT_HVK_MATERIAL);
		shape->SetMaterial(mtl);
	}

	return shape;
}

bhkSphereRepShapeRef Exporter::makeBoxShape(Object *obj)
{
	float length = 0;
	float height = 0;
	float width = 0; 
	IParamArray *params = obj->GetParamBlock();
	params->GetValue(obj->GetParamBlockIndex(BOXOBJ_LENGTH), 0, length, FOREVER);
	params->GetValue(obj->GetParamBlockIndex(BOXOBJ_HEIGHT), 0, height, FOREVER);
	params->GetValue(obj->GetParamBlockIndex(BOXOBJ_WIDTH), 0, width, FOREVER);

	bhkBoxShapeRef box = DynamicCast<bhkBoxShape>(CreateBlock("bhkBoxShape"));
	box->SetDimensions(Vector3(width, height, length));

	return bhkSphereRepShapeRef(DynamicCast<bhkSphereRepShape>(box));
}

bhkSphereRepShapeRef Exporter::makeSphereShape(Object *obj)
{
	float radius = 0;
	IParamArray *params = obj->GetParamBlock();
	params->GetValue(obj->GetParamBlockIndex(SPHERE_RADIUS), 0, radius, FOREVER);

	bhkSphereShapeRef sphere = DynamicCast<bhkSphereShape>(CreateBlock("bhkSphereShape"));
	sphere->SetRadius(radius);

	return bhkSphereRepShapeRef(DynamicCast<bhkSphereRepShape>(sphere));
}

bhkSphereRepShapeRef Exporter::makeCapsuleShape(Object *obj)
{
	float radius = 0;
	float height = 0;
	IParamArray *params = obj->GetParamBlock();
	params->GetValue(obj->GetParamBlockIndex(CAPSULE_RADIUS), 0, radius, FOREVER);
	params->GetValue(obj->GetParamBlockIndex(CAPSULE_HEIGHT), 0, height, FOREVER);

	bhkCapsuleShapeRef capsule = DynamicCast<bhkCapsuleShape>(CreateBlock("bhkCapsuleShape"));

	return bhkSphereRepShapeRef(DynamicCast<bhkSphereRepShape>(capsule));
}

bhkSphereRepShapeRef Exporter::makeTriStripsShape(INode *node)
{
	TimeValue t = 0;
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

	TriObject *tri = (TriObject *)os.obj->ConvertToType(t, Class_ID(TRIOBJ_CLASS_ID, 0));
	if (!tri)
		return false;

	Mesh *mesh = &tri->GetMesh();
	mesh->buildNormals();

	// setup shape data
	vector<Vector3> verts;
	vector<Vector3> vnorms;
	Triangles		tris;

	for (int i=0; i<mesh->getNumFaces(); i++)
		addFace(tris, verts, vnorms, i, vi, mesh);

	TriStrips strips;
	strippify(strips, verts, vnorms, tris);
	NiTriStripsDataRef data = makeTriStripsData(strips);
	data->SetVertices(verts);
	data->SetNormals(vnorms);

	// setup shape
	bhkNiTriStripsShapeRef shape = DynamicCast<bhkNiTriStripsShape>(CreateBlock("bhkNiTriStripsShape"));
	shape->SetNumStripsData(1);
	shape->SetStripsData(0, data);
/*
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
*/
/*	array<uint, 5> unknownInts1;
	unknownInts1[4] = 1;
	shape->SetUnknownInts1(unknownInts1);
*/
	vector<uint> unknownInts2;
	unknownInts2.resize(1);
	shape->SetUnknownInts2(unknownInts2);

	if (tri != os.obj)
		tri->DeleteMe();

	return bhkSphereRepShapeRef(DynamicCast<bhkSphereRepShape>(shape));
}
