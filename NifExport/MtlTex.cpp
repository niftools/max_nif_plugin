
#include "stdafx.h"

enum 
{ 
	C_BASE, 
	C_DARK, 
	C_DETAIL, 
	C_GLOSS, 
	C_GLOW, 
	C_BUMP, 
	C_NORMAL, 
	C_UNK2, 
	C_DECAL0, 
	C_DECAL1, 
	C_DECAL2, 
	C_ENVMASK, 
	C_ENV, 
	C_HEIGHT, 
	C_REFLECTION, 
	C_OPACITY,
	C_SPECULAR, 
	C_PARALLAX
};

static bool GetTexFullName( Texmap* texMap, TSTR& fName )
{
	if( texMap && texMap->ClassID( ) == Class_ID( BMTEX_CLASS_ID, 0 ) )
	{
		TSTR fileName = ((BitmapTex*) texMap)->GetMapName( );
		if( fileName.isNull( ) )
		{
			fileName = ((BitmapTex*) texMap)->GetFullName( );
			int idx = fileName.last( '(' );

			if( idx >= 0 )
			{
				fileName.remove( idx, fileName.length( ) - idx + 1 );
				while( --idx > 0 )
				{
					if( isspace( fileName[ idx ] ) )
						fileName.remove( idx );
				}
			}
		}

		fName = fileName;
		return true;
	}

	return false;
}

#undef GNORMAL_CLASS_ID
static const Class_ID GNORMAL_CLASS_ID( 0x243e22c6, 0x63f6a014 );


Mtl* Exporter::getMaterial( INode* node, int subMtl )
{
	Mtl* nodeMtl = node->GetMtl( );
	if( nodeMtl )
	{
		int numSub = nodeMtl->NumSubMtls( );
		if( numSub > 0 )
			return nodeMtl->GetSubMtl( subMtl % numSub );

		return nodeMtl;
	}

	return NULL;
}

void Exporter::getTextureMatrix( Matrix3& mat, Mtl* mtl )
{
	BitmapTex* tex = getTexture( mtl );
	if( tex )
		tex->GetUVTransform( mat );

	else
		mat.IdentityMatrix( );
}

BitmapTex* Exporter::getTexture( Mtl* mtl )
{
	if( !mtl )
		return NULL;

	int texMaps = mtl->NumSubTexmaps( );
	if( !texMaps )
		return NULL;

	for( int i = 0; i < texMaps; i++ )
	{
		Texmap* texMap = mtl->GetSubTexmap( i );
		if( !texMap )
			continue;

		if( texMap->ClassID( ) == Class_ID( BMTEX_CLASS_ID, 0 ) )
			return (BitmapTex*) texMap;

		else if( texMap && texMap->ClassID( ) == GNORMAL_CLASS_ID )
		{
			texMap = texMap->GetSubTexmap( 0 );
			if( texMap->ClassID( ) == Class_ID( BMTEX_CLASS_ID, 0 ) )
				return (BitmapTex*) texMap;
		}
	}

	return NULL;
}

BitmapTex* Exporter::getTexture( Mtl* mtl, int i )
{
	if( mtl )
	{
		int texMaps = mtl->NumSubTexmaps( );
		if( i < texMaps )
		{
			if( Texmap* texMap = mtl->GetSubTexmap( i ) )
			{
				if( texMap->ClassID( ) == Class_ID( BMTEX_CLASS_ID, 0 ) )
					return (BitmapTex*) texMap;

				else if( texMap && texMap->ClassID( ) == GNORMAL_CLASS_ID )
				{
					texMap = texMap->GetSubTexmap( 0 );
					if( texMap->ClassID( ) == Class_ID( BMTEX_CLASS_ID, 0 ) )
						return (BitmapTex*) texMap;
				}
			}
		}
	}

	return NULL;
}

void Exporter::makeMaterial( Niflib::NiAVObjectRef& parent, Mtl* mtl )
{
	// Fill-in using the Civ4 Shader if available
	bool done = exportNiftoolsShader( parent, mtl );
	if( done )
		return;

	std::wstring name;
	Niflib::NiMaterialPropertyRef mtlProp( CreateNiObject<Niflib::NiMaterialProperty>( ) );
	
	if( mtl )
	{
		Color c;

		c = mtl->GetAmbient( );
		mtlProp->SetAmbientColor( Niflib::Color3( c.r, c.g, c.b ) );

		c = mtl->GetDiffuse( );
		mtlProp->SetDiffuseColor( Niflib::Color3( c.r, c.g, c.b ) );

		c = mtl->GetSpecular( );
		mtlProp->SetSpecularColor( Niflib::Color3( c.r, c.g, c.b ) );

		c = ( mtl->GetSelfIllumColorOn( ) ) ? mtl->GetSelfIllumColor( ) : Color( 0, 0, 0 );
		mtlProp->SetEmissiveColor( Niflib::Color3( c.r, c.g, c.b ) );

		mtlProp->SetTransparency( 1 );
		mtlProp->SetGlossiness( mtl->GetShininess( ) * 100.0f );

		name = (wchar_t*) mtl->GetName( ).data( );

		if( mtl->ClassID( ) == Class_ID( DMTL_CLASS_ID, 0 ) )
		{
			StdMat2* smtl = (StdMat2*) mtl;
			mtlProp->SetTransparency( smtl->GetOpacity( 0 ) );

			if( smtl->SupportsShaders( ) )
			{
				if( Shader* s = smtl->GetShader( ) )
				{
					if( smtl->GetWire( ) )
					{
						Niflib::NiWireframePropertyRef wireProp = new Niflib::NiWireframeProperty( );
						wireProp->SetFlags( 1 );

						parent->AddProperty( wireProp );
					}

					if( smtl->GetTwoSided( ) )
					{
						Niflib::NiStencilPropertyRef stencil = new Niflib::NiStencilProperty( );
						stencil->SetStencilFunction( Niflib::TEST_GREATER );
						stencil->SetStencilState( false );
						stencil->SetPassAction( Niflib::ACTION_INCREMENT );
						stencil->SetFaceDrawMode( Niflib::DRAW_BOTH );
						stencil->SetFlags( 19840 );

						parent->AddProperty( stencil );
					}

					if( smtl->IsFaceted( ) )
					{
						Niflib::NiShadePropertyRef shade = CreateNiObject<Niflib::NiShadeProperty>( );
						shade->SetFlags( 0 );

						parent->AddProperty( shade );
					}
				}
			}
		}
	}
	else
	{
		mtlProp->SetAmbientColor( Niflib::Color3( 0.588f, 0.588f, 0.588f ) );
		mtlProp->SetDiffuseColor( Niflib::Color3( 1, 1, 1 ) );
		mtlProp->SetSpecularColor( Niflib::Color3( 0.9f, 0.9f, 0.9f ) );
		mtlProp->SetEmissiveColor( Niflib::Color3( 0, 0, 0 ) );
		mtlProp->SetTransparency( 1 );
		mtlProp->SetGlossiness( 10 );

		name = _T( "default" );
	}

	mtlProp->SetName( narrow( name ) );

	Niflib::NiPropertyRef prop = Niflib::DynamicCast<Niflib::NiProperty>( mtlProp );

	makeTexture( parent, mtl );
}

bool Exporter::exportNiftoolsShader( Niflib::NiAVObjectRef parent, Mtl* mtl )
{
	if( !mtl )
		return false;

	RefTargetHandle ref = mtl->GetReference( 2 /*shader*/ );
	if( !ref )
		return false;

	const Class_ID civ4shader( 0x670a77d0, 0x23ab5c7f );
	const Class_ID NIFSHADER_CLASS_ID( 0x566e8ccb, 0xb091bd48 );

	TSTR shaderByName;
	Class_ID shaderID = Class_ID( 0, 0 );

	if( mtl->ClassID( ) == Class_ID( DMTL_CLASS_ID, 0 ) )
	{
		StdMat2* smtl = (StdMat2*) mtl;
		if( smtl->SupportsShaders( ) )
		{
			if( Shader* s = smtl->GetShader( ) )
			{
				s->GetClassName( shaderByName );
				shaderID = s->ClassID( );
			}
		}
	}

	if( shaderID != NIFSHADER_CLASS_ID && shaderID != civ4shader )
	{
		if( shaderByName != TSTR( _T( "CivilizationIV Shader" ) ) )
			return false;
	}

	bool isGamebryoShader = ( shaderID == civ4shader );
	bool isCiv4Shader = isGamebryoShader && ( shaderByName == TSTR( _T( "CivilizationIV Shader" ) ) );

	Color ambient = Color( 0.0f, 0.0f, 0.0f );
	Color diffuse = Color( 0.0f, 0.0f, 0.0f );
	Color specular = Color( 0.0f, 0.0f, 0.0f );
	Color emittance = Color( 0.0f, 0.0f, 0.0f );

	float shininess = 0.0f;
	float alpha = 0.0f;
	float Magnitude = 0.0f;
	float LumaScale = 0.0f;
	float LumaOffset = 0.0f;

	int TestRef = 0;
	int srcBlend = 0;
	int destBlend = 0;
	int TestMode = 0;

	bool AlphaTestEnable = false;

	int ApplyMode = 0;
	int SrcVertexMode = 0;
	int LightingMode = 0;

	bool VertexColorsEnable = false;
	bool SpecularEnable = false;
	bool NoSorter = false;
	bool Dither = false;

	int alphaMode = 0;
	int BaseTextureExport = 0;
	int DarkTextureExport = 0;
	int DetailTextureExport = 0;
	int Decal1TextureExport = 0;
	int Decal2TextureExport = 0;
	int GlossTextureExport = 0;
	int GlowTextureExport = 0;

	LPWSTR CustomShader = NULL;

	int ShaderViewerTechnique = 0;
	int ShaderExportTechnique = 0;

	bool UseNormalMaps = false;

	int NormalMapTechnique = 0;

	bool ok = true;

	if( ok ) ok &= getMAXScriptValue( ref, _T( "ambient" ), 0, ambient );
	if( ok ) ok &= getMAXScriptValue( ref, _T( "diffuse" ), 0, diffuse );
	if( ok ) ok &= getMAXScriptValue( ref, _T( "specular" ), 0, specular );
	if( ok ) ok &= getMAXScriptValue( ref, _T( "emittance" ), 0, emittance );
	if( ok ) ok &= getMAXScriptValue( ref, _T( "shininess" ), 0, shininess );
	if( ok ) ok &= getMAXScriptValue( ref, _T( "alpha" ), 0, alpha );
	if( ok ) ok &= getMAXScriptValue( ref, _T( "Bump_Map_Magnitude" ), 0, Magnitude );
	if( ok ) ok &= getMAXScriptValue( ref, _T( "Bump_Map_Luma_Scale" ), 0, LumaScale );
	if( ok ) ok &= getMAXScriptValue( ref, _T( "Bump_Map_Luma_offset" ), 0, LumaOffset );
	if( ok ) ok &= getMAXScriptValue( ref, _T( "TestRef" ), 0, TestRef );
	if( ok ) ok &= getMAXScriptValue( ref, _T( "AlphaTestEnable" ), 0, AlphaTestEnable );
	if( ok ) ok &= getMAXScriptValue( ref, _T( "Vertex_Color_Enable" ), 0, VertexColorsEnable );
	if( ok ) ok &= getMAXScriptValue( ref, _T( "SpecularEnable" ), 0, SpecularEnable );
	if( ok ) ok &= getMAXScriptValue( ref, _T( "NoSorter" ), 0, NoSorter );
	if( ok ) ok &= getMAXScriptValue( ref, _T( "Dither" ), 0, Dither );
	if( ok ) ok &= getMAXScriptValue( ref, _T( "srcBlend" ), 0, srcBlend );
	if( ok ) ok &= getMAXScriptValue( ref, _T( "destBlend" ), 0, destBlend );
	if( ok ) ok &= getMAXScriptValue( ref, _T( "TestMode" ), 0, TestMode );
	if( ok ) ok &= getMAXScriptValue( ref, _T( "ApplyMode" ), 0, ApplyMode );
	if( ok ) ok &= getMAXScriptValue( ref, _T( "SourceVertexMode" ), 0, SrcVertexMode );
	if( ok ) ok &= getMAXScriptValue( ref, _T( "LightingMode" ), 0, LightingMode );
	if( ok ) ok &= getMAXScriptValue( ref, _T( "alphaMode" ), 0, alphaMode );
	if( ok ) ok &= getMAXScriptValue( ref, _T( "CustomShader" ), 0, CustomShader );

	if( ok ) // civ4 shader
	{
		if( mVertexColors &&  VertexColorsEnable )
		{
			Niflib::NiVertexColorPropertyRef vertexColor = CreateNiObject<Niflib::NiVertexColorProperty>( );
			parent->AddProperty( vertexColor );

			vertexColor->SetVertexMode( Niflib::VertMode( SrcVertexMode ) );
			vertexColor->SetLightingMode( Niflib::LightMode( LightingMode ) );
			vertexColor->SetFlags( ( LightingMode << 3 ) + ( SrcVertexMode << 4 ) );
		}

		if( SpecularEnable && !IsSkyrim( ) )
		{
			Niflib::NiSpecularPropertyRef prop = CreateNiObject<Niflib::NiSpecularProperty>( );
			parent->AddProperty( prop );

			prop->SetFlags( 1 );
		}

		if( Dither && !IsSkyrim( ) )
		{
			Niflib::NiDitherPropertyRef prop = CreateNiObject<Niflib::NiDitherProperty>( );
			parent->AddProperty( prop );

			prop->SetFlags( 1 );
		}

		if( alphaMode != 0 || AlphaTestEnable )
		{
			Niflib::NiAlphaPropertyRef alphaProp = CreateNiObject<Niflib::NiAlphaProperty>( );
			alphaProp->SetBlendState( true );

			if( alphaMode == 0 ) // Automatic
			{
				alphaProp->SetSourceBlendFunc( Niflib::NiAlphaProperty::BlendFunc( srcBlend ) );
				alphaProp->SetDestBlendFunc( Niflib::NiAlphaProperty::BlendFunc( destBlend ) );
			}

			else if( alphaMode == 1 ) // None
			{
				alphaProp->SetBlendState( false );
				alphaProp->SetSourceBlendFunc( Niflib::NiAlphaProperty::BF_SRC_ALPHA );
				alphaProp->SetDestBlendFunc( Niflib::NiAlphaProperty::BF_ONE_MINUS_SRC_ALPHA );
			}

			else if( alphaMode == 2 ) // Standard
			{
				alphaProp->SetSourceBlendFunc( Niflib::NiAlphaProperty::BF_SRC_ALPHA );
				alphaProp->SetDestBlendFunc( Niflib::NiAlphaProperty::BF_ONE_MINUS_SRC_ALPHA );
			}

			else if( alphaMode == 3 ) // Additive
			{
				alphaProp->SetSourceBlendFunc( Niflib::NiAlphaProperty::BF_ONE );
				alphaProp->SetDestBlendFunc( Niflib::NiAlphaProperty::BF_ONE );
			}

			else if( alphaMode == 4 ) // Multiplicative
			{
				alphaProp->SetSourceBlendFunc( Niflib::NiAlphaProperty::BF_ZERO );
				alphaProp->SetDestBlendFunc( Niflib::NiAlphaProperty::BF_SRC_COLOR );
			}

			else // Advanced
			{
				alphaProp->SetSourceBlendFunc( Niflib::NiAlphaProperty::BlendFunc( srcBlend ) );
				alphaProp->SetDestBlendFunc( Niflib::NiAlphaProperty::BlendFunc( destBlend ) );
			}

			alphaProp->SetTestFunc( Niflib::NiAlphaProperty::TestFunc( TestMode ) );
			alphaProp->SetTriangleSortMode( NoSorter );
			alphaProp->SetTestThreshold( TestRef );
			alphaProp->SetTestState( AlphaTestEnable );

			parent->AddProperty( alphaProp );
		}

		bool useDefaultShader = true;

		if( IsSkyrim( ) )
		{
			if( Niflib::BSLightingShaderPropertyRef texProp = new Niflib::BSLightingShaderProperty( ) )
			{
				extern const EnumLookupType BSShaderTypes[];
				int shaderType = StringToEnum( CustomShader, BSShaderTypes );

				if( shaderType < 100 || shaderType > 200 )
					shaderType = 0;
				else
					shaderType -= 100;

				texProp->SetSkyrimShaderType( Niflib::BSLightingShaderPropertyShaderType::DEFAULT );

				Niflib::BSShaderTextureSetRef texset = new Niflib::BSShaderTextureSet( );
				texProp->SetTextureSet( texset );

				std::vector<std::string> textures;
				textures.resize( 9 );

				Niflib::SkyrimShaderPropertyFlags1 flags1 = (Niflib::SkyrimShaderPropertyFlags1)( Niflib::SLSF1_SPECULAR | Niflib::SLSF1_SKINNED | Niflib::SLSF1_RECIEVE_SHADOWS | Niflib::SLSF1_CAST_SHADOWS | Niflib::SLSF1_OWN_EMIT | Niflib::SLSF1_REMAPPABLE_TEXTURES | Niflib::SLSF1_ZBUFFER_TEST );
				Niflib::SkyrimShaderPropertyFlags2 flags2 = (Niflib::SkyrimShaderPropertyFlags2)( Niflib::SLSF2_ZBUFFER_WRITE | Niflib::SLSF2_DOUBLE_SIDED | Niflib::SLSF2_ENVMAP_LIGHT_FADE );

				if( shaderType == 5 )
					flags2 = (Niflib::SkyrimShaderPropertyFlags2)( flags2 | Niflib::SLSF2_SOFT_LIGHTING );

				texProp->SetGlossiness( 80 );
				texProp->SetSpecularColor( Niflib::Color3( 0.933f, 0.855f, 0.804f ) );
				texProp->SetSpecularStrength( 1.0f );
				texProp->SetLightingEffect1( 0.3f );
				texProp->SetLightingEffect2( 2.0f );
				texProp->SetEnvironmentMapScale( 1.0f );

				TSTR diffuseStr;
				TSTR normalStr;
				TSTR glowStr;
				TSTR dispStr;
				TSTR envStr;
				TSTR envMaskStr;
				TSTR specularStr;
				TSTR parallaxStr;

				if( mtl && mtl->ClassID( ) == Class_ID( DMTL_CLASS_ID, 0 ) )
				{
					StdMat2* m = (StdMat2*) mtl;

					if( mtl && mtl->ClassID( ) == Class_ID( DMTL_CLASS_ID, 0 ) )
					{
						StdMat2* m = (StdMat2*) mtl;
						
						if( m->GetMapState( C_BASE ) == 2 )
						{
							if( Texmap* texMap = m->GetSubTexmap( C_BASE ) )
								GetTexFullName( texMap, diffuseStr );
						}

						if( m->GetMapState( C_NORMAL ) == 2 )
						{
							if( Texmap* texMap = m->GetSubTexmap( C_NORMAL) )
							{
								texMap = texMap->GetSubTexmap( 0 );
								GetTexFullName( texMap, normalStr );
							}
							else
								GetTexFullName( texMap, normalStr );
						}

						if( m->GetMapState( C_ENVMASK ) == 2 )
						{
							if( Texmap* texMap = m->GetSubTexmap( C_ENVMASK ) )
							{
								Texmap* envMap = texMap->GetSubTexmap( 0 );
								Texmap* envMaskMap = texMap->GetSubTexmap( 1 );
								
								GetTexFullName( envMap, envStr );
								GetTexFullName( envMaskMap, envMaskStr );
							}
							else
								GetTexFullName( texMap, envMaskStr );
						}

						if( m->GetMapState( C_ENV ) == 2 )
						{
							if( Texmap* texMap = m->GetSubTexmap( C_ENV ) )
							{
								Texmap* envMap = texMap->GetSubTexmap( 0 );
								Texmap* envMaskMap = texMap->GetSubTexmap( 1 );
								
								GetTexFullName( envMap, envStr );
								GetTexFullName( envMaskMap, envMaskStr );
							}
							else
								GetTexFullName( texMap, envMaskStr );
						}

						if( m->GetMapState( C_GLOW ) == 2 )
						{
							if( Texmap* texMap = m->GetSubTexmap( C_GLOW ) )
								GetTexFullName( texMap, glowStr );
						}

						if( m->GetMapState( C_SPECULAR ) == 2 )
						{
							if( Texmap* texMap = m->GetSubTexmap( C_SPECULAR ) )
								GetTexFullName( texMap, specularStr );
						}

						if( m->GetMapState( C_PARALLAX ) == 2 )
						{
							if( Texmap* texMap = m->GetSubTexmap( C_PARALLAX ) )
								GetTexFullName( texMap, parallaxStr );
						}
					}

					textures[ 0 ] = narrow( mAppSettings->GetRelativeTexPath( std::wstring( diffuseStr ), mTexPrefix ) );
					if( normalStr.isNull( ) )
					{
						char path_buffer[ _MAX_PATH ];
						char drive[ _MAX_PATH ];
						char dir[ _MAX_PATH ];
						char fname [ _MAX_PATH ];
						char ext[ _MAX_PATH ];

						_splitpath( textures[ 0 ].c_str( ), drive, dir, fname, ext );
						strcat( fname, "_n" );
						_makepath( path_buffer, drive, dir, fname, ext );

						textures[ 1 ] = path_buffer;
					}
					else
						textures[ 1 ] = narrow( mAppSettings->GetRelativeTexPath( std::wstring( normalStr ), mTexPrefix ) );

					if( !glowStr.isNull( ) )
					{
						textures[ 2 ] = narrow( mAppSettings->GetRelativeTexPath( std::wstring( glowStr ), mTexPrefix ) );
						flags1 = (Niflib::SkyrimShaderPropertyFlags1)( flags1 | Niflib::SLSF1_SKINNED | Niflib::SLSF1_CAST_SHADOWS | Niflib::SLSF2_SOFT_LIGHTING );
					}

					if( !dispStr.isNull( ) )
					{
						textures[ 3 ] = narrow( mAppSettings->GetRelativeTexPath( std::wstring( dispStr ), mTexPrefix ) );
					}

					if( !envStr.isNull( ) )
					{
						textures[ 4 ] = narrow( mAppSettings->GetRelativeTexPath( std::wstring( envStr ), mTexPrefix ) );
						flags1 = (Niflib::SkyrimShaderPropertyFlags1)( flags1 | Niflib::SLSF1_ENVIRONMENT_MAPPING );
					}

					if( !envMaskStr.isNull( ) )
					{
						textures[ 5 ] = narrow( mAppSettings->GetRelativeTexPath( std::wstring( envMaskStr ), mTexPrefix ) );
						flags1 = (Niflib::SkyrimShaderPropertyFlags1)( flags1 | Niflib::SLSF1_ENVIRONMENT_MAPPING );
					}

					if( !specularStr.isNull( ) )
					{
						textures[ 7 ] = narrow( mAppSettings->GetRelativeTexPath( std::wstring( specularStr ), mTexPrefix ) );
						flags1 = (Niflib::SkyrimShaderPropertyFlags1)( flags1 | Niflib::SLSF1_SPECULAR );
					}

					if( m->GetTwoSided( ) )
						flags2 = (Niflib::SkyrimShaderPropertyFlags2)( flags2 | Niflib::SLSF2_DOUBLE_SIDED );

					texProp->SetShaderFlags1( flags1 );
					texProp->SetShaderFlags2( flags2 );

					TimeValue t = 0;
					texProp->SetGlossiness( m->GetShininess( t ) * 100.0f );
					texProp->SetAlpha( m->GetOpacity( t ) / 100.0f );
					texProp->SetSpecularColor( TOCOLOR3( m->GetSpecular( t ) ) );
				}

				textures[ 0 ] = narrow( mAppSettings->GetRelativeTexPath( std::wstring( diffuseStr ), mTexPrefix ) );
				if( normalStr.isNull( ) )
				{
					char path_buffer[ _MAX_PATH ];
					char drive[ _MAX_PATH ];
					char dir[ _MAX_PATH ];
					char fname [ _MAX_PATH ];
					char ext[ _MAX_PATH ];

					_splitpath( textures[ 0 ].c_str( ), drive, dir, fname, ext );
					strcat( fname, "_n" );
					_makepath( path_buffer, drive, dir, fname, ext );

					textures[ 1 ] = path_buffer;
				}
				else
					textures[ 1 ] = narrow( mAppSettings->GetRelativeTexPath( std::wstring( normalStr ), mTexPrefix ) );

				if( !glowStr.isNull( ) )
					textures[ 2 ] = narrow( mAppSettings->GetRelativeTexPath( std::wstring( glowStr ), mTexPrefix ) );

				if( !parallaxStr.isNull( ) )
					textures[ 3 ] = narrow( mAppSettings->GetRelativeTexPath( std::wstring( parallaxStr ), mTexPrefix ) );

				if( !envStr.isNull( ) )
					textures[ 4 ] = narrow( mAppSettings->GetRelativeTexPath( std::wstring( envStr ), mTexPrefix ) );

				if( !envMaskStr.isNull( ) )
					textures[ 5 ] = narrow( mAppSettings->GetRelativeTexPath( std::wstring( envMaskStr ), mTexPrefix ) );

				if( !envStr.isNull( ) )
					textures[ 5 ] = narrow( mAppSettings->GetRelativeTexPath( std::wstring( envStr ), mTexPrefix ) );

				// shader must be first, alpha can be second
				Niflib::NiPropertyRef prop = Niflib::DynamicCast<Niflib::NiProperty>( texProp );
				std::vector<Niflib::NiPropertyRef> properties = parent->GetProperties( );

				parent->AddProperty( prop );
				if( properties.size( ) > 0 )
					parent->AddProperty( properties[ 0 ] );
			}

			useDefaultShader = false;
		}
		else if( IsFallout3( ) )
		{
			// TODO: ...
		}

		if( useDefaultShader )
		{
			// TODO: ...
		}

		return true;
	}

	return false;
}

void Exporter::makeTexture( Niflib::NiAVObjectRef &parent, Mtl* mtl )
{
	BitmapTex* bmTex = getTexture( mtl );
	if( !bmTex )
		return;

	if( IsSkyrim( ) )
	{
		Niflib::BSLightingShaderPropertyRef texProp = new Niflib::BSLightingShaderProperty( );

		// 0 = default, 1 = envmap, 2 = glow, 5 = skin, 6 = hair, 7 = unknown, 11 = ice/parallax, 15 = eye
		Niflib::BSShaderType shaderType = (Niflib::BSShaderType) 0;
		texProp->SetSkyrimShaderType( Niflib::BSLightingShaderPropertyShaderType::DEFAULT );

		Niflib::BSShaderTextureSetRef texset = new Niflib::BSShaderTextureSet( );
		texProp->SetTextureSet( texset );

		std::vector<std::string> textures;
		textures.resize( 9 );

		texProp->SetGlossiness( 80 );
		texProp->SetSpecularColor( Niflib::Color3( 0.933f, 0.855f, 0.804f ) );
		texProp->SetSpecularStrength( 1.0f );
		texProp->SetLightingEffect1( 0.3f );
		texProp->SetLightingEffect2( 2.0f );
		texProp->SetEnvironmentMapScale( 1.0f );
		texProp->SetTextureClampMode( Niflib::TexClampMode::WRAP_S_WRAP_T );

		TSTR diffuseStr;
		TSTR normalStr;
		TSTR glowStr;
		TSTR dispStr;
		TSTR envStr;
		TSTR envMaskStr;

		if( mtl && mtl->ClassID( ) == Class_ID( DMTL_CLASS_ID, 0 ))
		{
			StdMat2* m = (StdMat2*) mtl;

			if( m->GetMapState( ID_DI ) == 2 )
			{
				diffuseStr = m->GetMapName( ID_DI );
				if( Texmap* texMap = m->GetSubTexmap( ID_DI ) )
					GetTexFullName( texMap, diffuseStr );
			}
			else if( m->GetMapState( ID_AM ) == 2 )
			{
				diffuseStr = m->GetMapName( ID_AM );
				if( Texmap* texMap = m->GetSubTexmap( ID_AM ) )
					GetTexFullName( texMap, diffuseStr );
					
			}

			if( m->GetMapState( ID_BU ) == 2 )
			{
				normalStr = m->GetMapName( ID_BU );
				if( Texmap* texMap = m->GetSubTexmap( ID_BU ) )
				{
					if( texMap->ClassID( ) == GNORMAL_CLASS_ID )
						texMap = texMap->GetSubTexmap( 0 );
					
					GetTexFullName( texMap, normalStr );
				}
			}

			if( m->GetMapState( ID_SP ) == 2 )
			{
				envStr = m->GetMapName( ID_SP );
				if( Texmap* texMap = m->GetSubTexmap( ID_SP ) )
				{
					if( texMap->ClassID( ) == Class_ID( MASK_CLASS_ID, 0 ) )
					{
						Texmap* envMap = texMap->GetSubTexmap( 0 );
						Texmap* envMaskMap = texMap->GetSubTexmap( 1 );

						GetTexFullName( envMap, envStr );
						GetTexFullName( envMaskMap, envMaskStr );
					}
					else
						GetTexFullName( texMap, envStr );
				}
			}

			if( m->GetMapState( ID_SI ) == 2 )
			{
				diffuseStr = m->GetMapName( ID_SI );
				if( Texmap* texMap = m->GetSubTexmap( ID_SI ) )
					GetTexFullName( texMap, glowStr );
			}

			if( m->GetMapState( ID_RL ) == 2 )
			{
				diffuseStr = m->GetMapName( ID_RL );
				if( Texmap* texMap = m->GetSubTexmap( ID_RL ) )
					GetTexFullName( texMap, dispStr );
			}

			TimeValue t = 0;
			texProp->SetGlossiness( m->GetShininess( t ) * 100.0f );
			texProp->SetAlpha( 1 );
			texProp->SetSpecularColor( TOCOLOR3( m->GetSpecular( t ) ) );
		}

		textures[0] = narrow( mAppSettings->GetRelativeTexPath( std::wstring( diffuseStr ), mTexPrefix ) );
		if( normalStr.isNull( ) ) 
		{
			char path_buffer[ _MAX_PATH ];
			char drive[ _MAX_DRIVE ];
			char dir[ _MAX_DIR ];
			char fname[ _MAX_FNAME ];
			char ext[ _MAX_EXT ];

			_splitpath( textures[ 0 ].c_str( ), drive, dir, fname, ext );
			strcat( fname, "_n" );
			_makepath( path_buffer, drive, dir, fname, ext );
			textures[ 1 ] = path_buffer;			
		}
		else
			textures[ 1 ] = narrow( mAppSettings->GetRelativeTexPath( std::wstring( normalStr ), mTexPrefix ) );

		if( !glowStr.isNull( ) )
			textures[ 2 ] = narrow( mAppSettings->GetRelativeTexPath( std::wstring( glowStr ), mTexPrefix ) );

		if( !dispStr.isNull( ) )
			textures[ 3 ] = narrow( mAppSettings->GetRelativeTexPath( std::wstring( dispStr ), mTexPrefix ) );

		if( !envStr.isNull( ) )
			textures[ 4 ] = narrow( mAppSettings->GetRelativeTexPath( std::wstring( envStr ), mTexPrefix ) );

		if( !envMaskStr.isNull( ) )
			textures[ 5 ] = narrow( mAppSettings->GetRelativeTexPath( std::wstring( envMaskStr ), mTexPrefix ) );

		texset->SetTextures( textures );

		// shader must be first, alpha can be second
		Niflib::NiPropertyRef prop = Niflib::DynamicCast<Niflib::NiProperty>( texProp );
		std::vector<Niflib::NiPropertyRef> properties = parent->GetProperties( );
		
		parent->ClearProperties( );
		Niflib::NiGeometryRef geom = Niflib::DynamicCast<Niflib::NiGeometry>( parent );

		geom->SetBSProperty( 0, prop );
	}
	else if( IsFallout3( ) )
	{
		// TODO: ...
	}
	else if( Exporter::mNifVersionInt >= Niflib::VER_4_0_0_0 )
	{
		// TODO: ...
	}
	else
	{
		// TODO: ...
	}
}