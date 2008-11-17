#include "pch.h"
#include "stdmat.h"
#include "shaders.h"
#include "AppSettings.h"
#include "obj/NiWireframeProperty.h"
#include "obj/NiAlphaProperty.h"
#include "obj/NiStencilProperty.h"
#include "obj/NiShadeProperty.h"
#include "obj/NiVertexColorProperty.h"
#include "obj/NiDitherProperty.h"
#include "obj/NiSpecularProperty.h"
#include "obj/NiTextureProperty.h"
#include "obj/NiImage.h"
#include "obj/BSShaderPPLightingProperty.h"
#include "obj/BSShaderTextureSet.h"

void Exporter::makeTexture(NiAVObjectRef &parent, Mtl *mtl)
{
	BitmapTex *bmTex = getTexture(mtl);
	if (!bmTex)
		return;


	if (IsFallout3())
	{
		BSShaderPPLightingPropertyRef texProp = new BSShaderPPLightingProperty();
		texProp->SetFlags(1);
		texProp->SetShaderType( SHADER_DEFAULT );
		BSShaderTextureSetRef texset = new BSShaderTextureSet();
		texProp->SetTextureSet( texset );
		
		vector<string> textures;
		textures.resize(6);
		TexDesc td;
		if (makeTextureDesc(bmTex, td)) {
			char path_buffer[_MAX_PATH], drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME], ext[_MAX_EXT];
			textures[0] = td.source->GetTextureFileName();
			_splitpath(textures[0].c_str(), drive, dir, fname, ext);
			strcat(fname, "_n");
			_makepath(path_buffer, drive, dir, fname, ext);
			textures[1] = path_buffer;			
		}
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
		if (makeTextureDesc(bmTex, td))
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

bool Exporter::makeTextureDesc(BitmapTex *bmTex, TexDesc& td)
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
   bool done = exportCiv4Shader(parent, mtl);
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
	parent->AddProperty(prop);

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

	BitmapTex *bmTex = NULL;
	for (int i=0; i<texMaps; i++)
	{
		Texmap *texMap = mtl->GetSubTexmap(i);
		if (!texMap)
			continue;

		if (texMap->ClassID() == Class_ID(BMTEX_CLASS_ID, 0))
		{
			bmTex = (BitmapTex*)texMap;
			break;
		}
	}

	return bmTex;
}

BitmapTex *Exporter::getTexture(Mtl *mtl, int i)
{
   if (mtl) {
      int texMaps = mtl->NumSubTexmaps();
      if (i < texMaps) {
         if (Texmap *texMap = mtl->GetSubTexmap(i)) {
            if (texMap->ClassID() == Class_ID(BMTEX_CLASS_ID, 0)) {
               return (BitmapTex*)texMap;
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


bool Exporter::exportCiv4Shader(NiAVObjectRef parent, Mtl* mtl)
{
   if (!mtl) 
      return false;

   RefTargetHandle ref = mtl->GetReference(2/*shader*/);
   if (!ref)
      return false;

   TSTR shaderByName;
   if(mtl->ClassID() == Class_ID(DMTL_CLASS_ID, 0) )
   {
      StdMat2 * smtl = (StdMat2*)mtl;
      if (smtl->SupportsShaders()) {
         if (Shader *s = smtl->GetShader()) {
            s->GetClassName(shaderByName);
         }
      }
   }
   if (shaderByName != TSTR("CivilizationIV Shader"))
         return false;

   //if (Shader *s = mtl->GetShader()) {
   //   TSTR className;
   //   s->GetClassName(className);
   //   if (className != TSTR("CivilizationIV Shader"))
   //      return false;

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
   if(ok) ok &= getMAXScriptValue(ref, "UseNormalMaps", 0, UseNormalMaps );
   if(ok) ok &= getMAXScriptValue(ref, "srcBlend", 0, srcBlend);
   if(ok) ok &= getMAXScriptValue(ref, "destBlend", 0, destBlend);
   if(ok) ok &= getMAXScriptValue(ref, "TestMode", 0, TestMode );
   if(ok) ok &= getMAXScriptValue(ref, "ApplyMode", 0, ApplyMode);
   if(ok) ok &= getMAXScriptValue(ref, "SourceVertexMode", 0, SrcVertexMode);
   if(ok) ok &= getMAXScriptValue(ref, "LightingMode", 0, LightingMode);
   if(ok) ok &= getMAXScriptValue(ref, "alphaMode", 0, alphaMode);
   if(ok) ok &= getMAXScriptValue(ref, "BaseTextureExport", 0, BaseTextureExport);
   if(ok) ok &= getMAXScriptValue(ref, "DarkTextureExport", 0, DarkTextureExport);
   if(ok) ok &= getMAXScriptValue(ref, "DetailTextureExport", 0, DetailTextureExport);
   if(ok) ok &= getMAXScriptValue(ref, "Decal1TextureExport", 0, Decal1TextureExport);
   if(ok) ok &= getMAXScriptValue(ref, "Decal2TextureExport", 0, Decal2TextureExport);
   if(ok) ok &= getMAXScriptValue(ref, "GlossTextureExport", 0, GlossTextureExport);
   if(ok) ok &= getMAXScriptValue(ref, "GlowTextureExport", 0, GlowTextureExport);
   if(ok) ok &= getMAXScriptValue(ref, "CustomShader", 0, CustomShader );
   if(ok) ok &= getMAXScriptValue(ref, "ShaderViewerTechnique", 0, ShaderViewerTechnique);
   if(ok) ok &= getMAXScriptValue(ref, "ShaderExportTechnique", 0, ShaderExportTechnique);
   if(ok) ok &= getMAXScriptValue(ref, "NormalMapTechnique", 0, NormalMapTechnique );
   
   if (ok) // civ4 shader
   {
      NiMaterialPropertyRef mtlProp = CreateNiObject<NiMaterialProperty>();
      parent->AddProperty(mtlProp);

      mtlProp->SetName((char*)mtl->GetName());
      mtlProp->SetAmbientColor(TOCOLOR3(ambient));
      mtlProp->SetDiffuseColor(TOCOLOR3(diffuse));
      mtlProp->SetSpecularColor(TOCOLOR3(specular));
      mtlProp->SetEmissiveColor(TOCOLOR3(emittance));
      mtlProp->SetGlossiness(shininess);
      mtlProp->SetTransparency(alpha/100.0f);
      if(mtl->ClassID() == Class_ID(DMTL_CLASS_ID, 0) )
      {
         StdMat2 * smtl = (StdMat2*)mtl;
         if (smtl->SupportsShaders()) {
            if (Shader *s = smtl->GetShader()) {
               if (smtl->GetWire()){
                  NiWireframePropertyRef wireProp = CreateNiObject<NiWireframeProperty>();
                  wireProp->SetFlags(1);
                  parent->AddProperty(wireProp);
               }
               if (smtl->GetTwoSided()){
                  NiStencilPropertyRef stencil = CreateNiObject<NiStencilProperty>();
                  stencil->SetStencilFunction(TEST_GREATER);
                  stencil->SetStencilState(false);
                  stencil->SetPassAction(ACTION_INCREMENT);
                  stencil->SetFaceDrawMode(DRAW_BOTH);
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
      if (VertexColorsEnable && (SrcVertexMode != 2 || LightingMode != 1)) {
         NiVertexColorPropertyRef vertexColor = CreateNiObject<NiVertexColorProperty>();
         parent->AddProperty(vertexColor);
         vertexColor->SetVertexMode(VertMode(SrcVertexMode));
         vertexColor->SetLightingMode(LightMode(LightingMode));
         vertexColor->SetFlags(LightingMode + (SrcVertexMode << 3));
      }
      if (SpecularEnable) {
         NiSpecularPropertyRef prop = CreateNiObject<NiSpecularProperty>();
         parent->AddProperty(prop);
         prop->SetFlags(1);

      }
      if (Dither) {
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
         alphaProp->SetTriangleSortMode(!NoSorter);
         alphaProp->SetTestThreshold(TestRef);
         alphaProp->SetBlendState(AlphaTestEnable);
         parent->AddProperty(alphaProp);
      }

      int ntex = mtl->NumSubTexmaps();
      if (ntex > 0)
      {
         ntex = min(ntex, 7);
         TexType texmap[] = {BASE_MAP, DARK_MAP, DETAIL_MAP, DECAL_0_MAP, BUMP_MAP, GLOSS_MAP, GLOW_MAP, DECAL_1_MAP};
         NiTexturingPropertyRef texProp;
         for (int i = 0; i < ntex; ++i) {
            BitmapTex *bmTex = getTexture(mtl, i);
            if (!bmTex)
               continue;

            if (texProp == NULL)
            {
               texProp = new NiTexturingProperty();       
               texProp->SetApplyMode(Niflib::ApplyMode(ApplyMode));
               texProp->SetTextureCount(7);
            }
            TexDesc td;
            if (makeTextureDesc(bmTex, td)) {
               TexType textype = texmap[i];
               texProp->SetTexture(textype, td);
               if (textype == BUMP_MAP) {
                  td.source->SetPixelLayout(PIX_LAY_BUMPMAP);
                  texProp->SetLumaOffset(LumaOffset);
                  texProp->SetLumaScale(LumaScale);

                  Matrix22 m2;
                  m2[0][0] = m2[1][1] = Magnitude;
                  m2[0][1] = m2[1][0] = 0.0f;
                  texProp->SetBumpMapMatrix(m2);
               }
            }
         }

         if (texProp != NULL)
         {
            parent->AddProperty(texProp);
         }
      }
      return true;
   }
   return false;
}
