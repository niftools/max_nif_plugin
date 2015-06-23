
#include "stdafx.h"

inline bool equals( float a, float b, float thresh ) 
{
   return ( fabsf( a-b ) <= thresh );
}

inline bool equals( const Niflib::Vector3 &a, const Point3 &b, float thresh ) 
{
   return ( fabsf( a.x-b.x ) <= thresh ) && ( fabsf( a.y-b.y ) <= thresh ) && ( fabsf( a.z-b.z ) <= thresh );
}

inline bool equals( const Niflib::Color4 &a, const Niflib::Color4 &b, float thresh )  
{
   return ( fabsf( a.r-b.r ) <= thresh ) && ( fabsf( a.g-b.g ) <= thresh ) && ( fabsf( a.b-b.b ) <= thresh );
}

inline bool equals( const Point3 &a, const Point3 &b, float thresh ) 
{
   return a.Equals( b, thresh );
}

inline bool equals( const Point3 &a, const Point3 &b ) 
{
   return ( a == b );
}

struct VertexCompare
{
   typedef Exporter::VertexGroup VertexGroup;
   typedef Exporter::FaceGroup FaceGroup;

   VertexCompare( FaceGroup& g, float pt, float nt, float vt ) 
      : grp( g ), thresh( pt ), normthresh( nt ), vthresh( vt ) 
   {
         if( normthresh > thresh )
            normthresh = thresh;
         if( vthresh > thresh )
            vthresh = thresh;
   }

   inline bool operator()( const Niflib::TexCoord& lhs, const Niflib::TexCoord& rhs, float thresh ) const 
   {
      return compare( lhs, rhs, thresh ) < 0;
   }

   inline bool operator()( const VertexGroup& lhs, const VertexGroup& rhs ) const 
   {
      return compare( lhs, rhs ) < 0;
   }

   inline bool operator()( int lhs, const VertexGroup& rhs ) const 
   {
      return compare( lhs, rhs ) < 0;
   }

   inline bool operator()( const VertexGroup& lhs, int rhs ) const 
   {
      return compare( lhs, rhs ) < 0;
   }

   inline bool operator()( int lhs, int rhs ) const 
   {
      return compare( lhs, rhs ) < 0;
   }

   inline int compare( float a, float b, float thresh ) const 
   {
      if( equals( a, b, thresh ) ) 
		  return 0;

      return std::less<float>()( a, b ) ? -1 : 1;
   }

   inline int compare( const Point3 &a, const Point3 &b, float thresh ) const 
   {
      int d;
      if( ( d = compare( a.x,b.x,thresh ) ) != 0 ) return d;
      if( ( d = compare( a.y,b.y,thresh ) ) != 0 ) return d;
      if( ( d = compare( a.z,b.z,thresh ) ) != 0 ) return d;

      return 0;
   }

   inline int compare( const Niflib::Vector3 &a, const Point3 &b, float thresh ) const 
   {
      int d;
      if( ( d = compare( a.x,b.x,thresh ) ) != 0 ) return d;
      if( ( d = compare( a.y,b.y,thresh ) ) != 0 ) return d;
      if( ( d = compare( a.z,b.z,thresh ) ) != 0 ) return d;

      return 0;
   }

   inline int compare( const Point3 &a, const Niflib::Vector3 &b, float thresh ) const 
   {
      int d;
      if( ( d = compare( a.x,b.x,thresh ) ) != 0 ) return d;
      if( ( d = compare( a.y,b.y,thresh ) ) != 0 ) return d;
      if( ( d = compare( a.z,b.z,thresh ) ) != 0 ) return d;

      return 0;
   }

   inline int compare( const Niflib::Vector3 &a, const Niflib::Vector3 &b, float thresh ) const 
   {
      int d;
      if( ( d = compare( a.x,b.x,thresh ) ) != 0 ) return d;
      if( ( d = compare( a.y,b.y,thresh ) ) != 0 ) return d;
      if( ( d = compare( a.z,b.z,thresh ) ) != 0 ) return d;

      return 0;
   }

   inline int compare( const VertexGroup& lhs, const VertexGroup& rhs ) const 
   {
      int d;
      if( ( d = compare( lhs.pt,rhs.pt,thresh ) ) != 0 ) return d;
      if( ( d = compare( lhs.norm,rhs.norm,normthresh ) ) != 0) return d;
      if( ( d = compare( lhs.color,rhs.color,vthresh ) ) != 0) return d;
      if( ( d = lhs.uvs.size( ) - rhs.uvs.size( ) ) != 0) return d;

      for( unsigned int i=0; i<lhs.uvs.size( ); ++i ) 
         if( ( d = compare( lhs.uvs[i],rhs.uvs[ i ],vthresh ) ) != 0 ) 
			 return d;

      return 0;
   }

   inline int compare( const Niflib::TexCoord& lhs, const Niflib::TexCoord& rhs, float thresh ) const 
   {
      int d;
      if( ( d = compare( lhs.u,rhs.u,thresh ) ) != 0 ) return d;
      if( ( d = compare( lhs.v,rhs.v,thresh ) ) != 0 ) return d;

      return 0;
   }

   inline int compare( const Niflib::Color4 &a, const Niflib::Color4 &b, float thresh ) const 
   {
      int d;
      if( ( d = compare( a.r,b.r,thresh ) ) != 0 ) return d;
      if( ( d = compare( a.g,b.g,thresh ) ) != 0 ) return d;
      if( ( d = compare( a.b,b.b,thresh ) ) != 0 ) return d;

      return 0;
   }

   inline int compare( int lhs, const VertexGroup& rhs ) const 
   {
      return compare( grp.vgrp[ lhs ], rhs );
   }

   inline int compare( const VertexGroup& lhs, int rhs ) const 
   {
      return compare( lhs, grp.vgrp[ rhs ] );
   }

   inline int compare( int lhs, int rhs ) const 
   {
      return compare( grp.vgrp[ lhs ], grp.vgrp[ rhs ] );
   }

   FaceGroup& grp;
   float thresh, normthresh, vthresh;
};

namespace std
{
	template<>
	struct less<Niflib::Triangle> : public binary_function<Niflib::Triangle, Niflib::Triangle, bool>
	{
		bool operator()( const Niflib::Triangle& s1, const Niflib::Triangle& s2 ) const
		{
			int d = 0;
			if( d == 0 ) d = ( s1[ 0 ] - s2[ 0 ] );
			if( d == 0 ) d = ( s1[ 1 ] - s2[ 1 ] );
			if( d == 0 ) d = ( s1[ 2 ] - s2[ 2 ] );

			return d < 0;
		}
	};

	template<>
	struct less<Niflib::SkinWeight> : public binary_function<Niflib::SkinWeight, Niflib::SkinWeight, bool>
	{
		bool operator()( const Niflib::SkinWeight& lhs, const Niflib::SkinWeight& rhs ) const
		{
			if( lhs.weight == 0.0f )
			{
				if( rhs.weight == 0.0f )
					return rhs.index < lhs.index;

				else
					return true;

				return false;
			}
			else if( rhs.weight == lhs.weight )
				return lhs.index < rhs.index;

			else
				return rhs.weight < lhs.weight;
		}
	};
};

inline Niflib::Triangle& rotate( Niflib::Triangle &t )
{
	if( t[ 1 ] < t[ 0 ] && t[ 1 ] < t[ 2 ] )
		t.Set( t[ 1 ], t [ 2 ], t[ 0 ] );

	else if( t[ 2 ] < t[ 0 ] )
		t.Set( t[ 2 ], t[ 0 ], t[ 1 ] );

	return t;
}

typedef std::map<Niflib::Triangle, int> FaceMap;

static void FillBoneController( Exporter* exporter, Niflib::NiBSBoneLODControllerRef boneCtrl, INode* node )
{
	for( int i = 0; i < node->NumberOfChildren( ); i++ )
	{
		INode* child = node->GetChildNode( i );
		FillBoneController( exporter, boneCtrl, child );

		TSTR upb;
		child->GetUserPropBuffer( upb );

		if( !upb.isNull( ) )
		{
			// Check for bonelod and add bones to bone controller
			stringlist tokens = TokenizeString( upb.data( ), _T( "\r\n" ), true );
			for( stringlist::iterator itr = tokens.begin( ); itr != tokens.end( ); ++itr )
			{
				std::wstring& line = (*itr);
				if( wildmatch( _T( "*#" ), line ) ) // ends with #
				{
					stringlist bonelod = TokenizeString( line.c_str( ), _T( "#" ), true );
					for( stringlist::iterator token = bonelod.begin( ); token != bonelod.end( ); ++token )
					{
						if( wildmatch( _T( "??BoneLOD" ), (*token).c_str( ) ) )
						{
							if( ++token == bonelod.end( ) )
								break;

							int group = 0;
							std::wstringstream str(*token);
							str >> group;

							boneCtrl->AddNodeToGroup( group, exporter->getNode( child ) );
						}
					}
				}
			}
		}
	}
}

void InitializeRigidBody( Niflib::bhkRigidBodyRef body, INode* node )
{
	float value;
	if( node->GetUserPropFloat( _T( "Mass" ), value ) )
		body->SetMass( value );

	if( node->GetUserPropFloat( _T( "Ellasticity" ), value ) )
		body->SetRestitution( value );

	if( node->GetUserPropFloat( _T( "Friction" ), value ) )
		body->SetFriction( value );

	if( node->GetUserPropFloat( _T( "Unyielding" ), value ) )
		body->SetFriction( value );

	body->SetLayer( Niflib::OblivionLayer( Niflib::OL_BIPED | 0x200 ) );
	body->SetLayerCopy( Niflib::OblivionLayer( Niflib::OL_BIPED | 0x200 ) );

	Matrix3 tm = node->GetObjTMAfterWSM( 0 );
	body->SetRotation( TOQUATXYZW( Quat( tm ) ) );
	body->SetTranslation( TOVECTOR4( tm.GetTrans( ) / 7.0f ) );
}

Niflib::NiNodeRef Exporter::exportBone( Niflib::NiNodeRef parent, INode* node )
{
	bool local = !mFlattenHierarchy;
	Niflib::NiNodeRef newParent = makeNode( parent, node, local );

	// Special Skeleton Only handling routine
	if( mSkeletonOnly )
	{
		if( Exporter::mNifVersionInt >= Niflib::VER_10_0_1_0 )
			InitializeTimeController( new Niflib::NiTransformController( ), newParent );

		bool isBoneRoot = false;
		if( IsOblivion( ) || IsFallout3( ) || IsSkyrim( ) )
		{
			// Check for Bone Root
			TSTR upb;
			node->GetUserPropBuffer( upb );
			stringlist tokens = TokenizeString( upb.data( ), _T( "\r\n" ), true );

			for( stringlist::iterator itr = tokens.begin( ); itr != tokens.end( ); ++itr )
			{
				std::wstring& line = (*itr);
				if( wildmatch( _T( "*#" ), line ) )
				{
					stringlist bonelod = TokenizeString( line.c_str( ), _T( "#" ), true );
					for( stringlist::iterator token = bonelod.begin( ); token != bonelod.end( ); ++itr )
					{
						if( wildmatch( _T( "??BoneLOD" ), (*token).c_str( ) ) )
						{
							if( ++token == bonelod.end( ) )
								break;

							if( strmatch( _T( "BoneRoot" ), (*token).c_str( ) ) )
							{
								isBoneRoot = true;
								Niflib::NiBSBoneLODControllerRef boneCtrl = new Niflib::NiBSBoneLODController( );
								InitializeTimeController( boneCtrl, newParent );
								FillBoneController( this, boneCtrl, node );
								break;
							}
						}
					}
				}
			}

			if( !isBoneRoot )
				InitializeTimeController( new Niflib::bhkBlendController( ), newParent );

			if( mGenerateBoneCollision )
			{
				Matrix3 tm = node->GetObjTMAfterWSM( 0 );

				Niflib::bhkShapeRef shape;
				int nc = node->NumberOfChildren( );
				
				if( nc == 0 ) {
					// Nothing
				}
				else if( nc == 1 )
				{
					// Capsule
					INode* child = node->GetChildNode( 0 );
					Matrix3 ctm = Inverse( tm ) * child->GetObjTMAfterWSM( 0 );
					float len = ctm.GetTrans( ).Length( );
					float boxLen = mBoundingBox.Width( ).Length( );
					float ratio = len / boxLen;

					if( ratio < 0.05 ) {
						// Do Nothing
					}
					else if( ratio < 0.15 )
					{
						// Perpendicular Capsule
						Point3 center = ( ctm.GetTrans( ) / 2.0f ) + tm.GetTrans( );
						Matrix3 rtm = tm * RotateXMatrix( TORAD( 90 ) );
						rtm.SetTranslate( center );

						Point3 pt1 = VectorTransform( Point3( len, 0.0f, 0.0f ), rtm );
						Point3 pt2 = VectorTransform( Point3( -len, 0.0f, 0.0f ), rtm );
						float radius = len / 7.0f / 2.0f;

						Niflib::bhkCapsuleShapeRef capsule = new Niflib::bhkCapsuleShape( );
						capsule->SetRadius( radius );
						capsule->SetRadius1( radius );
						capsule->SetRadius2( radius );
						capsule->SetFirstPoint( TOVECTOR3( pt1 / 7.0f ) );
						capsule->SetSecondPoint( TOVECTOR3( pt2 / 7.0f ) );
						capsule->SetMaterial( Niflib::HAV_MAT_SKIN );

						shape = Niflib::StaticCast<Niflib::bhkShape>( capsule );
					}
					else
					{
						// Normal Capsule
						Point3 center = ( ctm.GetTrans( ) / 2.0f ) + tm.GetTrans( );
					}
				}
				else
				{
					// Sphere
					float radius = 0.0f;
					CalcBoundingSphere( node, tm.GetTrans( ), radius, 0 );

					Niflib::bhkSphereRepShapeRef sphere = new Niflib::bhkSphereShape( );
					sphere->SetRadius( radius / 7.0f );
					sphere->SetMaterial( Niflib::HAV_MAT_SKIN );

					shape = Niflib::StaticCast<Niflib::bhkShape>( sphere );
				}

				if( shape )
				{
					Niflib::bhkBlendCollisionObjectRef blendObj = new Niflib::bhkBlendCollisionObject( );
					Niflib::bhkRigidBodyRef body = new Niflib::bhkRigidBody( );

					InitializeRigidBody( body, node );
					body->SetMotionSystem( Niflib::MotionSystem( 6 ) );
					body->SetQualityType( Niflib::MO_QUAL_KEYFRAMED );
					body->SetShape( Niflib::StaticCast<Niflib::bhkShape>( shape ) );
					
					blendObj->SetBody( Niflib::StaticCast<Niflib::NiObject>( body ) );
					newParent->SetCollisionObject( Niflib::StaticCast<Niflib::NiCollisionObject>( blendObj ) );
				}
			}
		}

		if( mExportType != NIF_WO_ANIM && isNodeTracked( node ) )
		{
			Niflib::NiNodeRef accumNode = createAccumNode( newParent, node );
			
			// Transfer collision object to accum and creat blend on accum
			if( IsOblivion( ) || IsFallout3( ) || IsSkyrim( ) )
			{
				InitializeTimeController( new Niflib::bhkBlendController( ), accumNode );
				accumNode->SetCollisionObject( newParent->GetCollisionObject( ) );
				newParent->SetCollisionObject( Niflib::NiCollisionObjectRef( ) );
			}

			newParent = accumNode;
		}
		else if( isSkeletonRoot( node ) )
			newParent = createAccumNode( newParent, node );
	}
	else // normal handling
	{
		// Check for Accum Root using
		if( mExportType == NIF_WO_KF )
		{
			// Add controllers
			if( Exporter::mAllowAccum )
				newParent = createAccumNode( newParent, node );
		}
		else if( mExportType != NIF_WO_ANIM && isNodeTracked( node ) )
			newParent = createAccumNode( newParent, node );

		else if( isSkeletonRoot( node ) )
			newParent = createAccumNode( newParent, node );
	}

	return newParent;
}

Exporter::Result Exporter::exportMesh( Niflib::NiNodeRef& parent, INode* node, TimeValue t )
{
	ObjectState os = node->EvalWorldState( t );
	bool local = !mFlattenHierarchy;

	TriObject* tri = (TriObject*) os.obj->ConvertToType( t, Class_ID( TRIOBJ_CLASS_ID, 0 ) );
	if( !tri )
		return Skip;

	Mesh* copymesh = NULL;
	Mesh* mesh = &tri->GetMesh( );

	Matrix3 mtx( true ), rtx( true );
	if( Exporter::mCollapseTransforms )
	{
		mtx = GetNodeLocalTM( node, t );
		mtx.NoTrans( );
		Quat q( mtx );
		q.MakeMatrix( rtx );

		mesh = copymesh = new Mesh( *mesh );
		{
			unsigned int n = mesh->getNumVerts( );
			for( unsigned int i = 0; i < n; ++i )
			{
				Point3& vert = mesh->getVert( i );
				vert = mtx * vert;
			}

			mesh->checkNormals( TRUE );

#if VERSION_3DSMAX > ((5000<<16)+(15<<8)+0) // Version 6+

			MeshNormalSpec* specNorms = mesh->GetSpecifiedNormals( );
			if( NULL != specNorms )
			{
				specNorms->CheckNormals( );
				for( unsigned int i = 0; i < specNorms->GetNumNormals( ); ++i )
				{
					Point3& norm = specNorms->Normal( i );
					norm = ( rtx * norm ).Normalize( );
				}
			}

#endif

		}
	}

	// Note that calling setVCDisplayData will clear things like normals so we set this up first
	std::vector<Niflib::Color4> vertColors;
	if( mVertexColors )
	{
		bool hasvc = false;
		if( mesh->mapSupport( MAP_ALPHA ) )
		{
			mesh->setVCDisplayData( MAP_ALPHA );
			unsigned int n = mesh->getNumVertCol( );

			if( n > vertColors.size( ) )
				vertColors.assign( n, Niflib::Color4( 1.0f, 1.0f, 1.0f, 1.0f ) );

			VertColor* vertCol = mesh->vertColArray;
			if( vertCol )
			{
				for( unsigned int i = 0; i < n; ++i )
				{
					VertColor c = vertCol[ i ];
					float a = ( c.x + c.y + c.z ) / 3.0f;
					vertColors[ i ].a = a;
					hasvc |= ( a != 1.0f );
				}
			}
		}

		if( mesh->mapSupport( 0 ) )
		{
			mesh->setVCDisplayData( 0 );
			VertColor* vertCol = mesh->vertColArray;

			unsigned int n = mesh->getNumVertCol( );
			if( n > vertColors.size( ) )
				vertColors.assign( n, Niflib::Color4( 1.0f, 1.0f, 1.0f, 1.0f ) );

			if( vertCol )
			{
				for( unsigned int i = 0; i < n; ++i )
				{
					VertColor col = vertCol[ i ];
					vertColors[ i ] = Niflib::Color4( col.x, col.y, col.z, vertColors[ i ].a );
					hasvc |= ( col.x != 1.0f || col.y != 1.0f || col.z != 1.0f );
				}
			}
		}

		if( !hasvc )
			vertColors.clear( );
	}

#if VERSION_3DSMAX <= ((5000<<16)+(15<<8)+0) // Version 5

	mesh->checkNormals( TRUE );

#else

	MeshNormalSpec* specNorms = mesh->GetSpecifiedNormals( );
	if( NULL != specNorms )
	{
		specNorms->CheckNormals( );
		if( specNorms->GetNumNormals( ) == 0 )
			mesh->checkNormals( TRUE );
	}
	else
		mesh->checkNormals( TRUE );

#endif

	Result result = Ok;

	Modifier* geomMorpherMod = GetMorpherModifier( node );
	bool noSplit = FALSE;

	while( 1 )
	{
		FaceGroups grps;
		if( !splitMesh( node, *mesh, grps, t, vertColors, noSplit ) )
		{
			result = Error;
			break;
		}

		bool exportStrips = mTriStrips && ( Exporter::mNifVersionInt > Niflib::VER_4_2_2_0 );

		Niflib::Matrix44 tm = Niflib::Matrix44::IDENTITY;
		if( mExportExtraNodes || ( mExportType != NIF_WO_ANIM && isNodeKeyed( node ) ) )
			tm = TOMATRIX4( getObjectTransform( node, t, false ) * Inverse( getNodeTransform( node, t, false ) ) );

		else
		{
			Niflib::Matrix33 rot;
			Niflib::Vector3 trans;

			objectTransform( rot, trans, node, t, local );
			tm = Niflib::Matrix44( trans, rot, 1.0f );
		}

		tm = TOMATRIX4( Inverse( mtx ) ) * tm;

		TSTR basename = node->NodeName( );
		TSTR format = ( !basename.isNull( ) && grps.size( ) > 1 ) ? _T( "%s:%d" ) : _T( "%s" );

		int i = 1;
		FaceGroups::iterator grp;
		
		for( grp = grps.begin( ); grp != grps.end( ); ++grp, ++i )
		{
			std::wstring name = FormatString( format, basename.data( ), i );
			Niflib::NiTriBasedGeomRef shape = makeMesh( parent, getMaterial( node, grp->first ), grp->second, exportStrips );

			if( shape == NULL )
			{
				result = Error;
				break;
			}

			if( node->IsHidden( ) )
				shape->SetVisibility( false );

			shape->SetName( narrow( name ) );
			shape->SetLocalTransform( tm );

			if( Exporter::mZeroTransforms )
				shape->ApplyTransforms( );

			makeSkin( shape, node, grp->second, t );

			if( geomMorpherMod )
			{
				std::vector<Niflib::Vector3> verts = shape->GetData( )->GetVertices( );
				exportGeomMorpherControl( geomMorpherMod, verts, shape->GetData( )->GetVertexIndices( ), shape );
				shape->GetData( )->SetConsistencyFlags( Niflib::CT_VOLATILE );
			}
		}

		break;
	}

	if( tri != os.obj )
		tri->DeleteMe( );

	if( copymesh )
		delete copymesh;

	return result;
}

bool Exporter::splitMesh( INode* node, Mesh& mesh, FaceGroups& grps, TimeValue t, std::vector<Niflib::Color4>& vertColors, bool noSplit )
{
	Mtl* nodeMtl = node->GetMtl( );
	Matrix3 tm = node->GetObjTMAfterWSM( t );

	// Order of the vertices. Get 'em counter clockwise if the objects are
	// negatively scaled
	int vi[ 3 ];
	if( TMNegParity( tm ) )
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

	Matrix3 flip;
	flip.IdentityMatrix( );
	flip.Scale( Point3( 1, -1, 1 ) );

	int nv = mesh.getNumVerts( );
	int nf = mesh.getNumFaces( );

	if( noSplit )
	{
		int nv = mesh.getNumVerts( );
		int nf = mesh.getNumFaces( );
		
		// Don't split the mesh at all. For debugging purposes.
		FaceGroup& grp = grps[ 0 ];
		grp.vidx.resize( nv, -1 );
		grp.verts.resize( nv );
		grp.faces.resize( nf );
		grp.uvs.resize( nv );
		grp.vnorms.resize( nv );
		grp.fidx.resize( nf );

		Matrix3 texm;
		getTextureMatrix( texm, getMaterial( node, 0 ) );
		texm *= flip;

		for( int face = 0; face < nf; ++face )
		{
			grp.fidx[ face ] = face;
			for( int vi = 0; vi < 3; ++vi )
			{
				int idx = mesh.faces[ face ].getVert( vi );
				grp.faces[ face ][ vi ] = idx;

				// Calculate normal
				Point3 norm;
#if VERSION_3DSMAX <= ((5000<<16)+(15<<8)+0) // Version 5

				norm = getVertexNormal( &mesh, face, mesh.getRVertPtr( idx ) );

#else

				MeshNormalSpec* specNorms = mesh.GetSpecifiedNormals( );
				if( NULL != specNorms && specNorms->GetNumNormals( ) != 0 )
					norm = specNorms->GetNormal( face, vi );

				else
					norm = getVertexNormal( &mesh, face, mesh.getRVertPtr( idx ) );

#endif
				Point3 uv;
				if( mesh.tVerts && mesh.tvFace )
				{
					uv = mesh.tVerts[ mesh.tvFace[ face ].t[ vi ] ] * texm;
					uv.y += 1.0f;
				}

				if( grp.vidx[ idx ] == idx )
					ASSERT( grp.verts[ idx ] == TOVECTOR3( mesh.getVert( idx ) ) );

				else
				{
					grp.vidx[ idx ] = idx;
					grp.verts[ idx ] = TOVECTOR3( mesh.getVert( idx ) );
					grp.vnorms[ idx ] = TOVECTOR3( norm );
				}
			}
		}

		for( int i = 0; i < nv; ++i )
			ASSERT( grp.vidx[ i ] != -1 );
	}
	else
	{
		int face;
		int numSubMtls = nodeMtl ? nodeMtl->NumSubMtls( ) : 0;

		for( face = 0; face < mesh.getNumFaces( ); face++ )
		{
			int mtlID = ( numSubMtls != 0 ) ? ( mesh.faces[ face ].getMatID( ) % numSubMtls ) : 0;
			Mtl* mtl = getMaterial( node, mtlID );

			Matrix3 texm;
			getTextureMatrix( texm, mtl );
			texm *= flip;

			FaceGroup& grp = grps[ mtlID ];

			if( grp.uvMapping.size( ) == 0 ) // Only needs to be done once per face group
			{
				int nmaps = 0;
				int nmapsStart = max( 1, mesh.getNumMaps( ) - ( mesh.mapSupport( 0 ) ? 1 : 0 ) ); // Omit vertex color map.

				for( int ii = 1; ii <= nmapsStart; ii++ ) // Winnow out the unsupported maps.
				{
					if( !mesh.mapSupport( ii ) )
						continue;

					grp.uvMapping[ ii ] = nmaps++;
				}

				grp.uvs.resize( nmaps == 0 ? 1 : nmaps );
			}

			if( nv > int( grp.verts.capacity( ) ) )
			{
				grp.vgrp.reserve( nv );
				grp.verts.reserve( nv );
				grp.vnorms.reserve( nv );

				for( unsigned int i = 0; i < grp.uvs.size( ); ++i )
					grp.uvs[ i ].reserve( nv );

				grp.vcolors.reserve( nv );
				grp.vidx.reserve( nv );
			}

			if( nf > int( grp.faces.capacity( ) ) )
			{
				grp.faces.reserve( nf );
				grp.fidx.reserve( nf );
			}

			Niflib::Triangle tri;
			for( unsigned int i = 0; i < 3; i++ )
				tri[ i ] = addVertex( grp, face, vi[ i ], &mesh, texm, vertColors );

			grp.faces.push_back( tri );

			if( grp.fidx.size( ) < nf )
				grp.fidx.resize( nf, -1 );

			grp.fidx[ face ] = grp.faces.size( ) - 1;
		}
	}

	return true;
}

Niflib::NiTriBasedGeomRef Exporter::makeMesh( Niflib::NiNodeRef& parent, Mtl* mtl, FaceGroup& grp, bool exportStrips )
{
	Niflib::NiTriBasedGeomRef shape;
	Niflib::NiTriBasedGeomDataRef data;

	if( exportStrips )
	{
		shape = new Niflib::NiTriStrips( );
		data = new Niflib::NiTriStripsData( grp.faces, !mUseAlternateStripper );
	}
	else
	{
		shape = new Niflib::NiTriShape( );
		data = new Niflib::NiTriShapeData( grp.faces );
	}

	if( IsFallout3( ) || IsSkyrim( ) )
		shape->SetFlags( 14 );

	data->SetVertices( grp.verts );
	data->SetNormals( grp.vnorms );
	data->SetVertexIndices( grp.vidx );
	data->SetUVSetMap( grp.uvMapping );

	int nUVs = grp.uvs.size( );
	if( IsFallout3( ) || IsSkyrim( ) )
		nUVs = min( 1, nUVs );

	data->SetUVSetCount( nUVs );
	for( int i = 0; i < nUVs; ++i )
		data->SetUVSet( i, grp.uvs[ i ] );

	if( mVertexColors && grp.vcolors.size( ) > 0 )
	{
		bool allWhite = true;
		Niflib::Color4 white( 1.0f, 1.0f, 1.0f, 1.0f );

		for( int i = 0, n = grp.vcolors.size( ); i < n; ++i )
		{
			if( white != grp.vcolors[ i ] )
			{
				allWhite = false;
				break;
			}
		}

		if( !allWhite )
			data->SetVertexColors( grp.vcolors );
	}

	data->SetConsistencyFlags( Niflib::CT_STATIC );

	shape->SetData( data );

	if( Exporter::mTangentAndBinormalExtraData && ( Exporter::mNifVersionInt > Niflib::VER_4_2_2_0 ) )
	{
		// enable traditional tangents and binormals for non-oblivion games
		if( !IsOblivion( ) && ( Exporter::mNifVersionInt >= Niflib::VER_10_0_1_0 ) )
			data->SetTspaceFlag( 16 );

		shape->UpdateTangentSpace( Exporter::mTangentAndBinormalMethod );
	}

	parent->AddChild( Niflib::DynamicCast<Niflib::NiAVObject>( shape ) );

	Niflib::NiAVObjectRef av( Niflib::DynamicCast<Niflib::NiAVObject>( shape ) );
	makeMaterial( av, mtl );
	shape->SetActiveMaterial( 0 );

	return shape;
}

// Callback interface to register a Skin after entire structure is built due to contraints
// in the nif library
struct SkinInstance : public Exporter::NiCallback
{
	typedef std::vector<Niflib::SkinWeight> SkinWeightList;
	typedef std::vector<SkinWeightList> BoneWeightList;

	Exporter* owner;

	// Common data
	Niflib::NiTriBasedGeomRef shape;
	std::vector<Niflib::NiNodeRef> boneList;
	Niflib::NiObject* (*SkinInstConstructor)( );

	// Bone to weight map
	BoneWeightList boneWeights;

	// Fallout 3 dismemberment
	std::vector<Niflib::BodyPartList> partitions;
	std::vector<int> facePartList;

	Matrix3 bone_init_tm;
	Matrix3 node_init_tm;

	SkinInstance( Exporter* Owner )
		: owner( Owner ), SkinInstConstructor( NULL )
	{
	}

	virtual ~SkinInstance( ) { }
	virtual Exporter::Result execute( );
};

bool Exporter::makeSkin( Niflib::NiTriBasedGeomRef shape, INode* node, FaceGroup& grp, TimeValue t )
{
	if( !mExportSkin )
		return false;

	if( grp.verts.empty( ) )
		return false;

	// get the skin modifier
	Modifier* mod = GetSkin( node );
	if( !mod )
		return false;

	ISkin* skin = (ISkin*) mod->GetInterface( I_SKIN );
	if( !skin )
		return false;

	ISkinContextData* skinData = skin->GetContextInterface( node );
	if( !skinData )
		return false;

	if( grp.strips.empty( ) )
		strippify( grp );

	// Create new callback to finish export
	SkinInstance* si = new SkinInstance( this );
	mPostExportCallbacks.push_back( si );

	skin->GetSkinInitTM( node, si->bone_init_tm, false );
	skin->GetSkinInitTM( node, si->node_init_tm, true );

	si->shape = shape;

	// Get bone references (may not actually exist in proper structure at this time)
	int totalBones = skin->GetNumBones( );
	si->boneWeights.resize( totalBones );
	si->boneList.resize( totalBones );

	for( int i = 0; i < totalBones; ++i )
		si->boneList[ i ] = getNode( skin->GetBone( i ) );

	std::vector<int>& vidx = grp.vidx;
	int nv = vidx.size( );

	for( int i = 0; i < nv; ++i )
	{
		int vi = vidx[ i ];
		int nbones = skinData->GetNumAssignedBones( vi );

		for( int j = 0; j < nbones; j++ )
		{
			Niflib::SkinWeight sw;
			sw.index = i;
			sw.weight = skinData->GetBoneWeight( vi, j );
			int boneIndex = skinData->GetAssignedBone( vi, j );

			SkinInstance::SkinWeightList& weights = si->boneWeights[ boneIndex ];
			weights.push_back( sw );
		}
	}

	// remove unused bones
	std::vector<Niflib::NiNodeRef>::iterator bitr = si->boneList.begin( );
	SkinInstance::BoneWeightList::iterator switr = si->boneWeights.begin( );

	for( int i = 0; i < totalBones; ++i )
	{
		std::vector<Niflib::SkinWeight>& weights = (*switr);
		if( weights.empty( ) )
		{
			bitr = si->boneList.erase( bitr );
			switr = si->boneWeights.erase( switr );
		}
		else
		{
			++bitr;
			++switr;
		}
	}

	// Check for dismemberment
	if( IsFallout3( ) || IsSkyrim( ) )
	{
		Modifier* dismemberSkinMod = GetBSDismemberSkin( node );
		if( dismemberSkinMod )
		{
			if( IBSDismemberSkinModifier* disSkin = (IBSDismemberSkinModifier*) dismemberSkinMod->GetInterface( I_BSDISMEMBERSKINMODIFIER ) )
			{
				Tab<IBSDismemberSkinModifierData*> modData = disSkin->GetModifierData( );
				if( modData.Count( ) >= 1 )
				{
					IBSDismemberSkinModifierData* bsdsmd = modData[ 0 ];
					si->SkinInstConstructor = Niflib::BSDismemberSkinInstance::Create;

					Tab<BSDSPartitionData>& flags = bsdsmd->GetPartitionFlags( );
					GenericNamedSelSetList& fselSet = bsdsmd->GetFaceSelList( );

					FaceMap fmap;
					Niflib::NiTriBasedGeomDataRef data = Niflib::DynamicCast<Niflib::NiTriBasedGeomData>( shape->GetData( ) );
					std::vector<Niflib::Triangle> tris = data->GetTriangles( );

					for( unsigned int i = 0; i < tris.size( ); ++i )
					{
						Niflib::Triangle tri = tris[ i ];
						fmap[ rotate( tri ) ] = i;
					}

					// Build up list of partitions and face to partition map
					si->partitions.resize( flags.Count( ) );
					si->facePartList.resize( grp.faces.size( ), -1 );

					for( int i = 0; i < flags.Count( ); ++i )
					{
						Niflib::BodyPartList& bp = si->partitions[ i ];
						bp.bodyPart = (Niflib::BSDismemberBodyPartType) flags[ i ].bodyPart;
						bp.partFlag = (Niflib::BSPartFlag) ( flags[ i ].partFlag | Niflib::PF_START_NET_BONESET );

						BitArray& fSelect = fselSet[ i ];
						for( int j = 0; j < fSelect.GetSize( ); ++j )
						{
							if( fSelect[ j ] )
							{
								Niflib::Triangle tri = grp.faces[ grp.fidx[ j ] ];
								FaceMap::iterator fitr = fmap.find( rotate( tri ) );

								if( fitr != fmap.end( ) )
									si->facePartList[ (*fitr).second ] = i;
							}
						}
					}
				}
			}
		}
	}

	return true;
}

int Exporter::addVertex( FaceGroup& grp, int face, int vi, Mesh* mesh, const Matrix3& texm, std::vector<Niflib::Color4>& vertColors )
{
	VertexGroup vg;
	int vidx;
	
	vidx = vg.idx = mesh->faces[ face ].v[ vi ];
	vg.pt = mesh->verts[ vidx ];

#if VERSION_3DSMAX <= ((5000<<16)+(15<<8)+0) // Version 5

	vg.norm = getVertexNormal( mesh, face, mesh->getRVertPtr( vidx ) );

#else

	MeshNormalSpec* specNorms = mesh->GetSpecifiedNormals( );
	if( NULL != specNorms && specNorms->GetNumNormals( ) != 0 )
		vg.norm = specNorms->GetNormal( face, vi );

	else
		vg.norm = getVertexNormal( mesh, face, mesh->getRVertPtr( vidx ) );

#endif

	int nmaps = grp.uvMapping.size( );
	std::map<int, int>::iterator UVSetIter;
	vg.uvs.resize( nmaps > 0 ? nmaps : 1 );

	if( nmaps > 0 )
	{
		for( UVSetIter = grp.uvMapping.begin( ); UVSetIter != grp.uvMapping.end( ); UVSetIter++ )
		{
			int maxUVIdx = (*UVSetIter).first;
			int nifUVIdx = (*UVSetIter).second;
			Niflib::TexCoord& uvs = vg.uvs[ nifUVIdx ];

			UVVert* uv = mesh->mapVerts( maxUVIdx );
			TVFace* tv = mesh->mapFaces( maxUVIdx );

			if( uv && tv )
			{
				Point3 uvw = uv[ tv[ face ].t[ vi ] ] * texm;
				uvs.u = uvw[ 0 ];
				uvs.v = uvw[ 1 ] + 1.0f;
			}
		}
	}
	else
	{
		if( mesh->tVerts && mesh->tvFace )
		{
			Point3 uvw = mesh->tVerts[ mesh->tvFace[ face ].t[ vi ] ] * texm;
			vg.uvs[ 0 ].u = uvw[ 0 ];
			vg.uvs[ 0 ].v = uvw[ 1 ] + 1.0f;
		}
	}

	vg.color = Niflib::Color4( 1.0f, 1.0f, 1.0f );
	if( mVertexColors && !vertColors.empty( ) )
	{
		TVFace* vcFace = mesh->vcFaceData ? mesh->vcFaceData : mesh->vcFace;
		if( vcFace )
		{
			int vidx = vcFace[ face ].t[ vi ];
			vg.color = vertColors[ vidx ];
		}
	}

	VertexCompare vc( grp, Exporter::mWeldThresh, Exporter::mNormThresh, Exporter::mUVWThresh );
	int n = grp.verts.size( );

#if 0

	IntRange range = std::equal_range( grp.vmap.begin( ), grp.vmap.end( ), vg, vc );
	if( range.first != range.second )
		return (*range.first);

	grp.vmap.insert( range.first, n );

#else

	for( unsigned int i = 0; i < grp.vgrp.size( ); i++ )
		if( vc.compare( vg, i ) == 0 )
			return i;

	grp.vmap.push_back( n );

#endif

	grp.vidx.push_back( vidx );
	grp.vgrp.push_back( vg );
	grp.verts.push_back( TOVECTOR3( vg.pt ) );
	grp.vnorms.push_back( TOVECTOR3( vg.norm ) );

	for( unsigned int i = 0; i < grp.uvs.size( ); ++i )
	{
		TexCoords& uvs = grp.uvs[ i ];
		uvs.push_back( vg.uvs[ i ] );
	}

	grp.vcolors.push_back( vg.color );

	return n;
}

Exporter::Result SkinInstance::execute( )
{
	shape->BindSkinWith( boneList, SkinInstConstructor );
	unsigned int bone = 0;

	for( BoneWeightList::iterator bitr = boneWeights.begin( ); bitr != boneWeights.end( ); ++bitr, ++bone )
		shape->SetBoneWeights( bone, (*bitr) );

	int* faceMap = NULL;
	if( partitions.size( ) > 0 )
	{
		Niflib::BSDismemberSkinInstanceRef dismem = Niflib::DynamicCast<Niflib::BSDismemberSkinInstance>( shape->GetSkinInstance( ) );
		if( dismem != NULL )
			dismem->SetPartitions( partitions );

		faceMap = &facePartList[ 0 ];
	}

	if( Exporter::mNifVersionInt > Niflib::VER_4_0_0_2 )
	{
		if( Exporter::mMultiplePartitions )
			shape->GenHardwareSkinInfo( Exporter::mBonesPerPartition, Exporter::mBonesPerVertex, Exporter::mTriPartStrips, faceMap );

		else
			shape->GenHardwareSkinInfo( 0, 0, Exporter::mTriPartStrips );
	}

	return Exporter::Ok;
}