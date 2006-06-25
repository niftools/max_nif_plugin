#include "Max.h"
#include "NifPlugins.h"

bool npIsCollision(INode *node)
{
	int i;
	return npGetProp(node, NP_HASCOLL, i) && i;
}

void npSetCollision(INode *node, bool coll)
{
	npSetProp(node, NP_HASCOLL, (bool)coll);
}

bool npGetProp(INode *node, const TSTR &prop, int &value)
{
	return node && node->GetUserPropInt(prop, value);
}

void npSetProp(INode *node, const TSTR &prop, int value)
{
	if (node)
		node->SetUserPropInt(prop, value);
}

bool npGetProp(INode *node, const TSTR &prop, float &value)
{
	return node && node->GetUserPropFloat(prop, value);
}

void npSetProp(INode *node, const TSTR &prop, float value)
{
	if (node)
		node->SetUserPropFloat(prop, value);
}


