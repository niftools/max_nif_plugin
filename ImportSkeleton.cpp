
#include "stdafx.h"

extern Point3 TOEULER(const Matrix3 &m);
inline Point3 TORAD(const Point3& p){
   return Point3(TORAD(p[0]), TORAD(p[1]), TORAD(p[2]));
}

void GoToSkeletonBindPosition( std::vector<Niflib::NiNodeRef>& blocks )
{
	// Send all skeleton roots to bind positions
	for( unsigned int i = 0; i < blocks.size( ); ++i )
	{
		Niflib::NiNodeRef node = blocks[ i ];
		if( node != NULL && node->IsSkeletonRoot( ) )
			node->GoToSkeletonBindPosition( );
	}
}

float GetObjectLength( Niflib::NiAVObjectRef obj )
{
	float clen = obj->GetLocalTranslation( ).Magnitude( );
	if( clen < ( FLT_EPSILON * 10 ) )
	{
		if( Niflib::NiTriBasedGeomRef geom = Niflib::DynamicCast<Niflib::NiTriBasedGeom>( obj ) )
		{
			if( Niflib::NiTriBasedGeomDataRef data = geom->GetData( ) )
				clen = data->GetRadius( ) * 2.0f;
		}
	}

	return clen;
}

static bool HasControllerRef( std::map<std::wstring, int>& ctrlCount, const std::wstring& name )
{
	return ( ctrlCount.find( name ) != ctrlCount.end( ) );
}

bool NifImporter::HasSkeleton( )
{
	if( !skeletonCheck.empty( ) )
	{
		std::vector<Niflib::NiNodeRef> bipedRoots = SelectNodesByName( nodes, skeletonCheck.c_str( ) );
		return !bipedRoots.empty( );
	}

	return false;
}

bool NifImporter::IsBiped( )
{
	if( HasSkeleton( ) )
	{
		Niflib::NiNodeRef rootNode = root;
		if( rootNode )
		{
			std::list<Niflib::NiExtraDataRef> extraData = rootNode->GetExtraData( );
			if( !extraData.empty( ) )
			{
				if( Niflib::BSXFlagsRef flags = SelectFirstObjectOfType<Niflib::BSXFlags>( extraData ) )
					return ( flags->GetData( ) & 0x04 );
			}
		}
	}

	return false;
}

void NifImporter::ImportBones( Niflib::NiNodeRef node, bool recurse )
{
	try
	{
		//if( uncontrolledDummies )
		//	BuildControllerRefList( node, ctrlCount );

		std::wstring name = wide( node->GetName( ) );

		// Use type name if it has no name
		if( name.empty( ) )
			name = wide( node->GetType( ).GetTypeName( ) );

		std::vector<Niflib::NiAVObjectRef> children = node->GetChildren( );
		std::vector<Niflib::NiNodeRef> childNodes = Niflib::DynamicCast<Niflib::NiNode>( children );

		Niflib::NiAVObject::CollisionType cType = node->GetCollisionMode( );
		if( children.empty( ) && name == _T( "Bounding Box" ) )
			return;

		// Do all node manipulations here
		Niflib::NiNodeRef parent = node->GetParent( );
		std::wstring parentname = ( parent ? wide( parent->GetName( ) ) : _T( "" ) );
		Niflib::Matrix44 m4 = node->GetWorldTransform( );

		// Use type name if it has no name
		if( parent && parentname.empty( ) )
			parentname = wide( parent->GetType( ).GetTypeName( ) );

		// Check for Prn strings and change parent if necessary
		if( supportPrnStrings )
		{
			std::list<Niflib::NiStringExtraDataRef> strings = Niflib::DynamicCast<Niflib::NiStringExtraData>( node->GetExtraData( ) );
			for( std::list<Niflib::NiStringExtraDataRef>::iterator itr = strings.begin( ); itr != strings.end( ); ++itr )
			{
				if( strmatch( wide( (*itr)->GetName( ) ), _T( "Prn" ) ) )
				{
					parentname = wide( (*itr)->GetData( ) );
					if( INode* pn = GetNode( parentname ) )
					{
						// Apparently Heads tend to need to be rotated by 90 degrees on import for
						if( !rotate90Degrees.empty( ) && wildmatch( rotate90Degrees, parentname ) )
							m4 *= TOMATRIX4( RotateYMatrix( TORAD( 90 ) ) );

						m4 *= TOMATRIX4( pn->GetObjTMAfterWSM( 0, NULL ) );
					}
				}
			}
		}

		float len = node->GetLocalTranslation( ).Magnitude( );

		// Remove NonAccum nodes and merge into primary bone
		if( mergeNonAccum && wildmatch( _T( "* NonAccum" ), name ) && parent )
		{
			std::wstring realname = name.substr( 0, name.length( ) - 9 );
			if( strmatch( realname, wide( parent->GetName( ) ) ) )
			{
				Niflib::Matrix44 tm = parent->GetLocalTransform( ) + node->GetLocalTransform( );
				name = realname;
				len += tm.GetTranslation( ).Magnitude( );
				parent = parent->GetParent( );
			}
		}

		PosRotScale prs = prsDefault;
		Niflib::Vector3 pos;
		Niflib::Matrix33 rot;
		float scale;

		m4.Decompose( pos, rot, scale );

		Matrix3 im = TOMATRIX3( m4 );
		Point3 p = im.GetTrans( );
		Quat q( im );

		Niflib::Vector3 ppos;
		Point3 zAxis( 0, 0, 0 );
		bool hasChildren = !children.empty( );

		if( hasChildren )
		{
			float len = 0.0f;
			for( std::vector<Niflib::NiAVObjectRef>::iterator itr = children.begin( ), end = children.end( ); itr != end; ++itr )
				len += GetObjectLength( *itr );

			len /= float( children.size( ) );
			ppos = pos + Niflib::Vector3( len, 0.0f, 0.0f ); // Just really need magnitude as rotation will take care of positioning
		}
		else if( parent )
			ppos = pos + Niflib::Vector3( len / 3.0f, 0.0f, 0.0f );

		Point3 pp( ppos.x, ppos.y, ppos.z );
		Point3 qp = TORAD( TOEULER( im ) );

		INode* bone = NULL;
		//if( !doNotReuseExistingBones ) // Games like BC3 reuse the same bone names
		//{
		//	bone = FindNode( node );
		//	if( bone == NULL )
		//		bone = GetNode( name );
		//}

		if( bone )
		{
			// Is there a better wway of "Affect Pivot Only" behaviours?
			INode* pinode = bone->GetParentNode( );
			if( pinode )
				bone->Detach( 0, 1 );

			PosRotScaleNode( bone, p, q, scale, prs );
			if( pinode )
				pinode->AttachChild( bone, 1 );
		}
		else
		{
			bool isDummy = ( ( uncontrolledDummies && !HasControllerRef( ctrlCount, name ) )
				|| ( !dummyNodeMatches.empty( ) && wildmatch( dummyNodeMatches, name ) )
				|| ( convertBillboardsToDummyNodes && node->IsDerivedType( Niflib::NiBillboardNode::TYPE ) )
				);

			if( node->IsDerivedType( Niflib::NiSwitchNode::TYPE ) )
				specialNodes.push_back( node );
			
			if( wildmatch( _T( "Camera*" ), name ) )
			{
				if( enableCameras )
				{
					if( bone = CreateCamera( name ) )
					{
						PosRotScaleNode( bone, p, q, scale, prs );
						bone->Hide( node->GetVisibility( ) ? FALSE : TRUE );
					}
				}
			}
			else if( isDummy || createNubsForBones )
			{
				bone = CreateHelper( name, p );
				PosRotScaleNode( bone, p, q, scale, prs );
			}
			else if( bone = CreateBone( name, p, pp, zAxis  ) )
			{
				PosRotScaleNode( bone, p, q, scale, prs );
				bone->Hide( node->GetVisibility( ) ? FALSE : TRUE );
			}

			if( bone )
			{
				if( !parentname.empty( ) )
				{
					if( mergeNonAccum && wildmatch( _T( "* NonAccum" ), parentname ) )
						parentname = parentname.substr( 0, parentname.length( ) - 9 );

					//if( INode* pn = GetNode( parentname ) )
					if( INode* pn = FindNode( parent ) )
						pn->AttachChild( bone, 1 );
				}

				RegisterNode( node, bone );
			}
		}

		// Import UPB
		if( bone )
			ImportUPB( bone, node );

		// Import Havok Collision Data surrounding node,
		//     imfortunately this causes double import of collision so I'm disabling it for now.
		if( enableCollision && node->GetParent( ) )
			ImportCollision( node );

		if( bone && recurse )
			ImportBones( childNodes );
	}
	catch( std::exception& e )
	{
		e = e;
	}
	catch( ... )
	{
	}
}

INode* NifImporter::CreateCamera( const std::wstring& name )
{
	if( GenCamera* ob = (GenCamera*) gi->CreateInstance( CAMERA_CLASS_ID, Class_ID( SIMPLE_CAM_CLASS_ID, 0 ) ) )
	{
		ob->Enable( 1 );
		ob->NewCamera( 0 );
		ob->SetFOV( 0, TORAD( 75.0f ) );

		if( INode* n = gi->CreateObjectNode( ob ) )
		{
			n->Hide( TRUE );
			n->BoneAsLine( 1 );

			return n;
		}
	}

	return NULL;
}

INode* NifImporter::CreateHelper( const std::wstring& name, Point3 startPos )
{
	if( DummyObject* ob = (DummyObject*) gi->CreateInstance( HELPER_CLASS_ID, Class_ID( DUMMY_CLASS_ID, 0 ) ) )
	{
		const float DUMSZ = 1.0f;
		ob->SetBox( Box3( Point3( -DUMSZ, -DUMSZ, -DUMSZ ), Point3( DUMSZ, DUMSZ, DUMSZ ) ) );

		if( INode* n = CreateImportNode( name.c_str( ), ob, NULL ) )
		{
			n->SetWireColor( RGB( 192, 192, 192 ) );

			Quat q;
			q.Identity( );
			PosRotScaleNode( n, startPos, q, 1.0f, prsPos );

#if VERSION_3DSMAX > ((6000<<16)+(15<<8)+0) // Version 6

			n->BoneAsLine( dummyBonesAsLines ? 1 : 0 );
			n->ShowBone( 2 );

#else

			n->ShowBone( 1 );

#endif

			return n;
		}
	}

	return NULL;
}

INode* NifImporter::CreateBone( const std::wstring& name, Point3 startPos, Point3 endPos, Point3 zAxis )
{
	if( FPInterface* fpBones = GetCOREInterface( Interface_ID( 0x438aff72, 0xef9675ac ) ) )
	{
		FunctionID createBoneID = fpBones->FindFn( _T( "createBone" ) );
		FPValue result;
		FPParams params( 3, TYPE_POINT3, &startPos, TYPE_POINT3, &endPos, TYPE_POINT3, &zAxis );
		FPStatus status = fpBones->Invoke( createBoneID, result, &params );

		if( status == FPS_OK && result.type == TYPE_INODE )
		{
			if( INode* n = result.n )
			{
				n->SetName( const_cast<TCHAR*>( name.c_str( ) ) );

				float len = Length( endPos - startPos );
				float width = max( minBoneWidth, min( maxBoneWidth, len * boneWidthToLengthRatio ) );

				if( Object* o = n->GetObjectRef( ) )
				{
					setMAXScriptValue( o->GetReference( 0 ), _T( "width" ), 0, width );
					setMAXScriptValue( o->GetReference( 0 ), _T( "height" ), 0, width );
				}

#if VERSION_3DSMAX > ((6000<<16)+(15<<8)+0) // Version 6

				n->BoneAsLine( 1 );
				n->ShowBone( 2 );

#else

				n->ShowBone( 1 );

#endif
			}

			return result.n;
		}

		fpBones->ReleaseInterface( );
	}

	return NULL;
}

bool NifImporter::ImportUPB( INode* node, Niflib::NiNodeRef block )
{
	if( !importUPB )
		return false;

	bool ok = false;
	if( node && block )
	{
		std::list<Niflib::NiStringExtraDataRef> strings = Niflib::DynamicCast<Niflib::NiStringExtraData>( block->GetExtraData( ) );
		for( std::list<Niflib::NiStringExtraDataRef>::iterator itr = strings.begin( ); itr != strings.end( ); ++itr )
		{
			if( strmatch( wide( (*itr)->GetName( ) ), _T( "UserPropBuffer" ) ) || strmatch( wide( (*itr)->GetName( ) ), _T( "UPB" ) ) )
			{
				wchar_t buffer[ 1048 ];
				std::wistringstream istr( wide( (*itr)->GetData( ) ), std::ios_base::out );
				
				while( !istr.eof( ) )
				{
					wchar_t* line = buffer;
					buffer[ 0 ] = 0;

					istr.getline( buffer, _countof( buffer ) - 1 );
					if( LPWSTR equals = wcschr( line, '=' ) )
					{
						*equals++ = 0;

						Trim( line );
						Trim( equals );

						if( line[ 0 ] && equals [ 0 ] )
						{
							node->SetUserPropString( TSTR( line ), TSTR( equals ) );
							ok |= true;
						}
						else
						{
							Trim( line );
							int len = wcslen( line );

							// Handle bethesda special values
							if( len > 0 && line[ len - 1 ] == '#' )
							{
								TSTR buf;
								TSTR value;

								node->GetUserPropBuffer( buf );
								value.append( line ).append( _T( "\r\n" ) ).append( buf );

								node->SetUserPropBuffer( value );
							}
						}
					}
				}
			}
		}
	}

	return ok;
}

bool NifImporter::ImportSpecialNodes( )
{
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

	for( std::vector<Niflib::NiNodeRef>::iterator itr = specialNodes.begin( ); itr != specialNodes.end( ); ++itr )
	{
		if( (*itr)->IsDerivedType( Niflib::NiSwitchNode::TYPE ) )
		{
			if( Niflib::NiSwitchNodeRef node = Niflib::DynamicCast<Niflib::NiSwitchNode>( (*itr) ) )
			{
				INode* inode = FindNode( node );
				if( HelperObject* switchObj = (HelperObject*) gi->CreateInstance( HELPER_CLASS_ID, Class_ID( 0x3f071130, 0x5c1973a0 ) ) )
				{
					inode->SetObjectRef( switchObj );
					inode->ShowBone( 0 );

					IParamBlock2* pblock2 = switchObj->GetParamBlockByID( switchnode_params );

					int numChildren = inode->NumberOfChildren( );
					pblock2->SetValue( PB_NUMBRANCHES, 0, numChildren );
					
					for( int i = 0; i < numChildren; i++ )
					{
						INode* child = inode->GetChildNode( 0 );
						INode* subChild = NULL;
						int numSubChildren = child->NumberOfChildren( );

						for( int j = 0; j < numSubChildren; j++ )
						{
							subChild = child->GetChildNode( 0 );
							subChild->Detach( 0 );

							pblock2->SetValue( PB_BRANCHIDX, 0, i );
							pblock2->SetValue( PB_CHILDIDX, 0, j );
							pblock2->SetValue( PB_SETCHILD, 0, subChild );
						}

						child->Detach( 0 );
						child->Delete( 0, FALSE );
					}

					int activeIndex = node->GetActiveIndex( );
					pblock2->SetValue( PB_ACTIVEBRANCH, 0, activeIndex );

					Niflib::SwitchNodeFlags flags = node->GetSwitchFlags( );
					if( ( flags & Niflib::SN_UPDATEONLYACTIVE ) == Niflib::SN_UPDATEONLYACTIVE )
						pblock2->SetValue( PB_UPDATEACTIVE, 0, TRUE );
				}
			}
		}
	}

	return true;
}