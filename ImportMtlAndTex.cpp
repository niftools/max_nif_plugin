
#include "stdafx.h"

Texmap* NifImporter::CreateTexture( Niflib::TexDesc& desc )
{
	BitmapManager* bmpMgr = TheManager;
	if( Niflib::NiSourceTextureRef texSrc = desc.source )
	{
		std::wstring filename = wide( texSrc->GetTextureFileName( ) );
		if( bmpMgr->CanImport( filename.c_str( ) ) )
		{
			BitmapTex* bmpTex = NewDefaultBitmapTex( );
			std::wstring name = wide( texSrc->GetName( ) );

			if( name.empty( ) )
			{
				TCHAR buffer[ MAX_PATH ];
				wcscpy( buffer, PathFindFileName( filename.c_str( ) ) );
				PathRemoveExtension( buffer );

				name = buffer;
			}

			bmpTex->SetName( name.c_str( ) );
			bmpTex->SetMapName( const_cast<TCHAR*>( FindImage( filename ).c_str( ) ) );
			bmpTex->SetAlphaAsMono( TRUE );
			bmpTex->SetAlphaSource( ALPHA_NONE ); // TODO: ALPHA_DEFAULT == ALPHA_NONE??

			switch( desc.filterMode )
			{
			case Niflib::FILTER_TRILERP:	bmpTex->SetFilterType( FILTER_PYR ); break;
			case Niflib::FILTER_BILERP:		bmpTex->SetFilterType( FILTER_SAT ); break;
			case Niflib::FILTER_NEAREST:	bmpTex->SetFilterType( FILTER_NADA ); break;
			}

			if( UVGen* uvGen = bmpTex->GetTheUVGen( ) )
			{
				if( uvGen && uvGen->IsStdUVGen( ) )
				{
					StdUVGen* uvg = (StdUVGen*) uvGen;
					uvg->SetMapChannel( desc.uvSet + 1 );
				}

				switch( desc.clampMode )
				{
				case Niflib::WRAP_S_WRAP_T:		uvGen->SetTextureTiling( 3 ); break;
				case Niflib::WRAP_S_CLAMP_T:	uvGen->SetTextureTiling( 1 ); break;
				case Niflib::CLAMP_S_WRAP_T:	uvGen->SetTextureTiling( 2 ); break;
				case Niflib::CLAMP_S_CLAMP_T:	uvGen->SetTextureTiling( 0 ); break;
				}

				if( desc.hasTextureTransform )
				{
					if( RefTargetHandle ref = uvGen->GetReference( 0 ) )
					{
						Niflib::TexCoord trans = desc.translation;
						Niflib::TexCoord tiling = desc.tiling;
						float wangle = TODEG( desc.wRotation );

						setMAXScriptValue( ref, _T( "U_Offset" ), 0, trans.u );
						setMAXScriptValue( ref, _T( "V_Offset" ), 0, trans.v );
						setMAXScriptValue( ref, _T( "U_Tiling" ), 0, tiling.u );
						setMAXScriptValue( ref, _T( "V_Tiling" ), 0, tiling.v );
						setMAXScriptValue( ref, _T( "W_Angle" ), 0, wangle );
					}
				}
			}

			if( showTextures )
			{
				bmpTex->SetMtlFlag( MTL_TEX_DISPLAY_ENABLED, TRUE );
				bmpTex->ActivateTexDisplay( TRUE );
				bmpTex->NotifyDependents( FOREVER, PART_ALL, REFMSG_CHANGE );
			}

			return bmpTex;
		}
	}

	return NULL;
}

Texmap* NifImporter::CreateTexture( Niflib::NiTexturePropertyRef texSrc )
{
	BitmapManager* bmpMgr = TheManager;
	if( Niflib::NiImageRef imgRef = texSrc->GetImage( ) )
	{
		std::wstring filename = wide( imgRef->GetTextureFileName( ) );
		if( bmpMgr->CanImport( filename.c_str( ) ) )
		{
			BitmapTex* bmpTex = NewDefaultBitmapTex( );
			std::wstring name = wide( texSrc->GetName( ) );

			if( name.empty( ) )
			{
				TCHAR buffer[ MAX_PATH ];
				wcscpy( buffer, PathFindFileName( filename.c_str( ) ) );
				PathRemoveExtension( buffer );

				name = buffer;
			}

			bmpTex->SetName( name.c_str( ) );
			bmpTex->SetMapName( const_cast<TCHAR*>( FindImage( filename ).c_str( ) ) );
			bmpTex->SetAlphaAsMono( TRUE );
			bmpTex->SetAlphaSource( ALPHA_NONE );
			bmpTex->SetFilterType( FILTER_PYR );

			if( showTextures )
			{
				bmpTex->SetMtlFlag( MTL_TEX_DISPLAY_ENABLED, TRUE );
				bmpTex->ActivateTexDisplay( TRUE );
				bmpTex->NotifyDependents( FOREVER, PART_ALL, REFMSG_CHANGE );
			}

			if( UVGen* uvGen = bmpTex->GetTheUVGen( ) )
				uvGen->SetTextureTiling( 0 );

			return bmpTex;
		}
	}

	return NULL;
}

// Create a texture from a BSEffectShaderProperty node
Texmap* NifImporter::CreateTexture( Niflib::BSEffectShaderPropertyRef effectShader )
{
	std::wstring filename = wide( effectShader->GetSourceTexture( ) );
	if( filename.empty( ) )
		return NULL;

	BitmapManager* bmpMgr = TheManager;
	if( bmpMgr->CanImport( filename.c_str( ) ) )
	{
		BitmapTex* bmpTex = NewDefaultBitmapTex( );
		std::wstring name = filename;

		if( name.empty( ) )
		{
			TCHAR buffer[ MAX_PATH ];
			wcscpy( buffer, PathFindFileName( filename.c_str( ) ) );
			PathRemoveExtension( buffer );

			name = buffer;
		}

		bmpTex->SetName( name.c_str( ) );
		bmpTex->SetMapName( const_cast<TCHAR*>( FindImage( filename ).c_str( ) ) );
		bmpTex->SetAlphaAsMono( TRUE );
		bmpTex->SetAlphaSource( ALPHA_NONE );
		bmpTex->SetFilterType( FILTER_PYR );

		if( UVGen* uvGen = bmpTex->GetTheUVGen( ) )
		{
			switch( effectShader->GetTextureClampMode( ) )
			{
			case Niflib::WRAP_S_WRAP_T:		uvGen->SetTextureTiling( 3 ); break;
			case Niflib::WRAP_S_CLAMP_T:	uvGen->SetTextureTiling( 1 ); break;
			case Niflib::CLAMP_S_WRAP_T:	uvGen->SetTextureTiling( 2 ); break;
			case Niflib::CLAMP_S_CLAMP_T:	uvGen->SetTextureTiling( 0 ); break;
			}

			if( uvGen && uvGen->IsStdUVGen( ) )
			{
				Niflib::TexCoord offset = effectShader->GetUVOffset( );
				Niflib::TexCoord scale = effectShader->GetUVScale( );

				StdUVGen* stdUVGen = (StdUVGen*) uvGen;

				stdUVGen->SetUOffs( offset.u, 0 );
				stdUVGen->SetVOffs( offset.v, 0 );
				stdUVGen->SetUScl( scale.u, 0 );
				stdUVGen->SetVScl( scale.v, 0 );
			}
		}

		if( showTextures )
		{
			bmpTex->SetMtlFlag( MTL_TEX_DISPLAY_ENABLED, TRUE );
			bmpTex->ActivateTexDisplay( TRUE );
			bmpTex->NotifyDependents( FOREVER, PART_ALL, REFMSG_CHANGE );
		}

		return bmpTex;
	}

	return NULL;
}

// Create a texture and use settings from the BSLightingShaderProperty node
Texmap* NifImporter::CreateTexture( const std::wstring& filename, Niflib::BSLightingShaderPropertyRef lightingShaderRef )
{
	if( filename.empty( ) )
		return NULL;

	BitmapManager* bmpMgr = TheManager;
	if( bmpMgr->CanImport( filename.c_str( ) ) )
	{
		BitmapTex* bmpTex = NewDefaultBitmapTex( );
		std::wstring name = filename;

		if( name.empty( ) )
		{
			TCHAR buffer[ MAX_PATH ];
			wcscpy( buffer, PathFindFileName( filename.c_str( ) ) );
			PathRemoveExtension( buffer );

			name = buffer;
		}

		bmpTex->SetName( name.c_str( ) );
		bmpTex->SetMapName( const_cast<TCHAR*>( FindImage( filename ).c_str( ) ) );
		bmpTex->SetAlphaAsMono( TRUE );
		bmpTex->SetAlphaSource( ALPHA_NONE );
		bmpTex->SetFilterType( FILTER_PYR );

		int i = 0;
		if( UVGen* uvGen = bmpTex->GetTheUVGen( ) )
		{
			switch( lightingShaderRef->GetTextureClampMode( ) )
			{
			case Niflib::WRAP_S_WRAP_T:		uvGen->SetTextureTiling( U_WRAP | V_WRAP ); break;
			case Niflib::WRAP_S_CLAMP_T:	uvGen->SetTextureTiling( U_WRAP ); break;
			case Niflib::CLAMP_S_WRAP_T:	uvGen->SetTextureTiling( V_WRAP ); break;
			case Niflib::CLAMP_S_CLAMP_T:	uvGen->SetTextureTiling( 0 ); break;
			}

			if( uvGen && uvGen->IsStdUVGen( ) )
			{
				Niflib::TexCoord offset = lightingShaderRef->GetUVOffset( );
				Niflib::TexCoord scale = lightingShaderRef->GetUVScale( );

				StdUVGen* stdUVGen = (StdUVGen*) uvGen;

				stdUVGen->SetUOffs( offset.u, 0 );
				stdUVGen->SetVOffs( offset.v, 0 );
				stdUVGen->SetUScl( scale.u, 0 );
				stdUVGen->SetVScl( scale.v, 0 );
			}

			
		}

		if( showTextures )
		{
			bmpTex->SetMtlFlag( MTL_TEX_DISPLAY_ENABLED, TRUE );
			bmpTex->SetMtlFlag( MTL_MEDIT_TILING, TRUE );
			bmpTex->ActivateTexDisplay( TRUE );
			bmpTex->NotifyDependents( FOREVER, PART_ALL, REFMSG_CHANGE );
		}

		return bmpTex;
	}

	return NULL;
}

Texmap* NifImporter::CreateTexture( const std::wstring& filename )
{
	if( filename.empty( ) )
		return NULL;

	BitmapManager* bmpMgr = TheManager;
	if( bmpMgr->CanImport( filename.c_str( ) ) )
	{
		BitmapTex* bmpTex = NewDefaultBitmapTex( );
		std::wstring name = filename;

		if( name.empty( ) )
		{
			TCHAR buffer[ MAX_PATH ];
			wcscpy( buffer, PathFindFileName( filename.c_str( ) ) );
			PathRemoveExtension( buffer );

			name = buffer;
		}

		bmpTex->SetName( name.c_str( ) );
		bmpTex->SetMapName( const_cast<TCHAR*>( FindImage( filename ).c_str( ) ) );
		bmpTex->SetAlphaAsMono( TRUE );
		bmpTex->SetAlphaSource( ALPHA_NONE );
		bmpTex->SetFilterType( FILTER_PYR );

		if( showTextures )
		{
			bmpTex->SetMtlFlag( MTL_TEX_DISPLAY_ENABLED, TRUE );
			bmpTex->ActivateTexDisplay( TRUE );
			bmpTex->NotifyDependents( FOREVER, PART_ALL, REFMSG_CHANGE );
		}

		if( UVGen* uvGen = bmpTex->GetTheUVGen( ) )
			uvGen->SetTextureTiling( 0 );

		return bmpTex;
	}

	return NULL;
}

Texmap* NifImporter::CreateNormalBump( LPCTSTR name, Texmap* nmap )
{
	Texmap* texmap = (Texmap*) gi->CreateInstance( TEXMAP_CLASS_ID, GNORMAL_CLASS_ID );
	if( texmap != NULL )
	{
		TSTR tname = ( name == NULL ) ? FormatText( _T( "Norm %s" ), nmap->GetName( ).data( ) ) : TSTR( name );
		texmap->SetName( tname );
		texmap->SetSubTexmap( 0, nmap );

		return texmap;
	}

	return nmap;
}

Texmap* NifImporter::CreateMask( LPCTSTR name, Texmap* map, Texmap* mask )
{
	Texmap* texmap = (Texmap*) gi->CreateInstance( TEXMAP_CLASS_ID, Class_ID( MASK_CLASS_ID, 0 ) );
	if( texmap != NULL )
	{
		TSTR tname = ( name == NULL ) ? FormatText( _T( "Mask %s" ), map->GetName( ).data( ) ) : TSTR( name );
		texmap->SetName( tname );
		texmap->SetSubTexmap( 0, map );
		texmap->SetSubTexmap( 0, mask );

		return texmap;
	}

	return map;
}

StdMat2* NifImporter::ImportMaterialAndTextures( ImpNode* node, Niflib::NiAVObjectRef avObject )
{
	// Texture
	Niflib::NiGeometryRef geoprop = Niflib::DynamicCast<Niflib::NiGeometry>( avObject );
	std::vector<Niflib::NiPropertyRef> props = geoprop->GetProperties( );
	Niflib::array<2, Niflib::NiPropertyRef> shaderProps = geoprop->GetBSProperties( );

	Niflib::NiMaterialPropertyRef matRef = SelectFirstObjectOfType<Niflib::NiMaterialProperty>( props );
	Niflib::BSShaderPropertyRef shaderRef = SelectFirstObjectOfType<Niflib::BSShaderProperty>( props );
	Niflib::BSEffectShaderPropertyRef effectShaderRef = NULL; //SelectFirstObjectOfType<Niflib::BSEffectShaderProperty>( props );
	Niflib::BSLightingShaderPropertyRef lightingShaderRef = NULL; //SelectFirstObjectOfType<Niflib::BSLightingShaderProperty>( props );

	lightingShaderRef = Niflib::DynamicCast<Niflib::BSLightingShaderProperty>( shaderProps[ 0 ] );
	effectShaderRef = Niflib::DynamicCast<Niflib::BSEffectShaderProperty>( shaderProps[ 0 ] );

	if( matRef != NULL || shaderRef != NULL || effectShaderRef != NULL || lightingShaderRef != NULL )
	{
		StdMat2* m = NewDefaultStdMat( );
		if( matRef != NULL )
			m->SetName( wide( matRef->GetName( ) ).c_str( ) );

		else
			m->SetName( wide( avObject->GetName( ) ).c_str( ) );

		if( showTextures )
			m->SetMtlFlag( MTL_DISPLAY_ENABLE_FLAGS, TRUE );

		// try the civ4 shader first then default back to normal shaders
		//if( ImportNiftoolsShader( node, avObject, m ) )
		//	return m;

		Niflib::NiTexturingPropertyRef texRef = avObject->GetPropertyByType( Niflib::NiTexturingProperty::TYPE );
		Niflib::NiWireframePropertyRef wireRef = avObject->GetPropertyByType( Niflib::NiWireframeProperty::TYPE );
		Niflib::NiAlphaPropertyRef alphaRef = avObject->GetPropertyByType( Niflib::NiAlphaProperty::TYPE );
		Niflib::NiStencilPropertyRef stencilRef = avObject->GetPropertyByType( Niflib::NiStencilProperty::TYPE );
		Niflib::NiShadePropertyRef shadeRef = avObject->GetPropertyByType( Niflib::NiShadeProperty::TYPE );

		if( IsFallout3( ) || IsSkyrim( ) )
		{
			m->SetAmbient( Color( 0.588f, 0.588f, 0.588f ), 0 );
			m->SetDiffuse( Color( 0.588f, 0.588f, 0.588f ), 0 );
			m->SetSpecular( Color( 0.902f, 0.902f, 0.902f ), 0 );
		}
		else if( matRef != NULL )
		{
			m->SetAmbient( TOCOLOR( matRef->GetAmbientColor( ) ), 0 );
			m->SetDiffuse( TOCOLOR( matRef->GetDiffuseColor( ) ), 0 );
			m->SetSpecular( TOCOLOR( matRef->GetSpecularColor( ) ), 0 );
		}

		if( matRef != NULL )
		{
			Color c = TOCOLOR( matRef->GetEmissiveColor( ) );
			if( c.r != 0 || c.b != 0 || c.g != 0 )
			{
				m->SetSelfIllumColorOn( TRUE );
				m->SetSelfIllumColor( c, 0 );
			}

			m->SetShinStr( 0.0, 0 );
			m->SetOpacity( matRef->GetTransparency( ), 0 );
		}

		bool hasShaderAttributes = ( wireRef != NULL ) || ( stencilRef != NULL ) || ( shadeRef != NULL );
		if( m->SupportsShaders( ) && hasShaderAttributes )
		{
			if( Shader* s = m->GetShader( ) )
			{
				if( wireRef != NULL && ( wireRef->GetFlags( ) & 1 ) )
				{
					BOOL value = TRUE;
					m->SetWire( value );
				}

				if( stencilRef != NULL )
				{
					if( stencilRef->GetFaceDrawMode( ) == Niflib::DRAW_BOTH )
					{
						BOOL value = TRUE;
						m->SetTwoSided( value );
					}
				}

				if( shadeRef != NULL && shadeRef->GetFlags( ) & 1 )
					m->SetFaceted( TRUE );
			}
		}

		if( NULL != texRef )
		{
			// Handle Base/Detail ???
			if( texRef->HasTexture( Niflib::DECAL_0_MAP ) )
			{
				if( Texmap* tex = CreateTexture( texRef->GetTexture( Niflib::DECAL_0_MAP ) ) )
					m->SetSubTexmap( ID_DI, tex );

				if( texRef->HasTexture( Niflib::BASE_MAP ) )
				{
					m->LockAmbDiffTex( FALSE );
					if( Texmap* tex = CreateTexture( texRef->GetTexture( Niflib::BASE_MAP ) ) )
						m->SetSubTexmap( ID_AM, tex );
				}
			}
			else if( texRef->HasTexture( Niflib::BASE_MAP ) )
			{
				if( Texmap* tex = CreateTexture( texRef->GetTexture( Niflib::BASE_MAP ) ) )
				{
					m->SetSubTexmap( ID_DI, tex );
					if( showTextures )
						gi->ActivateTexture( tex, m );
				}
			}

			// Handle Bump map
			if( texRef->HasTexture( Niflib::BUMP_MAP ) )
			{
				if( Texmap* tex = CreateTexture( texRef->GetTexture( Niflib::BUMP_MAP ) ) )
					m->SetSubTexmap( ID_BU, CreateNormalBump( NULL, tex ) );
			}

			// Shiny map
			if( texRef->HasTexture( Niflib::GLOSS_MAP ) )
			{
				if( Texmap* tex = CreateTexture( texRef->GetTexture( Niflib::GLOSS_MAP ) ) )
					m->SetSubTexmap( ID_SS, tex );
			}

			// Self illumination
			if( texRef->HasTexture( Niflib::GLOW_MAP ) )
			{
				if( Texmap* tex = CreateTexture( texRef->GetTexture( Niflib::GLOW_MAP ) ) )
					m->SetSubTexmap( ID_SI, tex );
			}

			// Custom shader handling
			int nTex = texRef->GetShaderTextureCount( );
			if( nTex > 0 )
			{
				std::list<Niflib::NiExtraDataRef> data = avObject->GetExtraData( );
				Niflib::NiGeometryRef trigeom = Niflib::DynamicCast<Niflib::NiGeometry>( avObject );

				if( trigeom->HasShader( ) )
				{
					for( std::list<Niflib::NiExtraDataRef>::iterator itr = data.begin( ); itr != data.end( ); ++itr )
					{
						if( Niflib::NiIntegerExtraDataRef idx = Niflib::DynamicCast<Niflib::NiIntegerExtraData>( *itr ) )
						{
							std::wstring name = wide( idx->GetName( ) );
							if( wildmatch( _T( "*Index" ), name ) )
							{
								int shader = idx->GetData( );
								if( shader < nTex )
								{
									if( name == _T( "NormalMapIndex" ) )
									{
										if( Texmap* tex = CreateTexture( texRef->GetShaderTexture( shader ) ) )
											m->SetSubTexmap( ID_BU, CreateNormalBump( NULL, tex ) );
									}
									else if( name == _T( "SpecularIntensity" ) )
									{
										if( Texmap* tex = CreateTexture( texRef->GetShaderTexture( shader ) ) )
											m->SetSubTexmap( ID_SP, CreateNormalBump( NULL, tex ) );
									}
								}
							}
						}
					}
				}
			}
		}

		Niflib::BSShaderTextureSetRef textureSet = NULL;

		if( Niflib::NiTexturePropertyRef tex2Ref = avObject->GetPropertyByType( Niflib::NiTextureProperty::TYPE ) )
		{
			// Handle Base/Detail ???
			if( Texmap* tex = CreateTexture( tex2Ref ) )
				m->SetSubTexmap( ID_DI, tex );
		}

		if( Niflib::BSShaderNoLightingPropertyRef noLightShadeRef = SelectFirstObjectOfType<Niflib::BSShaderNoLightingProperty>( props ) )
		{
			if( Texmap* tex = CreateTexture( wide( noLightShadeRef->GetFileName( ) ) ) )
				m->SetSubTexmap( ID_DI, tex );
		}

		if( Niflib::BSShaderPPLightingPropertyRef ppLightShadeRef = SelectFirstObjectOfType<Niflib::BSShaderPPLightingProperty>( props ) )
			textureSet = ppLightShadeRef->GetTextureSet( );

		if( Niflib::Lighting30ShaderPropertyRef lighting30ShadeRef = SelectFirstObjectOfType<Niflib::Lighting30ShaderProperty>( props ) )
			textureSet = lighting30ShadeRef->GetTextureSet( );

		if( Niflib::SkyShaderPropertyRef skyShadeRef = SelectFirstObjectOfType<Niflib::SkyShaderProperty>( props ) )
		{
			if( Texmap* tex = CreateTexture( wide( skyShadeRef->GetFileName( ) ) ) )
				m->SetSubTexmap( ID_DI, tex );
		}

		if( Niflib::TileShaderPropertyRef tileShadeRef = SelectFirstObjectOfType<Niflib::TileShaderProperty>( props ) )
		{
			if( Texmap* tex = CreateTexture( wide( tileShadeRef->GetFileName( ) ) ) )
				m->SetSubTexmap( ID_DI, tex );
		}

		if( Niflib::TallGrassShaderPropertyRef grassShadeRef = SelectFirstObjectOfType<Niflib::TallGrassShaderProperty>( props ) )
		{
			if( Texmap* tex = CreateTexture( wide( grassShadeRef->GetFileName( ) ) ) )
				m->SetSubTexmap( ID_DI, tex );
		}

		if( lightingShaderRef != NULL )
		{
			textureSet = lightingShaderRef->GetTextureSet( );

			if( Texmap* tex = CreateTexture( wide( textureSet->GetTexture( 0 ) ), lightingShaderRef ) )
				m->SetSubTexmap( ID_DI, tex );

			if( Texmap* tex = CreateTexture( wide( textureSet->GetTexture( 1 ) ), lightingShaderRef ) )
				m->SetSubTexmap( ID_BU, CreateNormalBump( NULL, tex ) );

			if( Texmap* tex = CreateTexture( wide( textureSet->GetTexture( 3 ) ), lightingShaderRef ) )
				m->SetSubTexmap( ID_SI, tex );

			if( Texmap* tex = CreateTexture( wide( textureSet->GetTexture( 4 ) ), lightingShaderRef ) )
				m->SetSubTexmap( ID_RL, tex );

			if( Texmap* tex = CreateTexture( wide( textureSet->GetTexture( 5 ) ), lightingShaderRef ) )
			{
				if( Texmap* mask = CreateTexture( wide( textureSet->GetTexture( 2 ) ), lightingShaderRef ) )
					tex = CreateMask( NULL, tex, mask );

				m->SetSubTexmap( ID_SP, tex );
			}

			if( enableAnimations )
			{
				// Import any texture animations
				ImportMtlAndTexAnimation( lightingShaderRef->GetControllers( ), m );
			}

			textureSet = NULL;
		}

		if( effectShaderRef != NULL )
		{
			// Create the source texture
			if( Texmap* tex = CreateTexture( effectShaderRef ) )
				m->SetSubTexmap( ID_DI, tex );

			// Set the base color
			/*Niflib::Color4 color = effectShaderRef->GetEmissiveColor( );
			m->SetDiffuse( Color( color.r, color.g, color.b ), 0 );*/
			
			if( enableAnimations )
			{
				// Import any texture animations
				ImportMtlAndTexAnimation( effectShaderRef->GetControllers( ), m );
			}
		}

		if( textureSet != NULL )
		{
			if( Texmap* tex = CreateTexture( wide( textureSet->GetTexture( 0 ) ) ) )
				m->SetSubTexmap( ID_DI, tex );

			if( Texmap* tex = CreateTexture( wide( textureSet->GetTexture( 1 ) ) ) )
				m->SetSubTexmap( ID_BU, CreateNormalBump( NULL, tex ) );

			if( Texmap* tex = CreateTexture( wide( textureSet->GetTexture( 3 ) ) ) )
				m->SetSubTexmap( ID_SI, tex );

			if( Texmap* tex = CreateTexture( wide( textureSet->GetTexture( 4 ) ) ) )
				m->SetSubTexmap( ID_RL, tex );

			if( Texmap* tex = CreateTexture( wide( textureSet->GetTexture( 5 ) ) ) )
			{
				if( Texmap* mask = CreateTexture( wide( textureSet->GetTexture( 2 ) ) ) )
					tex = CreateMask( NULL, tex, mask );

				m->SetSubTexmap( ID_SP, tex );
			}
		}

		return m;
	}

	return NULL;
}

bool NifImporter::ImportMaterialAndTextures( ImpNode* node, std::vector<Niflib::NiTriBasedGeomRef>& glist )
{
	MultiMtl* mtl = NULL;
	int isubmtl = 0;

	for( std::vector<Niflib::NiTriBasedGeomRef>::iterator itr = glist.begin( ), end = glist.end( ); itr != end; ++itr, ++isubmtl )
	{
		Niflib::NiTriBasedGeomDataRef triGeomData = Niflib::StaticCast<Niflib::NiTriBasedGeomData>( (*itr)->GetData( ) );
		if( StdMat2* submtl = ImportMaterialAndTextures( node, (*itr) ) )
		{
			if( mtl == NULL )
				mtl = NewDefaultMultiMtl( );

			mtl->SetSubMtlAndName( isubmtl, submtl, submtl->GetName( ) );
		}
	}

	if( mtl != NULL )
	{
		gi->GetMaterialLibrary( ).Add( mtl );
		node->GetINode( )->SetMtl( mtl );

		return true;
	}

	return false;
}

std::wstring NifImporter::FindImage( const std::wstring& name )
{
	TCHAR buffer[ MAX_PATH ];

	// Simply check for fully qualified path
	if( !PathIsRelative( name.c_str( ) ) )
	{
		if( -1 != _waccess( name.c_str( ), 0 ) )
			return std::wstring( buffer );
	}

	if( !path.empty( ) )
	{
		PathCombine( buffer, path.c_str( ), name.c_str( ) ); // try as-is
		if( -1 != _waccess( buffer, 0 ) )
			return std::wstring( buffer );

		// try only filename in nif directory
		PathCombine( buffer, path.c_str( ), PathFindFileName( name.c_str( ) ) );
		if( -1 != _waccess( buffer, 0 ) )
			return std::wstring( buffer );
	}

	if( appSettings != NULL )
		return appSettings->FindImage( name );

	return name;
}