#include "pch.h"
#include "stdmat.h"
#include "shaders.h"
#include "AppSettings.h"
#include "gen/enums.h"
#include "Ref.h"
#include "obj/NiWireframeProperty.h"
#include "obj/NiAlphaProperty.h"
#include "obj/NiStencilProperty.h"
#include "obj/NiGeometry.h"
#include "obj/NiShadeProperty.h"
#include "obj/NiVertexColorProperty.h"
#include "obj/NiDitherProperty.h"
#include "obj/NiSpecularProperty.h"
#include "obj/NiTextureProperty.h"
#include "obj/NiImage.h"
#include "obj/WaterShaderProperty.h"
#include "obj/SkyShaderProperty.h"
#include "obj/TallGrassShaderProperty.h"
#include "obj/Lighting30ShaderProperty.h"
#include "obj/BSShaderNoLightingProperty.h"
#include "obj/BSShaderPPLightingProperty.h"
#include "obj/BSShaderTextureSet.h"
#include "obj/BSLightingShaderProperty.h"
#include "ObjectRegistry.h"

enum { C_BASE, C_DARK, C_DETAIL, C_GLOSS, C_GLOW, C_BUMP, C_NORMAL, C_UNK2, 
C_DECAL0, C_DECAL1, C_DECAL2, C_ENVMASK, C_ENV, C_HEIGHT, C_REFLECTION, C_OPACITY,
C_SPECULAR, C_PARALLAX
};

static bool GetTexFullName(Texmap *texMap, TSTR& fName)
{
	if (texMap && texMap->ClassID() == Class_ID(BMTEX_CLASS_ID, 0)) {
		TSTR fileName = ((BitmapTex*)texMap)->GetMapName();
		if (fileName.isNull()) {
			fileName = ((BitmapTex*)texMap)->GetFullName();
			int idx = fileName.last('(');
			if (idx >= 0) {
				fileName.remove(idx, fileName.length() - idx + 1);
				while ( --idx > 0 ) {
					if ( isspace(fileName[idx]) )
						fileName.remove(idx);
				}
			}
		}
		fName = fileName;
		return true;
	}
	return false;
}

#undef GNORMAL_CLASS_ID
static const Class_ID GNORMAL_CLASS_ID(0x243e22c6, 0x63f6a014);

void Exporter::makeTexture(NiAVObjectRef &parent, Mtl *mtl)
{
	BitmapTex *bmTex = getTexture(mtl);
	if (!bmTex)
		return;


	if (IsSkyrim())
	{
		BSLightingShaderPropertyRef texProp = new BSLightingShaderProperty();

      //0=default 1=EnvMap, 2=Glow, 5=Skin, 6=Hair, 7=Unknown, 11=Ice/Parallax, 15=Eye.
      BSShaderType shaderType = (BSShaderType)0;
	  texProp->SetSkyrimShaderType(BSLightingShaderPropertyShaderType::DEFAULT);

		BSShaderTextureSetRef texset = new BSShaderTextureSet();
		texProp->SetTextureSet( texset );
		
		vector<string> textures;
		textures.resize(9);

      //BSShaderFlags shFlags = BSShaderFlags(SF_ZBUFFER_TEST | SF_SHADOW_MAP | SF_SHADOW_FRUSTUM | SF_EMPTY | SF_UNKNOWN_31);
      //if (!envStr.isNull() || !dispStr.isNull())
      //	shFlags = BSShaderFlags(shFlags | SF_MULTIPLE_TEXTURES);
      //texProp->SetShaderFlags(shFlags);
      texProp->SetGlossiness(80);
      texProp->SetSpecularColor(Color3(0.933f,0.855f,0.804f));
      texProp->SetSpecularStrength(1.0f);
      texProp->SetLightingEffect1(0.3f);
      texProp->SetLightingEffect2(2.0f);
      texProp->SetEnvironmentMapScale(1.0f);
	  texProp->SetTextureClampMode(Niflib::TexClampMode::WRAP_S_WRAP_T);//changed

		TSTR diffuseStr, normalStr, glowStr, dispStr, envStr, envMaskStr;

		if(mtl && mtl->ClassID() == Class_ID(DMTL_CLASS_ID, 0) ) {
			StdMat2 *m = (StdMat2*)mtl;

			if (m->GetMapState(ID_DI) == 2) {
				diffuseStr = m->GetMapName(ID_DI);
				if (Texmap *texMap = m->GetSubTexmap(ID_DI)) {
					GetTexFullName(texMap, diffuseStr);
				}
			} else if (m->GetMapState(ID_AM) == 2) {
				diffuseStr = m->GetMapName(ID_AM);
				if (Texmap *texMap = m->GetSubTexmap(ID_AM)) {
					GetTexFullName(texMap, diffuseStr);
				}
			}
			if (m->GetMapState(ID_BU) == 2) {
				normalStr = m->GetMapName(ID_BU);     
				if (Texmap *texMap = m->GetSubTexmap(ID_BU)) {
					if (texMap->ClassID() == GNORMAL_CLASS_ID) {
						texMap = texMap->GetSubTexmap(0);
						GetTexFullName(texMap, normalStr);
					} else {
						GetTexFullName(texMap, normalStr);
					}
				}
			}
			if (m->GetMapState(ID_SP) == 2) {
				envStr = m->GetMapName(ID_SP);     
				if (Texmap *texMap = m->GetSubTexmap(ID_SP)) {
					if (texMap->ClassID() == Class_ID(MASK_CLASS_ID, 0)) {
						Texmap *envMap = texMap->GetSubTexmap(0);
						Texmap *envMaskMap = texMap->GetSubTexmap(1);
						GetTexFullName(envMap, envStr);
						GetTexFullName(envMaskMap, envMaskStr);
					} else {
						GetTexFullName(texMap, envStr);
					}
				}
			}
			if (m->GetMapState(ID_SI) == 2) {
				glowStr = m->GetMapName(ID_SI);     
				if (Texmap *texMap = m->GetSubTexmap(ID_SI)) {
					GetTexFullName(texMap, glowStr);
				}
			}
			if (m->GetMapState(ID_RL) == 2) {
				dispStr = m->GetMapName(ID_RL);     
				if (Texmap *texMap = m->GetSubTexmap(ID_RL)) {
					GetTexFullName(texMap, dispStr);
				}
			}


         TimeValue t = 0;
         texProp->SetGlossiness(m->GetShininess(t) * 100.0f);
         //texProp->SetSpecularColor(m->getsh);
         //texProp->SetSpecularStrength(m->GetShinStr(t) < 1.0f ? 3.0f : 0.0f);
         texProp->SetAlpha(1);
         texProp->SetSpecularColor(TOCOLOR3(m->GetSpecular(t)));
         //texProp->SetEmissiveColor(TOCOLOR(m->GetEmmis(t)));

         //mtl->SetShinStr(0.0,0);
         //mtl->SetShininess(shininess/100.0,0);
         //mtl->SetOpacity(alpha*100.0f,0);
         //mtl->SetSpecularColor(alpha*100.0f,0);

         //texProp->SetLightingEffect1(0.3f);
         //texProp->SetLightingEffect1(2.0f);
         //texProp->SetEnvironmentMapStrength(1.0f);

		}

		textures[0] = mAppSettings->GetRelativeTexPath(string(diffuseStr), mTexPrefix);
		if (normalStr.isNull()) {
			char path_buffer[_MAX_PATH], drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME], ext[_MAX_EXT];
			_splitpath(textures[0].c_str(), drive, dir, fname, ext);
			strcat(fname, "_n");
			_makepath(path_buffer, drive, dir, fname, ext);
			textures[1] = path_buffer;			
		}
		else
			textures[1] = mAppSettings->GetRelativeTexPath(string(normalStr), mTexPrefix);
		if (!glowStr.isNull())
			textures[2] = mAppSettings->GetRelativeTexPath(string(glowStr), mTexPrefix);
		if (!dispStr.isNull())
			textures[3] = mAppSettings->GetRelativeTexPath(string(dispStr), mTexPrefix);
		if (!envStr.isNull())
			textures[4] = mAppSettings->GetRelativeTexPath(string(envStr), mTexPrefix);
      if (!envMaskStr.isNull())
         textures[5] = mAppSettings->GetRelativeTexPath(string(envMaskStr), mTexPrefix);

		texset->SetTextures(textures);

	 
      // shader must be first, alpha can be second
      NiPropertyRef prop = DynamicCast<NiProperty>(texProp);
      vector<NiPropertyRef> properties = parent->GetProperties();
      parent->ClearProperties();
      NiGeometryRef geom = DynamicCast<NiGeometry>(parent);
	  geom->SetBSProperty(1, prop);
      
	}
	else if (IsFallout3())
	{
		BSShaderPPLightingPropertyRef texProp = new BSShaderPPLightingProperty();
		
		texProp->SetFlags(1);
		texProp->SetShaderType( SHADER_DEFAULT );
		BSShaderTextureSetRef texset = new BSShaderTextureSet();
		texProp->SetTextureSet( texset );

		vector<string> textures;
		textures.resize(6);

		TSTR diffuseStr, normalStr, glowStr, dispStr, envStr, envMaskStr;

		if(mtl && mtl->ClassID() == Class_ID(DMTL_CLASS_ID, 0) ) {
			StdMat2 *m = (StdMat2*)mtl;

			if (m->GetMapState(ID_DI) == 2) {
				diffuseStr = m->GetMapName(ID_DI);
				if (Texmap *texMap = m->GetSubTexmap(ID_DI)) {
					GetTexFullName(texMap, diffuseStr);
				}
			} else if (m->GetMapState(ID_AM) == 2) {
				diffuseStr = m->GetMapName(ID_AM);
				if (Texmap *texMap = m->GetSubTexmap(ID_AM)) {
					GetTexFullName(texMap, diffuseStr);
				}
			}
			if (m->GetMapState(ID_BU) == 2) {
				normalStr = m->GetMapName(ID_BU);     
				if (Texmap *texMap = m->GetSubTexmap(ID_BU)) {
					if (texMap->ClassID() == GNORMAL_CLASS_ID) {
						texMap = texMap->GetSubTexmap(0);
						GetTexFullName(texMap, normalStr);
					} else {
						GetTexFullName(texMap, normalStr);
					}
				}
			}
			if (m->GetMapState(ID_SP) == 2) {
				envStr = m->GetMapName(ID_SP);     
				if (Texmap *texMap = m->GetSubTexmap(ID_SP)) {
					if (texMap->ClassID() == Class_ID(MASK_CLASS_ID, 0)) {
						Texmap *envMap = texMap->GetSubTexmap(0);
						Texmap *envMaskMap = texMap->GetSubTexmap(1);
						GetTexFullName(envMap, envStr);
						GetTexFullName(envMaskMap, envMaskStr);
					} else {
						GetTexFullName(texMap, envStr);
					}
				}
			}
			if (m->GetMapState(ID_SI) == 2) {
				glowStr = m->GetMapName(ID_SI);     
				if (Texmap *texMap = m->GetSubTexmap(ID_SI)) {
					GetTexFullName(texMap, glowStr);
				}
			}
			if (m->GetMapState(ID_RL) == 2) {
				dispStr = m->GetMapName(ID_RL);     
				if (Texmap *texMap = m->GetSubTexmap(ID_RL)) {
					GetTexFullName(texMap, dispStr);
				}
			}
		}

		textures[0] = mAppSettings->GetRelativeTexPath(string(diffuseStr), mTexPrefix);
		if (normalStr.isNull()) {
			char path_buffer[_MAX_PATH], drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME], ext[_MAX_EXT];
			_splitpath(textures[0].c_str(), drive, dir, fname, ext);
			strcat(fname, "_n");
			_makepath(path_buffer, drive, dir, fname, ext);
			textures[1] = path_buffer;			
		}
		else
			textures[1] = mAppSettings->GetRelativeTexPath(string(normalStr), mTexPrefix);
		if (!envMaskStr.isNull())
			textures[2] = mAppSettings->GetRelativeTexPath(string(envMaskStr), mTexPrefix);
		if (!glowStr.isNull())
			textures[3] = mAppSettings->GetRelativeTexPath(string(glowStr), mTexPrefix);
		if (glowStr.isNull()){
			char path_buffer[_MAX_PATH], drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME], ext[_MAX_EXT];
			_splitpath(textures[0].c_str(), drive, dir, fname, ext);
			strcat(fname, "_g");
			_makepath(path_buffer, drive, dir, fname, ext);
			textures[3] = path_buffer;			
		} else {
			textures[3] = mAppSettings->GetRelativeTexPath(string(glowStr), mTexPrefix);
		}
		if (!dispStr.isNull())
			textures[4] = mAppSettings->GetRelativeTexPath(string(dispStr), mTexPrefix);
		if (!envStr.isNull())
			textures[5] = mAppSettings->GetRelativeTexPath(string(envStr), mTexPrefix);

		BSShaderFlags shFlags = BSShaderFlags(SF_SPECULAR | SF_SHADOW_MAP | SF_SHADOW_FRUSTUM | SF_EMPTY | SF_ZBUFFER_TEST);
		if (!envStr.isNull() || !dispStr.isNull())
			shFlags = BSShaderFlags(shFlags | SF_MULTIPLE_TEXTURES);
		texProp->SetShaderFlags(shFlags);
		texset->SetTextures(textures);
		NiPropertyRef prop = DynamicCast<NiProperty>(texProp);
		parent->AddProperty(prop);
	}
	else if (Exporter::mNifVersionInt >= VER_4_0_0_0)
	{
		NiTexturingPropertyRef texProp = CreateNiObject<NiTexturingProperty>();
		texProp->SetApplyMode(APPLY_MODULATE);
		texProp->SetTextureCount(7);

		TexDesc td;
		NiTriBasedGeom *shape(DynamicCast<NiTriBasedGeom>(parent));
		if (makeTextureDesc(bmTex, td, shape->GetData()))
			texProp->SetTexture(BASE_MAP, td);

		NiPropertyRef prop = DynamicCast<NiProperty>(texProp);
		parent->AddProperty(prop);
	}
	else
	{
		NiTexturePropertyRef texProp = CreateNiObject<NiTextureProperty>();
		NiImageRef imgProp = CreateNiObject<NiImage>();
		texProp->SetImage(imgProp);

		// Get file name and check if it matches the "app" settings in the ini file
		TSTR mapPath = bmTex->GetMapName();
		if (mAppSettings)
		{
			string newPath = mAppSettings->GetRelativeTexPath(string(mapPath), mTexPrefix);
			imgProp->SetExternalTexture(newPath);
		}
		else
		{
			TSTR p, f;
			SplitPathFile(mapPath, &p, &f);
			TSTR newPath = (mTexPrefix == "") ? f : (TSTR(mTexPrefix.c_str()) + _T("\\") + f);
			imgProp->SetExternalTexture(newPath.data());
		}
		NiPropertyRef prop = DynamicCast<NiProperty>(texProp);
		parent->AddProperty(prop);
	}
}

bool Exporter::makeTextureDesc(BitmapTex *bmTex, TexDesc& td, NiTriBasedGeomDataRef shape)
{
	td.source = new NiSourceTexture();

	// Filtering
	switch (bmTex->GetFilterType())
	{
	case FILTER_PYR:  td.filterMode = FILTER_TRILERP; break;
	case FILTER_SAT:  td.filterMode = FILTER_BILERP; break;
	case FILTER_NADA: td.filterMode = FILTER_NEAREST; break;
	}

	td.clampMode = TexClampMode();  
	switch (bmTex->GetTextureTiling())
	{
	case 3: td.clampMode = WRAP_S_WRAP_T; break;
	case 1: td.clampMode = WRAP_S_CLAMP_T; break;
	case 2: td.clampMode = CLAMP_S_WRAP_T; break;
	case 0: td.clampMode = CLAMP_S_CLAMP_T; break;
	}

	if (UVGen *uvGen = bmTex->GetTheUVGen()){
		if (uvGen && uvGen->IsStdUVGen()) {
			StdUVGen *uvg = (StdUVGen*)uvGen;
			td.uvSet = shape->GetUVSetIndex(uvg->GetMapChannel());
			if (td.uvSet == -1) td.uvSet = 1;
		}
		if (RefTargetHandle ref = uvGen->GetReference(0)){
			TexCoord trans, tiling;
			float wangle;
			bool ok = true;
			if(ok) ok &= getMAXScriptValue(ref, "U_Offset", 0, trans.u);
			if(ok) ok &= getMAXScriptValue(ref, "V_Offset", 0, trans.v);
			if(ok) ok &= getMAXScriptValue(ref, "U_Tiling", 0, tiling.u);
			if(ok) ok &= getMAXScriptValue(ref, "V_Tiling", 0, tiling.v);
			if(ok) ok &= getMAXScriptValue(ref, "W_Angle", 0, wangle);
			if (ok) {
				if (trans.u != 0.0f || trans.v != 0.0f || tiling.u != 1.0f || tiling.v != 1.0f || wangle != 0.0f) {
					td.hasTextureTransform = true;
					td.translation = trans;
					td.tiling = tiling;
					td.wRotation = TORAD(wangle);
					td.transformType_ = 1;
					td.centerOffset = TexCoord(0.5, 0.5);
				}
			}
		}
	}
	if (Exporter::mNifVersionInt >= VER_20_0_0_4)
	{
		td.source->SetPixelLayout( PIX_LAY_DEFAULT );
		td.source->SetMipMapFormat( MIP_FMT_DEFAULT );
		td.source->SetAlphaFormat( ALPHA_DEFAULT );
	}

	// Get file name and check if it matches the "app" settings in the ini file
	TSTR mapPath;
	mapPath = bmTex->GetMapName();

	if (mAppSettings)
	{
		string newPath = mAppSettings->GetRelativeTexPath(string(mapPath), mTexPrefix);
		td.source->SetExternalTexture(newPath);
	}
	else
	{
		TSTR p, f;
		SplitPathFile(mapPath, &p, &f);
		TSTR newPath;
		if (mTexPrefix != "")
			newPath = TSTR(mTexPrefix.c_str()) + _T("\\") + f;
		else
			newPath = f;

		td.source->SetExternalTexture(newPath.data());
	}
	return true;
}

void Exporter::makeMaterial(NiAVObjectRef &parent, Mtl *mtl)
{
	// Fill-in using the Civ4 Shader if available
	bool done = exportNiftoolsShader(parent, mtl);
	if (done)
		return;

	string name;
	NiMaterialPropertyRef mtlProp(CreateNiObject<NiMaterialProperty>());
	if (mtl)
	{
		Color c;

		c = mtl->GetAmbient();
		mtlProp->SetAmbientColor(Color3(c.r, c.g, c.b));

		c = mtl->GetDiffuse();
		mtlProp->SetDiffuseColor(Color3(c.r, c.g, c.b));

		c = mtl->GetSpecular();
		mtlProp->SetSpecularColor(Color3(c.r, c.g, c.b));

		c = (mtl->GetSelfIllumColorOn()) ? mtl->GetSelfIllumColor() : Color(0,0,0);
		mtlProp->SetEmissiveColor(Color3(c.r, c.g, c.b));

		mtlProp->SetTransparency(1);
		mtlProp->SetGlossiness(mtl->GetShininess() * 100.0f);
		name = (char*)mtl->GetName();

		if(mtl->ClassID() == Class_ID(DMTL_CLASS_ID, 0) )
		{
			StdMat2 * smtl = (StdMat2*)mtl;
			mtlProp->SetTransparency(smtl->GetOpacity(0));

			if (smtl->SupportsShaders()) {
				if (Shader *s = smtl->GetShader()) {
					if (smtl->GetWire()){
						NiWireframePropertyRef wireProp = new NiWireframeProperty();
						wireProp->SetFlags(1);
						parent->AddProperty(wireProp);
					}
					if (smtl->GetTwoSided()){
						NiStencilPropertyRef stencil = new NiStencilProperty();
						stencil->SetStencilFunction(TEST_GREATER);
						stencil->SetStencilState(false);
						stencil->SetPassAction(ACTION_INCREMENT);
						stencil->SetFaceDrawMode(DRAW_BOTH);
						stencil->SetFlags(19840);
						parent->AddProperty(stencil);
					}
					if (smtl->IsFaceted()) {
						NiShadePropertyRef shade = CreateNiObject<NiShadeProperty>();
						shade->SetFlags(0);
						parent->AddProperty(shade);
					}
				}
			}
		}
	} else
	{
		mtlProp->SetAmbientColor(Color3(0.588f, 0.588f, 0.588f));
		mtlProp->SetDiffuseColor(Color3(1, 1, 1));
		mtlProp->SetSpecularColor(Color3(0.9f, 0.9f, 0.9f));
		mtlProp->SetEmissiveColor(Color3(0, 0, 0));
		mtlProp->SetTransparency(1);
		mtlProp->SetGlossiness(10);
		name = "default";
	}

   
    mtlProp->SetName(name);

	NiPropertyRef prop = DynamicCast<NiProperty>(mtlProp);
	/*parent->AddProperty(prop);*/

	makeTexture(parent, mtl);
	}

Mtl *Exporter::getMaterial(INode *node, int subMtl)
{
	Mtl *nodeMtl = node->GetMtl();
	if (nodeMtl)
	{
		int numSub = nodeMtl->NumSubMtls();
		if (numSub > 0)
			return nodeMtl->GetSubMtl(subMtl % numSub);

		return nodeMtl;
	}
	return NULL;
}

BitmapTex *Exporter::getTexture(Mtl *mtl)
{
	if (!mtl)
		return NULL;
	int texMaps = mtl->NumSubTexmaps();
	if (!texMaps)
		return NULL;
	for (int i=0; i<texMaps; i++)
	{
		Texmap *texMap = mtl->GetSubTexmap(i);
		if (!texMap)
			continue;

		if (texMap->ClassID() == Class_ID(BMTEX_CLASS_ID, 0)){
			return (BitmapTex*)texMap;
		} else if (texMap && texMap->ClassID() == GNORMAL_CLASS_ID) {
			texMap = texMap->GetSubTexmap(0);
			if (texMap->ClassID() == Class_ID(BMTEX_CLASS_ID, 0)) {
				return ((BitmapTex*)texMap);
			}
		}
	}
	return NULL;
}

BitmapTex *Exporter::getTexture(Mtl *mtl, int i)
{
	if (mtl) {
		int texMaps = mtl->NumSubTexmaps();
		if (i < texMaps) {
			if (Texmap *texMap = mtl->GetSubTexmap(i)) {
				if (texMap->ClassID() == Class_ID(BMTEX_CLASS_ID, 0)) {
					return (BitmapTex*)texMap;
				} else if (texMap && texMap->ClassID() == GNORMAL_CLASS_ID) {
					texMap = texMap->GetSubTexmap(0);
					if (texMap->ClassID() == Class_ID(BMTEX_CLASS_ID, 0)) {
						return ((BitmapTex*)texMap);
					}
				}
			}
		}
	}
	return NULL;
}

void Exporter::getTextureMatrix(Matrix3 &mat, Mtl *mtl)
{
	BitmapTex *tex = getTexture(mtl);
	if (tex)
		tex->GetUVTransform(mat);
	else
		mat.IdentityMatrix();
}


bool Exporter::exportNiftoolsShader(NiAVObjectRef parent, Mtl* mtl)
{
	if (!mtl) 
		return false;

	RefTargetHandle ref = mtl->GetReference(2/*shader*/);
	if (!ref)
		return false;

	const Class_ID civ4Shader(0x670a77d0,0x23ab5c7f);
	const Class_ID NIFSHADER_CLASS_ID(0x566e8ccb, 0xb091bd48);

	TSTR shaderByName; Class_ID shaderID = Class_ID(0, 0);
	if(mtl->ClassID() == Class_ID(DMTL_CLASS_ID, 0) )
	{
		StdMat2 * smtl = (StdMat2*)mtl;
		if (smtl->SupportsShaders()) {
			if (Shader *s = smtl->GetShader()) {
				s->GetClassName(shaderByName);
				shaderID = s->ClassID();
			}
		}
	}
	if (shaderID != NIFSHADER_CLASS_ID && shaderID != civ4Shader)
	{
		if (shaderByName != TSTR("CivilizationIV Shader"))
			return false;
	}
	bool isGamebryoShader = (shaderID == civ4Shader);
	bool isCiv4Shader = isGamebryoShader && (shaderByName == TSTR("CivilizationIV Shader"));

	Color ambient = Color(0.0f,0.0f,0.0f), diffuse = Color(0.0f,0.0f,0.0f), specular = Color(0.0f,0.0f,0.0f), emittance = Color(0.0f,0.0f,0.0f);
	float shininess = 0.0f, alpha = 0.0f, Magnitude = 0.0f, LumaScale = 0.0f, LumaOffset = 0.0f;
	int TestRef = 0, srcBlend = 0, destBlend = 0, TestMode = 0;
	bool AlphaTestEnable = false;
	int ApplyMode = 0, SrcVertexMode = 0, LightingMode = 0;
	bool VertexColorsEnable = false, SpecularEnable = false, NoSorter = false, Dither = false;
	int alphaMode = 0, BaseTextureExport=0, DarkTextureExport=0, DetailTextureExport=0;
	int Decal1TextureExport=0, Decal2TextureExport=0, GlossTextureExport=0, GlowTextureExport=0;
	LPTSTR CustomShader = NULL;
	int ShaderViewerTechnique=0, ShaderExportTechnique=0;
	bool UseNormalMaps = false;
	int NormalMapTechnique=0;

	bool ok = true;

	if(ok) ok &= getMAXScriptValue(ref, "ambient", 0, ambient );
	if(ok) ok &= getMAXScriptValue(ref, "diffuse", 0, diffuse );
	if(ok) ok &= getMAXScriptValue(ref, "specular", 0, specular );
	if(ok) ok &= getMAXScriptValue(ref, "emittance", 0, emittance );
	if(ok) ok &= getMAXScriptValue(ref, "shininess", 0, shininess );
	if(ok) ok &= getMAXScriptValue(ref, "alpha", 0, alpha );
	if(ok) ok &= getMAXScriptValue(ref, "Bump_Map_Magnitude", 0, Magnitude);
	if(ok) ok &= getMAXScriptValue(ref, "Bump_Map_Luma_Scale", 0, LumaScale);
	if(ok) ok &= getMAXScriptValue(ref, "Bump_Map_Luma_offset", 0, LumaOffset);
	if(ok) ok &= getMAXScriptValue(ref, "TestRef", 0, TestRef );
	if(ok) ok &= getMAXScriptValue(ref, "AlphaTestEnable", 0, AlphaTestEnable );
	if(ok) ok &= getMAXScriptValue(ref, "Vertex_Color_Enable", 0, VertexColorsEnable);
	if(ok) ok &= getMAXScriptValue(ref, "SpecularEnable", 0, SpecularEnable);
	if(ok) ok &= getMAXScriptValue(ref, "NoSorter", 0, NoSorter);
	if(ok) ok &= getMAXScriptValue(ref, "Dither", 0, Dither );
	if(ok) ok &= getMAXScriptValue(ref, "srcBlend", 0, srcBlend);
	if(ok) ok &= getMAXScriptValue(ref, "destBlend", 0, destBlend);
	if(ok) ok &= getMAXScriptValue(ref, "TestMode", 0, TestMode );
	if(ok) ok &= getMAXScriptValue(ref, "ApplyMode", 0, ApplyMode);
	if(ok) ok &= getMAXScriptValue(ref, "SourceVertexMode", 0, SrcVertexMode);
	if(ok) ok &= getMAXScriptValue(ref, "LightingMode", 0, LightingMode);
	if(ok) ok &= getMAXScriptValue(ref, "alphaMode", 0, alphaMode);
	if(ok) ok &= getMAXScriptValue(ref, "CustomShader", 0, CustomShader );

	if (ok) // civ4 shader
	{
		//if ( !IsSkyrim() ) // skyrim does not use material properties
		//{
		//	NiMaterialPropertyRef mtlProp = CreateNiObject<NiMaterialProperty>();
		//	parent->AddProperty(mtlProp);

		//	mtlProp->SetName((char*)mtl->GetName());
		//	mtlProp->SetAmbientColor(TOCOLOR3(ambient));
		//	mtlProp->SetDiffuseColor(TOCOLOR3(diffuse));
		//	mtlProp->SetSpecularColor(TOCOLOR3(specular));
		//	mtlProp->SetEmissiveColor(TOCOLOR3(emittance));
		//	mtlProp->SetGlossiness(shininess);
		//	mtlProp->SetTransparency(alpha);
		//}
		//if(mtl->ClassID() == Class_ID(DMTL_CLASS_ID, 0) )
		//{
		//	/*StdMat2 * smtl = (StdMat2*)mtl;
		//	if (smtl->SupportsShaders() && !IsSkyrim()) {
		//		if (Shader *s = smtl->GetShader()) {
		//			if (smtl->GetWire()){
		//				NiWireframePropertyRef wireProp = CreateNiObject<NiWireframeProperty>();
		//				wireProp->SetFlags(1);
		//				parent->AddProperty(wireProp);
		//			}
		//			if (smtl->GetTwoSided()){
		//				NiStencilPropertyRef stencil = CreateNiObject<NiStencilProperty>();
		//				stencil->SetStencilFunction(TEST_GREATER);
		//				stencil->SetStencilState(false);
		//				stencil->SetPassAction(ACTION_INCREMENT);
		//				stencil->SetFaceDrawMode(DRAW_BOTH);
		//				parent->AddProperty(stencil);
		//			}
		//			if (smtl->IsFaceted()) {
		//				NiShadePropertyRef shade = CreateNiObject<NiShadeProperty>();
		//				shade->SetFlags(0);
		//				parent->AddProperty(shade);
		//			}
		//		}*/
		//	}
		//}
		if (mVertexColors && VertexColorsEnable) {
			NiVertexColorPropertyRef vertexColor = CreateNiObject<NiVertexColorProperty>();
			parent->AddProperty(vertexColor);
			vertexColor->SetVertexMode(VertMode(SrcVertexMode));
			vertexColor->SetLightingMode(LightMode(LightingMode));
			vertexColor->SetFlags( (LightingMode << 3) + (SrcVertexMode << 4) );
		}
		if (SpecularEnable && !IsSkyrim()) {
			NiSpecularPropertyRef prop = CreateNiObject<NiSpecularProperty>();
			parent->AddProperty(prop);
			prop->SetFlags(1);

		}
		if (Dither && !IsSkyrim()) {
			NiDitherPropertyRef prop = CreateNiObject<NiDitherProperty>();
			parent->AddProperty(prop);
			prop->SetFlags(1);
		}
		if (alphaMode != 0 || AlphaTestEnable) {
			// always add alpha ???
			NiAlphaPropertyRef alphaProp = CreateNiObject<NiAlphaProperty>();
			alphaProp->SetBlendState(true);
			if (alphaMode == 0) { // automatic
				alphaProp->SetSourceBlendFunc(NiAlphaProperty::BlendFunc(srcBlend));
				alphaProp->SetDestBlendFunc(NiAlphaProperty::BlendFunc(destBlend));
			} else if (alphaMode == 1) { // None
				alphaProp->SetBlendState(false);
				alphaProp->SetSourceBlendFunc(NiAlphaProperty::BF_SRC_ALPHA);
				alphaProp->SetDestBlendFunc(NiAlphaProperty::BF_ONE_MINUS_SRC_ALPHA);
			} else if (alphaMode == 2) { // Standard
				alphaProp->SetSourceBlendFunc(NiAlphaProperty::BF_SRC_ALPHA);
				alphaProp->SetDestBlendFunc(NiAlphaProperty::BF_ONE_MINUS_SRC_ALPHA);
			} else if (alphaMode == 3) { // Additive
				alphaProp->SetSourceBlendFunc(NiAlphaProperty::BF_ONE);
				alphaProp->SetDestBlendFunc(NiAlphaProperty::BF_ONE);
			} else if (alphaMode == 4) { // Multiplicative
				alphaProp->SetSourceBlendFunc(NiAlphaProperty::BF_ZERO);
				alphaProp->SetDestBlendFunc(NiAlphaProperty::BF_SRC_COLOR);
			} else { // Advanced
				alphaProp->SetSourceBlendFunc(NiAlphaProperty::BlendFunc(srcBlend));
				alphaProp->SetDestBlendFunc(NiAlphaProperty::BlendFunc(destBlend));
			}
			alphaProp->SetTestFunc(NiAlphaProperty::TestFunc(TestMode));
			alphaProp->SetTriangleSortMode(NoSorter);
			alphaProp->SetTestThreshold(TestRef);
			alphaProp->SetTestState(AlphaTestEnable);
			parent->AddProperty(alphaProp);
		}

		bool useDefaultShader = true;
      if (IsSkyrim())
      {
         if (BSLightingShaderPropertyRef texProp = new BSLightingShaderProperty())
         {
            extern const EnumLookupType BSShaderTypes[];
            int shaderType = StringToEnum(CustomShader, BSShaderTypes);
            if (shaderType<100 || shaderType > 200)
               shaderType = 0;
            else 
               shaderType -= 100;
			texProp->SetSkyrimShaderType(BSLightingShaderPropertyShaderType::DEFAULT);

            BSShaderTextureSetRef texset = new BSShaderTextureSet();
            texProp->SetTextureSet( texset );

            vector<string> textures;
            textures.resize(9);

            SkyrimShaderPropertyFlags1 flags1 = ( SkyrimShaderPropertyFlags1)(SLSF1_SPECULAR|SLSF1_SKINNED|SLSF1_RECIEVE_SHADOWS|SLSF1_CAST_SHADOWS|SLSF1_OWN_EMIT|SLSF1_REMAPPABLE_TEXTURES|SLSF1_ZBUFFER_TEST);
            SkyrimShaderPropertyFlags2 flags2 = ( SkyrimShaderPropertyFlags2)(SLSF2_ZBUFFER_WRITE|SLSF2_DOUBLE_SIDED|SLSF2_ENVMAP_LIGHT_FADE);

            if (shaderType == 5)
               flags2 = (SkyrimShaderPropertyFlags2)(flags2 | SLSF2_SOFT_LIGHTING);
            texProp->SetGlossiness(80);
            texProp->SetSpecularColor(Color3(0.933f,0.855f,0.804f));
            texProp->SetSpecularStrength(1.0f);
            texProp->SetLightingEffect1(0.3f);
            texProp->SetLightingEffect2(2.0f);
            texProp->SetEnvironmentMapScale(1.0f);

            TSTR diffuseStr, normalStr, glowStr, dispStr, envStr, envMaskStr, specularStr, parallaxStr;

            if(mtl && mtl->ClassID() == Class_ID(DMTL_CLASS_ID, 0) ) {
               StdMat2 *m = (StdMat2*)mtl;

               if(mtl && mtl->ClassID() == Class_ID(DMTL_CLASS_ID, 0) ) {
                  StdMat2 *m = (StdMat2*)mtl;
                  if (m->GetMapState(C_BASE) == 2) {
                     if (Texmap *texMap = m->GetSubTexmap(C_BASE)) {
                        GetTexFullName(texMap, diffuseStr);
                     }
                  }

                  if (m->GetMapState(C_NORMAL) == 2) {
                     if (Texmap *texMap = m->GetSubTexmap(C_NORMAL)) {
                        if (texMap->ClassID() == GNORMAL_CLASS_ID) {
                           texMap = texMap->GetSubTexmap(0);
                           GetTexFullName(texMap, normalStr);
                        } else {
                           GetTexFullName(texMap, normalStr);
                        }
                     }
                  }
                  if (m->GetMapState(C_ENVMASK) == 2) {
                     if (Texmap *texMap = m->GetSubTexmap(C_ENVMASK)) {
                        if (texMap->ClassID() == Class_ID(MASK_CLASS_ID, 0)) {
                           Texmap *envMap = texMap->GetSubTexmap(0);
                           Texmap *envMaskMap = texMap->GetSubTexmap(1);
                           GetTexFullName(envMap, envStr);
                           GetTexFullName(envMaskMap, envMaskStr);
                        } else {
                           GetTexFullName(texMap, envMaskStr);
                        }
                     }
                  }
                  if (m->GetMapState(C_GLOW) == 2) {
                     if (Texmap *texMap = m->GetSubTexmap(C_GLOW)) {
                        GetTexFullName(texMap, glowStr);
                     }
                  }
                  if (m->GetMapState(C_HEIGHT) == 2) {
                     if (Texmap *texMap = m->GetSubTexmap(C_HEIGHT)) {
                        GetTexFullName(texMap, dispStr);
                     }
                  }
                  if (m->GetMapState(C_ENV) == 2) {
                     if (Texmap *texMap = m->GetSubTexmap(C_ENV)) {
                        if (texMap->ClassID() == Class_ID(MASK_CLASS_ID, 0)) {
                           GetTexFullName(texMap->GetSubTexmap(0), envStr);
                           GetTexFullName(texMap->GetSubTexmap(1), envMaskStr);
                        } else {
                           GetTexFullName(texMap, envStr);
                        }
                     }
                  }
                  if (m->GetMapState(C_SPECULAR) == 2) {
                     if (Texmap *texMap = m->GetSubTexmap(C_SPECULAR)) {
                        GetTexFullName(texMap, specularStr);
                     }
                  }
                  if (m->GetMapState(C_PARALLAX) == 2) {
                     if (Texmap *texMap = m->GetSubTexmap(C_PARALLAX)) {
                        GetTexFullName(texMap, specularStr);
                     }
                  }
               }
               textures[0] = mAppSettings->GetRelativeTexPath(string(diffuseStr), mTexPrefix);
               if (normalStr.isNull()) {
                  char path_buffer[_MAX_PATH], drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME], ext[_MAX_EXT];
                  _splitpath(textures[0].c_str(), drive, dir, fname, ext);
                  strcat(fname, "_n");
                  _makepath(path_buffer, drive, dir, fname, ext);
                  textures[1] = path_buffer;			
               }
               else
                  textures[1] = mAppSettings->GetRelativeTexPath(string(normalStr), mTexPrefix);
               if (!glowStr.isNull())
               {
                  textures[2] = mAppSettings->GetRelativeTexPath(string(glowStr), mTexPrefix);
                  flags1 = (SkyrimShaderPropertyFlags1)(flags1 | SLSF1_SKINNED | SLSF1_CAST_SHADOWS | SLSF2_SOFT_LIGHTING);
               }
               if (!dispStr.isNull())
               {
                  textures[3] = mAppSettings->GetRelativeTexPath(string(dispStr), mTexPrefix);
                  //flags1 = (SkyrimShaderPropertyFlags1)(flags1 | Niflib::SLSF1_Skinned);
               }
               if (!envStr.isNull())
               {
                  textures[4] = mAppSettings->GetRelativeTexPath(string(envStr), mTexPrefix);
                  flags1 = (SkyrimShaderPropertyFlags1)(flags1 | Niflib::SLSF1_ENVIRONMENT_MAPPING);
               }
               if (!envMaskStr.isNull())
               {
                  textures[5] = mAppSettings->GetRelativeTexPath(string(envMaskStr), mTexPrefix);
                  flags1 = (SkyrimShaderPropertyFlags1)(flags1 | Niflib::SLSF1_ENVIRONMENT_MAPPING);
               }
               if (!specularStr.isNull())
               {
                  textures[7] = mAppSettings->GetRelativeTexPath(string(specularStr), mTexPrefix);
				  flags1 = (SkyrimShaderPropertyFlags1)(flags1 | Niflib::SLSF1_SPECULAR);
               }

               if ( m->GetTwoSided() ) flags2 = (SkyrimShaderPropertyFlags2)(flags2 | SLSF2_DOUBLE_SIDED);


               texProp->SetShaderFlags1(flags1);
               texProp->SetShaderFlags2(flags2);

               TimeValue t = 0;
               texProp->SetGlossiness(m->GetShininess(t) * 100.0f);
               //texProp->SetSpecularColor(m->getsh);
               //texProp->SetSpecularStrength(m->GetShinStr(t) < 1.0f ? 3.0f : 0.0f);
               texProp->SetAlpha(m->GetOpacity(t) / 100.0f);
               texProp->SetSpecularColor(TOCOLOR3(m->GetSpecular(t)));
               //texProp->SetEmissiveColor(TOCOLOR(m->GetEmmis(t)));

               //texProp->SetLightingEffect1(0.3f);
               //texProp->SetLightingEffect1(2.0f);
               //texProp->SetEnvironmentMapStrength(1.0f);
            }

            textures[0] = mAppSettings->GetRelativeTexPath(string(diffuseStr), mTexPrefix);
            if (normalStr.isNull()) {
               char path_buffer[_MAX_PATH], drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME], ext[_MAX_EXT];
               _splitpath(textures[0].c_str(), drive, dir, fname, ext);
               strcat(fname, "_n");
               _makepath(path_buffer, drive, dir, fname, ext);
               textures[1] = path_buffer;			
            }
            else
               textures[1] = mAppSettings->GetRelativeTexPath(string(normalStr), mTexPrefix);
            if (!glowStr.isNull())
               textures[2] = mAppSettings->GetRelativeTexPath(string(glowStr), mTexPrefix);
            if (!parallaxStr.isNull())
               textures[3] = mAppSettings->GetRelativeTexPath(string(parallaxStr), mTexPrefix);
            if (!envStr.isNull())
               textures[4] = mAppSettings->GetRelativeTexPath(string(envStr), mTexPrefix);
            if (!envMaskStr.isNull())
               textures[5] = mAppSettings->GetRelativeTexPath(string(envMaskStr), mTexPrefix);
            if (!envStr.isNull())
               textures[5] = mAppSettings->GetRelativeTexPath(string(envStr), mTexPrefix);

            texset->SetTextures(textures);

            // shader must be first, alpha can be second
            NiPropertyRef prop = DynamicCast<NiProperty>(texProp);
            vector<NiPropertyRef> properties = parent->GetProperties();
            /*parent->ClearProperties();*/
            parent->AddProperty(prop);
            if (properties.size() > 0)
               parent->AddProperty(properties[0]);
               
         }
         useDefaultShader = false;
      }
		else if (IsFallout3())
		{
			NiObjectRef root;
			if (CustomShader != NULL && strlen(CustomShader) != 0)
				root = Niflib::ObjectRegistry::CreateObject(CustomShader);

			if (root == NULL) {
				if (IsSkyrim()) {
					root = new BSLightingShaderProperty();
				} else {
					root = new BSShaderPPLightingProperty();
				}
			}

			if ( BSShaderPPLightingPropertyRef texProp = DynamicCast<BSShaderPPLightingProperty>(root) )
			{
				
				texProp->SetFlags(1);
				if (DynamicCast<Lighting30ShaderProperty>(root) != NULL)
					texProp->SetShaderType( SHADER_LIGHTING30 );
				else
					texProp->SetShaderType( SHADER_DEFAULT );

				BSShaderTextureSetRef texset = new BSShaderTextureSet();
				texProp->SetTextureSet( texset );

				vector<string> textures;
				textures.resize(6);

				TSTR diffuseStr, normalStr, glowStr, dispStr, envStr, envMaskStr;

				if(mtl && mtl->ClassID() == Class_ID(DMTL_CLASS_ID, 0) ) {
					StdMat2 *m = (StdMat2*)mtl;
					if (m->GetMapState(C_BASE) == 2) {
						if (Texmap *texMap = m->GetSubTexmap(C_BASE)) {
							GetTexFullName(texMap, diffuseStr);
						}
					}

					if (m->GetMapState(C_NORMAL) == 2) {
						if (Texmap *texMap = m->GetSubTexmap(C_NORMAL)) {
							if (texMap->ClassID() == GNORMAL_CLASS_ID) {
								texMap = texMap->GetSubTexmap(0);
								GetTexFullName(texMap, normalStr);
							} else {
								GetTexFullName(texMap, normalStr);
							}
						}
					}
					if (m->GetMapState(C_ENVMASK) == 2) {
						if (Texmap *texMap = m->GetSubTexmap(C_ENVMASK)) {
							if (texMap->ClassID() == Class_ID(MASK_CLASS_ID, 0)) {
								Texmap *envMap = texMap->GetSubTexmap(0);
								Texmap *envMaskMap = texMap->GetSubTexmap(1);
								GetTexFullName(envMap, envStr);
								GetTexFullName(envMaskMap, envMaskStr);
							} else {
								GetTexFullName(texMap, envMaskStr);
							}
						}
					}
					if (m->GetMapState(C_GLOW) == 2) {
						if (Texmap *texMap = m->GetSubTexmap(C_GLOW)) {
							GetTexFullName(texMap, glowStr);
						}
					}
					if (m->GetMapState(C_HEIGHT) == 2) {
						if (Texmap *texMap = m->GetSubTexmap(C_HEIGHT)) {
							GetTexFullName(texMap, dispStr);
						}
					}
					if (m->GetMapState(C_ENV) == 2) {
						if (Texmap *texMap = m->GetSubTexmap(C_ENV)) {
							if (texMap->ClassID() == Class_ID(MASK_CLASS_ID, 0)) {
								GetTexFullName(texMap->GetSubTexmap(0), envStr);
								GetTexFullName(texMap->GetSubTexmap(1), envMaskStr);
							} else {
								GetTexFullName(texMap, envStr);
							}
						}
					}
				}
				textures[0] = mAppSettings->GetRelativeTexPath(string(diffuseStr), mTexPrefix);
				if (normalStr.isNull()) {
					char path_buffer[_MAX_PATH], drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME], ext[_MAX_EXT];
					_splitpath(textures[0].c_str(), drive, dir, fname, ext);
					strcat(fname, "_n");
					_makepath(path_buffer, drive, dir, fname, ext);
					textures[1] = path_buffer;			
				}
				else
					textures[1] = mAppSettings->GetRelativeTexPath(string(normalStr), mTexPrefix);
				if (!glowStr.isNull())
					textures[2] = mAppSettings->GetRelativeTexPath(string(glowStr), mTexPrefix);
				if (!envMaskStr.isNull())
					textures[2] = mAppSettings->GetRelativeTexPath(string(envMaskStr), mTexPrefix);
				if (!dispStr.isNull())
					textures[4] = mAppSettings->GetRelativeTexPath(string(dispStr), mTexPrefix);
				if (!envStr.isNull())
					textures[5] = mAppSettings->GetRelativeTexPath(string(envStr), mTexPrefix);

				BSShaderFlags shFlags = BSShaderFlags(SF_SPECULAR | SF_SHADOW_MAP | SF_SHADOW_FRUSTUM | SF_EMPTY | SF_ZBUFFER_TEST);
				if (!envStr.isNull() || !dispStr.isNull() || !envMaskStr.isNull())
					shFlags = BSShaderFlags(shFlags | SF_MULTIPLE_TEXTURES);
				texProp->SetShaderFlags(shFlags);

				texset->SetTextures(textures);

				NiPropertyRef prop = DynamicCast<NiProperty>(texProp);
				parent->AddProperty(prop);
				useDefaultShader = false;
			}
			else if (BSShaderNoLightingPropertyRef texProp = StaticCast<BSShaderNoLightingProperty>(root))
			{
				texProp->SetFlags(1);
				texProp->SetShaderType( SHADER_NOLIGHTING );

				TSTR diffuseStr;
				if(mtl && mtl->ClassID() == Class_ID(DMTL_CLASS_ID, 0) ) {
					StdMat2 *m = (StdMat2*)mtl;
					if (m->GetMapState(C_BASE) == 2) {
						if (Texmap *texMap = m->GetSubTexmap(C_BASE)) {
							GetTexFullName(texMap, diffuseStr);
						}
					}
				}
				texProp->SetFileName( diffuseStr.data() );
				NiPropertyRef prop = DynamicCast<NiProperty>(texProp);
				parent->AddProperty(prop);
				useDefaultShader = false;
			}
			else if ( WaterShaderPropertyRef texProp = StaticCast<WaterShaderProperty>(root) )
			{
				texProp->SetFlags(1);
				texProp->SetShaderType( SHADER_WATER);

				TSTR diffuseStr;
				if(mtl && mtl->ClassID() == Class_ID(DMTL_CLASS_ID, 0) ) {
					StdMat2 *m = (StdMat2*)mtl;
					if (m->GetMapState(C_BASE) == 2) {
						if (Texmap *texMap = m->GetSubTexmap(C_BASE)) {
							GetTexFullName(texMap, diffuseStr);
						}
					}
				}
				//texProp->SetFileName( string(diffuseStr.data()) );
				NiPropertyRef prop = DynamicCast<NiProperty>(texProp);
				parent->AddProperty(prop);
				useDefaultShader = false;
			}
			else if ( SkyShaderPropertyRef texProp = StaticCast<SkyShaderProperty>(root) )
			{
				texProp->SetFlags(1);
				texProp->SetShaderType(SHADER_SKY);

				TSTR diffuseStr;
				if(mtl && mtl->ClassID() == Class_ID(DMTL_CLASS_ID, 0) ) {
					StdMat2 *m = (StdMat2*)mtl;
					if (m->GetMapState(C_BASE) == 2) {
						if (Texmap *texMap = m->GetSubTexmap(C_BASE)) {
							GetTexFullName(texMap, diffuseStr);
						}
					}
				}
				texProp->SetFileName( diffuseStr.data() );
				NiPropertyRef prop = DynamicCast<NiProperty>(texProp);
				parent->AddProperty(prop);
				useDefaultShader = false;
			}
			else if ( TallGrassShaderPropertyRef texProp = StaticCast<TallGrassShaderProperty>(root) )
			{
				texProp->SetFlags(1);
				texProp->SetShaderType( SHADER_TALL_GRASS );

				TSTR diffuseStr;
				if(mtl && mtl->ClassID() == Class_ID(DMTL_CLASS_ID, 0) ) {
					StdMat2 *m = (StdMat2*)mtl;
					if (m->GetMapState(C_BASE) == 2) {
						if (Texmap *texMap = m->GetSubTexmap(C_BASE)) {
							GetTexFullName(texMap, diffuseStr);
						}
					}
				}
				texProp->SetFileName( diffuseStr.data() );
				NiPropertyRef prop = DynamicCast<NiProperty>(texProp);
				parent->AddProperty(prop);
				useDefaultShader = false;
			}
		}
		if (useDefaultShader)
		{
			StdMat2 *m2 = NULL;
			if(mtl->ClassID() == Class_ID(DMTL_CLASS_ID, 0)) m2 = (StdMat2*)mtl;

			int ntex = mtl->NumSubTexmaps();
			if (ntex > 0)
			{
				int maxTexture = 7;
				NiTexturingPropertyRef texProp;
				for (int i = 0; i < ntex; ++i) {
					BitmapTex *bmTex = getTexture(mtl, i);
					if (!bmTex || (m2 && (m2->GetMapState(i)) != 2))
						continue;

					TexType textype = (TexType)i;
					if ( isCiv4Shader ) {
						const TexType civmap[] = {BASE_MAP, DARK_MAP, DETAIL_MAP, DECAL_0_MAP, BUMP_MAP, GLOSS_MAP, GLOW_MAP, DECAL_1_MAP, DECAL_2_MAP, DECAL_3_MAP};
						textype = civmap[i];
					} else if (isGamebryoShader) {
						const TexType civmap[] = {BASE_MAP, DARK_MAP, DETAIL_MAP, DECAL_0_MAP, NORMAL_MAP, UNKNOWN2_MAP, BUMP_MAP, GLOSS_MAP, GLOW_MAP, DECAL_1_MAP, DECAL_2_MAP, DECAL_3_MAP};
						textype = civmap[i];
					}

					// Fallout 3 only
					if ( textype >= C_ENVMASK )
						continue;

					if (texProp == NULL)
					{
						texProp = new NiTexturingProperty();       
						texProp->SetApplyMode(Niflib::ApplyMode(ApplyMode));
						texProp->SetTextureCount(7);
					}
					if ( Exporter::mNifVersionInt <= 0x14010003)
					{
						if (textype == C_DECAL0)
							texProp->SetTextureCount(9);
						else if (textype == C_DECAL1)
							texProp->SetTextureCount(10);
						else if (textype > C_DECAL1)
							continue;
					}
					else if ( textype >= texProp->GetTextureCount() )
					{
						texProp->SetTextureCount(textype + 1);
					}

					TexDesc td;
					NiTriBasedGeomRef shape(DynamicCast<NiTriBasedGeom>(parent));
					if (makeTextureDesc(bmTex, td, shape->GetData())) {
						if (textype == BUMP_MAP) {
							td.source->SetPixelLayout(PIX_LAY_BUMPMAP);
							texProp->SetLumaOffset(LumaOffset);
							texProp->SetLumaScale(LumaScale);

							Matrix22 m2;
							m2[0][0] = m2[1][1] = Magnitude;
							m2[0][1] = m2[1][0] = 0.0f;
							texProp->SetBumpMapMatrix(m2);
						}
						texProp->SetTexture(textype, td);

						// kludge for setting decal maps without messing up the file sizes
						if ( Exporter::mNifVersionInt <= 0x14010003)
						{
							if (textype == C_DECAL0)
								texProp->SetTextureCount(7);
							else if (textype == C_DECAL1)
								texProp->SetTextureCount(8);
						}

						if ( textype > maxTexture )
							maxTexture = textype;
					}
				}
				if ( Exporter::mNifVersionInt < 0x14010003 && maxTexture > 8)
					texProp->SetTextureCount(8);
				else if ( Exporter::mNifVersionInt > 0x14010003 && maxTexture > 12)
					texProp->SetTextureCount(12);
				if (texProp != NULL)
				{
					parent->AddProperty(texProp);
				}
			}
		}
		return true;
	}
	return false;
}