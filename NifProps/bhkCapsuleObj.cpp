/**********************************************************************
*<
FILE: bhkCapsuleObj.cpp

DESCRIPTION:	Collision Capsule Object Implementation

CREATED BY: tazpn (Theo)

HISTORY: 
  V1.0 - Derived from 3ds max prim sphere  example

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
#ifndef _countof
#define _countof(x) (sizeof(x)/sizeof((x)[0]))
#endif

static void BuildScubaMesh(Mesh &mesh, int segs, int smooth, int llsegs, 
                    float radius1, float radius2, Point3 cap1, Point3 cap2);

const Class_ID BHKCAPSULEOBJECT_CLASS_ID = Class_ID(0x7f8f629a, 0x1d88470a);

class bhkCapsuleObject : public SimpleObject, public IParamArray, public bhkRigidBodyIfcHelper
{
public:			
   // Class vars
   static IParamMap *pmapParam;
   static float crtRadius;
   static float crtRadius1;
   static float crtRadius2;
   static Point3 crtCapPos1;
   static Point3 crtCapPos2;
   static IObjParam *ip;

   bhkCapsuleObject(BOOL loading);		
   ~bhkCapsuleObject();		

   // From Object
   int CanConvertToType(Class_ID obtype);
   Object* ConvertToType(TimeValue t, Class_ID obtype);
   void GetCollapseTypes(Tab<Class_ID> &clist,Tab<TSTR*> &nlist);

   CreateMouseCallBack* GetCreateMouseCallBack();
   void BeginEditParams( IObjParam  *ip, ULONG flags,Animatable *prev);
   void EndEditParams( IObjParam *ip, ULONG flags,Animatable *next);
   RefTargetHandle Clone(RemapDir& remap);
   TCHAR *GetObjectName() { return GetString(IDS_RB_Capsule); }

   // Animatable methods		
   void DeleteThis() {delete this;}
   Class_ID ClassID() { return BHKCAPSULEOBJECT_CLASS_ID; } 
   SClass_ID SuperClassID() { return HELPER_CLASS_ID; }

   // From ReferenceTarget
   IOResult Load(ILoad *iload);
   IOResult Save(ISave *isave);

   // From ref
   int NumRefs() {return 2;}
   RefTargetHandle GetReference(int i);
   void SetReference(int i, RefTargetHandle rtarg);

   // From SimpleObject
   void BuildMesh(TimeValue t);
   BOOL OKtoDisplay(TimeValue t);
   void InvalidateUI();
   ParamDimension *GetParameterDim(int pbIndex);
   TSTR GetParameterName(int pbIndex);		

   void UpdateUI();

   BaseInterface* GetInterface(Interface_ID id);

};

class CapsuleObjCreateCallBack : public CreateMouseCallBack {
   IPoint2 sp[4];
   bhkCapsuleObject *ob;
   Point3 p[4];
public:
   int proc( ViewExp *vpt,int msg, int point, int flags, IPoint2 m, Matrix3& mat);
   void SetObj(bhkCapsuleObject *obj) {ob = obj;}
};
static CapsuleObjCreateCallBack CapsuleCreateCB;

// Misc stuff
#define MAX_SEGMENTS	200
#define MIN_SEGMENTS	4

#define MIN_RADIUS		float(0)
#define MAX_RADIUS		float(1.0E30)

#define MIN_SMOOTH		0
#define MAX_SMOOTH		1

#define DEF_SEGMENTS	32	// 16
#define DEF_RADIUS		float(0.0)

#define SMOOTH_ON	1
#define SMOOTH_OFF	0

#define MIN_SLICE	float(-1.0E30)
#define MAX_SLICE	float( 1.0E30)


//--- ClassDescriptor and class vars ---------------------------------

static BOOL sInterfaceAdded = FALSE;

// The class descriptor for Capsule
class bhkCapsuleClassDesc : public ClassDesc2 
{
public:
   int 			   IsPublic() { return 1; }
   void *			Create(BOOL loading = FALSE)
   {
      AddInterface (GetbhkRigidBodyInterfaceDesc());
      return new bhkCapsuleObject(loading);
   }
   const TCHAR *	ClassName() { return GetString(IDS_RB_CAPSULE_CLASS); }
   SClass_ID		SuperClassID() { return HELPER_CLASS_ID; }
   Class_ID		   ClassID() { return BHKCAPSULEOBJECT_CLASS_ID; }
   const TCHAR* 	Category() { return "NifTools"; }
   void			   ResetClassParams(BOOL fileReset);
};

static bhkCapsuleClassDesc CapsuleDesc;
extern ClassDesc2* GetbhkCapsuleDesc() { return &CapsuleDesc; }

// in prim.cpp  - The dll instance handle
extern HINSTANCE hInstance;

IParamMap *bhkCapsuleObject::pmapParam  = NULL;
IObjParam *bhkCapsuleObject::ip         = NULL;
float bhkCapsuleObject::crtRadius1      = 0.0f;
float bhkCapsuleObject::crtRadius2      = 0.0f;
Point3 bhkCapsuleObject::crtCapPos1;
Point3 bhkCapsuleObject::crtCapPos2;


void bhkCapsuleClassDesc::ResetClassParams(BOOL fileReset)
{
   bhkCapsuleObject::crtRadius1      = 0.0f;
   bhkCapsuleObject::crtRadius2      = 0.0f;
   bhkCapsuleObject::crtCapPos1.Set(0,0,0);
   bhkCapsuleObject::crtCapPos2.Set(0,0,0);
}


//--- Parameter map/block descriptors -------------------------------

// Parameter block indices
enum CapsuleParamIndicies
{
   PB_RADIUS1,
   PB_RADIUS2,
   PB_CAP_POS1,
   PB_CAP_POS2,
};


//
//
// Parameters

static ParamUIDesc descParam[] = {
   // Radius 1
   ParamUIDesc(
   PB_RADIUS1,
   EDITTYPE_UNIVERSE,
   IDC_RADIUS1,IDC_RADSPINNER1,
   MIN_RADIUS,MAX_RADIUS,
   SPIN_AUTOSCALE),	

   // Radius 2
   ParamUIDesc(
   PB_RADIUS2,
   EDITTYPE_UNIVERSE,
   IDC_RADIUS2,IDC_RADSPINNER2,
   MIN_RADIUS,MAX_RADIUS,
   SPIN_AUTOSCALE),	

   ParamUIDesc(
   PB_CAP_POS1,
   EDITTYPE_UNIVERSE,
   IDC_ED_POS1_X,IDC_SP_POS1_X,
   IDC_ED_POS1_Y,IDC_SP_POS1_Y,
   IDC_ED_POS1_Z,IDC_SP_POS1_Z,
   float(-1.0E30),float(1.0E30),
   SPIN_AUTOSCALE),

   ParamUIDesc(
   PB_CAP_POS2,
   EDITTYPE_UNIVERSE,
   IDC_ED_POS2_X,IDC_SP_POS2_X,
   IDC_ED_POS2_Y,IDC_SP_POS2_Y,
   IDC_ED_POS2_Z,IDC_SP_POS2_Z,
   float(-1.0E30),float(1.0E30),
   SPIN_AUTOSCALE),

};
const int PARAMDESC_LENGTH = _countof(descParam);

static ParamBlockDescID descVer0[] = {
   { TYPE_FLOAT, NULL, TRUE, 0 },		
   { TYPE_FLOAT, NULL, TRUE, 1 },		
   { TYPE_POINT3, NULL, TRUE, 2 },
   { TYPE_POINT3, NULL, TRUE, 3 } 
};
const int PBLOCK_LENGTH = _countof(descVer0);
static ParamBlockDescID *curDescVer = descVer0;

// Array of old versions
//static ParamVersionDesc versions[] = {
//   ParamVersionDesc(descVer0,_countof(descVer0),0),
//};
//const int NUM_OLDVERSIONS = _countof(versions);
static ParamVersionDesc* versions = NULL;
const int NUM_OLDVERSIONS = 0;

// Current version
const int CURRENT_VERSION = NUM_OLDVERSIONS + 1;
static ParamVersionDesc curVersion(descVer0,_countof(descVer0),CURRENT_VERSION);

class CapsuleParamDlgProc : public ParamMapUserDlgProc {
public:
   bhkCapsuleObject *so;
   HWND thishWnd;

   CapsuleParamDlgProc(bhkCapsuleObject *s) {so=s;thishWnd=NULL;}
   BOOL DlgProc(TimeValue t,IParamMap *map,HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam);
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

void CapsuleParamDlgProc::Update(TimeValue t)
{
   if (!thishWnd) 
      return;
   return;
}

BOOL CapsuleParamDlgProc::DlgProc(TimeValue t,IParamMap *map,HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
   thishWnd=hWnd;
   switch (msg) 
   {
   case WM_INITDIALOG: 
      {
         Update(t);
         break;
      }
   case WM_COMMAND:
      //switch (LOWORD(wParam)) 
      //{
      //}
      break;	
   }
   return FALSE;
}

//--- Capsule methods -------------------------------


bhkCapsuleObject::bhkCapsuleObject(BOOL loading)
{
   SetAFlag(A_PLUGIN1);
   ReplaceReference(0, CreateParameterBlock(curDescVer, PBLOCK_LENGTH, CURRENT_VERSION));
   assert(pblock);
   ReplaceReference(1, GetRBBlock());

   pblock->SetValue(PB_RADIUS1,0,crtRadius1);
   pblock->SetValue(PB_RADIUS2,0,crtRadius2);
   pblock->SetValue(PB_CAP_POS1,0,crtCapPos1);
   pblock->SetValue(PB_CAP_POS2,0,crtCapPos2);
}

bhkCapsuleObject::~bhkCapsuleObject() 
{
   CapsuleCreateCB.SetObj(NULL);
   if (pmapParam) {
      pmapParam->SetUserDlgProc(NULL);
      DestroyCPParamMap(pmapParam);
      pmapParam  = NULL;
   }
}

#define NEWMAP_CHUNKID	0x0100

IOResult bhkCapsuleObject::Load(ILoad *iload) 
{
   ClearAFlag(A_PLUGIN1);

   IOResult res;
   while (IO_OK==(res=iload->OpenChunk())) 
   {
      switch (iload->CurChunkID()) 
      {	
      case NEWMAP_CHUNKID:
         SetAFlag(A_PLUGIN1);
         break;
      }
      iload->CloseChunk();
      if (res!=IO_OK)  return res;
   }
   return IO_OK;
}

IOResult bhkCapsuleObject::Save(ISave *isave)
{
   if (TestAFlag(A_PLUGIN1)) {
      isave->BeginChunk(NEWMAP_CHUNKID);
      isave->EndChunk();
   }
   return IO_OK;
}

RefTargetHandle bhkCapsuleObject::GetReference(int i) 
{
   if (i == 1) 
      return GetRBBlock();
   return pblock;
}

void bhkCapsuleObject::SetReference(int i, RefTargetHandle rtarg) 
{
   if (i == 1)
      return;
   pblock=(IParamBlock*)rtarg;
}

BaseInterface* bhkCapsuleObject::GetInterface(Interface_ID id)
{
   if (id == BHKRIGIDBODYINTERFACE_DESC)
      return this;
   return SimpleObject::GetInterface(id);
}



void bhkCapsuleObject::BeginEditParams(IObjParam *ip,ULONG flags,Animatable *prev)
{
   SimpleObject::BeginEditParams(ip,flags,prev);

   // Gotta make a new one.
   if (NULL == pmapParam) 
   {
      pmapParam = CreateCPParamMap(
         descParam,PARAMDESC_LENGTH,
         pblock,
         ip,
         hInstance,
         MAKEINTRESOURCE(IDD_CAPSULEPARAM),
         GetString(IDS_RB_PARAMETERS),
         0);
   }
   this->ip = ip;

   if(pmapParam) {
      // A callback for the type in.
      pmapParam->SetUserDlgProc(new CapsuleParamDlgProc(this));
   }
   BeginEditRBParams(ip, flags, prev);
}

void bhkCapsuleObject::EndEditParams( IObjParam *ip, ULONG flags,Animatable *next )
{		
   SimpleObject::EndEditParams(ip,flags,next);
   this->ip = NULL;

   if (pmapParam && flags&END_EDIT_REMOVEUI ) {
      pmapParam->SetUserDlgProc(NULL);
      DestroyCPParamMap(pmapParam);
      pmapParam  = NULL;
   }
   EndEditRBParams(ip, flags, next);
}

void bhkCapsuleObject::BuildMesh(TimeValue t)
{
   int segs = 12;
   int hsegs = 1;
   int smooth = 1;

   // Start the validity interval at forever and widdle it down.
   //FixHeight(pblock,t,(pmapParam?pmapParam->GetHWnd():NULL),increate);
   ivalid = FOREVER;

   float radius1, radius2;
   Point3 pos1, pos2;
   pblock->GetValue(PB_RADIUS1,t,radius1,ivalid);
   pblock->GetValue(PB_RADIUS2,t,radius2,ivalid);
   pblock->GetValue(PB_CAP_POS1,t,pos1,ivalid);
   pblock->GetValue(PB_CAP_POS2,t,pos2,ivalid);
   LimitValue(radius1, MIN_RADIUS, MAX_RADIUS);
   LimitValue(radius2, MIN_RADIUS, MAX_RADIUS);
   if (radius2 == MIN_RADIUS) radius2 = radius1;

   if (radius1 == 0)
   {
      mesh.setNumVerts(0);
      mesh.setNumFaces(0);
      mesh.setNumTVerts(0);
      mesh.setNumTVFaces(0);
      mesh.setSmoothFlags(smooth != 0);
   }
   else
   {
      BuildScubaMesh(mesh, segs, smooth, hsegs, radius2, radius1, pos2, pos1);
   }
}

Object* bhkCapsuleObject::ConvertToType(TimeValue t, Class_ID obtype)
{
   return 0;
   //return SimpleObject::ConvertToType(t,obtype);
}

int bhkCapsuleObject::CanConvertToType(Class_ID obtype)
{
   return 0;
}

void bhkCapsuleObject::GetCollapseTypes(Tab<Class_ID> &clist,Tab<TSTR*> &nlist)
{
   Object::GetCollapseTypes(clist, nlist);
}

int CapsuleObjCreateCallBack::proc(ViewExp *vpt,int msg, int point, int flags, IPoint2 m, Matrix3& mat ) 
{
   float r;
   Point3 center;
   if (ob == NULL)
   {
      return CREATE_ABORT;
   }

   if (msg == MOUSE_FREEMOVE)
   {
      vpt->SnapPreview(m,m,NULL, SNAP_IN_3D);
   }

   if (msg==MOUSE_POINT||msg==MOUSE_MOVE) 
   {
      switch(point) 
      {
      case 0:  // only happens with MOUSE_POINT msg
         ob->pblock->SetValue(PB_RADIUS1,0,0.0f);
         ob->pblock->SetValue(PB_RADIUS2,0,0.0f);
         ob->suspendSnap = TRUE;				
         sp[0] = m;
         p[0] = vpt->SnapPoint(m,m,NULL,SNAP_IN_3D);
         ob->pblock->SetValue(PB_CAP_POS1,0,p[0]);
         ob->pblock->SetValue(PB_CAP_POS2,0,p[0]);
         mat.SetTrans(p[0]);
         break;

      case 1: // Fix radius of first cap
         mat.IdentityMatrix();
         //mat.PreRotateZ(HALFPI);
         sp[1] = m;
         p[1] = vpt->SnapPoint(m,m,NULL,SNAP_IN_3D);
         r = Length(p[1]-p[0]);
         mat.SetTrans(p[0]);

         ob->pblock->SetValue(PB_RADIUS1,0,r);
         ob->pblock->SetValue(PB_RADIUS2,0,r);
         ob->pmapParam->Invalidate();

         if (flags&MOUSE_CTRL) 
         {
            float ang = (float)atan2(p[1].y-p[0].y,p[1].x-p[0].x);					
            mat.PreRotateZ(ob->ip->SnapAngle(ang));
         }
         if (msg==MOUSE_POINT) 
         {
            if (Length(m-sp[0])<3 || Length(p[1]-p[0])<0.1f) {
               ob->suspendSnap = FALSE;
               return CREATE_ABORT;
            }
         }
         break;

      case 2: // Get second point
         mat.IdentityMatrix();
         //mat.PreRotateZ(HALFPI);
         sp[2] = m;
         p[2] = vpt->SnapPoint(m,m,NULL,SNAP_IN_3D);
         r = Length(p[1]-p[0]);
         mat.SetTrans(p[0]);

         ob->pblock->SetValue(PB_CAP_POS2,0,p[2]);
         ob->pmapParam->Invalidate();

         //if (flags&MOUSE_CTRL) 
         //{
         //   float ang = (float)atan2(p1.y-p[0].y,p1.x-p[0].x);					
         //   mat.PreRotateZ(ob->ip->SnapAngle(ang));
         //}
         //if (msg==MOUSE_POINT) 
         //{
         //   ob->suspendSnap = FALSE;
         //   return (Length(m-sp[0])<3 || Length(p1-p[0])<0.1f)?CREATE_ABORT:CREATE_STOP;
         //}
         break;

      case 3: // Get second point
         mat.IdentityMatrix();
         //mat.PreRotateZ(HALFPI);
         sp[3] = m;
         p[3] = vpt->SnapPoint(m,m,NULL,SNAP_IN_3D);
         if (flags&MOUSE_CTRL) // ignore radius
         {
            r = Length(p[1]-p[0]);
            ob->pblock->SetValue(PB_RADIUS2,0,r);
         }
         else
         {
            // start radius at r1
            r = Length((p[3]-p[2]) + (p[1]-p[0]));
            ob->pblock->SetValue(PB_RADIUS2,0,r);
         }
         ob->pmapParam->Invalidate();
         if (msg==MOUSE_POINT) 
         {
            ob->suspendSnap = FALSE;
            return CREATE_STOP;
         }
         break;
      }
   }
   else if (msg == MOUSE_ABORT) 
   {		
      ob->suspendSnap = FALSE;
      return CREATE_ABORT;
   }
   return TRUE;
}

CreateMouseCallBack* bhkCapsuleObject::GetCreateMouseCallBack() 
{
   CapsuleCreateCB.SetObj(this);
   return(&CapsuleCreateCB);
}


BOOL bhkCapsuleObject::OKtoDisplay(TimeValue t) 
{
   float radius;
   pblock->GetValue(PB_RADIUS1,t,radius,FOREVER);
   if (radius==0.0f) return FALSE;
   else return TRUE;
}

void bhkCapsuleObject::InvalidateUI() 
{
   if (pmapParam) pmapParam->Invalidate();
}

ParamDimension *bhkCapsuleObject::GetParameterDim(int pbIndex) 
{
   switch (pbIndex) 
   {
   case PB_RADIUS1:
      return stdWorldDim;			
   case PB_RADIUS2:
      return stdWorldDim;			
   case PB_CAP_POS1:
      return stdWorldDim;			
   case PB_CAP_POS2:
      return stdWorldDim;			
   default:
      return defaultDim;
   }
}

TSTR bhkCapsuleObject::GetParameterName(int pbIndex) 
{
   switch (pbIndex) 
   {
   case PB_RADIUS1:
      return TSTR(GetString(IDS_RB_RADIUS1));			
   case PB_RADIUS2:
      return TSTR(GetString(IDS_RB_RADIUS2));			
   case PB_CAP_POS1:
      return TSTR(GetString(IDS_RB_CAP_POS1));			
   case PB_CAP_POS2:
      return TSTR(GetString(IDS_RB_CAP_POS2));			
   default:
      return TSTR(_T(""));
   }
}

RefTargetHandle bhkCapsuleObject::Clone(RemapDir& remap) 
{
   bhkCapsuleObject* newob = new bhkCapsuleObject(FALSE);	
   newob->ReplaceReference(0,remap.CloneRef(pblock));
   newob->ivalid.SetEmpty();	
   BaseClone(this, newob, remap);
   return(newob);
}

void bhkCapsuleObject::UpdateUI()
{
   if (ip == NULL)
      return;
   CapsuleParamDlgProc* dlg = static_cast<CapsuleParamDlgProc*>(pmapParam->GetUserDlgProc());
   dlg->Update(ip->GetTime());
}

void AddFace(Face *f,int a,int b,int c,int evis,int smooth_group)
{ 
   const int ALLF = 4;
   f[0].setSmGroup(smooth_group);
   f[0].setMatID((MtlID)0); 	 /*default */
   if (evis==0) f[0].setEdgeVisFlags(1,1,0);
   else if (evis==1) f[0].setEdgeVisFlags(0,1,1);
   else if (evis==2) f[0].setEdgeVisFlags(0,0,1);
   else if (evis==ALLF) f[0].setEdgeVisFlags(1,1,1);
   else f[0].setEdgeVisFlags(1,0,1);	
   f[0].setVerts(a,b,c);
}

void BuildScubaMesh(Mesh &mesh, int segs, int smooth, int llsegs, 
                    float radius1, float radius2, Point3 cap1, Point3 cap2)
{
   Point3 p;
   int ix,jx,ic = 1;
   int nf=0,nv=0, capsegs=(int)(segs/2.0f),csegs=0;
   float ang;	
   float startAng = 0.0f;	
   float totalPie = TWOPI;
   int lsegs = llsegs-1 + 2*capsegs;
   int levels=csegs*2+(llsegs-1);
   int capv=segs,sideedge=capsegs+csegs;
   int totlevels=levels+capsegs*2+2;
   int tvinslice=totlevels+totlevels-2;
   float delta = (float)2.0*PI/(float)segs;
   int VertexPerLevel=segs;
   int nfaces=2*segs*(levels+1);
   int ntverts=2*(segs+1)+llsegs-1;
   int *edgelstl=new int[totlevels];
   int *edgelstr=new int[totlevels];
   int lastlevel=totlevels-1,dcapv=capv-1,dvertper=VertexPerLevel-1;
   edgelstr[0] = edgelstl[0] = 0;
   edgelstr[1] = 1;
   edgelstl[1] = capv;
   for (int i=2;i<=sideedge;i++){ 
      edgelstr[i]=edgelstr[i-1]+capv;
      edgelstl[i]=edgelstr[i]+dcapv;
   }
   while ((i<lastlevel)&&(i<=totlevels-sideedge)){ 
      edgelstr[i]=edgelstr[i-1]+VertexPerLevel;
      edgelstl[i]=edgelstr[i]+dcapv;
      i++;
   }
   while (i<lastlevel) { 
      edgelstr[i]=edgelstr[i-1]+capv;
      edgelstl[i]=edgelstr[i]+dcapv;
      i++;
   }
   edgelstl[lastlevel]= (edgelstr[lastlevel]=edgelstl[i-1]+1);
   int nverts=edgelstl[lastlevel]+1;
   nfaces+=2*segs*(2*capsegs-1);

   mesh.setNumVerts(nverts);
   mesh.setNumFaces(nfaces);
   mesh.setSmoothFlags(smooth != 0);
   mesh.setNumTVerts(0);
   mesh.setNumTVFaces(0);
   mesh.setSmoothFlags(smooth != 0);

   // bottom vertex 
   float cylh = (cap1 - cap2).Length();
   float height = cylh + radius1 + radius2;
   mesh.setVert(nv, Point3(0.0f,0.0f,height));
   mesh.setVert(nverts-1, Point3(0.0f,0.0f,0.0f));		

   // Top (1) and bottom (2) cap vertices
   float ru,cang,sang;
   int msegs=segs,deltaend=nverts-capv-1;
   ang = startAng;	 
   msegs--;
   float rincr=PI/(2.0f*capsegs),aincr;
   for (jx = 0; jx<=msegs; jx++) 
   {
      cang=(float)cos(ang);
      sang=(float)sin(ang);
      for(ix=1; ix<=sideedge; ix++) {
         aincr = (rincr*(float)ix);
         ru=(float)sin(aincr);

         p.x = cang*radius1*ru;
         p.y = sang*radius1*ru;	
         p.z = (jx==0) ? height-radius1*(1.0f-(float)cos(aincr)) : mesh.verts[edgelstr[ix]].z;
         mesh.setVert(edgelstr[ix]+jx, p);

         p.x = cang*radius2*ru;
         p.y = sang*radius2*ru;	
         p.z = (jx==0) ? radius2*(1.0f-(float)cos(aincr)) : mesh.verts[edgelstr[lastlevel-ix]].z ;
         mesh.setVert(edgelstr[lastlevel-ix]+jx,p);
      }
      ang += delta;
   }

   //// Middle vertices 
   //int sidevs,startv=edgelstr[sideedge],deltav;				
   //for(ix=1; ix<llsegs; ix++) {
   //   // Put center vertices all the way up
   //   float   u = float(ix)/float(llsegs);
   //   float rad = (radius1*(1.0f-u) + radius2*u);
   //   p.z = cylh *((float)ix/float(llsegs)) + radius2;
   //   ang = startAng;
   //   for (sidevs=0;sidevs<VertexPerLevel;sidevs++)
   //      p.x = (float)cos(ang)*rad;
   //      p.y = (float)sin(ang)*rad;
   //      mesh.setVert(nv, p);
   //      nv++;
   //      ang += delta;
   //   }	
   //}

   //top layer done, now reflect sides down 
   int sidevs,deltav;
   int startv=edgelstr[sideedge];
   int endv=edgelstr[totlevels-capsegs-1]; 
   if (llsegs>1)
   {
      float sincr = cylh/llsegs;
      for (sidevs=0;sidevs<VertexPerLevel;sidevs++)
      {
         Point3 topp = mesh.verts[startv];
         Point3 botp = mesh.verts[endv];
         p.x = (topp.x + botp.x) /  2.0f;
         p.y = (topp.y + botp.y) /  2.0f;
         deltav=VertexPerLevel;
         for (ic=1;ic<llsegs;ic++)
         {
            p.z = topp.z-sincr*ic;
            mesh.setVert(startv+deltav, p);
            deltav+=VertexPerLevel;
         }
         startv++;
      }
   }
   int lasttvl=0,lasttvr=0;
   int lvert=segs;
   int t0,t1,b0,b1,tvt0=0,tvt1=0,tvb0=1,tvb1=2,fc=0,smoothgr=(smooth?4:0),vseg=segs+1;
   int tvcount=0,lowerside=lastlevel-sideedge,onside=0;

   BOOL ok,wrap;
   // Now make faces ---
   for (int clevel=0;clevel<lastlevel-1;clevel++)
   {
      t1=(t0=edgelstr[clevel])+1;
      b1=(b0=edgelstr[clevel+1])+1;
      ok=1; wrap=FALSE;
      if ((clevel>0)&&(onside==1)) {
         tvt0++;tvt1++;tvb0++,tvb1++;
      }
      if (clevel==1) {
         tvt0=1;tvt1=2;
      }
      if (clevel==sideedge) {
         tvt1+=lvert;tvt0+=lvert;tvb0+=vseg;tvb1+=vseg;onside++;
      } else if (clevel==lowerside) {
         tvt1+=vseg;tvt0+=vseg;tvb0+=lvert;tvb1+=lvert;onside++;
      }
      while ((b0<edgelstl[clevel+1])||ok)
      {
         if (b1==edgelstr[clevel+2]) {
            b1=edgelstr[clevel+1]; 
            t1=edgelstr[clevel];
            ok=FALSE;
            wrap=(onside!=1);
         }
         if (smooth) smoothgr=4;
         AddFace(&mesh.faces[fc++],t0,b0,b1,0,smoothgr);
         if (clevel>0) {
            AddFace(&mesh.faces[fc++],t0,b1,t1,1,smoothgr);
            t0++;t1++;
         }
         b0++;b1++;tvb0++,tvb1++;
      }
   }
   smoothgr=(smooth?4:0);
   t1=(t0=edgelstr[lastlevel-1])+1;b0=edgelstr[lastlevel];
   int lastpt=lastlevel;
   if (onside==1) {
      tvt0++;
      tvt1++;
      tvb0++;
      tvb1++;
   }
   if (sideedge==1) {
      tvt1+=vseg;
      tvt0+=vseg;
      tvb0+=lvert;
      tvb1+=lvert;
      onside++;
   }
   while (t0<edgelstl[lastpt]) {
      if (t1==edgelstr[lastlevel]) {
         t1=edgelstr[lastlevel-1];
         tvt1-=segs;
      }
      AddFace(&mesh.faces[fc++],t0,b0,t1,1,smoothgr);
      t0++;t1++;
   }
   for (i=0;i<nverts;i++) 
      mesh.verts[i].z -= radius2;

   if (edgelstr) delete []edgelstr;
   if (edgelstl) delete []edgelstl;
   assert(fc==mesh.numFaces);
   //	assert(nv==mesh.numVerts);
   mesh.InvalidateTopologyCache();
}

#if 0
void BuildCylinderMesh(Mesh &mesh,
                       int segs, int smooth, int llsegs, int capsegs, 
                       float radius1, float radius2, float height, 
                       int doPie, float pie1, float pie2)
{
   Point3 p;
   int ix,na,nb,nc,nd,jx,kx, ic = 1;
   int nf=0,nv=0, lsegs;
   float delta,ang, u;	
   float totalPie, startAng = 0.0f;	

   lsegs = llsegs-1 + 2*capsegs;

   // Make pie2 < pie1 and pie1-pie2 < TWOPI
   while (pie1 < pie2) pie1 += TWOPI;
   while (pie1 > pie2+TWOPI) pie1 -= TWOPI;
   if (pie1==pie2) totalPie = TWOPI;
   else totalPie = pie1-pie2;	

   delta = (float)2.0*PI/(float)segs;

   if (height<0) delta = -delta;

   int nverts;
   int nfaces;
   nverts = 2+segs*(lsegs);
   nfaces = 2*segs*(lsegs);	
   mesh.setNumVerts(nverts);
   mesh.setNumFaces(nfaces);
   mesh.setSmoothFlags(smooth != 0);
   mesh.setNumTVerts(0);
   mesh.setNumTVFaces(0);

   // bottom vertex 
   mesh.setVert(nv, Point3(0.0,0.0,0.0));
   nv++;

   // Bottom cap vertices	
   for(ix=0; ix<capsegs; ix++) {

      // Put center vertices all the way up
      p.z = 0.0f;
      u   = float(ix+1)/float(capsegs);
      ang = startAng;
      for (jx = 0; jx<segs; jx++) {			
         p.x = (float)cos(ang)*radius1*u;
         p.y = (float)sin(ang)*radius1*u;	
         mesh.setVert(nv, p);
         nv++;
         ang += delta;
      }	
   }

   // Middle vertices 
   for(ix=1; ix<llsegs; ix++) {

      // Put center vertices all the way up
      float   u = float(ix)/float(llsegs);
      float rad = (radius1*(1.0f-u) + radius2*u);
      p.z = height*((float)ix/float(llsegs));
      ang = startAng;
      for (jx = 0; jx<segs; jx++) {
         p.x = (float)cos(ang)*rad;
         p.y = (float)sin(ang)*rad;
         mesh.setVert(nv, p);
         nv++;
         ang += delta;
      }	
   }

   // Top cap vertices	
   for(ix=0; ix<capsegs; ix++) {

      // Put center vertices all the way up
      p.z = height;
      u   = 1.0f-float(ix)/float(capsegs);
      ang = startAng;
      for (jx = 0; jx<segs; jx++) {			
         p.x = (float)cos(ang)*radius2*u;		
         p.y = (float)sin(ang)*radius2*u;	
         mesh.setVert(nv, p);
         nv++;
         ang += delta;
      }	
   }

   /* top vertex */
   mesh.setVert(nv, (float)0.0, (float)0.0, height);
   nv++;

   // Now make faces ---

   BitArray startSliceFaces;
   BitArray endSliceFaces;
   BitArray topCapFaces;
   BitArray botCapFaces;

   // Make bottom cap		

   for(ix=1; ix<=segs; ++ix) {
      nc=(ix==segs)?1:ix+1;
      mesh.faces[nf].setEdgeVisFlags(capsegs>1,1,capsegs>1);
      mesh.faces[nf].setSmGroup(1);
      mesh.faces[nf].setVerts(0,nc,ix);
      mesh.faces[nf].setMatID(1);
      nf++;
   }

   int topCapStartFace = 0;

   /* Make midsection */
   for(ix=0; ix<lsegs-1; ++ix) {
      jx=ix*segs+1;
      for(kx=0; kx<segs; ++kx) {
         DWORD grp = 0;
         int mtlid;
         BOOL inSlice = FALSE;

         if (kx==segs) {
            mtlid = 4;
            grp = (1<<2);
            inSlice = TRUE;
         } else if (ix < capsegs-1 || ix >= capsegs+llsegs-1) {
            grp = 1;
            mtlid = (ix<capsegs-1)?0:1;
         } else {		
            mtlid = 2;
            if (smooth) {				
               grp = (1<<3);	
            }			
         }

         na = jx+kx;
         nb = na+segs;
         nc = (kx==(segs-1))? jx+segs: nb+1;
         nd = (kx==(segs-1))? jx : na+1;			

         mesh.faces[nf].setEdgeVisFlags(0,!inSlice,1);
         mesh.faces[nf].setSmGroup(grp);
         mesh.faces[nf].setVerts(na,nc,nb);
         mesh.faces[nf].setMatID(mtlid);
         nf++;

         mesh.faces[nf].setEdgeVisFlags(!inSlice,1,0);
         mesh.faces[nf].setSmGroup(grp);
         mesh.faces[nf].setVerts(na,nd,nc);
         mesh.faces[nf].setMatID(mtlid);
         nf++;
      }
   }

   //Make Top cap 			
   na = mesh.getNumVerts()-1;	
   jx = (lsegs-1)*segs+1;

   for(ix=0; ix<segs; ++ix) {
      nb = jx+ix;
      nc = (ix==segs-1)? jx: nb+1;		
      mesh.faces[nf].setEdgeVisFlags(capsegs>1,1,capsegs>1);		
      mesh.faces[nf].setSmGroup( 1);
      mesh.faces[nf].setVerts(na,nb,nc);
      mesh.faces[nf].setMatID(0);
      nf++;
   }

   assert(nf==mesh.numFaces);
   assert(nv==mesh.numVerts);
   mesh.InvalidateTopologyCache();
}
#endif