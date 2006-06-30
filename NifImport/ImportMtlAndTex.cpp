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
#include "objectParams.h"
using namespace Niflib;

Texmap* NifImporter::CreateTexture(TexDesc& desc)
{
   BitmapManager *bmpMgr = TheManager;
   if (NiSourceTextureRef texSrc = desc.source){
      string filename = texSrc->GetExternalFileName();
      if (bmpMgr->CanImport(filename.c_str())){
         BitmapTex *bmpTex = NewDefaultBitmapTex();
         bmpTex->SetName(texSrc->GetName().c_str());
         bmpTex->SetMapName(const_cast<TCHAR*>(FindImage(filename).c_str()));
         bmpTex->SetAlphaAsMono(TRUE);
         bmpTex->SetAlphaSource(ALPHA_NONE);
         return bmpTex;
      }
   }
   return NULL;
}

bool NifImporter::ImportMaterialAndTextures(ImpNode *node, NiAVObjectRef avObject)
{
   // Texture
   vector<NiPropertyRef> props = avObject->GetProperties();
   NiTexturingPropertyRef texRef = SelectFirstObjectOfType<NiTexturingProperty>(props);
   NiMaterialPropertyRef matRef = SelectFirstObjectOfType<NiMaterialProperty>(props);
   NiWireframePropertyRef wireRef = SelectFirstObjectOfType<NiWireframeProperty>(props);
   NiAlphaPropertyRef alphaRef = SelectFirstObjectOfType<NiAlphaProperty>(props);
   NiStencilPropertyRef stencilRef = SelectFirstObjectOfType<NiStencilProperty>(props);

   bool hasTexture = (texRef && matRef);
   if (matRef != NULL){
      StdMat2 *m = NewDefaultStdMat();
      m->SetName(matRef->GetName().c_str());
      if (showTextures)
         m->SetMtlFlag(MTL_DISPLAY_ENABLE_FLAGS, TRUE);
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

      bool hasShaderAttributes = (wireRef != NULL) || (stencilRef != NULL);
      if (m->SupportsShaders() && hasShaderAttributes) {
         if (Shader *s = m->GetShader()) {
            if (wireRef != NULL && (wireRef->GetFlags() & 1)) {
               BOOL value = TRUE;
               setMAXScriptValue(s->GetReference(0), "wire", 0, value);
            }
            if (stencilRef != NULL) {

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
            if (Texmap* tex = CreateTexture(texRef->GetTexture(BASE_MAP)))
               m->SetSubTexmap(ID_DI, tex);
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
      gi->GetMaterialLibrary().Add(m);
      node->GetINode()->SetMtl(m);
   }
   return hasTexture;
}

string NifImporter::FindImage(const string& name)
{
   TCHAR buffer[MAX_PATH];

   // Simply check for fully qualified path
   if (PathIsRoot(name.c_str())) {
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