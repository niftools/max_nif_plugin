
#include "stdafx.h"

#define FurnitureMarker_CLASS_ID Class_ID( 0x5a38017d, 0x4006a661 )
#define PBLOCK_REF 0

class NifFurnitureMarker : public HelperObject
{
public:

	NifFurnitureMarker( );
	NifFurnitureMarker( const NifFurnitureMarker& fm );
	~NifFurnitureMarker( );

	// Loading/Saving
	IOResult				Load( ILoad* iload );
	IOResult				Save( ISave* isave ) { return IO_OK; }

	RefTargetHandle			Clone( RemapDir& remap );
	RefResult				NotifyRefChanged( Interval changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message ) { return REF_SUCCEED; }

	// BaseObject
	void					GetMat( TimeValue t, INode* inode, ViewExp* vpt, Matrix3& tm );
	int						HitTest( TimeValue t, INode* inode, int type, int crossing, int flag, IPoint2* p, ViewExp* vpt );
	int						Display( TimeValue t, INode* inode, ViewExp* vpt, int flags );
	
	CreateMouseCallBack*	GetCreateMouseCallBack( );
	
	void					BeginEditParams( IObjParam* ip, ULONG flags, Animatable* prev );
	void					EndEditParams( IObjParam* ip, ULONG flags, Animatable* next );

	const TCHAR*			GetObjectName( ) { return GetString( IDS_FURNITURE_MARKER_NF ); }

	// Animatable
	int						IsKeyable( ) { return 0; }
	int						NumSubs( ) { return 1; }
	void					DeleteThis( ) { delete this; }
	Class_ID				ClassID( ) { return FurnitureMarker_CLASS_ID; }
	SClass_ID				SuperClassID( ) { return HELPER_CLASS_ID; }
	void					GetClassName( TSTR& s ) { s = GetString( IDS_CLASS_NAME_NF ); }

	// Object
	Interval				ObjectValidity( TimeValue t );
	int						DoOwnSelectHilite( ) { return 1; }
	int						IsMappable( ) { return 0; }
	int						IsRenderable( ) { return 0; }

	void					GetWorldBoundBox( TimeValue t, INode* mat, ViewExp* vpt, Box3& box );
	void					GetLocalBoundBox( TimeValue t, INode* mat, ViewExp* vpt, Box3& box );

	int						UsesWireColor( ) { return TRUE; }

	// HelperObject
	void					InitNodeName( TSTR& s ) { s = GetString( IDS_FURNITURE_MARKER_NF ); }

	// Object
	ObjectState				Eval( TimeValue t );

	int						CanConvertToType( Class_ID obtype ) { return FALSE; }
	Object*					ConvertToType( TimeValue t, Class_ID obtype ) { return NULL; }

	void					setFrnPos( NpFrnPos type );
	NpFrnPos				getFrnPos( );

private:

	class CreateCallback : public CreateMouseCallBack
	{
	public:
		virtual int			proc( ViewExp* vpt, int msg, int point, int flags, IPoint2 m, Matrix3& mat );
	};

	static BOOL CALLBACK	dlgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

	INode*					getNode( );

	Interface*				mIP;
	HWND					mPanel;

	Mesh					mMesh;
	Point3					mFlip;

	NpComboBox				mCbType;

	static NifFurnitureMarker* mInstance;
	static CreateCallback	mCreateCallback;

	// maps combo box indices to marker types
	static NpFrnPos			mPosMap[];
};

NifFurnitureMarker::CreateCallback NifFurnitureMarker::mCreateCallback;
NifFurnitureMarker *NifFurnitureMarker::mInstance = NULL;

NpFrnPos NifFurnitureMarker::mPosMap[] = 
{ 
	NP_FRN_SLEEP_LEFT,
	NP_FRN_SLEEP_RIGHT,
	NP_FRN_BEDROLL_LEFT,
	NP_FRN_BEDROLL_RIGHT,
	NP_FRN_SIT_LEFT,
	NP_FRN_SIT_RIGHT,
	NP_FRN_SIT_BACK,
	NP_FRN_SIT_FRONT
};

//--- ClassDescriptor and class vars ---------------------------------

// The class descriptor for furniture
class NifFurnitureClassDesc : public ClassDesc2
{
public:

	int				IsPublic( ) { return 1; }
	void*			Create( BOOL loading = FALSE ) {  return new NifFurnitureMarker( ); }
	const TCHAR*	ClassName( ) { return GetString( IDS_CLASS_NAME_NF ); }
	SClass_ID		SuperClassID( ) { return HELPER_CLASS_ID; }
	Class_ID		ClassID( ) { return FurnitureMarker_CLASS_ID; }
	const TCHAR*	Category( ) { return _T( "NifTools" ); }

	// Returns fixed parsable name (scripter-visible name )
	const TCHAR*	InternalName( ) { return _T( "NifFurnitureMarker" ); }
	// Returns owning module handle
	HINSTANCE		HInstance( ) { return hInstance; }
};

static NifFurnitureClassDesc NifFurnitureDesc;
ClassDesc2* GetNifFurnitureDesc( ) { return &NifFurnitureDesc; }

// =============================================================
// NifFurnitureMarker::CreateCallback
// =============================================================

int NifFurnitureMarker::CreateCallback::proc( ViewExp* vpt, int msg, int point, int flags, IPoint2 m, Matrix3& mat )
{
	if( msg == MOUSE_POINT || msg == MOUSE_MOVE )
	{
		Point3 cen;
		cen = vpt->SnapPoint( m, m, NULL, SNAP_IN_3D );

		if( msg == MOUSE_POINT )
			return CREATE_STOP;
	}

	return CREATE_ABORT;
}

// =============================================================
// NifFurnitureMarker
// =============================================================

NifFurnitureMarker::NifFurnitureMarker( )
{
	mIP = NULL;
	mInstance = NULL;

	setFrnPos( NP_FRN_SLEEP_LEFT );
}

NifFurnitureMarker::NifFurnitureMarker( const NifFurnitureMarker& fm )
{
	mIP = NULL;
	mInstance = NULL;
}

NifFurnitureMarker::~NifFurnitureMarker( )
{
	mInstance = NULL;
}

void NifFurnitureMarker::setFrnPos( NpFrnPos type )
{
	const FurnitureMarker* mark;

	mFlip.Set( 1, 1, 1 );

	switch( type )
	{
	case 1:
		mark = &FurnitureMarker01;
		break;

	case 2:
		mFlip[ 0 ] = -1;
		mark = &FurnitureMarker01;
		break;

	case 3:
		mark = &FurnitureMarker03;
		break;

	case 4:
		mark = &FurnitureMarker04;
		break;

	case 11:
		mark = &FurnitureMarker11;
		break;

	case 12:
		mFlip[ 0 ] = -1;
		mark = &FurnitureMarker11;
		break;

	case 13:
		mark = &FurnitureMarker13;
		break;

	case 14:
		mark = &FurnitureMarker14;
		break;
	}

	mMesh.setNumVerts( mark->nv );
	mMesh.setNumFaces( mark->nf );

	int i;
	for( i = 0; i < mark->nv; i++ )
		mMesh.verts[ i ].Set( mark->verts[ i * 3 + 0 ], mark->verts[ i * 3 + 2 ], mark->verts[ i * 3 + 1 ] );

	for( i = 0; i < mark->nf; i++ )
	{
		mMesh.faces[ i ].setVerts( mark->faces[ i * 3 + 0 ], mark->faces[ i * 3 + 1 ], mark->faces[ i * 3 + 2 ] );
		mMesh.faces[ i ].setEdgeVisFlags( 1, 1, 1 );
	}

	npSetProp( getNode( ), NP_FRN_POS, type );
}

NpFrnPos NifFurnitureMarker::getFrnPos( )
{
	NpFrnPos type;
	npGetProp( getNode( ), NP_FRN_POS, (int&) type, NP_DEFAULT_FRN_MARKER_TYPE );

	return type;
}

INode* NifFurnitureMarker::getNode( )
{
	ULONG handle;
	NotifyDependents( FOREVER, (PartID) &handle, REFMSG_GET_NODE_HANDLE );

	return GetCOREInterface( )->GetINodeByHandle( handle );
}

RefTargetHandle NifFurnitureMarker::Clone( RemapDir& remap )
{
	NifFurnitureMarker* fm = new NifFurnitureMarker( *this );
	BaseClone( this, fm, remap );

	fm->setFrnPos( getFrnPos( ) );

	return fm;
}

ObjectState NifFurnitureMarker::Eval( TimeValue t )
{
	ObjectState os( this );
	return os;
}

CreateMouseCallBack* NifFurnitureMarker::GetCreateMouseCallBack( )
{
	return &mCreateCallback;
}

int NifFurnitureMarker::Display( TimeValue t, INode* inode, ViewExp* vpt, int flags )
{
	setFrnPos( getFrnPos( ) );

	Matrix3 m;
	GraphicsWindow* gw = vpt->getGW( );
	Material* mtl = gw->getMaterial( );

	DWORD rlim = gw->getRndLimits( );
	DWORD newrlim = GW_WIREFRAME | GW_BACKCULL;

#if VERSION_3DSMAX >= ((5000<<16)+(15<<8)+0) // Version 5+

	newrlim |= GW_EDGES_ONLY;

#endif

	gw->setRndLimits( newrlim );

	GetMat( t, inode, vpt, m );
	m.Scale( mFlip );

	gw->setTransform( m );
	if( inode->Selected( ) )
		gw->setColor( LINE_COLOR, 1.0f, 1.0f, 1.0f );

	else if( !inode->IsFrozen( ) )
		gw->setColor( LINE_COLOR, 0.4f, 0.0f, 0.6f );

	mMesh.render( gw, mtl, NULL, COMP_ALL );

	gw->setRndLimits( rlim );

	return 0;
}

Interval NifFurnitureMarker::ObjectValidity( TimeValue t )
{
	Interval ivalid;
	ivalid.SetInfinite( );

	return ivalid;
}

int NifFurnitureMarker::HitTest( TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2* p, ViewExp* vpt )
{
	Matrix3 m;
	int result = FALSE;
	HitRegion hitRegion;
	DWORD savedLimits;

	MakeHitRegion( hitRegion, type, crossing, 4, p );

	GraphicsWindow* gw = vpt->getGW( );
	savedLimits = gw->getRndLimits( );
	gw->setRndLimits( ( savedLimits | GW_PICK ) & ~GW_ILLUM );

	GetMat( t, inode, vpt, m );

	gw->setTransform( m );
	gw->clearHitCode( );

	Material* mtl = gw->getMaterial( );
	if( mMesh.select( gw, mtl, &hitRegion, flags & HIT_ABORTONHIT ) )
		result = TRUE;

	gw->setRndLimits( savedLimits );

	return result;
}

void NifFurnitureMarker::BeginEditParams( IObjParam* ip, ULONG flags, Animatable* prev )
{
	mIP = ip;
	mInstance = this;

	mPanel = ip->AddRollupPage( hInstance, MAKEINTRESOURCE( IDD_PANEL_NF ), (DLGPROC) dlgProc, GetString( IDS_PARAMS_NF ), 0 );
	
	NpFrnPos pos = getFrnPos( );
	setFrnPos( pos );

	for( int i = 0; i < mCbType.count( ); i++ )
	{
		if( mPosMap[ i ] == pos )
		{
			mCbType.select( i );
			break;
		}
	}
}

void NifFurnitureMarker::EndEditParams( IObjParam* ip, ULONG flags, Animatable* prev )
{
	ip->DeleteRollupPage( mPanel );
	mInstance = NULL;
}

void NifFurnitureMarker::GetMat( TimeValue t, INode* inode, ViewExp* vpt, Matrix3& tm )
{
	tm = inode->GetObjectTM( t );
}

void NifFurnitureMarker::GetLocalBoundBox( TimeValue t, INode* inode, ViewExp* vpt, Box3& box )
{
	box = mMesh.getBoundingBox( );
}

void NifFurnitureMarker::GetWorldBoundBox( TimeValue t, INode* inode, ViewExp* vpt, Box3& box )
{
	GetLocalBoundBox( t, inode, vpt, box );

	Matrix3 tm;
	GetMat( t, inode, vpt, tm );

	int nv = mMesh.getNumVerts( );
	box.Init( );

	for( int i = 0; i < nv; i++ )
		box += tm * mMesh.getVert( i );
}

BOOL NifFurnitureMarker::dlgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg )
	{
	case WM_INITDIALOG:
		mInstance->mCbType.init( GetDlgItem( hWnd, IDC_CB_TYPE_NF ) );
		mInstance->mCbType.add( _T( "Sleep Left" ) );			// 1
		mInstance->mCbType.add( _T( "Sleep Right" ) );			// 2
		mInstance->mCbType.add( _T( "Bedroll Left" ) );			// 3
		mInstance->mCbType.add( _T( "Bedroll Behind" ) );		// 4
		mInstance->mCbType.add( _T( "Sit Left" ) );				// 11
		mInstance->mCbType.add( _T( "Sit Right" ) );			// 12
		mInstance->mCbType.add( _T( "Sit Back" ) );				// 13
		mInstance->mCbType.add( _T( "Sit Front" ) );			// 14
		break;

	case WM_COMMAND:
		switch( LOWORD( wParam ) )
		{
		case IDC_CB_TYPE_NF:
			if( HIWORD( wParam ) == CBN_SELCHANGE )
			{
				mInstance->setFrnPos( (NpFrnPos) mPosMap[ mInstance->mCbType.selection( ) ] );
				mInstance->mIP->ForceCompleteRedraw( );
			}
			break;
		}
		break;
	}

	return FALSE;
}

IOResult NifFurnitureMarker::Load( ILoad* iload )
{
	setFrnPos( getFrnPos( ) );
	return IO_OK;
}