/**********************************************************************
*<
FILE: bhkSphereObj.cpp

DESCRIPTION:	Collision Sphere Object Implementation

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
#include "surf_api.h"
#include "notify.h"
#include "macroRec.h"
#include "bhkRigidBodyInterface.h"
#include "NifGui.h"
#include "NifStrings.h"
#include "bhkHelperFuncs.h"

#ifndef _countof
#define _countof(x) (sizeof(x)/sizeof((x)[0]))
#endif

#define MAX_SEGMENTS	200
#define MIN_SEGMENTS	4

Class_ID bhkSphereObject_CLASS_ID = Class_ID(0x8d532427, BHKRIGIDBODYCLASS_DESC.PartB());

extern void BuildSphere(Mesh&mesh, float radius, int segs, int smooth, float startAng);

class bhkSphereObject : public SimpleObject2
{
public:			
   // Class vars
   static IParamMap2 *pmapParam;
   static IObjParam *ip;

   bhkSphereObject(BOOL loading);		
   ~bhkSphereObject();		

   // From Object
   int CanConvertToType(Class_ID obtype);
   Object* ConvertToType(TimeValue t, Class_ID obtype);
   void GetCollapseTypes(Tab<Class_ID> &clist,Tab<TSTR*> &nlist);

   CreateMouseCallBack* GetCreateMouseCallBack();
   void BeginEditParams( IObjParam  *ip, ULONG flags,Animatable *prev);
   void EndEditParams( IObjParam *ip, ULONG flags,Animatable *next);
   RefTargetHandle Clone(RemapDir& remap);
   TCHAR *GetObjectName() { return GetString(IDS_RB_SPHERE); }

   // From GeomObject
   int IntersectRay(TimeValue t, Ray& ray, float& at, Point3& norm);

   // Animatable methods		
   void DeleteThis() {delete this;}
   Class_ID ClassID() { return bhkSphereObject_CLASS_ID; } 
   SClass_ID SuperClassID() { return HELPER_CLASS_ID; }

   int	NumParamBlocks() { return 1; }					// return number of ParamBlocks in this instance
   IParamBlock2* GetParamBlock(int i) { return pblock2; } // return i'th ParamBlock
   IParamBlock2* GetParamBlockByID(BlockID id) { return (pblock2->ID() == id) ? pblock2 : NULL; } // return id'd ParamBlock

   // From SimpleObject
   void BuildMesh(TimeValue t);
   BOOL OKtoDisplay(TimeValue t);
   void InvalidateUI();

   void UpdateUI();
   int Display(TimeValue t, INode* inode, ViewExp *vpt, int flags);
};


//--- ClassDescriptor and class vars ---------------------------------

// The class descriptor for sphere
class bhkSphereClassDesc : public ClassDesc2 
{
public:
   bhkSphereClassDesc();
   int 			   IsPublic() { return 1; }
   void *			Create(BOOL loading = FALSE)
   {
      return new bhkSphereObject(loading);
   }
   const TCHAR *	ClassName() { return GetString(IDS_RB_SPHERE_CLASS); }
   SClass_ID		SuperClassID() { return HELPER_CLASS_ID; }
   Class_ID		   ClassID() { return bhkSphereObject_CLASS_ID; }
   const TCHAR* 	Category() { return "NifTools"; }

   const TCHAR*	InternalName() { return _T("bhkSphere"); }	// returns fixed parsable name (scripter-visible name)
   HINSTANCE		HInstance() { return hInstance; }			// returns owning module handle
};

extern ClassDesc2* GetbhkSphereDesc();

class SphereObjCreateCallBack : public CreateMouseCallBack {
   IPoint2 sp0;
   bhkSphereObject *ob;
   Point3 p0;
public:
   int proc( ViewExp *vpt,int msg, int point, int flags, IPoint2 m, Matrix3& mat);
   void SetObj(bhkSphereObject *obj) {ob = obj;}
};
static SphereObjCreateCallBack sphereCreateCB;

// in prim.cpp  - The dll instance handle
extern HINSTANCE hInstance;

IParamMap2 *bhkSphereObject::pmapParam  = NULL;
IObjParam *bhkSphereObject::ip         = NULL;


//--- Parameter map/block descriptors -------------------------------

enum { sphere_params, };

// Parameter block indices
enum SphereParamIndicies
{
   PB_MATERIAL,
   PB_RADIUS,
   PB_SEGS,
   PB_SMOOTH,
};

enum { box_params_panel, };

static ParamBlockDesc2 param_blk ( 
    sphere_params, _T("bhkSphereParameters"),  0, NULL, P_AUTO_CONSTRUCT|P_AUTO_UI, 0,
    //rollout
    IDD_SPHEREPARAM2, IDS_PARAMS, 0, 0, NULL, 

    // params
    PB_MATERIAL, _T("material"), TYPE_INT, P_ANIMATABLE,	IDS_DS_MATERIAL,
       p_default,	NP_INVALID_HVK_MATERIAL,
       end,

    PB_RADIUS, _T("radius"), TYPE_FLOAT, P_ANIMATABLE,	IDS_RB_RADIUS,
       p_default,	   0.0,
       p_range,		float(0), float(1.0E30),
       p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_RADIUS, IDC_RADSPINNER, SPIN_AUTOSCALE,
       end,

    PB_SEGS, _T("segments"), TYPE_INT, P_ANIMATABLE,	IDS_RB_SEGS,
       p_default,	   16,
       p_range,		MIN_SEGMENTS, MAX_SEGMENTS,
       p_ui, TYPE_SPINNER, EDITTYPE_POS_INT, IDC_SEGMENTS, IDC_SEGSPINNER, 1.0f,
       end,

    PB_SMOOTH, _T("smooth"), TYPE_INT, P_ANIMATABLE,	IDS_RB_SMOOTH,
       p_default,	   TRUE,
       p_ui, TYPE_SINGLECHEKBOX, IDC_OBSMOOTH,
       end,

    end
    );

// static ClassDesc must be declared after static paramblock
static bhkSphereClassDesc sphereDesc;
extern ClassDesc2* GetbhkSphereDesc() { return &sphereDesc; }
bhkSphereClassDesc::bhkSphereClassDesc() {
   param_blk.SetClassDesc(this);
}

class SphereParamDlgProc : public ParamMap2UserDlgProc {
public:
   bhkSphereObject *so;
   HWND thishWnd;
   NpComboBox		mCbMaterial;

   SphereParamDlgProc(bhkSphereObject *s) {so=s;thishWnd=NULL;}
   INT_PTR DlgProc(TimeValue t,IParamMap2 *map,HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam);
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

void SphereParamDlgProc::Update(TimeValue t)
{
   if (!thishWnd) 
      return;
   return;
}

INT_PTR SphereParamDlgProc::DlgProc(TimeValue t,IParamMap2 *map,HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
   thishWnd=hWnd;
   switch (msg) 
   {
   case WM_INITDIALOG: 
      {
		  mCbMaterial.init(GetDlgItem(hWnd, IDC_CB_MATERIAL));
		  mCbMaterial.add("<Default>");
		  for (const char **str = NpHvkMaterialNames; *str; ++str)
			  mCbMaterial.add(*str);
		  Interval valid;
		  int sel = NP_INVALID_HVK_MATERIAL;
		  so->pblock2->GetValue( PB_MATERIAL, 0, sel, valid);
		  mCbMaterial.select( sel + 1 );

         Update(t);
         break;
      }
   case WM_COMMAND:
      switch (LOWORD(wParam)) 
      {
      case IDC_CB_MATERIAL:
         if (HIWORD(wParam)==CBN_SELCHANGE) {
            so->pblock2->SetValue( PB_MATERIAL, 0, mCbMaterial.selection() - 1 );
         }
         break;
      }
      break;	
   }
   return FALSE;
}

//--- Sphere methods -------------------------------


bhkSphereObject::bhkSphereObject(BOOL loading)
{
   SetAFlag(A_PLUGIN1);
   sphereDesc.MakeAutoParamBlocks(this);
   assert(pblock2);
}

bhkSphereObject::~bhkSphereObject() {
   param_blk.SetUserDlgProc();
   sphereCreateCB.SetObj(NULL);
   if (pmapParam) {
      pmapParam  = NULL;
   }
}

void bhkSphereObject::BeginEditParams(IObjParam *ip,ULONG flags,Animatable *prev)
{
   SimpleObject::BeginEditParams(ip,flags,prev);

   // Gotta make a new one.
   //if (NULL == pmapParam) 
   //{
   //   pmapParam = CreateCPParamMap2(
   //      0,
   //      pblock2,
   //      GetCOREInterface(),
   //      hInstance,
   //      MAKEINTRESOURCE(IDD_SPHEREPARAM2),
   //      GetString(IDS_RB_PARAMETERS),
   //      0);
   //}
   this->ip = ip;
   sphereDesc.BeginEditParams(ip,this,flags,prev);
   param_blk.SetUserDlgProc(new SphereParamDlgProc(this));
   pmapParam = pblock2->GetMap(sphere_params);
   //if(pmapParam) {
   //   // A callback for the type in.
   //   pmapParam->SetUserDlgProc(new SphereParamDlgProc(this));
   //}
}

void bhkSphereObject::EndEditParams( IObjParam *ip, ULONG flags,Animatable *next )
{		
   param_blk.SetUserDlgProc();

   SimpleObject::EndEditParams(ip,flags,next);
   this->ip = NULL;
   pmapParam = NULL;

   //if (pmapParam && flags&END_EDIT_REMOVEUI ) {
   //   DestroyCPParamMap2(pmapParam);
   //   pmapParam  = NULL;
   //}
   // tear down the appropriate auto-rollouts
   sphereDesc.EndEditParams(ip, this, flags, next);
}

void bhkSphereObject::BuildMesh(TimeValue t)
{
   float radius; int segs; int smooth;
   float startAng = 0.0f;
   if (TestAFlag(A_PLUGIN1)) startAng = HALFPI;

   // Start the validity interval at forever and whittle it down.
   ivalid = FOREVER;
   pblock2->GetValue(PB_RADIUS, t, radius, ivalid);
   pblock2->GetValue(PB_SEGS, t, segs, ivalid);
   pblock2->GetValue(PB_SMOOTH, t, smooth, ivalid);
   BuildSphere(mesh, (radius * 7.0f), segs, smooth, startAng);
}



Object* bhkSphereObject::ConvertToType(TimeValue t, Class_ID obtype)
{
   return 0;
   //return SimpleObject::ConvertToType(t,obtype);
}

int bhkSphereObject::CanConvertToType(Class_ID obtype)
{
   return 0;
}


void bhkSphereObject::GetCollapseTypes(Tab<Class_ID> &clist,Tab<TSTR*> &nlist)
{
   Object::GetCollapseTypes(clist, nlist);
}

int SphereObjCreateCallBack::proc(ViewExp *vpt,int msg, int point, int flags, IPoint2 m, Matrix3& mat ) 
{
   float r;
   Point3 p1,center;

   if (msg == MOUSE_FREEMOVE)
   {
      vpt->SnapPreview(m,m,NULL, SNAP_IN_3D);
   }

   if (msg==MOUSE_POINT||msg==MOUSE_MOVE) 
   {
      switch(point) 
      {
      case 0:  // only happens with MOUSE_POINT msg
         // Find the node and plug in the wire color
         {
            ULONG handle;
            ob->NotifyDependents(FOREVER, (PartID)&handle, REFMSG_GET_NODE_HANDLE);
            INode *node = GetCOREInterface()->GetINodeByHandle(handle);
            if (node) node->SetWireColor(RGB(255, 0, 0));
         }
         ob->pblock2->SetValue(PB_RADIUS,0,0.0f);
         ob->suspendSnap = TRUE;				
         sp0 = m;
         p0 = vpt->SnapPoint(m,m,NULL,SNAP_IN_3D);
         mat.SetTrans(p0);
         break;
      case 1:
         mat.IdentityMatrix();
         //mat.PreRotateZ(HALFPI);
         p1 = vpt->SnapPoint(m,m,NULL,SNAP_IN_3D);
         r = Length(p1-p0) / 7.0f;
         mat.SetTrans(p0);

         ob->pblock2->SetValue(PB_RADIUS,0,r);
         ob->pmapParam->Invalidate();

         if (flags&MOUSE_CTRL) 
         {
            float ang = (float)atan2(p1.y-p0.y,p1.x-p0.x);					
            mat.PreRotateZ(ob->ip->SnapAngle(ang));
         }

         if (msg==MOUSE_POINT) 
         {
            ob->suspendSnap = FALSE;
            return (Length(m-sp0)<3 || Length(p1-p0)<0.1f)?CREATE_ABORT:CREATE_STOP;
         }
         break;					   
      }
   }
   else
      if (msg == MOUSE_ABORT) 
      {		
         return CREATE_ABORT;
      }

      return TRUE;
}

CreateMouseCallBack* bhkSphereObject::GetCreateMouseCallBack() 
{
   sphereCreateCB.SetObj(this);
   return(&sphereCreateCB);
}


BOOL bhkSphereObject::OKtoDisplay(TimeValue t) 
{
   float radius;
   pblock2->GetValue(PB_RADIUS,t,radius,FOREVER);
   if (radius==0.0f) return FALSE;
   else return TRUE;
}

// From GeomObject
int bhkSphereObject::IntersectRay(TimeValue t, Ray& ray, float& at, Point3& norm)
{
   int smooth;
   pblock2->GetValue(PB_SMOOTH,t,smooth,FOREVER);

   float r;
   float a, b, c, ac4, b2, at1, at2;
   float root;
   BOOL neg1, neg2;

   pblock2->GetValue(PB_RADIUS,t,r,FOREVER);

   a = DotProd(ray.dir,ray.dir);
   b = DotProd(ray.dir,ray.p) * 2.0f;
   c = DotProd(ray.p,ray.p) - r*r;

   ac4 = 4.0f * a * c;
   b2 = b*b;

   if (ac4 > b2) return 0;

   // We want the smallest positive root
   root = float(sqrt(b2-ac4));
   at1 = (-b + root) / (2.0f * a);
   at2 = (-b - root) / (2.0f * a);
   neg1 = at1<0.0f;
   neg2 = at2<0.0f;
   if (neg1 && neg2) 
      return 0;
   else if (neg1 && !neg2) 
      at = at2;
   else if (!neg1 && neg2) 
      at = at1;
   else if (at1<at2) 
      at = at1;
   else 
      at = at2;
   norm = Normalize(ray.p + at*ray.dir);
   return 1;
}

void bhkSphereObject::InvalidateUI() 
{
   if (pmapParam) pmapParam->Invalidate();
}


RefTargetHandle bhkSphereObject::Clone(RemapDir& remap) 
{
   bhkSphereObject* newob = new bhkSphereObject(FALSE);	
   newob->ReplaceReference(0,remap.CloneRef(pblock));
   newob->ivalid.SetEmpty();	
   BaseClone(this, newob, remap);
   return(newob);
}

void bhkSphereObject::UpdateUI()
{
   if (ip == NULL)
      return;
   SphereParamDlgProc* dlg = static_cast<SphereParamDlgProc*>(pmapParam->GetUserDlgProc());
   dlg->Update(ip->GetTime());
}

int bhkSphereObject::Display(TimeValue t, INode* inode, ViewExp *vpt, int flags) 
{
   Matrix3 m;
   Color color = Color(inode->GetWireColor());
   GraphicsWindow *gw = vpt->getGW();
   Material *mtl = gw->getMaterial();
   m = inode->GetObjectTM(t);
   gw->setTransform(m);
   DWORD rlim = gw->getRndLimits();

   DWORD newrlim = GW_WIREFRAME/*|GW_Z_BUFFER*/;
#if VERSION_3DSMAX >= ((5000<<16)+(15<<8)+0) // Version 5+
   newrlim |= GW_EDGES_ONLY;
#endif
   gw->setRndLimits(newrlim);

   if (inode->Selected()) 
      gw->setColor( LINE_COLOR, GetSelColor());
   else if(!inode->IsFrozen() && !inode->Dependent())
      gw->setColor( LINE_COLOR, color);

   UpdateMesh(t);
   mesh.render( gw, mtl, NULL, COMP_ALL);	
   gw->setRndLimits(rlim);
   return 0;
}