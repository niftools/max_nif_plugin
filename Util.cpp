
#include "stdafx.h"

void Exporter::CalcBoundingBox( INode* node, Box3& box, int all )
{
	if( NULL == node )
		return;

	Matrix3 tm = node->GetObjTMAfterWSM( 0 );
	if( node->IsBoneShowing( ) )
		box.IncludePoints( const_cast<Point3*>( &tm.GetTrans( ) ), 1, NULL );

	else
	{
		if( Object* o = node->GetObjectRef( ) )
		{
			if( o->SuperClassID( ) == GEOMOBJECT_CLASS_ID )
			{
				if( o->ClassID( ) == BONE_OBJ_CLASSID
				 || o->ClassID( ) == Class_ID( BONE_CLASS_ID, 0 )
				 || o->ClassID( ) == Class_ID( 0x00009125, 0 ) /* Biped Twist Helpers */
				 )
				{
					box.IncludePoints( const_cast<Point3*>( &tm.GetTrans( ) ), 1, NULL );
				}
				else
				{
					Box3 local;

					o->GetLocalBoundBox( 0, node, &mI->GetActiveViewExp( ), local );
					box.IncludePoints( &local.Min( ), 1, NULL );
					box.IncludePoints( &local.Max( ), 1, NULL );
				}
			}
			else if( mExportCameras && o->SuperClassID( ) == CAMERA_CLASS_ID )
				box.IncludePoints( const_cast<Point3*>( &tm.GetTrans( ) ), 1, NULL );
		}
	}

	if( all < 0 )
		return;

	all = ( all > 0 ? all : 1 );
	for( int i = 0; i < node->NumberOfChildren( ); i++ )
		CalcBoundingBox( node->GetChildNode( i ), box, all );
}

void Exporter::CalcBoundingSphere( INode* node, Point3 center, float& radius, int all )
{
	if( NULL == node )
		return;

	Matrix3 tm = node->GetObjTMAfterWSM( 0 );
	Point3 pt = ( tm.GetTrans( ) - center );
	float len = pt.Length( );

	if( node->IsBoneShowing( ) )
		radius = max( len, radius );

	else
	{
		if( Object* o = node->GetObjectRef( ) )
		{
			if( o->SuperClassID( ) == GEOMOBJECT_CLASS_ID )
			{
				if( o->ClassID( ) == BONE_OBJ_CLASSID
				 || o->ClassID( ) == Class_ID( BONE_CLASS_ID, 0 )
				 || o->ClassID( ) == Class_ID( 0x00009125, 0 ) /* Biped Twist Helpers */
				 )
				{
					radius = max( len, radius );
				}
				else
					radius = max( len, radius );
			}
			else if( mExportCameras && o->SuperClassID( ) == CAMERA_CLASS_ID )
			{
				radius = max( len, radius );
			}
		}
	}

	if( all < 0 )
		return;

	all = ( all > 0 ? all : -1 );
	for( int i = 0; i < node->NumberOfChildren( ); i++ )
		CalcBoundingSphere( node->GetChildNode( i ), center, radius, all );
}

bool Exporter::exportUPB( Niflib::NiNodeRef& root, INode* node )
{
	bool ok = false;
	if( !mUserPropBuffer )
		return ok;

	// Write the actual UPB sans any np_ prefixed strings
	TSTR upb;
	node->GetUserPropBuffer( upb );

	if( !upb.isNull( ) )
	{
		std::wstring line;
		std::wistringstream istr( std::wstring( upb ), std::ios_base::out );
		std::wostringstream ostr;

		while( !istr.eof( ) )
		{
			std::getline( istr, line );
			if( !line.empty( ) && 0 != line.compare( 0, 3, _T( "np_" ) ) )
				ostr << line << std::endl;
		}

		if( !ostr.str( ).empty( ) )
		{
			Niflib::NiStringExtraDataRef strings = CreateNiObject<Niflib::NiStringExtraData>( );
			strings->SetName( "UPB" );
			strings->SetData( narrow( ostr.str( ) ) );
			root->AddExtraData( Niflib::DynamicCast<Niflib::NiExtraData>( strings ) );
			ok = true;
		}
	}

	return ok;
}

bool Exporter::removeUnreferencedBones( Niflib::NiNodeRef node )
{
	Niflib::NiNodeRef parent = node->GetParent( );
	bool remove = ( NULL != parent ) && !node->IsSkinInfluence( );
	Niflib::Matrix44 ntm = node->GetLocalTransform( );
	std::vector<Niflib::NiAVObjectRef> children = node->GetChildren( );

	for( std::vector<Niflib::NiAVObjectRef>::iterator itr = children.begin( ); itr != children.end( ); ++itr )
	{
		Niflib::NiAVObjectRef& child = (*itr);
		bool childRemove = false;

		if( child->IsDerivedType( Niflib::NiNode::TYPE ) )
			childRemove = removeUnreferencedBones( Niflib::StaticCast<Niflib::NiNode>( child ) );

		if( childRemove )
			node->RemoveChild( child );

		else if( remove )
		{
			Niflib::Matrix44 tm = child->GetLocalTransform( );
			child->SetLocalTransform( ntm * tm );

			node->RemoveChild( child );
			mNiRoot->AddChild( child );
		}
	}

	return remove;
}

struct SortNodeEquivalence
{
	inline bool operator()( const Niflib::NiAVObjectRef& lhs, const Niflib::NiAVObjectRef& rhs ) const
	{
		if( !lhs ) return !rhs;
		if( !rhs ) return true;

		std::string ltype = lhs->GetType( ).GetTypeName( );
		std::string rtype = rhs->GetType( ).GetTypeName( );

		if( ltype == rtype )
			return false;

		if( ltype == "bhkCollisionObject" )
			return true;

		if( rtype == "bhkCollisionObject" )
			return false;

		if( ltype == "NiNode" )
			return false;

		else if( rtype == "NiNode" )
			return true;

		return( ltype < rtype );
	}
};

void Exporter::sortNodes( Niflib::NiNodeRef node )
{
	node->SortChildren( SortNodeEquivalence( ) );

	std::vector<Niflib::NiNodeRef> children = Niflib::DynamicCast<Niflib::NiNode>( node->GetChildren( ) );
	for( std::vector<Niflib::NiNodeRef>::iterator itr = children.begin( ); itr != children.end( ); ++itr )
		sortNodes( *itr );
}

Exporter::Result Exporter::scanForIgnore( INode* node )
{
	if( NULL == node )
		return Exporter::Skip;

	BOOL ignore = FALSE;
	if( node->GetUserPropBool( _T( "np_ignore" ), ignore ) )
		markAsHandled( node );

	for( int i = 0; i < node->NumberOfChildren( ); i++ )
		scanForIgnore( node->GetChildNode( i ) );

	return Exporter::Ok;
}

void Exporter::convertMatrix( Niflib::Matrix33& dst, const Matrix3& src )
{
	Point3 r0 = src.GetRow( 0 );
	Point3 r1 = src.GetRow( 1 );
	Point3 r2 = src.GetRow( 2 );

	dst.Set( r0.x, r0.y, r0.z,
			 r1.x, r1.y, r1.z,
			 r2.x, r2.y, r2.z );
}

void Exporter::nodeTransform( Niflib::Matrix33& rot, Niflib::Vector3& trans, INode* node, TimeValue t, bool local )
{
	Matrix3 tm = getNodeTransform( node, t, local );
	convertMatrix( rot, tm );
	trans.Set( tm.GetTrans( ).x, tm.GetTrans( ).y, tm.GetTrans( ).z );
}

void Exporter::nodeTransform( Niflib::QuaternionXYZW& rot, Niflib::Vector3& trans, INode* node, TimeValue t, bool local )
{
	Niflib::Matrix33 rm;
	nodeTransform( rm, trans, node, t, local );

	Niflib::Quaternion q = rm.AsQuaternion( );
	rot.x = q.x;
	rot.y = q.y;
	rot.z = q.z;
	rot.w = q.w;
}

Matrix3 Exporter::getNodeTransform( INode* node, TimeValue t, bool local )
{
	Matrix3 tm = node->GetNodeTM( t );
	if( local )
	{
		INode* parent = node->GetParentNode( );
		if( parent != NULL )
		{
			Matrix3 pm = parent->GetNodeTM( t );
			pm.Invert( );
			tm *= pm;
		}
	}

	return tm;
}

Niflib::NiNodeRef Exporter::makeNode( Niflib::NiNodeRef& parent, INode* maxNode, bool local )
{
	std::wstring name = (wchar_t*) maxNode->GetName( );
	Niflib::NiNodeRef node = getNode( maxNode );

	Niflib::Matrix33 rot;
	Niflib::Vector3 trans;
	TimeValue t = 0;
	nodeTransform( rot, trans, maxNode, t, local );

	node->SetLocalRotation( rot );
	node->SetLocalTranslation( trans );

	exportUPB( node, maxNode );

	// Normal Embedded Animation
	if( mExportType == NIF_WO_KF )
		CreateController( maxNode, Interval( ) );

	parent->AddChild( Niflib::DynamicCast<Niflib::NiAVObject>( node ) );

	return node;
}

Niflib::NiNodeRef Exporter::makeSwitchNode( Niflib::NiNodeRef& parent, INode* maxNode, bool local )
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

	std::wstring name = (wchar_t*) maxNode->GetName( );
	Niflib::NiSwitchNodeRef node = new Niflib::NiSwitchNode( );

	if( IParamBlock2* pblock2 = maxNode->GetObjectRef( )->GetParamBlockByID( switchnode_params ) )
	{
		int activeIndex = pblock2->GetInt( PB_ACTIVEBRANCH );
		int numBranches = pblock2->GetInt( PB_NUMBRANCHES );
		Niflib::SwitchNodeFlags flags = (Niflib::SwitchNodeFlags) 2;
		BOOL bUpdateActive = FALSE;

		pblock2->GetValue( PB_UPDATEACTIVE, 0, bUpdateActive, FOREVER );
		if( bUpdateActive )
			flags = (Niflib::SwitchNodeFlags) 3;

		node->SetName( narrow( name ) );
		node->SetFlags( 2062 );
		node->SetSwitchFlags( flags );
		node->SetActiveIndex( activeIndex );

		Niflib::Matrix33 rot;
		Niflib::Vector3 trans;
		TimeValue t = 0;

		nodeTransform( rot, trans, maxNode, t, local );

		node->SetLocalRotation( rot );
		node->SetLocalTranslation( trans );

		exportUPB( Niflib::DynamicCast<Niflib::NiNode>( node ), maxNode );

		// Normal Embedded Animation
		if( mExportType == NIF_WO_KF )
			CreateController( maxNode, Interval( ) );

		parent->AddChild( Niflib::DynamicCast<Niflib::NiAVObject>( node ) );

		// HACK: Ensure all SwitchNode children get exported
		bool oldExportHidden = mExportHidden;
		mExportHidden = true;

		Result result = Ok;
		for( int i = 0; i < numBranches; i++ )
		{
			Niflib::NiNodeRef childNode = new Niflib::NiNode( );
			childNode->SetFlags( 14 );

			pblock2->SetValue( PB_BRANCHIDX, 0, i );
			int numChildren = pblock2->GetInt( PB_NUMCHILDREN );

			INode* curChild;
			for( int j = 0; j < numChildren; j++ )
			{
				pblock2->SetValue( PB_CHILDIDX, 0, j );
				curChild = pblock2->GetINode( PB_GETCHILD );

				exportNodes( childNode, curChild );
			}

			node->AddChild( Niflib::DynamicCast<Niflib::NiAVObject>( childNode ) );
		}

		// END OF HACK
		mExportHidden = oldExportHidden;

		return Niflib::DynamicCast<Niflib::NiNode>( node );
	}

	return NULL;
}

Niflib::NiNodeRef Exporter::findNode( INode* maxNode )
{
	NodeToNodeMap::iterator itr = mNodeMap.find( maxNode );
	if( itr != mNodeMap.end( ) )
		return (*itr).second;

	return Niflib::NiNodeRef( );
}

Niflib::NiNodeRef Exporter::getNode( const wstring& name )
{
	NodeMap::iterator itr = mNameMap.find( name );
	if( itr != mNameMap.end( ) )
		return (*itr).second;

	Niflib::NiNodeRef node = CreateNiObject<Niflib::NiNode>( );
	if( IsFallout3( ) || IsSkyrim( ) )
		node->SetFlags( 14 );

	node->SetName( narrow( name ) );
	mNameMap[ name ] = node;

	return node;
}

Niflib::NiNodeRef Exporter::getNode( INode* maxNode )
{
	std::wstring name = maxNode->GetName( );

	NodeToNodeMap::iterator itr = mNodeMap.find( maxNode );
	if( itr != mNodeMap.end( ) )
		return (*itr).second;

	Niflib::NiNodeRef node = getNode( name );
	mNodeMap[ maxNode ] = node;

	return node;
}

// special case of a single branch being exported

bool Exporter::exportPrn( Niflib::NiNodeRef& obj, INode* node )
{
	// Export Prn Text strings for any parent bones if parent is root
	if( mSupportPrnStrings && Exporter::mNifVersionInt >= Niflib::VER_10_0_1_0 )
	{
		if( INode* parentNode = node->GetParentNode( ) )
		{
			std::wstring parentName = parentNode->GetName( );
			
			Niflib::NiStringExtraDataRef strings = new Niflib::NiStringExtraData( );
			strings->SetName( "Prn" );
			strings->SetData( narrow( parentName ) );

			obj->AddExtraData( Niflib::DynamicCast<Niflib::NiExtraData>( strings ) );

			return true;
		}
	}

	return false;
}

int Exporter::countNodes( INode* node )
{
	int counter = 1;
	for( int i = 0; i < node->NumberOfChildren( ); i++ )
		counter += countNodes( node->GetChildNode( i ) );

	return counter;
}

bool Exporter::isSkeletonRoot( INode* node )
{
	if( wildmatch( _T( "Bip??" ), node->GetName( ) ) )
		return true;

	else if( node->GetParentNode( ) == mI->GetRootNode( ) )
		return true;

	return false;
}

void Exporter::ApplyAllSkinOffsets( Niflib::NiAVObjectRef& root )
{
	Niflib::NiGeometryRef niGeom = Niflib::DynamicCast<Niflib::NiGeometry>( root );
	if( niGeom != NULL && niGeom->IsSkin( ) == true )
		niGeom->ApplySkinOffset( );

	Niflib::NiNodeRef niNode = Niflib::DynamicCast<Niflib::NiNode>( root );
	if( niNode != NULL )
	{
		// Call this function on all children
		std::vector<Niflib::NiAVObjectRef> children = niNode->GetChildren( );

		for( unsigned i = 0; i < children.size( ); ++i )
			ApplyAllSkinOffsets( children[ i ] );
	}
}

bool Exporter::isCollisionGroup( INode* maxNode, bool root )
{
	if( root )
	{
		if( !maxNode->IsGroupHead( ) )
			return false;
	}
	else
	{
		if( isCollision( maxNode ) )
			return true;
	}

	for( int i = 0; i < maxNode->NumberOfChildren( ); i++ )
	{
		if( isCollisionGroup( maxNode->GetChildNode( i ), false ) )
			return true;
	}

	return false;
}

void Exporter::getChildNodes( INode* node, std::vector<Niflib::NiNodeRef>& list )
{
	for( int i = 0; i < node->NumberOfChildren( ); i++ )
	{
		INode* child = node->GetChildNode( i );
		ObjectState os = node->EvalWorldState( 0 );
		bool addBone = false;
		bool local = !mFlattenHierarchy;
		bool meshGroup = isMeshGroup( node );

		TSTR nodeName = node->GetName( );
		if( wildmatch( _T( "Bip?? Footsteps" ), nodeName ) )
			addBone = false;

		else if( node->IsBoneShowing( ) )
			addBone = true;

		else if( os.obj && os.obj->SuperClassID( ) == GEOMOBJECT_CLASS_ID )
		{
			Class_ID clsid = os.obj->ClassID( );
			if( os.obj && ( clsid == BONE_OBJ_CLASSID
				|| clsid == Class_ID( BONE_CLASS_ID, 0 )
				|| clsid == Class_ID( 0x00009125, 0 ) /* Biped Twist Helpers */
				))
			{
				// skip mesh groups in skeleton only situations
				if( !meshGroup || !local || !mSkeletonOnly )
					addBone = true;
			}
			else if( !mSkeletonOnly )
			{
				if( mExportType != NIF_WO_ANIM && isNodeTracked( node ) )
					addBone = true;

				else if( mExportExtraNodes || ( mExportType != NIF_WO_ANIM && isNodeKeyed( node ) ) )
					addBone = true;
			}
			else if( mExportCameras && os.obj && os.obj->SuperClassID( ) == CAMERA_CLASS_ID )
				addBone = true;

			else if( meshGroup && local && !mSkeletonOnly ) // only create node if local
				addBone = true;
		}

		if( addBone )
			list.push_back( getNode( child ) );

		getChildNodes( child, list );
	}
}

bool Exporter::isMeshGroup( INode* maxNode, bool root )
{
	if( root )
	{
		if( !maxNode->IsGroupHead( ) )
			return false;
	}
	else
	{
		if( !isCollision( maxNode ) )
		{
			TimeValue t = 0;
			ObjectState os = maxNode->EvalWorldState( t );
			
			if( os.obj->SuperClassID( ) == GEOMOBJECT_CLASS_ID )
				return true;
		}
	}

	for( int i = 0; i < maxNode->NumberOfChildren( ); i++ )
		if( isMeshGroup( maxNode->GetChildNode( i ), false ) )
			return true;

	return false;
}

Exporter::Result Exporter::exportLight( Niflib::NiNodeRef root, INode* node, GenLight* light )
{
	// TODO: ...
	return Ok;
}

Matrix3 Exporter::getObjectTransform( INode* node, TimeValue t, bool local )
{
	Matrix3 tm = node->GetObjTMAfterWSM( t );
	if( local )
	{
		INode* parent = node->GetParentNode( );
		if( parent != NULL )
		{
			Matrix3 pm = parent->GetNodeTM( t );
			pm.Invert( );
			tm *= pm;
		}
	}

	return tm;
}

void Exporter::objectTransform( Niflib::Matrix33& rot, Niflib::Vector3& trans, INode* node, TimeValue t, bool local )
{
	Matrix3 tm = getObjectTransform( node, t, local );
	convertMatrix( rot, tm );
	trans.Set( tm.GetTrans( ).x, tm.GetTrans( ).y, tm.GetTrans( ).z );
}

void Exporter::objectTransform( Niflib::QuaternionXYZW& rot, Niflib::Vector3& trans, INode* node, TimeValue t, bool local )
{
	Niflib::Matrix33 rm;
	objectTransform( rm, trans, node, t, local );

	Niflib::Quaternion q = rm.AsQuaternion( );
	rot.x = q.x;
	rot.y = q.y;
	rot.z = q.z;
	rot.w = q.w;
}

bool Exporter::TMNegParity( const Matrix3& m )
{
	return ( DotProd( CrossProd( m.GetRow( 0 ), m.GetRow( 1 ) ), m.GetRow( 2 ) ) < 0.0f ) ? true : false;
}

Point3 Exporter::getVertexNormal( Mesh* mesh, int faceNo, RVertex* rv )
{
	Face* f = &mesh->faces[ faceNo ];
	DWORD smGroup = f->smGroup;
	int numNormals;
	Point3 vertexNormal;

	// Is normal specified
	// SPECIFIED is not currently used, but may be used in future versions.
	if( rv->rFlags & SPECIFIED_NORMAL )
		vertexNormal = rv->rn.getNormal( );

	else
	{
		// If normal is not specified it's only available if the face belongs
		// to a smoothing group
		if( ( numNormals = rv->rFlags & NORCT_MASK ) && smGroup )
		{
			// If there is only one vertex is found in the rn member
			if( numNormals == 1 )
				vertexNormal = rv->rn.getNormal( );

			else
			{
				// If two or more vertices are there you need to step through them
				// and find the vertex with the same smoothing group as the current face.
				// You will find multiple normals in the ern member.
				for( int i = 0; i < numNormals; i++ )
				{
					if( rv->ern[ i ].getSmGroup( ) & smGroup )
						vertexNormal = rv->ern[ i ].getNormal( );
				}
			}
		}
		else
		{
			// Get the normal from the Face if no smoothing groups are there
			vertexNormal = mesh->getFaceNormal( faceNo );
		}
	}

	return vertexNormal;
}

struct SortVectorEquivalence
{
	inline bool operator()( const Niflib::Vector3& lhs, const Niflib::Vector3& rhs ) const
	{
		float diff = lhs.x - rhs.x;
		if (diff < 0)
			return true;
		if (diff == 0)
		{
			diff = lhs.y - rhs.y;
			if (diff < 0)
				return true;
			if (diff == 0)
			{
				diff = lhs.z - rhs.z;
				if (diff < 0)
					return true;
			}
		}
		return false;
	}

	inline bool operator()( const Niflib::Float4& lhs, const Niflib::Float4& rhs ) const
	{
		float diff = lhs[0] - rhs[0];
		if (diff < 0)
			return true;
		if (diff == 0)
		{
			diff = lhs[1] - rhs[1];
			if (diff < 0)
				return true;
			if (diff == 0)
			{
				diff = lhs[2] - rhs[2];
				if (diff < 0)
					return true;
				if (diff == 0)
				{
					diff = lhs[3] - rhs[3];
					if (diff < 0)
						return true;
				}
			}
		}
		return false;
	}
};

void Exporter::sortVector3( std::vector<Niflib::Vector3>& vector )
{
	std::stable_sort( vector.begin( ), vector.end( ), SortVectorEquivalence( ) );
}

void Exporter::sortVector4( std::vector<Niflib::Vector4>& vector )
{
	std::stable_sort( vector.begin( ), vector.end( ), SortVectorEquivalence( ) );
}

bool Exporter::equal( const Niflib::Vector3& a, const Point3& b, float thresh )
{
	return	( fabsf( a.x - b.x ) <= thresh ) &&
			( fabsf( a.y - b.y ) <= thresh ) &&
			( fabsf( a.z - b.z ) <= thresh );
}