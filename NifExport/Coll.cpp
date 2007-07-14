#include "pch.h"
#include "../NifProps/bhkRigidBodyInterface.h"
#include "obj/bhkListShape.h"
#include "obj/bhkConvexVerticesShape.h"
#include "obj/bhkTransformShape.h"
#include "obj/bhkSphereShape.h"
#include "obj/bhkBoxShape.h"
#include "obj/bhkCapsuleShape.h"
#include "obj/hkPackedNiTriStripsData.h"
#include "obj/bhkPackedNiTriStripsShape.h"

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
      Point3 pt = mesh->verts[ mesh->faces[ face ].v[ vi[i] ] ] * tm;
      Point3 norm = getVertexNormal(mesh, face, mesh->getRVertPtr(mesh->faces[ face ].v[ vi[i] ])) * tm;
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
		if (!newParent) {
			newParent = nodeParent;
		}
		//newParent = nodeParent; // always have collision one level up?

		TimeValue t = 0;

		bhkRigidBodyRef body = makeCollisionBody(node);
		if (body)
		{
			bool hasTrans = (body->IsDerivedType(bhkRigidBodyT::TYPE));
			Matrix3 tm = getTransform(node, t, false); // has transform
			Matrix3 pm = TOMATRIX3(newParent->GetWorldTransform());
			tm = tm * Inverse(pm);

			Matrix44 rm4 = TOMATRIX4(tm, false);
			Vector3 trans; Matrix33 rm; float scale;
			rm4.Decompose(trans, rm, scale);
			QuaternionXYZW q = TOQUATXYZW(rm.AsQuaternion());
			body->SetRotation(q);
			body->SetTranslation(trans / Exporter::bhkScaleFactor);

			if (hasTrans) {
				tm = getTransform(node, t, false);
				tm.NoScale();
				tm.Invert();
			} else {
				tm = TOMATRIX3(newParent->GetWorldTransform());
				tm.NoScale();
				tm.Invert();
				//tm.IdentityMatrix();
            }

			bhkShapeRef shape = makeCollisionShape(node, tm, body);
			if (shape)
			{
				body->SetShape(DynamicCast<bhkShape>(shape));

				bhkCollisionObjectRef co = new bhkCollisionObject();
				co->SetBody(DynamicCast<NiObject>(body));
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
	BOOL transenable = TRUE;

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
		transenable = irb->GetEnableTransform(0);
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
	bhkRigidBodyRef body = transenable ? new bhkRigidBodyT() : new bhkRigidBody();
	
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

	// setup shape
	bhkNiTriStripsShapeRef shape = StaticCast<bhkNiTriStripsShape>(bhkNiTriStripsShape::Create());
	shape->SetNumStripsData(1);
	shape->SetStripsData(0, data);
	shape->SetNumDataLayers(1);
	shape->SetOblivionLayer(0, OL_STATIC);
	return shape;
}

bhkPackedNiTriStripsShapeRef Exporter::makePackedTriStripsShape(Mesh& mesh, Matrix3& sm)
{
	// Need to separate the vertices based on material.  
	typedef vector<Triangle> Triangles;

	// setup shape data
	vector<Vector3> verts;
	vector<Vector3> norms;
	Triangles		tris;

	int vi[3];
	if (TMNegParity(sm)) {
		vi[0] = 2; vi[1] = 1; vi[2] = 0;
	} else {
		vi[0] = 0; vi[1] = 1; vi[2] = 2;
	}

	int nvert = mesh.getNumVerts();
	int nface = mesh.getNumFaces();
	mesh.buildNormals();

	tris.resize(nface);
	verts.resize(nvert);
	norms.resize(nface);
	for (int i=0; i<nvert; ++i)
	{
		Point3 vert = (mesh.getVert(i) * sm) / Exporter::bhkScaleFactor;
		verts[i] = TOVECTOR3(vert);
	}
	for (int i=0; i<nface; ++i)
	{
		Triangle& tri = tris[i];
		norms[i] = TOVECTOR3(mesh.getFaceNormal(i));
		Face& face = mesh.faces[i];
		tri[0] = face.getVert(0);
		tri[1] = face.getVert(1);
		tri[2] = face.getVert(2);
	}

	hkPackedNiTriStripsDataRef data = new hkPackedNiTriStripsData();
	data->SetNumFaces( tris.size() );
	data->SetVertices(verts);
	data->SetTriangles(tris);
	data->SetNormals(norms);

	// setup shape
	bhkPackedNiTriStripsShapeRef shape = new bhkPackedNiTriStripsShape();
	shape->SetData(data);
	//shape->SetStripsData(0, data);
	//shape->SetNumDataLayers(1);
	//shape->SetOblivionLayer(0, OL_STATIC);
	return shape;
}

bhkConvexVerticesShapeRef Exporter::makeConvexShape(Mesh& mesh, Matrix3& tm)
{
	bhkConvexVerticesShapeRef shape = StaticCast<bhkConvexVerticesShape>(bhkConvexVerticesShape::Create());
	Point3 center(0.0f, 0.0f, 0.0f);
	float radius = 0.10f;
	//CalcAxisAlignedSphere(mesh, center, radius);
	shape->SetRadius(radius);
	vector<Vector3> verts;
	vector<Float4> norms;
	int nvert = mesh.getNumVerts();
	int nface = mesh.getNumFaces();
	mesh.checkNormals(FALSE);
	mesh.buildNormals();

	verts.resize(nvert);
	norms.resize(nface);
	for (int i=0; i<nvert; ++i)
	{
		Point3 vert = (mesh.getVert(i) * tm) / Exporter::bhkScaleFactor;
		verts[i] = TOVECTOR3(vert);
	}
	for (int i=0; i<nface; ++i)
	{
		Float4 &value = norms[i];
		Point3 &pt = mesh.getFaceNormal(i);
		value[0] = pt.x;
		value[1] = pt.y;
		value[2] = pt.z;
		value[3] = -mesh.FaceCenter(i).Length();
	}
	sortVector3(verts);
	sortFloat4(norms);
	shape->SetVertices(verts);
	shape->SetNormalsAndDist(norms);
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
	if (IParamBlock2* pblock2 = obj->GetParamBlockByID(0))
	{
		pblock2->GetValue(BOXOBJ_LENGTH, 0, length, FOREVER);
		pblock2->GetValue(BOXOBJ_HEIGHT, 0, height, FOREVER);
		pblock2->GetValue(BOXOBJ_WIDTH, 0, width, FOREVER);
	}

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
	if (IParamBlock2* pblock2 = obj->GetParamBlockByID(0))
	{
		pblock2->GetValue(SPHERE_RADIUS, 0, radius, FOREVER);
	}

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

		int mtl = 0;
		float length = 0, width = 0, height = 0;
		pblock2->GetValue(PB_MATERIAL, 0, mtl, FOREVER, 0);
		pblock2->GetValue(PB_LENGTH, 0, length, FOREVER, 0);
		pblock2->GetValue(PB_WIDTH, 0, width, FOREVER, 0);
		pblock2->GetValue(PB_HEIGHT, 0, height, FOREVER, 0);

		bhkBoxShapeRef box = new bhkBoxShape();

		Vector3 dim(width * scale[0], length * scale[1], height * scale[2]);

		// Adjust translation for center of z axis in box
		tm.Translate(Point3(0.0, 0.0, dim.z / 2.0));
		box->SetDimensions(dim);
		box->SetMaterial(HavokMaterial(mtl));

		Matrix3 ltm = node->GetNodeTM(0) * tm;
		if (ltm.IsIdentity())
		{
			retval = StaticCast<bhkShape>(box);
		}
		else
		{
			ltm.SetTrans(ltm.GetTrans() / Exporter::bhkScaleFactor);

			bhkTransformShapeRef transform = new bhkTransformShape();
			transform->SetTransform(TOMATRIX4(ltm).Transpose());
			transform->SetShape(box);
			transform->SetMaterial(HavokMaterial(mtl));
			retval = StaticCast<bhkShape>(transform);
		}
	}

	return retval;
}

bhkShapeRef	Exporter::makebhkSphereShape(INode *node, Object *obj, Matrix3& tm)
{
	bhkShapeRef retval;

	enum { sphere_params, };
	enum { PB_MATERIAL, PB_RADIUS, PB_SEGS, PB_SMOOTH, };

	if (IParamBlock2* pblock2 = obj->GetParamBlockByID(sphere_params))
	{
		float radius = 0.0f;
		int mtl = NP_DEFAULT_HVK_MATERIAL;
		pblock2->GetValue(PB_RADIUS, 0, radius, FOREVER, 0);
		pblock2->GetValue(PB_MATERIAL, 0, mtl, FOREVER, 0);

		bhkSphereShapeRef shape = new bhkSphereShape();
		shape->SetRadius(radius);
		shape->SetMaterial(HavokMaterial(mtl));

        Matrix3 ltm = node->GetNodeTM(0) * tm;
		if (ltm.IsIdentity())
		{
			retval = StaticCast<bhkShape>(shape);
		}
		else
		{
			ltm.SetTrans(ltm.GetTrans() / Exporter::bhkScaleFactor);

			bhkTransformShapeRef transform = new bhkTransformShape();
			transform->SetTransform(TOMATRIX4(ltm).Transpose());
			transform->SetShape(shape);
			transform->SetMaterial(HavokMaterial(mtl));
			retval = StaticCast<bhkShape>(transform);
		}
	}
	return retval;
}

bhkShapeRef	Exporter::makebhkCapsuleShape(INode *node, Object *obj, Matrix3& tm)
{
	bhkShapeRef retval;

	enum { cap_params, };
	enum { PB_MATERIAL, PB_RADIUS1, PB_RADIUS2, PB_LENGTH, };

	if (IParamBlock2* pblock2 = obj->GetParamBlockByID(cap_params))
	{
		float radius1 = 0.0f, radius2 = 0.0f, len = 0.0f;
		int mtl = NP_DEFAULT_HVK_MATERIAL;
		pblock2->GetValue(PB_RADIUS1, 0, radius1, FOREVER, 0);
		pblock2->GetValue(PB_RADIUS2, 0, radius2, FOREVER, 0);
		pblock2->GetValue(PB_LENGTH, 0, len, FOREVER, 0);
		pblock2->GetValue(PB_MATERIAL, 0, mtl, FOREVER, 0);

		bhkCapsuleShapeRef shape = new bhkCapsuleShape();
		shape->SetRadius((radius1 + radius2)/2.0f);
		shape->SetRadius1(radius1);
		shape->SetRadius2(radius2);
		shape->SetMaterial(HavokMaterial(mtl));

        Matrix3 ltm = node->GetNodeTM(0) * tm;
		Point3 center = ltm.GetTrans();

		Matrix3 rot = ltm;
		rot.NoTrans();
		rot.NoScale();

		float distFromCenter = len*Exporter::bhkScaleFactor/2.0f;

		Point3 pt1 = ((TransMatrix(Point3(0.0f, 0.0f, +distFromCenter)) * rot).GetTrans() + center) / Exporter::bhkScaleFactor;
		Point3 pt2 = ((TransMatrix(Point3(0.0f, 0.0f, -distFromCenter)) * rot).GetTrans() + center) / Exporter::bhkScaleFactor;
		shape->SetFirstPoint(TOVECTOR3(pt1));
		shape->SetSecondPoint(TOVECTOR3(pt2));

		retval = StaticCast<bhkShape>(shape);
	}
	return retval;
}


bhkShapeRef Exporter::makeTriStripsShape(INode *node, Matrix3& tm)
{
	TimeValue t = 0;
	Matrix3 sm = ScaleMatrix( GetScale(tm) );
   
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

	int mtl = NP_DEFAULT_HVK_MATERIAL;
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
				switch (bvType)
				{
				case bv_type_box:
					shape = makeProxyBoxShape(node, obj, mesh, tm);
					break;

				//case bv_type_sphere:
				//	shape = makeProxySphereShape(node, obj, mesh, tm);
				//	break;

				case bv_type_shapes:
					shape = makeProxyTriStripShape(node, obj, mesh, tm);
					break;
				case bv_type_packed:
					shape = makeProxyTriStripShape(node, obj, mesh, tm);
					//shape = makeProxyPackedTriStripShape(node, obj, mesh, tm);
					break;

				case bv_type_convex: 
					shape = makeProxyConvexShape(node, obj, mesh, tm);
					break;
				}
			}
		}
	}
	return shape;
}

bhkShapeRef	Exporter::makeProxyBoxShape(INode *node, Object *obj, Mesh& mesh, Matrix3& tm)
{
	enum { list_params, bv_mesh, };  // pblock2 ID
	enum { PB_MATERIAL, PB_MESHLIST, PB_BOUND_TYPE, PB_CENTER, };
	enum { bv_type_none, bv_type_box, bv_type_shapes, bv_type_packed, bv_type_convex, };  // pblock ID

	bhkShapeRef retval;
	if (IParamBlock2* pblock2 = obj->GetParamBlockByID(list_params))
	{
		Box3 box; box.Init();
		CalcAxisAlignedBox(mesh, box, NULL);

		int mtl = 0;
		float length = 0, width = 0, height = 0;
		pblock2->GetValue(PB_MATERIAL, 0, mtl, FOREVER, 0);

		bhkBoxShapeRef shape = new bhkBoxShape();
		Vector3 dim(box.Max().x-box.Min().x, box.Max().y-box.Min().y, box.Max().z-box.Min().z);
		dim /= (Exporter::bhkScaleFactor * 2);

		shape->SetMaterial(HavokMaterial(mtl));
		shape->SetDimensions(dim);

		Matrix3 ltm = /*GetLocalTM(node) * */TransMatrix(box.Center()) * tm;
		if (ltm.IsIdentity())
		{
			retval = StaticCast<bhkShape>(shape);
		}
		else
		{
			ltm.SetTrans(ltm.GetTrans() / Exporter::bhkScaleFactor);

			bhkTransformShapeRef transform = new bhkTransformShape();
			transform->SetTransform(TOMATRIX4(ltm).Transpose());
			transform->SetShape(shape);
			transform->SetMaterial(HavokMaterial(mtl));
			retval = StaticCast<bhkShape>(transform);
		}
	}
	return retval;
}

bhkShapeRef	Exporter::makeProxySphereShape(INode *node, Object *obj, Mesh& mesh, Matrix3& tm)
{
	enum { list_params, bv_mesh, };  // pblock2 ID
	enum { PB_MATERIAL, PB_MESHLIST, PB_BOUND_TYPE, PB_CENTER, };
	enum { bv_type_none, bv_type_box, bv_type_shapes, bv_type_packed, bv_type_convex, };  // pblock ID

	bhkShapeRef shape;
	if (IParamBlock2* pblock2 = obj->GetParamBlockByID(list_params))
	{
		//Matrix3 tm = GetLocalTM(node) * TransMatrix(box.Center());

	}
	return shape;
}

bhkShapeRef	Exporter::makeProxyConvexShape(INode *node, Object *obj, Mesh& mesh, Matrix3& tm)
{
	enum { list_params, bv_mesh, };  // pblock2 ID
	enum { PB_MATERIAL, PB_MESHLIST, PB_BOUND_TYPE, PB_CENTER, };
	enum { bv_type_none, bv_type_box, bv_type_shapes, bv_type_packed, bv_type_convex, };  // pblock ID

	bhkShapeRef shape;
	if (IParamBlock2* pblock2 = obj->GetParamBlockByID(list_params))
	{
		if (bhkConvexVerticesShapeRef convShape = makeConvexShape(mesh, tm))
		{
			int mtl = pblock2->GetInt(PB_MATERIAL, 0, 0);
			convShape->SetMaterial(HavokMaterial(mtl));
			shape = StaticCast<bhkShape>(convShape);
		}
	}
	return shape;
}

bhkShapeRef	Exporter::makeProxyTriStripShape(INode *node, Object *obj, Mesh& mesh, Matrix3& tm)
{
	enum { list_params, bv_mesh, };  // pblock2 ID
	enum { PB_MATERIAL, PB_MESHLIST, PB_BOUND_TYPE, PB_CENTER, };
	enum { bv_type_none, bv_type_box, bv_type_shapes, bv_type_packed, bv_type_convex, };  // pblock ID

	bhkShapeRef shape;
	if (IParamBlock2* pblock2 = obj->GetParamBlockByID(list_params))
	{
		int mtl = pblock2->GetInt(PB_MATERIAL, 0, 0);

		// Transform location
		Mesh localmesh(mesh);
		MNMesh tmpMesh(localmesh);
		tmpMesh.Transform(tm);
		tmpMesh.buildNormals();
		tmpMesh.OutToTri(localmesh);
		localmesh.buildNormals();

		Matrix3 ident(true);
		bhkNiTriStripsShapeRef trishape = makeTriStripsShape(localmesh, ident);
		trishape->SetMaterial(HavokMaterial(mtl));

		shape = StaticCast<bhkShape>(trishape);
	}
	return shape;
}

bhkShapeRef	Exporter::makeProxyPackedTriStripShape(INode *node, Object *obj, Mesh& mesh, Matrix3& tm)
{
	enum { list_params, bv_mesh, };  // pblock2 ID
	enum { PB_MATERIAL, PB_MESHLIST, PB_BOUND_TYPE, PB_CENTER, };
	enum { bv_type_none, bv_type_box, bv_type_shapes, bv_type_packed, bv_type_convex, };  // pblock ID

	bhkShapeRef shape;
	if (IParamBlock2* pblock2 = obj->GetParamBlockByID(list_params))
	{
		int mtl = pblock2->GetInt(PB_MATERIAL, 0, 0);

		// Transform location
		Mesh localmesh(mesh);
		MNMesh tmpMesh(localmesh);
		tmpMesh.Transform(tm);
		tmpMesh.buildNormals();
		tmpMesh.OutToTri(localmesh);
		localmesh.buildNormals();

		Matrix3 ident(true);
		bhkPackedNiTriStripsShapeRef trishape = makePackedTriStripsShape(localmesh, ident);

		shape = StaticCast<bhkShape>(trishape);
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

	node->EvalWorldState(0);
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
		shape = makeModBoxShape(node, mod, const_cast<Mesh&>(*mesh), tm);
		break;

	case bv_type_sphere:
		shape = makeModSphereShape(node, mod, const_cast<Mesh&>(*mesh), tm);
		break;

	case bv_type_capsule:
		shape = makeModCapsuleShape(node, mod, const_cast<Mesh&>(*mesh), tm);
		break;

	case bv_type_shapes:
		shape = makeModTriStripShape(node, mod, const_cast<Mesh&>(*mesh), tm);
		break;

	case bv_type_convex:
		shape = makeModConvexShape(node, mod, const_cast<Mesh&>(*mesh), tm);
		break;
	}
	return shape;
}

bhkShapeRef	Exporter::makeModBoxShape(INode *node, Modifier* mod, Mesh& mesh, Matrix3& tm)
{
	enum { havok_params };
	enum { PB_BOUND_TYPE, PB_MATERIAL, };
	enum { bv_type_none, bv_type_box, bv_type_sphere, bv_type_capsule, bv_type_shapes, bv_type_convex, };  // pblock ID
	int material = NP_DEFAULT_HVK_MATERIAL;

	if (IParamBlock2* pblock2 = mod->GetParamBlockByID(havok_params))
	{
		pblock2->GetValue(PB_MATERIAL, 0, material, FOREVER, 0);
	}

	bhkShapeRef retval;
	if (bhkBoxShapeRef shape = new bhkBoxShape())
	{
		Box3 box; box.Init();
		CalcAxisAlignedBox(mesh, box, NULL);

		Vector3 dim(box.Max().x-box.Min().x, box.Max().y-box.Min().y, box.Max().z-box.Min().z);
		dim /= (Exporter::bhkScaleFactor * 2);
		shape->SetDimensions(dim);
		shape->SetMaterial(HavokMaterial(material));

		Matrix3 ltm = TransMatrix(box.Center()) * node->GetNodeTM(0) * tm;
		if (ltm.IsIdentity())
		{
			retval = StaticCast<bhkShape>(shape);
		}
		else
		{
			ltm.SetTrans(ltm.GetTrans() / Exporter::bhkScaleFactor);

			bhkTransformShapeRef transform = new bhkTransformShape();
			transform->SetTransform(TOMATRIX4(ltm).Transpose());
			transform->SetShape(shape);
			transform->SetMaterial(HavokMaterial(material));
			retval = StaticCast<bhkShape>(transform);
		}
	}
	return retval;
}

bhkShapeRef	Exporter::makeModSphereShape(INode *node, Modifier* mod, Mesh& mesh, Matrix3& tm)
{
	enum { havok_params };
	enum { PB_BOUND_TYPE, PB_MATERIAL, };
	enum { bv_type_none, bv_type_box, bv_type_sphere, bv_type_capsule, bv_type_shapes, bv_type_convex, };  // pblock ID
	int material = NP_DEFAULT_HVK_MATERIAL;

	if (IParamBlock2* pblock2 = mod->GetParamBlockByID(havok_params))
	{
		pblock2->GetValue(PB_MATERIAL, 0, material, FOREVER, 0);
	}

	bhkShapeRef retval;

	Point3 center = Point3::Origin;
	float radius = 0.0f;
	CalcCenteredSphere(mesh, center, radius);

	if (bhkSphereShapeRef shape = new bhkSphereShape())
	{
		shape->SetRadius(radius / Exporter::bhkScaleFactor);
		shape->SetMaterial(HavokMaterial(material));

		Matrix3 ltm = TransMatrix(center) * node->GetObjTMAfterWSM(0) * tm;
		if (ltm.IsIdentity())
		{
			retval = StaticCast<bhkShape>(shape);
		}
		else
		{
			ltm.SetTrans(ltm.GetTrans() / Exporter::bhkScaleFactor);

			bhkTransformShapeRef transform = new bhkTransformShape();
			transform->SetTransform(TOMATRIX4(ltm).Transpose());
			transform->SetShape(shape);
			transform->SetMaterial(HavokMaterial(material));
			retval = StaticCast<bhkShape>(transform);
		}
	}
	return retval;
}

bhkShapeRef	Exporter::makeModCapsuleShape(INode *node, Modifier* mod, Mesh& mesh, Matrix3& tm)
{
	enum { havok_params };
	enum { PB_BOUND_TYPE, PB_MATERIAL, };
	enum { bv_type_none, bv_type_box, bv_type_sphere, bv_type_capsule, bv_type_shapes, bv_type_convex, };  // pblock ID
	int material = NP_DEFAULT_HVK_MATERIAL;

	node->EvalWorldState(0);
	if (IParamBlock2* pblock2 = mod->GetParamBlockByID(havok_params))
	{
		pblock2->GetValue(PB_MATERIAL, 0, material, FOREVER, 0);
	}

	bhkShapeRef retval;

	Point3 center = Point3::Origin;
	float radius = 0.0f;
	CalcCenteredSphere(mesh, center, radius);

	if (bhkCapsuleShapeRef shape = new bhkCapsuleShape())
	{
		shape->SetRadius(radius / Exporter::bhkScaleFactor);
		shape->SetRadius1(radius / Exporter::bhkScaleFactor);
		shape->SetRadius2(radius / Exporter::bhkScaleFactor);
		shape->SetFirstPoint(TOVECTOR3(center/Exporter::bhkScaleFactor));
		shape->SetSecondPoint(TOVECTOR3(center/Exporter::bhkScaleFactor));
		shape->SetMaterial(HavokMaterial(material));

		Matrix3 ltm = TransMatrix(center) * node->GetObjTMAfterWSM(0) * tm;
		if (ltm.IsIdentity())
		{
			retval = StaticCast<bhkShape>(shape);
		}
		else
		{
			ltm.SetTrans(ltm.GetTrans() / Exporter::bhkScaleFactor);

			bhkTransformShapeRef transform = new bhkTransformShape();
			transform->SetTransform(TOMATRIX4(ltm).Transpose());
			transform->SetShape(shape);
			transform->SetMaterial(HavokMaterial(material));
			retval = StaticCast<bhkShape>(transform);
		}
	}
	return retval;
}

bhkShapeRef	Exporter::makeModConvexShape(INode *node, Modifier* mod, Mesh& mesh, Matrix3& tm)
{
	enum { havok_params };
	enum { PB_BOUND_TYPE, PB_MATERIAL, };
	enum { bv_type_none, bv_type_box, bv_type_sphere, bv_type_capsule, bv_type_shapes, bv_type_convex, };  // pblock ID
	int material = NP_DEFAULT_HVK_MATERIAL;

	Matrix3 ltm = node->GetObjTMAfterWSM(0) * tm;

	bhkShapeRef shape;
	if (bhkConvexVerticesShapeRef convShape = makeConvexShape(mesh, ltm))
	{
		if (IParamBlock2* pblock2 = mod->GetParamBlockByID(havok_params))
		{
			pblock2->GetValue(PB_MATERIAL, 0, material, FOREVER, 0);
		}
		convShape->SetMaterial(HavokMaterial(material));
		shape = StaticCast<bhkShape>(convShape);
	}
	return shape;
}

bhkShapeRef	Exporter::makeModTriStripShape(INode *node, Modifier* mod, Mesh& mesh, Matrix3& tm)
{
	enum { havok_params };
	enum { PB_BOUND_TYPE, PB_MATERIAL, };
	enum { bv_type_none, bv_type_box, bv_type_sphere, bv_type_capsule, bv_type_shapes, bv_type_convex, };  // pblock ID
	int material = NP_DEFAULT_HVK_MATERIAL;

	Matrix3 ltm = node->GetObjTMAfterWSM(0) * tm;

	bhkShapeRef shape;
	if (bhkNiTriStripsShapeRef trishape = makeTriStripsShape(mesh, ltm))
	{
		trishape->SetMaterial(HavokMaterial(material));
		if (IParamBlock2* pblock2 = mod->GetParamBlockByID(havok_params))
		{
			pblock2->GetValue(PB_MATERIAL, 0, material, FOREVER, 0);
		}
		shape = StaticCast<bhkShape>(trishape);
	}
	return shape;
}

bhkShapeRef	Exporter::makeModPackedTriStripShape(INode *node, Modifier* mod, Mesh& mesh, Matrix3& tm)
{
	return makeModTriStripShape(node, mod, mesh, tm);
}
