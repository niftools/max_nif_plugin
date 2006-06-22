#include "pch.h"
#include "stdmat.h"

void Exporter::makeTexture(NiAVObjectRef &parent, Mtl *mtl)
{
	if (!mtl)
		return;

	string mtlName = mtl->GetName();

	int texMaps = mtl->NumSubTexmaps();
	if (!texMaps)
		return;

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

	if (!bmTex)
		return;

	TSTR mapPath, p, f;
	mapPath = bmTex->GetMapName();
	SplitPathFile(mapPath, &p, &f);

	NiTexturingPropertyRef texProp(DynamicCast<NiTexturingProperty>(CreateBlock("NiTexturingProperty")));
	texProp->SetApplyMode(APPLY_MODULATE);
	texProp->SetTextureCount(7);

	TexDesc td;
	td.source = DynamicCast<NiSourceTexture>(CreateBlock("NiSourceTexture"));
	NiObjectRef unk_link(NULL);
	TSTR newPath;
	if (mTexPrefix != "")
		newPath = TSTR(mTexPrefix.c_str()) + _T("\\") + f;
	else
		newPath = f;
	
	char tmp[MAX_PATH]="";
	strncpy(tmp, (char*)newPath, newPath.Length());

	td.source->SetExternalTexture(tmp, unk_link);
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

		c = mtl->GetSelfIllumColor();
		mtlProp->SetEmissiveColor(Color3(c.r, c.g, c.b));

		mtlProp->SetTransparency(1);
		mtlProp->SetGlossiness(mtl->GetShininess());
		name = (char*)mtl->GetName();

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
