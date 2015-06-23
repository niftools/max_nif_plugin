
#include "stdafx.h"

void NifImporter::SetTriangles( Mesh& mesh, const std::vector<Niflib::Triangle>& v )
{
	int n = v.size( );
	int nm = mesh.getNumMaps( );

	mesh.setNumFaces( n );
	mesh.setNumTVFaces( nm );

	for( int j = 0; j < nm; ++j )
		mesh.setNumMapFaces( j + 1, n, TRUE );

	for( int i = 0; i < n; ++i )
	{
		const Niflib::Triangle& t = v[ i ];
		Face& f = mesh.faces[ i ];

		f.setVerts( t.v1, t.v2, t.v3 );
		f.Show( );
		f.setEdgeVisFlags( EDGE_VIS, EDGE_VIS, EDGE_VIS );

		TVFace& tf = mesh.tvFace[ i ];
		tf.setTVerts( t.v1, t.v2, t.v3 );

		for( int j = 0; j < nm; ++j )
		{
			if( TVFace* tvFace = mesh.mapFaces( j + 1 ) )
				tvFace[ i ].setTVerts( t.v1, t.v2, t.v3 );
		}
	}
}

void NifImporter::SetNormals( Mesh& mesh, const std::vector<Niflib::Triangle>& tris, const std::vector<Niflib::Vector3>& n )
{
	mesh.checkNormals( TRUE );
	if( n.size( ) > 0 )
	{
		bool needNormals = false;
		for( unsigned int i = 0; i < n.size( ); i++ )
		{
			Niflib::Vector3 v = n[ i ];
			Point3 norm( v.x, v.y, v.z );

			if( norm != mesh.getNormal( i ) )
			{
				needNormals = true;
				break;
			}
		}

		if( needNormals )
		{
#if VERSION_3DSMAX > ((5000<<16)+(15<<8)+0) // Version 5

			mesh.SpecifyNormals( );
			MeshNormalSpec* specNorms = mesh.GetSpecifiedNormals( );

			if( NULL != specNorms )
			{
				specNorms->ClearAndFree( );
				specNorms->SetNumFaces( tris.size( ) );
				specNorms->SetNumNormals( n.size( ) );

				Point3* norms = specNorms->GetNormalArray( );
				for( unsigned int i = 0; i < n.size( ); i++ )
				{
					Niflib::Vector3 v = n[ i ];
					norms[ i ] = Point3( v.x, v.y, v.z );
				}

				MeshNormalFace* pFaces = specNorms->GetFaceArray( );
				for( unsigned int i = 0; i < tris.size( ); i++ )
				{
					const Niflib::Triangle& tri = tris[ i ];

					pFaces[ i ].SpecifyNormalID( 0, tri.v1 );
					pFaces[ i ].SpecifyNormalID( 1, tri.v2 );
					pFaces[ i ].SpecifyNormalID( 2, tri.v3 );
				}
#if VERSION_3DSMAX > ((7000<<16)+(15<<8)+0) // Version 7+

				specNorms->SetAllExplicit( true );

#else

				for( int i = 0; i < specNorms->GetNumNormals( ); ++i )
					specNorms->SetNormalExplicit( i, true );

#endif

				specNorms->CheckNormals( );
			}

#endif
		}
	}
}

bool NifImporter::ImportMesh( Niflib::BSLODTriShapeRef triShape )
{
	bool ok = true;

	ImpNode* node = i->CreateNode( );
	if( !node )
		return false;

	TriObject* triObject = CreateNewTriObject( );
	node->Reference( triObject );

	std::wstring name = wide( triShape->GetName( ) );
	node->SetName( name.c_str( ) );

	INode* inode = node->GetINode( );

	// Texture
	Mesh& mesh = triObject->GetMesh( );
	Niflib::NiTriShapeDataRef triShapeData = Niflib::DynamicCast<Niflib::NiTriShapeData>( triShape->GetData( ) );
	
	if( triShapeData == NULL )
		return false;

	std::vector<Niflib::Triangle> tris = triShapeData->GetTriangles( );
	ok |= ImportMesh( node, triObject, triShape, triShapeData, tris );

	return ok;
}

bool NifImporter::ImportMesh( Niflib::NiTriShapeRef triShape )
{
	bool ok = true;

	ImpNode* node = i->CreateNode( );
	if( !node )
		return false;

	TriObject* triObject = CreateNewTriObject( );
	node->Reference( triObject );

	std::wstring name = wide( triShape->GetName( ) );
	node->SetName( name.c_str( ) );

	INode* inode = node->GetINode( );

	// Texture
	Mesh& mesh = triObject->GetMesh( );
	Niflib::NiTriShapeDataRef triShapeData = Niflib::DynamicCast<Niflib::NiTriShapeData>( triShape->GetData( ) );
	
	if( triShapeData == NULL )
		return false;

	std::vector<Niflib::Triangle> tris = triShapeData->GetTriangles( );
	ok |= ImportMesh( node, triObject, triShape, triShapeData, tris );

	return ok;
}

bool NifImporter::ImportMesh( Niflib::NiTriStripsRef triStrips )
{
	bool ok = true;

	ImpNode* node = i->CreateNode( );
	if( !node )
		return false;

	TriObject* triObject = CreateNewTriObject( );
	node->Reference( triObject );

	std::wstring name = wide( triStrips->GetName( ) );
	node->SetName( name.c_str( ) );

	INode* inode = node->GetINode( );

	// Texture
	Mesh& mesh = triObject->GetMesh( );
	Niflib::NiTriStripsDataRef triStripsData = Niflib::DynamicCast<Niflib::NiTriStripsData>( triStrips->GetData( ) );
	
	if( triStripsData == NULL )
		return false;

	std::vector<Niflib::Triangle> tris = triStripsData->GetTriangles( );
	ok |= ImportMesh( node, triObject, triStrips, triStripsData, tris );

	return ok;
}

bool NifImporter::ImportMesh( ImpNode* node, TriObject* o, Niflib::NiTriBasedGeomRef triGeom, Niflib::NiTriBasedGeomDataRef triGeomData, std::vector<Niflib::Triangle>& tris )
{
	Mesh& mesh = o->GetMesh( );
	INode* tnode = node->GetINode( );

	Niflib::Matrix44 baseTM = ( importBones ) ? triGeom->GetLocalTransform( ) : triGeom->GetWorldTransform( );
	node->SetTransform( 0, TOMATRIX3( baseTM ) );

	// Vertex info
	{
		int nVertices = triGeomData->GetVertexCount( );
		std::vector<Niflib::Vector3> vertices = triGeomData->GetVertices( );
		mesh.setNumVerts( nVertices );

		for( int i = 0; i < nVertices; ++i )
		{
			Niflib::Vector3& v = vertices[ i ];
			mesh.verts[ i ].Set( v.x, v.y, v.z );
		}
	}

	// uv texture info
	{
		int nUVSet = triGeomData->GetUVSetCount( );
		mesh.setNumMaps( nUVSet + 1, TRUE );

		int n = 0;
		int j = 0;

		for( j = 0; j < nUVSet; j++ )
		{
			std::vector<Niflib::TexCoord> texCoords = triGeomData->GetUVSet( j );
			n = texCoords.size( );

			if( j == 0 )
			{
				mesh.setNumTVerts( n, TRUE );
				for( int i = 0; i < n; ++i )
				{
					Niflib::TexCoord& texCoord = texCoords[ i ];
					mesh.tVerts[ i ].Set( texCoord.u, ( flipUVTextures ) ? 1.0f - texCoord.v : texCoord.v, 0 );
				}
			}

			mesh.setMapSupport( j + 1, TRUE );
			mesh.setNumMapVerts( j + 1, n, TRUE );

			if( UVVert* tVerts = mesh.mapVerts( j + 1 ) )
			{
				for( int i = 0; i < n; ++i )
				{
					Niflib::TexCoord& texCoord = texCoords[ i ];
					tVerts[ i ].Set( texCoord.u, ( flipUVTextures ) ? 1.0f - texCoord.v : texCoord.v, 0 );
				}
			}
		}
	}

	// Triangles and texture vertices
	SetTriangles( mesh, tris );
	SetNormals( mesh, tris, triGeomData->GetNormals( ) );

	std::vector<Niflib::Color4> cv = triGeomData->GetColors( );
	int z = cv.size( );
	ImportVertexColor( tnode, o, tris, cv, 0 );

	if( Mtl* m = ImportMaterialAndTextures( node, triGeom ) )
	{
		gi->GetMaterialLibrary( ).Add( m );
		node->GetINode( )->SetMtl( m );
	}

	if( removeDegenerateFaces )
		mesh.RemoveDegenerateFaces( );

	if( removeIllegalFaces )
		mesh.RemoveIllegalFaces( );

	if( enableSkinSupport )
		ImportSkin( node, triGeom );

	if( weldVertices )
		WeldVertices( mesh );

	i->AddNodeToScene( node );

	INode* inode = node->GetINode( );
	inode->EvalWorldState( 0 );

	// attach child
	if( INode* parent = GetNode( triGeom->GetParent( ) ) )
		parent->AttachChild( inode, 1 );

	inode->Hide( triGeom->GetVisibility( ) ? FALSE : TRUE );
	if( enableAutoSmooth )
		mesh.AutoSmooth( TORAD( autoSmoothAngle ), FALSE, FALSE );

	RegisterNode( triGeom, inode );

	return true;
}

bool NifImporter::ImportVertexColor( INode* tnode, TriObject* o, std::vector<Niflib::Triangle>& tris, std::vector<Niflib::Color4> cv, int cv_offset )
{
	bool hasAlpha = false;
	bool hasColor = false;
	Mesh& mesh = o->GetMesh( );

	if( vertexColorMode == 1 ) // Bake into mesh (No modifier)
	{
		int n = cv.size( );
		if( n > 0 )
		{
			std::vector<TVFace> vcFace;
			int nt = tris.size( );
			vcFace.resize( nt );
			mesh.setNumVCFaces( nt );

			for( int i = 0; i < nt; ++i )
			{
				Niflib::Triangle& t = tris[ i ];
				TVFace& vcf = vcFace[ i ];

				vcf.setTVerts( t.v1, t.v2, t.v3 );
				mesh.vcFace[ i ].setTVerts( t.v1, t.v2, t.v3 );
			}

			std::vector<VertColor> vertColors;
			std::vector<VertColor> vertAlpha;

			vertColors.resize( n );
			vertAlpha.resize( n );

			mesh.setNumVertCol( cv.size( ) );

			if( cv.size( ) > 0 )
			{
				hasColor = true;
				hasAlpha = true;
			}

			for( int i = 0; i < n; i++ )
			{
				Niflib::Color4& c = cv[ i ];
				//hasColor |= ( c.r != 1.0f && c.g != 1.0f && c.b != 1.0f );
				vertColors[ i ] = Color( c.r, c.g, c.b );

				//hasAlpha |= ( c.a != 1.0f );
				vertAlpha[ i ] = Color( c.a, c.a, c.a );
			}

			if( hasAlpha )
			{
				mesh.setMapSupport( MAP_ALPHA, TRUE );
				mesh.setNumMapVerts( MAP_ALPHA, n, FALSE );
				mesh.setNumMapFaces( MAP_ALPHA, nt, FALSE );
				mesh.setVCDisplayData( MAP_ALPHA, 0, 0 );

				for( int i = 0; i < nt; i++ )
					mesh.vcFaceData[ i ] = vcFace[ i ];

				for( int i = 0; i < n; i++ )
					mesh.vertColArray[ i ] = vertAlpha[ i ];
			}

			// Add the vertex paint color modifier now
			if( hasAlpha || hasColor )
			{
				mesh.setMapSupport( 0, TRUE );
				mesh.setNumMapVerts( 0, n, TRUE );
				mesh.setNumMapFaces( 0, nt, FALSE );
				mesh.setVCDisplayData( 0, NULL, NULL );

				for( int i =0; i < nt; i++ )
					mesh.vcFaceData[ i ] = vcFace[ i ];

				for( int i = 0; i < n; i++ )
					mesh.vertColArray[ i ] = vertColors[ i ];
			}
		}
	}
	else if( vertexColorMode == 2 )
	{
#if VERSION_3DSMAX > ((5000<<16)+(15<<8)+0) // Version 5

		int n = cv.size();
		if (n > 0)
		{
			std::vector<Color> colorMap;
			std::vector<Color> alphaMap;

			IVertexPaint::VertColorTab vertColors;
			IVertexPaint::VertColorTab vertAlpha;

			vertColors.SetCount( n, TRUE );
			vertAlpha.SetCount( n, TRUE );
			colorMap.resize( n );
			alphaMap.resize( n );
			mesh.setNumVertCol( cv.size( ) );

			for( int i=0; i < n; i++ )
			{
				Niflib::Color4& c = cv[ i ];
				mesh.vertCol[i].Set( c.r, c.g, c.b );

				hasColor |= ( c.r != 1.0f && c.g != 1.0f && c.b != 1.0f );
				colorMap[ i ] = Color( c.r,c.g,c.b );
				vertColors[ i ] = &colorMap[ i ];

				hasAlpha |= ( c.a != 1.0f );
				alphaMap[ i ] = Color( c.a,c.a,c.a );
				vertAlpha[ i ] = &alphaMap[ i ];
			}

			// Civ4 assumes that vcFace is filled in even if only alpha is given via color modifier
			if( hasColor || hasAlpha )
			{
				int n = tris.size( );
				mesh.setNumVCFaces( n );
				for( int i=0; i<n; ++i ) 
				{
					Niflib::Triangle& t = tris[ i ];
					TVFace& vcf = mesh.vcFace[ i ];
					vcf.setTVerts( t.v1, t.v2, t.v3 );
				}
			}

			// Add the Vertex Paint Color modifier now
			if( hasColor )
			{
				IDerivedObject *dobj = CreateDerivedObject( tnode->GetObjectRef( ) );
				Modifier * mod = (Modifier*) CreateInstance( OSM_CLASS_ID, PAINTLAYERMOD_CLASS_ID );
				dobj->AddModifier( mod );
				tnode->SetObjectRef( dobj );
				IVertexPaint* ivertexPaint = (IVertexPaint*) mod->GetInterface( IVERTEXPAINT_INTERFACE_ID );
				ObjectState os = tnode->EvalWorldState( 0 );
				IAssignVertexColors::Options o;
				ivertexPaint->GetOptions( o );
				o.mapChannel = 0;
				o.mixVertColors = true;
				ivertexPaint->SetOptions( o );
				ivertexPaint->SetColors( tnode, vertColors );
			}

			// Add the Vertex Paint Alpha modifier now
			if( hasAlpha )
			{
				IDerivedObject *dobj = CreateDerivedObject( tnode->GetObjectRef( ) );
				Modifier * mod = (Modifier*) CreateInstance( OSM_CLASS_ID, PAINTLAYERMOD_CLASS_ID );
				dobj->AddModifier( mod );
				tnode->SetObjectRef( dobj );
				IVertexPaint* ivertexPaint = (IVertexPaint*) mod->GetInterface( IVERTEXPAINT_INTERFACE_ID );
				ObjectState os = tnode->EvalWorldState( 0 );
				IAssignVertexColors::Options o;
				ivertexPaint->GetOptions( o );
				o.mapChannel = -2;
				o.mixVertColors = true;
				ivertexPaint->SetOptions( o );
				ivertexPaint->SetColors( tnode, vertAlpha );
			}
		}

#endif
	}

	return ( hasAlpha || hasColor );
}

struct VertexHolder
{
	VertexHolder( )
		: vertIndex( 0 ), count( 0 )
	{
	}

	int vertIndex;
	int count;
	Tab<INode*> boneNodeList;
	Tab<float> weights;
};

namespace std
{
   template<>
   struct less<Face> : public binary_function<Face, Face, bool>
   {
      bool operator()(const Face& s1, const Face& s2) const{
         int d = 0;
         if (d == 0) d = (s1.v[0] - s2.v[0]);
         if (d == 0) d = (s1.v[1] - s2.v[1]);
         if (d == 0) d = (s1.v[2] - s2.v[2]);
         return d < 0; 
      }
   };
}

inline Face& rotate(Face &t)
{
   if (t.v[1] < t.v[0] && t.v[1] < t.v[2]) {
      t.setVerts( t.v[1], t.v[2], t.v[0] );
   } else if (t.v[2] < t.v[0]) {
      t.setVerts( t.v[2], t.v[0], t.v[1] );
   }
   return t;
}
struct FaceEquivalence {
   bool operator()(const Face& s1, const Face& s2) const{
      int d = 0;
      if (d == 0) d = (s1.v[0] - s2.v[0]);
      if (d == 0) d = (s1.v[1] - s2.v[1]);
      if (d == 0) d = (s1.v[2] - s2.v[2]);
      return d < 0; 
   }
   bool operator()(const Face* s1, const Face* s2) const{
      return operator()(*s1, *s2);
   }
};

typedef std::map<Face,int, FaceEquivalence> FaceMap;

bool NifImporter::ImportSkin( ImpNode* node, Niflib::NiTriBasedGeomRef triGeom, int v_start )
{
	bool ok = true;
	Niflib::NiSkinInstanceRef nifSkin = triGeom->GetSkinInstance( );
	
	if( !nifSkin )
		return false;

	INode* tnode = node->GetINode( );

	Niflib::NiSkinDataRef data = nifSkin->GetSkinData( );
	Niflib::NiSkinPartitionRef part = nifSkin->GetSkinPartition( );

	std::vector<Niflib::NiNodeRef> nifBones = nifSkin->GetBones( );

	// Create a skin modifier and add it
	Modifier* skinMod = GetOrCreateSkin( tnode );
	TriObject* triObject = GetTriObject( tnode->GetObjectRef( ) );
	Mesh& m = triObject->GetMesh( );

	// get the skin interface
	if( ISkin* skin = (ISkin*) skinMod->GetInterface( I_SKIN ) )
	{
		ISkinImportData* iskinImport = (ISkinImportData*) skinMod->GetInterface( I_SKINIMPORTDATA );

		// Set the num weights to 4.  Yes its in the nif but Shon doesn't like to expose those values 
		//   and the value always seems to be 4 anyway.  I'd also this be more dynamic than hard coded numbers
		//   but I cant figure out the correct values to pass the scripting engine from here so I'm giving up.
		int numWeightsPerVertex = 4;

#if VERSION_3DSMAX >= ((5000<<16)+(15<<8)+0) // Version 5+

		IParamBlock2* params = skinMod->GetParamBlockByID( 2 /* advanced */ );
		params->SetValue( 0x07 /* bone_Limit */, 0, numWeightsPerVertex );

#endif

		// Can get some truly bizarre animations without this in MAX with Civ4 Leaderheads
#if VERSION_3DSMAX > ((5000<<16)+(15<<8)+0) // Version 6+

		BOOL ignore = TRUE;
		params->SetValue( 0x0E /* ignoreBoneScale */, 0, ignore );

#endif

		Matrix3 geom = TOMATRIX3( triGeom->GetLocalTransform( ) );
		Matrix3 m3 = TOMATRIX3( data->GetOverallTransform( ) );
		Matrix3 im3 = Inverse( m3 );
		Matrix3 nm3 = im3 * geom;

		iskinImport->SetSkinTm( tnode, nm3, nm3 );

		// Create bone list
		Tab<INode*> bones;
		for( size_t i = 0; i < nifBones.size( ); ++i )
		{
			Niflib::NiNodeRef bone = nifBones[ i ];
			if( INode* boneRef = FindNode( bone ) )
			{
				bones.Append( 1, &boneRef );
				iskinImport->AddBoneEx( boneRef, TRUE );

				// Set the bone transform
				Matrix3 b3 = TOMATRIX3( data->GetBoneTransform( i ) );
				Matrix3 ib3 = Inverse( b3 );

				ib3 *= geom;

				iskinImport->SetBoneTm( boneRef, ib3, ib3 );
			}
		}

		if( bones.Count( ) != data->GetBoneCount( ) )
			return false;

		ObjectState os = tnode->EvalWorldState( 0 );

		// Need to get a list of bones and weights for each vertex
		std::vector<VertexHolder> vertexHolders;
		vertexHolders.resize( m.numVerts );

		for( int i = 0, n = data->GetBoneCount( ); i < n; ++i )
		{
			if( INode* boneRef = bones[ i ] )
			{
				std::vector<Niflib::SkinWeight> weights = data->GetBoneWeights( i );
				for( std::vector<Niflib::SkinWeight>::iterator itr = weights.begin( ), end = weights.end( ); itr != end; ++itr )
				{
					VertexHolder& h = vertexHolders[ itr->index ];
					h.vertIndex = itr->index;
					
					++h.count;
					h.weights.Append( 1, &itr->weight );
					h.boneNodeList.Append( 1, &boneRef );
				}
			}
		}

		tnode->EvalWorldState( 0 );
		skinMod->DisableModInViews( );
		skinMod->EnableModInViews( );

#if VERSION_3DSMAX < ((5000<<16)+(15<<8)+0) // Version 4

		gi->SetCommandPanelTaskMode( TASK_MODE_MODIFY );
		gi->SelectNode( tnode );

#endif

		// Assign the weights
		for( std::vector<VertexHolder>::iterator itr = vertexHolders.begin( ), end = vertexHolders.end( ); itr != end; ++itr )
		{
			VertexHolder& h = (*itr);
			if( h.count )
			{
				float sum = 0.0f;
				for( int i = 0; i < h.count; ++i )
					sum += h.weights[ i ];

				ASSERT( fabs( sum - 1.0f ) < 0.001f );
				BOOL add = iskinImport->AddWeights( tnode, h.vertIndex, h.boneNodeList, h.weights );
				add = add;
			}
		}

		// This is a kludge to get skin transforms to update and avoid jumping around after modifying the transforms.
		//   Initially they show up incorrectly but magically fix up if you go to the modifier roll up.
		//   There is still an outstanding issue with skeleton and GetObjectTMBeforeWSM.
		skinMod->DisableModInViews( );
		skinMod->EnableModInViews( );

		// If BSDismembermentSkinInstance, ...
		if( Niflib::BSDismemberSkinInstanceRef bsdsi = Niflib::DynamicCast<Niflib::BSDismemberSkinInstance>( nifSkin ) )
		{
			Modifier* dismemberSkinMod = GetOrCreateBSDismemberSkin( tnode );
			if( IBSDismemberSkinModifier *disSkin = (IBSDismemberSkinModifier*) dismemberSkinMod->GetInterface( I_BSDISMEMBERSKINMODIFIER ) )
			{
				// Evaluate node ensure the modifier data is created
				ObjectState os = tnode->EvalWorldState( 0 );

				FaceMap faceMap;
				int nfaces = m.getNumFaces( );

				for( int i = 0; i < nfaces; ++i )
				{
					Face f = m.faces[ i ];
					faceMap[ rotate( f ) ] = i;
				}

				Tab<IBSDismemberSkinModifierData*> modData = disSkin->GetModifierData( );
				for( int i = 0; i < modData.Count( ); ++i )
				{
					IBSDismemberSkinModifierData* bsdsmd = modData[ i ];

					Tab<BSDSPartitionData>& flags = bsdsmd->GetPartitionFlags( );
					std::vector<Niflib::BodyPartList> partitions = bsdsi->GetPartitions( );

					if( partitions.empty( ) )
						continue;

					//bsdsmd->SetActivePartition( partitions.size( ) - 1 );
					for( unsigned int j = 0; j < ( partitions.size( ) - 1 ); ++j )
						bsdsmd->AddPartition( );

					for( unsigned int j = 0; j < partitions.size( ); ++j )
					{
						flags[ j ].bodyPart = (DismemberBodyPartType) partitions[ j ].bodyPart;
						flags[ j ].partFlag = partitions[ j ].partFlag;
					}

					for( int j = 0; j < part->GetNumPartitions( ); ++j )
					{
						bsdsmd->SetActivePartition( j );
						
						dismemberSkinMod->SelectAll( 3 ); // ensures bitarrays are properly synced to mesh
						dismemberSkinMod->ClearSelection( 3 );

						std::vector<Niflib::Triangle> triangles = part->GetTriangles( j );
						std::vector<unsigned short> map = part->GetVertexMap( j );

						GenericNamedSelSetList& fselSet = bsdsmd->GetFaceSelList( );
						if( BitArray* fsel = fselSet.GetSetByIndex( j ) )
						{
							for( std::vector<Niflib::Triangle>::iterator itrtri = triangles.begin( ); itrtri != triangles.end( ); ++itrtri )
							{
								Face f;
								f.setVerts( map[ (*itrtri).v1 ], map[ (*itrtri).v2 ], map[ (*itrtri).v3 ] );
								
								FaceMap::iterator fitr = faceMap.find( rotate( f ) );
								if( fitr != faceMap.end( ) )
									fsel->Set( (*fitr).second );
							}
						}
					}

					bsdsmd->SetActivePartition( 0 );
					disSkin->LocalDataChanged( );
				}
			}
		}
	}

	return ok;
}

void NifImporter::WeldVertices( Mesh& mesh )
{
	MeshDelta tmd( mesh );
	BitArray vTempSel;

	vTempSel.SetSize( mesh.getNumVerts( ) );
	vTempSel.SetAll( );

	tmd.WeldByThreshold( mesh, vTempSel, weldVertexThresh );
	tmd.Apply( mesh );
}

bool NifImporter::ImportMeshes( Niflib::NiNodeRef node )
{
	bool ok = true;
	try
	{
		std::vector<Niflib::NiTriShapeRef> trinodes = Niflib::DynamicCast<Niflib::NiTriShape>( node->GetChildren( ) );
		for( std::vector<Niflib::NiTriShapeRef>::iterator itr = trinodes.begin( ), end = trinodes.end( ); itr != end; ++itr )
			ok |= ImportMesh( *itr );

		std::vector<Niflib::NiTriStripsRef> tristrips = Niflib::DynamicCast<Niflib::NiTriStrips>( node->GetChildren( ) );
		for( std::vector<Niflib::NiTriStripsRef>::iterator itr = tristrips.begin( ), end = tristrips.end( ); itr != end; ++itr )
			ok |= ImportMesh( *itr );

		std::vector<Niflib::BSLODTriShapeRef> bstrinodes = Niflib::DynamicCast<Niflib::BSLODTriShape>( node->GetChildren( ) );
		for( std::vector<Niflib::BSLODTriShapeRef>::iterator itr = bstrinodes.begin( ), end = bstrinodes.end( ); itr != end; ++itr )
			ok |= ImportMesh( *itr );

		std::vector<Niflib::NiNodeRef> nodes = Niflib::DynamicCast<Niflib::NiNode>( node->GetChildren( ) );
		for( std::vector<Niflib::NiNodeRef>::iterator itr = nodes.begin( ), end = nodes.end( ); itr != end; ++itr )
			ok |= ImportMeshes( *itr );
	}
	catch( std::exception& e )
	{
		e = e;
		ok = false;
	}
	catch( ... )
	{
		ok = false;
	}

	return ok;
}