/**********************************************************************
*<
FILE: bhkListObj.cpp

DESCRIPTION:	Collision List Object Implementation

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

Class_ID BHKLISTOBJECT_CLASS_ID = Class_ID(0x236508a2, BHKRIGIDBODYCLASS_DESC.PartB());
class ListPickObjectMode;

class bhkListValidatorClass : public PBValidator
{
public:
	class bhkListObject *mod;
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
		// and out objects which support the RigidBodyInterface
		//if (os.obj->SuperClassID() == HELPER_CLASS_ID && NULL != GetInterface(BHKRIGIDBODYINTERFACE_DESC)) {
		//   return TRUE;
		//}
		if (os.obj->SuperClassID() == HELPER_CLASS_ID && os.obj->ClassID().PartB() == BHKRIGIDBODYCLASS_DESC.PartB() ) {
			return TRUE;
		}
		return FALSE;
	};
};

class bhkListObject : public SimpleObject2, public bhkRigidBodyIfcHelper
{
public:			
   // Class vars
   IParamMap2 *pmapParam;
   IObjParam *ip;
   ListPickObjectMode *pickObMode;
   IParamMap2 *pbvParams[1]; //proxy
   Interface		*mIP;
   bhkListValidatorClass validator;
   ICustButton		*iPickButton;

   bhkListObject(BOOL loading);		
   ~bhkListObject();		

   // From Object
   int CanConvertToType(Class_ID obtype);
   Object* ConvertToType(TimeValue t, Class_ID obtype);
   void GetCollapseTypes(Tab<Class_ID> &clist,Tab<TSTR*> &nlist);

   CreateMouseCallBack* GetCreateMouseCallBack();
   void BeginEditParams( IObjParam  *ip, ULONG flags,Animatable *prev);
   void EndEditParams( IObjParam *ip, ULONG flags,Animatable *next);
   RefTargetHandle Clone(RemapDir& remap);
   TCHAR *GetObjectName() { return GetString(IDS_RB_LIST); }

   int	NumParamBlocks() { return 1; }					// return number of ParamBlocks in this instance
   IParamBlock2* GetParamBlock(int i) { return pblock2; } // return i'th ParamBlock
   IParamBlock2* GetParamBlockByID(BlockID id) { return (pblock2->ID() == id) ? pblock2 : NULL; } // return id'd ParamBlock

   // Animatable methods		
   void DeleteThis() {delete this;}
   Class_ID ClassID() { return BHKLISTOBJECT_CLASS_ID; } 
   SClass_ID SuperClassID() { return HELPER_CLASS_ID; }

   // From SimpleObject
   void BuildMesh(TimeValue t);
   BOOL OKtoDisplay(TimeValue t);
   void InvalidateUI();

   virtual IOResult Save(ISave *isave);
   virtual IOResult Load(ILoad *iload);		

   int Display(TimeValue t, INode* inode, ViewExp *vpt, int flags);
   //int HitTest(TimeValue t, INode *inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt);

   void UpdateUI();

   BaseInterface *GetInterface(Interface_ID id) {
	   if (id == BHKRIGIDBODYINTERFACE_DESC)
		   return this;
	   return SimpleObject2::GetInterface(id);
   }
};

//--- ClassDescriptor and class vars ---------------------------------

// The class descriptor for box
class bhkListObjClassDesc : public ClassDesc2 
{
public:
   bhkListObjClassDesc();
   int 			   IsPublic() { return 1; }
   void *			Create(BOOL loading = FALSE) {
      return new bhkListObject(loading);
   }
   const TCHAR *	ClassName() { return GetString(IDS_RB_LIST_CLASS); }
   SClass_ID		SuperClassID() { return HELPER_CLASS_ID; }
   Class_ID		   ClassID() { return BHKLISTOBJECT_CLASS_ID; }
   const TCHAR* 	Category() { return "NifTools"; }

   const TCHAR*	InternalName() { return _T("bhkListShape"); }	// returns fixed parsable name (scripter-visible name)
   HINSTANCE		HInstance() { return hInstance; }			// returns owning module handle
};

extern ClassDesc2* GetbhkListObjDesc();

// in prim.cpp  - The dll instance handle
extern HINSTANCE hInstance;

//--- Parameter map/block descriptors -------------------------------

// Parameter and ParamBlock IDs
enum { list_params, bv_mesh, };  // pblock2 ID
enum 
{ 
   PB_MATERIAL,
   PB_MESHLIST,
};

enum { list_params_panel, };

static ParamBlockDesc2 param_blk ( 
    list_params, _T("parameters"),  0, NULL, P_AUTO_CONSTRUCT + P_AUTO_UI + P_MULTIMAP, 0,
    //rollout
    1,
    list_params, IDD_LISTPARAM, IDS_PARAMS, 0, 0, NULL, 

    // params
    PB_MATERIAL, _T("material"), TYPE_INT, P_ANIMATABLE,	IDS_DS_MATERIAL,
      p_default,	NP_DEFAULT_HVK_MATERIAL,
      end,

	PB_MESHLIST,   _T("meshList"),  TYPE_INODE_TAB,		0,	P_AUTO_UI|P_VARIABLE_SIZE,	IDS_MESHLIST,
	  p_ui,       list_params, TYPE_NODELISTBOX, IDC_LIST1,IDC_ADD,0,IDC_REMOVE,
	  end,

    end
    );

// bug in pb desc? forces us to use this rather than in inline version
static bhkListObjClassDesc listDesc;
extern ClassDesc2* GetbhkListObjDesc() { return &listDesc; }
bhkListObjClassDesc::bhkListObjClassDesc() {
   param_blk.SetClassDesc(this);
}


class ListPickObjectMode : 
	public PickModeCallback,
	public PickNodeCallback {
public:		
	bhkListObject *mod;

	BOOL HitTest(IObjParam *ip,HWND hWnd,ViewExp *vpt,IPoint2 m,int flags);
	BOOL Pick(IObjParam *ip,ViewExp *vpt);
	void EnterMode(IObjParam *ip);
	void ExitMode(IObjParam *ip);
	BOOL RightClick(IObjParam *ip,ViewExp *vpt)	{return TRUE;}
	BOOL Filter(INode *node);		
	PickNodeCallback *GetFilter() {return this;}
};

//--- ListPickObjectMode ------------------------------------------------

BOOL ListPickObjectMode::Filter(INode *node)
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

BOOL ListPickObjectMode::HitTest(
							 IObjParam *ip,HWND hWnd,ViewExp *vpt,IPoint2 m,int flags)
{
	INode *node = mod->mIP->PickNode(hWnd,m, this);	//added "this" argument such that the Filter above is used
	return node?TRUE:FALSE;
}

BOOL ListPickObjectMode::Pick(IObjParam *ip,ViewExp *vpt)
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

void ListPickObjectMode::EnterMode(IObjParam *ip)
{mod->iPickButton->SetCheck(TRUE);}

void ListPickObjectMode::ExitMode(IObjParam *ip)
{mod->iPickButton->SetCheck(FALSE);}

static ListPickObjectMode thePickMode;



class ListParamDlgProc : public ParamMap2UserDlgProc {
public:
   bhkListObject *so;
   HWND thishWnd;
   NpComboBox		mCbMaterial;

   ListParamDlgProc(bhkListObject *s) {so=s;thishWnd=NULL;}
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

void ListParamDlgProc::Update(TimeValue t)
{
   if (!thishWnd) 
      return;
   return;
}

INT_PTR ListParamDlgProc::DlgProc(TimeValue t,IParamMap2 *map,HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
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
      }
      break;	
   }
   return FALSE;
}
//--- List methods -------------------------------


bhkListObject::bhkListObject(BOOL loading)
{
	pmapParam = NULL;
	ip = NULL;
	pickObMode = NULL;
	pbvParams[0] = NULL;
	mIP = NULL;	
	iPickButton = NULL;
	validator.mod = this;

   SetAFlag(A_PLUGIN1);
   listDesc.MakeAutoParamBlocks(this);
   assert(pblock2);
}

bhkListObject::~bhkListObject() 
{
   param_blk.SetUserDlgProc();
   if (pmapParam) {
      pmapParam  = NULL;
   }
}

void bhkListObject::BeginEditParams(IObjParam *ip,ULONG flags,Animatable *prev)
{
   SimpleObject::BeginEditParams(ip,flags,prev);
   mIP = ip;

   //if (pmapParam == NULL) {
   //   pmapParam = CreateCPParamMap2(
   //      0,
   //      pblock2,
   //      GetCOREInterface(),
   //      hInstance,
   //      MAKEINTRESOURCE(IDD_LISTPARAM),
   //      GetString(IDS_PARAMS),
   //      0);
   //}
   listDesc.BeginEditParams(ip,this,flags,prev);
   param_blk.SetUserDlgProc(new ListParamDlgProc(this));
   pmapParam = pblock2->GetMap(list_params);

   BeginEditRBParams(ip, flags, prev);

	//pmapParam->GetIRollup()->Hide(1);


   this->ip = ip;

   //if(pmapParam) {
   //   // A callback for the type in.
   //   pmapParam->SetUserDlgProc(new ListParamDlgProc(this));
   //}

}

void bhkListObject::EndEditParams( IObjParam *ip, ULONG flags,Animatable *next )
{		
   param_blk.SetUserDlgProc();

   SimpleObject::EndEditParams(ip,flags,next);
   this->ip = NULL;
   pmapParam = NULL;
   //if (pmapParam && flags&END_EDIT_REMOVEUI ) {
   //   DestroyCPParamMap2 (pmapParam);
   //   pmapParam = NULL;
   //}

   if (iPickButton != NULL) {
	   ReleaseICustButton(iPickButton);
	   iPickButton = NULL;
   }

   // tear down the appropriate auto-rollouts
   listDesc.EndEditParams(ip, this, flags, next);
   EndEditRBParams(ip, flags, next);
   mIP = NULL;
}

void bhkListObject::UpdateUI()
{
   if (ip == NULL)
      return;
   ListParamDlgProc* dlg = static_cast<ListParamDlgProc*>(pmapParam->GetUserDlgProc());
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

void bhkListObject::BuildMesh(TimeValue t)
{
	extern void BuildBox(Mesh&mesh, float l, float w, float h);

	ivalid = FOREVER;	
	BuildBox(mesh, 10.0f, 10.0f, 10.0f);
}

Object* bhkListObject::ConvertToType(TimeValue t, Class_ID obtype)
{
   return 0;
   //return SimpleObject::ConvertToType(t,obtype);
}

int bhkListObject::CanConvertToType(Class_ID obtype)
{
   return 0;
}


void bhkListObject::GetCollapseTypes(Tab<Class_ID> &clist,Tab<TSTR*> &nlist)
{
   Object::GetCollapseTypes(clist, nlist);
}

class ListObjCreateCallBack: public CreateMouseCallBack {
   bhkListObject *ob;
   Point3 p0,p1;
   IPoint2 sp0, sp1;
   BOOL square;
public:
   int proc( ViewExp *vpt,int msg, int point, int flags, IPoint2 m, Matrix3& mat );
   void SetObj(bhkListObject *obj) { ob = obj; }
};

int ListObjCreateCallBack::proc(ViewExp *vpt,int msg, int point, int flags, IPoint2 m, Matrix3& mat ) {
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
            ob->suspendSnap = TRUE;								
            p0 = vpt->SnapPoint(m,m,NULL,SNAP_IN_3D);
            p1 = p0 + Point3(.01,.01,.01);
            mat.SetTrans(float(.5)*(p0+p1));				

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

static ListObjCreateCallBack listCreateCB;

CreateMouseCallBack* bhkListObject::GetCreateMouseCallBack() 
{
   listCreateCB.SetObj(this);
   return(&listCreateCB);
}


BOOL bhkListObject::OKtoDisplay(TimeValue t) 
{
   return TRUE;
}

void bhkListObject::InvalidateUI() 
{
   param_blk.InvalidateUI(pblock2->LastNotifyParamID());
   if (pmapParam) pmapParam->Invalidate();
}

RefTargetHandle bhkListObject::Clone(RemapDir& remap) 
{
   bhkListObject* newob = new bhkListObject(FALSE);	
   newob->ReplaceReference(0,remap.CloneRef(pblock2));
   newob->ivalid.SetEmpty();	
   BaseClone(this, newob, remap);
   return(newob);
}

int bhkListObject::Display(TimeValue t, INode* inode, ViewExp *vpt, int flags) 
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

   Matrix3 m3(true);

   float size = 5.0f;
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
   //mesh.render( gw, mtl, NULL, COMP_ALL);	
   gw->setRndLimits(rlim);
   return 0;
}

const USHORT kModChunkSelLevel = 0x0100;

IOResult bhkListObject::Save(ISave *isave)
{
	IOResult res;
	res = SimpleObject2::Save(isave);
	if (res == IO_OK)
		res = RBSave(isave);
	return res;
}

IOResult bhkListObject::Load(ILoad *iload)
{
	IOResult res;
	res = SimpleObject2::Load(iload);
	if (res == IO_OK)
		res = RBLoad(iload);
	return res;
}

