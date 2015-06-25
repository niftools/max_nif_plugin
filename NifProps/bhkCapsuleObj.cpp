
#include "stdafx.h"

const float bhkScaleFactor = 7.0f;

Class_ID BHKCAPSULEOBJECT_CLASS_ID = Class_ID( 0x7f8f629a, BHKRIGIDBODYCLASS_DESC.PartB( ) );

class bhkCapsuleObject : public SimpleObject2
{
public:
	// Class vars
	static IParamMap2* pmapParam;
	static IObjParam*  ip;

	bhkCapsuleObject( BOOL loading );
	~bhkCapsuleObject( );

	// From object
	int						CanConvertToType( Class_ID obtype ) { return 0; }
	Object*					ConvertToType( TimeValue t, Class_ID obtype ) { return 0; }
	void					GetCollapseTypes( Tab<Class_ID>& clist, Tab<TSTR*>& nlist ) { Object::GetCollapseTypes( clist, nlist ); }

	CreateMouseCallBack*	GetCreateMouseCallBack( );

	void					BeginEditParams( IObjParam* ip, ULONG flags, Animatable* prev );
	void					EndEditParams( IObjParam* ip, ULONG flags, Animatable* next );

	RefTargetHandle			Clone( RemapDir& remap );

	const TCHAR*			GetObjectName( ) { return GetString( IDS_RB_CAPSULE_NIFPROPS ); }

	// Animatable methods
	void					DeleteThis( ) { delete this; }
	Class_ID				ClassID( ) { return BHKCAPSULEOBJECT_CLASS_ID; }
	SClass_ID				SuperClassID( ) { return HELPER_CLASS_ID; }

	// Return number of ParamBlocks in this instance
	int						NumParamBlocks( ) { return 1; }
	// Return the i'th ParamBlock
	IParamBlock2*			GetParamBlock( int i ) { return pblock2; }
	// Return id'd ParamBlock
	IParamBlock2*			GetParamBlockByID( BlockID id ) { return ( pblock2->ID( ) == id ) ? pblock2 : NULL; }

	// From SimpleObject
	void					BuildMesh( TimeValue t );
	BOOL					OKtoDisplay( TimeValue t );
	void					InvalidateUI( );

	int						Display( TimeValue t, INode* inode, ViewExp* vpt, int flags );

	void					UpdateUI( );
};

//--- ClassDescriptor and class vars ---------------------------------

// The class descriptor for sphere
class bhkCapsuleClassDesc : public ClassDesc2
{
public:
	bhkCapsuleClassDesc( );

	int				IsPublic( ) { return 1; }
	void*			Create( BOOL loading = FALSE ) {  return new bhkCapsuleObject( loading ); }
	const TCHAR*	ClassName( ) { return GetString( IDS_RB_CAPSULE_CLASS ); }
	SClass_ID		SuperClassID( ) { return HELPER_CLASS_ID; }
	Class_ID		ClassID( ) { return BHKCAPSULEOBJECT_CLASS_ID; }
	const TCHAR*	Category( ) { return _T( "NifTools" ); }

	// Returns fixed parsable name (scripter-visible name )
	const TCHAR*	InternalName( ) { return _T( "bhkCapsule" ); }
	// Returns owning module handle
	HINSTANCE		HInstance( ) { return hInstance; }
};

extern ClassDesc2* GetbhkCapsuleDesc( );

class CapsuleObjCreateCallBack : public CreateMouseCallBack
{
	IPoint2 sp[ 4 ];
	bhkCapsuleObject* ob;
	Point3 p[ 4 ];

public:
	
	int		proc( ViewExp* bpt, int msg, int point, int flags, IPoint2 m, Matrix3& mat );
	void	SetObj( bhkCapsuleObject* obj ) { ob = obj; }
};

static CapsuleObjCreateCallBack capsuleCreateCB;

IParamMap2* bhkCapsuleObject::pmapParam = NULL;
IObjParam* bhkCapsuleObject::ip = NULL;

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
       p_end,

    PB_RADIUS1, _T("radius1"), TYPE_FLOAT, P_ANIMATABLE,	IDS_RB_RADIUS1,
       p_default,	   0.0,
       p_range,		float(0), float(1.0E30),
       p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_RADIUS1, IDC_RADSPINNER1, SPIN_AUTOSCALE,
       p_end,

    PB_RADIUS2, _T("radius2"), TYPE_FLOAT, P_ANIMATABLE,	IDS_RB_RADIUS2,
       p_default,	   0.0,
       p_range,		float(0), float(1.0E30),
       p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_RADIUS2, IDC_RADSPINNER2, SPIN_AUTOSCALE,
       p_end,

    PB_LENGTH, _T("length"), TYPE_FLOAT, P_ANIMATABLE,	IDS_DS_LENGTH,
       p_default,	   0.0,
       p_range,		float(-1.0E30), float(1.0E30),
       p_ui, TYPE_SPINNER, EDITTYPE_UNIVERSE, IDC_LENGTHEDIT, IDC_LENSPINNER, SPIN_AUTOSCALE,
       p_end,

    p_end
);

// static ClassDesc must be declared after static paramblock
static bhkCapsuleClassDesc capsuleDesc;
extern ClassDesc2* GetbhkCapsuleDesc( ) { return &capsuleDesc; }

bhkCapsuleClassDesc::bhkCapsuleClassDesc( )
{
	param_blk.SetClassDesc( this );
}

class CapsuleParamDlgProc : public ParamMap2UserDlgProc
{
public:
	bhkCapsuleObject* so;
	HWND thishWnd;
	NpComboBox mCbMaterial;

	CapsuleParamDlgProc( bhkCapsuleObject* s )
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

// =============================================================
// CapsuleParamDlgProc
// =============================================================

void CapsuleParamDlgProc::Update( TimeValue t )
{
	if( !thishWnd )
		return;

	return;
}

INT_PTR CapsuleParamDlgProc::DlgProc( TimeValue t, IParamMap2* map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
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
// bhkCapsuleObject
// =============================================================

bhkCapsuleObject::bhkCapsuleObject( BOOL loading )
{
	SetAFlag( A_PLUGIN1 );
	capsuleDesc.MakeAutoParamBlocks( this );
	
	assert( pblock2 );
}

bhkCapsuleObject::~bhkCapsuleObject( )
{
	param_blk.SetUserDlgProc( );
	capsuleCreateCB.SetObj( NULL );

	if( pmapParam )
		pmapParam = NULL;
}

void bhkCapsuleObject::BeginEditParams( IObjParam* ip, ULONG flags, Animatable* prev )
{
	SimpleObject::BeginEditParams( ip, flags, prev );

	capsuleDesc.BeginEditParams( ip, this, flags, prev );
	param_blk.SetUserDlgProc( new CapsuleParamDlgProc( this ) );
	pmapParam = pblock2->GetMap( cap_params );

	this->ip = ip;
}

void bhkCapsuleObject::EndEditParams( IObjParam* ip, ULONG flags, Animatable* next )
{
	param_blk.SetUserDlgProc( );

	SimpleObject::EndEditParams( ip, flags, next );
	this->ip = NULL;
	pmapParam = NULL;

	// tear down the appropriate auto-rollouts
	capsuleDesc.EndEditParams( ip, this, flags, next );
}

void bhkCapsuleObject::BuildMesh( TimeValue t )
{
	int segs = 12;
	int hsegs = 1;
	int smooth = 1;

	// Start the validity interval at forever and widdle it down.
	ivalid = FOREVER;

	float radius1;
	float radius2;
	float length;

	pblock2->GetValue( PB_RADIUS1, t, radius1, ivalid );
	pblock2->GetValue( PB_RADIUS2, t, radius2, ivalid );
	pblock2->GetValue( PB_LENGTH, t, length, ivalid );

	if( radius2 == 0.0f )
		radius2 = radius1;

	if( radius1 == 0.0f )
	{
		mesh.setNumVerts( 0 );
		mesh.setNumFaces( 0 );
		mesh.setNumTVerts( 0 );
		mesh.setNumTVFaces( 0 );
		mesh.setSmoothFlags( smooth != 0 );
	}
	else
		BuildScubaMesh( mesh, segs, smooth, hsegs, radius2 * bhkScaleFactor, radius1 * bhkScaleFactor, length * bhkScaleFactor );
}

CreateMouseCallBack* bhkCapsuleObject::GetCreateMouseCallBack( )
{
	capsuleCreateCB.SetObj( this );
	return &capsuleCreateCB;
}

BOOL bhkCapsuleObject::OKtoDisplay( TimeValue t )
{
	float radius;

	pblock2->GetValue( PB_RADIUS1, t, radius, FOREVER );
	if( radius == 0.0f )
		return FALSE;

	return TRUE;
}

void bhkCapsuleObject::InvalidateUI( )
{
	if( pmapParam )
		pmapParam->Invalidate( );
}

RefTargetHandle bhkCapsuleObject::Clone( RemapDir& remap )
{
	bhkCapsuleObject* newob = new bhkCapsuleObject( FALSE );

	newob->ReplaceReference( 0, remap.CloneRef( pblock ) );
	newob->ivalid.SetEmpty( );

	BaseClone( this, newob, remap );

	return newob;
}

void bhkCapsuleObject::UpdateUI( )
{
	if( ip == NULL )
		return;

	CapsuleParamDlgProc* dlg = static_cast<CapsuleParamDlgProc*>( pmapParam->GetUserDlgProc( ) );
	dlg->Update( ip->GetTime( ) );
}

int bhkCapsuleObject::Display( TimeValue t, INode* inode, ViewExp* vpt, int flags )
{
	Matrix3 m;
	Color color = Color( inode->GetWireColor( ) );
	GraphicsWindow* gw = vpt->getGW( );
	Material* mtl = gw->getMaterial( );

	m = inode->GetObjectTM( t );
	gw->setTransform( m );

	DWORD rlim = gw->getRndLimits( );
	DWORD newrlim = GW_WIREFRAME;

#if VERSION_3DSMAX >= ((5000<<16)+(15<<8)+0) // Version 5+

	newrlim |= GW_EDGES_ONLY;

#endif

	gw->setRndLimits( newrlim );

	if( inode->Selected( ) )
		gw->setColor( LINE_COLOR, GetSelColor( ) );

	else
		gw->setColor( LINE_COLOR, color );

	UpdateMesh( t );
	mesh.render( gw, mtl, NULL, COMP_ALL );
	gw->setRndLimits( rlim );

	return 0;
}

// =============================================================
// CapsuleObjCreateCallBack
// =============================================================

int CapsuleObjCreateCallBack::proc( ViewExp* vpt, int msg, int point, int flags, IPoint2 m, Matrix3& mat )
{
	float r;
	Point3 p1;
	Point3 center;

	if( msg == MOUSE_FREEMOVE )
		vpt->SnapPreview( m, m, NULL, SNAP_IN_3D );

	if( msg == MOUSE_POINT || msg == MOUSE_MOVE )
	{
		switch( point )
		{
		case 0: // only happens with MOUSE_POINT msg
			// Find the node and plug in the wire color
			{
				ULONG handle;
				ob->NotifyDependents( FOREVER, (PartID) &handle, REFMSG_GET_NODE_HANDLE );
				INode* node = GetCOREInterface( )->GetINodeByHandle( handle );

				if( node )
					node->SetWireColor( RGB( 255, 0, 0 ) );
			}

			ob->pblock2->SetValue( PB_RADIUS1, 0, 0.0f );
			ob->pblock2->SetValue( PB_RADIUS2, 0, 0.0f );
			ob->suspendSnap = TRUE;

			sp[ 0 ] = m;
			p[ 0 ] = vpt->SnapPoint( m, m, NULL, SNAP_IN_3D );
			
			ob->pblock2->SetValue( PB_LENGTH, 0, 0.0f );
			mat.SetTrans( p[ 0 ] );
			break;

		case 1: // Fix radius of first cap
			mat.IdentityMatrix( );

			sp[ 1 ] = m;
			p[ 1 ] = vpt->SnapPoint( m, m, NULL, SNAP_IN_3D );

			r = Length( p[ 1 ] - p[ 0 ] );
			mat.SetTrans( p[ 0 ] );

			ob->pblock2->SetValue( PB_RADIUS1, 0, r );
			ob->pblock2->SetValue( PB_RADIUS2, 0, r );
			ob->pmapParam->Invalidate( );

			if( flags & MOUSE_CTRL )
			{
				float ang = (float) atan2( p[ 1 ].y - p[ 0 ].y, p[ 1 ].x - p[ 0 ].x );
				mat.PreRotateZ( ob->ip->SnapAngle( ang ) );
			}

			if( msg == MOUSE_POINT )
			{
				if( Length( m - sp[ 0 ] ) < 3 || Length( p[ 1 ] - p[ 0 ] ) < 0.1f )
				{
					ob->suspendSnap = FALSE;
					return CREATE_ABORT;
				}
			}
			break;

		case 2: // Get second point
			mat.IdentityMatrix( );

			sp[ 2 ] = m;
			p[ 2 ] = vpt->SnapPoint( m, m, NULL, SNAP_IN_3D );

			r = Length( p[ 2 ] - p[ 1 ] );
			mat.SetTrans( p[ 0 ] );

			ob->pblock2->SetValue( PB_LENGTH, 0, r );
			ob->pmapParam->Invalidate( );

			// Stop unless Ctrl is selected then we size the final radius
			if( msg == MOUSE_POINT && ( !flags & MOUSE_CTRL ) )
			{
				ob->suspendSnap = FALSE;
				return CREATE_STOP;
			}
			break;

		case 3: // Size the second cap
			mat.IdentityMatrix( );

			sp[ 3 ] = m;
			p[ 3 ] = vpt->SnapPoint( m, m, NULL, SNAP_IN_3D );
			mat.SetTrans( p[ 0 ] );

			if( flags & MOUSE_CTRL )
			{
				r = Length( p[ 1 ] - p[ 0 ] );
				ob->pblock2->SetValue( PB_RADIUS2, 0, r );
			}
			else
			{
				// start radius at r1
				r = Length( ( p[ 3 ] - p[ 2 ] ) + ( p[ 1 ] - p[ 0 ] ) );
				ob->pblock2->SetValue( PB_RADIUS2, 0, r );
			}

			ob->pmapParam->Invalidate( );
			if( msg == MOUSE_POINT )
			{
				ob->suspendSnap = FALSE;
				return CREATE_STOP;
			}
			break;
		}
	}
	else if( msg == MOUSE_ABORT )
	{
		ob->suspendSnap = FALSE;
		return CREATE_ABORT;
	}

	return TRUE;
}