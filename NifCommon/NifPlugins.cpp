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

bool npGetProp(INode *node, const TSTR &prop, int &value, int def)
{
	bool ret;
	if (node)
	{
		if (!node->GetUserPropInt(prop, value))
		{
			value = def;
			ret = false;
		} else
			ret = true;
	} else
	{
		value = def;
		ret = false;
	}

	return ret;
}

void npSetProp(INode *node, const TSTR &prop, int value)
{
	if (node)
		node->SetUserPropInt(prop, value);
}

bool npGetProp(INode *node, const TSTR &prop, float &value, float def)
{
	bool ret;
	if (node)
	{
		if (!node->GetUserPropFloat(prop, value))
		{
			value = def;
			ret = false;
		} else
			ret = true;
	} else
	{
		value = def;
		ret = false;
	}

	return ret;
}

void npSetProp(INode *node, const TSTR &prop, float value)
{
	if (node)
		node->SetUserPropFloat(prop, value);
}

bool npGetProp(INode *node, const TSTR &prop, Vector3 &value, const Vector3 def)
{
	bool ret = false;
	if (node)
	{
		TSTR tmp;
		if (node->GetUserPropString(prop, tmp))
		{
			value = def;
			TCHAR *endp = NULL;
			value.x = _tcstod(tmp, &endp);
			if (endp)
				value.y = _tcstod(endp, &endp);
			if (endp)
				value.z = _tcstod(endp, &endp);

			ret = true;
		} else
		{
			value = def;
			ret = false;
		}

	} else
	{
		value = def;
		ret = false;
	}

	return ret;

}

void npSetProp(INode *node, const TSTR &prop, const Vector3 &value)
{
	if (node)
	{
		// TSTR's printf function seems to be locale-aware (prints ',')
		// so we have to do it this way...
		TCHAR tmp[256];
		_stprintf(tmp, "%f %f %f", value.x, value.y, value.z);
		node->SetUserPropString(prop, tmp);
	}
}


bool	npGetProp(INode *node, const TSTR &prop, TSTR &value, TSTR def)
{
   bool ret;
   if (node)
   {
      if (!node->GetUserPropString(prop, value))
      {
         value = def;
         ret = false;
      } 
      else
         ret = true;
   } 
   else
   {
      value = def;
      ret = false;
   }
   return ret;
}

void	npSetProp(INode *node, const TSTR &prop, const TSTR& value)
{
   if (node)
      node->SetUserPropString(prop, value);
}