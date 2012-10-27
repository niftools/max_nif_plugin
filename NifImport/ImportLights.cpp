/**********************************************************************
*<
FILE: ImportSkeleton.cpp

DESCRIPTION:	Skeleton import routines

CREATED BY: tazpn (Theo)

HISTORY: 

*>	Copyright (c) 2006, All Rights Reserved.
**********************************************************************/
#include "stdafx.h"
#include "MaxNifImport.h"
#include <obj/NiLight.h>
#include <obj/NiAmbientLight.h>
#include <obj/NiPointLight.h>
#include <obj/NiDirectionalLight.h>
#include <obj/NiSpotLight.h>
#include <float.h>
#include <dummy.h>

using namespace Niflib;

// Targeted Lights
// Normal Lights
static GenLight *CreateLight(Interface *gi, const NiLightRef& light, int lightType)
{
   if (GenLight *ob = gi->CreateLightObject(lightType)) {
      float dimmer = light->GetDimmer();
      Point3 ambient = TOPOINT3(light->GetAmbientColor());
      Point3 diffuse = TOPOINT3(light->GetDiffuseColor());
      Point3 specular = TOPOINT3(light->GetSpecularColor());
      Point3 black(0,0,0);

      ob->Enable(1);
      ob->SetUseLight(1);
      ob->SetIntensity(0, dimmer);
      if (diffuse != black) {
         ob->SetAffectDiffuse(TRUE);
         ob->SetRGBColor(0, diffuse);
      } else {
         ob->SetAffectDiffuse(FALSE);
      }
      if (specular != black) {
         ob->SetAffectSpecular(TRUE);
         ob->SetRGBColor(0, specular);
      } else {
         ob->SetAffectSpecular(FALSE);
      }
      return ob;
   }
   return NULL;
}

bool NifImporter::ImportLights(NiNodeRef node)
{
   bool ok = false;
   vector<NiAVObjectRef> children = node->GetChildren();
   ok |= ImportLights(DynamicCast<NiLight>(children));
   vector<NiNodeRef> childNodes = DynamicCast<NiNode>(children);
   for (vector<NiNodeRef>::iterator itr=childNodes.begin(), end=childNodes.end(); itr != end; ++itr)
      ok |= ImportLights(*itr);
   return ok;
}

bool NifImporter::ImportLights(vector<NiLightRef> lights)
{
   bool ok = false;
   for (vector<NiLightRef>::iterator itr=lights.begin(), end=lights.end(); itr != end; ++itr)
   {
      GenLight *ob = NULL;
      NiLightRef& light = (*itr);

      if (light->IsSameType(NiPointLight::TYPE)){
         ob = CreateLight(gi, light, OMNI_LIGHT);
         NiPointLightRef ptLight = light;
         float c = ptLight->GetConstantAttenuation();
         float l = ptLight->GetLinearAttenuation();
         float q = ptLight->GetQuadraticAttenuation();
         if (c != 0.0f){
            ob->SetDecayType(0);
         } else if (l != 0.0f) {
            ob->SetDecayType(1);
            ob->SetUseAtten(1);
            ob->SetAtten(0, ATTEN_START, 4.0f/l);
         } else if (q != 0.0f) {
            ob->SetDecayType(2);
            ob->SetAtten(0, ATTEN_START, 4.0f/(l*l));
         }
      } else if (light->IsSameType(NiDirectionalLight::TYPE)){
         ob = CreateLight(gi, light, DIR_LIGHT);
      } else if (light->IsSameType(NiAmbientLight::TYPE)){
         ob = CreateLight(gi, light, OMNI_LIGHT);
         ob->SetAmbientOnly(TRUE);
      } else if (light->IsSameType(NiSpotLight::TYPE)){
         ob = CreateLight(gi, light, FSPOT_LIGHT);
      }
	  if (INode *n = CreateImportNode(light->GetName().c_str(), ob, NULL)) {
         string name = light->GetName();
         if (!name.empty()) {
            n->SetName(const_cast<TCHAR*>(name.c_str()));
         }
         Matrix44 m4 = light->GetWorldTransform();
         Vector3 pos; Matrix33 rot; float scale;
         m4.Decompose(pos, rot, scale);
         Point3 p = TOPOINT3(pos);
         Quat q = TOQUAT(rot.AsQuaternion());

         PosRotScaleNode(n, p, q, scale, prsDefault);
         n->Hide(light->GetVisibility() ? FALSE : TRUE );

		 RegisterNode(light, n);
      }
      ok = true;
   }
   return ok;
}
