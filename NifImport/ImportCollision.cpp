
#include "stdafx.h"

extern Class_ID BHKLISTOBJECT_CLASS_ID;
extern Class_ID BHKRIGIDBODYMODIFIER_CLASS_ID;
extern Class_ID bhkBoxObject_CLASS_ID;
extern Class_ID BHKCAPSULEOBJECT_CLASS_ID;
extern Class_ID bhkSphereObject_CLASS_ID;

static Class_ID SCUBA_CLASS_ID(0x6d3d77ac, 0x79c939a9);

enum
{
   CAPSULE_RADIUS = 0,
   CAPSULE_HEIGHT = 1,
   CAPSULE_CENTERS = 2,
};

struct CollisionImport
{
	CollisionImport( NifImporter& parent ) : ni( parent ) { }

	NifImporter& ni;

	void			AddShape( INode* rbody, INode* shapeNode );
	
	bool			ImportRigidBody( Niflib::bhkRigidBodyRef rb, INode* node );
	INode*			CreateRigidBody( Niflib::bhkRigidBodyRef body, INode* parent, Matrix3& tm );

	bool			ImportBase( Niflib::bhkRigidBodyRef body, Niflib::bhkShapeRef shape, INode* parent, INode *shapeNode, Matrix3& tm );
	bool			ImportShape( INode *rbody, Niflib::bhkRigidBodyRef body, Niflib::bhkShapeRef shape, INode* parent, Matrix3& tm );
	bool			ImportBox( INode *rbody, Niflib::bhkRigidBodyRef body, Niflib::bhkBoxShapeRef shape, INode *parent, Matrix3& tm );
	bool			ImportSphere( INode *rbody, Niflib::bhkRigidBodyRef body, Niflib::bhkSphereShapeRef shape, INode *parent, Matrix3& tm );
	bool			ImportCapsule( INode *rbody, Niflib::bhkRigidBodyRef body, Niflib::bhkCapsuleShapeRef shape, INode *parent, Matrix3& tm );
	bool			ImportConvexVertices( INode *rbody, Niflib::bhkRigidBodyRef body, Niflib::bhkConvexVerticesShapeRef shape, INode *parent, Matrix3& tm );
	bool			ImportTriStripsShape( INode *rbody, Niflib::bhkRigidBodyRef body, Niflib::bhkNiTriStripsShapeRef shape, INode *parent, Matrix3& tm );
	bool			ImportMoppBvTreeShape( INode *rbody, Niflib::bhkRigidBodyRef body, Niflib::bhkMoppBvTreeShapeRef shape, INode *parent, Matrix3& tm );
	bool			ImportPackedNiTriStripsShape( INode *rbody, Niflib::bhkRigidBodyRef body, Niflib::bhkPackedNiTriStripsShapeRef shape, INode *parent, Matrix3& tm );   
	bool			ImportListShape( INode *rbody, Niflib::bhkRigidBodyRef body, Niflib::bhkListShapeRef shape, INode *parent, Matrix3& tm );
	bool			ImportTransform( INode *rbody, Niflib::bhkRigidBodyRef body, Niflib::bhkTransformShapeRef shape, INode *parent, Matrix3& tm );

	// Skyrim
	bool			ImportCompressedMeshShape( INode* rbody, Niflib::bhkRigidBodyRef body, Niflib::bhkCompressedMeshShapeRef shape, INode* parent, Matrix3& tm );

	INode* ImportCollisionMesh(
		const vector<Niflib::Vector3>& verts, 
		const vector<Niflib::Triangle>& tris,
		const vector<Niflib::Vector3>& norms,
		Matrix3& tm,
		INode *parent
		);

	enum { bv_type_none, bv_type_box, bv_type_sphere, bv_type_capsule, bv_type_shapes, bv_type_convex, bv_type_packed, bv_type_obb, };  // pblock ID
};

// =============================================================
// NifImporter
// =============================================================

bool NifImporter::ImportCollision( Niflib::NiNodeRef node )
{
	bool ok = false;
	if( !enableCollision )
		return false;

	// Currently only support the oblivion bhk basic objects
	Niflib::bhkNiCollisionObjectRef collObj = node->GetCollisionObject( );
	if( collObj )
	{
		Niflib::NiObjectRef body = collObj->GetBody( );
		if( body->IsDerivedType( Niflib::bhkRigidBody::TYPE ) )
		{
			Niflib::bhkRigidBodyRef rbody = Niflib::DynamicCast<Niflib::bhkRigidBody>( body );
			if( Niflib::bhkShapeRef shape = rbody->GetShape( ) )
			{
				INode* node = NULL;
				Niflib::NiNodeRef target = collObj->GetTarget( );

				if( mergeNonAccum && target && wildmatch( _T( "* NonAccum" ), wide( target->GetName( ) ) ) )
					node = FindNode( target->GetParent( ) );

				else if( target && strmatch( wide( target->GetName( ) ), _T( "Scene Root" ) ) )
					node = gi->GetRootNode( );

				else
					node = FindNode( target );

				CollisionImport ci( *this );
				Matrix3 tm( true );
				
				if( INode* body = ci.CreateRigidBody( rbody, node, tm ) )
				{
					if( !ci.ImportShape( body, rbody, shape, node, tm ) )
						gi->DeleteNode( body, FALSE );
				}
			}
		}
	}

	return ok;
}

// =============================================================
// CollisionImport
// =============================================================

INode* CollisionImport::CreateRigidBody( Niflib::bhkRigidBodyRef body, INode* parent, Matrix3& tm )
{
	INode* rbody = NULL;
	if( body == NULL )
		return rbody;

	Niflib::OblivionLayer lyr = body->GetLayer( );
	Niflib::MotionSystem msys = body->GetMotionSystem( );
	Niflib::MotionQuality qtype = body->GetQualityType( );

	float mass = body->GetMass( );
	float lindamp = body->GetLinearDamping( );
	float angdamp = body->GetAngularDamping( );
	float frict = body->GetFriction( );
	float resti = body->GetRestitution( );
	float maxlinvel = body->GetMaxLinearVelocity( );
	float maxangvel = body->GetMaxAngularVelocity( );
	float pendepth = body->GetPenetrationDepth( );
	
	Niflib::Vector4 center = body->GetCenter( );

	SimpleObject2* listObj = (SimpleObject2*) ni.gi->CreateInstance( HELPER_CLASS_ID, BHKLISTOBJECT_CLASS_ID );
	if( listObj != NULL )
	{
		bool isTransform = false;
		if( bhkRigidBodyInterface* irb = (bhkRigidBodyInterface*) listObj->GetInterface( BHKRIGIDBODYINTERFACE_DESC ) )
		{
			irb->SetLayer( lyr, 0 );
			irb->SetMotionSystem( msys, 0 );
			irb->SetQualityType( qtype, 0 );

			irb->SetMass( mass, 0 );
			irb->SetLinearDamping( lindamp, 0 );
			irb->SetAngularDamping( angdamp, 0 );
			irb->SetFriction( frict, 0 );
			irb->SetRestitution( resti, 0 );
			irb->SetMaxLinearVelocity( maxlinvel, 0 );
			irb->SetMaxAngularVelocity( maxangvel, 0 );
			irb->SetPenetrationDepth( pendepth, 0 );

			isTransform = ( body->IsDerivedType( Niflib::bhkRigidBodyT::TYPE ) );
			irb->SetEnableTransform( isTransform ? TRUE : FALSE, 0 );
		}

		TSTR clsName;
		listObj->GetClassName( clsName );

		if( INode* n = ni.CreateImportNode( clsName, listObj, parent ) )
		{
			Point3 pos = TOPOINT3( body->GetTranslation( ) * ni.bhkScaleFactor );
			Quat q = TOQUAT( body->GetRotation( ), true );
			PosRotScaleNode( n, pos, q, 1.0f, prsDefault );

			rbody = n;
			if( isTransform )
			{
				Matrix3 qm( true );
				q.MakeMatrix( qm );
				qm.Translate( pos );
				tm *= qm;
			}
		}
	}

	return rbody;
}

bool CollisionImport::ImportShape( INode* rbody, Niflib::bhkRigidBodyRef body, Niflib::bhkShapeRef shape, INode* parent, Matrix3& tm )
{
	bool ok = false;
	if( shape->IsDerivedType( Niflib::bhkBoxShape::TYPE ) )
		ok |= ImportBox( rbody, body, Niflib::bhkBoxShapeRef( shape ), parent, tm );

	else if( shape->IsDerivedType( Niflib::bhkCapsuleShape::TYPE ) )
		ok |= ImportCapsule( rbody, body, Niflib::bhkCapsuleShapeRef( shape ), parent, tm );

	else if( shape->IsDerivedType( Niflib::bhkSphereShape::TYPE ) )
		ok |= ImportSphere( rbody, body, Niflib::bhkSphereRepShapeRef( shape ), parent, tm );

	else if( shape->IsDerivedType( Niflib::bhkConvexVerticesShape::TYPE ) )
		ok |= ImportConvexVertices( rbody, body, Niflib::bhkConvexVerticesShapeRef( shape ), parent, tm );

	else if( shape->IsDerivedType( Niflib::bhkNiTriStripsShape::TYPE ) )
		ok |= ImportTriStripsShape( rbody, body, Niflib::bhkNiTriStripsShapeRef( shape ), parent, tm );

	else if( shape->IsDerivedType( Niflib::bhkMoppBvTreeShape::TYPE ) )
		ok |= ImportMoppBvTreeShape( rbody, body, Niflib::bhkMoppBvTreeShapeRef( shape ), parent, tm );

	else if( shape->IsDerivedType( Niflib::bhkPackedNiTriStripsShape::TYPE ) )
		ok |= ImportPackedNiTriStripsShape( rbody, body, Niflib::bhkPackedNiTriStripsShapeRef( shape ), parent, tm );

	else if( shape->IsDerivedType( Niflib::bhkListShape::TYPE ) )
		ok |= ImportListShape( rbody, body, Niflib::bhkListShapeRef( shape ), parent, tm );

	else if( shape->IsDerivedType( Niflib::bhkTransformShape::TYPE ) )
		ok |= ImportTransform( rbody, body, Niflib::bhkTransformShapeRef( shape ), parent, tm );

	else if( shape->IsDerivedType( Niflib::bhkCompressedMeshShape::TYPE ) )
		ok |= ImportCompressedMeshShape( rbody, body, Niflib::bhkCompressedMeshShapeRef( shape ), parent, tm  );

	return ok;
}

bool CollisionImport::ImportBase( Niflib::bhkRigidBodyRef body, Niflib::bhkShapeRef shape, INode* parent, INode* shapeNode, Matrix3& tm )
{
	// Now do common post processing for the node
	if( shapeNode != NULL )
	{
		shapeNode->SetName( TSTR( wide( shape->GetType( ).GetTypeName( ) ).c_str( ) ) );
		if( !tm.IsIdentity( ) )
		{
			Point3 pos = tm.GetTrans( );
			Quat rot( tm );

			PosRotScaleNode( shapeNode, pos, rot, 1.0f, prsDefault );
		}

		// Wireframe Red color
		StdMat2* collMat = NewDefaultStdMat( );
		collMat->SetDiffuse( Color( 1.0f, 0.0f, 0.0f ), 0 );
		collMat->SetWire( TRUE );
		collMat->SetFaceted( TRUE );
		
		ni.gi->GetMaterialLibrary( ).Add( collMat );
		shapeNode->SetMtl( collMat );

		shapeNode->SetPrimaryVisibility( FALSE );
		shapeNode->SetSecondaryVisibility( FALSE );
		shapeNode->BoneAsLine( TRUE );
		shapeNode->SetRenderable( FALSE );
		shapeNode->SetWireColor( RGB( 255, 0, 0 ) );

		if( parent )
			parent->AttachChild( shapeNode );

		return true;
	}

	return false;
}

void CollisionImport::AddShape( INode* rbody, INode* shapeNode )
{
	const int PB_MESHLIST = 1;

	if( IParamBlock2* pblock2 = rbody->GetObjectRef( )->GetParamBlockByID( 0 ) )
	{
		int nBlocks = pblock2->Count( PB_MESHLIST );

		pblock2->SetCount( PB_MESHLIST, nBlocks + 1 );
		pblock2->SetValue( PB_MESHLIST, 0, shapeNode, nBlocks );
	}
}

INode* CollisionImport::ImportCollisionMesh( 
	const std::vector<Niflib::Vector3>& verts,
	const std::vector<Niflib::Triangle>& tris,
	const std::vector<Niflib::Vector3>& norms,
	Matrix3& tm,
	INode* parent
	)
{
	INode* returnNode = NULL;
	if( ImpNode* node = ni.i->CreateNode( ) )
	{
		TriObject* triObject = CreateNewTriObject( );
		node->Reference( triObject );

		Mesh& mesh = triObject->GetMesh( );
		INode* tnode = node->GetINode( );

		// Vertex info
		{
			int nVertices = verts.size( );
			mesh.setNumVerts( nVertices );

			for( int i = 0; i < nVertices; ++i )
			{
				Niflib::Vector3 v = verts[ i ] * ni.bhkScaleFactor;
				mesh.verts[ i ].Set( v.x, v.y, v.z );
			}
		}

		// Triangles and texture vertices
		ni.SetTriangles( mesh, tris );

		MNMesh mn( mesh );
		mn.Transform( tm );
		mn.OutToTri( mesh );
		mesh.checkNormals( TRUE );

		ni.i->AddNodeToScene( node );

		returnNode = node->GetINode( );
		returnNode->EvalWorldState( 0 );

		if( parent != NULL )
			parent->AttachChild( tnode, 1 );
	}

	return returnNode;
}

bool CollisionImport::ImportBox( INode *rbody, Niflib::bhkRigidBodyRef body, Niflib::bhkBoxShapeRef shape, INode *parent, Matrix3& tm )
{
	enum { box_params, };
	enum { PB_MATERIAL, PB_LENGTH, PB_WIDTH, PB_HEIGHT, };

	float scaleFactor = ( ni.IsSkyrim( ) ) ? 10.0f : 1.0f;

	Niflib::bhkShapeRef retval;
	if( SimpleObject* obj = (SimpleObject*) ni.gi->CreateInstance( HELPER_CLASS_ID, bhkBoxObject_CLASS_ID ) )
	{
		if( IParamBlock2* pblock2 = obj->GetParamBlockByID( box_params ) )
		{
			float radius = shape->GetRadius( );
			int mtl = shape->GetMaterial( );
			Niflib::Vector3 dim = shape->GetDimensions( ) * scaleFactor;

			pblock2->SetValue( PB_MATERIAL, 0, mtl, 0 );
			pblock2->SetValue( PB_WIDTH, 0, dim.x, 0 );
			pblock2->SetValue( PB_LENGTH, 0, dim.y, 0 );
			pblock2->SetValue( PB_HEIGHT, 0, dim.z, 0 );

			if( INode* n = ni.CreateImportNode( wide( shape->GetType( ).GetTypeName( ) ).c_str( ), obj, parent ) )
			{
				ImportBase( body, shape, parent, n, tm );
				AddShape( rbody, n );

				return true;
			}
		}
	}

	return false;
}

bool CollisionImport::ImportSphere( INode *rbody, Niflib::bhkRigidBodyRef body, Niflib::bhkSphereShapeRef shape, INode *parent, Matrix3& tm )
{
	enum { sphere_params, };
	enum { PB_MATERIAL, PB_RADIUS, PB_SEGS, PB_SMOOTH, };

	float scaleFactor = ( ni.IsSkyrim( ) ) ? 10.0f : 1.0f;

	Niflib::bhkShapeRef retval;
	if( SimpleObject* obj = (SimpleObject*) ni.gi->CreateInstance( HELPER_CLASS_ID, bhkSphereObject_CLASS_ID ) )
	{
		if( IParamBlock2* pblock2 = obj->GetParamBlockByID( sphere_params ) )
		{
			float radius = shape->GetRadius( ) * scaleFactor;
			int mtl = shape->GetMaterial( );

			pblock2->SetValue( PB_MATERIAL, 0, mtl, 0 );
			pblock2->SetValue( PB_RADIUS, 0, radius, 0 );

			if( INode* n = ni.CreateImportNode( wide( shape->GetType( ).GetTypeName( ) ).c_str( ), obj, parent ) )
			{
				ImportBase( body, shape, parent, n, tm );
				AddShape( rbody, n );

				return true;
			}
		}
	}

	return false;
}

bool CollisionImport::ImportCapsule( INode *rbody, Niflib::bhkRigidBodyRef body, Niflib::bhkCapsuleShapeRef shape, INode *parent, Matrix3& tm )
{
	enum { cap_params, };
	enum { PB_MATERIAL, PB_RADIUS1, PB_RADIUS2, PB_LENGTH, };

	float scaleFactor = ( ni.IsSkyrim( ) ) ? 10.0f : 1.0f;

	Niflib::bhkShapeRef retval;
	if( SimpleObject* obj = (SimpleObject*) ni.gi->CreateInstance( HELPER_CLASS_ID, BHKCAPSULEOBJECT_CLASS_ID ) )
	{
		if( IParamBlock2* pblock2 = obj->GetParamBlockByID( cap_params ) )
		{
			float radius = shape->GetRadius( );
			int mtl = shape->GetMaterial( );

			float radius1 = shape->GetRadius1( ) * scaleFactor;
			float radius2 = shape->GetRadius2( ) * scaleFactor;
			Niflib::Vector3 pt1 = shape->GetFirstPoint( ) * scaleFactor;
			Niflib::Vector3 pt2 = shape->GetSecondPoint( ) * scaleFactor;
			float len = ( pt2 - pt1 ).Magnitude( );

			Point3 center = ( TOPOINT3( pt2 + pt1 ) / 2.0f ) * 7.0f;
			Point3 norm = Normalize( TOPOINT3( pt2 - pt1 ) );
			
			Matrix3 mat;
			MatrixFromNormal( norm, mat );
			Matrix3 newTM = tm * mat * TransMatrix( center );

			pblock2->SetValue( PB_MATERIAL, 0, mtl, 0 );
			pblock2->SetValue( PB_LENGTH, 0, len, 0 );
			pblock2->SetValue( PB_RADIUS1, 0, radius1, 0 );
			pblock2->SetValue( PB_RADIUS2, 0, radius2, 0 );

			if( INode* n = ni.CreateImportNode( wide( shape->GetType( ).GetTypeName( ) ).c_str( ), obj, parent ) )
			{
				ImportBase( body, shape, parent, n, newTM );
				AddShape( rbody, n );

				return true;
			}
		}
	}

	return false;
}

bool CollisionImport::ImportConvexVertices( INode *rbody, Niflib::bhkRigidBodyRef body, Niflib::bhkConvexVerticesShapeRef shape, INode *parent, Matrix3& tm )
{
	Matrix3 ltm( true );
	INode* returnNode = NULL;

	std::vector<Niflib::Vector3> verts = shape->GetVertices( );
	std::vector<Niflib::Vector3> norms = shape->GetNormals( );
	std::vector<Niflib::Triangle> tris = Niflib::NifQHull::compute_convex_hull( verts );

	returnNode = ImportCollisionMesh( verts, tris, norms, ltm, parent );

	CreatebhkCollisionModifier( returnNode, bv_type_convex, shape->GetMaterial( ), Niflib::OL_UNIDENTIFIED, 0 );
	ImportBase( body, shape, parent, returnNode, tm );
	AddShape( rbody, returnNode );

	return true;
}

bool CollisionImport::ImportTriStripsShape( INode *rbody, Niflib::bhkRigidBodyRef body, Niflib::bhkNiTriStripsShapeRef shape, INode *parent, Matrix3& tm )
{
	if( shape->GetNumStripsData( ) != 1 )
		return NULL;

	if( ImpNode* node = ni.i->CreateNode( ) )
	{
		TriObject* triObject = CreateNewTriObject( );
		node->Reference( triObject );

		INode* inode = node->GetINode( );

		// Texture
		Mesh& mesh = triObject->GetMesh( );
		Niflib::NiTriStripsDataRef triShapeData = shape->GetStripsData( 0 );

		if( triShapeData == NULL )
			return false;

		// Temporary shape
		Niflib::NiTriStripsRef triShape = new Niflib::NiTriStrips( );
		std::vector<Niflib::Triangle> tris = triShapeData->GetTriangles( );

		ni.ImportMesh( node, triObject, triShape, triShapeData, tris );

		CreatebhkCollisionModifier( inode, bv_type_shapes, shape->GetMaterial( ), Niflib::OL_UNIDENTIFIED, 0 );
		ImportBase( body, shape, parent, inode, tm );
		AddShape( rbody, inode );

		return true;
	}

	return false;
}

bool CollisionImport::ImportMoppBvTreeShape( INode *rbody, Niflib::bhkRigidBodyRef body, Niflib::bhkMoppBvTreeShapeRef shape, INode *parent, Matrix3& tm )
{
	bool retval = ImportShape( rbody, body, shape->GetShape( ), parent, tm );
	return retval;
}

bool CollisionImport::ImportPackedNiTriStripsShape( INode *rbody, Niflib::bhkRigidBodyRef body, Niflib::bhkPackedNiTriStripsShapeRef shape, INode *parent, Matrix3& tm )
{
	if( Niflib::hkPackedNiTriStripsDataRef data = shape->GetData( ) )
	{
		Matrix3 ltm( true );
		std::vector<Niflib::Vector3> verts = data->GetVertices( );
		std::vector<Niflib::Triangle> tris = data->GetTriangles( );
		std::vector<Niflib::Vector3> norms = data->GetNormals( );

		std::vector<Niflib::OblivionSubShape> subshapes = ( ni.IsFallout3( ) || ni.IsSkyrim( ) ) ? shape->GetSubShapes( ) : data->GetSubShapes( );
		if( subshapes.size( ) == 0 )
		{
			// Is this possible?
			INode* inode = ImportCollisionMesh( verts, tris, norms, tm, parent );

			CreatebhkCollisionModifier( inode, bv_type_packed, Niflib::HavokMaterial( NP_DEFAULT_HVK_MATERIAL ), Niflib::OL_UNIDENTIFIED, 0 );
			ImportBase( body, shape, parent, inode, tm );
			AddShape( rbody, inode );
		}
		else
		{
			unsigned int voff = 0;
			unsigned int toff = 0;

			INodeTab nodes;
			for( int i = 0, n = subshapes.size( ); i < n; ++i )
			{
				Niflib::OblivionSubShape& s = subshapes[ i ];

				std::vector<Niflib::Vector3> subverts;
				std::vector<Niflib::Triangle> subtris;
				std::vector<Niflib::Vector3> subnorms;

				subverts.resize( s.numVertices );
				for( unsigned int v = voff; v < ( voff + s.numVertices ); ++v )
					subverts.push_back( verts[ v ] );

				unsigned int vend = ( voff + s.numVertices );

				// TODO: Fix algorithm.  I do not know how to split the triangles here
				//       Basically, greedily take all triangles until next subshape
				//       This is not correct but seems to work with most meshes tested.
				subtris.reserve( s.numVertices / 2 );
				subnorms.reserve( s.numVertices / 2 );

				while( toff < tris.size( ) )
				{
					Niflib::Triangle t = tris[ toff ];
					if( t.v1 >= vend || t.v2 >= vend || t.v3 >= vend )
						break;

					// remove offset for mesh
					t.v1 -= voff;
					t.v2 -= voff;
					t.v3 -= voff;

					subtris.push_back( t );
					subnorms.push_back( norms[ toff ] );

					++toff;
				}

				voff += s.numVertices;

				INode* inode = ImportCollisionMesh( subverts, subtris, subnorms, tm, parent );

				CreatebhkCollisionModifier( inode, bv_type_packed, Niflib::HavokMaterial( s.material ), s.layer, s.colFilter );
				ImportBase( body, shape, parent, inode, tm );

				if( n > 1 )
					inode->SetName( FormatText( _T( "%s:%d" ), _T( "OblivionSubShape" ), i ).data( ) );

				nodes.Append( 1, &inode );
			}

			// TODO: Group nodes on import
			if( nodes.Count( ) > 1 )
			{
				TSTR shapeName = _T( "bhkPackedNiTriStripsShape" );
				INode* group = ni.gi->GroupNodes( &nodes, &shapeName, 0 );
				
				AddShape( rbody, group );
			}
			else if( nodes.Count( )  == 1 )
				AddShape( rbody, nodes[ 0 ] );
		}

		return true;
	}

	return false;
}

bool CollisionImport::ImportListShape( INode *rbody, Niflib::bhkRigidBodyRef body, Niflib::bhkListShapeRef shape, INode *parent, Matrix3& tm )
{
	bool ok = false;
	Niflib::HavokMaterial material = shape->GetMaterial( );

	const int PB_MATERIAL = 0;
	if( IParamBlock2* pblock2 = rbody->GetObjectRef( )->GetParamBlockByID( 0 ) )
		pblock2->SetValue( PB_MATERIAL, 0, material, 0 );

	std::vector<Niflib::bhkShapeRef> bhkshapes = shape->GetSubShapes( );
	for( int i = 0, n = bhkshapes.size( ); i < n; ++i )
		ok |= ImportShape( rbody, body, bhkshapes[ i ], parent, tm );

	return ok;
}

bool CollisionImport::ImportTransform( INode *rbody, Niflib::bhkRigidBodyRef body, Niflib::bhkTransformShapeRef shape, INode *parent, Matrix3& tm )
{
	Niflib::Matrix44 m4 = shape->GetTransform( ).Transpose( );
	Niflib::Vector3 trans;
	Niflib::Matrix33 rot;
	float scale;

	m4.Decompose( trans, rot, scale  );

	Matrix3 wm = TOMATRIX3( rot );
	wm.Translate( TOPOINT3( trans ) * ni.bhkScaleFactor );
	wm *= ScaleMatrix( Point3( scale, scale, scale ) );
	wm = wm * tm;

	return ImportShape( rbody, body, shape->GetShape( ), parent, wm );
}

bool CollisionImport::ImportCompressedMeshShape( INode* rbody, Niflib::bhkRigidBodyRef body, Niflib::bhkCompressedMeshShapeRef shape, INode* parent, Matrix3& tm )
{
	if( Niflib::bhkCompressedMeshShapeDataRef data = shape->GetData( ) )
	{
		if( data->GetNumBigVerts( ) > 0 )
		{
			std::vector<Niflib::Vector4> bigVerts = data->GetBigVerts( );
			std::vector<Niflib::bhkCMSDBigTris> bigTris = data->GetBigTris( );

			std::vector<Niflib::Vector3> verts;
			std::vector<Niflib::Triangle> tris;
			std::vector<Niflib::Vector3> norms;

			norms.resize( bigVerts.size( ) );

			for( int i = 0; i < bigVerts.size( ); i++ )
				verts.push_back( TOVECTOR3( bigVerts[ i ] ) );

			for( int i = 0; i < bigTris.size( ); i++ )
				tris.push_back( Niflib::Triangle( bigTris[ i ].triangle1, bigTris[ i ].triangle2, bigTris[ i ].triangle3 ) );

			Matrix3 wm( true );
			INode* inode = ImportCollisionMesh( verts, tris, norms, wm, parent );

			CreatebhkCollisionModifier( inode, bv_type_packed, Niflib::HavokMaterial( 0 ), Niflib::OL_UNIDENTIFIED, 0 );
			ImportBase( body, shape, parent, inode, tm );
		}

		std::vector<Niflib::bhkCMSDChunk> chunks = data->GetChunks( );
		for( std::vector<Niflib::bhkCMSDChunk>::iterator itr = chunks.begin( ); itr != chunks.end( ); ++itr )
		{
			const Niflib::bhkCMSDChunk chunk = (*itr);
			const Niflib::bhkCMSDMaterial mat = data->GetChunkMaterials( )[ chunk.materialIndex ];
			const Niflib::bhkCMSDTransform transform = data->GetChunkTransforms( )[ chunk.transformIndex ];

			std::vector<Niflib::Vector3> verts;
			std::vector<Niflib::Triangle> tris;
			std::vector<Niflib::Vector3> norms;

			for( int i = 0; i < chunk.vertices.size( ) / 3; i++ )
			{
				Niflib::Vector3 v;

				v.x = ( (float) chunk.vertices[ ( i * 3 ) + 0 ] ) / 1000.0f;
				v.y = ( (float) chunk.vertices[ ( i * 3 ) + 1 ] ) / 1000.0f;
				v.z = ( (float) chunk.vertices[ ( i * 3 ) + 2 ] ) / 1000.0f;

				v.x += chunk.translation.x;
				v.y += chunk.translation.y;
				v.z += chunk.translation.z;

				verts.push_back( v );
				norms.push_back( Niflib::Vector3( 0, 0, 0 ) );
			}

			int faceIdx = 0;
			int offset = 0;
			for( int stripIdx = 0; stripIdx < chunk.strips.size( ); stripIdx++ )
			{
				int numTris = chunk.strips[ stripIdx ] - 2;
				for( int i = 0; i < numTris; i++ )
				{
					Niflib::Triangle t;
					
					if( ( i + 1 ) % 2 == 0 )
					{
						t.v1 = chunk.indices[ faceIdx + 2 ];
						t.v2 = chunk.indices[ faceIdx + 1 ];
						t.v3 = chunk.indices[ faceIdx + 0 ];
					}
					else
					{
						t.v1 = chunk.indices[ faceIdx + 0 ];
						t.v2 = chunk.indices[ faceIdx + 1 ];
						t.v3 = chunk.indices[ faceIdx + 2 ];
					}

					tris.push_back( t );

					faceIdx++;
				}

				offset += chunk.strips[ stripIdx ];
				faceIdx = offset;
			}

			while( faceIdx < chunk.indices.size( ) )
			{
				Niflib::Triangle t;

				t.v1 = chunk.indices[ faceIdx++ ];
				t.v2 = chunk.indices[ faceIdx++ ];
				t.v3 = chunk.indices[ faceIdx++ ];

				tris.push_back( t );
			}

			Matrix3 rt( true );
			Matrix3 wm( true );
			rt.SetRotate( TOQUAT( transform.rotation ) );
			wm.Translate( TOPOINT3( transform.translation ) * ni.bhkScaleFactor );
			wm = rt * wm;

			INode* inode = ImportCollisionMesh( verts, tris, norms, wm, parent );

			CreatebhkCollisionModifier( inode, bv_type_packed, Niflib::HavokMaterial( 0 ), Niflib::OL_UNIDENTIFIED, 0 );
			ImportBase( body, shape, parent, inode, tm );
		}
	}

	return true;
}