
#include "stdafx.h"

Class_ID BHKLISTOBJECT_CLASS_ID = Class_ID( 0x236508a2, BHKRIGIDBODYCLASS_DESC.PartB( ) );
class ListPickObjectMode;

class bhkListValidatorClass : public PBValidator
{
public:
	class bhkListObject* mod;

private:

	BOOL Validate( PB2Value& v )
	{
		INode* node = (INode*) v.r;
		if( node->TestForLoop( FOREVER, (ReferenceMaker*) mod ) != REF_SUCCEED )
			return FALSE;

		ObjectState os = node->EvalWorldState( 0 );

		// Allow only tri object derived objects
		if( os.obj->CanConvertToType( triObjectClassID ) && os.obj->SuperClassID( ) != SHAPE_CLASS_ID )
			return TRUE;

		if( os.obj->SuperClassID( ) == HELPER_CLASS_ID && os.obj->ClassID( ).PartB( ) == BHKRIGIDBODYCLASS_DESC.PartB( ) )
			return TRUE;

		return FALSE;
	}
};

class bhkListObject : public SimpleObject2, public bhkRigidBodyIfcHelper
{
public:
	// Class vars
	IParamMap2*			pmapParam;
	IObjParam*			ip;
	ListPickObjectMode* pickObMode;
	IParamMap2*			pbvParams[ 1 ]; // Proxy
	Interface*			mIP;
	bhkListValidatorClass validator;
	ICustButton*		iPickButton;

	bhkListObject( BOOL loading );
	~bhkListObject( );

	// From Object
	int						CanConvertToType( Class_ID obtype ) { return 0; }
	Object*					ConvertToType( TimeValue t, Class_ID obtype ) { return 0; }
	void					GetCollapseTypes( Tab<Class_ID>& clist, Tab<TSTR*>& nlist ) { Object::GetCollapseTypes( clist, nlist ); }

	CreateMouseCallBack*	GetCreateMouseCallBack( );

	void					BeginEditParams( IObjParam* ip, ULONG flags, Animatable* prev );
	void					EndEditParams( IObjParam* ip, ULONG flags, Animatable* next );

	RefTargetHandle			Clone( RemapDir& remap );

	const TCHAR*			GetObjectName( ) { return GetString( IDS_RB_LIST ); }

	// Return number of ParamBlocks in this instance
	int						NumParamBlocks( ) { return 1; }
	// Return the i'th ParamBlock
	IParamBlock2*			GetParamBlock( int i ) { return pblock2; }
	// Return id'd ParamBlock
	IParamBlock2*			GetParamBlockByID( BlockID id ) { return ( pblock2->ID( ) == id ) ? pblock2 : NULL; }

	// Animatable methods
	void					DeleteThis( ) { delete this; }
	Class_ID				ClassID( ) { return BHKLISTOBJECT_CLASS_ID; }
	SClass_ID				SuperClassID( ) { return HELPER_CLASS_ID; }

	// From SimpleObject
	void					BuildMesh( TimeValue t );
	BOOL					OKtoDisplay( TimeValue t ) { return TRUE; }
	void					InvalidateUI( );

	virtual IOResult		Save( ISave* isave );
	virtual IOResult		Load( ILoad* iload );

	int						Display( TimeValue t, INode* inode, ViewExp* vpt, int flags );
	void					UpdateUI( );

	BaseInterface* GetInterface( Interface_ID id )
	{
		if( id == BHKRIGIDBODYINTERFACE_DESC )
			return this;

		return SimpleObject2::GetInterface( id );
	}
};

//--- ClassDescriptor and class vars ---------------------------------

// The class descriptor for list
class bhkListObjClassDesc : public ClassDesc2
{
public:
	bhkListObjClassDesc( );

	int				IsPublic( ) { return 1; }
	void*			Create( BOOL loading = FALSE ) {  return new bhkListObject( loading ); }
	const TCHAR*	ClassName( ) { return GetString( IDS_RB_LIST_CLASS ); }
	SClass_ID		SuperClassID( ) { return HELPER_CLASS_ID; }
	Class_ID		ClassID( ) { return BHKLISTOBJECT_CLASS_ID; }
	const TCHAR*	Category( ) { return _T( "NifTools" ); }

	// Returns fixed parsable name (scripter-visible name )
	const TCHAR*	InternalName( ) { return _T( "bhkListShape" ); }
	// Returns owning module handle
	HINSTANCE		HInstance( ) { return hInstance; }
};

extern ClassDesc2* GetbhkListDesc( );

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
      p_end,

	PB_MESHLIST,   _T("meshList"),  TYPE_INODE_TAB,		0,	P_AUTO_UI|P_VARIABLE_SIZE,	IDS_MESHLIST,
	  p_ui,       list_params, TYPE_NODELISTBOX, IDC_LIST1,IDC_ADD,0,IDC_REMOVE,
	  p_end,

    p_end
);

// bug in pb desc? forces us to use this rather than in inline version
static bhkListObjClassDesc listDesc;
extern ClassDesc2* GetbhkListDesc( ) { return &listDesc; }

bhkListObjClassDesc::bhkListObjClassDesc( ) 
{
	param_blk.SetClassDesc( this );
}

class ListPickObjectMode : public PickModeCallback, public PickNodeCallback
{
public:
	bhkListObject* mod;

	BOOL			  HitTest( IObjParam* ip, HWND hWnd, ViewExp* vpt, IPoint2 m, int flags );
	BOOL			  Pick( IObjParam* ip, ViewExp* vpt );
	void			  EnterMode( IObjParam* ip ) { mod->iPickButton->SetCheck( TRUE ); }
	void			  ExitMode( IObjParam* ip ) { mod->iPickButton->SetCheck( FALSE ); }
	BOOL			  RightClick( IObjParam* ip, ViewExp* vpt ) { return TRUE; }
	BOOL			  Filter( INode* node );
	PickNodeCallback* GetFilter( ) { return this; }
};

static ListPickObjectMode thePickMode;

class ListParamDlgProc : public ParamMap2UserDlgProc
{
public:
	bhkListObject* so;
	HWND thishWnd;
	NpComboBox mCbMaterial;

	ListParamDlgProc( bhkListObject* s )
	{
		so = s;
		thishWnd = NULL;
	}

	INT_PTR		DlgProc( TimeValue t, IParamMap2* map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );
	void		Update( TimeValue t );
	void		DeleteThis( ) { delete this; }

	//--- ParamDlgProc --------------------------------
	void TurnSpinner( HWND hWnd, int SpinNum, BOOL ison )
	{
		ISpinnerControl* spin2 = GetISpinner( GetDlgItem( hWnd, SpinNum ) );
		if( ison )
			spin2->Enable( );

		else
			spin2->Disable( );

		ReleaseISpinner( spin2 );
	}
};

class ListObjCreateCallBack : public CreateMouseCallBack
{
	bhkListObject* ob;
	Point3 p0;
	Point3 p1;
	IPoint2 sp0;
	IPoint2 sp1;
	BOOL square;

public:
	int		proc( ViewExp* vpt, int msg, int point, int flags, IPoint2 m, Matrix3& mat );
	void	SetObj( bhkListObject* obj ) { ob = obj; }
};

static ListObjCreateCallBack listCreateCB;

// =============================================================
// ListPickObjectMode
// =============================================================

BOOL ListPickObjectMode::Filter( INode* node )
{
	if( node )
	{
		node->BeginDependencyTest( );
		mod->NotifyDependents( FOREVER, 0, REFMSG_TEST_DEPENDENCY );

		if( node->EndDependencyTest( ) )
			return FALSE;

		for( int i = 0; i < mod->pblock2->Count( PB_MESHLIST ); i++ )
		{
			INode* tnode = NULL;
			mod->pblock2->GetValue( PB_MESHLIST, 0, tnode, FOREVER );
			
			if( node == tnode )
				return FALSE;
		}

		ObjectState os = node->EvalWorldState( 0 );

		// Added code such that lines are not selected
		if( ( os.obj->IsSubClassOf( triObjectClassID ) || os.obj->CanConvertToType( triObjectClassID ) )
			&& ( os.obj->SuperClassID( ) != SHAPE_CLASS_ID ) )
		{
			return TRUE;
		}

		if( os.obj->SuperClassID( ) == HELPER_CLASS_ID && os.obj->ClassID( ).PartB( ) == BHKRIGIDBODYCLASS_DESC.PartB( ) )
			return TRUE;
	}

	return FALSE;
}

BOOL ListPickObjectMode::HitTest( IObjParam* ip, HWND hWnd, ViewExp* vpt, IPoint2 m, int flags )
{
	INode* node = mod->mIP->PickNode( hWnd, m, this ); //added "this" argument such that the Filter above is used
	return node ? TRUE : FALSE;
}

BOOL ListPickObjectMode::Pick( IObjParam* ip, ViewExp* vpt )
{
	BOOL rv = FALSE;
	if( INode* node = vpt->GetClosestHit( ) )
	{
		theHold.Begin( );

		ObjectState os = node->EvalWorldState( 0 );
		if( os.obj->CanConvertToType( triObjectClassID ) )
		{
			mod->pblock2->Append( PB_MESHLIST, 1, &node, 1 );
			rv = TRUE;
		}

		theHold.Accept( GetString( IDS_ADD_MESH ) );
	}

	return rv;
}

// =============================================================
// ListParamDlgProc
// =============================================================

void ListParamDlgProc::Update( TimeValue t )
{
	if( !thishWnd )
		return;

	return;
}

INT_PTR ListParamDlgProc::DlgProc( TimeValue t, IParamMap2* map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	thishWnd = hWnd;

	switch( msg )
	{
	case WM_INITDIALOG:
		{
			mCbMaterial.init( GetDlgItem( hWnd, IDC_CB_MATERIAL ) );
			mCbMaterial.add( _T( "<Default>" ) );

			for( const wchar_t** str = NpHvkMaterialNames; *str; ++str )
				mCbMaterial.add( *str );

			Interval valid;
			int sel = NP_INVALID_HVK_MATERIAL;

			so->pblock2->GetValue( PB_MATERIAL, 0, sel, valid );
			mCbMaterial.select( sel + 1 );

			Update( t );
		}
		break;

	case WM_DESTROY:
		if( so && so->iPickButton != NULL )
		{
			ReleaseICustButton( so->iPickButton );
			so->iPickButton = NULL;
		}
		break;

	case WM_COMMAND:
		switch( LOWORD( wParam ) )
		{
		case IDC_CB_MATERIAL:
			if( HIWORD( wParam ) == CBN_SELCHANGE )
				so->pblock2->SetValue( PB_MATERIAL, 0, mCbMaterial.selection( ) - 1 );
			break;
		}
		break;
	}

	return FALSE;
}

// =============================================================
// bhkListObject
// =============================================================

bhkListObject::bhkListObject( BOOL loading )
{
	pmapParam = NULL;
	ip = NULL;
	pickObMode = NULL;
	pbvParams[ 0 ] = NULL;
	mIP = NULL;
	iPickButton = NULL;
	validator.mod = this;

	SetAFlag( A_PLUGIN1 );
	listDesc.MakeAutoParamBlocks( this );
	
	assert( pblock2 );
}

bhkListObject::~bhkListObject( )
{
	param_blk.SetUserDlgProc( );
	if( pmapParam )
		pmapParam = NULL;
}

void bhkListObject::BeginEditParams( IObjParam* ip, ULONG flags, Animatable* prev )
{
	SimpleObject::BeginEditParams( ip, flags, prev );
	mIP = ip;

	listDesc.BeginEditParams( ip, this, flags, prev );
	param_blk.SetUserDlgProc( new ListParamDlgProc( this ) );
	pmapParam = pblock2->GetMap( list_params );

	BeginEditRBParams( ip, flags, prev );

	this->ip = ip;
}

void bhkListObject::EndEditParams( IObjParam* ip, ULONG flags, Animatable* next )
{
	param_blk.SetUserDlgProc( );

	SimpleObject::EndEditParams( ip, flags, next );
	this->ip = NULL;
	pmapParam = NULL;

	// tear down the appropriate auto-rollouts
	listDesc.EndEditParams( ip, this, flags, next );
	EndEditRBParams( ip, flags, next );
	mIP = NULL;
}

void bhkListObject::UpdateUI( )
{
	if( ip == NULL )
		return;

	ListParamDlgProc* dlg = static_cast<ListParamDlgProc*>( pmapParam->GetUserDlgProc( ) );
	dlg->Update( ip->GetTime( ) );
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

void bhkListObject::BuildMesh( TimeValue t )
{
	extern void BuildBox( Mesh& mesh, float l, float w, float h );

	ivalid = FOREVER;
	BuildBox( mesh, 10.0f, 10.0f, 10.0f );
}

CreateMouseCallBack* bhkListObject::GetCreateMouseCallBack( )
{
	listCreateCB.SetObj( this );
	return &listCreateCB;
}

void bhkListObject::InvalidateUI( )
{
	param_blk.InvalidateUI( pblock2->LastNotifyParamID( ) );
	if( pmapParam )
		pmapParam->Invalidate( );
}

RefTargetHandle bhkListObject::Clone( RemapDir& remap )
{
	bhkListObject* newob = new bhkListObject( FALSE );

	newob->ReplaceReference( 0, remap.CloneRef( pblock2 ) );
	newob->ivalid.SetEmpty( );

	BaseClone( this, newob, remap );
	return newob;
}

int bhkListObject::Display( TimeValue t, INode* inode, ViewExp* vpt, int flags )
{
	Matrix3 m;
	Color color = Color( inode->GetWireColor( ) );
	GraphicsWindow* gw = vpt->getGW( );
	Material* mtl = gw->getMaterial( );

	m = inode->GetObjectTM( t );
	gw->setTransform( m );

	DWORD rlim = gw->getRndLimits( );
	DWORD newrlim = GW_WIREFRAME | GW_Z_BUFFER;

#if VERSION_3DSMAX >= ((5000<<16)+(15<<8)+0) // Version 5+

	newrlim |= GW_EDGES_ONLY;

#endif

	gw->setRndLimits( newrlim );

	if( inode->Selected( ) )
		gw->setColor( LINE_COLOR, GetSelColor( ) );

	else if( !inode->IsFrozen( ) && !inode->Dependent( ) )
		gw->setColor( LINE_COLOR, color );

	float size = 5.0f;
	Point3 pts[ 5 ];

	// X
   pts[ 0 ] = Point3( -size, 0.0f, 0.0f ); 
   pts[ 1 ] = Point3( size, 0.0f, 0.0f );
   vpt->getGW( )->polyline( 2, pts, NULL, NULL, FALSE, NULL );

   // Y
   pts[ 0 ] = Point3( 0.0f, -size, 0.0f ); 
   pts[ 1 ] = Point3( 0.0f, size, 0.0f );
   vpt->getGW( )->polyline( 2, pts, NULL, NULL, FALSE, NULL );

   // Z
   pts[ 0 ] = Point3( 0.0f, 0.0f, -size ); 
   pts[ 1 ] = Point3( 0.0f, 0.0f, size );
   vpt->getGW( )->polyline( 2, pts, NULL, NULL, FALSE, NULL );

   gw->setRndLimits( rlim );

   return 0;
}

const USHORT kModChunkSelLevel = 0x0100;

IOResult bhkListObject::Save( ISave* isave )
{
	IOResult res;

	res = SimpleObject2::Save( isave );
	if( res == IO_OK )
		res = RBSave( isave );

	return res;
}

IOResult bhkListObject::Load( ILoad* iload )
{
	IOResult res;

	res = SimpleObject2::Load( iload );
	if( res == IO_OK )
		res = RBLoad( iload );

	return res;
}

// =============================================================
// ListObjCreateCallBack
// =============================================================

int ListObjCreateCallBack::proc(ViewExp *vpt,int msg, int point, int flags, IPoint2 m, Matrix3& mat )
{
	Point3 d;
	if( msg == MOUSE_FREEMOVE )
		vpt->SnapPreview( m, m, NULL, SNAP_IN_3D );

	else if( msg == MOUSE_POINT || msg == MOUSE_MOVE )
	{
		switch( point )
		{
		case 0: // Find the node and plug in the wire color
			{
				ULONG handle;
				ob->NotifyDependents( FOREVER, (PartID) &handle, REFMSG_GET_NODE_HANDLE );
				INode* node = GetCOREInterface( )->GetINodeByHandle( handle );

				if( node )
					node->SetWireColor( RGB( 255, 0, 0 ) );
			}

			sp0 = m;
			ob->suspendSnap = TRUE;

			p0 = vpt->SnapPoint( m, m, NULL, SNAP_IN_3D );
			p1 = p0 + Point3( .01f, .01f, .01f );

			mat.SetTrans( float( .5f ) * ( p0 + p1 ) );
			ob->pmapParam->Invalidate( );

			if( msg == MOUSE_POINT )
			{
				ob->suspendSnap = FALSE;
				return CREATE_STOP;
			}
			break;

		}
	}
	else
	{
		if( msg == MOUSE_ABORT )
			return CREATE_ABORT;
	}

	return TRUE;
}