/**********************************************************************
*<
FILE: bhkProxyObj.cpp

DESCRIPTION:	Collision Proxy Object Implementation

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
#include "bhkHelperFuncs.h"
#include <stdmat.h>
#include <meshdelta.h>
#ifndef _countof
#define _countof(x) (sizeof(x)/sizeof((x)[0]))
#endif

extern void compute_convex_hull(Mesh& mesh, Mesh& outmesh);

Class_ID BHKPROXYOBJECT_CLASS_ID = Class_ID(0x3f087788, BHKRIGIDBODYCLASS_DESC.PartB());
class ProxyPickObjectMode;

class bhkProxyValidatorClass : public PBValidator
{
public:
	class bhkProxyObject *mod;
private:
	BOOL Validate(PB2Value &v) 
	{
		INode *node = (INode*) v.r;
		if (node->TestForLoop(FOREVER,(ReferenceMaker *) mod)!=REF_SUCCEED) return FALSE;

		ObjectState os = node->EvalWorldState(0);
		//Allow only tri object derived objects
		if (os.obj->CanConvertToType(triObjectClassID)  && os.obj->SuperClassID() != SHAPE_CLASS_ID) {			
			return TRUE;
		}
		return FALSE;
	};
};

class bhkProxyObject : public SimpleObject2
{
	typedef SimpleObject2 BaseClass;
public:			
   // Class vars
   IParamMap2 *pmapParam;
   IObjParam *ip;
   ProxyPickObjectMode *pickObMode;
   IParamMap2 *pbvParams[1]; //proxy
   Interface		*mIP;
   bhkProxyValidatorClass validator;
   ICustButton		*iPickButton;
   Mesh proxyMesh;
   Point3 proxyPos;
   bool forceRedraw;

   bhkProxyObject(BOOL loading);		
   ~bhkProxyObject();		

   // From Object
   int CanConvertToType(Class_ID obtype);
   Object* ConvertToType(TimeValue t, Class_ID obtype);
   void GetCollapseTypes(Tab<Class_ID> &clist,Tab<TSTR*> &nlist);

   CreateMouseCallBack* GetCreateMouseCallBack();
   void BeginEditParams( IObjParam  *ip, ULONG flags,Animatable *prev);
   void EndEditParams( IObjParam *ip, ULONG flags,Animatable *next);
   RefTargetHandle Clone(RemapDir& remap);
   TCHAR *GetObjectName() { return GetString(IDS_RB_PROXY); }

   int	NumParamBlocks() { return 1; }					// return number of ParamBlocks in this instance
   IParamBlock2* GetParamBlock(int i) { return pblock2; } // return i'th ParamBlock
   IParamBlock2* GetParamBlockByID(BlockID id) { return (pblock2->ID() == id) ? pblock2 : NULL; } // return id'd ParamBlock

   // Animatable methods		
   void DeleteThis() {delete this;}
   Class_ID ClassID() { return BHKPROXYOBJECT_CLASS_ID; } 
   SClass_ID SuperClassID() { return HELPER_CLASS_ID; }

   // From SimpleObject
   void BuildMesh(TimeValue t);
   BOOL OKtoDisplay(TimeValue t);
   void InvalidateUI();

   int Display(TimeValue t, INode* inode, ViewExp *vpt, int flags);

   void BuildEmpty();
   void BuildColBox();
   void BuildColStrips();
   void BuildColPackedStrips();
   void BuildColConvex();
   void BuildColCapsule();
   void BuildColOBB();
   void BuildOptimize(Mesh&mesh);

   void UpdateUI();
   void CreateMesh();

   Modifier *CreateWSMMod(INode *)
   {
	   return NULL;
   }
};

//--- ClassDescriptor and class vars ---------------------------------

// The class descriptor for box
class bhkProxyObjClassDesc : public ClassDesc2 
{
public:
   bhkProxyObjClassDesc();
   int 			   IsPublic() { return 1; }
   void *			Create(BOOL loading = FALSE) {
      return new bhkProxyObject(loading);
   }
   const TCHAR *	ClassName() { return GetString(IDS_RB_PROXY_CLASS); }
   SClass_ID		SuperClassID() { return HELPER_CLASS_ID; }
   Class_ID		   ClassID() { return BHKPROXYOBJECT_CLASS_ID; }
   const TCHAR* 	Category() { return "NifTools"; }

   const TCHAR*	InternalName() { return _T("bhkProxyShape"); }	// returns fixed parsable name (scripter-visible name)
   HINSTANCE		HInstance() { return hInstance; }			// returns owning module handle
};

extern ClassDesc2* GetbhkProxyObjDesc();

// in prim.cpp  - The dll instance handle
extern HINSTANCE hInstance;

//--- Parameter map/block descriptors -------------------------------

// Parameter and ParamBlock IDs
enum { list_params, opt_params, clone_params, subshape_params };  // pblock2 ID
enum 
{ 
   PB_MATERIAL, PB_MESHLIST, PB_BOUND_TYPE, PB_CENTER,
   PB_OPT_ENABLE, PB_MAXEDGE, PB_FACETHRESH, PB_EDGETHRESH, PB_BIAS, 
   PB_LAYER, PB_FILTER, 
};

enum { bv_type_none, bv_type_box, bv_type_shapes, bv_type_packed, bv_type_convex, bv_type_capsule, bv_type_obb };  // pblock ID

static ParamBlockDesc2 param_blk ( 
    list_params, _T("parameters"),  0, NULL, P_AUTO_CONSTRUCT + P_AUTO_UI + P_MULTIMAP, 0,
    //rollout
    4,
    list_params, IDD_PROXYPARAM1, IDS_PARAMS, 0, 0, NULL, 
	opt_params,		IDD_RB_MOD_PANEL1, IDS_OPT_PARAMS, 0, 0, NULL,
	clone_params,	IDD_CLONE_PANEL,   IDS_CLONE_PARAMS, 0, 0, NULL,
	subshape_params,	IDD_RB_MOD_PANEL5,   IDS_LIST_SUBSHAPEPROPS, 0, 0, NULL,

    // params
    PB_MATERIAL, _T("material"), TYPE_INT, P_ANIMATABLE,	IDS_DS_MATERIAL,
      p_default,	NP_INVALID_HVK_MATERIAL,
      end,

    PB_BOUND_TYPE, 	_T("boundType"),	TYPE_INT, 0, IDS_BV_BOUNDING_TYPE,
	  p_default, 		0, 
	  p_range, 			0, 5, 
	  p_ui, 			list_params,	TYPE_RADIO, 7, IDC_RDO_NO_COLL, IDC_RDO_AXIS_ALIGNED_BOX, IDC_RDO_STRIPS_SHAPE, IDC_RDO_PACKED_STRIPS, IDC_RDO_CONVEX, IDC_RDO_CAPSULE, IDC_RDO_OBB,
	  end,

	PB_MESHLIST,   _T("meshProxy"),  TYPE_INODE_TAB,		0,	P_AUTO_UI|P_VARIABLE_SIZE,	IDS_MESHLIST,
	  p_ui,       list_params, TYPE_NODELISTBOX, IDC_LIST1,IDC_ADD,0,IDC_REMOVE,
	  end,

    PB_CENTER,   _T("center"),  TYPE_POINT3,  P_TRANSIENT,	IDS_CENTER,
	  end,


	PB_OPT_ENABLE,	_T("enableOptimize"), TYPE_BOOL, 0, IDS_OPT_ENABLE,
	  p_default, 	FALSE, 
	  p_ui,			opt_params, TYPE_SINGLECHEKBOX, IDC_OPT_ENABLE,
	  end,

	PB_FACETHRESH,	_T("faceThresh"),	TYPE_FLOAT, P_RESET_DEFAULT, IDS_OPT_FACETHRESH,
	  p_default, 	0.1f, 
	  p_range, 		0.0f, 90.0f, 
	  p_ui,			opt_params, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_OPT_FACETHRESH, IDC_OPT_FACETHRESHSPIN, 0.01f,
	  p_uix,		opt_params,
	  end,

	PB_EDGETHRESH,		_T("edgeThresh"),		TYPE_FLOAT, 0, IDS_OPT_EDGETHRESH,
	  p_default, 	0.1f, 
	  p_range, 		0.0f, 90.0f, 
	  p_ui,			opt_params, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_OPT_EDGETHRESH, IDC_OPT_EDGETHRESHSPIN, 0.01f,
	  p_uix,		opt_params,
	  end,

	PB_BIAS,		_T("bias"),		TYPE_FLOAT, 0, IDS_OPT_BIAS,
	  p_default, 	0.1f, 
	  p_range, 		0.0f, 1.0f, 
	  p_ui,			opt_params, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_OPT_BIAS, IDC_OPT_BIASSPIN, 0.01f,
	  p_uix,		opt_params,
	  end,

	PB_MAXEDGE,		_T("maxEdge"),		TYPE_FLOAT, 0, IDS_OPT_MAXEDGE,
	  p_default, 	0.0f, 
	  p_range, 		0.0f, 1000.0f, 
	  p_ui,			opt_params, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_OPT_MAXEDGE, IDC_OPT_MAXEDGESPIN, SPIN_AUTOSCALE,
	  p_uix,		opt_params,
	  end,

	PB_LAYER, _T("layer"), TYPE_INT, P_ANIMATABLE,	IDS_DS_LAYER,
	  p_default,	NP_DEFAULT_HVK_LAYER,
	  end,

    PB_FILTER, _T("filter"), TYPE_INT, P_ANIMATABLE,	IDS_DS_FILTER,
	  p_default,	NP_DEFAULT_HVK_FILTER,
	  p_range, 		0, 255, 
	  p_ui,			subshape_params, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_ED_FILTER, IDC_SP_FILTER, 0.01f,
	  p_uix,		subshape_params,
	  end,

    end
    );

// bug in pb desc? forces us to use this rather than in inline version
static bhkProxyObjClassDesc listDesc;
extern ClassDesc2* GetbhkProxyObjDesc() { return &listDesc; }
bhkProxyObjClassDesc::bhkProxyObjClassDesc() {
   param_blk.SetClassDesc(this);
}


class ProxyPickObjectMode : 
	public PickModeCallback,
	public PickNodeCallback {
public:		
	bhkProxyObject *mod;

	BOOL HitTest(IObjParam *ip,HWND hWnd,ViewExp *vpt,IPoint2 m,int flags);
	BOOL Pick(IObjParam *ip,ViewExp *vpt);
	void EnterMode(IObjParam *ip);
	void ExitMode(IObjParam *ip);
	BOOL RightClick(IObjParam *ip,ViewExp *vpt)	{return TRUE;}
	BOOL Filter(INode *node);		
	PickNodeCallback *GetFilter() {return this;}
};

//--- ProxyPickObjectMode ------------------------------------------------

BOOL ProxyPickObjectMode::Filter(INode *node)
{
	if (node) {
		node->BeginDependencyTest();
		mod->NotifyDependents(FOREVER,0,REFMSG_TEST_DEPENDENCY);
		if (node->EndDependencyTest()) {		
			return FALSE;
		} 
		////added code for looptest
		//if (node->TestForLoop(FOREVER,(ReferenceMaker *) mod)!=REF_SUCCEED)
		//   return FALSE;

		for (int i = 0;i < mod->pblock2->Count(PB_MESHLIST); i++) {
			INode *tnode = NULL;
			mod->pblock2->GetValue(PB_MESHLIST,0,tnode,FOREVER,i);	
			if  (node == tnode)
				return FALSE;
		}

		ObjectState os = node->EvalWorldState(0);
		//added code such that lines are not selected
		if ( (os.obj->IsSubClassOf(triObjectClassID) || os.obj->CanConvertToType(triObjectClassID))
			&& (os.obj->SuperClassID() != SHAPE_CLASS_ID) 
			) 
		{			
			return TRUE;
		}
		if (os.obj->SuperClassID() == HELPER_CLASS_ID && os.obj->ClassID().PartB() == BHKRIGIDBODYCLASS_DESC.PartB() ) {
			return TRUE;
		}
	}
	return FALSE;
}

BOOL ProxyPickObjectMode::HitTest(
							 IObjParam *ip,HWND hWnd,ViewExp *vpt,IPoint2 m,int flags)
{
	INode *node = mod->mIP->PickNode(hWnd,m, this);	//added "this" argument such that the Filter above is used
	return node?TRUE:FALSE;
}

BOOL ProxyPickObjectMode::Pick(IObjParam *ip,ViewExp *vpt)
{
	BOOL rv = FALSE;
	if (INode *node = vpt->GetClosestHit()) {
		theHold.Begin();
		ObjectState os = node->EvalWorldState(0);
		if (os.obj->CanConvertToType(triObjectClassID)) {
			mod->pblock2->Append(PB_MESHLIST,1,&node,1);
			rv = TRUE;
		}
		theHold.Accept(GetString(IDS_ADD_MESH));
	}
	return rv;
}

void ProxyPickObjectMode::EnterMode(IObjParam *ip)
{mod->iPickButton->SetCheck(TRUE);}

void ProxyPickObjectMode::ExitMode(IObjParam *ip)
{mod->iPickButton->SetCheck(FALSE);}

static ProxyPickObjectMode thePickMode;



class ProxyParamDlgProc : public ParamMap2UserDlgProc {
public:
   bhkProxyObject *so;
   HWND thishWnd;
   NpComboBox		mCbMaterial;

   ProxyParamDlgProc(bhkProxyObject *s) {so=s;thishWnd=NULL;}
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

void ProxyParamDlgProc::Update(TimeValue t)
{
   if (!thishWnd) 
      return;
   return;
}

INT_PTR ProxyParamDlgProc::DlgProc(TimeValue t,IParamMap2 *map,HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
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
		 // Disable all types not currently implemented
		  EnableWindow(GetDlgItem(hWnd, IDC_RDO_CAPSULE), CanCalcCapsule() ? TRUE : FALSE);
		  EnableWindow(GetDlgItem(hWnd, IDC_RDO_OBB), CanCalcOrientedBox() ? TRUE : FALSE);
		 //EnableWindow(GetDlgItem(hWnd, IDC_RDO_PACKED_STRIPS), FALSE);

         Update(t);
         break;
      }
   case WM_DESTROY:
	   if (so && so->iPickButton != NULL) {
		   ReleaseICustButton(so->iPickButton);
		   so->iPickButton = NULL;
	   }
	   break;

   case WM_COMMAND:
      switch (LOWORD(wParam)) 
      {
      case IDC_CB_MATERIAL:
         if (HIWORD(wParam)==CBN_SELCHANGE) {
            so->pblock2->SetValue( PB_MATERIAL, 0, mCbMaterial.selection() - 1 );
         }
         break;

	  case IDC_BTN_CLONE:
		  so->CreateMesh();
		  break;
      }
      break;	
   }
   return FALSE;
}

namespace
{
	class CloneMeshDlgProc : public ParamMap2UserDlgProc {
	public:
		bhkProxyObject *mod;
		CloneMeshDlgProc(bhkProxyObject* m) {mod = m;}		
		void DeleteThis() {delete this;}		

		INT_PTR DlgProc (TimeValue t,IParamMap2 *map,HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
		{
			switch (msg) 
			{
			case WM_COMMAND:
				switch (LOWORD(wParam))
				{
				case IDC_BTN_CLONE:
					mod->CreateMesh();
					return TRUE;

				default:
					return FALSE;
				}
			}
			return FALSE;
		}
	};


	// Controller for subshape properies: layer and filter
	class SubShapeDlgProc : public ParamMap2UserDlgProc {
	public:
		bhkProxyObject *mod;
		NpComboBox		mCbLayer;
		SubShapeDlgProc(bhkProxyObject* m) {mod = m;}		
		INT_PTR DlgProc(TimeValue t,IParamMap2 *map,HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam);		
		void DeleteThis() {delete this;}		
	};

	INT_PTR SubShapeDlgProc::DlgProc (TimeValue t,IParamMap2 *map,HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
	{
		switch (msg) 
		{
		case WM_INITDIALOG:
			{
				mCbLayer.init(GetDlgItem(hWnd, IDC_CB_LAYER));
				for (const char **str = NpHvkLayerNames; *str; ++str)
					mCbLayer.add(*str);

				int sel = NP_DEFAULT_HVK_LAYER;
				Interval valid;
				mod->pblock2->GetValue( PB_LAYER, 0, sel, valid);
				mCbLayer.select( sel );

				Update(t);
				break;
			}

		case WM_DESTROY:
			break;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
			case IDC_CB_LAYER:
				if (HIWORD(wParam)==CBN_SELCHANGE) {
					mod->pblock2->SetValue( PB_LAYER, 0, mCbLayer.selection() );
				}
				break;

			default:
				return FALSE;
			}
		}		
		return FALSE;
	}
}

//--- Proxy methods -------------------------------


bhkProxyObject::bhkProxyObject(BOOL loading)
{
	pmapParam = NULL;
	ip = NULL;
	pickObMode = NULL;
	pbvParams[0] = NULL;
	mIP = NULL;	
	iPickButton = NULL;
	validator.mod = this;
	forceRedraw = false;

   SetAFlag(A_PLUGIN1);
   listDesc.MakeAutoParamBlocks(this);
   assert(pblock2);
}

bhkProxyObject::~bhkProxyObject() 
{
   param_blk.SetUserDlgProc();
   if (pmapParam) {
      pmapParam  = NULL;
   }
}

void bhkProxyObject::BeginEditParams(IObjParam *ip,ULONG flags,Animatable *prev)
{
   BaseClass::BeginEditParams(ip,flags,prev);
   mIP = ip;

   listDesc.BeginEditParams(ip,this,flags,prev);
   param_blk.SetUserDlgProc(new ProxyParamDlgProc(this));
   param_blk.SetUserDlgProc(clone_params, new CloneMeshDlgProc(this));
   param_blk.SetUserDlgProc(subshape_params, new SubShapeDlgProc(this));
   pmapParam = pblock2->GetMap(list_params);

   this->ip = ip;
}

void bhkProxyObject::EndEditParams( IObjParam *ip, ULONG flags,Animatable *next )
{		
   param_blk.SetUserDlgProc();

   BaseClass::EndEditParams(ip,flags,next);
   this->ip = NULL;
   pmapParam = NULL;

   if (iPickButton != NULL) {
	   ReleaseICustButton(iPickButton);
	   iPickButton = NULL;
   }

   // tear down the appropriate auto-rollouts
   listDesc.EndEditParams(ip, this, flags, next);
   mIP = NULL;
}

void bhkProxyObject::UpdateUI()
{
   if (ip == NULL)
      return;
   ProxyParamDlgProc* dlg = static_cast<ProxyParamDlgProc*>(pmapParam->GetUserDlgProc());
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

void bhkProxyObject::BuildMesh(TimeValue t)
{
	ivalid = FOREVER;	
	
	int bvType = 0;
	pblock2->GetValue(PB_BOUND_TYPE, 0, bvType, FOREVER, 0);

	BuildEmpty();
	
	switch (bvType)
	{ 
	default:
	case bv_type_none:
		// Delete mesh.
		break;

	case bv_type_box: // box
		BuildColBox();
		break;

	case bv_type_shapes: // Shapes
		BuildColStrips();
		//BuildBox(mesh,,,)
		break;

	case bv_type_packed: // Packed
		BuildColPackedStrips();
		//BuildSphere();
		break;

	case bv_type_convex:
		BuildColConvex();
		//BuildScubaMesh();
		break;

	case bv_type_capsule:
		BuildColCapsule();
		break;

	case bv_type_obb:
		BuildColOBB();
		break;
	}
}

Object* bhkProxyObject::ConvertToType(TimeValue t, Class_ID obtype)
{
	if (obtype == triObjectClassID)
	{
		int bvType = 0;
		pblock2->GetValue(PB_BOUND_TYPE, 0, bvType, FOREVER, 0);
		if (bvType != 0) 
		{
			TriObject *ob = CreateNewTriObject();
#if VERSION_3DSMAX >= ((5000<<16)+(15<<8)+0) // Version 5+
			ob->GetMesh().CopyBasics(proxyMesh);
#else
			ob->GetMesh() = proxyMesh;
#endif
			ob->SetChannelValidity(TOPO_CHAN_NUM,ObjectValidity(t));
			ob->SetChannelValidity(GEOM_CHAN_NUM,ObjectValidity(t));
			return ob;
		}
	}
	return 0;
}

int bhkProxyObject::CanConvertToType(Class_ID obtype)
{
	if (obtype == triObjectClassID) {
		int bvType = 0;
		pblock2->GetValue(PB_BOUND_TYPE, 0, bvType, FOREVER, 0);
		return (bvType != 0) ? TRUE : FALSE;
	}
	return FALSE;
}


void bhkProxyObject::GetCollapseTypes(Tab<Class_ID> &clist,Tab<TSTR*> &nlist)
{
   Object::GetCollapseTypes(clist, nlist);
}

class ProxyObjCreateCallBack: public CreateMouseCallBack {
   bhkProxyObject *ob;
   Point3 p0,p1;
   IPoint2 sp0, sp1;
   BOOL square;
public:
   int proc( ViewExp *vpt,int msg, int point, int flags, IPoint2 m, Matrix3& mat );
   void SetObj(bhkProxyObject *obj) { ob = obj; }
};

int ProxyObjCreateCallBack::proc(ViewExp *vpt,int msg, int point, int flags, IPoint2 m, Matrix3& mat ) {
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
            //ob->suspendSnap = TRUE;								
            p0 = vpt->SnapPoint(m,m,NULL,SNAP_IN_3D);
            p1 = p0 + Point3(.01,.01,.01);
            mat.SetTrans(float(.5)*(p0+p1));				

			ob->pmapParam->Invalidate();				

            if (msg==MOUSE_POINT) 
            {
               //ob->suspendSnap = FALSE;					
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

static ProxyObjCreateCallBack listCreateCB;

CreateMouseCallBack* bhkProxyObject::GetCreateMouseCallBack() 
{
   listCreateCB.SetObj(this);
   return(&listCreateCB);
}


BOOL bhkProxyObject::OKtoDisplay(TimeValue t) 
{
   return TRUE;
}

void bhkProxyObject::InvalidateUI() 
{
   param_blk.InvalidateUI(pblock2->LastNotifyParamID());
   if (pmapParam) pmapParam->Invalidate();
}

RefTargetHandle bhkProxyObject::Clone(RemapDir& remap) 
{
   bhkProxyObject* newob = new bhkProxyObject(FALSE);	
   newob->ReplaceReference(0,remap.CloneRef(pblock2));
   newob->ivalid.SetEmpty();	
   BaseClone(this, newob, remap);
   return(newob);
}

int bhkProxyObject::Display(TimeValue t, INode* inode, ViewExp *vpt, int flags) 
{
	if (forceRedraw)
	{
		Interface *gi = GetCOREInterface();
		gi->ForceCompleteRedraw();
	}

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

	Matrix3 m3(true);

	float size = 20.0f;
	Point3 pts[5];
	// X
	pts[0] = Point3(-size, 0.0f, 0.0f); pts[1] = Point3(size, 0.0f, 0.0f);
	vpt->getGW()->polyline(2, pts, NULL, NULL, FALSE, NULL);

	// Y
	pts[0] = Point3(0.0f, -size, 0.0f); pts[1] = Point3(0.0f, size, 0.0f);
	vpt->getGW()->polyline(2, pts, NULL, NULL, FALSE, NULL);

	// Z
	pts[0] = Point3(0.0f, 0.0f, -size); pts[1] = Point3(0.0f, 0.0f, size);
	vpt->getGW()->polyline(2, pts, NULL, NULL, FALSE, NULL);

	//UpdateMesh(t);

	int bvType = 0;
	pblock2->GetValue(PB_BOUND_TYPE, 0, bvType, FOREVER, 0);
	if (bvType != 0)
	{
		Matrix3 tm = gw->getTransform();
		Matrix3 proxyTM(true);
		proxyTM.SetTranslate(proxyPos);
		gw->setTransform(proxyTM);
		proxyMesh.render( gw, mtl, NULL, COMP_ALL);	
		gw->setTransform(tm);
	}
	gw->setRndLimits(rlim);
	return 0;
}

void bhkProxyObject::BuildEmpty()
{
	BuildBox(mesh, 10.0f, 10.0f, 10.0f);
	proxyMesh.FreeAll();
	proxyPos = Point3::Origin;
	forceRedraw = true;
}

void bhkProxyObject::BuildColBox()
{
	Box3 box; box.Init();
	for (int i = 0;i < pblock2->Count(PB_MESHLIST); i++) {
		INode *tnode = NULL;
		pblock2->GetValue(PB_MESHLIST,0,tnode,FOREVER,i);	
		if (tnode)
		{
			ObjectState os = tnode->EvalWorldState(0);
			Matrix3 wm = tnode->GetNodeTM(0);
			TriObject *tri = (TriObject *)os.obj->ConvertToType(0, Class_ID(TRIOBJ_CLASS_ID, 0));
			if (tri)
			{
				Box3 box2; box2.Init();
				Mesh& mesh = tri->GetMesh();
				CalcAxisAlignedBox(mesh, box2, &wm);
				box += box2;
			}
		}
	}
	BuildBox(proxyMesh, box.Max().y-box.Min().y, box.Max().x-box.Min().x, box.Max().z-box.Min().z);

	MNMesh mn(proxyMesh);
	Matrix3 tm(true);
	tm.SetTranslate(box.Center());
	mn.Transform(tm);
	mn.OutToTri(proxyMesh);

	//proxyPos = box.Center();
	proxyPos = Point3::Origin;
	forceRedraw = true;
}

void bhkProxyObject::BuildColStrips()
{
	proxyMesh.FreeAll();
	MeshDelta md(proxyMesh);
	for (int i = 0;i < pblock2->Count(PB_MESHLIST); i++) {
		INode *tnode = NULL;
		pblock2->GetValue(PB_MESHLIST,0,tnode,FOREVER,i);	
		if (tnode)
		{
			ObjectState os = tnode->EvalWorldState(0);
			Matrix3 wm = tnode->GetNodeTM(0);
			TriObject *tri = (TriObject *)os.obj->ConvertToType(0, Class_ID(TRIOBJ_CLASS_ID, 0));
			if (tri)
			{
				Mesh& mesh = tri->GetMesh();
				MeshDelta tmd (mesh);
				md.AttachMesh(proxyMesh, mesh, wm, 0);
				md.Apply(proxyMesh);
			}
		}
	}
	BuildOptimize(proxyMesh);
	proxyPos = Point3::Origin;
	forceRedraw = true;
}

void bhkProxyObject::BuildColPackedStrips()
{
	BuildColStrips();
}

void bhkProxyObject::BuildColConvex()
{
	proxyMesh.FreeAll();
	MeshDelta md(proxyMesh);
	for (int i = 0;i < pblock2->Count(PB_MESHLIST); i++) {
		INode *tnode = NULL;
		pblock2->GetValue(PB_MESHLIST,0,tnode,FOREVER,i);	
		if (tnode)
		{
			ObjectState os = tnode->EvalWorldState(0);
			Matrix3 wm = tnode->GetNodeTM(0);
			TriObject *tri = (TriObject *)os.obj->ConvertToType(0, Class_ID(TRIOBJ_CLASS_ID, 0));
			if (tri)
			{
				Mesh& mesh = tri->GetMesh();
				MeshDelta tmd (mesh);
				md.AttachMesh(proxyMesh, mesh, wm, 0);
				md.Apply(proxyMesh);
			}
		}
	}
	compute_convex_hull(proxyMesh, proxyMesh);

	BuildOptimize(proxyMesh);

	proxyPos = Point3::Origin;
	forceRedraw = true;
}

void bhkProxyObject::BuildColCapsule()
{
	proxyMesh.FreeAll();
	MeshDelta md(proxyMesh);
	for (int i = 0;i < pblock2->Count(PB_MESHLIST); i++) {
		INode *tnode = NULL;
		pblock2->GetValue(PB_MESHLIST,0,tnode,FOREVER,i);	
		if (tnode)
		{
			ObjectState os = tnode->EvalWorldState(0);
			Matrix3 wm = tnode->GetNodeTM(0);
			TriObject *tri = (TriObject *)os.obj->ConvertToType(0, Class_ID(TRIOBJ_CLASS_ID, 0));
			if (tri)
			{
				Mesh& mesh = tri->GetMesh();
				MeshDelta tmd (mesh);
				md.AttachMesh(proxyMesh, mesh, wm, 0);
				md.Apply(proxyMesh);
			}
		}
	}
	Point3 pt1 = Point3::Origin;
	Point3 pt2 = Point3::Origin;
	float r1 = 0.0;
	float r2 = 0.0;

	if (proxyMesh.getNumVerts() > 3) // Doesn't guarantee that the mesh is not a plane.
	{
		CalcCapsule(proxyMesh, pt1, pt2, r1, r2);
		BuildCapsule(proxyMesh, pt1, pt2, r1, r2);
	}

	proxyPos = Point3::Origin;
	forceRedraw = true;
}

void bhkProxyObject::BuildColOBB()
{
	proxyMesh.FreeAll();
	MeshDelta md(proxyMesh);
	for (int i = 0;i < pblock2->Count(PB_MESHLIST); i++) {
		INode *tnode = NULL;
		pblock2->GetValue(PB_MESHLIST,0,tnode,FOREVER,i);	
		if (tnode)
		{
			ObjectState os = tnode->EvalWorldState(0);
			Matrix3 wm = tnode->GetNodeTM(0);
			TriObject *tri = (TriObject *)os.obj->ConvertToType(0, Class_ID(TRIOBJ_CLASS_ID, 0));
			if (tri)
			{
				Mesh& mesh = tri->GetMesh();
				MeshDelta tmd (mesh);
				md.AttachMesh(proxyMesh, mesh, wm, 0);
				md.Apply(proxyMesh);
			}
		}
	}
	Matrix3 rtm(true);
	Point3 center = Point3::Origin;;
	float udim = 0.0f, vdim = 0.0f, ndim = 0.0f;

	if (proxyMesh.getNumVerts() > 3) // Doesn't guarantee that the mesh is not a plane.
	{
		// First build a convex mesh to put the box around;
		// the method acts oddly if extra vertices are present.
		BuildColConvex();
		CalcOrientedBox(proxyMesh, udim, vdim, ndim, center, rtm);
		BuildBox(proxyMesh, vdim, udim, ndim);
	}

	MNMesh mn(proxyMesh);
	mn.Transform(rtm);
	mn.OutToTri(proxyMesh);

	proxyPos = Point3::Origin;
	forceRedraw = true;
}


void bhkProxyObject::BuildOptimize(Mesh& mesh)
{
	BOOL enable = FALSE;
	pblock2->GetValue(PB_OPT_ENABLE, 0, enable, FOREVER, 0);
	if (enable)
	{
		float maxedge, facethresh, edgethresh, bias;
		pblock2->GetValue(PB_MAXEDGE, 0, maxedge, FOREVER, 0);
		pblock2->GetValue(PB_FACETHRESH, 0, facethresh, FOREVER, 0);
		pblock2->GetValue(PB_EDGETHRESH, 0, edgethresh, FOREVER, 0);
		pblock2->GetValue(PB_BIAS, 0, bias, FOREVER, 0);

		DWORD flags = OPTIMIZE_AUTOEDGE;
		mesh.Optimize(facethresh, edgethresh, bias, maxedge, flags, NULL);
	}
}

void bhkProxyObject::CreateMesh()
{
	if (Interface *gi = this->mIP)
	{
		if (const Mesh* pMesh = &this->proxyMesh)
		{
			if (TriObject *triObject = CreateNewTriObject())
			{
				MNMesh mnmesh(*pMesh);
				Mesh& mesh = triObject->GetMesh();
				mnmesh.OutToTri(mesh);

				INode *node = gi->CreateObjectNode(triObject);
				// Wireframe Red color
				StdMat2 *collMat = NewDefaultStdMat();
				collMat->SetDiffuse(Color(1.0f, 0.0f, 0.0f), 0);
				collMat->SetWire(TRUE);
				collMat->SetFaceted(TRUE);
				gi->GetMaterialLibrary().Add(collMat);
				node->SetMtl(collMat);

				node->SetPrimaryVisibility(FALSE);
				node->SetSecondaryVisibility(FALSE);
				node->BoneAsLine(TRUE);
				node->SetRenderable(FALSE);
				node->SetWireColor( RGB(255,0,0) );

				gi->SelectNode(node);
			}
		}
	}
}