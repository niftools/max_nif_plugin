
#include "stdafx.h"

extern Class_ID SwitchNode_CLASS_ID;

int Exporter::mVersion = 013;
bool Exporter::mSelectedOnly = false;
bool Exporter::mTriStrips = true;
bool Exporter::mExportHidden = false;
bool Exporter::mExportFurn = true;
bool Exporter::mExportLights = false;
bool Exporter::mVertexColors = true;
float Exporter::mWeldThresh = 0.01f;
float	Exporter::mNormThresh = 0.01f;
float	Exporter::mUVWThresh = 0.01f;
std::wstring Exporter::mTexPrefix = _T( "textures" );
bool Exporter::mExportCollision = true;
bool Exporter::mRemapIndices = true;
bool Exporter::mUseRegistry = false;
bool Exporter::mExportExtraNodes = false;
bool Exporter::mExportSkin = false;
bool Exporter::mUserPropBuffer = false;
bool Exporter::mFlattenHierarchy = false;
bool Exporter::mRemoveUnreferencedBones = false;
bool Exporter::mSortNodesToEnd = false;
std::wstring Exporter::mGameName = _T( "User" );
std::wstring Exporter::mNifVersion = _T( "20.0.0.5" );
int Exporter::mNifVersionInt = Niflib::VER_20_0_0_5;
int Exporter::mNifUserVersion = 0;
int Exporter::mNifUserVersion2 = 0;
bool Exporter::mSkeletonOnly = false;
bool Exporter::mExportCameras = false;
bool Exporter::mGenerateBoneCollision = false;
bool Exporter::mExportTransforms = true;
float Exporter::mDefaultPriority = 0.0f;
Exporter::ExportType Exporter::mExportType = NIF_WO_ANIM;
bool Exporter::mMultiplePartitions = false;
int Exporter::mBonesPerVertex = 4;
int Exporter::mBonesPerPartition = 20;
bool Exporter::mUseTimeTags = false;
bool Exporter::mAutoDetect = true;
bool Exporter::mAllowAccum = true;
std::wstring Exporter::mCreatorName;
bool Exporter::mCollapseTransforms = false;
bool Exporter::mZeroTransforms = false;
bool Exporter::mFixNormals = false;
bool Exporter::mTangentAndBinormalExtraData = false;
bool Exporter::mSupportPrnStrings = false;
stringlist Exporter::mRotate90Degrees;
bool Exporter::mSuppressPrompts = false;
bool Exporter::mUseAlternateStripper = false;
float Exporter::bhkScaleFactor = 7.0f;
int Exporter::mTangentAndBinormalMethod = 0;
bool Exporter::mStartNifskopeAfterStart = false;
std::wstring Exporter::mNifskopeDir;
bool Exporter::mTriPartStrips = true;
std::wstring Exporter::mRootType;
stringlist Exporter::mRootTypes;

std::wstring Exporter::mExportPath;

static bool IsNodeOrParentSelected( INode* node )
{
	if( node == NULL )
		return false;

	if( node->Selected( ) )
		return true;

	return IsNodeOrParentSelected( node->GetParentNode( ) );
}

Exporter::Exporter( Interface* i, AppSettings* appSettings )
	: mI( i ), mAppSettings( appSettings ), mSceneCollisionNode( NULL )
{
	memset( progressCounters, 0, sizeof( progressCounters ) );
	memset( progressMax, 0, sizeof( progressMax ) );
}

Exporter::Result Exporter::doExport( Niflib::NiNodeRef& root, INode* node )
{
	root->SetName( "Scene Root" );

	int nifVersion = Niflib::ParseVersionString( narrow( Exporter::mNifVersion ) );

	if( mUseTimeTags && nifVersion >= Niflib::VER_20_0_0_4 )
		throw runtime_error( "Time tag sequences are not supported for version 20.0.0.4 or higher." );

	if( !Exporter::mSelectedOnly )
	{
		if( IsOblivion( ) || IsFallout3 ( ) || IsSkyrim( ) )
		{
			if( mSkeletonOnly )
			{
				CalcBoundingBox( node, mBoundingBox );

				Niflib::BSBoundRef bsb = CreateNiObject<Niflib::BSBound>( );
				bsb->SetName( "BBX" );
				bsb->SetCenter( TOVECTOR3( mBoundingBox.Center( ) ) );
				bsb->SetDimensions( TOVECTOR3( mBoundingBox.Width( ) / 2.0f ) );
				root->AddExtraData( Niflib::DynamicCast<Niflib::NiExtraData>( bsb ) );

				Niflib::BSXFlagsRef bsx = CreateNiObject<Niflib::BSXFlags>( );
				bsx->SetName( "BSX" );
				bsx->SetData( 0x00000007 );
				root->AddExtraData( Niflib::DynamicCast<Niflib::NiExtraData>( bsx ) );
			}
			else if( ( mExportType != NIF_WO_ANIM ) && !IsSkyrim( ) )
			{
				Niflib::BSXFlagsRef bsx = CreateNiObject<Niflib::BSXFlags>( );
				bsx->SetName( "BSX" );
				bsx->SetData( 0x00000003 );
				root->AddExtraData( Niflib::DynamicCast<Niflib::NiExtraData>( bsx ) );
			}
			else if( mExportCollision )
			{
				Niflib::BSXFlagsRef bsx = CreateNiObject<Niflib::BSXFlags>( );
				bsx->SetName( "BSX" );
				bsx->SetData( IsSkyrim( ) ? 198 : IsFallout3( ) ? 202 : 2 );
				root->AddExtraData( Niflib::DynamicCast<Niflib::NiExtraData>( bsx ) );
			}
		}

		exportUPB( root, node );
	}

	// Always Scan for Collision Nodes first
	scanForIgnore( node );
	scanForCollision( node );
	scanForAnimation( node );

	mNiRoot = root;
	if( mSelectedOnly )
	{
		int count = 0;
		int n = mI->GetSelNodeCount( );
		std::vector<INode*> selectedRoots;

		for( int i = 0; i < n; ++i )
		{
			INode* selNode = mI->GetSelNode( i );
			if( !IsNodeOrParentSelected( selNode->GetParentNode( ) ) )
			{
				selectedRoots.push_back( selNode );
				count += countNodes( selNode );
			}
		}

		if( selectedRoots.size( ) == 0 )
			throw runtime_error( "No Nodes have been selected for Export." );

		progressMax[ Geometry ] = progressMax[ Skin ] = count;
		if( mExportCollision )
			progressMax[ Collision ] = progressMax[ Geometry ];

		if( mExportType == NIF_WO_ANIM )
			progressMax[ Animation ] = progressMax[ Geometry ];

		for( size_t i = 0; i < selectedRoots.size( ); i++ )
		{
			Result result = exportNodes( root, selectedRoots[ i ] );
			if( result != Ok && result != Skip )
				return result;

			if( mExportCollision )
			{
				result = exportCollision( root, selectedRoots[ i ] );
				if( result != Ok )
					return result;
			}	
		}

		// Always Zero out root transforms
		std::vector<Niflib::NiAVObjectRef> children = root->GetChildren( );
		for( size_t i = 0; i < children.size( ); ++i )
			children[ i ]->SetLocalTransform( Niflib::Matrix44::IDENTITY );

		// Fix Used Nodes that were never properly initialized.  Happens normally during select export
		for( NodeMap::iterator itr = mNameMap.begin( ); itr != mNameMap.end( ); ++itr )
		{
			Niflib::NiNodeRef bone = (*itr).second;
			if( bone->GetParent( ) == NULL )
			{
				if( INode* boneNode = mI->GetINodeByName( (*itr).first.c_str( ) ) )
					makeNode( root, boneNode, false );
			}
		}

		// Special case when exporting a single branch, use first child as scene root
		if( selectedRoots.size( ) == 1 )
		{
			std::vector<Niflib::NiNodeRef> childnodes = Niflib::DynamicCast<Niflib::NiNode>( root->GetChildren( ) );
			if( childnodes.size( ) == 1 )
			{
				Niflib::NiNodeRef child = childnodes[ 0 ];
				root->RemoveChild( child );
				root = child;
				mNiRoot = root;
				exportPrn( root, selectedRoots[ 0 ] );
			}
		}
	}
	else
	{
		// Estimate progress bar
		int count = countNodes( node );
		progressMax[ Geometry ] = progressMax[ Skin ] = count;

		if( mExportCollision )
			progressMax[ Collision ] = progressMax[ Geometry ];

		if( mExportType == NIF_WO_ANIM )
			progressMax[ Animation ] = progressMax[ Geometry ];

		// Calc Global Animation Range

		// Normal export
		Result result = exportNodes( root, node );
		if( result != Ok )
			return result;

		// Fix Used Nodes that where never properly initialized.  Happens normally during select export
		for( NodeMap::iterator itr = mNameMap.begin( ); itr != mNameMap.end( ); ++itr )
		{
			Niflib::NiNodeRef bone = (*itr).second;
			if( bone->GetParent( ) == NULL )
			{
				if( INode* boneNode = mI->GetINodeByName( (*itr).first.c_str( ) ) )
					makeNode( root, boneNode, false );
			}
		}

		if( mExportCollision )
		{
			result = exportCollision( root, node );
			if( result != Ok )
				return result;
		}
	}

	// handle post export callbacks (like skin)
	progressMax[ Skin ] = mPostExportCallbacks.size( );
	for( CallbackList::iterator cb = mPostExportCallbacks.begin( ); cb != mPostExportCallbacks.end( ); cb = mPostExportCallbacks.erase( cb ) )
	{
		ProgressUpdate( Skin, NULL );
		(*cb)->execute( );
		delete (*cb);
	}

	// Remove unreferenced bones
	if( mRemoveUnreferencedBones )
		removeUnreferencedBones( mNiRoot );

	if( mSortNodesToEnd )
		sortNodes( mNiRoot );

	ApplyAllSkinOffsets( Niflib::StaticCast<Niflib::NiAVObject>( mNiRoot ) );
	root = mNiRoot;

	return Ok;
}

// Primary recursive decent routine
Exporter::Result Exporter::exportNodes( Niflib::NiNodeRef& parent, INode* node )
{
	TSTR nodeName = node->GetName( );
	ProgressUpdate( Geometry, FormatText( _T( "'%s' Geometry" ), nodeName.data( ) ) );

	bool coll = isCollision( node );

	// Abort if is a collision node or is hidden and we are not exporting hidden
	if( coll || ( node->IsHidden( ) && !mExportHidden ) || isHandled( node ) )
		return Skip;

	bool local = !mFlattenHierarchy;
	Niflib::NiNodeRef nodeParent = mFlattenHierarchy ? mNiRoot : parent;

	Niflib::NiNodeRef newParent;
	TimeValue t = 0;
	ObjectState os = node->EvalWorldState( t );

	// Always skip bones and bipeds
	SClass_ID scid = node->SuperClassID( );
	Class_ID ncid = node->ClassID( );

	if( node->IsBoneShowing( ) )
		newParent = exportBone( nodeParent, node );

	else if( os.obj && os.obj->SuperClassID( ) == GEOMOBJECT_CLASS_ID )
	{
		SClass_ID oscid = os.obj->SuperClassID( );
		Class_ID oncid = os.obj->ClassID( );

		if( wildmatch( _T( "Bip?? Footsteps" ), nodeName ) )
		{
			// ignore footsteps
		}
		else if( Exporter::mNifVersionInt <= Niflib::VER_4_2_2_0 && strmatch( _T( "Bounding Box" ), nodeName ) )
		{
			// Morrowind style
			newParent = exportBone( nodeParent, node );
			Box3 b;
			CalcBoundingBox( node, b, 1 );
			
			Niflib::BoundingBox bb;
			bb.unknownInt = 1;
			bb.rotation = Niflib::Matrix33::IDENTITY;
			bb.radius = TOVECTOR3( b.Width( ) / 2.0f );
			bb.translation = newParent->GetLocalTranslation( );

			newParent->SetBoundingBox( bb );
			newParent->SetCollisionMode( Niflib::NiAVObject::CT_BOUNDINGBOX );
		}
		else if( os.obj && ( os.obj->ClassID( ) == BONE_OBJ_CLASSID
			|| os.obj->ClassID( ) == Class_ID( BONE_CLASS_ID, 0 )
			|| os.obj->ClassID( ) == Class_ID( 0x00009125, 0 ) /* Biped Twist Helpers */
			) )
		{
			newParent = exportBone( nodeParent, node );
		}
		else if( os.obj->ClassID( ) == SKELOBJ_CLASS_ID )
		{
			// ignore skeleton objects
		}
		else if( !mSkeletonOnly )
		{
			if( mExportType != NIF_WO_ANIM && isNodeTracked( node ) )
				// Create Node + Accum if has start track
				newParent = createAccumNode( makeNode( nodeParent, node, local ), node );

			else if( mExportExtraNodes || ( mExportType != NIF_WO_ANIM && isNodeKeyed( node ) ) )
				// Create node if using Extra Nodes or if exporting with anim and node has key values
				newParent = makeNode( nodeParent, node, local );

			else
				// Else dont create a node
				newParent = nodeParent;

			// No need to export meshes when NIF is not exported
			if( mExportType != SINGLE_KF_WO_NIF && mExportType != MULTI_KF_WO_NIF )
			{
				Result result = exportMesh( newParent, node, t );
				if( result != Ok )
					return result;
			}
		}
	}
	else if( os.obj && os.obj->ClassID( ) == SwitchNode_CLASS_ID )
	{
		newParent = makeSwitchNode( nodeParent, node, local );
		return Ok;
	}

	else if( mExportCameras && os.obj && os.obj->SuperClassID( ) == CAMERA_CLASS_ID )
		newParent = makeNode( nodeParent, node, local );

	else if( mExportLights && os.obj && os.obj->SuperClassID( ) == LIGHT_CLASS_ID )
		return exportLight( nodeParent, node, (GenLight*) os.obj );

	else if( isMeshGroup( node ) && local && !mSkeletonOnly ) // only create node if local
		newParent = makeNode( parent, node, local );

	else
		newParent = parent;

	for( int i = 0; i < node->NumberOfChildren( ); i++ )
	{
		Result result = exportNodes( newParent, node->GetChildNode( i ) );
		if( result != Ok && result != Skip )
			return result;
	}

	return Ok;
}

bool Exporter::IsSkyrim( ) const
{
	return ( mNifVersionInt == 0x14020007 && mNifUserVersion == 12 );
}

bool Exporter::IsFallout3( ) const
{
	return ( mNifVersionInt == 0x14020007 && mNifUserVersion == 11 );
}

bool Exporter::IsOblivion( ) const
{
	return ( ( mNifVersionInt == 0x14000004 || mNifVersionInt == 0x14000005 ) && ( mNifUserVersion == 11 || mNifUserVersion == 10 ) );
}

bool Exporter::IsMorrowind( ) const 
{
	return ( ( mNifVersionInt == 0x04000002 ) && ( mNifUserVersion == 11 || mNifUserVersion == 10 ) );
}

void Exporter::ProgressUpdate( ProgressSection section, const TCHAR* s )
{
	if( mSuppressPrompts )
		return;

	if( mI->GetCancel( ) )
		throw CancelExporterException( );

	int total = 1;
	int percent = 1;

	for( int i = 0; i < int( ProgressSectionCount ); i ++ )
	{
		total += progressMax[ i ];
		if( i < section )
			percent += progressMax[ i ];

		else
			percent += ( ++progressCounters[ i ] );
	}

	mI->ProgressUpdate( ( percent * 100 ) / total, s == NULL ? TRUE : FALSE, const_cast<TCHAR*>( s ) );
}