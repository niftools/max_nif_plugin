/**********************************************************************
*<
FILE: ImportMtlAndTex.cpp

DESCRIPTION:	Material and Texture Import routines

CREATED BY: tazpn (Theo)

HISTORY: 

*>	Copyright (c) 2006, All Rights Reserved.
**********************************************************************/
#include "stdafx.h"
#include <shaders.h>
#include <maxtypes.h>
#include "MaxNifImport.h"
#include "obj/NiWireframeProperty.h"
#include "obj/NiAlphaProperty.h"
#include "obj/NiStencilProperty.h"
#include "obj/NiShadeProperty.h"
#include "obj/NiVertexColorProperty.h"
#include "obj/NiDitherProperty.h"
#include "obj/NiSpecularProperty.h"
#include "obj/NiTextureProperty.h"
#include "obj/BSLightingShaderProperty.h"
#include "obj/BSShaderNoLightingProperty.h"
#include "obj/BSShaderPPLightingProperty.h"
#include "obj/BSShaderTextureSet.h"
#include "obj/SkyShaderProperty.h"
#include "obj/TileShaderProperty.h"
#include "obj/TallGrassShaderProperty.h"
#include "obj/Lighting30ShaderProperty.h"
#include "obj/NiImage.h"
#include "objectParams.h"
using namespace Niflib;

enum { C_BASE, C_DARK, C_DETAIL, C_GLOSS, C_GLOW, C_BUMP, C_NORMAL, C_UNK2, 
       C_DECAL0, C_DECAL1, C_DECAL2, C_ENVMASK, C_ENV, C_HEIGHT, C_REFLECTION,
       C_OPACITY, C_SPECULAR, C_PARALLAX
};

#undef GNORMAL_CLASS_ID
static const Class_ID GNORMAL_CLASS_ID(0x243e22c6, 0x63f6a014);

Texmap* NifImporter::CreateNormalBump(LPCTSTR name, Texmap* nmap)
{
   Texmap *texmap = (Texmap*)gi->CreateInstance(TEXMAP_CLASS_ID, GNORMAL_CLASS_ID);
   if(texmap != NULL)
   {
      TSTR tname = (name == NULL) ? FormatText("Norm %s", nmap->GetName().data()) : TSTR(name);
      texmap->SetName(tname);
      texmap->SetSubTexmap(0, nmap);
      return texmap;
   }
   return nmap;
}

Texmap* NifImporter::CreateMask(LPCTSTR name, Texmap* map, Texmap* mask)
{
   Texmap *texmap = (Texmap*)gi->CreateInstance(TEXMAP_CLASS_ID, Class_ID(MASK_CLASS_ID, 0));
   if(texmap != NULL)
   {
      TSTR tname = (name == NULL) ? FormatText("Mask %s", map->GetName().data()) : TSTR(name);
      texmap->SetName(tname);
      texmap->SetSubTexmap(0, map);
      texmap->SetSubTexmap(0, mask);
      return texmap;
   }
   return map;
}


Texmap* NifImporter::CreateTexture(TexDesc& desc)
{
   BitmapManager *bmpMgr = TheManager;
   if (NiSourceTextureRef texSrc = desc.source){
      string filename = texSrc->GetTextureFileName();
      if (bmpMgr->CanImport(filename.c_str())){
         BitmapTex *bmpTex = NewDefaultBitmapTex();
         string name = texSrc->GetName();
         if (name.empty()) {
            TCHAR buffer[MAX_PATH];
            _tcscpy(buffer, PathFindFileName(filename.c_str()));
            PathRemoveExtension(buffer);
            name = buffer;
         }         
         bmpTex->SetName(name.c_str());
         bmpTex->SetMapName(const_cast<TCHAR*>(FindImage(filename).c_str()));
         bmpTex->SetAlphaAsMono(TRUE);
         bmpTex->SetAlphaSource(ALPHA_DEFAULT);

         switch (desc.filterMode)
         {
         case FILTER_TRILERP: bmpTex->SetFilterType(FILTER_PYR); break;
         case FILTER_BILERP:  bmpTex->SetFilterType(FILTER_SAT); break;
         case FILTER_NEAREST: bmpTex->SetFilterType(FILTER_NADA); break;
         }

         if (UVGen *uvGen = bmpTex->GetTheUVGen()){
            if (uvGen && uvGen->IsStdUVGen()) {
               StdUVGen *uvg = (StdUVGen*)uvGen;
               uvg->SetMapChannel(desc.uvSet + 1);
            }

            switch (desc.clampMode)
            {
            case WRAP_S_WRAP_T : uvGen->SetTextureTiling(3); break;
            case WRAP_S_CLAMP_T: uvGen->SetTextureTiling(1); break;
            case CLAMP_S_WRAP_T: uvGen->SetTextureTiling(2); break;
            case CLAMP_S_CLAMP_T:uvGen->SetTextureTiling(0); break;
            }

            if (desc.hasTextureTransform) {
               if (RefTargetHandle ref = uvGen->GetReference(0)){
                  TexCoord trans = desc.translation;
                  TexCoord tiling = desc.tiling;
                  float wangle = TODEG(desc.wRotation);

                  setMAXScriptValue(ref, "U_Offset", 0, trans.u);
                  setMAXScriptValue(ref, "V_Offset", 0, trans.v);
                  setMAXScriptValue(ref, "U_Tiling", 0, tiling.u);
                  setMAXScriptValue(ref, "V_Tiling", 0, tiling.v);
                  setMAXScriptValue(ref, "W_Angle", 0, wangle);
               }
            }
         }
		 if (showTextures) {
			 bmpTex->SetMtlFlag(MTL_TEX_DISPLAY_ENABLED, TRUE);
			 bmpTex->ActivateTexDisplay(TRUE);
			 bmpTex->NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
		 }

         return bmpTex;
      }
   }
   return NULL;
}

Texmap* NifImporter::CreateTexture(NiTexturePropertyRef texSrc)
{
	BitmapManager *bmpMgr = TheManager;
	if (NiImageRef imgRef = texSrc->GetImage()) {
		string filename = imgRef->GetTextureFileName();
		if (bmpMgr->CanImport(filename.c_str())){
			BitmapTex *bmpTex = NewDefaultBitmapTex();
			string name = texSrc->GetName();
			if (name.empty()) {
				TCHAR buffer[MAX_PATH];
				_tcscpy(buffer, PathFindFileName(filename.c_str()));
				PathRemoveExtension(buffer);
				name = buffer;
			}         
			bmpTex->SetName(name.c_str());
			bmpTex->SetMapName(const_cast<TCHAR*>(FindImage(filename).c_str()));
			bmpTex->SetAlphaAsMono(TRUE);
			bmpTex->SetAlphaSource(ALPHA_DEFAULT);

			bmpTex->SetFilterType(FILTER_PYR); 

			if (showTextures) {
				bmpTex->SetMtlFlag(MTL_TEX_DISPLAY_ENABLED, TRUE);
				bmpTex->ActivateTexDisplay(TRUE);
				bmpTex->NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
			}

			if (UVGen *uvGen = bmpTex->GetTheUVGen()){
				uvGen->SetTextureTiling(0);
			}

			return bmpTex;
		}
	}
	return NULL;
}

Texmap* NifImporter::CreateTexture(const string& filename)
{
	if (filename.empty())
		return NULL;

	BitmapManager *bmpMgr = TheManager;
	if (bmpMgr->CanImport(filename.c_str())){
		BitmapTex *bmpTex = NewDefaultBitmapTex();
		string name = filename;
		if (name.empty()) {
			TCHAR buffer[MAX_PATH];
			_tcscpy(buffer, PathFindFileName(filename.c_str()));
			PathRemoveExtension(buffer);
			name = buffer;
		}         
		bmpTex->SetName(name.c_str());
		bmpTex->SetMapName(const_cast<TCHAR*>(FindImage(filename).c_str()));
		bmpTex->SetAlphaAsMono(TRUE);
		bmpTex->SetAlphaSource(ALPHA_DEFAULT);

		bmpTex->SetFilterType(FILTER_PYR); 

		if (showTextures) {
			bmpTex->SetMtlFlag(MTL_TEX_DISPLAY_ENABLED, TRUE);
			bmpTex->ActivateTexDisplay(TRUE);
			bmpTex->NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
		}

		if (UVGen *uvGen = bmpTex->GetTheUVGen()){
			uvGen->SetTextureTiling(0);
		}

		return bmpTex;
	}
	return NULL;
}

StdMat2 *NifImporter::ImportMaterialAndTextures(ImpNode *node, NiAVObjectRef avObject)
{
	// Texture
	NiGeometryRef geoprop=DynamicCast<NiGeometry>(avObject);
	vector<NiPropertyRef> props = geoprop->GetProperties();
	NiMaterialPropertyRef matRef = SelectFirstObjectOfType<NiMaterialProperty>(props);
	BSShaderPropertyRef shaderRef = SelectFirstObjectOfType<BSShaderProperty>(props);
   BSLightingShaderPropertyRef lightingShaderRef = SelectFirstObjectOfType<BSLightingShaderProperty>(props);
   if (matRef != NULL || shaderRef != NULL || lightingShaderRef != NULL){

		StdMat2 *m = NewDefaultStdMat();

		if (matRef != NULL) {
			m->SetName(matRef->GetName().c_str());
		} else {
			m->SetName(avObject->GetName().c_str());
		}
		if (showTextures) {
			m->SetMtlFlag(MTL_DISPLAY_ENABLE_FLAGS, TRUE);
		}

		// try the civ4 shader first then default back to normal shaders
		if (ImportNiftoolsShader(node, avObject, m)) {
			return m;
		}

		NiTexturingPropertyRef texRef = avObject->GetPropertyByType(NiTexturingProperty::TYPE);
		NiWireframePropertyRef wireRef = avObject->GetPropertyByType(NiWireframeProperty::TYPE);
		NiAlphaPropertyRef alphaRef = avObject->GetPropertyByType(NiAlphaProperty::TYPE);
		NiStencilPropertyRef stencilRef = avObject->GetPropertyByType(NiStencilProperty::TYPE);
		NiShadePropertyRef shadeRef = avObject->GetPropertyByType(NiShadeProperty::TYPE);

		if (IsFallout3() || IsSkyrim()) {
			m->SetAmbient(Color(0.588f, 0.588f, 0.588f),0);
			m->SetDiffuse(Color(0.588f, 0.588f, 0.588f),0);
			m->SetSpecular(Color(0.902f, 0.902f, 0.902f),0);
		} else if (matRef != NULL) {
			m->SetAmbient(TOCOLOR(matRef->GetAmbientColor()),0);
			m->SetDiffuse(TOCOLOR(matRef->GetDiffuseColor()),0);
			m->SetSpecular(TOCOLOR(matRef->GetSpecularColor()),0);
		}
		if (matRef != NULL) {
			Color c = TOCOLOR(matRef->GetEmissiveColor());
			if (c.r != 0 || c.b != 0 || c.g != 0) {
				m->SetSelfIllumColorOn(TRUE);
				m->SetSelfIllumColor(c,0);
			}
			m->SetShinStr(0.0,0);
			m->SetOpacity(matRef->GetTransparency(),0);
		}
		bool hasShaderAttributes = (wireRef != NULL) || (stencilRef != NULL) || (shadeRef != NULL);
		if (m->SupportsShaders() && hasShaderAttributes) {
			if (Shader *s = m->GetShader()) {
				if (wireRef != NULL && (wireRef->GetFlags() & 1)) {
					BOOL value = TRUE;
					m->SetWire(value);
				}
				if (stencilRef != NULL) {
					if (stencilRef->GetFaceDrawMode() == DRAW_BOTH) {
						BOOL value = TRUE;
						m->SetTwoSided(value);
					}
				}
				if (shadeRef != NULL && shadeRef->GetFlags() & 1) {
					m->SetFaceted(TRUE);
				}
			}
		}

		if (NULL != texRef)
		{
			// Handle Base/Detail ???
			if (texRef->HasTexture(DECAL_0_MAP)){
				if (Texmap* tex = CreateTexture(texRef->GetTexture(DECAL_0_MAP)))
					m->SetSubTexmap(ID_DI, tex);
				if (texRef->HasTexture(BASE_MAP)){
					m->LockAmbDiffTex(FALSE);
					if (Texmap* tex = CreateTexture(texRef->GetTexture(BASE_MAP)))
						m->SetSubTexmap(ID_AM, tex);
				}
			} else if (texRef->HasTexture(BASE_MAP)) {
				if (Texmap* tex = CreateTexture(texRef->GetTexture(BASE_MAP))) {
					m->SetSubTexmap(ID_DI, tex);
					if (showTextures) gi->ActivateTexture(tex,m);
			 }
			} 
			// Handle Bump map
			if (texRef->HasTexture(BUMP_MAP)) {
				if (Texmap* tex = CreateTexture(texRef->GetTexture(BUMP_MAP)))
					m->SetSubTexmap(ID_BU, CreateNormalBump(NULL, tex));
			}
			// Shiny map
			if (texRef->HasTexture(GLOSS_MAP)) {
				if (Texmap* tex = CreateTexture(texRef->GetTexture(GLOSS_MAP)))
					m->SetSubTexmap(ID_SS, tex);
			}
			// Self illumination
			if (texRef->HasTexture(GLOW_MAP)) {
				if (Texmap* tex = CreateTexture(texRef->GetTexture(GLOW_MAP)))
					m->SetSubTexmap(ID_SI, tex);
			}

			// Custom Shader Handling
			int nTex = texRef->GetShaderTextureCount();
			if (nTex > 0) {
				list<NiExtraDataRef> data = avObject->GetExtraData();
				NiGeometryRef trigeom = DynamicCast<NiGeometry>(avObject);
				if (trigeom->HasShader()) {
					for (list<NiExtraDataRef>::iterator itr = data.begin(); itr != data.end(); ++itr) {
						if ( NiIntegerExtraDataRef idx = DynamicCast<NiIntegerExtraData>(*itr) ) {
							string name = idx->GetName();
							if ( wildmatch("*Index", name) ) {
								int shader = idx->GetData();
								if (shader < nTex) {
									if ( name == "NormalMapIndex" ) {
										if (Texmap* tex = CreateTexture(texRef->GetShaderTexture(shader)))
											m->SetSubTexmap(ID_BU, CreateNormalBump(NULL, tex));
									} else if ( name == "SpecularIntensity" ) {
										if (Texmap* tex = CreateTexture(texRef->GetShaderTexture(shader)))
											m->SetSubTexmap(ID_SP, CreateNormalBump(NULL, tex));
									}
								}
							}
						}
					}
				}
			}
		}
		if (NiTexturePropertyRef tex2Ref = avObject->GetPropertyByType(NiTextureProperty::TYPE)){
			// Handle Base/Detail ???
			if (Texmap* tex = CreateTexture(tex2Ref)) {
				m->SetSubTexmap(ID_DI, tex);
			} 
		}
		if (BSShaderNoLightingPropertyRef noLightShadeRef = SelectFirstObjectOfType<BSShaderNoLightingProperty>(props)) {
			if ( Texmap* tex = CreateTexture( noLightShadeRef->GetFileName() ) ) {
				m->SetSubTexmap(ID_DI, tex);
			}
		}
		if (BSShaderPPLightingPropertyRef ppLightShadeRef = SelectFirstObjectOfType<BSShaderPPLightingProperty>(props)) {
			if ( BSShaderTextureSetRef textures = ppLightShadeRef->GetTextureSet() ) {
				if ( Texmap* tex = CreateTexture( textures->GetTexture(0) ) )
					m->SetSubTexmap(ID_DI, tex);
				if ( Texmap* tex = CreateTexture( textures->GetTexture(1) ) )
					m->SetSubTexmap(ID_BU, CreateNormalBump(NULL, tex));
				if ( Texmap* tex = CreateTexture( textures->GetTexture(3) ) )
					m->SetSubTexmap(ID_SI, tex);
				if ( Texmap* tex = CreateTexture( textures->GetTexture(4) ) )
					m->SetSubTexmap(ID_RL, tex);
				if ( Texmap* tex = CreateTexture( textures->GetTexture(5) ) ) {
					if ( Texmap* mask = CreateTexture( textures->GetTexture(2) ) )
						tex = CreateMask(NULL, tex, mask);
					m->SetSubTexmap(ID_SP, tex);
				}
			}
		}
		if (SkyShaderPropertyRef skyShadeRef = SelectFirstObjectOfType<SkyShaderProperty>(props)) {
			if ( Texmap* tex = CreateTexture( skyShadeRef->GetFileName() ) ) {
				m->SetSubTexmap(ID_DI, tex);
			}
		}
		if (TileShaderPropertyRef tileShadeRef = SelectFirstObjectOfType<TileShaderProperty>(props)) {
			if ( Texmap* tex = CreateTexture( tileShadeRef->GetFileName() ) ) {
				m->SetSubTexmap(ID_DI, tex);
			}
		}
		if (TallGrassShaderPropertyRef grassShadeRef = SelectFirstObjectOfType<TallGrassShaderProperty>(props)) {
			if ( Texmap* tex = CreateTexture( grassShadeRef->GetFileName() ) ) {
				m->SetSubTexmap(ID_DI, tex);
			}
		}
		if (Lighting30ShaderPropertyRef lighting30ShadeRef = SelectFirstObjectOfType<Lighting30ShaderProperty>(props)) {
			if ( BSShaderTextureSetRef textures = lighting30ShadeRef->GetTextureSet() ) {
				if ( Texmap* tex = CreateTexture( textures->GetTexture(0) ) )
					m->SetSubTexmap(ID_DI, tex);
				if ( Texmap* tex = CreateTexture( textures->GetTexture(1) ) )
					m->SetSubTexmap(ID_BU, CreateNormalBump(NULL, tex));
				if ( Texmap* tex = CreateTexture( textures->GetTexture(3) ) )
					m->SetSubTexmap(ID_SI, tex);
				if ( Texmap* tex = CreateTexture( textures->GetTexture(4) ) )
					m->SetSubTexmap(ID_RL, tex);
				if ( Texmap* tex = CreateTexture( textures->GetTexture(5) ) ) {
					if ( Texmap* mask = CreateTexture( textures->GetTexture(2) ) )
						tex = CreateMask(NULL, tex, mask);
					m->SetSubTexmap(ID_SP, tex);
				}
			}
		}
      if (lightingShaderRef != NULL) {
         if ( BSShaderTextureSetRef textures = lightingShaderRef->GetTextureSet() ) {
            if ( Texmap* tex = CreateTexture( textures->GetTexture(0) ) )
               m->SetSubTexmap(ID_DI, tex);
            if ( Texmap* tex = CreateTexture( textures->GetTexture(1) ) )
               m->SetSubTexmap(ID_BU, CreateNormalBump(NULL, tex));
            if ( Texmap* tex = CreateTexture( textures->GetTexture(3) ) )
               m->SetSubTexmap(ID_SI, tex);
            if ( Texmap* tex = CreateTexture( textures->GetTexture(4) ) )
               m->SetSubTexmap(ID_RL, tex);
            if ( Texmap* tex = CreateTexture( textures->GetTexture(5) ) ) {
               if ( Texmap* mask = CreateTexture( textures->GetTexture(2) ) )
                  tex = CreateMask(NULL, tex, mask);
               m->SetSubTexmap(ID_SP, tex);
            }
         }
      }

		return m;
	}
	return NULL;
}


bool NifImporter::ImportMaterialAndTextures(ImpNode *node, vector<NiTriBasedGeomRef>& glist)
{
   MultiMtl *mtl = NULL;
   int isubmtl = 0;
   for (vector<NiTriBasedGeomRef>::iterator itr = glist.begin(), end = glist.end(); itr != end; ++itr, ++isubmtl) {
      NiTriBasedGeomDataRef triGeomData = StaticCast<NiTriBasedGeomData>((*itr)->GetData());
      if ( StdMat2* submtl = ImportMaterialAndTextures(node, (*itr)) )
      {
         if (mtl == NULL) 
            mtl = NewDefaultMultiMtl();
         // SubMatIDs do not have to be contigious so we just use the offset
         mtl->SetSubMtlAndName(isubmtl, submtl, submtl->GetName());
      }
   }
   if (mtl != NULL)
   {
      gi->GetMaterialLibrary().Add(mtl);
      node->GetINode()->SetMtl(mtl);
      return true;
   }
   return false;
}

bool NifImporter::ImportNiftoolsShader(ImpNode *node, NiAVObjectRef avObject, StdMat2 *mtl)
{
	if (!useNiftoolsShader || !mtl || !mtl->SupportsShaders()) 
		return false;

	const Class_ID civ4Shader(0x670a77d0,0x23ab5c7f);
	const Class_ID NIFSHADER_CLASS_ID(0x566e8ccb, 0xb091bd48);

	if ((useNiftoolsShader & 1) == 0 || !mtl->SwitchShader(NIFSHADER_CLASS_ID)) {
		if ((useNiftoolsShader & 2) == 0 || !mtl->SwitchShader(civ4Shader))
			return false;
	}

	TSTR shaderByName;
	if (Shader *s = mtl->GetShader())
		s->GetClassName(shaderByName);

	Class_ID shaderID = mtl->ClassID();
	bool isGamebryoShader = (shaderID == civ4Shader);
	bool isCiv4Shader = isGamebryoShader && (shaderByName == TSTR("CivilizationIV Shader"));

	RefTargetHandle ref = mtl->GetReference(2/*shader*/);
	if (!ref)
		return false;

	vector<NiPropertyRef> props = avObject->GetProperties();

	if (NiMaterialPropertyRef matRef = SelectFirstObjectOfType<NiMaterialProperty>(props)){
		Color ambient = TOCOLOR(matRef->GetAmbientColor());
		Color diffuse = TOCOLOR(matRef->GetDiffuseColor());
		Color specular = TOCOLOR(matRef->GetSpecularColor());
		Color emittance = TOCOLOR(matRef->GetEmissiveColor());
		float shininess = matRef->GetGlossiness();
		float alpha = matRef->GetTransparency();

		if ( IsFallout3() || IsSkyrim()) {
			ambient = diffuse = Color(0.588f, 0.588f, 0.588f);
			//specular = Color(0.902f, 0.902f, 0.902f);
		}

		mtl->SetShinStr(0.0,0);
		mtl->SetShininess(shininess/100.0,0);
		mtl->SetOpacity(alpha*100.0f,0);

		setMAXScriptValue(ref, "ambient", 0, ambient );
		setMAXScriptValue(ref, "diffuse", 0, diffuse );
		setMAXScriptValue(ref, "specular", 0, specular );
		setMAXScriptValue(ref, "emittance", 0, emittance );
		setMAXScriptValue(ref, "shininess", 0, shininess );
		setMAXScriptValue(ref, "alpha", 0, alpha );
	}
	if (NiShadePropertyRef shadeRef = SelectFirstObjectOfType<NiShadeProperty>(props)) {
		if (shadeRef->GetFlags() & 1){
			mtl->SetFaceted(TRUE);
		}
	}
	if (NiWireframePropertyRef wireRef = SelectFirstObjectOfType<NiWireframeProperty>(props)) {
		if (wireRef->GetFlags() & 1){
			mtl->SetWire(TRUE);
		}
	}
	if (NiStencilPropertyRef stencilRef = SelectFirstObjectOfType<NiStencilProperty>(props)) {
		mtl->SetTwoSided(TRUE);
	}
	bool Dither = false;
	bool SpecularEnable = false;
	if (NiDitherPropertyRef ditherRef = SelectFirstObjectOfType<NiDitherProperty>(props)) {
		Dither = (ditherRef->GetFlags() & 1) ? true : false;
	}
	if (NiSpecularPropertyRef specRef = SelectFirstObjectOfType<NiSpecularProperty>(props)) {
		SpecularEnable = (specRef->GetFlags() & 1) ? true : false;
	}
	setMAXScriptValue(ref, "Dither", 0, Dither );
	setMAXScriptValue(ref, "SpecularEnable", 0, SpecularEnable);

	if (NiVertexColorPropertyRef vertexColor = SelectFirstObjectOfType<NiVertexColorProperty>(props)) {
		int SrcVertexMode = vertexColor->GetVertexMode();
		int LightingMode = vertexColor->GetLightingMode();
		bool VertexColorsEnable = true;
		setMAXScriptValue(ref, "Vertex_Color_Enable", 0, VertexColorsEnable);
		setMAXScriptValue(ref, "VertexColorsEnable", 0, VertexColorsEnable);
		setMAXScriptValue(ref, "SourceVertexMode", 0, SrcVertexMode);
		setMAXScriptValue(ref, "SrcVertexMode", 0, SrcVertexMode);
		setMAXScriptValue(ref, "LightingMode", 0, LightingMode);
	} else {
		bool VertexColorsEnable = false;
		setMAXScriptValue(ref, "Vertex_Color_Enable", 0, VertexColorsEnable);
		setMAXScriptValue(ref, "VertexColorsEnable", 0, VertexColorsEnable);
	}
	if (NiAlphaPropertyRef alphaRef = SelectFirstObjectOfType<NiAlphaProperty>(props)) {
		int TestRef = alphaRef->GetTestThreshold();
		int srcBlend = alphaRef->GetSourceBlendFunc(); 
		int destBlend = alphaRef->GetDestBlendFunc ();
		int TestMode = alphaRef->GetTestFunc();
		bool AlphaTestEnable = alphaRef->GetTestState();     
		bool NoSorter = alphaRef->GetTriangleSortMode();
		bool alphaBlend = alphaRef->GetBlendState();
		int alphaMode = 1;

		if (!alphaBlend) {
			alphaMode = 1; // none
		} else if (srcBlend == NiAlphaProperty::BF_SRC_ALPHA && destBlend == NiAlphaProperty::BF_ONE_MINUS_SRC_ALPHA) {
			alphaMode = 0; // standard or automatic?
		} else if (srcBlend == NiAlphaProperty::BF_ONE && destBlend == NiAlphaProperty::BF_ONE) {
			alphaMode = 3;
		} else if (srcBlend == NiAlphaProperty::BF_ZERO && destBlend == NiAlphaProperty::BF_SRC_COLOR) {
			alphaMode = 4;
		} else {
			alphaMode = 5;
		}
		setMAXScriptValue(ref, "AlphaTestEnable", 0, AlphaTestEnable );
		setMAXScriptValue(ref, "alphaMode", 0, alphaMode);
		setMAXScriptValue(ref, "srcBlend", 0, srcBlend);
		setMAXScriptValue(ref, "destBlend", 0, destBlend);
		setMAXScriptValue(ref, "NoSorter", 0, NoSorter);
		setMAXScriptValue(ref, "TestRef", 0, TestRef );
		setMAXScriptValue(ref, "TestMode", 0, TestMode );
	}
	if (NiTexturingPropertyRef texRef = SelectFirstObjectOfType<NiTexturingProperty>(props)) {
		Matrix22 m2 = texRef->GetBumpMapMatrix();
		float Magnitude = (m2[0][0] + m2[1][1]) / 2.0f;
		float LumaScale = texRef->GetLumaScale();
		float LumaOffset = texRef->GetLumaOffset();
		int ApplyMode = texRef->GetApplyMode();

		setMAXScriptValue(ref, "Bump_Map_Magnitude", 0, Magnitude);
		setMAXScriptValue(ref, "Bump_Map_Luma_Scale", 0, LumaScale);
		setMAXScriptValue(ref, "Bump_Map_Luma_offset", 0, LumaOffset);
		setMAXScriptValue(ref, "ApplyMode", 0, ApplyMode);

		int ntex = mtl->NumSubTexmaps();
		if (ntex > 0)
		{
			for (int i = 0; i < ntex; ++i) {

				TexType textype = (TexType)i;
				if ( isCiv4Shader ) {
					const TexType civmap[] = {BASE_MAP, DARK_MAP, DETAIL_MAP, DECAL_0_MAP, BUMP_MAP, GLOSS_MAP, GLOW_MAP, DECAL_1_MAP, DECAL_2_MAP, DECAL_3_MAP};
					textype = civmap[i];
				} else if (isGamebryoShader) {
					const TexType civmap[] = {BASE_MAP, DARK_MAP, DETAIL_MAP, DECAL_0_MAP, NORMAL_MAP, UNKNOWN2_MAP, BUMP_MAP, GLOSS_MAP, GLOW_MAP, DECAL_1_MAP, DECAL_2_MAP, DECAL_3_MAP};
					textype = civmap[i];
				}
				if ( nifVersion <= 0x14010003) {
					if (textype > C_NORMAL)
						textype = (TexType)(i + 2);
				}
				if (texRef->HasTexture(textype)){
					if (Texmap* tex = CreateTexture(texRef->GetTexture(textype))) {
						mtl->SetSubTexmap(i, tex);
					}
				}
			}
		}
		// Custom Shader Handling
		int nTex = texRef->GetShaderTextureCount();
		if (nTex > 0) {
			NiGeometryRef trigeom = DynamicCast<NiGeometry>(avObject);
			if (trigeom->HasShader()) {
				list<NiExtraDataRef> data = avObject->GetExtraData();
				for (list<NiExtraDataRef>::iterator itr = data.begin(); itr != data.end(); ++itr) {
					if ( NiIntegerExtraDataRef idx = DynamicCast<NiIntegerExtraData>(*itr) ) {
						string name = idx->GetName();
						if ( wildmatch("*Index", name) ) {
							int shader = idx->GetData();
							if (shader < nTex) {
								if ( name == "NormalMapIndex" ) {
									if (Texmap* tex = CreateTexture(texRef->GetShaderTexture(shader)))
										mtl->SetSubTexmap(C_NORMAL, CreateNormalBump(NULL, tex));
								} else if ( name == "SpecularIntensityIndex" ) {
									if (Texmap* tex = CreateTexture(texRef->GetShaderTexture(shader)))
										mtl->SetSubTexmap(C_GLOSS, tex);
								} else if ( name == "EnvironmentMapIndex" ) {
									if (Texmap* tex = CreateTexture(texRef->GetShaderTexture(shader)))
										mtl->SetSubTexmap(C_ENVMASK, tex);
								} else if ( name == "EnvironmentIntensityIndex" ) {
									if (Texmap* tex = CreateTexture(texRef->GetShaderTexture(shader)))
										mtl->SetSubTexmap(C_ENV, tex);
								}
							}
						}
					}
				}
			}
		}
	}
	if (NiTexturePropertyRef tex2Ref = avObject->GetPropertyByType(NiTextureProperty::TYPE)){
		// Handle Base/Detail ???
		if (Texmap* tex = CreateTexture(tex2Ref)) {
			mtl->SetSubTexmap(ID_DI, tex);
		} 
	}
	if (BSShaderNoLightingPropertyRef noLightShadeRef = SelectFirstObjectOfType<BSShaderNoLightingProperty>(props)) {
		if ( Texmap* tex = CreateTexture( noLightShadeRef->GetFileName() ) ) {
			mtl->SetSubTexmap(ID_DI, tex);
		}
		setMAXScriptValue(ref, "CustomShader", 0, TSTR(noLightShadeRef->GetType().GetTypeName().c_str()));
	}
	if (BSShaderPPLightingPropertyRef ppLightShadeRef = SelectFirstObjectOfType<BSShaderPPLightingProperty>(props)) {
		if ( BSShaderTextureSetRef textures = ppLightShadeRef->GetTextureSet() ) {
			if ( Texmap* tex = CreateTexture( textures->GetTexture(0) ) )
				mtl->SetSubTexmap(C_BASE, tex);
			if ( Texmap* tex = CreateTexture( textures->GetTexture(1) ) )
				mtl->SetSubTexmap(C_NORMAL, CreateNormalBump(NULL, tex));
			if ( Texmap* tex = CreateTexture( textures->GetTexture(2) ) )
				mtl->SetSubTexmap(C_ENVMASK, tex);
			if ( Texmap* tex = CreateTexture( textures->GetTexture(3) ) )
				mtl->SetSubTexmap(C_GLOW, tex);
			if ( Texmap* tex = CreateTexture( textures->GetTexture(4) ) )
				mtl->SetSubTexmap(C_HEIGHT, tex);
			if ( Texmap* tex = CreateTexture( textures->GetTexture(5) ) )
				mtl->SetSubTexmap(C_ENV, tex);
		}
		setMAXScriptValue(ref, "CustomShader", 0, TSTR(ppLightShadeRef->GetType().GetTypeName().c_str()));
	}
	if (SkyShaderPropertyRef skyShadeRef = SelectFirstObjectOfType<SkyShaderProperty>(props)) {
		if ( Texmap* tex = CreateTexture( skyShadeRef->GetFileName() ) ) {
			mtl->SetSubTexmap(ID_DI, tex);
		}
		setMAXScriptValue(ref, "CustomShader", 0, TSTR(skyShadeRef->GetType().GetTypeName().c_str()));
	}
	if (TileShaderPropertyRef tileShadeRef = SelectFirstObjectOfType<TileShaderProperty>(props)) {
		if ( Texmap* tex = CreateTexture( tileShadeRef->GetFileName() ) ) {
			mtl->SetSubTexmap(ID_DI, tex);
		}
		setMAXScriptValue(ref, "CustomShader", 0, TSTR(tileShadeRef->GetType().GetTypeName().c_str()));
	}
	if (TallGrassShaderPropertyRef grassShadeRef = SelectFirstObjectOfType<TallGrassShaderProperty>(props)) {
		if ( Texmap* tex = CreateTexture( grassShadeRef->GetFileName() ) ) {
			mtl->SetSubTexmap(ID_DI, tex);
		}
		setMAXScriptValue(ref, "CustomShader", 0, TSTR(grassShadeRef->GetType().GetTypeName().c_str()));
	}
	if (Lighting30ShaderPropertyRef lighting30ShadeRef = SelectFirstObjectOfType<Lighting30ShaderProperty>(props)) {
		if ( BSShaderTextureSetRef textures = lighting30ShadeRef->GetTextureSet() ) {
			if ( Texmap* tex = CreateTexture( textures->GetTexture(0) ) )
				mtl->SetSubTexmap(C_BASE, tex);
			if ( Texmap* tex = CreateTexture( textures->GetTexture(1) ) )
				mtl->SetSubTexmap(C_NORMAL, CreateNormalBump(NULL, tex));
			if ( Texmap* tex = CreateTexture( textures->GetTexture(2) ) )
				mtl->SetSubTexmap(C_ENVMASK, tex);
			if ( Texmap* tex = CreateTexture( textures->GetTexture(3) ) )
				mtl->SetSubTexmap(C_GLOW, tex);
			if ( Texmap* tex = CreateTexture( textures->GetTexture(4) ) )
				mtl->SetSubTexmap(C_HEIGHT, tex);
			if ( Texmap* tex = CreateTexture( textures->GetTexture(5) ) )
				mtl->SetSubTexmap(C_ENV, tex);
		}
		setMAXScriptValue(ref, "CustomShader", 0, TSTR(lighting30ShadeRef->GetType().GetTypeName().c_str()));
	}

   if ( BSLightingShaderPropertyRef lightingShaderRef = SelectFirstObjectOfType<BSLightingShaderProperty>(props) )
   {
      // Material like properties
      {
         //Color ambient = TOCOLOR(lightingShaderRef->GetAmbientColor());
         //Color diffuse = TOCOLOR(lightingShaderRef->GetDiffuseColor());
         Color specular = TOCOLOR(lightingShaderRef->GetSpecularColor());
         Color emittance = TOCOLOR(lightingShaderRef->GetEmissiveColor());
         float shininess = lightingShaderRef->GetGlossiness();
         float alpha = lightingShaderRef->GetAlpha();

         Color ambient = Color(0.588f, 0.588f, 0.588f);
         Color diffuse = Color(0.588f, 0.588f, 0.588f);

         mtl->SetShinStr(0.0,0);
         mtl->SetShininess(shininess/100.0,0);
         mtl->SetOpacity(alpha*100.0f,0);

         setMAXScriptValue(ref, "ambient", 0, ambient );
         setMAXScriptValue(ref, "diffuse", 0, diffuse );
         setMAXScriptValue(ref, "specular", 0, specular );
         setMAXScriptValue(ref, "emittance", 0, emittance );
         setMAXScriptValue(ref, "shininess", 0, shininess );
         setMAXScriptValue(ref, "alpha", 0, alpha );
      }
      // Texture Set
      {
         if ( BSShaderTextureSetRef textures = lightingShaderRef->GetTextureSet() ) {
            if ( Texmap* tex = CreateTexture( textures->GetTexture(0) ) )
               mtl->SetSubTexmap(C_BASE, tex);
            if ( Texmap* tex = CreateTexture( textures->GetTexture(1) ) )
               mtl->SetSubTexmap(C_NORMAL, CreateNormalBump(NULL, tex));
            if ( Texmap* tex = CreateTexture( textures->GetTexture(2) ) ) // Glow/Skin/Hair
               mtl->SetSubTexmap(C_GLOW, tex);
            if ( Texmap* tex = CreateTexture( textures->GetTexture(3) ) ) // Height/Parallax
               mtl->SetSubTexmap(C_HEIGHT, tex);
            if ( Texmap* tex = CreateTexture( textures->GetTexture(4) ) ) // Environment
               mtl->SetSubTexmap(C_ENV, tex);
            if ( Texmap* tex = CreateTexture( textures->GetTexture(5) ) ) // Environment Mask
               mtl->SetSubTexmap(C_ENVMASK, tex);
            if ( Texmap* tex = CreateTexture( textures->GetTexture(7) ) ) //specular
               mtl->SetSubTexmap(C_SPECULAR, tex);
            if ( Texmap* tex = CreateTexture( textures->GetTexture(8) ) ) //Parallax
               mtl->SetSubTexmap(C_PARALLAX, tex);
         }
      }

      if (NiStencilPropertyRef stencilRef = SelectFirstObjectOfType<NiStencilProperty>(props)) {
         
      }
      int flags2 = lightingShaderRef->GetShaderFlags2();
      mtl->SetTwoSided( (flags2 & SLSF2_DOUBLE_SIDED) ? TRUE : FALSE );

      extern const EnumLookupType BSShaderTypes[];
      TSTR shaderType = EnumToString(lightingShaderRef->GetSkyrimShaderType()+100, BSShaderTypes);
      setMAXScriptValue(ref, "CustomShader", 0, shaderType);
   }


	return true;
}


string NifImporter::FindImage(const string& name)
{
   TCHAR buffer[MAX_PATH];

   // Simply check for fully qualified path
   if (!PathIsRelative(name.c_str())) {
      if (-1 != _taccess(name.c_str(), 0))
         return string(buffer);
   } 
   if (!path.empty()) {
      PathCombine(buffer, path.c_str(), name.c_str()); // try as-is
      if (-1 != _taccess(buffer, 0))
         return string(buffer);

      // try only filename in nif directory
      PathCombine(buffer, path.c_str(), PathFindFileName(name.c_str()));
      if (-1 != _taccess(buffer, 0))
         return string(buffer);
   }
   if (appSettings != NULL) {
      return appSettings->FindImage(name);
   }
   return name;
}


