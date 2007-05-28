#pragma warning( disable:4800 )

#include <map>
#include "NifProps.h"
#include "NifStrings.h"
#include "NifPlugins.h"
#include "NifGui.h"
#include "bhkRigidBodyInterface.h"

using namespace std;

#define PBLOCK_REF	0
const Class_ID BHKRIGIDBODYMODIFIER_CLASS_ID = Class_ID(0x398fd801, 0x303e44e5);
class PickObjectMode;

extern void BuildBox(Mesh&mesh, float l, float w, float h);
extern void BuildSphere(Mesh&mesh, float radius, int segs=32, int smooth=1, float startAng = 0.0f);
extern void BuildScubaMesh(Mesh &mesh, int segs, int smooth, int llsegs, 
                    float radius1, float radius2, float cylh);

void CalcAxisAlignedBox(Mesh& mesh, Box3& box)
{
   int nv = mesh.getNumVerts();
   box.IncludePoints(mesh.getVertPtr(0), nv, NULL);
}

// Calculate bounding sphere using minimum-volume axis-align bounding box.  Its fast but not a very good fit.
void CalcAxisAlignedSphere(Mesh& mesh, Point3& center, float& radius)
{
   //--Calculate center & radius--//

   //Set lows and highs to first vertex
   int nv = mesh.getNumVerts();
   
   Point3 lows = mesh.getVert(0);
   Point3 highs = mesh.getVert(0);

   //Iterate through the vertices, adjusting the stored values
   //if a vertex with lower or higher values is found
   for ( unsigned int i = 0; i < nv; ++i ) {
      const Point3 & v = mesh.getVert(i);

      if ( v.x > highs.x ) highs.x = v.x;
      else if ( v.x < lows.x ) lows.x = v.x;

      if ( v.y > highs.y ) highs.y = v.y;
      else if ( v.y < lows.y ) lows.y = v.y;

      if ( v.z > highs.z ) highs.z = v.z;
      else if ( v.z < lows.z ) lows.z = v.z;
   }

   //Now we know the extent of the shape, so the center will be the average
   //of the lows and highs
   center = (highs + lows) / 2.0f;

   //The radius will be the largest distance from the center
   Point3 diff;
   float dist2(0.0f), maxdist2(0.0f);
   for ( unsigned int i = 0; i < nv; ++i ) {
      const Point3 & v = mesh.getVert(i);

      diff = center - v;
      dist2 = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
      if ( dist2 > maxdist2 ) maxdist2 = dist2;
   };
   radius = sqrt(maxdist2);
}

// Calculate bounding sphere using average position of the points.  Better fit but slower.
void CalcCenteredSphere(Mesh& mesh, Point3& center, float& radius)
{
   size_t nv = mesh.getNumVerts();
   Point3 sum;
   for (size_t i=0; i<nv; ++i)
      sum += mesh.getVert(i);
   center = sum / float(nv);
   float radsq = 0.0f;
   for (size_t i=0; i<nv; ++i){
      Point3 diff = mesh.getVert(i) - center;
      float mag = diff.LengthSquared();
      radsq = max(radsq, mag);
   }
   radius = Sqrt(radsq);
}


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
      //if (os.obj->SuperClassID() == HELPER_CLASS_ID && NULL != GetInterface(BHKRIGIDBODYINTERFACE_DESC)) {
      //   return TRUE;
      //}
      if (os.obj->SuperClassID() == HELPER_CLASS_ID && os.obj->ClassID().PartB() == BHKRIGIDBODYCLASS_DESC.PartB() ) {
         return TRUE;
      }
      return FALSE;
   };
};

enum { bv_type_none, bv_type_box, bv_type_sphere, bv_type_capsule, bv_type_mesh };  // pblock ID

class bhkRigidBodyModifier : public Modifier, public bhkRigidBodyIfcHelper
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


	void			SelectionSetChanged(Interface *ip,IUtil *iu);

	void			selectionChanged();
   void        saveState();
   BOOL			dlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

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

   BaseInterface *GetInterface(Interface_ID id) {
      if (id == BHKRIGIDBODYINTERFACE_DESC)
         return this;
      return Modifier::GetInterface(id);
   }

public:
   friend class PickObjectMode;

   StdMat2* collMat;
	Mesh mesh;

   PickObjectMode *pickObMode;
	IParamBlock2 *pblock;
   IParamMap2 *pmapParam;
	IParamMap2 *pbvParams[4]; //box, sphere, capsule, proxy

	Interface		*mIP;
   Tab<INode*> mNodes;

	NpComboBox		mCbLayer;
	NpComboBox		mCbMaterial;
	NpComboBox		mCbMotionSystem;
	NpComboBox		mCbQualityType;
   bhkValidatorClass validator;
   ICustButton		*iPickButton;

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
enum { havok_params, bv_box, bv_sphere, bv_capsule, bv_mesh};  // pblock ID
enum { PB_BOUND_TYPE, PB_MESHLIST, };
enum { havok_params_panel, };

class BVTypePBAccessor : public PBAccessor
{ 
public:

	void Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t)    // set from v
	{
		bhkRigidBodyModifier* p = (bhkRigidBodyModifier*)owner;
		switch (id)
		{
		case PB_BOUND_TYPE:
			{
				switch (v.i)
				{ 
            case bv_type_none:
               // Delete mesh.
               break;

            case bv_type_box: // Box
               
               //BuildBox(mesh,,,)
               break;

            case bv_type_sphere: // Sphere
               //BuildSphere();
               break;

            case bv_type_capsule: // Capsule
               //BuildScubaMesh();
               break;

            case bv_type_mesh: 
               break;
				}
			}
		}
	}
};


static BVTypePBAccessor bv_type_accessor;


static ParamBlockDesc2 havok_param_blk ( 
   havok_params, _T("BoundingVolumes"),  0, NULL, P_AUTO_CONSTRUCT + P_AUTO_UI + P_MULTIMAP, PBLOCK_REF,
   //rollout
   2,
   havok_params,	IDD_RB_MOD_PANEL,  IDS_PARAMS, 0, 0, NULL, 
	bv_mesh,			IDD_RB_MOD_PANEL4, IDS_RB_MOD_PANEL4, 0, 0, NULL, 

	PB_BOUND_TYPE, 	_T("boundType"),	TYPE_INT, 0, IDS_BV_BOUNDING_TYPE,
		p_default, 		0, 
		p_range, 		0, 4, 
		p_ui, 			havok_params,	TYPE_RADIO, 5, IDC_RDO_NO_COLL, IDC_RDO_AXIS_ALIGNED_BOX, IDC_RDO_SPHERE, IDC_RDO_CAPSULE, IDC_RDO_PROXY_MESH,
		p_accessor,		&bv_type_accessor,
		end,

	PB_MESHLIST,   _T("meshList"),  TYPE_INODE_TAB,		0,	P_AUTO_UI|P_VARIABLE_SIZE,	IDS_MESHLIST,
		p_ui,       bv_mesh, TYPE_NODELISTBOX, IDC_LIST1,IDC_ADD,0,IDC_REMOVE,
		end,

#if 0
   // params
   PB_RB_LAYER, _T("Layer"), TYPE_INT, 0,	IDS_DS_LAYER,
      p_default,	NP_DEFAULT_HVK_LAYER,
   end,

   PB_RB_MATERIAL, _T("Material"), TYPE_INT, 0,	IDS_DS_MATERIAL,
      p_default,	NP_DEFAULT_HVK_MATERIAL,
   end,

   PB_RB_MASS, _T("mass"), TYPE_FLOAT, 0,	IDS_DS_MASS,
      p_default,	NP_DEFAULT_HVK_MASS,
      p_range,		0.0, 1000.0,
      p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE,  IDC_ED_MASS, IDC_SP_MASS, SPIN_AUTOSCALE,
   end,

   PB_RB_FRICTION, _T("friction"), TYPE_FLOAT, 0,	IDS_DS_FRICTION,
      p_default,	NP_DEFAULT_HVK_FRICTION,
      p_range,		0.0, 10.0,
      p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE,  IDC_ED_FRICTION, IDC_SP_FRICTION, SPIN_AUTOSCALE,
   end,

   PB_RB_RESTITUTION, _T("restitution"), TYPE_FLOAT, 0,	IDS_DS_RESTITUTION,
      p_default,	NP_DEFAULT_HVK_RESTITUTION,
      p_range,		0.0, 10.0,
      p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE,  IDC_ED_RESTITUTION, IDC_SP_RESTITUTION, SPIN_AUTOSCALE,
   end,

   PB_RB_LINEAR_DAMPING, _T("linear_damping"), TYPE_FLOAT, 0,	IDS_DS_LINEAR_DAMPING,
      p_default,	NP_DEFAULT_HVK_LINEAR_DAMPING,
      p_range,		0.0, 10.0,
      p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE,  IDC_ED_LINEAR_DAMPING, IDC_SP_LINEAR_DAMPING, SPIN_AUTOSCALE,
   end,

   PB_RB_ANGULAR_DAMPING, _T("angular_damping"), TYPE_FLOAT, 0,	IDS_DS_ANGULAR_DAMPING,
      p_default,	NP_DEFAULT_HVK_ANGULAR_DAMPING,
      p_range,		0.0, 10.0,
      p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE,  IDC_ED_ANGULAR_DAMPING, IDC_SP_ANGULAR_DAMPING, SPIN_AUTOSCALE,
   end,

   PB_RB_MAX_LINEAR_VELOCITY, _T("max_linear_velocity"), TYPE_FLOAT, 0,	IDS_DS_MAX_LINEAR_VELOCITY,
      p_default,	NP_DEFAULT_HVK_MAX_LINEAR_VELOCITY,
      p_range,		0.0, 10.0,
      p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE,  IDC_ED_MAX_LINEAR_VELOCITY, IDC_SP_MAX_LINEAR_VELOCITY, SPIN_AUTOSCALE,
   end,

   PB_RB_MAX_ANGULAR_VELOCITY, _T("max_angular_velocity"), TYPE_FLOAT, 0,	IDS_DS_MAX_ANGULAR_VELOCITY,
      p_default,	NP_DEFAULT_HVK_MAX_ANGULAR_VELOCITY,
      p_range,		0.0, 10.0,
      p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE,  IDC_ED_MAX_ANGULAR_VELOCITY, IDC_SP_MAX_ANGULAR_VELOCITY, SPIN_AUTOSCALE,
   end,

   PB_RB_PENETRATION_DEPTH, _T("penetration_depth"), TYPE_FLOAT, 0,	IDS_DS_PENETRATION_DEPTH,
      p_default,	NP_DEFAULT_HVK_PENETRATION_DEPTH,
      p_range,		0.0, 10.0,
      p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE,  IDC_ED_PENETRATION_DEPTH, IDC_SP_PENETRATION_DEPTH, SPIN_AUTOSCALE,
   end,

   PB_RB_MOTION_SYSTEM, _T("motion_system"), TYPE_INT, 0, IDS_DS_MOTION_SYSTEM,
      p_default,	NP_DEFAULT_HVK_MOTION_SYSTEM,
   end,

   PB_RB_QUALITY_TYPE, _T("quality_type"), TYPE_INT, 0,	IDS_DS_QUALITY_TYPE,
      p_default,	NP_DEFAULT_HVK_QUALITY_TYPE,
   end,
#endif


   //PB_HK_NODE, 		_T("node"), 		TYPE_INODE, 	0,		IDS_DS_NODE,
   //   p_ui, 			TYPE_PICKNODEBUTTON,    IDC_PICK_NODE, 
   //   p_prompt,      IDS_PICKNODE_PROMPT,
   //   //p_validator, &validator,
   end,
   end
   );

static bhkRigidBodyModifierClassDesc bhkRigidBodyModifierDesc;
ClassDesc2* GetbhkRigidBodyModifierDesc() { return &bhkRigidBodyModifierDesc; }
ParamBlockDesc2& get_havok_param_blk() {
   return havok_param_blk;
}


class PickObjectMode : 
   public PickModeCallback,
   public PickNodeCallback {
public:		
   bhkRigidBodyModifier *mod;

   BOOL HitTest(IObjParam *ip,HWND hWnd,ViewExp *vpt,IPoint2 m,int flags);
   BOOL Pick(IObjParam *ip,ViewExp *vpt);
   void EnterMode(IObjParam *ip);
   void ExitMode(IObjParam *ip);
   BOOL RightClick(IObjParam *ip,ViewExp *vpt)	{return TRUE;}
   BOOL Filter(INode *node);		
   PickNodeCallback *GetFilter() {return this;}
};

//--- PickObjectMode ------------------------------------------------

BOOL PickObjectMode::Filter(INode *node)
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

      for (int i = 0;i < mod->pblock->Count(PB_MESHLIST); i++) {
         INode *tnode = NULL;
         mod->pblock->GetValue(PB_MESHLIST,0,tnode,FOREVER,i);	
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

BOOL PickObjectMode::HitTest(
                             IObjParam *ip,HWND hWnd,ViewExp *vpt,IPoint2 m,int flags)
{
   INode *node = mod->mIP->PickNode(hWnd,m, this);	//added "this" argument such that the Filter above is used
   return node?TRUE:FALSE;
}

BOOL PickObjectMode::Pick(IObjParam *ip,ViewExp *vpt)
{
   BOOL rv = FALSE;
   if (INode *node = vpt->GetClosestHit()) {
      theHold.Begin();
      ObjectState os = node->EvalWorldState(0);
      if (os.obj->CanConvertToType(triObjectClassID)) {
         mod->pblock->Append(PB_MESHLIST,1,&node,1);
         rv = TRUE;
      }
      theHold.Accept(GetString(IDS_ADD_MESH));
   }
   return rv;
}

void PickObjectMode::EnterMode(IObjParam *ip)
{mod->iPickButton->SetCheck(TRUE);}

void PickObjectMode::ExitMode(IObjParam *ip)
{mod->iPickButton->SetCheck(FALSE);}

static PickObjectMode thePickMode;

class bhkRigidBodyModifierDlgProc : public ParamMap2UserDlgProc {
public:
   bhkRigidBodyModifier *mod;
   bhkRigidBodyModifierDlgProc(bhkRigidBodyModifier* m) {mod = m;}		
   BOOL DlgProc(TimeValue t,IParamMap2 *map,HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam);		
   void DeleteThis() {delete this;}		
};

BOOL bhkRigidBodyModifierDlgProc::DlgProc (TimeValue t,IParamMap2 *map,HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
   switch (msg) 
   {
   case WM_INITDIALOG:
      //mod->iPickButton = GetICustButton(GetDlgItem(hWnd, IDC_ADD));
      //mod->iPickButton->SetType(CBT_CHECK);
      //mod->iPickButton->SetHighlightColor(GREEN_WASH);
      break;

   case WM_DESTROY:
      if (mod && mod->iPickButton != NULL) {
         ReleaseICustButton(mod->iPickButton);
         mod->iPickButton = NULL;
      }
      break;

   case WM_COMMAND:
      switch (LOWORD(wParam))
      {
      case IDC_ADD:
         {
            thePickMode.mod = mod;
            GetCOREInterface()->SetPickMode(&thePickMode);
            break;
         }
      }
      break;

   default:
      return FALSE;
   }

   return TRUE;
}

//--- bhkRigidBodyModifier -------------------------------------------------------

bhkRigidBodyModifier::bhkRigidBodyModifier()
{
	mIP = NULL;	
	pblock = NULL;
   pmapParam = NULL;
   pickObMode = NULL;
   validator.mod = this;
   iPickButton = NULL;
   bhkRigidBodyModifierDesc.MakeAutoParamBlocks(this);

   collMat = NewDefaultStdMat();
   collMat->SetDiffuse(Color(1.0f, 0.0f, 0.0f), 0);
   collMat->SetWire(TRUE);
}

bhkRigidBodyModifier::~bhkRigidBodyModifier()
{
   havok_param_blk.SetUserDlgProc();
#if 0
   if (NULL != pickObMode) {
      delete pickObMode;
      pickObMode = NULL;
   }
#endif
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
   return 2;
}

RefTargetHandle bhkRigidBodyModifier::GetReference(int i)
{
   if (i==0) return pblock;
   if (i==1) return GetRBBlock();
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
      if (changing_param == PB_MESHLIST)
      {
         if (partID == PART_TOPO) {
            NotifyDependents(FOREVER,GEOM_CHANNEL,REFMSG_CHANGE);
         }
      }
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

	Box3 box; box.Init();
   if (TriObject *tri = (TriObject *)os->obj->ConvertToType(t, Class_ID(TRIOBJ_CLASS_ID, 0)))
   {
      CalcAxisAlignedBox(tri->GetMesh(), box);
      BuildBox(mesh, box.Max().x-box.Min().x, box.Max().y-box.Min().y, box.Max().z-box.Min().z);
   }
   //Modifier::ModifyObject(t, mc, os, inode);
}
void bhkRigidBodyModifier::NotifyInputChanged(Interval changeInt, PartID partID, RefMessage message, ModContext *mc)
{
	OutputDebugString("bhkRigidBodyModifier::NotifyInputChanged\n");
   Modifier::NotifyInputChanged(changeInt, partID, message, mc);

}

struct ParamMapInfo
{
	MapID mapid;
	DWORD idd;
	DWORD ids;
};
static ParamMapInfo pminfo[] = {
	{bv_box,		IDD_RB_MOD_PANEL1,	IDS_RB_MOD_PANEL1},
	{bv_sphere,	IDD_RB_MOD_PANEL2,	IDS_RB_MOD_PANEL2},
	{bv_capsule,IDD_RB_MOD_PANEL3,	IDS_RB_MOD_PANEL3},
	{bv_mesh,	IDD_RB_MOD_PANEL4,	IDS_RB_MOD_PANEL4},
};

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
   //havok_param_blk.SetUserDlgProc(bv_mesh, new bhkRigidBodyModifierDlgProc(this));

	pmapParam = pblock->GetMap(havok_params);
	UpdateBVDialogs();
	//pmapParam->GetIRollup()->Hide(1);
	//pmapParam->GetIRollup()->Hide(3);

   BeginEditRBParams(ip, flags, prev);

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

   if (iPickButton != NULL) {
      ReleaseICustButton(iPickButton);
      iPickButton = NULL;
   }

   // For PB2 we ask the ClassDesc2 to take care of the EndEditParams - NH
   bhkRigidBodyModifierDesc.EndEditParams(ip,this,flags,next);
   EndEditRBParams(ip, flags, next);
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
	mesh.render( gw, mtl, NULL, COMP_ALL);	

   inode->SetWireColor(wrgb);
	gw->setRndLimits(rlim);

	return 0;
}