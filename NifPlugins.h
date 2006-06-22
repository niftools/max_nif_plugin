#ifndef __NIFPLUGINS_H__
#define __NIFPLUGINS_H__

#define NP_PROP_HASCOLL			"np_coll"
#define NP_PROP_HVK_LAYER		"np_hvk_layer"
#define NP_PROP_HVK_MATERIAL	"np_hvk_mtl"

#define NP_DEFAULT_HVK_MATERIAL	9
#define NP_DEFAULT_HVK_LAYER	1

bool	isCollision(INode *node);
void	setCollision(INode *node, bool coll);

bool	getHvkLayer(INode *node, int &layer);
void	setHvkLayer(INode *node, int layer);

bool	getHvkMaterial(INode *node, int &mtl);
void	setHvkMaterial(INode *node, int mtl);

#endif //  __NIFPLUGINS_H__
