/**********************************************************************
*<
FILE: bhkRigidBodyInterface.cpp

DESCRIPTION:	Collision RigidBody Object Implementation

CREATED BY: tazpn (Theo)

HISTORY: 
  V1.0 - Derived from 3ds max prim sphere example

*>	Copyright (c) 2006, All Rights Reserved.
**********************************************************************/
#pragma warning( disable:4800 )
#include <max.h>
#include "MAX_Mem.h"
#include <map>
#include "NifProps.h"
#include "iparamm.h"
#include "Simpobj.h"
#include "notify.h"
#include "macroRec.h"
#include "bhkRigidBodyInterface.h"
#include "NifPlugins.h"
#include "NifGui.h"
#include "NifStrings.h"
//--- Parameter map/block descriptors -------------------------------


FPInterfaceDesc thebhkRigidBodyInterface(
   BHKRIGIDBODYINTERFACE_DESC, _T("bhkRigidBody"), -1/*IDS_OPS*/, 0, FP_CORE,
   properties,
      //bhkRigidBodyInterface::get_material, bhkRigidBodyInterface::set_material, _T("Material"), 0, TYPE_ENUM, bhkRigidBodyInterface::enum_material,
      bhkRigidBodyInterface::get_layer, bhkRigidBodyInterface::set_layer, _T("Layer"), 0, TYPE_ENUM, bhkRigidBodyInterface::enum_layer,
      bhkRigidBodyInterface::get_mass, bhkRigidBodyInterface::set_mass, _T("Mass"), 0, TYPE_FLOAT,
      bhkRigidBodyInterface::get_friction, bhkRigidBodyInterface::set_friction, _T("Friction"), 0, TYPE_FLOAT,
      bhkRigidBodyInterface::get_restitution, bhkRigidBodyInterface::set_restitution, _T("Restitution"), 0, TYPE_FLOAT,
      bhkRigidBodyInterface::get_lineardamping, bhkRigidBodyInterface::set_lineardamping, _T("LinearDamping"), 0, TYPE_FLOAT,
      bhkRigidBodyInterface::get_angulardamping, bhkRigidBodyInterface::set_angulardamping, _T("AngularDamping"), 0, TYPE_FLOAT,
      bhkRigidBodyInterface::get_maxlinearvelocity, bhkRigidBodyInterface::set_maxlinearvelocity, _T("MaxLinearVelocity"), 0, TYPE_FLOAT,
      bhkRigidBodyInterface::get_maxangularvelocity, bhkRigidBodyInterface::set_maxangularvelocity, _T("MaxAngularVelocity"), 0, TYPE_FLOAT,
      bhkRigidBodyInterface::get_penetrationdepth, bhkRigidBodyInterface::get_penetrationdepth, _T("PenetrationDepth"), 0, TYPE_FLOAT,
      bhkRigidBodyInterface::get_motionsystem, bhkRigidBodyInterface::set_motionsystem, _T("MotionSystem"), 0, TYPE_ENUM, bhkRigidBodyInterface::enum_motionsystem,
      bhkRigidBodyInterface::get_qualitytype, bhkRigidBodyInterface::set_qualitytype, _T("QualityType"), 0, TYPE_ENUM, bhkRigidBodyInterface::enum_qualitytype,
	  bhkRigidBodyInterface::get_enabletransform, bhkRigidBodyInterface::set_enabletransform, _T("EnableTransform"), 0, TYPE_BOOL, 
   enums,
      bhkRigidBodyInterface::enum_material, 31,
         "Stone", 0,
         "Cloth", 1,
         "Dirt", 2,
         "Glass", 3,
         "Grass", 4,
         "Metal", 5,
         "Organic", 6,
         "Skin", 7,
         "Water", 8,
         "Wood", 9,
         "Heavy Stone", 10, 
         "Heavy Metal", 11,
         "Heavy Wood", 12,
         "Chain", 13,
         "Snow", 14,
         "Stone Stairs", 15,
         "Cloth Stairs", 16,
         "Dirt Stairs", 17,
         "Glass Stairs", 18,
         "Grass Stairs", 19,
         "Metal Stairs", 20,
         "Organic Stairs", 21,
         "Skin Stairs", 22,
         "Water Stairs", 23,
         "Wood Stairs", 24,
         "Heavy Stone Stairs", 25,
         "Heavy Metal Stairs", 26,
         "Heavy Wood Stairs", 27,
         "Chain Stairs", 28,
         "Snow Stairs", 29,
         "Elevator", 30,
      bhkRigidBodyInterface::enum_layer, 57,
         "Unidentified", 0,
         "Static", 1,
         "AnimStatic", 2,
         "Transparent", 3,
         "Clutter", 4,
         "Weapon", 5,
         "Projectile", 6,
         "Spell", 7,
         "Biped", 8,
         "Tree", 9,
         "Prop", 10,
         "Water", 11,
         "Trigger", 12,
         "Terrain", 13,
         "Trap", 14,
         "NonCollidable", 15,
         "CloudTrap", 16,
         "Ground", 17,
         "Portal", 18,
         "Stairs", 19,
         "CharController", 20,
         "AvoidBox", 21,
         "Unknown(22)", 22,
         "Unknown(23)", 23,
         "CameraPick", 24,
         "ItemPick", 25,
         "LineOfSight", 26,
         "PathPick", 27,
         "CustomPick1", 28,
         "CustomPick2", 29,
         "SpellExplosion", 30,
         "DroppingPick", 31,
         "Other", 32,
         "Head", 33,
         "Body", 34,
         "Spine1", 35,
         "Spine2", 36,
         "LUpperArm", 37,
         "LForeArm", 38,
         "LHand", 39,
         "LThigh", 40,
         "LCalf", 41,
         "LFoot", 42,
         "RUpperArm", 43,
         "RForeArm", 44,
         "RHand", 45,
         "RThigh", 46,
         "RCalf", 47,
         "RFoot", 48,
         "Tail", 49,
         "SideWeapon", 50,
         "Shield", 51,
         "Quiver", 52,
         "BackWeapon", 53,
         "BackWeapon", 54,
         "PonyTail", 55,
         "Wing", 56,
      bhkRigidBodyInterface::enum_motionsystem, 10,
         "Keyframed(0)", 0,
         "Box(1)", 1,
         "Sphere(2)", 2,
         "Sphere(2)", 3,
         "Box(4)", 4,
         "Box(5)", 5,
         "Keyframed(6)", 6,
         "Keyframed(7)", 7,
         "Box(8)", 8,
         "Keyframed(9)", 9,
      bhkRigidBodyInterface::enum_qualitytype, 9,
         "Moving", 0,
         "Fixed", 1,
         "Keyframed", 2,
         "Moving(3)", 3,
         "Moving(4)", 4,
         "Critical", 5,
         "Bullet", 6,
         "User", 7,
         "Null", 8,
   end);

FPInterfaceDesc *bhkRigidBodyInterface::GetDesc() {
   return &thebhkRigidBodyInterface;
}

FPInterfaceDesc* GetbhkRigidBodyInterfaceDesc()
{
   return &thebhkRigidBodyInterface;
}

//
//
// Parameters

static ParamUIDesc descRigidBodyParam[] = {

   // Mass
   ParamUIDesc(
   PB_RB_MASS,
   EDITTYPE_UNIVERSE,
   IDC_ED_MASS,IDC_SP_MASS,
   0, 1000,
   SPIN_AUTOSCALE),	

   // Friction
   ParamUIDesc(
   PB_RB_FRICTION,
   EDITTYPE_UNIVERSE,
   IDC_ED_FRICTION,IDC_SP_FRICTION,
   0, 10,
   SPIN_AUTOSCALE),

   // Restitution
   ParamUIDesc(
   PB_RB_RESTITUTION,
   EDITTYPE_UNIVERSE,
   IDC_ED_RESTITUTION,IDC_SP_RESTITUTION,
   0, 10,
   SPIN_AUTOSCALE),

   // Linear Damping
   ParamUIDesc(
   PB_RB_LINEAR_DAMPING,
   EDITTYPE_UNIVERSE,
   IDC_ED_LINEAR_DAMPING,IDC_SP_LINEAR_DAMPING,
   0, 10,
   SPIN_AUTOSCALE),

   // Angular Damping
   ParamUIDesc(
   PB_RB_ANGULAR_DAMPING,
   EDITTYPE_UNIVERSE,
   IDC_ED_ANGULAR_DAMPING,IDC_SP_ANGULAR_DAMPING,
   0, 10,
   SPIN_AUTOSCALE),

   // Max Linear Velocity
   ParamUIDesc(
   PB_RB_MAX_LINEAR_VELOCITY,
   EDITTYPE_UNIVERSE,
   IDC_ED_MAX_LINEAR_VELOCITY,IDC_SP_MAX_LINEAR_VELOCITY,
   0, 10,
   SPIN_AUTOSCALE),

   // Max Angular Velocity
   ParamUIDesc(
   PB_RB_MAX_ANGULAR_VELOCITY,
   EDITTYPE_UNIVERSE,
   IDC_ED_MAX_ANGULAR_VELOCITY,IDC_SP_MAX_ANGULAR_VELOCITY,
   0, 10,
   SPIN_AUTOSCALE),

   // Penetration Depth
   ParamUIDesc(
   PB_RB_PENETRATION_DEPTH,
   EDITTYPE_UNIVERSE,
   IDC_ED_PENETRATION_DEPTH,IDC_SP_PENETRATION_DEPTH,
   0, 10,
   SPIN_AUTOSCALE),

   // Enable Transform
   ParamUIDesc(
   PB_RB_ENABLE_TRANS,
   TYPE_SINGLECHEKBOX,
   IDC_TRANS_ENABLE),

};
const int descRigidBodyParamLength = _countof(descRigidBodyParam);

static ParamBlockDescID gRigidBlockParamDesc[] = {
   //{ TYPE_INT, NULL, FALSE, PB_RB_MATERIAL },
   { TYPE_INT, NULL, FALSE, PB_RB_LAYER },
   { TYPE_FLOAT, NULL, FALSE, PB_RB_MASS },
   { TYPE_FLOAT, NULL, FALSE, PB_RB_FRICTION },
   { TYPE_FLOAT, NULL, FALSE, PB_RB_RESTITUTION },
   { TYPE_FLOAT, NULL, FALSE, PB_RB_LINEAR_DAMPING },
   { TYPE_FLOAT, NULL, FALSE, PB_RB_ANGULAR_DAMPING },
   { TYPE_FLOAT, NULL, FALSE, PB_RB_MAX_LINEAR_VELOCITY },
   { TYPE_FLOAT, NULL, FALSE, PB_RB_MAX_ANGULAR_VELOCITY },
   { TYPE_FLOAT, NULL, FALSE, PB_RB_PENETRATION_DEPTH },
   { TYPE_INT, NULL, FALSE, PB_RB_MOTION_SYSTEM },
   { TYPE_INT, NULL, FALSE, PB_RB_QUALITY_TYPE },
   { TYPE_BOOL, NULL, FALSE, PB_RB_ENABLE_TRANS },
};
const int descRigidBodyDescIDLength = _countof(gRigidBlockParamDesc);

class RigidBodyParamDlgProc : public ParamMapUserDlgProc {
public:
   bhkRigidBodyInterface *so;
   HWND thishWnd;
   NpComboBox		mCbLayer;
   //NpComboBox		mCbMaterial;
   NpComboBox		mCbMotionSystem;
   NpComboBox		mCbQualityType;

   RigidBodyParamDlgProc(bhkRigidBodyInterface *s) {so=s;thishWnd=NULL;}
   INT_PTR DlgProc(TimeValue t,IParamMap *map,HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam);
   void Update(TimeValue t);
   void DeleteThis() {delete this;}

   //--- ParamDlgProc --------------------------------
   void TurnSpinner(HWND hWnd,int SpinNum,BOOL ison)
   {	
      ISpinnerControl *spin2 = GetISpinner(GetDlgItem(hWnd,SpinNum));
      if (ison) spin2->Enable();else spin2->Disable();
      ReleaseISpinner(spin2);
   };

};

void RigidBodyParamDlgProc::Update(TimeValue t)
{
   if (!thishWnd) 
      return;

   
   //mCbMaterial.select(max(0, min(so->GetMaterial(t), mCbMaterial.count()-1)));
   mCbLayer.select(max(0, min(so->GetLayer(t), mCbLayer.count()-1)));
   mCbMotionSystem.select(max(0, min(so->GetMotionSystem(t), mCbMotionSystem.count()-1)));
   mCbQualityType.select(max(0, min(so->GetQualityType(t), mCbQualityType.count()-1)));
   return;
}

INT_PTR RigidBodyParamDlgProc::DlgProc(TimeValue t,IParamMap *map,HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
   thishWnd=hWnd;
   switch (msg) 
   {
   case WM_INITDIALOG: 
      {
         //mCbMaterial.init(GetDlgItem(hWnd, IDC_CB_MATERIAL));
         //for (const char **str = NpHvkMaterialNames; *str; ++str)
         //   mCbMaterial.add(*str);

         mCbLayer.init(GetDlgItem(hWnd, IDC_CB_LAYER));
         for (const char **str = NpHvkLayerNames; *str; ++str)
            mCbLayer.add(*str);

         mCbMotionSystem.init(GetDlgItem(hWnd, IDC_CB_MOTION_SYSTEM));         
         for (const char **str = NpHvkMotionSystems; *str; ++str)
            mCbMotionSystem.add(*str);

         mCbQualityType.init(GetDlgItem(hWnd, IDC_CB_QUALITY_TYPE));
         for (const char **str = NpHvkQualityTypes; *str; ++str)
            mCbQualityType.add(*str);

         Update(t);
         break;
      }
   case WM_COMMAND:
      switch (LOWORD(wParam))
      {
      //case IDC_CB_MATERIAL:
      //   if (HIWORD(wParam)==CBN_SELCHANGE) {
      //      so->SetMaterial( mCbMaterial.selection(), t );
      //   }
      //   break;

      case IDC_CB_LAYER:
         if (HIWORD(wParam)==CBN_SELCHANGE) {
            so->SetLayer( mCbLayer.selection(), t );
         }
         break;

      case IDC_CB_MOTION_SYSTEM:
         if (HIWORD(wParam)==CBN_SELCHANGE) {
            so->SetMotionSystem( mCbMotionSystem.selection(), t );
         }
         break;

      case IDC_CB_QUALITY_TYPE:
         if (HIWORD(wParam)==CBN_SELCHANGE) {
            so->SetQualityType( mCbQualityType.selection(), t );
         }
         break;
      }
      break;
   }
   return FALSE;
}

//--- RigidBody methods -------------------------------

IParamMap *bhkRigidBodyIfcHelper::rbpmapParam = NULL;
IObjParam *bhkRigidBodyIfcHelper::rbip = NULL;

bhkRigidBodyIfcHelper::bhkRigidBodyIfcHelper()
{
   rbpblock = CreateParameterBlock(gRigidBlockParamDesc, descRigidBodyDescIDLength, 1);
   //rbpblock->SetValue(PB_RB_MATERIAL,0,NP_DEFAULT_HVK_MATERIAL);
   rbpblock->SetValue(PB_RB_LAYER,0,NP_DEFAULT_HVK_LAYER);
   rbpblock->SetValue(PB_RB_MASS,0,NP_DEFAULT_HVK_MASS);
   rbpblock->SetValue(PB_RB_FRICTION,0,NP_DEFAULT_HVK_FRICTION);
   rbpblock->SetValue(PB_RB_RESTITUTION,0,NP_DEFAULT_HVK_RESTITUTION);
   rbpblock->SetValue(PB_RB_LINEAR_DAMPING,0,NP_DEFAULT_HVK_LINEAR_DAMPING);
   rbpblock->SetValue(PB_RB_ANGULAR_DAMPING,0,NP_DEFAULT_HVK_ANGULAR_DAMPING);
   rbpblock->SetValue(PB_RB_MAX_LINEAR_VELOCITY,0,NP_DEFAULT_HVK_MAX_LINEAR_VELOCITY);
   rbpblock->SetValue(PB_RB_MAX_ANGULAR_VELOCITY,0,NP_DEFAULT_HVK_MAX_ANGULAR_VELOCITY);
   rbpblock->SetValue(PB_RB_PENETRATION_DEPTH,0,NP_DEFAULT_HVK_PENETRATION_DEPTH);
   rbpblock->SetValue(PB_RB_MOTION_SYSTEM,0,NP_DEFAULT_HVK_MOTION_SYSTEM);
   rbpblock->SetValue(PB_RB_QUALITY_TYPE,0,NP_DEFAULT_HVK_QUALITY_TYPE);
   rbpblock->SetValue(PB_RB_ENABLE_TRANS,0,TRUE);

}

bhkRigidBodyIfcHelper::~bhkRigidBodyIfcHelper()
{
   if (rbpmapParam) {
      rbpmapParam->SetUserDlgProc(NULL);
      DestroyCPParamMap(rbpmapParam);
      rbpmapParam  = NULL;
   }
}

IParamBlock* bhkRigidBodyIfcHelper::GetRBBlock()
{
   return rbpblock;
}

void bhkRigidBodyIfcHelper::BeginEditRBParams(IObjParam *ip,ULONG flags,Animatable *prev)
{
   // Gotta make a new one.
   if (NULL == rbpmapParam) 
   {
      rbpmapParam = CreateCPParamMap(
         descRigidBodyParam,descRigidBodyParamLength,
         rbpblock,
         ip,
         hInstance,
         MAKEINTRESOURCE(IDD_RIGIDBODY),
         GetString(IDS_RB_RIGIDBODY_PARAM),
         0);
   }
   this->rbip = ip;

   if(rbpmapParam) {
      // A callback for the type in.
      rbpmapParam->SetUserDlgProc(new RigidBodyParamDlgProc(this));
   }
}

void bhkRigidBodyIfcHelper::EndEditRBParams( IObjParam *ip, ULONG flags,Animatable *next )
{
   if (rbpmapParam && flags&END_EDIT_REMOVEUI ) {
      rbpmapParam->SetUserDlgProc(NULL);
      DestroyCPParamMap(rbpmapParam);
      rbpmapParam = NULL;
   }
}


//void bhkRigidBodyIfcHelper::SetMaterial(int value, TimeValue time)
//{
//   rbpblock->SetValue(PB_RB_MATERIAL,time,value);
//}
//
//int bhkRigidBodyIfcHelper::GetMaterial(TimeValue time, Interval& valid) const
//{
//   int value = NP_DEFAULT_HVK_MATERIAL;
//   rbpblock->GetValue(PB_RB_MATERIAL,time,value,valid);
//   return value;
//}

void bhkRigidBodyIfcHelper::SetLayer(int value, TimeValue time)
{
   rbpblock->SetValue(PB_RB_LAYER,time,value);
}
int bhkRigidBodyIfcHelper::GetLayer(TimeValue time, Interval& valid) const 
{
   int value = NP_DEFAULT_HVK_LAYER;
   rbpblock->GetValue(PB_RB_LAYER,time,value,valid);
   return value;
}

void bhkRigidBodyIfcHelper::SetMass(float value, TimeValue time)
{
   rbpblock->SetValue(PB_RB_MASS,time,value);
}
float bhkRigidBodyIfcHelper::GetMass(TimeValue time, Interval& valid) const 
{
   float value = NP_DEFAULT_HVK_MASS;
   rbpblock->GetValue(PB_RB_MASS,time,value,valid);
   return value;
}

void bhkRigidBodyIfcHelper::SetFriction(float value, TimeValue time)
{
   rbpblock->SetValue(PB_RB_FRICTION,time,value);
}

float bhkRigidBodyIfcHelper::GetFriction(TimeValue time, Interval& valid) const 
{
   float value = NP_DEFAULT_HVK_FRICTION;
   rbpblock->GetValue(PB_RB_FRICTION,time,value,valid);
   return value;
}

void bhkRigidBodyIfcHelper::SetRestitution (float value, TimeValue time)
{
   rbpblock->SetValue(PB_RB_RESTITUTION,time,value);
}

float bhkRigidBodyIfcHelper::GetRestitution (TimeValue time, Interval& valid) const 
{
   float value = NP_DEFAULT_HVK_RESTITUTION;
   rbpblock->GetValue(PB_RB_RESTITUTION,time,value,valid);
   return value;
}

void bhkRigidBodyIfcHelper::SetLinearDamping(float value, TimeValue time)
{
   rbpblock->SetValue(PB_RB_LINEAR_DAMPING,time,value);
}

float bhkRigidBodyIfcHelper::GetLinearDamping(TimeValue time, Interval& valid) const 
{
   float value = NP_DEFAULT_HVK_LINEAR_DAMPING;
   rbpblock->GetValue(PB_RB_LINEAR_DAMPING,time,value,valid);
   return value;
}

void bhkRigidBodyIfcHelper::SetAngularDamping(float value, TimeValue time)
{
   rbpblock->SetValue(PB_RB_ANGULAR_DAMPING,time,value);
}

float bhkRigidBodyIfcHelper::GetAngularDamping(TimeValue time, Interval& valid) const 
{
   float value = NP_DEFAULT_HVK_ANGULAR_DAMPING;
   rbpblock->GetValue(PB_RB_ANGULAR_DAMPING,time,value,valid);
   return value;
}

void bhkRigidBodyIfcHelper::SetMaxLinearVelocity(float value, TimeValue time)
{
   rbpblock->SetValue(PB_RB_MAX_LINEAR_VELOCITY,time,value);
}

float bhkRigidBodyIfcHelper::GetMaxLinearVelocity(TimeValue time, Interval& valid) const 
{
   float value = NP_DEFAULT_HVK_MAX_LINEAR_VELOCITY;
   rbpblock->GetValue(PB_RB_MAX_LINEAR_VELOCITY,time,value,valid);
   return value;
}

void bhkRigidBodyIfcHelper::SetMaxAngularVelocity(float value, TimeValue time)
{
   rbpblock->SetValue(PB_RB_MAX_ANGULAR_VELOCITY,time,value);
}
float bhkRigidBodyIfcHelper::GetMaxAngularVelocity(TimeValue time, Interval& valid) const 
{
   float value = NP_DEFAULT_HVK_MAX_ANGULAR_VELOCITY;
   rbpblock->GetValue(PB_RB_MAX_ANGULAR_VELOCITY,time,value,valid);
   return value;
}

void bhkRigidBodyIfcHelper::SetPenetrationDepth(float value, TimeValue time)
{
   rbpblock->SetValue(PB_RB_PENETRATION_DEPTH,time,value);
}
float bhkRigidBodyIfcHelper::GetPenetrationDepth(TimeValue time, Interval& valid) const 
{
   float value = NP_DEFAULT_HVK_PENETRATION_DEPTH;
   rbpblock->GetValue(PB_RB_PENETRATION_DEPTH,time,value,valid);
   return value;
}

void bhkRigidBodyIfcHelper::SetMotionSystem(int value, TimeValue time)
{
   rbpblock->SetValue(PB_RB_MOTION_SYSTEM,time,value);
}
int bhkRigidBodyIfcHelper::GetMotionSystem(TimeValue time, Interval& valid) const 
{
   int value = NP_DEFAULT_HVK_MOTION_SYSTEM;
   rbpblock->GetValue(PB_RB_MOTION_SYSTEM,time,value,valid);
   return value;
}

void bhkRigidBodyIfcHelper::SetQualityType(int value, TimeValue time)
{
   rbpblock->SetValue(PB_RB_QUALITY_TYPE,time,value);
}
int bhkRigidBodyIfcHelper::GetQualityType(TimeValue time, Interval& valid) const 
{
   int value = NP_DEFAULT_HVK_QUALITY_TYPE;
   rbpblock->GetValue(PB_RB_QUALITY_TYPE,time,value,valid);
   return value;
}

void bhkRigidBodyIfcHelper::SetEnableTransform(BOOL value, TimeValue time)
{
	rbpblock->SetValue(PB_RB_ENABLE_TRANS,time,value);
}

BOOL bhkRigidBodyIfcHelper::GetEnableTransform(TimeValue time, Interval& valid) const
{
	BOOL value = TRUE;
	rbpblock->GetValue(PB_RB_ENABLE_TRANS,time,value,valid);
	return value;
}

const USHORT kRBChunkSelLevel = 0x8000;
IOResult bhkRigidBodyIfcHelper::RBSave(ISave *isave)
{
	IOResult res;

	isave->BeginChunk(kRBChunkSelLevel);
	res = rbpblock->Save(isave);
	isave->EndChunk();

	return res;
}

IOResult bhkRigidBodyIfcHelper::RBLoad(ILoad *iload)
{
	IOResult res;
	while (IO_OK==(res=iload->OpenChunk())) {
		switch(iload->CurChunkID())  {
		case kRBChunkSelLevel:
			res = rbpblock->Load(iload);
			break;
		}
		iload->CloseChunk();
		if (res!=IO_OK) return res;
	}
	return IO_OK;
}
