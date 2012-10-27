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
#include "obj\bhkCollisionObject.h"
#include "obj\bhkBlendCollisionObject.h"
#include "obj\bhkRigidBody.h"
#include "obj\bhkShape.h"
#include "obj\bhkSphereShape.h"
#include "obj\bhkCapsuleShape.h"
#include "obj\bhkConvexVerticesShape.h"
#include "obj\bhkMoppBvTreeShape.h"
#include "obj\bhkPackedNiTriStripsShape.h"
#include "obj\hkPackedNiTriStripsData.h"
#include "obj\bhkListShape.h"
#include "obj\bhkTransformShape.h"
#include "..\NifProps\bhkRigidBodyInterface.h"
#include "NifPlugins.h"
#include "nifqhull.h"

using namespace Niflib;

extern Class_ID BHKLISTOBJECT_CLASS_ID;
extern Class_ID BHKRIGIDBODYMODIFIER_CLASS_ID;
extern Class_ID bhkBoxObject_CLASS_ID;
extern Class_ID BHKCAPSULEOBJECT_CLASS_ID;
extern Class_ID bhkSphereObject_CLASS_ID;

static Class_ID SCUBA_CLASS_ID(0x6d3d77ac, 0x79c939a9);
enum
{
   CAPSULE_RADIUS = 0,
   CAPSULE_HEIGHT = 1,
   CAPSULE_CENTERS = 2,
};

struct CollisionImport
{
   CollisionImport(NifImporter& parent) : ni(parent) {}

   NifImporter &ni;

   void AddShape(INode *rbody, INode *shapeNode);

   bool ImportRigidBody(bhkRigidBodyRef rb, INode* node);
   INode *CreateRigidBody(bhkRigidBodyRef body, INode* parent, Matrix3& tm);

   bool ImportBase(bhkRigidBodyRef body, bhkShapeRef shape, INode* parent, INode *shapeNode, Matrix3& tm);
   bool ImportShape(INode *rbody, bhkRigidBodyRef body, bhkShapeRef shape, INode* parent, Matrix3& tm);
   bool ImportBox(INode *rbody, bhkRigidBodyRef body, bhkBoxShapeRef shape, INode *parent, Matrix3& tm);
   bool ImportSphere(INode *rbody, bhkRigidBodyRef body, bhkSphereShapeRef shape, INode *parent, Matrix3& tm);
   bool ImportCapsule(INode *rbody, bhkRigidBodyRef body, bhkCapsuleShapeRef shape, INode *parent, Matrix3& tm);
   bool ImportConvexVertices(INode *rbody, bhkRigidBodyRef body, bhkConvexVerticesShapeRef shape, INode *parent, Matrix3& tm);
   bool ImportTriStripsShape(INode *rbody, bhkRigidBodyRef body, bhkNiTriStripsShapeRef shape, INode *parent, Matrix3& tm);
   bool ImportMoppBvTreeShape(INode *rbody, bhkRigidBodyRef body, bhkMoppBvTreeShapeRef shape, INode *parent, Matrix3& tm);
   bool ImportPackedNiTriStripsShape(INode *rbody, bhkRigidBodyRef body, bhkPackedNiTriStripsShapeRef shape, INode *parent, Matrix3& tm);   
   bool ImportListShape(INode *rbody, bhkRigidBodyRef body, bhkListShapeRef shape, INode *parent, Matrix3& tm);
   bool ImportTransform(INode *rbody, bhkRigidBodyRef body, bhkTransformShapeRef shape, INode *parent, Matrix3& tm);

   INode* ImportCollisionMesh(
	   const vector<Vector3>& verts, 
	   const vector<Triangle>& tris,
	   const vector<Vector3>& norms,
	   Matrix3& tm,
	   INode *parent
	   );

   enum { bv_type_none, bv_type_box, bv_type_sphere, bv_type_capsule, bv_type_shapes, bv_type_convex, bv_type_packed, bv_type_obb, };  // pblock ID
};

bool NifImporter::ImportCollision(NiNodeRef node)
{
   bool ok = false;
   if (!enableCollision)
      return false;
   // Currently only support the Oblivion bhk basic objects
   bhkNiCollisionObjectRef collObj = node->GetCollisionObject();
   if (collObj)
   {
	   NiObjectRef body = collObj->GetBody();
	   if (body->IsDerivedType(bhkRigidBody::TYPE))
	   {
		   bhkRigidBodyRef rbody = DynamicCast<bhkRigidBody>(body);

		   if (bhkShapeRef shape = rbody->GetShape()) {
			   INode *node = NULL;
			   NiNodeRef target = collObj->GetTarget();
			   if (mergeNonAccum && target && wildmatch("* NonAccum", target->GetName()) ) {
				   node = FindNode(target->GetParent());
			   } else if (target && strmatch(target->GetName(), "Scene Root")) {
				   node = gi->GetRootNode();
			   } else {
				   node = FindNode(target);
			   }

			   CollisionImport ci(*this);
			   Matrix3 tm(true);
			   if (INode *body = ci.CreateRigidBody(rbody, node, tm))
			   {
				   if (!ci.ImportShape(body, rbody, shape, node, tm))
				   {
					   gi->DeleteNode(body, FALSE);
				   }
			   }
		   }
	   }
   }
   return ok;
}


bool CollisionImport::ImportRigidBody(bhkRigidBodyRef body, INode* node)
{
   if (body == NULL)
      return false;

   int lyr = body->GetLayer();
   //body->GetLayerCopy(lyr);
   int msys = body->GetMotionSystem();
   int qtype = body->GetQualityType();
   float mass = body->GetMass();
   float lindamp = body->GetLinearDamping();
   float angdamp = body->GetAngularDamping();
   float frict = body->GetFriction();
   float resti = body->GetRestitution();
   float maxlinvel = body->GetMaxLinearVelocity();
   float maxangvel = body->GetMaxAngularVelocity();
   float pendepth = body->GetPenetrationDepth();
   Vector3 center = TOVECTOR3(body->GetCenter());

   // Update node
   npSetProp(node, NP_HVK_LAYER, lyr);
   //npSetProp(node, NP_HVK_MATERIAL, mtl);
   npSetProp(node, NP_HVK_MOTION_SYSTEM, msys);
   npSetProp(node, NP_HVK_QUALITY_TYPE, qtype);
   npSetProp(node, NP_HVK_MASS, mass);
   npSetProp(node, NP_HVK_LINEAR_DAMPING, lindamp);
   npSetProp(node, NP_HVK_ANGULAR_DAMPING, angdamp);
   npSetProp(node, NP_HVK_FRICTION, frict);
   npSetProp(node, NP_HVK_RESTITUTION, resti);
   npSetProp(node, NP_HVK_MAX_LINEAR_VELOCITY, maxlinvel);
   npSetProp(node, NP_HVK_MAX_ANGULAR_VELOCITY, maxangvel);
   npSetProp(node, NP_HVK_PENETRATION_DEPTH, pendepth);
   npSetProp(node, NP_HVK_CENTER, center);

   npSetCollision(node, true);
   return true;
}

INode* CollisionImport::CreateRigidBody(bhkRigidBodyRef body, INode *parent, Matrix3& tm)
{
	INode *rbody = NULL;
	if (body == NULL)
		return rbody;

	OblivionLayer lyr = body->GetLayer();
	//body->GetLayerCopy(lyr);
	MotionSystem msys = body->GetMotionSystem();
	MotionQuality qtype = body->GetQualityType();
	float mass = body->GetMass();
	float lindamp = body->GetLinearDamping();
	float angdamp = body->GetAngularDamping();
	float frict = body->GetFriction();
	float resti = body->GetRestitution();
	float maxlinvel = body->GetMaxLinearVelocity();
	float maxangvel = body->GetMaxAngularVelocity();
	float pendepth = body->GetPenetrationDepth();
	Vector4 center = body->GetCenter();

	SimpleObject2* listObj = (SimpleObject2*)ni.gi->CreateInstance(HELPER_CLASS_ID, BHKLISTOBJECT_CLASS_ID);
	if (listObj != NULL) 
	{
		bool isTransform = false;
		if (bhkRigidBodyInterface *irb = (bhkRigidBodyInterface *)listObj->GetInterface(BHKRIGIDBODYINTERFACE_DESC))
		{
			irb->SetLayer(lyr, 0);
			//irb->SetLayerCopy(lyr, 0);
			irb->SetMotionSystem(msys, 0);
			irb->SetQualityType(qtype, 0);
			irb->SetMass(mass, 0);
			irb->SetLinearDamping(lindamp, 0);
			irb->SetAngularDamping(angdamp, 0);
			irb->SetFriction(frict, 0);
			irb->SetRestitution(resti, 0);
			irb->SetMaxLinearVelocity(maxlinvel, 0);
			irb->SetMaxAngularVelocity(maxangvel, 0);
			irb->SetPenetrationDepth(pendepth, 0);
			//irb->SetCenter(center);

			isTransform = (body->IsDerivedType(bhkRigidBodyT::TYPE));
			irb->SetEnableTransform(isTransform ? TRUE : FALSE, 0);
		}
		TSTR clsName;
		listObj->GetClassName(clsName);
		if (INode *n = ni.CreateImportNode(clsName, listObj, parent)) {
			Point3 pos = TOPOINT3(body->GetTranslation()* ni.bhkScaleFactor);
			Quat q = TOQUAT(body->GetRotation(), true);
			PosRotScaleNode(n, pos, q, 1.0f, prsDefault);
			rbody = n;
			if (isTransform) {
				Matrix3 qm(true);
				q.MakeMatrix(qm);
				qm.Translate(pos);
				tm *= qm;
				//tm *= TransMatrix(pos);
			}
		}
	}

	//npSetCollision(node, true);
	return rbody;
}

void CollisionImport::AddShape(INode *rbody, INode *shapeNode)
{
	const int PB_MESHLIST = 1;

	if (IParamBlock2* pblock2 = rbody->GetObjectRef()->GetParamBlockByID(0))
	{
		int nBlocks = pblock2->Count(PB_MESHLIST);
		pblock2->SetCount(PB_MESHLIST, nBlocks+1);
		pblock2->SetValue(PB_MESHLIST, 0, shapeNode, nBlocks);
	}
}

bool CollisionImport::ImportBase(bhkRigidBodyRef body, bhkShapeRef shape, INode* parent, INode *shapeNode, Matrix3& tm)
{
	// Now do common post processing for the node
	if (shapeNode != NULL)
	{
		shapeNode->SetName( TSTR(shape->GetType().GetTypeName().c_str()) );

		if (!tm.IsIdentity())
		{
			Point3 pos = tm.GetTrans();
			Quat rot(tm);
			PosRotScaleNode(shapeNode, pos, rot, 1.0, prsDefault);
		}

		// Wireframe Red color
		StdMat2 *collMat = NewDefaultStdMat();
		collMat->SetDiffuse(Color(1.0f, 0.0f, 0.0f), 0);
		collMat->SetWire(TRUE);
		collMat->SetFaceted(TRUE);
		ni.gi->GetMaterialLibrary().Add(collMat);
		shapeNode->SetMtl(collMat);

		shapeNode->SetPrimaryVisibility(FALSE);
		shapeNode->SetSecondaryVisibility(FALSE);
		shapeNode->BoneAsLine(TRUE);
		shapeNode->SetRenderable(FALSE);
		//shapeNode->XRayMtl(TRUE);
		shapeNode->SetWireColor( RGB(255,0,0) );
		if (parent)
			parent->AttachChild(shapeNode);
		return true;
	}
	return false;
}

bool CollisionImport::ImportShape(INode *rbody, bhkRigidBodyRef body, bhkShapeRef shape, INode* parent, Matrix3& tm)
{
	bool ok = false;
	if (shape->IsDerivedType(bhkBoxShape::TYPE))
	{
		ok |= ImportBox(rbody, body, bhkBoxShapeRef(shape), parent, tm);
	}
	else if (shape->IsDerivedType(bhkCapsuleShape::TYPE))
	{
		ok |= ImportCapsule(rbody, body, bhkCapsuleShapeRef(shape), parent, tm);
	}
	else if (shape->IsDerivedType(bhkSphereShape::TYPE))
	{
		ok |= ImportSphere(rbody, body, bhkSphereShapeRef(shape), parent, tm);
	}
	else if (shape->IsDerivedType(bhkConvexVerticesShape::TYPE))
	{
		ok |= ImportConvexVertices(rbody, body, bhkConvexVerticesShapeRef(shape), parent, tm);
	}
	else if (shape->IsDerivedType(bhkNiTriStripsShape::TYPE))
	{
		ok |= ImportTriStripsShape(rbody, body, bhkNiTriStripsShapeRef(shape), parent, tm);
	}
	else if (shape->IsDerivedType(bhkMoppBvTreeShape::TYPE))
	{
		ok |= ImportMoppBvTreeShape(rbody, body, bhkMoppBvTreeShapeRef(shape), parent, tm);
	}
	else if (shape->IsDerivedType(bhkPackedNiTriStripsShape::TYPE))
	{
		ok |= ImportPackedNiTriStripsShape(rbody, body, bhkPackedNiTriStripsShapeRef(shape), parent, tm);
	}
	else if (shape->IsDerivedType(bhkListShape::TYPE))
	{
		ok |= ImportListShape(rbody, body, bhkListShapeRef(shape), parent, tm);
	}
	else if (shape->IsDerivedType(bhkTransformShape::TYPE))
	{
		ok |= ImportTransform(rbody, body, bhkTransformShapeRef(shape), parent, tm);
	}
	return ok;
}

INode *CollisionImport::ImportCollisionMesh(
	const vector<Vector3>& verts, 
	const vector<Triangle>& tris,
	const vector<Vector3>& norms,
	Matrix3& tm,
	INode *parent
	)
{
	INode *returnNode = NULL;
	if ( ImpNode *node = ni.i->CreateNode() )
	{
		TriObject *triObject = CreateNewTriObject();
		node->Reference(triObject);

		Mesh& mesh = triObject->GetMesh();
		INode *tnode = node->GetINode();

		// Vertex info
		{
			int nVertices = verts.size();
			mesh.setNumVerts(nVertices);
			for (int i=0; i < nVertices; ++i){
				Vector3 v = verts[i] * ni.bhkScaleFactor;
				mesh.verts[i].Set(v.x, v.y, v.z);
			}
		}

		// Triangles and texture vertices
		ni.SetTriangles(mesh, tris);
		//ni.SetNormals(mesh, tris, norms);

		MNMesh mn(mesh);
		mn.Transform(tm);
		mn.OutToTri(mesh);
		mesh.checkNormals(TRUE);

		ni.i->AddNodeToScene(node);   

		returnNode = node->GetINode();
		returnNode->EvalWorldState(0);

		if (parent != NULL)
			parent->AttachChild(tnode, 1);
	}
	return returnNode;
}

bool CollisionImport::ImportSphere(INode *rbody, bhkRigidBodyRef body, bhkSphereShapeRef shape, INode *parent, Matrix3& tm)
{
	bhkShapeRef retval;

	enum { sphere_params, };
	enum { PB_MATERIAL, PB_RADIUS, PB_SEGS, PB_SMOOTH, };

	if (SimpleObject *obj = (SimpleObject *)ni.gi->CreateInstance(HELPER_CLASS_ID, bhkSphereObject_CLASS_ID)) {

		if (IParamBlock2* pblock2 = obj->GetParamBlockByID(sphere_params))
		{
			float radius = shape->GetRadius();
			int mtl = shape->GetMaterial();

			pblock2->SetValue(PB_RADIUS, 0, radius, 0);
			pblock2->SetValue(PB_MATERIAL, 0, mtl, 0);

			if (INode *n = ni.CreateImportNode(shape->GetType().GetTypeName().c_str(), obj, parent)) {
				ImportBase(body, shape, parent, n, tm);
				AddShape(rbody, n);
				return true;
			}
		}
	}

#if 0
   if (SimpleObject *ob = (SimpleObject *)ni.gi->CreateInstance(GEOMOBJECT_CLASS_ID, Class_ID(SPHERE_CLASS_ID, 0))) {
      float radius = shape->GetRadius();

      RefTargetHandle t = ob->GetReference(0);
      setMAXScriptValue(t, "radius", 0, radius);

      if (INode *n = ni.CreateImportNode(shape->GetType().GetTypeName().c_str(), ob, parent)) {
#if VERSION_3DSMAX > ((5000<<16)+(15<<8)+0) // Version 5
         // Need to "Affect Pivot Only" and "Center to Object" first
         n->CenterPivot(0, FALSE);
#endif
		 CreatebhkCollisionModifier(n, bv_type_sphere, shape->GetMaterial(), OL_UNIDENTIFIED, 0);
		 
		 ImportBase(body, shape, parent, n, tm);
		 AddShape(rbody, n);
         return true;
      }
   }
#endif
   return false;
}

bool CollisionImport::ImportBox(INode *rbody, bhkRigidBodyRef body, bhkBoxShapeRef shape, INode *parent, Matrix3& tm)
{
	enum { box_params, };
	enum { PB_MATERIAL, PB_LENGTH, PB_WIDTH, PB_HEIGHT, };

	bhkShapeRef retval;
	if (SimpleObject *obj = (SimpleObject *)ni.gi->CreateInstance(HELPER_CLASS_ID, bhkBoxObject_CLASS_ID)) {

		if (IParamBlock2* pblock2 = obj->GetParamBlockByID(box_params))
		{
			float radius = shape->GetRadius();
			int mtl = shape->GetMaterial();
			Vector3 dim = shape->GetDimensions();

			pblock2->SetValue(PB_MATERIAL, 0, mtl, 0);
			pblock2->SetValue(PB_WIDTH, 0, dim.x, 0);
			pblock2->SetValue(PB_LENGTH, 0, dim.y, 0);
			pblock2->SetValue(PB_HEIGHT, 0, dim.z, 0);

			if (INode *n = ni.CreateImportNode(shape->GetType().GetTypeName().c_str(), obj, parent)) {
				ImportBase(body, shape, parent, n, tm);
				AddShape(rbody, n);
				return true;
			}
		}
	}
	return false;
}

bool CollisionImport::ImportCapsule(INode *rbody, bhkRigidBodyRef body, bhkCapsuleShapeRef shape, INode *parent, Matrix3& tm)
{
	enum { cap_params, };
	enum { PB_MATERIAL, PB_RADIUS1, PB_RADIUS2, PB_LENGTH, };

	bhkShapeRef retval;
	if (SimpleObject *obj = (SimpleObject *)ni.gi->CreateInstance(HELPER_CLASS_ID, BHKCAPSULEOBJECT_CLASS_ID)) {

		if (IParamBlock2* pblock2 = obj->GetParamBlockByID(cap_params))
		{
			float radius = shape->GetRadius();
			int mtl = shape->GetMaterial();
			float radius1 = shape->GetRadius1();
			float radius2 = shape->GetRadius2();
			Vector3 pt1 = shape->GetFirstPoint();
			Vector3 pt2 = shape->GetSecondPoint();
			float len = (pt2 - pt1).Magnitude();

			Point3 center = (TOPOINT3(pt2 + pt1) / 2.0f) * ni.bhkScaleFactor;
			Point3 norm = Normalize(TOPOINT3(pt2 - pt1));
			Matrix3 mat;
			MatrixFromNormal(norm,mat);
			Matrix3 newTM = tm * mat * TransMatrix(center);

			pblock2->SetValue(PB_MATERIAL, 0, mtl, 0);
			pblock2->SetValue(PB_LENGTH, 0, len, 0);
			pblock2->SetValue(PB_RADIUS1, 0, radius1, 0);
			pblock2->SetValue(PB_RADIUS2, 0, radius2, 0);

			if (INode *n = ni.CreateImportNode(shape->GetType().GetTypeName().c_str(), obj, parent)) {
				ImportBase(body, shape, parent, n, newTM);
				AddShape(rbody, n);
				return true;
			}
		}
	}
	return false;
#if 0
   if (SimpleObject *ob = (SimpleObject *)ni.gi->CreateInstance(GEOMOBJECT_CLASS_ID, SCUBA_CLASS_ID)) {
      float radius = shape->GetRadius();
      float radius1 = shape->GetRadius1();
      float radius2 = shape->GetRadius2();
      Point3 pt1 = TOPOINT3(shape->GetFirstPoint());
      Point3 pt2 = TOPOINT3(shape->GetSecondPoint());
      float height = Length(pt1-pt2);
      int heighttype = 1;

      RefTargetHandle t = ob->GetReference(0);
      if (IParamBlock2* pblock2 = ob->GetParamBlockByID(0))
	  {
		  pblock2->SetValue(CAPSULE_RADIUS, 0, radius);
		  pblock2->SetValue(CAPSULE_HEIGHT, 0, height);
		  pblock2->SetValue(CAPSULE_CENTERS, 0, heighttype);
	  }

	  if (INode *n = ni.CreateImportNode(shape->GetType().GetTypeName().c_str(), ob, parent)) {
		  // Need to "Affect Pivot Only" and "Center to Object" first
		  //n->CenterPivot(0, FALSE);

		  // Need to reposition the Capsule so that caps are rotated correctly for pts given

		  CreatebhkCollisionModifier(n, bv_type_capsule, shape->GetMaterial(), OL_UNIDENTIFIED, 0);
		  ImportBase(body, shape, parent, n, tm);
		  AddShape(rbody, n);
		  return true;
	  }
   }
   return true;
#endif
}

bool CollisionImport::ImportConvexVertices(INode *rbody, bhkRigidBodyRef body, bhkConvexVerticesShapeRef shape, INode *parent, Matrix3& tm)
{
	Matrix3 ltm(true);
	INode *returnNode = NULL;
	vector<Vector3> verts = shape->GetVertices();
	vector<Vector3> norms = shape->GetNormals();
	vector<Triangle> tris = NifQHull::compute_convex_hull(verts);
	returnNode = ImportCollisionMesh(verts, tris, norms, ltm, parent);

	CreatebhkCollisionModifier(returnNode, bv_type_convex, shape->GetMaterial(), OL_UNIDENTIFIED, 0);
	ImportBase(body, shape, parent, returnNode, tm);
	AddShape(rbody, returnNode);
	return true;
}

bool CollisionImport::ImportTriStripsShape(INode *rbody, bhkRigidBodyRef body, bhkNiTriStripsShapeRef shape, INode *parent, Matrix3& tm)
{
	if (shape->GetNumStripsData() != 1)
		return NULL;

	if ( ImpNode *node = ni.i->CreateNode() )
	{
		TriObject *triObject = CreateNewTriObject();
		node->Reference(triObject);

		INode *inode = node->GetINode();

		// Texture
		Mesh& mesh = triObject->GetMesh();
		NiTriStripsDataRef triShapeData = shape->GetStripsData(0);
		if (triShapeData == NULL)
			return false;

		// Temporary shape
		NiTriStripsRef triShape = new NiTriStrips();
		vector<Triangle> tris = triShapeData->GetTriangles();
		ni.ImportMesh(node, triObject, triShape, triShapeData, tris);
		CreatebhkCollisionModifier(inode, bv_type_shapes, shape->GetMaterial(), OL_UNIDENTIFIED, 0);
		ImportBase(body, shape, parent, inode, tm);
		AddShape(rbody, inode);
		return true;
	}
	return false;
}

bool CollisionImport::ImportMoppBvTreeShape(INode *rbody, bhkRigidBodyRef body, bhkMoppBvTreeShapeRef shape, INode *parent, Matrix3& tm)
{
	bool retval = ImportShape(rbody, body, shape->GetShape(), parent, tm);
	return retval;
}

bool CollisionImport::ImportPackedNiTriStripsShape(INode *rbody, bhkRigidBodyRef body, bhkPackedNiTriStripsShapeRef shape, INode *parent, Matrix3& tm)
{
	if (hkPackedNiTriStripsDataRef data = shape->GetData())
	{
		Matrix3 ltm(true);
		vector<Vector3> verts = data->GetVertices();
		vector<Triangle> tris = data->GetTriangles();
		vector<Vector3> norms = data->GetNormals();

		vector<Niflib::OblivionSubShape> subshapes = (ni.IsFallout3() || ni.IsSkyrim()) ? shape->GetSubShapes() : data->GetSubShapes();
		if (subshapes.size() == 0)
		{
			// Is this possible?
			INode *inode = ImportCollisionMesh(verts, tris, norms, tm, parent);
			CreatebhkCollisionModifier(inode, bv_type_packed, HavokMaterial(NP_DEFAULT_HVK_MATERIAL), OL_UNIDENTIFIED, 0);
			ImportBase(body, shape, parent, inode, ltm);
			AddShape(rbody, inode);
		}
		else
		{
			unsigned int voff = 0;
			unsigned int toff = 0;

			INodeTab nodes;
			for (int i = 0, n = subshapes.size(); i<n; ++i) {
				Niflib::OblivionSubShape& s = subshapes[i];
				
				vector<Vector3> subverts;
				vector<Triangle> subtris;
				vector<Vector3> subnorms;
				
				subverts.reserve(s.numVertices);
				for (unsigned int v=voff; v < (voff + s.numVertices); ++v) {
					subverts.push_back( verts[v] );
				}
				unsigned int vend = (voff + s.numVertices );

				// TODO: Fix algorithm.  I do not know how to split the triangles here
				//       Basically, greedily take all triangles until next subshape
				//       This is not correct but seems to work with most meshes tested.
				subtris.reserve( s.numVertices / 2 );
				subnorms.reserve( s.numVertices / 2 );
				while ( toff < tris.size() ){
					Triangle t = tris[toff];
					if ( t.v1 >= vend || t.v2 >= vend || t.v3 >= vend )
						break;
					// remove offset for mesh
					t.v1 -= voff; t.v2 -= voff; t.v3 -= voff;
					subtris.push_back( t );	
					subnorms.push_back( norms[toff] );	
					++toff;
				}
				voff += s.numVertices;

				INode *inode = ImportCollisionMesh(subverts, subtris, subnorms, tm, parent);

				CreatebhkCollisionModifier(inode, bv_type_packed, HavokMaterial(s.material), s.layer, s.colFilter);
				ImportBase(body, shape, parent, inode, ltm);

				if (n > 1)
					inode->SetName( FormatText("%s:%d", "OblivionSubShape", i).data() );

				nodes.Append(1, &inode);
			}
			// TODO: Group nodes on import
			if (  nodes.Count() > 1 )
			{
				TSTR shapeName = "bhkPackedNiTriStripsShape";
				INode *group = ni.gi->GroupNodes(&nodes, &shapeName, 0);			
				AddShape(rbody, group);
			}
			else if (  nodes.Count() == 1 )
			{
				AddShape(rbody, nodes[0]);
			}
		}
		return true;
	}

	return false;
}

bool CollisionImport::ImportListShape(INode *rbody, bhkRigidBodyRef body, bhkListShapeRef shape, INode *parent, Matrix3& tm)
{
	bool ok = false;
	HavokMaterial material = shape->GetMaterial();

	const int PB_MATERIAL = 0;
	if (IParamBlock2* pblock2 = rbody->GetObjectRef()->GetParamBlockByID(0))
	{
		pblock2->SetValue(PB_MATERIAL, 0, material, 0);
	}

	vector<Ref<bhkShape > > bhkshapes = shape->GetSubShapes();
	for (int i = 0, n = bhkshapes.size(); i<n; ++i) {
		ok |= ImportShape(rbody, body, bhkshapes[i], parent, tm);
	}
	return ok;
}

bool CollisionImport::ImportTransform(INode *rbody, bhkRigidBodyRef body, bhkTransformShapeRef shape, INode *parent, Matrix3& tm)
{
	Matrix44 m4 = shape->GetTransform().Transpose();
	Vector3 trans; Matrix33 rot; float scale;
	m4.Decompose(trans, rot, scale);
	Matrix3 wm = TOMATRIX3(rot);
	wm.Translate(TOPOINT3(trans) * ni.bhkScaleFactor);
	wm *= ScaleMatrix(Point3(scale, scale, scale));
	wm = wm * tm;
	return ImportShape(rbody, body, shape->GetShape(), parent, wm);
}