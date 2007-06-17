#include "pch.h"
#include "../NifProps/bhkRigidBodyInterface.h"
#include "obj/bhkListShape.h"
#include "obj/bhkConvexVerticesShape.h"
#include "..\NifProps\bhkHelperFuncs.h"
#include "..\NifProps\bhkHelperInterface.h"

#ifdef _DEBUG
#include <assert.h>
#include <crtdbg.h>
#define ASSERT _ASSERTE
#else
#define ASSERT(x)
#endif

static Class_ID SCUBA_CLASS_ID(0x6d3d77ac, 0x79c939a9);
extern Class_ID BHKRIGIDBODYMODIFIER_CLASS_ID;
extern Class_ID BHKLISTOBJECT_CLASS_ID;
extern Class_ID bhkBoxObject_CLASS_ID;
extern Class_ID BHKCAPSULEOBJECT_CLASS_ID;
extern Class_ID bhkSphereObject_CLASS_ID;
extern Class_ID BHKPROXYOBJECT_CLASS_ID;

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

		newParent = Exporter::findNode(node->GetParentNode());
		if (!newParent)
			newParent = nodeParent;

		//newParent = nodeParent; // always have collision one level up?

		TimeValue t = 0;
		Matrix3 tm = getTransform(node, t, local);

		bhkRigidBodyRef body = makeCollisionBody(node);
		if (body)
		{
			Matrix44 rm4 = TOMATRIX4(tm, false);
			Vector3 trans; Matrix33 rm; float scale;
			rm4.Decompose(trans, rm, scale);

			QuaternionXYZW q = TOQUATXYZW(rm.AsQuaternion());
			body->SetRotation(q);
			body->SetTranslation(trans / Exporter::bhkScaleFactor);

			bhkShapeRef shape = makeCollisionShape(node, tm, body);
			if (shape)
			{
				body->SetShape(DynamicCast<bhkShape>(shape));

				bhkCollisionObjectRef co = new bhkCollisionObject();
				co->SetBody(DynamicCast<NiObject>(body));

				//co->SetTarget(newParent);

				// link
				newParent->SetCollisionObject(DynamicCast<NiCollisionObject>(co));
			}
		}
	} else if (isCollisionGroup(node) && !mFlattenHierarchy) {
		newParent = makeNode(nodeParent, node);
   } else {
		newParent = Exporter::findNode(node->GetParentNode());
		if (!newParent)
			newParent = nodeParent;
   }
	if (!newParent)
		newParent = nodeParent;

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

bhkNiTriStripsShapeRef Exporter::makeTriStripsShape(Mesh& mesh, Matrix3& sm)
{
	typedef vector<Triangle> Triangles;

	// setup shape data
	vector<Vector3> verts;
	vector<Vector3> vnorms;
	Triangles		tris;

	int vi[3];
	if (TMNegParity(sm)) {
		vi[0] = 2; vi[1] = 1; vi[2] = 0;
	} else {
		vi[0] = 0; vi[1] = 1; vi[2] = 2;
	}

	for (int i=0; i<mesh.getNumFaces(); i++)
		addFace(tris, verts, vnorms, i, vi, &mesh, sm);

	NiTriStripsDataRef data = new NiTriStripsData(tris, Exporter::mUseAlternateStripper);
	data->SetVertices(verts);
	data->SetNormals(vnorms);

	//int lyr = OL_STATIC;
	//npGetProp(node, NP_HVK_LAYER, lyr, NP_DEFAULT_HVK_LAYER);

	//int mtl;
	//npGetProp(node, NP_HVK_MATERIAL, mtl, NP_DEFAULT_HVK_MATERIAL);
	//shape->SetMaterial(HavokMaterial(mtl));

	// setup shape
	bhkNiTriStripsShapeRef shape = StaticCast<bhkNiTriStripsShape>(bhkNiTriStripsShape::Create());
	shape->SetNumStripsData(1);
	shape->SetStripsData(0, data);
	shape->SetNumDataLayers(1);
	shape->SetOblivionLayer(0, OL_STATIC);

	//if (tri != os.obj)
	//	tri->DeleteMe();
	return shape;
}

bhkConvexVerticesShapeRef Exporter::makeConvexShape(Mesh& mesh, Matrix3& tm)
{
	bhkConvexVerticesShapeRef shape = StaticCast<bhkConvexVerticesShape>(bhkConvexVerticesShape::Create());
	Point3 center(0.0f, 0.0f, 0.0f);
	float radius = 0.10f;
	//CalcAxisAlignedSphere(mesh, center, radius);
	shape->SetRadius(radius);
	vector<Vector3> verts, norms;
	vector<float> dist;
	int nvert = mesh.getNumVerts();
	int nface = mesh.getNumFaces();

	verts.resize(nvert);
	norms.resize(nface);
	dist.resize(nface);
	for (int i=0; i<nvert; ++i)
	{
		Point3 vert = mesh.getVert(i) / Exporter::bhkScaleFactor;
		verts[i] = TOVECTOR3(vert);
	}
	for (int i=0; i<nface; ++i)
	{
		norms[i] = TOVECTOR3(mesh.getFaceNormal(i));
		dist[i] = -mesh.FaceCenter(i).Length();
	}
	shape->SetVertices(verts);
	shape->SetNormals(norms);
	shape->SetDistToCenter(dist);
	return shape;
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
	else if (os.obj->ClassID() == bhkBoxObject_CLASS_ID)
		shape = makebhkBoxShape(node, os.obj, tm);
	else if (os.obj->ClassID() == bhkSphereObject_CLASS_ID)
		shape = makebhkSphereShape(node, os.obj, tm);
	else if (os.obj->ClassID() == BHKCAPSULEOBJECT_CLASS_ID)
		shape = makebhkCapsuleShape(node, os.obj, tm);
	else if (os.obj->ClassID() == BHKLISTOBJECT_CLASS_ID)
		shape = makeListShape(node, tm, body);
	else if (os.obj->ClassID() == BHKPROXYOBJECT_CLASS_ID)
		shape = makeProxyShape(node, os.obj, tm);
	else if (os.obj->SuperClassID() == GEOMOBJECT_CLASS_ID)
	{
		if (Modifier* mod = GetbhkCollisionModifier(node))
		{
			shape = makeModifierShape(node, os.obj, mod, tm);
		}
		else
		{
			shape = makeTriStripsShape(node, tm);
		}
	}
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

	float radius = 0.1f;
	float height = 0.1f;
	if (IParamBlock2* params = obj->GetParamBlockByID(0))
	{
		params->GetValue(CAPSULE_RADIUS, 0, radius, FOREVER);
		params->GetValue(CAPSULE_HEIGHT, 0, height, FOREVER);
	}

	bhkCapsuleShapeRef capsule = CreateNiObject<bhkCapsuleShape>();

	capsule->SetRadius(radius);
	capsule->SetRadius1(radius);
	capsule->SetRadius2(radius);

	int mtl = 0;
	npGetProp(node, NP_HVK_MATERIAL, mtl, NP_DEFAULT_HVK_MATERIAL);
	capsule->SetMaterial(HavokMaterial(mtl));

	return bhkShapeRef(DynamicCast<bhkSphereRepShape>(capsule));
}

bhkShapeRef Exporter::makebhkBoxShape(INode *node, Object *obj, Matrix3& tm)
{
	enum { box_params, };
	enum { PB_MATERIAL, PB_LENGTH, PB_WIDTH, PB_HEIGHT, };

	bhkShapeRef retval;
	if (IParamBlock2* pblock2 = obj->GetParamBlockByID(box_params))
	{
		Point3 scale = GetScale(tm);
		float s = (scale[0] + scale[1] + scale[2]) / 3.0;

		int mtl = 0, length = 0, width = 0, height = 0;
		pblock2->GetValue(PB_MATERIAL, 0, mtl, FOREVER, 0);
		pblock2->GetValue(PB_LENGTH, 0, length, FOREVER, 0);
		pblock2->GetValue(PB_WIDTH, 0, width, FOREVER, 0);
		pblock2->GetValue(PB_HEIGHT, 0, height, FOREVER, 0);


		bhkBoxShapeRef box = new bhkBoxShape();
		Vector3 dim(width * scale[0], length * scale[1], height * scale[2]);

		// Adjust translation for center of z axis in box
		tm.Translate(Point3(0.0, 0.0, dim.z / 2.0));

		dim /= (Exporter::bhkScaleFactor * 2);
		box->SetDimensions(dim);

		box->SetMaterial(HavokMaterial(mtl));
		retval = StaticCast<bhkShape>(box);
	}

	return retval;
}

bhkShapeRef	Exporter::makebhkSphereShape(INode *node, Object *obj, Matrix3& tm)
{
	bhkShapeRef retval;

	return retval;
}

bhkShapeRef	Exporter::makebhkCapsuleShape(INode *node, Object *obj, Matrix3& tm)
{
	bhkShapeRef retval;

	return retval;
}


bhkShapeRef Exporter::makeTriStripsShape(INode *node, Matrix3& tm)
{
	TimeValue t = 0;
   Matrix3 sm = ScaleMatrix( GetScale(tm) );
   
	//Matrix3 tm = node->GetObjTMAfterWSM(t);

	// Order of the vertices. Get 'em counter clockwise if the objects is
	// negatively scaled.
	ObjectState os = node->EvalWorldState(t);

	TriObject *tri = (TriObject *)os.obj->ConvertToType(t, Class_ID(TRIOBJ_CLASS_ID, 0));
	if (!tri)
		return false;

	Mesh &mesh = tri->GetMesh();
	mesh.buildNormals();

	bhkNiTriStripsShapeRef shape = makeTriStripsShape(mesh, sm);

	int lyr = OL_STATIC;
	npGetProp(node, NP_HVK_LAYER, lyr, NP_DEFAULT_HVK_LAYER);
	shape->SetNumDataLayers(1);
	shape->SetOblivionLayer(0, OblivionLayer(lyr));

	int mtl;
	npGetProp(node, NP_HVK_MATERIAL, mtl, NP_DEFAULT_HVK_MATERIAL);
	shape->SetMaterial(HavokMaterial(mtl));

	return StaticCast<bhkShape>(shape);
}

bhkShapeRef	Exporter::makeConvexShape(INode *node, Object* obj, Matrix3& tm)
{
	bhkShapeRef shape;

	return shape;
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
	   else
	   {
		   Modifier* mod = GetbhkCollisionModifier(node);
		   if (mod != NULL)
		   {
			   mCollisionNodes.insert(node);
		   }
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
	// reset transform
	body->SetCenter(Vector3(0,0,0));
	body->SetTranslation(Vector3(0.0f,0.0f,0.0f));
	body->SetRotation(TOQUATXYZW(Quat(0.0f,0.0f,0.0f,1.0f)));

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

bhkShapeRef Exporter::makeProxyShape(INode *node, Object *obj, Matrix3& tm)
{
	enum { list_params, bv_mesh, };  // pblock2 ID
	enum { PB_MATERIAL, PB_MESHLIST, PB_BOUND_TYPE, PB_CENTER, };
	enum { bv_type_none, bv_type_box, bv_type_shapes, bv_type_packed, bv_type_convex, };  // pblock ID

	bhkShapeRef shape;
	if (IParamBlock2* pblock2 = obj->GetParamBlockByID(list_params))
	{
		int bvType = bv_type_none;
		pblock2->GetValue(PB_BOUND_TYPE, 0, bvType, FOREVER, 0);
		if (bvType != bv_type_none) 
		{
			if (TriObject *triObj = (TriObject *)obj->ConvertToType(0, triObjectClassID))
			{
				Mesh& mesh = triObj->GetMesh();
				mesh.buildNormals();

				switch (bvType)
				{
				case bv_type_box:
					break;

				case bv_type_shapes:
				case bv_type_packed:
					break;

				case bv_type_convex: 
					Matrix3 tm(true);
					if (bhkConvexVerticesShapeRef convShape = makeConvexShape(mesh, tm))
					{
						int mtl = pblock2->GetInt(PB_MATERIAL, 0, 0);
						convShape->SetMaterial(HavokMaterial(mtl));
						shape = StaticCast<bhkShape>(convShape);
					}
					break;
				}
			}
		}
	}
	return shape;
}

bhkShapeRef	Exporter::makeModifierShape(INode *node, Object* obj, Modifier* mod, Matrix3& tm)
{
	enum { havok_params };
	enum { PB_BOUND_TYPE, PB_MATERIAL, };
	enum { bv_type_none, bv_type_box, bv_type_sphere, bv_type_capsule, bv_type_shapes, bv_type_convex, };  // pblock ID

	bhkShapeRef shape;

	const Mesh* mesh = NULL;
	int material = NP_DEFAULT_HVK_MATERIAL;
	int type = bv_type_none;

	if (bhkHelperInterface* bhkHelp = (bhkHelperInterface*)mod->GetInterface(BHKHELPERINTERFACE_DESC))
	{
		mesh = bhkHelp->GetMesh();
	}
	else
	{
		if (TriObject *tri = (TriObject *)obj->ConvertToType(0, Class_ID(TRIOBJ_CLASS_ID, 0)))
		{
			mesh = &tri->GetMesh();
		}
	}
	if (IParamBlock2* pblock2 = mod->GetParamBlockByID(havok_params))
	{
		pblock2->GetValue(PB_BOUND_TYPE, 0, type, FOREVER, 0);
		pblock2->GetValue(PB_MATERIAL, 0, material, FOREVER, 0);
	}
	switch (type)
	{
	default:
	case bv_type_none:
		break;

	case bv_type_box:
		shape = makeBoxShape(node, obj, tm);
		break;

	case bv_type_sphere:
		shape = makeSphereShape(node, obj, tm);
		break;

	case bv_type_capsule:
		shape = makeCapsuleShape(node, obj, tm);
		break;

	case bv_type_shapes:
		if (bhkNiTriStripsShapeRef trishape = makeTriStripsShape(const_cast<Mesh&>(*mesh), tm))
		{
			trishape->SetMaterial(HavokMaterial(material));
			shape = StaticCast<bhkShape>(trishape);
		}
		break;

	case bv_type_convex:
		if (bhkConvexVerticesShapeRef convShape = makeConvexShape(const_cast<Mesh&>(*mesh), tm))
		{
			convShape->SetMaterial(HavokMaterial(material));
			shape = StaticCast<bhkShape>(convShape);
		}
		break;
	}
	return shape;
}