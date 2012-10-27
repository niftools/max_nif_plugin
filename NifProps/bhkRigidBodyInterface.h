/**********************************************************************
*<
FILE: bhkRigidBodyInterface.hpp

DESCRIPTION:	Collision RigidBody Object Declration

CREATED BY: tazpn (Theo)

HISTORY: 
  V1.0 - Derived from 3ds max prim RigidBody example

*>	Copyright (c) 2006, All Rights Reserved.
**********************************************************************/
#ifndef __BHKRIGIDBODYINTERFACE__H
#define __BHKRIGIDBODYINTERFACE__H

#include <ifnpub.h>

#ifndef _countof
#define _countof(x) (sizeof(x)/sizeof((x)[0]))
#endif

//! The unique instance of the rigid body interface
extern CoreExport FPInterfaceDesc gbhkRigidBodyDesc;

#define BHKRIGIDBODYCLASS_DESC Class_ID(0x00000000, 0x1d88470a)
const Interface_ID BHKRIGIDBODYINTERFACE_DESC(0x056aad53, 0x52c54024);

extern FPInterfaceDesc* GetbhkRigidBodyInterfaceDesc();

class bhkRigidBodyInterface : public FPMixinInterface
{
public:			
   //virtual void		SetMaterial(int value, TimeValue time) = 0;
   //virtual int		   GetMaterial(TimeValue time, Interval& valid = FOREVER) const = 0 ;

   virtual void		SetLayer(int value, TimeValue time) = 0;
   virtual int		   GetLayer(TimeValue time, Interval& valid = FOREVER) const = 0 ;

   virtual void		SetMass(float value, TimeValue time) = 0;
   virtual float		GetMass(TimeValue time, Interval& valid = FOREVER) const = 0 ;

   virtual void		SetFriction(float value, TimeValue time) = 0;
   virtual float		GetFriction(TimeValue time, Interval& valid = FOREVER) const = 0 ;

   virtual void		SetRestitution (float value, TimeValue time) = 0;
   virtual float		GetRestitution (TimeValue time, Interval& valid = FOREVER) const = 0 ;

   virtual void		SetLinearDamping(float value, TimeValue time) = 0;
   virtual float		GetLinearDamping(TimeValue time, Interval& valid = FOREVER) const = 0 ;

   virtual void		SetAngularDamping(float value, TimeValue time) = 0;
   virtual float		GetAngularDamping(TimeValue time, Interval& valid = FOREVER) const = 0 ;

   virtual void		SetMaxLinearVelocity(float value, TimeValue time) = 0;
   virtual float		GetMaxLinearVelocity(TimeValue time, Interval& valid = FOREVER) const = 0 ;

   virtual void		SetMaxAngularVelocity(float value, TimeValue time) = 0;
   virtual float		GetMaxAngularVelocity(TimeValue time, Interval& valid = FOREVER) const = 0 ;

   virtual void		SetPenetrationDepth(float value, TimeValue time) = 0;
   virtual float		GetPenetrationDepth(TimeValue time, Interval& valid = FOREVER) const = 0 ;

   virtual void		SetMotionSystem(int value, TimeValue time) = 0;
   virtual int		   GetMotionSystem(TimeValue time, Interval& valid = FOREVER) const = 0 ;

   virtual void		SetQualityType(int value, TimeValue time) = 0;
   virtual int		   GetQualityType(TimeValue time, Interval& valid = FOREVER) const = 0 ;

   virtual void		SetEnableTransform(BOOL value, TimeValue time) = 0;
   virtual BOOL		GetEnableTransform(TimeValue time, Interval& valid = FOREVER) const = 0 ;

   //Function Publishing System
   enum {  
      get_material, set_material,  enum_material,
      get_layer, set_layer, enum_layer,
      get_mass, set_mass,  
      get_friction, set_friction,  
      get_restitution, set_restitution,  
      get_lineardamping, set_lineardamping,  
      get_angulardamping, set_angulardamping,  
      get_maxlinearvelocity, set_maxlinearvelocity,  
      get_maxangularvelocity, set_maxangularvelocity,  
      get_penetrationdepth, set_penetrationdepth,  
      get_motionsystem, set_motionsystem, enum_motionsystem,
      get_qualitytype, set_qualitytype,  enum_qualitytype,
	  get_enabletransform, set_enabletransform,  
   };

   //Function Map For Mixin Interface
   //*************************************************
   BEGIN_FUNCTION_MAP
      //PROP_TFNS(get_material, GetMaterial, set_material, SetMaterial, TYPE_INT);
      PROP_TFNS(get_layer, GetLayer, set_layer, SetLayer, TYPE_INT);
      PROP_TFNS(get_mass, GetMass, set_mass, SetMass, TYPE_FLOAT);
      PROP_TFNS(get_friction, GetFriction, set_friction, SetFriction, TYPE_FLOAT);
      PROP_TFNS(get_restitution, GetRestitution, set_restitution, SetRestitution, TYPE_FLOAT);
      PROP_TFNS(get_lineardamping, GetLinearDamping, set_lineardamping, SetLinearDamping, TYPE_FLOAT);
      PROP_TFNS(get_angulardamping, GetAngularDamping, set_angulardamping, SetAngularDamping, TYPE_FLOAT);
      PROP_TFNS(get_maxlinearvelocity, GetMaxLinearVelocity, set_maxlinearvelocity, SetMaxLinearVelocity, TYPE_FLOAT);
      PROP_TFNS(get_maxangularvelocity, GetMaxAngularVelocity, set_maxangularvelocity, SetMaxAngularVelocity, TYPE_FLOAT);
      PROP_TFNS(get_penetrationdepth, GetPenetrationDepth, set_penetrationdepth, SetPenetrationDepth, TYPE_FLOAT);
      PROP_TFNS(get_motionsystem, GetMotionSystem, set_motionsystem, SetMotionSystem, TYPE_INT);
      PROP_TFNS(get_qualitytype, GetQualityType, set_qualitytype, SetQualityType, TYPE_INT);
	  PROP_TFNS(get_enabletransform, GetEnableTransform, set_enabletransform, SetEnableTransform, TYPE_BOOL);
   END_FUNCTION_MAP

   FPInterfaceDesc* GetDesc();    // <-- must implement 
   //**************************************************
};

//--- Parameter map/block descriptors -------------------------------

// Parameter block indices
enum RigidBodyParamIndicies
{
   //PB_RB_MATERIAL,
   PB_RB_LAYER,
   PB_RB_MASS,
   PB_RB_FRICTION,
   PB_RB_RESTITUTION,
   PB_RB_LINEAR_DAMPING,
   PB_RB_ANGULAR_DAMPING,
   PB_RB_MAX_LINEAR_VELOCITY,
   PB_RB_MAX_ANGULAR_VELOCITY,
   PB_RB_PENETRATION_DEPTH,
   PB_RB_MOTION_SYSTEM,
   PB_RB_QUALITY_TYPE,
   PB_RB_ENABLE_TRANS,
};

class bhkRigidBodyIfcHelper : public bhkRigidBodyInterface
{
public:
   bhkRigidBodyIfcHelper();
   ~bhkRigidBodyIfcHelper();

   IParamBlock*      GetRBBlock();
   void              BeginEditRBParams(IObjParam *ip,ULONG flags,Animatable *prev);
   void              EndEditRBParams(IObjParam *ip,ULONG flags,Animatable *next);

   //virtual void		SetMaterial(int value, TimeValue time);
   //virtual int		   GetMaterial(TimeValue time, Interval& valid = FOREVER) const ;

   virtual void		SetLayer(int value, TimeValue time);
   virtual int		   GetLayer(TimeValue time, Interval& valid = FOREVER) const ;

   virtual void		SetMass(float value, TimeValue time);
   virtual float		GetMass(TimeValue time, Interval& valid = FOREVER) const ;

   virtual void		SetFriction(float value, TimeValue time);
   virtual float		GetFriction(TimeValue time, Interval& valid = FOREVER) const ;

   virtual void		SetRestitution (float value, TimeValue time);
   virtual float		GetRestitution (TimeValue time, Interval& valid = FOREVER) const ;

   virtual void		SetLinearDamping(float value, TimeValue time);
   virtual float		GetLinearDamping(TimeValue time, Interval& valid = FOREVER) const ;

   virtual void		SetAngularDamping(float value, TimeValue time);
   virtual float		GetAngularDamping(TimeValue time, Interval& valid = FOREVER) const ;

   virtual void		SetMaxLinearVelocity(float value, TimeValue time);
   virtual float		GetMaxLinearVelocity(TimeValue time, Interval& valid = FOREVER) const ;

   virtual void		SetMaxAngularVelocity(float value, TimeValue time);
   virtual float		GetMaxAngularVelocity(TimeValue time, Interval& valid = FOREVER) const ;

   virtual void		SetPenetrationDepth(float value, TimeValue time);
   virtual float		GetPenetrationDepth(TimeValue time, Interval& valid = FOREVER) const ;

   virtual void		SetMotionSystem(int value, TimeValue time);
   virtual int		   GetMotionSystem(TimeValue time, Interval& valid = FOREVER) const ;

   virtual void		SetQualityType(int value, TimeValue time);
   virtual int		   GetQualityType(TimeValue time, Interval& valid = FOREVER) const ;

   virtual void		SetEnableTransform(BOOL value, TimeValue time);
   virtual BOOL		GetEnableTransform(TimeValue time, Interval& valid = FOREVER) const;

   virtual FPInterfaceDesc* GetDesc() { return GetbhkRigidBodyInterfaceDesc(); }

   virtual IOResult RBSave(ISave *isave);
   virtual IOResult RBLoad(ILoad *iload);
   
protected:
   IParamBlock* rbpblock;
   static IParamMap *rbpmapParam;
   static IObjParam *rbip;
};

#endif