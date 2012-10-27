#pragma warning( disable:4800 )

#include <map>
#include "NifProps.h"
#include "NifStrings.h"
#include "NifPlugins.h"
#include "NifGui.h"
#include "bhkRigidBodyInterface.h"
#include "bhkHelperInterface.h"
#include "bhkHelperFuncs.h"
#include <stdmat.h>
#include <MeshDelta.h>

using namespace std;

#define PBLOCK_REF	0
Class_ID BHKRIGIDBODYMODIFIER_CLASS_ID = Class_ID(0x398fd801, 0x303e44e5);

class bhkValidatorClass : public PBValidator
{
public:
   class bhkRigidBodyModifier *mod;
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
      if (os.obj->SuperClassID() == HELPER_CLASS_ID && NULL != GetInterface(BHKRIGIDBODYINTERFACE_DESC)) {
         return TRUE;
      }
      if (os.obj->SuperClassID() == HELPER_CLASS_ID && os.obj->ClassID().PartB() == BHKRIGIDBODYCLASS_DESC.PartB() ) {
         return TRUE;
      }
      return FALSE;
   };
};

class bhkRigidBodyModifier : public Modifier, public bhkHelperInterface
{

public:

	bhkRigidBodyModifier();
	~bhkRigidBodyModifier();		

   void DeleteThis() { delete this; }
   void GetClassName(TSTR& s) { s = "bhkRigidBodyModifier"; }  
   virtual Class_ID ClassID() { return BHKRIGIDBODYMODIFIER_CLASS_ID; }		
   RefTargetHandle Clone(RemapDir& remap);
   TCHAR *GetObjectName() { return "bhkRigidBodyModifier"; }

   // From modifier
	ChannelMask ChannelsUsed()  {return PART_GEOM|PART_TOPO;}
	ChannelMask ChannelsChanged() {return PART_GEOM;}
//	ChannelMask ChannelsUsed()  { return 0; }
//   ChannelMask ChannelsChanged() { return 0; }
   Class_ID InputType() { return mapObjectClassID; }
   void ModifyObject (TimeValue t, ModContext &mc, ObjectState *os, INode *node);
   Interval LocalValidity(TimeValue t) { return GetValidity(t); }
   Interval GetValidity (TimeValue t);
   BOOL DependOnTopology(ModContext &mc) { return TRUE; }
	void NotifyInputChanged(Interval changeInt, PartID partID, RefMessage message, ModContext *mc);

   void BeginEditParams (IObjParam  *ip, ULONG flags,Animatable *prev);
   void EndEditParams (IObjParam *ip,ULONG flags,Animatable *next);		

	int Display(TimeValue t, INode* inode, ViewExp *vpt, int flagst, ModContext *mc);

	void BuildEmpty(Mesh& mesh);
	void BuildColBox(Mesh& mesh);
	void BuildColSphere(Mesh& mesh);
	void BuildColCapsule(Mesh& mesh);
	void BuildColStrips(Mesh& mesh);
	void BuildColPackedStrips(Mesh& mesh);
	void BuildColConvex(Mesh& mesh);
	void BuildColOBB(Mesh& mesh);
	void BuildOptimize(Mesh& mesh);

	void			SelectionSetChanged(Interface *ip,IUtil *iu);

	void			selectionChanged();
   void        saveState();
   INT_PTR	dlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

   // ParamBlock2 access:
   int NumParamBlocks () { return 1; }
   IParamBlock2* GetParamBlock(int i) 
   { 
      if (i == 0) return pblock;
      return NULL;
   }
   IParamBlock2* GetParamBlockByID(BlockID id) { 
      if (pblock->ID() == id) return pblock;
      return NULL; 
   }

   CreateMouseCallBack* GetCreateMouseCallBack() {return NULL;} 

   virtual	int NumRefs();
   virtual  RefTargetHandle GetReference(int i);
   virtual  void SetReference(int i, RefTargetHandle rtarg);
   virtual  RefResult   NotifyRefChanged(Interval,RefTargetHandle,PartID &,RefMessage);

   int NumSubs() {return 1;}
   Animatable* SubAnim(int i) { return GetReference(i); }
   TSTR SubAnimName(int i) {return GetString (IDS_PARAMS);}

   BOOL SetObject(INode *newNode);
   INode* GetObject();

	void UpdateBVDialogs();

	const Mesh* GetMesh() const
	{
		return &proxyMesh;
	}

	virtual BaseInterface *GetInterface(Interface_ID id)
	{
		if (id == BHKHELPERINTERFACE_DESC)
			return (bhkHelperInterface*)this;
		return Modifier::GetInterface(id);
	}

	void CreateMesh();


public:
   StdMat2* collMat;
	Mesh proxyMesh;

	IParamBlock2 *pblock;
   IParamMap2 *pmapParam;
	IParamMap2 *pbvParams[4]; //box, sphere, capsule, proxy

	Interface		*mIP;
   Tab<INode*> mNodes;

   bhkValidatorClass validator;

	void enableGUI(BOOL object, BOOL hvk, BOOL anim);		
};

extern ClassDesc2* GetbhkRigidBodyModifierDesc();
static ParamBlockDesc2& get_havok_param_blk();

class bhkRigidBodyModifierClassDesc : public ClassDesc2
{
	public:
   bhkRigidBodyModifierClassDesc() {
      get_havok_param_blk().SetClassDesc(this);
   }
	int 			IsPublic() { return TRUE; }
	void *			Create(BOOL loading = FALSE) { 
      return new bhkRigidBodyModifier();
   }
	const TCHAR *	ClassName() { return GetString(IDS_RB_CLASS_NAME); }
	SClass_ID		SuperClassID() { return OSM_CLASS_ID; }
	Class_ID		ClassID() { return BHKRIGIDBODYMODIFIER_CLASS_ID; }
	const TCHAR* 	Category() { return GetString(IDS_CATEGORY); }

	const TCHAR*	InternalName() { return _T("bhkRigidBodyModifier"); }
	HINSTANCE		HInstance() { return hInstance; }
};



// Parameter and ParamBlock IDs
enum { havok_params, opt_params, clone_params, subshape_params };  // pblock ID
enum { PB_BOUND_TYPE, PB_MATERIAL, PB_OPT_ENABLE, PB_MAXEDGE, PB_FACETHRESH, PB_EDGETHRESH, PB_BIAS, PB_LAYER, PB_FILTER, };

enum { havok_params_panel, };

enum { bv_type_none, bv_type_box, bv_type_sphere, bv_type_capsule, bv_type_shapes, bv_type_convex, bv_type_packed, bv_type_obb, };  // pblock ID

static ParamBlockDesc2 havok_param_blk ( 
   havok_params, _T("BoundingVolumes"),  0, NULL, P_AUTO_CONSTRUCT + P_AUTO_UI + P_MULTIMAP, PBLOCK_REF,
   //rollout
    4,
    havok_params,	IDD_RB_MOD_PANEL,  IDS_PARAMS, 0, 0, NULL, 
	opt_params,		IDD_RB_MOD_PANEL1, IDS_OPT_PARAMS, 0, 0, NULL,
	clone_params,	IDD_CLONE_PANEL,   IDS_CLONE_PARAMS, 0, 0, NULL,
	subshape_params,	IDD_RB_MOD_PANEL5,   IDS_LIST_SUBSHAPEPROPS, 0, 0, NULL,

    PB_MATERIAL, _T("material"), TYPE_INT, 0,	IDS_DS_MATERIAL,
      p_default,	NP_INVALID_HVK_MATERIAL,
      end,

	PB_BOUND_TYPE, 	_T("boundType"),	TYPE_INT, 0, IDS_BV_BOUNDING_TYPE,
	  p_default, 		bv_type_shapes, 
	  p_range, 		0, 6, 
	  p_ui, 			havok_params,	TYPE_RADIO, 8, IDC_RDO_NO_COLL, IDC_RDO_AXIS_ALIGNED_BOX, IDC_RDO_SPHERE, IDC_RDO_CAPSULE, IDC_RDO_PROXY_MESH, IDC_RDO_CONVEX, IDC_RDO_PACKED_STRIPS, IDC_RDO_OBB,
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

	PB_LAYER, _T("layer"), TYPE_INT, P_ANIMATABLE,	IDS_DS_LAYER,
	  p_default,	NP_DEFAULT_HVK_LAYER,
	  end,

	PB_FILTER, _T("filter"), TYPE_INT, P_ANIMATABLE,	IDS_DS_FILTER,
	  p_default,	NP_DEFAULT_HVK_FILTER,
	  p_range, 		0, 255, 
	  p_ui,			subshape_params, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_ED_FILTER, IDC_SP_FILTER, 0.01f,
	  p_uix,		subshape_params,
	  end,


   end,
   end
   );

static bhkRigidBodyModifierClassDesc bhkRigidBodyModifierDesc;
ClassDesc2* GetbhkRigidBodyModifierDesc() { return &bhkRigidBodyModifierDesc; }
ParamBlockDesc2& get_havok_param_blk() {
   return havok_param_blk;
}


class bhkRigidBodyModifierDlgProc : public ParamMap2UserDlgProc {
public:
   bhkRigidBodyModifier *mod;
   NpComboBox		mCbMaterial;
   bhkRigidBodyModifierDlgProc(bhkRigidBodyModifier* m) {mod = m;}		
   INT_PTR DlgProc(TimeValue t,IParamMap2 *map,HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam);		
   void DeleteThis() {delete this;}		
};

INT_PTR bhkRigidBodyModifierDlgProc::DlgProc (TimeValue t,IParamMap2 *map,HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
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
		   mod->pblock->GetValue( PB_MATERIAL, 0, sel, valid);
		   mCbMaterial.select( sel + 1 );
		   EnableWindow(GetDlgItem(hWnd, IDC_RDO_CAPSULE), CanCalcCapsule() ? TRUE : FALSE);
 		   EnableWindow(GetDlgItem(hWnd, IDC_RDO_OBB), CanCalcOrientedBox() ? TRUE : FALSE);
		   Update(t);
		   break;
	   }

   case WM_DESTROY:
	   break;

   case WM_COMMAND:
	   switch (LOWORD(wParam))
	   {
	   case IDC_CB_MATERIAL:
		   if (HIWORD(wParam)==CBN_SELCHANGE) {
			   mod->pblock->SetValue( PB_MATERIAL, 0, mCbMaterial.selection() - 1 );
		   }
		   break;

	   case IDC_BTN_CLONE:
		   mod->CreateMesh();
		   break;

	   default:
		   return FALSE;
	   }
   }
   return FALSE;
}

namespace
{
	class CloneMeshDlgProc : public ParamMap2UserDlgProc {
	public:
		bhkRigidBodyModifier *mod;
		CloneMeshDlgProc(bhkRigidBodyModifier* m) {mod = m;}		
		INT_PTR DlgProc(TimeValue t,IParamMap2 *map,HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam);		
		void DeleteThis() {delete this;}		
	};

	INT_PTR CloneMeshDlgProc::DlgProc (TimeValue t,IParamMap2 *map,HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
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


	// Controller for subshape properies: layer and filter
	class SubShapeDlgProc : public ParamMap2UserDlgProc {
	public:
		bhkRigidBodyModifier *mod;
		NpComboBox		mCbLayer;
		SubShapeDlgProc(bhkRigidBodyModifier* m) {mod = m;}		
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
				mod->pblock->GetValue( PB_LAYER, 0, sel, valid);
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
					mod->pblock->SetValue( PB_MATERIAL, 0, mCbLayer.selection() );
				}
				break;

			default:
				return FALSE;
			}
		}		
		return FALSE;
	}
	
}

//--- bhkRigidBodyModifier -------------------------------------------------------

bhkRigidBodyModifier::bhkRigidBodyModifier()
{
	mIP = NULL;	
	pblock = NULL;
   pmapParam = NULL;
   validator.mod = this;
   bhkRigidBodyModifierDesc.MakeAutoParamBlocks(this);

   collMat = NewDefaultStdMat();
   collMat->SetDiffuse(Color(1.0f, 0.0f, 0.0f), 0);
   collMat->SetWire(TRUE);
}

bhkRigidBodyModifier::~bhkRigidBodyModifier()
{
   havok_param_blk.SetUserDlgProc();
   if (pmapParam) {
      DestroyCPParamMap2 (pmapParam);
      pmapParam = NULL;
   }
}

RefTargetHandle bhkRigidBodyModifier::Clone(RemapDir& remap) {
   bhkRigidBodyModifier *mod = new bhkRigidBodyModifier();
   mod->ReplaceReference(0,remap.CloneRef(GetReference(0)));
   mod->ReplaceReference(1,remap.CloneRef(GetReference(1)));
   BaseClone(this, mod, remap);
   return mod;
}


int bhkRigidBodyModifier::NumRefs()
{
   return 1;
}

RefTargetHandle bhkRigidBodyModifier::GetReference(int i)
{
   if (i==0) return pblock;
   return NULL;
}

void bhkRigidBodyModifier::SetReference(int i, RefTargetHandle rtarg)
{
   if (i==0) pblock = (IParamBlock2*)rtarg;
}

RefResult bhkRigidBodyModifier::NotifyRefChanged(
   Interval changeInt, 
   RefTargetHandle hTarget, 
   PartID& partID, 
   RefMessage message)
{
   switch (message) 
   {
   case REFMSG_CHANGE:
      ParamID changing_param = pblock->LastNotifyParamID();
      havok_param_blk.InvalidateUI(changing_param);
      break;
   }
   return REF_SUCCEED;
}

Interval bhkRigidBodyModifier::GetValidity (TimeValue t) {
   Interval ret = FOREVER;
   pblock->GetValidity (t, ret);
   return ret;
}

void bhkRigidBodyModifier::ModifyObject (TimeValue t, ModContext &mc, ObjectState *os, INode *inode) 
{
	OutputDebugString("bhkRigidBodyModifier::ModifyObject\n");
	int bvType = 0;
	pblock->GetValue(PB_BOUND_TYPE, 0, bvType, FOREVER, 0);
	if (bvType == 0)
		return;

	TriObject *tobj=NULL;
	bool needsDelete(false);
	if (os->obj->IsSubClassOf(triObjectClassID)) 
		tobj = (TriObject*)os->obj;
	else {
		if (os->obj->CanConvertToType (triObjectClassID)) {
			tobj = (TriObject *) os->obj->ConvertToType (t, triObjectClassID);
			if (tobj != os->obj) needsDelete = true;
		}
	}
	if (!tobj) return;	// If it's not an object we support, don't go any further.

	Mesh& mesh = tobj->GetMesh();
#if VERSION_3DSMAX >= ((5000<<16)+(15<<8)+0) // Version 5+
	proxyMesh.CopyBasics(mesh);
#else
	proxyMesh = mesh;
#endif

	switch (bvType)
	{ 
	default:
	case bv_type_none:
		// Delete mesh.
		break;

	case bv_type_box: // box
		BuildColBox(proxyMesh);
		break;

	case bv_type_sphere: // sphere
		BuildColSphere(proxyMesh);
		break;

	case bv_type_shapes: // Shapes
		BuildColStrips(proxyMesh);
		BuildOptimize(proxyMesh);
		break;

	case bv_type_capsule:
		BuildColCapsule(proxyMesh);
		break;

	case bv_type_obb:
		BuildColOBB(proxyMesh);
		break;

	case bv_type_convex:
		BuildColConvex(proxyMesh);
		BuildOptimize(proxyMesh);
		//BuildScubaMesh();
		break;

	case bv_type_packed: // Packed
		BuildColPackedStrips(proxyMesh);
		//BuildSphere();
		break;

	}
	
	//
	proxyMesh.buildNormals();

	if (needsDelete) {
		os->obj = tobj;
		os->obj->UnlockObject ();
	}
   //Modifier::ModifyObject(t, mc, os, inode);
}
void bhkRigidBodyModifier::NotifyInputChanged(Interval changeInt, PartID partID, RefMessage message, ModContext *mc)
{
	OutputDebugString("bhkRigidBodyModifier::NotifyInputChanged\n");
   Modifier::NotifyInputChanged(changeInt, partID, message, mc);

}

//struct ParamMapInfo
//{
//	MapID mapid;
//	DWORD idd;
//	DWORD ids;
//};
//static ParamMapInfo pminfo[] = {
//	{bv_box,		IDD_RB_MOD_PANEL1,	IDS_RB_MOD_PANEL1},
//	{bv_sphere,	IDD_RB_MOD_PANEL2,	IDS_RB_MOD_PANEL2},
//	{bv_capsule,IDD_RB_MOD_PANEL3,	IDS_RB_MOD_PANEL3},
//	{bv_mesh,	IDD_RB_MOD_PANEL4,	IDS_RB_MOD_PANEL4},
//};

void bhkRigidBodyModifier::UpdateBVDialogs()
{
	//if (NULL != pmapParam)
	//{
	//	int bvType = pblock2->GetInt(PB_BOUND_TYPE);
	//	if (bvType > 0) // none
	//	{
	//		int adjBvType = bvType - 1; //adjusted bvtype (ignoring none).
	//		for (int i=0; i<_countof(pminfo), ++i){
	//			if (pbvParams[i] != NULL && adjBvType != i)  {
	//				DestroyCPParamMap2( pbvParams[i] );
	//				pbvParams[i] = 0;
	//			}
	//		}
	//		Interface *ifc = GetCOREInterface();
	//		if (pbvParams[adjBvType] == NULL) {
	//			pbvParams[adjBvType] = CreateCPParamMap2( pminfo[adjBvType], pblock2, ifc, hInstance, 
	//				MAKEINTRESOURCE(pminfo[adjBvType]), GetString([adjBvType]), 0);
	//		}
	//	}
	//}
	//else
	//{
	//	// hide em all
	//	for (int i=0; i<_countof(pminfo), ++i){
	//		if (pbvParams[i] != NULL)  {
	//			DestroyCPParamMap2( pbvParams[i] );
	//			pbvParams[i] = 0;
	//		}
	//	}
	//}
}

void bhkRigidBodyModifier::BeginEditParams(IObjParam  *ip, ULONG flags,Animatable *prev)
{
   Modifier::BeginEditParams(ip, flags, prev);
	mIP = ip;

   TimeValue t = ip->GetTime();
   NotifyDependents(Interval(t,t), PART_ALL, REFMSG_BEGIN_EDIT);
   NotifyDependents(Interval(t,t), PART_ALL, REFMSG_MOD_DISPLAY_ON);
   SetAFlag(A_MOD_BEING_EDITED);
   
   bhkRigidBodyModifierDesc.BeginEditParams(ip,this,flags,prev);
   havok_param_blk.SetUserDlgProc(havok_params, new bhkRigidBodyModifierDlgProc(this));
   havok_param_blk.SetUserDlgProc(clone_params, new CloneMeshDlgProc(this));
   havok_param_blk.SetUserDlgProc(subshape_params, new SubShapeDlgProc(this));

	//pmapParam = pblock->GetMap(havok_params);
	//UpdateBVDialogs();
	//pmapParam->GetIRollup()->Hide(1);
	//pmapParam->GetIRollup()->Hide(3);

   // BeginEditRBParams(ip, flags, prev);

   mNodes.ZeroCount();
}
	
void bhkRigidBodyModifier::EndEditParams (IObjParam *ip,ULONG flags,Animatable *next)
{
   Modifier::EndEditParams(ip,flags,next);
   //saveState();
   mNodes.ZeroCount();
	mIP = NULL;

   // NOTE: This flag must be cleared before sending the REFMSG_END_EDIT
   ClearAFlag(A_MOD_BEING_EDITED);
   TimeValue t = ip->GetTime();
   NotifyDependents(Interval(t,t), PART_ALL, REFMSG_END_EDIT);
   NotifyDependents(Interval(t,t), PART_ALL, REFMSG_MOD_DISPLAY_OFF);

   havok_param_blk.SetUserDlgProc();

   if (flags&END_EDIT_REMOVEUI) {
		pmapParam = NULL;
		UpdateBVDialogs();
	}

   // For PB2 we ask the ClassDesc2 to take care of the EndEditParams - NH
   bhkRigidBodyModifierDesc.EndEditParams(ip,this,flags,next);
   //EndEditRBParams(ip, flags, next);
}

class CollisionWireMtl: public Material 
{
public:
   CollisionWireMtl();
};
static CollisionWireMtl swMtl;
CollisionWireMtl::CollisionWireMtl():Material() 
{	
   Ks[0] = Kd[0] = 1.0f;
   Ks[1] = Kd[1] = 0.0f;
   Ks[2] = Kd[2] = 0.0f;
   shininess = (float)0.0;
   shadeLimit = GW_WIREFRAME|GW_BACKCULL;
   selfIllum = (float)1.0;
}

int bhkRigidBodyModifier::Display(TimeValue t, INode* inode, ViewExp *vpt, int flagst, ModContext *mc)
{
	//OutputDebugString("bhkRigidBodyModifier::Display\n");
	if (!Modifier::IsEnabled())
		return 0;

	int bvType = 0;
	pblock->GetValue(PB_BOUND_TYPE, 0, bvType, FOREVER, 0);
	if (bvType == 0)
		return 0;

	Matrix3 m;
	GraphicsWindow *gw = vpt->getGW();

	int wrgb = inode->GetWireColor();
	inode->SetWireColor( RGB(255,0,0) );

	Material *mtl = &swMtl;
	//gw->setTransform(mat);
	if (inode->Selected()) 
		gw->setColor( LINE_COLOR, GetSelColor());
	else if(!inode->IsFrozen())
		gw->setColor(LINE_COLOR,GetUIColor(COLOR_SPACE_WARPS));

	m = inode->GetObjectTM(t);
	gw->setTransform(m);
	DWORD rlim = gw->getRndLimits();

	DWORD newrlim = GW_WIREFRAME/*|GW_Z_BUFFER*/;
#if VERSION_3DSMAX >= ((5000<<16)+(15<<8)+0) // Version 5+
	newrlim |= GW_EDGES_ONLY;
#endif
	gw->setRndLimits(newrlim);

	//UpdateMesh(t);
	gw->setMaterial(swMtl);
	gw->setColor(LINE_COLOR, 1.0f, 0.0f, 0.0f);
	gw->setColor(FILL_COLOR, 1.0f, 0.0f, 0.0f);
	proxyMesh.render( gw, mtl, NULL, COMP_ALL);	

	inode->SetWireColor(wrgb);
	gw->setRndLimits(rlim);

	return 0;
}

void bhkRigidBodyModifier::BuildEmpty(Mesh& mesh)
{
}

void bhkRigidBodyModifier::BuildColBox(Mesh& mesh)
{
	Box3 box; box.Init();
	CalcAxisAlignedBox(mesh, box, NULL);
	BuildBox(mesh, box.Max().y-box.Min().y, box.Max().x-box.Min().x, box.Max().z-box.Min().z);

	MNMesh mn(mesh);
	Matrix3 tm(true);
	tm.Translate(box.Center());
	mn.Transform(tm);
	mn.OutToTri(mesh);
}

void bhkRigidBodyModifier::BuildColSphere(Mesh& mesh)
{
	Point3 center = Point3::Origin;
	float radius = 0.0f;
	CalcAxisAlignedSphere(mesh, center, radius);
	BuildSphere(mesh, radius);

	MNMesh mn(mesh);
	Matrix3 tm(true);
	tm.Translate(center);
	mn.Transform(tm);
	mn.OutToTri(mesh);
}

void bhkRigidBodyModifier::BuildColCapsule(Mesh& mesh)
{
	Point3 pt1 = Point3::Origin;
	Point3 pt2 = Point3::Origin;
	float r1 = 0.0;
	float r2 = 0.0;

	CalcCapsule(mesh, pt1, pt2, r1, r2);
	BuildCapsule(mesh, pt1, pt2, r1, r2);
}

void bhkRigidBodyModifier::BuildColStrips(Mesh& mesh)
{
	MeshDelta tmd (mesh);
	tmd.Apply(mesh);
}

void bhkRigidBodyModifier::BuildColPackedStrips(Mesh& mesh)
{
	BuildColStrips(mesh);
}

void bhkRigidBodyModifier::BuildColConvex(Mesh& mesh)
{
	extern void compute_convex_hull(Mesh& mesh, Mesh& outmesh);

	compute_convex_hull(mesh, mesh);
}

void bhkRigidBodyModifier::BuildColOBB(Mesh& mesh)
{
	Matrix3 rtm(true);
	Point3 center;
	float udim, vdim, ndim;
	// First build a convex mesh to put the box around;
	// the method acts oddly if extra vertices are present.
	BuildColConvex(mesh);
	CalcOrientedBox(mesh, udim, vdim, ndim, center, rtm);
	BuildBox(mesh, vdim, udim, ndim);

	MNMesh mn(mesh);
	mn.Transform(rtm);
	mn.OutToTri(mesh);
}

void bhkRigidBodyModifier::BuildOptimize(Mesh& mesh)
{
	BOOL enable = FALSE;
	pblock->GetValue(PB_OPT_ENABLE, 0, enable, FOREVER, 0);
	if (enable)
	{
		float maxedge, facethresh, edgethresh, bias;
		pblock->GetValue(PB_MAXEDGE, 0, maxedge, FOREVER, 0);
		pblock->GetValue(PB_FACETHRESH, 0, facethresh, FOREVER, 0);
		pblock->GetValue(PB_EDGETHRESH, 0, edgethresh, FOREVER, 0);
		pblock->GetValue(PB_BIAS, 0, bias, FOREVER, 0);

		DWORD flags = OPTIMIZE_AUTOEDGE;
		mesh.Optimize(facethresh, edgethresh, bias, maxedge, flags, NULL);
	}
}

void bhkRigidBodyModifier::CreateMesh()
{
	if (Interface *gi = this->mIP)
	{
		if (const Mesh* pMesh = this->GetMesh())
		{
			if (TriObject *triObject = CreateNewTriObject())
			{
				MNMesh mnmesh(*pMesh);
				Mesh& mesh = triObject->GetMesh();
				mnmesh.buildNormals();
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

				if (gi->GetSelNodeCount() == 1)
				{
					if (INode *snode = gi->GetSelNode(0))
					{
						Matrix3 tm = snode->GetObjTMAfterWSM(0, NULL);
						node->SetNodeTM(0, tm);
					}
				}
				gi->SelectNode(node);
			}
		}
	}
}