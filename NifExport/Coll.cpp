
#include "stdafx.h"

static Class_ID SCUBA_CLASS_ID(0x6d3d77ac, 0x79c939a9);
extern Class_ID bhkBoxObject_CLASS_ID;
extern Class_ID bhkSphereObject_CLASS_ID;
extern Class_ID BHKCAPSULEOBJECT_CLASS_ID;
extern Class_ID BHKLISTOBJECT_CLASS_ID;
extern Class_ID BHKPROXYOBJECT_CLASS_ID;

enum
{
	CAPSULE_RADIUS = 0,
	CAPSULE_HEIGHT = 1,
};

class HavokMoppCode
{
private:
	typedef int (__stdcall * fnGenerateMoppCode)( int nVerts, Niflib::Vector3 const* verts, int nTris, Niflib::Triangle const *tris );
	typedef int (__stdcall * fnGenerateMoppCodeWithSubshapes)( int nShapes, int const *shapes, int nVerts, Niflib::Vector3 const* verts, int nTris, Niflib::Triangle const *tris );
	typedef int (__stdcall * fnRetrieveMoppCode)( int nBuffer, unsigned char *buffer );
	typedef int (__stdcall * fnRetrieveMoppScale)( float *value );
	typedef int (__stdcall * fnRetrieveMoppOrigin)( Niflib::Vector3 *value );

	HMODULE hMoppLib;
	fnGenerateMoppCode   GenerateMoppCode;
	fnRetrieveMoppCode   RetrieveMoppCode;
	fnRetrieveMoppScale  RetrieveMoppScale;
	fnRetrieveMoppOrigin RetrieveMoppOrigin;
	fnGenerateMoppCodeWithSubshapes GenerateMoppCodeWithSubshapes;

public:
	HavokMoppCode( )
		: hMoppLib( 0 ), GenerateMoppCode( 0 ), RetrieveMoppCode( 0 ),
		RetrieveMoppScale( 0 ), RetrieveMoppOrigin( 0 ), GenerateMoppCodeWithSubshapes( 0 )
	{
	}

	~HavokMoppCode( )
	{
		if( hMoppLib )
			FreeLibrary( hMoppLib );
	}

	bool Initialize( )
	{
		if( hMoppLib == NULL )
		{
			wchar_t curfile[ _MAX_PATH ];
			GetModuleFileName( hInstance, curfile, MAX_PATH );
			PathRemoveFileSpec( curfile );
			PathAppend( curfile, _T( "NifMopp.dll" ) );

			hMoppLib = LoadLibrary( curfile );
			if( hMoppLib == NULL )
				hMoppLib = LoadLibrary( _T( "NifMopp.lib" ) );

			GenerateMoppCode = (fnGenerateMoppCode) GetProcAddress( hMoppLib, "GenerateMoppCode" );
			GenerateMoppCodeWithSubshapes = (fnGenerateMoppCodeWithSubshapes) GetProcAddress( hMoppLib, "GenerateMoppCodeWithSubshapes" );
			RetrieveMoppCode = (fnRetrieveMoppCode) GetProcAddress( hMoppLib, "RetrieveMoppCode" );
			RetrieveMoppScale = (fnRetrieveMoppScale) GetProcAddress( hMoppLib, "RetrieveMoppScale" );
			RetrieveMoppOrigin = (fnRetrieveMoppOrigin) GetProcAddress( hMoppLib, "RetrieveMoppOrigin" );
		}

		return ( NULL != GenerateMoppCode && NULL != RetrieveMoppCode && NULL != RetrieveMoppScale && NULL != RetrieveMoppOrigin );
	}

	std::vector<Niflib::byte> CalculateMoppCode( std::vector<Niflib::Vector3> const& verts, std::vector<Niflib::Triangle> const& tris, Niflib::Vector3* origin, float* scale )
	{
		std::vector<Niflib::byte> code;
		if( Initialize( ) )
		{
			int len = GenerateMoppCode( verts.size( ), &verts[ 0 ], tris.size( ), &tris[ 0 ] );
			if( len > 0 )
			{
				code.resize( len );
				if( 0 != RetrieveMoppCode( len, &code[ 0 ] ) )
				{
					if( NULL != scale )
						RetrieveMoppScale( scale );

					if( NULL != origin )
						RetrieveMoppOrigin( origin );
				}
				else
					code.clear( );
			}
		}

		return code;
	}

	std::vector<Niflib::byte> CalculateMoppCode( std::vector<Niflib::OblivionSubShape> const& shapes, std::vector<Niflib::Vector3> const& verts, std::vector<Niflib::Triangle> const& tris, Niflib::Vector3* origin, float* scale )
	{
		std::vector<Niflib::byte> code;
		if( Initialize( ) )
		{
			std::vector<int> subshapeverts;
			for( std::vector<Niflib::OblivionSubShape>::const_iterator itr = shapes.begin( ); itr != shapes.end( ); ++itr )
				subshapeverts.push_back( itr->numVertices );

			int len = 0;
			if( GenerateMoppCodeWithSubshapes != NULL )
				len = GenerateMoppCodeWithSubshapes( subshapeverts.size( ), &subshapeverts[ 0 ], verts.size( ), &verts[ 0 ], tris.size( ), &tris[ 0 ] );

			else
				len = GenerateMoppCode( verts.size( ), &verts[ 0 ], tris.size( ), &tris[ 0 ] );

			if( len > 0 )
			{
				code.resize( len );
				if( 0 != RetrieveMoppCode( len, &code[ 0 ] ) )
				{
					if( NULL != scale )
						RetrieveMoppScale( scale );

					if( NULL != origin )
						RetrieveMoppOrigin( origin );
				}
				else
					code.clear( );
			}
		}

		return code;
	}

} TheHavokCode;

static void AccumulateNodesFromGroup( INode* node, INodeTab& map )
{
	map.Append( 1, &node );
	if( node->IsGroupHead( ) )
	{
		for( int i = 0; i < node->NumberOfChildren( ); i++ )
			AccumulateNodesFromGroup( node->GetChildNode( i ), map );
	}
}

static std::vector<Niflib::byte> ConstructHKMesh( Niflib::NiTriBasedGeomRef shape, Niflib::Vector3& origin, float& scale )
{
	Niflib::NiTriBasedGeomDataRef data = shape->GetData( );
	return TheHavokCode.CalculateMoppCode( data->GetVertices( ), data->GetTriangles( ), &origin, &scale );
}

static std::vector<Niflib::byte> ConstructHKMesh( Niflib::bhkPackedNiTriStripsShapeRef shape, Niflib::Vector3& origin, float& scale )
{
	Niflib::hkPackedNiTriStripsDataRef data = shape->GetData( );
	return TheHavokCode.CalculateMoppCode( shape->GetSubShapes( ), data->GetVertices( ), data->GetTriangles( ), &origin, &scale );
}

static std::vector<Niflib::byte> ConstructHKMesh( std::vector<Niflib::Vector3> verts, std::vector<Niflib::Triangle> tris, Niflib::Vector3& origin, float& scale )
{
	return TheHavokCode.CalculateMoppCode( verts, tris, &origin, &scale );
}

bool Exporter::markAsHandled( INode* node )
{
	mHandledNodes.insert( node );
	return true;
}

Exporter::Result Exporter::scanForCollision( INode* node )
{
	if( node == NULL || ( node->IsHidden( ) && !mExportHidden ) )
		return Exporter::Skip;

	// Get the bhk RigidBody modifier if available and then get the picked node.

	TSTR nodeName = node->GetName( );
	
	if( Modifier* mod = GetbhkCollisionModifier( node ) )
	{
		if( IParamBlock2* pblock = (IParamBlock2*) mod->GetReference( 0 ) )
		{
			if( INode* collMesh = pblock->GetINode( 0, 0 ) )
				mCollisionNodes.insert( collMesh );
		}
	}

	// Check self to see if is one of our bhkXXXObject classes
	if( Object* obj = node->GetObjectRef( ) )
	{
		if( obj->ClassID( ) == BHKLISTOBJECT_CLASS_ID )
		{
			mCollisionNodes.insert( node );

			// process all children of groups as collision
			INodeTab map;
			const int PB_MESHLIST = 1;
			IParamBlock2* pblock2 = obj->GetParamBlockByID( 0 );
			int nBlocks = pblock2->Count( PB_MESHLIST );

			for( int i = 0; i < nBlocks; i++ )
			{
				INode* tnode = NULL;
				pblock2->GetValue( PB_MESHLIST, 0, tnode, FOREVER, i );

				if( tnode != NULL && ( !tnode->IsHidden( ) || mExportHidden ) )
					AccumulateNodesFromGroup( tnode, map );
			}

			for( int i = 0; i < map.Count( ); i++ )
			{
				INode* cnode = map[ i ];
				if( !node->IsGroupHead( ) )
					mCollisionNodes.insert( cnode );

				markAsHandled( cnode ); // dont process collision since the list will
			}
		}

		else if( obj->SuperClassID( ) == HELPER_CLASS_ID && obj->ClassID( ).PartB( ) == BHKRIGIDBODYCLASS_DESC.PartB( ) )
			mCollisionNodes.insert( node );

		else
		{
			Modifier* mod = GetbhkCollisionModifier( node );
			if( mod != NULL )
				mCollisionNodes.insert( node );
		}
	}

	// process legacy collision
	if( npIsCollision( node ) )
		mCollisionNodes.insert( node );

	for( int i = 0; i < node->NumberOfChildren( ); i++ )
		scanForCollision( node->GetChildNode( i ) );

	return Exporter::Ok;
}

Exporter::Result Exporter::exportCollision( Niflib::NiNodeRef& parent, INode* node )
{
	float bhkScale = ( IsSkyrim( ) ) ? Exporter::bhkScaleFactor * 10.0f : Exporter::bhkScaleFactor;

	if( isHandled( node ) || ( node->IsHidden( ) && !mExportHidden ) )
		return Exporter::Skip;

	ProgressUpdate( Collision, FormatText( _T( "'%s' Collision" ), node->GetName( ) ) );

	// marked as collision?
	bool coll = isCollision( node );

	bool local = !mFlattenHierarchy;
	Niflib::NiNodeRef nodeParent = mFlattenHierarchy ? mNiRoot : parent;

	Niflib::NiNodeRef newParent;
	if( coll )
	{
		markAsHandled( node );

		newParent = Exporter::findNode( node->GetParentNode( ) );
		if( !newParent )
			newParent = nodeParent;

		TimeValue t = 0;

		Niflib::bhkRigidBodyRef body = makeCollisionBody( node );
		if( body )
		{
			bool hasTrans = ( body->IsDerivedType( Niflib::bhkRigidBodyT::TYPE ) );
			Matrix3 tm = getNodeTransform( node, t, false );
			Matrix3 pm = TOMATRIX3( newParent->GetWorldTransform( ) );
			tm = tm * Inverse( pm );

			Niflib::Matrix44 rm4 = TOMATRIX4( tm, false );
			Niflib::Vector3 trans;
			Niflib::Matrix33 rm;
			float scale;

			rm4.Decompose( trans, rm, scale );
			Niflib::QuaternionXYZW q = TOQUATXYZW( rm.AsQuaternion( ) );

			body->SetRotation( q );
			body->SetTranslation( trans / bhkScale );

			if( hasTrans )
			{
				tm = getNodeTransform( node, t, false );
				tm.NoScale( );
				tm.Invert( );
			}
			else
			{
				tm = TOMATRIX3( newParent->GetWorldTransform( ) );
				tm.NoScale( );
				tm.Invert( );
			}

			Niflib::bhkShapeRef shape = makeCollisionShape( node, tm, body, Niflib::HavokMaterial( NP_DEFAULT_HVK_MATERIAL ) );
			if( shape )
			{
				body->SetShape( Niflib::DynamicCast<Niflib::bhkShape>( shape ) );

				Niflib::bhkCollisionObjectRef co = new Niflib::bhkCollisionObject( );
				co->SetBody( Niflib::DynamicCast<Niflib::NiObject>( body ) );

				if( IsSkyrim( ) )
					co->SetFlags( 129 );

				if( Niflib::bhkMoppBvTreeShapeRef moppShape = Niflib::DynamicCast<Niflib::bhkMoppBvTreeShape>( shape ) )
				{
					if( Niflib::bhkCompressedMeshShapeRef compressedShape = Niflib::DynamicCast<Niflib::bhkCompressedMeshShape>( moppShape->GetShape( ) ) )
						compressedShape->SetTarget( newParent );
				}

				newParent->SetCollisionObject( Niflib::DynamicCast<Niflib::NiCollisionObject>( co ) );

				if( body->GetMass( ) != 0.0f )
					body->UpdateMassProperties( 1.0f, true, body->GetMass( ) );
			}
		}
	}
	else if( isCollisionGroup( node ) && !mFlattenHierarchy )
		newParent = makeNode( nodeParent, node );

	else
	{
		newParent = Exporter::findNode( node->GetParentNode( ) );
		if( !newParent )
			newParent = nodeParent;
	}

	if( !newParent )
		newParent = nodeParent;

	for( int i = 0; i < node->NumberOfChildren( ); i++ )
	{
		Result result = exportCollision( newParent, node->GetChildNode( i ) );
		if( result != Ok && result != Skip )
			return result;
	}

	return Ok;
}

bool Exporter::isCollision( INode* node )
{
	return( mCollisionNodes.find( node ) != mCollisionNodes.end( ) );
}

bool Exporter::isHandled( INode* node )
{
	return( mHandledNodes.find( node ) != mHandledNodes.end( ) );
}

int Exporter::addVertex( std::vector<Niflib::Vector3>& verts, std::vector<Niflib::Vector3>& vnorms, const Point3& pt, const Point3& norm )
{
	float bhkScale = ( IsSkyrim( ) ) ? Exporter::bhkScaleFactor * 10.0f : 1.0f;
	
	for( unsigned int i = 0; i < verts.size( ); i++ )
	{
		if( equal( verts[ i ], pt, mWeldThresh ) && equal( vnorms[ i ], norm, 0 ) )
			return i;
	}

	verts.push_back( Niflib::Vector3( pt.x, pt.y, pt.z ) / bhkScale );
	vnorms.push_back( Niflib::Vector3( norm.x, norm.y, norm.z ) );

	return verts.size( ) - 1;
}

void Exporter::addFace( Triangles& tris, std::vector<Niflib::Vector3>& verts, std::vector<Niflib::Vector3>& vnorms, int face, const int vi[ 3 ], Mesh* mesh, Matrix3& tm )
{
	Niflib::Triangle tri;
	for( int i = 0; i < 3; i++ )
	{
		Point3 pt = mesh->verts[ mesh->faces[ face ].v[ vi[ i ] ] ] * tm;
		Point3 norm = getVertexNormal( mesh, face, mesh->getRVertPtr( mesh->faces[ face ].v[ vi[ i ] ] ) ) * tm;

		tri[ i ] = addVertex( verts, vnorms, pt, norm );
	}

	tris.push_back( tri );
}

static void AccumulateSubShapesFromGroup( INode* node, INodeTab& packedShapes, INodeTab& otherShapes )
{
	ObjectState os = node->EvalWorldState( 0 );
	if( node->IsGroupHead( ) )
	{
		for( int i = 0; i < node->NumberOfChildren( ); i++ )
			AccumulateSubShapesFromGroup( node->GetChildNode( i ), packedShapes, otherShapes );
	}
	else if( ( os.obj->ClassID( ) == SCUBA_CLASS_ID )
		|| ( os.obj->ClassID( ) == Class_ID( BOXOBJ_CLASS_ID, 0 ) )
		|| ( os.obj->ClassID( ) == Class_ID( SPHERE_CLASS_ID, 0 ) )
		|| ( os.obj->ClassID( ) == bhkBoxObject_CLASS_ID )
		|| ( os.obj->ClassID( ) == bhkSphereObject_CLASS_ID )
		|| ( os.obj->ClassID( ) == BHKCAPSULEOBJECT_CLASS_ID )
		|| ( os.obj->ClassID( ) == BHKLISTOBJECT_CLASS_ID )
		)
	{
		otherShapes.Append( 1, &node );
	}
	else if( os.obj->ClassID( ) == BHKPROXYOBJECT_CLASS_ID )
	{
		enum { list_params, bv_mesh, };  // pblock2 ID
		enum { PB_MATERIAL, PB_MESHLIST, PB_BOUND_TYPE, PB_CENTER, };
		enum { bv_type_none, bv_type_box, bv_type_shapes, bv_type_packed, bv_type_convex, };  // pblock ID

		int type = bv_type_none;
		if( IParamBlock2* pblock2 = os.obj->GetParamBlockByID( list_params ) )
			pblock2->GetValue( PB_BOUND_TYPE, 0, type, FOREVER, 0 );

		if( type == bv_type_packed )
			packedShapes.Insert( 0, 1, &node );

		else
			otherShapes.Insert( 0, 1, &node );
	}
	else if( os.obj->SuperClassID( ) == GEOMOBJECT_CLASS_ID )
	{
		if( Modifier* mod = GetbhkCollisionModifier( node ) )
		{
			enum { havok_params };
			enum { PB_BOUND_TYPE, PB_MATERIAL, };
			enum { bv_type_none, bv_type_box, bv_type_sphere, bv_type_capsule, bv_type_shapes, bv_type_convex, bv_type_packed, bv_type_obb, };  // pblock ID

			int type = bv_type_none;
			if( IParamBlock2* pblock2 = mod->GetParamBlockByID( havok_params ) )
				pblock2->GetValue( PB_BOUND_TYPE, 0, type, FOREVER, 0 );

			if( type == bv_type_packed )
				packedShapes.Insert( 0, 1, &node );

			else
				otherShapes.Insert( 0, 1, &node );
		}
		else
			packedShapes.Insert( 0, 1, &node );
	}
}

Niflib::bhkRigidBodyRef Exporter::makeCollisionBody( INode* node )
{
	// get data from node
	int lyr = NP_DEFAULT_HVK_LAYER;
	int mtl = NP_DEFAULT_HVK_MATERIAL;
	int msys = NP_DEFAULT_HVK_MOTION_SYSTEM;
	int qtype = NP_DEFAULT_HVK_QUALITY_TYPE;
	float mass = NP_DEFAULT_HVK_MASS;
	float lindamp = NP_DEFAULT_HVK_LINEAR_DAMPING;
	float angdamp = NP_DEFAULT_HVK_ANGULAR_DAMPING;
	float frict = NP_DEFAULT_HVK_FRICTION;
	float maxlinvel = NP_DEFAULT_HVK_MAX_LINEAR_VELOCITY;
	float maxangvel = NP_DEFAULT_HVK_MAX_ANGULAR_VELOCITY;
	float resti = NP_DEFAULT_HVK_RESTITUTION;
	float pendepth = NP_DEFAULT_HVK_PENETRATION_DEPTH;

	Niflib::Vector3 center( 0, 0, 0 );
	BOOL transenable = TRUE;

	if( bhkRigidBodyInterface* irb = (bhkRigidBodyInterface*) node->GetObjectRef( )->GetInterface( BHKRIGIDBODYINTERFACE_DESC ) )
	{
		mass = irb->GetMass( 0 );
		frict = irb->GetFriction( 0 );
		resti = irb->GetRestitution( 0 );
		lyr = irb->GetLayer( 0 );
		msys = irb->GetMotionSystem( 0 );
		qtype = irb->GetQualityType( 0 );
		lindamp = irb->GetLinearDamping( 0 );
		angdamp = irb->GetAngularDamping( 0 );
		maxlinvel = irb->GetMaxLinearVelocity( 0 );
		pendepth = irb->GetPenetrationDepth( 0 );
		maxangvel = irb->GetMaxAngularVelocity( 0 );
		transenable = irb->GetEnableTransform( 0 );
	}

	else if( npIsCollision( node ) )
	{
		// Handle compatibility
		npGetProp( node, NP_HVK_MASS_OLD, mass, NP_DEFAULT_HVK_EMPTY );
		if( mass == NP_DEFAULT_HVK_EMPTY )
			npGetProp( node, NP_HVK_MASS, mass, NP_DEFAULT_HVK_MASS );

		npGetProp( node, NP_HVK_FRICTION_OLD, frict, NP_DEFAULT_HVK_EMPTY );
		if( mass == NP_DEFAULT_HVK_EMPTY )
			npGetProp( node, NP_HVK_FRICTION, frict, NP_DEFAULT_HVK_FRICTION );

		npGetProp( node, NP_HVK_RESTITUTION_OLD, resti, NP_DEFAULT_HVK_EMPTY );
		if( mass == NP_DEFAULT_HVK_EMPTY )
			npGetProp( node, NP_HVK_RESTITUTION, resti, NP_DEFAULT_HVK_RESTITUTION );

		npGetProp( node, NP_HVK_LAYER, lyr, NP_DEFAULT_HVK_LAYER );
		npGetProp( node, NP_HVK_MATERIAL, mtl, NP_DEFAULT_HVK_MATERIAL );
		npGetProp( node, NP_HVK_MOTION_SYSTEM, msys, NP_DEFAULT_HVK_MOTION_SYSTEM );
		npGetProp( node, NP_HVK_QUALITY_TYPE, qtype, NP_DEFAULT_HVK_QUALITY_TYPE );
		npGetProp( node, NP_HVK_LINEAR_DAMPING, lindamp, NP_DEFAULT_HVK_LINEAR_DAMPING );
		npGetProp( node, NP_HVK_ANGULAR_DAMPING, angdamp, NP_DEFAULT_HVK_ANGULAR_DAMPING );
		npGetProp( node, NP_HVK_MAX_LINEAR_VELOCITY, maxlinvel, NP_DEFAULT_HVK_MAX_LINEAR_VELOCITY );
		npGetProp( node, NP_HVK_MAX_ANGULAR_VELOCITY, maxangvel, NP_DEFAULT_HVK_MAX_ANGULAR_VELOCITY );
		npGetProp( node, NP_HVK_PENETRATION_DEPTH, pendepth, NP_DEFAULT_HVK_PENETRATION_DEPTH );
		npGetProp( node, NP_HVK_CENTER, center );
	}

	else
	{
		// Check self to see if is one of our bhkXXXObject classes
		if( Object* obj = node->GetObjectRef( ) )
		{
			if( obj->SuperClassID( ) == HELPER_CLASS_ID && obj->ClassID( ).PartB( ) == BHKRIGIDBODYCLASS_DESC.PartB( ) )
			{
				// TODO: do standard body export
			}
		}

		// else check redirection
	}

	// setup body
	Niflib::bhkRigidBodyRef body = transenable ? new Niflib::bhkRigidBodyT( ) : new Niflib::bhkRigidBody( );

	body->SetLayer( Niflib::OblivionLayer( lyr ) );
	body->SetLayerCopy( Niflib::OblivionLayer( lyr ) );
	body->SetMotionSystem( Niflib::MotionSystem( msys ) );
	body->SetQualityType( Niflib::MotionQuality( qtype ) );
	body->SetMass( mass );
	body->SetLinearDamping( lindamp );
	body->SetAngularDamping( angdamp );
	body->SetFriction( frict );
	body->SetRestitution( resti );
	body->SetMaxLinearVelocity( maxlinvel );
	body->SetMaxAngularVelocity( maxangvel );
	body->SetPenetrationDepth( pendepth );
	body->SetCenter( center );

	if( IsSkyrim( ) )
	{
		body->SetMotionSystem( Niflib::MO_SYS_BOX_STABILIZED );
		body->SetDeactivatorType( Niflib::DEACTIVATOR_NEVER );
		body->SetSolverDeactivation( Niflib::SOLVER_DEACTIVATION_OFF );
		body->SetQualityType( Niflib::MO_QUAL_INVALID );

		Niflib::array<7, unsigned short> unknownValues;
		unknownValues[ 0 ] = 0;
		unknownValues[ 1 ] = 56896;
		unknownValues[ 2 ] = 1343;
		unknownValues[ 3 ] = 0;
		unknownValues[ 4 ] = 0;
		unknownValues[ 5 ] = 1;
		unknownValues[ 6 ] = 65535;

		body->SetUnknown7Shorts( unknownValues );
	}

	Niflib::QuaternionXYZW q;
	q.x = 0.0f;
	q.y = 0.0f;
	q.z = 0.0f;
	q.w = 1.0f;

	body->SetRotation( q );

	return body;
}

Niflib::bhkShapeRef	Exporter::makeCollisionShape( INode* node, Matrix3& tm, Niflib::bhkRigidBodyRef body, Niflib::HavokMaterial mtlDefault )
{
	Niflib::bhkShapeRef shape;
	TimeValue t = 0;
	ObjectState os = node->EvalWorldState( t );

	if( node->IsGroupHead( ) )
		shape = makeModPackedTriStripShape( node, tm, mtlDefault );

	else if( os.obj->ClassID( ) == SCUBA_CLASS_ID )
		shape = makeCapsuleShape( node, os.obj, tm, mtlDefault );

	else if( os.obj->ClassID( ) == Class_ID( BOXOBJ_CLASS_ID, 0 ) )
		shape = makeBoxShape( node, os.obj, tm, mtlDefault );

	else if( os.obj->ClassID( ) == Class_ID( SPHERE_CLASS_ID, 0 ) )
		shape = makeSphereShape( node, os.obj, tm, mtlDefault );

	else if( os.obj->ClassID( ) == bhkBoxObject_CLASS_ID )
		shape = makebhkBoxShape( node, os.obj, tm, mtlDefault );

	else if( os.obj->ClassID( ) == bhkSphereObject_CLASS_ID )
		shape = makebhkSphereShape( node, os.obj, tm, mtlDefault );

	else if( os.obj->ClassID( ) == BHKCAPSULEOBJECT_CLASS_ID )
		shape = makebhkCapsuleShape( node, os.obj, tm, mtlDefault );

	else if( os.obj->ClassID( ) == BHKLISTOBJECT_CLASS_ID )
		shape = makeListShape( node, tm, body, mtlDefault );

	else if( os.obj->ClassID( ) == BHKPROXYOBJECT_CLASS_ID )
		shape = makeProxyShape( node, os.obj, tm, mtlDefault );

	else if( os.obj->SuperClassID( ) == GEOMOBJECT_CLASS_ID )
	{
		if( Modifier* mod = GetbhkCollisionModifier( node ) )
			shape = makeModifierShape( node, os.obj, mod, tm, mtlDefault );

		else
			shape = makeTriStripsShape( node, tm, mtlDefault );
	}

	return shape;
}

Niflib::bhkShapeRef Exporter::makeTriStripsShape( INode* node, Matrix3& tm, Niflib::HavokMaterial mtlDefault )
{
	TimeValue t = 0;
	Matrix3 sm = ScaleMatrix( GetScale( tm ) );

	// Order of the vertices. Get 'em counter clockwise if the objects are
	// negatively scaled.
	ObjectState os = node->EvalWorldState( t );

	TriObject* tri = (TriObject*) os.obj->ConvertToType( t, Class_ID( TRIOBJ_CLASS_ID, 0 ) );
	if( !tri )
		return false;

	Mesh& mesh = tri->GetMesh( );
	mesh.buildNormals( );

	Niflib::bhkNiTriStripsShapeRef shape = makeTriStripsShape( mesh, sm, mtlDefault );

	int lyr = Niflib::OL_STATIC;
	npGetProp( node, NP_HVK_LAYER, lyr, NP_DEFAULT_HVK_LAYER );

	shape->SetNumDataLayers( 1 );
	shape->SetOblivionLayer( 0, Niflib::OblivionLayer( lyr ) );

	int mtl = NP_DEFAULT_HVK_MATERIAL;
	npGetProp( node, NP_HVK_MATERIAL, mtl, NP_DEFAULT_HVK_MATERIAL );

	shape->SetMaterial( Niflib::HavokMaterial( mtl ) );

	return Niflib::StaticCast<Niflib::bhkShape>( shape );
}

Niflib::bhkShapeRef Exporter::makeBoxShape( INode* node, Object* obj, Matrix3& tm, Niflib::HavokMaterial mtlDefault )
{
	float bhkScale = ( IsSkyrim( ) ) ? Exporter::bhkScaleFactor * 10.0f : Exporter::bhkScaleFactor;

	Point3 scale = GetScale( tm );
	float length = 0;
	float height = 0;
	float width = 0;

	int numblocks = obj->NumParamBlocks( );
	if( IParamBlock2* pblock2 = obj->GetParamBlockByID( 0 ) )
	{
		pblock2->GetValue( BOXOBJ_LENGTH, 0, length, FOREVER );
		pblock2->GetValue( BOXOBJ_HEIGHT, 0, height, FOREVER );
		pblock2->GetValue( BOXOBJ_WIDTH, 0, width, FOREVER );
	}

	Niflib::bhkBoxShapeRef box = new Niflib::bhkBoxShape( );
	Niflib::Vector3 dim( width * scale[ 0 ], length * scale[ 1 ], length * scale[ 2 ] );

	// Adjust translation for center of z axis in box
	tm.Translate( Point3( 0.0f, 0.0f, dim.z / 2.0f ) );

	dim /= ( bhkScale * 2.0f );
	box->SetDimensions( dim );

	int mtl = mtlDefault;
	npGetProp( node, NP_HVK_MATERIAL, mtl, mtlDefault );
	box->SetMaterial( Niflib::HavokMaterial( mtl ) );

	return Niflib::bhkShapeRef( Niflib::DynamicCast<Niflib::bhkSphereRepShape>( box ) );
}

Niflib::bhkShapeRef Exporter::makeSphereShape( INode* node, Object* obj, Matrix3& tm, Niflib::HavokMaterial mtlDefault )
{
	Point3 scale = GetScale( tm );
	float s = ( scale[ 0 ] + scale[ 1 ] + scale[ 2 ] ) / 3.0f;

	float radius = 0;
	if( IParamBlock2* pblock2 = obj->GetParamBlockByID( 0 ) )
		pblock2->GetValue( SPHERE_RADIUS, 0, radius, FOREVER );

	Niflib::bhkSphereShapeRef sphere = new Niflib::bhkSphereShape( );
	sphere->SetRadius( radius * s );

	int mtl = mtlDefault;
	npGetProp( node, NP_HVK_MATERIAL, mtl, mtlDefault );
	sphere->SetMaterial( Niflib::HavokMaterial( mtl ) );

	return Niflib::bhkShapeRef( Niflib::DynamicCast<Niflib::bhkSphereRepShape>( sphere ) );
}

Niflib::bhkShapeRef Exporter::makeCapsuleShape( INode* node, Object* obj, Matrix3& tm, Niflib::HavokMaterial mtlDefault )
{
	Point3 scale = GetScale( tm );
	float s = ( scale[ 0 ] + scale[ 1 ] + scale[ 2 ] ) / 3.0f;

	float radius = 0.1f;
	float height = 0.1f;

	if( IParamBlock2* params = obj->GetParamBlockByID( 0 ) )
	{
		params->GetValue( CAPSULE_RADIUS, 0, radius, FOREVER );
		params->GetValue( CAPSULE_HEIGHT, 0, height, FOREVER );
	}

	Niflib::bhkCapsuleShapeRef capsule = CreateNiObject<Niflib::bhkCapsuleShape>( );

	capsule->SetRadius( radius );
	capsule->SetRadius1( radius );
	capsule->SetRadius2( radius );

	int mtl = mtlDefault;
	npGetProp( node, NP_HVK_MATERIAL, mtl, mtlDefault );
	capsule->SetMaterial( Niflib::HavokMaterial( mtl ) );

	return Niflib::bhkShapeRef( Niflib::DynamicCast<Niflib::bhkSphereRepShape>( capsule ) );
}

Niflib::bhkShapeRef Exporter::makeListShape( INode* node, Matrix3& tm, Niflib::bhkRigidBodyRef body, Niflib::HavokMaterial mtlDefault )
{
	const int PB_MATERIAL = 0;
	const int PB_MESHLIST = 1;

	IParamBlock2* pblock2 = node->GetObjectRef( )->GetParamBlockByID( 0 );
	int nBlocks = pblock2->Count( PB_MESHLIST );

	if( nBlocks > 0 )
	{
		if( bhkRigidBodyInterface* irb = (bhkRigidBodyInterface*) node->GetObjectRef( )->GetInterface( BHKRIGIDBODYINTERFACE_DESC ) )
		{
			float mass = irb->GetMass( 0 );
			float frict = irb->GetFriction( 0 );
			float resti = irb->GetRestitution( 0 );
			int lyr = irb->GetLayer( 0 );
			int msys = irb->GetMotionSystem( 0 );
			int qtype = irb->GetQualityType( 0 );
			float lindamp = irb->GetLinearDamping( 0 );
			float angdamp = irb->GetAngularDamping( 0 );
			float maxlinvel = irb->GetMaxLinearVelocity( 0 );
			float maxangvel = irb->GetMaxAngularVelocity( 0 );
			float pendepth = irb->GetPenetrationDepth( 0 );

			body->SetLayer( Niflib::OblivionLayer( lyr ) );
			body->SetLayerCopy( Niflib::OblivionLayer( lyr ) );
			body->SetMotionSystem( Niflib::MotionSystem( msys ) );
			body->SetQualityType( Niflib::MotionQuality( qtype ) );
			body->SetMass( mass );
			body->SetLinearDamping( lindamp );
			body->SetAngularDamping( angdamp );
			body->SetFriction( frict );
			body->SetRestitution( resti );
			body->SetMaxLinearVelocity( maxlinvel );
			body->SetMaxAngularVelocity( maxangvel );
			body->SetPenetrationDepth( pendepth );

			if( IsSkyrim( ) )
			{
				Niflib::array<7, unsigned short> unknownValues;
				unknownValues[ 0 ] = 0;
				unknownValues[ 1 ] = 56896;
				unknownValues[ 2 ] = 1343;
				unknownValues[ 3 ] = 0;
				unknownValues[ 4 ] = 0;
				unknownValues[ 5 ] = 1;
				unknownValues[ 6 ] = 65535;

				body->SetUnknown7Shorts( unknownValues );
			}
		}

		// Accumulate potential bhkPackedTriStripShapes

		Niflib::bhkListShapeRef shape = new Niflib::bhkListShape( );
		Niflib::HavokMaterial mtl = Niflib::HavokMaterial( pblock2->GetInt( PB_MATERIAL, 0, 0 ) );

		if( mtl < 0 )
			mtl = mtlDefault;

		shape->SetMaterial( mtl );

		// Locate all packed shapes for efficient mopp packing
		INodeTab packedShapes;
		INodeTab otherShapes;

		for( int i = 0; i < nBlocks; i++ )
		{
			INode* tnode = NULL;
			pblock2->GetValue( PB_MESHLIST, 0, tnode, FOREVER, i );
			
			if( tnode != NULL )
				AccumulateSubShapesFromGroup( tnode, packedShapes, otherShapes );
		}

		std::vector<Niflib::bhkShapeRef> shapes;
		if( packedShapes.Count( ) > 0 )
		{
			if( Niflib::bhkShapeRef subshape = makeModPackedTriStripShape( packedShapes, tm, Niflib::HavokMaterial( mtl ) ) )
				shapes.push_back( subshape );
		}

		for( int i = 0; i < otherShapes.Count( ); i++ )
		{
			INode* tnode = otherShapes[ i ];
			if( tnode != NULL && ( !tnode->IsHidden( ) || mExportHidden ) )
			{
				Niflib::bhkShapeRef subshape = makeCollisionShape( tnode, tm, body, Niflib::HavokMaterial( mtl ) );
				if( subshape )
					shapes.push_back( subshape );
			}
		}

		shape->SetSubShapes( shapes );

		if( shapes.size( ) == 1 ) // ignore the list when only one object is present
			return shapes[ 0 ];

		else if( !shapes.empty( ) )
			return Niflib::bhkShapeRef( shape );
	}

	return Niflib::bhkShapeRef( );
}

Niflib::bhkShapeRef Exporter::makebhkBoxShape( INode* node, Object* obj, Matrix3& tm, Niflib::HavokMaterial mtlDefault )
{
	float bhkScale = ( IsSkyrim( ) ) ? 10.0f : 1.0f;

	enum { box_params, };
	enum { PB_MATERIAL, PB_LENGTH, PB_WIDTH, PB_HEIGHT, };

	Niflib::bhkShapeRef retval;
	if( IParamBlock2* pblock2 = obj->GetParamBlockByID( box_params ) )
	{
		Point3 scale = GetScale( tm );
		float s = ( scale[ 0 ] + scale[ 1 ] + scale[ 2 ] ) / 3.0f;

		Niflib::HavokMaterial mtl = mtlDefault;
		float length = 0;
		float width = 0;
		float height = 0;

		mtl = Niflib::HavokMaterial( pblock2->GetInt( PB_MATERIAL, 0, 0 ) );
		if( mtl < 0 )
			mtl = mtlDefault;

		pblock2->GetValue( PB_LENGTH, 0, length, FOREVER, 0 );
		pblock2->GetValue( PB_WIDTH, 0, width, FOREVER, 0 );
		pblock2->GetValue( PB_HEIGHT, 0, height, FOREVER, 0 );

		Niflib::bhkBoxShapeRef box = new Niflib::bhkBoxShape( );
		Niflib::Vector3 dim( width * scale[ 0 ], length * scale[ 1 ], height * scale[ 2 ] );
		dim /= bhkScale;

		float radius = max( max( dim.x, dim.y ), dim.z );
		box->SetRadius( radius );

		// Adjust translation for center of z axis in box
		tm.Translate( Point3( 0.0f, 0.0f, dim.z / 2.0f ) );

		box->SetDimensions( dim );
		box->SetMaterial( mtl );

		Matrix3 ltm = node->GetNodeTM( 0 ) * tm;
		if( ltm.IsIdentity( ) )
			retval = Niflib::StaticCast<Niflib::bhkShape>( box );

		else
		{
			ltm.SetTrans( ltm.GetTrans( ) / ( Exporter::bhkScaleFactor * bhkScale ) );

			Niflib::bhkTransformShapeRef transform = new Niflib::bhkTransformShape( );
			transform->SetTransform( TOMATRIX4( ltm ).Transpose( ) );
			transform->SetShape( box );
			transform->SetMaterial( mtl );

			retval = Niflib::StaticCast<Niflib::bhkShape>( transform );
		}
	}

	return retval;
}

Niflib::bhkShapeRef Exporter::makebhkSphereShape( INode* node, Object* obj, Matrix3& tm, Niflib::HavokMaterial mtlDefault )
{
	float bhkScale = ( IsSkyrim( ) ) ? 10.0f : 1.0f;

	Niflib::bhkShapeRef retval;

	enum { sphere_params, };
	enum { PB_MATERIAL, PB_RADIUS, PB_SEGS, PB_SMOOTH, };

	if( IParamBlock2* pblock2 = obj->GetParamBlockByID( sphere_params ) )
	{
		float radius = 0.0f;
		pblock2->GetValue( PB_RADIUS, 0, radius, FOREVER, 0 );

		Niflib::HavokMaterial mtl = mtlDefault;
		mtl = Niflib::HavokMaterial( pblock2->GetInt( PB_MATERIAL, 0, 0 ) );

		if( mtl < 0 )
			mtl = mtlDefault;

		Niflib::bhkSphereShapeRef shape = new Niflib::bhkSphereShape( );
		shape->SetRadius( radius / bhkScale );
		shape->SetMaterial( mtl );

		Matrix3 ltm = node->GetNodeTM( 0 ) * tm;
		if( ltm.IsIdentity( ) )
			retval = Niflib::StaticCast<Niflib::bhkShape>( shape );

		else
		{
			ltm.SetTrans( ltm.GetTrans( ) / ( bhkScale * Exporter::bhkScaleFactor ) );

			Niflib::bhkTransformShapeRef transform = new Niflib::bhkTransformShape( );
			transform->SetTransform( TOMATRIX4( ltm ).Transpose( ) );
			transform->SetShape( shape );
			transform->SetMaterial( mtl );

			retval = Niflib::StaticCast<Niflib::bhkShape>( transform );
		}
	}

	return retval;
}

Niflib::bhkShapeRef Exporter::makebhkCapsuleShape( INode* node, Object* obj, Matrix3& tm, Niflib::HavokMaterial mtlDefault )
{
	float bhkScale = ( IsSkyrim( ) ) ? 10.0f : 1.0f;

	Niflib::bhkShapeRef retval;

	enum { cap_params, };
	enum { PB_MATERIAL, PB_RADIUS1, PB_RADIUS2, PB_LENGTH, };

	if( IParamBlock2* pblock2 = obj->GetParamBlockByID( cap_params ) )
	{
		float radius1 = 0.0f;
		float radius2 = 0.0f;
		float len = 0.0f;
		Niflib::HavokMaterial mtl = mtlDefault;

		pblock2->GetValue( PB_RADIUS1, 0, radius1, FOREVER, 0 );
		pblock2->GetValue( PB_RADIUS2, 0, radius2, FOREVER, 0 );
		pblock2->GetValue( PB_LENGTH, 0, len, FOREVER, 0 );

		mtl = Niflib::HavokMaterial( pblock2->GetInt( PB_MATERIAL, 0, 0 ) );
		if( mtl < 0 )
			mtl = mtlDefault;

		Niflib::bhkCapsuleShapeRef shape = new Niflib::bhkCapsuleShape( );
		shape->SetRadius( ( ( radius1 + radius2 ) / 2.0f ) / bhkScale );
		shape->SetRadius1( radius1 / bhkScale );
		shape->SetRadius2( radius2 / bhkScale );
		shape->SetMaterial( mtl );

		Matrix3 ltm = node->GetNodeTM( 0 ) * tm;
		Point3 center = ltm.GetTrans( );

		Matrix3 rot = ltm;
		rot.NoTrans( );
		rot.NoScale( );

		float distFromCenter = len * Exporter::bhkScaleFactor / 2.0f;

		Point3 pt2 = ( ( TransMatrix( Point3( 0.0f, 0.0f, +distFromCenter ) ) * rot ).GetTrans( ) + center ) / Exporter::bhkScaleFactor;
		Point3 pt1 = ( ( TransMatrix( Point3( 0.0f, 0.0f, -distFromCenter ) ) * rot ).GetTrans( ) + center ) / Exporter::bhkScaleFactor;

		shape->SetFirstPoint( TOVECTOR3( pt1 / bhkScale ) );
		shape->SetSecondPoint( TOVECTOR3( pt2 / bhkScale ) );

		retval = Niflib::StaticCast<Niflib::bhkShape>( shape );
	}

	return retval;
}

static Niflib::bhkMoppBvTreeShapeRef makeTreeShape( Niflib::bhkPackedNiTriStripsShapeRef mesh, Niflib::HavokMaterial mtl )
{
	Niflib::bhkMoppBvTreeShapeRef mopp = new Niflib::bhkMoppBvTreeShape( );
	mopp->SetMaterial( mtl );
	mopp->SetShape( mesh );

	try
	{
		Niflib::Vector3 offset;
		float scale;

		std::vector<Niflib::byte> moppcode = ConstructHKMesh( mesh, offset, scale );
		mopp->SetMoppCode( moppcode );
		mopp->SetMoppOrigin( offset );
		mopp->SetMoppScale( scale );
	}
	catch( ... )
	{
	}

	return mopp;
}

static Niflib::bhkMoppBvTreeShapeRef makeTreeShape( Niflib::bhkCompressedMeshShapeRef mesh, Niflib::HavokMaterial mtl )
{
	/*Niflib::bhkMoppBvTreeShapeRef mopp = new Niflib::bhkMoppBvTreeShape( );
	mopp->SetMaterial( mtl );
	mopp->SetShape( mesh );

	try
	{
		Niflib::Vector3 offset;
		float scale;

		std::vector<Niflib::byte> moppcode = ConstructHKMesh( mesh, offset, scale );
		mopp->SetMoppCode( moppcode );
		mopp->SetMoppOrigin( offset );
		mopp->SetMoppScale( scale );
	}
	catch( ... )
	{
	}*/

	//return mopp;
}

Niflib::bhkConvexVerticesShapeRef Exporter::makeConvexShape( Mesh& mesh, Matrix3& tm, Niflib::HavokMaterial mtlDefault )
{
	float bhkScale = ( IsSkyrim( ) ) ? 10.0f * Exporter::bhkScaleFactor : Exporter::bhkScaleFactor;

	Niflib::bhkConvexVerticesShapeRef shape = Niflib::StaticCast<Niflib::bhkConvexVerticesShape>( Niflib::bhkConvexVerticesShape::Create( ) );
	Point3 center( 0.0f, 0.0f, 0.0f );
	float radius = 0.10f;

	CalcCenteredSphere( mesh, center, radius );

	radius /= bhkScale;
	shape->SetRadius( radius );

	std::vector<Niflib::Vector3> verts;
	std::vector<Niflib::Vector4> norms;

	int nvert = mesh.getNumVerts( );
	int nface = mesh.getNumFaces( );

	mesh.checkNormals( FALSE );
	mesh.buildNormals( );

	verts.resize( nvert );
	norms.resize( nface );

	for( int i = 0; i < nvert; ++i )
	{
		Point3 vert = ( mesh.getVert( i ) * tm ) / bhkScale;
		verts[ i ] = TOVECTOR3( vert );
	}

	for( int i = 0; i < nface; ++i )
	{
		Niflib::Vector4& value = norms[ i ];
		Point3& pt = mesh.getFaceNormal( i );

		value[ 0 ] = pt.x;
		value[ 1 ] = pt.y;
		value[ 2 ] = pt.z;
		value[ 3 ] = -( mesh.FaceCenter( i ) * tm ).Length( ) / bhkScale;
	}

	sortVector3( verts );
	sortVector4( norms );

	shape->SetVertices( verts );
	shape->SetNormalsAndDist( norms );

	return shape;
}

Niflib::bhkNiTriStripsShapeRef Exporter::makeTriStripsShape( Mesh& mesh, Matrix3& sm, Niflib::HavokMaterial mtlDefault )
{
	typedef std::vector<Niflib::Triangle> Triangles;

	// Setup shape data
	std::vector<Niflib::Vector3> verts;
	std::vector<Niflib::Vector3> vnorms;
	Triangles tris;

	int vi[ 3 ];
	if( TMNegParity( sm ) )
	{
		vi[ 0 ] = 2;
		vi[ 1 ] = 1;
		vi[ 2 ] = 0;
	}
	else
	{
		vi[ 0 ] = 0;
		vi[ 1 ] = 1;
		vi[ 2 ] = 2;
	}

	for( int i = 0; i < mesh.getNumFaces( ); i++ )
		addFace( tris, verts, vnorms, i, vi, &mesh, sm );

	Niflib::NiTriStripsDataRef data = new Niflib::NiTriStripsData( tris, Exporter::mUseAlternateStripper );
	data->SetVertices( verts );
	data->SetNormals( vnorms );

	// Setup shape
	Niflib::bhkNiTriStripsShapeRef shape = Niflib::StaticCast<Niflib::bhkNiTriStripsShape>( Niflib::bhkNiTriStripsShape::Create( ) );
	shape->SetNumStripsData( 1 );
	shape->SetStripsData( 0, data );
	shape->SetNumDataLayers( 1 );
	shape->SetOblivionLayer( 0, Niflib::OL_STATIC );
	shape->SetMaterial( mtlDefault );

	return shape;
}

Niflib::bhkPackedNiTriStripsShapeRef Exporter::makePackedTriStripsShape( Mesh& mesh, Matrix3& sm, Niflib::HavokMaterial mtlDefault, Niflib::OblivionLayer layer, int colFilter )
{
	float bhkScale = ( IsSkyrim( ) ) ? Exporter::bhkScaleFactor * 10.0f : Exporter::bhkScaleFactor;

	// Need to seperate the vertices based on material.
	typedef std::vector<Niflib::Triangle> Triangles;

	// Setup shape data
	std::vector<Niflib::Vector3> verts;
	std::vector<Niflib::Vector3> norms;
	Triangles tris;

	int vi[ 3 ];
	if( TMNegParity( sm ) )
	{
		vi[ 0 ] = 2;
		vi[ 1 ] = 1;
		vi[ 2 ] = 0;
	}
	else
	{
		vi[ 0 ] = 0;
		vi[ 1 ] = 1;
		vi[ 2 ] = 2;
	}

	int nvert = mesh.getNumVerts( );
	int nface = mesh.getNumFaces( );
	mesh.buildNormals( );

	tris.resize( nface );
	verts.resize( nvert );
	norms.resize( nface );

	for( int i = 0; i < nvert; ++i )
	{
		Point3 vert = ( mesh.getVert( i ) * sm ) / bhkScale;
		verts[ i ] = TOVECTOR3( vert );
	}

	for( int i = 0; i < nface; ++i )
	{
		Niflib::Triangle& tri = tris[ i ];
		norms[ i ] = TOVECTOR3( mesh.getFaceNormal( i ) );

		Face& face = mesh.faces[ i ];
		tri[ 0 ] = (USHORT) face.getVert( 0 );
		tri[ 1 ] = (USHORT) face.getVert( 1 );
		tri[ 2 ] = (USHORT) face.getVert( 2 );
	}

	Niflib::hkPackedNiTriStripsDataRef data = new Niflib::hkPackedNiTriStripsData( );
	data->SetNumFaces( tris.size( ) );
	data->SetVertices( verts );
	data->SetTriangles( tris );
	data->SetNormals( norms );

	// Setup shape
	Niflib::bhkPackedNiTriStripsShapeRef shape = new Niflib::bhkPackedNiTriStripsShape( );
	shape->SetData( data );

	Niflib::OblivionSubShape subshape;
	subshape.layer = layer;
	subshape.material = mtlDefault;
	subshape.colFilter = colFilter;
	subshape.numVertices = verts.size( );

	std::vector<Niflib::OblivionSubShape> subshapes;
	subshapes.push_back( subshape );

	shape->SetSubShapes( subshapes );
	data->SetSubShapes( subshapes );

	return shape;
}

//Niflib::bhkMoppBvTreeShapeRef Exporter::makeCompressedMeshShape(FaceGroups& faceGrps, Matrix3& sm, Niflib::HavokMaterial mtlDefault, Niflib::OblivionLayer layer, int colFilter )
//{
//	Niflib::bhkCompressedMeshShapeRef shape;
//	Niflib::bhkMoppBvTreeShapeRef mopp;
//	std::wstring metadataName;
//	std::wstring TempPath;
//
//	wchar_t temppath[ MAX_PATH ];
//	GetTempPath( MAX_PATH, temppath );
//	TempPath = std::wstring( temppath );
//
//	float tempScaleFactor = bhkScaleFactor;
//	if( IsSkyrim( ) )
//		tempScaleFactor *= 10.0f;
//
//	if( Niflib::bhkCompressedMeshShapeDataRef data = new Niflib::bhkCompressedMeshShapeData( ) )
//	{
//		std::vector<Niflib::Vector3> moppverts;
//		std::vector<Niflib::Triangle> mopptris;
//
//		int nvert = faceGrps[ 0 ].verts.size( );//mesh.getNumVerts( );
//		int nface = faceGrps[ 0 ].faces.size( );//mesh.getNumFaces( );
//		//mesh.buildNormals( );
//
//		moppverts.resize( nvert );
//		mopptris.resize( nface );
//
//		metadataName = TempPath + _T( "\\temp.meta" );
//		std::ofstream metadata( metadataName.c_str( ), std::ios::out | std::ios::binary );
//
//		metadata.write( (char*) &nvert, sizeof( int ) );
//		for( int i = 0; i < nvert; i++ )
//		{
//			Point3& v = TOPOINT3( faceGrps[ 0 ].verts[ i ] ) * sm;//mesh.getVert( i );
//			v /= tempScaleFactor;
//			metadata.write( (char*) &v, sizeof( Point3 ) );
//
//			Niflib::Vector3& moppvert = moppverts[ i ];
//			moppvert = TOVECTOR3( v );
//		}
//
//		metadata.write( (char*) &nface, sizeof( int ) );
//		for( int i = 0; i < nface; i++ )
//		{
//			//Face& f = mesh.faces[ i ];
//			Niflib::Triangle& f = faceGrps[ 0 ].faces[ i ];
//			DWORD v1 = f.v1; //f.getVert( 0 );
//			DWORD v2 = f.v2; //f.getVert( 1 );
//			DWORD v3 = f.v3; //f.getVert( 2 );
//
//			metadata.write( (char*) &v1, sizeof( DWORD ) );
//			metadata.write( (char*) &v2, sizeof( DWORD ) );
//			metadata.write( (char*) &v3, sizeof( DWORD ) );
//
//			Niflib::Triangle& mopptri = mopptris[ i ];
//			mopptri.v1 = v1;
//			mopptri.v2 = v2;
//			mopptri.v3 = v3;
//		}
//
//		metadata.close( );
//
//		wchar_t curfile[ _MAX_PATH ];
//		GetModuleFileName( hInstance, curfile, MAX_PATH );
//		PathRemoveFileSpec( curfile );
//		PathAppend( curfile, _T( "NifColl.exe" ) );
//
//		wsprintf( curfile, _T( "%s \"%s\"" ), curfile, metadataName.c_str( ) );
//
//		DWORD retcode;
//		if( ExecuteCommandLine( curfile, retcode ) )
//		{
//			std::ifstream moppdata( TempPath + _T( "\\temp.out" ), std::ios::binary );
//
//			UINT bitsPerIndex;
//			UINT bitsPerWIndex;
//			UINT maskWIndex;
//			UINT maskIndex;
//			float error;
//			float scale;
//			Niflib::Vector3 origin;
//			Niflib::Vector4 minBound;
//			Niflib::Vector4 maxBound;
//			std::vector<Niflib::byte> moppcode;
//			std::vector<Niflib::Vector4> bigverts;
//			std::vector<Niflib::bhkCMSDBigTris> bigtris;
//			std::vector<Niflib::bhkCMSDTransform> transform;
//			std::vector<Niflib::bhkCMSDChunk> chunks;
//
//			// origin and scale
//			moppdata.read( (char*) &origin, sizeof( Niflib::Vector3 ) );
//			moppdata.read( (char*) &scale, sizeof( float ) );
//		
//			// mopp code
//			int moppcodesize;
//			moppdata.read( (char*) &moppcodesize, sizeof( int ) );
//
//			moppcode.resize( moppcodesize );
//			for( int i = 1; i < moppcodesize; i++ )
//				moppdata.read( (char*) &moppcode[ i ], sizeof( Niflib::byte ) );
//
//			moppdata.read( (char*) &moppcode[ 0 ], sizeof( Niflib::byte ) );
//
//			// bits per index/W index
//			moppdata.read( (char*) &bitsPerIndex, sizeof( UINT ) );
//			moppdata.read( (char*) &bitsPerWIndex, sizeof( UINT ) );
//
//			// masks
//			moppdata.read( (char*) &maskWIndex, sizeof( UINT ) );
//			moppdata.read( (char*) &maskIndex, sizeof( UINT ) );
//
//			// error
//			moppdata.read( (char*) &error, sizeof( float ) );
//
//			// min/max bound
//			moppdata.read( (char*) &minBound, sizeof( Niflib::Vector4 ) );
//			moppdata.read( (char*) &maxBound, sizeof( Niflib::Vector4 ) );
//
//			// big verts
//			int bvertsize;
//			moppdata.read( (char*) &bvertsize, sizeof( int ) );
//
//			bigverts.resize( bvertsize );
//			for( int i = 0; i < bvertsize; i++ )
//				moppdata.read( (char*) &bigverts[ i ], sizeof( Niflib::Vector4 ) );
//
//			// big tris
//			int btrisize;
//			moppdata.read( (char*) &btrisize, sizeof( int ) );
//
//			bigtris.resize( btrisize );
//			for( int i = 0; i < btrisize; i++ )
//			{
//				Niflib::bhkCMSDBigTris& btri = bigtris[ i ];
//
//				moppdata.read( (char*) &btri.triangle1, sizeof( unsigned short ) );
//				moppdata.read( (char*) &btri.triangle2, sizeof( unsigned short ) );
//				moppdata.read( (char*) &btri.triangle3, sizeof( unsigned short ) );
//				moppdata.read( (char*) &btri.unknownInt1, sizeof( unsigned int ) );
//				moppdata.read( (char*) &btri.unknownShort1, sizeof( unsigned short ) );
//
//				btri.unknownInt1 = 0;
//			}
//
//			// transforms
//			int transformsize;
//			moppdata.read( (char*) &transformsize, sizeof( int ) );
//
//			transform.resize( transformsize );
//			for( int i = 0; i < transformsize; i++ )
//			{
//				Niflib::bhkCMSDTransform& trans = transform[ i ];
//				moppdata.read( (char*) &trans, sizeof( Niflib::bhkCMSDTransform ) );
//			}
//
//			// chunks
//			int chunksize;
//			moppdata.read( (char*) &chunksize, sizeof( int ) );
//
//			chunks.resize( chunksize );
//			for( int i = 0; i < chunksize; i++ )
//			{
//				Niflib::bhkCMSDChunk& chunk = chunks[ i ];
//
//				moppdata.read( (char*) &chunk.translation, sizeof( Niflib::Vector4 ) );
//				moppdata.read( (char*) &chunk.materialIndex, sizeof( unsigned int ) );
//				moppdata.read( (char*) &chunk.unknownShort1, sizeof( USHORT ) );
//				moppdata.read( (char*) &chunk.transformIndex, sizeof( USHORT ) );
//				moppdata.read( (char*) &chunk.numVertices, sizeof( unsigned int ) );
//
//				chunk.vertices.resize( chunk.numVertices );
//				for( int i = 0; i < chunk.numVertices; i++ )
//					moppdata.read( (char*) &chunk.vertices[ i ], sizeof( USHORT ) );
//
//				moppdata.read( (char*) &chunk.numIndices, sizeof( unsigned int ) );
//
//				chunk.indices.resize( chunk.numIndices );
//				for( int i = 0; i < chunk.numIndices; i++ )
//					moppdata.read( (char*) &chunk.indices[ i ], sizeof( USHORT ) );
//
//				moppdata.read( (char*) &chunk.numStrips, sizeof( unsigned int ) );
//
//				chunk.strips.resize( chunk.numStrips );
//				for( int i = 0; i < chunk.numStrips; i++ )
//					moppdata.read( (char*) &chunk.strips[ i ], sizeof( USHORT ) );
//
//				moppdata.read( (char*) &chunk.numIndices2, sizeof( unsigned int ) );
//
//				chunk.indices2.resize( chunk.numIndices2 );
//				for( int i = 0; i < chunk.numIndices2; i++ )
//					moppdata.read( (char*) &chunk.indices2[ i ], sizeof( USHORT ) );
//			}
//
//			moppdata.close( );
//
//			// We have obtained all we need. Remove the out file
//			DeleteFile( std::wstring( TempPath + _T( "\\temp.out" ) ).c_str( ) );
//
//			// Temporary material
//			std::vector<Niflib::bhkCMSDMaterial> materials;
//			Niflib::bhkCMSDMaterial mat;
//
//			mat.skyrimMaterial = Niflib::SkyrimHavokMaterial::SKY_HAV_MAT_STONE;
//			mat.unknownInteger = 1;
//
//			materials.push_back( mat );
//
//			// Populate data
//			data->SetBitsPerIndex( bitsPerIndex );
//			data->SetBitsPerWIndex( bitsPerWIndex );
//			data->SetMaskWIndex( maskWIndex );
//			data->SetMaskIndex( maskIndex );
//			data->SetError( error );
//			data->SetBoundsMin( minBound );
//			data->SetBoundsMax( maxBound );
//			data->SetChunkMaterials( materials );
//			data->SetChunkTransforms( transform );
//			data->SetBigVerts( bigverts );
//			data->SetBigTris( bigtris );
//			data->SetChunks( chunks );
//
//			if( shape = new Niflib::bhkCompressedMeshShape( ) )
//			{
//				Point3 center;
//				float radius;
//
//				//CalcAxisAlignedSphere( mesh, center, radius );
//
//				shape->SetData( data );
//				shape->SetScale( 1.0f );
//				shape->SetRadius( 0.005f );
//
//				if( mopp = new Niflib::bhkMoppBvTreeShape( ) )
//				{
//					//std::vector<Niflib::byte> moppc = ConstructHKMesh( moppverts, mopptris, origin, scale );
//
//					//moppcode.insert( moppcode.begin( ), 1, 1 );
//
//					mopp->SetShape( shape );
//					mopp->SetMoppCode( moppcode );
//					mopp->SetMoppOrigin( origin );
//					mopp->SetMoppScale( scale );
//
//					return mopp;
//				}
//			}
//		}
//	}
//
//	return NULL;
//}

Niflib::bhkMoppBvTreeShapeRef Exporter::makeCompressedMeshShape( Mesh& mesh, Matrix3& sm, Niflib::HavokMaterial mtlDefault, Niflib::OblivionLayer layer, int colFilter )
{
	Niflib::bhkCompressedMeshShapeRef shape;
	Niflib::bhkMoppBvTreeShapeRef mopp;
	std::wstring metadataName;
	std::wstring TempPath;

	wchar_t temppath[ MAX_PATH ];
	GetTempPath( MAX_PATH, temppath );
	TempPath = std::wstring( temppath );

	float tempScaleFactor = bhkScaleFactor;
	if( IsSkyrim( ) )
		tempScaleFactor *= 10.0f;

	if( Niflib::bhkCompressedMeshShapeDataRef data = new Niflib::bhkCompressedMeshShapeData( ) )
	{
		std::vector<Niflib::Vector3> moppverts;
		std::vector<Niflib::Triangle> mopptris;

		int nvert = mesh.getNumVerts( );
		int nface = mesh.getNumFaces( );
		//mesh.buildNormals( );

		//moppverts.resize( nvert );
		//mopptris.resize( nface );

		metadataName = TempPath + _T( "\\temp.meta" );
		std::ofstream metadata( metadataName.c_str( ), std::ios::out | std::ios::binary );
		//std::ofstream objdata( _T( "F:\\Temp.obj" ), std::ios::out );
		//char line[ 1024 ];

		metadata.write( (char*) &nvert, sizeof( int ) );
		for( int i = 0; i < nvert; i++ )
		{
			Point3& v = mesh.getVert( i );
			
			//sprintf_s( line, "v %f %f %f\n", v.x, v.y, v.z );
			//objdata.write( (char*) line, strlen( line ) );

			v /= tempScaleFactor;
			metadata.write( (char*) &v, sizeof( Point3 ) );
		}

		metadata.write( (char*) &nface, sizeof( int ) );
		for( int i = 0; i < nface; i++ )
		{
			Face& f = mesh.faces[ i ];
			//Niflib::Triangle& f = faceGrps[ 0 ].faces[ i ];
			DWORD v1 = f.getVert( 0 );
			DWORD v2 = f.getVert( 1 );
			DWORD v3 = f.getVert( 2 );

			metadata.write( (char*) &v1, sizeof( DWORD ) );
			metadata.write( (char*) &v2, sizeof( DWORD ) );
			metadata.write( (char*) &v3, sizeof( DWORD ) );

			//sprintf_s( line, "f %d %d %d\n", (v1 + 1), (v2 + 1), (v3 + 1) );
			//objdata.write( (char*) line, strlen( line ) );
		}

		//objdata.close( );
		metadata.close( );

		wchar_t curfile[ _MAX_PATH ];
		GetModuleFileName( hInstance, curfile, MAX_PATH );
		PathRemoveFileSpec( curfile );
		PathAppend( curfile, _T( "NifColl.exe" ) );

		wsprintf( curfile, _T( "%s \"%s\"" ), curfile, metadataName.c_str( ) );

		DWORD retcode;
		if( ExecuteCommandLine( curfile, retcode ) )
		{
			std::ifstream moppdata( TempPath + _T( "\\temp.out" ), std::ios::binary );

			UINT bitsPerIndex;
			UINT bitsPerWIndex;
			UINT maskWIndex;
			UINT maskIndex;
			float error;
			float scale;
			Niflib::Vector3 origin;
			Niflib::Vector4 minBound;
			Niflib::Vector4 maxBound;
			std::vector<Niflib::byte> moppcode;
			std::vector<Niflib::Vector4> bigverts;
			std::vector<Niflib::bhkCMSDBigTris> bigtris;
			std::vector<Niflib::bhkCMSDTransform> transform;
			std::vector<Niflib::bhkCMSDChunk> chunks;

			// origin and scale
			moppdata.read( (char*) &origin, sizeof( Niflib::Vector3 ) );
			moppdata.read( (char*) &scale, sizeof( float ) );
		
			// mopp code
			int moppcodesize;
			moppdata.read( (char*) &moppcodesize, sizeof( int ) );
		
			moppcode.resize( moppcodesize + 1 );
			for( int i = 0; i < moppcodesize - 1; i++ )
				moppdata.read( (char*) &moppcode[ i + 1 ], sizeof( Niflib::byte ) );

			moppdata.read( (char*) &moppcode[ 0 ], sizeof( Niflib::byte ) );
			moppcode[ moppcodesize ] = moppcode[ 0 ];

			// bits per index/W index
			moppdata.read( (char*) &bitsPerIndex, sizeof( UINT ) );
			moppdata.read( (char*) &bitsPerWIndex, sizeof( UINT ) );

			// masks
			moppdata.read( (char*) &maskWIndex, sizeof( UINT ) );
			moppdata.read( (char*) &maskIndex, sizeof( UINT ) );

			// error
			moppdata.read( (char*) &error, sizeof( float ) );

			// min/max bound
			moppdata.read( (char*) &minBound, sizeof( Niflib::Vector4 ) );
			moppdata.read( (char*) &maxBound, sizeof( Niflib::Vector4 ) );

			// big verts
			int bvertsize;
			moppdata.read( (char*) &bvertsize, sizeof( int ) );

			bigverts.resize( bvertsize );
			for( int i = 0; i < bvertsize; i++ )
				moppdata.read( (char*) &bigverts[ i ], sizeof( Niflib::Vector4 ) );

			// big tris
			int btrisize;
			moppdata.read( (char*) &btrisize, sizeof( int ) );

			bigtris.resize( btrisize );
			for( int i = 0; i < btrisize; i++ )
			{
				Niflib::bhkCMSDBigTris& btri = bigtris[ i ];

				moppdata.read( (char*) &btri.triangle1, sizeof( unsigned short ) );
				moppdata.read( (char*) &btri.triangle2, sizeof( unsigned short ) );
				moppdata.read( (char*) &btri.triangle3, sizeof( unsigned short ) );
				moppdata.read( (char*) &btri.unknownInt1, sizeof( unsigned int ) );
				moppdata.read( (char*) &btri.unknownShort1, sizeof( unsigned short ) );

				btri.unknownInt1 = 0;
			}

			// transforms
			int transformsize;
			moppdata.read( (char*) &transformsize, sizeof( int ) );

			transform.resize( transformsize );
			for( int i = 0; i < transformsize; i++ )
			{
				Niflib::bhkCMSDTransform& trans = transform[ i ];
				moppdata.read( (char*) &trans, sizeof( Niflib::bhkCMSDTransform ) );
			}

			// chunks
			int chunksize;
			moppdata.read( (char*) &chunksize, sizeof( int ) );

			chunks.resize( chunksize );
			for( int i = 0; i < chunksize; i++ )
			{
				Niflib::bhkCMSDChunk& chunk = chunks[ i ];

				moppdata.read( (char*) &chunk.translation, sizeof( Niflib::Vector4 ) );
				moppdata.read( (char*) &chunk.materialIndex, sizeof( unsigned int ) );
				moppdata.read( (char*) &chunk.unknownShort1, sizeof( USHORT ) );
				moppdata.read( (char*) &chunk.transformIndex, sizeof( USHORT ) );
				moppdata.read( (char*) &chunk.numVertices, sizeof( unsigned int ) );

				chunk.vertices.resize( chunk.numVertices );
				for( int i = 0; i < chunk.numVertices; i++ )
					moppdata.read( (char*) &chunk.vertices[ i ], sizeof( USHORT ) );

				moppdata.read( (char*) &chunk.numIndices, sizeof( unsigned int ) );

				chunk.indices.resize( chunk.numIndices );
				for( int i = 0; i < chunk.numIndices; i++ )
					moppdata.read( (char*) &chunk.indices[ i ], sizeof( USHORT ) );

				moppdata.read( (char*) &chunk.numStrips, sizeof( unsigned int ) );

				chunk.strips.resize( chunk.numStrips );
				for( int i = 0; i < chunk.numStrips; i++ )
					moppdata.read( (char*) &chunk.strips[ i ], sizeof( USHORT ) );

				moppdata.read( (char*) &chunk.numIndices2, sizeof( unsigned int ) );

				chunk.indices2.resize( chunk.numIndices2 );
				for( int i = 0; i < chunk.numIndices2; i++ )
					moppdata.read( (char*) &chunk.indices2[ i ], sizeof( USHORT ) );
			}

			moppdata.close( );

			// We have obtained all we need. Remove the out file
			DeleteFile( std::wstring( TempPath + _T( "\\temp.out" ) ).c_str( ) );

			// Temporary material
			std::vector<Niflib::bhkCMSDMaterial> materials;
			Niflib::bhkCMSDMaterial mat;

			mat.skyrimMaterial = Niflib::SkyrimHavokMaterial::SKY_HAV_MAT_STONE;
			mat.unknownInteger = 1;

			materials.push_back( mat );

			// Populate data
			data->SetBitsPerIndex( bitsPerIndex );
			data->SetBitsPerWIndex( bitsPerWIndex );
			data->SetMaskWIndex( maskWIndex );
			data->SetMaskIndex( maskIndex );
			data->SetError( error );
			data->SetBoundsMin( minBound );
			data->SetBoundsMax( maxBound );
			data->SetChunkMaterials( materials );
			data->SetChunkTransforms( transform );
			data->SetBigVerts( bigverts );
			data->SetBigTris( bigtris );
			data->SetChunks( chunks );

			if( shape = new Niflib::bhkCompressedMeshShape( ) )
			{
				Point3 center;
				float radius;

				//CalcAxisAlignedSphere( mesh, center, radius );

				shape->SetData( data );
				shape->SetScale( 1.0f );
				shape->SetRadius( 0.005f );
				
				shape->SetUnknownFloat1( 0.005f );
				shape->SetUnknownFloats1( Niflib::Vector4( 1.0f, 1.0f, 1.0f, 0.0f ) );
				shape->SetUnknownFloat3( 1.0f );
				shape->SetUnknownFloat4( 1.0f );
				shape->SetUnknownFloat5( 1.0f );

				if( mopp = new Niflib::bhkMoppBvTreeShape( ) )
				{
					mopp->SetShape( shape );
					mopp->SetMoppCode( moppcode );
					mopp->SetMoppOrigin( origin );
					mopp->SetMoppScale( scale );

					return mopp;
				}
			}
		}
	}

	return NULL;
}

Niflib::bhkShapeRef Exporter::makeProxyShape( INode* node, Object* obj, Matrix3& tm, Niflib::HavokMaterial mtlDefault )
{
	enum { list_params, bv_mesh, };  // pblock2 ID
	enum { PB_MATERIAL, PB_MESHLIST, PB_BOUND_TYPE, PB_CENTER, };
	enum { bv_type_none, bv_type_box, bv_type_shapes, bv_type_packed, bv_type_convex, bv_type_capsule, bv_type_obb, };  // pblock ID

	Niflib::bhkShapeRef shape;
	if( IParamBlock2* pblock2 = obj->GetParamBlockByID( list_params ) )
	{
		int bvType = bv_type_none;
		pblock2->GetValue( PB_BOUND_TYPE, 0, bvType, FOREVER, 0 );

		if( bvType != bv_type_none )
		{
			if( TriObject* triObj = (TriObject*) obj->ConvertToType( 0, triObjectClassID ) )
			{
				Mesh& mesh = triObj->GetMesh( );
				switch( bvType )
				{
				case bv_type_box:
					shape = makeProxyBoxShape( node, obj, mesh, tm, mtlDefault );
					break;

				case bv_type_shapes:
					shape = makeProxyTriStripShape( node, obj, mesh, tm, mtlDefault );
					break;

				case bv_type_packed:
					shape = makeProxyPackedTriStripShape( node, obj, mesh, tm, mtlDefault );
					break;

				case bv_type_convex:
					shape = makeProxyConvexShape( node, obj, mesh, tm, mtlDefault );
					break;

				case bv_type_capsule:
					shape = makeProxyCapsuleShape( node, obj, mesh, tm, mtlDefault );
					break;

				case bv_type_obb:
					shape = makeProxyOBBShape( node, obj, mesh, tm, mtlDefault );
					break;
				}
			}
		}
	}

	return shape;
}

Niflib::bhkShapeRef Exporter::makeProxyBoxShape( INode* node, Object* obj, Mesh& mesh, Matrix3& tm, Niflib::HavokMaterial mtlDefault )
{
	float bhkScale = ( IsSkyrim( ) ) ? Exporter::bhkScaleFactor * 10.0f : Exporter::bhkScaleFactor;

	enum { list_params, bv_mesh, };  // pblock2 ID
	enum { PB_MATERIAL, PB_MESHLIST, PB_BOUND_TYPE, PB_CENTER, };
	enum { bv_type_none, bv_type_box, bv_type_shapes, bv_type_packed, bv_type_convex, };  // pblock ID

	Niflib::bhkShapeRef retval;
	if( IParamBlock2* pblock2 = obj->GetParamBlockByID( list_params ) )
	{
		Box3 box;
		box.Init( );

		CalcAxisAlignedBox( mesh, box, NULL );

		Niflib::HavokMaterial mtl = mtlDefault;
		float length = 0;
		float width = 0;
		float height = 0;

		mtl = Niflib::HavokMaterial( pblock2->GetInt( PB_MATERIAL, 0, 0 ) );
		if( mtl < 0 )
			mtl = mtlDefault;

		Niflib::bhkBoxShapeRef shape = new Niflib::bhkBoxShape( );
		Niflib::Vector3 dim( box.Max( ).x - box.Min( ).x, box.Max( ).y - box.Min( ).y, box.Max( ).z - box.Min( ).z );
		dim /= ( bhkScale * 2.0f );

		shape->SetMaterial( mtl );
		shape->SetDimensions( dim );

		Matrix3 ltm = TransMatrix( box.Center( ) ) * tm;
		if( ltm.IsIdentity( ) )
			retval = Niflib::StaticCast<Niflib::bhkShape>( shape );

		else
		{
			ltm.SetTrans( ltm.GetTrans( ) / bhkScale );

			Niflib::bhkTransformShapeRef transform = new Niflib::bhkTransformShape( );
			transform->SetTransform( TOMATRIX4( ltm ).Transpose( ) );
			transform->SetShape( shape );
			transform->SetMaterial( mtl );

			retval = Niflib::StaticCast<Niflib::bhkShape>( transform );
		}
	}

	return retval;
}

Niflib::bhkShapeRef Exporter::makeProxyOBBShape( INode* node, Object* obj, Mesh& mesh, Matrix3& tm, Niflib::HavokMaterial mtlDefault )
{
	enum { list_params, bv_mesh, };  // pblock2 ID
	enum { PB_MATERIAL, PB_MESHLIST, PB_BOUND_TYPE, PB_CENTER, };
	enum { bv_type_none, bv_type_box, bv_type_shapes, bv_type_packed, bv_type_convex, };  // pblock ID

	Niflib::bhkShapeRef retval;
	if( IParamBlock2* pblock2 = obj->GetParamBlockByID( list_params ) )
	{
		Matrix3 rtm( true );
		Point3 center;
		float udim;
		float vdim;
		float ndim;

		CalcOrientedBox( mesh, udim, vdim, ndim, center, rtm );

		Niflib::HavokMaterial mtl = mtlDefault;
		float length = 0;
		float width = 0;
		float height = 0;

		mtl = Niflib::HavokMaterial( pblock2->GetInt( PB_MATERIAL, 0, 0 ) );
		if( mtl < 0 )
			mtl = mtlDefault;

		Niflib::bhkBoxShapeRef shape = new Niflib::bhkBoxShape( );
		Niflib::Vector3 dim( udim, vdim, ndim );
		dim /= ( Exporter::bhkScaleFactor * 2 );

		shape->SetDimensions( dim );
		shape->SetMaterial( mtl );

		Matrix3 ltm = rtm * tm; // Translation already done in CalcOrientedBox( ).
		if( ltm.IsIdentity( ) )
			retval = Niflib::StaticCast<Niflib::bhkShape>( shape );

		else
		{
			ltm.SetTrans( ltm.GetTrans( ) / Exporter::bhkScaleFactor );

			Niflib::bhkTransformShapeRef transform = new Niflib::bhkTransformShape( );
			transform->SetTransform( TOMATRIX4( ltm ).Transpose( ) );
			transform->SetShape( shape );
			transform->SetMaterial( mtl );

			retval = Niflib::StaticCast<Niflib::bhkShape>( transform );
		}
	}

	return retval;
}

Niflib::bhkShapeRef Exporter::makeProxyCapsuleShape( INode* node, Object* obj, Mesh& mesh, Matrix3& tm, Niflib::HavokMaterial mtlDefault )
{
	enum { list_params, bv_mesh, };  // pblock2 ID
	enum { PB_MATERIAL, PB_MESHLIST, PB_BOUND_TYPE, PB_CENTER, };
	enum { bv_type_none, bv_type_box, bv_type_shapes, bv_type_packed, bv_type_convex, bv_type_capsule, bv_type_obb, };  // pblock ID

	Niflib::bhkShapeRef retval;
	if( IParamBlock2* pblock2 = obj->GetParamBlockByID( list_params ) )
	{
		Niflib::HavokMaterial mtl = mtlDefault;
		mtl = Niflib::HavokMaterial( pblock2->GetInt( PB_MATERIAL, 0, 0 ) );

		if( mtl < 0 )
			mtl = mtlDefault;

		Point3 center = Point3::Origin;
		Point3 pt1 = Point3::Origin;
		Point3 pt2 = Point3::Origin;
		Point3 trans_pt1 = Point3::Origin;
		Point3 trans_pt2 = Point3::Origin;
		float r1 = 0.0f;
		float r2 = 0.0f;

		CalcCapsule( mesh, pt1, pt2, r1, r2 ); // Both R's are the same.

		center = ( pt1 + pt2 ) / 2;
		trans_pt1 = pt1 - center;
		trans_pt2 = pt2 - center;

		if( Niflib::bhkCapsuleShapeRef shape = new Niflib::bhkCapsuleShape( ) )
		{
			shape->SetRadius( r1 / Exporter::bhkScaleFactor );
			shape->SetRadius1( r1 / Exporter::bhkScaleFactor );
			shape->SetRadius2( r1 / Exporter::bhkScaleFactor );
			shape->SetFirstPoint( TOVECTOR3( trans_pt1 / Exporter::bhkScaleFactor ) );
			shape->SetSecondPoint( TOVECTOR3( trans_pt2 / Exporter::bhkScaleFactor ) );
			shape->SetMaterial( mtl );

			Matrix3 ltm = TransMatrix( center ) * tm;
			if( ltm.IsIdentity( ) )
				retval = Niflib::StaticCast<Niflib::bhkShape>( shape );

			else
			{
				ltm.SetTrans( ltm.GetTrans( ) / Exporter::bhkScaleFactor );

				Niflib::bhkTransformShapeRef transform = new Niflib::bhkTransformShape( );
				transform->SetTransform( TOMATRIX4( ltm ).Transpose( ) );
				transform->SetShape( shape );
				transform->SetMaterial( mtl );

				retval = Niflib::StaticCast<Niflib::bhkShape>( transform );
			}
		}
	}

	return retval;
}

Niflib::bhkShapeRef Exporter::makeProxySphereShape( INode* node, Object* obj, Mesh& mesh, Matrix3& tm, Niflib::HavokMaterial mtlDefault )
{
	enum { list_params, bv_mesh, };  // pblock2 ID
	enum { PB_MATERIAL, PB_MESHLIST, PB_BOUND_TYPE, PB_CENTER, };
	enum { bv_type_none, bv_type_box, bv_type_shapes, bv_type_packed, bv_type_convex, };  // pblock ID

	Niflib::bhkShapeRef shape;
	if( IParamBlock2* pblock2 = obj->GetParamBlockByID( list_params ) )
	{
	}

	return shape;
}

Niflib::bhkShapeRef Exporter::makeProxyConvexShape( INode* node, Object* obj, Mesh& mesh, Matrix3& tm, Niflib::HavokMaterial mtlDefault )
{
	enum { list_params, bv_mesh, };  // pblock2 ID
	enum { PB_MATERIAL, PB_MESHLIST, PB_BOUND_TYPE, PB_CENTER, };
	enum { bv_type_none, bv_type_box, bv_type_shapes, bv_type_packed, bv_type_convex, };  // pblock ID

	Niflib::bhkShapeRef shape;
	if( IParamBlock2* pblock2 = obj->GetParamBlockByID( list_params ) )
	{
		if( Niflib::bhkConvexVerticesShapeRef convShape = makeConvexShape( mesh, tm, mtlDefault ) )
		{
			Niflib::HavokMaterial mtl = Niflib::HavokMaterial( pblock2->GetInt( PB_MATERIAL, 0, 0 ) );
			if( mtl < 0 )
				mtl = mtlDefault;

			convShape->SetMaterial( mtl );
			shape = Niflib::StaticCast<Niflib::bhkShape>( convShape );
		}
	}

	return shape;
}

Niflib::bhkShapeRef Exporter::makeProxyTriStripShape( INode* node, Object* obj, Mesh& mesh, Matrix3& tm, Niflib::HavokMaterial mtlDefault )
{
	enum { list_params, bv_mesh, };  // pblock2 ID
	enum { PB_MATERIAL, PB_MESHLIST, PB_BOUND_TYPE, PB_CENTER, };
	enum { bv_type_none, bv_type_box, bv_type_shapes, bv_type_packed, bv_type_convex, };  // pblock ID

	Niflib::bhkShapeRef shape;
	if( IParamBlock2* pblock2 = obj->GetParamBlockByID( list_params ) )
	{
		Niflib::HavokMaterial mtl = Niflib::HavokMaterial( pblock2->GetInt( PB_MATERIAL, 0, 0 ) );
		if( mtl < 0 )
			mtl = mtlDefault;

		// Transform location
		Mesh localmesh( mesh );
		MNMesh tmpMesh( localmesh );

		tmpMesh.Transform( tm );
		tmpMesh.buildNormals( );
		tmpMesh.OutToTri( localmesh );
		localmesh.buildNormals( );

		Matrix3 ident( true );

		Niflib::bhkNiTriStripsShapeRef trishape = makeTriStripsShape( localmesh, ident, mtl );
		trishape->SetMaterial( mtl );

		shape = Niflib::StaticCast<Niflib::bhkShape>( shape );
	}

	return shape;
}

Niflib::bhkShapeRef Exporter::makeProxyPackedTriStripShape( INode* node, Object* obj, Mesh& mesh, Matrix3& tm, Niflib::HavokMaterial mtlDefault )
{
	enum { list_params, bv_mesh, };  // pblock2 ID
	enum { PB_MATERIAL, PB_MESHLIST, PB_BOUND_TYPE, PB_CENTER, };
	enum { bv_type_none, bv_type_box, bv_type_shapes, bv_type_packed, bv_type_convex, };  // pblock ID

	Niflib::bhkShapeRef shape;
	if( IParamBlock2* pblock2 = obj->GetParamBlockByID( list_params ) )
	{
		Niflib::HavokMaterial mtl = Niflib::HavokMaterial( pblock2->GetInt( PB_MATERIAL, 0, 0 ) );
		if( mtl < 0 )
			mtl = mtlDefault;

		int layer = NP_DEFAULT_HVK_LAYER;
		int filter = NP_DEFAULT_HVK_FILTER;

		// Transform location
		Mesh localmesh( mesh );
		MNMesh tmpMesh( localmesh );

		tmpMesh.Transform( tm );
		tmpMesh.buildNormals( );
		tmpMesh.OutToTri( localmesh );
		localmesh.buildNormals( );

		Matrix3 ident( true );

		Niflib::bhkPackedNiTriStripsShapeRef trishape = makePackedTriStripsShape( localmesh, ident, mtlDefault, Niflib::OblivionLayer( layer ), filter );
		if( TheHavokCode.Initialize( ) )
			shape = Niflib::StaticCast<Niflib::bhkShape>( makeTreeShape( trishape, (Niflib::HavokMaterial) mtl ) );

		else
			shape = Niflib::StaticCast<Niflib::bhkShape>( trishape );
	}

	return shape;
}

Niflib::bhkShapeRef Exporter::makeModifierShape( INode* node, Object* obj, Modifier* mod, Matrix3& tm, Niflib::HavokMaterial mtlDefault )
{
	enum { havok_params };
	enum { PB_BOUND_TYPE, PB_MATERIAL };
	enum { bv_type_none, bv_type_box, bv_type_sphere, bv_type_capsule, bv_type_shapes, bv_type_convex, bv_type_packed, bv_type_obb, };  // pblock ID

	Niflib::bhkShapeRef shape;
	Niflib::HavokMaterial material = mtlDefault;
	const Mesh* mesh = NULL;
	int type = bv_type_none;

	node->EvalWorldState( 0 );
	if( bhkHelperInterface* bhkHelp = (bhkHelperInterface*) mod->GetInterface( BHKHELPERINTERFACE_DESC ) )
		mesh = bhkHelp->GetMesh( );

	else
	{
		if( TriObject* tri = (TriObject*) obj->ConvertToType( 0, Class_ID( TRIOBJ_CLASS_ID, 0 ) ) )
			mesh = &tri->GetMesh( );
	}

	if( IParamBlock2* pblock2 = mod->GetParamBlockByID( havok_params ) )
	{
		pblock2->GetValue( PB_BOUND_TYPE, 0, type, FOREVER, 0 );
		material = Niflib::HavokMaterial( pblock2->GetInt( PB_MATERIAL, 0, 0 ) );

		if( material < 0 )
			material = mtlDefault;
	}

	switch( type )
	{
	default:
	case bv_type_none:
		break;

	case bv_type_box:
		shape = makeModBoxShape( node, mod, const_cast<Mesh&>( *mesh ), tm, material );
		break;

	case bv_type_sphere:
		shape = makeModSphereShape( node, mod, const_cast<Mesh&>( *mesh ), tm, material );
		break;

	case bv_type_capsule:
		shape = makeModCapsuleShape( node, mod, const_cast<Mesh&>( *mesh ), tm, material );
		break;

	case bv_type_obb:
		shape = makeModOBBShape( node, mod, const_cast<Mesh&>( *mesh ), tm, material );
		break;

	case bv_type_shapes:
		shape = makeModTriStripShape( node, mod, const_cast<Mesh&>( *mesh ), tm, material );
		break;

	case bv_type_convex:
		shape = makeModConvexShape( node, mod, const_cast<Mesh&>( *mesh ), tm, material );
		break;

	case bv_type_packed:
		shape = makeModPackedTriStripShape( node, mod, const_cast<Mesh&>( *mesh ), tm, material );
		break;
	}

	return shape;
}

Niflib::bhkShapeRef Exporter::makeModBoxShape( INode* node, Modifier* mod, Mesh& mesh, Matrix3& tm, Niflib::HavokMaterial mtlDefault )
{
	float bhkScale = ( IsSkyrim( ) ) ? Exporter::bhkScaleFactor * 10.0f : Exporter::bhkScaleFactor;

	enum { havok_params };
	enum { PB_BOUND_TYPE, PB_MATERIAL };
	enum { bv_type_none, bv_type_box, bv_type_sphere, bv_type_capsule, bv_type_shapes, bv_type_convex, bv_type_packed, bv_type_obb, };  // pblock ID

	Niflib::HavokMaterial material = mtlDefault;

	if( IParamBlock2* pblock2 = mod->GetParamBlockByID( havok_params ) )
	{
		material = Niflib::HavokMaterial( pblock2->GetInt( PB_MATERIAL, 0, 0 ) );
		if( material < 0 )
			material = mtlDefault;
	}

	Niflib::bhkShapeRef retval;
	if( Niflib::bhkBoxShapeRef shape = new Niflib::bhkBoxShape( ) )
	{
		Box3 box;
		box.Init( );

		CalcAxisAlignedBox( mesh, box, NULL );

		Niflib::Vector3 dim( box.Max( ).x - box.Min( ).x, box.Max( ).y - box.Min( ).y, box.Max( ).z - box.Min( ).z );
		dim /= ( bhkScale * 2 );

		shape->SetDimensions( dim );
		shape->SetMaterial( material );

		Matrix3 ltm = TransMatrix( box.Center( ) ) * node->GetNodeTM( 0 ) * tm;
		if( ltm.IsIdentity( ) )
			retval = Niflib::StaticCast<Niflib::bhkShape>( shape );

		else
		{
			ltm.SetTrans( ltm.GetTrans( ) / bhkScale );

			Niflib::bhkTransformShapeRef transform = new Niflib::bhkTransformShape( );
			transform->SetTransform( TOMATRIX4( ltm ).Transpose( ) );
			transform->SetShape( shape );
			transform->SetMaterial( material );

			retval = Niflib::StaticCast<Niflib::bhkShape>( transform );
		}
	}

	return retval;
}

Niflib::bhkShapeRef Exporter::makeModSphereShape( INode* node, Modifier* mod, Mesh& mesh, Matrix3& tm, Niflib::HavokMaterial mtlDefault )
{
	float bhkScale = ( IsSkyrim( ) ) ? Exporter::bhkScaleFactor * 10.0f : Exporter::bhkScaleFactor;

	enum { havok_params };
	enum { PB_BOUND_TYPE, PB_MATERIAL };
	enum { bv_type_none, bv_type_box, bv_type_sphere, bv_type_capsule, bv_type_shapes, bv_type_convex, bv_type_packed, bv_type_obb, };  // pblock ID

	Niflib::HavokMaterial material = mtlDefault;

	if( IParamBlock2* pblock2 = mod->GetParamBlockByID( havok_params ) )
	{
		material = Niflib::HavokMaterial( pblock2->GetInt( PB_MATERIAL, 0, 0 ) );
		if( material < 0 )
			material = mtlDefault;
	}

	Niflib::bhkShapeRef retval;

	Point3 center = Point3::Origin;
	float radius = 0.0f;

	CalcCenteredSphere( mesh, center, radius );

	if( Niflib::bhkSphereRepShapeRef shape = new Niflib::bhkSphereShape( ) )
	{
		shape->SetRadius( radius / bhkScale );
		shape->SetMaterial( material );

		Matrix3 ltm = TransMatrix( center ) * node->GetObjTMAfterWSM( 0 ) * tm;
		if( ltm.IsIdentity( ) )
			retval = Niflib::StaticCast<Niflib::bhkShape>( shape );

		else
		{
			ltm.SetTrans( ltm.GetTrans( ) / bhkScale );

			Niflib::bhkTransformShapeRef transform = new Niflib::bhkTransformShape( );
			transform->SetTransform( TOMATRIX4( ltm ).Transpose( ) );
			transform->SetShape( shape );
			transform->SetMaterial( material );

			retval = Niflib::StaticCast<Niflib::bhkShape>( transform );
		}
	}

	return retval;
}

Niflib::bhkShapeRef Exporter::makeModCapsuleShape( INode* node, Modifier* mod, Mesh& mesh, Matrix3& tm, Niflib::HavokMaterial mtlDefault )
{
	float bhkScale = ( IsSkyrim( ) ) ? Exporter::bhkScaleFactor * 10.0f : Exporter::bhkScaleFactor;

	enum { havok_params };
	enum { PB_BOUND_TYPE, PB_MATERIAL };
	enum { bv_type_none, bv_type_box, bv_type_sphere, bv_type_capsule, bv_type_shapes, bv_type_convex, bv_type_packed, bv_type_obb, };  // pblock ID

	Niflib::HavokMaterial material = mtlDefault;

	if( IParamBlock2* pblock2 = mod->GetParamBlockByID( havok_params ) )
	{
		material = Niflib::HavokMaterial( pblock2->GetInt( PB_MATERIAL, 0, 0 ) );
		if( material < 0 )
			material = mtlDefault;
	}

	Niflib::bhkShapeRef retval;

	Point3 center = Point3::Origin;
	Point3 pt1 = Point3::Origin;
	Point3 pt2 = Point3::Origin;
	Point3 trans_pt1 = Point3::Origin;
	Point3 trans_pt2 = Point3::Origin;
	float r1 = 0.0f;
	float r2 = 0.0f;

	CalcCapsule( mesh, pt1, pt2, r1, r2 );

	center = ( pt1 + pt2 ) / 2;
	trans_pt1 = pt1 - center;
	trans_pt2 = pt2 - center;

	if( Niflib::bhkCapsuleShapeRef shape = new Niflib::bhkCapsuleShape( ) )
	{
		shape->SetRadius( r1 / bhkScale );
		shape->SetRadius1( r1 / bhkScale );
		shape->SetRadius2( r1 / bhkScale );
		shape->SetFirstPoint( TOVECTOR3( trans_pt1 / bhkScale ) );
		shape->SetSecondPoint( TOVECTOR3( trans_pt2 / bhkScale ) );
		shape->SetMaterial( Niflib::HavokMaterial( material ) );

		Matrix3 ltm = TransMatrix( center ) * node->GetObjTMAfterWSM( 0 ) * tm;
		if( ltm.IsIdentity( ) )
			retval = Niflib::StaticCast<Niflib::bhkShape>( shape );

		else
		{
			ltm.SetTrans( ltm.GetTrans( ) / bhkScale );

			Niflib::bhkTransformShapeRef transform = new Niflib::bhkTransformShape( );
			transform->SetTransform( TOMATRIX4( ltm ).Transpose( ) );
			transform->SetShape( shape );
			transform->SetMaterial( material );

			retval = Niflib::StaticCast<Niflib::bhkShape>( transform );
		}
	}

	return retval;
}

Niflib::bhkShapeRef Exporter::makeModConvexShape( INode* node, Modifier* mod, Mesh& mesh, Matrix3& tm, Niflib::HavokMaterial mtlDefault )
{
	enum { havok_params };
	enum { PB_BOUND_TYPE, PB_MATERIAL };
	enum { bv_type_none, bv_type_box, bv_type_sphere, bv_type_capsule, bv_type_shapes, bv_type_convex, bv_type_packed, bv_type_obb, };  // pblock ID

	Niflib::HavokMaterial material = mtlDefault;

	if( IParamBlock2* pblock2 = mod->GetParamBlockByID( havok_params ) )
	{
		material = Niflib::HavokMaterial( pblock2->GetInt( PB_MATERIAL, 0, 0 ) );
		if( material < 0 )
			material = mtlDefault;
	}

	Matrix3 ltm = node->GetObjTMAfterWSM( 0 ) * tm;

	Niflib::bhkShapeRef shape;
	if( Niflib::bhkConvexVerticesShapeRef convShape = makeConvexShape( mesh, ltm, material ) )
	{
		convShape->SetMaterial( material );
		shape = Niflib::StaticCast<Niflib::bhkShape>( convShape );
	}

	return shape;
}

Niflib::bhkShapeRef Exporter::makeModOBBShape( INode* node, Modifier* mod, Mesh& mesh, Matrix3& tm, Niflib::HavokMaterial mtlDefault )
{
	float bhkScale = ( IsSkyrim( ) ) ? Exporter::bhkScaleFactor * 10.0f : Exporter::bhkScaleFactor;

	enum { havok_params };
	enum { PB_BOUND_TYPE, PB_MATERIAL };
	enum { bv_type_none, bv_type_box, bv_type_sphere, bv_type_capsule, bv_type_shapes, bv_type_convex, bv_type_packed, bv_type_obb, };  // pblock ID

	Niflib::HavokMaterial material = mtlDefault;

	if( IParamBlock2* pblock2 = mod->GetParamBlockByID( havok_params ) )
	{
		material = Niflib::HavokMaterial( pblock2->GetInt( PB_MATERIAL, 0, 0 ) );
		if( material < 0 )
			material = mtlDefault;
	}

	Niflib::bhkShapeRef retval;
	if( Niflib::bhkBoxShapeRef shape = new Niflib::bhkBoxShape( ) )
	{
		Matrix3 rtm( true );
		Point3 center;
		float udim;
		float vdim;
		float ndim;

		CalcOrientedBox( mesh, udim, vdim, ndim, center, rtm );

		Niflib::Vector3 dim( udim, vdim, ndim );
		dim /= ( bhkScale * 2 );

		shape->SetDimensions( dim );
		shape->SetMaterial( Niflib::HavokMaterial( material ) );

		Matrix3 ltm = rtm * node->GetNodeTM( 0 ) * tm; // Translation already done in CalcOrientedBox( ).
		if( ltm.IsIdentity( ) )
			retval = Niflib::StaticCast<Niflib::bhkShape>( shape );

		else
		{
			ltm.SetTrans( ltm.GetTrans( ) / bhkScale );

			Niflib::bhkTransformShapeRef transform = new Niflib::bhkTransformShape( );
			transform->SetTransform( TOMATRIX4( ltm ).Transpose( ) );
			transform->SetShape( shape );
			transform->SetMaterial( material );

			retval = Niflib::StaticCast<Niflib::bhkShape>( transform );
		}
	}

	return retval;
}

Niflib::bhkShapeRef Exporter::makeModTriStripShape( INode* node, Modifier* mod, Mesh& mesh, Matrix3& tm, Niflib::HavokMaterial mtlDefault )
{
	enum { havok_params };
	enum { PB_BOUND_TYPE, PB_MATERIAL };
	enum { bv_type_none, bv_type_box, bv_type_sphere, bv_type_capsule, bv_type_shapes, bv_type_convex, bv_type_packed, bv_type_obb, };  // pblock ID

	Niflib::HavokMaterial material = mtlDefault;

	if( IParamBlock2* pblock2 = mod->GetParamBlockByID( havok_params ) )
	{
		material = Niflib::HavokMaterial( pblock2->GetInt( PB_MATERIAL, 0, 0 ) );
		if( material < 0 )
			material = mtlDefault;
	}

	Matrix3 ltm = node->GetObjTMAfterWSM( 0 ) * tm;

	Niflib::bhkShapeRef shape;
	if( Niflib::bhkNiTriStripsShapeRef trishape = makeTriStripsShape( mesh, ltm, material ) )
		shape = Niflib::StaticCast<Niflib::bhkShape>( trishape );

	return shape;
}

Niflib::bhkShapeRef Exporter::makeModPackedTriStripShape( INode* node, Modifier* mod, Mesh& mesh, Matrix3& tm, Niflib::HavokMaterial mtlDefault )
{
	enum { havok_params };
	enum { PB_BOUND_TYPE, PB_MATERIAL };
	enum { bv_type_none, bv_type_box, bv_type_sphere, bv_type_capsule, bv_type_shapes, bv_type_convex, bv_type_packed, bv_type_obb, };  // pblock ID

	Niflib::HavokMaterial material = mtlDefault;

	if( IParamBlock2* pblock2 = mod->GetParamBlockByID( havok_params ) )
	{
		material = Niflib::HavokMaterial( pblock2->GetInt( PB_MATERIAL, 0, 0 ) );
		if( material < 0 )
			material = mtlDefault;
	}

	Matrix3 ltm = node->GetObjTMAfterWSM( 0 ) * tm;
	Niflib::bhkShapeRef shape;

	if( !IsSkyrim( ) )
	{
		if( Niflib::bhkNiTriStripsShapeRef trishape = makeTriStripsShape( mesh, ltm, material ) )
			shape = Niflib::StaticCast<Niflib::bhkShape>( trishape );
	}
	else
	{
		Niflib::bhkMoppBvTreeShapeRef trishape = makeCompressedMeshShape( mesh, ltm, material, Niflib::OL_STATIC, 0 );
		shape = Niflib::StaticCast<Niflib::bhkShape>( trishape );
	}

	return shape;
}

Niflib::bhkShapeRef Exporter::makeModPackedTriStripShape( INode* tnode, Matrix3& tm, Niflib::HavokMaterial mtlDefault )
{
	INodeTab map;
	AccumulateNodesFromGroup( tnode, map );

	return makeModPackedTriStripShape( map, tm, Niflib::HavokMaterial( NP_DEFAULT_HVK_MATERIAL ) );
}

Niflib::bhkShapeRef Exporter::makeModPackedTriStripShape( INodeTab& map, Matrix3& tm, Niflib::HavokMaterial mtlDefault )
{
	float bhkScale = ( IsSkyrim( ) ) ? Exporter::bhkScaleFactor * 10.0f : Exporter::bhkScaleFactor;

	// Need to seperate the vertices based on material
	typedef std::vector<Niflib::Triangle> Triangles;

	// Setup shape data
	std::vector<Niflib::Vector3> verts;
	std::vector<Niflib::Vector3> norms;
	Triangles tris;
	int voff = 0;

	Niflib::HavokMaterial material = mtlDefault;
	std::vector<Niflib::OblivionSubShape> subshapes;

	for( int i = 0; i < map.Count( ); ++i )
	{
		INode* node = map[ i ];

		// skip group heads
		if( node->IsGroupHead( ) || ( node->IsHidden( ) && !mExportHidden ) )
			continue;

		ObjectState os = node->EvalWorldState( 0 );

		int layer = NP_DEFAULT_HVK_LAYER;
		int filter = NP_DEFAULT_HVK_FILTER;
		Mesh* mesh = NULL;

		bool bNodeTransform = true;
		if( os.obj->ClassID( ) == BHKPROXYOBJECT_CLASS_ID )
		{
			enum { list_params, bv_mesh, };  // pblock2 ID
			enum { PB_MATERIAL, PB_MESHLIST, PB_BOUND_TYPE, PB_CENTER, PB_OPT_ENABLE, PB_MAXEDGE, PB_FACETHRESH, PB_EDGETHRESH, PB_BIAS, PB_LAYER, PB_FILTER, };
			enum { bv_type_none, bv_type_box, bv_type_shapes, bv_type_packed, bv_type_convex, };  // pblock ID

			int type = bv_type_none;
			if( IParamBlock2* pblock2 = os.obj->GetParamBlockByID( list_params ) )
			{
				pblock2->GetValue( PB_BOUND_TYPE, 0, type, FOREVER, 0 );
				if( type == bv_type_packed )
				{
					material = Niflib::HavokMaterial( pblock2->GetInt( PB_MATERIAL, 0, 0 ) );
					if( material < 0 )
						material = mtlDefault;

					pblock2->GetValue( PB_FILTER, 0, filter, FOREVER, 0 );
					pblock2->GetValue( PB_LAYER, 0, filter, FOREVER, 0 );

					if( TriObject* tri = (TriObject*) os.obj->ConvertToType( 0, Class_ID( TRIOBJ_CLASS_ID, 0 ) ) )
						mesh = const_cast<Mesh*>( &tri->GetMesh( ) );

					bNodeTransform = false;
				}
			}
		}
		else if( Modifier* mod = GetbhkCollisionModifier( node ) )
		{
			enum { havok_params, opt_params, clone_params, subshape_params };  // pblock ID
			enum { PB_BOUND_TYPE, PB_MATERIAL, PB_OPT_ENABLE, PB_MAXEDGE, PB_FACETHRESH, PB_EDGETHRESH, PB_BIAS, PB_LAYER, PB_FILTER, };
			enum { bv_type_none, bv_type_box, bv_type_sphere, bv_type_capsule, bv_type_shapes, bv_type_convex, bv_type_packed, bv_type_obb, };  // pblock ID

			if( IParamBlock2* pblock2 = mod->GetParamBlockByID( havok_params ) )
			{
				material = Niflib::HavokMaterial( pblock2->GetInt( PB_MATERIAL, 0, 0 ) );
				if( material < 0 )
					material = mtlDefault;

				pblock2->GetValue( PB_FILTER, 0, filter, FOREVER, 0 );
				pblock2->GetValue( PB_LAYER, 0, filter, FOREVER, 0 );
			}

			if( bhkHelperInterface* bhkHelp = (bhkHelperInterface*) mod->GetInterface( BHKHELPERINTERFACE_DESC ) )
				mesh = const_cast<Mesh*>( bhkHelp->GetMesh( ) );
		}
		else
		{
			if( TriObject* tri = (TriObject*) os.obj->ConvertToType( 0, Class_ID( TRIOBJ_CLASS_ID, 0 ) ) )
				mesh = const_cast<Mesh*>( &tri->GetMesh( ) );
		}

		if( mesh == NULL )
			continue;

		Matrix3 ltm = ( node->GetObjTMAfterWSM( 0 ) * tm );
		int vi[ 3 ];

		if( TMNegParity( ltm ) )
		{
			vi[ 0 ] = 2;
			vi[ 1 ] = 1;
			vi[ 2 ] = 0;
		}
		else
		{
			vi[ 0 ] = 0;
			vi[ 1 ] = 1;
			vi[ 2 ] = 2;
		}

		int nvert = mesh->getNumVerts( );
		int nface = mesh->getNumFaces( );
		mesh->buildNormals( );

		for( int i = 0; i < nvert; ++i )
		{
			Point3 vert = ( mesh->getVert( i ) * ltm ) / bhkScale;
			verts.push_back( TOVECTOR3( vert ) );
		}

		for( int i = 0; i < nface; ++i )
		{
			Point3 norm = ( mesh->getFaceNormal( i ) * ltm ) / bhkScale;
			norms.push_back( TOVECTOR3( mesh->getFaceNormal( i ) ) );

			Niflib::Triangle tri;
			Face& face = mesh->faces[ i ];

			tri[ 0 ] = (USHORT) face.getVert( 0 ) + voff;
			tri[ 1 ] = (USHORT) face.getVert( 1 ) + voff;
			tri[ 2 ] = (USHORT) face.getVert( 2 ) + voff;

			tris.push_back( tri );
		}

		voff += nvert;

		Niflib::OblivionSubShape subshape;
		subshape.layer = Niflib::OblivionLayer( layer );
		subshape.material = material < 0 ? mtlDefault : Niflib::HavokMaterial( material );
		subshape.colFilter = filter;
		subshape.numVertices = nvert;

		subshapes.push_back( subshape );
	}

	Niflib::hkPackedNiTriStripsDataRef data = new Niflib::hkPackedNiTriStripsData( );
	data->SetNumFaces( tris.size ( ) );
	data->SetVertices( verts );
	data->SetTriangles( tris );
	data->SetNormals( norms );

	// Setup shape
	Niflib::bhkPackedNiTriStripsShapeRef shape = new Niflib::bhkPackedNiTriStripsShape( );
	shape->SetData( data );

	shape->SetSubShapes( subshapes );
	data->SetSubShapes( subshapes );

	if( TheHavokCode.Initialize( ) )
		return Niflib::StaticCast<Niflib::bhkShape>( makeTreeShape( shape, (Niflib::HavokMaterial)  mtlDefault ) );

	return shape;
}