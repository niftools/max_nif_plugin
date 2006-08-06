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
#include "NifPlugins.h"

using namespace Niflib;

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

   bool ImportRigidBody(bhkRigidBodyRef rb, INode* node);

   bool ImportShape(bhkRigidBodyRef body, bhkShapeRef shape, INode* parent);
   bool ImportSphere(bhkRigidBodyRef body, bhkSphereShapeRef shape, INode *parent);
   bool ImportCapsule(bhkRigidBodyRef body, bhkCapsuleShapeRef shape, INode *parent);
};

bool NifImporter::ImportCollision(NiNodeRef node)
{
   bool ok = false;
   if (!enableCollision)
      return false;
#ifdef USE_UNSUPPORTED_CODE
   // Currently only support the Oblivion bhk basic objects
   NiCollisionObjectRef collObj = node->GetCollisionObject();
   if (collObj)
   {
      CollisionImport ci(*this);
      if (INode *inode = FindINode(gi, collObj->GetParent()))
      {
         NiObjectRef body = collObj->GetBody();
         if (body->IsDerivedType(bhkRigidBody::TypeConst()))
         {
            ci.ImportRigidBody(bhkRigidBodyRef(body), inode);
         }
      }
   }
#endif
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
   Vector3 center = body->GetCenter();

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

   if (bhkShapeRef shape = body->GetShape()) {
      ImportShape(body, shape, node);
   }
   return true;
}

bool CollisionImport::ImportShape(bhkRigidBodyRef body, bhkShapeRef shape, INode* node)
{
   if (shape->IsDerivedType(bhkCapsuleShape::TypeConst()))
   {
      return ImportCapsule(body, bhkCapsuleShapeRef(shape), node);
   }
   else if (shape->IsDerivedType(bhkSphereShape::TypeConst()))
   {
      return ImportSphere(body, bhkSphereShapeRef(shape), node);
   }
   return false;
}

bool CollisionImport::ImportSphere(bhkRigidBodyRef body, bhkSphereShapeRef shape, INode *parent)
{
   bool ok = false;
   if (SimpleObject *ob = (SimpleObject *)ni.gi->CreateInstance(GEOMOBJECT_CLASS_ID, Class_ID(SPHERE_CLASS_ID, 0))) {
      float radius = shape->GetRadius();

      RefTargetHandle t = ob->GetReference(0);
      setMAXScriptValue(t, "radius", 0, radius);

      if (INode *n = ni.gi->CreateObjectNode(ob)) {
         TSTR name = "bhk";
         name.Append(parent->GetName());
         n->SetName(name);

         // Need to "Affect Pivot Only" and "Center to Object" first
         n->CenterPivot(0, FALSE);

         Point3 pos = TOPOINT3(body->GetTranslation()) * ni.bhkScaleFactor;
         Point3 center = TOPOINT3(body->GetCenter());
         Quat rot = TOQUAT(body->GetRotation());
         PosRotScaleNode(n, pos, rot, ni.bhkScaleFactor, prsDefault);

         n->SetPrimaryVisibility(FALSE);
         n->SetSecondaryVisibility(FALSE);
         n->BoneAsLine(TRUE);
         n->SetRenderable(FALSE);
         n->XRayMtl(TRUE);

         parent->AttachChild(n);
         ok = true;
      }
   }
   return ok;
}

bool CollisionImport::ImportCapsule(bhkRigidBodyRef body, bhkCapsuleShapeRef shape, INode *parent)
{
   bool ok = false;
   if (SimpleObject *ob = (SimpleObject *)ni.gi->CreateInstance(GEOMOBJECT_CLASS_ID, SCUBA_CLASS_ID)) {
      float radius = shape->GetRadius();
      float radius1 = shape->GetRadius1();
      float radius2 = shape->GetRadius2();
      Point3 pt1 = TOPOINT3(shape->GetFirstPoint());
      Point3 pt2 = TOPOINT3(shape->GetSecondPoint());
      float height = Length(pt1-pt2);
      int heighttype = 1;

      RefTargetHandle t = ob->GetReference(0);

      IParamArray *params = ob->GetParamBlock();
      params->SetValue(ob->GetParamBlockIndex(CAPSULE_RADIUS), 0, radius);
      params->SetValue(ob->GetParamBlockIndex(CAPSULE_HEIGHT), 0, height);
      params->SetValue(ob->GetParamBlockIndex(CAPSULE_CENTERS), 0, heighttype);

      //setMAXScriptValue(t, "radius", 0, radius);
      //setMAXScriptValue(t, "height", 0, height);
      //setMAXScriptValue(t, "heighttype", 0, heighttype);

      if (INode *n = ni.gi->CreateObjectNode(ob)) {
         TSTR name = "bhk";
         name.Append(parent->GetName());
         n->SetName(name);

         // Need to "Affect Pivot Only" and "Center to Object" first
         //n->CenterPivot(0, FALSE);

         Point3 pos = TOPOINT3(body->GetTranslation()) * ni.bhkScaleFactor;
         Point3 center = TOPOINT3(body->GetCenter());
         Quat rot = TOQUAT(body->GetRotation());
         float ang[] = { 0.0f, TORAD(-90.0f), TORAD(180.0f) };
         Quat q; EulerToQuat(ang, q);
         rot *= q;

         PosRotScaleNode(n, pos, rot, ni.bhkScaleFactor, prsDefault);

         n->SetPrimaryVisibility(FALSE);
         n->SetSecondaryVisibility(FALSE);
         n->BoneAsLine(TRUE);
         n->SetRenderable(FALSE);
         n->XRayMtl(TRUE);

         parent->AttachChild(n);
         ok = true;
      }
   }
   return ok;
}