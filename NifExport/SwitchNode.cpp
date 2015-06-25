
#include "stdafx.h"

Class_ID SwitchNode_CLASS_ID = Class_ID( 0x3f071130, 0x5c1973a0 );

// Parameter and ParamBlock IDs
enum { switchnode_params, };  // pblock2 ID
enum 
{ 
	PB_NUMBRANCHES,
	PB_ACTIVEBRANCH,
	PB_ACTIVECHILDREN,
	PB_UPDATEACTIVE,
	PB_VIEWALL,
	PB_BRANCHIDX,
	PB_CHILDIDX,
	PB_NUMCHILDREN,
	PB_SETCHILD,
	PB_GETCHILD,
};

class SwitchNodeObject : public HelperObject
{
public:
	Interface*			mIP;
	SwitchNodeObject*	editOb;
	IParamBlock2*		pblock2;
	Mesh				mesh;

	ICustButton*		addChildButton;

	SwitchNodeObject( );
	~SwitchNodeObject( );

	// Ref
	IOResult				Load( ILoad* iload );
	IOResult				Save( ISave* isave );

	RefTargetHandle			Clone( RemapDir& remap );
	RefResult				NotifyRefChanged( Interval changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message );

	int						NumRefs( ) { return 1; }
	RefTargetHandle			GetReference( int i ) { return pblock2; }

	// BaseObject
	int						HitTest( TimeValue t, INode* inode, int type, int crossing, int flag, IPoint2* p, ViewExp* vpt );
	int						Display( TimeValue t, INode* inode, ViewExp* vpt, int flags );
	
	CreateMouseCallBack*	GetCreateMouseCallBack( );
	
	void					BeginEditParams( IObjParam* ip, ULONG flags, Animatable* prev );
	void					EndEditParams( IObjParam* ip, ULONG flags, Animatable* next );

	const TCHAR*			GetObjectName( ) { return GetString( IDS_NAME_SN ); }

	// Animatable
	void					DeleteThis( ) { delete this; }
	SClass_ID				SuperClassID( ) { return HELPER_CLASS_ID; }
	Class_ID				ClassID( ) { return SwitchNode_CLASS_ID; }
	void					GetClassName( TSTR& s ) { s = GetString( IDS_CLASS_NAME_SN ); }
	int						IsKeyable( ) { return 0; }
	int						NumSubs( ) { return 1; }
	Animatable*				SubAnim( int i ) { return pblock2; }
	TSTR					SubAnimName( int i ) { return _T( "Parameters" ); }
	IParamArray*			GetParamBlock( ) { return (IParamArray*) pblock2; }
	int						GetParamBlockIndex( int id );
	int						NumParamBlocks( ) { return 1; }
	IParamBlock2*			GetParamBlock( int i ) { return pblock2; }
	IParamBlock2*			GetParamBlockByID( short id ) { return pblock2; }

	// Object
	Interval				ObjectValidity( TimeValue t );
	int						DoOwnSelectHilite( ) { return 1; }
	int						IsMappable( ) { return 0; }
	int						IsRenderable( ) { return 0; }

	void					GetWorldBoundBox( TimeValue t, INode* mat, ViewExp* vpt, Box3& box );
	void					GetLocalBoundBox( TimeValue t, INode* mat, ViewExp* vpt, Box3& box );

	int						UsesWireColor( ) { return TRUE; }

	// HelperObject
	void					InitNodeName( TSTR& s ) { s = GetString( IDS_NAME_SN ); }

	// Object
	ObjectState				Eval( TimeValue t );

	int						CanConvertToType( Class_ID obtype ) { return FALSE; }
	Object*					ConvertToType( TimeValue t, Class_ID obtype ) { return NULL; }

	// Local methods
	void					InvalidateUI( );
	void					BuildMesh( );

	void					AddNodeToList( INode* node );
	void					RemoveNodeFromList( int index );
	void					UpdateBranches( int oldBranch = -1 );

	void					AddBranch( );
	void					RemoveBranch( int index );

	void					SetAllNodesVisibility( BOOL visible );

	INodeTab*				GetNodeChildren( int index );

private:
	virtual void			SetReference( int i, RefTargetHandle rtarg ) { pblock2 = (IParamBlock2*) rtarg; }

	INode*					GetNodeRef( ReferenceMaker* rm );
	INode*					FindNodeRef( ReferenceTarget* rt );

private:
	int			branchChildCount;

	Tab<INodeTab> branchMap;
};

//--- ClassDescriptor and class vars ---------------------------------

// The class descriptor for list
class SwitchNodeClassDesc : public ClassDesc2
{
public:
	int				IsPublic( ) { return 1; }
	void*			Create( BOOL loading = FALSE ) {  return new SwitchNodeObject( ); }
	const TCHAR*	ClassName( ) { return GetString( IDS_CLASS_NAME_SN ); }
	SClass_ID		SuperClassID( ) { return HELPER_CLASS_ID; }
	Class_ID		ClassID( ) { return SwitchNode_CLASS_ID; }
	const TCHAR*	Category( ) { return _T( "NifTools" ); }

	// Returns fixed parsable name (scripter-visible name )
	const TCHAR*	InternalName( ) { return _T( "SwitchNode" ); }
	// Returns owning module handle
	HINSTANCE		HInstance( ) { return hInstance; }
};

static SwitchNodeClassDesc switchNodeDesc;
extern ClassDesc2* GetSwitchNodeClassDesc( );

class SwitchNodeParamDlgProc : public ParamMap2UserDlgProc
{
public:
	SwitchNodeObject* so;
	HWND thishWnd;

	NpComboBox mCbBranches;
	bool viewAllChecked;

	SwitchNodeParamDlgProc( SwitchNodeObject* s )
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

class SwitchNodeCreateCallback : public CreateMouseCallBack
{
	SwitchNodeObject* ob;

public:
	int			proc( ViewExp* vpt, int msg, int point, int flags, IPoint2 m, Matrix3& mat );
	void		SetObj( SwitchNodeObject* obj ) { ob = obj; }
};

static SwitchNodeCreateCallback switchNodeCreateCB;

class SwitchNodePickObjectMode : public PickModeCallback, public PickNodeCallback
{
public:
	SwitchNodeObject* mod;

	BOOL			  HitTest( IObjParam* ip, HWND hWnd, ViewExp* vpt, IPoint2 m, int flags );
	BOOL			  Pick( IObjParam* ip, ViewExp* vpt );
	void			  EnterMode( IObjParam* ip ) { mod->addChildButton->SetCheck( TRUE ); }
	void			  ExitMode( IObjParam* ip ) { mod->addChildButton->SetCheck( FALSE ); }
	BOOL			  RightClick( IObjParam* ip, ViewExp* vpt ) { return TRUE; }
	BOOL			  Filter( INode* node );
	PickNodeCallback* GetFilter( ) { return this; }
};

static SwitchNodePickObjectMode thePickMode;

//--- Parameter map/block descriptors -------------------------------
static ParamBlockDesc2 param_blk 
( 
    switchnode_params, _T( "parameters" ),  0, &switchNodeDesc, P_AUTO_CONSTRUCT + P_AUTO_UI, 0,

	//rollout
    IDD_SWITCHNODE, IDS_PARAMS_SN, 0, 0, NULL, 

    // params
	PB_NUMBRANCHES,  _T( "numBranches" ), TYPE_INT, 0, IDS_NUMBRANCHES_SN,
		p_default, 2,
		p_end,

    PB_ACTIVEBRANCH, _T( "activeBranch" ), TYPE_INT, 0, IDS_SELBRANCH_SN,
		p_default,   0,
		p_end,

	PB_ACTIVECHILDREN, _T( "activeChildren" ), TYPE_INODE_TAB, 0, P_AUTO_UI | P_VARIABLE_SIZE, IDS_BRCHILDREN_SN,
		p_ui,		TYPE_NODELISTBOX, IDC_SN_CHILDREN, 0, 0, 0,
		p_end,

	PB_UPDATEACTIVE, _T( "updateActive" ), TYPE_BOOL, 0, IDS_UPDATEACTIVE_SN,
		p_default,	FALSE,
		p_ui,		TYPE_SINGLECHEKBOX, IDC_SN_UPDATEACTIVE,
		p_end,

	PB_VIEWALL, _T( "viewAll" ), TYPE_BOOL, 0, IDS_VIEWALL_SN,
		p_default,	FALSE,
		p_end,

	/* The below two are used to access and set the underlying data */

	PB_BRANCHIDX, _T( "branchIdx" ), TYPE_INT, 0, IDS_BRANCHIDX_SN,
		p_default,  0,
		p_end,

	PB_CHILDIDX, _T( "childIdx" ), TYPE_INT, 0, IDS_CHILDIDX_SN,
		p_default,	0,
		p_end,

	PB_NUMCHILDREN, _T( "numChildren" ), TYPE_INT, 0, IDS_NUMCHILDREN_SN,
		p_end,

	PB_SETCHILD, _T( "setChild" ), TYPE_INODE, 0, IDS_SETCHILD_SN,
		p_end,

	PB_GETCHILD, _T( "getChild" ), TYPE_INODE, 0, IDS_GETCHILD_SN,
		p_end,

    p_end
);

// bug in pb desc? forces us to use this rather than in inline version
extern ClassDesc2* GetSwitchNodeClassDesc( ) { return &switchNodeDesc; }

// =============================================================
// SwitchNodeParamDlgProc
// =============================================================

void SwitchNodeParamDlgProc::Update( TimeValue t )
{
	if( !thishWnd )
		return;

	return;
}

INT_PTR SwitchNodeParamDlgProc::DlgProc( TimeValue t, IParamMap2* map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	int index = -1;
	thishWnd = hWnd;

	switch( msg )
	{
	case WM_INITDIALOG:
		{
			int count = 0;
			int sel = 0;
			BOOL viewAll = false;
			Interval valid;
			ICustButton* b;

			mCbBranches.init( GetDlgItem( hWnd, IDC_SN_SELBRANCH ) );
			so->pblock2->GetValue( PB_NUMBRANCHES, 0, count, FOREVER );

			for( int i = 0; i < count; i++ )
			{
				std::wstringstream ss;
				ss << ( i + 1 );

				mCbBranches.add( ss.str( ).c_str( ) );
			}

			if( count == 2 )
				EnableWindow( GetDlgItem( hWnd, IDC_SN_DELBRANCH ), FALSE );

			so->pblock2->GetValue( PB_ACTIVEBRANCH, 0, sel, valid );
			mCbBranches.select( sel );

			b = GetICustButton( GetDlgItem( hWnd, IDC_SN_VIEWALL ) );
			b->SetType( CBT_CHECK );

			so->pblock2->GetValue( PB_VIEWALL, 0, viewAll, FOREVER );
			if( viewAll )
				b->SetCheck( TRUE );

			ReleaseICustButton( b );

			so->addChildButton = GetICustButton( GetDlgItem( hWnd, IDC_SN_ADDCHILD ) );
			so->addChildButton->SetType( CBT_CHECK );

			so->UpdateBranches( );
			Update( t );
		}
		break;

	case WM_COMMAND:
		switch( LOWORD( wParam ) )
		{
		case IDC_SN_VIEWALL:
			{
				BOOL viewAll;
				so->pblock2->GetValue( PB_VIEWALL, 0, viewAll, FOREVER );

				viewAll = !viewAll;
				if( viewAll )
					so->SetAllNodesVisibility( FALSE );

				else
				{
					so->SetAllNodesVisibility( TRUE );
					so->UpdateBranches( );
				}

				so->pblock2->SetValue( PB_VIEWALL, 0, viewAll );
			}
			break;

		case IDC_SN_ADDCHILD:
			thePickMode.mod = so;
			so->mIP->SetPickMode( &thePickMode );
			break;

		case IDC_SN_DELCHILD:
			index = (int) SendMessage( GetDlgItem( hWnd, IDC_SN_CHILDREN ), LB_GETCURSEL, 0, 0 );
			so->RemoveNodeFromList( index );
			break;

		case IDC_SN_SELBRANCH:
			if( HIWORD( wParam ) == CBN_SELCHANGE )
			{
				BOOL viewAll;
				so->pblock2->GetValue( PB_VIEWALL, 0, viewAll, FOREVER );

				if( viewAll )
				{
					viewAll = FALSE;
					so->pblock2->SetValue( PB_VIEWALL, 0, viewAll );

					ICustButton* b = GetICustButton( GetDlgItem( hWnd, IDC_SN_VIEWALL ) );
					b->SetCheck( FALSE );

					ReleaseICustButton( b );

					so->SetAllNodesVisibility( TRUE );
				}

				so->pblock2->GetValue( PB_ACTIVEBRANCH, 0, index, FOREVER );
				so->pblock2->SetValue( PB_ACTIVEBRANCH, 0, mCbBranches.selection( ) );
				//so->UpdateBranches( index );
			}
			break;

		case IDC_SN_ADDBRANCH:
			{
				int count = mCbBranches.count( ) + 1;
				int selIndex = mCbBranches.selection( );

				so->AddBranch( );

				std::wstringstream branchIdxStream;
				branchIdxStream << count;
				
				mCbBranches.add( branchIdxStream.str( ).c_str( ) );
				mCbBranches.select( count - 1 );

				if( count > 2 )
					EnableWindow( GetDlgItem( hWnd, IDC_SN_DELBRANCH ), TRUE );

				so->UpdateBranches( selIndex );
			}
			break;

		case IDC_SN_DELBRANCH:
			{
				int selIndex= mCbBranches.selection( );
				int count = mCbBranches.count( );

				for( int i = 0; i < count; i++ )
					mCbBranches.remove( i );

				for( int i = 0; i < ( count - 1 ); i++ )
				{
					std::wstringstream ss;
					ss << ( i + 1 );

					mCbBranches.add( ss.str( ).c_str( ) );
				}

				mCbBranches.select( ( selIndex != 0 ) ? selIndex - 1 : 0 );

				if( ( count - 1  ) == 2 )
					EnableWindow( GetDlgItem( hWnd, IDC_SN_DELBRANCH ), FALSE );

				so->RemoveBranch( selIndex );
				so->UpdateBranches( );
			}
			break;

		}
		break;
	}

	return FALSE;
}

// =============================================================
// SwitchNodeCreateCallback
// =============================================================

int SwitchNodeCreateCallback::proc( ViewExp* vpt, int msg, int point, int flags, IPoint2 m, Matrix3& mat )
{
	if( !vpt || !vpt->IsAlive( ) )
		return FALSE;

	if( msg == MOUSE_FREEMOVE )
		vpt->SnapPreview( m, m, NULL, SNAP_IN_3D );

	if( msg == MOUSE_POINT || msg == MOUSE_MOVE )
	{
		switch( point )
		{
		case 0:
			{
				ULONG handle;
				ob->NotifyDependents( FOREVER, (PartID) &handle, REFMSG_GET_NODE_HANDLE );

				INode* node;
				node = GetCOREInterface( )->GetINodeByHandle( handle );

				if( node )
				{
					Point3 color = GetUIColor( COLOR_POINT_OBJ );
					node->SetWireColor( RGB( color.x * 255.0f, color.y * 255.0f, color.z * 255.0f ) );
				}

				mat.SetTrans( vpt->SnapPoint( m, m, NULL, SNAP_IN_3D ) );
			}
			break;

		case 1:
			mat.SetTrans( vpt->SnapPoint( m, m, NULL, SNAP_IN_3D ) );
			if( msg == MOUSE_POINT )
				return 0;

			break;
		}
	}
	else if( msg == MOUSE_ABORT )
		return CREATE_ABORT;

	return 1;
}

// =============================================================
// SwitchNodePickObjectMode
// =============================================================

BOOL SwitchNodePickObjectMode::Filter( INode* node )
{
	if( node )
	{
		node->BeginDependencyTest( );
		mod->NotifyDependents( FOREVER, 0, REFMSG_TEST_DEPENDENCY );

		if( node->EndDependencyTest( ) )
			return FALSE;

		for( int i = 0; i < mod->pblock2->Count( PB_ACTIVECHILDREN ); i++ )
		{
			INode* tnode = NULL;
			mod->pblock2->GetValue( PB_ACTIVECHILDREN, 0, tnode, FOREVER );
			
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

BOOL SwitchNodePickObjectMode::HitTest( IObjParam* ip, HWND hWnd, ViewExp* vpt, IPoint2 m, int flags )
{
	INode* node = mod->mIP->PickNode( hWnd, m, this );
	return node ? TRUE : FALSE;
}

BOOL SwitchNodePickObjectMode::Pick( IObjParam* ip, ViewExp* vpt )
{
	BOOL rv = FALSE;
	if( INode* node = vpt->GetClosestHit( ) )
	{
		theHold.Begin( );

		ObjectState os = node->EvalWorldState( 0 );
		if( os.obj->CanConvertToType( triObjectClassID ) )
		{
			mod->AddNodeToList( node );
			rv = TRUE;
		}

		theHold.Accept( GetString( IDS_ADD_MESH ) );
	}

	return rv;
}

// =============================================================
// SwitchNodeObject
// =============================================================

SwitchNodeObject::SwitchNodeObject( )
{
	mIP = NULL;
	pblock2 = NULL;

	branchChildCount = 0;
	branchMap.Append( 1, new INodeTab( ) );
	branchMap.Append( 1, new INodeTab( ) );

	switchNodeDesc.MakeAutoParamBlocks( this );
	SetAFlag( A_OBJ_CREATING );

	BuildMesh( );
}

SwitchNodeObject::~SwitchNodeObject( )
{
	DeleteAllRefsFromMe( );
}

void SwitchNodeObject::BeginEditParams( IObjParam* ip, ULONG flags, Animatable* prev )
{
	mIP = ip;
	editOb = this;

	param_blk.dlgProc = new SwitchNodeParamDlgProc( this );
	switchNodeDesc.BeginEditParams( ip, this, flags, prev );
}

void SwitchNodeObject::EndEditParams( IObjParam* ip, ULONG flags, Animatable* next )
{
	editOb = NULL;
	mIP = NULL;

	switchNodeDesc.EndEditParams( ip, this, flags, next );
	ClearAFlag( A_OBJ_CREATING );
}

int SwitchNodeObject::GetParamBlockIndex( int id )
{
	if( pblock2 && id >= 0 && id < pblock2->NumParams( ) )
		return id;

	else
		return -1;
}

void SwitchNodeObject::GetLocalBoundBox( TimeValue t, INode* inode, ViewExp* vpt, Box3& box )
{
	box = mesh.getBoundingBox( );
}

void SwitchNodeObject::GetWorldBoundBox( TimeValue t, INode* inode, ViewExp* vpt, Box3& box )
{
	GetLocalBoundBox( t, inode, vpt, box );

	Matrix3 tm = inode->GetObjectTM( t );
	int nv = mesh.getNumVerts( );

	box.Init( );

	for( int i = 0; i < nv; i++ )
		box += tm * mesh.getVert( i );
}

void SwitchNodeObject::BuildMesh( )
{
	mesh.setNumVerts( 34 );
	mesh.setNumFaces( 28 );

	for( int i = 0; i < 34; i++ )
		mesh.verts[ i ].Set( SwitchNodeVerts[ i * 3 + 0 ], SwitchNodeVerts[ i * 3 + 1 ], SwitchNodeVerts[ i * 3 + 2 ] );

	for( int i = 0; i < 28; i++ )
	{
		mesh.faces[ i ].setVerts( SwitchNodeFaces[ i * 3 + 0 ], SwitchNodeFaces[ i * 3 + 1 ], SwitchNodeFaces[ i * 3 + 2 ] );
		mesh.faces[ i ].setEdgeVisFlags( SwitchNodeEdgeVis[ i * 3 + 0 ], SwitchNodeEdgeVis[ i * 3 + 1 ], SwitchNodeEdgeVis[ i * 3 + 2 ] );
	}

	mesh.InvalidateGeomCache( );
}

int SwitchNodeObject::Display( TimeValue t, INode* inode, ViewExp* vpt, int flags )
{
	Matrix3 m;
	GraphicsWindow* gw = vpt->getGW( );
	Material* mtl = gw->getMaterial( );

	DWORD rlim = gw->getRndLimits( );
	gw->setRndLimits( GW_WIREFRAME | GW_BACKCULL | GW_EDGES_ONLY | GW_Z_BUFFER );

	m = inode->GetObjectTM( t );
	gw->setTransform( m );

	if( inode->Selected( ) )
		gw->setColor( LINE_COLOR, 1.0f, 1.0f, 1.0f );

	else if( !inode->IsFrozen( ) )
		gw->setColor( LINE_COLOR, 0.4f, 0.0f, 0.6f );

	mesh.render( gw, mtl, NULL, COMP_ALL );

	gw->setRndLimits( rlim );

	return 0;
}

RefResult SwitchNodeObject::NotifyRefChanged( Interval changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message )
{
	switch( message )
	{
	case REFMSG_CHANGE:
		{
			ParamID pid = pblock2->LastNotifyParamID( );
			switch( pid )
			{
			case PB_NUMBRANCHES:
				{
					int numBranches = pblock2->GetInt( PB_NUMBRANCHES );
					if( numBranches - branchMap.Count( ) > 0 )
					{
						for( int i = 0; i < ( numBranches - branchMap.Count( ) ); i++ )
							branchMap.Append( 1, new INodeTab( ) );
					}
				}
				break;

			case PB_BRANCHIDX:
				{
					int branchIdx = pblock2->GetInt( PB_BRANCHIDX );

					if( branchIdx != -1 )
						pblock2->SetValue( PB_NUMCHILDREN, 0, branchMap[ branchIdx ].Count( ) );
				}
				break;

			case PB_CHILDIDX:
				{
					int branchIdx = pblock2->GetInt( PB_BRANCHIDX );
					int childIdx = pblock2->GetInt( PB_CHILDIDX );

					// If clearing variables don't go any further
					if( branchIdx == -1 || childIdx == -1 )
					{
						// Nullify PB_GETCHILD
						pblock2->SetValue( PB_GETCHILD, 0, NULL );
						break;
					}
					
					INode* inode = NULL;

					// Index exists in map
					if( childIdx < branchMap[ branchIdx ].Count( ) )
					{
						// Get the child
						inode = branchMap[ branchIdx ][ childIdx ];

						// Assign child to PB_GETCHILD
						pblock2->SetValue( PB_GETCHILD, 0, inode );
					}

					// Index does not exist in map
					else
					{
						inode = NULL;

						// Extend map
						pblock2->SetValue( PB_SETCHILD, 0, inode );
					}
				}
				break;

			case PB_SETCHILD:
				{
					int branchIdx = pblock2->GetInt( PB_BRANCHIDX );
					int childIdx = pblock2->GetInt( PB_CHILDIDX );

					// Don't execute below code when clearing variables
					if( childIdx == -1 || branchIdx == -1 )
						break;

					// Get the child node to add. NULL has two purposes
					// 1. Extends the list if there is no room
					// 2. Clears the PB_CHILD variable to ensure no dependency link errors.

					INode* inode = pblock2->GetINode( PB_SETCHILD );

					// If slot exists
					if( childIdx < branchMap[ branchIdx ].Count( ) )
					{
						// Don't execute the below code when clearing PB_CHILD
						if( inode != NULL )
						{
							// Set PB_CHILD to NULL otherwise dependency link
							pblock2->SetValue( PB_SETCHILD, 0, (INode*) NULL );

							// Find node that this object is attached to
							if( INode* pnode = FindNodeRef( this ) )
							{
								// Attach child and add to branch map
								pnode->AttachChild( inode );
								branchMap[ branchIdx ][ childIdx ] = inode;
							}

							pblock2->SetValue( PB_BRANCHIDX, 0, -1 );
							pblock2->SetValue( PB_CHILDIDX, 0, -1 );
						}
					}

					// Slot doesn't exist
					else
						// extend map
						branchMap[ branchIdx ].Append( 1, &inode );
				}
				break;

			case PB_ACTIVEBRANCH:
					// Update the active branch and UI
					UpdateBranches( );
				break;
			}
			InvalidateUI( );
		}
		break;

	case REFMSG_NODE_LINK:
		break;

	//case REFMSG_NODE_DISPLAY_PROP_CHANGED:
	//	switch( partID )
	//	{
	//	case PART_REND_PROP_RENDERABLE:
	//		{
	//			INode* node = GetNodeRef( this );
	//			bool isHidden = node->IsNodeHidden( );

	//			int t = 10;
	//			t++;
	//		}
	//		break;
	//	}
	//	break;
	}

	return REF_SUCCEED;
}

void SwitchNodeObject::InvalidateUI( )
{
	param_blk.InvalidateUI( pblock2->LastNotifyParamID( ) );
}

Interval SwitchNodeObject::ObjectValidity( TimeValue t )
{
	Interval ivalid = FOREVER;

	return ivalid;
}

ObjectState SwitchNodeObject::Eval( TimeValue t )
{
	return ObjectState( this );
}

RefTargetHandle SwitchNodeObject::Clone( RemapDir& remap )
{
	SwitchNodeObject* newob = new SwitchNodeObject( );
	
	newob->ReplaceReference( 0, remap.CloneRef( pblock2 ) );
	BaseClone( this, newob, remap );

	return newob;
}

IOResult SwitchNodeObject::Save( ISave* isave )
{
	return IO_OK;
}

IOResult SwitchNodeObject::Load( ILoad* iload )
{
	return IO_OK;
}

int SwitchNodeObject::HitTest( TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2* p, ViewExp* vpt )
{
	Matrix3 m;
	int result = FALSE;
	HitRegion hitRegion;
	DWORD savedLimits;

	MakeHitRegion( hitRegion, type, crossing, 4, p );

	GraphicsWindow* gw = vpt->getGW( );
	savedLimits = gw->getRndLimits( );
	gw->setRndLimits( ( savedLimits | GW_PICK ) & ~GW_ILLUM );

	m = inode->GetObjectTM( t );

	gw->setTransform( m );
	gw->clearHitCode( );

	Material* mtl = gw->getMaterial( );
	if( mesh.select( gw, mtl, &hitRegion, flags & HIT_ABORTONHIT ) )
		result = TRUE;

	gw->setRndLimits( savedLimits );

	return result;
}

CreateMouseCallBack* SwitchNodeObject::GetCreateMouseCallBack( )
{
	switchNodeCreateCB.SetObj( this );
	return &switchNodeCreateCB;
}

INode* SwitchNodeObject::GetNodeRef( ReferenceMaker* rm )
{
	if( rm->SuperClassID( ) == BASENODE_CLASS_ID )
		return (INode*) rm;

	else
		return rm->IsRefTarget( ) ? FindNodeRef( (ReferenceTarget*) rm ) : NULL;
}

INode* SwitchNodeObject::FindNodeRef( ReferenceTarget* rt )
{
	DependentIterator di( rt );
	ReferenceMaker* rm;
	INode* nd = NULL;

	while( rm = di.Next( ) )
	{
		nd = GetNodeRef( rm );
		if( nd )
			return nd;
	}

	return NULL;
}

void SwitchNodeObject::AddNodeToList( INode* node )
{
	if( INode* inode = FindNodeRef( this ) )
	{
		inode->AttachChild( node );
		pblock2->Append( PB_ACTIVECHILDREN, 1, &node, 1 );

		int count = pblock2->GetInt( PB_ACTIVEBRANCH );
		branchMap[ count ].Append( 1, &node );
	}
}

void SwitchNodeObject::RemoveNodeFromList( int index )
{
	if( index == -1 )
		return;

	if( INode* inode = FindNodeRef( this ) )
	{
		INode* node = pblock2->GetINode( PB_ACTIVECHILDREN, 0, index );
		if( node != NULL )
			node->Detach( 0 );

		pblock2->Delete( PB_ACTIVECHILDREN, index, 1 );
		
		int activeBranch = pblock2->GetInt( PB_ACTIVEBRANCH );
		branchMap[ activeBranch ].Delete( index, 1 );
	}
}

static void UpdateVisibility( INode* parent, BOOL hidden )
{
	int childCount = parent->NumberOfChildren( );
	parent->Hide( hidden );

	for( int i = 0; i < childCount; i++ )
		UpdateVisibility( parent->GetChildNode( i ), hidden );
}

void SwitchNodeObject::UpdateBranches( int oldBranch )
{
	int selBranch;
	int count;

	pblock2->GetValue( PB_ACTIVEBRANCH, 0, selBranch, FOREVER );
	SetAllNodesVisibility( TRUE );

	count = branchMap[ selBranch ].Count( );
	pblock2->Resize( PB_ACTIVECHILDREN, count );

	for( int i = 0; i < count; i++ )
	{
		INode* curNode = branchMap[ selBranch ][ i ];
		pblock2->SetValue( PB_ACTIVECHILDREN, 0, curNode, i );

		//curNode->Hide( FALSE );
		UpdateVisibility( curNode, FALSE );
	}
	
}

void SwitchNodeObject::AddBranch( )
{
	branchMap.Append( 1, new INodeTab( ) );

	pblock2->SetValue( PB_NUMBRANCHES, 0, (int) branchMap.Count( ) );
	pblock2->SetValue( PB_ACTIVEBRANCH, 0, (int) branchMap.Count( ) - 1 );
}

void SwitchNodeObject::RemoveBranch( int index )
{
	pblock2->SetValue( PB_NUMBRANCHES, 0, (int) branchMap.Count( ) - 1 );

	int count = branchMap[ index ].Count( );
	for( int i = 0; i < count; i++ )
	{
		INode* curNode = branchMap[ index ][ 0 ];

		curNode->Detach( 0 );
		//curNode->Hide( FALSE );
		UpdateVisibility( curNode, FALSE );

		branchMap[ index ].RemoveNode( curNode );
	}

	branchMap.Delete( index, 1 );

	pblock2->SetValue( PB_ACTIVEBRANCH, 0, ( index != 0 ) ? index - 1 : 0 );
}

void SwitchNodeObject::SetAllNodesVisibility( BOOL hidden )
{
	int branchCount = branchMap.Count( );
	for( int i = 0; i < branchCount; i++ )
	{
		int nodeCount = branchMap[ i ].Count( );
		for( int j = 0; j < nodeCount; j++ )
		{
			INode* curNode = branchMap[ i ][ j ];
			if( curNode->GetObjectRef( )->ClassID( ) == SwitchNode_CLASS_ID )
			{
				// TODO: Visibility control
			}

			UpdateVisibility( curNode, hidden );
		}
	}
}

INodeTab* SwitchNodeObject::GetNodeChildren( int index )
{
	return &branchMap[ index ];
}