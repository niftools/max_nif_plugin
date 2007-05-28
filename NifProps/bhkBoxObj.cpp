/**********************************************************************
*<
FILE: bhkBoxObj.cpp

DESCRIPTION:	Collision Box Object Implementation

CREATED BY: tazpn (Theo)

HISTORY: 
  V1.0 - Derived from 3ds max prim box example

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

const Class_ID bhkBoxObject_CLASS_ID = Class_ID(0x86e19816, BHKRIGIDBODYCLASS_DESC.PartB());

static ParamBlockDesc2* GetbhkBoxParamBlockDesc();

class bhkBoxObject : public SimpleObject2
{
public:			
   // Class vars
   static IParamMap2 *pmapParam;
   static IObjParam *ip;

   bhkBoxObject(BOOL loading);		
   ~bhkBoxObject();		

   // From Object
   int CanConvertToType(Class_ID obtype);
   Object* ConvertToType(TimeValue t, Class_ID obtype);
   void GetCollapseTypes(Tab<Class_ID> &clist,Tab<TSTR*> &nlist);

   CreateMouseCallBack* GetCreateMouseCallBack();
   void BeginEditParams( IObjParam  *ip, ULONG flags,Animatable *prev);
   void EndEditParams( IObjParam *ip, ULONG flags,Animatable *next);
   RefTargetHandle Clone(RemapDir& remap);
   TCHAR *GetObjectName() { return GetString(IDS_RB_BOX); }

   int	NumParamBlocks() { return 1; }					// return number of ParamBlocks in this instance
   IParamBlock2* GetParamBlock(int i) { return pblock2; } // return i'th ParamBlock
   IParamBlock2* GetParamBlockByID(BlockID id) { return (pblock2->ID() == id) ? pblock2 : NULL; } // return id'd ParamBlock

   // Animatable methods		
   void DeleteThis() {delete this;}
   Class_ID ClassID() { return bhkBoxObject_CLASS_ID; } 
   SClass_ID SuperClassID() { return HELPER_CLASS_ID; }

   // From SimpleObject
   void BuildMesh(TimeValue t);
   BOOL OKtoDisplay(TimeValue t);
   void InvalidateUI();

   int Display(TimeValue t, INode* inode, ViewExp *vpt, int flags);

   void UpdateUI();
};

//--- ClassDescriptor and class vars ---------------------------------

// The class descriptor for box
class bhkBoxClassDesc : public ClassDesc2 
{
public:
   bhkBoxClassDesc();
   int 			   IsPublic() { return 1; }
   void *			Create(BOOL loading = FALSE) {
      return new bhkBoxObject(loading);
   }
   const TCHAR *	ClassName() { return GetString(IDS_RB_BOX_CLASS); }
   SClass_ID		SuperClassID() { return HELPER_CLASS_ID; }
   Class_ID		   ClassID() { return bhkBoxObject_CLASS_ID; }
   const TCHAR* 	Category() { return "NifTools"; }

   const TCHAR*	InternalName() { return _T("bhkBox"); }	// returns fixed parsable name (scripter-visible name)
   HINSTANCE		HInstance() { return hInstance; }			// returns owning module handle
};

extern ClassDesc2* GetbhkBoxDesc();

// in prim.cpp  - The dll instance handle
extern HINSTANCE hInstance;

IParamMap2 *bhkBoxObject::pmapParam  = NULL;
IObjParam *bhkBoxObject::ip         = NULL;

//--- Parameter map/block descriptors -------------------------------

// Parameter and ParamBlock IDs
enum { box_params, };  // pblock ID
enum 
{ 
   PB_MATERIAL,
   PB_LENGTH,
   PB_WIDTH,
   PB_HEIGHT,
};

enum { box_params_panel, };

static ParamBlockDesc2 param_blk ( 
    box_params, _T("parameters"),  0, NULL, P_AUTO_CONSTRUCT + P_AUTO_UI + P_MULTIMAP, 0,
    //rollout
    1,
    box_params, IDD_BOXPARAM1, IDS_PARAMS, 0, 0, NULL, 

    // params
    PB_MATERIAL, _T("material"), TYPE_INT, P_ANIMATABLE,	IDS_DS_MATERIAL,
      p_default,	NP_DEFAULT_HVK_MATERIAL,
      end,

    PB_LENGTH, _T("length"), TYPE_FLOAT, P_ANIMATABLE,	IDS_DS_LENGTH,
      p_default,	   0.0,
      p_range,		0.0, float(1.0E30),
      p_ui, box_params, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_LENGTHEDIT, IDC_LENSPINNER, SPIN_AUTOSCALE,
      end,

    PB_WIDTH, _T("width"), TYPE_FLOAT, P_ANIMATABLE,	IDS_DS_WIDTH,
      p_default,	   0.0,
      p_range,		0.0, float(1.0E30),
      p_ui, box_params, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_WIDTHEDIT, IDC_WIDTHSPINNER, SPIN_AUTOSCALE,
      end,

    PB_HEIGHT, _T("height"), TYPE_FLOAT, P_ANIMATABLE,	IDS_DS_HEIGHT,
      p_default,	   0.0,
      p_range,		float(-1.0E30), float(1.0E30),
      p_ui, box_params, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_HEIGHTEDIT, IDC_HEIGHTSPINNER, SPIN_AUTOSCALE,
      end,
    end
    );

// bug in pb desc? forces us to use this rather than in inline version
static bhkBoxClassDesc boxDesc;
extern ClassDesc2* GetbhkBoxDesc() { return &boxDesc; }
bhkBoxClassDesc::bhkBoxClassDesc() {
   param_blk.SetClassDesc(this);
}

class BoxParamDlgProc : public ParamMap2UserDlgProc {
public:
   bhkBoxObject *so;
   HWND thishWnd;
   NpComboBox		mCbMaterial;

   BoxParamDlgProc(bhkBoxObject *s) {so=s;thishWnd=NULL;}
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

void BoxParamDlgProc::Update(TimeValue t)
{
   if (!thishWnd) 
      return;
   return;
}

BOOL BoxParamDlgProc::DlgProc(TimeValue t,IParamMap2 *map,HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
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
//--- Box methods -------------------------------


bhkBoxObject::bhkBoxObject(BOOL loading)
{
   SetAFlag(A_PLUGIN1);
   boxDesc.MakeAutoParamBlocks(this);
   assert(pblock2);
}

bhkBoxObject::~bhkBoxObject() 
{
   param_blk.SetUserDlgProc();
   if (pmapParam) {
      pmapParam  = NULL;
   }
}

void bhkBoxObject::BeginEditParams(IObjParam *ip,ULONG flags,Animatable *prev)
{
   SimpleObject::BeginEditParams(ip,flags,prev);

   //if (pmapParam == NULL) {
   //   pmapParam = CreateCPParamMap2(
   //      0,
   //      pblock2,
   //      GetCOREInterface(),
   //      hInstance,
   //      MAKEINTRESOURCE(IDD_BOXPARAM1),
   //      GetString(IDS_PARAMS),
   //      0);
   //}
   boxDesc.BeginEditParams(ip,this,flags,prev);
   param_blk.SetUserDlgProc(new BoxParamDlgProc(this));
   pmapParam = pblock2->GetMap(box_params);

	pmapParam->GetIRollup()->Hide(1);


   this->ip = ip;

   //if(pmapParam) {
   //   // A callback for the type in.
   //   pmapParam->SetUserDlgProc(new BoxParamDlgProc(this));
   //}

}

void bhkBoxObject::EndEditParams( IObjParam *ip, ULONG flags,Animatable *next )
{		
   param_blk.SetUserDlgProc();

   SimpleObject::EndEditParams(ip,flags,next);
   this->ip = NULL;
   pmapParam = NULL;
   //if (pmapParam && flags&END_EDIT_REMOVEUI ) {
   //   DestroyCPParamMap2 (pmapParam);
   //   pmapParam = NULL;
   //}

   // tear down the appropriate auto-rollouts
   boxDesc.EndEditParams(ip, this, flags, next);
}

void bhkBoxObject::UpdateUI()
{
   if (ip == NULL)
      return;
   BoxParamDlgProc* dlg = static_cast<BoxParamDlgProc*>(pmapParam->GetUserDlgProc());
   dlg->Update(ip->GetTime());
}

enum 
{
   POSX = 0,	// right
   POSY = 1,	// back
   POSZ = 2,	// top
   NEGX = 3,	// left
   NEGY = 4,	// front
   NEGZ = 5,	// bottom
};

int direction(Point3 *v) {
   Point3 a = v[0]-v[2];
   Point3 b = v[1]-v[0];
   Point3 n = CrossProd(a,b);
   switch(MaxComponent(n)) {
      case 0: return (n.x<0)?NEGX:POSX;
      case 1: return (n.y<0)?NEGY:POSY;
      case 2: return (n.z<0)?NEGZ:POSZ;
   }
   return 0;
}

void bhkBoxObject::BuildMesh(TimeValue t)
{
	extern void BuildBox(Mesh&mesh, float l, float w, float h);

	float l, w, h;
	ivalid = FOREVER;	
	pblock2->GetValue(PB_LENGTH,t,l,ivalid);
	pblock2->GetValue(PB_WIDTH,t,w,ivalid);
	pblock2->GetValue(PB_HEIGHT,t,h,ivalid);
	BuildBox(mesh, l, w, h);
}

// Remap the sub-object material numbers so that the top face is the first one
// The order now is:
// Top / Bottom /  Left/ Right / Front / Back
static int mapDir[6] ={ 3, 5, 0, 2, 4, 1 };

// vertices ( a b c d ) are in counter clockwise order when viewd from 
// outside the surface unless bias!=0 in which case they are clockwise
static void MakeQuad(int nverts, Face *f, int a, int b , int c , int d, int sg, int bias) {
   int sm = 1<<sg;
   assert(a<nverts);
   assert(b<nverts);
   assert(c<nverts);
   assert(d<nverts);
   if (bias) {
      f[0].setVerts( b, a, c);
      f[0].setSmGroup(sm);
      f[0].setEdgeVisFlags(1,0,1);
      f[1].setVerts( d, c, a);
      f[1].setSmGroup(sm);
      f[1].setEdgeVisFlags(1,0,1);
   } else {
      f[0].setVerts( a, b, c);
      f[0].setSmGroup(sm);
      f[0].setEdgeVisFlags(1,1,0);
      f[1].setVerts( c, d, a);
      f[1].setSmGroup(sm);
      f[1].setEdgeVisFlags(1,1,0);
   }
}
#define MAKE_QUAD(na,nb,nc,nd,sm,b) {MakeQuad(nverts,&(mesh.faces[nf]),na, nb, nc, nd, sm, b);nf+=2;}

void BuildBox(Mesh&mesh, float l, float w, float h)
{
   int ix,iy,iz,nf,kv,mv,nlayer,topStart,midStart;
   int nverts,nv,nextk,nextm,wsp1;
   int nfaces;
   Point3 va,vb,p;
   BOOL bias = 0;
   const int lsegs = 1, wsegs = 1, hsegs = 1;

   // Start the validity interval at forever and whittle it down.
   if (h<0.0f) bias = 1;

   // Number of verts
   // bottom : (lsegs+1)*(wsegs+1)
   // top	: (lsegs+1)*(wsegs+1)
   // sides	: (2*lsegs+2*wsegs)*(hsegs-1)

   // Number of rectangular faces.
   // bottom : (lsegs)*(wsegs)
   // top	: (lsegs)*(wsegs)
   // sides	: 2*(hsegs*lsegs)+2*(wsegs*lsegs)

   wsp1 = wsegs + 1;
   nlayer	=  2*(lsegs+wsegs);
   topStart = (lsegs+1)*(wsegs+1);
   midStart = 2*topStart;

   nverts = midStart+nlayer*(hsegs-1);
   nfaces = 4*(lsegs*wsegs + hsegs*lsegs + wsegs*hsegs);

   mesh.setNumVerts(nverts);
   mesh.setNumFaces(nfaces);
   mesh.InvalidateTopologyCache();

   nv = 0;

   vb =  Point3(w,l,h)/float(2);	
   va = -vb;

   float dx = w/wsegs;
   float dy = l/lsegs;
   float dz = h/hsegs;

   // do bottom vertices.
   p.z = va.z;
   p.y = va.y;
   for(iy=0; iy<=lsegs; iy++) {
      p.x = va.x;
      for (ix=0; ix<=wsegs; ix++) {
         mesh.setVert(nv++, p);
         p.x += dx;
      }
      p.y += dy;
   }

   nf = 0;

   // do bottom faces.
   for(iy=0; iy<lsegs; iy++) {
      kv = iy*(wsegs+1);
      for (ix=0; ix<wsegs; ix++) {
         MAKE_QUAD(kv, kv+wsegs+1, kv+wsegs+2, kv+1, 1, bias);
         kv++;
      }
   }
   assert(nf==lsegs*wsegs*2);

   // do top vertices.
   p.z = vb.z;
   p.y = va.y;
   for(iy=0; iy<=lsegs; iy++) {
      p.x = va.x;
      for (ix=0; ix<=wsegs; ix++) {
         mesh.setVert(nv++, p);
         p.x += dx;
      }
      p.y += dy;
   }

   // do top faces (lsegs*wsegs);
   for(iy=0; iy<lsegs; iy++) {
      kv = iy*(wsegs+1)+topStart;
      for (ix=0; ix<wsegs; ix++) {
         MAKE_QUAD(kv, kv+1, kv+wsegs+2,kv+wsegs+1, 2, bias);
         kv++;
      }
   }
   assert(nf==lsegs*wsegs*4);

   // do middle vertices 
   for(iz=1; iz<hsegs; iz++) {

      p.z = va.z + dz * iz;

      // front edge
      p.x = va.x;	 p.y = va.y;
      for (ix=0; ix<wsegs; ix++) { mesh.setVert(nv++, p);	 p.x += dx;	}

      // right edge
      p.x = vb.x;	  p.y = va.y;
      for (iy=0; iy<lsegs; iy++) { mesh.setVert(nv++, p);	 p.y += dy;	}

      // back edge
      p.x =  vb.x;  p.y =	 vb.y;
      for (ix=0; ix<wsegs; ix++) { mesh.setVert(nv++, p);	 p.x -= dx;	}

      // left edge
      p.x = va.x;	 p.y =	vb.y;
      for (iy=0; iy<lsegs; iy++) { mesh.setVert(nv++, p);	 p.y -= dy;	}
   }

   if (hsegs==1) {
      // do FRONT faces -----------------------
      kv = 0;
      mv = topStart;
      for (ix=0; ix<wsegs; ix++) {
         MAKE_QUAD(kv, kv+1, mv+1, mv, 3, bias);
         kv++;
         mv++;
      }

      // do RIGHT faces.-----------------------
      kv = wsegs;	 
      mv = topStart + kv;
      for (iy=0; iy<lsegs; iy++) {
         MAKE_QUAD(kv, kv+wsp1, mv+wsp1, mv, 4, bias);
         kv += wsp1;
         mv += wsp1;
      }	

      // do BACK faces.-----------------------
      kv = topStart - 1;
      mv = midStart - 1;
      for (ix=0; ix<wsegs; ix++) {
         MAKE_QUAD(kv, kv-1, mv-1, mv, 5, bias);
         kv --;
         mv --;
      }

      // do LEFT faces.----------------------
      kv = lsegs*(wsegs+1);  // index into bottom
      mv = topStart + kv;
      for (iy=0; iy<lsegs; iy++) {
         MAKE_QUAD(kv, kv-wsp1, mv-wsp1, mv, 6, bias);
         kv -= wsp1;
         mv -= wsp1;
      }
   }

   else {
      // do front faces.
      kv = 0;
      mv = midStart;
      for(iz=0; iz<hsegs; iz++) {
         if (iz==hsegs-1) mv = topStart;
         for (ix=0; ix<wsegs; ix++) 
            MAKE_QUAD(kv+ix, kv+ix+1, mv+ix+1, mv+ix, 3, bias);
         kv = mv;
         mv += nlayer;
      }

      assert(nf==lsegs*wsegs*4 + wsegs*hsegs*2);

      // do RIGHT faces.-------------------------
      // RIGHT bottom row:
      kv = wsegs; // into bottom layer. 
      mv = midStart + wsegs; // first layer of mid verts


      for (iy=0; iy<lsegs; iy++) {
         MAKE_QUAD(kv, kv+wsp1, mv+1, mv, 4, bias);
         kv += wsp1;
         mv ++;
      }

      // RIGHT middle part:
      kv = midStart + wsegs; 
      for(iz=0; iz<hsegs-2; iz++) {
         mv = kv + nlayer;
         for (iy=0; iy<lsegs; iy++) {
            MAKE_QUAD(kv+iy, kv+iy+1, mv+iy+1, mv+iy, 4, bias);
         }
         kv += nlayer;
      }

      // RIGHT top row:
      kv = midStart + wsegs + (hsegs-2)*nlayer; 
      mv = topStart + wsegs;
      for (iy=0; iy<lsegs; iy++) {
         MAKE_QUAD(kv, kv+1, mv+wsp1, mv, 4, bias);
         mv += wsp1;
         kv++;
      }

      assert(nf==lsegs*wsegs*4 + wsegs*hsegs*2 + lsegs*hsegs*2);

      // do BACK faces. ---------------------
      // BACK bottom row:
      kv = topStart - 1;
      mv = midStart + wsegs + lsegs;
      for (ix=0; ix<wsegs; ix++) {
         MAKE_QUAD(kv, kv-1, mv+1, mv, 5, bias);
         kv --;
         mv ++;
      }

      // BACK middle part:
      kv = midStart + wsegs + lsegs; 
      for(iz=0; iz<hsegs-2; iz++) {
         mv = kv + nlayer;
         for (ix=0; ix<wsegs; ix++) {
            MAKE_QUAD(kv+ix, kv+ix+1, mv+ix+1, mv+ix, 5, bias);
         }
         kv += nlayer;
      }

      // BACK top row:
      kv = midStart + wsegs + lsegs + (hsegs-2)*nlayer; 
      mv = topStart + lsegs*(wsegs+1)+wsegs;
      for (ix=0; ix<wsegs; ix++) {
         MAKE_QUAD(kv, kv+1, mv-1, mv, 5, bias);
         mv --;
         kv ++;
      }

      assert(nf==lsegs*wsegs*4 + wsegs*hsegs*4 + lsegs*hsegs*2);

      // do LEFT faces. -----------------
      // LEFT bottom row:
      kv = lsegs*(wsegs+1);  // index into bottom
      mv = midStart + 2*wsegs +lsegs;
      for (iy=0; iy<lsegs; iy++) {
         nextm = mv+1;
         if (iy==lsegs-1) 
            nextm -= nlayer;
         MAKE_QUAD(kv, kv-wsp1, nextm, mv, 6, bias);
         kv -=wsp1;
         mv ++;
      }

      // LEFT middle part:
      kv = midStart + 2*wsegs + lsegs; 
      for(iz=0; iz<hsegs-2; iz++) {
         mv = kv + nlayer;
         for (iy=0; iy<lsegs; iy++) {
            nextm = mv+1;
            nextk = kv+iy+1;
            if (iy==lsegs-1) { 
               nextm -= nlayer;
               nextk -= nlayer;
            }
            MAKE_QUAD(kv+iy, nextk, nextm, mv, 6, bias);
            mv++;
         }
         kv += nlayer;
      }

      // LEFT top row:
      kv = midStart + 2*wsegs + lsegs+ (hsegs-2)*nlayer; 
      mv = topStart + lsegs*(wsegs+1);
      for (iy=0; iy<lsegs; iy++) {
         nextk = kv+1;
         if (iy==lsegs-1) 
            nextk -= nlayer;
         MAKE_QUAD(kv, nextk, mv-wsp1, mv, 6, bias);
         mv -= wsp1;
         kv++;
      }
   }

   mesh.setNumTVerts(0);
   mesh.setNumTVFaces(0);
   for (nf = 0; nf<nfaces; nf++) {
      Face& f = mesh.faces[nf];
      DWORD* nv = f.getAllVerts();
      Point3 v[3];
      for (int ix =0; ix<3; ix++)
         v[ix] = mesh.getVert(nv[ix]);
      int dir = direction(v);
      mesh.setFaceMtlIndex(nf,0);
      //mesh.setFaceMtlIndex(nf,mapDir[dir]);
   }

   mesh.InvalidateTopologyCache();
}

Object* bhkBoxObject::ConvertToType(TimeValue t, Class_ID obtype)
{
   return 0;
   //return SimpleObject::ConvertToType(t,obtype);
}

int bhkBoxObject::CanConvertToType(Class_ID obtype)
{
   return 0;
}


void bhkBoxObject::GetCollapseTypes(Tab<Class_ID> &clist,Tab<TSTR*> &nlist)
{
   Object::GetCollapseTypes(clist, nlist);
}

class BoxObjCreateCallBack: public CreateMouseCallBack {
   bhkBoxObject *ob;
   Point3 p0,p1;
   IPoint2 sp0, sp1;
   BOOL square;
public:
   int proc( ViewExp *vpt,int msg, int point, int flags, IPoint2 m, Matrix3& mat );
   void SetObj(bhkBoxObject *obj) { ob = obj; }
};

int BoxObjCreateCallBack::proc(ViewExp *vpt,int msg, int point, int flags, IPoint2 m, Matrix3& mat ) {
   Point3 d;
   if (msg == MOUSE_FREEMOVE)
   {
      vpt->SnapPreview(m,m,NULL, SNAP_IN_3D);
   }

   else if (msg==MOUSE_POINT||msg==MOUSE_MOVE) {
      switch(point) {
         case 0:
            // Find the node and plug in the wire color
            {
               ULONG handle;
               ob->NotifyDependents(FOREVER, (PartID)&handle, REFMSG_GET_NODE_HANDLE);
               INode *node = GetCOREInterface()->GetINodeByHandle(handle);
               if (node) node->SetWireColor(RGB(255, 0, 0));
            }

            sp0 = m;
            ob->pblock2->SetValue(PB_WIDTH,0,0.0f);
            ob->pblock2->SetValue(PB_LENGTH,0,0.0f);
            ob->pblock2->SetValue(PB_HEIGHT,0,0.0f);
            ob->suspendSnap = TRUE;								
            p0 = vpt->SnapPoint(m,m,NULL,SNAP_IN_3D);
            p1 = p0 + Point3(.01,.01,.01);
            mat.SetTrans(float(.5)*(p0+p1));				
            break;
         case 1:
            sp1 = m;
            p1 = vpt->SnapPoint(m,m,NULL,SNAP_IN_3D);
            p1.z = p0.z +(float).01; 
            if (flags&MOUSE_CTRL) {
               mat.SetTrans(p0);
            } else {
               mat.SetTrans(float(.5)*(p0+p1));
            }
            d = p1-p0;

            square = FALSE;
            if (flags&MOUSE_CTRL) {
               // Constrain to square base
               float len;
               if (fabs(d.x) > fabs(d.y)) len = d.x;
               else len = d.y;
               d.x = d.y = 2.0f * len;
               square = TRUE;
            }

            ob->pblock2->SetValue(PB_WIDTH,0,float(fabs(d.x)));
            ob->pblock2->SetValue(PB_LENGTH,0,float(fabs(d.y)));
            ob->pblock2->SetValue(PB_HEIGHT,0,float(fabs(d.z)));
            ob->pmapParam->Invalidate();										

            if (msg==MOUSE_POINT && (Length(sp1-sp0)<3 || Length(d)<0.1f)) {
                  return CREATE_ABORT;
            }
            break;
         case 2:
#ifdef _OSNAP
            p1.z = p0.z + vpt->SnapLength(vpt->GetCPDisp(p0,Point3(0,0,1),sp1,m,TRUE));
#else
            p1.z = p0.z + vpt->SnapLength(vpt->GetCPDisp(p1,Point3(0,0,1),sp1,m));
#endif				
            if (!square)
               mat.SetTrans(float(.5)*(p0+p1));

            d = p1-p0;
            if (square) 
            {
               // Constrain to square base
               float len;
               if (fabs(d.x) > fabs(d.y)) len = d.x;
               else len = d.y;
               d.x = d.y = 2.0f * len;					
            }

            ob->pblock2->SetValue(PB_WIDTH,0,float(fabs(d.x)));
            ob->pblock2->SetValue(PB_LENGTH,0,float(fabs(d.y)));
            ob->pblock2->SetValue(PB_HEIGHT,0,float(d.z));
            ob->pmapParam->Invalidate();				

            if (msg==MOUSE_POINT) 
            {
               ob->suspendSnap = FALSE;					
               return CREATE_STOP;
            }
            break;
      }
   }
   else
      if (msg == MOUSE_ABORT) {		
         return CREATE_ABORT;
      }

      return TRUE;
}

static BoxObjCreateCallBack boxCreateCB;

CreateMouseCallBack* bhkBoxObject::GetCreateMouseCallBack() 
{
   boxCreateCB.SetObj(this);
   return(&boxCreateCB);
}


BOOL bhkBoxObject::OKtoDisplay(TimeValue t) 
{
   return TRUE;
}

void bhkBoxObject::InvalidateUI() 
{
   param_blk.InvalidateUI(pblock2->LastNotifyParamID());
   if (pmapParam) pmapParam->Invalidate();
}

RefTargetHandle bhkBoxObject::Clone(RemapDir& remap) 
{
   bhkBoxObject* newob = new bhkBoxObject(FALSE);	
   newob->ReplaceReference(0,remap.CloneRef(pblock2));
   newob->ivalid.SetEmpty();	
   BaseClone(this, newob, remap);
   return(newob);
}

int bhkBoxObject::Display(TimeValue t, INode* inode, ViewExp *vpt, int flags) 
{
   Matrix3 m;
   Color color = Color(inode->GetWireColor());
   GraphicsWindow *gw = vpt->getGW();
   Material *mtl = gw->getMaterial();
   m = inode->GetObjectTM(t);
   gw->setTransform(m);
   DWORD rlim = gw->getRndLimits();

   DWORD newrlim = GW_WIREFRAME|GW_Z_BUFFER;
#if VERSION_3DSMAX >= ((5000<<16)+(15<<8)+0) // Version 5+
   newrlim |= GW_EDGES_ONLY;
#endif
   gw->setRndLimits(newrlim);

   if (inode->Selected()) 
      gw->setColor( LINE_COLOR, GetSelColor());
   else if(!inode->IsFrozen() && !inode->Dependent())
      gw->setColor( LINE_COLOR, color);

   //UpdateMesh(t);
   mesh.render( gw, mtl, NULL, COMP_ALL);	
   gw->setRndLimits(rlim);
   return 0;
}