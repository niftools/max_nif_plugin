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
#include "NifGui.h"
#include "NifStrings.h"

#ifndef _countof
#define _countof(x) (sizeof(x)/sizeof((x)[0]))
#endif

static void BuildScubaMesh(Mesh &mesh, int segs, int smooth, int llsegs, 
                    float radius1, float radius2, float length);

const Class_ID BHKCAPSULEOBJECT_CLASS_ID = Class_ID(0x7f8f629a, BHKRIGIDBODYCLASS_DESC.PartB());

class bhkCapsuleObject : public SimpleObject2
{
public:			
   // Class vars
   static IParamMap2 *pmapParam;
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
   TCHAR *GetObjectName() { return GetString(IDS_RB_CAPSULE); }

   // Animatable methods		
   void DeleteThis() {delete this;}
   Class_ID ClassID() { return BHKCAPSULEOBJECT_CLASS_ID; } 
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

class CapsuleObjCreateCallBack : public CreateMouseCallBack {
   IPoint2 sp[4];
   bhkCapsuleObject *ob;
   Point3 p[4];
public:
   int proc( ViewExp *vpt,int msg, int point, int flags, IPoint2 m, Matrix3& mat);
   void SetObj(bhkCapsuleObject *obj) {ob = obj;}
};
static CapsuleObjCreateCallBack CapsuleCreateCB;

//--- ClassDescriptor and class vars ---------------------------------

// The class descriptor for Capsule
class bhkCapsuleClassDesc : public ClassDesc2 
{
public:
   bhkCapsuleClassDesc();
   int 			   IsPublic() { return 1; }
   void *			Create(BOOL loading = FALSE)
   {
      return new bhkCapsuleObject(loading);
   }
   const TCHAR *	ClassName() { return GetString(IDS_RB_CAPSULE_CLASS); }
   SClass_ID		SuperClassID() { return HELPER_CLASS_ID; }
   Class_ID		   ClassID() { return BHKCAPSULEOBJECT_CLASS_ID; }
   const TCHAR* 	Category() { return "NifTools"; }

   const TCHAR*	InternalName() { return _T("bhkCapsule"); }	// returns fixed parsable name (scripter-visible name)
   HINSTANCE		HInstance() { return hInstance; }			// returns owning module handle
};

extern ClassDesc2* GetbhkCapsuleDesc();

// in prim.cpp  - The dll instance handle
extern HINSTANCE hInstance;

IParamMap2 *bhkCapsuleObject::pmapParam  = NULL;
IObjParam *bhkCapsuleObject::ip         = NULL;


//--- Parameter map/block descriptors -------------------------------

// Parameter block indices
enum { cap_params, };

enum CapsuleParamIndicies
{
   PB_MATERIAL,
   PB_RADIUS1,
   PB_RADIUS2,
   PB_LENGTH,
};

enum { box_params_panel, };

static ParamBlockDesc2 param_blk ( 
    cap_params, _T("bhkCapsuleParameters"),  0, NULL, P_AUTO_CONSTRUCT|P_AUTO_UI, 0,
    //rollout
    IDD_CAPSULEPARAM, IDS_PARAMS, 0, 0, NULL, 

    // params
    PB_MATERIAL, _T("material"), TYPE_INT, P_ANIMATABLE,	IDS_DS_MATERIAL,
       p_default,	NP_DEFAULT_HVK_MATERIAL,
       end,

    PB_RADIUS1, _T("radius1"), TYPE_FLOAT, P_ANIMATABLE,	IDS_RB_RADIUS1,
       p_default,	   0.0,
       p_range,		float(0), float(1.0E30),
       p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_RADIUS1, IDC_RADSPINNER1, SPIN_AUTOSCALE,
       end,

    PB_RADIUS2, _T("radius2"), TYPE_FLOAT, P_ANIMATABLE,	IDS_RB_RADIUS2,
       p_default,	   0.0,
       p_range,		float(0), float(1.0E30),
       p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_RADIUS2, IDC_RADSPINNER2, SPIN_AUTOSCALE,
       end,

    PB_LENGTH, _T("length"), TYPE_FLOAT, P_ANIMATABLE,	IDS_DS_LENGTH,
       p_default,	   0.0,
       p_range,		float(-1.0E30), float(1.0E30),
       p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_LENGTHEDIT, IDC_LENSPINNER, SPIN_AUTOSCALE,
       end,

    end
    );

// static ClassDesc must be declared after static paramblock
static bhkCapsuleClassDesc capsuleDesc;
extern ClassDesc2* GetbhkCapsuleDesc() { return &capsuleDesc; }
bhkCapsuleClassDesc::bhkCapsuleClassDesc() {
   param_blk.SetClassDesc(this);
}

class CapsuleParamDlgProc : public ParamMap2UserDlgProc {
public:
   bhkCapsuleObject *so;
   HWND thishWnd;
   NpComboBox		mCbMaterial;

   CapsuleParamDlgProc(bhkCapsuleObject *s) {so=s;thishWnd=NULL;}
   BOOL DlgProc(TimeValue t,IParamMap2 *map,HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam);
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

BOOL CapsuleParamDlgProc::DlgProc(TimeValue t,IParamMap2 *map,HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
   thishWnd=hWnd;
   switch (msg) 
   {
   case WM_INITDIALOG: 
      {
         mCbMaterial.init(GetDlgItem(hWnd, IDC_CB_MATERIAL));
         for (const char **str = NpHvkMaterialNames; *str; ++str)
            mCbMaterial.add(*str);

         int sel = NP_DEFAULT_HVK_MATERIAL;
         Interval valid;
         so->pblock2->GetValue( PB_MATERIAL, 0, sel, valid);
         mCbMaterial.select( sel );

         Update(t);
         break;
      }
   case WM_COMMAND:
      switch (LOWORD(wParam)) 
      {
      case IDC_CB_MATERIAL:
         if (HIWORD(wParam)==CBN_SELCHANGE) {
            so->pblock2->SetValue( PB_MATERIAL, 0, mCbMaterial.selection() );
         }
         break;
      }
      break;	
   }
   return FALSE;
}

//--- Capsule methods -------------------------------


bhkCapsuleObject::bhkCapsuleObject(BOOL loading)
{
   SetAFlag(A_PLUGIN1);
   capsuleDesc.MakeAutoParamBlocks(this);
   assert(pblock2);
}

bhkCapsuleObject::~bhkCapsuleObject() 
{
   param_blk.SetUserDlgProc();
   CapsuleCreateCB.SetObj(NULL);
   if (pmapParam) {
      pmapParam  = NULL;
   }
}

void bhkCapsuleObject::BeginEditParams(IObjParam *ip,ULONG flags,Animatable *prev)
{
   SimpleObject::BeginEditParams(ip,flags,prev);

   capsuleDesc.BeginEditParams(ip,this,flags,prev);
   param_blk.SetUserDlgProc(new CapsuleParamDlgProc(this));
   pmapParam = pblock2->GetMap(cap_params);
   this->ip = ip;
}

void bhkCapsuleObject::EndEditParams( IObjParam *ip, ULONG flags,Animatable *next )
{		
   param_blk.SetUserDlgProc();
   SimpleObject::EndEditParams(ip,flags,next);
   this->ip = NULL;
   pmapParam = NULL;

   // tear down the appropriate auto-rollouts
   capsuleDesc.EndEditParams(ip, this, flags, next);
}

void bhkCapsuleObject::BuildMesh(TimeValue t)
{
   int segs = 12;
   int hsegs = 1;
   int smooth = 1;

   // Start the validity interval at forever and widdle it down.
   //FixHeight(pblock,t,(pmapParam?pmapParam->GetHWnd():NULL),increate);
   ivalid = FOREVER;

   float radius1, radius2, length;
   pblock2->GetValue(PB_RADIUS1,t,radius1,ivalid);
   pblock2->GetValue(PB_RADIUS2,t,radius2,ivalid);
   pblock2->GetValue(PB_LENGTH,t,length,ivalid);
   if (radius2 == 0.0f) radius2 = radius1;

   if (radius1 == 0.0f)
   {
      mesh.setNumVerts(0);
      mesh.setNumFaces(0);
      mesh.setNumTVerts(0);
      mesh.setNumTVFaces(0);
      mesh.setSmoothFlags(smooth != 0);
   }
   else
   {
      BuildScubaMesh(mesh, segs, smooth, hsegs, radius2, radius1, length);
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
         // Find the node and plug in the wire color
         {
            ULONG handle;
            ob->NotifyDependents(FOREVER, (PartID)&handle, REFMSG_GET_NODE_HANDLE);
            INode *node = GetCOREInterface()->GetINodeByHandle(handle);
            if (node) node->SetWireColor(RGB(255, 0, 0));
         }
         ob->pblock2->SetValue(PB_RADIUS1,0,0.0f);
         ob->pblock2->SetValue(PB_RADIUS2,0,0.0f);
         ob->suspendSnap = TRUE;				
         sp[0] = m;
         p[0] = vpt->SnapPoint(m,m,NULL,SNAP_IN_3D);
         ob->pblock2->SetValue(PB_LENGTH,0,0.0f);
         mat.SetTrans(p[0]);
         break;

      case 1: // Fix radius of first cap
         mat.IdentityMatrix();
         //mat.PreRotateZ(HALFPI);
         sp[1] = m;
         p[1] = vpt->SnapPoint(m,m,NULL,SNAP_IN_3D);
         r = Length(p[1]-p[0]);
         mat.SetTrans(p[0]);

         ob->pblock2->SetValue(PB_RADIUS1,0,r);
         ob->pblock2->SetValue(PB_RADIUS2,0,r);
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
         r = Length(p[2]-p[1]);
         mat.SetTrans(p[0]);

         ob->pblock2->SetValue(PB_LENGTH,0,r);
         ob->pmapParam->Invalidate();

         // Stop unless Ctrl is selected then we size the final radius
         if (msg==MOUSE_POINT && !(flags&MOUSE_CTRL) )
         {
            ob->suspendSnap = FALSE;
            return CREATE_STOP;
         }
         break;

      case 3: // Size the second cap
         mat.IdentityMatrix();
         //mat.PreRotateZ(HALFPI);
         sp[3] = m;
         p[3] = vpt->SnapPoint(m,m,NULL,SNAP_IN_3D);
         if (flags&MOUSE_CTRL) // ignore radius
         {
            r = Length(p[1]-p[0]);
            ob->pblock2->SetValue(PB_RADIUS2,0,r);
         }
         else
         {
            // start radius at r1
            r = Length((p[3]-p[2]) + (p[1]-p[0]));
            ob->pblock2->SetValue(PB_RADIUS2,0,r);
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
   pblock2->GetValue(PB_RADIUS1,t,radius,FOREVER);
   if (radius==0.0f) return FALSE;
   else return TRUE;
}

void bhkCapsuleObject::InvalidateUI() 
{
   if (pmapParam) pmapParam->Invalidate();
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
                    float radius1, float radius2, float cylh)
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
      mesh.verts[i].z -= (radius2 + cylh/2.0f);

   if (edgelstr) delete []edgelstr;
   if (edgelstl) delete []edgelstl;
   assert(fc==mesh.numFaces);
   //	assert(nv==mesh.numVerts);
   mesh.InvalidateTopologyCache();
}

int bhkCapsuleObject::Display(TimeValue t, INode* inode, ViewExp *vpt, int flags) 
{
   Matrix3 m;
   Color color = Color(inode->GetWireColor());
   GraphicsWindow *gw = vpt->getGW();
   Material *mtl = gw->getMaterial();
   m = inode->GetObjectTM(t);
   gw->setTransform(m);
   DWORD rlim = gw->getRndLimits();
   gw->setRndLimits(GW_WIREFRAME|GW_EDGES_ONLY/*|GW_Z_BUFFER*/);
   if (inode->Selected()) 
      gw->setColor( LINE_COLOR, GetSelColor());
   else if(!inode->IsFrozen() && !inode->Dependent())
      gw->setColor( LINE_COLOR, color);

   UpdateMesh(t);
   mesh.render( gw, mtl, NULL, COMP_ALL);	
   gw->setRndLimits(rlim);
   return 0;
}