#include "pch.h"
#include "stdmat.h"
#include "AppSettings.h"

void Exporter::makeTexture(NiAVObjectRef &parent, Mtl *mtl)
{
	BitmapTex *bmTex = getTexture(mtl);

	if (!bmTex)
		return;

	NiTexturingPropertyRef texProp(DynamicCast<NiTexturingProperty>(CreateBlock("NiTexturingProperty")));
	texProp->SetApplyMode(APPLY_MODULATE);
	texProp->SetTextureCount(7);

	TexDesc td;
	td.source = DynamicCast<NiSourceTexture>(CreateBlock("NiSourceTexture"));

   // Get file name and check if it matches the "app" settings in the ini file
   TSTR mapPath;
   mapPath = bmTex->GetMapName();

   if (mAppSettings)
   {
      string newPath = mAppSettings->GetRelativeTexPath(string(mapPath), mTexPrefix);

      NiObjectRef unk_link(NULL);
      td.source->SetExternalTexture(newPath, unk_link);
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
   	
      NiObjectRef unk_link(NULL);
	   td.source->SetExternalTexture(newPath.data(), unk_link);
   }
	texProp->SetTexture(BASE_MAP, td);

	NiPropertyRef prop = DynamicCast<NiProperty>(texProp);
	parent->AddProperty(prop);
}

void Exporter::makeMaterial(NiAVObjectRef &parent, Mtl *mtl)
{
	string name;
	NiMaterialPropertyRef mtlProp(DynamicCast<NiMaterialProperty>(CreateBlock("NiMaterialProperty")));
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
		mtlProp->SetGlossiness(mtl->GetShininess() * 100.0);
		name = (char*)mtl->GetName();

      if(mtl->ClassID() == Class_ID(DMTL_CLASS_ID, 0) )
      {
         StdMat2 * smtl = (StdMat2*)mtl;
         mtlProp->SetTransparency(smtl->GetOpacity(0));
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

void Exporter::getTextureMatrix(Matrix3 &mat, Mtl *mtl)
{
	BitmapTex *tex = getTexture(mtl);
	if (tex)
		tex->GetUVTransform(mat);
	else
		mat.IdentityMatrix();
}
