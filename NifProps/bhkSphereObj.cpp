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
#ifndef _countof
#define _countof(x) (sizeof(x)/sizeof((x)[0]))
#endif

const Class_ID bhkSphereObject_CLASS_ID = Class_ID(0x8d532427, 0x8b4c64c7);

class bhkSphereObject : public SimpleObject, public IParamArray, public bhkRigidBodyIfcHelper
{
public:			
   // Class vars
   static IParamMap *pmapParam;
   static int dlgSegments;
   static int dlgSmooth;
   static float crtRadius;
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
   RefTargetHandle Clone(RemapDir& remap = NoRemap());
   TCHAR *GetObjectName() { return GetString(IDS_RB_SPHERE); }

   // From GeomObject
   int IntersectRay(TimeValue t, Ray& ray, float& at, Point3& norm);

   // Animatable methods		
   void DeleteThis() {delete this;}
   Class_ID ClassID() { return bhkSphereObject_CLASS_ID; } 
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

   void SetParams(float rad, int segs, BOOL smooth=TRUE);
   void UpdateUI();

   BaseInterface* GetInterface(Interface_ID id);

};


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

// The class descriptor for sphere
class bhkSphereClassDesc : public ClassDesc2 
{
public:
   int 			   IsPublic() { return 1; }
   void *			Create(BOOL loading = FALSE)
   {
      AddInterface (GetbhkRigidBodyInterfaceDesc());
      return new bhkSphereObject(loading);
   }
   const TCHAR *	ClassName() { return GetString(IDS_RB_SPHERE_CLASS); }
   SClass_ID		SuperClassID() { return HELPER_CLASS_ID; }
   Class_ID		   ClassID() { return bhkSphereObject_CLASS_ID; }
   const TCHAR* 	Category() { return "NifTools"; }
   void			   ResetClassParams(BOOL fileReset);
};

static bhkSphereClassDesc sphereDesc;
extern ClassDesc2* GetbhkSphereDesc() { return &sphereDesc; }

// in prim.cpp  - The dll instance handle
extern HINSTANCE hInstance;

int bhkSphereObject::dlgSegments       = DEF_SEGMENTS;
int bhkSphereObject::dlgSmooth         = SMOOTH_ON;
IParamMap *bhkSphereObject::pmapParam  = NULL;
IObjParam *bhkSphereObject::ip         = NULL;
float bhkSphereObject::crtRadius       = 0.0f;

void bhkSphereClassDesc::ResetClassParams(BOOL fileReset)
{
   bhkSphereObject::dlgSegments    = DEF_SEGMENTS;
   bhkSphereObject::dlgSmooth      = SMOOTH_ON;
   bhkSphereObject::crtRadius      = 0.0f;
}


//--- Parameter map/block descriptors -------------------------------

// Parameter block indices
enum SphereParamIndicies
{
   PB_RADIUS = 0,
   PB_SEGS = 1,
   PB_SMOOTH = 2,
};


//
//
// Parameters

static ParamUIDesc descParam[] = {
   // Radius
   ParamUIDesc(
   PB_RADIUS,
   EDITTYPE_UNIVERSE,
   IDC_RADIUS,IDC_RADSPINNER,
   MIN_RADIUS,MAX_RADIUS,
   SPIN_AUTOSCALE),	

   // Segments
   ParamUIDesc(
   PB_SEGS,
   EDITTYPE_INT,
   IDC_SEGMENTS,IDC_SEGSPINNER,
   (float)MIN_SEGMENTS,(float)MAX_SEGMENTS,
   0.1f),

   // Smooth
   ParamUIDesc(PB_SMOOTH,TYPE_SINGLECHEKBOX,IDC_OBSMOOTH),
};
const int PARAMDESC_LENGTH = _countof(descParam);

static ParamBlockDescID descVer0[] = {
   { TYPE_FLOAT, NULL, TRUE, 0 },		
   { TYPE_INT, NULL, TRUE, 1 },
   { TYPE_INT, NULL, TRUE, 2 } 
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

class SphereParamDlgProc : public ParamMapUserDlgProc {
public:
   bhkSphereObject *so;
   HWND thishWnd;

   SphereParamDlgProc(bhkSphereObject *s) {so=s;thishWnd=NULL;}
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

void SphereParamDlgProc::Update(TimeValue t)
{
   if (!thishWnd) 
      return;
   return;
}

BOOL SphereParamDlgProc::DlgProc(TimeValue t,IParamMap *map,HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
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

//--- Sphere methods -------------------------------


bhkSphereObject::bhkSphereObject(BOOL loading)
{
   SetAFlag(A_PLUGIN1);
   MakeRefByID(FOREVER, 0, CreateParameterBlock(curDescVer, PBLOCK_LENGTH, CURRENT_VERSION));
   assert(pblock);
   MakeRefByID(FOREVER, 1, GetRBBlock());

   pblock->SetValue(PB_RADIUS,0,crtRadius);
   pblock->SetValue(PB_SMOOTH,0,dlgSmooth);
   pblock->SetValue(PB_SEGS,0,dlgSegments);	
}

bhkSphereObject::~bhkSphereObject() {
}

#define NEWMAP_CHUNKID	0x0100

IOResult bhkSphereObject::Load(ILoad *iload) 
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

IOResult bhkSphereObject::Save(ISave *isave)
{
   if (TestAFlag(A_PLUGIN1)) {
      isave->BeginChunk(NEWMAP_CHUNKID);
      isave->EndChunk();
   }
   return IO_OK;
}

RefTargetHandle bhkSphereObject::GetReference(int i) 
{
   if (i == 1) 
      return GetRBBlock();
   return pblock;
}

void bhkSphereObject::SetReference(int i, RefTargetHandle rtarg) 
{
   if (i == 1)
      return;
   pblock=(IParamBlock*)rtarg;
}

BaseInterface* bhkSphereObject::GetInterface(Interface_ID id)
{
   if (id == BHKRIGIDBODYINTERFACE_DESC)
      return this;
   return SimpleObject::GetInterface(id);
}



void bhkSphereObject::BeginEditParams(IObjParam *ip,ULONG flags,Animatable *prev)
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
         MAKEINTRESOURCE(IDD_SPHEREPARAM2),
         GetString(IDS_RB_PARAMETERS),
         0);
   }
   this->ip = ip;

   if(pmapParam) {
      // A callback for the type in.
      pmapParam->SetUserDlgProc(new SphereParamDlgProc(this));
   }
   BeginEditRBParams(ip, flags, prev);
}

void bhkSphereObject::EndEditParams( IObjParam *ip, ULONG flags,Animatable *next )
{		
   SimpleObject::EndEditParams(ip,flags,next);
   this->ip = NULL;

   if (flags&END_EDIT_REMOVEUI ) {
      DestroyCPParamMap(pmapParam);
      pmapParam  = NULL;
   }
   EndEditRBParams(ip, flags, next);

   // Save these values in class variables so the next object created will inherit them.
   pblock->GetValue(PB_SEGS,ip->GetTime(),dlgSegments,FOREVER);
   pblock->GetValue(PB_SMOOTH,ip->GetTime(),dlgSmooth,FOREVER);	
}

void bhkSphereObject::SetParams(float rad, int segs, BOOL smooth)
{
   pblock->SetValue(PB_RADIUS,0, rad);				
   pblock->SetValue(PB_SEGS,0, segs);				
   pblock->SetValue(PB_SMOOTH,0, smooth);				
}			   

void bhkSphereObject::BuildMesh(TimeValue t)
{
   Point3 p;	
   int ix,na,nb,nc,nd,jx,kx;
   int nf=0,nv=0;
   float delta, delta2;
   float a,alt,secrad,secang,b,c;
   int segs, smooth;
   float radius;
   float hemi = 0.0f;
   BOOL genUVs = TRUE;
   float startAng = 0.0f;
   if (TestAFlag(A_PLUGIN1)) startAng = HALFPI;

   // Start the validity interval at forever and whittle it down.
   ivalid = FOREVER;
   pblock->GetValue(PB_RADIUS, t, radius, ivalid);
   pblock->GetValue(PB_SEGS, t, segs, ivalid);
   pblock->GetValue(PB_SMOOTH, t, smooth, ivalid);
   LimitValue(segs, MIN_SEGMENTS, MAX_SEGMENTS);
   LimitValue(smooth, MIN_SMOOTH, MAX_SMOOTH);
   LimitValue(radius, MIN_RADIUS, MAX_RADIUS);

   float totalPie(0.0f);
   if (hemi>=1.0f) hemi = 0.9999f;
   hemi = (1.0f-hemi) * PI;
   float basedelta=2.0f*PI/(float)segs;
   delta2 = basedelta;
   delta  = basedelta;

   int rows = int(hemi/delta) + 1;
   int realsegs=segs;
   int nverts = rows * realsegs + 2;
   int nfaces = rows * realsegs * 2;
   mesh.setNumVerts(nverts);
   mesh.setNumFaces(nfaces);
   mesh.setSmoothFlags(smooth != 0);
   int lastvert=nverts-1;

   // Top vertex 
   mesh.setVert(nv, 0.0f, 0.0f, radius);
   nv++;

   // Middle vertices 
   alt=delta;
   for(ix=1; ix<=rows; ix++) {		
      a = (float)cos(alt)*radius;		
      secrad = (float)sin(alt)*radius;
      secang = startAng; //0.0f
      for(jx=0; jx<segs; ++jx) {
         b = (float)cos(secang)*secrad;
         c = (float)sin(secang)*secrad;
         mesh.setVert(nv++,b,c,a);
         secang+=delta2;
      }
      alt+=delta;		
   }

   /* Bottom vertex */
   mesh.setVert(nv++, 0.0f, 0.0f,-radius);

   // Now make faces 

   BitArray startSliceFaces;
   BitArray endSliceFaces;

   // Make top conic cap
   for(ix=1; ix<=segs; ++ix) {
      nc=(ix==segs)?1:ix+1;
      mesh.faces[nf].setEdgeVisFlags(1,1,1);
      mesh.faces[nf].setSmGroup(smooth?1:0);
      mesh.faces[nf].setMatID(1);
      mesh.faces[nf].setVerts(0, ix, nc);
      nf++;
   }

   /* Make midsection */
   int lastrow=rows-1,lastseg=segs-1,almostlast=lastseg-1;
   for(ix=1; ix<rows; ++ix) {
      jx=(ix-1)*segs+1;
      for(kx=0; kx<segs; ++kx) {
         na = jx+kx;
         nb = na+segs;
         nc = (kx==lastseg)? jx+segs: nb+1;
         nd = (kx==lastseg)? jx : na+1;

         mesh.faces[nf].setEdgeVisFlags(1,1,0);
         mesh.faces[nf].setSmGroup(smooth?1:0);
         mesh.faces[nf].setMatID(1); 
         mesh.faces[nf].setVerts(na,nb,nc);
         nf++;

         mesh.faces[nf].setEdgeVisFlags(0,1,1);
         mesh.faces[nf].setSmGroup(smooth?1:0);
         mesh.faces[nf].setMatID(1);
         mesh.faces[nf].setVerts(na,nc,nd);
         nf++;
      }
   }

   // Make bottom conic cap
   na = mesh.getNumVerts()-1;
   int botsegs=segs;
   jx = (rows-1)*segs+1;lastseg=botsegs-1;
   int fstart = nf;
   for(ix=0; ix<botsegs; ++ix) {
      nc = ix + jx;
      nb = (ix==lastseg)?jx:nc+1;
      mesh.faces[nf].setEdgeVisFlags(1,1,1);
      mesh.faces[nf].setSmGroup(smooth?1:0);
      mesh.faces[nf].setMatID(1);
      mesh.faces[nf].setVerts(na, nb, nc);
      nf++;
   }

   mesh.setNumTVerts(0);
   mesh.setNumTVFaces(0);
   mesh.InvalidateTopologyCache();
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

class SphereObjCreateCallBack : public CreateMouseCallBack {
   IPoint2 sp0;
   bhkSphereObject *ob;
   Point3 p0;
public:
   int proc( ViewExp *vpt,int msg, int point, int flags, IPoint2 m, Matrix3& mat);
   void SetObj(bhkSphereObject *obj) {ob = obj;}
};

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
         ob->pblock->SetValue(PB_RADIUS,0,0.0f);
         ob->suspendSnap = TRUE;				
         sp0 = m;
         p0 = vpt->SnapPoint(m,m,NULL,SNAP_IN_3D);
         mat.SetTrans(p0);
         break;
      case 1:
         mat.IdentityMatrix();
         //mat.PreRotateZ(HALFPI);
         p1 = vpt->SnapPoint(m,m,NULL,SNAP_IN_3D);
         r = Length(p1-p0);
         mat.SetTrans(p0);

         ob->pblock->SetValue(PB_RADIUS,0,r);
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

static SphereObjCreateCallBack sphereCreateCB;

CreateMouseCallBack* bhkSphereObject::GetCreateMouseCallBack() 
{
   sphereCreateCB.SetObj(this);
   return(&sphereCreateCB);
}


BOOL bhkSphereObject::OKtoDisplay(TimeValue t) 
{
   float radius;
   pblock->GetValue(PB_RADIUS,t,radius,FOREVER);
   if (radius==0.0f) return FALSE;
   else return TRUE;
}

// From GeomObject
int bhkSphereObject::IntersectRay(TimeValue t, Ray& ray, float& at, Point3& norm)
{
   int smooth;
   pblock->GetValue(PB_SMOOTH,t,smooth,FOREVER);

   float r;
   float a, b, c, ac4, b2, at1, at2;
   float root;
   BOOL neg1, neg2;

   pblock->GetValue(PB_RADIUS,t,r,FOREVER);

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

ParamDimension *bhkSphereObject::GetParameterDim(int pbIndex) 
{
   switch (pbIndex) 
   {
   case PB_RADIUS:
      return stdWorldDim;			
   case PB_SEGS:
      return stdSegmentsDim;			
   case PB_SMOOTH:
      return stdNormalizedDim;
   default:
      return defaultDim;
   }
}

TSTR bhkSphereObject::GetParameterName(int pbIndex) 
{
   switch (pbIndex) 
   {
   case PB_RADIUS:
      return TSTR(GetString(IDS_RB_RADIUS));			
   case PB_SEGS:
      return TSTR(GetString(IDS_RB_SEGS));			
   case PB_SMOOTH:
      return TSTR(GetString(IDS_RB_SMOOTH));			
   default:
      return TSTR(_T(""));
   }
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
