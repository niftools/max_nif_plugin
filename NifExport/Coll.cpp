#include "pch.h"
#include "../NifProps/bhkRigidBodyInterface.h"
#include "obj/bhkListShape.h"
#ifdef _DEBUG
#include <assert.h>
#include <crtdbg.h>
#define ASSERT _ASSERTE
#else
#define ASSERT(x)
#endif

static Class_ID SCUBA_CLASS_ID(0x6d3d77ac, 0x79c939a9);
static Class_ID BHKRIGIDBODYMODIFIER_CLASS_ID(0x398fd801, 0x303e44e5);
extern Class_ID BHKLISTOBJECT_CLASS_ID;

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
			 int face, const int vi[3], Mesh *mesh, Matrix3& tm)
{
	Triangle tri;
	for (int i=0; i<3; i++)
	{
      Point3 pt = VectorTransform(mesh->verts[ mesh->faces[ face ].v[ vi[i] ] ], tm);
      Point3 norm = VectorTransform(getVertexNormal(mesh, face, mesh->getRVertPtr(mesh->faces[ face ].v[ vi[i] ])), tm);
		tri[i] = addVertex(verts, vnorms, pt, norm);
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

	//array<float, 2> unknownFloats1;
	//uint i1 = 0x3DCCCCCD;
	//uint i2 = 0x004ABE60;
	//unknownFloats1[0] = *((float*)&i1);
	//unknownFloats1[1] = *((float*)&i2);
	//shape->SetUnknownFloats1(unknownFloats1);

	//array<float, 3> unknownFloats2;
	//unknownFloats2[0] = 1;
	//unknownFloats2[1] = 1;
	//unknownFloats2[2] = 1;
	//shape->SetUnknownFloats2(unknownFloats2);

	//array<uint, 5> unknownInts1;
	//unknownInts1[4] = 1;
	//shape->SetUnknownInts1(unknownInts1);

	//vector<uint> unknownInts3;
	//unknownInts3.resize(1);
	//shape->SetUnknownInts3(unknownInts3);

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
	if (isHandled(node))
		return Exporter::Skip;

   ProgressUpdate(Collision, FormatText("'%s' Collision", node->GetName()));

	// marked as collision?
	//bool coll = npIsCollision(node);
   bool coll = isCollision(node);

   bool local = !mFlattenHierarchy;
   NiNodeRef nodeParent = mFlattenHierarchy ? mNiRoot : parent;

	NiNodeRef newParent;
	if (coll)
	{
		markAsHandled(node);

		newParent = nodeParent; // always have collision one level up?

		TimeValue t = 0;
		Matrix3 tm = getTransform(node, t, local);

		bhkRigidBodyRef body = makeCollisionBody(node);
		bhkShapeRef shape = makeCollisionShape(node, tm, body);
		if (shape)
		{
			body->SetShape(DynamicCast<bhkShape>(shape));

			Matrix44 rm4 = TOMATRIX4(tm, false);
			Vector3 trans; Matrix33 rm; float scale;
			rm4.Decompose(trans, rm, scale);

			QuaternionXYZW q = TOQUATXYZW(rm.AsQuaternion());
			body->SetRotation(q);
			body->SetTranslation(trans / Exporter::bhkScaleFactor);

			bhkCollisionObjectRef co = new bhkCollisionObject();
			co->SetBody(DynamicCast<NiObject>(body));

			//co->SetTarget(newParent);

			// link
			newParent->SetCollisionObject(DynamicCast<NiCollisionObject>(co));
		}
	} else if (isCollisionGroup(node) && !mFlattenHierarchy) {
		newParent = makeNode(nodeParent, node);
   } else {
		newParent = nodeParent;
   }
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
	int lyr = NP_DEFAULT_HVK_LAYER;
	int mtl = NP_DEFAULT_HVK_MATERIAL;
	int msys  = NP_DEFAULT_HVK_MOTION_SYSTEM;
	int qtype = NP_DEFAULT_HVK_QUALITY_TYPE;
	float mass = NP_DEFAULT_HVK_MASS;
	float lindamp = NP_DEFAULT_HVK_LINEAR_DAMPING;
	float angdamp = NP_DEFAULT_HVK_ANGULAR_DAMPING;
	float frict = NP_DEFAULT_HVK_FRICTION;
	float maxlinvel = NP_DEFAULT_HVK_MAX_LINEAR_VELOCITY;
	float maxangvel = NP_DEFAULT_HVK_MAX_ANGULAR_VELOCITY;
	float resti = NP_DEFAULT_HVK_RESTITUTION;
	float pendepth = NP_DEFAULT_HVK_PENETRATION_DEPTH;
	Vector3 center(0,0,0);

	if (bhkRigidBodyInterface *irb = (bhkRigidBodyInterface *)node->GetObjectRef()->GetInterface(BHKRIGIDBODYINTERFACE_DESC))
	{
		mass = irb->GetMass(0);
		frict = irb->GetFriction(0);
		resti = irb->GetRestitution(0);
		lyr = irb->GetLayer(0);
		msys = irb->GetMotionSystem(0);
		qtype = irb->GetQualityType(0);
		lindamp = irb->GetLinearDamping(0);
		angdamp = irb->GetAngularDamping(0);
		maxlinvel = irb->GetMaxLinearVelocity(0);
		pendepth = irb->GetPenetrationDepth(0);
		maxangvel = irb->GetMaxAngularVelocity(0);
	}
	else if (npIsCollision(node))
	{
		// Handle compatibility
		npGetProp(node, NP_HVK_MASS_OLD, mass, NP_DEFAULT_HVK_EMPTY);
		if (mass == NP_DEFAULT_HVK_EMPTY)
			npGetProp(node, NP_HVK_MASS, mass, NP_DEFAULT_HVK_MASS);
		npGetProp(node, NP_HVK_FRICTION_OLD, frict, NP_DEFAULT_HVK_EMPTY);
		if (frict == NP_DEFAULT_HVK_EMPTY)
			npGetProp(node, NP_HVK_FRICTION, frict, NP_DEFAULT_HVK_FRICTION);
		npGetProp(node, NP_HVK_RESTITUTION_OLD, resti, NP_DEFAULT_HVK_EMPTY);
		if (resti == NP_DEFAULT_HVK_EMPTY)
			npGetProp(node, NP_HVK_RESTITUTION, resti, NP_DEFAULT_HVK_RESTITUTION);

		npGetProp(node, NP_HVK_LAYER, lyr, NP_DEFAULT_HVK_LAYER);
		npGetProp(node, NP_HVK_MATERIAL, mtl, NP_DEFAULT_HVK_MATERIAL);
		npGetProp(node, NP_HVK_MOTION_SYSTEM, msys, NP_DEFAULT_HVK_MOTION_SYSTEM);
		npGetProp(node, NP_HVK_QUALITY_TYPE, qtype, NP_DEFAULT_HVK_QUALITY_TYPE);
		npGetProp(node, NP_HVK_LINEAR_DAMPING, lindamp, NP_DEFAULT_HVK_LINEAR_DAMPING);
		npGetProp(node, NP_HVK_ANGULAR_DAMPING, angdamp, NP_DEFAULT_HVK_ANGULAR_DAMPING);
		npGetProp(node, NP_HVK_MAX_LINEAR_VELOCITY, maxlinvel, NP_DEFAULT_HVK_MAX_LINEAR_VELOCITY);
		npGetProp(node, NP_HVK_MAX_ANGULAR_VELOCITY, maxangvel, NP_DEFAULT_HVK_MAX_ANGULAR_VELOCITY);
		npGetProp(node, NP_HVK_PENETRATION_DEPTH, pendepth, NP_DEFAULT_HVK_PENETRATION_DEPTH);
		npGetProp(node, NP_HVK_CENTER, center);
	}
	else
	{
		// Check self to see if is one of our bhkXXXObject classes
		if (Object* obj = node->GetObjectRef())
		{
			if (obj->SuperClassID() == HELPER_CLASS_ID &&
				obj->ClassID().PartB() == BHKRIGIDBODYCLASS_DESC.PartB()) 
			{
				// TODO: do standard body export
			}
		}

		// else check redirection 
	}


	// setup body
	bhkRigidBodyRef body = CreateNiObject<bhkRigidBodyT>();

	body->SetLayer(OblivionLayer(lyr));
	body->SetLayerCopy(OblivionLayer(lyr));
	body->SetMotionSystem(MotionSystem(msys));
	body->SetQualityType(MotionQuality(qtype));
	body->SetMass(mass);
	body->SetLinearDamping(lindamp);
	body->SetAngularDamping(angdamp);
	body->SetFriction(frict);
	body->SetRestitution(resti);
	body->SetMaxLinearVelocity(maxlinvel);
	body->SetMaxAngularVelocity(maxangvel);
	body->SetPenetrationDepth(pendepth);
	body->SetCenter(center);
   QuaternionXYZW q; q.x = q.y = q.z = 0; q.w = 1.0f;
   body->SetRotation(q);

	return body;
}

bhkShapeRef Exporter::makeCollisionShape(INode *node, Matrix3& tm, bhkRigidBodyRef body)
{
	bhkShapeRef shape;
	
	TimeValue t = 0;
	ObjectState os = node->EvalWorldState(t); 
	if (os.obj->ClassID() == SCUBA_CLASS_ID)
		shape = makeCapsuleShape(node, os.obj, tm);
	else if (os.obj->ClassID() == Class_ID(BOXOBJ_CLASS_ID, 0))
		shape = makeBoxShape(node, os.obj, tm);
	else if (os.obj->ClassID() == Class_ID(SPHERE_CLASS_ID, 0))
		shape = makeSphereShape(node, os.obj, tm);
	else if (os.obj->SuperClassID() == GEOMOBJECT_CLASS_ID)
		shape = makeTriStripsShape(node, tm);
	else if (os.obj->ClassID() == BHKLISTOBJECT_CLASS_ID)
		shape = makeListShape(node, tm, body);
	return shape;
}

bhkShapeRef Exporter::makeBoxShape(INode *node, Object *obj, Matrix3& tm)
{
   Point3 scale = GetScale(tm);
	float length = 0;
	float height = 0;
	float width = 0; 
	IParamArray *params = obj->GetParamBlock();
	params->GetValue(obj->GetParamBlockIndex(BOXOBJ_LENGTH), 0, length, FOREVER);
	params->GetValue(obj->GetParamBlockIndex(BOXOBJ_HEIGHT), 0, height, FOREVER);
	params->GetValue(obj->GetParamBlockIndex(BOXOBJ_WIDTH), 0, width, FOREVER);

	bhkBoxShapeRef box = new bhkBoxShape();
	Vector3 dim(width * scale[0], length * scale[1], height * scale[2]);

	// Adjust translation for center of z axis in box
	tm.Translate(Point3(0.0, 0.0, dim.z / 2.0));

	dim /= (Exporter::bhkScaleFactor * 2);
	box->SetDimensions(dim);

	int mtl = 0;
	npGetProp(node, NP_HVK_MATERIAL, mtl, NP_DEFAULT_HVK_MATERIAL);
	box->SetMaterial(HavokMaterial(mtl));

	return bhkShapeRef(DynamicCast<bhkSphereRepShape>(box));
}

bhkShapeRef Exporter::makeSphereShape(INode *node, Object *obj, Matrix3& tm)
{
   Point3 scale = GetScale(tm);
   float s = (scale[0] + scale[1] + scale[2]) / 3.0;

	float radius = 0;
	IParamArray *params = obj->GetParamBlock();
	params->GetValue(obj->GetParamBlockIndex(SPHERE_RADIUS), 0, radius, FOREVER);

	bhkSphereShapeRef sphere = new bhkSphereShape();
	sphere->SetRadius(radius * s);

	int mtl = 0;
	npGetProp(node, NP_HVK_MATERIAL, mtl, NP_DEFAULT_HVK_MATERIAL);
	sphere->SetMaterial(HavokMaterial(mtl));

	return bhkShapeRef(DynamicCast<bhkSphereRepShape>(sphere));
}

bhkShapeRef Exporter::makeCapsuleShape(INode *node, Object *obj, Matrix3& tm)
{
   Point3 scale = GetScale(tm);
   float s = (scale[0] + scale[1] + scale[2]) / 3.0;

	float radius = 0;
	float height = 0;
	IParamArray *params = obj->GetParamBlock();
	params->GetValue(obj->GetParamBlockIndex(CAPSULE_RADIUS), 0, radius, FOREVER);
	params->GetValue(obj->GetParamBlockIndex(CAPSULE_HEIGHT), 0, height, FOREVER);

	bhkCapsuleShapeRef capsule = CreateNiObject<bhkCapsuleShape>();

	capsule->SetRadius(radius);
	capsule->SetRadius1(radius);
	capsule->SetRadius2(radius);

	int mtl = 0;
	npGetProp(node, NP_HVK_MATERIAL, mtl, NP_DEFAULT_HVK_MATERIAL);
	capsule->SetMaterial(HavokMaterial(mtl));

	return bhkShapeRef(DynamicCast<bhkSphereRepShape>(capsule));
}

bhkShapeRef Exporter::makeTriStripsShape(INode *node, Matrix3& tm)
{
	TimeValue t = 0;
   Matrix3 sm = ScaleMatrix( GetScale(tm) );
   
	//Matrix3 tm = node->GetObjTMAfterWSM(t);

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
		addFace(tris, verts, vnorms, i, vi, mesh, sm);

	//TriStrips strips;
	//strippify(strips, verts, vnorms, tris);
	//NiTriStripsDataRef data = makeTriStripsData(strips);
	NiTriStripsDataRef data = new NiTriStripsData(tris, Exporter::mUseAlternateStripper);
	data->SetVertices(verts);
	data->SetNormals(vnorms);

	int lyr = OL_STATIC;
	npGetProp(node, NP_HVK_LAYER, lyr, NP_DEFAULT_HVK_LAYER);

	// setup shape
	bhkNiTriStripsShapeRef shape = StaticCast<bhkNiTriStripsShape>(bhkNiTriStripsShape::Create());
	shape->SetNumStripsData(1);
	shape->SetStripsData(0, data);
	shape->SetNumDataLayers(1);
	shape->SetOblivionLayer(0, OblivionLayer(lyr));

	int mtl;
	npGetProp(node, NP_HVK_MATERIAL, mtl, NP_DEFAULT_HVK_MATERIAL);
	shape->SetMaterial(HavokMaterial(mtl));

	//if (tri != os.obj)
	//	tri->DeleteMe();
	return StaticCast<bhkShape>(shape);
}

Exporter::Result Exporter::scanForCollision(INode *node)
{   
   if (NULL == node) 
      return Exporter::Skip;
   // Get the bhk RigidBody modifier if available and then get the picked node.
   if (Modifier * mod = GetbhkCollisionModifier(node)){
      if (IParamBlock2* pblock = (IParamBlock2*)mod->GetReference(0)) {
         if (INode *collMesh = pblock->GetINode(0, 0)) {
            mCollisionNodes.insert(collMesh);
         } else {
            if (mSceneCollisionNode != NULL) {
               if (mExportCollision) {
                  throw runtime_error("There are more than one Collision mesh found at the Scene Level.");
               }
            } else {
               mSceneCollisionNode = node;
            }
         }
      }
   }
   // Check self to see if is one of our bhkXXXObject classes
   if (Object* obj = node->GetObjectRef())
   {
	   if (obj->ClassID() == BHKLISTOBJECT_CLASS_ID)
	   {
		   mCollisionNodes.insert(node);

		   const int PB_MESHLIST = 1;
		   IParamBlock2* pblock2 = obj->GetParamBlockByID(0);
		   int nBlocks = pblock2->Count(PB_MESHLIST);
		   for (int i = 0;i < pblock2->Count(PB_MESHLIST); i++) {
			   INode *tnode = NULL;
			   pblock2->GetValue(PB_MESHLIST,0,tnode,FOREVER,i);	
			   if (tnode != NULL) {
				   mCollisionNodes.insert(tnode);
				   markAsHandled(tnode); // dont process collision since the list will 
			   }
		   }
	   }
	   else if (obj->SuperClassID() == HELPER_CLASS_ID &&
		   obj->ClassID().PartB() == BHKRIGIDBODYCLASS_DESC.PartB()) 
	   {
		   mCollisionNodes.insert(node);
	   }
   }
   if (npIsCollision(node))
   {
	   mCollisionNodes.insert(node);
   }

   for (int i=0; i<node->NumberOfChildren(); i++) {
      scanForCollision(node->GetChildNode(i));
   }
   return Exporter::Ok;
}

bool Exporter::isHandled(INode *node)
{
	return (mHandledNodes.find(node) != mHandledNodes.end());
}

bool Exporter::markAsHandled(INode* node)
{
	mHandledNodes.insert(node);
	return true;
}

bool Exporter::isCollision(INode *node)
{
	return (mCollisionNodes.find(node) != mCollisionNodes.end());
}

bhkShapeRef Exporter::makeListShape(INode *node, Matrix3& tm, bhkRigidBodyRef body)
{
	const int PB_MATERIAL = 0;
	const int PB_MESHLIST = 1;
	IParamBlock2* pblock2 = node->GetObjectRef()->GetParamBlockByID(0);
	int nBlocks = pblock2->Count(PB_MESHLIST);
	if (nBlocks > 0)
	{
		if (bhkRigidBodyInterface *irb = (bhkRigidBodyInterface *)node->GetObjectRef()->GetInterface(BHKRIGIDBODYINTERFACE_DESC))
		{
			int mass = irb->GetMass(0);
			float frict = irb->GetFriction(0);
			float resti = irb->GetRestitution(0);
			int lyr = irb->GetLayer(0);
			int msys = irb->GetMotionSystem(0);
			int qtype = irb->GetQualityType(0);
			float lindamp = irb->GetLinearDamping(0);
			float angdamp = irb->GetAngularDamping(0);
			float maxlinvel = irb->GetMaxLinearVelocity(0);
			float maxangvel = irb->GetMaxAngularVelocity(0);
			float pendepth = irb->GetPenetrationDepth(0);

			body->SetLayer(OblivionLayer(lyr));
			body->SetLayerCopy(OblivionLayer(lyr));
			body->SetMotionSystem(MotionSystem(msys));
			body->SetQualityType(MotionQuality(qtype));
			body->SetMass(mass);
			body->SetLinearDamping(lindamp);
			body->SetAngularDamping(angdamp);
			body->SetFriction(frict);
			body->SetRestitution(resti);
			body->SetMaxLinearVelocity(maxlinvel);
			body->SetMaxAngularVelocity(maxangvel);
			body->SetPenetrationDepth(pendepth);
		}

		bhkListShapeRef shape = new bhkListShape();

		int mtl = pblock2->GetInt(PB_MATERIAL, 0, 0);
		shape->SetMaterial(HavokMaterial(mtl));

		vector<bhkShapeRef> shapes;

		for (int i = 0; i < nBlocks; i++) {
			INode *tnode = NULL;
			pblock2->GetValue(PB_MESHLIST,0,tnode,FOREVER,i);	
			if (tnode != NULL)
			{
				bhkShapeRef subshape = makeCollisionShape(tnode, tm, body);
				if (subshape)
					shapes.push_back(subshape);
			}
		}
		shape->SetSubShapes(shapes);

		if (shapes.size() == 1) // ignore the list when only one object is present
		{
			return shapes[0];
		}
		else if (!shapes.empty())
		{
			return bhkShapeRef(shape);
		}
	}
	return bhkShapeRef();
}