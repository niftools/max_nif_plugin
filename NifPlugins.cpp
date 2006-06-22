#include "Max.h"
#include "NifPlugins.h"

bool isCollision(INode *node)
{
	BOOL b;
	return node && node->GetUserPropBool(_T(NP_PROP_HASCOLL), b) && b;
}

void setCollision(INode *node, bool coll)
{
	if (node)
		node->SetUserPropBool(_T(NP_PROP_HASCOLL), coll);
}

bool getHvkLayer(INode *node, int &layer)
{
	return node && node->GetUserPropInt(_T(NP_PROP_HVK_LAYER), layer);
}

void setHvkLayer(INode *node, int layer)
{
	if (node)
		node->SetUserPropInt(_T(NP_PROP_HVK_LAYER), layer);
}

bool getHvkMaterial(INode *node, int &mtl)
{
	return node && node->GetUserPropInt(_T(NP_PROP_HVK_MATERIAL), mtl);
}

void setHvkMaterial(INode *node, int mtl)
{
	if (node)
		node->SetUserPropInt(_T(NP_PROP_HVK_MATERIAL), mtl);
}
