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
#include "MaxNifImport.h"
#include "obj/NiWireframeProperty.h"
#include "obj/NiAlphaProperty.h"
#include "obj/NiStencilProperty.h"
#include "obj/NiShadeProperty.h"
#include "obj/NiVertexColorProperty.h"
#include "obj/NiDitherProperty.h"
#include "obj/NiSpecularProperty.h"
#include "obj/NiTextureProperty.h"
#include "obj/NiImage.h"
#include "objectParams.h"
using namespace Niflib;

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

StdMat2 *NifImporter::ImportMaterialAndTextures(ImpNode *node, NiAVObjectRef avObject)
{
   // Texture
   NiMaterialPropertyRef matRef = avObject->GetPropertyByType(NiMaterialProperty::TYPE);
   if (matRef != NULL){
      NiTexturingPropertyRef texRef = avObject->GetPropertyByType(NiTexturingProperty::TYPE);
	  NiTexturePropertyRef tex2Ref = avObject->GetPropertyByType(NiTextureProperty::TYPE);
      NiWireframePropertyRef wireRef = avObject->GetPropertyByType(NiWireframeProperty::TYPE);
      NiAlphaPropertyRef alphaRef = avObject->GetPropertyByType(NiAlphaProperty::TYPE);
      NiStencilPropertyRef stencilRef = avObject->GetPropertyByType(NiStencilProperty::TYPE);
      NiShadePropertyRef shadeRef = avObject->GetPropertyByType(NiShadeProperty::TYPE);

      StdMat2 *m = NewDefaultStdMat();
      m->SetName(matRef->GetName().c_str());
	  if (showTextures) {
         m->SetMtlFlag(MTL_DISPLAY_ENABLE_FLAGS, TRUE);
	  }

      // try the civ4 shader first then default back to normal shaders
      if (ImportCiv4Shader(node, avObject, m)) {
         return m;
		}
      m->SetAmbient(TOCOLOR(matRef->GetAmbientColor()),0);
      m->SetDiffuse(TOCOLOR(matRef->GetDiffuseColor()),0);
      m->SetSpecular(TOCOLOR(matRef->GetSpecularColor()),0);
      Color c = TOCOLOR(matRef->GetEmissiveColor());
      if (c.r != 0 || c.b != 0 || c.g != 0) {
         m->SetSelfIllumColorOn(TRUE);
         m->SetSelfIllumColor(c,0);
      }
      m->SetShinStr(0.0,0);
      m->SetShininess(matRef->GetGlossiness()/100.0,0);
      m->SetOpacity(matRef->GetTransparency(),0);

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
               m->SetSubTexmap(ID_BU, tex);
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
      }
	  if (NULL != tex2Ref)
	  {
		  // Handle Base/Detail ???
		  if (Texmap* tex = CreateTexture(tex2Ref)) {
			  m->SetSubTexmap(ID_DI, tex);
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

bool NifImporter::ImportCiv4Shader(ImpNode *node, NiAVObjectRef avObject, StdMat2 *mtl)
{
   if (!useCiv4Shader || !mtl || !mtl->SupportsShaders()) 
      return false;

   Class_ID civ4Shader(0x670a77d0,0x23ab5c7f);
   if (!mtl->SwitchShader(civ4Shader))
      return false;

   TSTR shaderByName;
   if (Shader *s = mtl->GetShader())
      s->GetClassName(shaderByName);
   if (shaderByName != TSTR("CivilizationIV Shader"))
      return false;

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
      float alpha = matRef->GetTransparency() * 100.0f;

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
      setMAXScriptValue(ref, "SourceVertexMode", 0, SrcVertexMode);
      setMAXScriptValue(ref, "LightingMode", 0, LightingMode);
   } else {
      bool VertexColorsEnable = false;
      setMAXScriptValue(ref, "Vertex_Color_Enable", 0, VertexColorsEnable);
   }
   if (NiAlphaPropertyRef alphaRef = SelectFirstObjectOfType<NiAlphaProperty>(props)) {
      int TestRef = alphaRef->GetTestThreshold();
      int srcBlend = alphaRef->GetSourceBlendFunc(); 
      int destBlend = alphaRef->GetSourceBlendFunc ();
      int TestMode = alphaRef->GetTestFunc();
      bool AlphaTestEnable = alphaRef->GetTestState();     
      bool NoSorter = !alphaRef->GetTriangleSortMode();
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
         ntex = min(ntex, 7);
         TexType texmap[] = {BASE_MAP, DARK_MAP, DETAIL_MAP, DECAL_0_MAP, BUMP_MAP, GLOSS_MAP, GLOW_MAP, DECAL_1_MAP};
         NiTexturingPropertyRef texProp = CreateNiObject<NiTexturingProperty>();       
         texProp->SetApplyMode(Niflib::ApplyMode(ApplyMode));
         texProp->SetTextureCount(7);
         for (int i = 0; i < ntex; ++i) {
            TexType textype = texmap[i];
            if (texRef->HasTexture(textype)){
               if (Texmap* tex = CreateTexture(texRef->GetTexture(textype))) {
                  mtl->SetSubTexmap(i, tex);
               }
            }
         }
      }
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


